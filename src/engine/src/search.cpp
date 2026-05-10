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
	float aspirationWindowFailPercantage;
	Clock::time_point start;
	bool stopped = false;
	int timeMS = 0;
	int nodes = 0;
};

constexpr int MaxQuiescenceDepth = 20;
constexpr int MaxSearchDepth = 100;

TranspositionTable TT(256); // 256 MB
Move killerMoves[MaxSearchDepth][2];
int historyTable[ColorNum][SquareNum][SquareNum];

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
	}

	return info.stopped;
}

int quiescence(Position& pos, int alpha, int beta, int ply, int depth, SearchInfo& searchInfo)
{
	searchInfo.nodes++;

	// make sure search hasnt been cancelled yet
	updateTime(searchInfo);

	if (searchInfo.stopped)
	{
		return -Evaluation::Infinity;
	}

	// check for draw
	if (pos.isThreefoldRepetition(2)) return 0;

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

	MovePicker picker(pos, moves, killerMoves[ply], historyTable);

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

	// check for search cancellation
	updateTime(searchInfo);
	if (searchInfo.stopped) return -Evaluation::Infinity;

	// check for 3 fold repetition
	if (pos.isThreefoldRepetition(2)) return 0;

	// Check for position in transposition table
	using enum TranspositionTable::MoveBound;
	const TranspositionTable::TTEntry* ttEntry = TT.probe(pos.zobristKey());
	Move ttMove = (ttEntry != nullptr) ? ttEntry->bestMove : Move::Null();

	if (ttEntry != nullptr && ttEntry->depth >= remainingDepth)
	{
		int ttScore = scoreFromTT(ttEntry->score, ply);
		if (ttEntry->bound == Lower) alpha = std::max(alpha, ttScore);
		if (ttEntry->bound == Upper) beta = std::min(beta, ttScore);
		if (ttEntry->bound == Exact) return ttScore;
		if (alpha >= beta) return alpha;
	}

	if (ply == maxDepth) return quiescence(pos, alpha, beta, ply + 1, ply + MaxQuiescenceDepth, searchInfo);

	auto moves = generateMoves<GenType::All>(pos);

	// check for stalemate/checkmate
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
		.depth		= remainingDepth
	};

	bool isCheck = pos.checkers();
	bool nullMoveAllowed = pos.stateInfo().prevMove != Move::Null();

	// Null Move Pruning
	if (nullMoveAllowed && pos.phase() >= 2 && !isCheck && remainingDepth > 3)
	{
		int reduction = 3 + remainingDepth / 6;
		int NMPDepth = std::max(ply + 1, maxDepth - reduction);

		pos.doNullMove();
		int nullEval = -negamax(pos, -beta, -beta + 1, ply + 1, NMPDepth, searchInfo);
		pos.undoNullMove();

		// if you skip your turn and you still fail high, 
		// then this position is too good for you 
		// and the opponent won't let you reach it, 
		// so you can safely prune it
		if (nullEval >= beta) return beta;
	}
	
	MovePicker picker(pos, moves, killerMoves[ply], historyTable, ttMove);
	int staticEval = Evaluation::evaluate(pos);
	int eval = -Evaluation::Infinity;

	while (picker.hasNext())
	{
		Move move = picker.pick();

		Piece movingPiece = pos.pieceOn(move.src());
		Piece capturedPiece = pos.pieceOn(move.dst());
		bool isCapture = capturedPiece != NonePiece || move.enPassant();

		pos.doMove(move);

		bool givesCheck = pos.checkers();

		// Futility Pruning
		if (remainingDepth <= 3 
			&& !isCapture  && !move.promotion() 
			&& !isCheck  && !givesCheck
			&& std::abs(alpha) < Evaluation::CheckMate - MaxSearchDepth)
		{
			static constexpr int futilityMargin[4] = { 0, 100, 200, 400 }; // in centipawns
			int margin = futilityMargin[remainingDepth];

			// if the static evaluation plus a margin is still below alpha, then this move is unlikely to raise alpha and can be pruned
			if (staticEval + margin < alpha)
			{
				pos.undoMove();
				continue;
			}
		}

		bool doLMR =
			remainingDepth >= 3 &&
			picker.pickedCnt() >= 3 &&
			!isCapture && !pos.checkers() &&
			!move.promotion() && !isCheck;

		if (doLMR)
		{
			int depthReduction = int(0.75f + std::log(remainingDepth) * std::log(picker.pickedCnt()) / 2.25f);
			int nullWindowDepth = std::max(ply + 1, maxDepth - depthReduction);

			// conduct null window search to check wether it raises alpha
			eval = -negamax(pos, -alpha - 1, -alpha, ply + 1, nullWindowDepth, searchInfo);

			if (eval > alpha)
			{
				// research at full depth since it raised alpha
				eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, searchInfo);
			}
		}

		else
		{
			// check extensions
			int extension = pos.checkers() ? 1 : 0;
			eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth + extension, searchInfo);
		}

		pos.undoMove();

		if (eval > alpha)
		{
			alpha = eval;

			newEntry.bound = Exact;
			newEntry.bestMove = move;
			newEntry.score = scoreToTT(eval, ply);
		}

		if (alpha >= beta)
		{
			newEntry.bound = Lower;
			newEntry.score = scoreToTT(alpha, ply);
			newEntry.bestMove = move;

			if (!isCapture)
			{
				if (killerMoves[ply][0] != move)
				{
					// update killer moves
					killerMoves[ply][1] = killerMoves[ply][0];
					killerMoves[ply][0] = move;
				}

				// update history — cap to prevent overflow
				Color c = pos.colorToMove();
				historyTable[c][move.src()][move.dst()] += remainingDepth * remainingDepth;
				historyTable[c][move.src()][move.dst()] = std::min(historyTable[c][move.src()][move.dst()], 16384);
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
		.start = Clock::now(),
		.stopped = false,
		.timeMS = timeMs,
		.nodes = 0
	};

	SearchResult searchResult =
	{
		.evaluation = -Evaluation::Infinity,
		.bestMove = Move::Null(),
		.nodes = 0,
		.depth = 0
	};

	// shift killer moves down the stack and clear the top level
	for (int ply = 0; ply < MaxSearchDepth - 1; ply++)
	{
		killerMoves[ply][0] = killerMoves[ply + 1][0];
		killerMoves[ply][1] = killerMoves[ply + 1][1];
	}

	killerMoves[MaxSearchDepth - 1][0] = Move::Null();
	killerMoves[MaxSearchDepth - 1][1] = Move::Null();

	// age history instead of clearing — preserves useful info
	for (auto& side : historyTable)
		for (auto& from : side)
			for (auto& val : from)
				val /= 2;

	MoveList moves = generateMoves<GenType::All>(pos);

	// check for stalemate/checkmate
	if (moves.empty())
	{
		searchResult.evaluation = pos.checkers() ? -Evaluation::CheckMate : Evaluation::StaleMate;
		return searchResult;
	}

	int prevIterationEval = 0;

	for (int currentMaxDepth = 1; currentMaxDepth <= 100; currentMaxDepth++)
	{
		// check for fitting transposition table entry with sufficient depth
		using Bound = TranspositionTable::MoveBound;
		const TranspositionTable::TTEntry* entry = TT.probe(pos.zobristKey());
		Move TTMove = (entry) ? entry->bestMove : Move::Null();
		Move bestIterMove = moves[0];

		if (entry != nullptr && entry->depth >= currentMaxDepth && entry->bound == Bound::Exact)
		{
			searchResult.bestMove = entry->bestMove;
			searchResult.evaluation = entry->score * ((pos.colorToMove() == White) ? 1 : -1);
			searchResult.depth++;
			continue;
		}

		// set initial delta for aspiration window to +/- 50 centipawns
		int delta = Evaluation::PieceValue[Pawn] / 2;
		int alpha, beta;

		if (currentMaxDepth > 4)
		{
			// set aspiration window bounds around previous iteration's evaluation
			alpha = prevIterationEval - delta;
			beta = prevIterationEval + delta;
		}
		else
		{
			alpha = -Evaluation::Infinity;
			beta = Evaluation::Infinity;
		}

		// Aspiration retry loop
		while (true)
		{
			// search alpha is the best score found so far, 
			// starting with the lower bound of the aspiration window
			int searchAlpha = alpha;
			bestIterMove = moves[0];

			MovePicker picker(pos, moves, killerMoves[0], historyTable, TTMove);

			while (picker.hasNext())
			{
				Move move = picker.pick();

				pos.doMove(move);
				int eval = -negamax(pos, -beta, -searchAlpha, 0, currentMaxDepth, searchInfo);
				pos.undoMove();

				if (searchInfo.stopped) goto searchStopped;

				// update best score found so far
				if (eval > searchAlpha)
				{
					searchAlpha = eval;
					bestIterMove = move;
				}

				// search failed high already -> no need to continue searching
				if (searchAlpha >= beta) break;
			}

			// store results in transposition table
			TranspositionTable::TTEntry newEntry =
			{
				.key = pos.zobristKey(),
				.bestMove = bestIterMove,
				.score = searchAlpha,
				.bound = Bound::Exact,
				.depth = currentMaxDepth
			};

			// Fail low — widen lower bound and retry (gradual widening)
			if (searchAlpha <= alpha)
			{
				newEntry.bound = Bound::Upper;
				TT.store(std::move(newEntry));
				alpha = std::max(alpha - delta, -Evaluation::Infinity);
				delta *= 2;
			}

			// Fail high — widen upper bound and retry (gradual widening)
			else if (searchAlpha >= beta)
			{
				newEntry.bound = Bound::Lower;
				TT.store(std::move(newEntry));
				beta = std::min(beta + delta, Evaluation::Infinity);
				delta *= 2;
			}

			// Score inside window
			else
			{
				newEntry.bound = Bound::Exact;
				TT.store(std::move(newEntry));
				prevIterationEval = searchAlpha;
				break;
			}
		}

		searchResult.bestMove = bestIterMove;
		searchResult.evaluation = prevIterationEval * Evaluation::colorMultiplier(pos.colorToMove());
		searchResult.depth++;

		bool checkMateFound = std::abs(searchResult.evaluation) >= Evaluation::CheckMate - MaxSearchDepth;
		int mateIn = (checkMateFound) ? std::abs(searchResult.evaluation - Evaluation::CheckMate) : 0;

		std::cout << "info depth " << currentMaxDepth
			<< " score cp " << (checkMateFound ? (searchResult.evaluation > 0) ? ("mate " + std::to_string(mateIn)) : ("mate -" + std::to_string(mateIn)) : (std::to_string(searchResult.evaluation)))
			<< " nodes " << searchInfo.nodes
			<< " time " << (std::chrono::duration<float>(Clock::now() - searchInfo.start).count()) << "s"
			<< " NPS " << searchInfo.nodes / (std::chrono::duration<float>(Clock::now() - searchInfo.start).count()) / 1000000.0f << "M"
			<< "\n";
	}

searchStopped:
	searchResult.nodes = searchInfo.nodes;
	return searchResult;
}

}