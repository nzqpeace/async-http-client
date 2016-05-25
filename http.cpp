//
// Created by 牛志强 on 16/5/16.
//

#include "http.h"
#include <sstream>

namespace zq{
    namespace http{

        Request::Request(const std::string& url, HTTP_CALLBACK cb/* = NULL */)
        : proto_major_(1), proto_minor_(1), cb_(cb)
        {
            url_.parse(url.c_str());
            if(!url_.get_host().empty()){
                std::ostringstream oss;
                oss << url_.get_host();
                if (url_.get_port() != 0){
                    oss << ":" << url_.get_port();
                }
                headers_["Host"] = oss.str();
            }
        }

        Response::Response()
        : success_(false), code_(0), status_(""), body_(""), url_(""), last_error_("")
        {

        }
        void Response::reset() {
            code_ = 0;
            status_ = "";
            body_ = "";
            headers_.clear();
            success_ = false;
        }

        Client::Client(HTTP_CALLBACK cb /* = NULL*/, uint64_t timeout /* = 5*1000*/)
                : zq::tcp::AsyncClient(), cb_(cb), start_parsing_(false), header_parsed_(false),
                  last_is_header_value(false), parse_completed_(false), chunk_mode_(false),
                  last_header_field_(""), last_header_value_(""), timeout_(timeout)

        {
            init();
        }

        Client::~Client() {
            delete resp_;
        }

        void Client::init() {
            zq::tcp::AsyncClient::reset(this);

            resp_ = new Response();

            loop_ = (uv_loop_t*)malloc(sizeof(uv_loop_t));
            uv_loop_init(loop_);

            set_uv_loop(loop_);
            set_timeout(); // default 5s

            // init http_parser
            http_parser_init(&parser_, HTTP_BOTH);
            parser_.data = reinterpret_cast<void*>(this);

            // init http_parser_settings
            http_parser_settings_init(&parser_settings_);

            parser_settings_.on_message_begin    = HTTPParserCallBack::on_message_begin;
            parser_settings_.on_url              = HTTPParserCallBack::on_url;
            parser_settings_.on_status           = HTTPParserCallBack::on_status;
            parser_settings_.on_header_field     = HTTPParserCallBack::on_header_field;
            parser_settings_.on_header_value     = HTTPParserCallBack::on_header_value;
            parser_settings_.on_headers_complete = HTTPParserCallBack::on_headers_complete;
            parser_settings_.on_body             = HTTPParserCallBack::on_body;
            parser_settings_.on_message_complete = HTTPParserCallBack::on_message_complete;
            parser_settings_.on_chunk_header     = HTTPParserCallBack::on_chunk_header;
            parser_settings_.on_chunk_complete   = HTTPParserCallBack::on_chunk_complete;
        }

        void Client::reset() {
            http_parser_init(&parser_, HTTP_BOTH);
            parser_.data = reinterpret_cast<void*>(this);

            start_parsing_ = false;
            header_parsed_ = false;
            parse_completed_ = false;
            chunk_mode_ = false;

            last_is_header_value = false;
            last_header_field_.clear();
            last_header_value_.clear();

            resp_->reset();
            req_ = NULL;
        }

        bool Client::request(http_method method, const std::string &url,
                             const std::string &body /* = "" */, HTTP_CALLBACK cb /* = NULL */)
        {
            reset();
            if (cb != NULL){
                cb_ = cb;
            }

            if (cb_ == NULL){
                return false; // not specified callback
            }

            req_ = new Request(url);
            req_->set_method(method);
            req_->set_body(body);

            if (!startRequest(req_)){
                return false;
            }

            return true;
        }

        bool Client::get(const std::string &url, HTTP_CALLBACK cb /* = NULL */){
            return request(HTTP_GET, url, "", cb);
        }

        bool Client::post(const std::string& url, const std::string& body, HTTP_CALLBACK cb /* = NULL */){
            return request(HTTP_POST, url, body, cb);
        }

        bool Client::startRequest(Request *req) {
            if (!resolve(req->url_.get_host(), req->url_.get_port())){
                return false;
            }
            int ret = uv_run(loop_, UV_RUN_DEFAULT);
            return true;
        }

        std::string Client::raw(Request* req)
        {
            if (req == NULL){
                return "";
            }

            std::ostringstream oss;
            oss << req->get_method() << " ";

            oss << req->url_.get_path();
            if(!req->url_.get_query().empty()){
                oss << "?";
                oss << req_->url_.get_query();
            }
            if (!req_->url_.get_fragment().empty()){
                oss << "#";
                oss << req_->url_.get_fragment();
            }
            oss << " HTTP/"
                << req->get_proto_major() << "." << req->get_proto_minor() << "\r\n";
            for(auto &kv : req->headers()){
                oss << kv.first << ":" << kv.second << "\r\n";
            }

            oss << "\r\n";
            oss << req->get_body();

            return oss.str();
        }

