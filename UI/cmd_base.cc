#include "cmd_base.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>

//---------------------------------------------------------------------------------------------
//--- GUI Command Dispatcher ------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

extern void cmd_read(PlotWindow &w, Command &cmd);
extern void cmd_anim(PlotWindow &w, Command &cmd);
extern void cmd_stop(PlotWindow &w, Command &cmd);
extern void cmd_ls(PlotWindow &w, Command &cmd);
extern void cmd_ls(const Parameter &p, const RootNamespace &rns, const std::set<Parameter*> used);
extern void cmd_ls(const Graph &g, int i, bool selected);
extern void cmd_get(PlotWindow &w, Command &cmd);
extern void cmd_set(PlotWindow &w, Command &cmd);
extern void cmd_select(PlotWindow &w, Command &cmd);
extern void cmd_param(PlotWindow &w, Command &cmd);
extern void cmd_graph(PlotWindow &w, Command &cmd);

void gui_cmd(Command &cmd, PlotWindow &w)
{
	try
	{
		switch (cmd.cid)
		{
			case CID::READ:   cmd_read  (w, cmd); break;
			case CID::LS:     cmd_ls    (w, cmd); break;
			case CID::ANIM:   cmd_anim  (w, cmd); break;
			case CID::STOP:   cmd_stop  (w, cmd); break;
			case CID::GET:    cmd_get   (w, cmd); return; //!
			case CID::SET:    cmd_set   (w, cmd); break;
			case CID::SELECT: cmd_select(w, cmd); break;
			case CID::GRAPH:  cmd_graph (w, cmd); break;
			case CID::PARAM:  cmd_param (w, cmd); break;
			case CID::FOCUS:
				if (!w.focus(w.window))
				{
					cmd.error("Focus: Failed.");
					return;
				}
				break;

			default: throw std::logic_error("Invalid command");
		}

		cmd.args.clear();
		cmd.cid = CID::RETURN;
		cmd.done();
	}
	catch(std::exception &e)
	{
		printf("Error: %s\n", e.what());
		cmd.error(e.what());
	}
	catch (...)
	{
		printf("Unknown exception (this should not happen)\n");
		cmd.error("Unknown");
	}
}


