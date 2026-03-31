#pragma once

#include <engine/core/core.h>
#include <engine/types/square.h>

#include <random>
#ifdef COMPILER_MSVC
#include <intrin.h>
#endif

namespace Bratwurst
{

using Bitboard = uint64;

constexpr Bitboard squareMask(Square s) 
{
	ASSERT(isValid(s));
	return 1ULL << s;
}

constexpr Bitboard rankMask(Rank r) 
{
	ASSERT(isValid(r));
	return 0xFFULL << (r * FileNum);
}

constexpr Bitboard fileMask(File f) 
{
	ASSERT(isValid(f));
	return 0x0101010101010101ULL << f;
}

inline Bitboard randomBitboard() 
{
	static std::random_device rd;
	static std::mt19937_64 gen(rd());
	static std::uniform_int_distribution<Bitboard> distrib;
	return distrib(gen);
}

inline Square lsb(Bitboard mask) 
{
	ASSERT(mask != 0ULL);

#if defined(COMPILER_MSVC)
	unsigned long lsb;
	_BitScanForward64(&lsb, mask);
	return static_cast<Square>(lsb);
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
	int i = _builtin_ctzll(mask);
	return static_cast<Square>(i);
#endif
}

inline Square msb(Bitboard mask) 
{
	ASSERT(mask != 0ULL);

#if defined(COMPILER_MSVC)
	unsigned long msb;
	_BitScanReverse64(&msb, mask);
	return static_cast<Square>(msb);
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
	return static_cast<Square>(H8 - __builtin_clzll(mask));
#endif
}

inline Square popLsb(Bitboard& mask) 
{
	ASSERT(mask != 0ULL);
	const Square square = lsb(mask);
	mask &= mask - 1;
	return square;
}

inline Square popMsb(Bitboard& mask) 
{
	ASSERT(mask != 0ULL);
	const Square square = msb(mask);
	mask &= mask ^ squareMask(square);
	return square;
}

inline int popCnt(Bitboard mask) 
{
#if defined(COMPILER_MSVC)
	return static_cast<int>(__popcnt64(mask));
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
	return static_cast<int>(__builtin_popcountll(mask));
#endif
}

template<Direction d>
constexpr Bitboard shift(Bitboard b)
{
	// if d is positive, shift left, else shift - right
	d > 0 ? b <<= d : b >>= -d;

	return b;
}

inline std::string toString(Bitboard b) 
{
	std::string str;

	for (Rank r = Rank8; isValid(r); r--)
	{
		for (File f = FileA; isValid(f); f++)
		{
			Square s = makeSquare(f, r);
			str += (b >> s) & 1 ? 'X' : '.';
			str += ' ';
		}

		str += '\n';
	}

	return str;
}

}