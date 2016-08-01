#ifndef IPOIRC_ZMQ_COMM_H
#define IPOIRC_ZMQ_COMM_H

#include "comm.h"
#include <set>
#include <memory>

namespace ZMQComm {

    class Socket : public Comm::SocketInt {
    public:
        Socket(void* s) : s(s) {}; /* internal */
        ~Socket();
        Socket(const Socket&) = delete;
        Socket(const Socket&&) = delete;
        std::vector<uint8_t> recv();
        bool send(const std::vector<uint8_t>&);
    private:
        void* s;
    };

    class Ctx : public Comm::CtxInt {
    public:
        Ctx();
        ~Ctx();
        Ctx(const Ctx&)  = delete;
        Ctx(const Ctx&&) = delete;
        Comm::Socket connect(Comm::SocketSide);
    private:
        std::set<Comm::SocketSide> socket_list;
        void *zmq_ctx;
        std::string to_socket_name(Comm::SocketSide);
    };

    Comm::Ctx create();
}

#endif