#include "cmd_base.h"

static bool parse_set(const std::vector<std::string> &args)
{
	int na = (int)args.size();
	std::vector<Argument> a;
	if (na > 0) a.emplace_back(args[0]); // property name
	if (na > 1)
	{
		a.emplace_back(args[1]); // value
		for (int i = 2; i < na; ++i)
		{
			a[1].s += ' ';
			a[1].s += args[i];
		}
	}
	cmd.send(CID::SET, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_set("set", NULL, CID::SET, parse_set, "set [property] [value]",
"Lists, prints or changes plot and graph settings.");

//---------------------------------------------------------------------------------------------

void cmd_set(PlotWindow &w, Command &cmd)
{
	const Graph *cg = w.plot.current_graph();
	
	if (cmd.args.empty())
	{
		printf("[Plot Settings]\n");
		w.plot.print_properties();

		if (!cg) return;

		printf("\n[Graph Settings]\n");
		cg->print_properties();
		return;
	}

	auto &name = cmd.get_str(0);
	auto &P = w.plot.properties();
	Property *p = NULL;
	auto i = P.find(name);
	if (i != P.end()) p = &i->second;
	else if (cg)
	{
		auto &Q = cg->properties();
		i = Q.find(name);
		if (i != Q.end()) p = &i->second;
	}
	if (!p) throw error("Property not found", name);

	if (cmd.args.size() == 1)
	{
		if (!p->get) throw error("Property not readable", name);
		printf("%s = %s\n", name.c_str(), p->get().c_str());
	}
	else
	{
		if (!p->set) throw error("Property not writeable", name);
		p->set(cmd.get_str(1,true));
		w.redraw();
		if (!p->visible()) printf("property set (but not visible)\n");
	}
}

