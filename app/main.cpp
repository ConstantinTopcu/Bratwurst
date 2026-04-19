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

    Position pos = Position::fromFEN("5bk1/2NQ2pr/5pn1/1pp2BPp/p1PpP2P/Pb1P1N2/1P1B1PK1/2R3R1 w - - 2 30").value();

    auto start = std::chrono::high_resolution_clock::now();
    int totalNodes = 0;

    for (int i = 0; i < 8; i++)
    {
        auto [eval, bestMove, nodes, depth] = Search::search(pos, 10000);
        pos.doMove(bestMove);
        totalNodes += nodes;

        std::cout << "move: " << bestMove.toString() << "; eval: " << eval << ", depth: " << depth << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);

    std::cout << "total nodes: " << totalNodes << "time: " << duration.count() << "s; average NPS: " << (totalNodes / duration.count()) / 1000000.0f << " M" << std::endl;

    Precomputed::cleanup();
}