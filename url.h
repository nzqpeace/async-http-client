//
// Created by 牛志强 on 16/5/19.
//

#ifndef HTTPCLIENT_URL_H
#define HTTPCLIENT_URL_H

#include "http_parser.h"
#include <string>

namespace zq{
    class URL{
    public:
        URL();
        virtual ~URL();

        bool parse(const char* data);
        std::string dump();

        const std::string& get_schema() { return schema_; }
        void set_schema(const std::string& schema) { schema_ = schema; }

        const std::string& get_host() { return host_; }
        void set_host(const std::string& host) { host_ = host; }

        int get_port() { return port_; }
        void set_port(int port) { port_ = port; }

        const std::string& get_path() { return path_; }
        void set_path(const std::string& path) { path_ = path; }

        const std::string& get_query() { return query_; }
        void set_query(const std::string& query) { query_ = query; }

        const std::string& get_fragment() { return fragment_; }
        void set_fragment(const std::string& fragment) { fragment_ = fragment; }

        const std::string& get_username() { return username_; }
        void set_username(const std::string& username) { username_ = username; }

        const std::string& get_password() { return password_; }
        void set_password(const std::string& password) { password_ = password; }

    private:
        URL(const URL&);
        URL& operator=(const URL&);

        std::string get_url_field(const char* buf, http_parser_url_fields field);

    private:
        http_parser_url url_parser_;
        std::string schema_;
        std::string host_;
        int port_;
        std::string path_;
        std::string query_;
        std::string fragment_;
        std::string username_;
        std::string password_;
    };
}

#endif //HTTPCLIENT_URL_H
