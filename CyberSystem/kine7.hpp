#ifndef KINE7_HPP__
#define KINE7_HPP__
#include <cmath>
#include <vector>
#include <utility>
#include <cassert>
#include <sstream>
#include "eigen3/Eigen/Eigen"
#include "kine_util.hpp"
#include "angular_interval.hpp"
#include "quadratic.hpp"
#include <assert.h>
namespace rpp
{
    namespace kine
    {
        template <typename T>
        class SingularityHandler
        {
        public:
            typedef T value_type;
            typedef ::rpp::util::AngularInterval<value_type> angular_interval;
            typedef std::vector<angular_interval> angular_interval_vector;
            typedef Eigen::Matrix<value_type, 7, 1> vec7;
            SingularityHandler(angular_interval_vector joint_limits)
                : _joint_limits(joint_limits) {}
			SingularityHandler(){}
            void update_current_joints(const vec7 current_joints)
            {
                _current_joints = current_joints;
            }
            void get_upper_joints(const value_type t13, value_type &t1, value_type &t3)
            {
                _get_joints(t13, 0, 2, t1, t3);
            }
            void get_lower_joints(const value_type t57, value_type &t5, value_type &t7)
            {
                _get_joints(t57, 4, 6, t5, t7);
            }
        private:
            void _get_joints(const value_type sum_, const int i, const int j, value_type &t_, value_type &tt_)
            {
                const value_type PI = ::rpp::util::Constants<value_type>::pi;
                const value_type t = _current_joints[i];
                const value_type tt = _current_joints[j];
                value_type l, u, ll, uu, d, dd;
                if (_joint_limits[i].contains(t))
                {
                    l = _joint_limits[i].lower(true);
                    u = _joint_limits[i].upper();
                }
                if (_joint_limits[j].contains(tt))
                {
                    ll = _joint_limits[j].lower(true);
                    uu = _joint_limits[j].upper();
                }
                if (sum_ < t + tt)
                {
                    d = t > l ? (t - l) : (t - l + 2*PI);
                    dd = tt > ll ? (tt - ll) : (tt - ll + 2*PI);
                }
                else
                {
                    d = u > t ? (u - t) : (u - t + 2*PI);
                    dd = uu > tt ? (uu - tt) : (uu - tt + 2*PI);
                }
                const auto delta = sum_ - t - tt;
                const auto alpha = d/(d + dd);
                std::cout << "alpha: " << alpha << " delta: " << delta << std::endl;
                std::cout << "t: " << t << " tt: " << tt << std::endl;
                std::cout << "sum: " << sum_ << std::endl;
                t_ = t + alpha * delta;
                tt_ = tt + (1 - alpha) * delta;
            }
            angular_interval_vector _joint_limits;
            vec7 _current_joints;
        };


        template<typename T>
        class SelfMotion
        {
        public:
            typedef T value_type;
            typedef ::rpp::util::AngularIntervalSet<value_type> angular_interval_set;
            typedef ::rpp::util::AngularInterval<value_type> angular_interval;
            typedef std::vector<angular_interval> angular_interval_vector;
            typedef Eigen::Matrix<value_type, 7, 1> vec7;
            typedef Eigen::Matrix<value_type, 3, 1> vec3;
            typedef Eigen::Matrix<value_type, 3, 3> mat3x3;
            SelfMotion(const angular_interval_set phi,
                    const angular_interval_vector joint_limits, 
                const value_type theta4, 
                const mat3x3 As, const mat3x3 Bs, const mat3x3 Cs, 
                const mat3x3 Aw, const mat3x3 Bw, const mat3x3 Cw, 
                const value_type singular_bound) 
                : _phi(phi), _joint_limits(joint_limits.begin(), joint_limits.end()),
                _theta4(theta4), _As(As), _Bs(Bs), _Cs(Cs), 
                _Aw(Aw), _Bw(Bw), _Cw(Cw), _singular_bound(singular_bound)
            {}

			SelfMotion(const SelfMotion& element)
				: _phi(element._phi), _joint_limits(element._joint_limits.begin(), element._joint_limits.end()),
				_theta4(element._theta4), _As(element._As), _Bs(element._Bs), _Cs(element._Cs), 
				_Aw(element._Aw), _Bw(element._Bw), _Cw(element._Cw), _singular_bound(element._singular_bound)
			{}

