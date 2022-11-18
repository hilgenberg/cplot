#include "readconst.h"
#include "../cnum.h"
#include "utf8/utf8.h"

#include <map>
#include <algorithm>

// all constant names must have balanced parentheses!
// keep the matching in readconst up-to-date with the names
static const std::map<const std::string, std::pair<const double, const std::string>> constants
{
	// <name> --> <value>,<unit>
	// ⁻⁰¹²³⁴⁵⁶⁷⁸⁹
	{"Avogadro", {6.02214129e23,   "mol⁻¹"       }},
	{"c0",       {299792458.0,     "m s⁻¹"       }}, // speed of light in vacuum
	{"c",        {299792458.0,     "m s⁻¹"       }}, // dito
	{"G",        {6.67384e-11,     "kg⁻¹ m³ s⁻²" }}, // Newtonian constant of gravitation
	{"h",        {6.62606957e-34,  "kg m² s⁻¹"   }}, // Planck constant
	{"ℏ",        {1.054571726e-34, "kg m² s⁻¹"   }}, // h / 2π
	{"hbar",     {1.054571726e-34, "kg m² s⁻¹"   }}, // dito
	{"µ0",       {M_PI*4e-7,       "kg m s⁻² A⁻²"}}, // magnetic constant / vacuum permeability
	{"eps0",     {1.0e-9 / (M_PI*4.0 * 2.99792458 * 2.99792458), "kg⁻¹ m⁻³ s⁴ A²"}}, // electric constant / vacuum permittivity
	{"alpha",    {7.2973525698e-3, "1"           }}, // fine-structure constant
	{"e",        {1.602176565e-19, "A s"         }}, // elementary charge
	{"lP",       {1.616199e-35,    "m"           }}, // Planck length
	{"mP",       {2.17651e-8,      "kg"          }}, // Planck mass
	{"tP",       {5.39106e-44,     "s"           }}, // Planck time
	{"qP",       {1.875545956e-18, "A s"         }}, // Planck charge
	{"TP",       {1.416833e32,     "K"           }}  // Planck temperature
};

static char tolower_(char c) { return (char)::tolower((int)(unsigned char)c); }
static char toupper_(char c) { return (char)::toupper((int)(unsigned char)c); }

static inline bool find_const(std::string &name, double &value)
{
	// try as-is
	auto it = constants.find(name);
	if (it != constants.end())
	{
		value = it->second.first;
		return true;
	}
	
	// trim spaces off the ends
	while (!name.empty() && isspace(name[0])) name.erase(0);
	while (!name.empty() && isspace(*name.rbegin())) name.erase(name.length()-1);
	it = constants.find(name);
	if (it != constants.end())
	{
		value = it->second.first;
		return true;
	}
	
	// try lowercase
	std::transform(name.begin(), name.end(), name.begin(), ::tolower_);
	it = constants.find(name);
	if (it != constants.end())
	{
		value = it->second.first;
		return true;
	}
	
	// try uppercase
	std::transform(name.begin(), name.end(), name.begin(), ::toupper_);
	it = constants.find(name);
	if (it != constants.end())
	{
		value = it->second.first;
		return true;
	}
	
	// give up
	return false;
}

size_t readconst(const std::string &S, size_t i0, size_t i1, double &value)
{
	try
	{
		if (i1 < i0 + 5+1+1) return 0; // "const" + '(' | '_' + at least one char
		if (S.find("const", i0) != i0) return 0;
		
		auto s = S.begin() + (i0+5), s1 = S.begin() + i1;
		char sep = *s++;
		auto n0 = s;
		size_t n = 0, off = 6; // namelen, additional chars
		
		if (sep == '(')
		{
			int pl = 1;
			auto s0 = s;
			while (pl > 0 && s < s1)
			{
				switch(utf8::next(s, s1))
				{
					case '(': ++pl; break;
					case ')': --pl; break;
				}
			}
			n = s - s0;
			if (pl != 0 || n <= 1) return 0;
			--n; ++off; // the closing ')'
		}
		else if (sep == '_')
		{
			auto s0 = s;
			while (s < s1)
			{
				auto s00 = s;
				uint32_t c = utf8::next(s, s1);
				if (isalnum(c) || c == '_' || c == 0x210F /*hbar*/ || c == 0xB5 /* µ */) continue;
				
				s = s00;
				break;
			}
			n = s - s0;
		}
		else
		{
			return 0;
		}
		
		std::string name = S.substr(n0-S.begin(), n);
		if (find_const(name, value)) return n+off;
	}
	catch(utf8::exception &)
	{
		// ignore
	}
	return 0;
}

std::string const_name(double value)
{
	for (auto i : constants)
	{
		if (fabs(i.second.first - value) < EPSILON) return i.first;
	}
	return std::string();
}

