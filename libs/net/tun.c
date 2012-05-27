#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>

int TUN_ALLOCATE(char *name, char *interfacename,int mode) {
	struct ifreq ifr;
	int fd;
	if ( (fd = open("/dev/net/tun",O_RDWR)) < 0) return -1;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = mode;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) return -2;

	interfacename=malloc(strlen(ifr.ifr_name+1));
	strcpy(interfacename,ifr.ifr_name);
	return fd;
}
