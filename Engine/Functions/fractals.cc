#include "Functions.h"

#define M 150 /* iteration limit */
using std::log;

const static double scale = 1.0/log(1.0+M);

void mandel(const cnum &c, cnum &ret)
{
	cnum z = c;
	int  i = 0;
	
	do{ z *= z; z += c; }while (++i < M && absq(z) < 4.0);
	
	ret = log(i) * scale;
}

void julia(const cnum &z0, const cnum &c, cnum &ret)
{
	cnum z = z0;
	int  i = 0;
	
	do{ z *= z; z += c; }while (++i < M && absq(z) < 4.0);
	
	ret = log(i) * scale;
}
