/*
 * nodepool.c
 * Copyright @_strobo
 */
#include <stdlib.h>
#include <string.h>
#include "nodepool.h"


/*
 * NodepoolAllocWith
 * 成功した場合は生成したnodeのアドレスを返す
 * メモリ不足により失敗した場合はNULLを返す
 */
NodePool_t* NodePoolAllocWith(char *key, uint32_t sessionID, const struct sockaddr *addr) {

	NodePool_t *node = malloc(sizeof(NodePool_t));
	if(node == NULL) {
		return NULL;
	}

	node->sessionID = sessionID;

	node->key = malloc(strlen(key));
	if(node->key == NULL) {
		return NULL;
	}
	strcpy(node->key, key);

	memcpy(&node->addr, addr, sizeof(struct sockaddr));
	node->partner = NULL;
	node->next = NULL;

	return node;
}

/*
 * NodePoolAdd
 * 成功した場合は先頭nodeを返す
 * メモリ不足または同じsessionIDが存在する場合はNULLを返す
 */
NodePool_t* NodePoolAdd(NodePool_t *nodePool, NodePool_t *newNode) {
	if(nodePool == NULL) {
		return NULL;
	}

	/* Check sessionID duplication and get last node */
	NodePool_t *latestNode = nodePool;
	while(1) {
		if(latestNode->sessionID == newNode->sessionID) {
			return NULL;
		}
		if(latestNode->next == NULL) {
			/* Append a new node to last node */
			latestNode->next = newNode;
			return nodePool;
		}
		latestNode = latestNode->next;
	}
}

/*
 * NodePoolDel
 * 成功した場合と削除すべきnodeが見つからない場合先頭nodeを返す
 * すべて削除した場合はNULLを返す
 * (削除の結果を返した方がいいかも)
 */
NodePool_t* NodePoolDel(NodePool_t *nodePool, NodePool_t *node) {
	if(nodePool == NULL) {
		return NULL;
	}

	NodePool_t *currentNode = nodePool;
	NodePool_t *previousNode;
	NodePool_t *res;

	if(currentNode == node) {
		res = (currentNode->next ? currentNode->next : NULL);
		free(currentNode->key);
		free(currentNode);
		return res;
	}

	while(currentNode->next) {
		previousNode = currentNode;
		currentNode = currentNode->next;
		if(currentNode == node) {
			previousNode->next = currentNode->next;
			free(currentNode->key);
			free(currentNode);
			return nodePool;
		}
	}
	return nodePool;
}

/*
 * NodePoolGetByKey
 * 成功した場合検索結果のnodeを返す
 * 対象のkeyが見つからない場合はNULLを返す
 */
NodePool_t* NodePoolGetByKey(NodePool_t *nodePool, char *key) {
	if(nodePool == NULL) {
		return NULL;
	}

	do {
		if(strcmp(nodePool->key, key) == 0) {
			return nodePool;
		}
		nodePool = nodePool->next;
	} while(nodePool);

	return NULL;
}

/*
 * NodePoolGetBySessionID
 * 成功した場合検索結果のnodeを返す
 * 対象のkeyが見つからない場合はNULLを返す
 */
NodePool_t* NodePoolGetBySessionID(NodePool_t *nodePool, uint32_t sessionID) {
	if(nodePool == NULL) {
		return NULL;
	}

	do {
		if(nodePool->sessionID == sessionID) {
			return nodePool;
		}
		nodePool = nodePool->next;
	} while(nodePool);

	return NULL;
}

/*
 * NodePoolRelease
 * すべてのnodeを解放する
 */
void NodePoolRelease(NodePool_t *nodePool) {
	if(nodePool == NULL) {
		return;
	}

	NodePool_t *nextNode;

	do {
		nextNode = nodePool->next;
		free(nodePool->key);
		free(nodePool);
		nodePool = nextNode;
	} while(nodePool);

}
