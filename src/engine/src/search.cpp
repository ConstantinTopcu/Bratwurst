#include <engine/search/search.h>
#include <engine/move_gen/move_gen.h>
#include <engine/search/evaluation.h>
#include <engine/search/move_picker.h>

#include <chrono>

namespace Bratwurst::Search
{

// constants for search
constexpr int MaxQuiescenceDepth = 20;

using Clock = std::chrono::high_resolution_clock;

struct SearchInfo
{
	Clock::time_point start;
	bool stopped = false;
	int timeMS = 0;
	int nodes = 0;
};

inline bool updateTime(SearchInfo& info)
{
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - info.start);
	info.stopped = elapsed.count() >= info.timeMS - 1; // 1 MS time buffer to cancel search
	return info.stopped;
}

int quiescence(Position& pos, int alpha, int beta, int ply, int depth, SearchInfo& searchInfo)
{
	// every 2048 nodes check wether time is about to run out
	if ((searchInfo.nodes & 2047) == 0)
	{
		updateTime(searchInfo);
	}
	
	if (searchInfo.stopped)
	{
		return -Evaluation::Infinity;
	}

	searchInfo.nodes++;

	int standPat = Evaluation::evaluate(pos);

	// assumes player is not in zugzwang
	if (standPat >= beta || ply == depth)
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

	while (picker.hasNext())
	{
		Move move = picker.pick();

		pos.doMove(move);
		int eval = -quiescence(pos, -beta, -alpha, ply + 1, depth, searchInfo);
		pos.undoMove();

		alpha = std::max(alpha, eval);

		if (alpha >= beta)
		{
			break; // beta cutoff
		}
	}

	return alpha;
}

int negamax(Position& pos, int alpha, int beta, int ply, int maxDepth, SearchInfo& searchInfo)
{
	if ((searchInfo.nodes & 2047) == 0)
	{
		updateTime(searchInfo);
	}

	if (searchInfo.stopped)
	{
		// return -Infinity so the move won't be considered
		return -Evaluation::Infinity;
	}

	searchInfo.nodes++;

	if (ply == maxDepth)
	{
		return quiescence(pos, alpha, beta, ply + 1, ply + MaxQuiescenceDepth, searchInfo);
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
		int eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, searchInfo);
		pos.undoMove();

		alpha = std::max(alpha, eval);

		if (alpha >= beta)
		{
			break; // beta cutoff
		}
	}

	return alpha;
}


SearchResult search(Position& pos, int timeMs)
{
	SearchInfo searchInfo;
	searchInfo.start = Clock::now();
	searchInfo.nodes = 0;
	searchInfo.stopped = false;
	searchInfo.timeMS = timeMs;

	SearchResult searchResult;
	searchResult.bestMove = Move::Null();
	searchResult.evaluation = -Evaluation::Infinity;
	searchResult.depth = 0;
	searchResult.nodes = 0;

	MoveList moves = generateMoves<GenType::All>(pos);

	// The game has terminated already
	if (moves.empty()) return searchResult;

	// set best move to moves[0] in case it terminates to quickly
	searchResult.bestMove = moves[0];

	for (int currentDepth = 1; currentDepth < 20; currentDepth++)
	{
		MovePicker picker(pos, moves);
		int alpha = -Evaluation::Infinity;
		Move bestIterMove = Move::Null();

		while (picker.hasNext())
		{
			searchInfo.nodes++;
			Move move = picker.pick();

			pos.doMove(move);
			int eval = -negamax(pos, -Evaluation::Infinity, -alpha, 1, currentDepth, searchInfo);
			pos.undoMove();

			if (searchInfo.stopped) break;

			if (eval > alpha)
			{
				alpha = eval;
				bestIterMove = move;
			}
		}

		if (searchInfo.stopped) break;

		searchResult.bestMove = bestIterMove;
		searchResult.evaluation = alpha;
		searchResult.depth++;
	}

	searchResult.nodes = searchInfo.nodes;

	return searchResult;
}

}