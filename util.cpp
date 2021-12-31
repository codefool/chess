// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "constants.h"

std::ostream& operator<<(std::ostream& os, const Pos& p) {
	char r = "12345678"[p.r()];
	char f = "abcdefgh"[p.f()];

	os << f << r;

	return os;
}

// the idea will be to display the move in alebraic notation [38]
std::ostream& operator<<(std::ostream& os, const Move& m) {
	os << Pos(m.f.source);

	switch(m.f.action) {
		case MV_CAPTURE:
		case MV_EN_PASSANT:
			os << 'x';
			break;
	}

	os << Pos(m.f.target);

	switch(m.f.action) {
		case MV_EN_PASSANT:
			os << "e.p.";
			break;
		case MV_CHECK:
			os << '+';
			break;
		case MV_CHECKMATE:
			os << "++";
			break;
	}

	return os;
}

