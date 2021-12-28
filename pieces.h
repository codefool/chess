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
#include "board.h"

// base class of all pieces

class Piece
{
protected:
	PieceType _t;
	Side      _s;
	char	  _c;
	Pos		  _p;

protected:
	static std::map<Dir,Offset> s_os;

protected:
	Piece(PieceType t, Side s, const char* c)
	: _t{t}, _s{s}, _c{c[s]}
	{}

public:
	bool is_on_move(Side m) { return _s == m; }

	const char toChar() const {return _c;}

	Pos& getPos() { return _p; }

	void setPos(Rank r, File f) {
		_p.r = r;
		_p.f = f;
	}

	void setPos(Pos p) {
		_p = p;
	}

	static PiecePtr create(PieceType pt, Side s);

    virtual MoveList getValidMoves(Board& b) = 0;
	virtual ~Piece() {}
};

class King : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	King(Side s)
	: Piece(PT_KING, s, "Kk")
	{}
    virtual MoveList getValidMoves(Board& b);
};

class Queen : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Queen(Side s)
	: Piece(PT_QUEEN, s, "Qq")
	{}
    virtual MoveList getValidMoves(Board& b);
};

class Rook : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Rook(Side s)
	: Piece(PT_ROOK, s, "Rr")
	{}
    virtual MoveList getValidMoves(Board& b);
};

class Knight : public Piece
{
private:
	static std::vector<Offset> _o;
public:
	Knight(Side s)
	: Piece(PT_KNIGHT, s, "Nn")
	{}
    virtual MoveList getValidMoves(Board& b);
};

class Bishop : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Bishop(Side s)
	: Piece(PT_BISHOP, s, "Bb")
	{}
    virtual MoveList getValidMoves(Board& b);
};

class Pawn : public Piece
{
private:
	bool	_off;

public:
	Pawn(Side s, bool off)
	: Piece((off) ? PT_PAWN_OFF : PT_PAWN, s, "Pp"), _off(off)
	{
	}
    virtual MoveList getValidMoves(Board& b);
};
