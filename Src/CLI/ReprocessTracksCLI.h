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

#include    "System.CLI11.h"
#include    "CLIDefines.h"

#include    "Strings.TFixedString.h"
#include    "Files.TOpenDoc.h"
#include    "TExportTracks.h"
#include    "ReprocessTracks.h"

#include    "TElectrodesDoc.h"
#include    "TRoisDoc.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Defining the interface
inline void     ReprocessTracksCLIDefine ( CLI::App* reprocsub )
{
if ( reprocsub == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters appearance follow the dialog's visual design
DefineCLIOptionString   ( reprocsub,        "",     __tracks,               "Tracks to export" Tab Tab Tab Tab "Special values: * gfp dis avg" )
->TypeOfOption          ( "TRACKS" );

DefineCLIOptionFile     ( reprocsub,        "",     __xyzfile,              __xyzfile_descr );
DefineCLIOptionFile     ( reprocsub,        "",     __roisfile,             __roisfile_descr );

ExcludeCLIOptions       ( reprocsub,        __xyzfile,      __roisfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( reprocsub,        "",     __timemin,              __timemin_descr );
DefineCLIOptionInt      ( reprocsub,        "",     __timemax,              __timemax_descr );

DefineCLIOptionString   ( reprocsub,        "",     __keeptriggers,         __keeptriggers_descr );
DefineCLIOptionString   ( reprocsub,        "",     __excludetriggers,      __excludetriggers_descr );

ExcludeCLIOptions       ( reprocsub,        __timemin,          __keeptriggers,     __excludetriggers );
ExcludeCLIOptions       ( reprocsub,        __timemax,          __keeptriggers,     __excludetriggers );
ExcludeCLIOptions       ( reprocsub,        __keeptriggers,     __excludetriggers );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     __nulltracks,           "List of null tracks to append" )
->TypeOfOption          ( "TRACKS" );

ExcludeCLIOptions       ( reprocsub,        __nulltracks,   __roisfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reprocess Tracks filters options
DefineCLIOptionString   ( reprocsub,        "",     __filters,              "A whole string of filtering options, in double quote, as coming from a verbose file" )
                                        // tricky check to raise an error if filters string was not provided: CLI11 will use the next option as a string, so we can simply test for "-" or "--"
->CheckOption           ( []( const string& str ) { return str.find ( "-", 0 ) == 0 ? "string must be in double quotes" : ""; } );


DefineCLIFlag           ( reprocsub,        __dc,   __baseline,             "Baseline / DC correction (recommended with any High-Pass or Band-Pass filter)" );

                                        // !We allow the simultaneous use of highpass and lowpass, which will be combined into a single bandpass!
DefineCLIOptionDouble   ( reprocsub,        "",     __highpass,             "High-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );
                                        // Testing High-Pass < Low-Pass here - NOT working anymore
//->CheckOption           ( [ &reprocsub ]( const string& str )
//    {   
//    double      cuth            = StringToDouble ( str.c_str () );
//    bool        hasl            = HasCLIOption       ( reprocsub, __lowpass );
//    double      cutl            = GetCLIOptionDouble ( reprocsub, __lowpass );
//
//    return         cuth <= 0
//        || hasl && cutl <= 0    ? "frequency cut should be above 0"
//         : hasl && cuth > cutl  ? "high-pass cut should be less than the low-pass cut"
//         :                        ""; 
//    } );

DefineCLIOptionDouble   ( reprocsub,        "",     __lowpass,              "Low-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );

                                        // Bandpass, however, excludes both highpass and lowpass
DefineCLIOptionDoubles  ( reprocsub,    2,  "",     __bandpass,             "Band-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );
ExcludeCLIOptions       ( reprocsub,        __bandpass,         __highpass,     __lowpass );


DefineCLIOptionInt      ( reprocsub,        "",     __order,                "Butterworth filter order" )
->DefaultString         ( string ( IntegerToString ( TFilterDefaultOrder ) ) + " or " + string ( IntegerToString ( 2 * TFilterDefaultOrder )  ) )
->CheckOption           ( [ &reprocsub ]( const string& str )   
    {   int         o               = StringToInteger ( str.c_str () );
        bool        orderevenok     = IsEven         ( o );
        bool        orderrangeok    = IsInsideLimits ( o, TFilterMinOrder, TFilterMaxOrder );
        return ! orderevenok || ! orderrangeok  ? string ( "Value should be either a multiple of 2 (highpass or lowpass), or 4 (bandpass), and within the range " ) + TFilterMinOrderString + ".." + TFilterMaxOrderString
             :                                    ""; 
    } );
//->CheckOption           ( [ &reprocsub ]( const string& str )   
//    {   int         o               = StringToInteger ( str.c_str () );
//        bool        optionok        = HasCLIOption ( reprocsub,__highpass ) || HasCLIOption ( reprocsub,__lowpass  ) || HasCLIOption ( reprocsub,__bandpass );
//        bool        orderevenok     = IsEven         ( o );
//        bool        orderrangeok    = IsInsideLimits ( o, TFilterMinOrder, TFilterMaxOrder );
//        return ! optionok                       ? __order requires one of the following {__highpass, __lowpass, bandpass} option"
//             : ! orderevenok || ! orderrangeok  ? string ( "Value should be an even number, and within the range " ) + TFilterMinOrderString + ".." + TFilterMaxOrderString
//             :                                    ""; 
//    } );


DefineCLIFlag           ( reprocsub,        "",     __causal,               "Causal filters, using only forward filtering" );

DefineCLIOptionDoubles  ( reprocsub,    -1, "",     __notches,              "Notches filter" );
DefineCLIFlag           ( reprocsub,        "",     __harmonics,            "Adding Notches harmonics" );
NeedsCLIOption          ( reprocsub,        __harmonics,    __notches );


DefineCLIOptionEnum     ( reprocsub,        "",     __spatialfilter,        "Spatial filter" );
NeedsCLIOption          ( reprocsub,        __spatialfilter,    __xyzfile )
                                            // Case sensitive, but allows a nice listing when requesting Help
->DefaultString         ( SpatialFilterShortName[ SpatialFilterDefault ] )
->CheckOption           ( CLI::IsMember ( vector<string> ( SpatialFilterShortName + SpatialFilterOutlier, SpatialFilterShortName + NumSpatialFilterTypes ) ) )
                                            // Allowing for case insensitive (and even the long names, although not advertized), but Help is not helping
//->CheckOption         ( []( const string& str )   {   string      allspatial  = "{"; 
//                                                      for ( int i = SpatialFilterOutlier; i < NumSpatialFilterTypes; i++ )
//                                                          allspatial  += string ( SpatialFilterShortName[ i ] ) + string ( i < NumSpatialFilterTypes - 1 ? ", " : "" ); 
//                                                      allspatial += "}";
//                                                      return  TextToSpatialFilterType ( str.c_str () ) == SpatialFilterNone ? string ( "Spatial Filter should be in " ) + allspatial : ""; } )
->ZeroOrOneArgument;


DefineCLIFlag           ( reprocsub,        "",     __ranking,              "Ranking data at each time point" );

DefineCLIOptionEnum     ( reprocsub,        "",     __rectification,        "Rectification, i.e. making data all positive" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { "abs", "absolute", "power", "squared" } ) ) );

DefineCLIOptionDouble   ( reprocsub,        "",     __envelope,             "Envelope filter on positive data (after rectification), value in [ms]" );
NeedsCLIOption          ( reprocsub,        __envelope,     __rectification )   // dialog does not enforce this, just in case data is already positive, but we do here
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "envelope window, in [ms], should be positive" : ""; } );

DefineCLIOptionDouble   ( reprocsub,        "",     __keepabove,            "Thresholding data, keeping data above value" );
DefineCLIOptionDouble   ( reprocsub,        "",     __keepbelow,            "Thresholding data, keeping data below value" );

DefineCLIOptionDouble   ( reprocsub,        "",     __samplingfrequency,    __samplingfrequency_descr )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "sampling frequency should be above 0" : ""; } );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     __reference,            __reference_descr )
->TypeOfOption          ( "TRACKS" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInts     ( reprocsub,    2,  "",     __baselinecorr,         "Baseline correction interval, in time frames since beginning of file" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     __rescaling,            "Scaling factor" Tab Tab Tab Tab "Special value: 'meangfp'" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "",     __sequential,           "Sequential output" Tab Tab Tab Tab "Default option" );
DefineCLIFlag           ( reprocsub,        "",     __average,              "Averaging the time dimension" );
ExcludeCLIOptions       ( reprocsub,        __sequential,   __average );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( reprocsub,        "",     __downsampling,         "Downsampling ratio" )
->CheckOption           ( []( const string& str ) { return StringToInteger ( str.c_str () ) <= 1 ? "factor should be above 1" : ""; } );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     __infix,                __infix_descr );

