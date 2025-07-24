/************************************************************************\
© 2024-2025 Denis Brunet, University of Geneva, Switzerland.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
\************************************************************************/

#pragma once

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-


namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Sub-commands
constexpr char*     __register                  = "register";
constexpr char*     __reprocesstracks           = "reprocesstracks";
constexpr char*     __interpolatetracks         = "interpolatetracks";
constexpr char*     __frequency                 = "frequency";
constexpr char*     __computingris              = "computingris";
constexpr char*     __ristovolume               = "ristovolume";


//----------------------------------------------------------------------------
                                        // Main application options
constexpr char*     __version                   = "--version";

constexpr char*     __nosplash                  = "--nosplash";

constexpr char*     __mainwindow                = "--mainwindow";
constexpr char*     __mainwindowsize            = "--mainwindowsize";
constexpr char*     __mainwindowpos             = "--mainwindowpos";
constexpr char*     __childwindow               = "--childwindow";
constexpr char*     __childwindowsize           = "--childwindowsize";
constexpr char*     __childwindowpos            = "--childwindowpos";
constexpr char*     __minimized                 = "minimized";
constexpr char*     __maximized                 = "maximized";
constexpr char*     __normal                    = "normal";

constexpr char*     __monitor                   = "--monitor";


//----------------------------------------------------------------------------
                                        // Common options, with most common associated description strings
#define             RequiredString              " (Required)"

constexpr char*     __files                     = "files";
constexpr char*     __files_descr               = "List of files";

constexpr char*     __h                         = "-h";
constexpr char*     __help                      = "--help";
constexpr char*     __help_descr                = "This message";

constexpr char*     __preset                    = "--preset";

constexpr char*     __tracks                    = "--tracks";
constexpr char*     __xyzfile                   = "--xyzfile";
constexpr char*     __xyzfile_descr             = "Using electrodes names from an Electrodes Coordinates file";
constexpr char*     __roisfile                  = "--roisfile";
constexpr char*     __roisfile_descr            = "Computing ROIs from a ROIs file";

constexpr char*     __timemin                   = "--timemin";
constexpr char*     __timemin_descr             = "Starting from time frame (Default:0)";
constexpr char*     __timemax                   = "--timemax";
constexpr char*     __timemax_descr             = "Ending at time frame (Default:EOF)";
constexpr char*     __keeptriggers              = "--keeptriggers";
constexpr char*     __keeptriggers_descr        = "Using only time intervals from a triggers / markers list";
constexpr char*     __excludetriggers           = "--excludetriggers";
constexpr char*     __excludetriggers_descr     = "Excluding time intervals from a triggers / markers list";

constexpr char*     __reference                 = "--reference";
constexpr char*     __reference_descr           = "Reference tracks" Tab Tab "Special values: 'none' (default), 'asinfile', 'average' or 'avgref'";

constexpr char*     __samplingfrequency         = "--samplingfrequency";
constexpr char*     __samplingfrequency_descr   = "Overriding Sampling Frequency";
constexpr char*     __downsampling              = "--downsampling";

constexpr char*     __subdir                    = "--subdir";
constexpr char*     __subdir_descr              = "Saving all resuts into a sub-directory";
constexpr char*     __prefix                    = "--prefix";
constexpr char*     __prefix_descr              = "Prefix added to the output file names";
constexpr char*     __infix                     = "--infix";
constexpr char*     __infix_descr               = "Infix inserted into the output file names";
constexpr char*     __postfix                   = "--postfix";
constexpr char*     __postfix_descr             = "Postfix appended to the output file names";
constexpr char*     __extension                 = "--extension";
constexpr char*     __ext                       = "--ext";
constexpr char*     __ext_descr                 = "Output file extension";


//----------------------------------------------------------------------------
                                        // Register Sub-command
