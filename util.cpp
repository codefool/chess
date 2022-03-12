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

Pos::Pos() {}

Pos::Pos(short p)
{
	fromByte(p);
}

Pos::Pos(Rank ra, File fi)
{
	set(ra,fi);
}

Pos::Pos(int ra, int fi)
{
	set(static_cast<Rank>(ra),static_cast<File>(fi));
}

void Pos::set(short pos) {
	fromByte(static_cast<uint8_t>(pos));
}

void Pos::set(Rank ra, File fi) {
	_r = ra;
	_f = fi;
}

bool Pos::in_bounds() const {
	return 0 <= _r && _r < 8 && 0 <= _f && _f < 8;
}

Pos Pos::operator+(const Offset& o) const {
	return Pos( _r + o.dr(), _f + o.df() );
}

Pos Pos::operator+=(const Offset& o) {
	_r += o.dr();
	_f += o.df();
	return *this;
}

bool Pos::operator<(const Pos& o) const {
	if ( _r == o._r )
		return _f < o._f;
	return _r < o._r;
}

bool Pos::operator<=(const Pos& o) const {
	if ( _r == o._r )
		return _f <= o._f;
	return _r <= o._r;
}

bool Pos::operator==(const Pos& o) const {
	return _r == o._r && _f == o._f;
}

uint8_t Pos::toByte() const {
	return (uint8_t)(_r<<3|_f);
}

void Pos::fromByte(uint8_t b) {
	_r = b >> 3;
	_f = b & 0x7;
}

const short Pos::r() const {
	return _r;
}

const short Pos::f() const {
	return _f;
}

const Rank Pos::rank() const {
	return static_cast<Rank>(r());
}
const File Pos::file() const {
	return static_cast<File>(f());
}

Pos Pos::withRank(Rank r) {
	return Pos(r, _f);
}
Pos Pos::withFile(File f) {
	return Pos(_r, f);
}

Move::Move()
: _a{MV_NONE}, _s{0}, _t{0}
{}

Move::Move(MoveAction a, Pos from, Pos to)
: _a{a}, _s{from}, _t{to}
{}

void Move::setAction(MoveAction a) { _a = a; }
MoveAction Move::getAction() { return _a; }

Pos Move::getSource() { return _s; }
Pos Move::getTarget() { return _t; }

Move Move::unpack(MovePacked& p) {
	Move ret;
	ret._a = static_cast<MoveAction>( p.f.action );
	ret._s.set(p.f.source);
	ret._t.set(p.f.target);

	return ret;
}

MovePacked Move::pack() {
	MovePacked p;
	p.f.action = static_cast<uint8_t>(_a);
	p.f.source = _s.toByte();
	p.f.target = _t.toByte();

	return p;
}

std::ostream& operator<<(std::ostream& os, const Pos& p) {
	os << g_file(p.f()) << g_rank(p.r());

	return os;
}

// the idea will be to display the move in alebraic notation [38]
std::ostream& operator<<(std::ostream& os, const Move& m) {
	os << static_cast<int>(m._a) << ':';
	// special cases
	if (m._a == MV_CASTLE_KINGSIDE)
		os << "O-O";
	else if( m._a == MV_CASTLE_QUEENSIDE)
		os << "O-O-O";
	else {
		os << m._s;

		switch(m._a) {
			// case MV_CAPTURE:
			case MV_EN_PASSANT:
				os << 'x';
				break;
		}

		os << m._t;

		switch(m._a) {
			case MV_EN_PASSANT:
				os << " e.p.";
				break;
			// case MV_CHECK:
			// 	os << '+';
			// 	break;
			// case MV_CHECKMATE:
				// os << "++";
				// break;
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
