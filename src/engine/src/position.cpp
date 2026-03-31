#include <engine/position/position.h>

#include <engine/move_gen/attacks.h>

#include <ranges>

namespace Bratwurst
{

void Position::clear() 
{
    std::fill(m_pieces, m_pieces + SquareNum, NonePiece);
    std::fill(m_typeBBs, m_typeBBs + PieceTypeNum, 0ULL);
    std::fill(m_colorBBs, m_colorBBs + ColorNum, 0ULL);

    while (!m_stateHistory.empty()) m_stateHistory.pop_back();

    m_colorToMove = NoneColor;
    m_fullMoveCounter = 0;
}
/* Parse a chess position from Forsyth-Edwards Notation (FEN)
* FEN format: "pieces active_color castling en_passant halfmove fullmove"
* Example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" */
std::expected<Position, Position::FenError> Position::fromFEN(const std::string& fen) 
{
    Position pos;
    pos.clear();

    pos.m_stateHistory.emplace_back();
    StateInfo& currentStateInfo = pos.m_stateHistory.back();

    // split FEN into its 6 components
    std::vector<std::string> parts;
    parts.reserve(6);
    auto view = std::views::split(fen, ' ');
    for (auto&& part : view) { parts.emplace_back(part.begin(), part.end()); }

    if (parts.size() != 6) return std::unexpected(FenError::InvalidFormat);

    // Fen starts at at left (A8)
    File currFile = FileA;
    Rank currRank = Rank8;

    // parse pieces
    for (char c : parts[0])
    {
        if (c >= '1' && c <= '8')
        {
            // skip squares if digit from 0-8
            int skip = c - '0';
            currFile += skip * Right;
        }
        else if (c == '/')
        {
            // move down one rank
            currRank--;
            if (currFile != (FileH + 1) || !isValid(currRank)) return std::unexpected(FenError::InvalidFormat);
            currFile = FileA;
        }
        else
        {
            // parse char into piece and place it on the current square
            Piece piece = charToPiece(c);
            if (!isValid(piece) || !isValid(currFile)) return std::unexpected(FenError::InvalidPiecePlacement);
            pos.placePiece(makeSquare(currFile, currRank), piece);
            currFile += Right;
        }


        if (currFile > FileH + 1) return std::unexpected(FenError::InvalidPiecePlacement);
    }

    if (popCnt(pos.pieceBB(White, King)) != 1 || popCnt(pos.pieceBB(Black, King)) != 1) return std::unexpected(FenError::InvalidPiecePlacement);
    if (currFile != (FileH + 1) || currRank != Rank1) return std::unexpected(FenError::InvalidPiecePlacement);

    // parse side to move ("w" = White, "b = Black)
    if ((parts[1] != "w" && parts[1] != "b") || parts[1].size() != 1) return std::unexpected(FenError::InvalidColorToMove);
    pos.m_colorToMove = (parts[1] == "w") ? White : Black;

    // parse castling rights 
    if (parts[2] != "-")
    {
        for (char c : parts[2])
        {
            CastlingRight right = charToCastlingRight(c);
            if (right == CastlingRightNone) return std::unexpected(FenError::InvalidCastlingRights);
            currentStateInfo.castlingRights.allowCastling(right);
        }
    }

    // Parse en passant square 
    currentStateInfo.enPassantSquare = stringToSquare(parts[3]);
    if (!isValid(currentStateInfo.enPassantSquare) && parts[3] != "-") return std::unexpected(FenError::InvalidEnPassantSquare);

    // parse halfmove clock
    try
    {
        int halfmoveClock = std::stoi(parts[4]);
        if (halfmoveClock < 0 || halfmoveClock > 50) return std::unexpected(FenError::InvalidHalfmoveClock);
        currentStateInfo.halfMoveClock = static_cast<uint8>(halfmoveClock);
    }
    catch (const std::invalid_argument& e)
    {
        return std::unexpected(FenError::InvalidHalfmoveClock);
    }

    // parse fullmove counter
    try
    {
        int fullmoveCounter = std::stoi(parts[5]);
        if (fullmoveCounter < 1) return std::unexpected(FenError::InvalidFullMoveCounter);
        pos.m_fullMoveCounter = fullmoveCounter;
    }
    catch (const std::invalid_argument& e)
    {
        return std::unexpected(FenError::InvalidFullMoveCounter);
    }

    pos.updateCheckers();
    pos.updatePinned();
    pos.initZobrist();

    return pos;
}

std::string Position::fen() const 
{
    std::string fen;
    fen.reserve(100);

    for (Rank r = Rank8; isValid(r); r--)
    {
        uint8 emptySquares = 0;

        for (File f = FileA; isValid(f); f++)
        {
            Square s = makeSquare(f, r);
            Piece piece = pieceOn(s);

            if (piece == NonePiece)
            {
                emptySquares++;
            }
            else
            {
                if (emptySquares != 0)
                {
                    fen += (emptySquares + '0');
                    emptySquares = 0;
                }
                fen += pieceToChar(piece);
            }
        }

        if (emptySquares != 0) fen += (emptySquares + '0');

        if (r != Rank1) fen += '/';
    }
        
    fen += (m_colorToMove == White) ? " w" : " b";

    fen += ' ';
    const StateInfo& state = stateInfo();
    std::string castling;

    if (state.castlingRights.canCastle(CastlingRight::WhiteOO)) castling += 'K';
    if (state.castlingRights.canCastle(CastlingRight::WhiteOOO)) castling += 'Q';
    if (state.castlingRights.canCastle(CastlingRight::BlackOO)) castling += 'k';
    if (state.castlingRights.canCastle(CastlingRight::BlackOOO)) castling += 'q';
    fen += castling.empty() ? "-" : castling;

    fen += ' ';
    fen += (state.enPassantSquare != NoneSquare) ? squareToString(state.enPassantSquare) : "-";

    fen += ' ' + std::to_string(state.halfMoveClock);
    fen += ' ' + std::to_string(m_fullMoveCounter);
   
    return fen;
}

void Position::updatePinned()
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

void Position::doMove(Move move)
{
    Square src = move.src();
    Square dst = move.dst();

    const StateInfo prevStateInfo = stateInfo();

    Piece srcPiece = m_pieces[src];
    PieceType srcType = pieceTypeOf(srcPiece);
    Color friendly = m_colorToMove;
    Color enemy = ~m_colorToMove;

    Piece capturedPiece = m_pieces[dst];
    bool isCapture = (capturedPiece != NonePiece);

    ASSERT(colorOf(srcPiece) == friendly);

    // Save current state for undo/redo
    StateInfo newStateInfo =
    {
        .castlingRights = prevStateInfo.castlingRights,
        .halfMoveClock = uint8(prevStateInfo.halfMoveClock + 1),
        .enPassantSquare = NoneSquare,
        .capturedPiece = capturedPiece,
        .prevMove = move,
        .zobristKey = prevStateInfo.zobristKey,
    };

    // Handle special cases
    if (isCapture)
    {
        ASSERT(colorOf(capturedPiece) == enemy);
        ASSERT(pieceTypeOf(capturedPiece) != King);

        // Remove captured piece from dst and update zobrist + halfmove clock
        removePiece(dst, capturedPiece);
        newStateInfo.zobristKey ^= Zobrist::piece[capturedPiece][dst];
        newStateInfo.halfMoveClock = 0;

    }
    if (move.promotion())
    {
        // Remove pawn, place promotion piece and update zobrist key
        ASSERT(srcPiece == makePiece(friendly, Pawn));

        Piece promotionPiece = makePiece(friendly, move.promotionType());
        newStateInfo.zobristKey ^= Zobrist::piece[srcPiece][src];
        newStateInfo.zobristKey ^= Zobrist::piece[promotionPiece][dst];
        
        removePiece(src, srcPiece);
        placePiece(dst, promotionPiece);

    }
    else if (move.enPassant())
    {
        // Capture pawn behind dst
        Square enemyPawnSquare = dst + ((friendly == White) ? Down : Up);
        Piece enemyPawn = makePiece(enemy, Pawn);

        ASSERT(m_pieces[enemyPawnSquare] == enemyPawn);

        removePiece(enemyPawnSquare, enemyPawn);
        newStateInfo.zobristKey ^= Zobrist::piece[enemyPawn][enemyPawnSquare];
    }
    else if (move.castling())
    {
        // Move rook in castling
        CastlingRight right = makeCastlingRight(friendly, move.castlingSide());
        Piece friendlyRook = makePiece(friendly, Rook);
        Square rookSrc = CastlingRookSrc[right];
        Square rookDst = CastlingRookDst[right];

        ASSERT(srcType == King && capturedPiece == NonePiece);
        ASSERT(m_pieces[rookSrc] == friendlyRook && m_pieces[rookDst] == NonePiece);

        movePiece(rookSrc, rookDst, friendlyRook);
        newStateInfo.zobristKey ^= Zobrist::piece[friendlyRook][rookSrc];
        newStateInfo.zobristKey ^= Zobrist::piece[friendlyRook][rookDst];
    }

    // move piece normally
    if (!move.promotion())
    {
        movePiece(src, dst, srcPiece);
        newStateInfo.zobristKey ^= Zobrist::piece[srcPiece][src];
        newStateInfo.zobristKey ^= Zobrist::piece[srcPiece][dst];
    }

    // Update castling rights
    if (srcType == King)
    {
        newStateInfo.zobristKey ^= Zobrist::castling[newStateInfo.castlingRights.data];
        newStateInfo.castlingRights.disallowCastling(makeCastlingRight(friendly, KingSide));
        newStateInfo.castlingRights.disallowCastling(makeCastlingRight(friendly, QueenSide));
        newStateInfo.zobristKey ^= Zobrist::castling[newStateInfo.castlingRights.data];
    }
    if (srcType == Rook || capturedPiece == makePiece(enemy, Rook))
    {
        newStateInfo.zobristKey ^= Zobrist::castling[newStateInfo.castlingRights.data];
        Square rookSquare = (srcType == Rook) ? src : dst;
        CastlingRight right = castlingRightByRookSrc(rookSquare);
        if (isValid(right)) newStateInfo.castlingRights.disallowCastling(right);
        newStateInfo.zobristKey ^= Zobrist::castling[newStateInfo.castlingRights.data];
    }

    // remove previous epSquare from zobrist key
    File epFile = fileOf(prevStateInfo.enPassantSquare);
    uint64 epZobrist = Zobrist::enPassant[epFile];
    newStateInfo.zobristKey ^= epZobrist;

    // Reset halfmove clock on pawn move nad handle ep
    if (srcType == Pawn)
    {
        newStateInfo.halfMoveClock = 0;
        bool doublePawnPush = std::abs(int(src) - int(dst)) == 2 * Up;

        if (doublePawnPush)
        {
            Square epSquare = src + pawnPushDir(friendly);
            newStateInfo.enPassantSquare = epSquare;
            File epFile = fileOf(epSquare);
            newStateInfo.zobristKey ^= Zobrist::enPassant[epFile];
        }
    }

    // Increment fullmove counter after Black's move 
    m_fullMoveCounter += friendly;

    m_colorToMove = enemy;
    newStateInfo.zobristKey ^= Zobrist::side;

    m_stateHistory.push_back(newStateInfo);

    updateCheckers();
    updatePinned();
}

void Position::undoMove()
{
    ASSERT(m_stateHistory.size() > 1);
    const StateInfo& prevStateInfo = stateInfo();
    Move move = prevStateInfo.prevMove;

    Square src = move.src();
    Square dst = move.dst();

    Color enemy = m_colorToMove;       // side that just moved
    Color friendly = ~m_colorToMove;   // side to move after undo

    Piece piece = m_pieces[dst];
    PieceType pieceType = pieceTypeOf(piece);

    Piece capturedPiece = prevStateInfo.capturedPiece;
    bool isCapture = (capturedPiece != NonePiece);

    if (!move.promotion())
    {
        movePiece(dst, src, piece);
    }

    if (move.promotion())
    {
        // Remove promoted piece and restore pawn
        Piece promotionPiece = makePiece(friendly, move.promotionType());
        ASSERT(piece == promotionPiece);
        removePiece(dst, promotionPiece);
        placePiece(src, makePiece(friendly, Pawn));
    }
    else if (move.enPassant())
    {
        // Restore captured pawn behind dst
        Square enemyPawnSquare = dst + ((friendly == White) ? Down : Up);
        Piece enemyPawn = makePiece(enemy, Pawn);
        ASSERT(m_pieces[enemyPawnSquare] == NonePiece);
        placePiece(enemyPawnSquare, enemyPawn);
    }
    else if (move.castling())
    {
        // Move rook back to original square
        CastlingRight right = makeCastlingRight(friendly, move.castlingSide());
        Piece friendlyRook = makePiece(friendly, Rook);
        Square rookSrc = CastlingRookSrc[right];
        Square rookDst = CastlingRookDst[right];
        ASSERT(pieceType == King);
        ASSERT(m_pieces[rookSrc] == NonePiece && m_pieces[rookDst] == friendlyRook);
        movePiece(rookDst, rookSrc, friendlyRook);
    }
    if (isCapture)
    {
        // Restore captured piece
        ASSERT(colorOf(capturedPiece) == enemy);
        placePiece(dst, capturedPiece);
    }

    m_fullMoveCounter -= friendly;
    m_colorToMove = friendly;
    m_stateHistory.pop_back();
}
}