#include "cmd_base.h"

void cmd_get(PlotWindow &w, Command &cmd)
{
	if (cmd.args.empty())
	{
		cmd.error("get: invalid arguments");
		assert(false);
		return;
	}
	Argument arg = cmd.args[0];

	if (arg.type != Argument::I)
	{
		cmd.error("get: invalid argument type");
		return;
	}
	GET what = (GET)arg.i;

	std::vector<Argument> args;
	std::swap(args, cmd.args);
	cmd.cid = CID::RETURN;

	switch (what)
	{
		case GET::PARAMETER_NAMES:
		case GET::USED_PARAMETER_NAMES:
		{
			bool all = (what == GET::PARAMETER_NAMES);
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
		case GET::DEFINITION_NAMES:
		{
			for (Element *e : w.rns)
			{
				if (!e->isFunction() || ((Function*)e)->base()) continue;
				UserFunction &f = *(UserFunction*)e;
				std::string nm = f.name();
				nm += '(';
				auto &V = f.arguments();
				for (int i = 0, n = f.arity(); i < n; ++i)
				{
					if (i > 0) nm += ',';
					if (n == (int)V.size())
					{
						nm += V[i]->name();
					}
					else
					{
						nm += format("x%d",i+1);
					}
				}
				nm += ')';
				cmd.args.push_back(nm);
			}
			break;
		}

		case GET::PROPERTY_NAMES:
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

		case GET::PROPERTY_VALUES:
		{
			if (args.size() != 2 || args[1].type != Argument::S)
			{
				cmd.error("get: invalid property name");
				assert(false);
				return;
			}
			std::string &n = args[1].s;
			auto i = w.plot.properties().find(n);
			if (i == w.plot.properties().end())
			{
				const Graph *cg = w.plot.current_graph();
				if (cg)
				{
					i = cg->properties().find(n);
					if (i == cg->properties().end())
					{
						cmd.error("get: property name not found");
						return;
					}
				}
			}
			Property &p = i->second;
			if (!p.values)
			{
				cmd.error("get: not enum type");
				return;
			}
			auto V = p.values();
			for (auto &v : V) cmd.args.push_back(v);
			break;
		}

		case GET::GRAPH_COUNT:
			cmd.args.push_back((int)w.plot.number_of_graphs());
			break;
			
		case GET::CURRENT_GRAPH_EXPRESSIONS:
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

