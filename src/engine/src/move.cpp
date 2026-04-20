#include <engine/types/move.h>
#include <engine/position/position.h>

namespace Bratwurst
{

Move Move::fromString(const std::string& str, const Position& pos)
{
	ASSERT(str.size() >= 4);

	Square src = stringToSquare(str.substr(0, 2));
	Square dst = stringToSquare(str.substr(2, 2));

	// Promotion
	if (str.size() == 5)
	{
		Flag flag;
		switch (str[4])
		{
		case 'n': flag = Flag::KnightPromotion; break;
		case 'b': flag = Flag::BishopPromotion; break;
		case 'r': flag = Flag::RookPromotion;   break;
		case 'q': flag = Flag::QueenPromotion;  break;
		default: ASSERT(false);
		}
		return Move(src, dst, flag);
	}

	// Castling
	if (pos.pieceOn(src) == King && std::abs((int)(src)-(int)(dst)) == 2)
		return Move(src, dst, (dst > src) ? Flag::CastlingOO : Flag::CastlingOOO);

	// En passant
	if (pos.pieceOn(src) == Pawn && dst == pos.enPassantSquare())
		return Move(src, dst, Flag::EnPassant);

	return Move(src, dst);
}

}