#include "search_engine.h"
#include "board.h"
#include <sys/time.h>
#include <stdio.h>

static const int MATE_VALUE = 10000; // 最高分值，即将死的分值
static const int BAN_VALUE = MATE_VALUE - 100;
static const int WIN_VALUE = MATE_VALUE - 200; // 搜索出胜负的分值界限，超出此值就说明已经搜索出杀棋了
static const int DRAW_VALUE = 20;
static const int ADVANCED_VALUE = 3; // 先行权分值

// 空步裁剪参数
static const int NULL_SAFE_MARGIN = 400;
static const int NULL_OKAY_MARGIN = 200;
static const int NULL_DEPTH = 2;

// 走法排序生成器
struct moves_generate_sorter
{
	struct search_engine * engine;
	int mvs[MAX_GENERATE_MOVES];
	int mvs_size;
	int mvs_idx;
	int mv_tt;
	int state;
	int killer_move1;
	int killer_move2;
};

static const int HASH_ALPHA = 1;
static const int HASH_BETA = 2;
static const int HASH_PV = 3;

#define STATE_TT 0
#define STATE_KILLER1	1
#define STATE_KILLER2 2
#define STATE_GENE 3
#define STATE_REST 4

static struct search_engine* g_engine = NULL;

static int compare_by_history(const void* left, const void* right)
{
	return g_engine->history_heuristic_table[*(const int*)right] - g_engine->history_heuristic_table[*(const int*)left];
}

static int compare_by_search(const void* left, const void* right)
{
	return (*(int*)left - *(int*)right);
}

static int compare_by_mvv_lva(const void* left, const void* right)
{
	return mvv_lva(g_engine->board, *(const int*)right) - mvv_lva(g_engine->board, *(const int*)left);
}

// return milliseconds
static uint64_t now()
{
	struct timeval tm;
	gettimeofday(&tm, NULL);
	return tm.tv_sec * 1000 + tm.tv_usec / 1000;
}

static inline int mate_value(struct search_engine* engine)
{
	return engine->distance - MATE_VALUE;
}

static inline int ban_value(struct search_engine* engine)
{
	return engine->distance - BAN_VALUE;
}

static inline int draw_value(struct search_engine* engine)
{
	return (engine->distance & 1) == 0  ? -DRAW_VALUE : DRAW_VALUE;
}

static inline int null_okay(struct search_engine* engine)
{
	return engine->board->current_side_player->value > NULL_OKAY_MARGIN;
}

static inline int null_safe(struct search_engine* engine)
{
	return engine->board->current_side_player->value > NULL_SAFE_MARGIN;
}

static inline void set_best_move(struct search_engine* engine, int mv, int depth)
{
  if (mv == 0) return;

	engine->history_heuristic_table[mv] += depth * depth;
	// int* killer_table = engine->killer_heuristic_table[depth];
 //  if (mv != killer_table[0])
 //  {
 //    killer_table[1] = killer_table[0];
 //    killer_table[0] = mv;
 //  }
}

static inline void search_engine_do_null_move(struct search_engine* engine)
{
	++engine->distance;
	make_null_move(engine->board);
	change_side(engine->board);
}

static inline void search_engine_undo_null_move(struct search_engine* engine)
{
	--engine->distance;
	undo_null_move(engine->board);
	change_side(engine->board);
}

static inline int search_engine_make_move(struct search_engine* engine, int mv)
{
	make_move(engine->board, mv);
	if(will_kill_self_king(engine->board))
	{
		undo_move(engine->board);
		return 0;
	}
	++engine->distance;
	change_side(engine->board);
	return 1;
}

static inline void search_engine_undo_move(struct search_engine* engine)
{
	undo_move(engine->board);
	--engine->distance;
	change_side(engine->board);
}

static inline int repetition_value(struct search_engine* engine, int rep)
{
	int vl = ((rep & 2) == 0 ? 0 : ban_value(engine)) + 
					 ((rep & 4) == 0 ? 0 : -ban_value(engine));
	return (vl == 0 ? draw_value(engine) : vl);
}

