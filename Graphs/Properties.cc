#include "Properties.h"
#include "../Engine/Namespace/Namespace.h"
#include <stdio.h>
#include <cstdarg>

void PropertyList::print_properties() const
{
	if (props.empty()) const_cast<PropertyList*>(this)->init_properties();
	std::vector<const std::string *> N, D;
	std::vector<std::string> V;
	int l1 = 0, l2 = 0, l;
	for (auto &i : props)
	{
		const Property &p = i.second;
		if (!p.visible() || !p.get) continue;
		N.push_back(&i.first);
		std::string v = p.get(); V.push_back(v);
		D.push_back(&p.desc);
		l = (int)i.first.length(); if (l > l1) l1 = l;
		l = (int)v.length();       if (l > l2) l2 = l;
	}

	l = (int)N.size();
	for (int i = 0; i < l; ++i)
	{
		printf("%-*s = %-*s (%s)\n", 
			l1, (*N[i]).c_str(), 
			l2, V[i].c_str(), 
			(*D[i]).c_str());
	}
}

void Property::set_enum(const EnumCvt &ec, ...)
{
	va_list ap; va_start(ap, ec);

	std::map<std::string, int> V;
	std::map<int, std::string> N;
	std::vector<std::string>   VS;

	const char *s;
	while ((s = va_arg(ap, const char*)))
	{
		int v = va_arg(ap, int);
		VS.push_back(s);
		V[s] = v;
		N[v] = s;
		std::string s0(1,*s);
		assert(V.find(s0) == V.end());
		V[s0] = v;
	}

	get = [ec,N]()->std::string
	{
		auto i = N.find(ec.get());
		if (i != N.end()) return i->second;
		return "corrupt";
	};
	set = [ec,V,this](const std::string &s)
	{
		auto i = V.find(s);
		if (i != V.end())
		{
			ec.set(i->second);
			if (ec.post_set) ec.post_set();
		}
		else throw std::runtime_error(format("Not a valid %s: \"%s\"", desc.c_str(), s.c_str()));
	};
	values = [VS]{ return VS; };
	va_end(ap);
}

double PropertyList::parse_double(const std::string &s) const
{
	cnum v = evaluate(s, pns());
	if (!defined(v) || !is_real(v)) throw error("Not a real number", s);
	return v.real();
}
std::string PropertyList::format_double(double d) const
{
	return format("%g", d);
}

double PropertyList::parse_percentage(const std::string &s) const
{
	double x;
	if (!s.empty() && *s.rbegin() == '%')
	{
		x = parse_double(s.substr(0, s.length()-1))*0.01;
	}
	else
	{
		x = parse_double(s);
	}
	return x < 0.0 ? 0.0 : x > 1.0 ? 1.0 : x;
}
std::string PropertyList::format_percentage(double d) const
{
	return format("%.2g", d);
}

bool PropertyList::parse_bool(const std::string &s) const
{
	if (s == "0" || s == "off" || s == "false" || s ==  "no") return false;
	if (s == "1" || s == "on"  || s == "true"  || s == "yes") return true;
	throw error("Not a valid boolean", s);
}

std::string PropertyList::format_bool(bool b) const
{
	return b ? "on" : "off";
}

void PropertyList::parse_range(const std::string &s, double &c0, double &r0) const
{
	// input range is [c0-r0, c0+r0]
	// syntax: [x0,x1], (x0,x1), [x0;x1], (x0;x1)
	//         xm+-r, r, xm+-

	size_t n = s.length(); if (!n) throw std::runtime_error("Invalid range");

	// intervals
	if (s[0] == '(' && s[n-1] == ')' || s[0] == '[' && s[n-1] == ']')
	{
		int pl = 0, nc = 0, c = 0;
		for (size_t i = 1; i+1 < n; ++i)
		{
			switch (s[i])
			{
				case '(': ++pl; break;
				case ')': --pl; break;
				case ',':
				case ';': if (!pl){ ++nc; c = i; } break;
			}
		}
		if (nc == 1)
		{
			double a = parse_double(s.substr(1, c-1));
			double b = parse_double(s.substr(c+1, n-c-2));
			c0 = (a+b)*0.5;
			r0 = fabs(b-a)*0.5;
			return;
		}
	}

	// m +- r
	int pl = 0, pm = -1;
	for (size_t i = 0; i < n; ++i)
	{
		switch (s[i])
		{
			case '(': ++pl; break;
			case ')': --pl; break;
			case '+': if (!pl && i+1 < n && s[i+1] == '-') pm = i; break;
		}
	}
	if (pm == 0)
	{
		r0 = fabs(parse_double(s.substr(2)));
	}
	else if (pm > 0)
	{
		// 1+-2
		double a = parse_double(s.substr(0, pm));
		double b = parse_double(s.substr(pm+2));
		c0 = a;
		r0 = fabs(b);
	}
	else
	{
		c0 = parse_double(s);
	}
}

std::string PropertyList::format_range(double a, double b, bool minmax) const
{
	if (!minmax){ b += a; a = 2.0*a-b; } // a-b and a+b now
	return format("[%g;%g]", a, b);
}
