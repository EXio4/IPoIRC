/*
 *		ip_over_irc.c - A simple implementation of IPoIRC for Linux
 *
 *		Copyright (C) 2012 EXio4 <exio4.com@gmail.com>
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

#define MAXCMD 1024

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
// BASE64  (encode-decode) library.
#include <libs/crypt/base64.h>

// MD5
#include <libs/crypt/md5.h> 


// "Random" MD5 Hash used to identify the connection/bot
char *hash;
typedef struct {
	char rawmsg[512];
	char nick[64];
	char ident[32];
	char host[128];
	char from[256];
	char command[32];
	char channel[128];
	char message[512];
} irc_line;
typedef struct {
	char *commands[MAXCMD];
	void (*run[MAXCMD])(irc_conn); 
} irc_cmd;
typedef struct {
	char	host[128]; // hostname of the ircserver 
	unsigned short port; // port to connect (For now only Non-SSL connections are available)
	int ssl; // no used
	char	nick[64]; // Nick of the bot
	char	ident[32]; // Ident for use (only if identd isn't running in system)
	char	realname[128]; // Default realname
	char	password[128]; // Server password, in freenode you can set it to "your nickserv password"
#ifdef SASL
	int 	sasl_mechanism; // now only support PLAIN (1) 
	char	sasl_username[128];
	char	sasl_password[128];
#endif
#ifdef AUTORUN 
	char	autorun[512]; // Some autorun command, for now only valid ONE RAW COMMAND 
#endif
	char	channel[512]; // Channel('s) to connect
	char	channel_key[512]; // Key of it
} irc_reg;
typedef struct {
	int		sock; // used for internal purposes
	irc_reg *server;
	char nick[64]; // "actual" Nick of the bot
	irc_line *line;
	irc_cmd *cmd;
} irc_conn;

irc_conn* NewIRC(void) {
#ifndef NULL
	int i;
#endif
	irc_conn *pos=malloc(sizeof(irc_conn));
	pos->server=malloc(sizeof(irc_reg));
	pos->cmd=malloc(sizeof(irc_cmd));
	pos->line=malloc(sizeof(irc_line));
#ifndef NULL
	for(i=0;i<MAXCMD;i++) {
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
char REQ[512];

void DEBUG(int level,char *fmt, ...) {
#ifdef NDEBUG
	va_list args;
	va_start(args, fmt);
   	vfprintf(stderr,fmt,args);
   	va_end(args);
#endif
   	return;
}

typedef struct {
	char *hash;
	char *data;
} irc_packet;

/*
 * Send RAW commands to the socket (and debug it)
 * @param client (stablished) irc connection
 * @param string "format" (printf-like) to send
 * @return chars writed to the server
 */

int usng=0;
char SAY1[512];
int IRC_RAW(irc_conn *client, char *fmt, ...) {
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
 * Identify the client (PASS,NICK,USER [and|or] SASL), send autorun cmd and join in channels
 * @param "client" irc data with the host, password, and other data.
 * @return negative int in error, 0 or positive if it success
 */

int IRC_START(irc_conn *client) {

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
		IRC_RAW(client,"PASS %s",client->server->password);
	}
#ifdef SASL
	if (client->server->sasl_mechanism>0) { 
		IRC_RAW(client,"CAP REQ SASL"); // Na, first need a PARSER for irc commands, 
	}
#endif
	IRC_RAW(client,"USER %s %s 0 :%s",client->server->ident,client->server->host,client->server->realname);
	IRC_RAW(client,"NICK %s",client->server->nick);
	strncpy(client->nick,client->server->nick,sizeof(client->nick));
#ifdef SASL
	if (client->server->sasl_mechanism>0) {
			IRC_RAW(client,"CAP END");
	}
#endif
#ifdef AUTORUN
	if (client->server->autorun) { 
			IRC_RAW(client,"%s",client->server->autorun);
	}
#endif
	return 0;
}
/*
 * Send QUIT to the server al close the socket
 * @param client - (stablished) irc_connection
 * @param quitmessage - Quit message sended to server (or NULL)
 * @return chars writed to the socket
 */

int IRC_QUIT(irc_conn *client,char *quitmessage) {
	int temp;
	if (!quitmessage)
		temp=IRC_RAW(client,"QUIT :"); 
	else
		temp=IRC_RAW(client,"QUIT :%s",quitmessage);
	close(client->sock);
	client->sock=-1;
	return temp; 
}

