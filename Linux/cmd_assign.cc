#include "cmd_base.h"

static bool assign(const std::vector<std::string> &args)
{
	assert(args.size() == 1);
	if (args.size() != 1) return false;

	const char *s = args[0].c_str();
	while (isspace(*s)) ++s;
	const char *s0 = s;
	while (*s && !isspace(*s) && *s != '=' && *s != '(' && *s != ')') ++s;
	if (s == s0) return false;
	std::string name(s0, s);
	while (isspace(*s)) ++s;
	int na = -1;
	if (*s == '(')
	{
		++s;
		while (isspace(*s)) ++s;
		na = (*s == ')' ? 0 : 1);
		int pl = 1;
		while (pl && *s)
		{
			char c = *s++;
			switch (c)
			{
				case '(': ++pl; break;
				case ')': --pl; break;
				case ',': ++na; break;
			}
		}
		if (pl) return false;
		while (isspace(*s)) ++s;
	}
	
	if (*s != '=') return false;

	++s;
	while (isspace(*s)) ++s;
	if (!*s) return false;

	std::vector<Argument> a;
	a.emplace_back(name);
	a.emplace_back(na);
	a.emplace_back(na < 0 ? s : s0);

	cmd.send(CID::ASSIGN, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

// the spaces in the name prohibit "= a b" from parsing
CommandInfo ci_assign(" = ", NULL, CID::ASSIGN, assign, "<item> = <value>",
"Changes definitions, parameter and property values.");

//---------------------------------------------------------------------------------------------

void cmd_assign(PlotWindow &w, Command &cmd)
{
	std::string &name = cmd.get_str(0);
	int          na   = cmd.get_int(1);
	std::string &val  = cmd.get_str(2, true);

	Element *e = w.rns.find(name, na, false);
	if (na >= 0)
	{
		UserFunction *f;
		if (!e || !e->isFunction() || ((Function*)e)->base())
		{
			f = new UserFunction;
			f->formula(val);
			w.rns.add(f);
		}
		else
		{
			f = (UserFunction*)e;
			f->formula(val);
		}
		Expression *ex = f->expression();
		if (!ex)
		{
			printf("Syntax error: Expected <name>(x1,..,xn) = <definition>\n");
		}
		else if (!ex->valid())
		{
			const ParsingResult &r = ex->result();
			if (!r.ok)
			{
				r.print(*ex);
			}
			else
			{
				printf("Syntax error");
			}
		}
		w.plot.reparse(name);
	}
	else if (e && e->isParameter())
	{
		Parameter &p = *(Parameter*)e;
		cnum v = evaluate(val, w.rns);
		if (!defined(v)) throw error("Parsing vavlue failed", val);
		p.value(v);
		w.plot.recalc(&p);
	}
	else
	{
		auto *cg = w.plot.current_graph();
		Property *p = NULL;
		auto &P = w.plot.properties();
		auto i = P.find(name);
		if (i != P.end()) p = &i->second;
		else if (cg)
		{
			auto &Q = cg->properties();
			i = Q.find(name);
			if (i != Q.end()) p = &i->second;
		}
		if (!p) throw error("Item not found", name);
		if (!p->set) throw error("Property not writeable", name);
		p->set(val);
	}
	w.redraw();
}

