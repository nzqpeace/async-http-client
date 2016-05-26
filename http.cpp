//
// Created by 牛志强 on 16/5/16.
//

#include "http.h"
#include <sstream>

namespace zq{
    namespace http{

        Client::Client(zq::loop* loop)
                : loop_(loop)
        {
        }

        Client::~Client() {
            Session::iterator it = session_maps_.begin();
            for (; it != session_maps_.end(); it++){
                ReqID req_id = it->first;
                ReqRes req_res = it->second;

                Request* req = req_res.first;
                delete req;

                session_maps_.erase(it);
            }
        }

        bool Client::get(const std::string &url, HTTP_CALLBACK cb /* = NULL */){
            return request(HTTP_GET, url, "", cb);
        }

        bool Client::post(const std::string& url, const std::string& body, HTTP_CALLBACK cb /* = NULL */){
            return request(HTTP_POST, url, body, cb);
        }

        bool Client::request(http_method method, const std::string &url,
                             const std::string &body /* = "" */, HTTP_CALLBACK cb /* = NULL */)
        {
            Request* req = new Request(loop_, method, url, body, cb);
//            ReqID req_id = req->get_id();
//            Response* res = req->get_response();
//
//            session_maps_[req_id] = std::make_pair(req, res);
            return start(req);
        }

        bool Client::start(Request *req) {
            loop_->add_async_event(req->get_async_handle());
            return true;
        }
    }
}