#include "search/evaluate.h"
#include "search/evaluation_constants.h"
#include "position.h"

namespace Bratwurst::Evaluation
{

int perspectiveMultiplier(Color c)
{
	return (c == White ? 1 : -1);
}

template<PieceType pt, Color c>
int mobilityBonus(const Position& pos)
{
	constexpr int MobilityBonus[PieceTypeNum] = { 0, 4, 4, 2, 1, 0 }; // pawn, knight, bishop, rook, queen, king

	Bitboard pieces = pos.pieceBB(c, pt);
	int score = 0;

	while (pieces)
	{
		Square s = popLsb(pieces);
		int attackCnt = popCnt(attacksBB<pt, c>(s, pos.occupancyBB()) & ~pos.colorBB(c));
		score += attackCnt * MobilityBonus[pt];
	}

	return score;
}

//TODO: store mgPstValue and egPstValue inside of stateInfo and update it incrementally
// Also store the stage as integer and compute the float dynamicly when needed
int evaluate(Position& pos)
{
	int eval =  pos.material(White) - pos.material(Black);

	int mgScore = 0;
	int egScore = 0;
	int gamePhase = 0;

	constexpr int maxGamePhase = 24; // 16 pawns + 4 knights + 4 bishops
	constexpr int phaseValues[PieceTypeNum] = { 0, 1, 1, 2, 4, 0 };
	
	float currentPhase = 1.0f - 
	std::min(
		pos.pieceCnt(WhitePawn) * phaseValues[Pawn] + pos.pieceCnt(BlackPawn) * phaseValues[Pawn] +
		pos.pieceCnt(WhiteKnight) * phaseValues[Knight] + pos.pieceCnt(BlackKnight) * phaseValues[Knight] +
		pos.pieceCnt(WhiteBishop) * phaseValues[Bishop] + pos.pieceCnt(BlackBishop) * phaseValues[Bishop] +
		pos.pieceCnt(WhiteRook) * phaseValues[Rook] + pos.pieceCnt(BlackRook) * phaseValues[Rook] +
		pos.pieceCnt(WhiteQueen) * phaseValues[Queen] + pos.pieceCnt(BlackQueen) * phaseValues[Queen], maxGamePhase) / maxGamePhase;

	for (Square s = A1; s < SquareNum; s++)
	{
		Piece p = pos.pieceOn(s);

		if (p != NonePiece)
		{
			if (colorOf(p) == White)
			{
				mgScore += PieceSquareTable[p][0][s];
				egScore += PieceSquareTable[p][1][s];
			}
			else
			{
				mgScore -= PieceSquareTable[p][0][s];
				egScore -= PieceSquareTable[p][1][s];
			}
		}
	}

	eval += mgScore * (1.0f-currentPhase);
	eval += egScore * currentPhase;

	eval += mobilityBonus<Knight, White>(pos) - mobilityBonus<Knight, Black>(pos);
	eval += mobilityBonus<Bishop, White>(pos) - mobilityBonus<Bishop, Black>(pos);
	eval += mobilityBonus<Rook, White>(pos) - mobilityBonus<Rook, Black>(pos);
	eval += mobilityBonus<Queen, White>(pos) - mobilityBonus<Queen, Black>(pos);

	return eval * perspectiveMultiplier(pos.colorToMove());
}

}