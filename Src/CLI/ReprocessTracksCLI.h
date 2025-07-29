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
#include    "Files.TVerboseFile.h"
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

DefineCLIOptionFile     ( reprocsub,        "",     __inputdir,             __inputdir_descr )
->TypeOfOption          ( __inputdir_type )
->CheckOption           ( CLI::ExistingDirectory ); // could be incomplete, but it helps a bit, though

DefineCLIOptionFile     ( reprocsub,        "",     __outputdir,            __outputdir_descr )
->TypeOfOption          ( __outputdir_type );

DefineCLIOptionString   ( reprocsub,        "",     __infix,                __infix_descr );

DefineCLIOptionEnum     ( reprocsub,        __ext,  __extension,            __ext_descr )
->DefaultString         ( SavingEegFileExtPreset[ PresetFileTypeDefaultEEG ] )
->CheckOption           ( CLI::IsMember ( vector<string> ( SavingEegFileExtPreset, SavingEegFileExtPreset + NumSavingEegFileTypes ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "",     __nomarkers,            "Not saving the markers to file" );

DefineCLIFlag           ( reprocsub,        "",     __concatenate,          "Concatenate all output into a single file" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        __h,    __help,                 __help_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating positional files option seems OK
DefineCLIOptionFiles    ( reprocsub,-1,      "",     __files,                __files_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     ReprocessTracksCLI ( CLI::App* reprocsub )
{
if ( ! IsSubCommandUsed ( reprocsub )  )
    return;


TFileName           inputdir        = GetCLIOptionDir   ( reprocsub, __inputdir );
TGoF                gof             = GetCLIOptionFiles ( reprocsub, __files, __inputdir );

if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tracks parameters

TFileName           roisfile        = GetCLIOptionFile ( reprocsub, __roisfile, __inputdir );
TFileName           xyzfile         = GetCLIOptionFile ( reprocsub, __xyzfile , __inputdir );

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
altfilters.SetFromStruct ( defaultsamplingfrequency, Silent );

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

TFileName           outputdir       = GetCLIOptionDir   ( reprocsub, __outputdir );

string              infix           = GetCLIOptionString( reprocsub, __infix );

string              extension       = GetCLIOptionEnum  ( reprocsub, __extension );

SavingEegFileTypes  filetype        = ExtensionToSavingEegFileTypes ( extension.c_str () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                outputmarkers   = ! HasCLIFlag ( reprocsub, __nomarkers );

ConcatenateOptions  concatenateoptions  = HasCLIFlag ( reprocsub, __concatenate ) && (int) gof > 1 ? ConcatenateTime
                                                                                                   : NoConcatenateTime;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();


TVerboseFile        verbose ( "cout", VerboseFileDefaultWidth );
char                buff[ KiloByte ];

verbose.NextTopic ( "Files:" );
{
if ( concatenateoptions == ConcatenateTime ) {
    verbose.Put ( "Number of Input Files:", (int) gof );

    for ( int i = 0; i < (int) gof; i++ )
        verbose.Put ( ! i ? "Input   Files        :" : "", gof[ i ] );

    verbose.NextLine ();
    }

verbose.NextLine ();
verbose.Put ( "Using electrode names from file:", XYZDoc .IsOpen () ? XYZDoc ->GetDocPath () : "No" );
verbose.Put ( "Using ROIs            from file:", RoisDoc.IsOpen () ? RoisDoc->GetDocPath () : "No" );
if ( concatenateoptions == ConcatenateTime )
    verbose.Put ( "Concatenate into 1 file:", concatenateoptions == ConcatenateTime );
}


verbose.NextTopic ( "Exporting:" );
{
verbose.Put ( "Type of export:", tracksoptions == ProcessTracks ? "Tracks" : "ROIs" );

if      ( tracksoptions == ProcessTracks ) {
    verbose.Put ( "Track names:",   tracks.empty () ? "All" : tracks );
    }
else if ( tracksoptions == ProcessRois ) {
    verbose.Put ( "ROIs names:",    RoisDoc->ROIs->RoiNamesToText ( buff ) );
    }
}


verbose.NextTopic ( "Input Time Period:" );
{
verbose.Put ( "Input time range defined by:", timeoptions == ExportTimeInterval ? "Interval" : /*timeoptions == ExportTimeTriggers || timeoptions == ExcludeTimeTriggers*/ "Triggers" );

if      ( timeoptions == ExportTimeInterval ) {
    verbose.Put ( "From      Time Frame :", timemin == 0                   ? "Beginning of file" : IntegerToString ( timemin ) );
    verbose.Put ( "To        Time Frame :", timemax == Highest ( timemax ) ? "End of file"       : IntegerToString ( timemax ) );
    }
else if ( timeoptions == ExportTimeTriggers ) {
    verbose.Put ( "Keeping Triggers     :", keeptriggers );
    }
else if ( timeoptions == ExcludeTimeTriggers ) {
    verbose.Put ( "Excluding Triggers   :", excludetriggers );
    }
}


verbose.NextTopic ( "Processing Parameters:" );
{
verbose.Put ( "Sampling Frequency [Hz]:", defaultsamplingfrequency );


verbose.NextLine ();
verbose.Put ( "Adding null tracks:", nulltracks.empty () ? "No" : nulltracks );


verbose.NextLine ();
if      ( filteringoptions == UsingOtherFilters   ) verbose.Put ( "Other filters:",     altfilters.ParametersToText ( buff ) );
else                                                verbose.Put ( "Filters:",           false );


verbose.NextLine ();
if      ( ref == ReferenceAsInFile          )       verbose.Put ( "Reference:", "No reference, as in file" );
else if ( ref == ReferenceAverage           )       verbose.Put ( "Reference:", "Average reference" );
else if ( ref == ReferenceArbitraryTracks   )       verbose.Put ( "Reference:", reflist );
else                                                verbose.Put ( "Reference:", "Unknown" );


verbose.NextLine ();
verbose.Put ( "Baseline correction:", baselinecorr );

if ( baselinecorr ) {
    verbose.Put ( "Inferior limit (absolute value) in TF:", baselinecorrint[ 0 ] );
    verbose.Put ( "Superior limit (absolute value) in TF:", baselinecorrint[ 1 ] );
    }


verbose.NextLine ();
verbose.Put ( "Rescaling:", rescalingoptions != NotRescaled );

if ( rescalingoptions != NotRescaled ) {

    if      ( rescalingoptions == ConstantRescaling )
        verbose.Put ( "Rescaling method:", "Fixed value" );
    else if ( rescalingoptions == GfpRescaling )
        verbose.Put ( "Rescaling method:", "Mean GFP" );

    if ( rescalingoptions == ConstantRescaling || rescalingoptions == GfpRescaling && concatenateoptions == NoConcatenateTime )
        verbose.Put ( "Rescaling factor:", rescalingfactor );
                                    // concatenation: one factor per file
    }
}


verbose.NextTopic ( "Output Time:" );
{
if ( concatenateoptions == ConcatenateTime )
    verbose.Put ( "Output time is:", sequenceoptions == SequenceProcessing ? "Sequential" : "Sequence of each file's average" );
else
    verbose.Put ( "Output time is:", sequenceoptions == SequenceProcessing ? "Sequential" : "Average" );

if ( sequenceoptions == SequenceProcessing ) {
    verbose.NextLine ();
    verbose.Put ( "Time downsampling:", downsampleratio != 0 );

    if ( downsampleratio != 0 ) {
        verbose.Put ( "Downsampling ratio:", downsampleratio );
        verbose.Put ( "Downsampled Sampling Frequency [Hz]:", defaultsamplingfrequency / downsampleratio );
        }
    }
}


verbose.NextTopic ( "Options:" );
{
verbose.Put ( "Input directory:",       inputdir .IsEmpty () ? "None" : inputdir  );
verbose.Put ( "Output directory:",      outputdir.IsEmpty () ? "None" : outputdir );
verbose.Put ( "File name infix:",       infix );
verbose.Put ( "Output file type:",      SavingEegFileExtPreset[ filetype ] );
verbose.Put ( "Saving markers:",        outputmarkers );
}


verbose.NextLine ();
verbose.NextLine ();

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                concatinputtime     = 0;
long                concatoutputtime    = 0;
TExportTracks       expfile;            // needed for files concatenation


for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Now Processing: " << gof  [ filei ]<< NewLine;
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
                    outputdir,
                    infix.c_str (),
                    "",                     // no frequency infix
                    filetype,
                    outputmarkers,
                    concatenateoptions,     concatenateoptions == ConcatenateTime ? &concatinputtime : 0,   concatenateoptions == ConcatenateTime ? &concatoutputtime : 0,
                    expfile,
                    &gof,
                    filei,
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
