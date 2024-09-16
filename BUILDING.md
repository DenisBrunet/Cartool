# Building Cartool from sources

Cartool only targets **Windows 64bit platform**.

There are very few dependencies: some are directly included within this repo, some can be set from the Visual Studio project property pages, and some need to be downloaded, either manually or through *vcpkg*.

## Prerequisites
- _Windows 7 64bit_ and above
- At least 8 CPU cores
- At least 8 GB of RAM
- SSD disk recommended

## Compiler
**Visual Studio 2019** is the current compiler for dev.

**Visual Studio 2022** should be working, but has not been fully tested yet.

## *vcpkg* C++ package manager
We are going to use the very convenient C++ package manager from Microsoft called *vcpkg*:
- [vcpkg overview](https://learn.microsoft.com/en-us/vcpkg/get_started/overview)
- [vcpkg installation](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-cmd)

***Note:*** it seems to be already installed with Visual Studio 2022

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
- **Do not use *vcpkg* to install Armadillo**, as it will forcibly install said *OpenBLAS*, which we do **not** want!

## pcre 7
Using *vcpkg*, install **pcre Version: 8.45**:
```
.\vcpkg install pcre
````
***Important***: there are 2 versions of pcre, we use pcre here, *not* pcre2!

## OwlNext 7
Cartool uses an application framework library called [***OwlNext***](https://sourceforge.net/p/owlnext/wiki/Main_Page/), which is very similar to the *Microsoft Foundation Class*.

It is *not* available from *vcpkg*, nor from *GitHub*, but only from *sourceforge.net* as source files. This means you need to build the library yourself, though there should not be any major difficulties if you follow the following steps.

*If you are going to compile and distribute Cartool from sources, you need to pay attention to the following licensing terms*:
- [OWLNext License.txt](https://sourceforge.net/p/owlnext/code/HEAD/tree/trunk/OWLNext%20License.txt)
- [Frequently_Asked_Questions/#license](https://sourceforge.net/p/owlnext/wiki/Frequently_Asked_Questions/#license)

Installation and build:
- [OwlNext step-by-step installation](https://sourceforge.net/p/owlnext/wiki/Installing_OWLNext/)
- Download latest stable release, here [7.0.12](https://sourceforge.net/p/owlnext/code/HEAD/tree/tags/7.0.12/)
- Edit the following files:
  | File | Line | Replace | by |
  | :- | :- | :-  | :- |
  | registry.h | 264 | std::byte | BYTE |
  | decframe.h | 24 | const int | #define |
  | framewin.h | 28, 29 | const unsigned int | #define |
- Open the provided solution file ".\source\owlcore\VS\OWLNext.sln" in Visual Studio
- Open *Project Property Pages* and change the following options:
  | Option | Set to |
  | :- | :- |
  | Output Directory | **..\\..\\..\\lib\\** |   
  | Target Name | **owl-7.0-v1920-x64-t.lib** (release) |   
  | Target Name | **owl-7.0-v1920-x64-dt.lib** (debug) |   
  | Character Set | **Not Set** |   
- **Build both Debug and Release for x64 platform**
- Check you now have 2 .lib files in the .\\lib directory

## Compiling Cartool
Open the *Visual Studio* solution **CartoolVS2019.sln** provided in the *.\CartoolVS2019* directory.

You need first to update the proper paths to the manually installed libraries (*vcpkg* libraries are taken care of automatically).
Instead of using Windows environment variables, which need rebooting all the time, we can use macros from a *property sheet*:
- Open the *Property Manager*
- Open the *LibrariesPaths* property sheet
- Select *User Macros* from the top-left
- You can now see a list of macros. Edit each of them with your actual paths:
  - *OwlNextRoot*: path to your own OwlNext root directory, f.ex. "D:\OwlNext"
  - *CartoolRoot*: path to your own Cartool root directory, f.ex. "D:\Dev\C++\Cartool"
  - *OwlNextRoot*: path to your own Armadillo root directory, f.ex. "D:\Dev\C++\Armadillo"

The provided *Solution* and *Project* are already set with the correct options. *Be careful if you update them!*

At that point, you should be able to **build the *Release* and *Debug* versions**! The executable files will be located in the *.\Bin* directory.

