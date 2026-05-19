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
using enum TranspositionTable::MoveBound;

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
int historyTable[ColorNum][SquareNum][SquareNum];

// PV tables
Move PvTable[MaxSearchDepth][MaxSearchDepth];
int  PvLength[MaxSearchDepth] = {};
Move prevPV[MaxSearchDepth];
int  prevPVLength = 0;

inline void updatePV(int ply, Move move)
{
	PvTable[ply][ply] = move;

	for (int i = ply + 1; i < PvLength[ply + 1]; i++)
		PvTable[ply][i] = PvTable[ply + 1][i];
	
	PvLength[ply] = PvLength[ply + 1];
}

inline void clearPV(int ply)
{
	PvLength[ply] = ply;
}

inline void clearPVTable()
{
	for (int i = 0; i < MaxSearchDepth; i++)
	{
		PvLength[i] = i;
		for (int j = 0; j < MaxSearchDepth; j++)
			PvTable[i][j] = Move::Null();
	}
}

inline bool isMateScore(int score)
{
	return std::abs(score) >= Evaluation::CheckMate - MaxSearchDepth;
}

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

inline void ageHistory()
{
	for (auto& side : historyTable)
		for (auto& from : side)
			for (auto& val : from)
				val /= 2;
}

inline void ageKillerMoves()
{
	for (int p = 1; p < MaxSearchDepth; p++)
	{
		killerMoves[p - 1][0] = killerMoves[p][0];
		killerMoves[p - 1][1] = killerMoves[p][1];
	}

	killerMoves[MaxSearchDepth - 1][0] = Move::Null();
	killerMoves[MaxSearchDepth - 1][1] = Move::Null();
}


inline void updateKillers(int ply, Move move)
{
	if (killerMoves[ply][0] != move)
	{
		killerMoves[ply][1] = killerMoves[ply][0];
		killerMoves[ply][0] = move;
	}
}

inline void updateHistory(Color c, Move move, int bonus)
{
	int& historyValue = historyTable[c][move.src()][move.dst()];
	historyValue = std::clamp(historyValue + bonus, -16384, 16384);
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
	if (searchInfo.stopped)	return -Evaluation::Infinity;

	// check for draw
	if (pos.isThreefoldRepetition(2)) return 0;

	Zobrist::Key key = pos.zobristKey();

	const TranspositionTable::TTEntry* ttEntry = TT.probe(key);

	if (ttEntry != nullptr)
	{
		int ttScore = scoreFromTT(ttEntry->score, ply);
		if (ttEntry->bound == Lower) alpha = std::max(alpha, ttScore);
		if (ttEntry->bound == Upper) beta = std::min(beta, ttScore);
		if (ttEntry->bound == Exact) return ttScore;
		if (alpha >= beta) return alpha;
	}

	int standPat = Evaluation::evaluate(pos);

	if (standPat >= beta || ply == depth)
	{
		TT.store
		({
			.key = key,
			.bestMove = Move::Null(),
			.score = scoreToTT(standPat, ply),
			.bound = Lower,
			.depth = 0,
		});

		return standPat;
	}

	MoveList moves = generateMoves<GenType::Quiescence>(pos);

	if (moves.empty())
	{
		return standPat;
	}

	TranspositionTable::TTEntry newEntry =
	{
		.key = key,
		.bestMove = Move::Null(),
		.score = alpha,
		.bound = Upper,
		.depth = 0,
	};

	Move ttMove = (ttEntry != nullptr) ? ttEntry->bestMove : Move::Null();
	MovePicker picker(pos, moves, killerMoves[ply], historyTable, ttMove);

	alpha = std::max(alpha, standPat);

	while (picker.hasNext())
	{
		Move move = picker.pick();
		Piece capturedPiece = pos.pieceOn(move.dst());
		PieceType capturedType = (capturedPiece != NonePiece) ? pieceTypeOf(capturedPiece) : NonePieceType;
		
		// calculate maximum material gain from this move
		int captureValue = 0;
		if (capturedPiece != NonePiece) captureValue += Evaluation::PieceValue[capturedType];
		if (move.promotion()) captureValue += Evaluation::PieceValue[move.promotionType()] - Evaluation::PieceValue[Pawn];
		if (move.enPassant()) captureValue = Evaluation::PieceValue[Pawn];

		// Delta Pruning:
		// if even after the capture you are still significantly below alpha, 
		// then this move is unlikely to raise alpha and can be pruned
		if (standPat + captureValue + 200 < alpha)
			continue; // skip move

		pos.doMove(move);
		int eval = -quiescence(pos, -beta, -alpha, ply + 1, depth, searchInfo);
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
			newEntry.bestMove = move;
			newEntry.score = scoreToTT(alpha, ply);
			newEntry.bound = Lower;

			break;
		}
	}

	TT.store(std::move(newEntry));

	return alpha;
}

