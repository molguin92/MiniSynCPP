/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_CONSTRAINTS_H
#define MINISYNCPP_CONSTRAINTS_H

# include <chrono>
# include <tuple>
# include <exception>
# include <map>
# include <set>

namespace MiniSync
{
    using us_t = std::chrono::duration<long double, std::chrono::microseconds::period>;

    class Point
    {
    private:
        us_t x;
        us_t y;
    public:
        Point() : x(0), y(0)
        {};

        Point(us_t x, us_t y) : x(x), y(y)
        {};

        const us_t& getX() const
        {
            return this->x;
        }

        const us_t& getY() const
        {
            return this->y;
        }

        bool operator<(const Point& o) const
        {
            return !(*this == o);
        }

        bool operator>(const Point& o) const
        {
            return !(*this == o);
        }

    protected:
        Point(const Point& o) = default;
        bool operator==(const Point& o) const;
    };

    class LowerPoint : public Point
    {
    public:
        LowerPoint() : Point()
        {}

        LowerPoint(us_t x, us_t y) : Point(x, y)
        {}

        LowerPoint(const LowerPoint& o) = default;

        bool operator==(const LowerPoint& o) const
        {
            return Point::operator==(o);
        };

        bool operator!=(const LowerPoint& o) const
        {
            return !Point::operator==(o);
        }
    };

    class HigherPoint : public Point
    {
    public:
        HigherPoint() : Point()
        {}

        HigherPoint(us_t x, us_t y) : Point(x, y)
        {}

        HigherPoint(const HigherPoint& o) = default;

        bool operator==(const HigherPoint& o) const
        {
            return Point::operator==(o);
        };

        bool operator!=(const HigherPoint& o) const
        {
            return !Point::operator==(o);
        }
    };

    class ConstraintLine
    {
    public:

        ConstraintLine() : A(0), B(0)
        {};
        ConstraintLine(const LowerPoint& p1, const HigherPoint& p2);

        ConstraintLine(const HigherPoint& p1, const LowerPoint& p2) : ConstraintLine(p2, p1)
        {};

        ConstraintLine(const ConstraintLine& o) :
        A(o.getA()), B(o.getB())
        {}

        long double getA() const
        { return this->A; }

        us_t getB() const
        { return this->B; }

        bool operator==(const ConstraintLine& o) const;

    private:
        long double A;
        us_t B;

    };
}
#endif //MINISYNCPP_CONSTRAINTS_H
