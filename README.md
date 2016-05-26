# async-http-clinet
An asychronous http client written by c++, provide a simple and convenient interface.

based on [libuv](https://github.com/libuv/libuv) and [http_parser](https://github.com/nodejs/http-parser).

# Feature
- Asychronous for all http request
- Cross all platform, linux/win/osx/bsd, benefit from libuv
- Lightweight, less than 2k-lines, http parser alse counted in

# Why?
Because I couldn't found a simple http client written by c++. libcurl is so complex, and interface is awkward.
I just want a simple http client in my cpp project, so I write this.

# Sample
```cpp
#include "http.h"
#include <iostream>

void http_cb(zq::http::Request* req, zq::http::Response* resp){
    if (req != NULL){
        std::cout << "RequestID:" << req->get_id() << std::endl;
        std::cout << req->raw() << std::endl;
    }

    if (resp->is_success()){
        std::cout << resp->raw() << std::endl;
    }else{
        std::cout << "HTTP Request Failed! " << resp->get_last_error() << std::endl;
    }
}

int main(int argc, char** argv) {
    // create an event loop for work
    zq::loop loop;
    loop.start();

    zq::http::Client * httpclinet  = new zq::http::Client(&loop);

    std::string get_url = "http://reqres.in/api/users/2";
    httpclinet->get(get_url, http_cb);

    // do not exit immediately, wait for http callback
    while (true){
        usleep(1000);
    }
}
```
# Details
`zq::loop` is an event loop based on [libuv](https://github.com/libuv/libuv), all http request would be processed in it, and it create a new thread for working when call `start()`.
```cpp
bool loop::start() {
    return uv_thread_create(thread_id_, loop::thread_func, this) == 0;
}
```
The async http client provide a simple and convenient interface as follow.
```cpp
bool get(const std::string &url, HTTP_CALLBACK cb = NULL);
bool post(const std::string& url, const std::string& body, HTTP_CALLBACK cb = NULL);
```
`HTTP_CALLBACK` is a function point for callback
```cpp
typedef void (*HTTP_CALLBACK)(Request*, Response*);
```

Also, we can create complex request through interface `bool start(Request *req)`, for example:
```cpp
// Request(zq::loop* loop, http_method method, const std::string& url, const std::string& body, HTTP_CALLBACK cb = NULL);
zq::http::Request* req = new zq::http::Request(loop, HTTP_GET, url, body, cb);
req.set_header(key, value);
req.set_proto_major(2);
req.set_proto_minor(0);
req.set_method(HTTP_POST);
req.set_body(body);
req.set_callback(anthod_cb);

httpclient->start(req);
```
