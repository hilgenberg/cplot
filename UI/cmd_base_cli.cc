#include "cmd_base_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include "../Utility/StringFormatting.h"

bool is_int(const std::string &s, int &v_)
{
	int v = 0;
	int sign = 1;
	size_t i = 0, n = s.length();
	if (!n) return false;
	if      (s[0] == '-'){ ++i; sign = -1; }
	else if (s[0] == '+'){ ++i;            }
	if (i >= n) return false;

	for (; i < n; ++i)
	{
		char c = s[i];
		if (!isdigit(c)) return false;
		v *= 10; v += c - '0';
	}
	v_ = sign * v;
	return true;
}
bool has_prefix(const std::string &s, const char *p, size_t &l)
{
	l = strlen(p);
	if (s.length() < l) return false;
	if (strncasecmp(s.c_str(), p, l) != 0) return false;
	while (l < s.length() && isspace(s[l])) ++l;
	return true;
}

//---------------------------------------------------------------------------------------------
//--- CLI Command Helpers ---------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

const char *CommandInfo::matches(const char *s, bool partial) const
{
	while(isspace(*s)) ++s;
	for (int i = 0; i < 2; ++i)
	{
		const char *n = i ? name2 : name; if (!n) continue;
		int l = strlen(s), ln = strlen(n);
		if (l <= ln)
		{
			if (!partial && l < ln) continue;
			// s="com" matches n="command" partially
			if (strncasecmp(s, n, l) == 0) return n;
		}
		else
		{
			// but s="commander" does not match n="command"
			// s="command foo" does match though
			if (strncasecmp(s, n, ln) != 0) continue;
			if (isspace(s[ln])) return n;
		}
	}
	return NULL;
}

CommandInfo *find_command(const char *line, const char **args, int cursor, int *idx)
{
	if (cursor < 0 && args && !idx)
	{
		int i;
		if (is_int(line, i))
		{
			CommandInfo *ci = find_command("select");
			*args = line;
			return ci;
		}
	}

	while (isspace(*line)){ ++line; --cursor; }

	if (idx)
	{
		*idx = 0;
		const char *s = line;
		for (int j = cursor; j > 0;)
		{
			if (*s == '"')
			{
				++s; --j;
				while (*s && *s != '"')
				{
					if (*s == '\\'){ ++s; --j; }
					if (*s){ ++s; --j; }
				}
				if (*s == '"'){ ++s; --j; }
			}
			else while (*s && !isspace(*s))
			{
				if (*s == '\\'){ ++s; --j; }
				if (*s){ ++s; --j; }
			}
			if (j > 0) ++ *idx;
			while (isspace(*s)){ ++s; --j; }
		}
	}

	for (CommandInfo **c = cli_commands; *c; ++c)
	{
		const char *cn = (*c)->matches(line, false);
		if (!cn) continue;
		if (args)
		{
			*args = line + strlen(cn);
			while (isspace(**args)) ++ *args;
		}
		return *c;
	}
	return NULL;
}

//---------------------------------------------------------------------------------------------
//--- Command Info List -----------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

extern CommandInfo ci_help, ci_ls, ci_anim, ci_stop, ci_graph, ci_param, ci_select, ci_read, ci_set;

#define NOOP std::function<bool(const std::vector<std::string>&)>()
CommandInfo ci_quit("quit", "q", CID::ERROR, NOOP, "quit", "Exits the program.");

CommandInfo *cli_commands[] = {
	&ci_quit, &ci_help, &ci_anim, &ci_stop, &ci_ls,
	&ci_read, &ci_select, &ci_graph, &ci_param, &ci_set,
	NULL};

