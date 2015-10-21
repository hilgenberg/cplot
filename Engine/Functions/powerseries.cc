#include "Functions.h"

/*
void besselJ(const cnum &z0, const cnum &nu0, cnum &ret)
{
	cnum w = 1.0, z = z0;
	z *= z; z /= -4.0;
	cnum nu = nu0; nu += 1.0;
	cnum t; gamma(nu, t); w /= t;
	t = z0; t *= 0.5; cpow(t, nu0, t); w *= t;
	ret = w;
	for (int k = 1; k < 2800; ++k)
	{
		w *= z; w /= k; w /= nu;
		nu += 1.0;
		ret += w;
		if (absq(w) < 1e-30) return;
	}
}
*/

#define G2  11.8170450080771157683163372834325820874206975794306966442
#define G3  0
#define N   50
#define EPS 1e-40

void weierp(const cnum &z0, cnum &ret)
{
	cnum z(z0.real()-2.0*((int)z0.real()/2), z0.imag()-2.0*((int)z0.imag()/2));
	// reduce to z \in [-1,1]x[-i,i]
	//z=z0;
	//while(z.r<-1) z.r+=2; while(z.r>1) z.r-=2;
	//while(z.i<-1) z.i+=2; while(z.i>1) z.i-=2;
	if (z.real() < -1.0) z += 2.0; if(z.real() > 1.0) z -= 2.0;
	if (z.imag() < -1.0) z.imag(z.imag() + 2.0); if(z.imag() > 1.0) z.imag(z.imag() - 2.0);

	int NN = (int)((absq(z)/2)*N) + 20;
	if (NN > N) NN = N;
	
	z *= z;
	cnum pz, t;
	ret = z; invert(ret);
	double c[N];
	c[2] = G2*0.05;
	c[3] = 0.0; // = G3/28
	pz  = z; t = pz; t *= c[2]; ret += t;
	pz *= z; t = pz; t *= c[3]; ret += t;
	for (int k = 4; k < NN; ++k)
	{
		c[k] = 0;
		for (int m = 2; m <= k-2; ++m) c[k] += c[m] * c[k-m];
		c[k] *= 3.0 / ((2*k+1)*(k-3));
		pz *= z; t = pz; t *= c[k]; ret += t;
		//if(RSQ(t)<EPS) return;
	}
}

