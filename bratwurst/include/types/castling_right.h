#pragma once

#include "core.h"
#include "piece.h"
#include "square.h"
#include "bitboard.h"

#include <type_traits>

namespace Bratwurst
{

enum CastlingSide
{
	KingSide,
	QueenSide,
	CastlingSideNum = 2
};

enum CastlingRight
{
	WhiteOO,
	WhiteOOO,
	BlackOO,
	BlackOOO,

	CastlingRightNone,
	CastlingRightNum = 4
};

constexpr bool isValid(CastlingRight right)
{
	return right < CastlingRightNone;
}
constexpr CastlingRight makeCastlingRight(Color c, CastlingSide side)
{
	ASSERT(isValid(c));
	return static_cast<CastlingRight>(c * CastlingSideNum + side);
}

constexpr CastlingRight castlingRightByRookSrc(Square s)
{
	switch (s)
	{
	case A1: return WhiteOOO;
	case H1: return WhiteOO;
	case A8: return BlackOOO;
	case H8: return BlackOO;
	default: return CastlingRightNone;
	}
}

class CastlingRights
{
public:
	constexpr CastlingRights() : data(0) {}

	//template<typename... Rights>
	//constexpr CastlingRights(Rights... rights) : data(((1 << static_cast<int>(rights)) | ...))
	//{ 
	//	//static_assert((std::is_same_v<Rights, CastlingRight>) && ...); 
	//}

	constexpr bool canCastle(CastlingRight right) const { return data & (1 << right); }
	constexpr void allowCastling(CastlingRight right) {	data |= (1 << right); }
	constexpr void disallowCastling(CastlingRight right) { data &= ~(1 << right); }
	constexpr bool canCastle(Color c, CastlingSide side) const { return data & (1 << makeCastlingRight(c, side)); }
	constexpr void allowCastling(Color c, CastlingSide side) { data |= (1 << makeCastlingRight(c, side)); }
	constexpr void disallowCastling(Color c, CastlingSide side)	{ data &= ~(1 << makeCastlingRight(c, side)); }

	uint8 data;
};

constexpr Square CastlingRookSrc[CastlingRightNum] = { H1, A1, H8, A8 };
constexpr Square CastlingRookDst[CastlingRightNum] = { F1, D1, F8, D8 };
constexpr Square CastlingKingSrc[CastlingRightNum] = { E1, E1, E8, E8 };
constexpr Square CastlingKingDst[CastlingRightNum] = { G1, C1, G8, C8 };

// Mask for the squares between the rook and king that must be empty
constexpr Bitboard CastlingPathMask[CastlingRightNum] = 
{
	0x0000000000000060ULL,  // WhiteOO:  F1, G1
	0x000000000000000eULL,  // WhiteOOO: B1, C1, D1
	0x6000000000000000ULL,  // BlackOO:  F8, G8
	0x0e00000000000000ULL   // BlackOOO: B8, C8, D8
};

} // namespace Bratwurst