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

static std::function<char*(const char *, int)> current_completer;
static char *gen(const char *text, int i){ return current_completer(text, i); }

static const char *LS_ARGS[] = {"all", "functions", "parameters", "graphs", "constants", "variables", "builtins", NULL};
static const char *ANIM_TYPES[] = {"repeat", "linear", "sine", "pingpong", NULL};

static char *complete_enum(const char *s, int i, const std::vector<std::string> &values, bool ignore_case)
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
static char *complete_enum(const char *s, int i, const char **values, bool ignore_case)
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

static char *complete_graph(const char *t, int idx)
{
	if (idx != 0) return NULL;
	if (*t && t[strlen(t)-1] != ';') return NULL;
	const char *s = rl_line_buffer;
	while (isspace(*s)) ++s;
	while (*s && !isspace(*s)) ++s;
	while (isspace(*s)) ++s;
	if (*s) return NULL;

	if (!cmd.send(CID::GET, (int)'F')) return NULL;
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
		case CID::FOCUS:
		case CID::RETURN: assert(false); break;

		case CID::ERROR: // used for local commands
			if (strcasecmp(ci->name, "quit") == 0) return NULL; // no args
			if (strcasecmp(ci->name, "help") == 0)
			{
				if (idx != 1) break;
				return rl_completion_matches((char*)text, complete_cmd);
			}
			assert(false);
			break;
		
		case CID::LS:
			current_completer = [](const char *s, int i){ return complete_enum(s, i, LS_ARGS, true); };
			return rl_completion_matches((char*)text, gen);

		case CID::SET:
			if (idx == 1)
			{
				if (!cmd.send(CID::GET, '.')) return NULL;
				std::vector<std::string> p;
				for (auto &r : cmd.args) p.push_back(r.s);
				current_completer = [p](const char *s, int i){ return complete_enum(s, i, p, false); };
				return rl_completion_matches((char*)text, gen);
			}
			else if (idx == 2)
			{
				// TODO: complete depending on property type
			}
			break;
		
		case CID::READ:
			if (idx != 1) break;
			rl_filename_quoting_desired = 1;
			return rl_completion_matches((char*)text, rl_filename_completion_function);

		case CID::STOP:
		case CID::ANIM:
		case CID::PARAM:
			if (idx == 1 || ci->cid == CID::STOP)
			{
				if (!cmd.send(CID::GET, 'p')) return NULL;
				std::vector<std::string> p;
				for (auto &r : cmd.args) p.push_back(r.s);
				current_completer = [p](const char *s, int i){ return complete_enum(s, i, p, false); };
				return rl_completion_matches((char*)text, gen);
			}
			else if (ci->cid == CID::ANIM && idx == 4 || idx == 5)
			{
				current_completer = [](const char *s, int i){ return complete_enum(s, i, ANIM_TYPES, true); };
				return rl_completion_matches((char*)text, gen);
			}

		case CID::GRAPH:
			if (idx == 0) return NULL;
			return rl_completion_matches((char*)text, complete_graph);

		case CID::CG:
		{
			if (idx != 1) break;
			if (!cmd.send(CID::GET, '#')) return NULL;
			int n = cmd.args[0].i;
			if (n <= 0) return NULL;
			current_completer = [n](const char *s, int i){ return complete_int(s, i, 0, n-1); };
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

		if (!strcasecmp(s, "q") || !strcasecmp(s, "quit")) break;

		const char *args0;
		CommandInfo *ci = find_command(s, &args0, -1);
		if (!ci)
		{
			printf("Unknown command\n");
			continue;
		}
		s = (char*)args0;
		std::vector<std::string> args;
		while (s && *s)
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
			if (strcasecmp(ci->name, "quit") == 0)
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