			SelfMotion operator = (const SelfMotion& element)
			{
				*this = element;
				return *this;
			}

            angular_interval_set arm_angle_range()
            {
                return _phi;
            }
            value_type elbow_joint()
            {
                return _theta4;
            }
            template <typename F>
            std::vector<vec7> get_joints(const value_type arm_angle, F sh)
            {
                const value_type EPS = ::rpp::util::Constants<value_type>::eps;
                const value_type PI = ::rpp::util::Constants<value_type>::pi;
                const value_type sp = sin(arm_angle);
                const value_type cp = cos(arm_angle);
                std::vector<vec3> theta123;
                auto c2 = -_As(2,1)*sp - _Bs(2,1)*cp - _Cs(2,1);
                auto t2 = (c2 <= 1 ? acos(c2) : 0);
                if (fabs(t2) <= _singular_bound + EPS)
                {
                    const value_type t13 = atan2(_As(1,0)*sp + _Bs(1,0)*cp + _Cs(1,0),
                            _As(0,0)*sp + _Bs(0,0)*cp + _Cs(0,0));
                    value_type t1, t3;
                    sh.get_upper_joints(t13, t1, t3);
                    theta123.push_back(vec3(t1, t2, t3));
                }
                else
                {
                    value_type s1 = -(_As(1,1)*sp + _Bs(1,1)*cp + _Cs(1,1));
                    value_type c1 = -(_As(0,1)*sp + _Bs(0,1)*cp + _Cs(0,1));
                    value_type t1 = atan2(s1, c1);
                    value_type s3 = (_As(2,2)*sp + _Bs(2,2)*cp + _Cs(2,2));
                    value_type c3 = -(_As(2,0)*sp + _Bs(2,0)*cp + _Cs(2,0));
                    value_type t3 = atan2(s3, c3);
                    theta123.push_back(vec3(t1, t2, t3));
                    t2 = -t2;
                    t1 = t1 > 0 ? t1 - PI : t1 + PI;
                    t3 = t3 > 0 ? t3 - PI : t3 + PI;
                    theta123.push_back(vec3(t1, t2, t3));
                }

                std::vector<vec3> theta567;
                value_type c6 = _Aw(2,2)*sp + _Bw(2,2)*cp + _Cw(2,2);
                value_type t6 = c6 <= 1? acos(c6) : 0;
                if (fabs(t6) <= _singular_bound + EPS)
                {
                    const value_type t57 = atan2(_Aw(1,0)*sp + _Bw(1,0)*cp + _Cw(1,0),
                            _Aw(0,0)*sp + _Bw(0,0)*cp + _Cw(0,0));
                    value_type t5, t7;
                    sh.get_lower_joints(t57, t5, t7);
                    theta567.push_back(vec3(t5, t6, t7));
                }
                else
                {
                    value_type s5 = _Aw(1,2)*sp + _Bw(1,2)*cp + _Cw(1,2);
                    value_type c5 = _Aw(0,2)*sp + _Bw(0,2)*cp + _Cw(0,2);
                    value_type t5 = atan2(s5, c5);
                    value_type s7 = _Aw(2,1)*sp + _Bw(2,1)*cp + _Cw(2,1);
                    value_type c7 = -(_Aw(2,0)*sp + _Bw(2,0)*cp + _Cw(2,0));
                    value_type t7 = atan2(s7, c7);
                    theta567.push_back(vec3(t5, t6, t7));
                    t6 = -t6;
                    t5 = t5 > 0 ? t5 - PI : t5 + PI;
                    t7 = t7 > 0 ? t7 - PI : t7 + PI;
                    theta567.push_back(vec3(t5, t6, t7));
                }

                std::vector<vec7> joints;
                for (auto it1 = theta123.begin(); it1 != theta123.end(); ++it1)
                    for (auto it2 = theta567.begin(); it2 != theta567.end(); ++it2)
                    {
                        vec7 v;
                        v << (*it1)(0), (*it1)(1), (*it1)(2), 
                            _theta4, (*it2)(0), (*it2)(1), (*it2)(2);
                        if (validate_joints(v)) joints.push_back(v);
                    }
                return joints;
            }
            bool validate_joints(vec7 joints)
            {
                for (int i = 0; i < 7; ++i)
                    if(!_joint_limits[i].contains(joints[i])) return false;
                return true;
            }
        private:
            const angular_interval_set _phi;
            const value_type _theta4;
            const mat3x3 _As;
            const mat3x3 _Bs;
            const mat3x3 _Cs;
            const mat3x3 _Aw;
            const mat3x3 _Bw;
            const mat3x3 _Cw;
            std::vector<angular_interval_set> _joint_limits;
            const value_type _singular_bound;
        };


