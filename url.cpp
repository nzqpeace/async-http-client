//
// Created by 牛志强 on 16/5/19.
//

#include "url.h"

#include <string.h>
#include <stdlib.h>
#include <sstream>

namespace zq{
    URL::URL() : schema_(""), host_(""), port_(0), path_(""), query_(""), fragment_(""), username_(""), password_("")
    {

    }

    URL::~URL() {

    }

    std::string URL::get_url_field(const char* buf, http_parser_url_fields field) {
        std::string url(buf, strlen(buf));

        if (url_parser_.field_set & (1 << field)){
            // check if cross the border
            uint16_t offset = url_parser_.field_data[field].off;
            uint16_t len = url_parser_.field_data[field].len;
            if( offset + len > url.size()){
                return "";
            }

            return url.substr(offset, len);
        }
        return "";
    }

    bool URL::parse(const char *buf) {
        http_parser_url_init(&url_parser_);

        int ret = http_parser_parse_url(buf, strlen(buf), 0, &url_parser_);
        if(ret != 0){
            return false;
        }

        schema_   = get_url_field(buf, UF_SCHEMA);
        host_     = get_url_field(buf, UF_HOST);
        port_     = atoi(get_url_field(buf, UF_PORT).c_str());
        if (port_ == 0) {
            port_ = 80;
        }

        path_     = get_url_field(buf, UF_PATH);
        if (path_.empty()){
            path_ = "/";
        }

        query_    = get_url_field(buf, UF_QUERY);
        fragment_ = get_url_field(buf, UF_FRAGMENT);

        std::string userinfo = get_url_field(buf, UF_USERINFO);
        size_t pos = userinfo.find_first_of(':');
        if (pos != std::string::npos){
            username_ = userinfo.substr(0, pos);
            password_ = userinfo.substr(pos + 1);
        }else{
            username_ = userinfo;
            password_ = "";
        }
        return true;
    }

    std::string URL::dump() {
        // scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
        std::ostringstream oss;
        oss << schema_ << ":" << "//";

        if(username_ != ""){
            oss << username_;
            if(password_ != ""){
                oss << ":";
                oss << password_;
            }
            oss << "@";
        }

        oss << host_;
        if (port_ != 0){
            oss << ":" << port_;
        }

        if (path_ == ""){
            oss << "/";
        }else{
            oss << path_;
        }

        if (query_ != ""){
            oss << "?";
            oss << query_;
        }

        if (fragment_ != ""){
            oss << "#";
            oss << fragment_;
        }

        return oss.str();
    }
}