constexpr char*     __y                         = "-y";
constexpr char*     __yes                       = "--yes";
constexpr char*     __n                         = "-n";
constexpr char*     __no                        = "--no";
constexpr char*     __r                         = "-r";
constexpr char*     __reset                     = "--reset";
//constexpr char*   __o                         = "-o";
//constexpr char*   __none                      = "--none";


//----------------------------------------------------------------------------
                                        // Reprocess Tracks
constexpr char*     __nulltracks                = "--nulltracks";

constexpr char*     __filters                   = "--filters";
constexpr char*     __baseline                  = "--baseline";
constexpr char*     __dc                        = "--dc";
constexpr char*     __highpass                  = "--highpass";
constexpr char*     __lowpass                   = "--lowpass";
constexpr char*     __bandpass                  = "--bandpass";
constexpr char*     __order                     = "--order";
constexpr char*     __causal                    = "--causal";
constexpr char*     __notches                   = "--notches";
constexpr char*     __harmonics                 = "--harmonics";
constexpr char*     __spatialfilter             = "--spatialfilter";
constexpr char*     __ranking                   = "--ranking";
constexpr char*     __rectification             = "--rectification";
constexpr char*     __envelope                  = "--envelope";
constexpr char*     __keepabove                 = "--keepabove";
constexpr char*     __keepbelow                 = "--keepbelow";

constexpr char*     __baselinecorr              = "--baselinecorr";
constexpr char*     __rescaling                 = "--rescaling";
constexpr char*     __sequential                = "--sequential";
constexpr char*     __average                   = "--average";

constexpr char*     __nomarkers                 = "--nomarkers";
constexpr char*     __concatenate               = "--concatenate";


//----------------------------------------------------------------------------
                                        // Interpolate Tracks
constexpr char*     __fromxyz                   = "--fromxyz";
constexpr char*     __fromleft                  = "--fromleft";
constexpr char*     __fromright                 = "--fromright";
constexpr char*     __fromfront                 = "--fromfront";
constexpr char*     __fromrear                  = "--fromrear";
constexpr char*     __fromtop                   = "--fromtop";
constexpr char*     __badelec                   = "--bad";
constexpr char*     __toxyz                     = "--toxyz";
constexpr char*     __toleft                    = "--toleft";
constexpr char*     __toright                   = "--toright";
constexpr char*     __tofront                   = "--tofront";
constexpr char*     __torear                    = "--torear";
constexpr char*     __totop                     = "--totop";
constexpr char*     __splinemethod              = "--method";
constexpr char*     __splinesurface             = "surfacespline";
constexpr char*     __splinespherical           = "sphericalspline";
constexpr char*     __spline3d                  = "3dspline";
constexpr char*     __splinecurrentdensity      = "currentdensity";
constexpr char*     __splinedegree              = "--degree";
constexpr char*     __nocleanup                 = "--nocleanup";


//----------------------------------------------------------------------------
                                        // Frequency Analysis
constexpr char*     __windowsize                = "--windowsize";
constexpr char*     __windowstep                = "--windowstep";
constexpr char*     __windowstep1               = "1tf";
constexpr char*     __windowstep25              = "25%";
constexpr char*     __windowstep100             = "100%";

constexpr char*     __freqmin                   = "--freqmin";
constexpr char*     __freqmax                   = "--freqmax";
constexpr char*     __freqlinstep               = "--freqlinstep";
constexpr char*     __freqlogdecade             = "--freqlogdecade";
constexpr char*     __freqbands                 = "--freqbands";

constexpr char*     __case                      = "--case";
constexpr char*     __caseeegsurf               = "eegsurface";
constexpr char*     __caseeegintra              = "eegintra";
constexpr char*     __caseeeggeneral            = "general";
constexpr char*     __method                    = "--method";
constexpr char*     __methodfft                 = "fft";
constexpr char*     __methodfftapprox           = "fftapprox";
constexpr char*     __methodstransform          = "stransform";
constexpr char*     __windowinghanning          = "--hanning";
constexpr char*     __rescalingnone             = "none";
constexpr char*     __rescalingsqrt             = "sqrt";
constexpr char*     __rescalingsize             = "parseval";
constexpr char*     __output                    = "--output";
constexpr char*     __outputreal                = "real";
constexpr char*     __outputnorm                = "norm";
constexpr char*     __outputpower               = "power";
constexpr char*     __outputcomplex             = "complex";
constexpr char*     __outputphase               = "phase";

