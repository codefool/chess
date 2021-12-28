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

union GameInfo {
private:
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
public:
	GameInfo() : i{0} {}

	uint32_t getWord() { return i;}
	void setWord(uint32_t w) {i = w;}
	//
	short getPieceCnt() { return f.piece_cnt; }
	void setPieceCnt(short cnt) { f.piece_cnt = static_cast<uint32_t>(cnt); }
	Side getOnMove() { return static_cast<Side>(f.on_move); }
	void setOnMove(Side m) { f.on_move = static_cast<uint32_t>(m); }
	bool isGameActive() { return getEndGameReason() == EGR_NONE; }
	EndGameReason getEndGameReason() { return static_cast<EndGameReason>(f.end_game_reason); }
	void setEndGameReason(EndGameReason r) { f.end_game_reason = static_cast<short>(r); }
	bool isWksCastleEnabled() { return f.wks_castle_disabled == 0; }
	void setWksCastleEnabled(bool s) { f.wks_castle_disabled = static_cast<uint32_t>(s);}
	bool isWqsCastleEnabled() { return f.wqs_castle_disabled == 0; }
	void setWqsCastleEnabled(bool s) { f.wqs_castle_disabled = static_cast<uint32_t>(s);}
	bool isBksCastleEnabled() { return f.bks_castle_disabled == 0; }
	void setBksCastleEnabled(bool s) { f.bks_castle_disabled = static_cast<uint32_t>(s);}
	bool isBqsCastleEnabled() { return f.bqs_castle_disabled == 0; }
	void setBqsCastleEnabled(bool s) { f.bqs_castle_disabled = static_cast<uint32_t>(s);}
	bool getEnPassantLatch() { return f.en_passant_latch == 1;}
	void setEnPassantLatch(bool s) {
		uint32_t v = 1;
		if (!s) {
			// if clearing the latch, set the file to zero
			v = 0;
			setEnPassantFile(Fa);
		}
		f.en_passant_latch = v;
	}
	short getEnPassantFile() { return (short)f.en_passant_file; }
	void setEnPassantFile(File fi) { f.en_passant_file = static_cast<uint32_t>(fi); }
};
# pragma pack()
