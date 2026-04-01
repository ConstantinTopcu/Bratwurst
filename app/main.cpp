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

    Position pos = Position::fromFEN("r2q1rk1/ppp2pp1/2np3p/2b1p3/2B1P1b1/2NP1N2/PPP2P2/R1BQ1RK1 w - - 0 12").value();

    auto start = std::chrono::high_resolution_clock::now();
    auto [eval, bestMove, nodes] = Search::search(pos, 7);
    auto end = std::chrono::high_resolution_clock::now();

    // duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    pos.doMove(bestMove);

    std::cout << "Best Move: " << bestMove.toString() << " (Eval: " << eval << "; Nodes: " << nodes << ")\n";
    std::cout << "Search time: " << duration << " ms\n";

    Precomputed::cleanup();
}