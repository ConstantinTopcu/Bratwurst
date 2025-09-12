#pragma once

#include "core.h"

#include "types/castling_right.h"
#include "types/bitboard.h"
#include "types/square.h"
#include "types/piece.h"
#include "types/move.h"

#include "attacks.h"
#include "precomputed.h"

#include <stack>
#include <expected>
#include <string_view>
#include <type_traits>
#include <string>

namespace Bratwurst
{
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

	Bitboard checkers;
	Bitboard pinned;

	constexpr bool hasCastlingRight(CastlingRight right) const noexcept
	{
		return castlingRights & (1 << right);
	}
		
	constexpr void removeCastlingRight(CastlingRight right) noexcept
	{
		ASSERT(right != CastlingRight::CastlingRightNone);
		castlingRights &= ~(1 << right);
	}
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
	Position() noexcept = default;
	~Position() noexcept = default;

	static std::expected<Position, FenError> fromFEN(const std::string& fen = std::string(StartPosFEN)) noexcept;
	std::string fen() const noexcept;

	void doMove(Move move) noexcept;
	void undoMove() noexcept;

	// Getters for pieces
	template<typename... PieceTypes>
	inline Bitboard typeBB(PieceTypes... types) const noexcept;
	inline Bitboard colorBB(Color c) const noexcept;
	
	template<typename... PieceTypes>
	inline Bitboard pieceBB(Color c, PieceTypes... types) const noexcept;
	inline Bitboard occupancyBB() const noexcept;
	inline Piece pieceOn(Square s) const noexcept;
	inline Square kingSquare(Color c) const noexcept;

	inline Bitboard attackers(Square s, Color attackingColor, Bitboard blockers) const noexcept;
	inline void updateCheckers() noexcept;
	inline Bitboard checkers() const noexcept;
	inline void updatePinned() noexcept;
	inline Bitboard pinned() const noexcept;

	inline Color colorToMove() const noexcept;
	inline uint16 fullMovecounter() const noexcept;

	// getters related to StateInfo
	inline StateInfo& stateInfo() noexcept;
	inline const StateInfo& stateInfo() const noexcept;
	inline bool hasCastlingRight(CastlingRight rights) const noexcept;
	inline uint8 halfmoveClock() const noexcept;
	inline Square enPassantSquare() const noexcept;

	// returns the piece that was captured last move
	// returns NonePiece in case there was no capture
	inline Piece capturedPiece() const noexcept;
	inline Move prevMove() const noexcept;

	inline void printMoveHistory() const noexcept
	{
		for (const StateInfo& stateInfo : m_stateHistory)
		{
			stateInfo.prevMove.print();
		}
	}

private:
	// Use both Piece array and bitboards for pieceTypes and colors, since both have their own advantages:
	// - The Piece array enables the fast retrieval of a piece on a given square.
	// - The Bitboards allow for fast bulk operations, like applying masks or counting pieces.
	Piece m_pieces[SquareNum];
	Bitboard m_typeBBs[PieceTypeNum];
	Bitboard m_colorBBs[ColorNum];

	Color m_colorToMove;
	uint16 m_fullMoveCounter;
	std::deque<StateInfo> m_stateHistory;

private:
	void clear() noexcept;

	inline void movePiece(Square src, Square dst, Piece srcPiece) noexcept;
	inline void placePiece(Square s, Piece piece) noexcept;
	inline void removePiece(Square s, Piece piece) noexcept;
};

template<typename... PieceTypes>
inline Bitboard Position::typeBB(PieceTypes... types) const noexcept
{
	static_assert((std::is_same_v<PieceTypes, PieceType> && ...), "All arguments must be of type PieceType");
	return (m_typeBBs[types] | ...);
}

