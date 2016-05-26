//
// Created by 牛志强 on 16/5/24.
//
#include <iostream>
#include <unistd.h>
#include <uv.h>

#include "../http.h"

uv_mutex_t* mu;

int kComplete = 0;
void httptest_cb(zq::http::Request* req, zq::http::Response* resp){
    if (req != NULL){
        std::cout << "RequestID:" << req->get_id() << std::endl;
        std::cout << req->raw() << std::endl;
    }

    if (resp->is_success()){
        std::cout << resp->raw() << std::endl;
    }else{
        std::cout << "HTTP Request Failed! " << resp->get_last_error() << std::endl;
    }
    std::cout << std::endl;
//
//    uv_mutex_lock(mu);
//    kComplete++;
//    uv_mutex_unlock(mu);
}

int main(int argc, char** argv) {
    zq::loop loop;
    loop.start();

    sleep(1);

    zq::http::Client * httpclinet  = new zq::http::Client(&loop);

    mu = (uv_mutex_t*)malloc(sizeof(uv_mutex_t));
    uv_mutex_init(mu);

    int count = 100;
    int index = 0;
    while(index < count){
        std::string get_url = "http://reqres.in/api/users/2";
        httpclinet->get(get_url, httptest_cb);

        index++;
    }

    while (true){
        usleep(1000);
    }
}
