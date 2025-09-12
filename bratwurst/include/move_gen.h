#pragma once

#include "core.h"

#include "position.h"
#include "types/move_list.h"

namespace Bratwurst
{
	enum class GenType
	{
		All,
		Capture,
		Quiet
	};

	template<GenType>
	void generateMoves(const Position& pos, MoveList& out) noexcept;

}