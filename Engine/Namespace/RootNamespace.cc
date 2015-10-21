#include "RootNamespace.h"
#include "../Functions/Functions.h"
#include "BaseFunction.h"
#include "Function.h"
#include "Operator.h"
#include "Constant.h"
#include "../Parser/Simplifier/Pattern.h"
#include "../../Utility/System.h"

#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>
#include <cassert>

#define ADDF(name, ctor...) add_builtin(e = new BaseFunction(name, ctor))
#define ADDC(name, ctor...) add_builtin(e = new Constant(name, cnum(ctor)))
#define ADDUO(name, postf, ctor...) add_builtin(e = new UnaryOperator(name, !postf, ctor))
#define ADDREL(name, prec, ctor...) add_builtin(e = new BinaryOperator(name, prec, true, false, false, false, ctor))
#define ADDBO(name, prec, assoc, commut, right, ctor...) add_builtin(e = new BinaryOperator(name, prec, false, assoc, commut, right, ctor))
#define DNAME(n) e->displayName(PS_Console, n)
#define MNAME(n) e->displayName(PS_Mathematica, n)
#define LNAME(n) e->displayName(PS_LaTeX, n)
#define HNAME(n) e->displayName(PS_HTML, n)
#define HDNAME(n) e->displayName(PS_Head, n)
#define STORE(x) (Element*&)x = e
#define store(x) Element *x = e
#define DIFF(cpx, s...) ((Function*)e)->gradient(std::vector<std::string>({s}), cpx)
#define POWER(p) ((Function*)e)->set_power(p)
#define EXP(b)   ((Function*)e)->set_exp(b)
#define INVO ((Function*)e)->involution(true)
#define PROJ ((Function*)e)->projection(true)
#define CLPS ((Function*)e)->collapses(true)
#define EF ((BaseFunction*)(e))
#define EVEN even(EF)
#define ODD  odd(EF)
#define INV(g) inverse((BaseFunction*)g, EF, false) /* g(e)=id */
#define COMMUTES(g) commutes(EF, g)
#define ADD ((Function*)e)->additive(true)
#define MUL ((Function*)e)->multiplicative(true)
#define LIN ((Function*)e)->linear(true)
#define RR EF->range(R_Real, R_Real)
#define UU EF->range(R_Unit, R_Unit)
#define II EF->range(R_Real|R_Unit, R_Real|R_Unit)

