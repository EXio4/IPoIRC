#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <thread>

#include "comm.h"
#include "ipoirc.h"
#include "config.h"
#include "ltun.h"
#include "tun.h"

void TunModule::worker_reader(std::shared_ptr<Tun> tun, Comm::Socket socket) const {
    while (1) {
        std::vector<uint8_t> buffer = socket->recv();
        log(Log::Debug) << "got " << buffer.size() << " bytes" << std::endl;
        tun->write(buffer);
    }
};
void TunModule::worker_writer(std::shared_ptr<Tun> tun, Comm::Socket socket) const {
    while (1) {
        std::vector<uint8_t> buffer = tun->read();
        log(Log::Debug) << "got " << buffer.size() << " from tun" << std::endl;
        socket->send(buffer);
    }
};

