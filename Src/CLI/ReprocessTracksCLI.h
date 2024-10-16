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
AddOptionString ( reprocsub,    xyzfile,            "",     "--xyzfile",            "Using electrodes names from a XYZ electrodes coordinates file" );
AddOptionString ( reprocsub,    roisfile,           "",     "--roisfile",           "Computing ROIs & using ROIs names from a ROIs file" )
->excludes ( optxyzfile  );

AddOptionString ( reprocsub,    tracks,             "",     "--tracks",             "Tracks to export" Tab Tab Tab Tab "Default: all tracks; Special values: 'gfp', 'dis' and 'avg'" );

AddOptionInt    ( reprocsub,    timemin,            "",     "--timemin",            "Exporting from Time Frame " Tab Tab "Default: 0" );
AddOptionInt    ( reprocsub,    timemax,            "",     "--timemax",            "Exporting to Time Frame   " Tab Tab "Default: end of file" );
AddOptionString ( reprocsub,    keeptriggers,       "",     "--keeptriggers",       "Exporting only the data from a triggers / markers list" );
AddOptionString ( reprocsub,    excludetriggers,    "",     "--excludetriggers",    "Exporting all data but from a triggers / markers list" );

opttimemin     ->excludes ( optkeeptriggers    )->excludes ( optexcludetriggers );
opttimemax     ->excludes ( optkeeptriggers    )->excludes ( optexcludetriggers );
optkeeptriggers->excludes ( optexcludetriggers );

AddOptionString ( reprocsub,    nulltracks,         "",     "--nulltracks",         "List of null tracks to append" );

AddOptionString ( reprocsub,    ref,                "",     "--reference",          "List of new reference tracks" Tab Tab "Special values: 'none' (default) or 'average'" );

AddOptionInts   ( reprocsub,    baselinecorr,   2,  "",     "--baselinecorr",       "Baseline correction interval, in time frames since beginning of file" );

AddOptionString ( reprocsub,    rescaling,          "",     "--rescaling",          "Scaling factor" Tab Tab Tab Tab "Special value: 'meangfp'" );

AddFlag         ( reprocsub,    average,            "",     "--averaging",          "Averaging the whole time dimension" );

AddOptionInt    ( reprocsub,    downsampleratio,    "",     "--downsampling",       "Downsampling ratio" )
->check ( []( const string& str ) { return StringToInteger ( str.c_str () ) <= 1 ? "factor should be above 1" : ""; } );

AddOptionString ( reprocsub,    extension,          "--ext","--extension",          string ( "Output file extension" Tab Tab Tab "Default: " ) + SavingEegFileExtPreset[ PresetFileTypeDefaultEEG ] )
->check ( CLI::IsMember ( vector<string> ( SavingEegFileExtPreset, SavingEegFileExtPreset + NumSavingEegFileTypes ) ) );

AddOptionString ( reprocsub,    infix,              "",     "--infix",              "Infix appended to the file name" );

AddFlag         ( reprocsub,    nomarkers,          "",     "--nomarkers",          "Not saving the markers to file" );

AddFlag         ( reprocsub,    concatenate,        "",     "--concatenate",        "Concatenate all output into a single file" );

AddFlag         ( reprocsub,    helpreproc,         "-h",   "--help",               "This message" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reprocess Tracks filters options

AddOptionString ( reprocsub,    filters,            "",     "--filters",            "A whole string of filtering options, in double quote, as coming from a verbose file" )
                                        // tricky check to raise an error if filters string was not provided: CLI11 will use the next option as a string, so we can simply test for "-" or "--"
->check ( []( const string& str ) { return str.find ( "-", 0 ) == 0 ? "string must be in double quotes" : ""; } );


AddFlag         ( reprocsub,    baseline,           "--dc", "--baseline",           "Baseline / DC correction (recommended with any High-Pass or Band-Pass filter)" );

                                        // !We allow the simultaneous use of highpass and lowpass, which will be combined into a single bandpass!
AddOptionDouble ( reprocsub,    highpass,           "",     "--highpass",           "High-Pass Butterworth filter" )
                                        // Testing High-Pass < Low-Pass here
->check ( [ &reprocsub ]( const string& str )
    {   
    double      cuth            = StringToDouble ( str.c_str () );
    bool        hasl            = HasSubOption       ( reprocsub,"--lowpass" );
    double      cutl            = GetSubOptionDouble ( reprocsub, "--lowpass" );

    return         cuth <= 0
        || hasl && cutl <= 0    ? "frequency cut should be above 0"
         : hasl && cuth > cutl  ? "high-pass cut should be less than the low-pass cut"
         :                        ""; 
    } );

AddOptionDouble ( reprocsub,    lowpass,            "",     "--lowpass",            "Low-Pass Butterworth filter" )
->check ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "frequency cut should be above 0" : ""; } );
                                        // Bandpass, however, excludes both highpass and lowpass
