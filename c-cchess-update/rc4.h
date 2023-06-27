#ifndef WSUN_CCHESS_RC4_H
#define WSUN_CCHESS_RC4_H

#include <inttypes.h>

struct rc4
{
	int x; 
	int y; 
	uint8_t state[256];
};

void rc4_swap(struct rc4* rc4, int i, int j);

void rc4_init(struct rc4* rc4);

uint8_t rc4_next_byte(struct rc4* rc4);

uint32_t rc4_next_long(struct rc4* rc4);

#endif
