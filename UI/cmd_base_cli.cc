#include "cmd_base_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include "../Utility/StringFormatting.h"

//---------------------------------------------------------------------------------------------
//--- Command Info List -----------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

extern CommandInfo ci_help, ci_ls, ci_rm, ci_anim, ci_stop, ci_graph, ci_param, ci_select,
                   ci_read, ci_write, ci_set, ci_assign;

#define NOOP std::function<bool(const std::vector<std::string>&)>()
CommandInfo ci_quit("quit", NULL, CID::ERROR, NOOP, "quit", "Exits the program.");

CommandInfo *cli_commands[] = {
	&ci_quit, &ci_help, &ci_anim, &ci_stop, &ci_ls, &ci_rm,
	&ci_read, &ci_write, &ci_select, &ci_graph, &ci_param,
	&ci_set, &ci_assign, NULL};

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
			if (strncmp(s, n, l) == 0) return n;
		}
		else
		{
			// but s="commander" does not match n="command"
			// s="command foo" does match though
			if (strncmp(s, n, ln) != 0) continue;
			if (isspace(s[ln])) return n;
		}
	}
	return NULL;
}

static const char *skip_expression(const char *s)
{
	while (isspace(*s)) ++s;
	const char *s0 = s;
	while (*s && !isspace(*s) && *s != '=' && *s != '(' && *s != ')') ++s;
	if (s == s0) return NULL;

	while (isspace(*s)) ++s;
	if (*s == '(')
	{
		int pl = 1; ++s;
		while (pl && *s)
		{
			switch (*s++)
			{
				case '(': ++pl; break;
				case ')': --pl; break;
			}
		}
		if (pl) return NULL;
		while (isspace(*s)) ++s;
	}
	return s;
}

CommandInfo *find_command(const char *line, const char **args, int cursor, int *idx)
{
	if (cursor < 0 && args && !idx)
	{
		int i;
		if (is_int(line, i))
		{
			*args = line;
			return &ci_select;
		}

		const char *s = skip_expression(line);
		if (s && *s == '=')
		{
			*args = line;
			return &ci_assign;
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

