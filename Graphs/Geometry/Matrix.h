#pragma once

#include "Vector.h"
#include "AxisIndex.h"

#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// 3 x 3 Matrix
//----------------------------------------------------------------------------------------------------------------------

template<typename T> struct M3
{
	T a11, a12, a13;
	T a21, a22, a23;
	T a31, a32, a33;
	
	M3(){ }
	M3(T x) : a11(x), a12(0), a13(0), a21(0), a22(x), a23(0), a31(0), a32(0), a33(x){ }

	inline M3 & operator = (const M3 &m){ memmove(this, &m, sizeof(M3)); return *this; }
	inline void clear(){ memset(this, 0, sizeof(M3)); }

	inline void operator -= (const M3 &x)
	{
		a11 -= x.a11;  a12 -= x.a12;  a13 -= x.a13;
		a21 -= x.a21;  a22 -= x.a22;  a23 -= x.a23;
		a31 -= x.a31;  a32 -= x.a32;  a33 -= x.a33;
	}
	inline void operator += (const M3 &x)
	{
		a11 += x.a11;  a12 += x.a12;  a13 += x.a13;
		a21 += x.a21;  a22 += x.a22;  a23 += x.a23;
		a31 += x.a31;  a32 += x.a32;  a33 += x.a33;
	}
	inline M3 operator - (const M3 &x){ M3 m(*this); m -= x; return m; }
	inline M3 operator + (const M3 &x){ M3 m(*this); m += x; return m; }

	inline M3 operator * (const M3 &x) const
	{
		M3 y;
		
		y.a11 = a11*x.a11 + a12*x.a21 + a13*x.a31;
		y.a12 = a11*x.a12 + a12*x.a22 + a13*x.a32;
		y.a13 = a11*x.a13 + a12*x.a23 + a13*x.a33;

		y.a21 = a21*x.a11 + a22*x.a21 + a23*x.a31;
		y.a22 = a21*x.a12 + a22*x.a22 + a23*x.a32;
		y.a23 = a21*x.a13 + a22*x.a23 + a23*x.a33;

		y.a31 = a31*x.a11 + a32*x.a21 + a33*x.a31;
		y.a32 = a31*x.a12 + a32*x.a22 + a33*x.a32;
		y.a33 = a31*x.a13 + a32*x.a23 + a33*x.a33;

		return y;
	}
	inline M3 &operator *= (const M3 &x)
	{
		return *this = *this * x;
	}
	inline P3<T> operator * (const P3<T> &v) const
	{
		P3<T> w;
		w.x = a11*v.x + a12*v.y + a13*v.z;
		w.y = a21*v.x + a22*v.y + a23*v.z;
		w.z = a31*v.x + a32*v.y + a33*v.z;
		return w;
	}
	inline M3 & transpose()
	{
		std::swap(a12, a21);		     
		std::swap(a13, a31);		     
		std::swap(a23, a32);
		return *this;
	}
	inline void operator *= (T a)
	{
		a11 *= a;  a12 *= a;  a13 *= a;
		a21 *= a;  a22 *= a;  a23 *= a;
		a31 *= a;  a32 *= a;  a33 *= a;
	}
	inline void operator /= (T a)
	{
		(*this) *= 1/a;
	}
	
	inline T det() const
	{
		return a11*a22*a33 + a12*a23*a31 + a13*a21*a32 - a31*a22*a13 - a32*a23*a11 - a33*a21*a12;
	}

	inline M3 inverse() const
	{
		T d = det(); assert(d != 0.0); // if d==0 let it become NAN but don't throw exceptions here
		d = 1 / d;
		M3 m;
		m.a11 = d * (a22*a33 - a32*a23);  m.a12 = d * (a13*a32 - a33*a12);  m.a13 = d * (a12*a23 - a22*a13);
		m.a21 = d * (a23*a31 - a33*a21);  m.a22 = d * (a11*a33 - a13*a31);  m.a23 = d * (a13*a21 - a23*a11);
		m.a31 = d * (a21*a32 - a31*a22);  m.a32 = d * (a12*a31 - a32*a11);  m.a33 = d * (a11*a22 - a21*a12);
		return m;
	}
	
	static M3 Rotation(T angle, AxisIndex axis)
	{
		M3 m(0);
		T ca = cos(angle), sa = sin(angle);
		switch (axis)
		{
			case X_AXIS:
				m.a22 = m.a33 = ca;
				m.a23 = -sa;
				m.a32 =  sa;
				m.a11 = 1;
				break;

			case Y_AXIS:
				m.a11 = m.a33 = ca;
				m.a13 =  sa;
				m.a31 = -sa;
				m.a22 = 1;
				break;

			case Z_AXIS:
				m.a11 = m.a22 = ca;
				m.a12 = -sa;
				m.a21 =  sa;
				m.a33 = 1;
				break;

			default: assert(false); break;	
		}
#ifdef DEBUG
		T d = m.det();
		assert(fabs(d - 1.0) < 1e-12);
#endif
		return m;
	}
	
	template<typename S> explicit operator M3<S> () const
	{
		M3<S> m;
		m.a11 = (S)a11; m.a12 = (S)a12; m.a13 = (S)a13;
		m.a21 = (S)a21; m.a22 = (S)a22; m.a23 = (S)a23;
		m.a31 = (S)a31; m.a32 = (S)a32; m.a33 = (S)a33;
		return m;
	}
	
	
#ifdef DEBUG
	bool operator < (double eps) const
	{
		return fabs(a11)<eps && fabs(a12)<eps && fabs(a12)<eps && 
		       fabs(a21)<eps && fabs(a22)<eps && fabs(a22)<eps && 
		       fabs(a31)<eps && fabs(a32)<eps && fabs(a32)<eps;
	}
#endif
};
typedef M3<float>  M3f;
typedef M3<double> M3d;