        // socket callback
        void Client::onResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res){
            if(0 != status){
                std::ostringstream oss;
                oss << "resolve failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }

            char addr[17] = {'\0'};
            uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
            std::string ip(addr);
            connect(ip, req_->url_.get_port());
        }

        void Client::onConnect(uv_connect_t* req, int status){
            if(0 != status){
                std::ostringstream oss;
                oss << "connect failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }

            std::string data = raw(req_);
            if (!data.empty()){
                startWrite(data.c_str(), data.size());
            }else{
                resp_->success_ = false;
                cb_(resp_);
            }
        }

        void Client::onReadDone(uv_stream_t *handle, ssize_t nread, const char *buf){
            if(nread < 0){
                std::ostringstream oss;
                oss << "read failed, error[";
                oss << uv_err_name(nread) << "] reason[" << uv_strerror(nread) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }

            size_t nparsed = http_parser_execute(&parser_, &parser_settings_, buf, nread);
            if (nparsed != nread){
                std::ostringstream oss;
                oss << "parse failed, error[";
                oss << http_errno_name((enum http_errno)parser_.http_errno) << "] reason[" << http_errno_description((enum http_errno)parser_.http_errno) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }

            if (!chunk_mode_ ||
                (chunk_mode_ && http_body_is_final(&parser_) != 0)){
                uv_read_stop(handle);
            }
        }

        void Client::onWriteDone(uv_write_t *req, int status){
            if(0 != status){
                std::ostringstream oss;
                oss << "write failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }
            startRead();
        }

        void Client::onTimer(uv_timer_t *handle, int status){
            if(0 != status){
                std::ostringstream oss;
                oss << "timeout, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                cb_(resp_);
                return;
            }
        }

        // http parser callback
        int Client::on_message_begin(http_parser* parser)
        {
            start_parsing_ = true;
            return 0;
        }
        int Client::on_url(http_parser* parser, const char *at, size_t length)
        {
            resp_->url_.append(at, length);
            return 0;
        }
        int Client::on_status(http_parser* parser, const char *at, size_t length)
        {
            resp_->status_.append(at, length);
            resp_->code_ = parser->status_code;
            return 0;
        }
        int Client::on_header_field(http_parser* parser, const char *at, size_t length)
        {
            if (last_is_header_value){
                last_header_field_.clear();
                last_header_value_.clear();
            }

            last_header_field_.append(at, length);
            return 0;
        }

        int Client::on_header_value(http_parser* parser, const char *at, size_t length)
        {
            last_header_value_.append(at, length);
            resp_->headers_[last_header_field_] = last_header_value_;
            last_is_header_value = true;
            return 0;
        }
        int Client::on_headers_complete(http_parser* parser)
        {
            header_parsed_ = true;
            if (parser->content_length == 0){
                return 1; // no body need parsed
            }
            return 0;
        }
        int Client::on_body(http_parser* parser, const char *at, size_t length)
        {
            resp_->body_.append(at, length);
            return 0;
        }
        int Client::on_message_complete(http_parser* parser)
        {
            parse_completed_ = true;
            resp_->success_ = true;
            cb_(resp_);
            resp_->reset();
            return 0;
        }
        int Client::on_chunk_header(http_parser* parser)
        {
            chunk_mode_ = true;
            return 0;
        }
        int Client::on_chunk_complete(http_parser* parser)
        {
            parse_completed_ = true;
            return 0;
        }


        int HTTPParserCallBack::on_message_begin(http_parser* parser)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_message_begin(parser);
        }
        int HTTPParserCallBack::on_url(http_parser* parser, const char *at, size_t length)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_url(parser, at, length);
        }
        int HTTPParserCallBack::on_status(http_parser* parser, const char *at, size_t length)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_status(parser, at, length);
        }
        int HTTPParserCallBack::on_header_field(http_parser* parser, const char *at, size_t length)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_header_field(parser, at, length);
        }
        int HTTPParserCallBack::on_header_value(http_parser* parser, const char *at, size_t length)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_header_value(parser, at, length);
        }
        int HTTPParserCallBack::on_headers_complete(http_parser* parser)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_headers_complete(parser);
        }
        int HTTPParserCallBack::on_body(http_parser* parser, const char *at, size_t length)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_body(parser, at, length);
        }
        int HTTPParserCallBack::on_message_complete(http_parser* parser)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_message_complete(parser);
        }
        int HTTPParserCallBack::on_chunk_header(http_parser* parser)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_chunk_header(parser);
        }
        int HTTPParserCallBack::on_chunk_complete(http_parser* parser)
        {
            Client* httpclient = reinterpret_cast<Client*>(parser->data);
            return httpclient->on_chunk_complete(parser);
        }
    }
}