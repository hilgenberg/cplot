#include "GL_Mask.h"
#include "../Geometry/Vector.h"
#include "../Geometry/Matrix.h"
#include "../Geometry/Triangle.h"
#include <cassert>
#include <random>
#include <GL/gl.h>

void GL_Mask::save(Serializer &s) const
{
	if (custom())
	{
		CHECK_VERSION(FILE_VERSION_1_7, "custom alpha mask");
		check_data();
	}
	
	if (_style > Mask_Fan) CHECK_VERSION(FILE_VERSION_1_4, "hexagon mask");
	s._enum(_style, Mask_Custom, Mask_Hexagon);
	
	if (custom())
	{
		s._uint32(_w);
		s._uint32(_h);
		s._data(_data);
	}

	s._double(_density);
}
void GL_Mask::load(Deserializer &s)
{
	s._enum(_style, s.version() < FILE_VERSION_1_7 ? Mask_Off : Mask_Custom,
	                s.version() < FILE_VERSION_1_4 ? Mask_Fan : Mask_Hexagon);

	if (custom())
	{
		s._uint32(_w);
		s._uint32(_h);
		s._data(_data);
		_update = false;

		if (_data.size() != (size_t)_w * _h)
		{
			_w = _h = 0;
			_data.clear();
			throw std::runtime_error("Reading alpha mask data failed. This file seems to be corrupted.");
		}
	}
	else
	{
		update_size();
		_update = true;
	}

	s._double(_density);
}

#define ALPHA(A) \
do{ \
	unsigned char *dp = _data.data(); \
	double iw = 2.0/((int)_w-1);\
	for(int y_ = 0; y_ < (int)_h; ++y_)\
	{\
		double y = y_*iw - 1.0; (void)y; \
		for(int x_ = 0; x_ < (int)_w; ++x_) \
		{\
			double x  = x_*iw - 1.0; (void)x; \
			*dp++ = ((A) ? 255 : 0); \
		}\
	}\
}while(0)

#define ALPHA_DISCRETE(A) \
do{ \
	unsigned char *dp = _data.data();\
	for (int y = 0; y < (int)_h; ++y)\
	{\
		for (int x = 0; x < (int)_w; ++x)\
		{\
			*dp++ = ((A) ? 255 : 0);\
		}\
	}\
}while(0)

static inline double sqr(double x){ return x*x; }

static bool hex(double x, double y, double r)
{
	// hexagon around 0 with radius r contains (x,y)?
	double rr = r*r;
	double vx = 1.0, vy = 1.0/sqrt(3.0);
	return y*y < rr*0.75 && sqr(x*vx + y*vy) < rr && sqr(y*vy - x*vx) < rr;
}

void GL_Mask::update_size()
{
	switch (_style)
	{
		case Mask_Custom:     break;
		case Mask_Off:        _w = _h = 0; break;
		case Mask_HLines:
		case Mask_VLines:     _w = _h = 101; break;
		case Mask_Chessboard: _w = _h = 2 + 2*(int)floor(fabs(2.0*_density-1.0)*6.0); break;
		default:              _w = _h = 512; break;
	}
}

