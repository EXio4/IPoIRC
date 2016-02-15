#ifndef IPOIRC_NET_H
#define IPOIRC_NET_H

#include "config.h"
#include "modules.h"

struct NetConfig {
    int port;
};

struct Net {
    int l_fd;
    int c_fd;
};

struct NetModuleT {
    using Config = NetConfig;
    using Priv   = Net;
    using Norm   = Unit;
    using State  = Net;
};


class NetModule : public LocalModule<NetModuleT> {
    std::string module_name() noexcept { return "net"; };
    HelpText help() noexcept { return
                                {{"port", "Local port (to listen to)"}
                                }; }; // any suggestions?
    NetConfig config(sol::table cfg) {
        return NetConfig {.port = cfg.get<int>("port")};
    }


    Net   priv_init(NetConfig cfg);
    Unit  norm_init(NetConfig    ) { return Unit {}; };

    Net start_thread(Net& x, Unit&) { return x; };

    void worker_reader(Net net, Comm::Socket s);
    void worker_writer(Net net, Comm::Socket s);
/* */
};

#endif
