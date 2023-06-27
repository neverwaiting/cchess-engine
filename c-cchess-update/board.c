#include "board.h"
#include "search_engine.h"
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

static const char* INIT_FEN_STRING = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";

static const char* CHINESE_MOVE_NUMBER[2][9] =
{
	{"一", "二", "三", "四", "五", "六", "七", "八", "九"},
	{"1", "2", "3", "4", "5", "6", "7", "8", "9"}
};

static const char* CHINESE_MOVE_PAWN_NUMBER[4][5] =
{
	{"前", "后", ""  , ""  , ""  },
	{"前", "中", "后", ""  , ""  },
	{"前", "二", "三", "后", ""  },
	{"前", "二", "三", "四", "后"}
};

static const char* PIECE_NAME_STRING[2][7] =
{
	{"帅", "仕", "相", "马", "车", "炮", "兵"},
	{"将", "士", "象", "马", "车", "炮", "卒"}
};

static int get_piece_type(char c)
{
	switch(c)
	{
		case 'K': return PIECE_TYPE_KING;
		case 'A': return PIECE_TYPE_ADVISOR;
		case 'B': return PIECE_TYPE_BISHOP;
		case 'N': return PIECE_TYPE_KNIGHT;
		case 'R': return PIECE_TYPE_ROOK;
		case 'C': return PIECE_TYPE_CANNON;
		case 'P': return PIECE_TYPE_PAWN;
		default: return -1;
	}
	return -1;
}

struct board* board_init()
{
	struct board* board = (struct board*)malloc(sizeof(struct board));
	memset(board, 0, sizeof(struct board));

	board->red_player = player_create(SIDE_TYPE_RED);
	board->black_player = player_create(SIDE_TYPE_BLACK);
	board->current_side_player = board->red_player;

	memset(board->pieces, 0, sizeof(board->pieces));

	// init history_step_records
  board->history_step_recorder = step_recorder_create(INIT_HISTORY_STEPS_RECORD_SIZE);

	board->zobrist_position = zobrist_position_create();
	return board;
}

struct board* board_create(int side)
{
	struct board* board = board_init();
	board_init_from_fen(board, INIT_FEN_STRING);
  // printf("%d\n", board->red_player->pieces_size);
  // for (int i = 0; i < board->red_player->pieces_size; ++i)
  //   printf("%p\n", board->red_player->pieces[i]);
  // printf("%d\n", board->black_player->pieces_size);
  // for (int i = 0; i < board->black_player->pieces_size; ++i)
  //   printf("%p\n", board->black_player->pieces[i]);
  board_display(board);
	return board;
}

void board_free_pieces(struct board* board)
{
	memset(board->pieces, 0, sizeof(board->pieces));
}

void board_release(struct board* board)
{
	player_release(board->red_player);
	player_release(board->black_player);
	board_free_pieces(board);
  step_recorder_release(board->history_step_recorder);
	zobrist_position_release(board->zobrist_position);
	free(board);
}

// 两边局势互换
struct board* board_exchange_side(const struct board* const src_board)
{
	struct board* board = board_init();
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		struct piece* p = src_board->pieces[i];
		if (p && p->show)
		{
			add_piece_to_board(board, p->type, 1 - p->side_player->side, 254 - p->pos);
		}
	}
	board->current_side_player = 
		(src_board->current_side_player->side == SIDE_TYPE_RED ?
		 board->black_player : board->red_player);

	return board;
}

// 得到一个镜像局面，左右互转
struct board* board_mirror(const struct board* const src_board)
{
	struct board* board = board_init();
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		struct piece* p = src_board->pieces[i];
		if (p && p->show)
		{
			add_piece_to_board(board, p->type, p->side_player->side, mirror_pos(p->pos));
		}
	}
	board->current_side_player =
		(src_board->current_side_player->side == SIDE_TYPE_RED ?
		 board->red_player : board->black_player);
	return board;
}

