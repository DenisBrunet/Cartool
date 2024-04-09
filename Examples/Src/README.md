# Code examples

## Intended purpose
The files from this directory are meant to be **simple coding examples, show-casing the use of a few typical classes from Cartool**.
These examples are **not designed** to be some sorts of *scientific* tools, f.ex. they might not handle all possible data cases, or they might implement only a sub-part of a given pipe-line.

To see the **full, validated pipe-lines of Cartool**, please refer to the *actual* code itself.

## Technical points
These examples compile into a single, plain console application, to be run from the command-line. Consequently, it has no such things as display capabilities nor GUI.

The provided project is already set with the correct options, so it should compile out-of-the-box.
Note that the linker needs to link against *all* the obj files.

The `main()` function in `Examples.cpp` acts as a single entry point, which then calls every demo functions in a row. This simplifies the *Visual Studio* project by having a single target `Examples.exe`. Just comment out the calls to the functions you don't want to see, recompile, and *voilà*.
