#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <libs/debug/debug.h>

int TUN_ALLOCATE(char *name, char *interfacename,int mode) {
	struct ifreq ifr;
	int fd;

	DEBUG("opening tun device .... \n");
	if ( (fd = open("/dev/net/tun",O_RDWR)) < 0) { DEBUG("[%s] error in [open]\n",name); return -1; }
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = mode;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	DEBUG("ok! allocating tun device\n");
	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) { DEBUG("[%s] error in [ioctl]\n",name); return -2; }
	DEBUG("tun device allocated..n");
	interfacename=malloc(strlen(ifr.ifr_name+1));
	strcpy(interfacename,ifr.ifr_name);
	return fd;
}
