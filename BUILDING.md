# Building Cartool from sources

Cartool currently targets only **Windows platform 64bit architecture**. It might be possible to target a 32bit architecture, as it was the case before being abandonned.
Porting to other platforms is not under investigations for the moment.

## Compiler

**Visual Studio 2019** is the current compiler for dev.

**Visual Studio 2022** should be working, but has not been fully tested yet.

**Visual Studio 2015** *might* be working for Cartool itself, but some dependencies might not support it.

## Dependencies

- OpenGL
- Intel® OneMKL
- Armadillo
- OpenMP
- pcre 7
- OwlNext 7
