/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <algorithm>
#include <cstdlib>

#ifdef LOGURU_ENABLE
#include <loguru/loguru.hpp>
#endif

#include "constraints.h"
#include "minisync.h"

bool MiniSync::Point::operator==(const Point& o) const
{
    return this->x == o.x && this->y == o.y;
}

MiniSync::ConstraintLine::ConstraintLine(const LowPoint& p1, const HighPoint& p2) : B({})
{
#ifdef LOGURU_ENABLE
    CHECK_NE_F(p1.getX(), p2.getX(), "Points in a constraint line cannot have the same REF timestamp!");
#else
    if (p1.getX() == p2.getX())
        throw std::runtime_error("Points in a constraint line cannot have the same REF timestamp!");
#endif

    this->A = (p2.getY() - p1.getY()).count() / (p2.getX() - p1.getX()).count();

    // slope can't be negative
    // CHECK_GT_F(this->A, 0, "Slope can't be negative!"); // it actually can, at least for the constrain lines
    this->B = us_t{p1.getY() - (this->A * p1.getX())};
}

bool MiniSync::ConstraintLine::operator==(const ConstraintLine& o) const
{
    return this->A == o.getA() && this->B == o.getB();
}
