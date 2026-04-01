#pragma once

#include <engine/types/piece.h>

namespace Bratwurst::Evaluation
{

constexpr int PieceValue[PieceNum] =
{
	 100,  300,  310,  500,  900, 0,
	-100, -300, -310, -500, -900, 0
};

constexpr int TypeValue[PieceTypeNum] =
{
	100, 300, 310, 500, 900, 0
};

}