int negamax(Position& pos, int alpha, int beta, int ply, int maxDepth, SearchInfo& searchInfo)
{
	clearPV(ply);
	searchInfo.nodes++;

	// check for search cancellation
	updateTime(searchInfo);
	if (searchInfo.stopped) return -Evaluation::Infinity;

	bool isRoot = (ply == 0);
	int remainingDepth = maxDepth - ply;

	Zobrist::Key key = pos.zobristKey();
	Color c = pos.colorToMove();
	bool isCheck = pos.checkers();
	int phase = pos.phase();
	
	// check for 3 fold repetition
	if (!isRoot && pos.isThreefoldRepetition(2)) return 0;

	// Check for position in transposition table
	const TranspositionTable::TTEntry* ttEntry = TT.probe(key);

	if (ttEntry != nullptr && ttEntry->depth >= remainingDepth && !isRoot)
	{
		int ttScore = scoreFromTT(ttEntry->score, ply);
		if (ttEntry->bound == Lower) alpha = std::max(alpha, ttScore);
		if (ttEntry->bound == Upper) beta = std::min(beta, ttScore);
		if (ttEntry->bound == Exact) return ttScore;
		if (alpha >= beta) return alpha;
	}

	// start quescence search at max depth to minimize horizon effect
	if (ply == maxDepth) 
		return quiescence(pos, alpha, beta, ply + 1, ply + MaxQuiescenceDepth, searchInfo);

	bool nullMoveAllowed = pos.stateInfo().prevMove != Move::Null();

	// Null Move Pruning (https://www.chessprogramming.org/Null_Move_Pruning):
	// if you skip your turn and you still fail high, then this position is too good for you 
	// and the opponent won't let you reach it, so you can safely prune it
	if (nullMoveAllowed && phase >= 2 && !isCheck && remainingDepth > 3)
	{
		int reduction = 3 + remainingDepth / 6;
		int NMPDepth = std::max(ply + 1, maxDepth - reduction);

		pos.doNullMove();
		int nullEval = -negamax(pos, -beta, -beta + 1, ply + 1, NMPDepth, searchInfo);
		pos.undoNullMove();

		if (nullEval >= beta) return beta; // prune
	}

	int staticEval = Evaluation::evaluate(pos);
	
	// Reverse Futility Pruning (https://www.chessprogramming.org/Reverse_Futility_Pruning):
	// if the static evaluation minus a margin, is still above beta, its very likely 
	// that this position will raise alpha and cause a beta cutoff, so we can safely prune it
	if (!isCheck && remainingDepth <= 6)
	{
		// scale the margin linearly based on remaining depth, 
		// to account for the increasing uncertainty 
		// of the static evaluation as we go deeper
		int margin = remainingDepth * 100;

		if (staticEval - margin >= beta)
		{
			return staticEval; // prune
		}
	}

	MoveList moves = generateMoves<GenType::All>(pos);

	// check for stalemate/checkmate
	if (moves.empty())
	{
		return isCheck ? -Evaluation::CheckMate + ply : Evaluation::StaleMate;
	}

	TranspositionTable::TTEntry newEntry =
	{
		.key = key,
		.bestMove = Move::Null(),
		.score = alpha,
		.bound = Upper,
		.depth = remainingDepth,
	};

	Move bestMove = (ttEntry != nullptr) ? ttEntry->bestMove : prevPV[ply];
	MovePicker picker(pos, moves, killerMoves[ply], historyTable, bestMove);
	int eval = -Evaluation::Infinity;

	while (picker.hasNext())
	{
		Move move = picker.pick();

		Piece movingPiece = pos.pieceOn(move.src());
		Piece capturedPiece = pos.pieceOn(move.dst());
		bool isCapture = capturedPiece != NonePiece || move.enPassant();

		pos.doMove(move);

		bool givesCheck = pos.checkers();

		// Futility Pruning (https://www.chessprogramming.org/Futility_Pruning):
		// if the static evaluation plus a margin is still below alpha, 
		// then this move is unlikely to raise alpha and can be pruned
		if (remainingDepth <= 3 
			&& !isCapture  && !move.promotion() 
			&& !isCheck  && !givesCheck
			&& !isMateScore(alpha))
		{
			static constexpr int futilityMargin[4] = { 0, 100, 200, 400 }; // in centipawns
			int margin = futilityMargin[remainingDepth];

			if (staticEval + margin < alpha)
			{
				pos.undoMove();
				continue;
			}
		}

		// Late Move Reduction (https://www.chessprogramming.org/Late_Move_Reductions):
		// if the position is quiet and the move is late in the move ordering, 
		// then we can search it with a reduced depth to save time,
		bool doLMR =
			remainingDepth >= 3 &&
			picker.pickedCnt() >= 3 &&
			!isCapture && !givesCheck &&
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
			int extension = isCheck ? 1 : 0;
			eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth + extension, searchInfo);
		}

		pos.undoMove();

		if (searchInfo.stopped) return -Evaluation::Infinity;

		if (eval > alpha)
		{
			alpha = eval;

			newEntry.bound = Exact;
			newEntry.bestMove = move;
			newEntry.score = scoreToTT(eval, ply);

			updatePV(ply, move);
		}

		if (alpha >= beta)
		{
			if (!isCapture)
			{
				updateKillers(ply, move);
				updateHistory(c, move, remainingDepth * remainingDepth);
			}

			newEntry.bound = Lower;
			newEntry.score = scoreToTT(alpha, ply);
			newEntry.bestMove = move;

			break; // beta cutoff
		}
	}

	TT.store(std::move(newEntry));

	return alpha;
}

