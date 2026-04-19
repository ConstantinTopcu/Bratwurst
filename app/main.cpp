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

    Position pos = Position::fromFEN("3qkb1r/1pp1n1p1/1rn2p2/1N2p1Pp/p2pP2P/PbPP1N1B/1P1BQP2/2R1R1K1 w k - 3 18").value();

    auto start = std::chrono::high_resolution_clock::now();
    int totalNodes = 0;

    for (int i = 0; i < 8; i++)
    {
        auto [eval, bestMove, nodes, depth] = Search::search(pos, 1000);
        pos.doMove(bestMove);
        totalNodes += nodes;

        std::cout << "move: " << bestMove.toString() << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);

    std::cout << "total nodes: " << totalNodes << "time: " << duration.count() << "s; average NPS: " << (totalNodes / duration.count()) / 1000000.0f << " M" << std::endl;

    Precomputed::cleanup();
}