RootNamespace::RootNamespace()
{
	Element *e = NULL;

	//------------------------------------------------------------------------------------------------------------------
	// Constants
	//------------------------------------------------------------------------------------------------------------------

	ADDC("eulergamma", 0.57721566490153286060651209008240243104215933593992);
	ADDC("pi",  M_PI); DNAME("π"); MNAME("PI"); LNAME("\\pi");
	ADDC("π",   M_PI); MNAME("PI"); LNAME("\\pi");
	ADDC("e",   M_E);  MNAME("E");
	ADDC("i",   0.0, 1.0); MNAME("I"); STORE(I);
	ADDC("°",   M_PI/180.0); HDNAME("2π/360"); MNAME("2PI/360");
	ADDC("NAN", UNDEFINED);
	
	//------------------------------------------------------------------------------------------------------------------
	// Operators
	//------------------------------------------------------------------------------------------------------------------

	// Prefix operators
	ADDUO("~",    false, conj, identity);     STORE(ConjOp);              DIFF(false, "1", "-i");  INVO; LIN; MUL;
	ADDUO("(U+)", false, identity, identity); STORE(UPlus);  DNAME("+");  DIFF(true, "1");  PROJ;  INVO; LIN;
	ADDUO("(U-)", false, negate, negate);     STORE(UMinus); DNAME("-");  DIFF(true, "-1");        INVO; LIN;
	ADDUO("(⁻¹)", true,  invert, invert);     STORE(Invert); DNAME("invert"); DIFF(true, "-1/z²"); INVO; MUL;
	
	// Postfix
	PowOps[0] = NULL;
	ADDUO("¹", true, identity, identity); DIFF(true, "1");    POWER(1); STORE(PowOps[1]); INVO; PROJ; LIN; MUL;
	ADDUO("²", true, sqr, sqr);           DIFF(true, "2*z");  POWER(2); STORE(PowOps[2]); MUL;
	ADDUO("³", true, cube, cube);         DIFF(true, "3*z²"); POWER(3); STORE(PowOps[3]); MUL;
	ADDUO("⁴", true, pow4, pow4);         DIFF(true, "4*z³"); POWER(4); STORE(PowOps[4]); MUL;
	ADDUO("⁵", true, pow5, pow5);         DIFF(true, "5*z⁴"); POWER(5); STORE(PowOps[5]); MUL;
	ADDUO("⁶", true, pow6, pow6);         DIFF(true, "6*z⁵"); POWER(6); STORE(PowOps[6]); MUL;
	ADDUO("⁷", true, pow7, pow7);         DIFF(true, "7*z⁶"); POWER(7); STORE(PowOps[7]); MUL;
	ADDUO("⁸", true, pow8, pow8);         DIFF(true, "8*z⁷"); POWER(8); STORE(PowOps[8]); MUL;
	ADDUO("⁹", true, pow9, pow9);         DIFF(true, "9*z⁸"); POWER(9); STORE(PowOps[9]); MUL;

	ADDUO("!", true, fakt, fakt); DIFF(true, "digamma(z1+1)*(z1!)");
	
	// Infix
	ADDBO("**",  5,  false,  false,  true,  cpow, NULL, NULL, rcpow); STORE(PowStar); DIFF(true, "z2*z1^(z2-1)", "ln(z1) z1^z2");
	ADDBO("^",   5,  false,  false,  true,  cpow, NULL, NULL, rcpow); STORE(Pow); DIFF(true, "z2*z1^(z2-1)", "ln(z1) z1^z2");

	ADDBO("/",   4,  false,  false,  false, cdiv, div);     STORE(Div); DIFF(true, "1/z2", "-z1/z2²");
	ADDBO("%",   4,  false,  false,  false, cmod, mod, crmod); DIFF(false, "1", "NAN", "-floor(x1/x2)", "NAN");
	
	ADDBO("*",   4,  true,   true,   false, mul,  mul);     STORE(Mul); DIFF(true, "z2", "z1");
	
	ADDBO("-",   3,  false,  false,  false, sub,  sub);     STORE(Minus); DIFF(true, "1", "-1");
	ADDBO("+",   3,  true,   true,   false, ::add,  ::add); STORE(Plus);  DIFF(true, "1", "1");
	
	ADDBO("xor", 2,  true,   true,   false, I_XOR, I_XOR); // nowhere differentiable (cantor dust)
	ADDBO("and", 2,  true,   true,   false, I_AND, I_AND);
	ADDBO("or",  2,  true,   true,   false, I_OR,  I_OR);
	
	ADDREL(">",  1, is_gt, is_gt, is_gt); DIFF(false, "0","0","0","0");
	ADDREL("<",  1, is_sm, is_sm, is_sm); DIFF(false, "0","0","0","0");
	/*ADDREL("&&",  4, log_and);
	 ADDREL("&",   4, log_and);
	 ADDREL("and", 4, log_and);
	 ADDREL("||",  3, log_or );
	 ADDREL("|",   3, log_or );
	 ADDREL("or",  3, log_or );
	 
	 ADDREL(">=",  2, is_gte);
	 ADDREL("≥",   2, is_gte);
	 ADDREL("<=",  2, is_sme);
	 ADDREL("≤",   2, is_sme);
	 ADDREL("==",  2, is_eq);
	 ADDREL("!=",  2, is_neq);
	 ADDREL("≠",   2, is_neq);*/

	//------------------------------------------------------------------------------------------------------------------
	// Functions - Basic
	//------------------------------------------------------------------------------------------------------------------

	ADDF("(id)", identity, identity); DNAME("id"); STORE(Identity); DIFF(true,"1"); POWER(1); INVO; PROJ; MUL; LIN;
	ADDF("complex", construct, NULL, NULL, construct); DIFF(true, "1", "i"); STORE(Combine);
	
	ADDF("conj", conj,   identity); DIFF(false, "1", "-i"); STORE(Conj); INVO; ODD; LIN; MUL;
	ADDF("re",   c_re,   identity, re  ); DIFF(false, "1", "0"); STORE(Re); PROJ; ODD; LIN;
	ADDF("im",   c_im,   zero,     im  ); DIFF(false, "0", "1"); STORE(Im); CLPS; ODD; LIN;
	ADDF("arg",  c_arg,  zero,     arg ); DIFF(false, "-y/absq(z)", "x/absq(z)"); store(arg); CLPS;
	combines(Re, Im,   Second); combines(Im, Re,   Zero);     combines(Re, Conj, First);
	combines(Conj, Re,  Second); combines(Conj, Im,  Second); combines(Conj, arg, Second);
	combines(Re, arg, Second);  combines(Im, arg, Zero);
	combines(arg, Re, Zero);    combines(arg, Im, Zero);
	
	#define REAL combines(Re, EF, Second); combines(Conj, EF, Second); combines(Im, EF, Zero); combines(arg, EF, Zero)
	
	ADDF("abs",  c_abs,  abs,      abs ); DIFF(false, "x/abs(z)", "y/abs(z)"); STORE(Abs); PROJ; EVEN; REAL; MUL;
	ADDF("absq", c_absq, sqr,      absq); DIFF(false, "2x", "2y"); store(absq); EVEN; REAL; MUL;
	combines(Abs, absq, Second);
	combines(absq, Abs, First);
	combines(Abs,  Conj, First);
	combines(absq, Conj, First);
	
	ADDF("sgn",  c_sgn,  sgn); DIFF(false, "y/(swap(z)*abs(z))", "-x/(swap(z)*abs(z))"); PROJ; ODD; MUL;
	
	ADDF("arg",  arg2,   arg2); REAL; DIFF(false, "-(y1+x2)/((x1-y2)²+(y1+x2)²)", "(x1-y2)/((x1-y2)²+(y1+x2)²)",
	                                               "(x1-y2)/((x1-y2)²+(y1+x2)²)", "(y1+x2)/((x1-y2)²+(y1+x2)²)");
	combines(arg, Re, Zero);
	combines(arg, Im, Zero);

	ADDF("round", round_, round); DIFF(false, "0", "0"); PROJ; ODD; COMMUTES(Conj);
	ADDF("floor", floor,  floor); DIFF(false, "0", "0"); PROJ;
	ADDF("ceil",  ceil,   ceil);  DIFF(false, "0", "0"); PROJ;

	ADDF("sqr", sqr, sqr); DIFF(true, "2*z"); POWER(2); MUL;
	
	ADDF("sqrt", sqrt_, NULL, NULL, sqrt_); DNAME("√"); DIFF(true, "1/(2*sqrt(z))"); POWER(0.5); STORE(Sqrt); MUL;
	ADDF("__rsqrt__", sqrt_, rsqrt);        DNAME("√"); DIFF(true, "1/(2*sqrt(z))"); POWER(0.5);// with √-1 = Undefined
	ADDF("√",   sqrt_, NULL, NULL, sqrt_);              DIFF(true, "1/(2*sqrt(z))"); POWER(0.5); MUL;

	ADDF("clamp", clamp,  clamp); DIFF(false, "(0<x)*(x<1)", "i*(0<y)*(y<1)"); PROJ; COMMUTES(Re); COMMUTES(Im);
	ADDF("clamp", clamps, clamps);
	DIFF(false, "(x2<x1)*(x1<x3)+(x3<x1)*(x1<x2)", "i*((y2<y1)*(y1<y3)+(y3<y1)*(y1<y2))",
	            "(x3<x2)*(x2<x1)+(x1<x2)*(x2<x3)", "i*((y3<y2)*(y2<y1)+(y1<y2)*(y2<y3))",
	            "(x1<x3)*(x3<x2)+(x2<x3)*(x3<x1)", "i*((y1<y3)*(y3<y2)+(y2<y3)*(y3<y1))");
	
	ADDF("swap",  swap            ); DIFF(false, "i", "1"); INVO; ODD; LIN; MUL;
	ADDF("det",   c_det, zero, det); DIFF(false, "y2", "x2", "y1", "x1"); REAL;
	ADDF("sp",    c_sp,  mul,  sp);  DIFF(false, "x2", "y2", "x1", "y2"); REAL;
	ADDF("spow",  spow,  spow);      DIFF(false, "spow(z1,z2-2)*(z2*x1-i*y1)", "spow(z1,z2-2)*(z2*y1+i*x1)",
	                                             "spow(z1,z2)*ln(abs(z1))", "i*spow(z1,z2)*ln(abs(z1))");
	ADDF("hypot", hypot, hypot); STORE(Hypot);
	DIFF(false, "x1/hypot(x1,x2)", "y1/hypot(x1,x2)", "x2/hypot(x1,x2)", "y2/hypot(x1,x2)");

	ADDF("mida", mida); DIFF(true, "0.5", "0.5");
	ADDF("midg", midg); DIFF(true, "z2 / 2sqrt(z1z2)", "z1 / 2sqrt(z1z2)");
	ADDF("midh", midh); DIFF(true, "2*(z2/(z1+z2))²", "2*(z1/(z1+z2))²");

	ADDF("min", r_min, r_min, r_min); DIFF(false, "x1<x2", "0", "x1>x2", "0"); REAL;
	ADDF("max", r_max, r_max, r_max); DIFF(false, "x1>x2", "0", "x1<x2", "0"); REAL;
	
	ADDF("absmin", c_absmin, c_absmin); DIFF(true, "abs(z1)<abs(z2)", "abs(z1)>abs(z2)");
	ADDF("absmax", c_absmax, c_absmax); DIFF(true, "abs(z1)>abs(z2)", "abs(z1)<abs(z2)");
	
	ADDF("realmin", c_rmin, r_min); DIFF(true, "x1<x2", "x1>x2");
	ADDF("realmax", c_rmax, r_max); DIFF(true, "x1>x2", "x1<x2");
	
	ADDF("imagmin", c_imin, c_imin); DIFF(true, "y1<y2", "y1>y2");
	ADDF("imagmax", c_imax, c_imax); DIFF(true, "y1>y2", "y1<y2");

	ADDF("blend",   blend_); DIFF(false, "1-clamp(x3)", "i*(1-clamp(x3))", "clamp(x3)", "i*clamp(x3)", "(z2-z1)*(x3>0)*(x3<1)", "0");
	ADDF("cblend", cblend_); DIFF(false, "(1+cos(π*x3))/2", "i*(1+cos(π*x3))/2", "(1-cos(π*x3))/2", "i*(1-cos(π*x3))/2", "π/2*(z2-z1)*sin(π*x3)", "0");
	ADDF("mix",       mix_); DIFF(true,  "1-z3", "z3", "z2-z1");

	//------------------------------------------------------------------------------------------------------------------
	// Functions - Exp
	//------------------------------------------------------------------------------------------------------------------
	
	ADDF("exp",    exp_, exp);  DIFF(true, "exp(z)"); STORE(Exp);
	ADDF("ln",     log_);       DIFF(true, "1/z"); STORE(Log_); INV(Exp);
	ADDF("log",    log_);       DIFF(true, "1/z"); STORE(Log);  INV(Exp);
	ADDF("log2",   log2);       DIFF(true, "1/(ln2(z))");  DNAME("log₂");
	ADDF("log10",  log10);      DIFF(true, "1/(ln10(z))"); DNAME("log₁₀");
	ADDF("normal", pdf_normal); DIFF(true, "-z*normal(z)");
	ADDF("erf",    erf_,  erf_);  DIFF(true,  "2*exp(-z²)/√π");
	ADDF("erfc",   erfc_, erfc_); DIFF(true, "-2*exp(-z²)/√π");
	
	//------------------------------------------------------------------------------------------------------------------
	// Functions - Trig
	//------------------------------------------------------------------------------------------------------------------

	ADDF("sin", sin, sin); DIFF(true, "cos(z)"); ODD; store(Sin); EF->range(R_Real, R_Interval);
	ADDF("cos", cos, cos); DIFF(true, "-sin(z)"); EVEN; store(Cos); EF->range(R_Real, R_Interval);
	ADDF("sec", sec, sec); DIFF(true, "tan(z)*sec(z)"); EVEN; store(Sec);
	ADDF("csc", csc, csc); DIFF(true, "-csc(z)*cot(z)"); ODD; store(Csc);
	ADDF("tan", tan, tan); DIFF(true, "1/(cosz)²"); ODD; store(Tan);
	ADDF("cot", cot, cot); DIFF(true, "-1/(sinz)²"); ODD; store(Cot);
	
	ADDF("sinh", sinh, sinh); DIFF(true, "cosh(z)"); ODD; store(Sinh);
	ADDF("cosh", cosh, cosh); DIFF(true, "sinh(z)"); EVEN; store(Cosh);
	ADDF("sech", sech, sech); DIFF(true, "-sech(z)*tanh(z)"); EVEN; store(Sech);
	ADDF("csch", csch, csch); DIFF(true, "-csch(z)*coth(z)"); ODD; store(Csch);
	ADDF("tanh", tanh, tanh); DIFF(true, "1-(tanhz)²"); ODD; store(Tanh);
	ADDF("coth", coth, coth); DIFF(true, "1-(cothz)²"); ODD; store(Coth);

	ADDF("arcsin", asin); DIFF(true,  "1/sqrt(1-z²)"); INV(Sin);
	ADDF("arccos", acos); DIFF(true, "-1/sqrt(1-z²)"); INV(Cos);
	ADDF("arcsec", asec); DIFF(true,  "1/(z²*sqrt(1-1/z²))"); INV(Sec);
	ADDF("arccsc", acsc); DIFF(true, "-1/(z²*sqrt(1-1/z²))"); INV(Csc);
	ADDF("arctan", atan, atan); DIFF(true,  "1/(1+z²)"); INV(Tan);
	ADDF("arccot", acot);       DIFF(true, "-1/(1+z²)"); INV(Cot);
	
	ADDF("arsinh", asinh); DIFF(true,  "1/sqrt(1+z²)"); INV(Sinh);
	ADDF("arcosh", acosh); DIFF(true,  "1/(sqrt(z+1)*sqrt(z-1))"); INV(Cosh);
	ADDF("arsech", asech); DIFF(true,  "sqrt((1-z)/(1+z))/(z*(z-1))"); INV(Sech);
	ADDF("arcsch", acsch); DIFF(true,  "-1/(z²*sqrt(1+1/z²))"); INV(Csch);
	ADDF("artanh", atanh); DIFF(true,  "1/(1-z²)"); INV(Tanh);
	ADDF("arcoth", acoth); DIFF(true,  "1/(1-z²)"); INV(Coth);

	//------------------------------------------------------------------------------------------------------------------
	// Functions - Gamma
	//------------------------------------------------------------------------------------------------------------------
	
	ADDF("gamma",     gamma_, gamma_); DIFF(true, "digamma(z)*gamma(z)");
	ADDF("Γ",         gamma_, gamma_); DIFF(true, "digamma(z)*gamma(z)");
	ADDF("digamma",   digamma);      DIFF(true, "trigamma(z)");
	ADDF("trigamma",  trigamma);
	ADDF("factorial", fakt, fakt); DIFF(true, "digamma(z+1)*factorial(z)");
	ADDF("bico", binomco); DIFF(true, "bico(z1,z2)*(digamma(z1+1)-digamma(n-k+1))",
	                                 "-bico(z1,z2)*(digamma(z2+1)-digamma(n-k+1))");
	ADDF("beta", beta); DIFF(true, "beta(z1,z2)*(digamma(z1)-digamma(z1+z2))",
	                               "beta(z1,z2)*(digamma(z2)-digamma(z1+z2))");
	//------------------------------------------------------------------------------------------------------------------
	// Functions - Random
	//------------------------------------------------------------------------------------------------------------------

	ADDF("riemann_random", riemann_rand, NULL, false);
	ADDF("rrnd",           riemann_rand, NULL, false);
	ADDF("disk_random", disk_rand, NULL, false);
	ADDF("drnd",        disk_rand, NULL, false);
	ADDF("random", real_rand, real_rand, false);
	ADDF("rnd",    real_rand, real_rand, false);
	ADDF("normal_random", normal_rand, normal_rand, false);
	ADDF("nrnd",          normal_rand, normal_rand, false);
	ADDF("normal_z_random", normal_z_rand, NULL, false);
	ADDF("nzrnd",           normal_z_rand, NULL, false);

	//------------------------------------------------------------------------------------------------------------------
	// Functions - Other
	//------------------------------------------------------------------------------------------------------------------
	
	ADDF("mandel", mandel); DIFF(true, "0"); combines(EF, Conj, First); REAL;
	ADDF("julia", julia); DIFF(true, "0", "0"); REAL;
	ADDF("fowler", fowler_angle, fowler_angle, fowler_angle); REAL;
	ADDF("wp",weierp);

	// boost
	ADDF("J", bessel_J, bessel_J, bessel_J); DIFF(true, "NAN", "(J(z1-1,z2)-J(z1+1,z2))/2");
	ADDF("Y", bessel_Y, bessel_Y, bessel_Y); DIFF(true, "NAN", "(Y(z1-1,z2)-Y(z1+1,z2))/2");
	ADDF("I", bessel_I, bessel_I, bessel_I); DIFF(true, "NAN", "(I(z1-1,z2)+I(z1+1,z2))/2");
	ADDF("K", bessel_K, bessel_K, bessel_K); DIFF(true, "NAN", "-(K(z1-1,z2)+K(z1+1,z2))/2");
	ADDF("Ei", expint_i, expint_i, expint_i); DIFF(true, "exp(z)/z");
	ADDF("En", expint_n, expint_n, expint_n); DIFF(true, "NAN", "-En(z1-1,z2)");
	ADDF("Ai", airy_ai, airy_ai, airy_ai);    DIFF(false, "Ai'(z)", "NAN");
	ADDF("Bi", airy_bi, airy_bi, airy_bi);    DIFF(false, "Bi'(z)", "NAN");
	ADDF("Ai'", airy_ai_prime, airy_ai_prime, airy_ai_prime);
	ADDF("Bi'", airy_bi_prime, airy_bi_prime, airy_bi_prime);

	ADDF("ieee_m", MANTISSA, MANTISSA); DIFF(false, "sgn(x)/2^ieee_e(x)", "i*sgn(y)/2^ieee_e(y)");
	ADDF("ieee_e", EXPONENT, EXPONENT); DIFF(false, "0", "0");
	ADDF("bit", IBIT, IBIT); DIFF(false, "0", "0", "0", "0");

	#if 0
	ADDBO("IEEE_AND",  8,  true,   true,   false, AND, AND);
	ADDBO("IEEE_OR",   8,  true,   true,   false, OR,  OR);
	ADDBO("IEEE_XOR",  8,  true,   true,   false, XOR, XOR);
	ADDUO("IEEE_NOT", false, NOT, NOT);
	ADDF("IEEE_SHL", SHL, SHL);
	ADDF("IEEE_SHR", SHR, SHR);
	ADDF("IEEE_ROL", ROL, ROL);
	ADDF("IEEE_ROR", ROR, ROR);
	ADDF("IEEE_BIT", DBIT, DBIT);
	#endif
	
	assert(nameless.empty());
}

