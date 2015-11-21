#include "cmd_base.h"

static bool parse_ls(const std::vector<std::string> &args)
{
	#define N 7
	const char *S[N]   = { "all", "builtins", "variables", "constants", "functions", "parameters", "graphs" };
	const char *H[N-1] = { "Builtin Functions", "Variables", "Constants", "Function Definitions", "Parameters", "Graphs" };
	bool f[N] = { false };
	if (args.empty()) f[4] = f[5] = f[6] = true;
	else for (auto &s_ : args)
	{
		const char *s = s_.c_str();
		bool found = false;
		for (int i = 0; i < N && !found; ++i) if (strcasecmp(s, S[i]) == 0) f[i] = found = true;
		if (found) continue;
		for (int j = 0, n = s_.length(); j < n; ++j)
		{
			found = false;
			for (int i = 0; i < N && !found; ++i)
			{
				if (tolower(s[j]) == S[i][0]) f[i] = found = true;
			}
			if (!found)
			{
				printf("Invalid argument: %s\n", s);
				return false;
			}
		}
	}
	int nf = 0; for (int i = 1; i < N; ++i) if (f[i]) ++nf;
	bool headers = (f[0] || nf > 1);
	for (int i = 1; i < N; ++i)
	{
		if (!f[0] && !f[i]) continue;
		if (headers) printf("\n[%s]\n", H[i-1]);
		cmd.send(CID::LS, S[i][0]);
	}
	#undef N
	return true;
}

//--------------------------------------------------------------------------------------------

const char *LS_ARGS[] = {"all", "functions", "parameters", "graphs", "constants", "variables", "builtins", NULL};

CommandInfo ci_ls("ls", "list", CID::LS, parse_ls,
"ls [all | functions | parameters | graphs | constants | variables | builtins | <afpgcvb>]", "Lists the matching items.");

//--------------------------------------------------------------------------------------------

void cmd_ls(const Parameter &p, const RootNamespace &rns, const std::set<Parameter*> used)
{
	bool real = false;
	switch (p.type())
	{
		case         Real: printf("[R]"); real = true; break;
		case      Complex: printf("[C]"); break;
		case        Angle: printf("[A]"); real = true; break;
		case ComplexAngle: printf("[S]"); break;
		case      Integer: printf("[Z]"); real = true; break;
	}
	const char *n = p.name().c_str();
	printf(" %s = ", n);
	if (p.type() == Angle && !p.angle_in_radians())
	{
		printf("%.2g°", p.rvalue()/DEGREES);
	}
	else
	{
		printf("%s", to_string(p.value(), rns).c_str());
	}
	double r1 = p.min(), r2 = p.max(), i1 = p.imin(), i2 = p.imax(), r = p.rmax();
	if (real)
	{
		if (!isnan(r1) && !isnan(r2)) printf(", %g < %s < %g", r1, n, r2);
		else if (!isnan(r1)) printf(", %s > %g", n, r1);
		else if (!isnan(r2)) printf(", %s < %g", n, r2);
	}
	else
	{
		if (!isnan(r1) && !isnan(r2)) printf(", %g < re(%s) < %g", r1, n, r2);
		else if (!isnan(r1)) printf(", re(%s) > %g", n, r1);
		else if (!isnan(r2)) printf(", re(%s) < %g", n, r2);
	
		if (!isnan(i1) && !isnan(i2)) printf(", %g < im(%s) < %g", i1, n, i2);
		else if (!isnan(i1)) printf(", im(%s) > %g", n, i1);
		else if (!isnan(i2)) printf(", im(%s) < %g", n, i2);
		
		if (!isnan(r)) printf(", |%s| < %g", n, r);
	}
	if (!used.count((Parameter*)&p)) printf(" (unused)");
}