        template <typename T>
        class Kine7
        {
        public:
            typedef T value_type;
            typedef Eigen::Matrix<value_type, 4, 4> mat4x4;
            typedef Eigen::Matrix<value_type, 3, 3> mat3x3;
            typedef Eigen::Matrix<value_type, 7, 1> vec7;
            typedef Eigen::Matrix<value_type, 3, 1> vec3;
            typedef ::rpp::util::AngularIntervalSet<value_type> angular_interval_set;
            typedef ::rpp::util::AngularInterval<value_type> angular_interval;
            typedef std::vector<angular_interval> angular_interval_vector;
        private:
            static value_type eval_sin_cos(
                    const value_type a, const value_type b, 
                    const value_type c, const value_type theta)
            {
                const value_type EPS = ::rpp::util::Constants<value_type>::eps;
                const value_type a_ = a - fmod(a, EPS);
                const value_type b_ = b - fmod(b, EPS);
                const value_type c_ = c - fmod(c, EPS);
                const value_type f = a_ * sin(theta) + b_ * cos(theta) + c_; 
                return f - fmod(f, EPS);
            }

            static std::vector<value_type> solve_sin_cos_eq(
                    const value_type a, const value_type b, 
                    const value_type c, const value_type f)
            {
                const value_type c_ = c - f;
                std::vector<value_type> res;
                ::rpp::util::Quadratic<value_type> q(c_ - b, 2*a, c_ + b);
                auto x = q.solve(0);
                for (std::size_t i = 0; i < x.n; ++i)
                    res.push_back(2*atan(x.v[i]));
                if (x.order < 2 && x.n != -1)
                    res.push_back(::rpp::util::Constants<value_type>::pi);
                return res;
            }

            static angular_interval_set solve_sin_cos_leq(
                    const value_type a, const value_type b, 
                    const value_type c, const value_type f)
            {
                // std::cout << "solve " << a << "sin(x) + " << b << " cos(x) + " << c << " <= 0" << std::endl;
                const value_type c_ = c - f;
                ::rpp::util::Quadratic<value_type> q(c_ - b, 2*a, c_ + b);
                auto x = q.solve_leq(0);
                angular_interval_set res;
                for (auto it = x.begin(); it != x.end(); ++it)
                {
                    // std::cout << "quadratic res: " << it->first << ", " << it->second << std::endl;
                    value_type l = 2*atan(it->first);
                    value_type u = 2*atan(it->second);
                    // std::cout << "sin-cos res: " << l << ", " << u << std::endl;
                    angular_interval r(l, u);
                    res += r;
                }
                return res;
            }

            static angular_interval_set solve_sin_cos_geq(
                    const value_type a, const value_type b, 
                    const value_type c, const value_type f)
            {
                return solve_sin_cos_leq(-a, -b, -c, -f);
            }

