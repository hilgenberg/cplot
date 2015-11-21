#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <functional>
#include <pthread.h>
#include <cassert>
#include <algorithm>
#include <string> 
#include "main.h"
#include "Command.h"
#include "../Utility/StringFormatting.h"
#include "cmd_base_cli.h"
#include <readline/readline.h>
#include <readline/history.h>
#undef RETURN

static const char *
#include "../version.h"
#ifdef DEBUG
" (Debug build"
#ifndef NDEBUG
", assertions enabled"
#endif
")"
#endif
;

//--- completion helpers -----------------------------------------------------------------------

static std::vector<std::function<char*(const char *, int)>> completers;
static char *gen(const char *text, int i)
{
	static int j = 0, i0 = 0;
	if (i == 0) j = i0 = 0;
	while (j < (int)completers.size())
	{
		char *ret = completers[j](text, i-i0);
		if (ret) return ret;
		++j; i0 = i;
	}
	return NULL;
}

extern const char *LS_ARGS[], *ANIM_TYPES[], *GRAPH_ARGS[];

static char *complete_enum(const char *s, int i, const std::vector<std::string> &values, bool ignore_case = false)
{
	auto cmp = ignore_case ? strncasecmp : strncmp;
	static size_t j; if (!i) j = 0;
	while (j < values.size())
	{
		const char *v = values[j++].c_str();
		if (cmp(s, v, strlen(s)) == 0) return strdup(v);
	}
	return NULL;
}
static char *complete_enum(const char *s, int i, const char **values, bool ignore_case = false)
{
	auto cmp = ignore_case ? strncasecmp : strncmp;
	static int j; if (!i) j = 0;
	const char *v;
	while ((v = values[j++]))
	{
		if (cmp(s, v, strlen(s)) == 0) return strdup(v);
	}
	return NULL;
}
static char *complete_int(const char *s, int i, int min, int max)
{
	if (*s || i > max-min) return NULL;
	return strdup(format("%d", min+i).c_str());
}
static char *complete_nope(const char *, int)
{
	return NULL;
}

static char *complete_graph(const char *t, int idx, bool all)
{
	static std::vector<std::string> X;
	if (idx == 0)
	{
		if (!cmd.get(all ? GET::ALL_GRAPH_EXPRESSIONS : GET::CURRENT_GRAPH_EXPRESSIONS)) return NULL;
		for (auto &x : cmd.args)
		{
			if (x.type != Argument::S){ assert(false); continue; }
			X.push_back(x.s);
		}
	}

	if (all) return (idx >= 0 && (size_t)idx < X.size()) ? strdup(X[idx].c_str()) : NULL;
	
	if (idx != 0) return NULL;
	bool first = true;
	std::string f;
	for (auto &a : cmd.args)
	{
		if (!first) f += " ; "; first = false;
		f += a.s;
	}
	return strdup(f.c_str());
}

static char *complete_cmd(const char *text, int i)
{
	static CommandInfo **ci = NULL; if (!i) ci = cli_commands;

	while (ci && *ci)
	{
		const char *cn = (*ci++)->matches(text, true);
		if (!cn) continue;
		if (strlen(cn) < strlen(text)) continue;
		return strdup(cn);
	}
	return NULL;
}

//--- main completion function ----------------------------------------------------------------

