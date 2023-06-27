#include <benchmark/benchmark.h>
#include "../board.h"
#include "../utils.h"
#include "../search_engine.h"
#include <memory>
#include <iostream>

using ::wsun::cchess::cppupdate::Board;
using ::wsun::cchess::cppupdate::SearchEngine;

void bench_func(benchmark::State& state)
{
	std::unique_ptr<Board> b(new Board);
	std::unique_ptr<SearchEngine> engine(new SearchEngine(b.get()));
	int mvs[MAX_GENERATE_MOVES];
	for (auto _ : state)
	{
		b->generateAllMoves<::wsun::cchess::cppupdate::GENERAL>(mvs);
	}
}

BENCHMARK(bench_func);

BENCHMARK_MAIN();

