
## 2025-07-07
- **Frequency Analysis** toolbox:
    - Added an _Excluding Bad Epochs_ option
    - Help updated

## 2025-06-26
- **Computing Results of Inverse Solutions** toolbox major update:
    - Toolbox now allows for individual inverse matrices to be loaded
    - Command-Line Interface has been fully implemented
    - Help fully rewritten
- Frequency files boosted (read, write, batch averaging, split and merge, display)

## 2025-04-03
- **Reprocessing MRI** toolbox upgraded
    - Added an optional Clean-Up first stage
    - Added the option to port solution points from a common space to subjects' MRIs
    - Help updated

## 2025-03-31
- **RIS To Volume** toolbox upgraded:
    - It can now output 4D nifti files
    - Saved files are now correctly calibrated
    - Added a Command-Line Interface
    - Help updated

## 2025-03-17
- **Generate Markers from Tracks** toolbox upgraded:
    - Toolbox can now truly run in batch mode
    - Now it can run through all sessions of each EEG
    - Now outputting a verbose file!
    - Help updated
- Whole Markers operations big overhaul, increasing speed and reliability

## 2025-02-07
- **Micro-States Meta-Criterion** updated:
    - Re-tested all available criteria, and slightly changed the "basket" used to compute the meta-criterion

## 2025-02-03
- **Cartool GitHub Website** updated:
    - Moved the old Cartool Community website to GitHub
    - Moved the whole [**Reference Guide online**](https://denisbrunet.github.io/Cartool/ReferenceGuide/index.html)

## 2025-01-15
- **Fixed giganormous EEG files opening and access** (Geodesics MFF, BrainVision...)
- Fixed exporting to EDF files
- Fixed Reprocess/Export Tracks when _Adding Null Tracks_ and _Re-Referencing_ or _Filtering_ etc..
- Frequency Analysis has a new _Parseval_ normalization option
- Fixed the _Standard Deviation_ display on top of EEG Tracks
- Faster EEG opening

## 2024-11-23
- _**Command-Line Interface (CLI)**_ implementation:
    - Controlling the main window size, position, and even screen at opening time
    - Controlling every child window size and position
    - _Reprocess / Export Tracks_ toolbox is now fully accessible from the commnad-line

## 2024-09-10
- Making Cartool fully _**DPI Aware**_:
    - Buttons, texts, graphics and dialogs all **adapt to the current screen _Dot Per Inch_** (DPI, or precision)
    - Help as been ported to be more DPI Aware, too
    - Once running, Cartool can now be **dragged onto another screen**, even with different resolution and DPI, and will adapt itself

## 2024-08-06
- Fixed **Middle-clicking in MRI and ESI** consistency
- Fixed **mouse scroll button** behavior
- Lots of various fixes

## 2024-05-15
- Micro-States Segmentation:
    - Checked GEV output values are correct
    - ESI Space Segmentation fixes
    - **_Meta-Criterion_ re-evaluated and slightly modified**
- Fixed Tissues thicknesses computation, especially for the CSF

## 2024-04-16
- Adding a _Visual Studio 2019_ project for Cartool compilation
- Adding an _Examples_ project to show how to directly use a few Cartool classes in some C++ code
- Creating most of the GitHub files

## 2024-03-14
- **Cartool Open Source kickstarting**. Original source files uploaded.
