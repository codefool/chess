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

const char g_rank(const Rank r) { return '1' + r; }
const char g_file(const File f) { return 'a' + f; }

Pos::Pos() : _r{0}, _f{0} {}

Pos::Pos(Rank ra, File fi)
{
	set(ra,fi);
}

Pos::Pos(short ra, short fi)
: _r(ra), _f(fi)
{}

Pos::Pos(uint8_t b)
{
	fromByte(b);
}

void Pos::set(short ra, short fi) {
	_r = ra;
	_f = fi;
}

void Pos::setRank(short ra) {
	_r = ra;
}

void Pos::setFile(short fi) {
	_f = fi;
}

void Pos::set(Rank ra, File fi) {
	set(static_cast<short>(ra), static_cast<short>(fi));
}

void Pos::set(uint8_t b) {
	fromByte(b);
}

Pos Pos::operator+(const Offset& o) {
	return Pos(_r + o.dr, _f + o.df);
}

Pos Pos::operator+=(const Offset& o) {
	_r += o.dr;
	_f += o.df;
	return *this;
}

bool Pos::operator<(const Pos& o) {
	if (_r == o._r)
		return _f < o._f;
	return _r < o._r;
}

bool Pos::operator<=(const Pos& o) {
	if (_r == o._r)
		return _f <= o._f;
	return _r <= o._r;
}

bool Pos::operator==(const Pos& o) {
	return _r == o._r && _f == o._f;
}

Pos Pos::offset(Offset& o) {
	return Pos(_r + o.dr, _f + o.df);
}

uint8_t Pos::toByte() {
	return (uint8_t)(_r << 3 | _f);
}

void Pos::fromByte(uint8_t b) {
	_f = static_cast<short>(b & 0x07);
	_r = static_cast<short>((b >> 3) & 0x07);
}

const short Pos::r() const { return _r; }
const short Pos::f() const { return _f; }

inline const Rank Pos::rank() const { return static_cast<Rank>(_r); }
inline const File Pos::file() const { return static_cast<File>(_f); }

Move::Move(MoveAction a, Pos from, Pos to)
: _a{a}, _s{from}, _t{to}
{}

void Move::setAction(MoveAction a) { _a = a; }
MoveAction Move::getAction() { return _a; }

Pos Move::getSource() { return _s; }
Pos Move::getTarget() { return _t; }

Move Move::decode(MovePacked& p) {
	_a = static_cast<MoveAction>( p.f.action );
	_s.set(p.f.source);
	_t.set(p.f.target);

	return *this;
}

MovePacked Move::encode() {
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
