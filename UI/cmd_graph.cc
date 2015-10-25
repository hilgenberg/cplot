#include "cmd_base.h"

static bool parse_type(const std::string &s0, int &type)
{
	const char *s = s0.c_str();
	char X, Y, n, m; // X^n --> Y^m

	// Syntax:
	// (R|C|S)[[^]\d] [->] (R|C|S)[[^]\d]
	// or R[^k]|C for R^k->R | C->C
	while (isspace(*s)) ++s;
	if (*s!='R' && *s!='C' && *s!='S') return false;
	X = *s++;
	while (isspace(*s)) ++s;
	if (*s=='^')
	{
		++s;
		while (isspace(*s)) ++s;
		if (!isdigit(*s)) return false;
		n = *s++ - '0';
	}
	else if (isdigit(*s))
	{
		n = *s++ - '0';
	}
	else
	{
		n = 1;
	}
	while (isspace(*s)) ++s;
	bool arrow = false;
	if (*s == '-'){ ++s; arrow=true; while (*s == '-') ++s; if (*s != '>') return false; ++s; }
	while (isspace(*s)) ++s;

	if (*s!='R' && *s!='C' && *s!='S')
	{
		if (arrow) return false;
		Y = X; m = 1;
	}
	else
	{
		Y = *s++;
		while (isspace(*s)) ++s;
		if (*s=='^')
		{
			++s;
			while (isspace(*s)) ++s;
			if (!isdigit(*s)) return false;
			m = *s++ - '0';
		}
		else if (isdigit(*s))
		{
			m = *s++ - '0';
		}
		else
		{
			m = 1;
		}
	}
	while (isspace(*s)) ++s;
	if (*s) return false;

	const char *D[] = {"R1R1","R2R1","C1C1", "R1R2","R1R3","S1R2","S1R3", "R2R3","S2R3", "R3R1","R2R2","R3R3", NULL};
	int         T[] = { R_R  , R2_R , C_C  ,  R_R2 , R_R3 , S1_R2, S1_R3,  R2_R3, S2_R3,  R3_R , R2_R2, R3_R3,  -1 };

	for (int i = 0; D[i]; ++i)
	{
		const char *d = D[i];
		if (X==d[0] && Y==d[2] && n+'0' == d[1] && m+'0' == d[3])
		{
			type = T[i];
			return true;
		}
	}

	throw error("Graph type not supported", s0);
}

static bool parse_coord(const std::string &s, int &coord)
{
	if (s == "cart" || s == "cartesian") coord = GC_Cartesian;
	else if (s == "polar") coord = GC_Polar;
	else if (s == "sph" || s == "spherical") coord = GC_Spherical;
	else if (s == "cyl" || s == "cylindrical") coord = GC_Cylindrical;
	else return false;
	return true;
}

static bool parse_mode(const std::string &s, int &mode)
{
	if (s == "graph") mode = GM_Graph;
	else if (s == "image") mode = GM_Image;
	else if (s == "riemann") mode = GM_Riemann;
	else if (s == "vf") mode = GM_VF;
	else if (s == "re") mode = GM_Re;
	else if (s == "im") mode = GM_Im;
	else if (s == "abs") mode = GM_Abs;
	else if (s == "phase") mode = GM_Phase;
	else if (s == "zero" || s == "implicit") mode = GM_Implicit;
	else if (s == "color") mode = GM_Color;
	else if (s == "rcolor") mode = GM_RiemannColor;
	else if (s == "histogram" || s=="hist" || s=="histo") mode = GM_Histogram;
	else return false;
	return true;
}

static bool graph(const std::vector<std::string> &args)
{
	auto i = args.begin();
	
	bool add = false;
	int type = -1, mode = -1, coord = -1;
	while (i != args.end())
	{
		if (*i == "--")
		{
			++i;
			break;
		}
		else if (!add && (*i == "add" || *i == "new"))
		{
			++i;
			add = true;
		}
		else if (type  < 0 && parse_type (*i, type) ||
		         coord < 0 && parse_coord(*i, coord) ||
		         mode  < 0 && parse_mode (*i, mode))
		{
			++i;
		}
		else break;
	}

	std::vector<Argument> A;
	A.emplace_back((int)add);
	A.emplace_back(type);
	A.emplace_back(coord);
	A.emplace_back(mode);

	// remaining args are the plot expressions
	// they can contain unquoted whitespace, so just concat everything (separator is ';')
	std::string f;
	int nf = 0;
	for (; i != args.end(); ++i)
	{
		const std::string &s = *i;
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
				A.emplace_back(std::move(f)); ++nf;
				f = "";
				i0 = i1+1;
			}
		}
	}
	if (!f.empty())
	{
		A.emplace_back(std::move(f)); ++nf;
	}
	if (nf > 3) return false;

	// remove outer whitespace from the functions
	for (int i = 0; i < nf; ++i)
	{
		auto &s = A[A.size()-nf+i].s;
		size_t n = s.length(), k = 0;
		while (k < n && isspace(s[n-1-k])) ++k;
		if (k) s.erase(n -= k);
		k = 0;
		while (k < n && isspace(s[k])) ++k;
		if (k) s.erase(0, k);
	}

	cmd.send(CID::GRAPH, std::move(A));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_graph("graph", "g", CID::GRAPH, graph, "graph [new|add] [domain->range] [re|im|abs|asre|asim] [cart|polar|sph|cyl] [--] [f1|- [; f2|- [; f3|-]]]",
"Changes or prints the modes and expressions for the selected graph.");

//---------------------------------------------------------------------------------------------

extern void cmd_ls(const Graph &g, int i, bool selected);

void cmd_graph(PlotWindow &w, Command &cmd)
{
	Graph *cg = w.plot.current_graph();
	if (!cg) throw std::runtime_error("No graph selected");
	Graph &g = *cg;

	int i = 0, na = (int)cmd.args.size();
	bool add = (bool)cmd.get_int(i++);
	int gt = cmd.get_int(i++);
	int gc = cmd.get_int(i++);
	int gm = cmd.get_int(i++);

	if (!add && gt < 0 && gc < 0 && gm < 0 && i == na)
	{
		cmd_ls(g, w.plot.current_graph_index(), false /*it is current, but don't print it here*/);
		return;
	}

	if (add)
	{
		printf("todo...\n");
		return;
	}
	if (gt >= 0) g.type((GraphType)gt);
	if (gc >= 0) g.coords((GraphCoords)gc);
	if (gm >= 0) g.mode((GraphMode)gm);

	//int nc = g.n_components(), nf = na-i;
	//if (nc < 0 || na > nc) throw std::runtime_error(format("Graph only has %d components (%d given)", nc, na));
	std::string f1, f2, f3;
	if (i < na) f1 = cmd.get_str(i++);
	if (i < na) f2 = cmd.get_str(i++);
	if (i < na) f3 = cmd.get_str(i++);

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

