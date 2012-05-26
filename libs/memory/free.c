#include <stdlib.h>
#include <string.h>
#include <libs/memory/free.h>
#include <libs/debug/debug.h>

void TONULL(void *ptr) {
	if (ptr==NULL) {
		return;
	}
	DEBUG("poiting %p to NULL\n",ptr);
	ptr=NULL;
}

void FREE(void *ptr) {
	if (ptr==NULL) { DEBUG("already free() to this pointer!\n"); return; }
	DEBUG("FREE(%p) pointer..\n",ptr);
	free(ptr);
	TONULL(ptr);
}
