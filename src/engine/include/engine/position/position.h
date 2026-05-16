#pragma once

#include <engine/core/core.h>

#include <engine/types/castling_right.h>
#include <engine/types/bitboard.h>
#include <engine/types/square.h>
#include <engine/types/piece.h>
#include <engine/types/move.h>
#include <engine/types/zobrist.h>

#include <engine/move_gen/attacks.h>
#include <engine/position/StateInfo.h>
#include <engine/search/evaluation_constants.h>
 
#include <expected>
#include <string_view>
#include <type_traits>
#include <string>

namespace Bratwurst
{

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

	void doNullMove();
	void undoNullMove();

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

	[[nodiscard]] inline Bitboard checkers() const { return stateInfo().checkers; }
	[[nodiscard]] inline Bitboard pinned() const { return stateInfo().pinned; }
	[[nodiscard]] inline Bitboard attackers(Square s, Color attackingColor, Bitboard blockers) const;

	[[nodiscard]] inline Color colorToMove() const { return m_colorToMove; }
	[[nodiscard]] inline uint16 fullMoveCounter() const { return m_fullMoveCounter; }
	[[nodiscard]] inline StateInfo& stateInfo() { return m_stateHistory.back(); }
	[[nodiscard]] inline const StateInfo& stateInfo() const { return m_stateHistory.back(); }
	[[nodiscard]] inline bool hasCastlingRight(CastlingRight right) const { return stateInfo().castlingRights.canCastle(right); }
	[[nodiscard]] inline uint8 halfmoveClock() const { return stateInfo().halfMoveClock; }
	[[nodiscard]] inline Square enPassantSquare() const { return stateInfo().enPassantSquare; }
	[[nodiscard]] inline Move prevMove() const { return stateInfo().prevMove; }

	[[nodiscard]] inline bool isThreefoldRepetition(int repCnt = 3) const;
	[[nodiscard]] inline bool isFiftyMoveRule() const { return halfmoveClock() >= 100; }

	[[nodiscard]] inline Zobrist::Key zobristKey() const { return stateInfo().zobristKey; }
	[[nodiscard]] inline Zobrist::Key pawnKey() const { return stateInfo().pawnKey; }

	[[nodiscard]] inline int material() const { return m_material; }
	[[nodiscard]] inline int egPSQT() const { return m_egPSQT; }
	[[nodiscard]] inline int mgPSQT() const { return m_mgPSQT; }
	[[nodiscard]] inline int phase() const { return std::min(m_phase, Evaluation::MAX_PHASE); }

	[[nodiscard]] inline std::string toString() const;

private:
	Piece m_pieces[SquareNum];
	Bitboard m_typeBBs[PieceTypeNum];
	Bitboard m_colorBBs[ColorNum];

	Color m_colorToMove;
	uint16 m_fullMoveCounter;
	StateHistory m_stateHistory;

	// Incrementally updated to speed up evaluation
	int m_material;
	int m_egPSQT;
	int m_mgPSQT;

	// this value is an indicator to how far the game has progressed.
	// 24 meaning the game has just started and 0 there are no pieces except pawns on the board.
	int m_phase;

private:
	// due to performance these 3 functions don't check for required absence or presence of pieces internally
	inline void movePiece(Square src, Square dst, Piece srcPiece);
	inline void placePiece(Square s, Piece piece);
	inline void removePiece(Square s, Piece piece);
};

inline std::ostream& operator<<(std::ostream& os, const Position& pos)
{
	return os << pos.toString();
}

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

inline bool Position::isThreefoldRepetition(int repCnt) const
{
	const Zobrist::Key current = zobristKey();
	int repetitions = 0;

	const int maxBack = std::min<int>(halfmoveClock(), static_cast<int>(m_stateHistory.size()) - 1);

	// iterate backwards over only the reversible portion of history
	int size = static_cast<int>(m_stateHistory.size());
	for (int i = size - 1; i >= size - 1 - maxBack; i -= 2) // step by 2: same side to move
	{
		if (m_stateHistory[i].zobristKey == current)
		{
			++repetitions;
			if (repetitions >= repCnt) return true;
		}
	}

	return false;
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

inline void Position::movePiece(Square src, Square dst, Piece srcPiece) 
{
	ASSERT(isValid(src) && isValid(dst));
	ASSERT(m_pieces[src] == srcPiece);
	ASSERT(m_pieces[dst] == NonePiece);
	ASSERT(isValid(srcPiece));

	// Update PSQT
	int srcEntry = Evaluation::PSQT[srcPiece][src];
	int dstEntry = Evaluation::PSQT[srcPiece][dst];
	m_mgPSQT += Evaluation::mg(dstEntry) - Evaluation::mg(srcEntry);
	m_egPSQT += Evaluation::eg(dstEntry) - Evaluation::eg(srcEntry);

	// move the actuall piece
	Bitboard moveMask = bb(src) | bb(dst);

	m_colorBBs[colorOf(srcPiece)] ^= moveMask;
	m_typeBBs[pieceTypeOf(srcPiece)] ^= moveMask;

	m_pieces[src] = NonePiece;
	m_pieces[dst] = srcPiece;
}

inline void Position::placePiece(Square s, Piece piece) 
{
	ASSERT(isValid(piece) && isValid(s));
	ASSERT(m_pieces[s] == NonePiece);

	PieceType pt = pieceTypeOf(piece);

	// update PSQT
	int entry = Evaluation::PSQT[piece][s];
	m_mgPSQT += Evaluation::mg(entry);
	m_egPSQT += Evaluation::eg(entry);

	// update game phase and material
	m_phase += Evaluation::PiecePhaseValue[pt];
	m_material += Evaluation::PieceValue[piece];

	// place Piece
	Bitboard mask = bb(s);
	m_colorBBs[colorOf(piece)] |= mask;
	m_typeBBs[pt] |= mask;
	m_pieces[s] = piece;
}

inline void Position::removePiece(Square s, Piece piece) 
{
	ASSERT(isValid(s) && isValid(piece));
	ASSERT(m_pieces[s] == piece);

	PieceType pt = pieceTypeOf(piece);

	// update PSQT
	int entry = Evaluation::PSQT[piece][s];
	m_mgPSQT -= Evaluation::mg(entry);
	m_egPSQT -= Evaluation::eg(entry);

	// update game phase and material
	m_phase -= Evaluation::PiecePhaseValue[pt];
	m_material -= Evaluation::PieceValue[piece];

	// remove piece
	Bitboard mask = bb(s);
	m_colorBBs[colorOf(piece)] ^= mask;
	m_typeBBs[pt] ^= mask;
	m_pieces[s] = NonePiece;
}

inline std::string Position::toString() const
{
	std::string pos;
	pos.reserve(8 * 8 * 2 + 8); // pieces + spaces + newlines

	for (Rank r = Rank8; isValid(r); --r)
	{
		for (File f = FileA; f < FileNum; ++f)
		{
			const Square s = makeSquare(f, r);
			pos += pieceToChar(m_pieces[s]);
			pos += ' ';
		}

		pos += '\n';
	}

	return pos;
}

} 