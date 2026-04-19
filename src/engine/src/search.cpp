#include <engine/search/search.h>
#include <engine/move_gen/move_gen.h>
#include <engine/search/evaluation.h>
#include <engine/search/move_picker.h>

namespace Bratwurst::Search
{

// constants for search
constexpr int MaxQuiescenceDepth = 20;

int quiescence(Position& pos, int alpha, int beta, int ply, int maxDepth, int& nodes)
{
	nodes++;

	Color c = pos.colorToMove();

	int standPat = Evaluation::evaluate(pos);

	// assumes player is not in zugzwang
	if (standPat >= beta || ply == maxDepth)
	{
		return standPat;
	}

	alpha = std::max(alpha, standPat);

	MoveList moves = generateMoves<GenType::Captures>(pos);

	if (moves.empty())
	{
		return standPat;
	}

	MovePicker picker(pos, moves);

	while(picker.hasNext())
	{
		Move move = picker.pick();

		pos.doMove(move);
		int eval = -quiescence(pos, -beta, -alpha, ply + 1, maxDepth, nodes);
		pos.undoMove();

		alpha = std::max(alpha, eval);

		if (alpha >= beta)
		{
			break; // beta cutoff
		}
	}

	return alpha;
}

int negamax(Position& pos, int alpha, int beta, int ply, int maxDepth, int& nodes)
{
	nodes++;

	Color c = pos.colorToMove();

	if (ply == maxDepth)
	{
		return quiescence(pos, alpha, beta, ply, maxDepth + MaxQuiescenceDepth, nodes);
	}

	auto moves = generateMoves<GenType::All>(pos);
	
	if (moves.empty())
	{
		return (pos.checkers()) ? -Evaluation::CheckMate + ply : Evaluation::StaleMate;
	}

	MovePicker picker(pos, moves);

	while (picker.hasNext())
	{
		Move move = picker.pick();

		pos.doMove(move);
		int eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, nodes);
		pos.undoMove();

		alpha = std::max(alpha, eval);

		if (alpha >= beta)
		{
			break; // beta cutoff
		}
	}

	return alpha;
}


SearchResult search(Position& pos, int depth)
{
	int alpha = -Evaluation::Infinity;
	Move bestMove = Move::Null();
	int nodes = 0;

	auto moves = generateMoves<GenType::All>(pos);
	MovePicker picker(pos, moves);

	while (picker.hasNext())
	{
		Move move = picker.pick();
		
		pos.doMove(move);
		int eval = -negamax(pos, -Evaluation::Infinity, -alpha, 1, depth, nodes);
		pos.undoMove();

		if (eval > alpha)
		{
			alpha = eval;
			bestMove = move;
		}
	}

	return { alpha, bestMove, nodes};
}

}