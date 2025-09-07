#include "precomputed.h"

#include <atomic>

namespace Bratwurst
{

// precomputed tables definition
extern Magic rookMagics[2][SquareNum];
extern Bitboard pseudoAttacks[PieceTypeNum + 1][SquareNum];
static std::atomic_bool initialized = false;


void init() noexcept
{
	if (initialized) return;

	// Initialize everything

	initialized = true;
}

void cleanup() noexcept
{
	if (!initialized) return;

	// cleanup heap allocated memory

	initialized = false;
}

}