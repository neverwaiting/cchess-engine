#include "../board.h"
#include "../player_piece.h"
#include "../zobrist_position.h"
#include "../utils.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>

// 自己跟自己对弈，每一步都是随机从生成的所有走法中选的，
// 然后将棋盘做镜像处理，验证镜像棋盘局面所生成的走法与原走法数相同

static int equal_zobrist(struct zobrist* z1, struct zobrist* z2)
{
	return z1->key == z2->key && z1->lock1 == z2->lock1 && z1->lock2 == z2->lock2;
}

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
	struct board* board = board_create(0);
	struct board* mboard = NULL;
	struct board* exchange_board = NULL;

	char fenString[1000] = {0};
	board_to_fen(board, fenString);
	printf("fen: %s\n", fenString);
	printf("%d, %d\n", board->red_player->value, board->black_player->value);

	int round = 2;
	int n =0;
	//while ((n = generate_all_moves(board, mvs, 0)) > 0)
	while ((n = generate_all_moves_noncheck(board, mvs, 0)) > 0)
	{
		//int capatured_n = generate_all_moves(board, capatured_mvs, 1);
		int capatured_n = generate_all_moves_noncheck(board, capatured_mvs, 1);
		
		mboard = board_mirror(board);
		exchange_board = board_exchange_side(board);

		int mirror_n = generate_all_moves_noncheck(mboard, mirror_mvs, 0);
		int mirror_capatured_n = generate_all_moves_noncheck(mboard, mirror_capatured_mvs, 1);

		int exchange_n = generate_all_moves_noncheck(exchange_board, exchange_mvs, 0);
		int exchange_capatured_n = generate_all_moves_noncheck(exchange_board, exchange_capatured_mvs, 1);

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

		assert(board->red_player->value == mboard->red_player->value);
		assert(board->black_player->value == mboard->black_player->value);
		assert(evaluate(board) == evaluate(mboard));

		//printf("%d, %d\n", board->red_player->value, exchange_board->black_player->value);
		//printf("%d, %d\n", board->black_player->value, exchange_board->red_player->value);
		//printf("%d, %d\n", evaluate(board), evaluate(exchange_board));
		assert(board->red_player->value == exchange_board->black_player->value);
		assert(board->black_player->value == exchange_board->red_player->value);
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

		assert(!board->pieces[end_of_move(mv)] || board->pieces[end_of_move(mv)]->type != PIECE_TYPE_KING);

		board_play(board, mv);

		char iccsmv[5] = {0};
		move_to_iccs_move(iccsmv, mv);
		if (round % 2 == 0)
		{
			printf("第%d回合: %s", round / 2, iccsmv);
		}
		else
		{
			printf(" %s\n", iccsmv);
		}
		++round;
	}

	printf("\n游戏结束，总共%d回合!\n", (round - 1) / 2);

	board_release(board);
	
	return 0;
}
