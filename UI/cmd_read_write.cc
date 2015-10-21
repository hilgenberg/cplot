#include "cmd_base.h"

static bool parse_read(const std::vector<std::string> &args)
{
	if (args.size() != 1) return false;
	cmd.send(CID::READ, args[0]);
	return true;
}

//--------------------------------------------------------------------------------------------

CommandInfo ci_read("read", "r", CID::READ, parse_read, "read <file>",
"Loads <file>, replacing the current state with the file's contents.");

//--------------------------------------------------------------------------------------------

void cmd_read(PlotWindow &w, Command &cmd)
{
	w.load(cmd.get_str(0,true));
}

