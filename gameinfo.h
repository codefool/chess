// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once
#include <cstdint>
#include "constants.h"
/*
xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... ..xx .... .... .... .... .... .... = drawn game reason
.... .... x... .... .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .x.. .... .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... ..x. .... .... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... ...x .... .... .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... x... .... .... .... .... = en passant latch
.... .... .... .xxx .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... .... x... .... .... .... = drawn game
.... .... .... .... .xx. .... .... .... = 14D reason
.... .... .... .... ...? ???? ???? ???? = unused (zeroes)

It is imperitive that all unused bits - or bits that are out of scope - be set to 0
*/

#pragma pack(1)

union GameInfo {
private:
	uint32_t i;
	struct {
		// number of active pieces on the board (0..31)
		uint32_t piece_cnt          :  5;
		uint32_t on_move            :  1; // 0=white on move, 1=black
		// drawn reason
		// 00 - stalemate (14A)
		// 01 - triple of position (14C)
		// 10 - no material (14D)
		// 11 - 50 move rule (14F)
		uint32_t drawn_reason       :  2;
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_disabled:  1;	// WK or WKR has moved
		uint32_t wqs_castle_disabled:  1;	// WK or WQR has moved
		uint32_t bks_castle_disabled:  1;	// BK or BKR has moved
		uint32_t bqs_castle_disabled:  1;	// BK or BQR has moved
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant_latch   :  1;   // pawn subject to en passant
		uint32_t en_passant_file    :  3;   // file number where pawn rests
		//
		uint32_t drawn_game         :  1;	// 1=drawn game (terminal)
		// if drawn reason is 14D, this is the sub-reason:
		// 00 - King v King (14D1)
		// 01 - King v King with Bishop/Knight (14D2)
		// 10 - King with Bishop/Knight v King with Bishop/Knight (14D3)
		uint32_t reason_14d         :  2;
		uint32_t unused             : 13; // for future use
	} f;
public:
	GameInfo() : i{0} {}

	uint32_t getWord() { return i;}
	void setWord(uint32_t w) {i = w;}
	//
	short getPieceCnt() { return f.piece_cnt; }
	void setPieceCnt(short cnt) { f.piece_cnt = (uint32_t)cnt; }
	bool  getOnMove() { return f.on_move == 0x01;}
	void setOnMove(bool b) { f.on_move = (b) ? 0x01 : 0x00; }
	DrawReason getDrawnReason() { return (DrawReason)f.drawn_reason; }
	void setDrawnReason(DrawReason r) { 
		f.drawn_reason = (short)r; 
		if (r != R_14D_NO_MATERIAL)
			// if reason is not 14D, then clear the 14D reason
			set14DReason(R_14D_NONE);
	}
	bool isWksCastleEnabled() { return f.wks_castle_disabled == 0; }
	void setWksCastleEnabled(bool s) { f.wks_castle_disabled = (s) ? 1 : 0;}
	bool isWqsCastleEnabled() { return f.wqs_castle_disabled == 0; }
	void setWqsCastleEnabled(bool s) { f.wqs_castle_disabled = (s) ? 1 : 0;}
	bool isBksCastleEnabled() { return f.bks_castle_disabled == 0; }
	void setBksCastleEnabled(bool s) { f.bks_castle_disabled = (s) ? 1 : 0;}
	bool isBqsCastleEnabled() { return f.bqs_castle_disabled == 0; }
	void setBqsCastleEnabled(bool s) { f.bqs_castle_disabled = (s) ? 1 : 0;}
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
	void setEnPassantFile(File fi) { f.en_passant_file = (uint32_t)fi; }
	bool getDrawnGameState() { return f.drawn_game == 1; }
	void setDrawnGameState(bool s) { f.drawn_game = (s) ? 1 : 0;}
	Draw14dReason get14DReason() { return (Draw14dReason)f.reason_14d;}
	void set14DReason(Draw14dReason r) { f.reason_14d = (uint32_t)r; }
};
# pragma pack()