AddOptionDoubles( reprocsub,    bandpass,       2,  "",     "--bandpass",           "Band-Pass Butterworth filter" );

optbandpass->excludes ( optlowpass  )
           ->excludes ( opthighpass );


AddOptionInt    ( reprocsub,    order,              "",     "--order",              string ( "Butterworth filter order" ) + Tab + Tab + Tab + "Default: " + TFilterDefaultOrderString )
->check ( [ &reprocsub ]( const string& str )   
    {   int         o               = StringToInteger ( str.c_str () );
        bool        optionok        = HasSubOption ( reprocsub,"--highpass" ) || HasSubOption ( reprocsub,"--lowpass"  ) || HasSubOption ( reprocsub,"--bandpass" );
        bool        orderevenok     = IsEven         ( o );
        bool        orderrangeok    = IsInsideLimits ( o, TFilterMinOrder, TFilterMaxOrder );
        return ! optionok                       ? "--order requires one of the following {--highpass, --lowpass, --bandpass} option"
             : ! orderevenok || ! orderrangeok  ? string ( "Value should be an even number, and within the range " ) + TFilterMinOrderString + ".." + TFilterMaxOrderString
             :                                    ""; 
    } );

AddFlag         ( reprocsub,    causal,             "",     "--causal",             "Causal filters, using only forward filtering" );

AddOptionDoubles( reprocsub,    notches,       -1,  "--notch","--notches",          "Notches filter" );
AddFlag         ( reprocsub,    notchesharm,        "",     "--harmonics",          "Adding Notches harmonics" );
optnotchesharm->needs ( optnotches );


AddOptionString ( reprocsub,    spatial,            "",     "--spatial",            string ( "Spatial filter" ) + Tab + Tab + Tab + Tab + "Default: " + SpatialFilterShortName[ SpatialFilterDefault ] )
->needs ( optxyzfile )
                                            // Case sensitive, but allows a nice listing when requesting Help
->check ( CLI::IsMember ( vector<string> ( SpatialFilterShortName + SpatialFilterOutlier, SpatialFilterShortName + NumSpatialFilterTypes ) ) )
                                            // Allowing for case insensitive (and even the long names, although not advertized), but Help is not helping
//->check ( []( const string& str )   {   string      allspatial  = "{"; 
//                                        for ( int i = SpatialFilterOutlier; i < NumSpatialFilterTypes; i++ )
//                                            allspatial  += string ( SpatialFilterShortName[ i ] ) + string ( i < NumSpatialFilterTypes - 1 ? ", " : "" ); 
//                                        allspatial += "}";
//                                        return  TextToSpatialFilterType ( str.c_str () ) == SpatialFilterNone ? string ( "Spatial Filter should be in " ) + allspatial : ""; } )
->expected ( 0, 1 ); // This allows 0 or 1 arguments


AddFlag         ( reprocsub,    ranking,            "",     "--ranking",            "Ranking data at each time point to [0..1] range" );

AddOptionString ( reprocsub,    rectification,      "",     "--rectification",      "Rectification, i.e. making data all positive" )
->check ( CLI::IsMember ( { "abs", "absolute", "power", "squared" } ) );

AddOptionDouble ( reprocsub,    envelope,           "",     "--envelope",           "Adding a sliding-window smoothing after Rectification, value in [ms]" )
->needs ( optrectification )
->check ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "smoothing window, in [ms], should be positive" : ""; } );

AddOptionDouble ( reprocsub,    keepabove,          "",     "--keepabove",          "Thresholding data, keeping data above value" );
AddOptionDouble ( reprocsub,    keepbelow,          "",     "--keepbelow",          "Thresholding data, keeping data below value" );

