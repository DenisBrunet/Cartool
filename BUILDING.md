# Building Cartool from sources

Cartool currently targets **Windows 64bit platform** only. 32bit architecture was available at some point but is not supported anymore.

Porting to other platforms is not under investigations for the moment.

## Compiler

**Visual Studio 2019** is the current compiler for dev.

**Visual Studio 2022** should be working, but has not been fully tested yet.

**Visual Studio 2015** *might* be working for Cartool itself, but some dependencies might not support it.

## Dependencies
Here is the list of all the dependencies needed to compile Cartool:
- OpenGL
- OpenMP
- Intel® OneMKL
- Armadillo
- pcre 7
- OwlNext 7

## OpenGL
*OpenGL* graphics library is directly supported from Visual Studio. A few files have been added to the repo so that you *do not have to install any additional files*:
| File | Installation |
| :- | :- |
| gl.h | installed from Windows Kits |
| glu.h | installed from Windows Kits |
| khrplatform.h | included in repo |
| glext.h | included in repo |
| OpenGL32.Lib | included in repo |
| GlU32.Lib | included in repo |

## OpenMP
*OpenMP* parallel computation library is directly supported from Visual Studio.

Open the project property pages, and from the *C/C++ / Language* panel set:
| Option | Set to |
| :- | :- |
| Open MP Support | **Yes (/openmp)** |

## Intel® OneMKL
*Intel® OneMKL* linear algebra library is directly supported from Visual Studio.

Open the project property pages, and from the *Intel Libraries for OneAPI* panel set:
| Option | Set to |
| :- | :- |
| Use OneMKL | **Parallel** |
| Use ILP64 interfaces | **Yes** |

## Armadillo
*Armadillo* is a C++ linear algebra, header-only library:
- Just download the latest [Armadillo package](https://arma.sourceforge.net/download.html)
- Un-zip it on your machine, like in "D:\Dev\C++\Armadillo", and *voilà* (not need to compile it into a library!)

***Important:***
- You do **not** need to download *OpenBLAS* or the like, as Cartool uses the *Intel® OneMKL* library for speeding up computations!
- Also, do **not** use *vcpkg* to install Armadillo, as it will forcibly install said *OpenBLAS*, which we do **not** want!

## pcre 7


## OwlNext 7

