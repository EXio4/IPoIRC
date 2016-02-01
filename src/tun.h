#ifndef IPOIRC_TUN_H
#define IPOIRC_TUN_H

#include "ltun.h"
#include "modules.h"

void tun_thread(void* zmq_context, const Tun& tun);

struct TunConfig {
    std::string local;
    std::string remote;
};

class TunModule : LocalModule<TunConfig, const Tun, Unit, const Tun&> {
    std::string module_name() noexcept { return "tun"; };
    HelpText help() noexcept { return
                                {{"local_ip" ,"Local IP" }
                                ,{"remote_ip","Remote IP"}
                                }; }; // any suggestions?
    boost::optional<TunConfig> config(YAML::Node cfg) noexcept {
        if (cfg["localip"] && cfg["remoteip"]) {
            return TunConfig
                     {.local  = cfg["localip" ].as<std::string>()
                     ,.remote = cfg["remoteip"].as<std::string>()};
        } else {
            return boost::none;
        }
    };

    const Tun priv_init(TunConfig);
    Unit      norm_init(TunConfig) { return Unit {}; };

    const Tun& start_thread(const Tun& x, Unit&) { return x; }

    void worker_reader(const Tun& tun, Comm::Socket s);
    void worker_writer(const Tun& tun, Comm::Socket s);
/* */
};

#endif
