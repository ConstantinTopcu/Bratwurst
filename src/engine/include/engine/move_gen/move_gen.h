#pragma once

#include <engine/core/core.h>

#include <engine/position/position.h>
#include <engine/types/move.h>

namespace Bratwurst
{

enum GenType
{
	All,
	Quiescence
};

template<GenType>
MoveList generateMoves(const Position& pos);

}