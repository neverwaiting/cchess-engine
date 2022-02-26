#ifndef __WSUN_CCHESS_ZOBRIST_HELPER_H__
#define __WSUN_CCHESS_ZOBRIST_HELPER_H__

#include "utils.h"
#include <inttypes.h>
#include <array>

namespace wsun
{
namespace cchess
{
// RC4密码流
class RC4
{
public:
	RC4() : x_(0), y_(0)
	{
		int i = 0;
		int j = 0;
		for (i = 0; i < state_.size(); ++i)
		{
			state_[i] = i;
		}
		for (i = 0; i < state_.size(); ++i)
		{
			j = (j + state_[i]) & 0xff;
			std::swap(state_[i], state_[j]);
		}
	}

	uint8_t nextByte()
	{
		x_ = (x_ + 1) & 0xff;
		y_ = (y_ + state_[x_]) & 0xff;
		std::swap(state_[x_], state_[y_]);
		return state_[(state_[x_] + state_[y_]) & 0xff];
	}

	uint32_t nextLong()
	{
		return nextByte() + (nextByte() << 8) + (nextByte() << 16) + (nextByte() << 24);
	}

private:
	int x_;
	int y_;
	std::array<uint8_t, 256> state_;
};

struct Zobrist
{
	Zobrist() : key_(0), lock1_(0), lock2_(0)
	{
	}

	Zobrist(RC4& rc4)
	{
		key_ = rc4.nextLong();
		lock1_ = rc4.nextLong();
		lock2_ = rc4.nextLong();
	}

	void reset()
	{
		key_ = lock1_ = lock2_ = 0;
	}

	void zXOR(const Zobrist& rhs)
	{
		key_ ^= rhs.key_;
		lock1_ ^= rhs.lock1_;
		lock2_ ^= rhs.lock2_;
	}

	uint32_t key_;
	uint32_t lock1_;
	uint32_t lock2_;
};

class ZobristHelper {
public:
	ZobristHelper()
	{
		RC4 rc4;
		zobristPlayer_ = Zobrist(rc4);
		for (int side = 0; side < 2; ++side)
		{
			for (int type = PIECE_TYPE_KING; type <= PIECE_TYPE_PAWN; ++type)
			{
				for (int pos = 0; pos < 256; ++pos)
				{
					pieceZoristTable_[side][type][pos] = Zobrist(rc4);
				}
			}
		}
	}

	void reset() 
	{
		zobrist_ = Zobrist();
	}

	const Zobrist& getZobrist() const
	{
		return zobrist_;
	}

	void updateByChangeSide()
	{
		zobrist_.zXOR(zobristPlayer_);
	}

	void updateByChangePiece(int side, int type, int pos)
	{
		zobrist_.zXOR(pieceZoristTable_[side][type][pos]);
	}

private:
	// 对应当前局面的一个值，用来区分每一个不同的局面
	Zobrist zobrist_;				
	// 用来标志下棋方的一个zobrist值
	Zobrist zobristPlayer_;
	// 每种棋子在每种位置所对应的一个zobrist值
	Zobrist pieceZoristTable_[2][7][256];	
};

} // namespace cchess
} // namespace wsun

#endif
