/*
 * holepunch.c
 * Copyright @_strobo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hpserver.h"
#include "hpnode.h"
#include "hputils.h"

int help(void) {
	printf("Usage: holepunch server <port>\n");
	printf("                 node <serverAddress[:port]> <key>\n");
	return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
	if(argc >= 2) {
		if(strcmp(argv[1], "server") == 0) {
			char *port = argv[2];

			return runServer(port);
		} else if(strcmp(argv[1], "node") == 0) {
			char *serverAddress = argv[2];
			char *key = argv[3];

			if((key == NULL) || (serverAddress == NULL)) {
				return help();
			}

			if(strlen(key) > 15) {
				ERR("key must less than 15 characters\n");
				return help();
			}

			return runNode(serverAddress, key);
		}
	}

	return help();
}

