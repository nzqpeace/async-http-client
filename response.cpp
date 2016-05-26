//
// Created by 牛志强 on 16/5/25.
//

#include "response.h"
#include <sstream>

namespace zq{
    namespace http{
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
        
        std::string Response::raw() {
            std::ostringstream oss;
            oss << "HTTP/";
            oss <<  proto_major_ << "." << proto_minor_ << " " << code_ << " " << status_ << "\r\n";

            Header::iterator it = headers_.begin();
            for(; it != headers_.end(); it++){
                oss << it->first << ":" << it->second << "\r\n";
            }

            oss << "\r\n";
            oss << body_;

            return oss.str();
        }
    }
}