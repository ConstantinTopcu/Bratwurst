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

    Position pos = Position::fromFEN().value();

    for (int i = 0; i < 50; i++)
    {
        auto [eval, bestMove, nodes] = Search::search(pos, 6);
        pos.doMove(bestMove);
        std::cout << pos << std::endl;
    }

    Precomputed::cleanup();
}