#pragma once

#include "core.h"
#include "enum_ops.h"

namespace Bratwurst
{

	enum Square : uint8
	{
		A1, B1, C1, D1, E1, F1, G1, H1,
		A2, B2, C2, D2, E2, F2, G2, H2,
		A3, B3, C3, D3, E3, F3, G3, H3,
		A4, B4, C4, D4, E4, F4, G4, H4,
		A5, B5, C5, D5, E5, F5, G5, H5,
		A6, B6, C6, D6, E6, F6, G6, H6,
		A7, B7, C7, D7, E7, F7, G7, H7,
		A8, B8, C8, D8, E8, F8, G8, H8,

		NoneSquare = 64, SquareNum = 64
	};

	enum File : uint8
	{
		FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH,
		NoneFile = 8, FileNum = 8
	};

	enum Rank : uint8
	{
		Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8,
		NoneRank = 8, RankNum = 8
	};

	enum Direction : int8
	{
		Up = 8,
		Down = -8,
		Right = 1,
		Left = -1,

		UpRight = Up + Right,
		UpLeft = Up + Left,
		DownRight = Down + Right,
		DownLeft = Down + Left
	};

	// Enable Incr/Decr operators
	ENABLE_ENUM_ARITHMETIC(Square);
	ENABLE_ENUM_ARITHMETIC(File);
	ENABLE_ENUM_ARITHMETIC(Rank);

	// Operator overloading to allow change of Square via applying a Direction using the + operator
	constexpr Direction operator*(int factor, Direction dir) noexcept { return static_cast<Direction>(factor * static_cast<int>(dir)); }
	inline Direction& operator*=(Direction& d, int factor) noexcept	{ return d = factor * d; }
	constexpr Square operator+(Square s, Direction d) noexcept { return static_cast<Square>(static_cast<int>(s) + static_cast<int>(d)); }
	inline Square& operator+=(Square& s, Direction d) noexcept { return s = s + d; }

	// Operator overload to allow addition for Rank and File enums
	constexpr File operator+(File f, int offset) { return static_cast<File>(static_cast<int>(f) + offset); }
	inline File& operator+=(File& f, int offset) { return f = f + offset; }
	constexpr Rank operator+(Rank r, int offset) { return static_cast<Rank>(static_cast<int>(r) + offset); }
	inline Rank& operator+=(Rank& r, int offset) { return r = r + offset; }


	// Validation functions
	constexpr bool isValid(Square s) noexcept
	{
		return s < NoneSquare;
	}

	constexpr bool isValid(File f) noexcept
	{
		return f < NoneFile;
	}

	constexpr bool isValid(Rank r) noexcept
	{
		return r < NoneRank;
	}

	// Conversion functions
	constexpr Rank rankOf(Square s) noexcept
	{
		ASSERT(isValid(s));
		return static_cast<Rank>(static_cast<uint8>(s) >> 3);
	}

	constexpr File fileOf(Square s) noexcept
	{
		ASSERT(isValid(s));
		return static_cast<File>(static_cast<uint8>(s) & 0b111);
	}

	constexpr Square makeSquare(File f, Rank r) noexcept
	{
		ASSERT(isValid(f) && isValid(r));
		return static_cast<Square>(f + r * FileNum);
	}

}