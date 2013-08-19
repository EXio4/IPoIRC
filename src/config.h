#ifndef _CONFIG_H
#define _CONFIG_H

// IRC Network to where the bot(s) will connect
#define IRC_NETWORK "irc.freenode.net"

// password for the irc network (use 0 if that doesn't apply for you)
#define IRC_PASSWD 0

// IRC base nick (printf-like format for printing the random value)
#define IRC_NICK "testbot%d"

// IRC channel where the bot(s) will talk
#define IRC_CHANNEL "######"

// internals

// mtu you will use

#define MTU 1500

// "final" data per line

#define FLINE 64

#endif
