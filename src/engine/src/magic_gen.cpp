#pragma once

#include <engine/move_gen/magic_gen.h>
#include <engine/move_gen/precomputed.h>
#include <engine/move_gen/attacks.h>

// This File is not included in the build and is only for creating precomputed magic bitboards
// Due to time cost of this algorithm, the 128 precomputed Magic Bitboards are just stored inside an array
namespace Bratwurst::MagicGenerator
{

// this function uses a shift value of 64 - popCnt(relevantBlockerMask)
Bitboard findMagicBitboard(Square s, std::span<const int[2]> directions, int maxTries)
{
	Bitboard blockerMask = dynamicAttacks<true, true>(s, directions);
	int relevantBlockerCnt = popCnt(blockerMask);
	int shift = 64 - relevantBlockerCnt;

	// Cache blockers and corresponding attacks
	size_t attacksCnt = 1ULL << relevantBlockerCnt;
	using LookupEntry = std::pair<Bitboard, Bitboard>;
	std::vector<LookupEntry> lookupTable(attacksCnt);

	for (size_t i = 0; i < attacksCnt; i++)
	{
		Bitboard blockers = 0ULL;
		Bitboard tempMask = blockerMask;
		Bitboard tempIndex = i;

		while (tempMask)
		{
			Square relevantSquare = popLsb(tempMask);
			blockers |= ((tempIndex & 1ULL) << relevantSquare);
			tempIndex >>= 1;
		}

		Bitboard attacks = dynamicAttacks<true>(s, directions, blockers);
		lookupTable[i] = LookupEntry(blockers, attacks);
	}

	Bitboard* attackTable = new Bitboard[attacksCnt];

	Bitboard magic = 0ULL;

	for (int i = 0; i < maxTries; i++)
	{
		magic = randomBitboard() & randomBitboard() & randomBitboard();
		std::memset(attackTable, 0ULL, sizeof(Bitboard) * attacksCnt);
		bool collides = false;

		for (const auto& [occupancy, attacks] : lookupTable)
		{
			size_t index = (occupancy * magic) >> shift;
			Bitboard& slot = attackTable[index];
			
			if (slot != 0ULL && slot != attacks)
			{
				collides = true;
				break;
			}

			slot = attacks;
		}

		if (!collides)
		{
			break;
		}
	}

	delete[] attackTable;
	return magic;
}

}