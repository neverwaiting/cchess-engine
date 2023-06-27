#include "player_piece.h"

namespace wsun
{
namespace cchess
{
namespace cppupdate
{

template <>
int Piece::generateMvs<GENERAL>(const PieceArray& pieces, int* mvs)
{
	if (!show()) return 0;
	return generateGeneralMovesFunc_(this, pieces, mvs);
}

template <>
int Piece::generateMvs<CAPTURE>(const PieceArray& pieces, int* mvs)
{
	if (!show()) return 0;
	return generateCaptureMovesFunc_(this, pieces, mvs);
}

void Piece::bindGenerateMovesFunc()
{
  bool red_side = sidePlayer_->side() == SIDE_TYPE_RED;
  switch (type_)
  {
    case PIECE_TYPE_KING:
      name = red_side ? "帅" : "将";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_KING>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_KING,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_KING,CAPTURE>;
      break;
    case PIECE_TYPE_ADVISOR:
      name = red_side ? "仕" : "士";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_ADVISOR>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_ADVISOR,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_ADVISOR,CAPTURE>;
      break;
    case PIECE_TYPE_BISHOP:
      name = red_side ? "相" : "象";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_BISHOP>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_BISHOP,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_BISHOP,CAPTURE>;
      break;
    case PIECE_TYPE_KNIGHT:
      name = "马";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_KNIGHT>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_KNIGHT,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_KNIGHT,CAPTURE>;
      break;
    case PIECE_TYPE_ROOK:
      name = "车";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_ROOK>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_ROOK,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_ROOK,CAPTURE>;
      break;
    case PIECE_TYPE_CANNON:
      name = "炮";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_CANNON>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_CANNON,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_CANNON,CAPTURE>;
      break;
    case PIECE_TYPE_PAWN:
      name = red_side ? "兵" : "卒";
      legalmoveFunc_ = legalMovePiece<PIECE_TYPE_PAWN>;
      generateGeneralMovesFunc_ = generateMoves<PIECE_TYPE_PAWN,GENERAL>;
      generateCaptureMovesFunc_ = generateMoves<PIECE_TYPE_PAWN,CAPTURE>;
      break;
    default:
      break;
  }
}

} // namespace cppupdate
} // namespace cchess
} // namespace wsun

