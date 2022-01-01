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
		uint32_t piece_cnt          :  5;
		uint32_t on_move            :  1; // 0=white on move, 1=black
		//
		uint32_t end_game_reason    :  4;
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant_latch   :  1; // pawn exists subject to en passant
		uint32_t en_passant_file    :  3; // file number where pawn rests
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_disabled:  1; // WK or WKR has moved
		uint32_t wqs_castle_disabled:  1; // WK or WQR has moved
		uint32_t bks_castle_disabled:  1; // BK or BKR has moved
		uint32_t bqs_castle_disabled:  1; // BK or BQR has moved
		//
		uint32_t unused             : 14; // for future use
	} f;
	GameInfoPacked() : i{0} {}
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
	bool          en_passant_latch;
	uint32_t      en_passant_file;
	// Castling is possible only if the participating pieces have not
	// moved (among other rules, but have nothing to do with prior movement.)
	bool          wks_castle_disabled;
	bool          wqs_castle_disabled;
	bool          bks_castle_disabled;
	bool          bqs_castle_disabled;
public:
	short getPieceCnt() { return piece_cnt; }
	void setPieceCnt(short cnt) { piece_cnt = cnt; }
	Side getOnMove() { return on_move; }
	void setOnMove(Side m) { on_move = m; }
	bool isGameActive() { return getEndGameReason() == EGR_NONE; }
	EndGameReason getEndGameReason() { return end_game_reason; }
	void setEndGameReason(EndGameReason r) { end_game_reason = r; }
	bool isWksCastleDisabled() { return wks_castle_disabled; }
	void setWksCastleDisabled(bool s) { wks_castle_disabled = s;}
	bool isWqsCastleDisabled() { return wqs_castle_disabled; }
	void setWqsCastleDisabled(bool s) { wqs_castle_disabled = s;}
	bool isBksCastleDisabled() { return bks_castle_disabled; }
	void setBksCastleDisabled(bool s) { bks_castle_disabled = s;}
	bool isBqsCastleDisabled() { return bqs_castle_disabled; }
	void setBqsCastleDisabled(bool s) { bqs_castle_disabled = s;}
	bool getEnPassantLatch() { return en_passant_latch;}
	void setEnPassantLatch(bool s) {
		if (!s) {
			// if clearing the latch, set the file to zero
			setEnPassantFile(Fa);
		}
		en_passant_latch = s;
	}
	short getEnPassantFile() { return en_passant_file; }
	void setEnPassantFile(File fi) { en_passant_file = static_cast<uint32_t>(fi); }
	void setEnPassantFile(short fi) { en_passant_file = fi; }

	GameInfo decode(GameInfoPacked p) {
		setPieceCnt(static_cast<short>(p.f.piece_cnt));
		setOnMove(static_cast<Side>(p.f.on_move));
		setEndGameReason(static_cast<EndGameReason>(p.f.end_game_reason));
		setEnPassantLatch(p.f.en_passant_latch == 1);
		setEnPassantFile(static_cast<short>(p.f.en_passant_file));
		setWksCastleDisabled(p.f.wks_castle_disabled == 1);
		setWqsCastleDisabled(p.f.wqs_castle_disabled == 1);
		setBksCastleDisabled(p.f.bks_castle_disabled == 1);
		setBqsCastleDisabled(p.f.bqs_castle_disabled == 1);
		return *this;
	}

	GameInfoPacked encode() {
		GameInfoPacked p;

	    p.f.piece_cnt = static_cast<uint32_t>(piece_cnt);
		p.f.on_move = static_cast<uint32_t>(on_move);
		p.f.end_game_reason = static_cast<short>(end_game_reason);
		p.f.wks_castle_disabled = static_cast<uint32_t>(wks_castle_disabled);
		p.f.wqs_castle_disabled = static_cast<uint32_t>(wqs_castle_disabled);
		p.f.bks_castle_disabled = static_cast<uint32_t>(bks_castle_disabled);
		p.f.bqs_castle_disabled = static_cast<uint32_t>(bqs_castle_disabled);
		p.f.en_passant_latch    = static_cast<uint32_t>(en_passant_latch);
		p.f.en_passant_file     = static_cast<uint32_t>(en_passant_file);

		return p;
	}
};
# pragma pack()
