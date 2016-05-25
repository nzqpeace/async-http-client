#include "async_client.h"
#include <sstream>

namespace zq{
    namespace tcp{
        size_t AsyncClient::baseID_ = 1;
        AsyncClient::AsyncClient()
        {
            connect_  = (uv_connect_t*)malloc(sizeof(uv_connect_t));
            write_    = (uv_write_t*)malloc(sizeof(uv_write_t));
            //read_   = (uv_stream_t*)malloc(sizeof(uv_stream_t));
            timer_    = (uv_timer_t*)malloc(sizeof(uv_timer_t));
            stream_   = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
            resolver_ = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));

            // get client identify
            id_ = baseID_ ++;
        }

        AsyncClient::AsyncClient(uv_loop_t *loop, uint64_t timeout /* = 5*1000 */ )
                : loop_(loop), timeout_(timeout)
        {
            connect_  = (uv_connect_t*)malloc(sizeof(uv_connect_t));
            write_    = (uv_write_t*)malloc(sizeof(uv_write_t));
            //read_   = (uv_stream_t*)malloc(sizeof(uv_stream_t));
            timer_    = (uv_timer_t*)malloc(sizeof(uv_timer_t));
            stream_   = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
            resolver_ = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));

            // get client identify
            id_ = baseID_ ++;
        }

        AsyncClient::~AsyncClient()
        {
            free(connect_);
            free(write_);
            //free(read_);
            free(timer_);
            free(stream_);
            free(resolver_);
        }

        void AsyncClient::reset(RequestCallback* cb)
        {
            cb_ = cb;
            status_ = Initialize;
            connect_->data   = reinterpret_cast<void*>(this);
            write_->data     = reinterpret_cast<void*>(this);
            timer_->data     = reinterpret_cast<void*>(this);
            stream_->data    = reinterpret_cast<void*>(this);
            resolver_->data  = reinterpret_cast<void*>(this);
        }

        void AsyncClient::set_uv_loop(uv_loop_t *loop) {
            loop_ = loop;
        }

        void AsyncClient::set_timeout(uint64_t timeout) {
            timeout_ = timeout;
        }

        size_t AsyncClient::getClientID() {
            return id_;
        }

        void AsyncClient::setStatus(Status status)
        {
            status_  = status;
        }

        AsyncClient::Status AsyncClient::getStatus()
        {
            return status_;
        }

        std::string AsyncClient::getStatusString(Status status)
        {
            switch(status){
                case Initialize:
                    return "Initialize";
                case Connecting:
                    return "Connecting";
                case Connected:
                    return "Connected";
                case Reading:
                    return "Reading";
                case ReadDone:
                    return "ReadDone";
                case Writing:
                    return "Writing";
                case WriteDone:
                    return "WriteDone";
                case Timeout:
                    return "Timeout";
                case Idle:
                    return "Idle";
                case Closed:
                    return "Closed";
                default:
                    return "";
            }
        }

        bool AsyncClient::resolve(const std::string& node, int service){
            std::ostringstream oss;
            oss << service;
            return resolve(node, oss.str());
        }

        bool AsyncClient::resolve(const std::string& node, const std::string& service){
            struct addrinfo hints;
            hints.ai_family = PF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = 0;

            int ret = uv_getaddrinfo(loop_, resolver_, resolveDone, node.c_str(), service.c_str(), &hints);
            if (0 != ret){
                return false;
            }
            setStatus(Resolving);
            return true;
        }

        bool AsyncClient::connect(std::string ip, int port)
        {
            int ret = uv_tcp_init(loop_, stream_);
            if(0 != ret) {
                return false;
            }

            struct sockaddr_in dest;
            uv_ip4_addr(ip.c_str(), port, &dest);

            struct sockaddr_in client;
            uv_ip4_addr("0.0.0.0", 0, &client);   // use loopback for test
            uv_tcp_bind(stream_, (struct sockaddr *)&client, 0);

            ret = uv_tcp_connect(connect_, stream_, (struct sockaddr *)&dest, connectDone);
            if(0 != ret){

                return false;
            }

            setStatus(Connecting);
            return true;
        }

        bool AsyncClient::startRead()
        {
            int ret = uv_read_start((uv_stream_t*)stream_, bufferAlloc, readDone);
            if(0 != ret){
                return false;
            }
            setStatus(Reading);
            return true;
        }

        bool AsyncClient::startWrite(const char* data, size_t len)
        {
            uv_buf_t buf;
            buf.base = (char*)data;
            buf.len  = len;

            int ret = uv_write(write_, (uv_stream_t*)stream_, &buf, 1, writeDone);
            if(0 != ret){
                return false;
            }
            setStatus(Writing);
            return true;
        }

        void AsyncClient::resolveDone(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
            AsyncClient* pThis = reinterpret_cast<AsyncClient*>(resolver->data);
            pThis->setStatus(Resolved);
            pThis->cb_->onResolved(resolver, status, res);
        }

        void AsyncClient::connectDone(uv_connect_t* req, int status)
        {
            AsyncClient* pThis = reinterpret_cast<AsyncClient*>(req->data);
            pThis->setStatus(Connected);
            pThis->cb_->onConnect(req, status);
        }

        void AsyncClient::readDone(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
        {
            AsyncClient* pThis = reinterpret_cast<AsyncClient*>(handle->data);
            pThis->setStatus(ReadDone);
            pThis->cb_->onReadDone(handle, nread, buf->base);

            if (buf->base)
                free(buf->base);
        }

        void AsyncClient::writeDone(uv_write_t *req, int status)
        {
            AsyncClient* pThis = reinterpret_cast<AsyncClient*>(req->data);
            pThis->setStatus(WriteDone);
            pThis->cb_->onWriteDone(req, status);
        }

        void AsyncClient::timeout(uv_timer_t *handle, int status)
        {
            AsyncClient* pThis = reinterpret_cast<AsyncClient*>(handle->data);
            pThis->setStatus(Timeout);
            pThis->cb_->onTimer(handle, status);
        }
    }
}
