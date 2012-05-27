typedef struct {
	char *name;
	void (*run)(void*,void*,void*); 
} pcmd;

int EVENT_ATTACH(pcmd **evs,int arraylen,char *eventname,void (*function));
int EVENT_UNATTACH(pcmd **evs,int num);
int EVENT_RUN(pcmd **evs,int arraylen, char *eventname, void* par1,void* par2,void* par3);
