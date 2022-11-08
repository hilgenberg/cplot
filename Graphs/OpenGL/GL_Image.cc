#include "GL_Image.h"
#include "../../Utility/Mutex.h"
#include <cassert>
#include <random>
#include <map>
#include <iostream>
#include <GL/gl.h>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------
// pattern images
//----------------------------------------------------------------------------------------------------------------------

static std::map<GL_ImagePattern, GL_Image> patterns;
static Mutex pattern_mutex;

#define P(x,y) data[(((y)%h) * (size_t)w + ((x)%w)) * 4]
#define PP(x,y,c) do{ int c_ = (int)(c); P(x,y) = (c_ < 0 ? 0 : c_ > 255 ? 255 : (unsigned char)c_); }while(0)

static void gen_plasma(unsigned char *data, unsigned w, unsigned h)
{
	//unsigned seed = (unsigned)time(NULL);
	//std::cerr << "seed = " << seed << std::endl;
	std::mt19937 rng(1353167819);
	
	std::uniform_int_distribution<>  idist(0, 255);
	std::uniform_real_distribution<> rdist(-1.0, 1.0);
	
	assert(!(w & (w-1)));
	assert(!(h & (h-1)));
	assert(w == h);
	memset(data, 255, (size_t)w * h * 4); // for alpha
	for (int i = 0; i < 3; ++i, ++data)
	{
		PP(0, 0, idist(rng));
		for (unsigned d = w; d; d /= 2)
		{
			for (unsigned y = 0; y < h; y += d)
			{
				for (unsigned x = 0; x < w; x += d)
				{
					int c1 = P(x,  y);
					int c2 = P(x+d,y);
					int c3 = P(x,  y+d);
					int c4 = P(x+d,y+d);
					PP(x+d/2, y, (c1+c2+0.6*rdist(rng)*d)/2);
					PP(x, y+d/2, (c1+c3+0.6*rdist(rng)*d)/2);
					PP(x+d/2, y+d/2, (c1+c2+c3+c4+0.6*rdist(rng)*d)/4);
				}
			}
		}
	}
}
#undef P
#undef PP

static double hue2rgb(double p, double q, double t)
{
	if(t < 0.0) t += 6.0;
	if(t > 6.0) t -= 6.0;
	if(t < 1.0) return p + (q - p) * t;
	if(t < 3.0) return q;
	if(t < 4.0) return p + (q - p) * (4.0 - t);
	return p;
}
static void hsl_to_rgb(double h, double s, double l, unsigned char *dst)
{
	assert(s >= 0.0 && s <= 1.0);
	assert(l >= 0.0 && l <= 1.0);
	if (s == 0.0) // achromatic
	{
		unsigned char y = (unsigned char)(255.0*l);
		*dst++ = y;
		*dst++ = y;
		*dst++ = y;
		*dst++ = 255;
	}
	else
	{
		h *= 0.5*M_1_PI; if (h < 0.0) ++h;
		assert(h >= 0.0 && h <= 1.0);
		double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
		double p = 2.0 * l - q;
		*dst++ = (unsigned char)(255.0*hue2rgb(p, q, 6.0*h + 2.0));
		*dst++ = (unsigned char)(255.0*hue2rgb(p, q, 6.0*h));
		*dst++ = (unsigned char)(255.0*hue2rgb(p, q, 6.0*h - 2.0));
		*dst++ = 255;
	}
}

#define LOOP(foo) do{ for(int y = 0; y < (int)im._h; ++y) for(int x = 0; x < (int)im._w; ++x){ foo }}while(0)
#define COLOR(r,g,b) LOOP( *d++ = (unsigned char)(r); *d++ = (unsigned char)(g); *d++ = (unsigned char)(b); *d++ = 255; )
#define COLORA(r,g,b,a) LOOP( *d++ = (unsigned char)(r); *d++ = (unsigned char)(g); *d++ = (unsigned char)(b); *d++ = (unsigned char)(a); )
#define GREY(Y)      LOOP( unsigned char g = (unsigned char)(Y); *d++ = g; *d++ = g; *d++ = g; *d++ = 255; )
#define GREYA(Y,A)   LOOP( unsigned char g = (unsigned char)(Y); *d++ = g; *d++ = g; *d++ = g; *d++ = (unsigned char)(A); )
#define FLOOP(foo) do{ for(int yi = 0; yi < (int)im._h; ++yi){ double y=(double)yi/(im._h-1); for(int xi = 0; xi < (int)im._w; ++xi){ double x = (double)xi/(im._w-1); foo }}}while(0)
#define FCOLOR(r,g,b) FLOOP( *d++ = (unsigned char)(255.0*(r)); *d++ = (unsigned char)(255.0*(g)); *d++ = (unsigned char)(255.0*(b)); *d++ = 255; )
#define FCOLORHSL(h,s,l) FLOOP( hsl_to_rgb(h,s,l,d); d += 4; )

