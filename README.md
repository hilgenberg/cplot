## CPlot

Linux (formerly Windows, formerly Mac) function plotter. Screenshots and Windows binaries are on the [homepage](http://zoon.cc/cplot/).

<p align="center"><img src="Linux/screenshot.png?raw=true" alt="interface"/></p>

Supports various projection modes and function types, natural expression syntax (sin xy instead of Sin[x*y] f.e.), blending between functions (identity and your target function for example), realtime parameter variation (where parameters are things like a mass or spring constant, order of a pole, etc), ...

### Linux build:

On Arch: `sudo pacman -S --needed git python ninja boost zlib sdl2 libgl glu glew pango cairo`.<br>
On Debian: `sudo apt install git python3 ninja-build libboost-dev libz-dev libsdl2-dev libgl-dev libglu-dev libglew-dev libpango-1.0-dev libcairo2-dev`.

Then build with:
```Shell
git clone https://github.com/hilgenberg/cplot
cd cplot
git submodule update --init --recursive
./build
./cplot test.cplot
```

Press Escape to show/hide the GUI.
Documentation is available from the menu under View > Show Help.
Some demo files are in the "Plot Examples" directory.

### Windows build:

(Somewhat defunct because I have no Windows box at the moment - the 1.10 tag should still work.)
Copy boost headers to Windows/boost/, open CPlot.sln in Visual Studio 2017, and it should just build.
