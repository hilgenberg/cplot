## CPlot

Linux and Windows function plotter. Screenshots and Windows binaries are on the [homepage](http://zoon.cc/cplot/).

<p align="center"><img src="Windows/Help/Media/CPlot128@2x.png?raw=true" alt="icon"/></p>

Supports various projection modes and function types, natural expression syntax (sin xy instead of Sin[x*y] f.e.), blending between functions (identity and your target function for example), realtime parameter variation (where parameters are things like a mass or spring constant, order of a pole, etc), ...

### Windows build:

Copy boost headers to Windows/boost/, open CPlot.sln in Visual Studio 2017, and it should just build.

### Linux build:

Needs
- scons and g++
- libz, libpthread, libreadline
- libX11, libXinput2
- GL, GLU, GLEW
- pangocairo
- boost headers

After building (just *scons* or *scons --release*),
try ./cplot test.cplot, press 1 + left/right arrows to change n,
type *help* for command list, *g csc z^2* to change the function, *q* to quit, ...
It can actually do quite a bit more, but the UI does not exist yet.
