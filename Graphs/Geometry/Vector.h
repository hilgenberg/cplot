#pragma once
#include <ostream>
#include <cstring>
#include "../../Engine/cnum.h"
#include "../../Utility/MemoryPool.h"

/**
 * 3D vectors
 */

template<typename T> struct P3
{
	POOL_ITEM_BASE(P3)
	
	T x, y, z;
	P3(){ }
	P3(T x_, T y_, T z_) : x(x_), y(y_), z(z_){ }
	inline constexpr P3(const P3 &) = default;
	inline void operator = (const P3 &b){ memmove(this, &b, sizeof(P3)); }
	template<typename S> explicit operator P3<S> () const{ return P3<S>((S)x, (S)y, (S)z); }
	inline void clear(){ memset(this, 0, sizeof(P3)); }
	inline void set(T x_, T y_, T z_){ x = x_; y = y_; z = z_; }
	inline operator       T* ()      { return (T*)this; }
	inline operator const T* () const{ return (T*)this; }

	inline void  operator -= (const P3 &a){ x -= a.x; y -= a.y; z -= a.z; }
	inline void  operator += (const P3 &a){ x += a.x; y += a.y; z += a.z; }
	inline T     operator *  (const P3 &a) const { return x*a.x + y*a.y + z*a.z; }
	inline P3    operator +  (const P3 &a) const { return P3(x+a.x, y+a.y, z+a.z); }
	inline P3    operator -  (const P3 &a) const { return P3(x-a.x, y-a.y, z-a.z); }
	inline P3    operator -  ()            const { return P3(-x, -y, -z); }
	inline P3    operator *  (T a) const { return P3(a*x, a*y, a*z); }
	inline P3    operator /  (T a) const { return P3(x/a, y/a, z/a); }

	inline void operator *= (T a){ x *= a; y *= a; z *= a; }
	inline void operator /= (T a){ x /= a; y /= a; z /= a; }

	inline T absq() const{ return x*x + y*y + z*z; }
	inline T abs () const{ return (T)sqrt(absq()); }
	inline P3 &to_unit(){ *this /= abs(); return *this; }
};
template<typename T> inline P3<T> operator*(T a, const P3<T> &v){ return P3<T>(a*v.x, a*v.y, a*v.z); }
template<typename T> std::ostream &operator<<(std::ostream &o, const P3<T> &v)
{
	return o << '(' << v.x << ", " << v.y << ", " << v.z << ")";
}

typedef P3<float>  P3f;
typedef P3<double> P3d;

/**
 * Project onto the Riemann sphere
 * @param p Point to project
 * @param q Storage for the result
 */
template <typename T>
inline void riemann(const cnum &p, P3<T> &q)
{
	double l = 2.0 / (absq(p) + 1.0);
	q.x = (T)(l * p.real());
	q.y = (T)(l * p.imag());
	q.z = (T)(l - 1.0);
}

/**
 * Project from Riemann sphere back to C
 * @param p Point to project
 * @param q Storage for the result
 */
template <typename T>
inline void riemann(const P3<T> &p, cnum &q)
{
	double l = p.z + 1.0;
	q.real(p.x / l);
	q.imag(p.y / l);
	// => |q|^2 = (1-p.z) / (1+p.z);
}

/**
 * 2D vectors
 */

template<typename T> struct P2;
template<typename T> inline T vabs(const P2<T> &v){ return (T)sqrt(v.absq()); }

template<typename T> struct P2
{
	T x, y;
	P2(){ }
	P2(T x_, T y_) : x(x_), y(y_){ }
	inline constexpr P2(const P2 &) = default;
	inline void operator = (const P2 &b){ memmove(this, &b, sizeof(P2)); }
	template<typename S> explicit operator P2<S> () const{ return P2<S>((S)x, (S)y); }
	inline void clear(){ memset(this, 0, sizeof(P2)); }
	inline void set(T x_, T y_){ x = x_; y = y_; }
	inline operator       T* ()      { return (T*)this; }
	inline operator const T* () const{ return (T*)this; }

	inline void operator -= (const P2 &a){ x -= a.x; y -= a.y; }
	inline void operator += (const P2 &a){ x += a.x; y += a.y; }
	inline T    operator *  (const P2 &a) const { return x*a.x + y*a.y; }
	inline P2   operator +  (const P2 &a) const { return P2(x+a.x, y+a.y); }
	inline P2   operator -  (const P2 &a) const { return P2(x-a.x, y-a.y); }
	inline P2   operator *  (T a)         const { return P2(a*x, a*y); }
	inline P2   operator /  (T a)         const { return P2(x/a, y/a); }

	inline void operator *= (T a){ x *= a; y *= a; }
	inline void operator /= (T a){ x /= a; y /= a; }
	
	inline T absq() const{ return x*x + y*y; }
	inline T abs () const{ return vabs(*this); }
	inline void to_unit(){ *this *= (T)1 / abs(); }
	inline P3<T> riemann() // projects onto Riemann sphere
	{
		float l = 2.0f / (x*x + y*y + 1.0f);
		return P3T(l * x, l * y, l - 1.0f);
		
		// _________p__ +1/2  that's supposed to be a sphere with diameter 1 around (0,0,0) and a line
		// _____()/____ -1/2  from its south pole at (0,0,-1/2) through some sphere point up to (x,y,1/2)
		//                    so after projecting, we scale the sphere by 2 (this is done so that points
		//                    with |p|=1 are projected onto the equator)
		
		// |q|² = l² (x² + y² + 1) - 2l + 1 = 4 / (x² + y² + 1) - 4 / (x² + y² + 1) + 1 = 1
		
		//    q/2    - (0,0,-1/2) = (l/2 x, l/2 y, l/2) = l/2 (x,y,1)
		// (x,y,1/2) - (0,0,-1/2) = (x,y,1)
		
		// (0,0) -> (0,0,+1)
		//   ∞   -> (0,0,-1)
	}
};
typedef P2<float>  P2f;
typedef P2<double> P2d;

