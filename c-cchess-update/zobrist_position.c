#include <stdlib.h>
#include <string.h>
#include "zobrist_position.h"
#include "utils.h"

struct zobrist_position* zobrist_position_create()
{
	struct zobrist_position* zp = 
		(struct zobrist_position*)malloc(sizeof(struct zobrist_position));
	memset(zp, 0, sizeof(struct zobrist_position));
	struct rc4 rc4;
	rc4_init(&rc4);
	zobrist_init(&zp->zobrist_player, &rc4);

	for (int side = 0; side < 2; ++side)
	{
		for (int type = PIECE_TYPE_KING; type <= PIECE_TYPE_PAWN; ++type)
		{
			for (int pos = 0; pos < 256; ++pos)
			{
				zobrist_init(&zp->zobrist_piece_table[side][type][pos], &rc4);
			}
		}
	}
	return zp;
}

void zobrist_position_release(struct zobrist_position* zp)
{
	free(zp);
}

void zobrist_position_reset(struct zobrist_position* zp)
{
	memset(&zp->zobrist, 0, sizeof(struct zobrist));
}
