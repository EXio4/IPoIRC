/*
 *      ip_over_irc.c - A simple implementation of IPoIRC for Linux
 *
 *      Copyright (C) 2012 EXio4 <exio4.com@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#define _GNU_SOURCE 1

#ifndef LEVEL
#define LEVEL 9999
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <getopt.h>
#include <sys/ioctl.h>

// Base64 and MD5 libs
#include <libs/crypt/base64.h>
#include <libs/crypt/md5.h>

// error codes
#define PERROR(x) do { perror(x); exit(1); } while (0)
#define ERROR(x, args ...) do { fprintf(stderr,"ERROR:" x, ## args); exit(1); } while (0)

#define MAXCMD 1024
#define BUFSIZE 512

// bool semantic type
typedef enum { false, true } bool;

// irc line
typedef struct {
    char rawmsg[BUFSIZE];
    char nick[64];
    char ident[32];
    char host[128];
    char from[256];
    char command[32];
    char channel[128];
    char message[BUFSIZE];
} irc_line;

typedef struct {
    char *commands[MAXCMD];
    void (*run[MAXCMD])(irc_conn);
} irc_cmd;

typedef struct {
    char host[128];             // hostname of the ircserver
    unsigned short port;        // port to connect (For now only Non-SSL connections are available)
    int ssl;                    // not used
    char nick[64];              // bot's nick
    char ident[32];             // Ident for use (only if identd isn't running in system)
    char realname[128];         // default realname
    char password[128];         // server password, in freenode you can set it to "your nickserv password"
#ifdef SASL
    int sasl_mechanism;         // now only support PLAIN (1)
    char sasl_username[128];
    char sasl_password[128];
#endif
#ifdef AUTORUN
    char autorun[BUFSIZE];          // some autorun command, for now only valid ONE RAW COMMAND
#endif
    char channel[BUFSIZE];          // channel('s) to connect
    char channel_key[BUFSIZE];      // key of it
} irc_reg;

typedef struct {
    int sock;                   // used for internal purposes
    irc_reg *server;
    char nick[64];              // bot's CURRENT nick
    irc_line *line;
    irc_cmd *cmd;
} irc_conn;

typedef struct {
    char *hash;
    char *data;
} irc_packet;

// "Random" MD5 Hash used to identify the connection/bot
char *hash;

char REQ[BUFSIZE];
int usng=0;

void DEBUG(int level,char *fmt, ...)
{
#ifdef NDEBUG
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr,fmt,args);
    va_end(args);
#endif
    return;
}

/*
 * Read up to BUFSIZE bytes from socket
 */
int socket_read(int sock, char** out)
{
    /* current buffer size */
    unsigned int size = BUFSIZE;
    /* current location in buffer */
    int i = 0;
    /* whether the previous char was a \r */
    int cr = 0;
    int tmp;
    char ch;

    /* result, grows as we need it to */
    *out = malloc(sizeof(char) * size);

    for ( ; ; ) {
        tmp=recv(sock, &ch, 1, 0);
        if (tmp < 0) {
            return -1;
        }
        if (tmp==0) return -2;
        if (i >= size) {
            /* grow buffer */
            size *= 2;
            *out = realloc(*out, size);
        }

        if (ch == '\n') {
            /* if preceded by a \r, we overwrite it */
            if (cr) {
                i--;
            }

            (*out)[i] = '\0';
            break;

        } else {
            cr = (ch == '\r' ? 1 : 0);
            (*out)[i] = ch;
        }

        i++;
    }
    return i + 1;
}

/*
 * Allocate and initialize a new irc_conn
 */
irc_conn* NewIRC(void)
{
#ifndef NULL
    int i;
#endif
    irc_conn *pos=malloc(sizeof(irc_conn));
    pos->server=malloc(sizeof(irc_reg));
    pos->cmd=malloc(sizeof(irc_cmd));
    pos->line=malloc(sizeof(irc_line));
#ifndef NULL
    for(i=0; i<MAXCMD; i++) {
        pos->cmd->commands[i]==NULL;
    }
#endif
    pos->sock = -1;
    pos->server->port = 6667;
#ifdef SASL
    pos->server->sasl_mechanism = 0;
#endif
    return pos;
}

