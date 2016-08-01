
#include <iostream>
#include "net.h"
#include "log.h"
#include "utils.h"

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

void NetModule::worker_reader(Net net, Comm::Socket socket) const {

    while (1) {
        std::vector<uint8_t> buffer = socket->recv();
        log(Log::Debug) << "got " << buffer.size() << " bytes" << std::endl;
        write(net.c_fd, buffer.data(), buffer.size());
    }
};
void NetModule::worker_writer(Net net, Comm::Socket socket) const {
    char sbuffer[MTU];

    int nbytes = -1;
    while ((nbytes = read(net.c_fd, sbuffer, MTU)) != 0) {
        if (nbytes > 0) {
            log(Log::Debug) << "got " << nbytes << " from local" << std::endl;
            socket->send(Utils::from_ptr(sbuffer, nbytes));
        } else {
            log(Log::Error) << "error reading data from tun: " << strerror(errno) << std::endl;
        }
    }
};