#include <stdio.h>
#include <string.h>
#include <libs/crypt/base64.h>
#include <libs/debug/debug.h>
#include <libs/net/irc.h>

int IRC_BIN_SEND(irc_conn *client,char *channel,char *data,int maxperline, int len) {
	if (!client) return -1;
	char *encdata;
	int i,max=maxperline;
	char *tmpmsg;
	DEBUG("(bin_data)> len=%d max=%d\n",len,max);
	IRC_EVENT_RUN(client,"sendbinarydata",client,data,&len);
	for(i=0;i<len;i+=max) {
		if (i+max>len) {
			max=len-i;
		}
		IRC_EVENT_RUN(client,"prevencode",client,data+i,&max);
		base64_encode_alloc (data+i,max,&encdata);
		IRC_EVENT_RUN(client,"postencode",client,encdata,NULL)
		DEBUG("encoded data: %s\n",encdata);
		IRC_PRIVMSG(client,channel,"%s",encdata);
	}	
}

int IRC_BIN_READ(irc_conn *client,char *data) {
	int len;
	if(strcmp(client->message->command,"PRIVMSG")) return -1;
	// TODO: implement "checks" 
	IRC_EVENT_RUN(client,"prevdecode",client,NULL,NULL);
	base64_decode_alloc(client->message->message,strlen(client->message->message),data,&len);
	IRC_EVENT_RUN(client,"postdecode",client,data,&len);
	return len;	
}
