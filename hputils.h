/*
 * hputils.h
 * Copyright @_strobo
 */
#ifndef __HP_UTILS_H
#define __HP_UTILS_H

#include <stdint.h>

#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#define ERR(...) fprintf(stderr, " ERR(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)

uint32_t random32bit(void);
uint32_t random32bitWithKey(char *key);

#endif
