/*
 *      simple event library for IP over IRC
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
			DEBUG("evs[%d]->name=malloc(%d)",i,strlen(eventname)+1);
			evs[i]->name=malloc(strlen(eventname)+1);
			DEBUG(".. pointer %p\n",evs[i]->name);
			strncpy(evs[i]->name,eventname,strlen(eventname));
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

int EVENT_RUN(pcmd **evs,int arraylen, char *eventname, void* par1,void* par2,void* par3) {
	int i,event=-1;
	DEBUG("running events! ... %s with param1: %p param2: %p param3: %p\n",eventname,par1,par2,par3);
	for (i=0;i<arraylen;i++) {
		DEBUG("trying with [%d] event\n",i);
		if (evs[i]->name) {
			DEBUG("valid pointer! (%p)\n",evs[i]->name);
			if (strcmp(evs[i]->name,eventname)==0) {
				DEBUG("yeah! trying to run %p",evs[i]->run);
				evs[i]->run(par1,par2,par3);
				DEBUG("event++ [%d -> %d]",event,event+1);
				event++;
			} else { DEBUG("event %s != %s\n",evs[i]->name,eventname); }
		} else {
			DEBUG("null pointer (name at %d)\n",i);
		}
	}
	return event;
}
