#ifndef __WSUN_CCHESS_SEARCH_ENGINE_H__
#define __WSUN_CCHESS_SEARCH_ENGINE_H__

#include <inttypes.h>

#define MAX_GENERATE_MOVES 128
#define LIMIT_DEPTH 64 // 最大的搜索深度
#define HISTORY_HEURISTIC_TABLE_SIZE (1 << 16)
#define TRANSPOSITION_TABLE_SIZE (1 << 20)

#ifdef __cplusplus
extern "C" {
#endif

struct tt_item
{
	uint8_t depth;	// 深度
	uint8_t flag;		// alpha、beta、pv 三种节点类型
	int value;			// 局面分数值
	int mv;					// 走法
	uint32_t checksum_lower32;  // 局面zobrist校验值checksum 64位
	uint32_t checksum_higher32; 
};

struct search_engine
{
	struct board* board;
	int distance;
	int all_nodes;
	int mv_best;
	int history_heuristic_table[HISTORY_HEURISTIC_TABLE_SIZE];
	int killer_heuristic_table[LIMIT_DEPTH][2];
	struct tt_item transposition_table[TRANSPOSITION_TABLE_SIZE];
};

struct search_engine* search_engine_create(struct board* board);
void search_engine_release(struct search_engine* engine);
void search_engine_reset(struct search_engine* engine);

int search_quiescence(struct search_engine* engine, int value_alpha, int value_beta);
int search_full(struct search_engine* engine, int value_alpha, int value_beta, int depth, int nonull);
int search_root(struct search_engine* engine, int depth);
int search(struct search_engine* engine, int milliseconds);

#ifdef __cplusplus
}
#endif

#endif
