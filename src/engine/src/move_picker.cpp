#include <engine/search/move_picker.h>

namespace Bratwurst::Search
{
	constexpr int HeuristicCaptureBonus = 200;
	constexpr int HeuristicPromotionBonus = 1000;

	int score_move(const Position& pos, Move move, Move* killers, const int history[][64][64], Move ttMove)
	{
		if (move == ttMove) return Evaluation::Infinity;

		int eval = 0;

		Piece srcPiece = pos.pieceOn(move.src());
		Piece capturePiece = pos.pieceOn(move.dst());
		Color c = pos.colorToMove();

		if (capturePiece != NonePiece)
		{
			eval += Evaluation::PieceValue[capturePiece];
			eval -= Evaluation::PieceValue[srcPiece] >> 3;
			eval += HeuristicCaptureBonus;
		}

		else
		{
			if (killers != nullptr)
			{
				if (move == killers[0]) eval += 70;
				if (move == killers[1])	eval += 65;
			}

			if (history != nullptr)
			{
				eval += history[c][move.src()][move.dst()] >> 7;
			}
		}

		// if you move to a square that is attacked by a pawn, you likely loose your piece
		if (Precomputed::pseudoAttacks[makePiece(c, Pawn)][move.dst()] & pos.pieceBB(~c, Pawn))
		{
			eval -= Evaluation::PieceValue[srcPiece];
		}

		if (move.promotion())
		{
			eval += HeuristicPromotionBonus + Evaluation::PieceValue[move.promotionType()];
		}

		return eval;
	}

}