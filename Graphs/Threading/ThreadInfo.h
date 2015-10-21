#pragma once
#include "../Graphics/Info.h"
#include "../../Engine/Parser/BoundContext.h"

//----------------------------------------------------------------------------------------------------------------------
// ThreadInfo
//----------------------------------------------------------------------------------------------------------------------

struct ThreadInfo
{
	//------------------------------------------------------------------------------------------------------------------
	// standard thread init / finish
	//------------------------------------------------------------------------------------------------------------------
	
	static void thread_setup(const void *info_, void *&data)
	{
		std::vector<void *> &info = *(std::vector<void *>*)info_;
		assert(info.size() == 4);
		data = new ThreadInfo(*(const DI_Calc*)info[0], *(const DI_Axis*)info[1],
							  *(const DI_Subdivision*)info[2], *(const DI_Grid*)info[3]);
	}
	static void thread_finish(void *data)
	{
		delete (ThreadInfo*)data;
	}

	
	ThreadInfo(const DI_Calc &ic, const DI_Axis &ia, const DI_Subdivision &is, const DI_Grid &ig)
	: ic(ic), ia(ia), is(is), ig(ig), ec(*ic.e0){}
	
	BoundContext          ec;
	const DI_Calc        &ic;
	const DI_Axis        &ia;
	const DI_Subdivision &is;
	const DI_Grid        &ig;
	char  padding[128-sizeof(EvalContext)-4*8];

	//------------------------------------------------------------------------------------------------------------------
	// computation
	//------------------------------------------------------------------------------------------------------------------

	inline void extract_complex(double u, double v, P3f &p, bool &exists)
	{
		const cnum &z = ec.output(0);
		if ((exists = defined(z)))
		{
			P3d dp;
			switch (ic.projection)
			{
				case GM_Re:    dp.set(u, v, z.real()); ia.map(dp, p); break;
				case GM_Im:    dp.set(u, v, z.imag()); ia.map(dp, p); break;
				case GM_Abs:   dp.set(u, v, abs(z));   ia.map(dp, p); break;
				case GM_Phase: dp.set(u, v, arg(z));   ia.map(dp, p); break;
				case GM_Image: dp.set(z.real(), z.imag(), ia.elevation(u,v)); ia.map(dp, p); break;
				case GM_Riemann:
					riemann(z, p);
					p *= (float)(1.0 - 0.0008 * ( (u - ia.in_min[0]) / ia.in_range[0] + (v - ia.in_min[1]) / ia.in_range[1]));
					break;
				default: assert(false); break;
			}
		}
	}
	
