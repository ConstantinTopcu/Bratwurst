#pragma once

#include <engine/position/position.h>

// Refractor into own namespace Evaluation when evaluation grows larger
namespace Bratwurst::Evaluation
{

inline int colorMultiplier(Color c)
{
	return (c == White) ? 1 : -1;
}

inline int evalPawnShield(const Position& pos, Color c)
{
	const Square kingSq = pos.kingSquare(c);
	const Bitboard ourPawns = pos.pieceBB(c, Pawn);

	// The 3 squares directly in front of the king
	Bitboard shield = Precomputed::pawnShieldMask[c][kingSq];
	int pawnsPresent = popCnt(shield & ourPawns); 

	return pawnsPresent * 20;
}

// evalutate kingsafety based on how many pieces attack squares next to the king
inline int evalAttackingKingZone(const Position& pos, Color c)
{
	int eval = 0;

	const Square kingSq = pos.kingSquare(c);
	const Bitboard occupancy = pos.colorBB(Black) | pos.colorBB(White);
	const Bitboard kingZone = Precomputed::pseudoAttacks[King][kingSq];
	const Color enemy = ~c;

	int attackingPieceCnt = 0; 
	int valueOfAttacks = 0;

	auto addAttacks = [&]<PieceType pt>()
	{
		Bitboard pieces = pos.pieceBB(enemy, pt);

		while (pieces)
		{
			Square sq = popLsb(pieces);
			Bitboard attacks = attacksBB<pt>(sq, occupancy) & kingZone;

			if (attacks)
			{
				attackingPieceCnt++;
				valueOfAttacks += popCnt(attacks) * AttackerWeight[pt];
			}
		}
	};

	addAttacks.operator()<Knight>();
	addAttacks.operator()<Bishop>();
	addAttacks.operator()<Rook>();
	addAttacks.operator()<Queen>();

	int index = std::min(attackingPieceCnt, 9);
	return -valueOfAttacks * AttackWeight[index] / 100;
}

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

inline int evalDoubledPawns(const Position& pos, Color c)
{
	int eval = 0;

	Bitboard pawns = pos.pieceBB(c, Pawn);

	for (File f = FileA; f <= FileH; f++)
	{
		Bitboard mask = fileMask(f);
		Bitboard pawnsOnFile = pawns & mask;
		int doubledCnt = std::max(0, popCnt(pawnsOnFile) - 1); // only penalize if there are 2 or more pawns on the file
		eval -= doubledCnt * 20;
	}

	return eval;
}

inline int evalIsolatedPawns(const Position& pos, Color c)
{
	int eval = 0;
	Bitboard pawns = pos.pieceBB(c, Pawn);
	bool hasLeftNeighbor = false;

	for (File f = FileA; f <= FileH; f++)
	{
		Bitboard mask = fileMask(f);
		Bitboard pawnsOnFile = pawns & mask;

		if (!pawnsOnFile)
		{
			hasLeftNeighbor = false;
			continue;
		}

		bool hasRightNeighbor = (f < FileH) && (pawns & fileMask(File(f + 1)));
		eval -= 20 * (!hasLeftNeighbor * !hasRightNeighbor) * popCnt(pawnsOnFile);
		hasLeftNeighbor = true;
	}

	return eval;
}


inline int evalPassedPawns(const Position& pos, Color c)
{
	Bitboard pawns = pos.pieceBB(c, Pawn);
	Bitboard enemyPawns = pos.pieceBB(~c, Pawn);
	int eval = 0;

	while (pawns)
	{
		Square sq = popLsb(pawns);
		bool passedPawn = (passedPawnMasks[c][sq] & enemyPawns) == 0ULL;
		eval += 40 * passedPawn;
	}

	return eval;
}

// evaluates position relative to the color whose turn it is
inline int evaluate(const Position& pos)
{
	Color c = pos.colorToMove();
	const int phase = pos.phase();
	const int egPhase = MAX_PHASE - phase;
	const int materialEval = pos.material();

	// PSQT bonus
	const int psqtEval = (pos.mgPSQT() * phase + pos.egPSQT() * egPhase) / MAX_PHASE;
	
	// mobility bonus
	const int mobilityEval =
		evalMobility<Knight>(pos) +
		evalMobility<Bishop>(pos) +
		evalMobility<Rook>(pos) +
		evalMobility<Queen>(pos);

	// king safety bonus
	int attackingKingZoneEval = evalAttackingKingZone(pos, White) - evalAttackingKingZone(pos, Black);
	int pawnShieldEval = evalPawnShield(pos, White) - evalPawnShield(pos, Black);
	int kingSafetyEval = ((attackingKingZoneEval + pawnShieldEval) * phase) / MAX_PHASE;

	// pawn structure bonus
	//int doubledPawnEval = evalDoubledPawns(pos, White) - evalDoubledPawns(pos, Black);
	//int isolatedPawnEval = evalIsolatedPawns(pos, White) - evalIsolatedPawns(pos, Black);
	//int passedPawnEval = evalPassedPawns(pos, White) - evalPassedPawns(pos, Black);
	//int pawnStructureEval = isolatedPawnEval;
	
	//pawnStructureEval = (pawnStructureEval / 2) + pawnStructureEval * egPhase / (2 * MAX_PHASE); // scale pawn structure eval so it has more impact in endgame

	int eval = psqtEval + materialEval + mobilityEval + kingSafetyEval; 

	return (c == White) ? eval : -eval;
}

}