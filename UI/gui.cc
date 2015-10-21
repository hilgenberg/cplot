#include "PlotWindow.h"
#include "../Utility/System.h"
#include "main.h"
#include "Command.h"

extern void gui_cmd(Command &cmd, PlotWindow &w); // cmd_base.cc

void *gui(void *)
{
	try
	{
		PlotWindow w;
		XEvent e;

		XFlush(w.display);
		const int xfd = ConnectionNumber(w.display);

		while (true)
		{
			if (quit) break;

			if (cmd)
			{
				gui_cmd(cmd, w);
			}

			while (!quit && XPending(w.display))
			{
				XNextEvent(w.display, &e);
				w.handle(e);
				if (!w) return NULL;
			}

			if (quit) break;

			double t1 = w.next_frame_schedule();
			if (t1 > 0.0)
			{
				double t = now();
				while (t < t1)
				{
					if (quit) break;
					sleep(t1 - t);
					t = now();
				}

				if (quit) break;
				w.animate(t);
			}
			else
			{
				if (w.needs_redraw()) w.draw();
				if (w.next_frame_schedule() > 0.0) continue;

				// XPeekEvent with timeout...
				#define SLEEP_MIN (1.0 / 1000)
				#define SLEEP_MAX (1.0 / 30)
				double st = SLEEP_MIN;
				while (!XPending(w.display))
				{
					fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds);
					struct timeval tv; tv.tv_usec = (int)(1000000*st); tv.tv_sec = 0;
					bool event = select(xfd+1, &fds, NULL, NULL, &tv);
					if (event || quit || cmd) break;
					st *= 2.0;
					if (st > SLEEP_MAX) st = SLEEP_MAX;
				}
			}
		}
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s: %s\n", arg0, e.what());
		exit(2);
	}
	return NULL;
}


