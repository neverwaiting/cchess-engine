#ifndef WSUN_CCHESS_BOARD_H
#define WSUN_CCHESS_BOARD_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "zobrist_position.h"
#include "player_piece.h"
#include "step_recorder.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

static const int INIT_HISTORY_STEPS_RECORD_SIZE = (2 << 10);
// static const char* INIT_FEN_STRING = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";

struct board
{
	struct player* red_player;             // 红方
	struct player* black_player;           // 黑方
	struct player* current_side_player;    // 当前下棋方
	struct piece* pieces[256];             // 棋盘上的所有棋子
  struct step_recorder* history_step_recorder;
	struct zobrist_position* zobrist_position;
  // int accum_steps_from_capture = 0;
  // int turn_num = 1;
};

struct board* board_create(int side);
void board_release(struct board* board);

struct board* board_exchange_side(const struct board* const src_board);
struct board* board_mirror(const struct board* const src_board);
void get_mirror_zobrist(const struct board* const bd, struct zobrist* zb);

void add_piece_to_board(struct board* board, int type, int side, int pos);

void board_reset_data(struct board* board);
// 用fen串信息来初始化局面
int board_init_from_fen(struct board* board, const char* fen_string);
// 根据fen串信息重置局面
void board_reset_from_fen(struct board* board, const char* fen_string);
// 重置到最初局面
void board_reset(struct board* board);

static inline struct player*
get_opponent_player_by_player(const struct board* board, const struct player* self)
{
	return (self == board->red_player ? board->black_player : board->red_player);
}

// 对手player
static inline struct player*
get_opponent_player(const struct board* board)
{
	return get_opponent_player_by_player(board, board->current_side_player);
}

// 更换下棋方
static inline void change_side(struct board* board)
{
	board->current_side_player = get_opponent_player(board);
	update_by_change_side(board->zobrist_position);
}

// 当前局面评价函数
static inline int evaluate(const struct board* board)
{
	return board->current_side_player->value - get_opponent_player(board)->value + 3;
}

int will_kill_king(struct board* board, const struct player* player);

// 被对手将军
static inline int will_kill_self_king(struct board* board)
{
	return will_kill_king(board, board->current_side_player);
}

// 将对手军
static inline int will_kill_opponent_king(struct board* board)
{
	return will_kill_king(board, get_opponent_player(board));
}

// 根据当前局面输出fen格式的局面信息
void board_to_fen(const struct board* board, char* fen);

// 生成所有棋子所有的走法 capature: 是否只生成吃子走法
int generate_all_moves(struct board* board, int* mvs);
int generate_all_capture_moves(struct board* board, int* mvs);

// 生成的走法不会被将军
int generate_all_moves_noncheck(struct board* board, int* mvs);
int generate_all_capture_moves_noncheck(struct board* board, int* mvs);

// 困毙，是否无棋可走
int no_way_to_move(struct board* board);

void add_piece(struct board* board, struct piece* piece, int pos);
struct piece* del_piece(struct board* board, int pos);

// 走法是否合理
int legal_move_piece(struct board* board, struct piece* piece, int dest);
int legal_move(struct board* board, int mv);

// 真正走棋的动作
void make_move(struct board* board, int mv);

// 撤销上一步走棋
void undo_move(struct board* board);

// 将每一步走法记录到历史表
void make_history_step(struct board* board, int mv, struct piece* end_piece, int in_check, uint32_t zobrist_key);

void make_null_move(struct board* board);

void undo_null_move(struct board* board);

// 用于AI
static inline int mvv_lva(struct board* board, int mv)
{
	int start = start_of_move(mv);
	int end = end_of_move(mv);
	
	// 必须是吃子着法
	//assert(board->pieces[start] && board->pieces[end]);

	int mvv = array_mvv_lva[board->pieces[end]->type] * 10;
	int lva = array_mvv_lva[board->pieces[start]->type];
	return mvv - lva;
}

// 检测重复局面
int repetition_status(struct board* board, int recur);

// 选子提示所有走法
int prompt(struct board* board, int pos, int* mvs, int capatured);

static int is_capatured(struct board* board, int mv)
{
	return board->pieces[end_of_move(mv)] != NULL;
}

static inline void board_play(struct board* board, int mv)
{
	make_move(board, mv);
	change_side(board);
}

static inline void board_play_iccs(struct board* board, const char* iccs_mv)
{
	int mv = iccs_move_to_move(iccs_mv);
	board_play(board, mv);
}

static inline void board_back_one_step(struct board* board)
{
	undo_move(board);
	change_side(board);
}

void board_to_chinese_mv(struct board* board, char* out_mv, int mv);

void board_display(struct board* board);

#ifdef __cplusplus
}
#endif

// bool isCapatured(int mv)
// {
//   return pieces_[end_of_move(mv)] != NULL;
// }
//
// void play(int mv)
// {
//   if (isCapatured(mv))
//   {
//     accumStepsFromCapture_ = 0;
//   }
//   else
//   {
//     ++accumStepsFromCapture_;
//   }
//   ++turnNums_;
//   makeMove(mv);
//   changeSide();
// }
//
// // 多少回合没有吃子
// int turnsNonCapture() const
// {
//   return (accumStepsFromCapture_ + 1) / 2;
// }
//
#endif
