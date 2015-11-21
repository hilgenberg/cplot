#include "cmd_base.h"

static bool param(const std::vector<std::string> &args)
{
	int na = (int)args.size();
	if (na < 1) return false;
	std::vector<Argument> a;
	a.emplace_back(args[0]); // parameter name
	if (na > 1)
	{
		a.emplace_back(args[1]); // value
		for (int i = 2; i < na; ++i)
		{
			a[1].s += ' ';
			a[1].s += args[i];
		}
	}
	cmd.send(CID::PARAM, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_param("param", NULL, CID::PARAM, param, "param <name> [value]",
"Changes or prints the value of the named parameter.");

//---------------------------------------------------------------------------------------------

extern void cmd_ls(const Parameter &p, const RootNamespace &rns, const std::set<Parameter*> used);

void cmd_param(PlotWindow &w, Command &cmd)
{
	auto &pn = cmd.get_str(0);
	Element *e = w.rns.find(pn, -1, false);
	if (!e || !e->isParameter()) throw error("Parameter not found", pn);
	Parameter &p = *(Parameter*)e;

	if (cmd.args.size() == 1)
	{
		std::set<Parameter*> used = w.plot.used_parameters();
		cmd_ls(p, w.rns, used);
		printf("\n");
	}
	else
	{
		auto &vs = cmd.get_str(1, true);
		cnum v = evaluate(vs, w.rns);
		if (!defined(v)) throw error("Parsing value failed", vs);
		p.value(v);
		w.plot.recalc(&p);
		w.redraw();
	}
}