            static angular_interval_set solve_quadrant_leq(
                    const value_type s1, const value_type s2,
                    const value_type lhs[3], const value_type rhs[3],
                    const value_type u)
            {
                using namespace ::rpp::util;
                const auto EPS = Constants<value_type>::eps;
                const value_type PI = Constants<value_type>::pi;
                angular_interval_set res;
                if ((s2 < 0 && u == PI) || (s2 > 0 && u == 0))
                    return res;
                if ((s2 < 0 && u == 0) || (s2 > 0 && u == PI))
                {
                    res += angular_interval(-PI, PI);
                    return res;
                }
                const value_type s = s1*s2;
                const value_type v = 1/tan(u);
                value_type a_ = s*(v*lhs[0] - rhs[0]);
                a_ = a_ - fmod(a_, EPS);
                value_type b_ = s*(v*lhs[1] - rhs[1]);
                b_ = b_ - fmod(b_, EPS);
                value_type c_ = s*(v*lhs[2] - rhs[2]);
                c_ = c_ - fmod(c_, EPS);
                return solve_sin_cos_leq(a_, b_, c_, 0);
            }
            static angular_interval_set solve_quadrant_geq(
                    const value_type s1, const value_type s2,
                    const value_type lhs[3], const value_type rhs[3],
                    const value_type l)
            {
                using namespace ::rpp::util;
                const auto EPS = Constants<value_type>::eps;
                const value_type PI = Constants<value_type>::pi;
                angular_interval_set res;
                if ((s2 < 0 && l == PI) || (s2 > 0 && l == 0))
                {
                    res += angular_interval(-PI, PI);
                    return res;
                }
                if ((s2 < 0 && l == 0) || (s2 > 0 && l == PI))
                    return res;
                const auto s = s1 * s2;
                const auto v = 1/tan(l);
                auto a_ = s*(v*lhs[0] - rhs[0]); 
                a_ = a_ - fmod(a_, EPS);
                auto b_ = s*(v*lhs[1] - rhs[1]); 
                b_ = b_ - fmod(b_, EPS);
                auto c_ = s*(v*lhs[2] - rhs[2]); 
                c_ = c_ - fmod(c_, EPS);
                return solve_sin_cos_geq(a_, b_, c_, 0);
            }

            static angular_interval_set solve_quadrant(
                    const value_type s1, const value_type s2,
                    const value_type lhs[3], const value_type rhs[3],
                    const angular_interval_set &in)
            {
                // std::cout << "lhs: " << lhs[0] << ", " << lhs[1] << ", " << lhs[2] << std::endl;
                // std::cout << "lhs: " << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << std::endl;
                angular_interval_set res;
                if (in.empty()) return res;
                const auto s = s1 * s2;
                angular_interval_set res1, res2;
                // std::cout << "solve_quadrant(eq1) begin " << std::endl;
                // std::cout << "coeffs: " << s*lhs[0] << ", " << s*lhs[1] << ", " << s*lhs[2] << std::endl;
                res1 = solve_sin_cos_geq(s*lhs[0], s*lhs[1], s*lhs[2], 0);
                // std::cout << "solve_quadrant(eq1) end: " << res1 << std::endl;
                for (auto it = in.begin(); it != in.end(); ++it)
                {
                    auto t1 = solve_quadrant_geq(s1, s2, lhs, rhs, it->lower());
                    auto t2 = solve_quadrant_leq(s1, s2, lhs, rhs, it->upper());
                    res2 += (t1 * t2);
                }
                // std::cout << "solve_quadrant(eq2): " << res2 << std::endl;
                res1 *= res2;
                // std::cout << "solve_quadrant res: " << res1 << std::endl;
                return res1;
            }

            static std::pair<angular_interval_set, angular_interval_set >
                solve_tan_type(const value_type lhs[3], const value_type rhs[3], const angular_interval_set &in)
            {
                const value_type PI = ::rpp::util::Constants<value_type>::pi;
                angular_interval_set neg(angular_interval(-PI, 0));
                angular_interval_set pos(angular_interval(0, PI));
                neg *= in;
                pos *= in;
                // 1st quadrant
                auto r1 = solve_quadrant(1, 1, lhs, rhs, pos);
                // 4th quadrant
                auto r4 = solve_quadrant(1, -1, lhs, rhs, neg);
                // 2nd quadrant
                auto r2 = solve_quadrant(-1, 1, lhs, rhs, pos);
                // 3rd quadrant
                auto r3 = solve_quadrant(-1, -1, lhs, rhs, neg);
                // std::cout << "solve_tan_type -- 1st quad: " << r1 << std::endl;
                // std::cout << "solve_tan_type -- 4st quad: " << r4 << std::endl;
                // std::cout << "solve_tan_type -- 2st quad: " << r2 << std::endl;
                // std::cout << "solve_tan_type -- 3st quad: " << r3 << std::endl;
                auto res_pos = r1 + r4;
                auto res_neg = r2 + r3;
                return std::make_pair(res_neg, res_pos);
            }
            static std::pair<angular_interval_set, angular_interval_set >
                solve_tan_type(const value_type lhs1, const value_type lhs2, const value_type lhs3, 
                        const value_type rhs1, const value_type rhs2, const value_type rhs3, const angular_interval_set &in)
            {
                value_type lhs[3] = {lhs1, lhs2, lhs3};
                value_type rhs[3] = {rhs1, rhs2, rhs3};
                return solve_tan_type(lhs, rhs, in);
            }