// 得到一个镜像局面zobrist值
void get_mirror_zobrist(const struct board* const bd, struct zobrist* zb)
{
	struct zobrist_position* mirror = zobrist_position_create();
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		struct piece* piece = bd->pieces[i];
		update_by_change_piece(mirror, piece->type, 
														piece->side_player->side, 
														mirror_pos(piece->pos));
	}
	int side = bd->current_side_player->side;
	if (side == SIDE_TYPE_BLACK)
	{
		update_by_change_side(mirror);
	}
	zb->key = mirror->zobrist.key;
	zb->lock1 = mirror->zobrist.lock1;
	zb->lock2 = mirror->zobrist.lock2;
	zobrist_position_release(mirror);
}

void add_piece_to_board(struct board* board, int type, int side, int pos)
{
	struct player* side_player = 
		(side == SIDE_TYPE_RED ? board->red_player : board->black_player);
	struct piece* piece = piece_create(type, side_player, pos);
	board->pieces[pos] = piece;
	player_add_piece(side_player, piece);
	update_by_change_piece(board->zobrist_position, side, type, pos);
}

void board_reset_data(struct board* board)
{
	player_reset(board->red_player);
	player_reset(board->black_player);
	board->current_side_player = board->red_player;
	board_free_pieces(board);
	board->history_step_recorder->size = 0;
	zobrist_position_reset(board->zobrist_position);
}

// 用fen串信息来初始化局面
int board_init_from_fen(struct board* board, const char* fen_string)
{
	int row = 0;
	int col = 0;

	int i = 0;
	while (!isspace(fen_string[i]))
	{
		char c = fen_string[i];
		if (isdigit(c))
		{
			col += (int)(c - '0');
		}
		else if (isalpha(c))
		{
			int side = SIDE_TYPE_RED;
			if (islower(c))
			{
				side = SIDE_TYPE_BLACK;
			}
			int type = get_piece_type(toupper(c));

			add_piece_to_board(board, type, side, convert_to_pos(row, col));
			++col;
		}
		else if (c == '/')
		{
			++row;
			col = 0;
		}
		++i;
	}

	if (fen_string[++i] == 'b')
		return SIDE_TYPE_BLACK;
	else
		return SIDE_TYPE_RED;
}

// 根据fen串信息重置局面
void board_reset_from_fen(struct board* board, const char* fen_string)
{
	board_reset_data(board);

	int side = board_init_from_fen(board, fen_string);
	board->current_side_player = 
		(side == SIDE_TYPE_RED ? board->red_player : board->black_player);

	if (side == SIDE_TYPE_BLACK)
	{
		update_by_change_side(board->zobrist_position);
	}
}

// 重置到最初局面
void board_reset(struct board* board)
{
	board_reset_data(board);
	board_init_from_fen(board, INIT_FEN_STRING);
}

// 根据当前局面输出fen格式的局面信息
void board_to_fen(const struct board* board, char* fen)
{
	for (int row = 0; row < 10; ++row)
	{
		int number = 0;
		for (int col = 0; col < 9; ++col)
		{
			int pos = convert_to_pos(row, col);
			if (board->pieces[pos] && board->pieces[pos]->show)
			{
				if (number > 0)
				{
					*fen = (char)('0' + number);
					++fen;
					number = 0;
				}
				int side = board->pieces[pos]->side_player->side;
				*fen = fen_piece_char[side][board->pieces[pos]->type];
				++fen;
			}
			else 
			{
				++number;
			}
		}
		if (number > 0)
		{
			*fen = (char)('0' + number);
			++fen;
		}

		*fen = '/';
		++fen;
	}

	--fen;
	*fen = ' ';
	++fen;
	*fen = (board->current_side_player->side == SIDE_TYPE_RED ? 'w' : 'b');
}

int will_kill_king(struct board* board, const struct player* player)
{
	struct player* opp_player = get_opponent_player_by_player(board, player);
	struct piece** pieces = opp_player->pieces;
	struct piece* king_piece = player->king_piece;

	for (int i = 0; i < opp_player->pieces_size; ++i)
	{
		if (pieces[i]->show && legal_move_piece(board, pieces[i], king_piece->pos))
		{
			return 1;
		}
	}
	return 0;
}

