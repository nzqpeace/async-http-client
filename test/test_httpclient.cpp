//
// Created by 牛志强 on 16/5/23.
//
#include <gtest/gtest.h>
#include "../http.h"

// typedef void (*HTTP_CALLBACK)(bool success, Response* resp);
void httptest_cb(zq::http::Response* resp){
    if (resp->is_success()){
        std::cout << "HTTP: " << resp->get_code() << " " << resp->get_status() << std::endl;
        for(auto &kv: resp->headers()){
            std::cout << kv.first << ":" << kv.second << std::endl;
        }
        std::cout << std::endl;
        std::cout << resp->get_body() << std::endl;
    }else{
        std::cout << "HTTP Request Failed!" << std::endl;
    }
}

TEST(HTTPTest, Get){
    zq::http::Client * httpclinet  = new zq::http::Client(httptest_cb);
     std::string url = "http://reqres.in/api/users/2";
//    std::string url = "http://10.15.54.78:6333/api/v1/all?offset=0&count=1000";
    EXPECT_TRUE(httpclinet->get(url));
}

TEST(HTTPTest, Post){
    zq::http::Client * httpclinet  = new zq::http::Client(httptest_cb);
    std::string url = "http://reqres.in/api/users";
    std::string body = "{\n\"name\": \"morpheus\",\n\"job\": \"leader\"\n }";
    EXPECT_TRUE(httpclinet->post(url, body));
}