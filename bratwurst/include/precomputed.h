#pragma once

#include "core.h"

#include "types/bitboard.h"
#include "types/square.h"
#include "types/piece.h"

namespace Bratwurst::Precomputed
{

// Stores precomputed attacks and hashing values for one given square.
// Used to speed up move/attack generation
struct Magic
{
	Bitboard* attacksTable = nullptr;
	uint8 shift = 64;
	Bitboard mask = 0ULL;
	Bitboard magic = 0ULL;

	inline size_t index(Bitboard blockers) const 
	{
		ASSERT(attacksTable != nullptr);
		return ((blockers & mask) * magic) >> shift;
	}

	inline Bitboard attacks(Bitboard blockers) const 
	{
		ASSERT(attacksTable != nullptr);
		size_t i = index(blockers);
		return attacksTable[i];
	}

};

// magics for rook and bishop
// use 'pieceType - Bishop' for first index
extern Magic magics[2][SquareNum];

// Stores a bitboard of pseudo-attacks (ignoring blockers) for each piece on each square.
// White and Black pawns have different attack patterns, so use WhitePawn and BlackPawn respectively.
extern Bitboard pseudoAttacks[PieceTypeNum + 1][SquareNum];

// [0][s1][s2]: bitboard of continious line that goes throught the squares s1 and s2.
// [1][s1][s2]: bitboard of line, thats in between the squares s1 and s2, where s2 is also included.
// If s1 and s2 do not align, the bitboard is set to 0ULL.
extern Bitboard lineBBs[2][SquareNum][SquareNum];

void init() ;
void cleanup() ;
}