#pragma once

#include <engine/core/core.h>

#include <engine/types/square.h>
#include <engine/types/piece.h>
#include <engine/types/castling_right.h>
#include <engine/types/static_vector.h>

#include <iostream>

namespace Bratwurst
{

class Position;

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

	constexpr Move(Square src = A1, Square dst = A1, Flag flag = Flag::None) 
		: data(src | (dst << 6) | (flag << 12))	{ }
	static constexpr Move Null() { return Move(A1, A1, Flag::None); }

	constexpr Square src() const  { return static_cast<Square>(data & 0b111111); }
	constexpr Square dst() const { return static_cast<Square>((data >> 6) & 0b111111); }
	constexpr Flag flag() const  { return static_cast<Flag>((data >> 12) & 0b1111); }

	constexpr bool special() const  { return flag() != Flag::None; }
	constexpr bool enPassant() const  { return flag() == Flag::EnPassant; }
	constexpr bool castling() const  { return flag() & Flag::CastlingMask; }
	constexpr bool promotion() const  { return flag() & Flag::PromotionMask; }
	
	constexpr CastlingSide castlingSide() const;
	constexpr PieceType promotionType() const;

	std::string toString() const;
	static Move fromString(const std::string& str, const Position& pos);

	constexpr bool operator==(const Move& other) const
	{
		return data == other.data;
	}

	constexpr bool operator!=(const Move& other) const
	{
		return data != other.data;
	}

private:
	// The move is packed into 16 bits with the following layout:
	// [ 0..5 ]  (6 bits)  Source square       (A1 - H8)
	// [ 6..11]  (6 bits)  Destination square  (A1 - H8)
	// [12..15]  (4 bits)  Flag                (value of Move::Flag enum)
	uint16 data;
};

constexpr CastlingSide Move::castlingSide() const
{
	ASSERT(castling());
	return static_cast<CastlingSide>(flag() & 1);
}

constexpr PieceType Move::promotionType() const
{ 
	ASSERT(promotion());
	return static_cast<PieceType>((flag() ^ Flag::PromotionMask) + 1); 
}

constexpr size_t MaxMoves = 218;
using MoveList = StaticVector<Move, MaxMoves>;
}