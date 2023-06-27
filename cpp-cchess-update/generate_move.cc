#include "generate_move.h"
#include "player_piece.h"
#include <assert.h>

namespace wsun
{
namespace cchess
{
namespace cppupdate
{

template <>
bool legalMovePiece<PIECE_TYPE_KING>(Piece* piece, const PieceArray& pieces, int dest)
{
	if (in_fort(dest) && array_legal_span[dest - piece->pos() + 256] == 1) return true;
	int offset = get_offset(piece->pos(), dest);
	if (offset == 0) return false;
	int curPos = piece->pos() + offset;
	while (curPos != dest && !pieces[curPos])
	{
		curPos += offset;
	}
	return curPos == dest && piece->oppSide(pieces[dest]) && pieces[dest]->type() == PIECE_TYPE_KING;
}

template <>
bool legalMovePiece<PIECE_TYPE_ADVISOR>(Piece* piece, const PieceArray& pieces, int dest)
{
	return in_fort(dest) && array_legal_span[dest - piece->pos() + 256] == 2;
}

template <>
bool legalMovePiece<PIECE_TYPE_BISHOP>(Piece* piece, const PieceArray& pieces, int dest)
{
	return same_half(piece->pos(), dest) &&
				 array_legal_span[dest - piece->pos() + 256] == 3 &&
				 !pieces[((piece->pos() + dest) >> 1)];
}

template <>
bool legalMovePiece<PIECE_TYPE_KNIGHT>(Piece* piece, const PieceArray& pieces, int dest)
{
	return piece->pos() != piece->pos() + array_knight_pin[dest - piece->pos() + 256] && 
				 !pieces[(piece->pos() + array_knight_pin[dest - piece->pos() + 256])];
}

template <>
bool legalMovePiece<PIECE_TYPE_ROOK>(Piece* piece, const PieceArray& pieces, int dest)
{
	int offset = get_offset(piece->pos(), dest);
	if (offset == 0) return false;

	int curPos = piece->pos() + offset;
	while (curPos != dest && !pieces[curPos])
	{
		curPos += offset;
	}
	return curPos == dest && !piece->sameSide(pieces[dest]);
}

template <>
bool legalMovePiece<PIECE_TYPE_CANNON>(Piece* piece, const PieceArray& pieces, int dest)
{
	int offset = get_offset(piece->pos(), dest);
	if (offset == 0) return false;
	int curPos = piece->pos() + offset;
	while (curPos != dest && !pieces[curPos])
	{
		curPos += offset;
	}
	if (curPos == dest)
	{
			return !pieces[dest];
	}
	else
	{
		curPos += offset;
		while (curPos != dest && !pieces[curPos])
		{
			curPos += offset;
		}
		return curPos == dest && piece->oppSide(pieces[dest]);
	}
}

template <>
bool legalMovePiece<PIECE_TYPE_PAWN>(Piece* piece, const PieceArray& pieces, int dest)
{
	if (!same_half(piece->sidePlayer()->kingPiece()->pos(), piece->pos()) && 
			(piece->pos() + 1 == dest || piece->pos() - 1 == dest))
	{
		return true;
	}
	return dest == piece->forwardStep();
}

template <>
int generateMoves<PIECE_TYPE_KING, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	// 九宫内的走法
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_king_delta[i];
		if (!in_fort(dest)) continue;

		if (!piece->sameSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_ADVISOR, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_advisor_delta[i];
		if (!in_fort(dest)) continue;

		if (!piece->sameSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_BISHOP, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_advisor_delta[i];

		if (!in_board(dest) || 
				!same_half(piece->pos(), dest) || 
				pieces[dest])
		{
			continue;
		}

		dest += array_advisor_delta[i];
		
		if (!piece->sameSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_KNIGHT, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_king_delta[i];
		if (pieces[dest]) continue;

		for (int j = 0; j < 2; ++j)
		{
			dest = piece->pos() + array_knight_delta[i][j];
			if (!in_board(dest)) continue;

			if (!piece->sameSide(pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_ROOK, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos() + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			mvs[nums++] = get_move(piece->pos(), dest);
			dest += nDelta;
		}
		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_CANNON, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos() + nDelta;
		while (in_board(dest))
		{
			if (!pieces[dest])
			{
				mvs[nums++] = get_move(piece->pos(), dest);
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
				if (piece->oppSide(pieces[dest]))
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
				break;
			}
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_PAWN, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	int dest = piece->forwardStep();
	if (in_board(dest))
	{
		// capatured move
		if (!piece->sameSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}

	// 过河兵
	if (!same_half(piece->sidePlayer()->kingPiece()->pos(), piece->pos()))
	{
		for (int nDelta = -1; nDelta <= 1; nDelta += 2)
		{
			dest = piece->pos() + nDelta;

			if (!in_board(dest)) continue;

			// capatured move
			if (!piece->sameSide(pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_KING, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	// 九宫内的走法
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_king_delta[i];
		if (!in_fort(dest)) continue;

		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_ADVISOR, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_advisor_delta[i];
		if (!in_fort(dest)) continue;

		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_BISHOP, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_advisor_delta[i];

		if (!in_board(dest) || 
				!same_half(piece->pos(), dest) || 
				pieces[dest])
		{
			continue;
		}

		dest += array_advisor_delta[i];

		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_KNIGHT, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int dest = piece->pos() + array_king_delta[i];
		if (pieces[dest]) continue;

		for (int j = 0; j < 2; ++j)
		{
			dest = piece->pos() + array_knight_delta[i][j];
			if (!in_board(dest)) continue;

			if (piece->oppSide(pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_ROOK, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos() + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_CANNON, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	for (int i = 0; i < 4; ++i)
	{
		int nDelta = array_king_delta[i];
		int dest = piece->pos() + nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		dest += nDelta;
		while (in_board(dest) && !pieces[dest])
		{
			dest += nDelta;
		}
		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}
	return nums;
}

template <>
int generateMoves<PIECE_TYPE_PAWN, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs)
{
	int nums = 0;
	int dest = piece->forwardStep();
	if (in_board(dest))
	{
		// capatured move
		if (piece->oppSide(pieces[dest]))
		{
			mvs[nums++] = get_move(piece->pos(), dest);
		}
	}

	// 过河兵
	if (!same_half(piece->sidePlayer()->kingPiece()->pos(), piece->pos()))
	{
		for (int nDelta = -1; nDelta <= 1; nDelta += 2)
		{
			dest = piece->pos() + nDelta;

			if (!in_board(dest)) continue;

			// capatured move
			if (piece->oppSide(pieces[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	return nums;
}

}
}
}
