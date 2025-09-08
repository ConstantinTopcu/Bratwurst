#include "precomputed.h"

#include <atomic>
#include <span>

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

// Computes attacks for a square in given directions.
// Used only for precomputing attack tables (dynamic computation is slow).
// 'sliding' controls whether the piece slides multiple squares.
// 'excludeEdges' stops attacks one square before the board edge to help generate
// the relevant-blockers mask for the Magic struct.
template<bool sliding = false, bool excludeEdges = false>
constexpr Bitboard dynamicAttacks(Square s, const std::span<const int[2]>& directions, Bitboard blockers = 0ULL) noexcept
{
	Bitboard attacks = 0ULL;

	for (auto [dx, dy] : directions)
	{
		File f = fileOf(s) + dx;
		Rank r = rankOf(s) + dy;

		// stop one iteration early if excludeEdges is enabled
		while (isValid(r + dy * excludeEdges) && isValid(f + dx * excludeEdges))
		{
			Square dst = makeSquare(f, r);
			Bitboard mask = squareMask(dst);
			attacks |= mask;

			if constexpr (!sliding) break;

			if (blockers & mask) break;

			f += dx;
			r += dy;
		}
	}

	return attacks;
}

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