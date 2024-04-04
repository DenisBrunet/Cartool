# Building Cartool from sources

Cartool currently targets **Windows 64bit platform** only. 32bit architecture was available at some point but is not supported anymore.

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

### OpenGL

Cartool visualizations are written in OpenGL3. It uses the "classical" pipe-line, and does not use shaders.
It could even be downgraded to OpenGL1 software emulation if needed, though some features will not work anymore.

It uses the following headers:
- gl.h (from Windows Kits)
- glu.h (from Windows Kits)
- khrplatform.h (included in repo)
- glext.h (included in repo)

### Intel® OneMKL


### Armadillo


### OpenMP


### pcre 7


### OwlNext 7

