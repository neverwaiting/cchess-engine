#include "../board.h"
#include <assert.h>
#include <stdio.h>
#include <memory>

using wsun::cchess::cppupdate::Board;
int main(int argc, char **argv)
{
	std::unique_ptr<Board> b(new Board);
	int mvs[128];
	int n = b->generateAllMovesNoncheck<wsun::cchess::cppupdate::GENERAL>(mvs);
	assert(n > 0);
	printf("zobrist key: %u\n", b->getZobrist().key_);
	b->play(mvs[0]);
	printf("zobrist key: %u\n", b->getZobrist().key_);
	b->makeNullMove();

	b->changeSide();

	printf("zobrist key: %u\n", b->getZobrist().key_);
	b->undoNullMove();
	b->changeSide();
	printf("zobrist key: %u\n", b->getZobrist().key_);
	b->backOneStep();
	printf("zobrist key: %u\n", b->getZobrist().key_);

	return 0;
}
