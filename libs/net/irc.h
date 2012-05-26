#define ServerName 0xA0
#define Port 0xA1
#define Nick 0xA2
#define Ident 0xA3
#define RealName 0xA4
#define Password 0xA5
#define AutoRun 0xA6
#define Channel 0xA7
#define ChannelKey 0xA8

typedef struct {
	char *raw;
	char *nick;
	char *ident;
	char *host;
	char *from;
	char *command;
	char *channel;
	char *message;
} irc_raw;

typedef struct {
	pcmd *cmd[MX];
} irc_cmd;

/* implement topic, modes, etc */
typedef struct {
	char *name;
	char *key;
} irc_channel;

typedef struct {
	char *host;
	unsigned int port;
	char *nick;
	char *ident;
	char *realname;
	char *password;
	char *autorun[MX];
	irc_channel *channels[MX];
} irc_serverinfo;

typedef struct {
	int sock;
	irc_raw *message;
	irc_cmd *command;
	irc_serverinfo *connect;
} irc_conn;

irc_conn* NewIRC(void);
int IRC_FREE(irc_conn *client);
int IRC_START(irc_conn *client);
int IRC_RAW(irc_conn *client, char *fmt, ...);
int IRC_JOIN(irc_conn *client,char *channel,void *key);
int IRC_PART(irc_conn *client,char *channel);
int IRC_PRIVMSG(irc_conn *client,char *channel, char *fmt, ...);
int IRC_NOTICE(irc_conn *client,char *channel, char *fmt, ...);
int IRC_READ(irc_conn *client);
int IRC_PARSE(irc_conn *client);
int IRC_RUN(irc_conn *client);
int IRC_QUIT(irc_conn *client,char *msg);
void IRC_SET(irc_conn *client, int option, char *value);
