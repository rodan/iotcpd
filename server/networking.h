#ifndef __networking_h__
#define __networking_h__

#include "main.h"

#ifdef NET_LEVEL
    #define NET_EXPORT
#else
    #define NET_EXPORT extern 
#endif

NET_EXPORT struct epoll_event *events;

int network_glue(void);
void network_free(void);

#endif
