#include <engine/position/position.h>

#include <sstream>

namespace Bratwurst
{

using namespace Evaluation;

void Position::clear() 
{
    std::fill(m_pieces, m_pieces + SquareNum, NonePiece);
    std::fill(m_typeBBs, m_typeBBs + PieceTypeNum, 0ULL);
    std::fill(m_colorBBs, m_colorBBs + ColorNum, 0ULL);

    m_stateHistory.clear();
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

    pos.m_stateHistory.emplace();
    StateInfo& currentStateInfo = pos.m_stateHistory.back();

    // split FEN into its 6 components
    std::vector<std::string> parts;
    std::istringstream iss(fen);
    std::string token;

    while (iss >> token)
        parts.push_back(token);

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
        if (halfmoveClock < 0 || halfmoveClock >= 100) return std::unexpected(FenError::InvalidHalfmoveClock);
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
    pos.recomputeState();

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
    StateInfo& st = stateInfo();

    Color c = m_colorToMove;
    Square kingSq = kingSquare(c);
    Bitboard occupancy = occupancyBB();

    Bitboard alignedRooks = Precomputed::pseudoAttacks[Rook][kingSq] & typeBB(Rook, Queen);
    Bitboard alignedBishops = Precomputed::pseudoAttacks[Bishop][kingSq] & typeBB(Bishop, Queen);
    Bitboard potentialPinners = (alignedRooks | alignedBishops) & colorBB(~c);

    st.pinned = 0ULL;

    while (potentialPinners)
    {
        Square pinner = popLsb(potentialPinners);
        Bitboard b = betweenBB(kingSq, pinner) & occupancy;

        if (!exactlyOne(b) || (b & colorBB(~c))) continue;

        st.pinned |= b;
    }
}

void Position::doNullMove()
{
    StateInfo newSt;
    const StateInfo& prevSt = stateInfo();
    std::memcpy(&newSt, &prevSt, offsetof(StateInfo, enPassantSquare));

    newSt.capturedPiece     = NonePiece;
    newSt.enPassantSquare   = NoneSquare;
    newSt.prevMove          = Move::Null(); 
    newSt.checkers          = 0ULL;

    newSt.halfMoveClock ++;
    
    if (prevSt.enPassantSquare != NoneSquare)
    {
        File epFile = fileOf(prevSt.enPassantSquare);
        newSt.zobristKey ^= Zobrist::enPassant[epFile];
        newSt.zobristKey ^= Zobrist::enPassant[NoneFile];
    }

    newSt.zobristKey ^= Zobrist::side;
    m_fullMoveCounter += (m_colorToMove == Black) ? 1 : 0;
    m_colorToMove = ~m_colorToMove;

    m_stateHistory.push(std::move(newSt));
    updatePinned();
}

void Position::undoNullMove()
{
    m_fullMoveCounter -= (m_colorToMove == White) ? 1 : 0;
    m_colorToMove = ~m_colorToMove;
    m_stateHistory.pop();
}

void Position::doMove(Move move)
{
    ASSERT(move != Move::Null());

    Square src = move.src();
    Square dst = move.dst();
    Color friendly = m_colorToMove;
    Color enemy = ~friendly;

    Piece srcPiece = m_pieces[src];
    PieceType srcType = pieceTypeOf(srcPiece);
    Piece capturedPiece = m_pieces[dst];
    bool isCapture = (capturedPiece != NonePiece);

    ASSERT(colorOf(srcPiece) == friendly);

    StateInfo newSt;
    const StateInfo& prevSt = stateInfo();
    std::memcpy(&newSt, &prevSt, offsetof(StateInfo, enPassantSquare));

    newSt.capturedPiece     = capturedPiece;
    newSt.enPassantSquare   = NoneSquare;
    newSt.prevMove          = move;

    newSt.halfMoveClock ++;

    if (isCapture)
    {
        ASSERT(colorOf(capturedPiece) == enemy);
        ASSERT(pieceTypeOf(capturedPiece) != King);

        removePiece(dst, capturedPiece);

        int32 PsqtEntry = PSQT[capturedPiece][dst];
        newSt.mgPSQT -= mg(PsqtEntry);
        newSt.egPSQT -= eg(PsqtEntry);

        newSt.zobristKey ^= Zobrist::piece[capturedPiece][dst];
        newSt.material[enemy] -= PieceValue[capturedPiece];
        newSt.phase -= PiecePhaseValue[capturedPiece];

        newSt.halfMoveClock = 0;

        // update pawnKey
#ifndef DISABLE_PAWN_HASH
        if (pieceTypeOf(capturedPiece) == Pawn)
            newSt.pawnKey ^= Zobrist::piece[capturedPiece][dst];
#endif

    }
    if (move.promotion())
    {
        ASSERT(srcPiece == makePiece(friendly, Pawn));

        Piece promoPiece = makePiece(friendly, move.promotionType());

        int32 srcPSQT = PSQT[srcPiece][src];
        int32 promotionPSQT = PSQT[promoPiece][dst];
        newSt.mgPSQT += mg(promotionPSQT) - mg(srcPSQT);
        newSt.egPSQT += eg(promotionPSQT) - eg(srcPSQT);

        newSt.material[friendly] += PieceValue[promoPiece] - PieceValue[Pawn];
        newSt.zobristKey ^= Zobrist::piece[srcPiece][src];
        newSt.zobristKey ^= Zobrist::piece[promoPiece][dst];
        newSt.phase += PiecePhaseValue[promoPiece];

#ifndef DISABLE_PAWN_HASH
        newSt.pawnKey ^= Zobrist::piece[srcPiece][src];
#endif

        removePiece(src, srcPiece);
        placePiece(dst, promoPiece);

    }
    else if (move.enPassant())
    {
        // cpature pawn from en passant
        Square enemyPawnSquare = dst + ((friendly == White) ? Down : Up);
        Piece enemyPawn = makePiece(enemy, Pawn);
        int32 epPSQT = PSQT[enemyPawn][enemyPawnSquare];

        ASSERT(m_pieces[enemyPawnSquare] == enemyPawn);

        newSt.zobristKey ^= Zobrist::piece[enemyPawn][enemyPawnSquare];
        newSt.material[enemy] -= PieceValue[Pawn];
        newSt.mgPSQT -= mg(epPSQT);
        newSt.egPSQT -= eg(epPSQT);

#ifndef DISABLE_PAWN_HASH
        newSt.pawnKey ^= Zobrist::piece[enemyPawn][enemyPawnSquare];
#endif

        removePiece(enemyPawnSquare, enemyPawn);
    }
    else if (move.castling())
    {
        // Move rook in castling
        CastlingSide side = move.castlingSide();
        CastlingRight right = makeCastlingRight(friendly, side);
        Piece friendlyRook = makePiece(friendly, Rook);
        Square rookSrc = CastlingRookSrc[right];
        Square rookDst = CastlingRookDst[right];

        ASSERT(srcType == King && capturedPiece == NonePiece);
        ASSERT(m_pieces[rookSrc] == friendlyRook && m_pieces[rookDst] == NonePiece);

        int32 srcPSQT = PSQT[friendlyRook][rookSrc];
        int32 dstPSQT = PSQT[friendlyRook][rookDst];

        newSt.zobristKey ^= Zobrist::piece[friendlyRook][rookSrc];
        newSt.zobristKey ^= Zobrist::piece[friendlyRook][rookDst];
        newSt.mgPSQT += mg(dstPSQT) - mg(srcPSQT);
        newSt.egPSQT += eg(dstPSQT) - eg(srcPSQT);

        movePiece(rookSrc, rookDst, friendlyRook);
    }

    // move piece normally
    if (!move.promotion())
    {
        int srcEntry = Evaluation::PSQT[srcPiece][src];
        int dstEntry = Evaluation::PSQT[srcPiece][dst];

        // update zobrist key for piece movement
        newSt.zobristKey ^= Zobrist::piece[srcPiece][src];
        newSt.zobristKey ^= Zobrist::piece[srcPiece][dst];
        newSt.mgPSQT += Evaluation::mg(dstEntry) - Evaluation::mg(srcEntry);
        newSt.egPSQT += Evaluation::eg(dstEntry) - Evaluation::eg(srcEntry);

#ifndef DISABLE_PAWN_HASH
        if (srcType == Pawn) newSt.pawnKey ^= Zobrist::piece[srcPiece][src] | Zobrist::piece[srcPiece][dst];
#endif

        movePiece(src, dst, srcPiece);
    }

    // update Castling rigths when king or rook moves or get captured
    if (srcType == King)
    {
        newSt.zobristKey ^= Zobrist::castling[newSt.castlingRights.data];
        newSt.castlingRights.disallowCastling(makeCastlingRight(friendly, KingSide));
        newSt.castlingRights.disallowCastling(makeCastlingRight(friendly, QueenSide));
        newSt.zobristKey ^= Zobrist::castling[newSt.castlingRights.data];
    }
    if (srcType == Rook || capturedPiece == makePiece(enemy, Rook))
    {
        Square rookSquare = (srcType == Rook) ? src : dst;
        CastlingRight right = castlingRightByRookSrc(rookSquare);
        newSt.zobristKey ^= Zobrist::castling[newSt.castlingRights.data];
        if (isValid(right)) newSt.castlingRights.disallowCastling(right);
        newSt.zobristKey ^= Zobrist::castling[newSt.castlingRights.data];
    }

    // remove previous epSquare from zobrist key
    File epFile = fileOf(prevSt.enPassantSquare);
    uint64 epZobrist = Zobrist::enPassant[epFile];
    newSt.zobristKey ^= epZobrist;

    if (srcType == Pawn)
    {
        // reset halfmove clock on pawn push
        newSt.halfMoveClock = 0;
        bool doublePawnPush = std::abs(int(src) - int(dst)) == 2 * Up;

        // update en passant square on double pawn push
        if (doublePawnPush)
        {
            Square epSquare = src + pawnPushDir(friendly);
            newSt.enPassantSquare = epSquare;
            File epFile = fileOf(epSquare);
            newSt.zobristKey ^= Zobrist::enPassant[epFile];
        }
    }

    m_colorToMove = enemy;
    newSt.zobristKey ^= Zobrist::side;
    m_fullMoveCounter += (friendly == Black) ? 1 : 0; // fullmove finishes after black moves

    m_stateHistory.push(std::move(newSt));
    updateCheckers();
    updatePinned();
}

void Position::undoMove()
{
    ASSERT(m_stateHistory.size() > 1);

    const StateInfo& st = stateInfo();
    Move move = st.prevMove;

    ASSERT(move != Move::Null());

    Square src = move.src();
    Square dst = move.dst();
    Color enemy = m_colorToMove;       // side that just moved
    Color friendly = ~m_colorToMove;   // side to move after undo

    Piece piece = m_pieces[dst];
    PieceType pieceType = pieceTypeOf(piece);
    Piece capturedPiece = st.capturedPiece;
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
        ASSERT(colorOf(capturedPiece) == enemy);
        placePiece(dst, capturedPiece);
    }

    m_fullMoveCounter -= (friendly == White) ? 1 : 0;
    m_colorToMove = ~m_colorToMove;
    m_stateHistory.pop();
}
}