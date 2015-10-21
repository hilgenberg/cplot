#include "cmd_base.h"

static bool graph(const std::vector<std::string> &args)
{
	std::vector<Argument> A;
	std::string f;
	for (auto &s : args)
	{
		if (s.empty()) continue;
		if (!f.empty()) f += ' ';
		size_t i0 = 0, n = s.length();
		while (i0 < n)
		{
			size_t i1 = s.find(';', i0);
			if (i1 == std::string::npos)
			{
				f += s.substr(i0);
				break;
			}
			else
			{
				f += s.substr(i0, i1-i0);
				A.emplace_back(std::move(f));
				f = "";
				i0 = i1+1;
			}
		}
	}
	if (!f.empty()) A.emplace_back(std::move(f));
	if (A.size() > 3) return false;
	for (auto &a : A)
	{
		size_t n = a.s.length(), k = 0;
		while (k < n && isspace(a.s[n-1-k])) ++k;
		if (k) a.s.erase(n -= k);
		k = 0;
		while (k < n && isspace(a.s[k])) ++k;
		if (k) a.s.erase(0, k);
		if (a.s.empty()) a.s = "-";//return false;
	}

	cmd.send(CID::GRAPH, std::move(A));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_graph("graph", "g", CID::GRAPH, graph, "graph [f1|- [; f2|- [; f3|-]]]",
"Changes or prints the plotted expressions for the selected graph.");

//---------------------------------------------------------------------------------------------

extern void cmd_ls(const Graph &g, int i, bool selected);

void cmd_graph(PlotWindow &w, Command &cmd)
{
	Graph *cg = w.plot.current_graph();
	if (!cg) throw std::runtime_error("No graph selected");

	Graph &g = *cg;
	if (cmd.args.empty())
	{
		cmd_ls(g, w.plot.current_graph_index(), false /*just don't print that*/);
		return;
	}
	int nc = g.n_components(), na = (int)cmd.args.size();
	if (nc < 0 || na > nc) throw std::runtime_error(format("Graph only has %d components (%d given)", nc, na));
	std::string f1 = cmd.get_str(0), f2, f3;
	if (na > 1) f2 = cmd.get_str(1);
	if (na > 2) f3 = cmd.get_str(2, true);

	if (f1.empty() || f1 == "-") f1 = g.f1();
	if (f2.empty() || f2 == "-") f2 = g.f2();
	if (f3.empty() || f3 == "-") f3 = g.f3();
	
	g.set(f1, f2, f3);
	if (!g.isValid())
	{
		printf("Graph updated, but there were parsing errors:\n");
		Expression *x = g.expression();
		if (!x) throw std::logic_error("NULL expression (this should not happen)");
		const ParsingResult &r = x->result();
		printf("%s. At this point:\n", r.info.c_str());
		std::string &fe = (r.index==0 ? f1 : r.index==1 ? f2 : f3);
		printf("%s\n%*s", fe.c_str(), (int)r.pos, "");
		if (r.len == 0)
		{
			printf("^--\n");
		}
		else
		{
			for (size_t i = 0; i < r.len; ++i) putchar('~');
			putchar('\n');
		}
	}

	w.plot.update_axis();
	w.redraw();

	if (w.plot.number_of_visible_graphs() > 0 && !w.plot.axis.valid())
	{
		printf("Error: Axis type mismatch.\n");
	}
}

