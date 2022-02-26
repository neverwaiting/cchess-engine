#ifndef __WSUN_CCHESS_SEARCH_ENGINE_H__
#define __WSUN_CCHESS_SEARCH_ENGINE_H__

#include <vector>
#include <inttypes.h>
#include "board.h"

namespace wsun
{
namespace cchess
{

#define MAX_GENERATE_MOVES 128
#define LIMIT_DEPTH 64 // 最大的搜索深度
#define HISTORY_HEURISTIC_TABLE_SIZE (1 << 16)
#define TRANSPOSITION_TABLE_SIZE (1 << 20)

//static const char* OPENBOOK_FILE_PATH = "../cchess-cpp-release/BOOK.DAT";
static const char* OPENBOOK_FILE_PATH = "../BOOK.DAT";

static const int MATE_VALUE = 10000; // 最高分值，即将死的分值
static const int BAN_VALUE = MATE_VALUE - 100;
static const int WIN_VALUE = MATE_VALUE - 200; // 搜索出胜负的分值界限，超出此值就说明已经搜索出杀棋了
static const int DRAW_VALUE = 20;
static const int ADVANCED_VALUE = 3; // 先行权分值

// 空步裁剪参数
static const int NULL_SAFE_MARGIN = 400;
static const int NULL_OKAY_MARGIN = 200;
static const int NULL_DEPTH = 2;

struct tt_item
{
	uint8_t depth;	// 深度
	uint8_t flag;		// alpha、beta、pv 三种节点类型
	int value;			// 局面分数值
	int mv;					// 走法
	uint32_t checksum_lower32;  // 局面zobrist校验值checksum 64位
	uint32_t checksum_higher32; 
};

// 开局库
class OpenBook
{
public:
	// 开局库条目信息
	struct BookItem
	{
		uint32_t lock_;
		uint16_t mv_;
		uint16_t value_;
	};

	explicit OpenBook(const char* filePath)
	{
		load(filePath);
	}

	// 加载开局库
	void load(const char* filePath)
	{
		FILE* fp = fopen(filePath, "rb");
		if (fp)
		{
			BookItem item;
			int n = 0;
			while ((n = fread(&item, sizeof(BookItem), 1, fp)) > 0)
				bookItems_.emplace_back(item);

			fclose(fp);
			printf("load success!\n");
		}
	}

	int findBestMove(uint32_t checksum, uint32_t mirrorChecksum)
	{
		int mv = 0;

		// 从开局库寻找最佳着法
		auto res = findBestMoveInner(checksum);
		if (res.first)
		{
			mv = res.second;
		}
		else
		{
			// 从开局库寻找镜像的最佳着法
			res = findBestMoveInner(mirrorChecksum);
			if (res.first)
			{
				mv = convert_mirror_move(res.second);
			}
		}
		return mv;
	}

private:
	std::pair<bool,int> findBestMoveInner(uint32_t checksum)
	{
		std::pair<bool,int> res(false, 0);
		auto it = std::lower_bound(bookItems_.begin(), bookItems_.end(), checksum, 
				[](const BookItem& item, uint32_t lock_)
				{
					return item.lock_ < lock_;
				});

		// 可能不止一种好的着法
		std::vector<int> bestMoves;
		while (it != bookItems_.end() && it->lock_ == checksum)
		{
			bestMoves.push_back(it->mv_);
			++it;
		}

		// 找到最佳着法
		if (!bestMoves.empty())
		{
			//printf("best move in openbook num: %lu\n", bestMoves.size());
			// 随机选取其中一种好的着法
			srand(time(NULL));
			size_t i = rand() % bestMoves.size();
			res.first = true;
			res.second = bestMoves[i];
		}
		return res;
	}

	std::vector<BookItem> bookItems_; // 按照校验值进行有序排列的
};

class SearchEngine
{
public:
	SearchEngine(Board* board) : board_(board), openBook_(OPENBOOK_FILE_PATH) { }
	void reset()
	{
		distance_ = 0;
		allNodes_ = 0;
		memset(historyHeuristicTable_, 0, sizeof(int) * HISTORY_HEURISTIC_TABLE_SIZE);
		memset(killerHeuristicTable_, 0, sizeof(int) * 2 * LIMIT_DEPTH);
		memset(transpositionTable_, 0, sizeof(struct tt_item) * TRANSPOSITION_TABLE_SIZE);
	}

	int search(int milliseconds);

	const int *getHistoryHeuristicTable() const { return historyHeuristicTable_; }
	Board* board() const { return board_; }
	int getKillerMove(int which) const
	{
		return killerHeuristicTable_[distance_][which];
	}

private:
	int mateValue() const { return distance_ - MATE_VALUE; }
	int banValue() const { return distance_ - BAN_VALUE; }
	int drawValue() const 
	{
		return (distance_ & 1) == 0  ? -DRAW_VALUE : DRAW_VALUE;
	}

	bool nullOkay() const
	{
		return board_->currentSidePlayer()->value() > NULL_OKAY_MARGIN;
	}
	bool nullSafe() const
	{
		return board_->currentSidePlayer()->value() > NULL_SAFE_MARGIN;
	}

	void setBestMove(int mv, int depth)
	{
		historyHeuristicTable_[mv] += depth * depth;
		int* killer_table = killerHeuristicTable_[depth];
		if (mv == killer_table[0])
		{
			killer_table[1] = killer_table[0];
			killer_table[0] = mv;
		}
	}

	void doNullMove()
	{
		++distance_;
		board_->makeNullMove();
		board_->changeSide();
	}

	void undoNullMove()
	{
		--distance_;
		board_->undoNullMove();
		board_->changeSide();
	}

	bool makeMove(int mv)
	{
		board_->makeMove(mv);
		if(board_->willKillSelfKing())
		{
			board_->undoMove();
			return false;
		}
		++distance_;
		board_->changeSide();
		return true;
	}

	void undoMove()
	{
		board_->undoMove();
		board_->changeSide();
		--distance_;
	}

	int repetitionValue(int rep) const
	{
		int vl = ((rep & 2) == 0 ? 0 : banValue()) + 
						 ((rep & 4) == 0 ? 0 : -banValue());
		return (vl == 0 ? drawValue() : vl);
	}

	int transpositionTableGrab(int vlAlpha, int vlBeta, int depth, int* mv);
	void transpositionTableInsert(int flag, int value, int depth, int mv);

	int searchQuiescence(int valueAlpha, int valueBeta);
	int searchFull(int valueAlpha, int valueBeta, int depth, int nonull);
	int searchRoot(int depth);
	
private:
	Board* board_;
	int distance_;
	int allNodes_;
	int mvBest_;
	int historyHeuristicTable_[HISTORY_HEURISTIC_TABLE_SIZE];
	int killerHeuristicTable_[LIMIT_DEPTH][2];
	tt_item transpositionTable_[TRANSPOSITION_TABLE_SIZE];

	OpenBook openBook_;
};

} // namespace cchess
} // namespace wsun

#endif
