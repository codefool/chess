#include "constants.h"

GameInfoPacked::GameInfoPacked()
: i{0}
{
    f.wks_castle_enabled =
    f.wqs_castle_enabled =
    f.bks_castle_enabled =
    f.bqs_castle_enabled = 1;
    f.piece_cnt          = 32;
}

GameInfo::GameInfo()
{
    init();
}

void GameInfo::init()
{
	setPieceCnt(32);
	setOnMove(SIDE_WHITE);
	setWksCastleEnabled(true);
	setWqsCastleEnabled(true);
	setBksCastleEnabled(true);
	setBqsCastleEnabled(true);
	setEnPassantFile(EP_NONE);
    pack();
}

GameInfo& GameInfo::unpack(const GameInfoPacked& p)
{
    setPieceCnt(static_cast<short>(p.f.piece_cnt));
    setOnMove(static_cast<Side>(p.f.on_move));
    setEnPassantFile(static_cast<EnPassantFile>(p.f.en_passant_file));
    setWksCastleEnabled(p.f.wks_castle_enabled == 1);
    setWqsCastleEnabled(p.f.wqs_castle_enabled == 1);
    setBksCastleEnabled(p.f.bks_castle_enabled == 1);
    setBqsCastleEnabled(p.f.bqs_castle_enabled == 1);
    return *this;
}

const GameInfoPacked GameInfo::pack() const
{
    GameInfoPacked p;
    p.i                    = 0;
    p.f.piece_cnt          = static_cast<uint32_t>(piece_cnt);
    p.f.on_move            = static_cast<uint32_t>(on_move);
    p.f.wks_castle_enabled = static_cast<uint32_t>(wks_castle_enabled);
    p.f.wqs_castle_enabled = static_cast<uint32_t>(wqs_castle_enabled);
    p.f.bks_castle_enabled = static_cast<uint32_t>(bks_castle_enabled);
    p.f.bqs_castle_enabled = static_cast<uint32_t>(bqs_castle_enabled);
    p.f.en_passant_file    = static_cast<uint32_t>(en_passant_file);

    return p;
}

std::ostream& operator<<(std::ostream& os, const GameInfo& o) {
    os << o.getPieceCnt() << ' '
       << "WB"[o.getOnMove()] << ' '
       << "CW" << "Kx"[o.isWksCastleEnabled()] << "Qx"[o.isWqsCastleEnabled()] << ' '
       << "CB" << "Kx"[o.isBksCastleEnabled()] << "Qx"[o.isBqsCastleEnabled()] << ' '
       << "EP ";
       if (o.enPassantExists()) {
           os << g_file(o.getEnPassantFile());
       } else {
           os << "x";
       }

    return os;
}