AddOptionDouble ( reprocsub,    samplingfrequency,  "",     "--samplingfrequency",  "Default Sampling Frequency, only in case a file has none" )
->check ( []( const string& str ) { return StringToDouble ( str.c_str () ) <= 0 ? "sampling frequency should be above 0" : ""; } );

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
string              roisfile        = GetSubOptionString ( reprocsub, "--roisfile" );
string              xyzfile         = GetSubOptionString ( reprocsub, "--xyzfile"  );

TracksOptions       tracksoptions   = ! roisfile.empty () ? ProcessRois 
                                    :                       ProcessTracks;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Wraps everything for us: existing file name & opening doc
TOpenDoc<TElectrodesDoc>    XYZDoc  ( xyzfile .c_str (), OpenDocHidden );
TOpenDoc<TRoisDoc>          RoisDoc ( roisfile.c_str (), OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              tracks          = GetSubOptionString ( reprocsub, "--tracks" );

string              nulltracks      = GetSubOptionString ( reprocsub, "--nulltracks" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters, exclusively between time interval, triggers to keep or triggers to exclude
                                        // default values
TimeOptions         timeoptions     = ExportTimeInterval;
long                timemin         = 0;                      
long                timemax         = Highest ( timemax );    
string              keeptriggers;
string              excludetriggers;


if      ( HasSubOption ( reprocsub, "--timemin" )
       || HasSubOption ( reprocsub, "--timemax" ) ) {

    timeoptions     = ExportTimeInterval;

    if  ( HasSubOption ( reprocsub, "--timemin" ) )
        timemin     = GetSubOptionInt ( reprocsub, "--timemin" );

    if  ( HasSubOption ( reprocsub, "--timemax" ) )
        timemax     = GetSubOptionInt ( reprocsub, "--timemax" );
    }

else if ( HasSubOption ( reprocsub, "--keeptriggers" ) ) {

    timeoptions     = ExportTimeTriggers;
    keeptriggers    = GetSubOptionString ( reprocsub, "--keeptriggers" );
    }

else if ( HasSubOption ( reprocsub, "--excludetriggers" ) ) {

    timeoptions     = ExcludeTimeTriggers;
    excludetriggers = GetSubOptionString ( reprocsub, "--excludetriggers" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For total safety, it is convenient to have a default sampling frequency at hand, which could be used for EEGs that don't have one
double              defaultsamplingfrequency    = 0;

                                        // First see if caller provided one
if ( HasSubOption ( reprocsub, "--samplingfrequency" ) )

    defaultsamplingfrequency    = GetSubOptionDouble ( reprocsub, "--samplingfrequency" );

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
if ( HasSubOption ( reprocsub, "--filters" ) ) {

    string              filters         = GetSubOptionString ( reprocsub, "--filters" );

    altfilters.TextToParameters ( filters.c_str (), XYZDoc.IsOpen () ? XYZDoc->GetDocPath () : 0, defaultsamplingfrequency );
    }

                                        // 2) Setting or overwriting each parameter, one at a time - Heavily based on TTracksFilters::TextToParameters method
if ( HasSubOption ( reprocsub, "--bandpass" ) ) {

    vector<string>      band            = GetSubOptionStrings ( reprocsub, "--bandpass" );
    
    fp.SetButterworthHigh ( band[ 0 ].c_str () );
    fp.SetButterworthLow  ( band[ 1 ].c_str () );
    }

if ( HasSubOption ( reprocsub, "--highpass" ) )

    fp.SetButterworthHigh ( GetSubOptionString ( reprocsub, "--highpass" ).c_str () );

if ( HasSubOption ( reprocsub, "--lowpass" ) )

    fp.SetButterworthLow  ( GetSubOptionString ( reprocsub, "--lowpass" ).c_str () );

if ( HasSubOption ( reprocsub, "--order" ) ) {

    int                 order           = GetSubOptionInt ( reprocsub, "--order" );

    if ( fp.IsBandPass () )
        order  /= 2;                // set half the band-pass order

    fp.SetOrderHigh ( IntegerToString ( order ) );
    fp.SetOrderLow  ( IntegerToString ( order ) );
    }


if ( HasSubFlag ( reprocsub, "--causal" ) )

    fp.SetCausal ( HasSubFlag ( reprocsub, "--causal" ) ? "Causal" : "Non-Causal" );    // Default is already Non-Causal, but let's be extra-cautious here

if ( HasSubFlag ( reprocsub, "--baseline" ) )

    fp.SetBaseline ( "Baseline" );

if ( HasSubOption ( reprocsub, "--notches" ) ) {

    vector<string>      notches         = GetSubOptionStrings ( reprocsub, "--notches" );
    string              allnotches;

    for ( const auto& s : notches )
        allnotches  += s + ",";

    fp.SetNotches ( allnotches.c_str () );

    fp.SetNotchesAutoHarmonics ( HasSubFlag ( reprocsub, "--harmonics" ) );
    }


if ( HasSubOption ( reprocsub, "--spatial" )
    && HasSubOption ( reprocsub, "--xyzfile" ) ) {
                                    // we will fill the missing value with the default option
    if ( GetSubOptionString ( reprocsub, "--spatial" ).empty () )
        altfilters.SpatialFilter    = SpatialFilterDefault;
    else
        altfilters.SpatialFilter    = TextToSpatialFilterType ( GetSubOptionString ( reprocsub, "--spatial" ).c_str () );


    fp.SetSpatialFiltering ( XYZDoc->GetDocPath () );
    }


if ( HasSubFlag ( reprocsub, "--ranking" ) )

    fp.SetRanking ( "Rank" );

if ( HasSubOption ( reprocsub, "--rectification" ) )

    fp.SetRectification ( GetSubOptionString ( reprocsub, "--rectification" ).c_str () );

if ( HasSubOption ( reprocsub, "--envelope" ) ) {

    fp.SetEnvelopeWidth ( GetSubOptionString ( reprocsub, "--envelope" ).c_str () );

    if ( ! fp.IsRectification () )  // shouldn't happen
        fp.SetRectification ( "Power" );
    }

if ( HasSubOption ( reprocsub, "--keepabove" ) )

    fp.SetThresholdAbove ( GetSubOptionString ( reprocsub, "--keepabove" ).c_str () );

if ( HasSubOption ( reprocsub, "--keepbelow" ) )

    fp.SetThresholdBelow ( GetSubOptionString ( reprocsub, "--keepbelow" ).c_str () );


                                        // 3) Finally setting the filters from the struct - A quite robust and tested method
altfilters.SetFromStruct ( defaultsamplingfrequency, true );

                                        // A final consistency check
FiltersOptions          filteringoptions    = altfilters.HasNoFilters () ? NotUsingFilters      // default
                                                                         : UsingOtherFilters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              reflist         = GetSubOptionString ( reprocsub, "--reference" );

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

vector<int>         baselinecorrint = GetSubOptionInts ( reprocsub, "--baselinecorr" );

bool                baselinecorr    = baselinecorrint.size () == 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              rescaling       = GetSubOptionString ( reprocsub, "--rescaling" );

RescalingOptions    rescalingoptions= rescaling.empty ()                ? NotRescaled
                                    : rescaling == "meangfp"
                                   || rescaling == "mean gfp"           ? GfpRescaling
                                    :                                     ConstantRescaling;

                                        // just to avoid crashing in case a string was provided
double              rescalingfactor = StringToDouble ( GetSubOptionString ( reprocsub, "--rescaling" ).c_str () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                average         = HasSubFlag ( reprocsub, "--averaging" );

SequenceOptions     sequenceoptions = average ? AverageProcessing
                                    :           SequenceProcessing;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ReprocessTracks will ignore if 0
int                 downsampleratio = timeoptions     == ExportTimeInterval 
                                   && sequenceoptions == SequenceProcessing ? GetSubOptionInt ( reprocsub, "--downsampling" )
                                                                            : 0;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              extension       = GetSubOptionString ( reprocsub, "--extension" );

SavingEegFileTypes  filetype        = ExtensionToSavingEegFileTypes ( extension.c_str () );

string              infix           = GetSubOptionString ( reprocsub, "--infix" );

bool                outputmarkers   = ! HasSubFlag ( reprocsub, "--nomarkers" );

ConcatenateOptions  concatenateoptions  = HasSubFlag ( reprocsub, "--concatenate" ) && (int) gof > 1 ? ConcatenateTime
                                                                                                     : NoConcatenateTime;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                silent              = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Console output prototype
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
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                    concatinputtime             = 0;
long                    concatoutputtime            = 0;
TExportTracks           expfile;            // needed for files concatenation


for ( int filei = 0; filei < (int) gof; filei++ ) {

//  cout << "Processing: " << gof  [ filei ]<< NewLine;

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


//DeleteConsole ( true );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
