#pragma once

#include <engine/types/castling_right.h>
#include <engine/types/zobrist.h>
#include <engine/types/move.h>

namespace Bratwurst
{

// The StateInfo struct stores additional information needed,
// to undo a position successfully, since these members of the
// board representation cannot be recalculated using the move alone
struct StateInfo
{
	// Using raw type over std::bitset<CastlingRightNum>
	// to avoid the abstraction overhead for maximum performance
	CastlingRights castlingRights;
	uint8 halfMoveClock;
	Square enPassantSquare;
	Piece capturedPiece;
	Move prevMove;

	// the following members are stored, 
	// due to being expensive to recompute
	Bitboard pinned;
	Bitboard checkers;
	Zobrist::Key zobristKey;
	uint16 material[ColorNum];
};

}