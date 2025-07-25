/************************************************************************\
© 2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "System.CLI11.h"
#include    "CLIDefines.h"

#include    "Math.FFT.h"
#include    "FrequencyAnalysis.h"
#include    "TFrequencyAnalysisDialog.h"
#include    "Files.TVerboseFile.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Defining the interface
inline void     FrequencyAnalysisCLIDefine ( CLI::App* freqan )
{
if ( freqan == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( freqan,   "",     __case,                 "Analysis case" RequiredString )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __caseeegsurf, __caseeegintra, __caseeeggeneral } ) ) );

DefineCLIOptionEnum     ( freqan,   "",     __method,               "Analysis method" RequiredString )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __methodfft, __methodfftapprox, __methodstransform } ) ) );

DefineCLIFlag           ( freqan,   "",     __sequential,           "Sequential output" );
DefineCLIFlag           ( freqan,   "",     __average,              "Averaging blocks" );
ExcludeCLIOptions       ( freqan,     __sequential,     __average );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( freqan,   "",     __tracks,               "Tracks to analyze (Default:all)" )
->TypeOfOption          ( "TRACKS" );

DefineCLIOptionFile     ( freqan,   "",     __xyzfile,              __xyzfile_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( freqan,   "",     __timemin,              __timemin_descr );
DefineCLIOptionInt      ( freqan,   "",     __timemax,              __timemax_descr );

DefineCLIOptionString   ( freqan,   "",     __excludetriggers,      __excludetriggers_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionDouble   ( freqan,   "",     __samplingfrequency,    __samplingfrequency_descr );

DefineCLIOptionInt      ( freqan,   "",     __windowsize,           "FFT Window Size, in [TF] (not for STransform)" );

DefineCLIOptionEnum     ( freqan,   "",     __windowstep,           "FFT Window Step (not for STransform)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __windowstep1, __windowstep25, __windowstep100 } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionDouble   ( freqan,   "",     __freqmin,              "Lowest frequency to save" RequiredString );

DefineCLIOptionDouble   ( freqan,   "",     __freqmax,              "Highest frequency to save" RequiredString );

DefineCLIOptionDouble   ( freqan,   "",     __freqlinstep,          "Saving frequencies as Linear Interval, with Frequency step" );

DefineCLIOptionInt      ( freqan,   "",     __freqlogdecade,        "Saving frequencies as Log Interval, with this amount per Decade" );
//->DefaultInteger        ( DefaultSaveLogDecade );

DefineCLIOptionIntervals( freqan,   -1, "",     __freqbands,        "Saving a list of Frequency Bands (f.ex. 1-4,4-8,8-14)" );

ExcludeCLIOptions       ( freqan,     __freqlinstep,    __freqlogdecade,    __freqbands );
ExcludeCLIOptions       ( freqan,     __freqlogdecade,  __freqbands );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( freqan,   "",     __windowinghanning,     "Hanning Windowing" );


DefineCLIOptionEnum     ( freqan,   "",     __rescaling,            "Window rescaling (Default:parseval)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __rescalingnone, __rescalingsqrt, __rescalingsize } ) ) );


DefineCLIOptionEnum     ( freqan,   "",     __output,               "Output type (Default:power)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __outputreal, __outputnorm, __outputpower, __outputcomplex, __outputphase } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( freqan,   "",     __reference,            __reference_descr )
->TypeOfOption          ( "TRACKS" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( freqan,   "",     __inputdir,             __inputdir_descr )
->TypeOfOption          ( __inputdir_type )
->CheckOption           ( CLI::ExistingDirectory ); // could be incomplete, but it helps a bit, though

DefineCLIOptionString   ( freqan,   "",     __infix,                __infix_descr );

DefineCLIFlag           ( freqan,   "",     __subdir,               __subdir_descr );
DefineCLIFlag           ( freqan,   "",     __savingfreq,           "Saving .freq files" );
DefineCLIFlag           ( freqan,   "",     __splitelec,            "Splitting results by electrodes" );
DefineCLIFlag           ( freqan,   "",     __splitfreq,            "Splitting results by frequencies" );
DefineCLIFlag           ( freqan,   "",     __splitspectrum,        "Splitting results by spectrum" );
DefineCLIFlag           ( freqan,   "",     __downsampling,         "Optimally downsampling time for STransform" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( freqan,   __h,    __help,                 __help_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating positional files option seems OK
DefineCLIOptionFiles    ( freqan, -1,"",    __files,                __files_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     FrequencyAnalysisCLI ( CLI::App* freqan )
{
if ( ! IsSubCommandUsed ( freqan )  )
    return;


TGoF                gof             = GetCLIOptionFiles ( freqan, __files, __inputdir );

if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merging flags to set the analysis case
if ( ! HasCLIOption ( freqan, __case ) ) {

    ConsoleErrorMessage ( __case, "Analysis case is not specified!" );
    return;
    }

string              optioncase      = GetCLIOptionEnum ( freqan, __case   );


if ( ! HasCLIOption ( freqan, __method ) ) {

    ConsoleErrorMessage ( __method, "Analysis method is not specified!" );
    return;
    }

string              method          = GetCLIOptionEnum ( freqan, __method );


FreqAnalysisCases   analysis        = FreqCaseUndefined;

                                        // First setting the use case
if      ( optioncase == __caseeegsurf       )   SetFlags ( analysis, FreqCaseEEGSurface );
else if ( optioncase == __caseeegintra      )   SetFlags ( analysis, FreqCaseEEGIntra   );
else /*__caseeeggeneral*/                       SetFlags ( analysis, FreqCaseGeneral    );

                                        // Then setting the method