RootNamespace::Combination RootNamespace::combine(const Element *f, const Element *g) const
{
	// only interesting for functions that survive the normalization in WorkingTree
	
	if (f == ConjOp) f = Conj;
	if (g == ConjOp) g = Conj;
	
	if (f == g && f->isFunction())
	{
		Function *F = (Function*)f;
		return F->involution() ? Collapses
		: F->projection() ? First // or second, whatever
		: F->collapses()  ? Zero
		: Commutes;
	}

	auto i = combinations.find(std::make_pair(f,g));
	return i == combinations.end() ? Nothing : i->second;
}

void RootNamespace::candidates(const std::string &s, size_t pos, std::vector<Element*> &ret) const
{
	if (pos >= s.length()) return;
	Namespace::candidates(s, pos, ret);
	if      (s[pos] == '+') ret.push_back(UPlus);
	else if (s[pos] == '-') ret.push_back(UMinus);
}
void RootNamespace::rcandidates(const std::string &s, size_t pos, size_t n, std::vector<Element*> &ret) const
{
	if (pos >= s.length() || n == 0) return;
	Namespace::rcandidates(s, pos, n, ret);
	if      (s[pos+n-1] == '+') ret.push_back(UPlus);
	else if (s[pos+n-1] == '-') ret.push_back(UMinus);
}

void RootNamespace::clear()
{
	auto it = named.begin();
	while (it != named.end())
	{
		Element *x = it->second;
		if (!x->builtin())
		{
			x->ns = NULL;
			delete x;
			named.erase(it++);
		}
		else
		{
			++it;
		}
	}
	
	for (Element *p : nameless)
	{
		assert(!p->builtin());
		p->ns = NULL;
		delete p;
	}
	nameless.clear();
	
	// leave linked as is
}

const std::vector<Rule> &RootNamespace::rules(const Element *head) const
{
	if (m_rules.empty())
	{
		std::vector<Rule> R;
		::load(rules_file_path(), R, *const_cast<RootNamespace*>(this));
		for (Rule &r : R)
		{
			const Element *e = r.head();
			m_rules[e].push_back(std::move(r));
		}
	}
	auto i = m_rules.find(head);
	if (i != m_rules.end()) return i->second;
	static std::vector<Rule> nothing;
	return nothing;
}
