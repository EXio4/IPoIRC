
#include <iostream>
#include "net.h"
#include "log.h"

#include <zmq.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Net  NetModule::priv_init(NetConfig cfg) const {
    int listen_fd, comm_fd;

    struct sockaddr_in servaddr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero( &servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(cfg.port);

    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(listen_fd, 1);

    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    return Net { .l_fd = listen_fd
               , .c_fd = comm_fd 
               };
};

void NetModule::worker_reader(Net net, Comm::Socket s) const {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    if (!sbuffer) return;

    int nbytes = -1;
    while ((nbytes = zmq_recv(s, sbuffer, MTU, 0)) >= 0) {
        log(Log::Debug) << "got " << nbytes << " bytes" << std::endl;
        if (nbytes == 0) {
            continue;
        } else if (nbytes > MTU) {
            log(Log::Warning) << "warning: some message got truncated by " << nbytes - MTU << "(" << nbytes << " - " << MTU << "), this means the MTU is too low for you!" << std::endl;
        }
        write(net.c_fd , sbuffer , nbytes);
    }
};
void NetModule::worker_writer(Net net, Comm::Socket s) const {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    if (!sbuffer) return;

    int nbytes = -1;
    while ((nbytes = read(net.c_fd, sbuffer, MTU)) != 0) {
        if (nbytes > 0) {
            log(Log::Debug) << "got " << nbytes << " from local" << std::endl;
            if (zmq_send(s, sbuffer, nbytes, 0) < 0) {
                log(Log::Warning) << "error when trying to send a message to the irc thread (warning, we continue here!) :" << zmq_strerror(errno) << std::endl;
            }
        } else {
            log(Log::Error) << "error reading data from tun: " << strerror(errno) << std::endl;
        }
    }
};