inline Bitboard Position::colorBB(Color c) const noexcept
{
	ASSERT(isValid(c));
	return m_colorBBs[c];
}
template<typename... PieceTypes>
inline Bitboard Position::pieceBB(Color c, PieceTypes... types) const noexcept
{
	ASSERT(isValid(c));
	return m_colorBBs[c] & typeBB(types...);
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

inline Square Position::kingSquare(Color c) const noexcept
{
	return lsb(pieceBB(c, King));
}

inline Bitboard Position::attackers(Square s, Color attackingColor, Bitboard blockers) const noexcept
{
	Bitboard attackers = 0ULL;

	attackers |= Precomputed::pseudoAttacks[makePiece(~attackingColor, Pawn)][s] & pieceBB(attackingColor, Pawn);
	attackers |= attacksBB<Knight>(s) & pieceBB(attackingColor, Knight);
	attackers |= attacksBB<Bishop>(s, blockers) & (typeBB(Bishop, Queen) & colorBB(attackingColor));
	attackers |= attacksBB<Rook>(s, blockers) & (typeBB(Rook, Queen) & colorBB(attackingColor));
	attackers |= attacksBB<King>(s, blockers) & pieceBB(attackingColor, King);

	return attackers;
}

inline void Position::updateCheckers() noexcept
{
	stateInfo().checkers = attackers(kingSquare(m_colorToMove), ~m_colorToMove, occupancyBB());
}

inline Bitboard Position::checkers() const noexcept
{
	return stateInfo().checkers;
}

inline void Position::updatePinned() noexcept
{
	Color friendly = m_colorToMove;
	Color enemy = ~friendly;
	Square kingSq = kingSquare(friendly);
	Bitboard friendlyPieces = colorBB(friendly);
	Bitboard enemyPieces = colorBB(enemy);
	Bitboard occupancy = friendlyPieces | enemyPieces;

	Bitboard potentialPinned = attacksBB<Queen>(kingSq, occupancy) & friendlyPieces;
	occupancy ^= potentialPinned;

	Bitboard rooks = typeBB(Rook, Queen) & enemyPieces;
	Bitboard bishops = typeBB(Bishop, Queen) & enemyPieces;
	Bitboard XRayRookAttacks = attacksBB<Rook>(kingSq, occupancy);
	Bitboard XRayBishopAttacks = attacksBB<Bishop>(kingSq, occupancy);
	Bitboard pinners = (XRayRookAttacks & rooks) | (XRayBishopAttacks & bishops);

	Bitboard pinned = 0ULL;
	while (pinners)
	{
		Square pinner = popLsb(pinners);
		pinned |= (Precomputed::lineBBs[1][kingSq][pinner] & friendlyPieces);
	}

	stateInfo().pinned = pinned;
}

inline Bitboard Position::pinned() const noexcept
{
	return stateInfo().pinned;;
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
	return m_stateHistory.back();
}

inline const StateInfo& Position::stateInfo() const noexcept
{
	ASSERT(!m_stateHistory.empty());
	return m_stateHistory.back();
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

inline Square Position::enPassantSquare() const noexcept
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

inline void Position::movePiece(Square src, Square dst, Piece srcPiece) noexcept
{
	ASSERT(isValid(src) && isValid(dst));
	ASSERT(m_pieces[src] == srcPiece);
	ASSERT(m_pieces[dst] == NonePiece);
	ASSERT(isValid(srcPiece));

	Bitboard moveMask = squareMask(src) | squareMask(dst);
	m_colorBBs[colorOf(srcPiece)] ^= moveMask;
	m_typeBBs[pieceTypeOf(srcPiece)] ^= moveMask;
	m_pieces[src] = NonePiece;
	m_pieces[dst] = srcPiece;
}

inline void Position::placePiece(Square s, Piece piece) noexcept
{
	ASSERT(isValid(piece) && isValid(s));
	ASSERT(m_pieces[s] == NonePiece);

	Bitboard mask = squareMask(s);
	m_colorBBs[colorOf(piece)] |= mask;
	m_typeBBs[pieceTypeOf(piece)] |= mask;
	m_pieces[s] = piece;
}

inline void Position::removePiece(Square s, Piece piece) noexcept
{
	ASSERT(isValid(s) && isValid(piece));
	ASSERT(m_pieces[s] == piece);

	Bitboard mask = squareMask(s);
	m_colorBBs[colorOf(piece)] ^= mask;
	m_typeBBs[pieceTypeOf(piece)] ^= mask;
	m_pieces[s] = NonePiece;
}
} 