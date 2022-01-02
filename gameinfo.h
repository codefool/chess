// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once
#include "constants.h"
/*
xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... ..xx xx.. .... .... .... .... .... = end game reason
.... .... ..x. .... .... .... .... .... = en passant latch
.... .... ...x xx.. .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... ..x. .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .... ...x .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... .... .... x... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... .... .... .x.. .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... .... ..?? ???? ???? ???? = unused (zeroes)

It is imperitive that all unused bits - or bits that are out of scope - be set to 0
*/

#pragma pack(1)

union GameInfoPacked {
	uint32_t i;
	struct {
		// number of active pieces on the board (1..31)
		uint32_t piece_cnt        :  5;
		uint32_t on_move          :  1; // 0=white on move, 1=black
		//
		uint32_t end_game_reason  :  4;
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant        :  4; // file number where pawn rests
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_enabled:  1; // neither WK or WKR has moved
		uint32_t wqs_castle_enabled:  1; // neither WK or WQR has moved
		uint32_t bks_castle_enabled:  1; // neither BK or BKR has moved
		uint32_t bqs_castle_enabled:  1; // neither BK or BQR has moved
		//
		uint32_t unused             : 14; // for future use
	} f;
	GameInfoPacked() : i{0} {
		f.wks_castle_enabled =
		f.wqs_castle_enabled =
		f.bks_castle_enabled =
		f.bqs_castle_enabled = 1;
	}
};

class GameInfo {
private:
	// number of active pieces on the board (1..31)
	short         piece_cnt;
	Side          on_move; // 0=white on move, 1=black
	//
	EndGameReason end_game_reason;
	// en passant
	// If set, the pawn that rests on file en_passant_file moved two
	// positions. This signals that a pawn subject to en passant capture
	// exists, and does not mean that there is an opposing pawn that can
	// affect it.
	EnPassant     en_passant;
	// Castling is possible only if the participating pieces have not
	// moved (among other rules, but have nothing to do with prior movement.)
	bool          wks_castle_enabled;
	bool          wqs_castle_enabled;
	bool          bks_castle_enabled;
	bool          bqs_castle_enabled;
public:
	short getPieceCnt() { return piece_cnt; }
	void setPieceCnt(short cnt) { piece_cnt = cnt; }
	Side getOnMove() { return on_move; }
	void setOnMove(Side m) { on_move = m; }
	bool isGameActive() { return getEndGameReason() == EGR_NONE; }
	EndGameReason getEndGameReason() { return end_game_reason; }
	void setEndGameReason(EndGameReason r) { end_game_reason = r; }
	bool isWksCastleEnabled() { return wks_castle_enabled; }
	void setWksCastleEnabled(bool s) { wks_castle_enabled = s;}
	bool isWqsCastleEnabled() { return wqs_castle_enabled; }
	void setWqsCastleEnabled(bool s) { wqs_castle_enabled = s;}
	bool isBksCastleEnabled() { return bks_castle_enabled; }
	void setBksCastleEnabled(bool s) { bks_castle_enabled = s;}
	bool isBqsCastleEnabled() { return bqs_castle_enabled; }
	void setBqsCastleEnabled(bool s) { bqs_castle_enabled = s;}
	bool enPassantExists() { return en_passant != EP_NONE;}
	EnPassant getEnPassant() { return en_passant; }
	void setEnPassant(EnPassant ep) { en_passant = ep; }
	short getEnPassantFile() { return en_passant & EP_FILE; }

	GameInfo decode(GameInfoPacked p) {
		setPieceCnt(static_cast<short>(p.f.piece_cnt));
		setOnMove(static_cast<Side>(p.f.on_move));
		setEndGameReason(static_cast<EndGameReason>(p.f.end_game_reason));
		setEnPassant(static_cast<EnPassant>(en_passant));
		setWksCastleEnabled(p.f.wks_castle_enabled == 1);
		setWqsCastleEnabled(p.f.wqs_castle_enabled == 1);
		setBksCastleEnabled(p.f.bks_castle_enabled == 1);
		setBqsCastleEnabled(p.f.bqs_castle_enabled == 1);
		return *this;
	}

	GameInfoPacked encode() {
		GameInfoPacked p;

		p.i                    = 0;
	    p.f.piece_cnt          = static_cast<uint32_t>(piece_cnt);
		p.f.on_move            = static_cast<uint32_t>(on_move);
		p.f.end_game_reason    = static_cast<short>(end_game_reason);
		p.f.wks_castle_enabled = static_cast<uint32_t>(wks_castle_enabled);
		p.f.wqs_castle_enabled = static_cast<uint32_t>(wqs_castle_enabled);
		p.f.bks_castle_enabled = static_cast<uint32_t>(bks_castle_enabled);
		p.f.bqs_castle_enabled = static_cast<uint32_t>(bqs_castle_enabled);
		p.f.en_passant         = static_cast<uint32_t>(en_passant);

		return p;
	}
};
# pragma pack()
