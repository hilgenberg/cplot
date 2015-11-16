#include "cmd_base.h"

static bool parse_read(const std::vector<std::string> &args)
{
	if (args.size() != 1) return false;
	cmd.send(CID::READ, args[0]);
	return true;
}

static bool parse_write(const std::vector<std::string> &args)
{
	if (args.size() > 1) return false;
	std::vector<Argument> a;
	if (!args.empty()) a.emplace_back(args[0]);
	cmd.send(CID::WRITE, std::move(a));
	return true;
}

//--------------------------------------------------------------------------------------------

CommandInfo ci_read("read", "r", CID::READ, parse_read, "read <file>",
"Loads <file>, replacing the current state with the file's contents.");

CommandInfo ci_write("write", "w", CID::WRITE, parse_write, "write [file]",
"Save to [file]. Will overwrite existing file without confirmation!");

//--------------------------------------------------------------------------------------------

void cmd_read(PlotWindow &w, Command &cmd)
{
	w.load(cmd.get_str(0,true));
}

void cmd_write(PlotWindow &w, Command &cmd)
{
	std::string path;
	if (cmd.args.empty())
	{
		path = w.path;
	}
	else
	{
		path = cmd.get_str(0, true);
	}
	if (path.empty()) throw std::runtime_error("Need valid filename");

	w.saveAs(path);
	printf("written to %s\n", path.c_str());
}

