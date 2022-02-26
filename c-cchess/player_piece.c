#include "player_piece.h"

struct piece* piece_create(int type, struct player* side_player, int pos)
{
	struct piece* piece = (struct piece*)malloc(sizeof(struct piece));
	memset(piece, 0, sizeof(struct piece));
	piece->side_player = side_player;
	piece->type = type;
	piece->pos = pos;
	piece->show = 1;
	int red_side = side_player->side == SIDE_TYPE_RED;
	if (type == PIECE_TYPE_KING)
		strcpy(piece->name, (red_side ? "帅" : "将"));
	else if (type == PIECE_TYPE_ADVISOR)
		strcpy(piece->name, (red_side ? "仕" : "士"));
	else if (type == PIECE_TYPE_BISHOP)
		strcpy(piece->name, (red_side ? "相" : "象"));
	else if (type == PIECE_TYPE_KNIGHT)
		strcpy(piece->name, "马");
	else if (type == PIECE_TYPE_ROOK)
		strcpy(piece->name, "车");
	else if (type == PIECE_TYPE_CANNON)
		strcpy(piece->name, "炮");
	else if (type == PIECE_TYPE_PAWN)
		strcpy(piece->name, (red_side ? "兵" : "卒"));
	return piece;
}

void piece_release(struct piece* p)
{
	free(p);
}

struct player* player_create(int side)
{
	struct player* player = (struct player*)malloc(sizeof(struct player));
	memset(player, 0, sizeof(struct player));
	player->side = side;
	return player;
}

void player_release(struct player* ply)
{
	player_pieces_release(ply);
	free(ply);
}

void player_pieces_release(struct player* ply)
{
	for (int i = 0; i < ply->pieces_size; ++i)
	{
		piece_release(ply->pieces[i]);
	}
	memset(ply->pieces, 0, 16 * sizeof(struct piece*));
	ply->pieces_size = 0;
}

void player_reset(struct player* ply)
{
	ply->value = 0;
	ply->king_piece = NULL;
	player_pieces_release(ply);
}

void player_add_piece(struct player* ply, struct piece* p)
{
	ply->pieces[ply->pieces_size++] = p;
	if (p->type == PIECE_TYPE_KING)
	{
		ply->king_piece = p;
	}
	player_add_piece_value(ply, p);
}
