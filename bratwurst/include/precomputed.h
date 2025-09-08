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
	Bitboard* attacks = nullptr;
	uint8 shift = 64;
	Bitboard mask = 0ULL;
	Bitboard magic = 0ULL;

	inline size_t index(Bitboard blockers) const noexcept
	{
		ASSERT(attacks != nullptr);
		return ((blockers & mask) * magic) >> shift;
	}
};

// magics for rook and bishop
// use 'pieceType - Bishop' for first index
extern Magic magics[2][SquareNum];

// Stores a bitboard of pseudo-attacks (ignoring blockers) for each piece on each square.
// White and Black pawns have different attack patterns, so use WhitePawn and BlackPawn respectively.
extern Bitboard pseudoAttacks[PieceTypeNum + 1][SquareNum];

void init() noexcept;
void cleanup() noexcept;
}