#pragma once

#include "../cnum.h"
#include "boost_wrappers.h"

/**
 * @addtogroup cnum
 * @{
 */

//--- powerseries.cc ---------------------------------------------------------------------------------------------------

void weierp(const cnum &z0, cnum &ret);

//--- random.cc --------------------------------------------------------------------------------------------------------

void     real_rand(cnum &r);
double   real_rand();
void  riemann_rand(cnum &z);
void     disk_rand(cnum &z);
double normal_rand();
void   normal_rand(cnum &z);
void normal_z_rand(cnum &z);

//--- wrappers.cc ------------------------------------------------------------------------------------------------------

void  add(const cnum &z, const cnum &w, cnum &r);
void  sub(const cnum &z, const cnum &w, cnum &r);
void  mul(const cnum &z, const cnum &w, cnum &r);
void cdiv(const cnum &z, const cnum &w, cnum &r);
void cmod(const cnum &z, const cnum &w, cnum &r);
void  XOR(const cnum &z, const cnum &w, cnum &r);
double  add(double z, double w);
double  sub(double z, double w);
double  mul(double z, double w);
double  div(double z, double w);
double  mod(double z, double w);
double crmod(const cnum &z, const cnum &w);
double  XOR(double z, double w);

void   negate(const cnum &z, cnum &r);
void   invert(const cnum &z, cnum &r);
double negate(double      z);
double invert(double      z);

void construct(const cnum &re, const cnum &im, cnum &r);
void construct(double re, double im, cnum &r);

//--- bithacks.cc ------------------------------------------------------------------------------------------------------

double XOR(double z, double w);
double AND(double z, double w);
double  OR(double z, double w);
double NOT(double z);
double SHL(double z, double w);
double SHR(double z, double w);
double ROL(double z, double w);
double ROR(double z, double w);
double MANTISSA(double z);
double EXPONENT(double z);

void XOR(const cnum &z, const cnum &w, cnum &ret);
void AND(const cnum &z, const cnum &w, cnum &ret);
void  OR(const cnum &z, const cnum &w, cnum &ret);
void NOT(const cnum &z,                cnum &ret);
void SHL(const cnum &z, const cnum &w, cnum &ret);
void SHR(const cnum &z, const cnum &w, cnum &ret);
void ROL(const cnum &z, const cnum &w, cnum &ret);
void ROR(const cnum &z, const cnum &w, cnum &ret);
void MANTISSA(const cnum &z, cnum &ret);
void EXPONENT(const cnum &z, cnum &ret);

double DBIT(double x, double i0);
double IBIT(double x, double i0);
void DBIT(const cnum &z, const cnum &w, cnum &ret);
void IBIT(const cnum &z, const cnum &w, cnum &ret);

double I_XOR(double x, double y);
double I_OR(double x, double y);
double I_AND(double x, double y);
void I_XOR(const cnum &z, const cnum &w, cnum &ret);
void I_OR (const cnum &z, const cnum &w, cnum &ret);
void I_AND(const cnum &z, const cnum &w, cnum &ret);

//--- comparison.cc ----------------------------------------------------------------------------------------------------

void is_sm (const cnum &z, const cnum &w, cnum &r);
void is_sme(const cnum &z, const cnum &w, cnum &r);
void is_gt (const cnum &z, const cnum &w, cnum &r);
void is_gte(const cnum &z, const cnum &w, cnum &r);
void is_sm (const cnum &z, const cnum &w, cnum &r);
void is_sme(const cnum &z, const cnum &w, cnum &r);
void is_gt (const cnum &z, const cnum &w, cnum &r);
void is_gte(const cnum &z, const cnum &w, cnum &r);
void is_eq (const cnum &z, const cnum &w, cnum &r);
void is_neq(const cnum &z, const cnum &w, cnum &r);

double is_gt(double x, double y);
double is_sm(double x, double y);
double is_gt(const cnum &z, const cnum &w);
double is_sm(const cnum &z, const cnum &w);

void   r_min(const cnum &z, const cnum &w, cnum &ret);
void   r_max(const cnum &z, const cnum &w, cnum &ret);
double r_min(const cnum &z, const cnum &w);
double r_max(const cnum &z, const cnum &w);
double r_min(double z, double w);
double r_max(double z, double w);

void c_absmin(const cnum &z, const cnum &w, cnum &ret);
void c_absmax(const cnum &z, const cnum &w, cnum &ret);
void c_rmin  (const cnum &z, const cnum &w, cnum &ret);
void c_rmax  (const cnum &z, const cnum &w, cnum &ret);
void c_imin  (const cnum &z, const cnum &w, cnum &ret);
void c_imax  (const cnum &z, const cnum &w, cnum &ret);

double c_absmin(double z, double w);
double c_absmax(double z, double w);
double c_imin(double z, double w);
double c_imax(double z, double w);

//--- basic.cc ---------------------------------------------------------------------------------------------------------