void moves_generate_sorter_init(struct moves_generate_sorter* sorter, struct search_engine* engine, int mv_tt)
{
	memset(sorter, 0, sizeof(struct moves_generate_sorter));
	sorter->mv_tt = mv_tt;
	sorter->state = mv_tt == 0 ? STATE_GENE : STATE_TT;
  
	/* memset(sorter, 0, sizeof(struct moves_generate_sorter));
	sorter->engine = engine;
	sorter->mv_tt = mv_tt;
	// 如果被将军的话就不能直接用置换表启发和杀手启发走法
	if (will_kill_self_king(engine->board))
	{
		sorter->state = STATE_REST;
		int n = generate_all_moves(engine->board, sorter->mvs);
		sorter->mvs_size = n;
    qsort(sorter->mvs, n, sizeof(int), compare_by_history);
		int tmp_killer_move1 = engine->killer_heuristic_table[engine->distance][0];
		int tmp_killer_move2 = engine->killer_heuristic_table[engine->distance][1];
		if (mv_tt != 0 &&
				bsearch(&mv_tt, sorter->mvs, n, sizeof(int), compare_by_search) != NULL)
		{
			sorter->mv_tt = mv_tt;
			sorter->state = STATE_TT;
      // printf("在被将军的情况下，从生成的走法中取到置换表中最好的走法\n");
		}
		if (tmp_killer_move1 != 0 && bsearch(&tmp_killer_move1, sorter->mvs, n, sizeof(int), compare_by_search) != NULL)
		{
      // printf("在被将军的情况下，从生成的走法中取到杀手表1中的走法\n");
			sorter->killer_move1 = tmp_killer_move1;
      if (sorter->state == STATE_REST) sorter->state = STATE_KILLER1;
		}
		if (tmp_killer_move2 != 0 && bsearch(&tmp_killer_move2, sorter->mvs, n, sizeof(int), compare_by_search) != NULL)
		{
      // printf("在被将军的情况下，从生成的走法中取到杀手表2中的走法\n");
			sorter->killer_move2 = tmp_killer_move2;
			if (sorter->state == STATE_REST) sorter->state = STATE_KILLER2;
		}
	}
	else
	{
		sorter->state = STATE_TT;
		sorter->killer_move1 = engine->killer_heuristic_table[engine->distance][0];
    sorter->killer_move2 = engine->killer_heuristic_table[engine->distance][1];
  } */
  // printf("%d, %d, %d\n", sorter->mv_tt, sorter->killer_move1, sorter->killer_move2);
}

int moves_generate_sorter_next_move(struct moves_generate_sorter* sorter)
{
	int n = 0;
	switch(sorter->state)
	{
		case STATE_TT:
      sorter->state = STATE_GENE;
			if (sorter->mv_tt != 0)
				return sorter->mv_tt;
		case STATE_GENE:
			sorter->state = STATE_REST;
			n = generate_all_moves(g_engine->board, sorter->mvs);
			qsort(sorter->mvs, n, sizeof(int), compare_by_history);
			sorter->mvs_size = n;
		case STATE_REST:
			while (sorter->mvs_idx < sorter->mvs_size)
			{
				int mv = sorter->mvs[sorter->mvs_idx++];
				// if (mv != sorter->mv_tt)
					return mv;
			}
		default:
			return 0;
	}
	/* int n = 0;
	switch(sorter->state)
	{
		case STATE_TT:
			//sorter->state = STATE_GENE;
			sorter->state = STATE_KILLER1;
			if (sorter->mv_tt != 0)
      {
				// printf("从置换表中检索到好棋\n");
				return sorter->mv_tt;
      }
		case STATE_KILLER1:
			sorter->state = STATE_KILLER2;
			if (sorter->killer_move1 != 0 && 
					sorter->killer_move1 != sorter->mv_tt &&
          legal_move(sorter->engine->board, sorter->killer_move1))
			{
				// printf("从杀手表1中检索到杀棋\n");
				return sorter->killer_move1;
			}
		case STATE_KILLER2:
			sorter->state = STATE_GENE;
			if (sorter->killer_move2 != 0 && 
					sorter->killer_move2 != sorter->mv_tt &&
					sorter->killer_move2 != sorter->killer_move1 &&
          legal_move(sorter->engine->board, sorter->killer_move2))
			{
				// printf("从杀手表2中检索到杀棋\n");
				return sorter->killer_move2;
			}
		case STATE_GENE:
			sorter->state = STATE_REST;
      if (sorter->mvs_size > 0) {
        n = generate_all_moves(sorter->engine->board, sorter->mvs);
        qsort(sorter->mvs, n, sizeof(int), compare_by_history);
        sorter->mvs_size = n;
      }
		case STATE_REST:
			while (sorter->mvs_idx < sorter->mvs_size)
			{
				int mv = sorter->mvs[sorter->mvs_idx++];
				if (mv != sorter->mv_tt &&
						mv != sorter->killer_move1 && 
						mv != sorter->killer_move2)
        {
          // printf("依次从生成的所有走法中取出一步走法\n");
					return mv;
        }
			}
		default:
			return 0;
	} */
}

struct search_engine* search_engine_create(struct board* board)
{
	struct search_engine* engine = (struct search_engine*)malloc(sizeof(struct search_engine));
	memset(engine, 0, sizeof(struct search_engine));
	engine->board = board;
	g_engine = engine;
	return engine;
}

void search_engine_release(struct search_engine* engine)
{
	free(engine);
}

