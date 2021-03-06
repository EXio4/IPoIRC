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

struct TunModuleT {
    using Config = TunConfig;
    using Priv   = std::shared_ptr<Tun>;
    using Norm   = Unit;
    using State  = std::shared_ptr<Tun>;
};

class TunModule : public LocalModule<TunModuleT> {
    std::string module_name() const noexcept  { return "tun"; };
    HelpText help() const noexcept { return
                                {{"inet_name","Interface name (should have placeholder '%d')"}
                                ,{"local_ip" ,"Local IP" }
                                ,{"remote_ip","Remote IP"}
                                }; }; // any suggestions?
    TunConfig config(sol::table cfg) const {
        return TunConfig
                    {.inet_name = cfg.get<std::string>("inet_name")
                    ,.local     = cfg.get<std::string>("local_ip" )
                    ,.remote    = cfg.get<std::string>("remote_ip")};
    }


    std::shared_ptr<Tun> priv_init(TunConfig cfg) const { return std::shared_ptr<Tun>(new Tun(cfg.inet_name, MTU, cfg.local, cfg.remote)); };
    Unit                 norm_init(TunConfig    ) const { return Unit {}; };

    std::shared_ptr<Tun> start_thread(std::shared_ptr<Tun> x, Unit) const { return x; }

    void worker_reader(std::shared_ptr<Tun> tun, Comm::Socket s) const;
    void worker_writer(std::shared_ptr<Tun> tun, Comm::Socket s) const;
/* */
};

#endif
