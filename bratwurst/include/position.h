#pragma once

#include "core.h"

#include "types/bitboard.h"
#include "types/square.h"
#include "types/piece.h"
#include "types/move.h"

#include <stack>
#include <expected>
#include <string_view>
#include <type_traits>
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
		Piece capturedPiece;
		Move prevMove;
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
		inline Position() noexcept = default;
		inline ~Position() noexcept = default;

		static std::expected<Position, FenError> fromFEN(const std::string& fen = std::string(StartPosFEN)) noexcept;

		// Getters for pieces
		template<typename... PieceTypes>
		inline Bitboard typeBB(PieceTypes... types) const noexcept;
		inline Bitboard colorBB(Color c) const noexcept;
		inline Bitboard pieceBB(Color c, PieceType type) const noexcept;
		inline Bitboard occupancyBB() const noexcept;
		inline Piece pieceOn(Square s) const noexcept;

		inline Color colorToMove() const noexcept;
		inline uint16 fullMovecounter() const noexcept;

		// getters related to StateInfo
		inline StateInfo& stateInfo() noexcept;
		inline const StateInfo& stateInfo() const noexcept;
		inline bool hasCastlingRight(CastlingRight rights) const noexcept;
		inline uint8 halfmoveClock() const noexcept;
		inline Square enPassantMove() const noexcept;

		// returns the piece that was captured last move
		// returns NonePiece in case there was no capture
		inline Piece capturedPiece() const noexcept;
		inline Move prevMove() const noexcept;

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

	private:
		void clear() noexcept;
	};

	template<typename... PieceTypes>
	inline Bitboard Position::typeBB(PieceTypes... types) const noexcept
	{
		static_assert((std::is_same_v<PieceTypes, PieceType> && ...), "All arguments must be of type PieceType");
		(ASSERT(isValid(types)), ...);
		return (m_typeBBs[types] | ...);
	}

	inline Bitboard Position::colorBB(Color c) const noexcept
	{
		ASSERT(isValid(c));
		return m_colorBBs[c];
	}

	inline Bitboard Position::pieceBB(Color c, PieceType type) const noexcept
	{
		ASSERT(isValid(c) && isValid(type));
		return m_colorBBs[c] & m_typeBBs[type];
	}

	inline Bitboard Position::occupancyBB() const noexcept
	{
		return m_colorBBs[White] | m_colorBBs[Black];
	}

	inline Piece Position::pieceOn(Square s) const noexcept
	{
		ASSERT(isValid(s));
		return m_pieces[s];
	}

	inline Color Position::colorToMove() const noexcept
	{
		return m_colorToMove;
	}

	inline uint16 Position::fullMovecounter() const noexcept
	{
		return m_fullMoveCounter;
	}

	// getters related to StateInfo
	inline StateInfo& Position::stateInfo() noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return m_stateHistory.top();
	}

	inline const StateInfo& Position::stateInfo() const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return m_stateHistory.top();
	}

	inline bool Position::hasCastlingRight(CastlingRight right) const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return (stateInfo().castlingRights >> right) & 1;
	}

	inline uint8 Position::halfmoveClock() const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return stateInfo().halfMoveClock;
	}

	inline Square Position::enPassantMove() const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return stateInfo().enPassantSquare;
	}

	// returns the piece that was captured last move
	// returns NonePiece in case there was no capture
	inline Piece Position::capturedPiece() const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return stateInfo().capturedPiece;
	}

	inline Move Position::prevMove() const noexcept
	{
		ASSERT(!m_stateHistory.empty());
		return stateInfo().prevMove;
	}
}