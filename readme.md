This is a utility to generate maps for 
[Mystic Balloon](https://github.com/Team-ARG-Museum/ID-34-Mystic-Balloon),
the Arduboy platformer by the former Team a.r.g. When I was playing the game,
I got a bit lost in some levels due to the limited viewing area, so I thought
it would be fun to make a map generater tool.

This tool generates maps for all defined levels in bitmaps.h. It also write a
few sprite images to disk just for fun, even if those can easily be found
at the official repo as well. All maps and bitmaps are stored directly in
the C source code as data arrays.

The mapper is written in C++, and aside from a C++ compiler
(assuming binary of `c++`), the tool only has a single dependency:
[Magick++](http://www.imagemagick.org/Magick++/).

To get the dependency on a Debian-based system, just type:

    # apt install libmagick++-dev

(or `libgraphicsmagick++1-dev` if you prefer)

To build and run, simply type:

    $ make
    $ ./mbmapper
