#pragma once

#include "core.h"
#include "piece.h"
#include "square.h"
#include "bitboard.h"

namespace Bratwurst
{

	enum CastlingSide
	{
		KingSide, QueenSide,
		CastlingSideNum = 2
	};

	enum CastlingRight
	{
		WhiteOO, WhiteOOO,
		BlackOO, BlackOOO,
		CastlingRightNum = 4
	};

	constexpr Square rookCastlingSources[CastlingRightNum]		= { H1, A1, H8, A8 };
	constexpr Square rookCastlingDestinations[CastlingRightNum] = { F1, D1, F8, D8 };
	constexpr Square kingCastlingSources[CastlingRightNum]		= { E1, E1, E8, E8 };
	constexpr Square kingCastlingDestinations[CastlingRightNum] = { G1, C1, G8, C8 };

	// CastlingPath is a mask for the squares inbetween the rook and king 
	constexpr Bitboard castlingPathMask[CastlingRightNum] = { 0x0000000000000060ULL, 0x000000000000000eULL, 0x6000000000000000ULL, 0x0e00000000000000ULL };

	constexpr CastlingRight makeCastlingRight(Color c, CastlingSide side) noexcept
	{
		ASSERT(isValid(c));
		return static_cast<CastlingRight>(c * 2 + side);
	}

}