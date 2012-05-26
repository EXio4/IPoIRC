#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/debug/debug.h>
#include <libs/memory/free.h>
#include <libs/event.h>
int EVENT_ATTACH(pcmd **evs,int arraylen,char *eventname,void (*function)) {
	int i;
	for ( i=0; i<arraylen; i++ ) {
		DEBUG("[event_attach] checking if [%d] point is available ..\n");
		if (evs[i]->name==NULL) {
			DEBUG(" .. yes!\n");
			DEBUG("evs[%d]->name=malloc(%d)",i,sizeof(eventname));
			evs[i]->name=malloc(sizeof(eventname));
			DEBUG(".. pointer %p\n",evs[i]->name);
			strncpy(evs[i]->name,eventname,sizeof(evs[i]->name));
			evs[i]->run=function;
			return i;
		} else { DEBUG("no, trying other...\n"); }
	}
	DEBUG("no available point to attach!\n");
	return -1;
}

int EVENT_UNATTACH(pcmd **evs,int num) {
	if (evs[num]->name) {
		DEBUG("unattaching [%d] event...\n");
		FREE(evs[num]->name);
		evs[num]->run=NULL;
		return 0;
	}
	DEBUG("null event, can't unattach");
	return 1;
}

int EVENT_RUN(pcmd **evs,int arraylen, char *eventname, void* parameter) {
	int i,event=-1;
	DEBUG("running events! ... %s with param: %p\n",eventname,parameter);
	for (i=0;i<arraylen;i++) {
		DEBUG("trying with [%d] event\n",i);
		if (evs[i]->name) {
			DEBUG("valid pointer! (%p)\n",evs[i]->name);
			if (strcmp(evs[i]->name,eventname)==0) {
				DEBUG("yeah! trying to run %p",evs[i]->run);
				evs[i]->run(parameter);
				DEBUG("event++ [%d -> %d]",event,event+1);
				event++;
			} else { DEBUG("event %s != %s\n",evs[i]->name,eventname); }
		} else {
			DEBUG("null pointer (name at %d)\n",i);
		}
	}
	return event;
}
