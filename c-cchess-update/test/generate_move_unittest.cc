#include "../board.h"
#include "../search_engine.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string>

// 自己跟自己对弈，每一步都是随机从生成的所有走法中选的，
// 然后将棋盘做镜像处理，验证镜像棋盘局面所生成的走法与原走法数相同

static int get_rand_move(struct board* board, int* mvs, const int size)
{
	if (size == 1)
	{
		return mvs[0];
	}

	int array[size];
	int total = 0;
	for (int i = 0; i < size; ++i)
	{
		array[i] = 0;
	}

	int mv = 0;
	while (total != size)
	{
		int idx = rand() % size;
		if (array[idx]) continue;

		mv = mvs[idx];
    board_play(board, mv);
		if (repetition_status(board, 1) == 0)
		{
      board_back_one_step(board);
			break;
		}
    board_back_one_step(board);
		array[idx] = 1;
		++total;
	}
	return total == size ? 0 : mv;
}

int main()
{
	srand(time(NULL));
	int mvs[128];
	int capatured_mvs[128];
	int mirror_mvs[128];
	int mirror_capatured_mvs[128];
	int exchange_mvs[128];
	int exchange_capatured_mvs[128];
	struct board* board = board_create(SIDE_TYPE_RED);
	struct board* mboard = nullptr;
	struct board* exchange_board = nullptr;

  char fen_str[1024];
  board_to_fen(board, fen_str);
	std::string fenString(fen_str);
	printf("fen: %s\n", fenString.c_str());
	assert(board->current_side_player->value == get_opponent_player(board)->value);

	int round = 2;
	int n = 0;
	//while ((n = generate_all_moves(board, mvs, 0)) > 0)
	while ((n = generate_all_moves_noncheck(board, mvs)) > 0)
	{
		//int capatured_n = generate_all_moves(board, capatured_mvs, 1);
		int capatured_n = generate_all_capture_moves_noncheck(board, capatured_mvs);

		mboard = board_mirror(board);
		exchange_board = board_exchange_side(board);

		int mirror_n = generate_all_moves_noncheck(mboard, mirror_mvs);
		int mirror_capatured_n= generate_all_capture_moves_noncheck(mboard, mirror_capatured_mvs);

		int exchange_n = generate_all_moves_noncheck(exchange_board, exchange_mvs);
		int exchange_capatured_n= generate_all_capture_moves_noncheck(exchange_board, exchange_capatured_mvs);

		if (n != mirror_n)
		{
			printf("generate num: %d %d\n", n, mirror_n); 
			char iccsmv[5] = {0};
			for (int i = 0; i < n; ++i)
			{
				int start = start_of_move(mvs[i]);
				move_to_iccs_move(iccsmv, mvs[i]);
				printf("raw type: %d, %s\n", board->pieces[start]->type, iccsmv);
			}
			for (int i = 0; i < mirror_n; ++i)
			{
				int start = start_of_move(mvs[i]);
				move_to_iccs_move(iccsmv, mvs[i]);
				printf("mirror type: %d, %s\n", mboard->pieces[start]->type, iccsmv);
			}
		}

		assert(n == mirror_n);
		assert(capatured_n == mirror_capatured_n);
		assert(n == exchange_n);
		assert(capatured_n == exchange_capatured_n);

		assert(board->current_side_player->value == mboard->current_side_player->value);
		assert(get_opponent_player(board)->value == get_opponent_player(mboard)->value);
		assert(evaluate(board) == evaluate(mboard));

		assert(board->current_side_player->value == exchange_board->current_side_player->value);
		assert(get_opponent_player(board)->value == get_opponent_player(exchange_board)->value);
		assert(evaluate(board) == evaluate(exchange_board));

    board_release(mboard);
    board_release(exchange_board);

		// 随机选一种走法
		int mv = get_rand_move(board, mvs, n);
		if (mv == 0) 
		{
			printf("repeat situation, shutdown\n");
			break;
		}

    char chinesemv[50] = {0};
    // board_to_chinese_mv(board, chinesemv, mv); 

		assert(!board->pieces[end_of_move(mv)] || board->pieces[end_of_move(mv)]->type != PIECE_TYPE_KING);

    board_play(board, mv);

		char iccsmv[5] = {0};
		move_to_iccs_move(iccsmv, mv);
		if (round % 2 == 0)
		{
			printf("第%d回合: %s(%s)\n", round / 2, iccsmv, chinesemv);
		}
		else
		{
			printf(" %s(%s)\n", iccsmv, chinesemv);
		}
		++round;
    board_display(board);
	}

	printf("\n游戏结束，总共%d回合!\n", (round - 1) / 2);

  board_release(board);
	
	return 0;
}
