#pragma once

#include "core.h"

#include "types/square.h"
#include "types/piece.h"
#include "types/castling_right.h"

#include <iostream>

namespace Bratwurst
{

struct Move
{
	enum Flag : uint8
	{
		None = 0b0000,

		EnPassant = 0b0001,

		CastlingMask = 0b1000,
		CastlingOO = 0b1000,
		CastlingOOO = 0b1001,

		PromotionMask = 0b0100,
		KnightPromotion = 0b0100,
		BishopPromotion = 0b0101,
		RookPromotion = 0b0110,
		QueenPromotion = 0b0111
	};

	constexpr Move(Square src = A1, Square dst = A1, Flag flag = Flag::None) noexcept
		: data(src | (dst << 6) | (flag << 12))	{ }
	
	// Helpers to encode move easly
	constexpr Square src() const noexcept { return static_cast<Square>(data & 0b111111); }
	constexpr Square dst() const noexcept{ return static_cast<Square>((data >> 6) & 0b111111); }
	constexpr Flag flag() const noexcept { return static_cast<Flag>((data >> 12) & 0b1111); }

	constexpr bool special() const noexcept { return flag() != Flag::None; }
	constexpr bool enPassant() const noexcept { return flag() == Flag::EnPassant; }
	
	constexpr bool castling() const noexcept { return flag() & Flag::CastlingMask; }
	constexpr CastlingSide castlingSide() const noexcept { return static_cast<CastlingSide>(flag() & 1); }
	
	constexpr bool promotion() const noexcept { return flag() & Flag::PromotionMask; }

	constexpr PieceType promotionType() const noexcept
	{
		ASSERT(promotion());
		return static_cast<PieceType>((flag() ^ Flag::PromotionMask) + 1);
	}

	inline void print() const noexcept
	{
		std::cout << toString() << "\n";
	}

	inline std::string toString() const noexcept
	{
		return std::string("[" + squareToString(src()) + "][" + squareToString(dst()) + "][" + std::to_string(static_cast<int>(flag())) + "]");
	}

	// The move is packed into 16 bits with the following layout:
	// [ 0..5 ]  (6 bits)  Source square       (Square::A1 - Square::H8)
	// [ 6..11]  (6 bits)  Destination square  (Square::A1 - Square::H8)
	// [12..15]  (4 bits)  Flag                (value of Move::Flag enum)
	uint16 data;

};
}