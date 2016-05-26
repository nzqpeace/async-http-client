//
// Created by 牛志强 on 16/5/16.
//

#ifndef HTTPCLIENT_HTTPCLIENT_H
#define HTTPCLIENT_HTTPCLIENT_H

#include "http_parser.h"
#include "url.h"
#include "async_client.h"
#include "request.h"
#include "response.h"
#include "loop.h"

#include <string>
#include <map>
#include <map>

namespace zq {
    namespace http {
        typedef std::pair<Request*, Response*> ReqRes;
        typedef uint64_t ReqID;
        typedef std::map<ReqID, ReqRes> Session;

        class Client {
        public:
            Client(zq::loop* loop);
            virtual ~Client();

            bool get(const std::string &url, HTTP_CALLBACK cb = NULL);
            bool post(const std::string& url, const std::string& body, HTTP_CALLBACK cb = NULL);

            void set_header(const std::string& key, const std::string& value) { headers_[key] = value; }
            std::string get_header(const std::string& key) { return headers_[key]; }
            Header headers() { return headers_; }

            void reset_header();

        private:
            Client(const Client &);
            Client &operator=(const Client &);

            bool start(Request *req);

            bool request(http_method method, const std::string& url, const std::string& body = "", HTTP_CALLBACK cb = NULL);

        private:
            zq::loop* loop_;
            Header headers_;
            Session session_maps_;
        };
    }
}

#endif //HTTPCLIENT_HTTPCLIENT_H
