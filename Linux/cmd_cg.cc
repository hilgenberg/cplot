#include "cmd_base.h"

static bool cg(const std::vector<std::string> &args)
{
	if (args.size() > 1) return false;
	std::vector<Argument> a;
	if (!args.empty())
	{
		int i;
		if (!is_int(args[0], i)) return false;
		a.emplace_back(i);
	}
	cmd.send(CID::CG, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_select("cg", NULL, CID::CG, cg, "cg [graph_number] | graph_number",
"Sets or prints the index of the currently selected graph.");

//---------------------------------------------------------------------------------------------

void cmd_cg(PlotWindow &w, Command &cmd)
{
	int n = w.plot.number_of_graphs();
	if (cmd.args.empty())
	{
		int i = w.plot.current_graph_index();
		if (!n) i = -1;
		printf("Current graph: [%d]%s\n", i, (n ? "" : " (plot is empty)"));
	}
	else
	{
		int i = cmd.get_int(0, true);
		if (i < 0) i = n+i;
		if (!n)
		{
			printf("Invalid index (plot is empty)");
		
		}
		else if (i < 0 || i >= n)
		{
			printf("Invalid index (%d .. %d are valid)", -n, n-1);
		}
		else
		{
			w.plot.set_current_graph(i);
			printf("Current graph: [%d]\n", i);
		}
	}
}