	inline void extract_real(double u, P3f &p, bool &exists)
	{
		switch (ic.dim)
		{
			case 1: // (u,0,f(u)) or (u,f(u),0)
			{
				const cnum &xc = ec.output(0);
				if ((exists = is_real(xc)))
				{
					if (ic.embed_XZ)
					{
						P3d dp(u, 0.0, xc.real());
						ia.map(dp, p);
					}
					else
					{
						P3d dp(u, xc.real(), 0.0);
						ia.map(dp, p);
					}
				}
				break;
			}
				
			case 2: // (fx, 0, fy) or (fx, fy, 0)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				if ((exists = (is_real(xc) && is_real(yc))))
				{
					P3d dp;
					if (ic.embed_XZ)
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.z, dp.x);
							dp.x *= r;
							dp.z *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.z = yc.real();
						}
						dp.y = 0.0;
					}
					else
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.y, dp.x);
							dp.x *= r;
							dp.y *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.y = yc.real();
						}
						dp.z = 0.0;
					}
					ia.map(dp, p);
				}
				break;
			}
				
			case 3: // (fx, fy, fz)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				const cnum &zc = ec.output(2);
				if ((exists = (is_real(xc) && is_real(yc) && is_real(zc))))
				{
					P3d dp;
					if (ic.polar) // r, phi, z
					{
						double r = xc.real(), a = yc.real();
						sincos(a, dp.y, dp.x);
						dp.x *= r;
						dp.y *= r;
						dp.z  = zc.real();
					}
					else if (ic.spherical) // r, phi, theta
					{
						double r = xc.real(), phi = yc.real(), theta = zc.real(), st;
						sincos(theta, st, dp.z);
						sincos(phi, dp.y, dp.x);
						dp.z *= r;
						r    *= st;
						dp.x *= r;
						dp.y *= r;
					}
					else
					{
						dp.x = xc.real();
						dp.y = yc.real();
						dp.z = zc.real();
					}
					ia.map(dp, p);
				}
				break;
			}
				
			default:
				assert(false);
				break;
		}
	}

	inline void extract_real(double u, double v, P3f &p, bool &exists)
	{
		switch (ic.dim)
		{
			case 1: // (u,v,f(u,v)) - embed_XZ must be handled by caller!
			{
				const cnum &xc = ec.output(0);
				if ((exists = is_real(xc)))
				{
					P3d dp(u, v, xc.real());
					ia.map(dp, p);
				}
				break;
			}
				
			case 2: // (fx, 0, fy) or (fx, fy, 0)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				if ((exists = (is_real(xc) && is_real(yc))))
				{
					P3d dp;
					if (ic.embed_XZ)
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.z, dp.x);
							dp.x *= r;
							dp.z *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.z = yc.real();
						}
						dp.y = ia.elevation(u,v);
					}
					else
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.y, dp.x);
							dp.x *= r;
							dp.y *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.y = yc.real();
						}
						dp.z = ia.elevation(u,v);
					}
					ia.map(dp, p);
				}
				break;
			}
				
			case 3: // (fx, fy, fz)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				const cnum &zc = ec.output(2);
				if ((exists = (is_real(xc) && is_real(yc) && is_real(zc))))
				{
					P3d dp;
					if (ic.polar) // r, phi, z
					{
						double r = xc.real(), a = yc.real();
						sincos(a, dp.y, dp.x);
						dp.x *= r;
						dp.y *= r;
						dp.z  = zc.real();
					}
					else if (ic.spherical) // r, phi, theta
					{
						double r = xc.real(), phi = yc.real(), theta = zc.real(), st;
						sincos(theta, st, dp.z);
						sincos(phi, dp.y, dp.x);
						dp.z *= r;
						r    *= st;
						dp.x *= r;
						dp.y *= r;
					}
					else
					{
						dp.x = xc.real();
						dp.y = yc.real();
						dp.z = zc.real();
					}
					ia.map(dp, p);
				}
				break;
			}
				
			default:
				assert(false);
				break;
		}
	}
	
	inline void extract_vector(P3f &v, bool &exists)
	{
		switch (ic.dim)
		{
			case 2: // (fx, 0, fy) or (fx, fy, 0)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				if ((exists = (is_real(xc) && is_real(yc))))
				{
					P3d dp;
					if (ic.embed_XZ)
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.z, dp.x);
							dp.x *= r;
							dp.z *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.z = yc.real();
						}
						dp.y = 0.0;
					}
					else
					{
						if (ic.polar)
						{
							double r = xc.real(), a = yc.real();
							sincos(a, dp.y, dp.x);
							dp.x *= r;
							dp.y *= r;
						}
						else
						{
							dp.x = xc.real();
							dp.y = yc.real();
						}
						dp.z = 0.0;
					}
					ia.map_vector(dp, v);
				}
				break;
			}
				
			case 3: // (fx, fy, fz)
			{
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				const cnum &zc = ec.output(2);
				if ((exists = (is_real(xc) && is_real(yc) && is_real(zc))))
				{
					P3d dp;
					if (ic.polar) // r, phi, z
					{
						double r = xc.real(), a = yc.real();
						sincos(a, dp.y, dp.x);
						dp.x *= r;
						dp.y *= r;
						dp.z  = zc.real();
					}
					else if (ic.spherical) // r, phi, theta
					{
						double r = xc.real(), phi = yc.real(), theta = zc.real(), st;
						sincos(theta, st, dp.z);
						sincos(phi, dp.y, dp.x);
						dp.z *= r;
						r    *= st;
						dp.x *= r;
						dp.y *= r;
					}
					else
					{
						dp.x = xc.real();
						dp.y = yc.real();
						dp.z = zc.real();
					}
					ia.map_vector(dp, v);
				}
				break;
			}
				
			default:
				assert(false);
				break;
		}
	}
	
	inline void eval(double t, P3f &p, bool &exists)
	{
		assert(!ic.complex && !ic.vector_field && (ic.dim == 1 || !ic.embed_XZ));
		
		if (ic.xi >= 0) ec.set_input(ic.xi, t);
		
		ec.eval();
		extract_real(t, p, exists);
	}
	
	inline void eval(double u, double v, P3f &p, bool &exists, bool same_u = false, bool same_v = false)
	{
		assert(!ic.vector_field);
		
		if (ic.complex)
		{
			if (ic.xi >= 0) ec.set_input(ic.xi, cnum(u,v));
			ec.eval();
			extract_complex(u, v, p, exists);
		}
		else
		{
			if (!same_u && ic.xi >= 0) ec.set_input(ic.xi, u);
			if (!same_v && ic.yi >= 0) ec.set_input(ic.yi, v);
			#ifdef DEBUG
			if (same_u && ic.xi >= 0) assert(ec.input(ic.xi).real() == u);
			if (same_v && ic.yi >= 0) assert(ec.input(ic.yi).real() == v);
			#endif
			ec.eval();
			extract_real(u, v, p, exists);
		}
	}
	
	inline void eval_vector(double u, double v, P3f &p, bool &exists, bool same_u = false, bool same_v = false)
	{
		assert(ic.vector_field && !ic.complex);
		
		if (!same_u && ic.xi >= 0) ec.set_input(ic.xi, u);
		if (!same_v && ic.yi >= 0) ec.set_input(ic.yi, v);
			#ifdef DEBUG
		if (same_u && ic.xi >= 0) assert(ec.input(ic.xi).real() == u);
		if (same_v && ic.yi >= 0) assert(ec.input(ic.yi).real() == v);
			#endif
		ec.eval();
		extract_vector(p, exists);
	}
	
	inline void eval_vector(double u, double v, double w, P3f &p, bool &exists, bool same_u = false, bool same_v = false, bool same_w = false)
	{
		assert(ic.dim == 3 && !ic.complex); // can only be vectorfield at the moment
		
		if (!same_u && ic.xi >= 0) ec.set_input(ic.xi, u);
		if (!same_v && ic.yi >= 0) ec.set_input(ic.yi, v);
		if (!same_w && ic.zi >= 0) ec.set_input(ic.zi, w);
		#ifdef DEBUG
		if (same_u && ic.xi >= 0) assert(ec.input(ic.xi).real() == u);
		if (same_v && ic.yi >= 0) assert(ec.input(ic.yi).real() == v);
		if (same_w && ic.zi >= 0) assert(ec.input(ic.zi).real() == w);
		#endif
		ec.eval();
		extract_vector(p, exists);
	}
	
	inline double eval(double u, double v, bool same_u = false, bool same_v = false)
	{
		assert(ic.dim == 1 && !ic.complex);
		
		if (!same_u && ic.xi >= 0) ec.set_input(ic.xi, u);
		if (!same_v && ic.yi >= 0) ec.set_input(ic.yi, v);
		#ifdef DEBUG
		if (same_u && ic.xi >= 0) assert(ec.input(ic.xi).real() == u);
		if (same_v && ic.yi >= 0) assert(ec.input(ic.yi).real() == v);
		#endif
		ec.eval();
		
		const cnum &xc = ec.output(0);
		return is_real(xc) ? xc.real() : UNDEFINED;
	}

	inline double eval(double u, double v, double w, bool same_u = false, bool same_v = false, bool same_w = false)
	{
		assert(ic.dim == 1 && !ic.complex);
		
		if (!same_u && ic.xi >= 0) ec.set_input(ic.xi, u);
		if (!same_v && ic.yi >= 0) ec.set_input(ic.yi, v);
		if (!same_w && ic.zi >= 0) ec.set_input(ic.zi, w);
		#ifdef DEBUG
		if (same_u && ic.xi >= 0) assert(ec.input(ic.xi).real() == u);
		if (same_v && ic.yi >= 0) assert(ec.input(ic.yi).real() == v);
		if (same_w && ic.zi >= 0) assert(ec.input(ic.zi).real() == w);
		#endif
		ec.eval();
		
		const cnum &xc = ec.output(0);
		return is_real(xc) ? xc.real() : UNDEFINED;
	}

};

