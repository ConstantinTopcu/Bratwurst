#pragma once

#include <engine/types/castling_right.h>
#include <engine/types/zobrist.h>
#include <engine/types/move.h>

namespace Bratwurst
{

// The StateInfo struct stores additional information for a given ply,
// which are either too expensive or even impossible to recompute
struct StateInfo
{
	// update incrementally every ply
	Zobrist::Key	zobristKey;
	Zobrist::Key	pawnKey;
	uint16			material[ColorNum];
	int16			egPSQT;
	int16			mgPSQT;
	uint16			phase;

	CastlingRights	castlingRights;
	uint8			halfMoveClock;

	// recomputed every ply
	Square			enPassantSquare;
	Piece			capturedPiece;
	Move			prevMove;
	Bitboard		pinned;
	Bitboard		checkers;
};

}