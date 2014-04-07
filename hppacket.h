/*
 * hppacket.h
 * Copyright @_strobo
 */
#ifndef __HP_PACKET_H
#define __HP_PACKET_H
#include <stdint.h>

#define HP_PACKET_HEADER_LEN 8

typedef enum {
	HP_Request = 0,
	HP_Accepted,
	HP_Found,
	HP_Punch,
	HP_Payback,
	HP_Complete,
	HP_Exit,
	HP_Error
} HP_Type_t;

typedef struct {
	uint8_t prefix[2];
	uint8_t type;
	uint8_t  keyLength;
	uint32_t sessionID;
} HP_Packet;

ssize_t HP_PacketLength(HP_Packet *packet);
HP_Packet* HP_PacketAllocByType(HP_Type_t type);
HP_Packet* HP_PacketAlloc(ssize_t size);
HP_Packet* HP_PacketAllocWithKey(const char *key);
char* HP_GetDataPtr(const void *packet);
char* HP_GetTvPtr(HP_Packet *packet);

#endif