constexpr char*     __savingfreq                = "--savefreq";
constexpr char*     __splitelec                 = "--splite";
constexpr char*     __splitfreq                 = "--splitf";
constexpr char*     __splitspectrum             = "--splits";


//----------------------------------------------------------------------------
                                        // Ris To Volumes
constexpr char*     __spfile                    = "--spfile";
constexpr char*     __greyfile                  = "--greyfile";

constexpr char*     __timestep                  = "--timestep";

constexpr char*     __interpolation             = "--interpolation";
constexpr char*     __1NN                       = "1NN";
constexpr char*     __4NN                       = "4NN";
constexpr char*     __linear                    = "linear";
constexpr char*     __cubickernel               = "cubickernel";

constexpr char*     __fileformat                = "--file";
constexpr char*     __nifti                     = "nifti";
constexpr char*     __analyze                   = "analyze";

constexpr char*     __typeformat                = "--type";
constexpr char*     __byte                      = "byte";
constexpr char*     __float                     = "float";

constexpr char*     __dimensions                = "--dim";
constexpr char*     __3D                        = "3";
constexpr char*     __4D                        = "4";


//----------------------------------------------------------------------------
                                        // Computing Ris
constexpr char*     __preset1                   = "ErpGrandAverages";
constexpr char*     __preset2                   = "ErpSubjectsAverages";
constexpr char*     __preset3                   = "ErpSubjectsEpochs";
constexpr char*     __preset4                   = "ErpSubjectsClusters";    // Segmentation or Fitting
constexpr char*     __preset5                   = "IndSubjectsEpochs";
constexpr char*     __preset6                   = "SpontSubjects";
constexpr char*     __preset7                   = "SpontSubjectsClusters";
constexpr char*     __preset8                   = "FreqComplex";

constexpr char*     __listfiles                 = "--listfiles";

constexpr char*     __inversefile               = "--isfile";
                                                // options for __typeformat
constexpr char*     __norm                      = "norm";
constexpr char*     __vector                    = "vector";

constexpr char*     __reg                       = "--reg";
constexpr char*     __regularization            = "--regularization";
constexpr char*     __regauto                   = "auto";
constexpr char*     __regnone                   = "none";
constexpr char*     __reg0                      = "0";
constexpr char*     __reg1                      = "1";
constexpr char*     __reg2                      = "2";
constexpr char*     __reg3                      = "3";
constexpr char*     __reg4                      = "4";
constexpr char*     __reg5                      = "5";
constexpr char*     __reg6                      = "6";
constexpr char*     __reg7                      = "7";
constexpr char*     __reg8                      = "8";
constexpr char*     __reg9                      = "9";
constexpr char*     __reg10                     = "10";
constexpr char*     __reg11                     = "11";
constexpr char*     __reg12                     = "12";

constexpr char*     __zscore                    = "--zscore";
constexpr char*     __compute                   = "compute";
constexpr char*     __loadfile                  = "loadfile";

constexpr char*     __savingsubjectsS           = "-s";
constexpr char*     __savingsubjects            = "--savingsubjects";
constexpr char*     __savingsubjectsepochsS     = "-e";
constexpr char*     __savingsubjectsepochs      = "--savingepochs";
constexpr char*     __savinggrandaverageS       = "-g";
constexpr char*     __savinggrandaverage        = "--savinggrandaverages";
constexpr char*     __savingtemplatesS          = "-t";
constexpr char*     __savingtemplates           = "--savingtemplates";
constexpr char*     __savingzscoreS             = "-z";
constexpr char*     __savingzscore              = "--savingzscore";


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
