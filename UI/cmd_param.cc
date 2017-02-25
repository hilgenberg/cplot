#include "cmd_base.h"

static bool param(const std::vector<std::string> &args)
{
	std::vector<Argument> a;
	auto i = args.begin();
	if (i == args.end())
	{
		cmd.send(CID::LS, (int)'p');
		return true;
	}
	a.emplace_back(*i++); // parameter name
	int t = -1;
	if (i != args.end() && i->length() == 1)
	{
		switch (toupper((*i)[0]))
		{
			case 'R': t = Real; break;
			case 'C': t = Complex; break;
			case 'Z': t = Integer; break;
			case 'A': t = Angle; break;
			case 'S': t = ComplexAngle; break;
		}
		if (t >= 0) ++i;
	}
	a.emplace_back(t);
	if (i != args.end())
	{
		std::string val(*i++);
		while (i != args.end())
		{
			val += ' ';
			val += *i++;
		}
		a.emplace_back(val); // value
	}
	cmd.send(CID::PARAM, std::move(a));
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_param("param", "p", CID::PARAM, param, "param [name] [R|C|Z|S|A] [value]",
"Lists, prints, adds, or changes parameters.");

//---------------------------------------------------------------------------------------------

extern void cmd_ls(const Parameter &p, const RootNamespace &rns, const std::set<Parameter*> used);

void cmd_param(PlotWindow &w, Command &cmd)
{
	auto &pn = cmd.get_str(0);
	int type = cmd.get_int(1);
	bool set = cmd.args.size() > 2;
	Element *e = w.rns.find(pn, -1, false);
	Parameter *p = (e && e->isParameter()) ? (Parameter*)e : NULL;
	
	cnum v; bool radians = true;
	if (set)
	{
		auto &vs = cmd.get_str(2, true);
		v = evaluate(vs, w.rns);
		if (!defined(v)) throw error("Parsing value failed", vs);
		radians = (vs.find("°") == std::string::npos);
	}
		
	if (p)
	{
		if (!set && type < 0)
		{
			std::set<Parameter*> used = w.plot.used_parameters();
			cmd_ls(*p, w.rns, used);
			printf("\n");
			return;
		}
		
		if (type >= 0) p->type((ParameterType)type);
		
		if (set)
		{
			p->value(v);
			if (type == Angle) p->angle_in_radians(radians);
		}
		
		w.plot.recalc(p);
	}
	else
	{
		if (!set && type < 0) throw error("Parameter not found", pn);
		if (e) throw error("Name is reserved", pn);
		
		if (set)
		{
			if (type < 0) type = is_real(v) ? Real : Complex;
			p = new Parameter(pn, (ParameterType)type, v);
			if (type == Angle) p->angle_in_radians(radians);
		}
		else
		{
			p = new Parameter(pn, (ParameterType)type);
		}

		w.rns.add(p);
		w.plot.reparse(pn);
	}
	w.redraw();
}