void cmd_ls(const Graph &g, int i, bool selected)
{
	if (i >= 0)
	{
		printf("[%d] ", i);
		if (selected) printf("*selected* ");
		if (g.options.hidden) printf("hidden ");
		if (!g.isValid()) printf("errors ");
	}
	switch (g.type())
	{
		case  R_R:  printf("R -> R"); break;
		case R2_R:  printf("R^2 -> R"); break;
		case R3_R:  printf("R^3 -> R"); break;
		case  C_C:  printf("C -> C"); break;
		case  R_R2: printf("R -> R^2"); break;
		case S1_R2: printf("S^1 -> R^2"); break;
		case R2_R2: printf("R^2 -> R^2"); break;
		case  R_R3: printf("R -> R^3"); break;
		case S1_R3: printf("S^1 -> R^3"); break;
		case R2_R3: printf("R^2 -> R^3"); break;
		case S2_R3: printf("S^2 -> R^3"); break;
		case R3_R3: printf("R^3 -> R^3"); break;
	}
	printf("  (");
	switch (g.coords())
	{
		case GC_Cartesian:   printf("cartesian"); break;
		case GC_Polar:       printf("polar"); break;
		case GC_Spherical:   printf("spherical"); break;
		case GC_Cylindrical: printf("cylindrical"); break;
	}
	printf(")  mode: ");
	switch (g.mode())
	{
		case GM_Graph:   printf("Graph"); break;
		case GM_Image:   printf("Image"); break;
		case GM_Riemann:   printf("Riemann"); break;
		case GM_VF:   printf("Vector Field"); break;
		case GM_Re:   printf("Real Part"); break;
		case GM_Im:   printf("Imainary Part"); break;
		case GM_Abs:   printf("Absolute Value"); break;
		case GM_Phase:   printf("Phase"); break;
		case GM_Implicit:     printf("Implicit"); break;
		case GM_Color:        printf("Color"); break;
		case GM_RiemannColor: printf("Riemann color"); break;
		case GM_Histogram:    printf("Histogram"); break;
	}
	printf("\n");
	auto cmp = g.components();
	int nc = (int)cmp.size();
	if (nc >= 1) printf(" f1=f%s: %s\n", cmp[0].c_str(), g.f1().c_str());
	if (nc >= 2) printf(" f2=f%s: %s\n", cmp[1].c_str(), g.f2().c_str());
	if (nc >= 3) printf(" f3=f%s: %s\n", cmp[2].c_str(), g.f3().c_str());
}

void cmd_ls(PlotWindow &w, Command &cmd)
{
	int what = cmd.get_int(0, true);
	bool first = true;
	switch (what)
	{
		case 'b':
			for (Element *e : w.rns)
			{
				if (!e->isFunction() || !e->builtin()) continue;
				char c0 = e->name()[0];
				if (c0 == '(' || c0 == '_') continue;
				Function &f = *(Function*)e;
				
				if (!first) printf(", "); first = false;
				printf("%s(", f.name().c_str());
				for (int i = 0; i < f.arity(); ++i)
				{
					if (i > 0) printf(", ");
					printf("-");
				}
				printf(")");
			}
			printf("\n");
			break;

		case 'f':
			for (Element *e : w.rns)
			{
				if (!e->isFunction() || e->builtin()) continue;
				UserFunction &f = *(UserFunction*)e;
				printf("%s\n", f.formula().c_str());
			}
			break;

		case 'v':
		{
			Graph *g = w.plot.current_graph();
			if (!g){ printf("(no graph selected)\n"); break; }
			for (auto *v : g->plotvars())
			{
				if (!first) printf(", "); first = false;
				printf("%s", v->name().c_str());
			}
			if (!first) printf("\n");
			first = true;
			for (Element *e : g->internal_namespace())
			{
				if (!e->isAlias()) continue;
				printf("%s = %s\n", e->name().c_str(),
					((AliasVariable*)e)->replacement().to_string().c_str());
			}
			break;
		}

		case 'c':
			for (Element *e : w.rns)
			{
				if (!e->isConstant()) continue;
				Constant &c = *(Constant*)e;
				
				printf("%s = %s\n",
					c.name().c_str(),
					to_string(c.value()).c_str());
			}
			break;


		case 'p':
		{
			std::set<Parameter*> used = w.plot.used_parameters();
			for (Element *e : w.rns)
			{
				if (!e->isParameter()) continue;
				Parameter &p = *(Parameter*)e;
				cmd_ls(p, w.rns, used);
				printf("\n");
			}
			break;
		}

		case 'g':
		{
			int ng = w.plot.number_of_graphs();
			for (int i = 0; i < ng; ++i)
			{
				const Graph &g = *w.plot.graph(i);
				cmd_ls(g, i, ng > 1 && &g == w.plot.current_graph());
			}
			break;
		}

		default: throw std::logic_error("Invalid argument");
	}
}