if      ( method     == __methodfft         )   SetFlags ( analysis, FreqMethodFFT       );
else if ( method     == __methodfftapprox   )   SetFlags ( analysis, FreqMethodFFTApprox );
else if ( method     == __methodstransform  )   SetFlags ( analysis, FreqMethodST        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              tracks          = GetCLIOptionString ( freqan, __tracks );

TFileName           xyzfile         = GetCLIOptionFile   ( freqan, __xyzfile, __inputdir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters, exclusively between time interval, triggers to keep or triggers to exclude
                                        // default values
long                timemin         = 0;                      
long                timemax         = Highest ( timemax );    


if  ( HasCLIOption ( freqan, __timemin ) )
    timemin     = GetCLIOptionInt ( freqan, __timemin );

if  ( HasCLIOption ( freqan, __timemax ) )
    timemax     = GetCLIOptionInt ( freqan, __timemax );

CheckOrder ( timemin, timemax );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SkippingEpochsType  badepochs       = HasCLIOption ( freqan, __excludetriggers )  ? SkippingBadEpochsList
                                    :                                               NoSkippingBadEpochs;


TFixedString<EditSizeText>  listbadepochs;

if ( badepochs == SkippingBadEpochsList ) {

    listbadepochs   = GetCLIOptionString ( freqan, __excludetriggers ).c_str ();
    listbadepochs.CleanUp ();

    if ( listbadepochs.IsEmpty () )
        badepochs   = NoSkippingBadEpochs;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For total safety, it is convenient to have a default sampling frequency at hand, which could be passed to EEGs that lack one
double              samplingfrequency   = 0;

                                        // First see if caller provided one
if ( HasCLIOption ( freqan, __samplingfrequency ) ) {

    samplingfrequency   = GetCLIOptionDouble ( freqan, __samplingfrequency );

    if ( samplingfrequency <= 0 ) {

        ConsoleErrorMessage ( __samplingfrequency, "Sampling frequency should greater than 0!" );
        return;
        }
    }
else {
                                        // Loop through all files and stop at first positive sampling frequency
    for ( int filei = 0; filei < (int) gof; filei++ )

        if ( ReadFromHeader ( gof[ filei ],  ReadSamplingFrequency,    &samplingfrequency )
          && samplingfrequency > 0 )

            break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                blocksize       = 0;

if ( ! IsSTMethod ( analysis ) ) {
    
    if ( ! HasCLIOption ( freqan, __windowsize ) ) {

        ConsoleErrorMessage ( __windowsize, "FFT Window size is not specified!" );
        return;
        }

    blocksize   = GetCLIOptionInt ( freqan, __windowsize );

    if ( blocksize < 2 ) {

        ConsoleErrorMessage ( __windowsize, "FFT Window size is too small!" );
        return;
        }
    }
else {

    if ( HasCLIOption ( freqan, __windowsize ) ) {

        ConsoleErrorMessage ( 0, "You do not need to specify ", __windowsize, " option for the S-Transform!" );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              blockstep;
double              blocksoverlap   = 0;

if ( ! IsSTMethod ( analysis ) ) {
    
    if ( ! HasCLIOption ( freqan, __windowstep ) ) {

        ConsoleErrorMessage ( __windowstep, "FFT Window step is not specified!" );
        return;
        }

    blockstep       = GetCLIOptionEnum ( freqan, __windowstep );

    blocksoverlap   = blockstep == __windowstep100 ? 0.00
                    : blockstep == __windowstep25  ? 0.75
                    : blockstep == __windowstep1   ? ( blocksize > 0 ? (double) ( blocksize - 1 ) / blocksize : 0 )
                    :                                0.00;
    }
else {

    if ( HasCLIOption ( freqan, __windowstep ) ) {

        ConsoleErrorMessage ( 0, "You do not need to specify ", __windowstep, " option for the S-Transform!" );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FreqOutputBands     outputinterval      = HasCLIOption ( freqan, __freqlinstep   ) ? OutputLinearInterval
                                        : HasCLIOption ( freqan, __freqlogdecade ) ? OutputLogInterval
                                        : HasCLIOption ( freqan, __freqbands     ) ? OutputBands
                                        :                                            (FreqOutputBands) -1;

if ( outputinterval == -1 ) {

    ConsoleErrorMessage ( 0, "Either a linear interval, a logarithmic interval, or frequency bands should be specified!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              nyquist         = GetNyquist ( samplingfrequency, IsSTMethod ( analysis ) );


if ( ! HasCLIOption ( freqan, __freqmin ) && outputinterval != OutputBands ) {

    ConsoleErrorMessage ( __freqmin, "Frequency min is not specified!" );
    return;
    }
else if ( HasCLIOption ( freqan, __freqmin ) && outputinterval == OutputBands ) {

    ConsoleErrorMessage ( __freqmin, "You do not need the ", __freqmin, " option when saving frequency bands!" );
    return;
    }

double              freqmin         = GetCLIOptionDouble ( freqan, __freqmin );

if ( ! IsInsideLimits ( freqmin, (double) 0, nyquist ) ) {

    ConsoleErrorMessage ( __freqmin, "Frequency min is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! HasCLIOption ( freqan, __freqmax ) && outputinterval != OutputBands ) {

    ConsoleErrorMessage ( __freqmax, "Frequency max is not specified!" );
    return;
    }
else if ( HasCLIOption ( freqan, __freqmax ) && outputinterval == OutputBands ) {

    ConsoleErrorMessage ( __freqmax, "You do not need the ", __freqmax, " option when saving frequency bands!" );
    return;
    }

double              freqmax         = GetCLIOptionDouble ( freqan, __freqmax );

if ( ! IsInsideLimits ( freqmax, (double) 0, nyquist ) ) {

    ConsoleErrorMessage ( __freqmax, "Frequency max is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( freqmin > freqmax ) {

    ConsoleErrorMessage ( 0, "Frequency min is greater than Frequency max!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              outputfreqstep      = GetCLIOptionDouble    ( freqan, __freqlinstep   );


if ( outputinterval == OutputLinearInterval && outputfreqstep <= 0 ) {

    ConsoleErrorMessage ( __freqlinstep, "Linear interval step should be greater than 0!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 outputdecadestep    = GetCLIOptionInt       ( freqan, __freqlogdecade );

if ( outputinterval == OutputLogInterval && outputdecadestep <= 0 ) {

    ConsoleErrorMessage ( __freqlogdecade, "Logarithmic number of steps per decade should be greater than 0!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

vector<interval>    outputbands         = GetCLIOptionIntervals ( freqan, __freqbands     );
TFixedString<KiloByte>  freqbands;


if ( outputinterval == OutputBands ) {
    
    if ( outputbands.empty () ) {

        ConsoleErrorMessage ( __freqbands, "Frequency Bands appear to be empty!" );
        return;
        }

    for ( const auto& i : outputbands ) {

        if ( ! IsInsideLimits ( i.first, (double) 0, nyquist ) ) {
            ConsoleErrorMessage ( __freqbands, "Frequency band first value '", FloatToString ( i.first ), "' is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
            return;
            }
        if ( ! IsInsideLimits ( i.second, (double) 0, nyquist ) ) {
            ConsoleErrorMessage ( __freqbands, "Frequency band second value '", FloatToString ( i.second ), "' is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
            return;
            }
        }

                                        // we have to manually concatenate all bands back into a single string
    for ( const auto& s : GetCLIOptionStrings ( freqan, __freqbands ) )
        freqbands  += ( freqbands.IsEmpty () ? "" : "," ) + TFixedString<KiloByte> ( s.c_str () );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsSTMethod ( analysis ) ) {
                                        // __sequential is implicit, but still allowed
    if ( HasCLIFlag ( freqan, __average ) ) {

        ConsoleErrorMessage ( 0, "You can not specify the ", __average, " option for the S-Transform!" );
        return;
        }
    }

else {

    if ( ! HasCLIFlag ( freqan, __sequential )
      && ! HasCLIFlag ( freqan, __average    ) ) {

        ConsoleErrorMessage ( 0, "You need to specify either ", __sequential, " or ", __average, " output for the FFT!" );
        return;
        }
    }

bool                outputsequential    = HasCLIFlag ( freqan, __sequential )
                                       || IsSTMethod ( analysis );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                outputmarkers       = outputsequential;

MarkerType          outputmarkerstype   = AllMarkerTypes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FreqWindowingType   windowing           = HasCLIFlag ( freqan, __windowinghanning ) ? IsSTMethod ( analysis ) ? FreqWindowingHanningBorder
                                                                                                              : FreqWindowingHanning
                                                                                    :                           FreqWindowingNone;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( ! HasCLIOption ( freqan, __rescaling ) ) {
//
//    ConsoleErrorMessage ( __rescaling, "Rescaling has to be specified!" );
//    return;
//    }

string              rescaling           = GetCLIOptionEnum ( freqan, __rescaling );

FFTRescalingType    fftnorm             = rescaling == __rescalingnone ? FFTRescalingNone
                                        : rescaling == __rescalingsqrt ? FFTRescalingSqrtWindowSize
                                        :                                FFTRescalingWindowSize;    // default is Parseval


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              outputtype          = GetCLIOptionEnum ( freqan, __output );

FreqOutputAtomType  outputatomtype      = outputtype == __outputreal    ? OutputAtomReal
                                        : outputtype == __outputnorm    ? OutputAtomNorm
                                        : outputtype == __outputpower   ? OutputAtomNorm2
                                        : outputtype == __outputcomplex ? OutputAtomComplex
                                        : outputtype == __outputphase   ? OutputAtomPhase
                                        :                               ( IsFFTApproxMethod ( analysis ) ? OutputAtomReal : OutputAtomNorm2 ); // default is power


const char*         aac                 = AnalysisAtomtypeCompatibility (   analysis, 
                                                                            HasCLIOption ( freqan, __freqbands ), 
                                                                            ! outputsequential, 
                                                                            outputatomtype      );

if ( StringIsNotEmpty ( aac ) ) {

    ConsoleErrorMessage ( 0, aac );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              reflist         = GetCLIOptionString ( freqan, __reference );

ReferenceType       ref             = reflist == "none"    || reflist == "asinfile" ? ReferenceAsInFile
                                    : reflist == "average" || reflist == "avgref"   ? ReferenceAverage
                                    : ! reflist.empty ()                            ? ReferenceArbitraryTracks
                                    : IsSurfaceCase ( analysis )                    ? ReferenceAverage          // default for surface case
                                    :                                                 ReferenceAsInFile;        // default for other cases

                                        // force overriding?
//if ( IsPowerMaps ( analysis ) ) {
//    
//    if ( ref != ReferenceAverage ) {
//        ConsoleErrorMessage ( __reference, "Power Maps analysis need an Average Reference!" );
//        return;
//        }
//    }


//CheckReference ( ref, datatypein );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              infix           = GetCLIOptionString ( freqan, __infix );

bool                subdir          = HasCLIFlag ( freqan, __subdir        );


bool                savingfreq      = HasCLIFlag ( freqan, __savingfreq    );
bool                splitelec       = HasCLIFlag ( freqan, __splitelec     );
bool                splitfreq       = HasCLIFlag ( freqan, __splitfreq     );
bool                splitspectrum   = HasCLIFlag ( freqan, __splitspectrum );

bool                savefftapprox   = IsFFTApproxMethod ( analysis );

if ( ! ( savingfreq || splitelec || splitfreq || splitspectrum || savefftapprox ) ) {

    ConsoleErrorMessage ( 0, "No output files has been selected!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasCLIFlag ( freqan, __downsampling ) && ! IsSTMethod ( analysis ) ) {

    ConsoleErrorMessage ( __downsampling, "Optimal downsampling is only compatible with STransform type of analysis!" );
    return;
    }

bool                optimaldownsampling = HasCLIFlag ( freqan, __downsampling );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();


TVerboseFile        verbose ( "cout", VerboseFileDefaultWidth );

verbose.NextTopic ( "Analysis:" );
{
verbose.Put ( "Analysis general case:   ", GetCaseString   ( analysis ) );
verbose.Put ( "Analysis method:         ", GetMethodString ( analysis ) );
verbose.Put ( "Power Maps analysis case:", IsPowerMaps     ( analysis ) );  // we can be more specific here
verbose.Put ( "Output data is:          ", outputsequential ? "Sequential" : "Averaged" );

verbose.NextLine ();

verbose.Put ( "Windowing function:", FreqWindowingString[ windowing ] );

if ( ! IsSTMethod ( analysis ) )
    verbose.Put ( "FFT Time Windows rescaling:", FFTRescalingString[ fftnorm ] );

verbose.Put ( "Output values:", FreqOutputAtomString[ outputatomtype ] );
}


verbose.NextTopic ( "Tracks to analyse:" );
{
verbose.Put ( "Tracks to analyze:", tracks.empty () ? "All" : tracks );
}


verbose.NextTopic ( "Reference of data:" );
{
verbose.Put ( "Reference:", ReferenceNames[ ref ] );

if ( ref == ReferenceArbitraryTracks )
    verbose.Put ( "Reference list:", reflist );
}


verbose.NextTopic ( "Time windows:" );
{
verbose.Put ( "From Time Frame:", timemin == 0                   ? "Beginning of file" : IntegerToString ( timemin ) );
verbose.Put ( "To   Time Frame:", timemax == Highest ( timemax ) ? "End of file"       : IntegerToString ( timemax ) );

if ( ! IsSTMethod ( analysis ) ) {

    verbose.NextLine ();
    verbose.Put ( "Windows size:", blocksize, 0, " [TF]" );
    //verbose.Put ( "Windows step:", blockstep );
    verbose.Put ( "Windows step:", blocksoverlap == 0.00 ? "100% Window"    // transform overlap back to jump
                                 : blocksoverlap == 0.75 ? "25% Window" 
                                 :                         "1 TF"       );  // blocksoverlap = ( blocksize - 1 ) / blocksize

    //verbose.Put ( "Windows overlap:", blocksoverlap * 100, 2, "%" );
    }

verbose.NextLine ();
verbose.Put ( "Excluding bad epochs:", SkippingEpochsNames[ badepochs ] );
if ( badepochs == SkippingBadEpochsList )
    verbose.Put ( "Skipping markers:", listbadepochs );
}


verbose.NextTopic ( "Frequencies:" );
{
verbose.Put ( "Sampling frequency:", samplingfrequency, -1, " [Hz]" );

verbose.NextLine ();
if          ( outputinterval == OutputLinearInterval )  verbose.Put ( "Saving:", "Linear Interval" );
else if     ( outputinterval == OutputLogInterval    )  verbose.Put ( "Saving:", "Log Interval" );
else if     ( outputinterval == OutputBands          )  verbose.Put ( "Saving:", "Frequency Bands" );
else                                                    verbose.Put ( "Saving:", "Unknown" );

if          ( outputinterval == OutputLinearInterval ) {

    verbose.Put ( "Frequency min  (requested):", freqmin, -1, " [Hz]" );
    verbose.Put ( "Frequency max  (requested):", freqmax, -1, " [Hz]" );
    verbose.Put ( "Frequency step (requested):", outputfreqstep, -1, " [Hz]" );
    }

else if     ( outputinterval == OutputLogInterval ) {

    verbose.Put ( "Frequency min  (requested):", freqmin, -1, " [Hz]" );
    verbose.Put ( "Frequency max  (requested):", freqmax, -1, " [Hz]" );
    verbose.Put ( "Number of frequencies per Log Decade:", outputdecadestep );
    }

else if     ( outputinterval == OutputBands ) {

    verbose.Put ( "Number of frequency bands:", (int) outputbands.size () );
    verbose.Put ( "Frequency bands:", freqbands );

    //for ( const auto& i : outputbands )
    //    verbose.Put ( "Freq Bands: ", i.first, " to ", i.second );
    }
}


verbose.NextTopic ( "Input files:" );
{
verbose.Put ( "Number of input files:", (int) gof );

for ( int i = 0; i < (int) gof; i++ )
    verbose.Put ( "Input file:", gof[ i ] );

verbose.Put ( "Electrodes coordinates file:", xyzfile.IsNotEmpty () ? xyzfile : "None" );
}


verbose.NextTopic ( "Options:" );
{
verbose.Put ( "File name infix:",                       infix );
verbose.Put ( "Creating sub-directory for results:",    subdir );

verbose.NextLine ();
verbose.Put ( "Saving all freqs into a single file:",   savingfreq );
verbose.Put ( "Splitting results per electrode:",       splitelec );
verbose.Put ( "Splitting results per frequency:",       splitfreq );
verbose.Put ( "Splitting results per spectrum :",       splitspectrum );
//verbose.Put ( "Saving FFT Approximation:",              savefftapprox );

verbose.NextLine ();
verbose.Put ( "Optimally downsampling the file:",       optimaldownsampling );
}


verbose.Close ();

cout << NewLine;
cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fileoutfreq;
TFileName           fileoutsplitelec;
TFileName           fileoutsplitfreq;
TFileName           fileoutspectrum;
TFileName           fileoutapprfreqs;


for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Now Processing: " << gof  [ filei ]<< NewLine;
#endif

    TOpenDoc<TTracksDoc>    EEGDoc ( gof[ filei ], OpenDocHidden );


    FrequencyAnalysis   (   EEGDoc,
                            xyzfile,
                            analysis,
                            tracks.c_str (),
                            ref,                reflist.c_str (),
                            timemin,            timemax,
                            badepochs,          listbadepochs,
                            samplingfrequency,
                            blocksize,          blocksoverlap,
                            fftnorm,
                            outputinterval,
                            outputatomtype,
                            outputmarkers,      outputmarkerstype,
                            freqbands,                              // providing the whole list of intervals as a single string
                            freqmin,            freqmax,
                            outputfreqstep,     
                            outputdecadestep,
                            outputsequential,
                            windowing,
                            optimaldownsampling,
                            infix.c_str (),     subdir,
                            savingfreq    ? (char*) fileoutfreq        : (char*) 0,
                            splitelec     ? (char*) fileoutsplitelec   : (char*) 0,
                            splitfreq     ? (char*) fileoutsplitfreq   : (char*) 0,
                            splitspectrum ? (char*) fileoutspectrum    : (char*) 0,
                            savefftapprox ? (char*) fileoutapprfreqs   : (char*) 0,
                            Silent
                        );

    } // for file


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
