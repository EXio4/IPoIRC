/*
 *      libsdemo.c    simple demo of some libs
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libs/crypt/base64.h>
#include <libs/crypt/md5.h> 
#include <libs/memory/free.h>
#include <libs/debug/debug.h>
#include <libs/event.h>
#include <libs/net/sockets.h>
#include <libs/net/irc.h>

void usage(char *prog) {
	printf("%s usage:\n\t%s server nick password\n",prog,prog);
	exit(1);
}
int main(int argc,char *argv[]) {
	if (argc<3) { usage(argv[0]); }
	printf("creating ... ");
	irc_conn* client=NewIRC();
	printf("\n");
	IRC_SET(client,ServerName,argv[1]);
	IRC_SET(client,Nick,argv[2]);
	IRC_SET(client,Password,argv[3]);
	IRC_START(client);
	while (IRC_READ(client)>0) {
		if (IRC_PARSE(client)==0) {
			printf("(%s) [%s] >> %s\n",client->message->command,client->message->channel,client->message->message);
		} else {
			printf("(raw) %s\n",client->message->raw);
		}
		IRC_RUN(client);
	}
	IRC_QUIT(client,"lol");
	printf("freeing ... ");
	IRC_FREE(client);
	printf("\n");
	return 0;
}
