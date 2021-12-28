// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

std::vector<Dir> King::_d = {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL};

MoveList King::getValidMoves() {
    MoveList ret;

    short r = _p.r;
    short f = _p.f;

    for(auto d : _d) {
        Offset o = s_os[d];
        r += o.dr;
        f += o.df;

    }


    return ret;
}
