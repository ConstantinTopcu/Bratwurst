#pragma once

#include <engine/core/core.h>

#include <engine/position/position.h>
#include <engine/types/move.h>

namespace Bratwurst
{

enum GenType
{
	Captures = 1 << 0,
	Quiets = 1 << 1,
	Promotions = 1 << 2,
	All = Captures | Quiets | Promotions,
};

template<GenType>
MoveList generateMoves(const Position& pos);

}