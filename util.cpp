// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <cstring>

#include "constants.h"

const Pos POS_WQR(R1,Fa);
const Pos POS_WKR(R1,Fh);
const Pos POS_BQR(R8,Fa);
const Pos POS_BKR(R8,Fh);

Pos::Pos() : _p{0} {}

Pos::Pos(short p)
{
	_p = p;
}

Pos::Pos(Rank ra, File fi)
{
	set(ra,fi);
}

void Pos::set(Rank ra, File fi) {
	_p = (ra<<3)|fi;
}

void Pos::set(uint8_t b) {
	fromByte(b);
}

Pos Pos::operator+(const short o) {
	return Pos(_p + o);
}

Pos Pos::operator+=(const short o) {
	_p += o;
	return *this;
}

bool Pos::operator<(const Pos& o) {
	return _p < o._p;
}

bool Pos::operator<=(const Pos& o) {
	return _p <= o._p;
}

bool Pos::operator==(const Pos& o) {
	return _p == o._p;
}

uint8_t Pos::toByte() {
	return (uint8_t)(_p);
}

void Pos::fromByte(uint8_t b) {
	_p = static_cast<short>(b & 0x3f);
}

const short Pos::r() const {
	return _p >> 3;
}

const short Pos::f() const {
	return _p & 0x7;
}

const Rank Pos::rank() const {
	return static_cast<Rank>(r());
}
const File Pos::file() const {
	return static_cast<File>(f());
}

Pos Pos::withRank(Pos s, Rank r) {
	return Pos(r, s.file());
}
Pos Pos::withFile(Pos s, File f) {
	return Pos(s.rank(), f);
}


Move::Move(MoveAction a, Pos from, Pos to)
: _a{a}, _s{from}, _t{to}
{}

void Move::setAction(MoveAction a) { _a = a; }
MoveAction Move::getAction() { return _a; }

Pos Move::getSource() { return _s; }
Pos Move::getTarget() { return _t; }

Move Move::unpack(MovePacked& p) {
	_a = static_cast<MoveAction>( p.f.action );
	_s.set(p.f.source);
	_t.set(p.f.target);

	return *this;
}

MovePacked Move::pack() {
	MovePacked p;
	p.f.action = static_cast<uint8_t>(_a);
	p.f.source = _s.toByte();
	p.f.target = _t.toByte();

	return p;
}

std::ostream& operator<<(std::ostream& os, const Pos& p) {
	os << g_file(p.file()) << g_rank(p.rank());

	return os;
}

// the idea will be to display the move in alebraic notation [38]
std::ostream& operator<<(std::ostream& os, const Move& m) {
	os << static_cast<int>(m._a) << ' ';
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
