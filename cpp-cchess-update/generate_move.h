#ifndef __WSUN_CCHESS_CPP_UPDATE_GENERATE_MOVE_H__
#define __WSUN_CCHESS_CPP_UPDATE_GENERATE_MOVE_H__

#include "utils.h"

namespace wsun
{
namespace cchess
{
namespace cppupdate
{

template <PieceType pt>
bool legalMovePiece(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_KING>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_ADVISOR>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_BISHOP>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_KNIGHT>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_ROOK>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_CANNON>(Piece* piece, const PieceArray& pieces, int dest);

template <>
bool legalMovePiece<PIECE_TYPE_PAWN>(Piece* piece, const PieceArray& pieces, int dest);

template <PieceType pt, MoveGenerateType mgt>
int generateMoves(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_KING, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_ADVISOR, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_BISHOP, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_KNIGHT, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_ROOK, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_CANNON, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_PAWN, GENERAL>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_KING, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_ADVISOR, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_BISHOP, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_KNIGHT, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_ROOK, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_CANNON, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

template <>
int generateMoves<PIECE_TYPE_PAWN, CAPTURE>(Piece* piece, const PieceArray& pieces, int* mvs);

}
}
}

#endif

