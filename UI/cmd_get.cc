#include "cmd_base.h"

void cmd_get(PlotWindow &w, Command &cmd)
{
	if (cmd.args.size() != 1)
	{
		cmd.error("get: invalid arguments");
		return;
	}
	Argument arg = cmd.args[0];

	if (arg.type != Argument::I)
	{
		cmd.error("get: invalid argument type");
		return;
	}

	cmd.args.clear();
	cmd.cid = CID::RETURN;

	switch (arg.i)
	{
		case 'p': // used parameter names
		case 'P': // all parameter names
		{
			bool all = (arg.i == 'P');
			std::set<Parameter*> used = w.plot.used_parameters();
			for (Element *e : w.rns)
			{
				if (!e->isParameter()) continue;
				Parameter &p = *(Parameter*)e;
				if (!all && !used.count(&p)) continue;
				cmd.args.push_back(p.name());
			}
			break;
		}

		case '.': // property names
		{
			for (auto &i : w.plot.properties())
			{
				cmd.args.push_back(i.first);
			}
			const Graph *cg = w.plot.current_graph();
			if (cg) for (auto &i : cg->properties())
			{
				cmd.args.push_back(i.first);
			}
			break;
		}

		case '#': // graph count
			cmd.args.push_back((int)w.plot.number_of_graphs());
			break;
			
		case 'F': // current graph def
		{
			const Graph *cg = w.plot.current_graph();
			if (!cg){ cmd.error("get: no current graph"); return; }
			const Graph &g = *cg;
			int nc = g.n_components();
			if (nc>0) cmd.args.emplace_back(g.f1());
			if (nc>1) cmd.args.emplace_back(g.f2());
			if (nc>2) cmd.args.emplace_back(g.f3());
			break;
		}

		default: cmd.error("get: unknown argument"); return;
	}

	cmd.done();
}