// 生成所有棋子所有的走法 capatured: 是否只生成吃子走法
int generate_all_moves_noncheck(struct board* board, int* mvs)
{
	int nums = 0;
	int n = 0;
	int temp[MAX_GENERATE_MOVES];
	struct piece** pieces = board->current_side_player->pieces;
  int size = board->current_side_player->pieces_size;
	for (int i = 0; i < size; ++i)
	{
		n += piece_generate_moves(pieces[i], board->pieces, &temp[n]);
	}
	for (int i = 0; i < n; ++i)
	{
		make_move(board, temp[i]);
		if (!will_kill_self_king(board))
		{
			mvs[nums++] = temp[i];
		}
		undo_move(board);
	}

	return nums;
}

int generate_all_capture_moves_noncheck(struct board* board, int* mvs)
{
	int nums = 0;
	int n = 0;
	int temp[MAX_GENERATE_MOVES];
	struct piece** pieces = board->current_side_player->pieces;
  int size = board->current_side_player->pieces_size;
	for (int i = 0; i < size; ++i)
	{
		n += piece_generate_capture_moves(pieces[i], board->pieces, &temp[n]);
	}
	for (int i = 0; i < n; ++i)
	{
		make_move(board, temp[i]);
		if (!will_kill_self_king(board))
		{
			mvs[nums++] = temp[i];
		}
		undo_move(board);
	}

	return nums;
}

int generate_all_moves(struct board* board, int* mvs)
{
	int nums = 0;
	struct piece** pieces = board->current_side_player->pieces;
  int size = board->current_side_player->pieces_size;
	for (int i = 0; i < size; ++i)
	{
		nums += piece_generate_moves(pieces[i], board->pieces, &mvs[nums]);
	}
	return nums;
}

int generate_all_capture_moves(struct board* board, int* mvs)
{
	int nums = 0;
	struct piece** pieces = board->current_side_player->pieces;
  int size = board->current_side_player->pieces_size;
	for (int i = 0; i < size; ++i)
	{
		nums += piece_generate_capture_moves(pieces[i], board->pieces, &mvs[nums]);
	}
	return nums;
}

void add_piece(struct board* board, struct piece* piece, int pos)
{
	if (!piece) return ;
	piece->show = 1;
	piece->pos = pos;
	board->pieces[pos] = piece;
	player_add_piece_value(piece->side_player, piece);

	int side = piece->side_player->side;
	update_by_change_piece(board->zobrist_position, side, piece->type, pos);
}

struct piece* del_piece(struct board* board, int pos)
{
	struct piece* piece = board->pieces[pos];
	if (!piece) return NULL;

	piece->show = 0;
	player_del_piece_value(piece->side_player, piece);
	board->pieces[pos] = NULL;

	int side = piece->side_player->side;
	update_by_change_piece(board->zobrist_position, side, piece->type, pos);

	return piece;
}

int legal_move_piece(struct board* board, struct piece* piece, int dest)
{
  return piece_legal_move(piece, board->pieces, dest);
}

int legal_move(struct board* board, int mv)
{
	int legal = 0;
	struct piece* piece = board->pieces[start_of_move(mv)];
	int dest = end_of_move(mv);
	if (piece && piece->show && 
			piece->side_player == board->current_side_player && 
			!compare_piece_same_side(piece, board->pieces[dest]) &&
			legal_move_piece(board, piece, dest))
	{
		make_move(board, mv);
		if (!will_kill_self_king(board))
		{
			legal = 1;
		}
		undo_move(board);
	}
	return legal;
}

// 真正走棋的动作
void make_move(struct board* board, int mv)
{
	int start = start_of_move(mv);
	int end = end_of_move(mv);

	uint32_t zkey = board->zobrist_position->zobrist.key;

	struct piece* end_piece = del_piece(board, end);
	struct piece* ret_piece = del_piece(board, start);
	add_piece(board, ret_piece, end);

	int in_check = will_kill_opponent_king(board);

	make_history_step(board, mv, end_piece, in_check, zkey);
}

// 撤销上一步走棋
void undo_move(struct board* board)
{
	if (step_recorder_empty(board->history_step_recorder))
		return;

	struct step* step = step_recorder_back_step(board->history_step_recorder);
	int mv = step->mv;
	int start = start_of_move(mv);
	int end = end_of_move(mv);

	struct piece* ret_piece = del_piece(board, end);
	add_piece(board, ret_piece, start);
	add_piece(board, step->end_piece, end);
}

