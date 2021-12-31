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
