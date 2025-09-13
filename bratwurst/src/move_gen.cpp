#include "move_gen.h"
#include "attacks.h"
#include "types/castling_right.h"

#include <iostream>

namespace Bratwurst
{



template<Color friendly, GenType genType>
void generatePawnMoves(const Position& pos, Bitboard mask, MoveList& out) noexcept
{
	// helper lamndas to avoid code duplication
	auto blockedByPin = [&pos](Square src, Square dst) -> bool
		{
			if (pos.pinned() & squareMask(src))
			{
				return (Precomputed::lineBBs[0][src][pos.kingSquare(friendly)] & squareMask(dst)) == 0ULL;
			}

			return false;
		};

	auto addMovesByOffset = [&](Bitboard attacks, Direction offset)
		{
			while (attacks)
			{
				Square dst = popLsb(attacks);
				Square src = dst - offset;
				if (blockedByPin(src, dst)) continue;
				out.emplace_back(src, dst, Move::Flag::None);
			}
		};
	auto addPromotionMoves = [&](Bitboard attacks, Direction offset)
		{
			while (attacks)
			{
				Square dst = popLsb(attacks);
				Square src = Square(dst - offset);

				if (blockedByPin(src, dst)) continue;

				out.emplace_back(src, dst, Move::Flag::QueenPromotion);
				out.emplace_back(src, dst, Move::Flag::KnightPromotion);
				out.emplace_back(src, dst, Move::Flag::BishopPromotion);
				out.emplace_back(src, dst, Move::Flag::RookPromotion);
			}
		};

	constexpr Color enemy = ~friendly;

	constexpr Direction forwardLeft = (friendly == White) ? UpLeft : DownLeft;
	constexpr Direction forwardRight = (friendly == White) ? UpRight : DownRight;
	constexpr Direction forward = (friendly == White) ? Up : Down;
	constexpr Direction doubleForward = 2 * forward;

	constexpr Bitboard promotionRank = (friendly == White) ? rankMask(Rank8) : rankMask(Rank1);
	constexpr Bitboard notFileAMask = ~fileMask(FileA);
	constexpr Bitboard notFileHMask = ~fileMask(FileH);
	constexpr Bitboard rankStartMask = (friendly == White) ? rankMask(Rank3) : rankMask(Rank6);

	Bitboard pawns = pos.pieceBB(friendly, Pawn);
	Bitboard enemyOccupancy = pos.colorBB(enemy);
	Bitboard occupancy = pos.occupancyBB();

	// en passant handling
	Square epSquare = pos.enPassantSquare();
	if (isValid(epSquare))
	{		
		Square enemyPawnSquare = epSquare + ((friendly == White) ? Down : Up);
		ASSERT(pos.pieceBB(enemy, Pawn) & squareMask(enemyPawnSquare));
		Bitboard epPawns = pawns & attacksBB<Pawn, enemy>(epSquare);

		while (epPawns)
		{
			// handle tricky ep pins
			Square src = popLsb(epPawns);
			Bitboard postEpOccupancy = pos.occupancyBB() ^ (squareMask(enemyPawnSquare) | squareMask(src)) | squareMask(epSquare);
			bool resultsInCheck =	attacksBB<Rook>(pos.kingSquare(friendly), postEpOccupancy) & (pos.pieceBB(enemy, Rook, Queen)) ||
									attacksBB<Bishop>(pos.kingSquare(friendly), postEpOccupancy) & (pos.pieceBB(enemy, Bishop, Queen));

			if (!resultsInCheck) out.emplace_back(src, epSquare, Move::Flag::EnPassant);
		}
	}

	if constexpr (genType != GenType::Quiet)
	{
		Bitboard attacksLeft = (shift<forwardLeft>(pawns & notFileAMask)) & enemyOccupancy;
		Bitboard attacksRight = (shift<forwardRight>(pawns & notFileHMask)) & enemyOccupancy;
		addMovesByOffset(attacksLeft & mask & ~promotionRank, forwardLeft);
		addMovesByOffset(attacksRight & mask & ~promotionRank, forwardRight);
		addPromotionMoves(attacksLeft & mask & promotionRank, forwardLeft);
		addPromotionMoves(attacksRight & mask & promotionRank, forwardRight);
	}

	if constexpr (genType != GenType::Capture)
	{
		Bitboard singlePush = shift<forward>(pawns) & ~occupancy;
		Bitboard doublePush = shift<forward>(singlePush & rankStartMask) & ~occupancy;
		addMovesByOffset(singlePush & mask & ~promotionRank, forward);
		addMovesByOffset(doublePush & mask, doubleForward);
		addPromotionMoves(singlePush & mask & promotionRank, forward);
	}
}

// generates legal moves for all pieces of pt
template<PieceType pt, GenType genType>
void generateTypeMoves(const Position& pos, Bitboard mask, MoveList& out) noexcept
{
	static_assert(isValid(pt), "invalid pieceType");

	Color friendly = pos.colorToMove();
	Color enemy = ~friendly;

	if constexpr (pt == Pawn)
	{
		(friendly == White) ? 
			generatePawnMoves<White, genType>(pos, mask, out) : 
			generatePawnMoves<Black, genType>(pos, mask, out);
	}

	else if constexpr (pt == King)
	{
		Square kingSq = pos.kingSquare(friendly);
		Bitboard attacks = attacksBB<King>(kingSq) & mask;
		Bitboard occupancy = pos.occupancyBB() ^ squareMask(kingSq);

		CastlingRight kingSideRight = makeCastlingRight(friendly, KingSide);
		CastlingRight queenSideRight = makeCastlingRight(friendly, QueenSide);		
		Direction offset = friendly * 7 * FileNum * Right;

		// kingside castling
		if ((castlingPathMask[kingSideRight] & occupancy) == 0ULL &&
			pos.checkers() == 0ULL &&
			pos.hasCastlingRight(kingSideRight) && 
			pos.attackers(F1 + offset, enemy, occupancy) == 0ULL && 
			pos.attackers(G1 + offset, enemy, occupancy) == 0ULL)
		{
			ASSERT(pos.pieceOn(rookCastlingSources[kingSideRight]) == makePiece(friendly, Rook));
			ASSERT(pos.pieceOn(kingCastlingSources[kingSideRight]) == makePiece(friendly, King));
			out.emplace_back(kingCastlingSources[kingSideRight], kingCastlingDestinations[kingSideRight], Move::Flag::CastlingOO);
		}

		// queenside castling
		if ((castlingPathMask[queenSideRight] & occupancy) == 0ULL &&
			pos.checkers() == 0ULL &&
			pos.hasCastlingRight(queenSideRight) &&
			pos.attackers(C1 + offset, enemy, occupancy) == 0ULL && 
			pos.attackers(D1 + offset, enemy, occupancy) == 0ULL)
		{
			ASSERT(pos.pieceOn(rookCastlingSources[queenSideRight]) == makePiece(friendly, Rook));
			ASSERT(pos.pieceOn(kingCastlingSources[queenSideRight]) == makePiece(friendly, King));
			out.emplace_back(kingCastlingSources[queenSideRight], kingCastlingDestinations[queenSideRight], Move::Flag::CastlingOOO);
		}

		while (attacks)
		{
			Square dst = popLsb(attacks);
			if (pos.attackers(dst, enemy, occupancy)) continue;
			out.emplace_back(kingSq, dst, Move::Flag::None);
		}
	}

	else
	{
		// handle move generation for all pieces except Pawn and King
		Bitboard pieces = pos.pieceBB(friendly, pt);
		Bitboard blockers = pos.occupancyBB();

		while (pieces)
		{
			Square src = popLsb(pieces);
			Bitboard attacks = attacksBB<pt>(src, blockers) & mask;
			if (pos.pinned() & squareMask(src)) attacks &= Precomputed::lineBBs[0][src][pos.kingSquare(friendly)];

			while (attacks)
			{
				Square dst = popLsb(attacks);
				out.emplace_back(src, dst, Move::Flag::None);
			}
		}
	}
}

template<GenType genType>
void generateMoves(const Position& pos, MoveList& out) noexcept 
{
	Color friendly = pos.colorToMove();
	Color enemy = ~friendly;

	Bitboard checkers = pos.checkers();
	uint8 checkersCnt = popCnt(checkers);
	Square kingSq = pos.kingSquare(friendly);
	
	// mask contains squares that can be attacked
	Bitboard mask = (checkersCnt == 1) ? Precomputed::lineBBs[1][kingSq][lsb(checkers)] | checkers : ~pos.colorBB(friendly);
	if constexpr (genType == GenType::Capture) mask &= pos.colorBB(enemy);
	if constexpr (genType == GenType::Quiet) mask &= ~pos.colorBB(enemy);

	if (checkersCnt != 2)
	{
		generateTypeMoves<Pawn, genType>(pos, mask, out);
		generateTypeMoves<Knight, genType>(pos, mask, out);
		generateTypeMoves<Bishop, genType>(pos, mask, out);
		generateTypeMoves<Rook, genType>(pos, mask, out);
		generateTypeMoves<Queen, genType>(pos, mask, out);
	}
	
	generateTypeMoves<King, genType>(pos, ~pos.colorBB(friendly), out);
}

template void generateMoves<GenType::All>(const Position& pos, MoveList& out) noexcept;
template void generateMoves<GenType::Capture>(const Position& pos, MoveList& out) noexcept;
template void generateMoves<GenType::Quiet>(const Position& pos, MoveList& out) noexcept;
}