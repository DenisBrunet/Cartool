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
DefineCLIOptionString   ( freqan,   "",     __tracks,               "Tracks to analyze" )
->TypeOfOption          ( "TRACKS" );

DefineCLIOptionFile     ( freqan,   "",     __xyzfile,              __xyzfile_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( freqan,   "",     __timemin,              __timemin_descr );
DefineCLIOptionInt      ( freqan,   "",     __timemax,              __timemax_descr );

DefineCLIOptionString   ( freqan,   "",     __excludetriggers,      __excludetriggers_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionDouble   ( freqan,   "",     __samplingfrequency,    __samplingfrequency_descr )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "sampling frequency should be above 0" : ""; } );

DefineCLIOptionInt      ( freqan,   "",     __windowsize,           "Window Size, in [TF]" );

DefineCLIOptionEnum     ( freqan,           "",     __windowstep,           "Window Step" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __windowstep1, __windowstep25, __windowstep100 } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionDouble   ( freqan,   "",     __freqmin,              "Lowest frequency" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );

DefineCLIOptionDouble   ( freqan,   "",     __freqmax,              "Highest frequency" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );

DefineCLIOptionDouble   ( freqan,   "",     __freqlinstep,          "Saving a Linear Interval, with Frequency step" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );

DefineCLIOptionInt      ( freqan,   "",     __freqlogdecade,        "Saving a Log Interval, with Number per Decade" )
->DefaultInteger        ( DefaultSaveLogDecade );

// add interval type?
DefineCLIOptionStrings  ( freqan,   -1, "",     __freqbands,        "Saving a list of Frequency Bands (f.ex. 1-4;4-8;8-14)" )
->TypeOfOption          ( "INTERVALS" );

ExcludeCLIOptions       ( freqan,     __freqlinstep,    __freqlogdecade,    __freqbands );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( freqan,   "",     __analysis,             "Type of analysis" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __analysisfft, __analysisfftapprox, __analysisstransform } ) ) );

DefineCLIFlag           ( freqan,   "",     __windowinghanning,     "Hanning Windowing" );

DefineCLIFlag           ( freqan,   "",     __sequential,           "Sequential output" );
DefineCLIFlag           ( freqan,   "",     __average,              "Average output" );
ExcludeCLIOptions       ( freqan,     __sequential,     __average );


DefineCLIOptionEnum     ( freqan,   "",     __rescaling,            "Window rescaling" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __rescalingnone, __rescalingsqrt, __rescalingsize } ) ) );


DefineCLIOptionEnum     ( freqan,   "",     __output,               "Output type" )
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

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