            static angular_interval_set solve_cos_type_util(const value_type F[3], const std::pair<bool, T> l, const std::pair<bool, T> u)
            {
                const value_type PI = ::rpp::util::Constants<value_type>::pi;
                angular_interval_set r_u(angular_interval(-PI, PI));
                auto r_l = r_u;
                if (u.first) r_u = solve_sin_cos_leq(F[0], F[1], F[2], cos(u.second));
                if (l.first) r_l = solve_sin_cos_geq(F[0], F[1], F[2], cos(l.second));
                // std::cout << "l: " << l.first << ", " << l.second << std::endl;
                // std::cout << "u: " << u.first << ", " << u.second << std::endl;
                // std::cout << "F: " << F[0] << ", " << F[1] << ", " << F[2] << std::endl;
                // std::cout << "solve_cos_type -- r_u: " << r_u << std::endl;
                // std::cout << "solve_cos_type -- r_l: " << r_l << std::endl;
                // std::cout << "solve_cos_type -- r_u*r_l: " << r_u*r_l << std::endl;
                return r_u * r_l;
            }
            static std::pair<angular_interval_set, angular_interval_set>
                solve_cos_type(const value_type coeffs[3], const angular_interval_set &in, const value_type singular_bound = 5e-5)
            {
                // std::cout << "solve_cos_type -- in: " << in << std::endl;
                const value_type PI = ::rpp::util::Constants<value_type>::pi;
                angular_interval_set in_neg(angular_interval(-PI, -singular_bound));
                angular_interval_set in_pos(angular_interval(singular_bound, PI));
                in_neg *= in;
                in_pos *= in;
                // std::cout << "solve_cos_type -- in_neg: " << in_neg << std::endl;
                // std::cout << "solve_cos_type -- in_pos: " << in_pos << std::endl;
                angular_interval_set res_neg, res_pos;
                for (auto it = in_neg.begin(); it != in_neg.end(); ++it)
                {
                    auto l = std::make_pair(it->lower(true)!=PI, it->lower(true));
                    auto u = std::make_pair(it->upper()!=0, it->upper());
                    res_neg = solve_cos_type_util(coeffs, l, u);
                }
                for (auto it = in_pos.begin(); it != in_pos.end(); ++it)
                {
                    auto l = std::make_pair(it->upper() != PI, it->upper());
                    auto u = std::make_pair(it->lower(true) != 0, it->lower(true));
                    res_pos = solve_cos_type_util(coeffs, l, u);
                }
                return std::make_pair(res_neg, res_pos);
            }
            static std::pair<angular_interval_set, angular_interval_set>
                solve_cos_type(const value_type c1, const value_type c2, const value_type c3,
                        const angular_interval_set &in, const value_type singular_bound = 5e-5)
            {
                value_type coeffs[3] = {c1, c2, c3};
                return solve_cos_type(coeffs, in, singular_bound);
            }
            void reference_plane(const value_type theta4, 
                    const value_type p0, 
                    const value_type q0, 
                    const value_type r0, 
                    value_type &theta1_ref,
                    value_type &theta2_ref)
            {
                const static value_type PI = ::rpp::util::Constants<value_type>::pi;
                const value_type c4 = cos(theta4);
                const value_type s4 = sin(theta4);
                const value_type p3 = - c4 * _D + s4 * _L2 + _D;
                const value_type q3 = -_D*s4 - _L2*c4 - _L1;
                if (0 == p0 && 0 == q0)
                {
                     theta1_ref = 0;
                     theta2_ref = atan2(-p3, -q3);
                }
                else
                {
                    std::vector<value_type> theta2;
                    if (r0*r0 >= p3*p3 + q3*q3)
                        theta2.push_back((r0 >=0 ? atan2(p3, q3) : -PI + atan2(p3, q3)));
                    else
                        theta2 = solve_sin_cos_eq(p3, q3, r0, 0);
                    for (auto t2 = theta2.begin(); t2 != theta2.end(); ++t2)
                    {
                        const value_type c2 = cos(*t2);
                        const value_type s2 = sin(*t2);
                        const value_type a = (p3*c2 - q3*s2 >= 0 ? 1 : -1);
                        const value_type b = (p3 >= 0 ? 1 : -1);
                        theta2_ref = *t2;
                        theta1_ref = atan2(a*q0, a*p0);
                        if (a*b >= 0) break;
                    }
                }
            }
            void handle_singularity(
                    const value_type F_cos_a,
                    const value_type F_cos_b,
                    const value_type F_cos_c,
                    const value_type F_sum_sin_a,
                    const value_type F_sum_sin_b,
                    const value_type F_sum_sin_c,
                    const value_type F_sum_cos_a,
                    const value_type F_sum_cos_b,
                    const value_type F_sum_cos_c,
                    const angular_interval_set valid_range,
                    angular_interval_set &arm_angle_range)
            {
                const static value_type PI = ::rpp::util::Constants<value_type>::pi;
                auto half_n = angular_interval_set(angular_interval(-PI, 0));
                auto half_p = angular_interval_set(angular_interval(0, PI));
                auto neg_range = half_n * valid_range;
                auto pos_range = half_p * valid_range;
                auto singular_range = solve_sin_cos_geq(F_cos_a, F_cos_b, F_cos_c, cos(_singular_bound));
                value_type F_sum_sin[3] = {F_sum_sin_a, F_sum_sin_b, F_sum_sin_c};
                value_type F_sum_cos[3] = {F_sum_cos_a, F_sum_cos_b, F_sum_cos_c};
                auto res_pos = solve_quadrant(1, 1, F_sum_sin, F_sum_cos, pos_range);
                auto res_neg = solve_quadrant(1, -1, F_sum_sin, F_sum_cos, neg_range);
                auto res = (res_pos + res_neg) * singular_range;
                arm_angle_range += res;
            }
        private:
            template <typename ITER>
            void around(ITER src, ITER dst, std::size_t size)
            {
                const value_type EPS = ::rpp::util::Constants<value_type>::eps;
                for (std::size_t i = 0; i < size; ++i)
                {
                    *dst = *src - fmod(*src, EPS);
                    dst++; src++;
                }
            }
        public:
            Kine7(const value_type L1, const value_type L2,
                const value_type L3, const value_type D,
                angular_interval_vector joint_limits)
                : _L1(L1), _L2(L2), _L3(L3), _D(D), _joint_limits(joint_limits),
                _singular_bound(5e-5)
            {
                using namespace ::rpp::util;
                const static value_type PI = Constants<value_type>::pi;
                assert(_joint_limits.size() == 7);
                _DH << -PI/2, 0, 0, 0, 
                       PI/2,  0, 0, 0,
                       -PI/2, D, 0, L1,
                       PI/2, -D, 0, 0,
                       -PI/2, 0, 0, L2,
                       PI/2,  0, 0, 0,
                       0,     0, 0, L3;
            }

