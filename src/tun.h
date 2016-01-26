#ifndef IPOIRC_TUN_H
#define IPOIRC_TUN_H

#include "ltun.h"

void tun_thread(void* zmq_context, const Tun& tun);
#endif
