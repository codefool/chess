#include <iostream>
#include "db.h"

std::istream& operator>>(std::istream& is, PositionPacked& p) {
    return is;
}
