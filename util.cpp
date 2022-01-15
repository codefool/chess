// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "constants.h"

const Pos POS_WQR(R1,Fa);
const Pos POS_WKR(R1,Fh);
const Pos POS_BQR(R8,Fa);
const Pos POS_BKR(R8,Fh);

std::ostream& operator<<(std::ostream& os, const Pos& p) {
	char r = "12345678"[p.r()];
	char f = "abcdefgh"[p.f()];

	os << f << r;

	return os;
}

// the idea will be to display the move in alebraic notation [38]
std::ostream& operator<<(std::ostream& os, const Move& m) {
	// special cases
	if (m._a == MV_CASTLE_KINGSIDE)
		os << "O-O";
	else if( m._a == MV_CASTLE_QUEENSIDE)
		os << "O-O-O";
	else {
		os << m._s;

		switch(m._a) {
			case MV_CAPTURE:
			case MV_EN_PASSANT:
				os << 'x';
				break;
		}

		os << m._t;

		switch(m._a) {
			case MV_EN_PASSANT:
				os << " e.p.";
				break;
			case MV_CHECK:
				os << '+';
				break;
			case MV_CHECKMATE:
				os << "++";
				break;
			case MV_PROMOTION_BISHOP:
				os << "=B";
				break;
			case MV_PROMOTION_KNIGHT:
				os << "=N";
				break;
			case MV_PROMOTION_ROOK:
				os << "=R";
				break;
			case MV_PROMOTION_QUEEN:
				os << "=Q";
				break;
		}
	}

	return os;
}
