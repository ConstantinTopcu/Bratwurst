#pragma once

#include <engine/types/bitboard.h>
#include <engine/types/square.h>

#include <span>

// This File is not included in the build and is only for creating precomputed magic bitboards
namespace Bratwurst::MagicGenerator
{

// this function uses a shift value of 64 - popCnt(relevantBlockerMask)
Bitboard findMagicBitboard(Square s, std::span<const int[2]> directions, int maxTries = 10000);

}