// 将每一步走法记录到历史表
void make_history_step(struct board* board, int mv, struct piece* end_piece, int in_check, uint32_t zobrist_key)
{
  struct step step;
	step.mv = mv;
	step.end_piece = end_piece;
	step.in_check = in_check;
	step.zobrist_key = zobrist_key;
  step_recorder_add_step(board->history_step_recorder, step);
}

void make_null_move(struct board* board)
{
  struct step step;
  step.mv = 0;
  step.in_check = 0;
  step.end_piece = NULL;
  step.zobrist_key = board->zobrist_position->zobrist.key;
  step_recorder_add_step(board->history_step_recorder, step);
}

void undo_null_move(struct board* board)
{
  step_recorder_back_step(board->history_step_recorder);
}

// 检测重复局面
int repetition_status(struct board* board, int recur)
{
	int self_side = 0;
	int perp_check = 1;
	int opponent_perp_check = 1;

	if (step_recorder_empty(board->history_step_recorder)) 
		return 0;

	for(int i = board->history_step_recorder->size - 1; i >= 0; --i)
	{
		struct step* step = board->history_step_recorder->steps[i];
		if (step->mv <= 0 || step->end_piece) break;

		if (self_side)
		{
			perp_check = perp_check && step->in_check;
			if (step->zobrist_key == board->zobrist_position->zobrist.key)
			{
				if (--recur == 0)
				{
					return 1 + (perp_check ? 2 : 0) + (opponent_perp_check ? 4 : 0);
				}
			}
		}
		else 
		{
			opponent_perp_check = opponent_perp_check && step->in_check;
		}
		self_side = !self_side;
	}

	return 0;
}

// int prompt(struct board* board, int pos, int* mvs, int capatured)
// {
// 	struct piece* piece = board->pieces[pos];
// 	int n = 0;
// 	int nums = 0;
// 	int temp[MAX_GENERATE_MOVES];
// 	if (piece && piece->side_player == board->current_side_player)
// 	{
// 		nums = generate_moves(board, piece, temp, capatured);
// 	}
// 	for (int i = 0; i < nums; ++i)
// 	{
// 		make_move(board, temp[i]);
// 		if (!will_kill_self_king(board))
// 		{
// 			mvs[n++] = temp[i];
// 		}
// 		undo_move(board);
// 	}
//
// 	return n;
// }

// 困毙，是否无棋可走
int no_way_to_move(struct board* board)
{
	int mvs[MAX_GENERATE_MOVES];
	int n = generate_all_moves_noncheck(board, mvs);
	return n == 0;
}

char* assign_word(char* dest, const char* src)
{
	size_t len = strlen(src);
	memcpy(dest, src, len);
	return dest + len;
}

