#include "search_engine.h"
#include "board.h"
#include <sys/time.h>
#include <stdio.h>
#include <algorithm>

namespace wsun
{
namespace cchess
{
namespace cppupdate
{

static const int HASH_ALPHA = 1;
static const int HASH_BETA = 2;
static const int HASH_PV = 3;

#define STATE_TT 0
#define STATE_KILLER1	1
#define STATE_KILLER2 2
#define STATE_GENE 3
#define STATE_REST 4

struct CompareByHistory
{
	CompareByHistory(SearchEngine* engine) : engine_(engine)
	{
	}
	bool operator()(int left, int right)
	{
		return engine_->getHistoryHeuristicTable()[right] < engine_->getHistoryHeuristicTable()[left];
	}

	SearchEngine* engine_;
};

struct CompareByMvvLua
{
	CompareByMvvLua(Board* board) : board_(board)
	{
	}
	bool operator()(int left, int right)
	{
		return board_->mvvLva(right) < board_->mvvLva(left);
	}

	Board* board_;
};

// return milliseconds
static uint64_t now()
{
	struct timeval tm;
	gettimeofday(&tm, NULL);
	return tm.tv_sec * 1000000 + tm.tv_usec;
}

// 走法排序生成器
struct moves_generate_sorter
{
	SearchEngine* engine;
	int mvs[MAX_GENERATE_MOVES];
	int mvs_size;
	int mvs_idx;
	int mv_tt;
	int state;
	int killer_move1;
	int killer_move2;
};

void moves_generate_sorter_init(struct moves_generate_sorter* sorter, SearchEngine* engine, int mv_tt)
{
	memset(sorter, 0, sizeof(struct moves_generate_sorter));
	sorter->engine = engine;
	sorter->mv_tt = mv_tt;
	// 如果被将军的话就不能直接用置换表启发和杀手启发走法
	if (engine->board()->willKillSelfKing())
	{
		sorter->state = STATE_REST;
		int n = engine->board()->generateAllMoves<GENERAL>(sorter->mvs);
		sorter->mvs_size = n;
		std::sort(sorter->mvs, sorter->mvs + n, CompareByHistory(engine));
		int tmp_killer_move1 = engine->getKillerMove(0);
		int tmp_killer_move2 = engine->getKillerMove(1);
		if (mv_tt != 0 &&
				std::find(sorter->mvs, sorter->mvs + n, mv_tt) != sorter->mvs + n)
		{
      sorter->mvs_idx = 1;
			sorter->mv_tt = mv_tt;
			sorter->state = STATE_TT;
		}
		if (tmp_killer_move1 != 0 && 
				std::find(sorter->mvs, sorter->mvs + n, tmp_killer_move1) != sorter->mvs + n)
		{
      sorter->mvs_idx = 1;
			sorter->killer_move1 = tmp_killer_move1;
			if (sorter->state == STATE_REST) sorter->state = STATE_KILLER1;
		}
		if (tmp_killer_move2 != 0 && 
				std::find(sorter->mvs, sorter->mvs + n, tmp_killer_move2) != sorter->mvs + n)
		{
      sorter->mvs_idx = 1;
			sorter->killer_move2 = tmp_killer_move2;
      if (sorter->state == STATE_REST) sorter->state = STATE_KILLER2;
		}
	}
	else
	{
		sorter->state = STATE_TT;
		sorter->killer_move1 = engine->getKillerMove(0);
		sorter->killer_move2 = engine->getKillerMove(1);
	}
	//sorter->state = mv_tt == 0 ? STATE_GENE : STATE_TT;
}

int moves_generate_sorter_next_move(struct moves_generate_sorter* sorter)
{
	int n = 0;
	switch(sorter->state)
	{
		case STATE_TT:
			//sorter->state = STATE_GENE;
			sorter->state = STATE_KILLER1;
			if (sorter->mv_tt != 0)
				return sorter->mv_tt;
		case STATE_KILLER1:
			sorter->state = STATE_KILLER2;
			if (sorter->killer_move1 != 0 && 
					sorter->killer_move1 != sorter->mv_tt &&
					sorter->engine->board()->legalMove(sorter->killer_move1))
			{
				// printf("从杀手表1中检索到杀棋\n");
				return sorter->killer_move1;
			}
		case STATE_KILLER2:
			sorter->state = STATE_GENE;
			if (sorter->killer_move2 != 0 && 
					sorter->killer_move2 != sorter->mv_tt &&
					sorter->killer_move2 != sorter->killer_move1 &&
					sorter->engine->board()->legalMove(sorter->killer_move2))
			{
				// printf("从杀手表2中检索到杀棋\n");
				return sorter->killer_move2;
			}
		case STATE_GENE:
			sorter->state = STATE_REST;
      if (sorter->mvs_size == 0) {
        n = sorter->engine->board()->generateAllMoves<GENERAL>(sorter->mvs);
        std::sort(sorter->mvs, sorter->mvs + n, CompareByHistory(sorter->engine));
        sorter->mvs_size = n;
      }
		case STATE_REST:
			while (sorter->mvs_idx < sorter->mvs_size)
			{
				int mv = sorter->mvs[sorter->mvs_idx++];
				if (mv != sorter->mv_tt &&
						mv != sorter->killer_move1 && 
						mv != sorter->killer_move2)
					return mv;
			}
		default:
			return 0;
	}
}

int SearchEngine::transpositionTableGrab(int vlAlpha, int vlBeta, int depth, int* mv)
{
	const Zobrist* zobrist = &board_->getZobrist();
	tt_item* item = &transpositionTable_[((TRANSPOSITION_TABLE_SIZE - 1) & zobrist->key_)];
	if (item->checksum_lower32 != zobrist->lock1_ || item->checksum_higher32 != zobrist->lock2_)
	{
		*mv = 0;
		return -MATE_VALUE;
	}

	*mv = item->mv;
	int mate = 0;
	if (item->value > WIN_VALUE)
	{
		if (item->value <= BAN_VALUE)
			return -MATE_VALUE;
		item->value -= distance_;
		mate = 1;
	}
	else if (item->value < -WIN_VALUE)
	{
		if (item->value >= -BAN_VALUE)
		{
			return -MATE_VALUE;
		}
		item->value += distance_;
		mate = 1;
	}
	else if (item->value == drawValue())
	{
		return -MATE_VALUE;
	}

	if (item->depth < depth && !mate)
	{
		return -MATE_VALUE;
	}
	if (item->flag == HASH_BETA)
	{
		return (item->value >= vlBeta ? item->value : -MATE_VALUE);
	}
	if (item->flag == HASH_ALPHA)
	{
		return (item->value <= vlAlpha ? item->value : -MATE_VALUE);
	}
	return item->value;
}

void SearchEngine::transpositionTableInsert(int flag, int value, int depth, int mv)
{
	const Zobrist* zobrist = &board_->getZobrist();
	struct tt_item* item = &transpositionTable_[((TRANSPOSITION_TABLE_SIZE - 1) & zobrist->key_)];
	if (item->depth > depth)
		return ;

	item->flag = flag;
	item->depth = depth;
	if (value > WIN_VALUE)
	{
		if (mv == 0 && value <= BAN_VALUE) return;
		item->value = value + distance_;
	}
	else if (value < -WIN_VALUE)
	{
		if (mv == 0 && value >= -BAN_VALUE) return;
		item->value = value - distance_;
	}
	else if (value == drawValue() && mv == 0)
	{
		return ;
	}
	else
	{
		item->value = value;
	}
	item->mv = mv;
	item->checksum_lower32 = zobrist->lock1_;
	item->checksum_higher32 = zobrist->lock2_;
}

int SearchEngine::searchQuiescence(int value_alpha, int value_beta)
{
	//printf("search quiescence\n");
	ndepth_ = ndepth_ < distance_ ? distance_ : ndepth_;
	++allNodes_;

	// 1. 杀棋步数裁剪
	int value = mateValue();
	if (value >= value_beta)
		return value;

	// 2. 重复裁剪
	int value_rep = board_->repetitionStatus(1);
	if (value_rep > 0)
	{
		return repetitionValue(value_rep);
	}

	if (distance_ == LIMIT_DEPTH)
	{
		printf("Quiesc limit depth\n");
		return board_->evaluate();
	}

	// 4. 初始化
	int value_best = -MATE_VALUE;

	int mvs[MAX_GENERATE_MOVES];
	int n = 0;
	if (board_->willKillSelfKing())
	{
		n = board_->generateAllMoves<GENERAL>(mvs);
		std::sort(mvs, mvs + n, CompareByHistory(this));
	}
	else
	{
		value = board_->evaluate();
		if (value > value_best)
		{
			if (value >= value_beta)
			{
				return value;
			}
			value_best = value;
			value_alpha = value > value_alpha ? value : value_alpha;
		}

		// 对于未被将军的局面，生成并排序所有吃子着法（MVV/LVA启发）
		n = board_->generateAllMoves<CAPTURE>(mvs);
		std::sort(mvs, mvs + n, CompareByMvvLua(this->board()));
	}

	for (int i = 0; i < n; ++i)
	{
		int mv = mvs[i];
		if (!makeMove(mv)) 
			continue;

		value = -searchQuiescence(-value_beta, -value_alpha);
		undoMove();
		if (value > value_best)
		{
			if (value >= value_beta)
			{
				return value;
			}
			value_best = value;
			value_alpha = value > value_alpha ? value : value_alpha;
		}
	}

	if (value_best == -MATE_VALUE)
	{
		return mateValue();
	}
	else
	{
		return value_best;
	}

}

int SearchEngine::searchFull(int value_alpha, int value_beta, int depth, int nonull)
{
	// 1. 到达水平线，由于水平线效应，应进行静态搜索
	if (depth <= 0) {
		return searchQuiescence(value_alpha, value_beta);
	}

	++allNodes_; // 更新搜索节点数

	// 当上边界beta为一个很大的负数时，说明到了即将被将死的局面
	int value = mateValue();
	if (value_beta <= value)
		return value;

	// 检测重复局面，不要在根节点上检查，否则无法得到走法
	// 出现重复局面直接返回，避免无限循环导致浪费搜索时间
	int value_rep = board_->repetitionStatus(1);
	if (value_rep > 0)
	{
		return repetitionValue(value_rep);
	}

	int mv_tt = 0;
	value = transpositionTableGrab(value_alpha, value_beta, depth, &mv_tt);
	if (value > -MATE_VALUE)
		return value;

	// 超过最大搜索层数直接返回
	if (distance_ == LIMIT_DEPTH)
	{
		printf("searchfull limit depth\n");
		return board_->evaluate();
	}

	// 空步裁剪
	if (!nonull && !board_->willKillSelfKing() && nullOkay())
	{
		doNullMove();
		value = -searchFull(-value_beta, 1 - value_beta, depth - NULL_DEPTH - 1, 1);
		undoNullMove();
		if (value >= value_beta && 
				(nullSafe() || searchFull(value_alpha, value_beta, depth - NULL_DEPTH, 1) >= value_beta))
		{
			return value;
		}
	}

	int tt_flag = HASH_ALPHA;
	int value_best = -MATE_VALUE;
	int mv_best = 0;
	int mv = 0;
	int new_depth = 0;

	struct moves_generate_sorter sorter;
	moves_generate_sorter_init(&sorter, this, mv_tt);

	while ((mv = moves_generate_sorter_next_move(&sorter)) > 0)
	{
		if (!makeMove(mv))
			continue;

		//将军延伸(即将军的走法应该让它多搜索一层)
		new_depth = board_->willKillSelfKing() ? depth : depth - 1;

		// PVS算法
		// 先对第一个走法做全窗口搜索
		if (value_best == -MATE_VALUE)
		{
			value = -searchFull(-value_beta, -value_alpha, new_depth, 0);
		}
		else
		{
			// 根据对第一个走法做全窗口搜索得到的下边界的值，对剩余的走法做零窗口搜索
			value = -searchFull(-value_alpha - 1, -value_alpha, new_depth, 0);

			// 检验零窗口搜索, 搜索失败则再对其进行全窗口搜索
			if (value > value_alpha && value < value_beta)
			{
				value = -searchFull(-value_beta, -value_alpha, new_depth, 0);
			}
		}
		undoMove();

		// 找到更好的走法，并保存走法以及走法所对应的分值
		if (value > value_best)
		{
			value_best = value;
			// 调整上边界, 并产生截断
			if (value >= value_beta)
			{
				mv_best = mv;
				tt_flag = HASH_BETA;
				break;
			}
			// 调整下边界
			if (value > value_alpha)
			{
				value_alpha = value;
				mv_best = mv;
				tt_flag = HASH_PV;
			}
		}
	}

	// 无棋可走（即被困毙或者被绝杀）
	if (value_best == -MATE_VALUE)
	{
		// 如果是杀棋，就根据杀棋步数给出评价
		return mateValue();
	}

	transpositionTableInsert(tt_flag, value_best, depth, mv_best);

	//把最佳走法保存到历史表，返回最佳分值
	if (mv_best > 0)
	{
		setBestMove(mv_best, depth);
	}

	return value_best;
}

int SearchEngine::searchRoot(int depth)
{
	int value = 0;
	int value_best = -MATE_VALUE;
	int mv = 0;
	int new_depth = 0;

	int mvs[MAX_GENERATE_MOVES];
	int n = board_->generateAllMoves<GENERAL>(mvs);
	std::sort(mvs, mvs + n, CompareByHistory(this));

	for(int i = 0; i < n; ++i)
	{
		int mv = mvs[i];
		if (!makeMove(mv))
			continue;

		//将军延伸(即将军的走法应该让它多搜索一层)
		new_depth = board_->willKillSelfKing() ? depth : depth - 1;

		// PVS算法
		// 先对第一个走法做全窗口搜索
		if (value_best == -MATE_VALUE)
		{
			value = -searchFull(-MATE_VALUE, MATE_VALUE, new_depth, 1);
		}
		else
		{
			// 根据对第一个走法做全窗口搜索得到的下边界的值，对剩余的走法做零窗口搜索
			value = -searchFull(-value_best-1, -value_best, new_depth, 0);

			// 检验零窗口搜索, 搜索失败则再对其进行全窗口搜索
			if (value > value_best)
			{
				value = -searchFull(-MATE_VALUE, -value_best, new_depth, 1);
			}
		}
		undoMove();
		if (value > value_best)
		{
			value_best = value;
			mvBest_ = mv;
		}
	}

	setBestMove(mvBest_, depth);

	return value_best;
}

int SearchEngine::search(int milliseconds)
{
	uint32_t checksum = board_->getZobrist().lock2_;
	uint32_t mirrorChecksum = board_->getMirrorZobrist().lock2_;
	int mv = openBook_.findBestMove(checksum, mirrorChecksum);
	if (mv != 0 && makeMove(mv))// && repetitionValue(board_->repetitionStatus(3)) == 0)
	{
		printf("find best mv in openbook:%d\n", mv);
		undoMove();
		return mv;
	}

	reset();
	uint64_t t = now();
	int value = 0;
	// iterative deepening 迭代加深
	for (int depth = 1; depth <= LIMIT_DEPTH; ++depth)
	{
		ndepth_ = 0;
		value = searchRoot(depth);

		uint64_t spendTime = now() - t;

		printf("搜索[%2d]层数(real: %3d)\t<best mv>: %6d\t", depth, ndepth_, mvBest_);
		uint64_t nps = (uint64_t)allNodes_ * 1000 * 1000 / spendTime;
		printf("<spend time>: %5lu\t<all nodes>: %10d\t<speed>: %7lu nodes per second\n", 
					 spendTime / 1000, allNodes_, nps);
		if (spendTime / 1000 >= milliseconds)
		{
				/*
				printf("搜索层数：%d(real: %d), best mv: %d\n", depth, ndepth_, mvBest_);
				uint64_t nps = (uint64_t)allNodes_ * 1000 / spendTime;
				printf("info spend time: %lu, all nodes: %d, speed: %lu nodes per second\n\n", 
							 spendTime, allNodes_, nps);
							 */
				break;
		}

		// 搜索到杀棋，就终止搜索
		if (value > WIN_VALUE || value < -WIN_VALUE)
		{
			if (value > WIN_VALUE)
				printf("已搜索到必胜之棋!mv=%d\n\n", mvBest_);
			else
				printf("已搜索到必输之棋!mv=%d\n\n", mvBest_);
			break;
		}
	}

	return mvBest_;
}

} // namespace cppupdate
} // namespace cchess
} // namespace wsun