SearchResult search(Position& pos, int timeMs)
{
	TT.startNewSearch();
	ageKillerMoves();
	ageHistory();

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

	MoveList moves = generateMoves<GenType::All>(pos);

	// check for stalemate/checkmate
	if (moves.empty())
	{
		searchResult.evaluation = pos.checkers() ? -Evaluation::CheckMate : Evaluation::StaleMate;
		return searchResult;
	}

	Color friendly = pos.colorToMove();
	Zobrist::Key zobristKey = pos.zobristKey();
	int prevIterationEval = 0;

	// Iterative Deepening (https://www.chessprogramming.org/Iterative_Deepening):
	for (int currentMaxDepth = 1; currentMaxDepth <= 100; currentMaxDepth++)
	{
		// set initial delta for aspiration window to +/- 50 centipawns
		int delta = Evaluation::PieceValue[Pawn] / 2;
		int alpha =  -Evaluation::Infinity;
		int beta =  Evaluation::Infinity;

		// set aspiration window around previous iteration evaluation
		if (currentMaxDepth > 4)
		{
			alpha = prevIterationEval - delta;
			beta = prevIterationEval + delta;
		}

		// Aspiration retry loop
		while (true)
		{
			int score = negamax(pos, alpha, beta, 0, currentMaxDepth, searchInfo);

			if (searchInfo.stopped) goto searchStopped;

			if (score <= alpha)  // fail low
			{
				alpha = std::max(alpha - delta, -Evaluation::Infinity);
				delta *= 2;
			}
			else if (score >= beta)  // fail high
			{
				beta = std::min(beta + delta, Evaluation::Infinity);
				delta *= 2;
			}
			else  // inside window
			{
				prevIterationEval = score;
				break;
			}
		}

		// copy PV line from this iteration to previous iteration PV line
		prevPVLength = PvLength[0];
		for (int i = 0; i < prevPVLength; i++)
			prevPV[i] = PvTable[0][i];

		searchResult.bestMove = prevPV[0];  // root move is just the first PV entry
		searchResult.evaluation = prevIterationEval * Evaluation::colorMultiplier(friendly);
		searchResult.depth++;

		bool checkMateFound = std::abs(searchResult.evaluation) >= Evaluation::CheckMate - MaxSearchDepth;
		int mateIn = checkMateFound ? std::abs(searchResult.evaluation - Evaluation::CheckMate) : 0;
		std::string mateInfo = checkMateFound ? ((searchResult.evaluation > 0) ? ("mate " + std::to_string(mateIn)) : ("mate -" + std::to_string(mateIn))) : std::to_string(searchResult.evaluation);

		std::cout << "info depth " << currentMaxDepth
			<< " score cp " << mateInfo
			<< " nodes " << searchInfo.nodes
			<< " time " << (std::chrono::duration<float>(Clock::now() - searchInfo.start).count()) << "s"
			<< " NPS " << searchInfo.nodes / (std::chrono::duration<float>(Clock::now() - searchInfo.start).count()) / 1000000.0f << "M"
			<< "\n";
	}

searchStopped:
	// print pv
	std::cout << "info pv ";

	for (int i = 0; i < prevPVLength; i++)
		std::cout << prevPV[i].toString() << " ";

	std::cout << "\n";

	searchResult.nodes = searchInfo.nodes;	
	return searchResult;
}

}