void search_engine_reset(struct search_engine* engine)
{
	engine->distance = 0;
	engine->all_nodes = 0;
  engine->mv_best = 0;
	memset(engine->history_heuristic_table, 0, sizeof(int) * HISTORY_HEURISTIC_TABLE_SIZE);
	memset(engine->killer_heuristic_table, 0, sizeof(int) * 2 * LIMIT_DEPTH);
	memset(engine->transposition_table, 0, sizeof(struct tt_item) * TRANSPOSITION_TABLE_SIZE);
}

int transposition_table_grab(struct search_engine* engine, int vlAlpha, int vlBeta, int depth, int* mv)
{
	struct zobrist* zobrist = &engine->board->zobrist_position->zobrist;
	struct tt_item* item = &engine->transposition_table[((TRANSPOSITION_TABLE_SIZE - 1) & zobrist->key)];
	if (item->checksum_lower32 != zobrist->lock1 || item->checksum_higher32 != zobrist->lock2)
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
		item->value -= engine->distance;
		mate = 1;
	}
	else if (item->value < -WIN_VALUE)
	{
		if (item->value >= -BAN_VALUE)
		{
			return -MATE_VALUE;
		}
		item->value += engine->distance;
		mate = 1;
	}
	else if (item->value == draw_value(engine))
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

void transposition_table_insert(struct search_engine* engine, int flag, int value, int depth, int mv)
{
	struct zobrist* zobrist = &engine->board->zobrist_position->zobrist;
	struct tt_item* item = &engine->transposition_table[((TRANSPOSITION_TABLE_SIZE - 1) & zobrist->key)];
	if (item->depth > depth)
		return ;

	item->flag = flag;
	item->depth = depth;
	if (value > WIN_VALUE)
	{
		if (mv == 0 && value <= BAN_VALUE) return;
		item->value = value + engine->distance;
	}
	else if (value < -WIN_VALUE)
	{
		if (mv == 0 && value >= -BAN_VALUE) return;
		item->value = value - engine->distance;
	}
	else if (value == draw_value(engine) && mv == 0)
	{
		return ;
	}
	else
	{
		item->value = value;
	}
	item->mv = mv;
	item->checksum_lower32 = zobrist->lock1;
	item->checksum_higher32 = zobrist->lock2;
}

int search_quiescence(struct search_engine* engine, int value_alpha, int value_beta)
{
	//printf("search quiescence\n");
	++engine->all_nodes;

	// 1. 杀棋步数裁剪
	int value = mate_value(engine);
	if (value >= value_beta)
		return value;

	// 2. 重复裁剪
	int value_rep = repetition_status(engine->board, 1);
	if (value_rep > 0)
	{
		return repetition_value(engine, value_rep);
	}

	if (engine->distance == LIMIT_DEPTH)
	{
		printf("Quiesc limit depth\n");
		return evaluate(engine->board);
	}

	// 4. 初始化
	int value_best = -MATE_VALUE;

	int mvs[MAX_GENERATE_MOVES];
	int n = 0;
	if (will_kill_self_king(engine->board))
	{
		n = generate_all_moves(engine->board, mvs);
		qsort(mvs, n, sizeof(int), compare_by_history);
	}
	else
	{
		value = evaluate(engine->board);
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
		n = generate_all_capture_moves(engine->board, mvs);
		qsort(mvs, n, sizeof(int), compare_by_mvv_lva);
	}

	for (int i = 0; i < n; ++i)
	{
		int mv = mvs[i];
		if (!search_engine_make_move(engine, mv)) continue;

		value = -search_quiescence(engine, -value_beta, -value_alpha);
		search_engine_undo_move(engine);
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
		return mate_value(engine);
	}
	else
	{
		return value_best;
	}

}

