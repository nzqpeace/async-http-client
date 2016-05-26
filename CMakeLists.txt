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
    request.cpp
    request.h
    response.cpp
    response.h
    loop.cpp
    loop.h)
add_library(httpclient ${SOURCE_FILES})

if (APPLE)
    target_link_libraries(httpclient uv.a)
else (UNIX)
    target_link_libraries(httpclient uv.a rt)
endif (APPLE)

add_executable(async_http_test
    test/main.cpp
    test/test_url.cpp
)

if (APPLE)
    target_link_libraries(async_http_test gtest.a httpclient)
else (UNIX)
    target_link_libraries(async_http_test gtest.a httpclient pthread)
endif (APPLE)

add_executable(async_http_sample
        sample/main.cpp
        sample/main.cpp)

if (APPLE)
    target_link_libraries(async_http_sample httpclient)
else (UNIX)
    target_link_libraries(async_http_sample httpclient pthread rt)
endif (APPLE)
