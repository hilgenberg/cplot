#include "cmd_base.h"

// case insensitive and skips whitespace after the prefix
// l will be total length of the token (prefix + whitespace)
// returns true iff found.
static bool eat_prefix(const std::string &s, const char *p, size_t &l)
{
	l = strlen(p);
	if (s.length() < l) return false;
	if (strncasecmp(s.c_str(), p, l) != 0) return false;
	while (l < s.length() && isspace(s[l])) ++l;
	return true;
}


static bool animate(const std::vector<std::string> &args)
{
	int na = (int)args.size();
	if (na < 3) return false;
	std::vector<Argument> a;
	a.emplace_back(args[0]); // parameter name
	a.emplace_back(args[1]); // start value
	a.emplace_back(args[2]); // end value

	int i = 3;
	if (i < na) // check for dt
	{
		char c0 = args[i][0];
		if (isdigit(c0) || c0 == '.')
		{
			a.emplace_back(args[i++]);
		}
		else
		{
			a.emplace_back(1.0);
		}
	}
	else
	{
		a.emplace_back(1.0);
	}
	int reps = 1, type = 'p';
	if (i < na) // check for type
	{
		size_t l = 0;
		if      (eat_prefix(args[i], "linear", l)) type = 'l';
		else if (eat_prefix(args[i], "repeat", l)) type = 'r';
		else if (eat_prefix(args[i], "pingpong", l)) type = 'p';
		else if (eat_prefix(args[i], "ping-pong", l)) type = 'p';
		else if (eat_prefix(args[i], "sine", l)) type = 's';
		else if (eat_prefix(args[i], "l", l)) type = 'l';
		else if (eat_prefix(args[i], "r", l)) type = 'r';
		else if (eat_prefix(args[i], "p", l)) type = 'p';
		else if (eat_prefix(args[i], "s", l)) type = 's';

		if (l < args[i].length())
		{
			if (args[i][l] == '*' && l+1 == args[i].length())
			{
				reps = -1;
			}
			else if (!is_int(args[i].substr(l), reps) || i+1 != na)
			{
				return false;
			}
		}
		++i;
	}
	if (i < na)
	{
		if (args[i] == "*") reps = -1;
		else if (!is_int(args[i], reps)) return false;
		++i;
	}
	if (i != na) return false;
	if (reps == 0 || args[1] == args[2]) return true;
	a.emplace_back(type);
	a.emplace_back(reps);

	assert(a.size() == 6);
	cmd.send(CID::ANIM, std::move(a));
	return true;
}

static bool stop(const std::vector<std::string> &args)
{
	std::vector<Argument> a;
	for (auto &arg : args) a.emplace_back(arg); // parameter names
	cmd.send(CID::STOP, std::move(a));
	return true;
}

//--------------------------------------------------------------------------------------------

CommandInfo ci_anim("animate", "anim", CID::ANIM, animate,
"animate <parameter> <v0|'-'> v1 [dt] [repeat | linear | sine | pingpong | <rlsp>] [repetitions|*]",
"Animate parameter from v0 (or its current value) to v1 over dt seconds.");

CommandInfo ci_stop("stop", ".", CID::STOP, stop, "stop [parameter]", "Stops parameter animation.");

//---------------------------------------------------------------------------------------------

void cmd_anim(PlotWindow &w, Command &cmd)
{
	auto &pn = cmd.get_str(0);
	Element *e = w.rns.find(pn, -1, false);
	if (!e || !e->isParameter()) throw error("Parameter not found", pn);
	Parameter *p = (Parameter*)e;

	auto &v0s = cmd.get_str(1);
	cnum v0 = (v0s == "-" ? p->value() : evaluate(v0s, w.rns));
	if (!defined(v0)) throw error("Invalid initial value", v0s);
	auto &v1s = cmd.get_str(2);
	cnum v1 = (v1s == "-" ? p->value() : evaluate(v1s, w.rns));
	if (!defined(v1)) throw error("Invalid final value", v1s);

	auto &dts = cmd.get_arg(3);
	double dt;
	if (dts.type == Argument::D)
	{
		dt = dts.d;
	}
	else if (dts.type == Argument::S)
	{
		cnum v = evaluate(dts.s, w.rns);
		if (!defined(v)) throw error("Parsing dt value failed", dts.s);
		if (!is_real(v)) throw std::runtime_error("dt must be real");
		dt = v.real();
	}
	else
	{
		throw std::logic_error("Wrong argument type for dt");
	}

	PlotWindow::AnimType type;
	switch (cmd.get_int(4))
	{
		case 'l': type = PlotWindow::Linear; break;
		case 'r': type = PlotWindow::Saw; break;
		case 's': type = PlotWindow::Sine; break;
		case 'p': type = PlotWindow::PingPong; break;
		default: throw std::logic_error("Invalid animation type");
	}

	int reps = cmd.get_int(5, true);

	if (eq(v0, v1) || dt <= 0.0 || reps == 0) return;
	w.animate(*p, v0, v1, dt, reps, type);
}

void cmd_stop(PlotWindow &w, Command &cmd)
{
	if (cmd.args.empty())
	{
		w.stop_animations();
	}
	for (auto &arg : cmd.args)
	{
		if (arg.type != Argument::S) throw std::logic_error("Invalid argument type");
		auto &pn = arg.s;
		Element *e = w.rns.find(pn, -1, false);
		if (!e || !e->isParameter())
		{
			printf("Parameter \"%s\" not found\n", pn.c_str());
			continue;
		}
		w.stop_animation(*(Parameter*)e);
	}
}

