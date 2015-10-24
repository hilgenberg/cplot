#pragma once
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <stdio.h>
#include <stdarg.h>

enum PropertyType{ PT_Color, PT_Bool, PT_Font, PT_Other };

struct Property
{
	Property() : type(PT_Other){ }
	std::string desc; // name is always a map key
	std::function<bool()> vis; // leave empty for always visible
	
	std::function<std::string()> get; // should never be empty and never throw
	std::function<void(const std::string &v)> set; // throws on failure, empty for ro

	std::function<std::vector<std::string>()> values; // for enum type properties

	PropertyType type;

	bool visible() const{ return !vis || vis(); }

	void set_enum(int &x, ...);
	void set_enum(std::function<void(void)> post_set, int &x, ...);
private:
	void set_enum(std::function<void(void)> ps, int &x, va_list va);
};

#define VALUES(...) values=[]{ const char *v[]={__VA_ARGS__}; return std::vector<std::string>(v,v+sizeof(v)/sizeof(v[0]));}
// for static list of values - usage: prop.VALUES("foo", "bar");

class Namespace;
class PropertyList
{
public:
	PropertyList() = default;
	PropertyList(const PropertyList &) : props(/*don't copy*/){}

	std::map<std::string, Property> &properties() const
	{
		if (props.empty()) const_cast<PropertyList*>(this)->init_properties();
		return const_cast<std::map<std::string, Property> &>(props);
	}

	void print_properties() const;

protected:
	mutable std::map<std::string, Property> props;

	// this should actually be const, but the setters would need too
	// much casting if we enforce that
	virtual void init_properties() = 0;

	virtual const Namespace &pns() const = 0;

	double parse_double(const std::string &s) const;
	double parse_percentage(const std::string &s) const; // in [0,1]
	bool   parse_bool(const std::string &s) const;
	void parse_range(const std::string &s, double &c0, double &r0) const;
	std::string format_double(double d) const;
	std::string format_percentage(double d) const;
	std::string format_bool(bool b) const;
	std::string format_range(double a, double b, bool minmax) const; // minmax: range is [a,b], else [a-b, a+b] (a = center, b = halfrange)
};

