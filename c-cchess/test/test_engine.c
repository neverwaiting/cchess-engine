#include "../board.h"
#include "../search_engine.h"

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("%s search-milliseconds\n", argv[0]);
		return -1;
	}
	int search_time = atoi(argv[1]);
	struct board* b = board_create(0);
	struct search_engine* engine = search_engine_create(b);
	int mv = 0;
	char chinese_mv[13] = {0};
	while ((mv = search(engine, search_time)) > 0)
	{
		memset(chinese_mv, 0, sizeof(chinese_mv));
		board_to_chinese_mv(b, chinese_mv, mv);
		printf("%d : %s\n", mv, chinese_mv);
		board_play(b, mv);
	}
	printf("下棋结束\n");

	board_release(b);
}
