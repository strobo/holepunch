/*
 * hpserver.c
 * Copyright @_strobo
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "hpserver.h"
#include "hppacket.h"
#include "nodepool.h"
#include "hputils.h"

#include <event2/event.h>

#define HP_SERVER_DEFAULT_PORT "60000"

void on_server_recv(int fd, short event, void *arg) {
	uint32_t sessionID;
	struct sockaddr addr;
	char buf[1024];

	HPServerContext *sCtx = (HPServerContext *)arg;
	socklen_t addrSize = sizeof(addr);
	int nread = recvfrom(fd, buf, 1024, 0, &addr, &addrSize);
	if (nread > 0) {
		HP_Packet *packet = (HP_Packet *)buf;

		if(!((packet->prefix[0] == 'H') && (packet->prefix[1] == 'P'))) {
			return;
		}

		switch(packet->type) {
			case HP_Request:
				DBG("HP_Request recieved\n");
				NodePool_t *newNode = NULL;

				sessionID = packet->sessionID;
				if(sessionID == 0) {
					/* A Request doesn't have session ID. Create new node pool for it's request */
					DBG("Request doesn't have sessionID. Create new node pool for is's request\n");

					/* Get requested 'key' */
					char *key = HP_GetDataPtr(packet);

					/* Make a new session ID. */
					sessionID = random32bitWithKey(key);
					if(sCtx->nodePool == NULL) {
						DBG("Create a new node which has key(%s)\n", key);
						/* Node pool doesn't exist. Create new node pool. */
						/* NodePoolにnodeの情報を追加 */

						sCtx->nodePool = NodePoolAllocWith(key, sessionID, &addr);
						if(sCtx->nodePool == NULL) {
							/* Allocの失敗を通知 */
							break;
						}
					} else {
						/* Node pool exist. Create new node. */
						newNode = NodePoolAllocWith(key, sessionID, &addr);
						if(newNode == NULL) {
							/* Allocの失敗を通知 */
							break;
						}

						/* Find a node which holds requested a 'key'. */
						NodePool_t *matchedNode = NodePoolGetByKey(sCtx->nodePool, key);
						if(matchedNode) {
							/* Partner found. */
							if(matchedNode->partner == NULL) {
								/* Partner is alone. Link it each other. */
								matchedNode->partner = newNode;
								newNode->partner = matchedNode;
							} else {
								DBG("A node which has key(%s) are already connected to another node\n", key);
							}
						}

						/* Append a new node to node pool */
						DBG("Append a new node which has key(%s)\n", key);
						sCtx->nodePool = NodePoolAdd(sCtx->nodePool, newNode);
					}

					HP_Packet *rPacket = HP_PacketAlloc(HP_PACKET_HEADER_LEN + sizeof(struct sockaddr));
					rPacket->type = HP_Accepted;
					rPacket->sessionID = sessionID;
					rPacket->keyLength = 0;
					memcpy(HP_GetDataPtr(rPacket), &addr, sizeof(struct sockaddr));
					sendto(fd, rPacket, HP_PacketLength(rPacket), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));

					if(newNode && newNode->partner) {
						DBG("key(%s) matched!\n", key);
						rPacket->type = HP_Found;
						rPacket->sessionID = sessionID;
						rPacket->keyLength = 0;
						/* 相手nodeのアドレスを今回のアクセス元に返信 */
						memcpy(HP_GetDataPtr(rPacket), &newNode->partner->addr, sizeof(struct sockaddr));
						sendto(fd, rPacket, HP_PacketLength(rPacket), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));

						/* 今回のアクセス元のアドレスを相手nodeに送信 */
						memcpy(HP_GetDataPtr(rPacket), &addr, sizeof(struct sockaddr));
						sendto(fd, rPacket, HP_PacketLength(rPacket), 0, (struct sockaddr *)&newNode->partner->addr, sizeof(struct sockaddr));
					}
					free(rPacket);

				} else {
					/* A Resuest has session ID */

					if(sCtx->nodePool) {
						/* If node pool exists. Search a node which holds requested session ID */
						NodePool_t *matchedNode = NodePoolGetBySessionID(sCtx->nodePool, sessionID);
						if(matchedNode) {
							/* Same session ID found. */
							DBG("Requested session ID is registered in node pool\n");
							if(matchedNode->partner) {
								/* Found node has already matched to another node */
								DBG("Another node already registered\n");
								HP_Packet *fPacket = HP_PacketAlloc(HP_PACKET_HEADER_LEN + sizeof(struct sockaddr));
								fPacket->type = HP_Found;
								fPacket->sessionID = sessionID;
								fPacket->keyLength = 0;
								memcpy(HP_GetDataPtr(fPacket), &newNode->partner->addr, sizeof(struct sockaddr));
								sendto(fd, fPacket, HP_PacketLength(fPacket), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
								free(fPacket);
							} else {
								/* Found node has not matched yet. Update NodePool */
								DBG("Update NodePool\n");
							}
						} else {
							DBG("sessioinID not found\n");
						}
					}
				}

				break;

			case HP_Complete:
				DBG("HP_Complete\n");
				uint32_t sessionID = packet->sessionID;

				NodePool_t *node = NodePoolGetBySessionID(sCtx->nodePool, sessionID);
				if(node != NULL) {
					DBG("deleting node which has key(%s)\n", node->key);
					NodePool_t *partner = node->partner;
					sCtx->nodePool = NodePoolDel(sCtx->nodePool, node);
					sCtx->nodePool = NodePoolDel(sCtx->nodePool, partner);
				}
				break;

			case HP_Punch:
			case HP_Accepted:
			case HP_Found:
			default:
				ERR("wrong packet");
				return;
				break;
		}
	} else if( nread < 0){
		perror("recvfrom");
	}
}


int runServer(char *port) {
	DBG("start server\n");

	int res;
	HPServerContext *sCtx;
	struct addrinfo hints = {0}, *local;

	sCtx = malloc(sizeof(HPServerContext));
	if(sCtx == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	if(port == NULL) {
		port = HP_SERVER_DEFAULT_PORT;
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	res = getaddrinfo(NULL, port, &hints, &local);
	if (res != 0) {
		ERR("getaddrinfo: %s\n", gai_strerror(res));
		exit(EXIT_FAILURE);
	}

	printf("matching server running at port %d\n",
			ntohs(((struct sockaddr_in *)(local->ai_addr))->sin_port)
		  );

	sCtx->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sCtx->sock < 0) {
		ERR("sock init failed\n");
		exit(EXIT_FAILURE);
	}
	if(bind(sCtx->sock, (struct sockaddr *)local->ai_addr, sizeof(struct sockaddr)) < 0) {
		ERR("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* set up the event loop */
	sCtx->eb = event_base_new();
	sCtx->udpRecvEv = event_new(sCtx->eb, sCtx->sock, EV_READ | EV_PERSIST, on_server_recv, sCtx);
	event_add(sCtx->udpRecvEv, NULL);

	/* Run the event loop */
	event_base_dispatch(sCtx->eb);

	event_free(sCtx->udpRecvEv);
	event_base_free(sCtx->eb);
	close(sCtx->sock);
	freeaddrinfo(local);

	if(sCtx->nodePool) {
		NodePoolRelease(sCtx->nodePool);
	}

	free(sCtx);

	return EXIT_SUCCESS;
}
