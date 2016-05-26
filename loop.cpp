//
// Created by 牛志强 on 16/5/25.
//

#include "loop.h"
#include <unistd.h>
#include <stdlib.h>

namespace zq{
    loop::loop() {
        uv_loop_ = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        idler_ = (uv_idle_t*)malloc(sizeof(uv_idle_t));
        thread_id_ = (uv_thread_t*)malloc(sizeof(uv_thread_t));
    }

    loop::~loop() {
        if (uv_loop_){
            uv_idle_stop(idler_);
            uv_stop(uv_loop_);
            while (uv_loop_alive(uv_loop_) != 0){
                usleep(10);
            }
            free(uv_loop_);
        }

        if (idler_){
            free(idler_);
        }
    }

    bool loop::start() {
        return uv_thread_create(thread_id_, loop::thread_func, this) == 0;
    }

    void loop::thread_func(void *args) {
        loop* pthis = reinterpret_cast<loop*>(args);
        pthis->run();
    }

    void loop::idle_func(uv_idle_t *handle) {
        loop* pthis = reinterpret_cast<loop*>(handle->data);
        pthis->idle_process(handle);
    }

    void loop::idle_process(uv_idle_t *handle) {
        usleep(10);
    }

    void loop::run() {
        uv_loop_init(uv_loop_);

        // set idle pattern
        uv_idle_init(uv_loop_, idler_);
        idler_->data = reinterpret_cast<void*>(this);

        uv_idle_start(idler_, loop::idle_func);
        uv_run(uv_loop_, UV_RUN_DEFAULT);
    }

    void loop::add_async_event(uv_async_t *async_handle) {
        uv_async_send(async_handle);
    }
}