static void pattern_dim(GL_ImagePattern p, unsigned &w, unsigned &h)
{
	w = h = ((p==IP_PLASMA || p==IP_PHASE || p==IP_UNITS) ? 512 : 256);
}
static bool pattern_opacity(GL_ImagePattern p)
{
	return p == IP_UNITS ? 0 : p == IP_CUSTOM ? -1 : 1;
}
GL_Image &GL_Image::pattern(GL_ImagePattern p)
{
	Lock lock(pattern_mutex);
	auto ii = patterns.find(p);
	if (ii == patterns.end())
	{
		GL_Image &im = patterns[p];
		unsigned w, h;
		pattern_dim(p, w, h);
		unsigned char *d0 = im.redim(w, h), *d = d0;
		im._pattern = p;
		
		switch(p)
		{
			case IP_CUSTOM:   assert(false); return im;
				
			case IP_XOR:      COLOR(y, ~x, y^x); break;
			case IP_XOR_GREY: GREY(y & x); break;
			case IP_COLORS:   COLOR(x, y, ~y); break;
			case IP_CHECKER:  GREY((x % 16 >= 8 ? 0 : 255) ^ (y % 16 >= 8 ? 0 : 255)); break;
			case IP_PLASMA:   gen_plasma(d, 512, 512); break;
			case IP_COLORS_2: FCOLOR(sin(y*M_PI), sin(x*M_PI), 1.0-sin(y*M_PI)); break;
			case IP_PHASE:    FCOLORHSL(atan2(y-0.5, x-0.5), 0.99, std::min(1.0, 2.0*hypot(x-0.5, y-0.5))); break;
			case IP_UNITS:{   FCOLORHSL(atan2(y-0.5, x-0.5), 0.99, std::min(1.0, 2.0*hypot(x-0.5, y-0.5)));
				double r = 0.02, f;
				d = d0;
				FLOOP(f = std::min(r, std::min(hypot(y-0.5,  x-0.5), std::min(fabs(hypot(y-0.5,  x-0.5)-0.5),
						  std::min(hypot(y-0.75, x-0.5), std::min(hypot(y-0.25, x-0.5), std::min(hypot(y-0.5, x-0.75),
						  hypot(y-0.5, x-0.25))))))) / r;
					  d[0] = (unsigned char)(f*128+(1.0-f)*d[0]);
					  d[1] = (unsigned char)(f*128+(1.0-f)*d[1]);
					  d[2] = (unsigned char)(f*128+(1.0-f)*d[2]);
					  d[3] = (unsigned char)((1.0-f)*255);
					  d += 4;);
				break;}
		}
		im._opacity = pattern_opacity(p);
		return im;
	}
	
	return ii->second;
}

//----------------------------------------------------------------------------------------------------------------------
// GL_Image
//----------------------------------------------------------------------------------------------------------------------

#ifdef __linux__
#define STB_IMAGE_IMPLEMENTATION
#include "../../Linux/stb/stb_image.h"

bool GL_Image::load(const std::string &path)
{
	int x,y,n;
	unsigned char *d1 = stbi_load(path.c_str(), &x, &y, &n, 4);
	if (!d1)
	{
		fprintf(stderr, "Error loading %s: %s\n", path.c_str(), stbi_failure_reason());
		return false;
	}
	if (x <= 0 || y <= 0 || n <= 0)
	{
		stbi_image_free(d1);
		return false;
	}

	unsigned char *d2 = redim(x, y);
	if (!d2) return false;
	memcpy(d2, d1, x*y*4);
	stbi_image_free(d1);
	return true;
}
#endif

void GL_Image::save(Serializer &s) const
{
	s.bool_(_pattern == IP_CUSTOM);
	if (_pattern == IP_CUSTOM)
	{
		check_data();
		s.uint32_(_w);
		s.uint32_(_h);

		if (s.version() >= FILE_VERSION_1_7)
		{
			size_t n = (size_t)_w * _h;
			std::vector<unsigned char> channel(4*n);
			unsigned char *cd = channel.data();

			const unsigned char *d = _data.data() + 3;
			for (size_t j = 0; j < n; ++j, d += 4) *cd++ = *d;
			for (int i = 2; i >= 0; --i)
			{
				d = _data.data() + i;
				for (size_t j = 0; j < n; ++j, d += 4) *cd++ = *d - d[1];
			}
			s.data_(channel);
		}
		else
		{
			s.data_(_data);
		}
	}
	else
	{
		if (_pattern > IP_COLORS_2) CHECK_VERSION(FILE_VERSION_1_4, "phase and units textures");
		s.enum_(_pattern, IP_COLORS, IP_UNITS);
	}
}
void GL_Image::load(Deserializer &s)
{
	bool custom;
	s.bool_(custom);
	if (custom)
	{
		_pattern = IP_CUSTOM;
		s.uint32_(_w);
		s.uint32_(_h);
		
		if (s.version() >= FILE_VERSION_1_7)
		{
			size_t n = (size_t)_w * _h;
			_data.resize(n * 4);
			std::vector<unsigned char> channel;
			s.data_(channel);
			if (channel.size() != 4*n)
			{
				_w = _h = 0;
				_data.clear();
				throw std::runtime_error("Reading texture data failed. This file seems to be corrupted.");
			}
			const unsigned char *cd = channel.data();
			unsigned char *d = _data.data() + 3;
			for (size_t j = 0; j < n; ++j, d += 4) *d = *cd++;
			for (int i = 2; i >= 0; --i)
			{
				d = _data.data() + i;
				for (size_t j = 0; j < n; ++j, d += 4) *d = *cd++ + d[1];
			}
		}
		else
		{
			s.data_(_data);
			
			if (_data.size() != (size_t)_w * _h * 4)
			{
				_w = _h = 0;
				_data.clear();
				throw std::runtime_error("Reading texture data failed. This file seems to be corrupted.");
			}
		}
		
		_opacity = -1;
		check_data();
	}
	else
	{
		s.enum_(_pattern, IP_COLORS, s.version() < FILE_VERSION_1_4 ? IP_COLORS_2 : IP_UNITS);
		pattern_dim(_pattern, _w, _h);
		_data.clear();
		_opacity = pattern_opacity(_pattern);
	}
	++_state;
	modify();
}