			// default constructor
			Kine7():_L1(55),_L2(30),_L3(6.1),_D(4.5),_singular_bound(5e-5)
			{
				angular_interval_vector joint_limits;
				joint_limits.push_back(angular_interval(-2.62, 2.62));
				joint_limits.push_back(angular_interval(-2.01, 2.01));
				joint_limits.push_back(angular_interval(-2.97, 2.97));
				joint_limits.push_back(angular_interval(-0.87, 3.14));
				joint_limits.push_back(angular_interval(-1.27, 4.79));
				joint_limits.push_back(angular_interval(-1.57, 1.57));
				joint_limits.push_back(angular_interval(-2.35, 2.35));

				_joint_limits = joint_limits;

				using namespace ::rpp::util;
				const static value_type PI = Constants<value_type>::pi;
				assert(_joint_limits.size() == 7);
				_DH << -PI/2, 0, 0, 0, 
					PI/2,  0, 0, 0,
					-PI/2, _D, 0, _L1,
					PI/2, -_D, 0, 0,
					-PI/2, 0, 0, _L2,
					PI/2,  0, 0, 0,
					0,     0, 0, _L3;
			}

			// copy constructor
			Kine7(const Kine7& element)
				: _L1(element._L1), _L2(element._L2), _L3(element._L3), _D(element._D), _joint_limits(element._joint_limits),
				_singular_bound(5e-5)
			{
				using namespace ::rpp::util;
				const static value_type PI = Constants<value_type>::pi;
				assert(_joint_limits.size() == 7);
				_DH << -PI/2, 0, 0, 0, 
					PI/2,  0, 0, 0,
					-PI/2, element._D, 0, element._L1,
					PI/2, -element._D, 0, 0,
					-PI/2, 0, 0, element._L2,
					PI/2,  0, 0, 0,
					0,     0, 0, element._L3;
			}

