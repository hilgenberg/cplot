## CPlot

Linux and Windows function plotter. Screenshots and Windows binaries are on the [homepage](http://zoon.cc/cplot/).

<p align="center"><img src="Windows/Help/Media/CPlot128@2x.png?raw=true" alt="icon"/></p>

Supports various projection modes and function types, natural expression syntax (sin xy instead of Sin[x*y] f.e.), blending between functions (identity and your target function for example), realtime parameter variation (where parameters are things like a mass or spring constant, order of a pole, etc), ...

### Windows build:

Copy boost headers to Windows/boost/, open CPlot.sln in Visual Studio 2017, and it should just build.

### Linux build:

```Shell
sudo pacman -S git python ninja boost
sudo pacman -S zlib sdl2 libgl glu glew pango cairo
git clone https://github.com/hilgenberg/cplot
cd cplot
git submodule update --init --recursive
./build [debug|release]
./cplot test.cplot
```

Press Escape to show/hide the GUI.
Press 1 + left/right arrows to change n, press 1 + space to toggle animation of n.
Control + arrows, control + plus/minus to change the axis range.
Alt + same for the input range of parametric functions.

TODO: add documentation to GUI!
