#include <iostream>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <yaml-cpp/yaml.h>
#include <zmq.h>

#include "irc.h"
#include "tun.h"
#include "config.h"
#include "ltun.h"
#include "ipoirc.h"


std::ostream& debug() {
    return debug_gen("main");
}

void usage(char *h) {
    printf( "%s parameters:\n"
            "\t-m S [netid]\n"
            "\t\t it is the ID that identifies the computer in the IRC<>IRC protocol\n"
            "\t-n S [nick]\n"
            "\t\t nick used for the IRC bots, allows the usage of %%d to setup a random number in the nick\n"
            "\t-i S [network]\n"
            "\t\t the IRC network where the bot(s) will connect\n"
            "\t-x I [port]\n"
            "\t\t (*) port of the IRC server\n"
            "\t-p S [password]\n"
            "\t\t (*) password for connecting to the IRC server\n"
            "\t-c S [channel]\n"
            "\t\t channel where the bots will reside\n"
            "\t-l X [localip]\n"
            "\t\t local IP, used in the tun interface\n"
            "\t-r X [remoteip\n"
            "\t\t remote IP, used in the tun interface\n"
            "\t-U I [uid]\n"
            "\t-G I [gid]\n"
            "\t\t (*) User and Group ID, used to drop privs\n"
            "\t-t I [threads]\n"
            "\t\t IRC threads used (equal to the number of IRC bots connected to the IRC network)\n"
            "\t-f S\n"
            "\t\t (*) configuration file to read"
            "\n(*) means the parameter is optional (unless it is in the config file)\n",
            h);
    exit(1);
}

int main(int argc, char **argv) {

    srand(time(NULL));

    void* zmq_context = zmq_ctx_new();

    std::string netid         = "";
    std::string nick          = "";
    std::string network       = "";
    std::string password      = "";
    long int  port      = 6667;
    std::string channel       = "";
    std::string localip       = "";
    std::string remoteip      = "";
    std::string config        = "";
    long int  threads   = 1;
    long int  gid       = 0;
    long int  uid       = 0;

    int i               = 0;
    int c               = 0;

    while ((c = getopt (argc, argv, "hm:n:i:x:p:c:l:r:U:G:t:f:")) != -1) {
        switch (c) {
            case 'm': netid     = optarg;
                break;
            case 'n': nick      = optarg;
                break;
            case 'i': network   = optarg;
                break;
            case 'x': port      = atoi(optarg);
                break;
            case 'p': password  = optarg;
                break;
            case 'c': channel   = optarg;
                break;
            case 'l': localip   = optarg;
                break;
            case 'r': remoteip  = optarg;
                break;
            case 'U': uid       = atoi(optarg);
                break;
            case 'G': gid       = atoi(optarg);
                break;
            case 't': threads   = atoi(optarg);
                break;
            case 'f': config    = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    if (config != "") {
        YAML::Node cfg = YAML::LoadFile(config);
        netid    = cfg["netid"   ] ? cfg["netid"   ].as<std::string>() : netid    ;
        nick     = cfg["nick"    ] ? cfg["nick"    ].as<std::string>() : nick     ;
        network  = cfg["network" ] ? cfg["network" ].as<std::string>() : network  ;
        password = cfg["password"] ? cfg["password"].as<std::string>() : password ;
        channel  = cfg["channel" ] ? cfg["channel" ].as<std::string>() : channel  ;
        localip  = cfg["localip" ] ? cfg["localip" ].as<std::string>() : localip  ;
        remoteip = cfg["remoteip"] ? cfg["remoteip"].as<std::string>() : remoteip ;
        port     = cfg["port"    ] ? cfg["port"    ].as<int>        () : port     ;
        uid      = cfg["uid"     ] ? cfg["uid"     ].as<int>        () : uid      ;
        gid      = cfg["gid"     ] ? cfg["gid"     ].as<int>        () : gid      ;
        threads  = cfg["threads" ] ? cfg["threads" ].as<int>        () : threads  ;
    };


    if (threads > MAX_IRC_THREADS) {
        debug() << "WARNING: you can't define more than " << MAX_IRC_THREADS << " IRC threads, starting in single-thread mode" << std::endl;
        threads = 1;
    }
    if (threads < 1)
        threads = 1;

    if (netid    == "" || nick     == "" ||
        network  == "" || channel  == "" ||
        localip  == "" || remoteip == "")
            usage(argv[0]);

    try {
        Tun tun_handle("irc%d", MTU, localip, remoteip);


        if (getuid() == 0 && gid != 0 && uid != 0) {
            if (setgid(gid) != 0 || setuid(uid) != 0) {
                debug() << "unable to drop privileges: " << strerror(errno) << std::endl;
                exit(1);
            }
        }

        debug() << "running as " << getuid() << std::endl;

        std::thread tun_th([zmq_context,&tun_handle]() {
            return tun_thread(zmq_context, tun_handle);
        });

        sleep(1);


        for (i=0; i<threads; i++) {
            /* note: every thread gets its own copies of netid, nick, pass, etc */
            irc_closure cl;
            cl.xid     = i;
            cl.context = zmq_context;
            // netid, nick, chan, server are guaranteed to be NOT NULL
            cl.netid   = netid; // "socket id" (used in irc <-> irc communication)
            cl.nick    = nick;
            cl.chan    = channel;
            cl.server  = network;
            cl.pass    = password;
            cl.port    = port;
            cl.irc_s   = NULL;
            std::thread th([cl]() {
                return irc_thread(cl);
            });
            th.detach();
        }


        tun_th.join();
    } catch(TunError const &e) {
        debug() << "error setting up tun, are you root?" << std::endl;
    };

    return 0;
}
