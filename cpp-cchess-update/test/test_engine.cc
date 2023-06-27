#include "../board.h"
#include "../search_engine.h"
#include <memory>

using namespace ::wsun::cchess::cppupdate;

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("%s search-milliseconds\n", argv[0]);
		exit(1);
	}
	int searchTime = atoi(argv[1]);
	std::unique_ptr<Board> b(new Board);
	std::unique_ptr<SearchEngine> engine(new SearchEngine(b.get()));
	while (!b->noWayToMove())
	{
		int mv = engine->search(searchTime);
		if (mv > 0)
			b->play(mv);
		else
			break;
	}
}
