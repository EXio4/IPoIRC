#include <iostream>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sol/sol.hpp>

#include "zmq-comm.h"
#include "log.h"
#include "irc.h"
#include "tun.h"
#include "net.h"
#include "config.h"
#include "ltun.h"
#include "ipoirc.h"
#include "modexists.h"


std::ostream& log(Log::Level l) {
    return Log::gen("main", l);
}

template <typename T>
void usage(std::string self, const std::vector<std::shared_ptr<T>> &mods) {
    std::cout << self << std::endl;
    std::cout << "# General" << std::endl;
    std::cout << "\t-c <file>" << std::endl;
    std::cout << "\t\tconfig file" << std::endl;

    std::cout << "# Config file fields" << std::endl;

    /* missing:
     *  gid
     *  uid
     *  local_module
     */

    for (auto local : mods) {
        std::cout << "* " << local->module_name() << " settings" << std::endl;
        for (auto& v : local->help()) {
            std::cout << "\t"   << v[0] << std::endl;
            std::cout << "\t\t" << v[1] << std::endl;
        }
    }

    exit(1);
}


void program(sol::table& cfg, std::shared_ptr<EX::Local_Config> local_s1) {

    Comm::Ctx ctx = ZMQComm::create();

    log(Log::Info) << "Loading general settings" << std::endl;
    int uid = cfg.get<int>("uid");
    int gid = cfg.get<int>("gid");

    log(Log::Info) << "Loading " << local_s1->module_name() << " config" << std::endl;
    auto local_s2 = local_s1->config(cfg.get<sol::table>(local_s1->module_name()));

    auto local_s3 = local_s2->priv_init();

    if (getuid() == 0 && gid != 0 && uid != 0) {
        if (setgid(gid) != 0 || setuid(uid) != 0) {
            log(Log::Fatal) << "unable to drop privileges: " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    auto local_s4 = local_s3->norm_init();

    auto local_s5 = local_s4->start_th();

    log(Log::Info) << "running as " << getuid() << std::endl;

    std::thread net_writer_th([ctx,&local_s5]() {
        std::thread net_recv_th([ctx,&local_s5]() {
            Comm::Socket socket = ctx->connect(Comm::Net2Chat);
            local_s5->worker_reader(socket);
        });

        {
            Comm::Socket socket = ctx->connect(Comm::Chat2Net);
            local_s5->worker_writer(socket);
        }
        net_recv_th.join();
    });

    sleep(1);


    log(Log::Info) << "Loading IRC Config" << std::endl;
    sol::table c = cfg.get<sol::table>("irc");
    int threads = c.get<int>("threads");
    for (int i=0; i < threads; i++) {
        irc_closure cl;
        cl.xid     = i;
        cl.ctx     = ctx;
        cl.netid   = c.get<std::string>("netid"   );
        cl.nick    = c.get<std::string>("nick"    );
        cl.chan    = c.get<std::string>("channel" );
        cl.server  = c.get<std::string>("network" );
        cl.pass    = c.get<std::string>("password");
        cl.port    = c.get<int>        ("port"    );
        cl.irc_s   = NULL;
        std::thread th([cl]() {
            return irc_thread(cl);
        });
        th.detach();
    }


    net_writer_th.join();

}

int main(int argc, char **argv) {

    srand(time(NULL));

    std::string config;

    TunModule TUN;
    NetModule NET;

    std::vector<std::shared_ptr<EX::Local_Config>> l_modules { EX::local_module(&NET)
                                                             , EX::local_module(&TUN) };
    {   char c;
        while ((c = getopt (argc, argv, "c:")) != -1) {
            if (c == 'c') config = optarg;
        }
    }

    if (config == "") {
        usage(argv[0], l_modules);
        return 0;
    }

    try {
        sol::state lua;
        lua.open_file(config);
        sol::table cfg = lua.get<sol::table>("config");

        std::shared_ptr<EX::Local_Config> local = nullptr;
        std::string local_module = cfg.get<std::string>("local_module");
        for (auto& l_m : l_modules)  {
            if (l_m->module_name() == local_module) {
                log(Log::Info) << "Using " << local_module << std::endl;
                local = l_m;
                break;
            }
        }
        if (local == nullptr) {
            log(Log::Warning) << "No module matched, using fallback " << l_modules[0]->module_name() << " module" << std::endl;
            local = l_modules[0];
        }
        program(cfg, local);
    } catch(TunError const &e) {
        log(Log::Fatal) << "error setting up tun, are you root?" << std::endl;
    } catch(sol::error const &e) {
        log(Log::Fatal) << e.what() << std::endl;
        usage(argv[0], l_modules);
    };

    return 0;
}