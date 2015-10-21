#include "cmd_base.h"

static bool parse_set(const std::vector<std::string> &args)
{
	int na = (int)args.size();
	if (na < 1) return false;
	std::vector<Argument> a;
	a.emplace_back(args[0]); // property name
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

CommandInfo ci_set("set", NULL, CID::SET, parse_set, "set <property> [value]",
"Changes or prints plot and graph settings.");

//---------------------------------------------------------------------------------------------

void cmd_set(PlotWindow &w, Command &cmd)
{
	auto &name = cmd.get_str(0);
	auto &P = w.plot.properties();
	Property *p = NULL;
	auto i = P.find(name);
	if (i != P.end()) p = &i->second;
	else if (w.plot.current_graph())
	{
		auto &Q = w.plot.current_graph()->properties();
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

