#include <engine/search/search.h>
#include <engine/move_gen/move_gen.h>
#include <engine/search/evaluation.h>

namespace Bratwurst::Search
{



int score_move(const Position& pos, Move move)
{
	int eval = 0;

	Piece srcPiece = pos.pieceOn(move.src());
	Piece capturePiece = pos.pieceOn(move.dst());
	Color c = pos.colorToMove();

	if (capturePiece != NonePiece)
	{
		eval += Evaluation::TypeValue[pieceTypeOf(capturePiece)];
		eval -= Evaluation::TypeValue[pieceTypeOf(srcPiece)] >> 3;
		eval += 200;
	}

	// if you move to a square that is attacked by a pawn, you likely loose your piece
	if (Precomputed::pseudoAttacks[makePiece(c, Pawn)][move.dst()] & pos.pieceBB(~c, Pawn))
	{
		eval -= Evaluation::TypeValue[pieceTypeOf(srcPiece)];
	}

	if (move.promotion())
	{
		eval += 1'000 + Evaluation::TypeValue[move.promotionType()];
	}

	return eval;
}

int negamax(Position& pos, int alpha, int beta, int ply, int maxDepth, int& nodes)
{
	nodes++;

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

	// create parralel vector
	std::vector<int> evals;
	evals.reserve(moves.size());

	for (int i = 0; i < moves.size(); i++)
	{
		evals[i] = score_move(pos, moves[i]);
	}

	for (int i = 0; i < moves.size(); i++)
	{
		int bestHeuristicIndex = i;

		for (int j = i; j < moves.size(); j++)
		{
			if (evals[j] > evals[bestHeuristicIndex])
			{
				bestHeuristicIndex = j;
			}
		}

		Move bestHeuristicMove = moves[bestHeuristicIndex];
		std::swap(moves[bestHeuristicIndex], moves[i]);
		std::swap(evals[bestHeuristicIndex], evals[i]);

		pos.doMove(bestHeuristicMove);

		// recursivly call negamax
		int eval = -negamax(pos, -beta, -alpha, ply + 1, maxDepth, nodes);
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
	int nodes = 0;

	auto moves = generateMoves<GenType::All>(pos);

	for (Move move : moves)
	{
		pos.doMove(move);

		// recursivly call minimax
		int eval = -negamax(pos, -Infinity, +Infinity, 1, depth, nodes);

		if (eval > bestEval)
		{
			bestEval = eval;
			bestMove = move;
		}

		pos.undoMove();
	}

	return { bestEval, bestMove, nodes};
}

}