template<typename T> inline P3<T> operator * (const P3<T> &v, const M3<T> &m) // (v_transposed * m)_transposed
{
	P3<T> w;
	w.x = m.a11*v.x + m.a21*v.y + m.a31*v.z;
	w.y = m.a12*v.x + m.a22*v.y + m.a32*v.z;
	w.z = m.a13*v.x + m.a23*v.y + m.a33*v.z;
	return w;
}


//----------------------------------------------------------------------------------------------------------------------
// 2 x 2 Matrix
//----------------------------------------------------------------------------------------------------------------------

template<typename T> struct M2
{
	T a11, a12;
	T a21, a22;
	
	M2(){ }
	M2(T x) : a11(x), a12(0), a21(0), a22(x){ }
	
	inline M2 & operator = (const M2 &m){ memmove(this, &m, sizeof(M2)); return *this; }
	inline void clear(){ memset(this, 0, sizeof(M2)); }
	
	inline void operator -= (const M2 &x)
	{
		a11 -= x.a11;  a12 -= x.a12;
		a21 -= x.a21;  a22 -= x.a22;
	}
	inline void operator += (const M2 &x)
	{
		a11 += x.a11;  a12 += x.a12;
		a21 += x.a21;  a22 += x.a22;
	}
	inline M2 operator - (const M2 &x){ M2 m(*this); m -= x; return m; }
	inline M2 operator + (const M2 &x){ M2 m(*this); m += x; return m; }
	
	inline M2 operator * (const M2 &x) const
	{
		M2 y;
		y.a11 = a11*x.a11 + a12*x.a21;
		y.a12 = a11*x.a12 + a12*x.a22;
		y.a21 = a21*x.a11 + a22*x.a21;
		y.a22 = a21*x.a12 + a22*x.a22;
		return y;
	}
	inline M2 &operator *= (const M2 &x)
	{
		return *this = *this * x;
	}
	inline P2<T> operator * (const P2<T> &v) const
	{
		P2<T> w;
		w.x = a11*v.x + a12*v.y;
		w.y = a21*v.x + a22*v.y;
		return w;
	}
	inline M2 & transpose()
	{
		std::swap(a12, a21);
		return *this;
	}
	inline void operator *= (T a)
	{
		a11 *= a;  a12 *= a;
		a21 *= a;  a22 *= a;
	}
	inline void operator /= (T a)
	{
		(*this) *= 1/a;
	}
	
	inline T det() const
	{
		return a11*a22 - a12*a21;
	}
	
	template<typename S> explicit operator M2<S> () const
	{
		M2<S> m;
		m.a11 = (S)a11; m.a12 = (S)a12;
		m.a21 = (S)a21; m.a22 = (S)a22;
		return m;
	}
	
};
typedef M2<float>  M2f;
typedef M2<double> M2d;
	
template<typename T> inline P2<T> operator * (const P2<T> &v, const M2<T> &m) // (v_transposed * m)_transposed
{
	P2<T> w;
	w.x = m.a11*v.x + m.a21*v.y;
	w.y = m.a12*v.x + m.a22*v.y;
	return w;
}

//----------------------------------------------------------------------------------------------------------------------
// 4 x 4 Matrix
//----------------------------------------------------------------------------------------------------------------------

