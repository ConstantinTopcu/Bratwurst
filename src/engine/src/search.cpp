#include <engine/move_gen/move_gen.h>

#include <engine/search/search.h>
#include <engine/search/evaluation.h>
#include <engine/search/move_picker.h>
#include <engine/search/transposition_table.h>

#include <chrono>

namespace Bratwurst::Search
{

// constants for search

using Clock = std::chrono::high_resolution_clock;

struct SearchInfo
{
	Clock::time_point start;
	bool stopped = false;
	int timeMS = 0;
	int nodes = 0;
};

constexpr int MaxQuiescenceDepth = 20;
constexpr int MaxSearchDepth = 100;

// Transposition table of size 256 MB
TranspositionTable TT(256);

inline bool updateTime(SearchInfo& info)
{
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - info.start);
	info.stopped = elapsed.count() >= info.timeMS - 5; // 1 MS time buffer to cancel search
	return info.stopped;
}

int quiescence(Position& pos, int alpha, int beta, int ply, int depth, SearchInfo& searchInfo)
{
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

		if (eval > alpha)
		{
			alpha = eval;
		}

		if (alpha >= beta)
		{
			break;
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

	// check for 3 fold repetition


	// Check for position in transposition table
	const TranspositionTable::TTEntry* entry = TT.probe(pos.zobristKey());

	if (entry != nullptr && entry->depth >= maxDepth - ply)
	{
		using enum TranspositionTable::MoveBound;

		if (entry->bound == Lower) alpha = std::max(alpha, entry->score);
		if (entry->bound == Upper) beta = std::min(beta, entry->score);
		if (entry->bound == Exact) return entry->score;
		if (alpha >= beta) return entry->score;
	}

	if (ply == maxDepth)
	{
		return quiescence(pos, alpha, beta, ply + 1, ply + MaxQuiescenceDepth, searchInfo);
	}

	auto moves = generateMoves<GenType::All>(pos);
	
	if (moves.empty())
	{
		return (pos.checkers()) ? -Evaluation::CheckMate + ply : Evaluation::StaleMate;
	}

	TranspositionTable::TTEntry newEntry =
	{
		.key		= pos.zobristKey(),
		.bestMove	= Move::Null(),
		.score		= alpha,
		.bound		= TranspositionTable::MoveBound::Upper,
		.depth		= maxDepth - ply
	};

	MovePicker picker(pos, moves, (entry) ? entry->bestMove : Move::Null());

	while (picker.hasNext())
	{
		Move move = picker.pick();

		pos.doMove(move);
		int eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, searchInfo);
		pos.undoMove();

		if (eval > alpha)
		{
			alpha = eval;

			newEntry.bound = TranspositionTable::MoveBound::Exact;
			newEntry.bestMove = move;
			newEntry.score = eval;
		}

		if (alpha >= beta)
		{
			newEntry.bound = TranspositionTable::MoveBound::Lower;
			newEntry.score = alpha;
			newEntry.bestMove = move;

			break; // beta cutoff
		}
	}

	if (!searchInfo.stopped)
	{
		TT.store(std::move(newEntry));
	}

	return alpha;
}


SearchResult search(Position& pos, int timeMs)
{
	SearchInfo searchInfo =  
	{
		.start		= Clock::now(),
		.stopped	= false, 
		.timeMS		= timeMs, 
		.nodes		= 0 
	};

	SearchResult searchResult = 
	{ 
		.evaluation	= -Evaluation::Infinity,
		.bestMove	= Move::Null(),
		.nodes		= 0, 
		.depth		= 0
	};

	MoveList moves = generateMoves<GenType::All>(pos);

	// make sure game hasnt terminated yet
	if (moves.empty()) return searchResult;

	for (int currentMaxDepth = 1; currentMaxDepth <= 100; currentMaxDepth++)
	{
		int alpha = -Evaluation::Infinity;
		Move bestIterMove = moves[0];

		const TranspositionTable::TTEntry* entry = TT.probe(pos.zobristKey());
		Move TTMove = (entry) ? entry->bestMove : Move::Null();

		if (entry != nullptr && entry->depth >= currentMaxDepth)
		{
			using enum TranspositionTable::MoveBound;

			if (entry->bound == Lower) alpha = std::max(alpha, entry->score);

			if (entry->bound == Exact)
			{
				searchResult.bestMove = entry->bestMove;
				searchResult.evaluation = entry->score * ((pos.colorToMove() == White) ? 1 : -1);
				searchResult.depth++;
				continue;
			}
		}

		TranspositionTable::TTEntry newEntry =
		{
			.key		= pos.zobristKey(),
			.bestMove	= Move::Null(),
			.score		= alpha,
			.bound		= TranspositionTable::MoveBound::Upper,
			.depth		= currentMaxDepth
		};

		MovePicker picker(pos, moves, TTMove);

		while (picker.hasNext())
		{

			searchInfo.nodes++;
			Move move = picker.pick();

			pos.doMove(move);
			int eval = -negamax(pos, -Evaluation::Infinity, -alpha, 1, currentMaxDepth, searchInfo);
			pos.undoMove();

			if (searchInfo.stopped) break;

			if (eval > alpha)
			{
				alpha = eval;
				bestIterMove = move;

				newEntry.bound = TranspositionTable::MoveBound::Exact;
				newEntry.bestMove = move;
				newEntry.score = eval;
			}
		}

		if (searchInfo.stopped) break;

		TT.store(std::move(newEntry));

		searchResult.bestMove = bestIterMove;
		searchResult.evaluation = alpha * ((pos.colorToMove() == White) ? 1 : -1);
		searchResult.depth++;

		std::cout << "info depth " << currentMaxDepth
			<< " score cp " << alpha
			<< " nodes " << searchInfo.nodes
			<< "\n";
	}

	searchResult.nodes = searchInfo.nodes;

	return searchResult;
}

}