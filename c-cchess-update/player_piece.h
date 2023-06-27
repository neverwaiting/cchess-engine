#ifndef WSUN_CCHESS_PLAYER_PIECE_H
#define WSUN_CCHESS_PLAYER_PIECE_H

#include <stdlib.h>
#include <string.h>
#include "constants.h"

struct piece;
typedef int (*LEGAL_MOVE_FUNC)(struct piece*, struct piece* const* piece_array, int dest);
typedef int (*GENERATE_MOVES_FUNC)(struct piece*, struct piece* const* piece_array, int*);

struct piece
{
	struct player* side_player; // 棋子所属玩家( 红方或者黑方)
	int type;										// 棋子的类型（7种类型）
	int pos;										// 棋子在棋盘上的位置
	int show;										// 棋子是否在棋盘上的标志位
	char name[4];								// 棋子的名称
  LEGAL_MOVE_FUNC legal_move_func;
  GENERATE_MOVES_FUNC generate_moves_func;
  GENERATE_MOVES_FUNC generate_captured_moves_func;
};

struct player
{
	int side;										// 红黑方
	int value;									// 玩家的子力价值总分数
	struct piece* king_piece;		// 玩家的将或帥棋子
	struct piece* pieces[16];	  // 玩家的所有棋子
	int pieces_size;
};


struct piece* piece_create(int type, struct player* side_player, int pos);
void piece_release(struct piece* p);

static inline int
piece_legal_move(struct piece* p, struct piece* const* piece_array, int dest)
{
  return p->legal_move_func(p, piece_array, dest);
}

static inline int
piece_generate_moves(struct piece* p, struct piece* const* piece_array, int* mvs)
{
  if (!p->show) return 0;
  return p->generate_moves_func(p, piece_array, mvs);
}

static inline int
piece_generate_capture_moves(struct piece* p, struct piece* const* piece_array, int* mvs)
{
  if (!p->show) return 0;
  return p->generate_captured_moves_func(p, piece_array, mvs);
}

// 判断两个棋子是否同边
static inline int
compare_piece_same_side(const struct piece* p1, const struct piece* p2)
{
	return p2 && p1->side_player->side == p2->side_player->side;
}

// 判断两个棋子是否为对立面
static inline int
compare_piece_opponent_side(const struct piece* p1, const struct piece* p2)
{
	return p2 && p1->side_player->side != p2->side_player->side;
}

// 棋子的子力价值
static inline int get_piece_value(const struct piece* p)
{
	int pos = p->side_player->side == SIDE_TYPE_RED ? p->pos : 254 - p->pos;
	return array_piece_value[p->type][pos];
}

static inline int piece_forward_step(const struct piece* piece)
{
	// down side 帥(将)在下方 -16 ,否则+16
	return ((piece->side_player->king_piece->pos & 0x80) ? 
			piece->pos - 16 : piece->pos + 16);
}

struct player* player_create(int side);

void player_release(struct player* ply);

void player_pieces_release(struct player* ply);

void player_reset(struct player* ply);

static inline void player_add_piece_value(struct player* ply, const struct piece* p)
{
	ply->value += get_piece_value(p);
}

static inline void player_del_piece_value(struct player* ply, const struct piece* p)
{
	ply->value -= get_piece_value(p);
}

void player_add_piece(struct player* ply, struct piece* p);

#endif