const std::vector<unsigned char> &GL_Mask::data() const
{
	if (!_update) return _data;
	assert(_style != Mask_Custom);
	_update = false;

	#ifdef DEBUG
	unsigned w0 = _w, h0 = _h;
	const_cast<GL_Mask*>(this)->update_size();
	assert(_w == w0 && _h == h0);
	#endif
	
	_data.resize((size_t)_w * _h);
	double d = 2.0*_density-1.0;
	switch (_style)
	{
		case Mask_Custom: break;
		case Mask_Off:    break;
		
		case Mask_Circles:
			if (d > 0)
				ALPHA(hypot(x,y) > d*M_SQRT2);
			else
				ALPHA(hypot(x,y) < (1.0+d)*M_SQRT2);
			break;
		case Mask_Squares:
			if (d > 0)
				ALPHA(fabs(x) > d || fabs(y) > d);
			else
				ALPHA(fabs(x) < 1.0+d && fabs(y) < 1.0+d);
			break;
		case Mask_Rounded_Rect:
		{
			double r = (d > 0 ? d*(0.75 - 0.5*d) : (1.0+d)*(0.25 - 0.5*d));
			double rq = r*r;
			if (d > 0)
				ALPHA(!(fabs(x) < d && fabs(y) < d-r || fabs(y) < d && fabs(x) < d-r || sqr(fabs(x)-(d-r)) + sqr(fabs(y)-(d-r)) < rq));
			else
				ALPHA(fabs(x) <= 1.0+d && fabs(y) <= 1.0+d-r || fabs(y) <= 1.0+d && fabs(x) <= 1.0+d-r || sqr(fabs(x)-(1.0+d-r)) + sqr(fabs(y)-(1.0+d-r)) <= rq);
			break;
		}
		case Mask_Triangles:
		{
			double dd = fabs(d);
			if (d > 0) dd = 1.0-d;
			Triangle T1(P2d(0.0,2.0*dd/3.0), P2d(1.0-dd, 1.0-dd/3.0), P2d(-1.0+dd, 1.0-dd/3.0));
			Triangle T2(P2d(1.0,1.0-2.0/3.0*dd), P2d(dd, dd/3.0), P2d(2.0-dd, dd/3.0));
			if (d > 0)
				ALPHA(!T1.contains(x,y) && !T1.contains(x,-y) && !T2.contains(x,y) && !T2.contains(-x,y) && !T2.contains(x,-y) && !T2.contains(-x,-y));
			else
				ALPHA(T1.contains(x,y) || T1.contains(x,-y) || T2.contains(x,y) || T2.contains(-x,y) || T2.contains(x,-y) || T2.contains(-x,-y));
			break;
		}
		case Mask_Chessboard:
		{
			int invert = (_density < 0.5 ? 1 : 0);
			ALPHA_DISCRETE(((x ^ y) & 1) ^ invert);
			break;
		}
		case Mask_HLines:
		{
			int w2 = (int)(_w-1)/2;
			int sw = (int)(w2 * (1.0-_density));
			ALPHA_DISCRETE(abs(y-w2) > sw);
			break;
		}
		case Mask_VLines:
		{
			int w2 = (int)(_w-1)/2;
			int sw = (int)(w2 * (1.0-_density));
			ALPHA_DISCRETE(abs(x-w2) > sw);
			break;
		}
		case Mask_Rings:
		{
			int w2 = (int)_w/2;
			int rd = 2+(int)(w2*fabs(d));
			if (d > 0)
				ALPHA_DISCRETE((int)hypot(x-w2, y-w2) % rd < rd/2);
			else
				ALPHA_DISCRETE((int)hypot(x-w2, y-w2) % rd >= rd/2);
			break;
		}
		case Mask_Static:
		{
			std::mt19937 rng(1234234924);
			std::uniform_real_distribution<> dist(0.0, 1.0);
			ALPHA(dist(rng) > _density);
			break;
		}
		case Mask_Fan:
		{
			double n = 4.0 + 2.0*floor(16.0*_density);
			double p = 0.0;//8.0*m_density - floor(8.0*m_density);
			p *= 2.0*M_PI;
			assert(p >= 0.0);
			ALPHA(fmod((2.0*M_PI+atan2(y,x))*n+p, 2.0*M_PI) < M_PI);
			break;
		}
		case Mask_Hexagon:
		{
			double yf = sqrt(1.0/3.0);
			double dd = fabs(d);
			if (d > 0) dd = 1.0-dd;
			double r = dd*2.0/3.0;
			if (d > 0)
				ALPHA(hex(x,y*yf,r) || hex(x-1.0, y*yf-yf, r) || hex(x+1.0, y*yf-yf, r) || hex(x-1.0, y*yf+yf, r) || hex(x+1.0, y*yf+yf, r));
			else
				ALPHA(!(hex(x,y*yf,r) || hex(x-1.0, y*yf-yf, r) || hex(x+1.0, y*yf-yf, r) || hex(x-1.0, y*yf+yf, r) || hex(x+1.0, y*yf+yf, r)));
			break;
			
		}
		default:
			assert(false);
			ALPHA(x+y < 1000.0);
			break;
	}

	return _data;
}

void GL_Mask::mix(std::vector<unsigned char> &dst) const
{
	check_data();
	const unsigned char *d = _data.data();
	const size_t n = _data.size();
	dst.resize(n);
	unsigned char *d1 = dst.data();

	unsigned level = (unsigned)(_density * (255 * 2 - 1));
	assert(level <= 254*255);
	if (level > 254)
	{
		level -= 255;
		for (size_t i = 0; i < n; ++i) *d1++ = (*d++ > level ? 0 : 255);
	}
	else
	{
		for (size_t i = 0; i < n; ++i) *d1++ = (*d++ > level ? 255 : 0);
	}
}

void GL_Mask::upload(GL_RM &rm, const GL_MaskScale &scale) const
{
	rm.upload_mask(*this);
	assert(!_update);

	rm.setup(true, false, true);

	double mw, mh;
	
	switch (_style)
	{
		case Mask_Custom:     mw = _w; mh = _h; break;
		case Mask_Triangles:  mw = 1.0; mh = sqrt(3.0); break;
		case Mask_Hexagon:    mw = sqrt(3.0); mh = 1.0; break;
		case Mask_Chessboard: glScaled(0.5, 0.5, 1.0); // fallthrough
		default:              mw = mh = 1.0; break;
	}

	glScaled(1.0 / scale.sx, 1.0 / scale.sy, 1.0);
	glTranslated(-scale.dx, -scale.dy, 0.0);

	if (mw < mh)
	{
		glTranslated(0.0, 0.5 * (mh - mw) / mh, 0.0);
		glScaled(1.0, mw / mh, 1.0);
	}
	else
	{
		glTranslated(0.5 * (mw - mh) / mw, 0.0, 0.0);
		glScaled(mh / mw, 1.0, 1.0);
	}

	glAlphaFunc(GL_GREATER, 1e-8f);
}
