/*
 *      simple socket library for IP over IRC
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <libs/debug/debug.h>
#define FSOCK
#include <libs/net/sockets.h>

int SOCK_NEW(char *hostname,int port) {
	int sock;
	struct sockaddr_in srv;
	
	DEBUG("creating socket ... ");
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) { DEBUG("fail!\n"); return -1; }
	DEBUG("ok\n");	
	DEBUG("filling data .. ");
	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	DEBUG(".sin_addr.s_addr = inet_addr(%s) ...",hostname); 
	srv.sin_addr.s_addr = inet_addr(hostname);
	DEBUG(".sin_port = htons(%d) ... ",port);
	srv.sin_port = htons(port);
	DEBUG("ok\n");
	DEBUG("... connect ... ");
	if (connect(sock,(struct sockaddr *) &srv,sizeof(srv)) < 0) { DEBUG(" error\n"); return -3; }
	DEBUG("ok\n");
	
    return sock; 
}

int SOCK_CLOSE(int sock) {
	DEBUG("closing socket [%d]...\n",sock);
	return close(sock);
}


int SOCK_WRITE(int sock, char *fmt, ...) {
	if (sock<0) { 
		DEBUG("<write|invalid sock> = %d\n",sock);
		return sock;
	}
	char buffer[1024]="";
	int tmp;
    va_list args;
    va_start(args, fmt);
   	vsnprintf(buffer,sizeof(buffer),fmt,args);
    DEBUG("sending to socket <%d> : %s\n",sock,buffer);
    va_end(args);
   	tmp=write(sock, buffer, strlen(buffer));
   	DEBUG("writed %d bytes to socket <%d>\n",tmp,sock);
	return tmp;
}

int SOCK_READ(int sock,void *result,int len)  {
	int i=recv(sock,result,len,0);
	DEBUG("readed %d bytes from <%d> pointer to data: %p\n",i,sock,result);
	return i;
}


int SOCK_READLINE(int sock, char** out) {
        /* current buffer size */
        unsigned int size = 512;
        /* current location in buffer */
        int i = 0;
        /* whether the previous char was a \r */
        int cr = 0;
        int tmp;
        char ch;

        /* result, grows as we need it to */
        DEBUG("*out = malloc(%d*%d)\n",sizeof(char),size);
        *out = malloc(sizeof(char) * size);
		DEBUG("pointer of *out = %p\n",*out);
        for ( ; ; ) {
				tmp=SOCK_READ(sock, &ch, 1);
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
