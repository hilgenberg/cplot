#include "cmd_base.h"

static bool help(const std::vector<std::string> &args)
{
	const char *cols = getenv("COLUMNS");
	int x = 0, w = cols ? atoi(cols) : 0;
	if (args.empty())
	{
		printf("\nValid commands are:\n");
		for (CommandInfo **ci = cli_commands; *ci; ++ci)
		{
			CommandInfo &c = **ci;
			int n = strlen(c.name);
			if (c.name2) n += 1+strlen(c.name2);
			bool last = !(ci+1);
			if (!last) n += 2;
			if (w > 0 && x > 0 && x+n > w){ puts("\n"); x = 0; }
			printf("%s", c.name);
			if (c.name2) printf("/%s", c.name2);
			if (!last) printf(", ");
			x += n;
		}
		printf("\n\nFor help on a specific command: help <command>\n"
		       "Details are in the man page.\n");
	}
	else if (args.size() == 1)
	{
		const std::string &s = args[0];
		CommandInfo *match = NULL;
		for (CommandInfo **ci = cli_commands; *ci; ++ci)
		{
			if (strcasecmp(s.c_str(), (*ci)->name) == 0 ||
			    (*ci)->name2 && strcasecmp(s.c_str(), (*ci)->name2) == 0)
			{
				match = *ci;
				break;
			}
		}
		if (!match)
		{
			printf("%s is not a valid command\n", s.c_str());
		}
		else
		{
			if (match->usage) printf("Usage: %s\n", match->usage);
			else{ printf("Help is missing for %s.\n", s.c_str()); assert(false); }
			if (match->desc) printf("%s\n", match->desc);
		}
	}
	else
	{
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------------------------

CommandInfo ci_help("help", "h", CID::ERROR, help, "help [command]", "Shows help for command or lists all commands.");