			// donot need to reload operator "="
// 			Kine7 operator = (const Kine7& element)
// 			{
// 				Kine7 ele(element._L1, element._L2, element._L3, element._D, element._joint_limits);
// // 				_L1 = element._L1;
// // 				_L2 = element._L2;
// // 				_L3 = element._L3;
// // 				_D = element._D;
// // 				_joint_limits = element._joint_limits;
// // 				_singular_bound = 5e-5;
// 
// // 				using namespace ::rpp::util;
// // 				const static value_type PI = Constants<value_type>::pi;
// // 				assert(_joint_limits.size() == 7);
// // 				_DH << -PI/2, 0, 0, 0, 
// // 					PI/2,  0, 0, 0,
// // 					-PI/2, element._D, 0, element._L1,
// // 					PI/2, -element._D, 0, 0,
// // 					-PI/2, 0, 0, element._L2,
// // 					PI/2,  0, 0, 0,
// // 					0,     0, 0, element._L3;
// 
// 				return ele;
// 			}


            mat4x4 forward(const vec7& q)
            {
                mat4x4 TT = mat4x4::Identity();
                for (int i = 0; i < _DH.rows(); ++i)
                {
                    TT *= ::rpp::kine::dh2t(_DH(i, 0), _DH(i, 1), q(i) - _DH(i, 2), _DH(i, 3));
                }
                return TT;
            }


