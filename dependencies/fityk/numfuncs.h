// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__NUMFUNCS__H__
#define FITYK__NUMFUNCS__H__

#include <stdlib.h>
#include "fityk.h"

namespace fityk {

/// Point used for linear interpolation and polyline convex hull algorithm.
struct PointD
{
    double x, y;
    PointD() {}
    PointD(double x_, double y_) : x(x_), y(y_) {}
    bool operator< (const PointD& b) const { return x < b.x; }
};


/// Point used for spline/polyline interpolation.
/// The q parameter is used for cubic spline computation.
struct PointQ
{
    double x, y;
    double q; /* q is used for spline */
    PointQ() {}
    PointQ(double x_, double y_) : x(x_), y(y_) {}
    bool operator< (const PointQ& b) const { return x < b.x; }
};

/// must be run before computing value of cubic spline in point x
/// results are written in PointQ::q
/// based on Numerical Recipes www.nr.com
FITYK_API void prepare_spline_interpolation (std::vector<PointQ> &bb);

// instantiated for T = PointQ, PointD
template<typename T>
typename std::vector<T>::iterator
get_interpolation_segment(std::vector<T> &bb,  double x);

FITYK_API double get_spline_interpolation(std::vector<PointQ> &bb, double x);

FITYK_API double get_linear_interpolation(std::vector<PointD> &bb, double x);
FITYK_API double get_linear_interpolation(std::vector<PointQ> &bb, double x);

// random number utilities
inline double rand_1_1() { return 2.0 * rand() / RAND_MAX - 1.; }
inline double rand_0_1() { return static_cast<double>(rand()) / RAND_MAX; }
inline double rand_uniform(double a, double b) { return a + rand_0_1()*(b-a); }
inline bool rand_bool() { return rand() < RAND_MAX / 2; }
double rand_gauss();
double rand_cauchy();

// very simple matrix utils
void jordan_solve(std::vector<realt>& A, std::vector<realt>& b, int n);
void invert_matrix(std::vector<realt>&A, int n);
// format (for printing) matrix m x n stored in vec. `mname' is name/comment.
std::string format_matrix(const std::vector<realt>& vec,
                          int m, int n, const char *mname);

// Simple Polyline Convex Hull Algorithms
// takes as input a sequence of points (x,y), with increasing x coord (added
// in push_point()) and returns points of convex hull (get_vertices())
class FITYK_API SimplePolylineConvex
{
public:
    void push_point(double x, double y) { push_point(PointD(x, y)); }
    void push_point(PointD const& p);
    std::vector<PointD> const& get_vertices() const { return vertices_; }
    // test if point p2 left of the line through p0 and p1
    static bool is_left(PointD const& p0, PointD const& p1, PointD const& p2)
        { return (p1.x - p0.x)*(p2.y - p0.y) > (p2.x - p0.x)*(p1.y - p0.y); }

private:
    std::vector<PointD> vertices_;
};

} // namespace fityk
#endif
