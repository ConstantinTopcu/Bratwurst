#include "search/search.h"

#include "position.h"
#include "move_gen.h"
#include "search/evaluate.h"

#include <chrono>

namespace Bratwurst::Search
{

// helps improve move ordering for quiet moves
constexpr int MaxHistory = 2048;
uint16 HistoryTable[SquareNum][SquareNum] = {0};

// evaluation constants
constexpr int Infinity = 100'000'000;
constexpr int MateBaseScore = 100'000;

// debug variables
size_t Nodes = 0;

int heuristicScore(Position& pos, Move m)
{
	constexpr int PromotionBonus = 100000;
	constexpr int CaptureBonus = MaxHistory; 

	Piece capturedPiece = pos.pieceOn(m.dst());

	if (capturedPiece != NonePiece)
	{
		//MVV-LVA
		return CaptureBonus + Evaluation::PieceValue[capturedPiece] - Evaluation::PieceValue[pos.pieceOn(m.src())];
	}

	if (m.promotion())
	{
		return PromotionBonus + Evaluation::PieceValue[m.promotionType()] - Evaluation::PieceValue[Pawn];
	}

	// quiet move
	return HistoryTable[m.src()][m.dst()];
}

int qsearch(Position& pos, int alpha, int beta, int ply)
{
	Nodes++;

	// Evaluate current position
	int standPat = Evaluation::evaluate(pos);

	// Fail-hard beta cutoff
	if (standPat >= beta)
	{
		return beta;
	}

	// Raise alpha if standPat is better
	if (standPat > alpha)
	{
		alpha = standPat;
	}

	// Generate capture moves only
	MoveList moves = generateMoves<GenType::Captures>(pos);

	StaticVector<int, MaxMoves> scores(moves.size());
	for (int i = 0; i < moves.size(); i++)
	{
		scores[i] = heuristicScore(pos, moves[i]);
	}

	int bestScore = standPat;

	for (int i = 0; i < moves.size(); i++)
	{
		int bestIdx = i;

		for (int j = i + 1; j < moves.size(); j++)
		{
			if (scores[j] > scores[bestIdx])
			{
				bestIdx = j;
			}
		}

		std::swap(scores[i], scores[bestIdx]);
		std::swap(moves[i], moves[bestIdx]);
			
		Move m = moves[i];

		pos.doMove(m);
		int score = -qsearch(pos, -beta, -alpha, ply + 1);
		pos.undoMove();

		if (score > bestScore)
		{
			bestScore = score;
			if (score > alpha)
			{
				alpha = score;
				if (alpha >= beta)
				{
					return beta;
				}
			}
		}
	}

	return bestScore;
}

int negamax(Position& pos, int alpha, int beta, int depth, int ply)
{
	Nodes++;

	// evaluate
	if (depth == 0)
	{
		return qsearch(pos, alpha, beta, ply);
	}

	MoveList moves = generateMoves<GenType::All>(pos);

	if (moves.size() == 0)
	{
		// either checkmate or stalemate
		return (pos.checkers()) ? -MateBaseScore + ply : 0;
	}

	StaticVector<int, MaxMoves> scores(moves.size());

	for (int i = 0; i < moves.size(); i++)
	{
		Move m = moves[i];
		int score = heuristicScore(pos, m);
		scores[i] = score;
	}

	int bestScore = -Infinity;

	for (int i = 0; i < moves.size(); i++)
	{
		int bestMoveIdx = i;

		for (int j = i; j < scores.size(); j++)
		{
			if (scores[j] > scores[bestMoveIdx])
			{
				bestMoveIdx = j;
			}
		}

		std::swap(moves[i], moves[bestMoveIdx]);
		std::swap(scores[i], scores[bestMoveIdx]);
		Move m = moves[i];

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
			if (pos.pieceOn(m.dst()) == NonePiece)
			{
				int bonus = depth * depth;
				int clampedBonus = std::clamp(bonus, -MaxHistory, MaxHistory);
				HistoryTable[m.src()][m.dst()] += clampedBonus - HistoryTable[m.src()][m.dst()] * abs(clampedBonus) / MaxHistory;
			}

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

	for (Square s1 = A1; s1 < SquareNum; s1++)
	{
		for (Square s2 = A1; s2 < SquareNum; s2++)
		{
			HistoryTable[s1][s2] = 0;
		}
	}

	for (int currDepth = 1; currDepth <= depth; currDepth++)
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