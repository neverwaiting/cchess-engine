#ifndef WSUN_CCHESS_ZOBRIST_H
#define WSUN_CCHESS_ZOBRIST_H

#include <inttypes.h>
#include "rc4.h"

struct zobrist
{
	uint32_t key;
	uint32_t lock1; //lock1 and lock2 as a 64 bit checksum
	uint32_t lock2;
};

void zobrist_init(struct zobrist* zobrist, struct rc4* rc4);

static inline void zobrist_xor(struct zobrist* zobrist, const struct zobrist* rhs)
{
	zobrist->key ^= rhs->key;
	zobrist->lock1 ^= rhs->lock1;
	zobrist->lock2 ^= rhs->lock2;
}

#endif
