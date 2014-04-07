/*
 * hpserver.h
 * Copyright @_strobo
 */
#ifndef __HP_SERVER_H
#define __HP_SERVER_H

#include "nodepool.h"

typedef struct {
	int sock;
	struct event_base *eb;
	struct event *udpRecvEv;
	NodePool_t *nodePool;
} HPServerContext;

void on_server_recv(int fd, short event, void *arg);
int runServer(char *port);

#endif
