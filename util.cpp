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
	_r = (b >> 3) & 0x07;
	_f = b & 0x07;
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

std::ostream& operator<<(std::ostream& os, const PositionPacked& pp)
{
	auto oflags = os.flags(std::ios::hex);
    auto ofill  = os.fill('0');
	auto owid   = os.width(8);

    os << pp.gi.i << ',';
    os.width(16);
    os << pp.pop << ','
       << pp.hi << ','
       << pp.lo;

	os.flags(oflags);
	os.fill(ofill);
	os.width(owid);
	return os;
}

PosInfo::PosInfo()
: id{0}, src{0}, move(Move().pack()), move_cnt{0}, distance{0},
  fifty_cnt{0}, egr{EGR_NONE}
#ifdef POSINFO_HAS_REFS
  , refs{nullptr}
#endif
{}

PosInfo::PosInfo(PositionId i, PosInfo s, MovePacked m)
: id{i}, src{s.id}, move(m), move_cnt{0},
  distance{s.distance + 1},
  fifty_cnt{s.fifty_cnt + 1},
  egr{EGR_NONE}
#ifdef POSINFO_HAS_REFS
  , refs{nullptr}
#endif
{}

#ifdef POSINFO_HAS_REFS
// it's possible that multiple threads for the same position
// can cause issues here. Since we have at most THREAD_COUNT
// possible collisions, we only need that many mutex's.
// We do a poor man's hash and just id mod THREAD_COUNT
// should provide adequate protection without pausing all
// threads every time we add a reference (of which there are
// many)
std::vector<std::mutex> posrefmtx(THREAD_COUNT);

void PosInfo::add_ref(Move move, PositionId trg)
{
    std::lock_guard<std::mutex> lock(posrefmtx[id % THREAD_COUNT]);
    if (refs == nullptr) {
        refs = new PosRefMap();
        refs->reserve(10);
    }
    refs->push_back(PosRef(move,trg));
}
#endif

bool PosInfo::operator==(const PosInfo& other)
{
    if (id       != other.id
    || src       != other.src
    || move.i    != other.move.i
    || move_cnt  != other.move_cnt
    || distance  != other.distance
    || fifty_cnt != other.fifty_cnt
    || egr       != other.egr
#ifdef POSINFO_HAS_REFS
    || (refs == nullptr && other.refs != nullptr)
    || (refs != nullptr && other.refs == nullptr)
#endif
    )
        return false;
#ifdef POSINFO_HAS_REFS
    if (refs != nullptr && other.refs != nullptr)
    {
        if(refs->size() != other.refs->size())
            return false;
        // deep compare all of the references
    }
#endif
    return true;
}


PositionFile::PositionFile(std::string base_path, std::string base_name, int level, bool use_thread_id, bool write_header)
: line_cnt{0}
{
    std::stringstream ss;
    ss << base_path << level << '/';
    std::filesystem::create_directories(ss.str());
    ss << base_name << '_' << level;
    if ( use_thread_id )
        ss << '_' << std::this_thread::get_id();
    ss << ".csv";
    fspec = ss.str();
    ofs.open(fspec, std::ios_base::app);
    ofs.flags(std::ios::hex);
    ofs.fill('0');
    if ( write_header )
    {
        ofs << "\"id\","
            << "\"parent\","
            << "\"gameinfo\","
            << "\"population\","
            << "\"hi\","
            << "\"lo\","
            << "\"move_cnt\","
            << "\"move_packed\","
            << "\"distance\","
            << "\"50_cnt\","
            << "\"end_game\","
            << "\"ref_cnt\","
            << "\"move/parent...\""
            << '\n';
    }
}

PositionFile::~PositionFile()
{
    ofs << std::flush;
    ofs.close();
}

void PositionFile::write(const PositionPacked& pos, const PosInfo& info)
{
    //id    gi   pop    hi     lo     src    mv   dist 50m
    auto ow = ofs.width(16);
    ofs << info.id << ','
        << info.src << ',';
    ofs.width(8);
    ofs << pos.gi.i << ',';
    ofs.width(16);
    ofs << pos.pop << ','
        << pos.hi << ','
        << pos.lo << ',';
    ofs.width(ow);
    ofs << info.move_cnt << ','
        << info.move.i << ','
        << info.distance << ','
        << info.fifty_cnt << ','
        << static_cast<int>(info.egr) << ',';

#ifdef POSINFO_HAS_REFS
    if (info.refs == nullptr) {
        ofs << 0;
    } else {
        ofs << info.refs->size() << ',';
        bool second = false;
        for (auto e : *info.refs) {
            if (second)
              ofs << ',';
            ofs << e.move.i << ',' << e.trg;
            second = true;
        }
    }
#endif
    ofs << '\n';
    if ((++line_cnt % 100) == 0)
      ofs << std::flush;
}