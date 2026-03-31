#include <engine/search/search.h>
#include <engine/move_gen/move_gen.h>
#include <engine/search/evaluation.h>

namespace Bratwurst::Search
{

int negamax(Position& pos, int alpha, int beta, int ply, int maxDepth)
{
	Color c = pos.colorToMove();

	if (ply == maxDepth)
	{
		return evaluate(pos);
	}

	auto moves = generateMoves<GenType::All>(pos);

	if (moves.empty())
	{
		return (pos.checkers()) ? -CheckMate + ply : StaleMate;
	}

	for (Move m : moves)
	{
		pos.doMove(m);

		// recursivly call minimax
		int eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth);
		alpha = std::max(alpha, eval);

		pos.undoMove();

		if (alpha >= beta)
		{
			break; // beta cutoff
		}
	}

	return alpha;
}


SearchResult search(Position& pos, int depth)
{
	int bestEval = INT_MIN;
	Move bestMove = Move::Null();

	auto moves = generateMoves<GenType::All>(pos);

	for (Move move : moves)
	{
		pos.doMove(move);

		// recursivly call minimax
		int eval = -negamax(pos, -Infinity, +Infinity, 1, depth);

		if (eval > bestEval)
		{
			bestEval = eval;
			bestMove = move;
		}

		pos.undoMove();
	}

	return { bestEval, bestMove };
}

}