DefineCLIOptionEnum     ( reprocsub,        __ext,  __extension,            __ext_descr )
->DefaultString         ( SavingEegFileExtPreset[ PresetFileTypeDefaultEEG ] )
->CheckOption           ( CLI::IsMember ( vector<string> ( SavingEegFileExtPreset, SavingEegFileExtPreset + NumSavingEegFileTypes ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "",     __nomarkers,            "Not saving the markers to file" );

DefineCLIFlag           ( reprocsub,        "",     __concatenate,          "Concatenate all output into a single file" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        __h,    __help,                 __help_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     ReprocessTracksCLI ( CLI::App* reprocsub, const TGoF& gof )
{
if ( ! IsSubCommandUsed ( reprocsub )  )
    return;


if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tracks parameters

TFileName           roisfile        = GetCLIOptionFile ( reprocsub, __roisfile );
TFileName           xyzfile         = GetCLIOptionFile ( reprocsub, __xyzfile  );

TracksOptions       tracksoptions   = roisfile.IsNotEmpty () ? ProcessRois 
                                    :                          ProcessTracks;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Wraps everything for us: existing file name & opening doc
TOpenDoc<TElectrodesDoc>    XYZDoc  ( xyzfile,  OpenDocHidden );
TOpenDoc<TRoisDoc>          RoisDoc ( roisfile, OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              tracks          = GetCLIOptionString ( reprocsub, __tracks );

string              nulltracks      = GetCLIOptionString ( reprocsub, __nulltracks );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters, exclusively between time interval, triggers to keep or triggers to exclude
                                        // default values
TimeOptions         timeoptions     = ExportTimeInterval;
long                timemin         = 0;                      
long                timemax         = Highest ( timemax );    
string              keeptriggers;
string              excludetriggers;


if      ( HasCLIOption ( reprocsub, __timemin )
       || HasCLIOption ( reprocsub, __timemax ) ) {

    timeoptions     = ExportTimeInterval;

    if  ( HasCLIOption ( reprocsub, __timemin ) )
        timemin     = GetCLIOptionInt ( reprocsub, __timemin );

    if  ( HasCLIOption ( reprocsub, __timemax ) )
        timemax     = GetCLIOptionInt ( reprocsub, __timemax );

    CheckOrder ( timemin, timemax );
    }

else if ( HasCLIOption ( reprocsub, __keeptriggers ) ) {

    timeoptions     = ExportTimeTriggers;
    keeptriggers    = GetCLIOptionString ( reprocsub, __keeptriggers );
    }

else if ( HasCLIOption ( reprocsub, __excludetriggers ) ) {

    timeoptions     = ExcludeTimeTriggers;
    excludetriggers = GetCLIOptionString ( reprocsub, __excludetriggers );
    }

else // no options provided
    timeoptions     = ExportTimeInterval;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For total safety, it is convenient to have a default sampling frequency at hand, which could be passed to EEGs that lack one
double              defaultsamplingfrequency    = 0;

                                        // First see if caller provided one
if ( HasCLIOption ( reprocsub, __samplingfrequency ) )

    defaultsamplingfrequency    = GetCLIOptionDouble ( reprocsub, __samplingfrequency );

                                        // Still null?
if ( defaultsamplingfrequency == 0 )
                                        // Loop through all files and stop at first positive sampling frequency
    for ( int filei = 0; filei < (int) gof; filei++ )

        if ( ReadFromHeader ( gof[ filei ],  ReadSamplingFrequency,    &defaultsamplingfrequency )
          && defaultsamplingfrequency > 0 )

            break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filters
TTracksFilters<float>   altfilters;
TTracksFiltersStruct&   fp              = altfilters.FiltersParam;


                                        // 1) Converting a whole optional string, as coming from the verbose output, to parameters
if ( HasCLIOption ( reprocsub, __filters ) ) {

    string              filters         = GetCLIOptionString ( reprocsub, __filters );

    altfilters.TextToParameters ( filters.c_str (), XYZDoc.IsOpen () ? XYZDoc->GetDocPath () : 0, defaultsamplingfrequency );
    }

                                        // 2) Setting or overwriting each parameter, one at a time - Heavily based on TTracksFilters::TextToParameters method
                                        // We currently allow highpass + lowpass parameters to define a bandpass
if      ( HasCLIOption ( reprocsub, __bandpass ) 
       || HasCLIOption ( reprocsub, __highpass ) && HasCLIOption ( reprocsub, __lowpass  ) ) {

    string      hs;
    string      ls;

    if ( HasCLIOption ( reprocsub, __bandpass ) ) {

        vector<string>      band            = GetCLIOptionStrings ( reprocsub, __bandpass );
        
        hs      = band[ 0 ];
        ls      = band[ 1 ];
        }
    else {
        hs      = GetCLIOptionString ( reprocsub, __highpass );
        ls      = GetCLIOptionString ( reprocsub, __lowpass  );
        }

                                        // check order here, as CLI11 check method seems to crash
    double      h   = StringToDouble ( hs.c_str () );
    double      l   = StringToDouble ( ls.c_str () );

    fp.SetButterworthHigh ( h < l ? hs.c_str () : ls.c_str () );
    fp.SetButterworthLow  ( h < l ? ls.c_str () : hs.c_str () );
    }

else if ( HasCLIOption ( reprocsub, __highpass ) )

    fp.SetButterworthHigh ( GetCLIOptionString ( reprocsub, __highpass ).c_str () );

else if ( HasCLIOption ( reprocsub, __lowpass ) )

    fp.SetButterworthLow  ( GetCLIOptionString ( reprocsub, __lowpass ).c_str () );


if ( HasCLIOption ( reprocsub, __order ) ) {

    int                 order           = GetCLIOptionInt ( reprocsub, __order );

    if ( fp.IsBandPass () )
        order  /= 2;                // set half the band-pass order
    
    order   = Clip ( RoundToEven ( order ), TFilterMinOrder, TFilterMaxOrder );

    fp.SetOrderHigh ( IntegerToString ( order ) );
    fp.SetOrderLow  ( IntegerToString ( order ) );
    }


if ( HasCLIFlag ( reprocsub, __causal ) )

    fp.SetCausal ( HasCLIFlag ( reprocsub, __causal ) ? "Causal" : "Non-Causal" );    // Default is already Non-Causal, but let's be extra-cautious here

if ( HasCLIFlag ( reprocsub, __baseline ) )

    fp.SetBaseline ( "Baseline" );

if ( HasCLIOption ( reprocsub, __notches ) ) {

    vector<string>      notches         = GetCLIOptionStrings ( reprocsub, __notches );
    string              allnotches;

    for ( const auto& s : notches )
        allnotches  += s + ",";

    fp.SetNotches ( allnotches.c_str () );

    fp.SetNotchesAutoHarmonics ( HasCLIFlag ( reprocsub, __harmonics ) );
    }


if ( HasCLIOption ( reprocsub, __spatialfilter )
  && HasCLIOption ( reprocsub, __xyzfile       ) ) {

    altfilters.SpatialFilter    = TextToSpatialFilterType ( GetCLIOptionEnum ( reprocsub, __spatialfilter ).c_str () );

    fp.SetSpatialFiltering ( XYZDoc->GetDocPath () );
    }


if ( HasCLIFlag ( reprocsub, __ranking ) )

    fp.SetRanking ( "Rank" );

if ( HasCLIOption ( reprocsub, __rectification ) )

    fp.SetRectification ( GetCLIOptionEnum ( reprocsub, __rectification ).c_str () );

if ( HasCLIOption ( reprocsub, __envelope ) ) {

    fp.SetEnvelopeWidth ( GetCLIOptionString ( reprocsub, __envelope ).c_str () );

    if ( ! fp.IsRectification () )  // shouldn't happen
        fp.SetRectification ( "Power" );
    }

if ( HasCLIOption ( reprocsub, __keepabove ) )

    fp.SetThresholdAbove ( GetCLIOptionString ( reprocsub, __keepabove ).c_str () );

if ( HasCLIOption ( reprocsub, __keepbelow ) )

    fp.SetThresholdBelow ( GetCLIOptionString ( reprocsub, __keepbelow ).c_str () );


                                        // 3) Finally setting the filters from the struct - A quite robust and tested method
altfilters.SetFromStruct ( defaultsamplingfrequency, true );

                                        // A final consistency check
FiltersOptions          filteringoptions    = altfilters.HasNoFilters () ? NotUsingFilters      // default
                                                                         : UsingOtherFilters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              reflist         = GetCLIOptionString ( reprocsub, __reference );

ReferenceType       ref             = reflist == "none"    || reflist == "asinfile" ? ReferenceAsInFile
                                    : reflist == "average" || reflist == "avgref"   ? ReferenceAverage
                                    : ! reflist.empty ()                            ? ReferenceArbitraryTracks
                                    :                                                 ReferenceAsInFile;        // default


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

vector<int>         baselinecorrint = GetCLIOptionInts ( reprocsub, __baselinecorr );

bool                baselinecorr    = baselinecorrint.size () == 2;

if ( baselinecorr )
    CheckOrder ( baselinecorrint[ 0 ], baselinecorrint[ 1 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              rescaling       = GetCLIOptionString ( reprocsub, __rescaling );

RescalingOptions    rescalingoptions= rescaling.empty ()                ? NotRescaled
                                    : rescaling == "meangfp"            ? GfpRescaling
                                    :                                     ConstantRescaling;

                                        // just to avoid crashing in case a string was provided
double              rescalingfactor = StringToDouble ( GetCLIOptionString ( reprocsub, __rescaling ).c_str () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                sequential      = HasCLIFlag ( reprocsub, __sequential );
bool                average         = HasCLIFlag ( reprocsub, __average    );

SequenceOptions     sequenceoptions = sequential ? SequenceProcessing
                                    : average    ? AverageProcessing
                                    :              SequenceProcessing;  // default value


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ReprocessTracks will ignore if 0
int                 downsampleratio = timeoptions     == ExportTimeInterval 
                                   && sequenceoptions == SequenceProcessing ? GetCLIOptionInt ( reprocsub, __downsampling )
                                                                            : 0;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              extension       = GetCLIOptionEnum ( reprocsub, __extension );

SavingEegFileTypes  filetype        = ExtensionToSavingEegFileTypes ( extension.c_str () );

string              infix           = GetCLIOptionString ( reprocsub, __infix );

bool                outputmarkers   = ! HasCLIFlag ( reprocsub, __nomarkers );

ConcatenateOptions  concatenateoptions  = HasCLIFlag ( reprocsub, __concatenate ) && (int) gof > 1 ? ConcatenateTime
                                                                                                   : NoConcatenateTime;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                silent          = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();

cout << ( tracksoptions == ProcessRois ? "Exporting ROIs: " : "Exporting Tracks: " );

if      ( RoisDoc.IsOpen () )   {   cout << roisfile << ", " << RoisDoc->ROIs->GetName () << ": " << RoisDoc->ROIs->GetNumRois () << " x " << RoisDoc->ROIs->GetDimension (); }
else if ( XYZDoc .IsOpen () )   {   cout << xyzfile << ": " << XYZDoc->GetNumElectrodes (); }
else                            {   cout << "EEG tracks names"; }
cout << NewLine;

cout << "Tracks: " << ( tracks.empty () ? "All tracks" : tracks ) << NewLine;

cout << "Time Options: " << ( timeoptions == ExportTimeInterval  ? "Time Interval" 
                            : timeoptions == ExportTimeTriggers  ? "Keeping triggers List"
                            : timeoptions == ExcludeTimeTriggers ? "Excluding triggers List"
                            :                                      "Unknown" ) << NewLine;
if ( timeoptions == ExportTimeInterval ) {
    cout << "Time Min: " << ( timemin == 0                   ? "Beginning of file" : IntegerToString ( timemin ) ) << NewLine;
    cout << "Time Max: " << ( timemax == Highest ( timemax ) ? "End of file"       : IntegerToString ( timemax ) ) << NewLine;
    }
else if ( timeoptions == ExportTimeTriggers )
    cout << "Keep Triggers: " << keeptriggers << NewLine;
else if ( timeoptions == ExcludeTimeTriggers )
    cout << "Exclude Triggers: " << excludetriggers << NewLine;

cout << "Adding Null Tracks: " << BoolToString ( ! nulltracks.empty () ) << NewLine;
if ( ! nulltracks.empty () )
    cout << "Null tracks: " << nulltracks << NewLine;


cout << "Filtering: " << BoolToString ( filteringoptions == UsingOtherFilters ) << NewLine;
if ( filteringoptions == UsingOtherFilters ) {
    char        buff[ KiloByte ];
    cout << "Filters: " << altfilters.ParametersToText ( buff ) << NewLine;
    }
cout << "Default Sampling Frequency: " << defaultsamplingfrequency << NewLine;


cout << "Reference: " << ReferenceNames[ ref ] << NewLine;
if ( ref == ReferenceArbitraryTracks )
    cout << "Reference list: " << reflist << NewLine;

cout << "Baseline: " << BoolToString ( baselinecorr ) << NewLine;
if ( baselinecorr )
    cout << "From: " << baselinecorrint[ 0 ] << " To " << baselinecorrint[ 1 ] << NewLine;

cout << "Rescaling: " << ( rescalingoptions == NotRescaled          ? "No rescaling" 
                         : rescalingoptions == ConstantRescaling    ? "Constant rescaling"
                         : rescalingoptions == GfpRescaling         ? "Mean GFP rescaling"
                         :                                            "Unknown" ) << NewLine;
if ( rescalingoptions == ConstantRescaling )
    cout << "Scaling Factor: " << rescalingfactor << NewLine;
else if ( rescalingoptions == GfpRescaling )
    cout << "Scaling Factor: " << "Mean GFP" << NewLine;

cout << "Sequence Option: " << ( sequenceoptions == SequenceProcessing  ? "Sequential Data" 
                               : sequenceoptions == AverageProcessing   ? "Averaging Data in Time"
                               :                                          "Unknown" ) << NewLine;
cout << "Downsample Ratio: " << ( downsampleratio == 0 ? "No" : IntegerToString ( downsampleratio ) ) << NewLine;
cout << "File Type & Extension: " << filetype << ", " << SavingEegFileExtPreset[ filetype ] << NewLine;
cout << "Infix: " << ( infix.empty () ? ""<none>" : infix ) << NewLine;
cout << "Saving Markers: " << BoolToString ( outputmarkers ) << NewLine;

cout << "Concatenate Time: " << BoolToString ( concatenateoptions ) << NewLine;
if ( concatenateoptions == ConcatenateTime ) {
    cout << "Concatenating " << (int) gof << " files" << NewLine;
    for ( int filei = 0; filei < (int) gof; filei++ )
        cout << "Concatenate File: " << gof[ filei ]   << NewLine;
    }

cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                concatinputtime     = 0;
long                concatoutputtime    = 0;
TExportTracks       expfile;            // needed for files concatenation


for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Processing: " << gof  [ filei ]<< NewLine;
#endif

    TOpenDoc<TTracksDoc>    EEGDoc ( gof[ filei ], OpenDocHidden );


    ReprocessTracks (
                    EEGDoc,
                    tracksoptions,          tracks.c_str (),
                    XYZDoc,
                    RoisDoc,
                    timeoptions,
                    timemin,                timemax,
                    keeptriggers.c_str (),  excludetriggers.c_str (),
                    nulltracks.c_str (),
                    filteringoptions,       &altfilters,        defaultsamplingfrequency,
                    ref,                    reflist.c_str (),
                    baselinecorr,           baselinecorr ? baselinecorrint[ 0 ] : 0,    baselinecorr ? baselinecorrint[ 1 ] : 0,
                    rescalingoptions,       rescalingfactor,
                    sequenceoptions,
                    downsampleratio,
                    filetype,
                    infix.c_str (),
                    "",
                    outputmarkers,
                    concatenateoptions,     concatenateoptions == ConcatenateTime ? &concatinputtime : 0,   concatenateoptions == ConcatenateTime ? &concatoutputtime : 0,
                    expfile,
                    &gof,
                    filei,
                    silent
                    );
    } // for file


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
