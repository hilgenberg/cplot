#include <signal.h>
#include "PlotWindow.h"
#include "../Utility/System.h"
#include "../Utility/Preferences.h"
#include "GUI.h"
#include <SDL.h>
#include <SDL_opengl.h>

volatile bool quit = false;
static void signalHandler(int){ quit = true; }

int main(int argc, char *argv[])
{
	const char *arg0 = argv[0]; // program name without path
	for (const char *s = arg0; *s; ++s) if (*s == '/') arg0 = s+1;
	
	Preferences::reset();
	
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = signalHandler;
	sigaction(SIGINT,  &sa, 0);
	sigaction(SIGPIPE, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		fprintf(stderr, "Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,   16);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,  16);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("CPlot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "GLEW init failed!\n");
		SDL_GL_DeleteContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	GL_CHECK;

	int retcode = 0;

	try
	{
		GL_CHECK;
		GL_Context context(gl_context);
		PlotWindow w(window, context);
		GUI gui(window, gl_context, w);

		for (int i = 1; i < argc; ++i) w.load(argv[i]);
		GL_CHECK;

		while (!quit)
		{
			GL_CHECK;
			SDL_Event event;
			while (!quit && SDL_PollEvent(&event))
			{
				if (!w) quit = true;
				if (gui.handle_event(event)) continue;
				if (w.handle_event(event)) continue;
			}
			if (!w) quit = true;
			if (quit) break;

			GL_CHECK;

			int W, H; SDL_GL_GetDrawableSize(window, &W, &H);
			//int ww, hh; SDL_GetWindowSize(window, &ww, &hh);
			//printf("%d x %d vs %d x %d\n", W, H, ww, hh);
			GL_CHECK;
			w.reshape(W, H);
			GL_CHECK;
	
			double t1 = w.next_frame_schedule();
			if (t1 > 0.0)
			{
				gui.update();
				GL_CHECK;

				double t = now();
				while (t < t1)
				{
					if (quit) break;
					sleep(t1 - t);
					t = now();
				}
				if (quit) break;

				w.animate(t);
				GL_CHECK;
				gui.draw();
				GL_CHECK;
				SDL_GL_SwapWindow(window);
			}
			else if (w.needs_redraw() || gui.needs_redraw())
			{
				gui.update();
				GL_CHECK;
				w.draw();
				GL_CHECK;
				gui.draw();
				GL_CHECK;
				SDL_GL_SwapWindow(window);
			}
			GL_CHECK;

			if (!(w.animating() || w.needs_redraw() || gui.needs_redraw()))
			{
				constexpr double SLEEP_MIN = 1.0 / 1000, SLEEP_MAX = 1.0 / 30;
				double st = SLEEP_MIN;
				while (!SDL_WaitEventTimeout(NULL, (int)(st*1000)))
				{
					if (quit) break; // signal handler can set it
					st *= 2.0;
					if (st > SLEEP_MAX) st = SLEEP_MAX;
				}
			}
		}
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s: %s\n", arg0, e.what());
		retcode = 2;
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	Preferences::flush();

	return retcode;
}
