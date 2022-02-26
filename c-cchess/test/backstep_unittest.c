#include "../board.h"
#include "../zobrist_position.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	struct board* b = board_create(0);
	int mvs[128];
	int n = generate_all_moves(b, mvs, 0);
	assert(n > 0);
	printf("zobrist key: %u\n", b->zobrist_position->zobrist.key);
	board_play(b, mvs[0]);
	printf("zobrist key: %u\n", b->zobrist_position->zobrist.key);
	make_null_move(b);
	change_side(b);
	printf("zobrist key: %u\n", b->zobrist_position->zobrist.key);
	undo_null_move(b);
	change_side(b);
	printf("zobrist key: %u\n", b->zobrist_position->zobrist.key);
	board_back_one_step(b);
	printf("zobrist key: %u\n", b->zobrist_position->zobrist.key);

	return 0;
}
