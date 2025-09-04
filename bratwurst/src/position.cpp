#include "position.h"

#include <ranges>

namespace Bratwurst
{

void Position::clear() noexcept
{
    std::fill(m_pieces, m_pieces + SquareNum, NonePiece);
    std::fill(m_typeBBs, m_typeBBs + PieceTypeNum, 0ULL);
    std::fill(m_colorBBs, m_colorBBs + ColorNum, 0ULL);

    while (!m_stateHistory.empty()) m_stateHistory.pop();

    m_colorToMove = NoneColor;
    m_fullMoveCounter = 0;
}
/* Parse a chess position from Forsyth-Edwards Notation (FEN)
 * FEN format: "pieces active_color castling en_passant halfmove fullmove"
 * Example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" */
std::expected<Position, Position::FenError> Position::fromFEN(const std::string& fen) noexcept
{
    Position pos;
    pos.clear();

    pos.m_stateHistory.emplace();
    StateInfo& currentStateInfo = pos.m_stateHistory.top();

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
        if (c <= '8' && c > '0')
        {
            // skip squares if digit from 0-8
            int skip = c - '0';
            currFile += skip * Right;
        } 
        else if (c == '/')
        {
            // move down one rank
            currRank --;
            if (currFile != (FileH+1) || !isValid(currRank)) return std::unexpected(FenError::InvalidFormat);
            currFile = FileA;
        }
        else
        {
            // parse char into piece and place it on the current square
            Piece piece = charToPiece(c);
            if (!isValid(piece)) return std::unexpected(FenError::InvalidPiecePlacement);

            Square currSquare = makeSquare(currFile, currRank);
            Bitboard mask = squareMask(currSquare);

            // place piece on board
            pos.m_pieces[currSquare] = piece;
            pos.m_colorBBs[colorOf(piece)] |= mask;
            pos.m_typeBBs[pieceTypeOf(piece)] |= mask;

            currFile += Right;
        }


        if (currFile > FileH+1) return std::unexpected(FenError::InvalidPiecePlacement);
    }

    if (currFile != (FileH+1) || currRank != Rank1) return std::unexpected(FenError::InvalidPiecePlacement);

    // parse side to move ("w" = White, "b = Black)
    if ((parts[1] != "w" && parts[1] != "b") || parts[1].size() != 1) return std::unexpected(FenError::InvalidColorToMove);
    pos.m_colorToMove = (parts[1] == "w") ? White : Black;

    // parse castling rights 
    currentStateInfo.castlingRights = 0;
    if (parts[2] != "-")
    {
        for (char right : parts[2])
        {
            switch (right)
            {
            // map castling right char to the right mask
            case 'K': currentStateInfo.castlingRights |= 1 << CastlingRight::WhiteOO; break;
            case 'Q': currentStateInfo.castlingRights |= 1 << CastlingRight::WhiteOOO; break;
            case 'k': currentStateInfo.castlingRights |= 1 << CastlingRight::BlackOO; break;
            case 'q': currentStateInfo.castlingRights |= 1 << CastlingRight::BlackOOO; break;
            default: return std::unexpected(FenError::InvalidCastlingRights);
            }
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
        if (fullmoveCounter < 0) return std::unexpected(FenError::InvalidFullMoveCounter);
        pos.m_fullMoveCounter = fullmoveCounter; 
    }
    catch (const std::invalid_argument& e)
    { 
        return std::unexpected(FenError::InvalidFullMoveCounter); 
    }



    return pos;
}

}