void board_to_chinese_mv(struct board* board, char* out_mv, int mv)
{
	char* res = out_mv;

	int start = start_of_move(mv);
	int end = end_of_move(mv);

	struct piece* piece = board->pieces[start];

	// 兵卒另外处理
	if (piece->type == PIECE_TYPE_PAWN)
	{
		// 显示与该棋子（兵）同列的兵有多少个
		int same_cols = 0;
		// 当前该棋子（兵）与同一列上的兵相比，所处位置是（最前面还是在中间还是在后面）
		int cur_pos = 0;

		int line[9] = {0};
		struct piece** pieces = piece->side_player->pieces;
		int pieces_size = piece->side_player->pieces_size;
		for (int i = 0; i < pieces_size; ++i)
		{
			struct piece* p = pieces[i];
			if (p != piece && p->type == piece->type && p->show)
			{
				++line[col_of_pos(p->pos)];
				if (same_col(piece->pos, p->pos))
				{
					++same_cols;
					if (p->pos < piece->pos) ++cur_pos;
				}
			}
		}
		int down_side = (0x80 & piece->side_player->king_piece->pos);

		// 同列上不止一个兵（卒）
		if (same_cols > 0)
		{
			int multi_same_col = 0;
			for (int i = 0; i < 9; ++i)
			{
				if (i != col_of_pos(piece->pos) && line[i] > 1)
				{
					multi_same_col = 1;
					break;
				}
			}
			cur_pos = (down_side ? cur_pos : same_cols - cur_pos);
			res = assign_word(res, CHINESE_MOVE_PAWN_NUMBER[same_cols - 1][cur_pos]);
			// 另外一路有两个兵或以上
			if (multi_same_col && !(same_cols == 2 && cur_pos == 1))
			{
				int col = (down_side ? 8 - col_of_pos(piece->pos) : col_of_pos(piece->pos));
				res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
			} 
			else
			{
				res = assign_word(res, piece->name);
			}
		}
		else
		{
			res = assign_word(res, piece->name); // 名字
			int col = (down_side ? 8 - col_of_pos(piece->pos) : col_of_pos(piece->pos));
			res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
		}
		// 走法的起点和终点同行,否则同列
		if (same_row(piece->pos, end))
		{
			res = assign_word(res, "平");
			int col = (down_side ? 8 - col_of_pos(end) : col_of_pos(end));
			res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
		}
		else
		{
			if (down_side)
					res = assign_word(res, (piece->pos > end ? "进" : "退"));
			else
					res = assign_word(res, (piece->pos > end ? "退" : "进"));
			int n = (abs(piece->pos - end) >> 4) - 1;
			res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][n]);
		}
		return ;
	}

	// 前两个字
	struct piece* same_type_piece = NULL;
	struct piece** pieces = piece->side_player->pieces;
	int pieces_size = piece->side_player->pieces_size;
	for (int i = 0; i < pieces_size; ++i)
	{
		struct piece* p = pieces[i];
		if (p != piece && p->type == piece->type && p->show)
		{
			same_type_piece = p;
			break;
		}
	}
	int down_side = (0x80 & piece->side_player->king_piece->pos);
	// 同类的子有两个并且在同一路上
	if (same_type_piece && same_col(piece->pos, same_type_piece->pos))
	{
		if (down_side)
			res = assign_word(res, (piece->pos > same_type_piece->pos ? "后" : "前"));
		else
			res = assign_word(res, (piece->pos > same_type_piece->pos ? "前" : "后"));
		res = assign_word(res, piece->name);
	}
	else
	{
		res = assign_word(res, piece->name); // 名字
		int col = (down_side ? 8 - col_of_pos(piece->pos) : col_of_pos(piece->pos));
		res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
	}

	// 后两个字

	// 士象马处理(只有进退)
	if (piece->type == PIECE_TYPE_ADVISOR ||
			piece->type == PIECE_TYPE_BISHOP ||
			piece->type == PIECE_TYPE_KNIGHT)
	{
		if (down_side)
			res = assign_word(res, (piece->pos > end ? "进" : "退"));
		else
			res = assign_word(res, (piece->pos > end ? "退" : "进"));

		int col = (down_side ? 8 - col_of_pos(end) : col_of_pos(end));
		res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
	}
	// 車炮将(既有进退，又有平)
	else if (piece->type == PIECE_TYPE_ROOK ||
					 piece->type == PIECE_TYPE_CANNON ||
					 piece->type == PIECE_TYPE_KING)
	{
		// 走法的起点和终点同行,否则同列
		if (same_row(piece->pos, end))
		{
			res = assign_word(res, "平");
			int col = (down_side ? 8 - col_of_pos(end) : col_of_pos(end));
			res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][col]);
		}
		else
		{
			if (down_side)
				res = assign_word(res, (piece->pos > end ? "进" : "退"));
			else
				res = assign_word(res, (piece->pos > end ? "退" : "进"));

			int n = (abs(piece->pos - end) >> 4) - 1;
			res = assign_word(res, CHINESE_MOVE_NUMBER[piece->side_player->side][n]);
		}
	}
}

void board_display(struct board* board)
{
  for (int i = 0; i < 256; ++i)
  {
    if (!in_board(i)) continue;
    int col = col_of_pos(i);
    int row = row_of_pos(i);
    char str[4] = {0};
    const char* space_square = col == 8 ? "+ " : "+-";
    const char* square_name = board->pieces[i] ? board->pieces[i]->name : space_square;
    col == 8 ? printf("%s%d", square_name, 9 - row) : printf("%s", square_name);
    if (col == 8)
    {
      row == 9 ?
        printf("\na    b    c    d    e    f    g    h    i\n") :
        printf("\n|    |    |    |    |    |    |    |    |\n");
    }
    else printf("---");
  }
}
