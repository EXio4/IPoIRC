#ifndef IPOIRC_TUN_H
#define IPOIRC_TUN_H

#include "ltun.h"
#include "config.h"
#include "modules.h"

struct TunConfig {
    std::string inet_name;
    std::string local;
    std::string remote;
};

class TunModule : public LocalModule<TunConfig, Tun, Unit, Tun&> {
    std::string module_name() noexcept { return "tun"; };
    HelpText help() noexcept { return
                                {{"inet_name","Interface name (should have placeholder '%d')"}
                                ,{"local_ip" ,"Local IP" }
                                ,{"remote_ip","Remote IP"}
                                }; }; // any suggestions?
    TunConfig config(sol::table cfg) {
        return TunConfig
                    {.inet_name = cfg.get<std::string>("inet_name")
                    ,.local     = cfg.get<std::string>("local_ip" )
                    ,.remote    = cfg.get<std::string>("remote_ip")};
    }


    Tun   priv_init(TunConfig cfg) { return Tun(cfg.inet_name, MTU, cfg.local, cfg.remote); };
    Unit  norm_init(TunConfig    ) { return Unit {}; };

    Tun& start_thread(Tun& x, Unit&) { return x; }

    void worker_reader(Tun& tun, Comm::Socket s);
    void worker_writer(Tun& tun, Comm::Socket s);
/* */
};

#endif
