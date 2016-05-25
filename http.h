//
// Created by 牛志强 on 16/5/16.
//

#ifndef HTTPCLIENT_HTTPCLIENT_H
#define HTTPCLIENT_HTTPCLIENT_H

#include "http_parser.h"
#include "url.h"
#include "async_client.h"

#include <string>
#include <map>
#include <map>

namespace zq {
    namespace http {
        class Request;
        class Response;

        typedef std::map<std::string, std::string> Header;
        typedef void (*HTTP_CALLBACK)(Response* resp);

        class Request {
        public:
            Request(const std::string& url, HTTP_CALLBACK cb = NULL);
            virtual ~Request(){}

            void reset();
            void set_callback(HTTP_CALLBACK cb) { cb_ = cb; }
            HTTP_CALLBACK get_callback() { return cb_; }

            void set_header(const std::string &key, const std::string &value){}
            std::string get_header(const std::string &key) { return headers_[key]; }
            Header headers() {return headers_; }

            void set_method(http_method method) { method_ = method; }
            std::string get_method() { return http_method_str(method_); }

            const std::string& get_remote_addr() { return remote_addr_; };

            const std::string& get_body() { return body_; }
            void set_body(const std::string& body) { body_ = body; }

            std::string get_url() { return url_.dump(); }

            int get_proto_major() { return proto_major_; }
            void set_proto_major(int proto_major) { proto_major_ = proto_major; }

            int get_proto_minor() { return proto_minor_; }
            void set_proto_minor(int proto_minor) { proto_minor_ = proto_minor; }

        private:
            Request(const Request &);
            Request &operator=(const Request &);

        private:
            http_method method_;
            int proto_major_;
            int proto_minor_;
            std::string remote_addr_;
            std::string body_;
            Header headers_;
            HTTP_CALLBACK cb_;

        public:
            zq::URL url_;
        };

        class Response {
        public:
            Response();
            virtual ~Response(){};

            void reset();
            const std::string& get_body() { return body_; }
            void set_body(const char *at, size_t length) {
                body_.append(at, length);
            }
            Header headers() { return headers_; }

            const std::string& get_status() { return status_; }
            void set_status(const std::string& status) { status_ = status; }

            const std::string& get_url() { return url_; }
            void set_url(const std::string& url) { url_ = url; }

            int get_code() { return code_; }
            void set_code(int code) { code_ = code; }

            const std::string& get_last_error() { return last_error_; }
            void set_last_error(const std::string& last_error) { last_error_ = last_error; }

            bool is_success() { return success_; }
        private:
            Response(const Response &);
            Response &operator=(const Response &);

        private:
            friend class Client;

            bool success_;
            int code_;
            std::string status_;
            std::string body_;
            std::string url_;
            Header headers_;
            std::string last_error_;
        };

        class HTTPParserCallBack{
        public:
            static int on_message_begin(http_parser *parser);
            static int on_url(http_parser *parser, const char *at, size_t length);
            static int on_status(http_parser *parser, const char *at, size_t length);
            static int on_header_field(http_parser *parser, const char *at, size_t length);
            static int on_header_value(http_parser *parser, const char *at, size_t length);
            static int on_headers_complete(http_parser *parser);
            static int on_body(http_parser *parser, const char *at, size_t length);
            static int on_message_complete(http_parser *parser);
            static int on_chunk_header(http_parser *parser);
            static int on_chunk_complete(http_parser *parser);
        };

        class Client : public zq::tcp::AsyncClient, public zq::tcp::RequestCallback{
        public:
            Client(HTTP_CALLBACK cb = NULL, uint64_t timeout = 5*1000);
            virtual ~Client();

            bool get(const std::string &url, HTTP_CALLBACK cb = NULL);
            bool post(const std::string& url, const std::string& body, HTTP_CALLBACK cb = NULL);

            void set_header(const std::string& key, const std::string& value) {
                if (req_){
                    req_->set_header(key, value);
                }
            }
            Header headers() { return req_->headers(); }

            void reset_header();
            const std::string& get_last_error() { return last_error_; }

        private:
            Client(const Client &);
            Client &operator=(const Client &);

            // socket callback
            virtual void onResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);
            virtual void onConnect(uv_connect_t* req, int status);
            virtual void onReadDone(uv_stream_t *handle, ssize_t nread, const char *buf);
            virtual void onWriteDone(uv_write_t *req, int status);
            virtual void onTimer(uv_timer_t *handle, int status);

            // http parser callback
            friend class HTTPParserCallBack;
            int on_message_begin(http_parser *parser);
            int on_url(http_parser *parser, const char *at, size_t length);
            int on_status(http_parser *parser, const char *at, size_t length);
            int on_header_field(http_parser *parser, const char *at, size_t length);
            int on_header_value(http_parser *parser, const char *at, size_t length);
            int on_headers_complete(http_parser *parser);
            int on_body(http_parser *parser, const char *at, size_t length);
            int on_message_complete(http_parser *parser);
            int on_chunk_header(http_parser *parser);
            int on_chunk_complete(http_parser *parser);

            void init();
            void reset();
            bool startRequest(Request* req);
            std::string raw(Request* req);
            void set_last_error(const std::string& last_error) { last_error_ = last_error; }

            bool request(http_method method, const std::string& url, const std::string& body = "", HTTP_CALLBACK cb = NULL);

        private:
            http_parser parser_;
            http_parser_settings parser_settings_;
            std::string last_error_;
            std::string last_header_field_;
            std::string last_header_value_;
            bool last_is_header_value;

            bool start_parsing_;
            bool header_parsed_;
            bool parse_completed_;
            bool chunk_mode_;

            uint64_t timeout_;

            uv_loop_t* loop_;

            Request* req_;
            Response* resp_;
            HTTP_CALLBACK cb_;
        };
    }
}

#endif //HTTPCLIENT_HTTPCLIENT_H
