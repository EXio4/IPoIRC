typedef struct {
	char *name;
	void (*run)(void*); 
} pcmd;

int EVENT_ATTACH(pcmd **evs,int arraylen,char *eventname,void (*function));
int EVENT_UNATTACH(pcmd **evs,int num);
int EVENT_RUN(pcmd **evs,int arraylen, char *eventname, void* parameter);
