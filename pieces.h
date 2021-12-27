// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "constants.h"

// base class of all pieces
class Piece
{
private:
	PieceType _t;
	bool      _s;
	char	  _c;

protected:
	static std::map<Dir,Offset> s_os;

protected:
	Piece(PieceType t, bool s, const char* c)
	: _t{t}, _s{s}, _c{c[s]}
	{}

public:
	bool is_on_move(bool m) { return _s == m; }
	bool is_not_on_move(bool m) { return _s != m; }

	const char toChar() const {return _c;}

	static std::shared_ptr<Piece*> create(PieceType pt, bool s);
	static std::shared_ptr<Piece*> createWhite(PieceType pt) { return Piece::create(pt, false);}
	static std::shared_ptr<Piece*> createBlack(PieceType pt) { return Piece::create(pt, true);}
};

class King : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	King(bool s)
	: Piece(PT_KING, s, "Kk")
	{}
};

class Queen : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Queen(bool s)
	: Piece(PT_QUEEN, s, "Qq")
	{}
};

class Rook : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Rook(bool s)
	: Piece(PT_ROOK, s, "Rr")
	{}
};

class Knight : public Piece
{
private:
	static std::vector<Offset> _o;
public:
	Knight(bool s)
	: Piece(PT_KNIGHT, s, "Nn")
	{}
};

class Bishop : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Bishop(bool s)
	: Piece(PT_BISHOP, s, "Bb")
	{}
};

// pawns are filthy animals
//
// Movement rules are wack.
// 1. On initial move, can move 1 or 2 spaces
// 2. Can only capture UPL or UPR (for white, opp for black)
// 3. Can en passant capture if:
//    a. pawn is on its 5th rank
//    b. has never left its own file, and
//    c. target pawn moved 2 spaces on prior turn.
// 4. If moved 2 spaces, flag as en passant target.
// 5. Upon reaching its eight rank is promoted.
//
class Pawn : public Piece
{
private:
	bool	_off;

public:
	Pawn(bool s, bool off)
	: Piece((off) ? PT_PAWN_OFF : PT_PAWN, s, "Pp"), _off(off)
	{
	}
};
