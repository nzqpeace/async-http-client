cmake_minimum_required(VERSION 2.6)
project(httpclient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(SOURCE_FILES
    http.h
    http.cpp
    http_parser.h
    http_parser.c
    url.h
    url.cpp
    async_client.h
    async_client.cpp
)
add_library(httpclient ${SOURCE_FILES})
target_link_libraries(httpclient uv.a)

add_executable(async_http_test
    test/main.cpp
    test/test_url.cpp
    test/test_httpclient.cpp
)
target_link_libraries(async_http_test gtest.a httpclient)

add_executable(async_http_sample
        sample/main.cpp
        sample/main.cpp)
target_link_libraries(async_http_sample httpclient)