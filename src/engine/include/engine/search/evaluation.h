#pragma once

#include <engine/position/position.h>

// Refractor into own namespace Evaluation when evaluation grows larger
namespace Bratwurst::Evaluation
{

template<PieceType pt>
inline int evalMobility(const Position& pos)
{
	int eval = 0;

	const Bitboard occupancy = pos.colorBB(Black) | pos.colorBB(White);

	Bitboard whitePieces = pos.pieceBB(White, pt);
	Bitboard blackPieces = pos.pieceBB(Black, pt);

	while (whitePieces != 0ULL)
	{
		Square s = popLsb(whitePieces);
		Bitboard pseudoAttacks = attacksBB<pt>(s, occupancy) & ~pos.colorBB(White);
		eval += MobilityTable[pt][popCnt(pseudoAttacks)];
	}

	while (blackPieces != 0ULL)
	{
		Square s = popLsb(blackPieces);
		Bitboard pseudoAttacks = attacksBB<pt>(s, occupancy) & ~pos.colorBB(Black);
		eval -= MobilityTable[pt][popCnt(pseudoAttacks)];
	}

	return eval;
}

// evaluates position relative to the color whose turn it is
inline int evaluate(const Position& pos)
{
	Color c = pos.colorToMove();

	const int phase = pos.phase();

	// Evaluate Material
	const int materialEval = pos.material();

	// PSQT bonus
	const int psqtEval = (pos.mgPSQT() * phase + pos.egPSQT() * (MAX_PHASE - phase)) / MAX_PHASE;
	
	// mobility bonus
	const int mobilityEval =
		evalMobility<Knight>(pos) +
		evalMobility<Bishop>(pos) +
		evalMobility<Rook>(pos) +
		evalMobility<Queen>(pos);

	int eval = psqtEval + materialEval + mobilityEval;

	return (c == White) ? eval : -eval;
}

}