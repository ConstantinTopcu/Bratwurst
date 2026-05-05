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

TranspositionTable TT(256); // 256 MB
Move killerMoves[MaxSearchDepth][2];

// Helper functions to store scores in TT and probe it from TT
inline int scoreToTT(int score, int ply) 
{
	if (score >= Evaluation::CheckMate - MaxSearchDepth) return score + ply;  // make it relative to this node
	if (score <= -Evaluation::CheckMate + MaxSearchDepth) return score - ply;
	return score;
}

inline int scoreFromTT(int score, int ply) 
{
	if (score >= Evaluation::CheckMate - MaxSearchDepth) return score - ply;  // convert back to root-relative
	if (score <= -Evaluation::CheckMate + MaxSearchDepth) return score + ply;
	return score;
}

inline bool updateTime(SearchInfo& info)
{
	if ((info.nodes & 2047) == 0)
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - info.start);
		info.stopped = elapsed.count() >= info.timeMS * 0.95f; // 1 MS time buffer to cancel search
		return info.stopped;
	}
}

int quiescence(Position& pos, int alpha, int beta, int ply, int depth, SearchInfo& searchInfo)
{
	searchInfo.nodes++;

	if (pos.isThreefoldRepetition(2)) return -10;

	updateTime(searchInfo);

	if (searchInfo.stopped)
	{
		return -Evaluation::Infinity;
	}

	int standPat = Evaluation::evaluate(pos);

	if (standPat >= beta || ply == depth)
	{
		return standPat;
	}

	alpha = std::max(alpha, standPat);

	MoveList moves = generateMoves<GenType::Quiescence>(pos);

	if (moves.empty())
	{
		return standPat;
	}

	MovePicker picker(pos, moves, killerMoves[ply]);

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
	searchInfo.nodes++;

	int remainingDepth = maxDepth - ply;

	// check for 3 fold repetition
	if (pos.isThreefoldRepetition(2)) return -10;

	updateTime(searchInfo);

	// return -Infinity so the move won't be considered
	if (searchInfo.stopped) return -Evaluation::Infinity;

	// Check for position in transposition table
	const TranspositionTable::TTEntry* ttEntry = TT.probe(pos.zobristKey());
	Move ttMove = (ttEntry != nullptr) ? ttEntry->bestMove : Move::Null();

	if (ttEntry != nullptr && ttEntry->depth >= remainingDepth)
	{
		using enum TranspositionTable::MoveBound;

		int ttScore = scoreFromTT(ttEntry->score, ply);
		if (ttEntry->bound == Lower) alpha = std::max(alpha, ttScore);
		if (ttEntry->bound == Upper) beta = std::min(beta, ttScore);
		if (ttEntry->bound == Exact) return ttScore;
		if (alpha >= beta) return ttScore;
	}

	if (ply == maxDepth)
		return quiescence(pos, alpha, beta, ply + 1, ply + MaxQuiescenceDepth, searchInfo);

	auto moves = generateMoves<GenType::All>(pos);

	// check for stalemate/checkmate
	if (moves.empty())
		return (pos.checkers()) ? -Evaluation::CheckMate + ply : Evaluation::StaleMate;
	
	TranspositionTable::TTEntry newEntry =
	{
		.key		= pos.zobristKey(),
		.bestMove	= Move::Null(),
		.score		= alpha,
		.bound		= TranspositionTable::MoveBound::Upper,
		.depth		= remainingDepth
	};

	MovePicker picker(pos, moves, killerMoves[ply], ttMove);

	while (picker.hasNext())
	{
		Move move = picker.pick();
		int eval;

		bool isCheck = pos.checkers();

		pos.doMove(move);

		bool isCapture = pos.stateInfo().capturedPiece != NonePiece;
		bool givesCheck = pos.checkers();

		bool doLMR =
			remainingDepth >= 3 &&
			picker.pickedCnt() >= 3 &&
			!isCapture &&
			!givesCheck &&
			!move.promotion() &&
			!isCheck;

		if (doLMR)
		{
			int depthReduction = int(0.75f + std::log(remainingDepth) * std::log(picker.pickedCnt()) / 2.25f);

			// conduct null window search to check wether it raises alpha
			eval = -negamax(pos, -alpha - 1, -alpha, ply + 1, maxDepth - depthReduction, searchInfo);

			// research at full depth since it raised alpha
			if (eval > alpha)
			{
				eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, searchInfo);
			}
		}

		else
		{
			eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, searchInfo);
		}

		pos.undoMove();

		if (eval > alpha)
		{
			alpha = eval;

			newEntry.bound = TranspositionTable::MoveBound::Exact;
			newEntry.bestMove = move;
			newEntry.score = scoreToTT(eval, ply);
		}

		if (alpha >= beta)
		{
			newEntry.bound = TranspositionTable::MoveBound::Lower;
			newEntry.score = scoreToTT(alpha, ply);
			newEntry.bestMove = move;

			if (!isCapture && killerMoves[ply][0] != move)
			{
				killerMoves[ply][1] = killerMoves[ply][0];
				killerMoves[ply][0] = move;
			}

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

	for (int ply = 0; ply < MaxSearchDepth; ply++)
	{
		killerMoves[ply][0] = Move::Null();
		killerMoves[ply][1] = Move::Null();
	}

	// make sure game hasnt terminated yet
	if (moves.empty()) return searchResult;

	for (int currentMaxDepth = 1; currentMaxDepth <= 100; currentMaxDepth++)
	{
		const TranspositionTable::TTEntry* entry = TT.probe(pos.zobristKey());
		Move TTMove = (entry) ? entry->bestMove : Move::Null();
		int alpha = -Evaluation::Infinity;
		Move bestIterMove = moves[0];

		if (entry != nullptr && entry->depth >= currentMaxDepth)
		{
			using enum TranspositionTable::MoveBound;

			if (entry->bound == Lower)
			{
				alpha = std::max(alpha, entry->score);
			}

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

		MovePicker picker(pos, moves, killerMoves[0], TTMove);

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
			<< " time " << (std::chrono::duration<float>(Clock::now() - searchInfo.start).count()) << "s"
			<< " NPS " << searchInfo.nodes / (std::chrono::duration<float>(Clock::now() - searchInfo.start).count())/1000000.0f << "M"
			<< "\n";
	}

	searchResult.nodes = searchInfo.nodes;

	return searchResult;
}

}