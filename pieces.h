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
protected:
	PieceType _t;
	bool      _s;
	char	  _c;
	Pos		  _p;

protected:
	static std::map<Dir,Offset> s_os;

protected:
	Piece(PieceType t, bool s, const char* c)
	: _t{t}, _s{s}, _c{c[s]}
	{}

public:
	bool is_on_move(bool m) { return _s == m; }

	const char toChar() const {return _c;}

	Pos getPos() { return _p; }

	void setPos(Rank r, File f) {
		_p.r = r;
		_p.f = f;
	}

	void setPos(Pos p) {
		_p = p;
	}

	static std::shared_ptr<Piece> create(PieceType pt, bool s);

    virtual MoveList getValidMoves() = 0;
	virtual ~Piece() {}
};

class King : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	King(bool s)
	: Piece(PT_KING, s, "Kk")
	{}
    virtual MoveList getValidMoves();
};

class Queen : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Queen(bool s)
	: Piece(PT_QUEEN, s, "Qq")
	{}
    virtual MoveList getValidMoves();
};

class Rook : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Rook(bool s)
	: Piece(PT_ROOK, s, "Rr")
	{}
    virtual MoveList getValidMoves();
};

class Knight : public Piece
{
private:
	static std::vector<Offset> _o;
public:
	Knight(bool s)
	: Piece(PT_KNIGHT, s, "Nn")
	{}
    virtual MoveList getValidMoves();
};

class Bishop : public Piece
{
private:
	static std::vector<Dir> _d;
public:
	Bishop(bool s)
	: Piece(PT_BISHOP, s, "Bb")
	{}
    virtual MoveList getValidMoves();
};

class Pawn : public Piece
{
private:
	bool	_off;

public:
	Pawn(bool s, bool off)
	: Piece((off) ? PT_PAWN_OFF : PT_PAWN, s, "Pp"), _off(off)
	{
	}
    virtual MoveList getValidMoves();
};
