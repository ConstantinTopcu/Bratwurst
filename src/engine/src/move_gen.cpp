#include <engine/move_gen/move_gen.h>
#include <engine/move_gen/attacks.h>
#include <engine/types/castling_right.h>

namespace Bratwurst
{
	inline bool breaksPin(const Position& pos, Square kingSq, Square src, Square dst)
	{
		if (pos.pinned() & bb(src))
		{
			Bitboard line = Precomputed::lineBBs[0][src][kingSq];
			return (line & bb(dst)) == 0ULL;
		}

		return false;
	}

	// iterates over all moves by using the final destinations and original offset to backtrack src
	// it then calls a template lamda function and gives it the src and dst to allow for custom emit behavior
	template<typename EmitFn>
	inline void forEachMoveFromDests(const Position& pos, Square kingSquare, Bitboard dests, Direction offset, EmitFn emit)
	{
		while (dests)
		{
			Square dst = popLsb(dests);
			Square src = dst - offset;

			if (breaksPin(pos, kingSquare, src, dst)) continue;

			emit(src, dst);
		}
	}

	template<Color friendly, GenType genType>
	void generatePawnMoves(const Position& pos, Bitboard mask, MoveList& out) 
	{
		auto addMoveFn = [&](Square src, Square dst)
			{
				out.emplace(src, dst, Move::Flag::None);
			};

		auto addPromotionMovesFn = [&](Square src, Square dst)
			{
				out.emplace(src, dst, Move::Flag::QueenPromotion);
				out.emplace(src, dst, Move::Flag::KnightPromotion);
				out.emplace(src, dst, Move::Flag::BishopPromotion);
				out.emplace(src, dst, Move::Flag::RookPromotion);
			};

		constexpr Color enemy = ~friendly;

		constexpr Direction forwardLeft		= (friendly == White) ? UpLeft : DownLeft;
		constexpr Direction forwardRight	= (friendly == White) ? UpRight : DownRight;
		constexpr Direction forward			= (friendly == White) ? Up : Down;
		constexpr Direction doubleForward	= 2 * forward;

		constexpr Bitboard promotionRankMask= (friendly == White) ? rankMask(Rank8) : rankMask(Rank1);
		constexpr Bitboard rankStartMask	= (friendly == White) ? rankMask(Rank3) : rankMask(Rank6);
		constexpr Bitboard notFileAMask		= ~fileMask(FileA);
		constexpr Bitboard notFileHMask		= ~fileMask(FileH);

		Bitboard pawns			= pos.pieceBB(friendly, Pawn);
		Bitboard enemyOccupancy = pos.colorBB(enemy);
		Bitboard occupancy		= pos.occupancyBB();
		Square kingSquare		= pos.kingSquare(friendly);
		Square epSquare			= pos.enPassantSquare();

		if (isValid(epSquare))
		{
			Square enemyPawnSquare = epSquare + ((friendly == White) ? Down : Up);
			ASSERT(pos.pieceBB(enemy, Pawn) & bb(enemyPawnSquare));
			Bitboard epPawns = pawns & attacksBB<Pawn, enemy>(epSquare);

			while (epPawns)
			{
				// handle tricky en passant pins
				Square src = popLsb(epPawns);
				Bitboard postEpOccupancy = occupancy ^ (bb(enemyPawnSquare) | bb(src)) | bb(epSquare);

				bool resultsInCheck = 
					attacksBB<Rook>(kingSquare, postEpOccupancy) & (pos.pieceBB(enemy, Rook, Queen)) ||
					attacksBB<Bishop>(kingSquare, postEpOccupancy) & (pos.pieceBB(enemy, Bishop, Queen));

				if (!resultsInCheck) out.emplace(src, epSquare, Move::Flag::EnPassant);
			}
		}

		// add pawn capture moves
		Bitboard nonPromotionMask	= mask & ~promotionRankMask;
		Bitboard attacksLeft		= (shift<forwardLeft>(pawns & notFileAMask)) & enemyOccupancy;
		Bitboard attacksRight		= (shift<forwardRight>(pawns & notFileHMask)) & enemyOccupancy;

		forEachMoveFromDests(pos, kingSquare, attacksLeft & nonPromotionMask, forwardLeft, addMoveFn);
		forEachMoveFromDests(pos, kingSquare, attacksRight & nonPromotionMask, forwardRight, addMoveFn);

		// add promotions moves
		Bitboard promotionMask		= mask & promotionRankMask;
		Bitboard singlePush			= shift<forward>(pawns) & ~occupancy;
		
		forEachMoveFromDests(pos, kingSquare, singlePush & promotionMask, forward, addPromotionMovesFn);
		forEachMoveFromDests(pos, kingSquare, attacksLeft & promotionMask, forwardLeft, addPromotionMovesFn);
		forEachMoveFromDests(pos, kingSquare, attacksRight & promotionMask, forwardRight, addPromotionMovesFn);

		// add single/double pawn pushes
		if constexpr (genType == GenType::All)
		{
			Bitboard doublePush = shift<forward>(singlePush & rankStartMask) & ~occupancy;
			forEachMoveFromDests(pos, kingSquare,singlePush & nonPromotionMask, forward, addMoveFn);
			forEachMoveFromDests(pos, kingSquare, doublePush & mask, doubleForward, addMoveFn);
		}
	}

