/*
 * hpnode.c
 * Copyright @_strobo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "hpnode.h"
#include "hppacket.h"
#include "hputils.h"

#include <event2/event.h>

#define HP_SERVER_DEFAULT_PORT "60000"

/* from ping.c */
/*
 * tvsub
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
static void
tvsub(register struct timeval *out, register struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

void punchingNode(int fd, short event, void *arg) {
	HPNodeContext *nCtx = (HPNodeContext *)arg;
	char *key = nCtx->key;

	if(!nCtx->isConnected) {
		/* stop a timer when punching 4 times */
		if(nCtx->punchCount >= 4) {
			event_del(nCtx->punchTimerEv);
			return;
		}
		nCtx->punchCount++;
	}

	char *ip = inet_ntoa(((struct sockaddr_in *)&nCtx->remoteNodeAddress)->sin_addr);
	int port = ntohs(((struct sockaddr_in *)&nCtx->remoteNodeAddress)->sin_port);
	printf("Punching to another node %s:%d.\n", ip, port);
	if(!nCtx->isConnected) {
		printf("Waiting 3sec until receive a payback\n");
	}

	if(nCtx->token == 0) {
		nCtx->token = random32bit();
	}
	/* Send a HP_Punch packet to another node. */
	HP_Packet *pPacket = HP_PacketAlloc(HP_PACKET_HEADER_LEN + strlen(key) + sizeof(struct timeval));
	pPacket->type = HP_Punch;
	pPacket->sessionID = nCtx->token;
	pPacket->keyLength = strlen(key);
	strcpy(HP_GetDataPtr(pPacket), key);
	gettimeofday((struct timeval *)HP_GetTvPtr(pPacket), (struct timezone *)NULL);
	sendto(nCtx->sock, pPacket, HP_PacketLength(pPacket), 0, (struct sockaddr *)&nCtx->remoteNodeAddress, sizeof(struct sockaddr));
	free(pPacket);
	return;
}

