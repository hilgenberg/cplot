#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <pthread.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "PlotWindow.h"
#include "../Utility/System.h"
#include "cli.h"
#include "gui.h"

volatile bool quit = false;
static void signalHandler(int){ quit = true; }

int argc;
char **argv;
const char *arg0; // program name without path
unsigned long terminalID = 0;

int main(int argc, char *argv[])
{
	::argc = argc;
	::argv = argv;
	arg0 = argv[0];
	for (const char *s = arg0; *s; ++s) if (*s == '/') arg0 = s+1;
	
	const char *t = getenv("WINDOWID");
	if (t){ auto tt = atoll(t); if (tt > 0) terminalID = (unsigned long)tt; }

	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = signalHandler;
	sigaction(SIGINT,  &sa, 0);
	sigaction(SIGPIPE, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	pthread_t guiID = 0, cliID = 0;
	if (0 != pthread_create(&cliID, NULL, cli, NULL))
	{
		fprintf(stderr, "%s: can't create CLI thread\n", arg0);
		exit(1);
	}
	if (0 != pthread_create(&guiID, NULL, gui, NULL))
	{
		fprintf(stderr, "%s: can't create GUI thread\n", arg0);
		pthread_cancel(cliID);
		exit(1);
	}
	
	pthread_join(guiID, NULL); quit = true;
	struct timespec tv; tv.tv_nsec = 1000000000/2; tv.tv_sec = 0;
	if (pthread_timedjoin_np(cliID, NULL, &tv)) pthread_cancel(cliID);
	printf("\n");

	return 0;
}