	// generates legal moves for all pieces of pt
	template<PieceType pt, GenType genType>
	void generateTypeMoves(const Position& pos, Bitboard mask, MoveList& out) 
	{
		static_assert(isValid(pt), "invalid pieceType");

		Color friendly	= pos.colorToMove();
		Color enemy		= ~friendly;

		if constexpr (pt == Pawn)
		{
			(friendly == White) ?
				generatePawnMoves<White, genType>(pos, mask, out) :
				generatePawnMoves<Black, genType>(pos, mask, out);
		}

		else if constexpr (pt == King)
		{
			Square kingSq		= pos.kingSquare(friendly);
			Bitboard attacks	= attacksBB<King>(kingSq) & mask;
			Bitboard occupancy	= pos.occupancyBB() ^ bb(kingSq);

			CastlingRight kingSideRight		= makeCastlingRight(friendly, KingSide);
			CastlingRight queenSideRight	= makeCastlingRight(friendly, QueenSide);
			Bitboard castlingMaskOO			= CastlingPathMask[kingSideRight];
			Bitboard castlingMaskOOO		= CastlingPathMask[queenSideRight];

			Direction offset				= friendly * 7 * FileNum * Right;

			// kingside castling
			if (
				genType == GenType::All &&
				(castlingMaskOO & occupancy) == 0ULL &&
				pos.checkers() == 0ULL &&
				pos.hasCastlingRight(kingSideRight) &&
				pos.attackers(F1 + offset, enemy, occupancy) == 0ULL &&
				pos.attackers(G1 + offset, enemy, occupancy) == 0ULL)
			{
				ASSERT(pos.pieceOn(CastlingRookSrc[kingSideRight]) == makePiece(friendly, Rook));
				ASSERT(pos.pieceOn(CastlingKingSrc[kingSideRight]) == makePiece(friendly, King));
				out.emplace(CastlingKingSrc[kingSideRight], CastlingKingDst[kingSideRight], Move::Flag::CastlingOO);
			}

			// queenside castling
			if (
				genType == GenType::All &&
				(castlingMaskOOO & occupancy) == 0ULL &&
				pos.checkers() == 0ULL &&
				pos.hasCastlingRight(queenSideRight) &&
				pos.attackers(C1 + offset, enemy, occupancy) == 0ULL &&
				pos.attackers(D1 + offset, enemy, occupancy) == 0ULL)
			{
				ASSERT(pos.pieceOn(CastlingRookSrc[queenSideRight]) == makePiece(friendly, Rook));
				ASSERT(pos.pieceOn(CastlingKingSrc[queenSideRight]) == makePiece(friendly, King));
				out.emplace(CastlingKingSrc[queenSideRight], CastlingKingDst[queenSideRight], Move::Flag::CastlingOOO);
			}

			// create legal king moves
			while (attacks)
			{
				Square dst = popLsb(attacks);
				if (pos.attackers(dst, enemy, occupancy)) continue;
				out.emplace(kingSq, dst, Move::Flag::None);
			}
		}

		else
		{
			// handle move generation for all pieces except Pawn and King
			Bitboard pieces		= pos.pieceBB(friendly, pt);
			Bitboard blockers	= pos.occupancyBB();
			Square kingSq		= pos.kingSquare(friendly);

			while (pieces)
			{
				// create pseudo-legal move for piece on square src
				Square src = popLsb(pieces);
				Bitboard attacks = attacksBB<pt>(src, blockers) & mask;

				// account for pins
				if (pos.pinned() & bb(src)) 
					attacks &= Precomputed::lineBBs[0][src][kingSq];

				while (attacks)
				{
					Square dst = popLsb(attacks);
					out.emplace(src, dst, Move::Flag::None);
				}
			}
		}
	}

	template<GenType genType>
	MoveList generateMoves(const Position& pos) 
	{
		MoveList moves;

		Color friendly		= pos.colorToMove();
		Color enemy			= ~friendly;
		Square kingSq		= pos.kingSquare(friendly);
		Bitboard checkers	= pos.checkers();
		uint8 checkersCnt	= popCnt(checkers);

		Bitboard mask		= 0ULL;
		Bitboard kingMask	= 0ULL;
		Bitboard pawnMask	= 0ULL;

		// generate evasions
		if (checkersCnt == 1)
		{
			// only allow moves that move in between king and checking piece or capture it
			Square checker = lsb(checkers);
			mask = Precomputed::lineBBs[1][kingSq][checker] | checkers;
			kingMask = ~pos.colorBB(friendly);
			pawnMask = mask;
		}
		else
		{
			mask = ~pos.colorBB(friendly);
			if constexpr (genType == GenType::Quiescence) mask &= pos.colorBB(enemy);

			kingMask = mask;
			Bitboard promotionRank = (friendly == White) ? rankMask(Rank8) : rankMask(Rank1);
			pawnMask = mask | promotionRank;
		}

		// if in double check, only king moves are possible
		if (checkersCnt < 2)
		{	
			generateTypeMoves<Pawn, genType>(pos, pawnMask, moves);
			generateTypeMoves<Knight, genType>(pos, mask, moves);
			generateTypeMoves<Bishop, genType>(pos, mask, moves);
			generateTypeMoves<Rook, genType>(pos, mask, moves);
			generateTypeMoves<Queen, genType>(pos, mask, moves);
		}

		generateTypeMoves<King, genType>(pos, kingMask, moves);

		return moves;
	}

	template MoveList generateMoves<GenType::All>(const Position& pos);
	template MoveList generateMoves<GenType::Quiescence>(const Position& pos);
}
