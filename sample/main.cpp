//
// Created by 牛志强 on 16/5/24.
//
#include <iostream>
#include <unistd.h>

#include "../http.h"

bool kComplete = false;
void httptest_cb(zq::http::Response* resp){
    if (resp->is_success()){
        std::cout << "HTTP: " << resp->get_code() << " " << resp->get_status() << std::endl;
        for(auto &kv: resp->headers()){
            std::cout << kv.first << ":" << kv.second << std::endl;
        }
        std::cout << std::endl;
        std::cout << resp->get_body() << std::endl;
    }else{
        std::cout << "HTTP Request Failed! " << resp->get_last_error() << std::endl;
    }
    kComplete = true;
}

int main(int argc, char** argv) {
    zq::http::Client * httpclinet  = new zq::http::Client(httptest_cb);
    std::string url = "http://reqres.in/api/users/2";
    httpclinet->get(url);

    while (!kComplete){
        usleep(1000);
    }
}
