#include "cmd_base.h"

static bool help(const std::vector<std::string> &args)
{
	if (args.empty())
	{
		std::set<std::string> names;
		printf("\nValid commands are:\n");
		for (CommandInfo **ci = cli_commands; *ci; ++ci)
		{
			CommandInfo &c = **ci;
			names.insert(c.name2 ? format("%s/%s", c.name, c.name2) : c.name);
		}
		bool first = true;
		for (auto &s : names)
		{
			if (!first) printf(", "); first = false;
			printf("%s", s.c_str());
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

