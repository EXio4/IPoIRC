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

void TunModule::worker_reader(const Tun& tun, Comm::Socket socket) {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    if (!sbuffer) return;

    int nbytes = -1;
    while ((nbytes = zmq_recv(socket, sbuffer, MTU, 0)) >= 0) {
        tun_debug() << "got " << nbytes << " bytes" << std::endl;
        if (nbytes == 0) {
            continue;
        } else if (nbytes > MTU) {
            tun_debug() << "warning: some message got truncated by " << nbytes - MTU << "(" << nbytes << " - " << MTU << "), this means the MTU is too low for you!" << std::endl;
        }
        tun.write(sbuffer, nbytes);
    }
};
void TunModule::worker_writer(const Tun& tun, Comm::Socket socket) {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    if (!sbuffer) return;

    int nbytes = -1;
    while ((nbytes = tun.read(sbuffer, MTU)) != 0) {
        if (nbytes > 0) {
            tun_debug() << "got " << nbytes << " from tun" << std::endl;
            if (zmq_send(socket, sbuffer, nbytes, 0) < 0) {
                tun_debug() << "error when trying to send a message to the irc thread (warning, we continue here!) :" << zmq_strerror(errno) << std::endl;
            }
        } else {
            tun_debug() << "error reading data from tun: " << strerror(errno) << std::endl;
        }
    }
};