template<typename T> struct M4
{
	T a11, a12, a13, a14;
	T a21, a22, a23, a24;
	T a31, a32, a33, a34;
	T a41, a42, a43, a44;
	
	M4(){ }
	M4(T x)
	{
		assert((void*)this == (void*)&a11);
		memset(this, 0, 16*sizeof(double));
		a11 = a22 = a33 = a44 = x;
	}
	inline M4 & operator = (T x)
	{
		memset(this, 0, 16*sizeof(double));
		a11 = a22 = a33 = a44 = x;
		return *this;
	}
	
	inline M4 & operator = (const M4 &m){ memmove(this, &m, sizeof(M4)); return *this; }
	inline void clear(){ memset(this, 0, sizeof(M4)); }
	
	inline void operator -= (const M4 &x)
	{
		a11 -= x.a11;  a12 -= x.a12;  a13 -= x.a13;  a14 -= x.a14;
		a21 -= x.a21;  a22 -= x.a22;  a23 -= x.a23;  a24 -= x.a24;
		a31 -= x.a31;  a32 -= x.a32;  a33 -= x.a33;  a34 -= x.a34;
		a41 -= x.a41;  a42 -= x.a42;  a43 -= x.a43;  a44 -= x.a44;
	}
	inline void operator += (const M4 &x)
	{
		a11 += x.a11;  a12 += x.a12;  a13 += x.a13;  a14 += x.a14;
		a21 += x.a21;  a22 += x.a22;  a23 += x.a23;  a24 += x.a24;
		a31 += x.a31;  a32 += x.a32;  a33 += x.a33;  a34 += x.a34;
		a41 += x.a41;  a42 += x.a42;  a43 += x.a43;  a44 += x.a44;
	}
	inline M4 operator - (const M4 &x){ M4 m(*this); m -= x; return m; }
	inline M4 operator + (const M4 &x){ M4 m(*this); m += x; return m; }
	
	inline M4 operator * (const M4 &x) const
	{
		M4 y;
		
		y.a11 = a11*x.a11 + a12*x.a21 + a13*x.a31 + a14*x.a41;
		y.a12 = a11*x.a12 + a12*x.a22 + a13*x.a32 + a14*x.a42;
		y.a13 = a11*x.a13 + a12*x.a23 + a13*x.a33 + a14*x.a43;
		y.a14 = a11*x.a14 + a12*x.a24 + a13*x.a34 + a14*x.a44;
		
		y.a21 = a21*x.a11 + a22*x.a21 + a23*x.a31 + a24*x.a41;
		y.a22 = a21*x.a12 + a22*x.a22 + a23*x.a32 + a24*x.a42;
		y.a23 = a21*x.a13 + a22*x.a23 + a23*x.a33 + a24*x.a43;
		y.a24 = a21*x.a14 + a22*x.a24 + a23*x.a34 + a24*x.a44;
		
		y.a31 = a31*x.a11 + a32*x.a21 + a33*x.a31 + a34*x.a41;
		y.a32 = a31*x.a12 + a32*x.a22 + a33*x.a32 + a34*x.a42;
		y.a33 = a31*x.a13 + a32*x.a23 + a33*x.a33 + a34*x.a43;
		y.a34 = a31*x.a14 + a32*x.a24 + a33*x.a34 + a34*x.a44;
		
		y.a41 = a41*x.a11 + a42*x.a21 + a43*x.a31 + a44*x.a41;
		y.a42 = a41*x.a12 + a42*x.a22 + a43*x.a32 + a44*x.a42;
		y.a43 = a41*x.a13 + a42*x.a23 + a43*x.a33 + a44*x.a43;
		y.a44 = a41*x.a14 + a42*x.a24 + a43*x.a34 + a44*x.a44;
		
		return y;
	}
	inline M4 &operator *= (const M4 &x)
	{
		return *this = *this * x;
	}
	inline P4<T> operator * (const P4<T> &v) const
	{
		P4<T> w;
		w.x = a11*v.x + a12*v.y + a13*v.z + a14*v.w;
		w.y = a21*v.x + a22*v.y + a23*v.z + a24*v.w;
		w.z = a31*v.x + a32*v.y + a33*v.z + a34*v.w;
		w.w = a41*v.x + a42*v.y + a43*v.z + a44*v.w;
		return w;
	}
	inline M4 & transpose()
	{
		std::swap(a12, a21);
		std::swap(a13, a31);
		std::swap(a14, a41);
		std::swap(a23, a32);
		std::swap(a24, a42);
		std::swap(a34, a43);
		return *this;
	}
	inline void operator *= (T a)
	{
		a11 *= a;  a12 *= a;  a13 *= a;  a14 *= a;
		a21 *= a;  a22 *= a;  a23 *= a;  a24 *= a;
		a31 *= a;  a32 *= a;  a33 *= a;  a34 *= a;
		a41 *= a;  a42 *= a;  a43 *= a;  a44 *= a;
	}
	inline void operator /= (T a)
	{
		(*this) *= 1/a;
	}
};
typedef M4<float>  M4f;
typedef M4<double> M4d;

template<typename T> inline P3<T> operator * (const P4<T> &v, const M4<T> &m) // (v_transposed * m)_transposed
{
	P4<T> w;
	w.x = m.a11*v.x + m.a21*v.y + m.a31*v.z + m.a41*v.w;
	w.y = m.a12*v.x + m.a22*v.y + m.a32*v.z + m.a42*v.w;
	w.z = m.a13*v.x + m.a23*v.y + m.a33*v.z + m.a43*v.w;
	w.w = m.a14*v.x + m.a24*v.y + m.a34*v.z + m.a44*v.w;
	return w;
}

