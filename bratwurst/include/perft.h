#pragma once

#include "position.h"
#include "move_gen.h"

namespace Bratwurst::Perft
{

size_t perft(Position& pos, int depth, bool print = false)
{
	if (depth == 0) return 1;

	MoveList moves = generateMoves<GenType::All>(pos);
		
	// Optimizations: no need to apply the moves if depth == 1
	if (depth == 1 && !print) return moves.size();

	size_t totalNodes = 0;

	for (Move m : moves)
	{
		pos.doMove(m);
		size_t newNodes = perft(pos, depth - 1);
		pos.undoMove();

		if (print) std::cout << m.toString() << ": " << newNodes << "\n";

		totalNodes += newNodes;
	}

	return totalNodes;
}

}