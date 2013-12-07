#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <confuse.h>
#include <zmq.h>

#include "irc.h"
#include "tun.h"
#include "config.h"
#include "ltun.h"
#include "helpers.h"
#include "ipoirc.h"

void debug(char * format, ...) {
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
            "\t-r X [remoteip\\n"
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

    void* context = zmq_ctx_new();

    irc_thread_data irc_data[MAX_IRC_THREADS];
    tun_thread_data tun_data;

    pthread_t irc_threads[MAX_IRC_THREADS];
    pthread_t tun_threadS;



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

    // define shared data (context and thread id)
    for (i=0; i<threads; i++) {
        irc_data[i].d.id       = i;
        irc_data[i].d.context  = context;
    }

    tun_data.d.id      = -1;
    tun_data.d.context = context;

    for (i=0; i<threads; i++) {
        irc_data[i].netid   = netid; // "socket id" (used in irc <-> irc communication)
        irc_data[i].nick    = nick;
        irc_data[i].pass    = pass;
        irc_data[i].server  = net;
        irc_data[i].port    = port;
        irc_data[i].chan    = chan;
        irc_data[i].irc_s   = NULL;
    }

    int rc = 0;

    tun_data.tun = (void*)ltun_alloc("irc%d", MTU, h1, h2);

    if (!tun_data.tun) {
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

    rc = pthread_create(&tun_threadS, NULL, tun_thread, &tun_data);
    if (rc) {
        debug("error creating tun thread, fatal!");
        exit(1);
    }

    sleep(1);


    for (i=0; i<threads; i++) {
        rc = pthread_create(&irc_threads[i], NULL, irc_thread, &irc_data[i]);
        if (rc) {
            if (i == 0) {
                debug("main IRC thread (%d) failed to create, fatal!", i);
                exit(1);
            } else {
                debug("WARNING: IRC thread %d failed to create", i);
            }
        }
    }


    pthread_join(tun_threadS, NULL);

    return 0;
}