/*
 * Some "BASIC" commands
 */

int IRC_PRIVMSG(irc_conn *client,char *chan,char *message) {
	return IRC_RAW(client,"PRIVMSG %s :%s",chan,message);
}

int IRC_NOTICE(irc_conn *client,char *chan,char *message) {
	return IRC_RAW(client,"NOTICE %s :%s",chan,message);
}

int IRC_JOIN(irc_conn *client,char *chan) {
	return IRC_RAW(client,"JOIN %s",chan);
}

int IRC_PART(irc_conn *client,char *chan,char *message) {
	return IRC_RAW(client,"PART %s :%s",chan,message);
} 

int IRC_MODE(irc_conn *client,char *target,char *modes) {
	return IRC_RAW(client,"MODE %s %s",target,modes);
}

int IRC_UMODE(irc_conn *client,char *modes) {
	return IRC_MODE(client,client->nick,modes);
}

int IRC_PARSE(irc_conn *client) {

	sscanf(client->line->rawmsg, ":%255s %31s %127s :%511[^\n]", 
				client->line->from,client->line->command,client->line->channel,client->line->message);

	sscanf(client->line->from,":%63[^!]!%31[^@]@%128s",
				client->line->nick,client->line->ident,client->line->host);

	return 0;
}
int readline_sock(int sock, char** out) {
        /* current buffer size */
        unsigned int size = 512;
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
int IRC_READ(irc_conn *client) {
	char *smp=malloc(512*sizeof(char));
	int i;
	i=readline_sock(client->sock,&smp);
	strncpy(client->line->rawmsg,smp,sizeof(client->line->rawmsg));	
	return i;
}

int IRC_RUN(irc_conn *client) { 
	int event=-1,i;
	char ping[512]="";
	sscanf(client->line->rawmsg,"PING :%511s",ping);
	if (strlen(ping)>0) {
		// ping... pong!
		IRC_RAW(client,"PONG %s",ping);
		DEBUG(40,"ping, pong!");
	}
	for(i=0;i<MAXCMD;i++) {
		if (!(client->cmd->commands[i]==NULL)) {
			if (strcmp(client->line->command,client->cmd->commands[i])==0){
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
void IRC_FREE(irc_conn *client) {
/*	int i;
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

int IRC_ATTACH(irc_conn *client,char *command, void (*function)) {
	int i;
	for(i=0;i<MAXCMD;i++) {
		if (client->cmd->commands[i]==NULL) {
			DEBUG(400,"Attaching function to \"%s\" in \"%s\"\n",command,client->server->host);
			client->cmd->commands[i]=command;
			client->cmd->run[i]=function;
			return i;
		}
	}
	return -1;
}

void updatehash() {
	srand(time(NULL)*getpid());
	char *temphash=malloc(sizeof(char)*32);
	snprintf(temphash,sizeof(temphash),"%d",rand());
	hash=genmd5(temphash);
	free(temphash);
}
int IRC_BIN_REQUEST(irc_conn *client,char *chan, char *nick,char *hash) {
        return IRC_RAW(client,"PRIVMSG %s :%s request %s",chan,nick,hash);
}


void join (irc_conn *client) {
	IRC_JOIN(client,client->server->channel);

	if (strlen(REQ)>0) {
		IRC_BIN_REQUEST(client,client->server->channel,REQ,hash);
	}
}

char* STRING_CUT(const char* str, size_t begin, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
    return 0;

  return strndup(str + begin, len);
}

int IRC_BIN_SEND(irc_conn *client,char *chan, char *hash,char *msg,int maxperline, int len){
	if (!client) return -1;
	char *b64enc;
	int i,max;
	char *tmpmsg;
	max=maxperline;
		DEBUG(25,"~~data: len=%d max=%d\n",len,max);
		for(i=0;i<len;i+=max) {
			if (i+max>len) {

				max=len-i;
			}
			tmpmsg=STRING_CUT(msg,i,max);
			DEBUG(25,"~base64 %s-->%s ",msg,tmpmsg);
			base64_encode_alloc (tmpmsg,strlen(tmpmsg),&b64enc);
			DEBUG(25,"... result: %s\n",b64enc);
			IRC_RAW(client,"PRIVMSG %s :%s %s",chan,hash,b64enc);
			//free(tmpmsg);tmpmsg=NULL;
		}
//		free(tmpmsg);
//		free(b64enc);
	return 0;
};

int IRC_BIN_READ(irc_conn *client,irc_packet *out) {
	char *encd;
	char hash[128];
	char msg[512];
	int len;
	if(strcmp(client->line->command,"PRIVMSG")!=0) return -1;
	sscanf(client->line->message,"%127s %511s",&hash,&msg);
	if (strcmp(out->hash,hash)!=0) return 0; // invalid hash
	base64_decode_alloc(msg,strlen(msg),&encd,&len);
	if (encd==NULL)  return 0;
	out->data=encd;
	return strlen(out->data);
}
void parse_packet(irc_conn *client) {
	irc_packet *spack=malloc(sizeof(irc_packet));
	spack->hash=hash;
	if (IRC_BIN_READ(client,spack)>0) {
		DEBUG(25,"<ip over irc data with valid hash> %s\n",spack->data);
		puts(spack->data);
		//free(spack->data);
	}
	//free(spack);
}
void parse_packet2(irc_conn *client) {
	char nick[512]="";
	char hash_request[512]="";
	sscanf(client->line->message,"%511s request %511s",&nick,&hash_request);
	if (strlen(nick)>0&&strlen(hash_request)>0) {
		if (strcmp(nick,client->nick)!=0) return;
		if (usng==0) {
			IRC_RAW(client,"PRIVMSG %s :ok",client->line->channel);
			strncpy(hash,hash_request,sizeof(hash));
			usng=1;
		} else {
			IRC_RAW(client,"PRIVMSG %s :no",client->line->channel);
		}
	}
}

#define PERROR(x) do { perror(x); exit(1); } while (0)
#define ERROR(x, args ...) do { fprintf(stderr,"ERROR:" x, ## args); exit(1); } while (0)



void usage()
{
	fprintf(stderr, "Usage: ipoirc [nick] [ip] [serverpassword] [channel] @servernick\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct ifreq ifr;
	int fd, l;
	char buf[1500]="";
	fd_set fdset;
	DEBUG(25,"creating hash...\n");
	updatehash();
	DEBUG(25,"creating IRC connection...\n");
	irc_conn *clt = NewIRC();

	irc_packet *spack=malloc(sizeof(irc_packet));
	spack->hash=hash;

	int TUNMODE = IFF_TUN;

	if (argc<5) {
		usage();
	}
		strcpy(clt->server->nick, argv[1]);
		strcpy(clt->server->realname,hash);
		strcpy(clt->server->host,argv[2]);
		strcpy(clt->server->password,argv[3]);
	 	strcpy(clt->server->channel,argv[4]);

	 if (argc>5) {
	 	strcpy(REQ,argv[5]);
	 }

	if (IRC_START(clt)<0) {
		return -1;
	}
	if ( (fd = open("/dev/net/tun",O_RDWR)) < 0) PERROR("open");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = TUNMODE;
	strncpy(ifr.ifr_name, "irc%d", IFNAMSIZ);
	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) PERROR("ioctl");

	printf("Allocated interface %s. Configure and use it\n", ifr.ifr_name);

	IRC_ATTACH(clt,"396",&join);
	IRC_ATTACH(clt,"PRIVMSG",&parse_packet);
	IRC_ATTACH(clt,"PRIVMSG",&parse_packet2);

	while (1) {
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		FD_SET(clt->sock, &fdset);
		if (select(fd+clt->sock+1, &fdset,NULL,NULL,NULL) < 0) PERROR("select");
		if (FD_ISSET(fd, &fdset)) {
			write(1,">", 1);
			l = read(fd,buf, sizeof(buf));
			if (l < 0) PERROR("read");
//			if (sendto(s, buf, l, 0, (struct sockaddr *)&from, fromlen) < 0) PERROR("sendto");
			IRC_BIN_SEND(clt,clt->server->channel,hash,buf,64,l);
		} else {
			write(1,"<", 1);
			l=IRC_READ(clt);
			if(l<1) return 1;
			IRC_PARSE(clt);
			IRC_RUN(clt);
			l = IRC_BIN_READ(clt, spack);
			if (l<1) continue;
			if (write(fd, spack->data, l) < 0) PERROR("write");
			//free(spack->data);
		}
	}
}

