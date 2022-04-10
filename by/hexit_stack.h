#pragma once

#include <iostream>
#include <bit>
#include <cstring>

// HexitStack
//
// The idea here is to divide a 64-bit word array, and interpret it
// as a series of octets, with each octet containing two hexits
// (4-bit values) per octet.
//
// Each 64-bit word can contain 32 hexits. The total capacity of the
// stack would be the number of 64-bit words x 16.
//
// So, if we have an array of size 2, then we have 32 potential hexits
// |...:....1....:....2....:....3..|...:....1....:....2....:....3.| words
// |...:...|...:...|...:...|...:...|...:...|...:...|...:...|...:..| octets
// |...|...|...|...|...|...|...|...|...|...|...|...|...|...|...|..| hexit
//
// By allocating the word array as contiguous memory, we can use an
// OCTET*
//
// THE ISSUE IS THAT UNLESS ALL THE HEXITS ARE USED, HOW DO WE KNOW HOW
// MANY ENTRIES IN THE STACK ARE VALID?!

typedef uint8_t  OCTET;

const short  OCTETS_PER_WORD   = sizeof(uint64_t);
const short  HEXITS_PER_OCTET = 2;
const short  HEXITS_PER_WORD  = OCTETS_PER_WORD * HEXITS_PER_OCTET;

class HexitStack {
private:
	uint64_t *_ull;		// buffer
	OCTET    *_p;		// current octet
	OCTET    *_p_end;   // last byte in the buffer
	size_t    _s;		// size of buffer
	size_t	  _cap;		// total hexit capacity
	bool      _hi_hexit;// false for lo hexit, true for hi

public:
	HexitStack(size_t s)
	: _s{s},
	  _hi_hexit{false},
	  _cap{OCTETS_PER_WORD * s}
	{
		_ull   = new uint64_t[s];
		_p     = (OCTET*)_ull;
		_p_end = _p + _cap - 1;
		std::memset(_ull, 0x00, sizeof(uint64_t) * s);
	}

	HexitStack(size_t u, std::initializer_list<uint64_t> i)
	: HexitStack(i.size())
	{
		uint64_t *q = _ull;
		for(auto itr = i.begin(); itr != i.end(); ++itr)
			*q++ = *itr;
		_p_end = _p + u - 1;
		_hi_hexit = true;
	}

	void reset()
	{
		_p = (OCTET*)_ull;
		_hi_hexit = false;
	}

	void goto_end()
	{
		_p = _p_end;
		_hi_hexit = true;
	}

	uint64_t operator[](size_t idx)
	{
		return _ull[idx];
	}

	void push(OCTET val)
	{
		if ( _hi_hexit )
		{
			// hi hexit - set high hexit and inc to next OCTET
			*_p = ( *_p & 0x0f ) | (( val << 4 ) & 0xf0 );
			if ( _p < _p_end )
				_p++;
		}
		else
		{
			// lo hexit - set lo hexit, stay on same OCTET
			*_p = ( *_p & 0xf0 ) | ( val & 0x0f );
		}
		_hi_hexit = !_hi_hexit;	// always toggle hexit id
	}

	OCTET pop()
	{
		OCTET r = *_p;
		if ( _hi_hexit )
		{
			r >>= 4;
		}
		else
		{
			if (_p > (OCTET*)_ull)
				_p--;
		}
		_hi_hexit = !_hi_hexit;
		return r & 0x0f;
	}

    static size_t pop( uint64_t x )
    {
        return std::popcount(x);
    }
};
