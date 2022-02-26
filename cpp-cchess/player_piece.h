#ifndef __WSUN_CCHESS_PLAYER_PIECE_H__
#define __WSUN_CCHESS_PLAYER_PIECE_H__

#include <algorithm>
#include <stdlib.h>
#include <string>
#include "constants.h"

namespace wsun
{
namespace cchess
{

static const int SIDE_TYPE_RED = 0;
static const int SIDE_TYPE_BLACK = 1;

class Player;

class Piece
{
public:
	Piece(int type, Player* sidePlayer, int pos)
		: sidePlayer_(sidePlayer), type_(type), pos_(pos), show_(true)
	{
	}

	Player* sidePlayer() const { return sidePlayer_; }
	int type() const { return type_; }
	void setType(int type) { type_ = type; }
	int pos() const { return pos_; }
	void setPos(int pos) { pos_ = pos; }
	bool show() const { return show_; }
	void setShow(int show) { show_ = show; }

	// 判断两个棋子是否同边
	bool sameSide(Piece* p) const
	{
		return p && sidePlayer_ == p->sidePlayer_;
	}

	// 判断两个棋子是否为对立面
	bool oppSide(Piece* p) const
	{
		return p && sidePlayer_ != p->sidePlayer_;
	}

	// 棋子的子力价值
	int value() const;
	int forwardStep() const;

private:
	Player* sidePlayer_;				  // 棋子所属玩家( 红方或者黑方)
	int type_;										// 棋子的类型（7种类型）
	int pos_;											// 棋子在棋盘上的位置
	bool show_;										// 棋子是否在棋盘上的标志位
	std::string name;							// 棋子的名称
};

class Player
{
public:
	Player(int side) : side_(side), value_(0), kingPiece_(nullptr), piecesNum_(0)
	{ 
		initPieceArray();
	}

	~Player()
	{
		releasePieceArray();
	}

	void initPieceArray()
	{
		std::fill(pieces_, pieces_ + 16, nullptr);
	}
	void releasePieceArray()
	{
		for(int i = 0; i < piecesNum_; ++i)
		{
			delete pieces_[i];
			pieces_[i] = nullptr;
		}
		piecesNum_ = 0;
	}

	void reset()
	{
		value_ = 0;
		kingPiece_ = nullptr;
		releasePieceArray();
	}

	int side() const { return side_; }
	int value() const { return value_; }
	Piece* kingPiece() const { return kingPiece_; }
	Piece** pieces() { return pieces_; }
	int piecesNum() const { return piecesNum_; }

	void addPieceValue(Piece* p)
	{
		value_ += p->value();
	}
	void delPieceValue(Piece* p)
	{
		value_ -= p->value();
	}

	void addPiece(Piece* p)
	{
		pieces_[piecesNum_++] = p;
		if (p->type() == PIECE_TYPE_KING)
		{
			kingPiece_ = p;
		}
		addPieceValue(p);
	}

private:
	int side_;							// 红黑方
	int value_;							// 玩家的子力价值总分数
	Piece* kingPiece_;			// 玩家的将或帥棋子
	Piece* pieces_[16];			// 玩家的所有棋子
	int piecesNum_;
};

inline int Piece::value() const
{
	int pos = sidePlayer_->side() == SIDE_TYPE_RED ? pos_ : 254 - pos_;
	return array_piece_value[type_][pos];
}

inline int Piece::forwardStep() const
{
	// down side 帥(将)在下方 -16 ,否则+16
	return ((sidePlayer_->kingPiece()->pos() & 0x80) ? pos_ - 16 : pos_ + 16);
}

} // namespace cchess
} // namespace wsun

#endif
