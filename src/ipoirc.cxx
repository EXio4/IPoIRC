#include <iostream>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sol/sol.hpp>
#include <zmq.h>

#include "local_helpers.h"
#include "irc.h"
#include "tun.h"
#include "net.h"
#include "config.h"
#include "ltun.h"
#include "ipoirc.h"


std::ostream& debug() {
    return debug_gen("main");
}

void usage(std::string self, const std::vector<CoreModule*> &mods) {
    std::cout << self << std::endl;
    std::cout << "# General" << std::endl;
    std::cout << "\t-c <file>" << std::endl;
    std::cout << "\t\tconfig file" << std::endl;

    std::cout << "# Config file fields" << std::endl;

    for (auto local : mods) {
        std::cout << "* " << local->module_name() << " settings" << std::endl;
        for (auto& v : local->help()) {
            std::cout << "\t"   << v[0] << std::endl;
            std::cout << "\t\t" << v[1] << std::endl;
        }
    }

    exit(1);
}


template <typename LocalCFG, typename LocalC1, typename LocalC2, typename LocalT>
void program(sol::table& cfg, LocalModule<LocalCFG, LocalC1, LocalC2, LocalT>& local) {

    void *zmq_context = zmq_ctx_new();

    debug() << "Loading general settings" << std::endl;
    int uid = cfg.get<int>("uid");
    int gid = cfg.get<int>("gid");

    debug() << "Loading " << local.module_name() << " config" << std::endl;
    LocalCFG local_cfg = local.config(cfg.get<sol::table>(local.module_name()));

    LocalC1 l_c1 = local.priv_init(local_cfg);

    if (getuid() == 0 && gid != 0 && uid != 0) {
        if (setgid(gid) != 0 || setuid(uid) != 0) {
            debug() << "unable to drop privileges: " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    LocalC2 l_c2 = local.norm_init(local_cfg);

    LocalT loc = local.start_thread(l_c1, l_c2);

    debug() << "running as " << getuid() << std::endl;

    std::thread tun_th([zmq_context,&local,&loc]() {
        std::thread zmq_th([zmq_context,&local,&loc]() {
            void *socket = zmq_socket(zmq_context, ZMQ_PULL);
            if (!socket) return;

            if (zmq_bind(socket, "inproc://#irc_to_#tun")) {
                loc_debug() << "(tun_thread_zmq) error when connecting to IPC socket - " << zmq_strerror(errno) << std::endl;
                zmq_close(socket);
                return;
            }
            local.worker_reader(loc, socket);
            zmq_close(socket);
        });

        {
            void *socket = zmq_socket(zmq_context, ZMQ_PUSH);
            if (!socket) return;

            if (zmq_bind(socket, "inproc://#tun_to_#irc")) {
                loc_debug() << "error when creating IPC socket - " << zmq_strerror(errno) << std::endl;
                zmq_close(socket);
                return;
            }
            local.worker_writer(loc, socket);
            zmq_close(socket);
        }
        zmq_th.join();
    });

    sleep(1);


    debug() << "Loading IRC Config" << std::endl;
    sol::table c = cfg.get<sol::table>("irc");
    int threads = c.get<int>("threads");
    for (int i=0; i < threads; i++) {
        irc_closure cl;
        cl.xid     = i;
        cl.context = zmq_context;
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


    tun_th.join();

}

int main(int argc, char **argv) {

    srand(time(NULL));

    std::string config;

    TunModule TUN;
    NetModule NET;

    std::vector<CoreModule*> modules { &TUN
                                     , &NET
                                     };

    {   char c;
        while ((c = getopt (argc, argv, "c:")) != -1) {
            if (c == 'c') config = optarg;
        }
    }

    if (config == "") {
        usage(argv[0], modules);
        return 0;
    }

    try {
        sol::state lua;
        lua.open_file(config);
        sol::table cfg = lua.get<sol::table>("config");
        // how could we pick the "local" module at runtime without a kick-ass?
        program(cfg, TUN);
    } catch(TunError const &e) {
        debug() << "error setting up tun, are you root?" << std::endl;
    } catch(sol::error const &e) {
        debug() << e.what() << std::endl;
        usage(argv[0], modules);
    };

    return 0;
}
