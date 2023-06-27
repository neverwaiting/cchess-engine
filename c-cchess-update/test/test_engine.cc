#include "../board.h"
#include "../search_engine.h"
#include <memory>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("%s search-milliseconds\n", argv[0]);
		exit(1);
	}
	int searchTime = atoi(argv[1]);
  struct board* b = board_create(SIDE_TYPE_RED);
  
	struct search_engine* engine = search_engine_create(b);
	while (!no_way_to_move(b))
	{
		int mv = search(engine, searchTime);
		if (mv > 0)
      board_play(b, mv);
		else
			break;
	}
}