/*
 * Send RAW commands to the socket (and debug it)
 * @param client (stablished) irc connection
 * @param string "format" (printf-like) to send
 * @return chars writed to the server
 */
int irc_raw(irc_conn *client, char *fmt, ...)
{
    if (client->sock<0) {
        DEBUG(10,"[error] sock = %d\n",client->sock);
        return client->sock;
    }
    char buffer[1024];
    int tmp;
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer,1024,fmt,args);
    DEBUG(100,"[raw] %s >>> %s\n",client->server->host,buffer);
    va_end(args);
    tmp=write(client->sock, buffer, strlen(buffer));
    tmp=tmp+write(client->sock,"\r\n",2);
    return tmp;
}

/*
 * Identify the client (PASS,NICK,USER [and|or] SASL), send autorun cmd and join to channels
 * @param "client" irc data with the host, password, and other data.
 * @return negative int in error, 0 or positive if it success
 */
int irc_start(irc_conn *client)
{

    if (!client) return -1;
    if (!client->server->host) return -2;
    if (!client->server->port) return -2;
    if (!client->server->nick) return -3;
    if (client->server->ident[0]=='\0') strncpy(client->server->ident, client->server->nick,sizeof(client->server->ident));
    if (client->server->realname[0]=='\0') strncpy(client->server->realname, client->server->nick,sizeof(client->server->realname));
#ifdef SASL
    if (client->server->sasl_mechanism>0) {
        // SASL enabled
        if (!client->server->sasl_username || !client->server->sasl_password) return -4;
    }
#endif
    struct sockaddr_in ircserver;
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return -10;
    memset(&ircserver, 0, sizeof(ircserver));
    ircserver.sin_family = AF_INET;
    ircserver.sin_addr.s_addr = inet_addr(client->server->host);
    ircserver.sin_port = htons(client->server->port);
    if (connect(sock, (struct sockaddr *) &ircserver, sizeof(ircserver)) < 0) return -11;
    client->sock = sock;
    if (strlen(client->server->password)>0) {
        irc_raw(client,"PASS %s",client->server->password);
    }
#ifdef SASL
    if (client->server->sasl_mechanism>0) {
        irc_raw(client,"CAP REQ SASL"); // Na, first need a PARSER for irc commands,
    }
#endif
    irc_raw(client,"USER %s %s 0 :%s",client->server->ident,client->server->host,client->server->realname);
    irc_raw(client,"NICK %s",client->server->nick);
    strncpy(client->nick,client->server->nick,sizeof(client->nick));
#ifdef SASL
    if (client->server->sasl_mechanism>0) {
        irc_raw(client,"CAP END");
    }
#endif
#ifdef AUTORUN
    if (client->server->autorun) {
        irc_raw(client,"%s",client->server->autorun);
    }
#endif
    return 0;
}

/*
 * Send QUIT to the server and close the socket
 * @param client - (stablished) irc_connection
 * @param quitmessage - Quit message sended to server (or NULL)
 * @return chars writed to the socket
 */
int irc_quit(irc_conn *client,char *quitmessage)
{
    int temp;
    if (!quitmessage)
        temp=irc_raw(client,"QUIT :");
    else
        temp=irc_raw(client,"QUIT :%s",quitmessage);

    close(client->sock);
    client->sock=-1;

    return temp;
}

/*
 * Some basic commands
 */
int irc_privmsg(irc_conn *client,char *chan,char *message)
{
    return irc_raw(client,"PRIVMSG %s :%s",chan,message);
}

int irc_notice(irc_conn *client,char *chan,char *message)
{
    return irc_raw(client,"NOTICE %s :%s",chan,message);
}