GL_Image &GL_Image::operator=(GL_ImagePattern p)
{
	if (_pattern == p) return *this;
	_pattern = p;
	
	if (p == IP_CUSTOM)
	{
		assert(false); // should call redim(...) instead
		redim(_w, _h);
		return *this;
	}

	pattern_dim(p, _w, _h);
	_data.clear();
	_opacity = pattern_opacity(_pattern);
	++_state;
	modify();

	return *this;
}

const std::vector<unsigned char> &GL_Image::data() const
{
	return (_pattern == IP_CUSTOM ? _data : GL_Image::pattern(_pattern)._data);
}

void GL_Image::mix(const GL_Color &base, float alpha, std::vector<unsigned char> &dst) const
{
	check_data();
	const std::vector<unsigned char> &d = data();
	dst.resize(d.size());
	
	unsigned br = (base.r <= 0.0f ? 0 : base.r >= 1.0f ? 255 : (unsigned char)(255.0f * base.r));
	unsigned bg = (base.g <= 0.0f ? 0 : base.g >= 1.0f ? 255 : (unsigned char)(255.0f * base.g));
	unsigned bb = (base.b <= 0.0f ? 0 : base.b >= 1.0f ? 255 : (unsigned char)(255.0f * base.b));
	unsigned ba = (base.a <= 0.0f ? 0 : base.a >= 1.0f ? 255 : (unsigned char)(255.0f * base.a));
	unsigned a1 = (alpha  <= 0.0f ? 0 : alpha  >= 1.0f ? 255 : (unsigned char)(255.0f * alpha ));

	for (size_t i = 0, n = d.size(); i < n; i += 4)
	{
		unsigned a = (a1*d[i+3])/255;
		dst[i  ] = (unsigned char)((d[i  ]*a + br*(255-a))/255);
		dst[i+1] = (unsigned char)((d[i+1]*a + bg*(255-a))/255);
		dst[i+2] = (unsigned char)((d[i+2]*a + bb*(255-a))/255);
		dst[i+3] = (unsigned char)ba;
	}
}
void GL_Image::mix(float alpha, std::vector<unsigned char> &dst) const
{
	check_data();
	const std::vector<unsigned char> &d = data();
	dst = d;

	if (alpha < 1.0-1e-5)
	{
		unsigned a1 = (alpha <= 0.0f ? 0 : alpha >= 1.0f ? 255 : (unsigned char)(255.0f*alpha));
		for (size_t i = 3; i < d.size(); i += 4)
		{
			dst[i] = (unsigned char)(a1 * dst[i] / 255);
		}
	}
}

void GL_Image::prettify(bool circle)
{
	assert(_pattern == IP_CUSTOM);
	check_data();
	unsigned char *d = _data.data();
	
	size_t n = (size_t)_w * _h, l = data().size(), no = 0, nt = 0;
	for (size_t i = 3; i < l; i += 4){ if (d[i] > 245) ++no; if (d[i] < 10) ++nt; }
	
	if (no == 0 && nt == n) // fully transparent -> make fully opaque
	{
		for (size_t i = 3; i < l; i += 4) d[i] = 255;
		no = n; nt = 0;
	}
	if (circle && nt == 0 && no == n) // fully opaque
	{
		d += 3;
		int r = std::min(_w, _h);
		int x0 = (int)_w - r/2, y0 = (int)_h - r/2;
		for (int i = 0; i < (int)_h; ++i)
		for (int j = 0; j < (int)_w; ++j)
		{
			double rr = hypot((j - x0), (i - y0)) - r*0.4;
			if (rr >= 0.0) *d = std::min((unsigned char)std::max(0.0, 255.0*(1.0 - rr/0.1)), (unsigned char)255);
			d += 4;
		}
	}
}
