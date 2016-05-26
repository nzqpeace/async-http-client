//
// Created by 牛志强 on 16/5/25.
//

#ifndef HTTPCLIENT_RESPONSE_H
#define HTTPCLIENT_RESPONSE_H

#include "http_parser.h"

#include <string>
#include <map>

namespace zq{
    namespace http{
        typedef std::map<std::string, std::string> Header;

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

            void set_req_id(uint64_t req_id) { req_id_ = req_id; }
            uint64_t get_req_id() { return req_id_; }

            const std::string& get_status() { return status_; }
            void set_status(const std::string& status) { status_ = status; }

            const std::string& get_url() { return url_; }
            void set_url(const std::string& url) { url_ = url; }

            int get_code() { return code_; }
            void set_code(int code) { code_ = code; }

            int get_proto_major() { return proto_major_; }
            void set_proto_major(int proto_major) { proto_major_ = proto_major; }

            int get_proto_minor() { return proto_minor_; }
            void set_proto_minor(int proto_minor) { proto_minor_ = proto_minor; }

            const std::string& get_last_error() { return last_error_; }
            void set_last_error(const std::string& last_error) { last_error_ = last_error; }

            bool is_success() { return success_; }

            std::string raw();
        private:
            Response(const Response &);
            Response &operator=(const Response &);

        private:
            friend class Request;

            uint64_t req_id_;
            bool success_;
            int code_;
            int proto_major_;
            int proto_minor_;
            std::string status_;
            std::string body_;
            std::string url_;
            Header headers_;
            std::string last_error_;
        };
    }
}


#endif //HTTPCLIENT_RESPONSE_H
