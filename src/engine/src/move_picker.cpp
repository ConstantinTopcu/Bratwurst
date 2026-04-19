#include <engine/search/move_picker.h>

namespace Bratwurst::Search
{
	constexpr int HeuristicCaptureBonus = 200;
	constexpr int HeuristicPromotionBonus = 1000;

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
			eval += HeuristicCaptureBonus;
		}

		// if you move to a square that is attacked by a pawn, you likely loose your piece
		if (Precomputed::pseudoAttacks[makePiece(c, Pawn)][move.dst()] & pos.pieceBB(~c, Pawn))
		{
			eval -= Evaluation::TypeValue[pieceTypeOf(srcPiece)];
		}

		if (move.promotion())
		{
			eval += HeuristicPromotionBonus + Evaluation::TypeValue[move.promotionType()];
		}

		return eval;
	}

}