double identity(double      z);
void   identity(const cnum &z, cnum &r);
double zero(double       );
double zero(double,double);
void sqrt_(const cnum &z, cnum &r);
void sqrt_(double z, cnum &r);
double rsqrt(double x);
void    sqr(const cnum &z, cnum &r);
double  sqr(double      z);
void   cube(const cnum &z, cnum &r);
double cube(double      z);
void   pow4(const cnum &z, cnum &r);
double pow4(double      z);
void   pow5(const cnum &z, cnum &r);
double pow5(double      z);
void   pow6(const cnum &z, cnum &r);
double pow6(double      z);
void   pow7(const cnum &z, cnum &r);
double pow7(double      z);
void   pow8(const cnum &z, cnum &r);
double pow8(double      z);
void   pow9(const cnum &z, cnum &r);
double pow9(double      z);
void  c_abs(const cnum &z, cnum &r);
double  abs(const cnum &z);
void c_absq(const cnum &z, cnum &r);
double absq(const cnum &z);
void  c_arg(const cnum &z, cnum &r);
double  arg(const cnum &z);
void  c_sgn(const cnum &z, cnum &r);
double  sgn(double      z);
void   c_re(const cnum &z, cnum &r);
double   re(const cnum &z);
void   c_im(const cnum &z, cnum &r);
double   im(const cnum &z);
void   arg2(const cnum &dx, const cnum &dy, cnum &r);
double arg2(double dx, double dy);

void   fowler_angle(const cnum &z, cnum &r);
double fowler_angle(const cnum &z);
double fowler_angle(double x);

void    swap(const cnum &z, cnum &r);
void    conj(const cnum &z, cnum &r);
void   round_(const cnum &z, cnum &r);
double round_(double    z);
void   floor(const cnum &z, cnum &r);
void    ceil(const cnum &z, cnum &r);
void    c_sp(const cnum &x, const cnum &y, cnum &r);
double    sp(const cnum &x, const cnum &y);
void   c_det(const cnum &x, const cnum &y, cnum &r);
double   det(const cnum &x, const cnum &y);
void  c_dist(const cnum &x, const cnum &y, cnum &r);
double  dist(const cnum &x, const cnum &y);
double  dist(double      x, double      y);
void   hypot(const cnum &x, const cnum &y, cnum &r);
void mida(const cnum &x, const cnum &y, cnum &r);
void midg(const cnum &x, const cnum &y, cnum &r);
void midh(const cnum &x, const cnum &y, cnum &r);
double mida(double x, double y);
double midg(double x, double y);
double midh(double x, double y);
void qtest(const cnum &a, const cnum &b, const cnum &c, const cnum &d, cnum &r);
void cblend_(const cnum &a, const cnum &b, const cnum &time, cnum &r);
void blend_(const cnum &a, const cnum &b, const cnum &time, cnum &r);
void mix_(const cnum &a, const cnum &b, const cnum &time, cnum &r);

double clamp(double x); // to [0,1]
void   clamp(const cnum &x, cnum &ret);

double clamps(double x, double a, double b); // to [a,b] or [b,a]
void   clamps(const cnum &x, const cnum &a, const cnum &b, cnum &ret);

//--- exp.cc -----------------------------------------------------------------------------------------------------------

void exp_(const cnum &x, cnum &r);
void log_(const cnum &x, cnum &r);
void log10(const cnum &x, cnum &r);
void log2(const cnum &x, cnum &r);
void cpow(const cnum &x, const cnum &y, cnum &r);
void pow(const cnum &x,double y, cnum &r);
void pow(const cnum &x, unsigned n, cnum &r);
void rcpow(double x, double y, cnum &r);
void spow(const cnum &x, const cnum &y, cnum &r);
double spow(double x, double y);

void cos(const cnum &v, cnum &r);
void sin(const cnum &v, cnum &r);
void tan(const cnum &v, cnum &r);
void cot(const cnum &v, cnum &r);
void sec(const cnum &v, cnum &r);
void csc(const cnum &v, cnum &r);
double cot(double v);
double sec(double v);
double csc(double v);
void cosh(const cnum &v, cnum &r);
void sinh(const cnum &v, cnum &r);
void tanh(const cnum &v, cnum &r);
void coth(const cnum &v, cnum &r);
void sech(const cnum &v, cnum &r);
void csch(const cnum &v, cnum &r);
/*double cosh(double v);
double sinh(double v);
double tanh(double v);*/
double coth(double v);
double sech(double v);
double csch(double v);
void acos(const cnum &z, cnum &r);
void asin(const cnum &z, cnum &r);
void atan(const cnum &z, cnum &r);
void acot(const cnum &z, cnum &r);
void asec(const cnum &z, cnum &r);
void acsc(const cnum &z, cnum &r);
void asinh(const cnum &z, cnum &r);
void acosh(const cnum &z, cnum &r);
void atanh(const cnum &z, cnum &r);
void acoth(const cnum &z, cnum &r);
void acsch(const cnum &z, cnum &r);
void asech(const cnum &z, cnum &r);

//--- fractals.cc ------------------------------------------------------------------------------------------------------

void mandel(const cnum &c, cnum &r);
void julia(const cnum &z0, const cnum &c, cnum &r);

//--- gamma.cc ---------------------------------------------------------------------------------------------------------

void   gamma_(const cnum &z, cnum &r);
double gamma_(double x);
void   fakt(const cnum &z, cnum &r);
double fakt(double x);
void digamma(const cnum &A, cnum &r);
void trigamma(const cnum &A, cnum &r);
void beta(const cnum &x, const cnum &y, cnum &r);
void binomco(const cnum &n, const cnum &k, cnum &r);

//--- statistics.cc ----------------------------------------------------------------------------------------------------

void pdf_normal(const cnum &x, cnum &r);
void erf_ (const cnum &z, cnum &r);
void erfc_(const cnum &z, cnum &r);
double erf_ (double x);
double erfc_(double x);

/** @} */

