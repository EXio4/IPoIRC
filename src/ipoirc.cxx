
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <confuse.h>
#include <zmq.h>

#include "irc.h"
#include "tun.h"
#include "config.h"
#include "ltun.h"
#include "helpers.h"
#include "ipoirc.h"

void debug(const char * format, ...) {
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char timestamp[128];
    strftime(timestamp, 127, "%y-%m-%d %H:%M:%S", loctime);

    char buffer[512];
    va_list args;
    va_start(args, format);

    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s | [main] %s\n", timestamp, buffer);

    va_end(args);
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

    char *netid         = NULL;
    char *nick          = NULL;
    char *net           = NULL;
    char *pass          = NULL;
    long int  port      = 6667;
    char *chan          = NULL;
    char *h1            = NULL;
    char *h2            = NULL;
    char *config        = NULL;
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
            case 'i': net       = optarg;
                break;
            case 'x': port      = atoi(optarg);
                break;
            case 'p': pass      = optarg;
                break;
            case 'c': chan      = optarg;
                break;
            case 'l': h1        = optarg;
                break;
            case 'r': h2        = optarg;
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

    if (config) {
        cfg_opt_t opts[] = {
            CFG_SIMPLE_STR("netid",    &netid),
            CFG_SIMPLE_STR("nick",     &nick),
            CFG_SIMPLE_STR("network",  &net),
            CFG_SIMPLE_STR("password", &pass),
            CFG_SIMPLE_STR("channel",  &chan),
            CFG_SIMPLE_STR("localip",  &h1),
            CFG_SIMPLE_STR("remoteip", &h2),
            CFG_SIMPLE_INT("port",     &port),
            CFG_SIMPLE_INT("uid",      &uid),
            CFG_SIMPLE_INT("gid",      &gid),
            CFG_SIMPLE_INT("threads",  &threads),
            CFG_END()
        };

        cfg_t *cfg = cfg_init(opts, CFGF_NONE);

        if (cfg_parse(cfg, config) != CFG_SUCCESS) {
            debug("error parsing config");
        }

        cfg_free(cfg);
    };


    if (threads > MAX_IRC_THREADS) {
        debug("WARNING: you can't define more than %d IRC threads, starting in single-thread mode\n", MAX_IRC_THREADS);
        threads = 1;
    }
    if (threads < 1)
        threads = 1;

    if (netid == NULL || nick  == NULL ||
        net   == NULL || chan  == NULL ||
        h1    == NULL || h2    == NULL)
            usage(argv[0]);

    ltun_t* tun_handle  = ltun_alloc("irc%d", MTU, h1, h2);

    if (!tun_handle) {
        debug("error starting the tun interface, are you root?");
        exit(1);
    }

    if (getuid() == 0 && gid != 0 && uid != 0) {
        if (setgid(gid) != 0 || setuid(uid) != 0) {
            debug("unable to drop privileges: %s", strerror(errno));
            exit(1);
        }
    }

    debug("running as %d", getuid());

    std::thread tun_th([zmq_context,tun_handle]() {
        return tun_thread(zmq_context, tun_handle);
    });

    sleep(1);


    for (i=0; i<threads; i++) {
        irc_closure cl;
        cl.xid     = i;
        cl.context = zmq_context;
        cl.netid   = netid; // "socket id" (used in irc <-> irc communication)
        cl.nick    = nick;
        cl.pass    = pass;
        cl.server  = net;
        cl.port    = port;
        cl.chan    = chan;
        cl.irc_s   = NULL;
        std::thread th([cl]() {
            return irc_thread(cl);
        });
        th.detach();
    }


    tun_th.join();

    return 0;
}
