/*
 *      simple IRC library for IP over IRC
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
 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <libs/memory/free.h>
#include <libs/debug/debug.h>
#include <libs/event.h>
#include <libs/net/sockets.h>
#include <libs/net/irc.h>

irc_conn* NewIRC(void) {
	DEBUG("Allocating new irc_conn... ");
	irc_conn *client=malloc(sizeof(irc_conn));
	DEBUG(" .. pointer %p\n",client);
	if (client==NULL) return NULL;
	int i;
	DEBUG("Allocation sub-structs... ");
	client->connect=malloc(sizeof(irc_serverinfo));
	DEBUG("@->connect = %p ",client->connect);
	client->command=malloc(sizeof(irc_cmd));
	DEBUG("@->command = %p ",client->command);
	client->message=malloc(sizeof(irc_raw));
	DEBUG("@->message = %p\n",client->message);
	DEBUG(".. ->sock = -1\n");
	client->sock = -1;
	DEBUG(".. ->connect-<port = 6667\n");
	client->connect->port = 6667;
	for (i=0;i<MX;i++) {
		DEBUG(".. ->connect->autorun[%d] = NULL\n",i);
		client->connect->autorun[i]=NULL;
		DEBUG("client->command->cmd[%d]=malloc(%d)",i,sizeof(pcmd));
		client->command->cmd[i]=malloc(sizeof(pcmd));
		DEBUG(" .. pointer (%p)\n",client->command->cmd[i]);
		DEBUG(".. ->command->cmd[%d]->name = NULL\n",i);
		client->command->cmd[i]->name=NULL;
		DEBUG(".. ->command->cmd[%d]->run = NULL\n",i);
		client->command->cmd[i]->run=NULL;
		DEBUG("client->connect->channels[%d]=malloc(%d)",i,sizeof(irc_channel));
		client->connect->channels[i]=malloc(sizeof(irc_channel));
		DEBUG(" .. pointer (%p)\n",client->connect->channels[i]);
		DEBUG(".. ->connect->channels[%d]->name = NULL\n",i);
		client->connect->channels[i]->name=NULL;
		DEBUG(".. ->connect->channels[%d]->key = NULL\n",i);
		client->connect->channels[i]->key=NULL;
	}
	DEBUG(".. ->connect->host = NULL\n",i);
	client->connect->host=NULL;
	DEBUG(".. ->connect->nick = NULL\n",i);
	client->connect->nick=NULL;
	DEBUG(".. ->connect->ident = NULL\n",i);	
	client->connect->ident=NULL;
	DEBUG(".. ->connect->realname = NULL\n",i);
	client->connect->realname=NULL;
	DEBUG(".. ->connect->password = NULL\n",i);	
	client->connect->password=NULL;
	DEBUG(".. ->message->raw = NULL\n",i);	
	client->message->raw=NULL;
	DEBUG(".. ->message->nick = NULL\n",i);	
	client->message->nick=NULL;
	DEBUG(".. ->message->ident = NULL\n",i);	
	client->message->ident=NULL;
	DEBUG(".. ->message->host = NULL\n",i);		
	client->message->host=NULL;
	DEBUG(".. ->message->from = NULL\n",i);	
	client->message->from=NULL;
	DEBUG(".. ->message->command = NULL\n",i);		
	client->message->command=NULL;
	DEBUG(".. ->message->channel = NULL\n",i);	
	client->message->channel=NULL;
	DEBUG(".. ->message->message = NULL\n",i);	
	client->message->message=NULL;
	return client;
}

int IRC_RAW(irc_conn *client, char *fmt, ...) {
	if (client==NULL) { DEBUG("irc_raw only valid with allocated irc_conn\n"); return -1; }
	if (client->sock<0) { 
		DEBUG(".. irc_raw to no connected irc_connection\n");
		return client->sock;
	}
	DEBUG("char *buffer=malloc(%d*1024); ",sizeof(char));
	char *buffer=malloc(sizeof(char)*1024);
	DEBUG(" ... pointer %p\n",buffer);
	DEBUG(" ... sizeof buffer = %d - needed: %d\n",sizeof(buffer),1024);
    va_list args;
    va_start(args, fmt);
   	vsnprintf(buffer,1024,fmt,args);
    DEBUG("sending raw command (%s) to %s in [%d] sock\n",buffer,client->connect->host,client->sock);
    va_end(args);
	IRC_EVENT_RUN(client,"on_sendraw",client,buffer,NULL);
	int i=SOCK_WRITE(client->sock, "%s\r\n", buffer);
	FREE(buffer);
	return i;
}

int IRC_JOIN(irc_conn *client,char *channel,void *key) { 
	if (key)
		return IRC_RAW(client,"JOIN %s",channel);
	else
		return IRC_RAW(client,"JOIN %s %s",channel,(char*)key);		
}
int IRC_PART(irc_conn *client,char *channel) {
	return IRC_RAW(client,"PART %s",channel);
}

int IRC_PRIVMSG(irc_conn *client,char *channel, char *fmt, ...) {
	DEBUG("char *buffer=malloc(%d*1024); ",sizeof(char));
	char *buffer=malloc(sizeof(char)*1024);
	DEBUG(" ... pointer %p\n",buffer);
    va_list args;
    va_start(args, fmt);
   	vsnprintf(buffer,sizeof(buffer),fmt,args);
    DEBUG("sending privmsg to %s with msg: %s\n",channel,buffer);
    va_end(args);
   	IRC_EVENT_RUN(client,"on_sendprivmsg",client,buffer,NULL);
   	int i=IRC_RAW(client,"PRIVMSG %s :%s",channel,buffer);
	FREE(buffer);
	return i;
}

int IRC_NOTICE(irc_conn *client,char *channel, char *fmt, ...) {
	DEBUG("char *buffer=malloc(%d*1024); ",sizeof(char));
	char *buffer=malloc(sizeof(char)*1024);
	DEBUG(" ... pointer %p\n",buffer);
    va_list args;
    va_start(args, fmt);
   	vsnprintf(buffer,sizeof(buffer),fmt,args);
    DEBUG("sending notice to %s with msg: %s\n",channel,buffer);
    va_end(args);
   	IRC_EVENT_RUN(client,"on_sendnotice",client,buffer,NULL);
   	int i=IRC_RAW(client,"NOTICE %s :%s",channel,buffer);
	FREE(buffer);
	return i;
}

int IRC_START(irc_conn *client) {
	int i;
	if (client==NULL) { DEBUG("start only valid with allocated irc_conn\n"); return -1; }
	if (client->sock!=-1) { DEBUG("already connected\n");return -1; };
	if (client->connect==NULL) { return -1; }; // wtf?	
	// safe check
	if (!client->connect->host) { DEBUG("need a hostname to connect\n");return -2; }
	if (!client->connect->port) { DEBUG("uh? need a port\n"); return -2; }
	if (!client->connect->nick) { DEBUG("need a nick!\n"); return -2; }
	DEBUG("creating socket... \n");
	client->sock = SOCK_NEW(client->connect->host,client->connect->port);
	if (client->sock < 0 ) {
		DEBUG("socket fail\n");
		client->sock=-1;
		return -1; 
	} else DEBUG("socket ok..\n");
	char *ident=client->connect->ident,*realname=client->connect->realname;
	if (!ident) { DEBUG("fallback ident to nick\n");ident=client->connect->nick; }
	if (!realname) { DEBUG("fallback realname to nick\n");realname=client->connect->nick; }
	IRC_EVENT_RUN(client,"prev_register",client,NULL,NULL);
	if (client->connect->password) {
		DEBUG("sending pass to server...\n");
		IRC_RAW(client,"PASS %s",client->connect->password);
	}
	IRC_RAW(client,"USER %s %s * :%s",ident,client->connect->host,realname);
	IRC_RAW(client,"NICK %s",client->connect->nick);
	IRC_EVENT_RUN(client,"post_register",client,NULL,NULL);
	// autorun commands
	for (i=0;i<MX;i++) {
		if (!client->connect->autorun[i]) {
				DEBUG("end of autorun commands\n");
				break;
			}
			DEBUG("running command [%d] .. \n",i);
			IRC_RAW(client,client->connect->autorun[i]);
	}
	DEBUG("connected to %s!\n",client->connect->host);
	DEBUG("joining in channels...\n");
	for (i=0;i<MX;i++) {
		if (!client->connect->channels[i]->name) {
				DEBUG("end channel joins...\n");
				break;
		}
			DEBUG("joining in [%s] .. \n",client->connect->channels[i]->name);
			IRC_JOIN(client,client->connect->channels[i]->name,client->connect->channels[i]->key);
	}
	return 0;
}

int IRC_READ(irc_conn *client) {
	DEBUG("reading from ircserver [%s]...\n",client->connect->host);
	FREE(client->message->raw);
	char *temp;
	int i=SOCK_READLINE(client->sock,&temp);
	client->message->raw=temp;
	return i;
}

int IRC_PARSE(irc_conn *client) {
	FREE(client->message->from);
	FREE(client->message->command);
	FREE(client->message->channel);
	FREE(client->message->message);
	FREE(client->message->nick);
	FREE(client->message->ident);
	FREE(client->message->host);
	DEBUG("client->message->from=malloc(%d)",256);
	client->message->from=malloc(sizeof(char)*256);
	DEBUG(" .. pointer (%p)\n",client->message->from);
	DEBUG("client->message->command=malloc(%d)",64);
	client->message->command=malloc(sizeof(char)*64);
	DEBUG(" .. pointer (%p)\n",client->message->command);
	DEBUG("client->message->channel=malloc(%d)",128);
	client->message->channel=malloc(sizeof(char)*128);
	DEBUG(" .. pointer (%p)\n",client->message->channel);
	DEBUG("client->message->message=malloc(%d)",512);
	client->message->message=malloc(sizeof(char)*512);
	DEBUG(" .. pointer (%p)\n",client->message->message);
	DEBUG("client->message->nick=malloc(%d)",64);
	client->message->nick=malloc(sizeof(char)*64);
	DEBUG(" .. pointer (%p)\n",client->message->nick);
	DEBUG("client->message->ident=malloc(%d)",32);
	client->message->ident=malloc(sizeof(char)*32);
	DEBUG(" .. pointer (%p)\n",client->message->ident);
	DEBUG("client->message->host=malloc(%d)",128);
	client->message->host=malloc(sizeof(char)*128);
	DEBUG(" .. pointer (%p)\n",client->message->host);
	DEBUG("allocated ram! \n");
	DEBUG("... pointer: %p value: %s\n",client->message->raw,client->message->raw);
	DEBUG("parsing messages [raw]...\n");
	int a1=sscanf(client->message->raw, ":%255s %63s %127s :%511[^\n]", client->message->from,client->message->command,client->message->channel,client->message->message);
	DEBUG("parsing messages [from]..\n"); 
	int a2=sscanf(client->message->from,":%63[^!]!%31[^@]@%128s", client->message->nick,client->message->ident,client->message->host);

	DEBUG("parsing return code: %d -- %d\n",a1,a2); 
	if (a1!=4) {
		DEBUG("non common line\n");
		TONULL(client->message->from);
		TONULL(client->message->command);
		TONULL(client->message->channel);
		TONULL(client->message->message);
		TONULL(client->message->nick);
		TONULL(client->message->ident);
		TONULL(client->message->host);
		return 1;
	}
	return 0;	
}

int IRC_RUN(irc_conn *client) {
	DEBUG("(irc_run) ping=malloc(%d)",128);
	char *ping=malloc(sizeof(char)*128);
	DEBUG(" .. pointer (%p)\n",ping);
	// ping?
	if (strncmp(client->message->raw,"PING",4)==0) {
		DEBUG("ping!\n");
		sscanf(client->message->raw,"PING :%511s",ping);
		IRC_RAW(client,"PONG :%s",ping);
		IRC_EVENT_RUN(client,"ping",client,NULL,NULL);
		DEBUG(">pong!\n");	
	}
	if (strncmp(client->message->raw,"PONG",4)==0) {
		DEBUG("<pong!\n");
		sscanf(client->message->raw,"PONG :%511s",ping);
		IRC_EVENT_RUN(client,"pong",client,ping,NULL);
	}	
	FREE(ping);
	if (client->message->command) {
		DEBUG("running %s hook!\n",client->message->command);
		IRC_EVENT_RUN(client,client->message->command,client,NULL,NULL);
	} else {
		DEBUG("running unknown hook! (raw: %s)\n",client->message->raw);
		IRC_EVENT_RUN(client,"unknown",client,NULL,NULL);
	}
	return 0;
}

int IRC_EVENT_RUN(irc_conn *client,char *a1,void *p1,void *p2, void *p3) {
	return EVENT_RUN(client->command->cmd,MX,a1,p1,p2,p3);
}
int IRC_ATTACH(irc_conn *client,char *command, void (*function)) {
	DEBUG("attaching %p to %s\n",function,command);
	return EVENT_ATTACH(client->command->cmd,MX,command,function);
}
int IRC_DETACH(irc_conn *client,int num) {
	DEBUG("unattaching event...\n");
	return EVENT_DETACH(client->command->cmd,num);
}
int IRC_QUIT(irc_conn *client,char *msg) {
	if (client==NULL) { DEBUG("irc_quit ... only valid allocated irc_conn\n"); return -1; }
	if (client->sock==-1) { DEBUG("irc_quit ... need a connected server\n");return -1; };
	DEBUG("disconnecting from sock <%d> with msg: %s \n",client,msg);
	IRC_EVENT_RUN(client,"prev_quit",client,msg,NULL);
	int i=IRC_RAW(client,"QUIT :%s",msg);
	IRC_EVENT_RUN(client,"post_quit",client,NULL,NULL);
	SOCK_CLOSE(client->sock);
	client->sock=-1;
	return i; 
}

int IRC_FREE(irc_conn *client) {
	int i;
	DEBUG("Free to IRC connection ... \n");
	if (!client) { DEBUG("client = NULL\n");;return 0; } 
 	if (client->sock >= 0 ) { DEBUG("Already connected irc_conn...\n");return 1; };
 	IRC_EVENT_RUN(client,"on_destroy",client,NULL,NULL);
	for (i=0;i<MX;i++) {
		FREE(client->connect->autorun[i]);
		FREE(client->command->cmd[i]->name);
		FREE(client->command->cmd[i]);
		FREE(client->connect->channels[i]->name);
		FREE(client->connect->channels[i]->key);
		FREE(client->connect->channels[i]);
	}
	FREE(client->connect->host);
	FREE(client->connect->nick);
	FREE(client->connect->ident);
	FREE(client->connect->realname);
	FREE(client->connect->password);
	FREE(client->connect);
	FREE(client->command);
	FREE(client->message->raw);
	FREE(client->message->nick);
	FREE(client->message->ident);
	FREE(client->message->host);
	FREE(client->message->from);
	FREE(client->message->command);
	FREE(client->message->channel);
	FREE(client->message->message);
	FREE(client->message);
	FREE(client);
	return 0;
}

void IRC_SET(irc_conn *client, int option, char *value) {
	DEBUG("(irc_set) temp=malloc(%d)",strlen(value)+1);
	char *temp=malloc(sizeof(char)*(strlen(value)+1));
	DEBUG(" .. pointer (%p)\n",temp);
	strcpy(temp,value); 
	DEBUG("tempvalue=%s || need: %s\n",temp,value);
	switch(option) {
		case ServerName:
				client->connect->host=temp;
			break;
		case Port:
				client->connect->port=atoi(temp);
				FREE(temp);
			break;
		case Nick:
				client->connect->nick=temp;
			break;
		case Ident:
				client->connect->ident=temp;
			break;
		case RealName:
				client->connect->realname=temp;
			break;
		case Password:
				client->connect->password=temp;
			break;
	}
	IRC_EVENT_RUN(client,"set",client,&option,value);	
}
