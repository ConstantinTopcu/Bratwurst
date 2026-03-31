#pragma once

#include <engine/core/core.h>

#include <engine/types/bitboard.h>
#include <engine/types/square.h>
#include <engine/types/piece.h>

#include <engine/move_gen/precomputed.h>

#include <span>

namespace Bratwurst
{

// Computes attacks for a square in given directions.
// Used only for precomputing attack tables (dynamic computation is slow).
// 'sliding' controls whether the piece slides multiple squares.
// 'excludeEdges' stops attacks one square before the board edge to help generate
// the relevant-blockers mask for the Magic struct.
template<bool sliding = false, bool excludeEdges = false>
constexpr Bitboard dynamicAttacks(Square s, const std::span<const int[2]> directions, Bitboard blockers = 0ULL)
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

// Color irrelevant except for pawn attacks
template<PieceType pt, Color c = White>
inline Bitboard attacksBB(Square s, [[maybe_unused]] Bitboard blockers = 0ULL) 
{
	static_assert(isValid(c) && isValid(pt));
	
	ASSERT(isValid(s));

	if constexpr (pt == Pawn && c == White) return Precomputed::pseudoAttacks[WhitePawn][s];
	else if constexpr (pt == Pawn && c == Black) return Precomputed::pseudoAttacks[BlackPawn][s];
	else if constexpr (pt == Knight) return Precomputed::pseudoAttacks[Knight][s];
	else if constexpr (pt == Bishop) return Precomputed::magics[0][s].attacks(blockers);
	else if constexpr (pt == Rook) return Precomputed::magics[1][s].attacks(blockers);
	else if constexpr (pt == Queen) return (attacksBB<Bishop>(s, blockers) | attacksBB<Rook>(s, blockers));
	else if constexpr (pt == King) return Precomputed::pseudoAttacks[King][s];
}

}