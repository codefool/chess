#include <iostream>
#include "db.h"

bool PositionPacked::operator==(const PositionPacked& o) const {
    return c == o.c
            && g == o.g
            && equals(b, o.b)
            && std::memcmp(m, o.m, c * sizeof(uint8_t)) == 0;
}


std::istream& operator>>(std::istream& is, PositionPacked& p) {
    
    return is;
}
