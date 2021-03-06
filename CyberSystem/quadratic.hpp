#ifndef QUADRATIC_HPP__
#define QUADRATIC_HPP__

#include <cmath>
#include <algorithm>
#include <limits>
#include <ios>
#include <vector>
#include <utility>

namespace rpp
{
    namespace util
    {
        template <typename T>
        class Quadratic
        {
        public:
            typedef T value_type;
            struct roots_type
            {
                std::size_t order;
                std::size_t n;
                std::vector<value_type> v;
            };
            typedef std::vector<std::pair<value_type, value_type> > intervals_type;

            Quadratic(const value_type a, const value_type b, const value_type c)
                : _a(a), _b(b), _c(c) {}
            value_type eval(const value_type x)
            {
                return (_a * x + _b) * x + _c;
            }
            roots_type solve(const value_type f)
            {
                const auto a = _a, b = _b, c = _c - f;
                roots_type roots;
                if (a == 0)
                {
                    if (b == 0)
                    {
                        roots.order = 0;
                        if (c == 0) 
                        {
                            roots.n = -1;
                        }
                        else
                        {
                            roots.n = 0;
                        }
                    }
                    else
                    {
                        roots.order = 1;
                        roots.n = 1;
                        roots.v.push_back(-c/b);
                    }
                }
                else
                {
                    roots.order = 2;
                    auto D = b*b - 4*a*c;
                    if (D < 0)
                    {
                        roots.n = 0;
                    }
                    else if (D == 0)
                    {
                        roots.n = 1;
                        roots.v.push_back(-b/(2*a));
                    }
                    else
                    {
                        auto Ds = sqrt(D);
                        roots.n = 2;
                        auto x1 = (-b - Ds)/(2*a);
                        auto x2 = (-b + Ds)/(2*a);
                        roots.v.push_back((std::min)(x1, x2));
                        roots.v.push_back((std::max)(x1, x2));
                    }
                }
                return roots;
            }
            intervals_type solve_leq(const value_type f) 
            {
                intervals_type res;
                auto roots = solve(f);
                const value_type INF = std::numeric_limits<value_type>::infinity();
                if (roots.order == 0)
                    if (_c <= 0) res.push_back(std::make_pair(-INF, INF));
                if (roots.order == 1)
                    if (_b > 0) res.push_back(std::make_pair(-INF, roots.v[0]));
                    else res.push_back(std::make_pair(roots.v[0], INF));
                if (roots.order == 2)
                {
                    if (roots.n == 2)
                        if (_a > 0) res.push_back(std::make_pair(roots.v[0], roots.v[1]));
                        else {
                            res.push_back(std::make_pair(-INF, roots.v[0]));
                            res.push_back(std::make_pair(roots.v[1], INF));
                        }
                    if (roots.n == 1)
                        if (_a > 0) res.push_back(std::make_pair(roots.v[0], roots.v[0]));
                        else res.push_back(std::make_pair(-INF, INF));
                    if (roots.n == 0)
                        if (_a <= 0) res.push_back(std::make_pair(-INF, INF));
                }
                return res;
            }
            const value_type _a;
            const value_type _b;
            const value_type _c;
        };

        template <typename T>
        std::ostream& operator << (std::ostream &os, Quadratic<T> q)
        {
            os << q._a << "x^2 + " << q._b << "x + " << q._c; 
			return os;
        }
        template <typename T>
        std::ostream & operator << (std::ostream &os, std::vector<std::pair<T, T> > intervals)
        {
            os << "{";
            for (auto it = intervals.begin(); it != intervals.end(); ++it)
            {
                os << (it != intervals.begin()? " ":"") << "[" << it->first << ", " << it->second << "]";
            }
            os << "}";

			return os;
        }
    }
}
#endif // QUADRATIC_HPP__
