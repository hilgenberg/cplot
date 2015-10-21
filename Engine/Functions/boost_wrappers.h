#pragma once
#include "../cnum.h"

/**
 * @addtogroup cnum
 * @{
 */

#define BESSEL(NAME, FUNC) \
void   NAME(const cnum &n, const cnum &z, cnum &r);\
double NAME(const cnum &n, const cnum &z);\
double NAME(double n, double z);
BESSEL(bessel_J, cyl_bessel_j)
BESSEL(bessel_Y, cyl_neumann)
BESSEL(bessel_I, cyl_bessel_i)
BESSEL(bessel_K, cyl_bessel_k)
#undef BESSEL

void   expint_i(const cnum &z, cnum &r);
double expint_i(const cnum &z);
double expint_i(double z);

void   expint_n(const cnum &n, const cnum &z, cnum &r);
double expint_n(const cnum &n, const cnum &z);
double expint_n(double n, double z);

#define AIRY(NAME, FUNC) \
void NAME(const cnum &z, cnum &r);\
double NAME(const cnum &z);\
double NAME(double z);
AIRY(airy_ai, airy_ai)
AIRY(airy_bi, airy_bi)
AIRY(airy_ai_prime, airy_ai_prime)
AIRY(airy_bi_prime, airy_bi_prime)
#undef AIRY

/** @} */