int irc_join(irc_conn *client,char *chan)
{
    return irc_raw(client,"JOIN %s",chan);
}

int irc_part(irc_conn *client,char *chan,char *message)
{
    return irc_raw(client,"PART %s :%s",chan,message);
}

int irc_mode(irc_conn *client,char *target,char *modes)
{
    return irc_raw(client,"MODE %s %s",target,modes);
}

int irc_umode(irc_conn *client,char *modes)
{
    return irc_mode(client,client->nick,modes);
}

int irc_parse(irc_conn *client)
{

    sscanf(client->line->rawmsg, ":%255s %31s %127s :%511[^\n]",
           client->line->from,client->line->command,client->line->channel,client->line->message);

    sscanf(client->line->from,":%63[^!]!%31[^@]@%128s",
           client->line->nick,client->line->ident,client->line->host);

    return 0;
}

int irc_read(irc_conn *client)
{
    char *smp;
    int i;
    i=socket_read(client->sock,&smp);
    strncpy(client->line->rawmsg,smp,sizeof(client->line->rawmsg));
    return i;
}

int irc_run(irc_conn *client)
{
    int i, event = -1;
    char ping[BUFSIZE] = "";

    sscanf(client->line->rawmsg, "PING :%511s", ping);

    if (strlen(ping) > 0) {
        // ping... pong!
        irc_raw(client,"PONG %s",ping);
        DEBUG(40,"ping, pong!");
    }

    for(i=0; i<MAXCMD; i++) {
        if (!(client->cmd->commands[i]==NULL)) {
            if (strcmp(client->line->command,client->cmd->commands[i])==0) {
                if (!client->cmd->run[i]) continue;
                (*client->cmd->run[i])(client);
                event++;
            }
        }
    }

    if (event<0) {
        // TODO: Implement "unknown command" trigger
        return -1;
    }

    return (event+1);
}

void irc_free(irc_conn *client)
{
    /*  int i;
        for (i=0;i<MAXCMD;i++) {
            if (client->cmd->commands[i]) free(client->cmd->commands[i]);
            if (client->cmd->run[i]) free(client->cmd->run[i]);
        } */
    free(client->server);
    free(client->cmd);
    free(client->line);
    free(client);
    client=NULL;
}

int irc_attach(irc_conn *client,char *command, void (*function))
{
    int i;
    for(i=0; i<MAXCMD; i++) {
        if (client->cmd->commands[i]==NULL) {
            DEBUG(400,"Attaching function to \"%s\" in \"%s\"\n",command,client->server->host);
            client->cmd->commands[i]=command;
            client->cmd->run[i]=function;
            return i;
        }
    }
    return -1;
}

void updatehash()
{
    srand(time(NULL)*getpid());
    char *temphash=malloc(sizeof(char)*32);
    snprintf(temphash,sizeof(temphash),"%d",rand());
    hash=genmd5(temphash);
    free(temphash);
}

int irc_bin_req(irc_conn *client,char *chan, char *nick,char *hash)
{
    return irc_raw(client,"PRIVMSG %s :%s request %s",chan,nick,hash);
}

void join (irc_conn *client)
{
    irc_join(client,client->server->channel);

    if (strlen(REQ)>0) {
        irc_bin_req(client,client->server->channel,REQ,hash);
    }
}

int irc_bin_send(irc_conn *client,char *chan, char *hash,char *msg,int maxperline, int len)
{
    if (!client) return -1;
    char *b64enc;
    int i,max;
    char *tmpmsg;
    max=maxperline;
    DEBUG(25,"~~data: len=%d max=%d\n",len,max);
    for(i=0; i<len; i+=max) {
        if (i+max>len) {

            max=len-i;
        }
        base64_encode_alloc (msg+i,max,&b64enc);
        DEBUG(25,"~base64 result: %s\n",b64enc);
        irc_raw(client,"PRIVMSG %s :%s %s",chan,hash,b64enc);
        //free(tmpmsg);tmpmsg=NULL;
    }
//      free(tmpmsg);
//      free(b64enc);
    return 0;
};

