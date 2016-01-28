#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <thread>
#include <zmq.h>

#include "ipoirc.h"
#include "config.h"
#include "ltun.h"
#include "tun.h"
#include "tun_helpers.h"

void tun_thread_zmq(void* zmq_context, const Tun& tun) {

    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    void *socket = NULL;

    if (!sbuffer) goto exit;
    socket = zmq_socket(zmq_context, ZMQ_PULL); // client of the tun_socket
    if (zmq_bind(socket, "inproc://#irc_to_#tun")) {
        tun_debug() << "(tun_thread_zmq) error when connecting to IPC socket - " << zmq_strerror(errno) << std::endl;
        goto exit;
    }
    while (1) {
        tun_debug() << ">> zmq_recv <<" << std::endl;
        memset(sbuffer, 0, MTU);
        int nbytes = zmq_recv(socket, sbuffer, MTU, 0);
        tun_debug() << "got " << nbytes << "bytes" << std::endl;
        if (nbytes < 0 ) {
            tun_debug() << "error when reading from zeromq socket" << std::endl;
            goto exit; // a cute break here!
        } else if (nbytes == 0) {
            continue;
        } else if (nbytes > MTU) {
            tun_debug() << "warning: some message got truncated by " << nbytes - MTU << "(" << nbytes << " - " << MTU << "), this means the MTU is too low for you!" << std::endl;
        }
        tun.write(sbuffer, nbytes);
    }
    exit:
    if (socket)
        zmq_close(socket);
}

void tun_thread_dt(void* zmq_context, const Tun& tun) {

    void *socket = zmq_socket(zmq_context, ZMQ_PUSH);

    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    if (!sbuffer) goto exit;

    if (zmq_bind(socket, "inproc://#tun_to_#irc")) {
        tun_debug() << "error when creating IPC socket - " << zmq_strerror(errno) << std::endl;
        goto exit;
    }

    // tell_to_other_threads_the_tun2irc_socket_is_binded
    tun_debug() << "[data] created tun (data) thread!" << std::endl;
    int nbytes;
    while ((nbytes = tun.read(sbuffer, MTU)) != 0) {
        if (nbytes > 0) {
            tun_debug() << "got " << nbytes << "from tun" << std::endl;
            if (zmq_send(socket, sbuffer, nbytes, 0) < 0) {
                tun_debug() << "error when trying to send a message to the irc thread (warning, we continue here!) :" << zmq_strerror(errno) << std::endl;
            }
        } else {
            tun_debug() << "error reading data from tun: " << strerror(errno) << std::endl;
        }
    }

    exit:
    if (socket)
        zmq_close(socket);
}


void tun_thread(void* zmq_context, const Tun& tun) {

    std::thread zmq_th([zmq_context,&tun]() {
        return tun_thread_zmq(zmq_context, tun);
    });

    std::thread dt_th([zmq_context,&tun]() {
        return tun_thread_dt(zmq_context, tun);
    });

    dt_th.join();
    zmq_th.join();
}
