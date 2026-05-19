#pragma once

#include <engine/position/position.h>

// Refractor into own namespace Evaluation when evaluation grows larger
namespace Bratwurst::Evaluation
{

struct PawnScore
{
	int mg = 0;
	int eg = 0;
	PawnScore operator+(const PawnScore& s) const { return { mg + s.mg, eg + s.eg }; }
	PawnScore operator-(const PawnScore& s) const { return { mg - s.mg, eg - s.eg }; }
	PawnScore& operator+=(const PawnScore& s) { mg += s.mg; eg += s.eg; return *this; }
};

struct PawnEntry
{
	Zobrist::Key key;
	PawnScore score;
};


// Pawn hash table
inline constexpr int PAWN_TABLE_SIZE = 1 << 16; // 65536 entries
inline PawnEntry pawnTable[PAWN_TABLE_SIZE];

// some constants
inline constexpr int MobilityTable[PieceTypeNum][28] =
{
	{}, // Pawn
	{ -24,-18,-12,-6,0,4,8,12,16 }, // knight
	{  -20,-14,-10,-6,-2,2,6,10,14,18,22,26,30,34 }, // bishop
	{ -12,-8,-5,-2,0,2,4,6,8,10,12,14,16,18,20 }, // rook
	{ -10,-8,-6,-4,-2,0,1,2,3,4,5,6,7,8,9,10,11,12, 13,14,15,16,17,18,19,20,21,22 }, // queen
	{} // King
};

inline constexpr int AttackerWeight[PieceTypeNum] = { 0, 20, 20, 40, 80, 0 };
inline constexpr int AttackWeight[10] = { 0, 50, 75, 88, 94, 97, 99, 99, 99, 99 };

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

			if (attacks == 0UL) continue;

			attackingPieceCnt++;
			valueOfAttacks += popCnt(attacks) * AttackerWeight[pt];
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

inline PawnScore evalDoubledPawns(const Position& pos, Color c)
{
	PawnScore score = { 0, 0 };

	Bitboard pawns = pos.pieceBB(c, Pawn);

	for (File f = FileA; f <= FileH; f++)
	{
		Bitboard mask = fileMask(f);
		Bitboard pawnsOnFile = pawns & mask;
		int doubledCnt = std::max(0, popCnt(pawnsOnFile) - 1); // only penalize if there are 2 or more pawns on the file
		score.mg += doubledCnt * -15;
		score.eg += doubledCnt * -60;
	}

	return score;
}

inline PawnScore evalIsolatedPawns(const Position& pos, Color c)
{
	PawnScore score = { 0, 0 };
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
		score.mg += -22 * (!hasLeftNeighbor && !hasRightNeighbor) * popCnt(pawnsOnFile);
		score.eg += -30 * (!hasLeftNeighbor && !hasRightNeighbor) * popCnt(pawnsOnFile);
		hasLeftNeighbor = true;
	}

	return score;
}


inline PawnScore evalPassedPawns(const Position& pos, Color c)
{
	constexpr int PassedMG[8] = { 0,  0,  7, 22, 37, 60,  90, 0 };
	constexpr int PassedEG[8] = { 0,  0, 22, 45, 82, 127, 195, 0 };

	Bitboard pawns = pos.pieceBB(c, Pawn);
	Bitboard enemyPawns = pos.pieceBB(~c, Pawn);
	PawnScore score = { 0, 0 };

	while (pawns)
	{
		Square sq = popLsb(pawns);
		if ((passedPawnMasks[c][sq] & enemyPawns) == 0ULL)
		{
			Rank r = (c == White) ? rankOf(sq) : (Rank)(Rank7 - rankOf(sq));
			score.mg += PassedMG[r];
			score.eg += PassedEG[r];
		}
	}
	return score;
}

inline int evalPawnStructure(const Position& pos)
{
	Zobrist::Key pawnKey = pos.pawnKey();
	PawnEntry& entry = pawnTable[pawnKey & (PAWN_TABLE_SIZE-1)];

	int mgPhase = pos.phase();
	int egPhase = MaxPhase - pos.phase();

	if (entry.key == pawnKey)
	{
		return (entry.score.mg * mgPhase + entry.score.eg * egPhase) / MaxPhase;
	}

	//Pawn structure
	PawnScore doubledPawnEval = evalDoubledPawns(pos, White) - evalDoubledPawns(pos, Black); // no scaling needed
	PawnScore isolatedPawnEval = evalIsolatedPawns(pos, White) - evalIsolatedPawns(pos, Black); // 60% midgame and 100% endgame
	PawnScore passedPawnEval = evalPassedPawns(pos, White) - evalPassedPawns(pos, Black); // 40% midgame and 100% endgame

	PawnScore totalPawnEval = doubledPawnEval + isolatedPawnEval + passedPawnEval;

	entry = { pawnKey, totalPawnEval};

	return (totalPawnEval.mg * mgPhase + totalPawnEval.eg * egPhase) / MaxPhase;
}

inline int bishopPairBonus(const Position& pos, Color c)
{
	Bitboard bishops = pos.pieceBB(c, Bishop);
	return (popCnt(bishops) >= 2) ? 30 : 0;
}

// evaluates for both colors and return difference
inline int evalOpenRookFiles(const Position& pos, Color c)
{
	Bitboard rooks = pos.pieceBB(c, Rook);
	Bitboard friendlyPawns = pos.pieceBB(c, Pawn);
	Bitboard enemyPawns = pos.pieceBB(~c, Pawn);
	int eval = 0;

	while (rooks)
	{
		Square sq = popLsb(rooks);
		File f = fileOf(sq);
		Bitboard fileBB = fileMask(f);

		bool noFriendlyPawns = (fileBB & friendlyPawns) == 0ULL;
		bool noEnemyPawns = (fileBB & enemyPawns) == 0ULL;

		bool openFile = noFriendlyPawns && noEnemyPawns;
		bool semiOpenFile = noFriendlyPawns && !noEnemyPawns;

		if (openFile) eval += 30; // fully open file
		else if (semiOpenFile) eval += 15; // semi-open file (only our pawns gone)
	}

	return eval;
}

// evaluates position relative to the color whose turn it is
inline int evaluate(const Position& pos)
{
	Color c = pos.colorToMove();
	const int phase = pos.phase();
	const int egPhase = MaxPhase - phase;
	const int materialEval = pos.material();

	// PSQT bonus
	const int psqtEval = (pos.mgPSQT() * phase + pos.egPSQT() * egPhase) / MaxPhase;

	// mobility bonus
	const int mobilityEval =
		evalMobility<Knight>(pos) +
		evalMobility<Bishop>(pos) +
		evalMobility<Rook>(pos) +
		evalMobility<Queen>(pos);

	// king safety (midgame only)
	int attackingKingZoneEval = evalAttackingKingZone(pos, White) - evalAttackingKingZone(pos, Black);
	int pawnShieldEval = evalPawnShield(pos, White) - evalPawnShield(pos, Black);
	int kingSafetyEval = ((attackingKingZoneEval + pawnShieldEval) * phase) / MaxPhase;

	int bishopPairEval = bishopPairBonus(pos, White) - bishopPairBonus(pos, Black);
	int rookFileEval = evalOpenRookFiles(pos, White) - evalOpenRookFiles(pos, Black);

	int eval = psqtEval + materialEval + mobilityEval + kingSafetyEval + rookFileEval + bishopPairEval;

	return (c == White) ? eval : -eval;
}

}