void on_node_recv(int fd, short event, void *arg) {
	char *srcIP;
	int srcPort, nread;
	struct sockaddr *globalAddr;
	char buf[1024];
	struct sockaddr addr;
	struct timeval tv, *tp;

	gettimeofday(&tv, (struct timezone *)NULL);
	HPNodeContext *nCtx = (HPNodeContext *)arg;

	socklen_t addrSize = sizeof(addr);
	nread = recvfrom(fd, buf, 1024, 0, &addr, &addrSize);

	if (nread > 0) {
		HP_Packet *packet = (HP_Packet *)buf;

		if(!((packet->prefix[0] == 'H') && (packet->prefix[1] == 'P'))) {
			if(nCtx->isConnected) {
				char *text = calloc(sizeof(char), nread+1);
				snprintf(text, nread, "%s", buf);
				/* outputs recieved text */
				fprintf(stderr, "%s", text);
				free(text);
			}
			return;
		}

		switch(packet->type) {
			case HP_Punch:
				DBG("HP_Punch recieved!\n");

				/* Get requested 'key' */
				char *key = HP_GetDataPtr(packet);

				if(strncmp(nCtx->key, key, packet->keyLength) == 0) {
					HP_Packet *pPacket = HP_PacketAlloc(HP_PACKET_HEADER_LEN + sizeof(struct timeval));
					pPacket->type = HP_Payback;
					pPacket->sessionID = packet->sessionID;
					pPacket->keyLength = 0;
					memcpy(HP_GetTvPtr(pPacket), HP_GetTvPtr(packet), sizeof(struct timeval));
					sendto(fd, pPacket, HP_PacketLength(pPacket), 0, &addr, sizeof(struct sockaddr));
					free(pPacket);
				} else {
					DBG("key unmatched\n");
				}
				break;

			case HP_Payback:
				DBG("HP_Payback recieved\n");
				if(nCtx->token == packet->sessionID) {
					if(!nCtx->isConnected) {
						/* Connection established to another node */

						/* Change a timer interval for keepalive */
						event_del(nCtx->punchTimerEv);
						nCtx->punchInterval.tv_sec = 30;
						event_add(nCtx->punchTimerEv, &nCtx->punchInterval);

						srcIP = inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr);
						srcPort = ntohs(((struct sockaddr_in *)&addr)->sin_port);
						printf("Connection established to %s:%d\n", srcIP, srcPort);

						DBG("send HP_Complete to matchingServer\n");
						HP_Packet *cPacket = HP_PacketAlloc(HP_PACKET_HEADER_LEN);
						cPacket->type = HP_Complete;
						cPacket->sessionID = nCtx->sessionID;
						cPacket->keyLength = 0;
						sendto(fd, cPacket, HP_PacketLength(cPacket), 0, &nCtx->matchingServerAddress, sizeof(struct sockaddr));
						free(cPacket);
						nCtx->isConnected = 1;
					} else {
						tp = (struct timeval *)HP_GetTvPtr(packet);
						tvsub(&tv, tp);
						long triptime = tv.tv_sec * 10000 + (tv.tv_usec / 100);
						printf(" time=%ld.%ld ms\n", triptime/10, triptime%10);
					}
				}
				break;

			case HP_Accepted:
				DBG("HP_Accepted recieved\n");
				nCtx->sessionID = packet->sessionID;
				globalAddr = (struct sockaddr *)HP_GetDataPtr(packet);
				srcIP = inet_ntoa(((struct sockaddr_in *)globalAddr)->sin_addr);
				srcPort = ntohs(((struct sockaddr_in *)globalAddr)->sin_port);
				printf("my global ip address is: %s:%d\n", srcIP, srcPort);
				printf("Waiting for packet with 'HP_Found'\n");
				break;

			case HP_Found:
				DBG("HP_Found\n");
				struct sockaddr *remoteNodeAddress = (struct sockaddr *)HP_GetDataPtr(packet);
				srcIP = inet_ntoa(((struct sockaddr_in *)remoteNodeAddress)->sin_addr);
				srcPort = ntohs(((struct sockaddr_in *)remoteNodeAddress)->sin_port);
				printf("another node's address is: %s:%d\n", srcIP, srcPort);

				memcpy(&nCtx->remoteNodeAddress, remoteNodeAddress, sizeof(struct sockaddr));

				/* set up a punching timer */
				nCtx->punchCount = 0;
				nCtx->punchTimerEv = event_new(nCtx->eb, -1, EV_PERSIST, punchingNode, nCtx);
				nCtx->punchInterval.tv_sec = 3;
				nCtx->punchInterval.tv_usec = 0;
				event_add(nCtx->punchTimerEv, &nCtx->punchInterval);
				break;

			case HP_Exit:
				DBG("HP_Exit\n");
				printf("Recieved disconnect packet. Exiting\n");
				event_base_loopbreak(nCtx->eb);
				break;

			default:
				ERR("wrong packet\n");
				return;
		}

	} else if( nread < 0) {
		perror("recvfrom");
	}
	return;
}

void on_stdin(int fd, short event, void *arg) {
	HPNodeContext *nCtx = (HPNodeContext *)arg;
	char buf[80];

	int nread = read(fd, buf, 80);
	if (nread < 0) {
		return;
	}
	if(nCtx->isConnected) {
		sendto(nCtx->sock, buf, nread, 0, (struct sockaddr *)&nCtx->remoteNodeAddress, sizeof(struct sockaddr));
	} else {
		DBG("You're not established to another node yet.\n");
	}
	return;
}

