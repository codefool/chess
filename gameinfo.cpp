#include "gameinfo.h"


GameInfoPacked::GameInfoPacked()
: i{0}
{
    f.wks_castle_enabled =
    f.wqs_castle_enabled =
    f.bks_castle_enabled =
    f.bqs_castle_enabled = 1;
}

GameInfo::GameInfo()
{
    init();
}

void GameInfo::init()
{
	setPieceCnt(32);
	setOnMove(SIDE_WHITE);
	setEndGameReason(EGR_NONE);
	setWksCastleEnabled(true);
	setWqsCastleEnabled(true);
	setBksCastleEnabled(true);
	setBqsCastleEnabled(true);
	setEnPassantFile(EP_NONE);
    encode();
}

GameInfo& GameInfo::decode(const GameInfoPacked& p)
{
    setPieceCnt(static_cast<short>(p.f.piece_cnt));
    setOnMove(static_cast<Side>(p.f.on_move));
    setEndGameReason(static_cast<EndGameReason>(p.f.end_game_reason));
    setEnPassantFile(static_cast<EnPassantFile>(en_passant_file));
    setWksCastleEnabled(p.f.wks_castle_enabled == 1);
    setWqsCastleEnabled(p.f.wqs_castle_enabled == 1);
    setBksCastleEnabled(p.f.bks_castle_enabled == 1);
    setBqsCastleEnabled(p.f.bqs_castle_enabled == 1);
    return *this;
}

GameInfoPacked& GameInfo::encode()
{
    _p = encode_c();
    return _p;
}

const GameInfoPacked GameInfo::encode_c() const
{
    GameInfoPacked p;
    p.i                    = 0;
    p.f.piece_cnt          = static_cast<uint32_t>(piece_cnt);
    p.f.on_move            = static_cast<uint32_t>(on_move);
    p.f.end_game_reason    = static_cast<uint32_t>(end_game_reason);
    p.f.wks_castle_enabled = static_cast<uint32_t>(wks_castle_enabled);
    p.f.wqs_castle_enabled = static_cast<uint32_t>(wqs_castle_enabled);
    p.f.bks_castle_enabled = static_cast<uint32_t>(bks_castle_enabled);
    p.f.bqs_castle_enabled = static_cast<uint32_t>(bqs_castle_enabled);
    p.f.en_passant_file    = static_cast<uint32_t>(en_passant_file);

    return p;
}