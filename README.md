# libfive
*Infrastructure for solid modeling.*

(formerly known as [mkeeter/ao](https://github.com/mkeeter/ao))

----

### This repository is a port of libfive to WebAssembly via Emscripten

(Tested on Mac OS X with dependencies installed via Homebrew.)

Building:

 * Install dependencies with `brew install cmake pkg-config eigen boost`.

 * Install & enable `emsdk` as per <http://webassembly.org/getting-started/developers-guide/>.

 * Clone <https://github.com/Omniblox/libfive/tree/add-web-assembly-support>.

 * At root of `Omniblox/libfive` repository:

       mkdir build-wa
       cd build-wa

       emcmake cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix)
       emmake make

       pushd libfive/examples/ && emrun --no_browser --port 8089 .


       if you encounter an error: "Please specify rounding control mechanism.", a simple workaround is to comment out that line.


 * Now visit <http://localhost:8089/example-cxx-libfive-basic.html> in your WebAssembly-capable browser.

 * When run, the `example-cxx-libfive-to-stl` example will present an
   input dialog--the value you type will be used as the sphere radius
   (to limit the size of the generated STL file recommended test value
   is 2 or less).

   Click the "Cancel" button the second time the dialog appears (which
   signifies "end of input"). [Hey, this is only a Proof-of-Concept, ok...]

   The generated STL file will be displayed via the three.js library.

   You can interact with the exported STL file via WebAssembly's `FS`
   object, e.g.:

       FS.stat("/exported.stl")

----

## About
- [Project homepage](https://libfive.com) (with demo videos!)
- [API Examples](https://libfive.com/examples)
- [Downloads](https://libfive.com/download)

`libfive` is a framework for solid modeling using
[functional representations](https://en.wikipedia.org/wiki/Function_representation).

It includes several layers, ranging from infrastructure to GUI:

- The `libfive` shared library contains functions to build, manipulate, and render f-reps.
A great deal of work has gone into the meshing algorithm,
which produces watertight, manifold,
hierarchical, feature-preserving triangle meshes.
The library is written in C++ and exposes a C API in `libfive.h`.
- `libfive-guile` is a [Guile](https://www.gnu.org/software/guile/)
binding to `libfive`.
It exposes a high-level API to construct shapes,
and includes a standard library
of shapes, transforms, and CSG operations.
- **Studio** is a GUI application in the style of
[OpenSCAD](http://www.openscad.org/).
It wraps `libfive-guile` and allows for live-coding of solid models.
The interface also includes direct modeling,
where the user can push and pull on the model's surface
to change variables in the script.

## Compiling from source
The full system (`libfive` + `libfive-guile` + **Studio**)
has been successfully compiled on Mac and Linux.
There's also a third-party fork
[here](https://github.com/bradrothenberg/ao/tree/win64)
that builds `libfive` on Windows with MSVC.

### Dependencies
- [`cmake`](https://cmake.org/)
- [`pkg-config`](https://www.freedesktop.org/wiki/Software/pkg-config/)
- [Eigen 3.x](http://eigen.tuxfamily.org/index.php?title=Main_Page)
- [`libpng`](http://www.libpng.org/pub/png/libpng.html)
- [Qt 5.7 or later](https://www.qt.io)
- [Guile 2.2 or later](https://www.gnu.org/software/guile/)
- [Boost](https://www.boost.org)

On a Mac with `homebrew` installed, run
```
brew install cmake pkg-config eigen libpng qt guile boost
```

On a Ubuntu machine, run
```
sudo apt-get install cmake pkg-config libeigen3-dev libpng-dev qtbase5-dev guile-2.2-dev libboost-all-dev
```
(untested; open an issue or PR if this doesn't work for you)

Qt and Guile are optional; if they aren't present, then
the Guile bindings and Studio will not be included in the build
(and `cmake` will print a message to that effect).

### Invocation
```
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.7.0  ..
make
```
Adjust based on your Qt installation path, and consider using [`ninja`](https://ninja-build.org/) for faster builds.

## License
(c) 2015-2017 Matthew Keeter

Different layers of this project are released under different licenses:
- The `libfive` dynamic library is released under the LGPL, version 2 or later.
- `libfive-guile` and `Studio` are released under the GPL, version 2 or later.

Contact me to discuss custom development,
integration,
or commercial support.
