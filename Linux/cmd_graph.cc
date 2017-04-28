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
	if      (s == "cart" || s == "cartesian")  coord = GC_Cartesian;
	else if (s == "polar")                     coord = GC_Polar;
	else if (s == "sph" || s == "spherical")   coord = GC_Spherical;
	else if (s == "cyl" || s == "cylindrical") coord = GC_Cylindrical;
	else return false;
	return true;
}

static bool parse_mode(const std::string &s, int &mode)
{
	if      (s == "graph")    mode = GM_Graph;
	else if (s == "image")    mode = GM_Image;
	else if (s == "riemann")  mode = GM_Riemann;
	else if (s == "vf")       mode = GM_VF;
	else if (s == "re")       mode = GM_Re;
	else if (s == "im")       mode = GM_Im;
	else if (s == "abs")      mode = GM_Abs;
	else if (s == "phase")    mode = GM_Phase;
	else if (s == "zero" || 
	         s == "implicit") mode = GM_Implicit;
	else if (s == "color")    mode = GM_Color;
	else if (s == "rcolor")   mode = GM_RiemannColor;
	else if (s == "histogram" ||
	         s == "hist" ||
	         s == "histo")    mode = GM_Histogram;
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
		else if (!add && *i == "add")
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

const char *GRAPH_ARGS[] = {
	"add", "--", "R->R", "C->C", "R->R2", "R->R3", "R2->R", "R3->R",
	"S1->R2", "S1->R3", "R2->R3", "S2->R3", "R2->R2", "R3->R3",
	"graph", "image", "riemann", "vf", "re", "im", "abs", "phase",
	"implicit", "color", "rcolor", "histogram",
	"cartesian", "polar", "spherical", "cylindrical", NULL};

CommandInfo ci_graph("graph", "g", CID::GRAPH, graph, "graph [add] [domain->range] "
	"[graph|image|riemann|vf|re|im|abs|phase|implicit|color|rcolor|histogram] "
	"[cartesian|polar|spherical|cylindrical] "
	"[--] [f1|- [; f2|- [; f3|-]]]",
"Changes or prints the modes and expressions for the selected or a new graph.");

//---------------------------------------------------------------------------------------------

extern void cmd_ls(const Graph &g, int i, bool selected);

void cmd_graph(PlotWindow &w, Command &cmd)
{
	Graph *cg = w.plot.current_graph();

	int i = 0, na = (int)cmd.args.size();
	bool add = (bool)cmd.get_int(i++);
	int gt = cmd.get_int(i++);
	int gc = cmd.get_int(i++);
	int gm = cmd.get_int(i++);

	int nf = na-i; assert(nf <= 3);
	std::string f1, f2, f3;
	if (i < na){ f1 = cmd.get_str(i++); if (f1 == "-") f1.clear(); }
	if (i < na){ f2 = cmd.get_str(i++); if (f2 == "-") f2.clear(); }
	if (i < na){ f3 = cmd.get_str(i++); if (f3 == "-") f3.clear(); }

	if (!add && gt < 0 && gc < 0 && gm < 0 && !nf)
	{
		if (cg)
		{
			cmd_ls(*cg, w.plot.current_graph_index(), false /*it is current, but don't print it here*/);
		}
		else
		{
			printf("No graph selected.\n");
		}
		return;
	}

	if (add || !cg)
	{
		bool init_all = !cg; // otherwise it's a copy of cg
		w.plot.add_graph();
		cg = w.plot.current_graph();
		if (!cg) throw std::runtime_error("Creating graph failed.");

		if (!cg->init(init_all, gt, gc, gm, nf, f1, f2, f3))
		{
			printf("Type, coordinates, mode and/or number of components don't match.\n");
		}
	}
	else
	{
		// TODO: use init here also
		if (gt >= 0) cg->type((GraphType)gt);
		if (gc >= 0) cg->coords((GraphCoords)gc);
		if (gm >= 0) cg->mode((GraphMode)gm);

		if (f1.empty()) f1 = cg->f1();
		if (f2.empty()) f2 = cg->f2();
		if (f3.empty()) f3 = cg->f3();
		cg->set(f1, f2, f3);
	}

	Graph &g = *cg;
	int nc = g.n_components();
	if (nf && nc > nf) printf("Graph has %d components (%d given). Remaining components left unchanged.\n", nc, nf);
	if (nc < nf) printf("Graph has %d components (%d given). Remaining components will be set but need type change to become effective.\n", nc, nf);
	
	if (!g.isValid())
	{
		Expression *x = g.expression();
		if (!x) throw std::logic_error("NULL expression (this should not happen)");
		const ParsingResult &r = x->result();
		r.print(*x);
	}

	w.plot.update_axis();
	w.redraw();

	if (w.plot.number_of_visible_graphs() > 0 && !w.plot.axis.valid())
	{
		printf("Error: Axis type mismatch.\n");
	}
}

