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
    wks_castle_enabled = true;
    wqs_castle_enabled = true;
    bks_castle_enabled = true;
    bqs_castle_enabled = true;
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
    _p.i                    = 0;
    _p.f.piece_cnt          = static_cast<uint32_t>(piece_cnt);
    _p.f.on_move            = static_cast<uint32_t>(on_move);
    _p.f.end_game_reason    = static_cast<short>(end_game_reason);
    _p.f.wks_castle_enabled = static_cast<uint32_t>(wks_castle_enabled);
    _p.f.wqs_castle_enabled = static_cast<uint32_t>(wqs_castle_enabled);
    _p.f.bks_castle_enabled = static_cast<uint32_t>(bks_castle_enabled);
    _p.f.bqs_castle_enabled = static_cast<uint32_t>(bqs_castle_enabled);
    _p.f.en_passant_file    = static_cast<uint32_t>(en_passant_file);

    return _p;
}