/* When Press Ctrl-C, Send HP_Exit and Exit */
void on_sigint(int fd, short event, void *arg) {
	HPNodeContext *nCtx = (HPNodeContext *)arg;

	HP_Packet *packet = HP_PacketAlloc(HP_PACKET_HEADER_LEN);
	packet->keyLength = 0;
	if(nCtx->isConnected) {
		/* Send a HP_Exit packet to another node. */
		packet->type = HP_Exit;
		packet->sessionID = 0;
		sendto(nCtx->sock, packet, HP_PacketLength(packet), 0, &nCtx->remoteNodeAddress, sizeof(struct sockaddr));
	} else {
		packet->type = HP_Complete;
		packet->sessionID = nCtx->sessionID;
		sendto(nCtx->sock, packet, HP_PacketLength(packet), 0, &nCtx->matchingServerAddress, sizeof(struct sockaddr));
	}
	free(packet);

	printf("\nExiting\n");
	event_base_loopbreak(nCtx->eb);
}

int runNode(char *serverAddress, char *key) {
	DBG("start node\n");

	HPNodeContext *nCtx;
	char *domain, *port;
	int res;
	struct addrinfo hints = {0}, *local, *remote;


	nCtx = malloc(sizeof(HPNodeContext));
	if(nCtx == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	domain = strtok(serverAddress, ":");
	if(domain == NULL) {
		domain = serverAddress;
	}

	port = strtok(NULL, ":");
	if(port == NULL) {
		port = HP_SERVER_DEFAULT_PORT;
	}

	res = getaddrinfo(NULL, "0", &hints, &local);
	if(res != 0) {
		ERR("getaddrinfo local");
		ERR("getaddrinfo: %s\n", gai_strerror(res));
		exit(EXIT_FAILURE);
	}

	res = getaddrinfo(domain, port, &hints, &remote);
	if(res != 0) {
		ERR("getaddrinfo remote");
		ERR("getaddrinfo: %s\n", gai_strerror(res));
		exit(EXIT_FAILURE);
	}

	printf("remote: %s(%s):%d\n",
			domain,
			inet_ntoa(((struct sockaddr_in *)(remote->ai_addr))->sin_addr),
			ntohs(((struct sockaddr_in *)(remote->ai_addr))->sin_port)
		  );

	nCtx->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(nCtx->sock < 0) {
		ERR("sock init failed\n");
		exit(EXIT_FAILURE);
	}
	if(bind(nCtx->sock, (struct sockaddr *)local->ai_addr, sizeof(struct sockaddr)) < 0) {
		ERR("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Send a HP_Request */
	HP_Packet *packet = HP_PacketAllocWithKey(key);
	sendto(nCtx->sock, packet, HP_PacketLength(packet), 0, (struct sockaddr *)remote->ai_addr, sizeof(struct sockaddr));
	free(packet);

	/* store some infomations to HPNodeContext */
	memcpy(&nCtx->matchingServerAddress, remote->ai_addr, sizeof(struct sockaddr));
	strcpy(nCtx->key, key);

	/* init punchInterval */
	nCtx->punchInterval.tv_sec = 0;
	nCtx->punchInterval.tv_usec = 0;

	/* set up the event loop */
	nCtx->eb = event_base_new();
	nCtx->stdinEv = event_new(nCtx->eb, 0, EV_READ | EV_PERSIST, on_stdin, nCtx);
	nCtx->udpRecvEv = event_new(nCtx->eb, nCtx->sock, EV_READ | EV_PERSIST, on_node_recv, nCtx);
	nCtx->sigintEv = evsignal_new(nCtx->eb, SIGINT, on_sigint, nCtx);
	event_add(nCtx->stdinEv, NULL);
	event_add(nCtx->udpRecvEv, NULL);
	event_add(nCtx->sigintEv, NULL);

	/* Run the event loop */
	event_base_dispatch(nCtx->eb);

	event_free(nCtx->stdinEv);
	event_free(nCtx->udpRecvEv);
	event_free(nCtx->sigintEv);
	event_base_free(nCtx->eb);
	close(nCtx->sock);
	freeaddrinfo(local);
	freeaddrinfo(remote);
	free(nCtx);

	return EXIT_SUCCESS;
}
