//
// Created by 牛志强 on 16/5/25.
//

#include "request.h"

#include <sstream>

namespace zq{
    namespace http{

        uint64_t Request::auto_id_ = 0;
        Request::Request(zq::loop* loop, http_method method, const std::string& url, const std::string& body, HTTP_CALLBACK cb/* = NULL */)
                : uv_loop_(loop->get_uv_loop()), method_(method), body_(body), proto_major_(1), proto_minor_(1), cb_(cb)
        {
            url_.parse(url.c_str());
            if(!url_.get_host().empty()){
                std::ostringstream oss;
                oss << url_.get_host();
                if (url_.get_port() != 0 && url_.get_port() != 80){
                    oss << ":" << url_.get_port();
                }
                headers_["Host"] = oss.str();
            }

            auto_id_++;
            id_ = auto_id_;

            init();
        }

        Request::~Request() {
            free(async_handle_);
            delete resp_;
        }

        void Request::init() {
            zq::tcp::AsyncClient::reset(this);

            resp_ = new Response();

            set_uv_loop(uv_loop_);
            set_timeout(); // default 5s

            // init async handle
            async_handle_ = (uv_async_t*)malloc(sizeof(uv_async_t));
            uv_async_init(uv_loop_, async_handle_, AsyncEventCallBack::async_cb);
            async_handle_->data = reinterpret_cast<void*>(this);

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

        std::string Request::raw() {
            std::ostringstream oss;
            oss << get_method() << " ";

            oss << url_.get_path();
            if(!url_.get_query().empty()){
                oss << "?";
                oss << url_.get_query();
            }
            if (!url_.get_fragment().empty()){
                oss << "#";
                oss << url_.get_fragment();
            }
            oss << " HTTP/"
            << proto_major_ << "." << proto_minor_ << "\r\n";

            Header::iterator it = headers_.begin();
            for(; it != headers_.end(); it++){
                oss << it->first << ":" << it->second << "\r\n";
            }

            oss << "\r\n";
            oss << body_;

            return oss.str();
        }

        void Request::async_cb(uv_async_t *async_handle) {
            resolve(url_.get_host(), url_.get_port());
        }

        // socket callback
        void Request::onResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res){
            if(0 != status){
                std::ostringstream oss;
                oss << "resolve failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                callback(resp_);
                return;
            }

            char addr[17] = {'\0'};
            uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
            std::string ip(addr);
            connect(ip, url_.get_port());
        }

        void Request::onConnect(uv_connect_t* req, int status){
            if(0 != status){
                std::ostringstream oss;
                oss << "connect failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                callback(resp_);
                return;
            }

            std::string data = raw();
            if (!data.empty()){
                startWrite(data.c_str(), data.size());
            }else{
                resp_->success_ = false;
                callback(resp_);
            }
        }

        void Request::onReadDone(uv_stream_t *handle, ssize_t nread, const char *buf){
            if(nread < 0){
                std::ostringstream oss;
                oss << "read failed, error[";
                oss << uv_err_name(nread) << "] reason[" << uv_strerror(nread) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                callback(resp_);
                return;
            }

            size_t nparsed = http_parser_execute(&parser_, &parser_settings_, buf, nread);
            if (nparsed != nread){
                std::ostringstream oss;
                oss << "parse failed, error[";
                oss << http_errno_name((enum http_errno)parser_.http_errno) << "] reason[" << http_errno_description((enum http_errno)parser_.http_errno) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                callback(resp_);
                return;
            }

            if (!chunk_mode_ ||
                (chunk_mode_ && http_body_is_final(&parser_) != 0)){
                uv_read_stop(handle);

//                delete this;
            }
        }

        void Request::onWriteDone(uv_write_t *req, int status){
            if(0 != status){
                std::ostringstream oss;
                oss << "write failed, error[";
                oss << uv_err_name(status) << "] reason[" << uv_strerror(status) << "]";
                resp_->set_last_error(oss.str());

                resp_->success_ = false;
                callback(resp_);
                return;
            }
            startRead();
        }

        void Request::onTimer(uv_timer_t *handle){
            uv_timer_stop(handle);

            resp_->success_ = false;
            callback(resp_);

            return;
        }

        // http parser callback
        int Request::on_message_begin(http_parser* parser) {
            start_parsing_ = true;
            return 0;
        }

        int Request::on_url(http_parser* parser, const char *at, size_t length) {
            resp_->url_.append(at, length);
            return 0;
        }

        int Request::on_status(http_parser* parser, const char *at, size_t length) {
            resp_->status_.append(at, length);
            resp_->code_ = parser->status_code;

            resp_->proto_major_ = parser->http_major;
            resp_->proto_minor_ = parser->http_minor;
            return 0;
        }

        int Request::on_header_field(http_parser* parser, const char *at, size_t length) {
            if (last_is_header_value){
                last_header_field_.clear();
                last_header_value_.clear();
            }

            last_header_field_.append(at, length);
            return 0;
        }

        int Request::on_header_value(http_parser* parser, const char *at, size_t length) {
            last_header_value_.append(at, length);
            resp_->headers_[last_header_field_] = last_header_value_;
            last_is_header_value = true;
            return 0;
        }

        int Request::on_headers_complete(http_parser* parser) {
            header_parsed_ = true;
            if (parser->content_length == 0){
                return 1; // no body need parsed
            }
            return 0;
        }

        int Request::on_body(http_parser* parser, const char *at, size_t length) {
            resp_->body_.append(at, length);
            return 0;
        }

        int Request::on_message_complete(http_parser* parser) {
            parse_completed_ = true;
            resp_->success_ = true;
            callback(resp_);
            return 0;
        }

        int Request::on_chunk_header(http_parser* parser) {
            chunk_mode_ = true;
            return 0;
        }

        int Request::on_chunk_complete(http_parser* parser) {
            parse_completed_ = true;
            return 0;
        }

        void Request::callback(Response *resp) {
            if(cb_){
                cb_(this, resp);
                cb_ = NULL;
            }
            resp->reset();
        }

        void AsyncEventCallBack::async_cb(uv_async_t *async_handle) {
            Request* req = reinterpret_cast<Request*>(async_handle->data);
            req->async_cb(async_handle);
        }

        int HTTPParserCallBack::on_message_begin(http_parser* parser) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_message_begin(parser);
        }

        int HTTPParserCallBack::on_url(http_parser* parser, const char *at, size_t length) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_url(parser, at, length);
        }

        int HTTPParserCallBack::on_status(http_parser* parser, const char *at, size_t length) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_status(parser, at, length);
        }

        int HTTPParserCallBack::on_header_field(http_parser* parser, const char *at, size_t length) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_header_field(parser, at, length);
        }

        int HTTPParserCallBack::on_header_value(http_parser* parser, const char *at, size_t length) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_header_value(parser, at, length);
        }

        int HTTPParserCallBack::on_headers_complete(http_parser* parser)
        {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_headers_complete(parser);
        }
        int HTTPParserCallBack::on_body(http_parser* parser, const char *at, size_t length) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_body(parser, at, length);
        }

        int HTTPParserCallBack::on_message_complete(http_parser* parser) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_message_complete(parser);
        }

        int HTTPParserCallBack::on_chunk_header(http_parser* parser) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_chunk_header(parser);
        }

        int HTTPParserCallBack::on_chunk_complete(http_parser* parser) {
            Request* httpclient = reinterpret_cast<Request*>(parser->data);
            return httpclient->on_chunk_complete(parser);
        }
    }
}