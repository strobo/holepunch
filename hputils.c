/*
 * hputils.c
 * Copyright @_strobo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "hputils.h"

uint32_t random32bit() {
	ssize_t r;
	uint32_t rand;
	int fd = open("/dev/urandom", O_RDONLY);

	if(fd == 0) {
		return -1;
	}
	r = read(fd, &rand, sizeof(rand));
	if(r < 0) {
		return -1;
	}
	close(fd);
	return rand;
}

uint32_t random32bitWithKey(char *key) {
	ssize_t r;
	uint32_t rand;
	int fd = open("/dev/urandom", O_RDWR);

	if(fd == 0) {
		return -1;
	}
	r = write(fd, key, strlen(key));
	if(r < 0) {
		return -1;
	}
	r = read(fd, &rand, sizeof(rand));
	if(r < 0) {
		return -1;
	}
	close(fd);
	return rand;
}
