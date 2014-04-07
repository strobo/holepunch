/*
 * hppacket.c
 * Copyright @_strobo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "hppacket.h"

ssize_t HP_PacketLength(HP_Packet *packet) {
	switch(packet->type) {
		case HP_Request:
			return (HP_PACKET_HEADER_LEN + packet->keyLength);
			break;
		case HP_Punch:
		case HP_Payback:
			return (HP_PACKET_HEADER_LEN + packet->keyLength + sizeof(struct timeval));
			break;

		case HP_Accepted:
		case HP_Found:
			return (HP_PACKET_HEADER_LEN + sizeof(struct sockaddr));
			break;

		case HP_Complete:
		case HP_Exit:
			return HP_PACKET_HEADER_LEN;
			break;
		default:
			return 0;
	}
}
HP_Packet* HP_PacketAllocByType(HP_Type_t type) {
	return (HP_Packet *)0;
}

HP_Packet* HP_PacketAlloc(ssize_t size) {
	HP_Packet *packet = malloc(size);
	if(packet == NULL) {
		return NULL;
	}
	packet->prefix[0] = 'H';
	packet->prefix[1] = 'P';
	return packet;
}

HP_Packet* HP_PacketAllocWithKey(const char *key) {
	ssize_t keyLength = strlen(key);
	ssize_t packetLength = HP_PACKET_HEADER_LEN + keyLength;
	HP_Packet *packet = malloc(packetLength);
	if(packet == NULL) {
		return NULL;
	}
	packet->prefix[0] = 'H';
	packet->prefix[1] = 'P';
	packet->type = HP_Request;
	packet->keyLength = keyLength;
	packet->sessionID = 0;
	memcpy(HP_GetDataPtr(packet), key, keyLength);
	return packet;
}

char* HP_GetDataPtr(const void *packet) {
	return (char *)(packet + HP_PACKET_HEADER_LEN);
}

char* HP_GetTvPtr(HP_Packet *packet) {
	return (char *)(((void *)packet) + HP_PACKET_HEADER_LEN + packet->keyLength);
}
