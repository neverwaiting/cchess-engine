#include "../board.h"
#include "../search_engine.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>

using namespace ::wsun::cchess;

// 自己跟自己对弈，每一步都是随机从生成的所有走法中选的，
// 然后将棋盘做镜像处理，验证镜像棋盘局面所生成的走法与原走法数相同

static int get_rand_move(Board* board, int* mvs, const int size)
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
		board->play(mv);
		if (board->repetitionStatus(1) == 0)
		{
			board->backOneStep();
			break;
		}
		board->backOneStep();
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
	Board* board = new Board;
	Board* mboard = nullptr;
	Board* exchange_board = nullptr;

	std::string fenString = board->toFen();
	printf("fen: %s\n", fenString.c_str());
	assert(board->currentSidePlayer()->value() == board->getOpponentPlayer()->value());

	int round = 2;
	int n =0;
	//while ((n = generate_all_moves(board, mvs, 0)) > 0)
	while ((n = board->generateAllMovesNoncheck(mvs, false)) > 0)
	{
		//int capatured_n = generate_all_moves(board, capatured_mvs, 1);
		int capatured_n = board->generateAllMovesNoncheck(capatured_mvs, true);

		mboard = board->getMirrorBoard();
		exchange_board = board->getExchangeSideBoard();

		int mirror_n = mboard->generateAllMovesNoncheck(mirror_mvs, false);
		int mirror_capatured_n= mboard->generateAllMovesNoncheck(mirror_capatured_mvs, true);

		int exchange_n = exchange_board->generateAllMovesNoncheck(exchange_mvs, false);
		int exchange_capatured_n = exchange_board->generateAllMovesNoncheck(exchange_capatured_mvs, true);

		if (n != mirror_n)
		{
			printf("generate num: %d %d\n", n, mirror_n); 
			char iccsmv[5] = {0};
			for (int i = 0; i < n; ++i)
			{
				int start = start_of_move(mvs[i]);
				move_to_iccs_move(iccsmv, mvs[i]);
				printf("raw type: %d, %s\n", board->pieces()[start]->type(), iccsmv);
			}
			for (int i = 0; i < mirror_n; ++i)
			{
				int start = start_of_move(mvs[i]);
				move_to_iccs_move(iccsmv, mvs[i]);
				printf("mirror type: %d, %s\n", mboard->pieces()[start]->type(), iccsmv);
			}
		}

		assert(n == mirror_n);
		assert(capatured_n == mirror_capatured_n);
		assert(n == exchange_n);
		assert(capatured_n == exchange_capatured_n);

		assert(board->currentSidePlayer()->value() == mboard->currentSidePlayer()->value());
		assert(board->getOpponentPlayer()->value() == mboard->getOpponentPlayer()->value());
		assert(board->evaluate() == mboard->evaluate());

		assert(board->currentSidePlayer()->value() == exchange_board->currentSidePlayer()->value());
		assert(board->getOpponentPlayer()->value() == exchange_board->getOpponentPlayer()->value());
		assert(board->evaluate() == exchange_board->evaluate());

		delete mboard;
		delete exchange_board;

		// 随机选一种走法
		int mv = get_rand_move(board, mvs, n);
		if (mv == 0) 
		{
			printf("repeat situation, shutdown\n");
			break;
		}

		assert(!board->pieces()[end_of_move(mv)] || board->pieces()[end_of_move(mv)]->type() != PIECE_TYPE_KING);

		board->play(mv);

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

	delete board;
	
	return 0;
}
