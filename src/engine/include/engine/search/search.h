#pragma once

#include <engine/position/position.h>

namespace Bratwurst::Search
{

struct SearchResult
{
	int evaluation;
	Move bestMove;
};

SearchResult search(Position& position, int depth = 4);

}