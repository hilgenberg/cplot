#include "cmd_base.h"

static bool rm(const std::vector<std::string> &args)
{
	int na = (int)args.size();
	if (na < 1) return false;
	std::vector<Argument> a;
	for (auto &S : args)
	{
		// graphs
		int idx;
		if (is_int(S, idx))
		{
			a.emplace_back(idx);
			continue;
		}
		// parameters
		const char *s = S.c_str();
		while (isspace(*s)) ++s;
		const char *s0 = s;
		while (*s && *s != '(') ++s;
		if (!*s)
		{
			a.emplace_back(S);
			a.emplace_back(-1);
			continue;
		}
		// definitions
		const char *s1 = s;
		while (s1 > s0 && isspace(s1[-1])) --s1;
		if (s1 == s0) return false;
		a.emplace_back(std::string(s0,s1));
		++s; while (isspace(*s)) ++s;
		if (*s == ')')
		{
			++s; while (isspace(*s)) ++s;
			if (*s) return false;
			a.emplace_back(0);
			continue;
		}
		int pl = 1, na = 1;
		while (pl && *s)
		{
			switch (*s++)
			{
				case '(': ++pl; break;
				case ')': --pl; if (pl < 0) return false; break;
				case ',': if (pl == 1) ++na; break;
			}
		}
		while (isspace(*s)) ++s;
		if (*s || pl) return false;
		a.emplace_back(na);
	}
	cmd.send(CID::RM, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_rm("rm", NULL, CID::RM, rm, "rm <item>...",
"Deletes graphs (by index: 0, 1, ...), parameters (by name), and definitions (by signature: f(), f(x,y), ...).");

//---------------------------------------------------------------------------------------------

void cmd_rm(PlotWindow &w, Command &cmd)
{
	std::set<Graph*> graphs;
	std::set<Parameter*> params;
	std::set<UserFunction*> defs;
	for (size_t i = 0, n = cmd.args.size(); i < n; ++i)
	{
		auto &a = cmd.args[i];
		if (a.type == Argument::I)
		{
			int i = a.i;
			if (i < 0 || i >= w.plot.number_of_graphs()) throw std::runtime_error("Invalid graph index");
			graphs.insert(w.plot.graph(i));
			continue;
		}
		std::string &name = cmd.get_str(i);
		int na = cmd.get_int(++i);
		Element *e = w.rns.find(name, na, false);
		if (na < 0)
		{
			if (!e || !e->isParameter()) throw error("Parameter not found", name);
			params.insert((Parameter*)e);
		}
		else
		{
			if (!e || !e->isFunction() || ((Function*)e)->base()) throw error("Definition not found", name);
			defs.insert((UserFunction*)e);
		}
	}

	for (auto *g : graphs)
	{
		w.plot.delete_graph(g);
	}
	for (auto *f : defs)
	{
		std::string nm = f->name();
		delete f;
		w.plot.reparse(nm);
	}
	for (auto *p : params)
	{
		std::string nm = p->name();
		delete p;
		w.plot.reparse(nm);
	}

	w.plot.update_axis();
	w.redraw();
}

