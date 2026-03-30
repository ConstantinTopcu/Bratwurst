#pragma once

#include <core.h>

#include <types/castling_right.h>
#include <types/static_vector.h>
#include <types/bitboard.h>
#include <types/square.h>
#include <types/piece.h>
#include <types/move.h>

#include <zobrist.h>
#include <attacks.h>

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

class Position
{

public:
	using StateHistory = StaticVector<StateInfo, 512>;
	
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
	Position()  = default;
	~Position()  = default;

	static std::expected<Position, FenError> fromFEN(const std::string& fen = std::string(StartPosFEN));
	[[nodiscard]] std::string fen() const;

	void doMove(Move move);
	void undoMove();
	void clear();

	// avoid this function when incremental, manual updates are possible, 
	// since it recalculates the entire zobrist key from scratch
	inline void initZobrist();
	inline void updateCheckers();
	inline void updatePinned();

	template<typename... PieceTypes>
	[[nodiscard]] inline Bitboard typeBB(PieceTypes... types) const;
	[[nodiscard]] inline Bitboard colorBB(Color c) const;
	template<typename... PieceTypes>
	[[nodiscard]] inline Bitboard pieceBB(Color c, PieceTypes... types) const;
	[[nodiscard]] inline Bitboard occupancyBB() const { return m_colorBBs[White] | m_colorBBs[Black]; }
	[[nodiscard]] inline Piece pieceOn(Square s) const;
	[[nodiscard]] inline Square kingSquare(Color c) const;

	inline Bitboard checkers() const { return stateInfo().checkers; }
	inline Bitboard pinned() const { return stateInfo().pinned; }
	inline Bitboard attackers(Square s, Color attackingColor, Bitboard blockers) const;

	[[nodiscard]] inline Color colorToMove() const { return m_colorToMove; }
	[[nodiscard]] inline uint16 fullMoveCounter() const { return m_fullMoveCounter; }
	[[nodiscard]] inline StateInfo& stateInfo() { return m_stateHistory.back(); }
	[[nodiscard]] inline const StateInfo& stateInfo() const { return m_stateHistory.back(); }
	[[nodiscard]] inline bool hasCastlingRight(CastlingRight right) const { return stateInfo().castlingRights.canCastle(right); }
	[[nodiscard]] inline uint8 halfmoveClock() const { return stateInfo().halfMoveClock; }
	[[nodiscard]] inline Square enPassantSquare() const { return stateInfo().enPassantSquare; }
	[[nodiscard]] inline Move prevMove() const { return stateInfo().prevMove; }
	[[nodiscard]] inline Zobrist::Key zobristKey() const { return stateInfo().zobristKey; }

private:
	Piece m_pieces[SquareNum];
	Bitboard m_typeBBs[PieceTypeNum];
	Bitboard m_colorBBs[ColorNum];

	Color m_colorToMove;
	uint16 m_fullMoveCounter;
	StateHistory m_stateHistory;

private:
	// due to performance these 3 functions don't check for required absence or presence of pieces internally
	inline void movePiece(Square src, Square dst, Piece srcPiece);
	inline void placePiece(Square s, Piece piece);
	inline void removePiece(Square s, Piece piece);
};

template<typename... PieceTypes>
inline Bitboard Position::typeBB(PieceTypes... types) const 
{
	static_assert((std::is_same_v<PieceTypes, PieceType> && ...), "All arguments must be of type PieceType");
	return (m_typeBBs[types] | ...);
}

inline Bitboard Position::colorBB(Color c) const 
{
	ASSERT(isValid(c));
	return m_colorBBs[c];
}

template<typename... PieceTypes>
inline Bitboard Position::pieceBB(Color c, PieceTypes... types) const 
{
	ASSERT(isValid(c));
	return m_colorBBs[c] & typeBB(types...);
}

inline Piece Position::pieceOn(Square s) const 
{
	ASSERT(isValid(s));
	return m_pieces[s];
}

inline Square Position::kingSquare(Color c) const 
{
	ASSERT(pieceBB(c, King) != 0ULL);
	return lsb(pieceBB(c, King));
}

inline Bitboard Position::attackers(Square s, Color attackingColor, Bitboard blockers) const 
{
	Bitboard attackers = 0ULL;

	attackers |= Precomputed::pseudoAttacks[makePiece(~attackingColor, Pawn)][s] & pieceBB(attackingColor, Pawn);
	attackers |= attacksBB<Knight>(s) & pieceBB(attackingColor, Knight);
	attackers |= attacksBB<Bishop>(s, blockers) & (typeBB(Bishop, Queen) & colorBB(attackingColor));
	attackers |= attacksBB<Rook>(s, blockers) & (typeBB(Rook, Queen) & colorBB(attackingColor));
	attackers |= attacksBB<King>(s, blockers) & pieceBB(attackingColor, King);

	return attackers;
}

inline void Position::updateCheckers() 
{
	stateInfo().checkers = attackers(kingSquare(m_colorToMove), ~m_colorToMove, occupancyBB());
}

inline void Position::initZobrist()
{
	Zobrist::Key zobrist = 0ULL;

	for (Square s = A1; s < SquareNum; s++)
	{
		zobrist ^= Zobrist::piece[pieceOn(s)][s];
	}

	zobrist ^= Zobrist::castling[stateInfo().castlingRights.data];
	zobrist ^= Zobrist::enPassant[stateInfo().enPassantSquare];
	zobrist ^= (m_colorToMove == Black) ? Zobrist::side : 0ULL;

	stateInfo().zobristKey = zobrist;
}

inline void Position::updatePinned() 
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

inline void Position::movePiece(Square src, Square dst, Piece srcPiece) 
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

inline void Position::placePiece(Square s, Piece piece) 
{
	ASSERT(isValid(piece) && isValid(s));
	ASSERT(m_pieces[s] == NonePiece);

	Bitboard mask = squareMask(s);
	m_colorBBs[colorOf(piece)] |= mask;
	m_typeBBs[pieceTypeOf(piece)] |= mask;
	m_pieces[s] = piece;
}

inline void Position::removePiece(Square s, Piece piece) 
{
	ASSERT(isValid(s) && isValid(piece));
	ASSERT(m_pieces[s] == piece);

	Bitboard mask = squareMask(s);
	m_colorBBs[colorOf(piece)] ^= mask;
	m_typeBBs[pieceTypeOf(piece)] ^= mask;
	m_pieces[s] = NonePiece;
}
} 