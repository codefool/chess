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
		uint32_t unused            : 11; // for future use
		//
		uint32_t end_game_reason   :  4;
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant_file   :  4; // file number where pawn rests
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_enabled:  1; // neither WK or WKR has moved
		uint32_t wqs_castle_enabled:  1; // neither WK or WQR has moved
		uint32_t bks_castle_enabled:  1; // neither BK or BKR has moved
		uint32_t bqs_castle_enabled:  1; // neither BK or BQR has moved
		//
		// number of active pieces on the board (2..32)
		uint32_t on_move           :  1; // 0=white on move, 1=black
		uint32_t piece_cnt         :  8;
	} f;
	GameInfoPacked();
	GameInfoPacked(uint32_t v)
	: i{v}
	{}
	bool operator==(const GameInfoPacked& o) const {
		return i == o.i;
	}
	bool operator<(const GameInfoPacked& o) const {
		return i < o.i;
	}
};
# pragma pack()

class GameInfo {
private:
	GameInfoPacked	_p;
	// number of active pieces on the board (1..31)
	short           piece_cnt;
	Side            on_move; // 0=white on move, 1=black
	//
	EndGameReason end_game_reason;
	// en passant
	// If set, the pawn that rests on file en_passant_file moved two
	// positions. This signals that a pawn subject to en passant capture
	// exists, and does not mean that there is an opposing pawn that can
	// affect it.
	EnPassantFile en_passant_file;
	// Castling is possible only if the participating pieces have not
	// moved (among other rules, but have nothing to do with prior movement.)
	bool          wks_castle_enabled;
	bool          wqs_castle_enabled;
	bool          bks_castle_enabled;
	bool          bqs_castle_enabled;
public:
	GameInfo();

	GameInfo(GameInfo& o)
	{
		unpack(o.pack());
	}

	void init();

	short getPieceCnt() const { return piece_cnt; }
	void setPieceCnt(short cnt) { piece_cnt = cnt; }
	Side getOnMove() const { return on_move; }
	void setOnMove(Side m) { on_move = m; }
	bool isGameActive() const { return getEndGameReason() == EGR_NONE; }
	EndGameReason getEndGameReason() const { return end_game_reason; }
	void setEndGameReason(EndGameReason r) { end_game_reason = r; }
	bool isWksCastleEnabled() const { return wks_castle_enabled; }
	void setWksCastleEnabled(bool s) { wks_castle_enabled = s;}
	bool isWqsCastleEnabled() const { return wqs_castle_enabled; }
	void setWqsCastleEnabled(bool s) { wqs_castle_enabled = s;}
	bool isBksCastleEnabled() const { return bks_castle_enabled; }
	void setBksCastleEnabled(bool s) { bks_castle_enabled = s;}
	bool isBqsCastleEnabled() const { return bqs_castle_enabled; }
	void setBqsCastleEnabled(bool s) { bqs_castle_enabled = s;}
	bool enPassantExists() const { return (en_passant_file & EP_HERE_MASK) != 0;}
	File getEnPassantFile() const { return static_cast<File>(en_passant_file & EP_FILE_MASK); }
	void setEnPassantFile(EnPassantFile ep) { en_passant_file = ep; }
	void setEnPassantFile(File f) {
		setEnPassantFile(static_cast<EnPassantFile>(EP_HERE_MASK | f));
	}

	GameInfo& unpack(const GameInfoPacked& p);
	const GameInfoPacked pack_c() const;

	GameInfoPacked& pack();

	bool operator==(const GameInfo& o) const {
		return pack_c() == o.pack_c();
	}

	friend std::ostream& operator<<(std::ostream& os, const GameInfo& o);
};
