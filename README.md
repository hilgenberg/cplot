## cplot

Linux port of the [Mac OS X](http://www.zoon.cc/cplot/) function plotter.

Its main function is to help understanding how some function (like the complex
exponential or the solution to some differential equation, say) actually works.

For that, it supports various projection modes and function types, natural expression
syntax (sin xy instead of Sin[x*y] f.i.), blending between functions (identity and your
target function for example), and realtime parameter variation (where parameters
are things like a mass or spring constant, order of a pole, etc).

Some screenshots are at the link above, but the interface is completely different
on Linux.

Work in progress. Build needs:

- scons and g++
- libz, libpthread, libreadline
- libX11, libXi
- GL, GLU, GLEW
- boost headers

After building (just *scons* and there's no release build yet),
try ./cplot test.cplot, press 1 + left/right arrows to change n,
type *help* for command list, *g csc z^2* to change the function, *q* to quit, ...
It can actually do quite a bit more, but the UI does not exist yet.

Fun project with lots to be done.
This is mostly C++11, some OpenGL, some X11, some unixy readline stuff, ...
The current construction sites are mainly in UI/ (cmd_xxx are like shell builtins)
and Graphs/xxx_Properties (for the *set* command).
