#pragma once

#include "core.h"

#include "types/bitboard.h"
#include "types/square.h"
#include "types/piece.h"

#include <stack>
#include <expected>
#include <string_view>
#include <string>

namespace Bratwurst
{

	enum CastlingRight
	{
		WhiteOO, WhiteOOO,
		BlackOO, BlackOOO,
		CastlingRightNum = 4
	};

	// The StateInfo struct stores additional information needed,
	// to undo a position successfully, since these members of the
	// board representation cannot be recalculated using the move alone
	struct StateInfo
	{
		// Using raw type over std::bitset<CastlingRightNum>
		// to avoid the abstraction overhead for maximum performance
		uint8 castlingRights;
		uint8 halfMoveClock;
		Square enPassantSquare;
	};

	class Position
	{

	public:
		static constexpr std::string_view StartPosFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

		enum class FenError
		{
			InvalidFormat,
			InvalidPiecePlacement,
			InvalidColorToMove,
			InvalidCastlingRights,
			InvalidEnPassantSquare,
			InvalidHalfmoveClock,
			InvalidFullMoveCounter
		};

	public:
		Position() noexcept;
		~Position() noexcept;

		void clear() noexcept;
		static std::expected<Position, FenError> fromFEN(const std::string& fen = std::string(StartPosFEN)) noexcept;

	private:
		// Use both Piece array and bitboards for pieceTypes and colors, since both have their own advantages:
		// - The Piece array enables the fast retrieval of a piece on a given square.
		// - The Bitboards allow for fast bulk operations, like applying masks or counting pieces.
		Piece m_pieces[SquareNum];
		Bitboard m_typeBBs[PieceTypeNum];
		Bitboard m_colorBBs[ColorNum];

		Color m_colorToMove;
		uint16 m_fullMoveCounter;
		std::stack<StateInfo> m_stateHistory;
	};

}