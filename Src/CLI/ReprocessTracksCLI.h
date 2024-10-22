/************************************************************************\
Copyright 2024 CIBM (Center for Biomedical Imaging), Lausanne, Switzerland

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
void        ReprocessTracksCLIDefine ( CLI::App* reprocsub )
{
if ( reprocsub == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters appearance follow the dialog's visual design
DefineCLIOptionString   ( reprocsub,        "",     "--tracks",             "Tracks to export" Tab Tab Tab Tab "Special values: 'gfp', 'dis' and 'avg'" )
->ShowDefault           ( "Default:All" );

DefineCLIOptionString   ( reprocsub,        "",     "--xyzfile",            "Using electrodes names from a XYZ electrodes coordinates file" );
DefineCLIOptionString   ( reprocsub,        "",     "--roisfile",           "Computing ROIs & using ROIs names from a ROIs file" );

ExcludeCLIOptions       ( reprocsub,        "--xyzfile",    "--roisfile" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( reprocsub,        "",     "--timemin",            "Exporting from Time Frame" )
->ShowDefault           ( "Default:0" );
DefineCLIOptionInt      ( reprocsub,        "",     "--timemax",            "Exporting to Time Frame" )
->ShowDefault           ( "Default:End-of-file" );
DefineCLIOptionString   ( reprocsub,        "",     "--keeptriggers",       "Exporting only the data from a triggers / markers list" );
DefineCLIOptionString   ( reprocsub,        "",     "--excludetriggers",    "Exporting all data but from a triggers / markers list" );

ExcludeCLIOptions       ( reprocsub,        "--timemin",       "--keeptriggers",   "--excludetriggers" );
ExcludeCLIOptions       ( reprocsub,        "--timemax",       "--keeptriggers",   "--excludetriggers" );
ExcludeCLIOptions       ( reprocsub,        "--keeptriggers",  "--excludetriggers" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     "--nulltracks",         "List of null tracks to append" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reprocess Tracks filters options
DefineCLIOptionString   ( reprocsub,        "",     "--filters",            "A whole string of filtering options, in double quote, as coming from a verbose file" )
                                        // tricky check to raise an error if filters string was not provided: CLI11 will use the next option as a string, so we can simply test for "-" or "--"
->CheckOption           ( []( const string& str ) { return str.find ( "-", 0 ) == 0 ? "string must be in double quotes" : ""; } );


DefineCLIFlag           ( reprocsub,        "--dc", "--baseline",           "Baseline / DC correction (recommended with any High-Pass or Band-Pass filter)" );

                                        // !We allow the simultaneous use of highpass and lowpass, which will be combined into a single bandpass!
DefineCLIOptionDouble   ( reprocsub,        "",     "--highpass",           "High-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );
                                        // Testing High-Pass < Low-Pass here - NOT working anymore
//->CheckOption           ( [ &reprocsub ]( const string& str )
//    {   
//    double      cuth            = StringToDouble ( str.c_str () );
//    bool        hasl            = HasCLIOption       ( reprocsub, "--lowpass" );
//    double      cutl            = GetCLIOptionDouble ( reprocsub, "--lowpass" );
//
//    return         cuth <= 0
//        || hasl && cutl <= 0    ? "frequency cut should be above 0"
//         : hasl && cuth > cutl  ? "high-pass cut should be less than the low-pass cut"
//         :                        ""; 
//    } );

DefineCLIOptionDouble   ( reprocsub,        "",     "--lowpass",            "Low-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );

                                        // Bandpass, however, excludes both highpass and lowpass
DefineCLIOptionDoubles  ( reprocsub,    2,  "",     "--bandpass",           "Band-Pass Butterworth filter" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );
ExcludeCLIOptions       ( reprocsub,        "--bandpass",      "--highpass",   "--lowpass"     );


DefineCLIOptionInt      ( reprocsub,        "",     "--order",              "Butterworth filter order" )
->ShowDefault           ( "Default:" + string ( IntegerToString ( TFilterDefaultOrder ) ) + " or " + string ( IntegerToString ( 2 * TFilterDefaultOrder )  ) )
->CheckOption           ( [ &reprocsub ]( const string& str )   
    {   int         o               = StringToInteger ( str.c_str () );
        bool        orderevenok     = IsEven         ( o );
        bool        orderrangeok    = IsInsideLimits ( o, TFilterMinOrder, TFilterMaxOrder );
        return ! orderevenok || ! orderrangeok  ? string ( "Value should be either a multiple of 2 (highpass or lowpass), or 4 (bandpass), and within the range " ) + TFilterMinOrderString + ".." + TFilterMaxOrderString
             :                                    ""; 
    } );
//->CheckOption           ( [ &reprocsub ]( const string& str )   
//    {   int         o               = StringToInteger ( str.c_str () );
//        bool        optionok        = HasCLIOption ( reprocsub,"--highpass" ) || HasCLIOption ( reprocsub,"--lowpass"  ) || HasCLIOption ( reprocsub,"--bandpass" );
//        bool        orderevenok     = IsEven         ( o );
//        bool        orderrangeok    = IsInsideLimits ( o, TFilterMinOrder, TFilterMaxOrder );
//        return ! optionok                       ? "--order requires one of the following {--highpass, --lowpass, --bandpass} option"
//             : ! orderevenok || ! orderrangeok  ? string ( "Value should be an even number, and within the range " ) + TFilterMinOrderString + ".." + TFilterMaxOrderString
//             :                                    ""; 
//    } );


DefineCLIFlag           ( reprocsub,        "",     "--causal",             "Causal filters, using only forward filtering" );

DefineCLIOptionDoubles  ( reprocsub,    -1, "",     "--notches",            "Notches filter" );
DefineCLIFlag           ( reprocsub,        "",     "--harmonics",          "Adding Notches harmonics" );
NeedsCLIOption          ( reprocsub,        "--harmonics",  "--notches" );


DefineCLIOptionString   ( reprocsub,        "",     "--spatial",            "Spatial filter" )
->ShowDefault           ( "Default:" + string ( SpatialFilterShortName[ SpatialFilterDefault ] ) );
NeedsCLIOption          ( reprocsub,        "--spatial",    "--xyzfile" )
                                            // Case sensitive, but allows a nice listing when requesting Help
->TypeOfOption          ( "ENUM" )
->CheckOption           ( CLI::IsMember ( vector<string> ( SpatialFilterShortName + SpatialFilterOutlier, SpatialFilterShortName + NumSpatialFilterTypes ) ) )
                                            // Allowing for case insensitive (and even the long names, although not advertized), but Help is not helping
//->CheckOption         ( []( const string& str )   {   string      allspatial  = "{"; 
//                                                      for ( int i = SpatialFilterOutlier; i < NumSpatialFilterTypes; i++ )
//                                                          allspatial  += string ( SpatialFilterShortName[ i ] ) + string ( i < NumSpatialFilterTypes - 1 ? ", " : "" ); 
//                                                      allspatial += "}";
//                                                      return  TextToSpatialFilterType ( str.c_str () ) == SpatialFilterNone ? string ( "Spatial Filter should be in " ) + allspatial : ""; } )
->ZeroOrOneArgument;


DefineCLIFlag           ( reprocsub,        "",     "--ranking",            "Ranking data at each time point to [0..1] range" );

DefineCLIOptionString   ( reprocsub,        "",     "--rectification",      "Rectification, i.e. making data all positive" )
->TypeOfOption          ( "ENUM" )
->CheckOption           ( CLI::IsMember ( { "abs", "absolute", "power", "squared" } ) );

DefineCLIOptionDouble   ( reprocsub,        "",     "--envelope",           "Adding a sliding-window smoothing after Rectification, value in [ms]" );
NeedsCLIOption          ( reprocsub,        "--envelope",   "--rectification" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "smoothing window, in [ms], should be positive" : ""; } );

DefineCLIOptionDouble   ( reprocsub,        "",     "--keepabove",          "Thresholding data, keeping data above value" );
DefineCLIOptionDouble   ( reprocsub,        "",     "--keepbelow",          "Thresholding data, keeping data below value" );

DefineCLIOptionDouble   ( reprocsub,        "",     "--samplingfrequency",  "Default Sampling Frequency, only in case a file has none" )
->CheckOption           ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "sampling frequency should be above 0" : ""; } );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     "--reference",          "List of new reference tracks" Tab Tab "Special values: 'none' (default) or 'average'" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInts     ( reprocsub,    2,  "",     "--baselinecorr",       "Baseline correction interval, in time frames since beginning of file" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     "--rescaling",          "Scaling factor" Tab Tab Tab Tab "Special value: 'meangfp'" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "",     "--sequential",         "Sequential output" Tab Tab Tab Tab "Default option" );
DefineCLIFlag           ( reprocsub,        "",     "--average",            "Averaging the time dimension" );
ExcludeCLIOptions       ( reprocsub,        "--sequential", "--average" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( reprocsub,        "",     "--downsampling",       "Downsampling ratio" )
->CheckOption           ( []( const string& str ) { return StringToInteger ( str.c_str () ) <= 1 ? "factor should be above 1" : ""; } );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( reprocsub,        "",     "--infix",              "Infix appended to the file name" );

DefineCLIOptionString   ( reprocsub,        "--ext","--extension",          "Output file extension" )
->TypeOfOption          ( "ENUM" )
->ShowDefault           ( "Default:" + string ( SavingEegFileExtPreset[ PresetFileTypeDefaultEEG ] ) )
->CheckOption           ( CLI::IsMember ( vector<string> ( SavingEegFileExtPreset, SavingEegFileExtPreset + NumSavingEegFileTypes ) ) );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "",     "--nomarkers",          "Not saving the markers to file" );

DefineCLIFlag           ( reprocsub,        "",     "--concatenate",        "Concatenate all output into a single file" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( reprocsub,        "-h",   "--help",               "This message" );
}


//----------------------------------------------------------------------------
                                        // Running the command
void        ReprocessTracksCLI ( CLI::App* reprocsub, const TGoF& gof )
{
if ( ! IsSubCommandUsed ( reprocsub )
  || gof.IsEmpty () )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tracks parameters
string              roisfile        = GetCLIOptionString ( reprocsub, "--roisfile" );
string              xyzfile         = GetCLIOptionString ( reprocsub, "--xyzfile"  );

TracksOptions       tracksoptions   = ! roisfile.empty () ? ProcessRois 
                                    :                       ProcessTracks;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Wraps everything for us: existing file name & opening doc
TOpenDoc<TElectrodesDoc>    XYZDoc  ( xyzfile .c_str (), OpenDocHidden );
TOpenDoc<TRoisDoc>          RoisDoc ( roisfile.c_str (), OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              tracks          = GetCLIOptionString ( reprocsub, "--tracks" );

string              nulltracks      = GetCLIOptionString ( reprocsub, "--nulltracks" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters, exclusively between time interval, triggers to keep or triggers to exclude
                                        // default values
TimeOptions         timeoptions     = ExportTimeInterval;
long                timemin         = 0;                      
long                timemax         = Highest ( timemax );    
string              keeptriggers;
string              excludetriggers;


if      ( HasCLIOption ( reprocsub, "--timemin" )
       || HasCLIOption ( reprocsub, "--timemax" ) ) {

    timeoptions     = ExportTimeInterval;

    if  ( HasCLIOption ( reprocsub, "--timemin" ) )
        timemin     = GetCLIOptionInt ( reprocsub, "--timemin" );

    if  ( HasCLIOption ( reprocsub, "--timemax" ) )
        timemax     = GetCLIOptionInt ( reprocsub, "--timemax" );
    }

else if ( HasCLIOption ( reprocsub, "--keeptriggers" ) ) {

    timeoptions     = ExportTimeTriggers;
    keeptriggers    = GetCLIOptionString ( reprocsub, "--keeptriggers" );
    }

else if ( HasCLIOption ( reprocsub, "--excludetriggers" ) ) {

    timeoptions     = ExcludeTimeTriggers;
    excludetriggers = GetCLIOptionString ( reprocsub, "--excludetriggers" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For total safety, it is convenient to have a default sampling frequency at hand, which could be used for EEGs that don't have one
double              defaultsamplingfrequency    = 0;

                                        // First see if caller provided one
if ( HasCLIOption ( reprocsub, "--samplingfrequency" ) )

    defaultsamplingfrequency    = GetCLIOptionDouble ( reprocsub, "--samplingfrequency" );

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
if ( HasCLIOption ( reprocsub, "--filters" ) ) {

    string              filters         = GetCLIOptionString ( reprocsub, "--filters" );

    altfilters.TextToParameters ( filters.c_str (), XYZDoc.IsOpen () ? XYZDoc->GetDocPath () : 0, defaultsamplingfrequency );
    }

                                        // 2) Setting or overwriting each parameter, one at a time - Heavily based on TTracksFilters::TextToParameters method
                                        // We currently allow highpass + lowpass parameters to define a bandpass
if      ( HasCLIOption ( reprocsub, "--bandpass" ) 
       || HasCLIOption ( reprocsub, "--highpass" ) && HasCLIOption ( reprocsub, "--lowpass"  ) ) {

    string      hs;
    string      ls;

    if ( HasCLIOption ( reprocsub, "--bandpass" ) ) {

        vector<string>      band            = GetCLIOptionStrings ( reprocsub, "--bandpass" );
        
        hs      = band[ 0 ];
        ls      = band[ 1 ];
        }
    else {
        hs      = GetCLIOptionString ( reprocsub, "--highpass" );
        ls      = GetCLIOptionString ( reprocsub, "--lowpass"  );
        }

                                        // check order here, as CLI11 check method seems to crash
    double      h   = StringToDouble ( hs.c_str () );
    double      l   = StringToDouble ( ls.c_str () );

    fp.SetButterworthHigh ( h < l ? hs.c_str () : ls.c_str () );
    fp.SetButterworthLow  ( h < l ? ls.c_str () : hs.c_str () );
    }

else if ( HasCLIOption ( reprocsub, "--highpass" ) )

    fp.SetButterworthHigh ( GetCLIOptionString ( reprocsub, "--highpass" ).c_str () );

else if ( HasCLIOption ( reprocsub, "--lowpass" ) )

    fp.SetButterworthLow  ( GetCLIOptionString ( reprocsub, "--lowpass" ).c_str () );


if ( HasCLIOption ( reprocsub, "--order" ) ) {

    int                 order           = GetCLIOptionInt ( reprocsub, "--order" );

    if ( fp.IsBandPass () )
        order  /= 2;                // set half the band-pass order

    fp.SetOrderHigh ( IntegerToString ( order ) );
    fp.SetOrderLow  ( IntegerToString ( order ) );
    }


if ( HasCLIFlag ( reprocsub, "--causal" ) )

    fp.SetCausal ( HasCLIFlag ( reprocsub, "--causal" ) ? "Causal" : "Non-Causal" );    // Default is already Non-Causal, but let's be extra-cautious here

if ( HasCLIFlag ( reprocsub, "--baseline" ) )

    fp.SetBaseline ( "Baseline" );

if ( HasCLIOption ( reprocsub, "--notches" ) ) {

    vector<string>      notches         = GetCLIOptionStrings ( reprocsub, "--notches" );
    string              allnotches;

    for ( const auto& s : notches )
        allnotches  += s + ",";

    fp.SetNotches ( allnotches.c_str () );

    fp.SetNotchesAutoHarmonics ( HasCLIFlag ( reprocsub, "--harmonics" ) );
    }


if ( HasCLIOption ( reprocsub, "--spatial" )
  && HasCLIOption ( reprocsub, "--xyzfile" ) ) {
                                    // we will fill the missing value with the default option
    if ( GetCLIOptionString ( reprocsub, "--spatial" ).empty () )
        altfilters.SpatialFilter    = SpatialFilterDefault;
    else
        altfilters.SpatialFilter    = TextToSpatialFilterType ( GetCLIOptionString ( reprocsub, "--spatial" ).c_str () );


    fp.SetSpatialFiltering ( XYZDoc->GetDocPath () );
    }


if ( HasCLIFlag ( reprocsub, "--ranking" ) )

    fp.SetRanking ( "Rank" );

if ( HasCLIOption ( reprocsub, "--rectification" ) )

    fp.SetRectification ( GetCLIOptionString ( reprocsub, "--rectification" ).c_str () );

if ( HasCLIOption ( reprocsub, "--envelope" ) ) {

    fp.SetEnvelopeWidth ( GetCLIOptionString ( reprocsub, "--envelope" ).c_str () );

    if ( ! fp.IsRectification () )  // shouldn't happen
        fp.SetRectification ( "Power" );
    }

if ( HasCLIOption ( reprocsub, "--keepabove" ) )

    fp.SetThresholdAbove ( GetCLIOptionString ( reprocsub, "--keepabove" ).c_str () );

if ( HasCLIOption ( reprocsub, "--keepbelow" ) )

    fp.SetThresholdBelow ( GetCLIOptionString ( reprocsub, "--keepbelow" ).c_str () );


                                        // 3) Finally setting the filters from the struct - A quite robust and tested method
altfilters.SetFromStruct ( defaultsamplingfrequency, true );

                                        // A final consistency check
FiltersOptions          filteringoptions    = altfilters.HasNoFilters () ? NotUsingFilters      // default
                                                                         : UsingOtherFilters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              reflist         = GetCLIOptionString ( reprocsub, "--reference" );

ReferenceType       ref             = reflist.empty ()
                                   || reflist == "none"
                                   || reflist == "no reference"
                                   || reflist == "noreference"
                                   || reflist == "as in file"
                                   || reflist == "asinfile"             ? ReferenceAsInFile
                                    : reflist == "average"
                                   || reflist == "average reference"
                                   || reflist == "avgref"               ? ReferenceAverage
                                    :                                     ReferenceMultipleTracks;  // ReprocessTracks will sort out between  ReferenceSingleTrack  and  ReferenceMultipleTracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

vector<int>         baselinecorrint = GetCLIOptionInts ( reprocsub, "--baselinecorr" );

bool                baselinecorr    = baselinecorrint.size () == 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              rescaling       = GetCLIOptionString ( reprocsub, "--rescaling" );

RescalingOptions    rescalingoptions= rescaling.empty ()                ? NotRescaled
                                    : rescaling == "meangfp"
                                   || rescaling == "mean gfp"           ? GfpRescaling
                                    :                                     ConstantRescaling;

                                        // just to avoid crashing in case a string was provided
double              rescalingfactor = StringToDouble ( GetCLIOptionString ( reprocsub, "--rescaling" ).c_str () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                sequential      = HasCLIFlag ( reprocsub, "--sequential" );
bool                average         = HasCLIFlag ( reprocsub, "--average" );

SequenceOptions     sequenceoptions = sequential ? SequenceProcessing
                                    : average    ? AverageProcessing
                                    :              SequenceProcessing;  // default value


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ReprocessTracks will ignore if 0
int                 downsampleratio = timeoptions     == ExportTimeInterval 
                                   && sequenceoptions == SequenceProcessing ? GetCLIOptionInt ( reprocsub, "--downsampling" )
                                                                            : 0;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              extension       = GetCLIOptionString ( reprocsub, "--extension" );

SavingEegFileTypes  filetype        = ExtensionToSavingEegFileTypes ( extension.c_str () );

string              infix           = GetCLIOptionString ( reprocsub, "--infix" );

bool                outputmarkers   = ! HasCLIFlag ( reprocsub, "--nomarkers" );

ConcatenateOptions  concatenateoptions  = HasCLIFlag ( reprocsub, "--concatenate" ) && (int) gof > 1 ? ConcatenateTime
                                                                                                     : NoConcatenateTime;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                silent              = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
#define     CLIConsoleOutput
//#undef      CLIConsoleOutput

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
    cout << "Time Min: " << ( timemin == 0 ? "Beginning of file" : IntegerToString ( timemin ) ) << NewLine;
    cout << "Time Max: " << ( timemax == Highest ( timemax ) ? "End of file" : IntegerToString ( timemax ) ) << NewLine;
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
if ( ref == ReferenceMultipleTracks )
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
cout << "Infix: " << ( infix.empty () ? "None" : infix ) << NewLine;
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
