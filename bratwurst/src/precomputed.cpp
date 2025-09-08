#include "precomputed.h"
#include "attacks.h"

#include <atomic>

namespace Bratwurst::Precomputed
{

// precomputed tables definition
Magic magics[2][SquareNum];
Bitboard pseudoAttacks[PieceTypeNum + 1][SquareNum];
static std::atomic_bool initialized = false;

// piece offsets
constexpr int whitePawnOffsets[2][2] = { {-1, 1}, {1, 1} };
constexpr int blackPawnOffsets[2][2] = { {-1, -1}, {1, -1} };
constexpr int knightOffsets[8][2] = { {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2} };
constexpr int kingOffsets[8][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1, 0}, {0, 1}, {-1, 0}, {0, -1} };

// piece directions
constexpr int bishopDirections[4][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
constexpr int rookDirections[4][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
constexpr int queenDirections[8][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };

void init() noexcept
{
	if (initialized) return;

	for (Square s = A1; s < SquareNum; s++)
	{
		pseudoAttacks[WhitePawn][s] = dynamicAttacks(s, whitePawnOffsets);
		pseudoAttacks[BlackPawn][s] = dynamicAttacks(s, blackPawnOffsets);
		pseudoAttacks[Knight][s] = dynamicAttacks(s, knightOffsets);
		pseudoAttacks[Bishop][s] = dynamicAttacks<true>(s, bishopDirections);
		pseudoAttacks[Rook][s] = dynamicAttacks<true>(s, rookDirections);
		pseudoAttacks[Queen][s] = pseudoAttacks[Bishop][s] | pseudoAttacks[Rook][s];
		pseudoAttacks[King][s] = dynamicAttacks(s, kingOffsets);
	}

	initialized = true;

	std::cout << "Successfully initialized precomputed data!" << std::endl;
}

void cleanup() noexcept
{
	if (!initialized) return;

	// cleanup heap allocated memory

	initialized = false;

	std::cout << "Successfully cleaned up precomputed data!" << std::endl;
}

}