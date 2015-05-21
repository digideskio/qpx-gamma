// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "func.h"

#include "common.h"
#include "bfunc.h"
#include "settings.h"
#include "udf.h"

using namespace std;

namespace fityk {

vector<realt> Function::calc_val_xx(1);
vector<realt> Function::calc_val_yy(1);

Function::Function(const Settings* settings,
                   const string &name_,
                   const Tplate::Ptr tp,
                   const vector<string> &vars)
    : Func(name_),
      used_vars_(vars),
      settings_(settings),
      tp_(tp),
      av_(vars.size()),
      center_idx_(-1)
{
}

void Function::init()
{
    center_idx_ = index_of_element(tp_->fargs, "center");
    if (center_idx_ == -1 && (tp_->traits & Tplate::kSigmoid))
        center_idx_ = index_of_element(tp_->fargs, "xmid");
    if (av_.size() != tp_->fargs.size())
        throw ExecuteError("Function " + tp_->name + " requires "
           + S(tp_->fargs.size()) + " argument(s), got " + S(av_.size()) + ".");
}

void Function::do_precomputations(const vector<Variable*> &variables)
{
    //precondition: recalculate() for all variables
    multi_.clear();
    for (int i = 0; i < used_vars_.get_count(); ++i) {
        const Variable *v = variables[used_vars_.get_idx(i)];
        av_[i] = v->value();
        v_foreach (Variable::ParMult, j, v->recursive_derivatives())
            multi_.push_back(Multi(i, *j));
    }
    this->more_precomputations();
}

void Function::erased_parameter(int k)
{
    vm_foreach (Multi, i, multi_)
        if (i->p > k)
            -- i->p;
}


void Function::calculate_value(const vector<realt> &x, vector<realt> &y) const
{
    realt left, right;
    double cut_level = settings_->function_cutoff;
    if (cut_level != 0. && get_nonzero_range(cut_level, left, right)) {
        int first = lower_bound(x.begin(), x.end(), left) - x.begin();
        int last = upper_bound(x.begin(), x.end(), right) - x.begin();
        this->calculate_value_in_range(x, y, first, last);
    } else
        this->calculate_value_in_range(x, y, 0, x.size());
}

realt Function::calculate_value(realt x) const
{
    calc_val_xx[0] = x;
    calc_val_yy[0] = 0.;
    calculate_value_in_range(calc_val_xx, calc_val_yy, 0, 1);
    return calc_val_yy[0];
}

void Function::calculate_value_deriv(const vector<realt> &x,
                                     vector<realt> &y,
                                     vector<realt> &dy_da,
                                     bool in_dx) const
{
    realt left, right;
    double cut_level = settings_->function_cutoff;
    if (cut_level != 0. && get_nonzero_range(cut_level, left, right)) {
        int first = lower_bound(x.begin(), x.end(), left) - x.begin();
        int last = upper_bound(x.begin(), x.end(), right) - x.begin();
        this->calculate_value_deriv_in_range(x, y, dy_da, in_dx, first, last);
    } else
        this->calculate_value_deriv_in_range(x, y, dy_da, in_dx, 0, x.size());
}

bool Function::get_center(realt* a) const
{
    if (center_idx_ != -1) {
        *a = av_[center_idx_];
        return true;
    }
    return false;
}

bool Function::get_iwidth(realt* a) const
{
    realt area, height;
    if (this->get_area(&area) && this->get_height(&height)) {
        *a = height != 0. ? area / height : 0.;
        return true;
    }
    return false;
}

/// return sth like: %f = Linear($foo, $_2)
string Function::get_basic_assignment() const
{
    string r = "%" + name + " = " + tp_->name + "(";
    v_foreach (string, i, used_vars_.names())
        r += (i == used_vars_.names().begin() ? "$" : ", $") + *i;
    r += ")";
    return r;
}

/// return sth like: %f = Linear(a0=$foo, a1=~3.5)
string Function::get_current_assignment(const vector<Variable*> &variables,
                                        const vector<realt> &parameters) const
{
    vector<string> vs;
    for (int i = 0; i < used_vars_.get_count(); ++i) {
        const Variable* v = variables[used_vars_.get_idx(i)];
        string t = get_param(i) + "="
            + (v->is_simple() ? v->get_formula(parameters) : "$" + v->name);
        vs.push_back(t);
    }
    return "%" + name + " = " + tp_->name + "(" + join_vector(vs, ", ") + ")";
}

void Function::replace_symbols_with_values(string &t, const char* num_fmt) const
{
    for (size_t i = 0; i < tp_->fargs.size(); ++i) {
        const string& symbol = tp_->fargs[i];
        string value = format1<double, 32>(num_fmt, av_[i]);
        // like replace_words(t,symbol,value) but adds () in a^n for a<0
        string::size_type pos = 0;
        while ((pos=t.find(symbol, pos)) != string::npos) {
            int k = symbol.size();
            if ((pos == 0
                    || !(isalnum(t[pos-1]) || t[pos-1]=='_' || t[pos-1]=='$'))
               && (pos+k==t.size() || !(isalnum(t[pos+k]) || t[pos+k]=='_'))) {
                string new_word = value;
                // rare special case
                if (pos+k < t.size() && t[pos+k] == '^' && av_[i] < 0)
                    new_word = "("+value+")";
                t.replace(pos, k, new_word);
                pos += new_word.size();
            } else
                pos++;
        }
    }
}

string Function::get_current_formula(const string& x, const char* num_fmt) const
{
    string t;
    if (contains_element(tp_->rhs, '#')) {
        t = tp_->name + "(";
        for (int i = 0; i != nv(); ++i) {
            string value = format1<double, 32>(num_fmt, av_[i]);
            t += value;
            t += (i+1 < nv() ? ", " : ")");
        }
    } else {
        t = tp_->rhs;
        replace_symbols_with_values(t, num_fmt);
    }

    replace_words(t, "x", x);
    return t;
}

int Function::get_param_nr(const string& param) const
{
    int n = index_of_element(tp_->fargs, param);
    if (n == -1)
        throw ExecuteError("%" + name + " has no parameter `" + param + "'");
    return n;
}

realt Function::get_param_value(const string& param) const  throw(ExecuteError)
{
    realt a;
    if (!param.empty() && islower(param[0]))
        return av_[get_param_nr(param)];
    else if (param == "Center" && get_center(&a)) {
        return a;
    } else if (param == "Height" && get_height(&a)) {
        return a;
    } else if (param == "FWHM" && get_fwhm(&a)) {
        return a;
    } else if (param == "Area" && get_area(&a)) {
        return a;
    } else if (get_other_prop(param, &a)) {
        return a;
    } else
        throw ExecuteError("%" + name + " (" + tp_->name
                           + ") has no parameter `" + param + "'");
}

realt Function::numarea(realt x1, realt x2, int nsteps) const
{
    if (nsteps <= 1)
        return 0.;
    realt xmin = min(x1, x2);
    realt xmax = max(x1, x2);
    realt h = (xmax - xmin) / (nsteps-1);
    vector<realt> xx(nsteps), yy(nsteps);
    for (int i = 0; i < nsteps; ++i)
        xx[i] = xmin + i*h;
    calculate_value(xx, yy);
    realt a = (yy[0] + yy[nsteps-1]) / 2.;
    for (int i = 1; i < nsteps-1; ++i)
        a += yy[i];
    return a*h;
}

/// search x in [x1, x2] for which %f(x)==val,
/// x1, x2, val: f(x1) <= val <= f(x2) or f(x2) <= val <= f(x1)
/// bisection + Newton-Raphson
realt Function::find_x_with_value(realt x1, realt x2, realt val,
                                  int max_iter) const
{
    realt y1 = calculate_value(x1) - val;
    realt y2 = calculate_value(x2) - val;
    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
        throw ExecuteError("Value " + S(val) + " is not bracketed by "
                           + S(x1) + "(" + S(y1+val) + ") and "
                           + S(x2) + "(" + S(y2+val) + ").");
    int n = 0;
    v_foreach (Multi, j, multi_)
        n = max(j->p + 1, n);
    vector<realt> dy_da(n+1);
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        swap(x1, x2);
    realt t = (x1 + x2) / 2.;
    for (int i = 0; i < max_iter; ++i) {
        //check if converged
        if (is_eq(x1, x2))
            return (x1+x2) / 2.;

        // calculate f and df
        calc_val_xx[0] = t;
        calc_val_yy[0] = 0;
        dy_da.back() = 0;
        calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
        realt f = calc_val_yy[0] - val;
        realt df = dy_da.back();

        // narrow range
        if (f == 0.)
            return t;
        else if (f < 0)
            x1 = t;
        else // f > 0
            x2 = t;

        // select new guess point
        realt dx = -f/df * 1.02; // 1.02 is to jump to the other side of point
        if ((t+dx > x2 && t+dx > x1) || (t+dx < x2 && t+dx < x1)  // outside
                            || i % 20 == 19) {                 // precaution
            //bisection
            t = (x1 + x2) / 2.;
        } else {
            t += dx;
        }
    }
    throw ExecuteError("The search has not converged in " + S(max_iter)
                       + " steps");
}

/// finds root of derivative, using bisection method
realt Function::find_extremum(realt x1, realt x2, int max_iter) const
{
    int n = 0;
    v_foreach (Multi, j, multi_)
        n = max(j->p + 1, n);
    vector<realt> dy_da(n+1);

    // calculate df
    calc_val_xx[0] = x1;
    dy_da.back() = 0;
    calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
    realt y1 = dy_da.back();

    calc_val_xx[0] = x2;
    dy_da.back() = 0;
    calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
    realt y2 = dy_da.back();

    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
        throw ExecuteError("Derivatives at " + S(x1) + " and " + S(x2)
                           + " have the same sign.");
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        swap(x1, x2);
    for (int i = 0; i < max_iter; ++i) {
        realt t = (x1 + x2) / 2.;

        // calculate df
        calc_val_xx[0] = t;
        dy_da.back() = 0;
        calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
        realt df = dy_da.back();

        // narrow range
        if (df == 0.)
            return t;
        else if (df < 0)
            x1 = t;
        else // df > 0
            x2 = t;

        //check if converged
        if (is_eq(x1, x2))
            return (x1+x2) / 2.;
    }
    throw ExecuteError("The search has not converged in " + S(max_iter)
                       + " steps");
}

} // namespace fityk
