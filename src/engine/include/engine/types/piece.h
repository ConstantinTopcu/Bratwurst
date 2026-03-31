#pragma once

#include <engine/core/core.h>
#include <engine/types/enum_ops.h>
#include <engine/types/square.h>

namespace Bratwurst
{
	// Currently Piece has a stride of 6, meaning extractions cannot be made using a bitmask.
	// If this becomes a hotpath bottleneck, the stride might be changed to 8, to enable faster extractions
	enum Piece : uint8
	{
		WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
		BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing,
		NonePiece, PieceNum = 12
	};

	enum PieceType : uint8
	{
		Pawn, Knight, Bishop, Rook, Queen, King,
		NonePieceType, PieceTypeNum = 6
	};

	enum Color : uint8
	{
		White, Black,
		NoneColor, ColorNum = 2
	};

	// Enable Incr/Decr operators
	ENABLE_ENUM_ARITHMETIC(Piece);
	ENABLE_ENUM_ARITHMETIC(PieceType);
	ENABLE_ENUM_ARITHMETIC(Color);

	// Validation functions
	constexpr bool isValid(Piece p) 
	{
		return p < NonePiece;
	}

	constexpr bool isValid(PieceType pt) 
	{
		return pt < NonePieceType;
	}

	constexpr bool isValid(Color c) 
	{
		return c < NoneColor;
	}

	constexpr PieceType pieceTypeOf(Piece p) 
	{
		ASSERT(isValid(p));
		return (static_cast<PieceType>(p % PieceTypeNum));
	}

	constexpr Color colorOf(Piece p) 
	{
		ASSERT(isValid(p));
		return static_cast<Color>(p >= BlackPawn);
	}

	constexpr Piece makePiece(Color c, PieceType pt) 
	{
		ASSERT(isValid(c) && isValid(pt));
		return static_cast<Piece>(c * PieceTypeNum + pt);
	}

	constexpr Direction pawnPushDir(Color c)
	{
		return (c == White) ? Up : Down;
	}

	constexpr char pieceToChar(Piece p)  
	{ 
		switch (p) 
		{ 
		case WhitePawn: return 'P'; 
		case WhiteKnight: return 'N'; 
		case WhiteBishop: return 'B'; 
		case WhiteRook: return 'R'; 
		case WhiteQueen: return 'Q';
		case WhiteKing: return 'K'; 
		
		case BlackPawn: return 'p'; 
		case BlackKnight: return 'n';
		case BlackBishop: return 'b';
		case BlackRook: return 'r'; 
		case BlackQueen: return 'q';
		case BlackKing: return 'k'; 
		}
		
		return '.';
	} 

	constexpr Piece charToPiece(char p)  
	{ 
		switch (p)
		{
		case 'P': return WhitePawn;
		case 'N': return WhiteKnight;
		case 'B': return WhiteBishop;
		case 'R': return WhiteRook;
		case 'Q': return WhiteQueen;
		case 'K': return WhiteKing;

		case 'p': return BlackPawn;
		case 'n': return BlackKnight;
		case 'b': return BlackBishop;
		case 'r': return BlackRook;
		case 'q': return BlackQueen;
		case 'k': return BlackKing;
		}

		return NonePiece;
	}

	constexpr Color operator~(Color c)
	{
		ASSERT(isValid(c));
		return static_cast<Color>(c ^ 1);
	}
}