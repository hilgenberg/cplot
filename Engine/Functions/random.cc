#include "../cnum.h"
#include "../../Graphs/Geometry/Vector.h"

#if 1

#include <random>
#include <cassert>
static std::mt19937 seed_rng; // TODO: lock this
static thread_local std::mt19937 rng(seed_rng());
static std::uniform_real_distribution<> udist(-1.0, 1.0);

#define URND (udist(rng))

#else

#include <cstdlib>
static double rnd()
{
	return (double)arc4random_uniform(std::numeric_limits<u_int32_t>::max()) / std::numeric_limits<u_int32_t>::max();
}
// URND: [-1,1]
#define URND (2.0*rnd()-1.0)

#endif

double real_rand()
{
	// [-1,1]
	return URND;
}
void real_rand(cnum &z)
{
	// re(z) in [-1,1], im(z) = 0
	// should always be optimized to the real version above
	z.real(URND);
	z.imag(0.0);
}

double normal_rand()
{
	// (0,1)-normal variate (Marsaglia polar)
	double r;
	cnum z;
	do{
		z.real(URND);
		z.imag(URND);
		r = absq(z);
	}while(r >= 1.0);
	return z.real() * sqrt(-2.0*log(r) / r);
}
void normal_rand(cnum &z)
{
	// normal re(z), im(z) = 0
	// should always be optimized to the real version above
	double r;
	do{
		z.real(URND);
		z.imag(URND);
		r = absq(z);
	}while(r >= 1.0);
	z.real(z.real() * sqrt(-2.0*log(r) / r));
	z.imag(0.0);
}

void normal_z_rand(cnum &z)
{
	// normal real and imag parts
	double r;
	do{
		z.real(URND);
		z.imag(URND);
		r = absq(z);
	}while(r >= 1.0);
	z *= sqrt(-2.0*log(r) / r);
}

void riemann_rand(cnum &z)
{
	// uniform distribution on riemann sphere
	P3d v; double d;
	do{
		v.x = URND;
		v.y = URND;
		d = v.x*v.x + v.y*v.y;
	}while(d >= 1.0);
	
	v.z = 1.0 - 2.0*d;
	d = 2.0*sqrt(1.0-d);
	v.x *= d;
	v.y *= d;
	riemann(v, z);
}

void disk_rand(cnum &z)
{
	// uniform distibution on the unit disk
	do{
		z.real(URND);
		z.imag(URND);
	}while(absq(z) >= 1.0);
}

