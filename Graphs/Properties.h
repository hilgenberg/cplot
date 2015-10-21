#pragma once
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <stdio.h>

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
};

class PropertyList
{
public:
	std::map<std::string, Property> &properties() const
	{
		if (props.empty()) const_cast<PropertyList*>(this)->init_properties();
		return const_cast<std::map<std::string, Property> &>(props);
	}

	void print_properties() const
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

protected:
	mutable std::map<std::string, Property> props;

	// this should actually be const, but the setters would need too
	// much casting if we enforce that
	virtual void init_properties() = 0;
};

class GL_Color;
std::string format(const GL_Color &c);
void parse(const std::string &s, GL_Color &c);

std::string format(double x);
void parse(const std::string &s, double &x);

