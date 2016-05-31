#ifndef __ASYNC_CLIENT_H
#define __ASYNC_CLIENT_H

#include <string>
#include <uv.h>
#include <stdlib.h>

namespace zq{
    namespace tcp{
        class RequestCallback
        {
        public:
            virtual ~RequestCallback(){}

            virtual void onResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) = 0;
            virtual void onConnect(uv_connect_t* req, int status) = 0;
            virtual void onReadDone(uv_stream_t *handle, ssize_t nread, const char *buf) = 0;
            virtual void onWriteDone(uv_write_t *req, int status) = 0;
            virtual void onTimer(uv_timer_t *handle) = 0;
        };

        class AsyncClient
        {
        public:
            virtual ~AsyncClient();
            enum Status{
                Initialize,
                Resolving,
                Resolved,
                Connecting,
                Connected,
                Reading,
                ReadDone,
                Writing,
                WriteDone,
                Timeout,
                Idle,
                Closed
            };

            void setStatus(Status status);
            Status getStatus();
            std::string getStatusString(Status status);
            size_t getClientID();

            void reset(RequestCallback* cb);
//protect:
            AsyncClient();
            AsyncClient(uv_loop_t *loop, uint64_t timeout = 5*1000);

            void set_uv_loop(uv_loop_t *loop);
            void set_timeout(uint64_t timeout = 5*1000);

            bool resolve(const std::string& node, int service);
            bool resolve(const std::string& node, const std::string& service);
            bool connect(std::string ip, int port);
            bool startRead();
            bool startWrite(const char* data, size_t len);

            static void bufferAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
                *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
            }

        private:
            static void resolveDone(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);
            static void connectDone(uv_connect_t* req, int status);
            static void readDone(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf);
            static void writeDone(uv_write_t *req, int status);
            static void timeout(uv_timer_t *handle);

        private:
            static size_t   baseID_;
            size_t          id_;
            uv_loop_t*      loop_;
            uv_connect_t*   connect_;
            uv_write_t*     write_;
            //uv_stream_t*    read_;
            uv_timer_t*     timer_;
            uv_tcp_t*       stream_;
            uv_getaddrinfo_t* resolver_;
            std::string     serverIP_;
            int             serverPort_;
            uint64_t        timeout_;
            RequestCallback*    cb_;
            Status              status_;
        };
    }
}

#endif
