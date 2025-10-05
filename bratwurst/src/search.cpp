#include "search/search.h"

#include "position.h"
#include "move_gen.h"
#include "search/evaluate.h"

#include <chrono>

namespace Bratwurst::Search
{

constexpr int Infinity = 100'000'000;
constexpr int MateBaseScore = 100'000;

// debug variables
size_t Nodes = 0;

int negamax(Position& pos, int alpha, int beta, int depth, int ply)
{
	// evaluate
	if (depth == 0)
	{
		Nodes++;
		return Evaluation::evaluate(pos);
	}

	MoveList moves = generateMoves<GenType::All>(pos);

	if (moves.size() == 0)
	{
		// either checkmate or stalemate
		return (pos.checkers()) ? -MateBaseScore + ply : 0;
	}

	int bestScore = -Infinity;

	for (Move m : moves)
	{
		pos.doMove(m);
		int score = -negamax(pos, -beta, -alpha, depth - 1, ply + 1);
		pos.undoMove();

		if (score > bestScore)
		{
			bestScore = score;
			alpha = std::max(score, alpha);
		}

		if (alpha >= beta)
		{
			break;
		}
	}

	return bestScore;
}

SearchResult search(Position& pos, uint8 depth)
{
	using Clock = std::chrono::high_resolution_clock;
	auto start = Clock::now();

	Nodes = 0;

	MoveList moves = generateMoves<GenType::All>(pos);

	int bestScore = -Infinity;
	Move bestMove = Move::Null();

	for (int currDepth = 1; currDepth < depth; currDepth++)
	{
		bestScore = -Infinity;
		bestMove = Move::Null();

		for (Move m : moves)
		{
			pos.doMove(m);
			int score = -negamax(pos, -Infinity, -bestScore, currDepth, 0);
			pos.undoMove();

			if (score > bestScore)
			{
				bestScore = score;
				bestMove = m;
			}
		}
	}

	auto end = Clock::now();

	SearchResult result = {};
	result.bestMove = bestMove;
	result.eval = bestScore;
	result.depth = depth;
	result.timeNS = (end - start).count();
	result.nodes = Nodes;
	result.nps = static_cast<uint32>(Nodes / (result.timeNS / 1'000'000'000.0));

	return result;
}

}