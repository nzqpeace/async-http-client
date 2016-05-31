//
// Created by 牛志强 on 16/5/25.
//

#ifndef HTTPCLIENT_REQUEST_H
#define HTTPCLIENT_REQUEST_H

#include "url.h"
#include "http_parser.h"
#include "async_client.h"
#include "response.h"
#include "loop.h"

#include <string>
#include <map>

namespace zq{
    namespace http{
        class Request;
        typedef std::map<std::string, std::string> Header;
        typedef void (*HTTP_CALLBACK)(Request*, Response*);

        class AsyncEventCallBack{
        public:
            static void async_cb(uv_async_t* async_handle);
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

        class Request : public zq::tcp::AsyncClient,
                        public zq::tcp::RequestCallback {
        public:
            Request(zq::loop* loop, http_method method, const std::string& url, const std::string& body, HTTP_CALLBACK cb = NULL);
            virtual ~Request();

            void init();
            void reset();
            void set_callback(HTTP_CALLBACK cb) { cb_ = cb; }
            HTTP_CALLBACK get_callback() { return cb_; }

            uint64_t get_id() { return id_; }
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

            std::string raw();

            Response* get_response() { return resp_; }
            uv_async_t* get_async_handle() { return async_handle_; }

            // async event callback
            void async_cb(uv_async_t* async_handle);

            // socket callback
            virtual void onResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);
            virtual void onConnect(uv_connect_t* req, int status);
            virtual void onReadDone(uv_stream_t *handle, ssize_t nread, const char *buf);
            virtual void onWriteDone(uv_write_t *req, int status);
            virtual void onTimer(uv_timer_t *handle);

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

        private:
            Request(const Request &);
            Request &operator=(const Request &);

            void callback(Response* resp);

        private:
            static uint64_t auto_id_;
            uint64_t id_;
            http_method method_;
            int proto_major_;
            int proto_minor_;
            std::string remote_addr_;
            std::string body_;
            Header headers_;
            HTTP_CALLBACK cb_;

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

            uv_timer_t* timer_;
            uv_async_t* async_handle_;
            uv_loop_t* uv_loop_;
            Response* resp_;

        public:
            zq::URL url_;
        };

    }
}


#endif //HTTPCLIENT_REQUEST_H