int irc_bin_read(irc_conn *client,irc_packet *out)
{
    char *encd;
    char hash[128];
    char msg[BUFSIZE];
    int len;
    if(strcmp(client->line->command,"PRIVMSG")!=0) return -1;
    sscanf(client->line->message,"%127s %511s",&hash,&msg);
    if (strcmp(out->hash,hash)!=0) return 0; // invalid hash
    base64_decode_alloc(msg,strlen(msg),&encd,&len);
    if (encd==NULL)  return 0;
    out->data=encd;
    return strlen(out->data);
}

void parse_packet(irc_conn *client)
{
    irc_packet *spack=malloc(sizeof(irc_packet));
    spack->hash=hash;
    if (irc_bin_read(client,spack)>0) {
        DEBUG(25,"<ip over irc data with valid hash> %s\n",spack->data);
        puts(spack->data);
        //free(spack->data);
    }
    //free(spack);
}

void parse_packet2(irc_conn *client)
{
    char nick[BUFSIZE]="";
    char hash_request[BUFSIZE]="";
    sscanf(client->line->message,"%511s request %511s",&nick,&hash_request);
    if (strlen(nick)>0&&strlen(hash_request)>0) {
        if (strcmp(nick,client->nick)!=0) return;
        if (usng==0) {
            irc_raw(client,"PRIVMSG %s :ok",client->line->channel);
            strncpy(hash,hash_request,sizeof(hash));
            usng=1;
        } else {
            irc_raw(client,"PRIVMSG %s :no",client->line->channel);
        }
    }
}

void usage()
{
    fprintf(stderr, "Usage: ipoirc [nick] [ip] [serverpassword] [channel] @servernick\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc<5) {
        usage();
    }

    struct ifreq ifr;
    int fd, l;
    char buf[1500]="";

    DEBUG(25,"creating IRC connection...\n");
    irc_conn *client = NewIRC();

    strcpy(client->server->nick, argv[1]);
    strcpy(client->server->realname,hash);
    strcpy(client->server->host,argv[2]);
    strcpy(client->server->password,argv[3]);
    strcpy(client->server->channel,argv[4]);

    if (argc>5) {
        strcpy(REQ,argv[5]);
    }

    if (irc_start(client)<0) {
        return -1;
    }

    DEBUG(25,"creating hash...\n");
    updatehash();
    irc_packet *spack=malloc(sizeof(irc_packet));
    spack->hash=hash;


    int TUNMODE = IFF_TUN;
    if ( (fd = open("/dev/net/tun",O_RDWR)) < 0) PERROR("open");
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = TUNMODE;
    strncpy(ifr.ifr_name, "irc%d", IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) PERROR("ioctl");

    printf("Allocated interface %s. Configure and use it\n", ifr.ifr_name);

    irc_attach(client,"396",&join);
    irc_attach(client,"PRIVMSG",&parse_packet);
    irc_attach(client,"PRIVMSG",&parse_packet2);

    fd_set fdset;
    while (1) {
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        FD_SET(client->sock, &fdset);
        if (select(fd+client->sock+1, &fdset,NULL,NULL,NULL) < 0) PERROR("select");
        if (FD_ISSET(fd, &fdset)) {
            write(1,">", 1);

            l = read(fd,buf, sizeof(buf));
            if (l < 0) PERROR("read");
//          if (sendto(s, buf, l, 0, (struct sockaddr *)&from, fromlen) < 0) PERROR("sendto");

            irc_bin_send(client,client->server->channel,hash,buf,64,l);
        } else {
            write(1,"<", 1);

            l=irc_read(client);
            if(l<1) return 1;

            irc_parse(client);
            irc_run(client);

            l = irc_bin_read(client, spack);
            if (l<1) continue;

            if (write(fd, spack->data, l) < 0) PERROR("write");
            //free(spack->data);
        }
    }
}