int search_full(struct search_engine* engine, int value_alpha, int value_beta, int depth, int nonull)
{
	// 1. 到达水平线，由于水平线效应，应进行静态搜索
	if (depth <= 0) {
		return search_quiescence(engine, value_alpha, value_beta);
	}

	++engine->all_nodes; // 更新搜索节点数

	// 当上边界beta为一个很大的负数时，说明到了即将被将死的局面
	int value = mate_value(engine);
	if (value_beta <= value)
		return value;

	// 检测重复局面，不要在根节点上检查，否则无法得到走法
	// 出现重复局面直接返回，避免无限循环导致浪费搜索时间
	int value_rep = repetition_status(engine->board, 1);
	if (value_rep > 0)
	{
		return repetition_value(engine, value_rep);
	}

	int mv_tt = 0;
	value = transposition_table_grab(engine, value_alpha, value_beta, depth, &mv_tt);
	if (value > -MATE_VALUE)
		return value;

	// 超过最大搜索层数直接返回
	if (engine->distance == LIMIT_DEPTH)
	{
		printf("searchfull limit depth\n");
		return evaluate(engine->board);
	}

	// 空步裁剪
	if (!nonull && !will_kill_self_king(engine->board) && null_okay(engine))
	{
		search_engine_do_null_move(engine);
		value = -search_full(engine, -value_beta, 1 - value_beta, depth - NULL_DEPTH - 1, 1);
		search_engine_undo_null_move(engine);
		if (value >= value_beta && 
				(null_safe(engine) || search_full(engine, value_alpha, value_beta, depth - NULL_DEPTH, 1) >= value_beta))
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
	moves_generate_sorter_init(&sorter, g_engine, mv_tt);

	while ((mv = moves_generate_sorter_next_move(&sorter)) > 0)
	{
		if (!search_engine_make_move(engine, mv)) continue;

		//将军延伸(即将军的走法应该让它多搜索一层)
		new_depth = will_kill_self_king(engine->board) ? depth : depth - 1;

		// PVS算法
		// 先对第一个走法做全窗口搜索
		if (value_best == -MATE_VALUE)
		{
			value = -search_full(engine, -value_beta, -value_alpha, new_depth, 0);
		}
		else
		{
			// 根据对第一个走法做全窗口搜索得到的下边界的值，对剩余的走法做零窗口搜索
			value = -search_full(engine, -value_alpha - 1, -value_alpha, new_depth, 0);

			// 检验零窗口搜索, 搜索失败则再对其进行全窗口搜索
			if (value > value_alpha && value < value_beta)
			{
				value = -search_full(engine, -value_beta, -value_alpha, new_depth, 0);
			}
		}
		search_engine_undo_move(engine);

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
		return mate_value(engine);
	}

	transposition_table_insert(engine, tt_flag, value_best, depth, mv_best);

	//把最佳走法保存到历史表，返回最佳分值
	if (mv_best > 0)
	{
		set_best_move(engine, mv_best, depth);
	}

	return value_best;
}

int search_root(struct search_engine* engine, int depth)
{
	int value = 0;
	int value_best = -MATE_VALUE;
	int mv = 0;
	int new_depth = 0;

	int mvs[MAX_GENERATE_MOVES];
	int n = generate_all_moves(engine->board, mvs);
	qsort(mvs, n, sizeof(int), compare_by_history);

	for(int i = 0; i < n; ++i)
	{
		mv = mvs[i];
		if (!search_engine_make_move(engine, mv)) continue;

		//将军延伸(即将军的走法应该让它多搜索一层)
		new_depth = will_kill_self_king(engine->board) ? depth : depth - 1;

		// PVS算法
		// 先对第一个走法做全窗口搜索
		if (value_best == -MATE_VALUE)
		{
			value = -search_full(engine, -MATE_VALUE, MATE_VALUE, new_depth, 1);
		}
		else
		{
			// 根据对第一个走法做全窗口搜索得到的下边界的值，对剩余的走法做零窗口搜索
			value = -search_full(engine, -value_best-1, -value_best, new_depth, 0);

			// 检验零窗口搜索, 搜索失败则再对其进行全窗口搜索
			if (value > value_best)
			{
				value = -search_full(engine, -MATE_VALUE, -value_best, new_depth, 1);
			}
		}
		search_engine_undo_move(engine);
		if (value > value_best)
		{
			value_best = value;
			engine->mv_best = mv;
		}
	}

	set_best_move(engine, engine->mv_best, depth);

	return value_best;
}

int search(struct search_engine* engine, int milliseconds)
{
	search_engine_reset(engine);
	uint64_t t = now();
	int value = 0;
	// iterative deepening 迭代加深
	for (int depth = 1; depth <= LIMIT_DEPTH; ++depth)
	{
		value = search_root(engine, depth);

		uint64_t spendTime = now() - t;
		if (spendTime >= milliseconds)
		{
      char chinese_mv[50] = {0};
      char iccs_mv[5] = {0};
      move_to_iccs_move(iccs_mv, engine->mv_best);
      board_to_chinese_mv(engine->board, chinese_mv, engine->mv_best);
      printf("搜索的层数：%d, best mv: %d(%s %s)\n", depth, engine->mv_best, iccs_mv, chinese_mv);
      uint64_t nps = (uint64_t)engine->all_nodes * 1000 / spendTime;
      printf("info spend time: %lu, all nodes: %d, speed: %lu nodes per second\n", 
             spendTime, engine->all_nodes, nps);
      board_display(engine->board);
      break;
		}

		// 搜索到杀棋，就终止搜索
		if (value > WIN_VALUE || value < -WIN_VALUE)
		{
      const char* side = engine->board->current_side_player->side == SIDE_TYPE_RED ? "红方" : "黑方";
			if (value > WIN_VALUE)
      {
				printf("%s 已搜索到必胜之棋!\n", side);
      }
			else
      {
				printf("%s 已搜索到必输之棋!\n", side);
      }
			break;
		}
	}

	return engine->mv_best;
}
