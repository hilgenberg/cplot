## CPlot

Linux and Windows function plotter. Screenshots and Windows binaries are on the [homepage](http://zoon.cc/cplot/).

<p align="center"><img src="Windows/Help/Media/CPlot128@2x.png?raw=true" alt="icon"/></p>

Supports various projection modes and function types, natural expression syntax (sin xy instead of Sin[x*y] f.e.), blending between functions (identity and your target function for example), realtime parameter variation (where parameters are things like a mass or spring constant, order of a pole, etc), ...

### Windows build:

Copy boost headers to Windows/boost/, open CPlot.sln in Visual Studio 2017, and it should just build.

### Linux build:

```Shell
sudo pacman -S --needed git python ninja boost zlib sdl2 libgl glu glew pango cairo
git clone https://github.com/hilgenberg/cplot
cd cplot
git submodule update --init --recursive
./build
./cplot test.cplot
```

Press Escape to show/hide the GUI.
Documentation is available from the menu under View > Show Help.
