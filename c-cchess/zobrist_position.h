#ifndef __WSUN_CCHESS_ZOBRIST_POSITION_H__
#define __WSUN_CCHESS_ZOBRIST_POSITION_H__

#include "zobrist.h"

struct zobrist_position
{
	// 对应当前局面的一个值，用来区分每一个不同的局面
	struct zobrist zobrist;

	// 用来标志下棋方的一个zobrist值
	struct zobrist zobrist_player;
	// 每种棋子在每种位置所对应的一个zobrist值
	struct zobrist zobrist_piece_table[2][7][256];	
};

struct zobrist_position* zobrist_position_create();

void zobrist_position_release(struct zobrist_position* zp);

void zobrist_position_reset(struct zobrist_position* zp);

static inline void update_by_change_side(struct zobrist_position* zp)
{
	zobrist_xor(&zp->zobrist, &zp->zobrist_player);
}

static inline void update_by_change_piece(struct zobrist_position* zp, int side, int type, int pos)
{
	zobrist_xor(&zp->zobrist, &zp->zobrist_piece_table[side][type][pos]);
}

#endif
