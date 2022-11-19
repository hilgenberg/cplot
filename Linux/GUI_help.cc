#include "GUI.h"
#include "imgui/imgui.h"
#include "../Utility/Preferences.h"
extern const char *VERSION;

#define SPC ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing()
static constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
static constexpr int tn_flags = ImGuiTreeNodeFlags_Selected|ImGuiTreeNodeFlags_FramePadding|ImGuiTreeNodeFlags_SpanAvailWidth;//ImGuiTreeNodeFlags_DefaultOpen;

#define ROW2(a,b) ImGui::TableNextRow();\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(a);\
	ImGui::TableNextColumn(); ImGui::TextWrapped(b)
#define ROW3(a,b,c) ImGui::TableNextRow();\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(a);\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(b);\
	ImGui::TableNextColumn(); ImGui::TextWrapped(c)
#define ROW4(a,b,c,d) ImGui::TableNextRow();\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(a);\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(b);\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(c);\
	ImGui::TableNextColumn(); ImGui::TextWrapped(d)
#define ROW_OP(a,b,c,d) ImGui::TableNextRow();\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(a);\
	ImGui::TableNextColumn(); ImGui::TextWrapped(b);\
	ImGui::TableNextColumn(); ImGui::TextUnformatted(c);\
	ImGui::TableNextColumn(); ImGui::TextWrapped(d)

