#include <assert.h>
#include "generate_move.h"
#include "player_piece.h"
#include "utils.h"

int legal_move_king(struct piece* piece, struct piece* const* pieces, int dest)
{
	if (in_fort(dest) && array_legal_span[dest - piece->pos + 256] == 1) return 1;
	int offset = get_offset(piece->pos, dest);
	if (offset == 0) return 0;
	int cur_pos = piece->pos + offset;
	while (cur_pos != dest && !pieces[cur_pos])
	{
		cur_pos += offset;
	}
	return cur_pos == dest && compare_piece_opponent_side(piece, pieces[dest]) && pieces[dest]->type == PIECE_TYPE_KING;
}

int legal_move_advisor(struct piece* piece, struct piece* const* pieces, int dest)
{
	return in_fort(dest) && array_legal_span[dest - piece->pos + 256] == 2;
}

int legal_move_bishop(struct piece* piece, struct piece* const* pieces, int dest)
{
	return same_half(piece->pos, dest) &&
				 array_legal_span[dest - piece->pos + 256] == 3 &&
				 !pieces[((piece->pos + dest) >> 1)];
}

int legal_move_knight(struct piece* piece, struct piece* const* pieces, int dest)
{
	return piece->pos != piece->pos + array_knight_pin[dest - piece->pos + 256] && 
				 !pieces[(piece->pos + array_knight_pin[dest - piece->pos + 256])];
}

int legal_move_rook(struct piece* piece, struct piece* const* pieces, int dest)
{
	int offset = get_offset(piece->pos, dest);
	if (offset == 0) return 0;

	int cur_pos = piece->pos + offset;
	while (cur_pos != dest && !pieces[cur_pos])
	{
		cur_pos += offset;
	}
	return cur_pos == dest && !compare_piece_same_side(piece, pieces[dest]);
}

int legal_move_cannon(struct piece* piece, struct piece* const* pieces, int dest)
{
	int offset = get_offset(piece->pos, dest);
	if (offset == 0) return 0;
	int cur_pos = piece->pos + offset;
	while (cur_pos != dest && !pieces[cur_pos])
	{
		cur_pos += offset;
	}
	if (cur_pos == dest)
	{
			return !pieces[dest];
	}
	else
	{
		cur_pos += offset;
		while (cur_pos != dest && !pieces[cur_pos])
		{
			cur_pos += offset;
		}
		return cur_pos == dest && compare_piece_opponent_side(piece, pieces[dest]);
	}
}

int legal_move_pawn(struct piece* piece, struct piece* const* pieces, int dest)
{
	if (!same_half(piece->side_player->king_piece->pos, piece->pos) && 
			(piece->pos + 1 == dest || piece->pos - 1 == dest))
	{
		return 1;
	}
	return dest == piece_forward_step(piece);
}

int generate_moves_king(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	// 九宫内的走法
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_king_delta[i];
		if (!in_fort(dest)) continue;

		if (!compare_piece_same_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_moves_advisor(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_advisor_delta[i];
		if (!in_fort(dest)) continue;

		if (!compare_piece_same_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_moves_bishop(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_advisor_delta[i];

		if (!in_board(dest) || 
				!same_half(piece->pos, dest) || 
				pieces[dest])
		{
			continue;
		}

		dest += array_advisor_delta[i];
		
		if (!compare_piece_same_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_moves_knight(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_king_delta[i];
		if (pieces[dest]) continue;

		for (int j = 0; j < 2; ++j)
		{
			dest = piece->pos + array_knight_delta[i][j];
			if (!in_board(dest)) continue;

			if (!compare_piece_same_side(piece, pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos, dest);
			}
		}
	}
	return nums;
}

int generate_moves_rook(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			mvs[nums++] = get_move(piece->pos, dest);
			dest += nDelta;
		}
		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_moves_cannon(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos + nDelta;
		while (in_board(dest))
		{
			if (!pieces[dest])
			{
				mvs[nums++] = get_move(piece->pos, dest);
			}
			else break;

			dest += nDelta;
		}
		dest += nDelta;
		while (in_board(dest))
		{
			if (!pieces[dest]) dest += nDelta;
			else
			{
				if (compare_piece_opponent_side(piece, pieces[dest]))
				{
					mvs[nums++] = get_move(piece->pos, dest);
				}
				break;
			}
		}
	}
	return nums;
}

int generate_moves_pawn(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	int dest = piece_forward_step(piece);
	if (in_board(dest))
	{
		// capatured move
		if (!compare_piece_same_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}

	// 过河兵
	if (!same_half(piece->side_player->king_piece->pos, piece->pos))
	{
		for (int nDelta = -1; nDelta <= 1; nDelta += 2)
		{
			dest = piece->pos + nDelta;

			if (!in_board(dest)) continue;

			// capatured move
			if (!compare_piece_same_side(piece, pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos, dest);
			}
		}
	}
	return nums;
}

int generate_capture_moves_king(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	// 九宫内的走法
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_king_delta[i];
		if (!in_fort(dest)) continue;

		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_capture_moves_advisor(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_advisor_delta[i];
		if (!in_fort(dest)) continue;

		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_capture_moves_bishop(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_advisor_delta[i];

		if (!in_board(dest) || 
				!same_half(piece->pos, dest) || 
				pieces[dest])
		{
			continue;
		}

		dest += array_advisor_delta[i];

		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_capture_moves_knight(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos + array_king_delta[i];
		if (pieces[dest]) continue;

		for (int j = 0; j < 2; ++j)
		{
			dest = piece->pos + array_knight_delta[i][j];
			if (!in_board(dest)) continue;

			if (compare_piece_opponent_side(piece, pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos, dest);
			}
		}
	}
	return nums;
}

int generate_capture_moves_rook(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_capture_moves_cannon(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		dest += nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}
	return nums;
}

int generate_capture_moves_pawn(struct piece* piece, struct piece* const* pieces, int* mvs)
{
	int nums = 0;
	int dest = piece_forward_step(piece);
	if (in_board(dest))
	{
		// capatured move
		if (compare_piece_opponent_side(piece, pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos, dest);
		}
	}

	// 过河兵
	if (!same_half(piece->side_player->king_piece->pos, piece->pos))
	{
		for (int nDelta = -1; nDelta <= 1; nDelta += 2)
		{
			dest = piece->pos + nDelta;

			if (!in_board(dest)) continue;

			// capatured move
			if (compare_piece_opponent_side(piece, pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos, dest);
			}
		}
	}
	return nums;
}

