
#include "zmq-comm.h"
#include <zmq.h>
#include "utils.h"


namespace ZMQComm {

    Ctx::Ctx() {
        zmq_ctx = zmq_ctx_new();
    }
    Ctx::~Ctx() {
        zmq_ctx_destroy(zmq_ctx);
    }

    Comm::Socket Ctx::connect(Comm::SocketSide side) {
        std::string s = to_socket_name(side);
        void *socket = zmq_socket(zmq_ctx, ZMQ_PUSH);
        if (!socket) return NULL; /* exception later */
        if (socket_list.find(side) != socket_list.end()) { /* if we aren't already connected, then we must bind the socket */
            socket_list.insert(side);
            if (zmq_bind(socket, s.c_str())) {
                /* throw exception here */
            }
        } else {                                          /* otherwise, it already exists, and you must simply connect to it */
            if (zmq_connect(socket, s.c_str())) {
                /* throw exception here */
            }
        }
        return std::shared_ptr<Socket>(new Socket(socket));
    }
    std::string Ctx::to_socket_name(Comm::SocketSide side) {
        switch (side) {
            case Comm::SocketSide::Chat2Net:
                    return "inproc://ipoirc_chat2net";
                    break;
            case Comm::SocketSide::Net2Chat:
                    return "inproc://ipoirc_net2chat";
                    break;
            default:
                    return "";
                    /* throw exception */
        }
    }
    Comm::Ctx create() {
        return std::shared_ptr<Comm::CtxInt>((Comm::CtxInt*)(new Ctx()));
    }

    Socket::~Socket() {
        zmq_close(s);
    }

    bool Socket::send(const std::vector<uint8_t> &uc) {
        return zmq_send(s, uc.data(), uc.size(), 0) >= 0;
    }
    std::vector<uint8_t> Socket::recv() {
        char buffer[4096];
        int n = zmq_recv(s, buffer, 4096, 0);
        if (n >= 0) {
            return Utils::from_ptr(buffer, n);
        } else {
            /* throw exception */
            return {};
        }
    }

}