/*
 * nodepool.h
 * Copyright @_strobo
 */
#ifndef __NODEPOOL_H
#define __NODEPOOL_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>

typedef struct NodePool {
	uint32_t sessionID;
	char *key;
	struct sockaddr addr;
	struct NodePool *partner;
	struct NodePool *next;
} NodePool_t;

//typedef enum {
//	NodePool_KeyNotFound = -1,
//	NodePool_NotEnoughMemory = -2,
//} NodePoolError_t;


/*
 * NodepoolAllocWith
 * 成功した場合は生成したnodeのアドレスを返す
 * メモリ不足により失敗した場合はNULLを返す
 */
NodePool_t* NodePoolAllocWith(char *key, uint32_t sessionID, const struct sockaddr *addr);

/*
 * NodePoolAdd
 * 成功した場合は先頭nodeを返す
 * メモリ不足または同じkeyが存在する場合はNULLを返す
 */
NodePool_t* NodePoolAdd(NodePool_t *nodePool, NodePool_t *newNode);

/*
 * NodePoolDel
 * 成功した場合と削除すべきnodeが見つからない場合先頭nodeを返す
 * すべて削除した場合はNULLを返す
 * (削除の結果を返した方がいいかも)
 */
NodePool_t* NodePoolDel(NodePool_t *nodePool, NodePool_t *node);

/*
 * NodePoolGetByKey
 * 成功した場合検索結果のnodeを返す
 * 対象のkeyが見つからない場合はNULLを返す
 */
NodePool_t* NodePoolGetByKey(NodePool_t *nodePool, char *key);

/*
 * NodePoolGetBySessionID
 * 成功した場合検索結果のnodeを返す
 * 対象のkeyが見つからない場合はNULLを返す
 */
NodePool_t* NodePoolGetBySessionID(NodePool_t *nodePool, uint32_t sessionID);

/*
 * NodePoolRelease
 * すべてのnodeを解放する
 */
void NodePoolRelease(NodePool_t *nodePool);

#endif
