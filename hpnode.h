/*
 * hpnode.h
 * Copyright @_strobo
 */
#ifndef __HP_NODE_H
#define __HP_NODE_H

#include <stdint.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <event2/event.h>

typedef struct {
	int sock;
	struct event_base *eb;
	uint32_t token;
	uint32_t sessionID;
	struct sockaddr matchingServerAddress;
	struct sockaddr remoteNodeAddress;
	char key[16];
	uint8_t punchCount;
	struct timeval  punchInterval;
	struct event *stdinEv;
	struct event *udpRecvEv;
	struct event *punchTimerEv;
	struct event *sigintEv;
	_Bool isConnected;
} HPNodeContext;

void punchingNode(int fd, short event, void *arg);
void on_node_recv(int fd, short event, void *arg);
void on_sigint(int fd, short event, void *arg);
int runNode(char *serverAddress, char *key);
#endif
