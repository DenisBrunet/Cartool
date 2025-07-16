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
                                        // Parameters appearance follow the dialog's visual design
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

DefineCLIOptionEnum     ( freqan,   "",     __analysis,             "Type of analysis" RequiredString )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __analysispowermaps, __analysisfft, __analysisfftapprox, __analysisstransform } ) ) );

DefineCLIFlag           ( freqan,   "",     __windowinghanning,     "Hanning Windowing" );

DefineCLIFlag           ( freqan,   "",     __sequential,           "Sequential output" );
DefineCLIFlag           ( freqan,   "",     __average,              "Average output" );
ExcludeCLIOptions       ( freqan,     __sequential,     __average );


DefineCLIOptionEnum     ( freqan,   "",     __rescaling,            "Window rescaling (Default:parseval)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __rescalingnone, __rescalingsqrt, __rescalingsize } ) ) );


DefineCLIOptionEnum     ( freqan,   "",     __output,               "Output type (Default:power)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __outputreal, __outputnorm, __outputpower, __outputcomplex, __outputphase } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( freqan,   "",     __reference,            __reference_descr )
->TypeOfOption          ( "TRACKS" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( freqan,   "",     __infix,                __infix_descr );

DefineCLIFlag           ( freqan,   "",     __subdir,               __subdir_descr );
DefineCLIFlag           ( freqan,   "",     __savingfreq,           "Saving .freq files" );
DefineCLIFlag           ( freqan,   "",     __splitelec,            "Splitting results by electrodes" );
DefineCLIFlag           ( freqan,   "",     __splitfreq,            "Splitting results by frequencies" );
DefineCLIFlag           ( freqan,   "",     __splitspectrum,        "Splitting results by spectrum" );
DefineCLIFlag           ( freqan,   "",     __downsampling,         "Optimally downsampling time for STransform" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( freqan,     __h,    __help,               __help_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     FrequencyAnalysisCLI ( CLI::App* freqan, const TGoF& gof )
{
if ( ! IsSubCommandUsed ( freqan )  )
    return;


if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
if ( ! HasCLIOption ( freqan, __preset ) ) {

    ConsoleErrorMessage ( __preset, "No preset specified!" );
    return;
    }


string              preset          = GetCLIOptionEnum ( freqan, __preset );

ComputingRisPresetsEnum esicase     = preset == __preset1 ? ComputingRisPresetErpGroupMeans
                                    : preset == __preset2 ? ComputingRisPresetErpIndivMeans
                                    : preset == __preset3 ? ComputingRisPresetErpIndivEpochs
                                    : preset == __preset4 ? ComputingRisPresetErpFitClusters
                                    : preset == __preset5 ? ComputingRisPresetIndIndivEpochs
                                    : preset == __preset6 ? ComputingRisPresetSpont
                                    : preset == __preset7 ? ComputingRisPresetSpontClusters
                                    : preset == __preset8 ? ComputingRisPresetFreq
                                    :                       ComputingRisPresetSeparator1;       // null preset

if ( CRISPresets[ esicase ].Flags == CRISPresetNone ) {

    ConsoleErrorMessage ( __preset, "Wrong preset!" );
    return;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! HasCLIOption ( freqan, __analysis ) ) {

    ConsoleErrorMessage ( __analysis, "Type of analysis is not specified!" );
    return;
    }

string              typeanalysis    = GetCLIOptionEnum ( freqan, __analysis );

FreqAnalysisType    analysis        = typeanalysis == __analysispowermaps   ? FreqAnalysisPowerMaps         // most specific case first
                                    : typeanalysis == __analysisfft         ? FreqAnalysisFFT
                                    : typeanalysis == __analysisfftapprox   ? FreqAnalysisFFTApproximation
                                    :                                         FreqAnalysisSTransform;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              tracks          = GetCLIOptionString ( freqan, __tracks );

TFileName           xyzfile         = GetCLIOptionFile   ( freqan, __xyzfile );


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

if ( analysis != FreqAnalysisSTransform ) {
    
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


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              blockstep;
double              blocksoverlap   = 0;

if ( analysis != FreqAnalysisSTransform ) {
    
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


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              nyquist         = GetNyquist ( samplingfrequency, analysis == FreqAnalysisSTransform );


if ( ! HasCLIOption ( freqan, __freqmin ) ) {

    ConsoleErrorMessage ( __freqmin, "Frequency min is not specified!" );
    return;
    }

double              freqmin         = GetCLIOptionDouble ( freqan, __freqmin );

if ( ! IsInsideLimits ( freqmin, (double) 0, nyquist ) ) {

    ConsoleErrorMessage ( __freqmin, "Frequency min is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
    return;
    }


if ( ! HasCLIOption ( freqan, __freqmax ) ) {

    ConsoleErrorMessage ( __freqmax, "Frequency max is not specified!" );
    return;
    }

double              freqmax         = GetCLIOptionDouble ( freqan, __freqmax );

if ( ! IsInsideLimits ( freqmax, (double) 0, nyquist ) ) {

    ConsoleErrorMessage ( __freqmax, "Frequency max is not in range [0..", FloatToString ( nyquist, 2 ), "]" );
    return;
    }


if ( freqmin >= freqmax ) {

    ConsoleErrorMessage ( 0, "Frequency max is equal or lower than Frequency min!" );
    return;
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

double              outputfreqstep      = GetCLIOptionDouble    ( freqan, __freqlinstep   );
int                 outputdecadestep    = GetCLIOptionInt       ( freqan, __freqlogdecade );
vector<interval>    outputbands         = GetCLIOptionIntervals ( freqan, __freqbands     );


if ( HasCLIOption ( freqan, __freqlinstep ) && outputfreqstep <= 0 ) {

    ConsoleErrorMessage ( __freqlinstep, "Linear interval step should be greater than 0!" );
    return;
    }


if ( HasCLIOption ( freqan, __freqlogdecade ) && outputdecadestep <= 0 ) {

    ConsoleErrorMessage ( __freqlogdecade, "Logarithmic number of steps per decade should be greater than 0!" );
    return;
    }


if ( HasCLIOption ( freqan, __freqbands ) ) {
    
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
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( analysis == FreqAnalysisSTransform ) {

    if ( HasCLIFlag ( freqan, __sequential )
      || HasCLIFlag ( freqan, __average    ) ) {

        ConsoleErrorMessage ( 0, "You do not need to specify ", __sequential, " or ", __average, " options for the S-Transform!" );
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
                                       || analysis == FreqAnalysisSTransform;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                outputmarkers       = outputsequential;

MarkerType          outputmarkerstype   = AllMarkerTypes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FreqWindowingType   windowing           = HasCLIFlag ( freqan, __windowinghanning ) ? analysis != FreqAnalysisSTransform ? FreqWindowingHanning 
                                                                                                                         : FreqWindowingHanningBorder 
                                                                                    :                                      FreqWindowingNone;

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
                                        :                               ( analysis == FreqAnalysisFFTApproximation ? OutputAtomReal : OutputAtomNorm2 ); // default is power


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

ReferenceType       ref             = reflist.empty ()
                                   || reflist == "none"
                                   || reflist == "asinfile"             ? ReferenceAsInFile
                                    : reflist == "average"
                                   || reflist == "avgref"               ? ReferenceAverage
                                    :                                     ReferenceArbitraryTracks;


//CheckReference ( ref, datatypein );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              infix           = GetCLIOptionString ( freqan, __infix );

bool                subdir          = HasCLIFlag ( freqan, __subdir        );


bool                savingfreq      = HasCLIFlag ( freqan, __savingfreq    );
bool                splitelec       = HasCLIFlag ( freqan, __splitelec     );
bool                splitfreq       = HasCLIFlag ( freqan, __splitfreq     );
bool                splitspectrum   = HasCLIFlag ( freqan, __splitspectrum );

bool                savefftapprox   = analysis == FreqAnalysisFFTApproximation;

if ( ! ( savingfreq || splitelec || splitfreq || splitspectrum || savefftapprox ) ) {

    ConsoleErrorMessage ( 0, "No output files has been selected!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasCLIFlag ( freqan, __downsampling ) && analysis != FreqAnalysisSTransform ) {

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

cout << "Tracks:            " << ( tracks.empty () ? "All" : tracks ) << NewLine;
if ( xyzfile.IsNotEmpty () )
    cout << "XYZ file:          " << xyzfile << NewLine;

cout << "Time Min:          " << ( timemin == 0                   ? "Beginning of file" : IntegerToString ( timemin ) ) << NewLine;
cout << "Time Max:          " << ( timemax == Highest ( timemax ) ? "End of file"       : IntegerToString ( timemax ) ) << NewLine;

if ( badepochs == SkippingBadEpochsList )
    cout << "Exclude Triggers:  " << listbadepochs << NewLine;

cout << NewLine;

cout << "Analysis:          " << typeanalysis << NewLine;
cout << "Sampling Frequency:" << samplingfrequency << NewLine;
if ( analysis != FreqAnalysisSTransform ) {
    cout << "Window Size:       " << blocksize << NewLine;
    cout << "Window Step:       " << blockstep << NewLine;
    cout << "Window Overlap:    " << ( blocksoverlap * 100 ) << "%" << NewLine;
    }
cout << "Frequency Min:     " << freqmin << NewLine;
cout << "Frequency Max:     " << freqmax << NewLine;
if      ( outputinterval == OutputLinearInterval )  cout << "Linear Interv Step:" << outputfreqstep << NewLine;
else if ( outputinterval == OutputLogInterval    )  cout << "Log, # per Decade: " << outputdecadestep << NewLine;
else if ( outputinterval == OutputBands          ) {
    cout << "Num Freq Bands:    " << outputbands.size () << NewLine;
    for ( const auto& i : outputbands )
        cout << "Freq Bands:        " << i.first << " to " << i.second << NewLine;
    }

cout << NewLine;

cout << "Sequential output: " << BoolToString ( outputsequential ) << NewLine;
cout << "Windowing:         " << FreqWindowingString    [ windowing ] << NewLine;
cout << "Rescaling:         " << FFTRescalingString     [ fftnorm ] << NewLine;
cout << "Output:            " << FreqOutputAtomString   [ outputatomtype ] << NewLine;
cout << "Reference:         " << ReferenceNames         [ ref ] << NewLine;
if ( ref == ReferenceArbitraryTracks )
    cout << "Reference list:    " << reflist << NewLine;

cout << NewLine;

cout << "Infix:             " << infix << NewLine;
cout << "Sub-Directory:     " << BoolToString ( subdir              ) << NewLine;
cout << "Saving .freq:      " << BoolToString ( savingfreq          ) << NewLine;
cout << "Split Electrodes:  " << BoolToString ( splitelec           ) << NewLine;
cout << "Split Frequencies: " << BoolToString ( splitfreq           ) << NewLine;
cout << "Split Spectrum:    " << BoolToString ( splitspectrum       ) << NewLine;
cout << "Saving FFT Approx: " << BoolToString ( savefftapprox       ) << NewLine;
cout << "Optimal Downsampl: " << BoolToString ( optimaldownsampling ) << NewLine;

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
    cout << "Processing:        " << gof  [ filei ]<< NewLine;
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
                            GetCLIOptionString ( freqan, __freqbands ).c_str (),    // provide the whole list of intervals as a single string
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
