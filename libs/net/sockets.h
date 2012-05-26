#ifndef FSOCK 
#define FSOCK
int SOCK_NEW(char *hostname,int port);
int SOCK_CLOSE(int sock);
int SOCK_WRITE(int sock, char *fmt, ...);
int SOCK_READ(int sock,void *result,int len);
int SOCK_READLINE(int sock,void *result);
#endif
