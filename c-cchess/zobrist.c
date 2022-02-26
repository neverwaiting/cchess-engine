#include "zobrist.h"

void zobrist_init(struct zobrist* zobrist, struct rc4* rc4)
{
	zobrist->key = rc4_next_long(rc4);
	zobrist->lock1 = rc4_next_long(rc4);
	zobrist->lock2 = rc4_next_long(rc4);
}
