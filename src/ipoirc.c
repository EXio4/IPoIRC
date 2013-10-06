#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
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
            "\t-m S\n"
            "\t\t it is the ID that identifies the computer in the IRC<>IRC protocol\n"
            "\t-n S\n"
            "\t\t nick used for the IRC bots, allows the usage of %%d to setup a random number in the nick\n"
            "\t-i S\n"
            "\t\t the IRC network where the bot(s) will connect\n"
            "\t-x I\n"
            "\t\t (*) port of the IRC server\n"
            "\t-p S\n"
            "\t\t (*) password for connecting to the IRC server\n"
            "\t-c S\n"
            "\t\t channel where the bots will reside\n"
            "\t-l X\n"
            "\t\t local IP, used in the tun interface\n"
            "\t-r X\n"
            "\t\t remote IP, used in the tun interface\n"
            "\t-U I\n"
            "\t-G I\n"
            "\t\t (*) User and Group ID, used to drop privs\n"
            "\t-t I\n"
            "\t\t IRC threads used (equal to the number of IRC bots connected to the IRC network)\n"
            "\n(*) means the parameter is optional\n",
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



    char *netid  = NULL;
    char *nick   = NULL;
    char *net    = NULL;
    char *pass   = NULL;
    int  port    = 6667;
    char *chan   = NULL;
    char *h1     = NULL;
    char *h2     = NULL;
    int  threads = 1;
    int  gid     = 0;
    int  uid     = 0;

    int i        = 0;
    int c        = 0;

    while ((c = getopt (argc, argv, "hm:n:i:x:p:c:l:r:U:G:t:")) != -1) {
        switch (c) {
            case 'm': netid = optarg;
                break;
            case 'n': nick = optarg;
                break;
            case 'i': net = optarg;
                break;
            case 'x': port = atoi(optarg);
                break;
            case 'p': pass = optarg;
                break;
            case 'c': chan = optarg;
                break;
            case 'l': h1 = optarg;
                break;
            case 'r': h2 = optarg;
                break;
            case 'U': uid = atoi(optarg);
                break;
            case 'G': gid = atoi(optarg);
                break;
            case 't': threads = atoi(optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    if (threads > MAX_IRC_THREADS) {
        debug("WARNING: you can't define more than %d IRC threads, starting in single-thread mode\n", MAX_IRC_THREADS);
        threads = 1;
    }
    if (threads < 1)
        threads = 1;

    if (netid == NULL ||
        nick  == NULL ||
        net   == NULL ||
        chan  == NULL ||
        h1    == NULL ||
        h2    == NULL)
      usage(argv[0]);

    // define shared data (context and thread id)
    for (i=0; i<threads; i++) {
        irc_data[i].d.id       = i;
        irc_data[i].d.context = context;
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
                debug("WARNING: IRC thread % failed to create", i);
            }
        }
    }


    pthread_join(tun_threadS, NULL);

    return 0;
}