void GUI::help_panel()
{
	if (!show_help_panel) return;
	ImGui::SetNextWindowBgAlpha(0.85f);
	if (!ImGui::Begin("Help", &show_help_panel)) { ImGui::End(); return; }

	if (ImGui::CollapsingHeader("About"))
	{
		ImGui::BulletText("Version:   %s"
		#ifdef DEBUG
		" (debug build)"
		#endif
		, VERSION);
		ImGui::BulletText("CPU Cores: Using %d of %d", Preferences::threads(), n_cores);
		ImGui::BulletText("Config:    %s", Preferences::directory().c_str());
		ImGui::BulletText("Homepage:  http://zoon.cc/cplot/");
		ImGui::BulletText("Source:    https://github.com/hilgenberg/cplot/");
	}

	if (ImGui::CollapsingHeader("Controls"))
	{
		ImGui::TextWrapped("The view of a plot is controlled by three components:");
		ImGui::Bullet(); ImGui::TextWrapped("The axis range.");
		ImGui::Bullet(); ImGui::TextWrapped("Camera zoom and rotation for 3D plots.");
		ImGui::Bullet(); ImGui::TextWrapped("The preimage range of parametric plots.");
		SPC;
		ImGui::TextWrapped("These can be set precisely with text fields in the lower part of the side panel or via mouse and keyboard. Generally, holding Shift turns the translation actions into zoom actions. Holding Control is for the axis, Alt is for the preimage range. Two-finger-scrolling on a trackpad does the same as the arrow keys. Holding the left and right mouse buttons is the same as holding the middle button.");
		SPC;
		ImGui::TextWrapped("Supported inputs are:");
		ImGui::Bullet(); ImGui::TextWrapped("Keyboard: Arrows and +/- keys.");
		ImGui::Bullet(); ImGui::TextWrapped("Mouse: Dragging with left, right, or both buttons. Right for axis, middle for preimage range.");
		ImGui::Bullet(); ImGui::TextWrapped("Mouse: Scroll wheel(s), combined with mouse buttons or modifier keys.");
		ImGui::Bullet(); ImGui::TextWrapped("Trackpad: Combined with modifier keys.");
		SPC;
		ImGui::TextWrapped("Parameters can be changed by dragging them in the status bar at the bottom. Hold Alt and/or Control to slow down, Shift to speed up the changes.");
	}

	if (ImGui::CollapsingHeader("Hotkeys"))
	{
		ImGui::TextWrapped("In addition the the menu hotkeys, these keys work directly on the plot view:");
		ImGui::BulletText("Hold 0..9 and arrows to change parameters");
		ImGui::BulletText("Hold 0..9 and space to animate parameters");
		ImGui::BulletText("Period: Stop all animation");
		ImGui::BulletText("a: toggle axis drawing");
		ImGui::BulletText("c: toggle clipping");
		ImGui::BulletText("C: toggle custom clipping plane");
		ImGui::BulletText("l: toggle ccp locking");
		ImGui::BulletText("L: reset ccp to view");
		ImGui::BulletText("d: toggle discontinuity detection");
		ImGui::BulletText("g: toggle grid");
		ImGui::BulletText("v/V: cycle vector field modes");
		//ImGui::BulletText("Ctrl-L: focus function text field");
	}

	if (ImGui::CollapsingHeader("Builtin Functions"))
	{
		if (ImGui::TreeNodeEx("Constants", tn_flags))
		{
			if (ImGui::BeginTable("Constants Table", 2, table_flags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();
				ROW2("i",          "Imaginary unit: i² = -1");
				ROW2("pi, π",      "Smallest positive zero of sin(x).");
				ROW2("e",          "Euler's number");
				ROW2("eulergamma", "Euler-Mascheroni constant");
				ROW2("°",          "2pi/360, so 360° = 360 * ° = 2pi");
				ROW2("NAN",        "Undefined value. f(NAN) = NAN.");
				ImGui::EndTable();
			}
			SPC;
			ImGui::TextWrapped("For physical constants use either const(name) or const_name with any of these (names are case-insensitive):");
			if (ImGui::BeginTable("Physical Constants Table", 4, table_flags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Unit", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();
				ROW4("Avogadro", "6.022…e23",  "mol⁻¹", "particles per mole");
				ROW4("c0, c",    "2.997…e8",   "m s⁻¹", "speed of light");
				ROW4("G",        "6.673…e-11", "kg⁻¹ m³ s⁻²", "gravitation constant");
				ROW4("h",        "6.626…e-34", "J s",   "Planck's constant");
				ROW4("ℏ, hbar",  "1.054…e-34", "J s",   "const_h / 2π");
				ROW4("µ0",       "1.256…",     "N A⁻²", "magnetic constant");
				ROW4("eps0",     "8.854…e-12", "F m⁻¹", "electric constant");
				ROW4("alpha",    "7.297…e-3",  "",      "fine-structure constant");
				ROW4("e",        "1.602…e-19", "A s",   "elementary charge");
				ROW4("lP",       "1.616…e-35", "m",     "Planck length");
				ROW4("mP",       "2.176…e-8",  "kg",    "Planck mass");
				ROW4("tP",       "5.391…e-44", "s",     "Planck time");
				ROW4("qP",       "1.875…e-18", "A s",   "Planck charge");
				ROW4("TP",       "1.416…e32",  "K",     "Planck temperature");
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Operators, in order of precedence", tn_flags))
		{
			if (ImGui::BeginTable("Operators Table", 4, table_flags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Meaning", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Assoc", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Examples", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();
				ROW_OP("(), [], {}", "Grouping (see below)", "-", " ");
				ROW_OP("|z|", "Absolute value", "-", "|z| = abs(z)");
				ROW_OP("z! z¹²³", "Postfix operators (factorial, powers)", "-", "x^3! = x^(3!) = x^6");
				ROW_OP("", "Function call", "-", "sinx² = sin(x²), also sin(x)² = sin(x²), but (sinx)² = (sin(x))²");
				ROW_OP("-z ~z", "Prefix operators (Negation, complex conjugation)", "-", "-x² = -(x²)");
				ROW_OP(" ", "Implicit Multiplication", "left", "ab^cd = (ab)^(cd), abc = (ab)c, sinxy = (sinx)*y, xx² = x^3, not x^4");
				ROW_OP("^ **", "Exponentiation", "right", "a^b^c = a^(b^c)");
				ROW_OP("* / %", "Multiplication, Division, Modulus (only defined for real numbers: a %% b = a-b*floor(a/b). The result is always between 0 and b)", "left", "a/b/c = (a/b)/c = a/(bc), a*b^c*d = a*(b^c)*d");
				ROW_OP("+ -", "Addition, Subtraction", "left", "a-b-c = (a-b)-c = a-(b+c)");
				ROW_OP("and, or, xor", "Binary AND, OR, XOR (see bit function)", "left", "-(a and b) = -a or -b, a xor b = -a xor -b");
				ROW_OP("< >", "Comparison - only defined for real numbers: a > b := 1 if a > b, else 0", "-", "(a < b) * (b < c) is a < b < c.\nSets can be plotted like this: sin xx - sin yy > 0.");
				ROW_OP(" ", "Whitespace (grouping)", "-", "see below");
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Functions", tn_flags))
		{
			if (ImGui::BeginTable("Functions Table", 2, table_flags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				#define HEADING(a) ImGui::TableNextRow(ImGuiTableRowFlags_Headers);\
				ImGui::TableNextColumn();ImGui::TableNextColumn();ImGui::TextWrapped(a)

				HEADING("Simple & Polynomials");
				ROW2("re", "Real part: re(a+ib) = a");
				ROW2("im", "Imaginary part: im(a+ib) = b");
				ROW2("abs", "Magnitude/absolute value. Same as |z|.");
				ROW2("absq", "Squared magnitude: absq(z) := abs(z)^2 (but faster).");
				ROW2("arg", "Angle between the positive real axis and the connection line from 0 to z. In the range [-pi,pi).\narg(x, y) := arg(x+iy).");
				ROW2("sgn", "Sign function: sgn(z) = z / |z| or 0 if z = 0");
				ROW2("conj", "Complex conjugation, same as ~z.");
				ROW2("complex", "complex(x,y) = x+iy.");
				ROW2("hypot", "hypot(z,w) = sqrt(z²+w²)");
				ROW2("sqr", "sqr(z) = z²");
				ROW2("√, sqrt", "Square root, branch cut along the negative real axis.");
				ROW2("clamp", "clamp(x, a, b) restricts x to [min(a,b), max(a,b)]. clamp(x) = clamp(x,0,1).\nclamp(z, a, b) = clamp(re z, re a, re b) + i clamp(im z, im a, im b) for complex z, a, b.\nclamp(z) = clamp(z, 0, 1+i) = clamp(re z) + i clamp(im z).\nInvariant under permutation: clamp(a,b,c) = clamp(b,c,a) = clamp(c,b,a) = ...\nclamp(INF,x,y) = max(x,y), clamp(-INF,x,y) = min(x,y)\nclamp(x,x,y) = x, if a <= b <= c then clamp(a,b,c) = b");
				ROW2("det", "det(a+ib, c+id) = ad-bc");
				ROW2("sp", "sp(a+ib, c+id) = ac+bd");
				ROW2("swap", "swap(a+ib) = b+ia");
				ROW2("mida", "Arithmetic mean: mida(z,w) = (z+w)/2");
				ROW2("midg", "Geometric mean: midg(z,w) = √(z*w)");
				ROW2("midh", "Harmonic mean: midh(z,w) = 2zw / (z+w)");

				HEADING("Min/Max Variations");
				ROW2("min, max", "Minimum/Maximum. Only defined for real numbers: min(a,b) := a if a < b, else b");
				ROW2("absmin, absmax", "Returns the number with the smaller/larger absolute value: absmin(z, w) := z if |z| < |w|, else w");
				ROW2("realmin, realmax", "Returns the number with the smaller/larger real part");
				ROW2("imagmin, imagmax", "Returns the number with the smaller/larger imaginary part");

				HEADING("Exponential");
				ROW2("exp", "e^z (but exp(z) is a little faster and slightly more precise than the operator)");
				ROW2("ln, log", "Natural logarithm. Inverse of exp with branch cut along the negative real axis.");
				ROW2("log2", "Binary logarithm: log2(z) = log(z) / log(2)");
				ROW2("log10", "Decadic logarithm: log10(z) = log(z) / log(10)");
				ROW2("spow", "spow(x,y) = |x|^y * sgn(x)");

				HEADING("Trigonometric");
				ROW2("sin, cos", "Sine and Cosine");
				ROW2("tan", "Tangent function: tan(z) = sin(z) / cos(z)");
				ROW2("sec", "Secant: sec(z) = 1 / cos(z)");
				ROW2("csc", "Cosecant: csc(z) = 1 / sin(z)");
				ROW2("cot", "Cotangent: cot(z) = 1 / tan(z) = cos(z) / sin(z)");
				ROW2("sinh", "Hyperbolic sine: sinh(z) = (exp(z)-exp(-z))/2");
				ROW2("cosh", "Hyperbolic cosine: cosh(z) = (exp(z)+exp(-z))/2");
				ROW2("tanh", "Hyperbolic tangent: tanh(z) = sinh(z) / cosh(z)");
				ROW2("sech", "Hyperbolic secant: sech(z) = 1 / cosh(z)");
				ROW2("csch", "Hyperbolic cosecant: csch(z) = 1 / sinh(z)");
				ROW2("coth", "Hyperbolic cotangent: coth(z) = 1 / tanh(z)");
				ROW2("arcsin, arccos", "Inverse of sin and cos.");
				ROW2("arctan", "Inverse of tan(z)");
				ROW2("arcsec", "Inverse of sec(z), arcsec(z) = arccos(1/z)");
				ROW2("arccsc", "Inverse of csc(z), arccsc(z) = arcsin(1/z)");
				ROW2("arccot", "Inverse of cot(z), arccot(z) = arctan(1/z)");
				ROW2("arsinh, arcosh,\nartanh, arsech,\narcsch, arcoth", "Inverse hyperbolic functions");

				HEADING("Gamma");
				ROW2("gamma, Γ", "Gamma function");
				ROW2("factorial", "factorial(z) = z! = gamma(z+1)");
				ROW2("beta", "Beta function: beta(a,b) = gamma(a)gamma(b) / gamma(a+b)");
				ROW2("bico", "Binomial coefficient: bico(n,k) = n! / k!(n-k)!");
				ROW2("digamma", "Logarithmic derivative of the Gamma function: digamma(z) = (ln gamma)'(z) = gamma'(z) / gamma(z)");
				ROW2("trigamma", "Second logarithmic derivative of the Gamma function: trigamma(z) = (ln gamma)''(z)");

				HEADING("Special Functions");
				ROW2("wp", "Weierstraß' elliptic P-function. Doubly periodic with periods 2 and 2i, i.e. wp(z+2n+2im) = wp(z) for all integer n,m.");
				ROW2("J(n,x)", "Bessel function of the first kind, implemented only for real n and x.");
				ROW2("Y(n,x)", "Bessel function of the second kind, implemented only for real n and x.");
				ROW2("I(n,x)", "Hyperbolic Bessel function of the first kind, implemented only for real n and x.");
				ROW2("K(n,x)", "Hyperbolic Bessel function of the second kind, implemented only for real n and x.");
				ROW2("Ai(x)", "Airy function of the first kind, implemented only for real x.");
				ROW2("Bi(x)", "Airy function of the second kind, implemented only for real x.");
				ROW2("Ai'(x)", "Derivative of Ai, implemented only for real x.");
				ROW2("Bi'(x)", "Derivative of Bi, implemented only for real x.");
				ROW2("Ei(x)", "Exponential integral, implemented only for real x.");
				ROW2("En(n,x)", "Exponential integral, implemented only for real x and integer n >= 0.");

				HEADING("Probability & Random");
				ROW2("normal(z)", "Probability density function of the normal distribution with µ=0 and σ=1");
				ROW2("erf", "Error function");
				ROW2("erfc", "Complementary error function: erfc(z) = 1 - erf(z)");
				ROW2("random, rnd", "Generates uniformly distributed random variates in [-1,1].");
				ROW2("riemann_random,\nrrnd", "Generates complex numbers that are uniformly distributed on the Riemann sphere.");
				ROW2("disk_random,\ndrnd", "Generates complex numbers that are uniformly distributed on the unit disk {z: |z| < 1}.");
				ROW2("normal_random,\nnrnd", "Generates normal variates (µ=0, σ=1).");
				ROW2("normal_z_random,\nnzrnd", "Generates normally distributed complex numbers (i.e. real and imaginary parts are independent and normally distributed). Same as nrnd + i nrnd.");

				HEADING("Rounding");
				ROW2("round", "round(z) rounds the real and imaginary parts of z to the nearest integer.");
				ROW2("floor", "floor(z) rounds the real and imaginary parts of z down (i.e. towards -∞).");
				ROW2("ceil", "ceil(z) rounds the real and imaginary parts of z up (i.e. towards +∞).");

				HEADING("Binary");
				ROW2("bit(x,k)", "Return k'th bit of x. Only defined for integer k. bit(-x,k) = 1-bit(x,k) for all x and k (0=+0 with all bits zero). For x >= 0: x = sum(bit(x,k) * 2^k). For complex numbers: bit(x+iy, n) = bit(x,n) + i bit(y,n)");
				ROW2("ieee_m", "Mantissa of an IEEE 754 double precision floating point number. Includes the leading 1. Properties:\n|x| = ieee_m(x) * 2 ^ ieee_e(x)\nieee_m(-x) = ieee_m(x)\nieee_m(2x) = ieee_m(x)");
				ROW2("ieee_e", "Exponent of an IEEE 754 double precision floating point number. Bias is already subtracted, so ieee_e(1) = 0, not 1023.");

				HEADING("Misc");
				ROW2("blend", "Blends two functions, which can be useful for the image modes (\"Image\" and \"Riemann\"). Let s = max(0, min(1, t)), then blend(z,w,t) = s w + (1-s) z, which gives blend(z,w,0) = z and blend(z,w,1) = w. If t is complex, its imaginary part is ignored.");
				ROW2("cblend", "Similar to blend, but uses s = (1 + cos(pi t))/2, which gives cblend(z,w,2k) = z and cblend(z,w,2k+1) = w. Tends to look better in animation. If t is complex, its imaginary part is ignored.");
				ROW2("mix", "mix(z,w,t) = (1-t)z + tw. Unlike blend and cblend, does not ignore t's imaginary part.");
				ROW2("fowler", "Returns the Fowler angle of a complex number. Result is in the range [0,8).");
				ROW2("julia", "julia(z,c) = 1 if z is in the Julia set for c (the numbers for which the iteration z >> z²+c stays finite), otherwise closer to 0, depending on how quickly it diverges.");
				ROW2("mandel", "mandel(c) = 1 if c is in the Mandelbrot set (the numbers for which the iteration z >> z²+c stays finite when starting with z = 0), otherwise closer to 0, depending on how quickly it diverges.");

				ImGui::EndTable();
				#undef HEADING
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Expression Syntax"))
	{
		if (ImGui::TreeNodeEx("Plotting Variables", tn_flags))
		{
			ImGui::TextWrapped("Each preimage range has a set of primary variables and a set of automatically defined alias variables (but they can be overriden by parameters and zero-ary functions with the same name).");

			if (ImGui::BeginTable("Variables Table", 3, table_flags))
			{
				ImGui::TableSetupColumn("Preimage", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Primary", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Aliases", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				ROW3("R, S¹", "x", "u = t = z = x, v = y = 0, r = |x|");
				ROW3("R², S¹xS¹", "x, y", "u = x, v = y, z = 0, r = |x+iy|, phi = arg(x+iy)");
				ROW3("C", "z", "x = re(z), y = im(z), r = abs(z), phi = arg(z), u = x, v = y");
				ROW3("R³", "x, y, z", "r = length(x,y,z), phi, theta (spherical coordinates) and again u = x, v = y");
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Names and Numbers", tn_flags))
		{
			ImGui::TextWrapped("Names of functions, variables and parameters cannot contain spaces, commas, bars ('|'), control characters, parentheses, brackets, braces, unicode exponents or start with a digit. But other than that, the full unicode range is valid and there is no limit on the length.");
			SPC;
			ImGui::TextWrapped("Numbers can be decimal like 0.1234 or -12.34e-5, or (probably not too useful here) binary: 0b110.001101e1101 (the exponent is binary too), or hexadecimal: 0xA8B.CD53q-7F (since e is already a digit, we use q as exponent marker).");
			ImGui::TextWrapped("Numbers are not localized - the decimal separator is always a dot, never a comma because the comma separates multiple arguments.");
			SPC;
			ImGui::TextWrapped("Unicode superscripts are read as power operators. For example x⁻⁴⁺⁵ⁱ is equal to x^(-4+5i).");
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Expressions", tn_flags))
		{
			ImGui::Bullet(); ImGui::TextWrapped("Parentheses are optional unless a function has more than one argument (sin(x) and sinx are the same).");
			ImGui::Bullet(); ImGui::TextWrapped("Multiplication is implicit, so x*y and xy are the same.");
			ImGui::Bullet(); ImGui::TextWrapped("Whitespace is for grouping, so (x+y)*(z+w) can be written as x+y * z+w and sin(x+y) as sin x+y.");
			ImGui::Bullet(); ImGui::TextWrapped("Writing |z| for abs(z) is ok. When there are ambiguities, the parser will try bars as closing bars first. For example: |x|y|x| is abs(x) y abs(x) and not abs(x absy x).");
			ImGui::Bullet(); ImGui::TextWrapped("Functions can be overloaded (different functions with the same name for different numbers of arguments).");
			ImGui::Bullet(); ImGui::TextWrapped("When an expression is split into separate tokens, longer matches are always tried first if there are several possibilities. For example, even if there are parameters a and c, arcsin is never read as a*r*c*sin.");

			SPC;
			ImGui::TextUnformatted("Parentheses and whitespace:");
			ImGui::TextWrapped("Any block of tokens (without mismatched parentheses) that is surrounded by whitespace (beginning and end of the expression count as whitespace) and can be put inside parentheses without creating syntax errors will be put in parentheses and then whitespace is removed.");
			ImGui::TextUnformatted("For example:");
			ImGui::Bullet(); ImGui::TextWrapped("x*y ^ x*y becomes (x*y)^(x*y),");
			ImGui::Bullet(); ImGui::TextWrapped("x+y sin x becomes (x+y)sin(x),");
			ImGui::Bullet(); ImGui::TextWrapped("x* x+y * x becomes (x*(x+y))*(x), which is x*(x+y)*x,");
			ImGui::Bullet(); ImGui::TextWrapped("x *x+y x becomes ((x)*x+y)(x), which is (x*x+y)*x.");
			SPC;
			ImGui::TextWrapped("Parentheses work as usual for grouping subexpressions. Brackets and braces can be used as well and they must match: {sin[x]+cos[x]+1} is ok, but {x+y) is a syntax error.");
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Dear ImGui User Guide"))
	{
		ImGui::ShowUserGuide();
	}
	
	SPC;
	if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x, 0))) show_help_panel = false;
	ImGui::End();
}