            std::vector<SelfMotion<T> > inverse(const mat4x4& T_07, std::ostream *os = nullptr)
            {
                std::vector<SelfMotion<T> > self_motions;
                std::ostringstream os_;
                if (os == nullptr) os = &os_;
                vec3 x_wt_7;
                x_wt_7 << 0, 0, _L3;
                vec3 x_st_0 = T_07.template block<3, 1>(0, 3);
                mat3x3 R_70 = T_07.template block<3, 3>(0, 0);
                vec3 x_sw_0 = x_st_0 - R_70 * x_wt_7;
                for (int i = 0; i < 3; ++i)
                    x_sw_0(i, 0) = x_sw_0(i, 0) - fmod(x_sw_0(i, 0), _singular_bound);
                value_type p0 = x_sw_0(0);
                value_type q0 = x_sw_0(1);
                value_type r0 = x_sw_0(2);
                auto t4_res = solve_sin_cos_eq(2*_D*(_L1+_L2), 2*(_L1*_L2-_D*_D),
                        2*_D*_D+_L1*_L1+_L2*_L2, x_sw_0.squaredNorm());
                if (t4_res.empty())
                {
                    (*os) << "theta4 has no solution" << std::endl;
                    return self_motions;
                }
                for (auto t4 = t4_res.begin(); t4 != t4_res.end(); ++t4)
                {
                    value_type theta1_ref, theta2_ref;
                    reference_plane(*t4, p0, q0, r0, theta1_ref, theta2_ref);
                    const value_type c4 = cos(*t4);
                    const value_type s4 = sin(*t4);
                    mat3x3 R_43;
                    R_43 << c4, 0, s4, 
                         s4, 0, -c4, 
                         0, 1, 0;
                    const value_type s1_ref = sin(theta1_ref);
                    const value_type c1_ref = cos(theta1_ref);
                    const value_type s2_ref = sin(theta2_ref);
                    const value_type c2_ref = cos(theta2_ref);
                    vec3 v = x_sw_0.normalized();
                    mat3x3 V;
                    V << 0, -v(2), v(1),
                         v(2), 0,  -v(0),
                         -v(1), v(0), 0;
                    mat3x3 V_squared = V * V;
                    mat3x3 R_30_ref;
                    R_30_ref << c1_ref*c2_ref, -c1_ref * s2_ref, -s1_ref,
                                s1_ref*c2_ref, -s1_ref * s2_ref, c1_ref,
                                -s2_ref,       -c2_ref,          0;
                    mat3x3 As = V * R_30_ref;
                    mat3x3 Bs = -V_squared * R_30_ref;
                    mat3x3 Cs = (mat3x3::Identity() + V_squared) * R_30_ref;
                    around(As.data(), As.data(), As.size());
                    around(Bs.data(), Bs.data(), Bs.size()); 
                    around(Cs.data(), Cs.data(), Cs.size());
                    // upper arm
                    auto p2 = solve_cos_type(-As(2,1), -Bs(2,1), -Cs(2,1), _joint_limits[1]);
                    auto p1 = solve_tan_type(-As(1,1), -Bs(1,1), -Cs(1,1),
                            -As(0,1), -Bs(0,1), -Cs(0,1), _joint_limits[0]);
                    auto p3 = solve_tan_type(As(2,2), Bs(2,2), Cs(2,2),
                            -As(2,0), -Bs(2,0), -Cs(2,0), _joint_limits[2]);
                    auto Pu = (p1.first * p2.first * p3.first) + (p1.second * p2.second * p3.second); 
                    // std::cout << "p1: " << p1.first << ", " << p1.second << std::endl;
                    // std::cout << "p2: " << p2.first << ", " << p2.second << std::endl;
                    // std::cout << "p3: " << p3.first << ", " << p3.second << std::endl;
                    // std::cout << "Pu: " << Pu << std::endl;
                    handle_singularity(-As(2,1), -Bs(2,1), -Cs(2,1),
                            As(1,0), Bs(1,0), Cs(1,0),
                            As(0,0), Bs(0,0), Cs(0,0), 
                            _joint_limits[0] + _joint_limits[2],
                            Pu);
                    // lower arm
                    mat3x3 Aw = (As * R_43).transpose() * R_70;
                    around(Aw.data(), Aw.data(), Aw.size());
                    mat3x3 Bw = (Bs * R_43).transpose() * R_70;
                    around(Bw.data(), Bw.data(), Bw.size());
                    mat3x3 Cw = (Cs * R_43).transpose() * R_70;
                    around(Cw.data(), Cw.data(), Cw.size());
                    auto p6 = solve_cos_type(Aw(2,2), Bw(2,2), Cw(2,2), _joint_limits[5]);
                    auto p5 = solve_tan_type(Aw(1,2), Bw(1,2), Cw(1,2),
                            Aw(0,2), Bw(0,2), Cw(0,2), _joint_limits[4]);
                    auto p7 = solve_tan_type(Aw(2,1), Bw(2,1), Cw(2,1),
                            -Aw(2,0), -Bw(2,0), -Cw(2,0), _joint_limits[6]);
                    auto Pl = (p5.first * p6.first * p7.first) + (p5.second * p6.second * p7.second);
                    // std::cout << "p5: " << p5.first << ", " << p5.second << std::endl;
                    // std::cout << "p5 coeffs: " << Aw(1,2) << ", " << Bw(1,2) << ", " << Cw(1,2)
                    //     << ", " << Aw(0,2) << ", " << Bw(0,2) << ", " << Cw(0,2) << std::endl;
                    // std::cout << "p5 lim: " << _joint_limits[4] << std::endl;
                    // std::cout << "p6: " << p6.first << ", " << p6.second << std::endl;
                    // std::cout << "p7: " << p7.first << ", " << p7.second << std::endl;
                    // std::cout << "Pl: " << Pl << std::endl;
                    handle_singularity(Aw(2,2), Bw(2,2), Cw(2,2),
                            Aw(1,0), Bw(1,0), Cw(1,0),
                            Aw(0,0), Bw(0,0), Cw(0,0),
                            _joint_limits[4] + _joint_limits[6],
                            Pl);
                    auto P = Pu * Pl;
                    SelfMotion<value_type> self_motion(P, _joint_limits, 
                        *t4, As, Bs, Cs, Aw, Bw, Cw, _singular_bound);
                    self_motions.push_back(self_motion);
                }
				return self_motions;
            }
        private:
//             const value_type _L1;
//             const value_type _L2;
//             const value_type _L3;
//             const value_type _D;
			value_type _L1;
			value_type _L2;
			value_type _L3;
			value_type _D;
            angular_interval_vector _joint_limits;
            Eigen::Array<value_type, 7, 4> _DH;
//            const value_type _singular_bound;
			value_type _singular_bound;
        };
    }
}
#endif // KINE7_HPP__
