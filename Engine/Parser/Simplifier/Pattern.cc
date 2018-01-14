#include "Pattern.h"
#include "../ParsingTree.h"
#include "../WorkingTree.h"
#include "../../Namespace/Function.h"
#include "../../Namespace/RootNamespace.h"
#include "../../Namespace/Variable.h"
#include "../../../Utility/StringFormatting.h"
#include <fstream>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------
// helper functions
//----------------------------------------------------------------------------------------------------------------------

static bool equal(const WorkingTree &a, const WorkingTree &b)
{
	return a == b; // todo - handle commutativity, etc
}

//----------------------------------------------------------------------------------------------------------------------
// Pattern
//----------------------------------------------------------------------------------------------------------------------

bool Rule::Pattern::matches(const WorkingTree &t, Rule::Pattern::Bindings &bindings) const
{
	switch (type)
	{
		case TT_Constant: return t.type == WorkingTree::TT_Constant && t.constant == constant;
		case TT_Number:   return t.type == WorkingTree::TT_Number && eq((cnum)t, number);
		case TT_Function:
		{
			if (!t.is_function(function)) return false;
			int n = num_children(); if (n != t.num_children()) return false;
			Bindings b0(bindings);
			for (int i = 0; i < n; ++i)
			{
				// todo - handle commutativity
				if (!child(i)->matches(t.child(i), b0)) return false;
			}
			bindings = b0;
			return true;
		}
			
		case TT_Wildcard:
		{
			auto i = bindings.find(this);
			if (i == bindings.end())
			{
				if (!subset(t.range(), range)) return false;
				bindings.insert(std::make_pair(this, &t));
				return true;
			}
			else
			{
				return equal(t, *i->second);
			}
		}
		case TT_Sum:
		case TT_Product:
		{
			if (t.type != (type == TT_Sum ? WorkingTree::TT_Sum : WorkingTree::TT_Product)) return false;
			//if (num_children() > t.num_children()) return false;
			// todo: handle a+b+c vs $x+$y --> (a+b)+c
			int n = num_children(); if (n != t.num_children()) return false;
			Bindings b0(bindings);
			for (int i = 0; i < n; ++i)
			{
				// todo - handle commutativity
				if (!child(i)->matches(t.child(i), b0)) return false;
			}
			bindings = b0;
			return true;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

WorkingTree Rule::Pattern::apply(const Rule::Pattern::Bindings &bindings, const RootNamespace &rns) const
{
	switch (type)
	{
		case TT_Constant: return WorkingTree(constant);
		case TT_Number:   return WorkingTree(number, rns);
		case TT_Function:
		{
			WorkingTree t(function);
			for (auto *c : children) t.children.emplace_back(c->apply(bindings, rns));
			assert(t.verify());
			return t;
		}
			
		case TT_Wildcard:
		{
			auto i = bindings.find(this);
			if (i == bindings.end()) throw std::logic_error("apply with unbound wildcard");
			assert(i->second->verify());
			return *i->second;
		}
		case TT_Sum:
		case TT_Product:
		{
			WorkingTree t(&rns, type == TT_Sum ? WorkingTree::TT_Sum : WorkingTree::TT_Product);
			for (auto *c : children) t.children.emplace_back(c->apply(bindings, rns));
			assert(t.verify());
			return t;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

void Rule::Pattern::collect_elements(std::set<Element*> &e, const RootNamespace &rns) const
{
	for (auto *c : children) c->collect_elements(e, rns);
	switch (type)
	{
		case TT_Wildcard:
		case TT_Number:   break;

		case TT_Constant: e.insert((Element*)constant); break;
		case TT_Function: e.insert((Element*)function); break;
		case TT_Sum:      e.insert((Element*)rns.Plus); break;
		case TT_Product:  e.insert((Element*)rns.Mul); break;
	}
}

Function *Rule::Pattern::head(const RootNamespace &rns) const
{
	switch (type)
	{
		case TT_Constant:
		case TT_Number:
		case TT_Wildcard: assert(false); return NULL;
			
		case TT_Function: return (Function*)function;
		case TT_Sum:      return rns.Plus;
		case TT_Product:  return rns.Mul;
	}
	assert(false); throw std::logic_error("can't happen");
}

Rule::Pattern *Rule::Pattern::load(bool replacement, const WorkingTree &t,
								   const std::map<const Variable *, Range> &types,
								   std::map<const Variable*, Rule::Pattern*> &vars)
{
	Pattern *p = NULL;
	
	switch (t.type)
	{
		case WorkingTree::TT_Root:
		case WorkingTree::TT_Parameter: assert(false); return NULL;

		case WorkingTree::TT_Constant:
			p = new Pattern(TT_Constant);
			p->constant = t.constant;
			return p;

		case WorkingTree::TT_Number:
			p = new Pattern(TT_Number);
			p->number = (cnum)t;
			return p;

		case WorkingTree::TT_Variable:
		{
			auto i = vars.find(t.variable);
			if (i != vars.end()) return i->second;
			if (replacement){ assert(false); return NULL; }

			auto j = types.find(t.variable);
			if (j == types.end()){ assert(false); return NULL; }

			p = new Pattern(TT_Wildcard); p->range = j->second;
			vars.insert(std::make_pair(t.variable, p));
			return p;
		}

		case WorkingTree::TT_Function:
		case WorkingTree::TT_Operator:
			p = new Pattern(TT_Function);
			p->function = t.function;
			break;
			
		case WorkingTree::TT_Sum:     p = new Pattern(TT_Sum);     break;
		case WorkingTree::TT_Product: p = new Pattern(TT_Product); break;
	}
	assert(p);
	
	for (auto &c : t)
	{
		Pattern *cp = load(replacement, c, types, vars);
		if (!cp){ delete p; return NULL; }
		p->add_child(cp);
	}
	return p;
}

//----------------------------------------------------------------------------------------------------------------------
// Rule
//----------------------------------------------------------------------------------------------------------------------

#ifdef _WINDOWS
__declspec(noreturn)
#endif
static inline void syntax_error(const std::string &s)
{
	throw std::runtime_error(format("Invalid rule: %s", s.c_str()));
}

Rule::Rule(RootNamespace &rns, const std::string &s) : rns(rns), pattern(NULL), replacement(NULL)
{
	// syntax: "f(foo(x,z),x) = g(z), x > 0, y int, a,b,c real"
	size_t eq = s.find('=');  if (eq == std::string::npos) syntax_error(s);
	size_t cp = s.rfind(')'); cp = s.find(',', cp == std::string::npos ? 0 : cp+1);
	
	//------------------------------------------------------------------------------------------------------------------
	// setup vars
	//------------------------------------------------------------------------------------------------------------------
	Namespace vars; vars.link(&rns);
	std::map<const Variable *, Range> types;
	if (cp == std::string::npos)
	{
		Variable *v;
		vars.add(v = new Variable("x",  true));  types[v] = R_Real;
		vars.add(v = new Variable("x1", true));  types[v] = R_Real;
		vars.add(v = new Variable("x2", true));  types[v] = R_Real;
		vars.add(v = new Variable("y",  true));  types[v] = R_Real;
		vars.add(v = new Variable("y1", true));  types[v] = R_Real;
		vars.add(v = new Variable("y2", true));  types[v] = R_Real;
		vars.add(v = new Variable("z",  false)); types[v] = R_Complex;
		vars.add(v = new Variable("w",  false)); types[v] = R_Complex;
		vars.add(v = new Variable("z1", false)); types[v] = R_Complex;
		vars.add(v = new Variable("z2", false)); types[v] = R_Complex;
		vars.add(v = new Variable("n",  true));  types[v] = R_Integer;
		vars.add(v = new Variable("k",  true));  types[v] = R_Integer;
		vars.add(v = new Variable("k1", true));  types[v] = R_Integer;
		vars.add(v = new Variable("k2", true));  types[v] = R_Integer;
	}
	else
	{
		size_t l = s.length(), i = cp+1;
		while (i < l)
		{
			while (i < l && isspace(s[i])) ++i;
			std::vector<std::string> names;
			while (true)
			{
				size_t i0 = i;
				while (i < l && !isspace(s[i]) && s[i] != ',' && s[i] != '>') ++i;
				if (i == i0) syntax_error(s);
				names.push_back(s.substr(i0, i-i0));
				while (i < l && isspace(s[i])) ++i;
				if (i >= l) syntax_error(s);
				if (s[i] != ',') break;
			}
			Range r;
			const char *d = s.c_str() + i;
			if (s[i] == '>')
			{
				bool gte = false;
				if (++i >= l) syntax_error(s);
				if (s[i] == '='){ gte = true; ++i; }
				while (i < l && isspace(s[i])) ++i;
				if (i >= l || s[i] != '0') syntax_error(s);
				++i;
				r = gte ? R_NonNegative : R_Positive;
			}
			#define TST(t, T) else if (0 == strncasecmp(t, d, strlen(t))){ i += strlen(t); r = (T); }
			TST("real",      R_Real)
			TST("complex",   R_Complex)
			TST("imaginary", R_Imag)
			TST("imag",      R_Imag)
			TST("disc",      R_Unit)
			TST("unit",      R_Interval)
			TST("integer",   R_Integer)
			TST("int",       R_Integer)
			TST("natural",   R_Integer|R_NonNegative)
			#undef TST
			else syntax_error(s);
			assert(!names.empty());
			
			Variable *v;
			for (auto &n : names){ vars.add(v = new Variable(n,  false)); types[v] = r; }
			while (i < l && isspace(s[i])) ++i;
			if (i < l && s[i] != ',') syntax_error(s);
			++i;
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// parse pattern and replacement, convert both from WorkingTree to Pattern
	//------------------------------------------------------------------------------------------------------------------

	ParsingResult info;
	ParsingTree *c = ParsingTree::parse(s.substr(0, eq), vars, info);
	if (!c) syntax_error(s);
	assert(info.ok);
	
	std::map<const Variable*, Pattern*> vm;
	bool change = false;

	WorkingTree w1(*c, rns); delete c;
	w1.normalize(); w1.flatten(change); assert(w1.verify());
	pattern = Pattern::load(false, w1, types, vm);
	if (!pattern) syntax_error(s);

	c = ParsingTree::parse(s.substr(eq+1, cp == std::string::npos ? cp : cp-eq-1), vars, info);
	if (!c){ delete pattern; pattern = NULL; syntax_error(s); }
	assert(info.ok);
	
	WorkingTree w2(*c, rns); delete c;
	w2.normalize(); w2.flatten(change); assert(w2.verify());

	replacement = Pattern::load(true, w2, types, vm);
	
	if (!replacement){ delete pattern; pattern = NULL; syntax_error(s); }
	
	pattern->retain();
	replacement->retain();
	
	pattern->collect_elements(es, rns);
}

bool Rule::transform(WorkingTree &t) const
{
	Pattern::Bindings b;
	if (!pattern->matches(t, b)) return false;
	t = replacement->apply(b, rns);
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
// loading rule files
//----------------------------------------------------------------------------------------------------------------------

void load(const std::string &path, std::vector<Rule> &rules, RootNamespace &rns)
{
	std::ifstream f(path);
	if (!f.good()) return;
	
	std::string line;
	while (std::getline(f, line))
	{
		size_t i = 0, l = line.length();
		while (i < l && isspace(line[i])) ++i;
		size_t h = line.find('#', i);
		if (i >= l || h != std::string::npos && i >= h) continue;
		if (h != std::string::npos) line = line.substr(i, h-i);
		try
		{
			rules.emplace_back(rns, line);
		}
		catch(...)
		{
			assert(false);
		}
	}
}
