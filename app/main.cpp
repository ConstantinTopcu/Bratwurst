#include <engine/position/position.h>
#include <engine/testing/perft.h>
#include <engine/search/search.h>

#include <iostream>
#include <chrono>

using namespace Bratwurst;

int main(char argc, char* argv[])
{
    Precomputed::init();
    Zobrist::init();

    Position pos = Position::fromFEN("r1bqkbnr/pp1pp2p/2p2p2/4P1p1/4n3/2N2Q1P/PPPP1PP1/R1B1KBNR w KQkq - 1 7").value();

    auto start = std::chrono::high_resolution_clock::now();
    auto [eval, bestMove] = Search::search(pos, 7);

    auto end = std::chrono::high_resolution_clock::now();

    // duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    pos.doMove(bestMove);

    std::cout << "Best Move: " << bestMove.toString()
        << " (Eval: " << eval << ")\n";

    std::cout << "Search time: " << duration << " ms\n";

    Precomputed::cleanup();
}