#include "Functions.h"

#define BOOL(op) ((is_real(z) && is_real(w)) ? z.real() op w.real() ? 1.0 : 0.0 : UNDEFINED)
#define RF(f)    ((is_real(z) && is_real(w)) ? f(z.real(), w.real()) : UNDEFINED)

void is_sm (const cnum &z, const cnum &w, cnum &ret){ ret = BOOL(< ); }
void is_sme(const cnum &z, const cnum &w, cnum &ret){ ret = BOOL(<=); }
void is_gt (const cnum &z, const cnum &w, cnum &ret){ ret = BOOL(> ); }
void is_gte(const cnum &z, const cnum &w, cnum &ret){ ret = BOOL(>=); }
void is_eq (const cnum &z, const cnum &w, cnum &ret){ ret = z==w ? 1.0 : 0.0; }
void is_neq(const cnum &z, const cnum &w, cnum &ret){ ret = z==w ? 0.0 : 1.0; }

double is_gt(double x, double y){ return x > y ? 1.0 : 0.0; }
double is_sm(double x, double y){ return x < y ? 1.0 : 0.0; }
double is_sm (const cnum &z, const cnum &w){ return BOOL(< ); }
double is_sme(const cnum &z, const cnum &w){ return BOOL(<=); }
double is_gt (const cnum &z, const cnum &w){ return BOOL(> ); }
double is_gte(const cnum &z, const cnum &w){ return BOOL(>=); }

void log_and(const cnum &z, const cnum &w, cnum &ret){ ret = (isz(z) || isz(w)) ? 0.0 : 1.0; }
void log_or (const cnum &z, const cnum &w, cnum &ret){ ret = (isz(z) && isz(w)) ? 0.0 : 1.0; }

void cond(const cnum &c, const cnum &z1, const cnum &z2, cnum &ret){ ret = isz(c) ? z2 : z1; }

void   r_min(const cnum &z, const cnum &w, cnum &ret){ ret = RF(std::min); }
void   r_max(const cnum &z, const cnum &w, cnum &ret){ ret = RF(std::max); }
double r_min(const cnum &z, const cnum &w){ return RF(std::min); }
double r_max(const cnum &z, const cnum &w){ return RF(std::max); }
double r_min(double z, double w){ return std::min(z, w); }
double r_max(double z, double w){ return std::max(z, w); }

void c_absmin(const cnum &z, const cnum &w, cnum &ret){ ret = abs(z) < abs(w) ? z : w; }
void c_absmax(const cnum &z, const cnum &w, cnum &ret){ ret = abs(z) > abs(w) ? z : w; }
void c_rmin(const cnum &z, const cnum &w, cnum &ret){ ret = z.real() < w.real() ? z : w; }
void c_rmax(const cnum &z, const cnum &w, cnum &ret){ ret = z.real() > w.real() ? z : w; }
void c_imin(const cnum &z, const cnum &w, cnum &ret){ ret = z.imag() < w.imag() ? z : w; }
void c_imax(const cnum &z, const cnum &w, cnum &ret){ ret = z.imag() > w.imag() ? z : w; }

double c_absmin(double z, double w){ return abs(z) < abs(w) ? z : w; }
double c_absmax(double z, double w){ return abs(z) > abs(w) ? z : w; }
double c_imin(double z, double w){ (void)z; return w; }
double c_imax(double z, double w){ (void)z; return w; }

void c_rimin(const cnum &z, cnum &r){ r = z.imag() <= z.real() ? z.imag() : z.real(); }
void c_rimax(const cnum &z, cnum &r){ r = z.imag() >= z.real() ? z.imag() : z.real(); }
double rimin(const cnum &z){ return z.imag() <= z.real() ? z.imag() : z.real(); }
double rimax(const cnum &z){ return z.imag() >= z.real() ? z.imag() : z.real(); }
double rimin(double      z){ return z < 0.0 ? z : 0.0; }
double rimax(double      z){ return z > 0.0 ? z : 0.0; }
