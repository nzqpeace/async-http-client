//
// Created by 牛志强 on 16/5/25.
//

#ifndef HTTPCLIENT_LOOP_H
#define HTTPCLIENT_LOOP_H

#include <uv.h>

namespace zq{
    class loop {
    public:
        loop();
        virtual ~loop();

        bool start();

        uv_loop_t* get_uv_loop() { return uv_loop_; }

        void add_async_event(uv_async_t* async_handle);
        void run();
        void idle_process(uv_idle_t* handle);

    private:
        static void thread_func(void* args);
        static void idle_func(uv_idle_t* handle);



    private:
        loop(const loop&);
        loop& operator=(const loop&);

        uv_idle_t* idler_;
        uv_loop_t* uv_loop_;
        uv_thread_t* thread_id_;
    };
}


#endif //HTTPCLIENT_LOOP_H
