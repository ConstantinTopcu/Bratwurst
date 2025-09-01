#pragma once

#include "core.h"
#include "enum_ops.h"

namespace Bratwurst
{

	// Currently Piece has a stride of 6, meaning extractions cannot be made using a bitmask.
	// If this becomes a hotpath bottleneck, the stride might be changed to 8, to enable faster extractions
	enum Piece : uint8
	{
		WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
		BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing,
		NonePiece, PieceNum = 12
	};

	enum PieceType : uint8
	{
		Pawn, Knight, Bishop, Rook, Queen, King,
		NonePieceType, PieceTypeNum = 6
	};

	enum Color : uint8
	{
		White, Black,
		NoneColor, ColorNum = 2
	};

	// Enable Incr/Decr operators
	ENABLE_ENUM_ARITHMETIC(Piece);
	ENABLE_ENUM_ARITHMETIC(PieceType);
	ENABLE_ENUM_ARITHMETIC(Color);

	// Validation functions
	constexpr bool isValid(Piece p) noexcept
	{
		return p < NonePiece;
	}

	constexpr bool isValid(PieceType pt) noexcept
	{
		return pt < NonePieceType;
	}

	constexpr bool isValid(Color c) noexcept
	{
		return c < NoneColor;
	}

	constexpr PieceType pieceTypeOf(Piece p) noexcept
	{
		ASSERT(isValid(p));
		return (static_cast<PieceType>(p % PieceTypeNum));
	}

	constexpr Color colorOf(Piece p) noexcept
	{
		ASSERT(isValid(p));
		return static_cast<Color>(p >= BlackPawn);
	}

	constexpr Piece makePiece(Color c, PieceType pt) noexcept
	{
		ASSERT(isValid(c) && isValid(pt));
		return static_cast<Piece>(c * PieceTypeNum + pt);
	}
}