#include "dreid.h"

namespace dreid {

GameInfoPacked::GameInfoPacked()
: i{0}
{
    f.castle_rights = 0x0f;
    f.piece_cnt     = 32;
}

GameInfo::GameInfo()
{
    init();
}

GameInfo::GameInfo(const GameInfo& o)
{
    unpack(o.pack());
}

short GameInfo::getPieceCnt() const
{
    return piece_cnt;
}

void GameInfo::setPieceCnt(short cnt)
{
    piece_cnt = cnt;
}

void GameInfo::decPieceCnt()
{
    piece_cnt--;
}

Side GameInfo::getOnMove() const
{
    return on_move;
}

void GameInfo::setOnMove(Side m)
{
    on_move = m;
}

void GameInfo::toggleOnMove()
{
    on_move = (on_move == SIDE_WHITE) ? SIDE_BLACK : SIDE_WHITE;
}

uint8_t GameInfo::getCastleRights() const
{
    return castle_rights;
}

bool GameInfo::hasCastleRights() const
{
    return castle_rights != 0;
}

bool GameInfo::hasCastleRight(CastleRight which) const
{
    return (castle_rights & which) != 0;
}

void GameInfo::enableCastleRight(CastleRight which)
{
    castle_rights |= which;
}

void GameInfo::revokeCastleRight(CastleRight which)
{
    castle_rights &= ~which;
}

void GameInfo::revokeCastleRights(Side s, int which)
{
    if (s == SIDE_BLACK)
        which <<= 2;
    castle_rights &= ~which;
}

bool GameInfo::hasEnPassant() const
{
    return (en_passant_file & EP_HERE_MASK) != 0;
}

File GameInfo::getEnPassantFile() const
{
    return static_cast<File>(en_passant_file & EP_FILE_MASK);
}

void GameInfo::setEnPassantFile(EnPassantFile ep)
{
    en_passant_file = ep;
}

void GameInfo::setEnPassantFile(File f)
{
    setEnPassantFile(static_cast<EnPassantFile>(EP_HERE_MASK | f));
}

bool GameInfo::operator==(const GameInfo& o) const
{
    return pack() == o.pack();
}

void GameInfo::init()
{
	setPieceCnt(32);
	setOnMove(SIDE_WHITE);
    castle_rights = 0x0f;
	setEnPassantFile(EP_NONE);
    pack();
}

GameInfo& GameInfo::unpack(const GameInfoPacked& p)
{
    setPieceCnt(static_cast<short>(p.f.piece_cnt));
    setOnMove(static_cast<Side>(p.f.on_move));
    setEnPassantFile(static_cast<EnPassantFile>(p.f.en_passant_file));
    castle_rights = p.f.castle_rights;
    return *this;
}

const GameInfoPacked GameInfo::pack() const
{
    GameInfoPacked p;
    p.i                 = 0;
    p.f.piece_cnt       = static_cast<uint32_t>(piece_cnt);
    p.f.on_move         = static_cast<uint32_t>(on_move);
    p.f.castle_rights   = static_cast<uint32_t>(castle_rights);
    p.f.en_passant_file = static_cast<uint32_t>(en_passant_file);

    return p;
}

std::ostream& operator<<(std::ostream& os, const GameInfo& o) {
    os << o.getPieceCnt() << ' '
       << "WB"[o.getOnMove()] << ' '
       << "CW" << "Kx"[o.hasCastleRight(CR_WHITE_KING_SIDE)]
               << "Qx"[o.hasCastleRight(CR_WHITE_QUEEN_SIDE)] << ' '
       << "CB" << "Kx"[o.hasCastleRight(CR_BLACK_KING_SIDE)]
               << "Qx"[o.hasCastleRight(CR_BLACK_QUEEN_SIDE)] << ' '
       << "EP ";
       if (o.hasEnPassant()) {
           os << g_file(o.getEnPassantFile());
       } else {
           os << "x";
       }

    return os;
}

} // namespace dreid