template<> inline double vabs(const P2d &v){ return hypot (v.x, v.y); }
template<> inline float  vabs(const P2f &v){ return hypotf(v.x, v.y); }

/**
 * 4D vectors
 */

template<typename T> struct P4
{
	T x, y, z, w;
	P4(){ }
	P4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_){ }
	explicit P4(T v) : x(v), y(v), z(v), w(v){ }
	inline constexpr P4(const P4 &) = default;
	inline void operator = (const P4 &b){ memmove(this, &b, sizeof(P4)); }
	inline void clear(){ memset(this, 0, sizeof(P4)); }
	inline void set(T x_, T y_, T z_, T w_){ x = x_; y = y_; z = z_; w = w_; }
	inline void set(const P3<T> &v, T w_){ x = v.x; y = v.y; z = v.z; w = w_; }
	inline operator       T* ()      { return (T*)this; }
	inline operator const T* () const{ return (T*)this; }

	inline void operator -= (const P4 &a){ x -= a.x; y -= a.y; z -= a.z; w -= a.w; }
	inline void operator += (const P4 &a){ x += a.x; y += a.y; z += a.z; w += a.w; }
	inline T    operator *  (const P4 &a) const { return x*a.x + y*a.y + z*a.z + w*a.w; }
	inline P4   operator +  (const P4 &a) const { return P4(x+a.x, y+a.y, z+a.z, w+a.w); }
	inline P4   operator -  (const P4 &a) const { return P4(x-a.x, y-a.y, z-a.z, w-a.w); }
	inline P4   operator *  (T a)         const { return P4(a*x, a*y, a*z, a*w); }
	inline P4   operator /  (T a)         const { return P4(x/a, y/a, z/a, w/a); }

	inline void operator *= (T a){ x *= a; y *= a; z *= a; w *= a; }
	inline void operator /= (T a){ x /= a; y /= a; z /= a; w /= a; }
	
	inline T absq() const{ return x*x + y*y + z*z + w*w; }
	inline T abs () const{ return sqrt(absq()); }
	inline void to_unit(){ *this *= (T)1 / abs(); }
};
typedef P4<float>  P4f;
typedef P4<double> P4d;

//----------------------------------------------------------------------------------------------------------------------
// Operators and pseudo-operators (f.e. sub(x,a,b): x = a-b
//----------------------------------------------------------------------------------------------------------------------

template<typename T> inline void sub(P2<T> &x, const P2<T> &a, const P2<T> &b)
{
	x.x = a.x - b.x;
	x.y = a.y - b.y;
}

template<typename T> inline void sub(P3<T> &x, const P3<T> &a, const P3<T> &b)
{
	x.x = a.x - b.x;
	x.y = a.y - b.y;
	x.z = a.z - b.z;
}

template<typename T> inline void add(P3<T> &x, const P3<T> &a, const P3<T> &b)
{
	x.x = a.x + b.x;
	x.y = a.y + b.y;
	x.z = a.z + b.z;
}
template<typename T> inline void add(P2<T> &x, const P2<T> &a, const P2<T> &b)
{
	x.x = a.x + b.x;
	x.y = a.y + b.y;
}

template<typename T> inline void cross(P3<T> &x, const P3<T> &a, const P3<T> &b)
{
	/// @todo Find some really fast assembler way to do this
	x.x = a.y*b.z - a.z*b.y;
	x.y = a.z*b.x - a.x*b.z;
	x.z = a.x*b.y - a.y*b.x;
}

template<typename T> inline T area(const P2<T> &a, const P2<T> &b, const P2<T> &c)
{
	P2<T> d1, d2;
	sub(d1, b, a);
	sub(d2, c, a);
	return fabs(d1.x*d2.y - d1.y*d2.x); // cross.z
}

template<typename T> inline T distq(const P3<T> &a, const P3<T> &b)
{
	P3<T> t;
	sub(t, a, b);
	return t.absq();
}
template<typename T> inline T distq(const P2<T> &a, const P2<T> &b)
{
	P2<T> t;
	sub(t, a, b);
	return t.absq();
}