static char ** complete(const char *text, int start, int end)
{
	//printf("Complete %s [%d,%d]\n", text, start, end);
	bool first_word = true;
	for (int i = 0; i < start; ++i)
	{
		if (isspace(rl_line_buffer[i])) continue;
		first_word = false; break;
	}
	if (first_word) return rl_completion_matches((char*)text, complete_cmd);

	const char *args;
	int idx;
	CommandInfo *ci = find_command(rl_line_buffer, &args, rl_point, &idx);
	if (ci) switch (ci->cid)
	{
		case CID::GET:
		case CID::ASSIGN:
		case CID::FOCUS:
		case CID::RETURN: assert(false); break;

		case CID::ERROR: // used for local commands
			if (strcmp(ci->name, "quit") == 0) return NULL; // no args
			if (strcmp(ci->name, "help") == 0)
			{
				if (idx != 1) break;
				return rl_completion_matches((char*)text, complete_cmd);
			}
			assert(false);
			break;
		
		case CID::LS:
			completers.clear();
			completers.push_back([](const char *s, int i){ return complete_enum(s, i, LS_ARGS, true); });
			return rl_completion_matches((char*)text, gen);

		case CID::SET:
			completers.clear();
			if (idx == 1)
			{
				if (!cmd.get(GET::PROPERTY_NAMES)) return NULL;
				std::vector<std::string> p;
				for (auto &r : cmd.args) p.push_back(r.s);
				completers.push_back([p](const char *s, int i){ return complete_enum(s, i, p); });
				return rl_completion_matches((char*)text, gen);
			}
			else if (idx == 2 && args)
			{
				const char *e = args; while (*e && !isspace(*e)) ++e;
				if (!cmd.get(GET::PROPERTY_VALUES, std::string(args, e))) return NULL;
				std::vector<std::string> v;
				for (auto &r : cmd.args) v.push_back(r.s);
				completers.push_back([v](const char *s, int i){ return complete_enum(s, i, v); });
				return rl_completion_matches((char*)text, gen);
			}
			break;
		
		case CID::READ:
		case CID::WRITE:
			if (idx != 1) break;
			rl_filename_quoting_desired = 1;
			return rl_completion_matches((char*)text, rl_filename_completion_function);

		case CID::STOP:
		case CID::ANIM:
		case CID::PARAM:
			completers.clear();
			if (idx == 1 || ci->cid == CID::STOP)
			{
				if (!cmd.get(GET::USED_PARAMETER_NAMES)) return NULL;
				std::vector<std::string> p;
				for (auto &r : cmd.args) p.push_back(r.s);
				completers.push_back([p](const char *s, int i){ return complete_enum(s, i, p); });
				return rl_completion_matches((char*)text, gen);
			}
			else if (ci->cid == CID::ANIM && idx == 4 || idx == 5)
			{
				completers.push_back([](const char *s, int i){ return complete_enum(s, i, ANIM_TYPES, true); });
				return rl_completion_matches((char*)text, gen);
			}

		case CID::GRAPH:
		{
			completers.clear();

			bool ep = false;
			const char *s = rl_line_buffer;
			for (int i = 0, n = strlen(s); i+3 < rl_point && i+3 < n; ++i)
			{
				if (isspace(s[i]) && s[i+1]=='-' && s[i+2] == '-' && isspace(s[i+3]))
				{
					ep = true;
					i += 4;
					while (i < n && isspace(s[i])) ++i;
					if (i == n)
					{
						completers.push_back([](const char *s, int i){ return complete_graph(s, i, false); });
						return rl_completion_matches((char*)text, gen);
					}
					break;
				}
			}
			if (ep) completers.push_back([](const char *s, int i){ return complete_graph(s, i, true); });
			if (!ep) completers.push_back([](const char *s, int i){ return complete_enum(s, i, GRAPH_ARGS, true); });
			return rl_completion_matches((char*)text, gen);
		}

		case CID::CG:
		{
			if (idx != 1) break;
			if (!cmd.get(GET::GRAPH_COUNT)) return NULL;
			int n = cmd.args[0].i;
			if (n <= 0) return NULL;
			completers.clear();
			completers.push_back([n](const char *s, int i){ return complete_int(s, i, 0, n-1); });
			return rl_completion_matches((char*)text, gen);
		}

		case CID::RM:
		{
			completers.clear();
			if (!cmd.get(GET::GRAPH_COUNT)) return NULL;
			int n = cmd.args[0].i;
			completers.push_back([n](const char *s, int i){ return complete_int(s, i, 0, n-1); });
			
			if (!cmd.get(GET::PARAMETER_NAMES)) return NULL;
			std::vector<std::string> p; for (auto &r : cmd.args) p.push_back(r.s);
			completers.push_back([p](const char *s, int i){ return complete_enum(s, i, p); });
			
			if (!cmd.get(GET::DEFINITION_NAMES)) return NULL;
			std::vector<std::string> d; for (auto &r : cmd.args) d.push_back(r.s);
			completers.push_back([d](const char *s, int i){ return complete_enum(s, i, d); });
				
			return rl_completion_matches((char*)text, gen);
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------

void *cli(void *)
{
	int dummy; pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);

	printf("CPlot %s\n", VERSION);

	for (int i = 1; i < argc; ++i)
	{
		CommandInfo *ci = find_command("read");
		assert(ci);
		std::vector<std::string> args(1, argv[i]);
		ci->run(args);
	}

	rl_bind_key('\t',rl_complete);
	rl_attempted_completion_function = complete;
	rl_completion_entry_function = complete_nope;
	rl_catch_signals = 0;
	rl_readline_name = "cplot";
	rl_basic_word_break_characters = " \t\n";
	rl_completer_quote_characters = "\"";
	rl_filename_quote_characters  = " ";

	std::unique_ptr<char,void(*)(void*)> S(NULL, free);
	while (true)
	{
		S.reset(readline("> "));
		char *s = S.get();
		if (!s) break;

		while (isspace(*s)) ++s;
		int len = strlen(s);
		while (len && isspace(s[len-1])) s[--len] = 0;
		if (!*s)
		{
			cmd.send(CID::FOCUS);
			continue;
		}
		if (s == S.get()) add_history(s);

		if (!strcmp(s, "quit")) break;

		const char *args0;
		CommandInfo *ci = find_command(s, &args0, -1);
		if (!ci)
		{
			printf("Unknown command\n");
			continue;
		}

		bool split = (args0 != s);
		s = (char*)args0;
		std::vector<std::string> args;
		if (!split)
		{
			size_t n = strlen(s);
			while (n > 0 && isspace(s[n-1])) --n;
			args.emplace_back(s, n);
		}
		else while (s && *s)
		{
			while (isspace(*s)) ++s;
			if (!*s) break;
			int n = 0;
			if (*s == '"')
			{
				++n;
				while (s[n] && s[n] != '"')
				{
					if (s[n]=='\\') ++n;
					if (s[n]) ++n;
				}
				if (s[n++] != '"')
				{
					printf("Closing quote missing.");
					ci = NULL; // mark as error
					break;
				}
				args.emplace_back(s+1, n-2);
			}
			else
			{
				while (s[n] && !isspace(s[n]))
				{
					if (s[n]=='\\') ++n;
					if (s[n]) ++n;
				}
				args.emplace_back(s, n);
			}
			s += n;
			std::string &arg = args.back();
			n = arg.length()-1;
			for (int i = 0; i < n; ++i)
			{
				if (arg[i] == '\\')
				{
					arg.erase(i,1);
					switch (arg[i])
					{
						case 'n': arg[i] = '\n'; break;
						case 't': arg[i] = '\t'; break;
						default: break;
					}
					--n;
				}
			}
		}
		if (!ci){ printf("\n"); continue; }

		if (ci->run)
		{
			bool ok = ci->run(args);
			if (!ok)
			{
				printf("Usage: %s\n", ci->usage ? ci->usage : "(missing)");
			}
		}
		else
		{
			if (strcmp(ci->name, "quit") == 0)
			{
				printf("quit has no arguments.\n");
			}
			else assert(false);
			
		}
		printf("\n");
	}
	quit = true;
	return NULL;
}


