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

    protected:
        Point(const Point& o) = default;
        bool operator==(const Point& o) const;
    };

    class LowPoint : public Point
    {
    public:
        LowPoint() : Point()
        {}

        LowPoint(us_t x, us_t y) : Point(x, y)
        {}

        LowPoint(const LowPoint& o) = default;

        bool operator==(const LowPoint& o) const
        {
            return Point::operator==(o);
        };

        bool operator!=(const LowPoint& o) const
        {
            return !Point::operator==(o);
        }

        bool operator<(const LowPoint& o) const
        {
            return this->getX() < o.getX();
        }

        bool operator>(const LowPoint& o) const
        {
            return this->getX() > o.getX();
        }
    };

    class HighPoint : public Point
    {
    public:
        HighPoint() : Point()
        {}

        HighPoint(us_t x, us_t y) : Point(x, y)
        {}

        HighPoint(const HighPoint& o) = default;

        bool operator==(const HighPoint& o) const
        {
            return Point::operator==(o);
        };

        bool operator!=(const HighPoint& o) const
        {
            return !Point::operator==(o);
        }

        bool operator<(const HighPoint& o) const
        {
            return this->getX() < o.getX();
        }

        bool operator>(const HighPoint& o) const
        {
            return this->getX() > o.getX();
        }
    };

    class ConstraintLine
    {
    public:

        ConstraintLine() : A(0), B(0)
        {};
        ConstraintLine(const LowPoint& p1, const HighPoint& p2);

        ConstraintLine(const HighPoint& p1, const LowPoint& p2) : ConstraintLine(p2, p1)
        {};

        ConstraintLine(const ConstraintLine& o) :
        A(o.getA()), B(o.getB())
        {}

        long double getA() const
        { return this->A; }

        us_t getB() const
        { return this->B; }

        bool operator==(const ConstraintLine& o) const;

        std::string toString() const;

    private:
        long double A;
        us_t B;

    };
}

namespace std
{
    template<>
    struct hash<MiniSync::Point>
    {
        size_t operator()(const MiniSync::Point& point) const;
    };
}
#endif //MINISYNCPP_CONSTRAINTS_H
