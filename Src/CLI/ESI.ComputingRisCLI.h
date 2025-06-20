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

#include    "ESI.ComputingRis.h"
#include    "TComputingRisDialog.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Defining the interface
inline void     ComputingRisCLIDefine ( CLI::App* computingris )
{
if ( computingris == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( computingris,     "",     __preset,               "EEG Preset (Required);  IndSubjectsEpochs needs: --envelope" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __preset1, __preset2, __preset3, __preset4, __preset5, __preset6, __preset7, __preset8 } ) ) );

//DefineCLIOptionFile     ( computingris,     "",     __listfiles,            "A .csv or .txt file containing the input files" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __inversefile,          "Inverse Matrix file (Required)" );
//->Required ();    // interferes with --help

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __xyzfile,              "Electrodes coordinates file, for Spatial Filter" );

                                        // !it is enough to provide the xyzfile to activate the default spatial filter!
DefineCLIOptionEnum     ( computingris,     "",     __spatialfilter,        "Spatial filter" );
NeedsCLIOption          ( computingris,     __spatialfilter,    __xyzfile )
->CheckOption           ( CLI::IsMember ( vector<string> ( SpatialFilterShortName + SpatialFilterOutlier, SpatialFilterShortName + NumSpatialFilterTypes ) ) )
->DefaultString         ( SpatialFilterShortName[ SpatialFilterDefault ] )
->ZeroOrOneArgument;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( computingris,     "",     __typeformat,           "Final file type (Required)" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __norm, __vector } ) ) );
// !no default enum!

DefineCLIOptionEnum     ( computingris,     __reg,  __regularization,       "Regularization level" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __regnone, __regauto, __reg0, __reg1, __reg2, __reg3, __reg4, __reg5, __reg6, __reg7, __reg8, __reg9, __reg10, __reg11, __reg12 } ) ) )
->DefaultString         ( __regauto );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( computingris,     "",     __zscore,           "Final file type" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __compute, __loadfile } ) ) );
// !no default enum!

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( computingris,     "",     __ranking,              "Ranking results" );


DefineCLIOptionDouble   ( computingris,     "",     __envelope,             "Envelope filter on positive data, value in [ms]" )
//NeedsCLIOption          ( computingris,     __envelope,     __typeformat )  // needs __norm
->CheckOption           ( [ &computingris ]( const string& str )   
    {   if ( StringToDouble ( str.c_str () ) <= 0 ) return "envelope window, in [ms], should be positive";
      //if ( GetCLIOptionEnum ( computingris, __typeformat ) == __vector ) return "can not process envelope on vectorial data";
        return ""; 
    } );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __roisfile,             "Computing ROIs" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( computingris,     "",     __prefix,               "Output files optional prefix" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Options with short version
DefineCLIFlag           ( computingris,     __savingsubjectsS,          __savingsubjects,           "Saving subjects" );
DefineCLIFlag           ( computingris,     __savingsubjectsepochsS,    __savingsubjectsepochs,     "Saving subjects epochs" );
DefineCLIFlag           ( computingris,     __savinggrandaverageS,      __savinggrandaverage,       "Saving Grand Averages" );
DefineCLIFlag           ( computingris,     __savingtemplatesS,         __savingtemplates,          "Saving Templates" );
DefineCLIFlag           ( computingris,     __savingzscoreS,            __savingzscore,             "Saving Z-Score Factors" );

ExcludeCLIOptions       ( computingris,     __savingtemplates,      __savingsubjects );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( computingris,     __h,    __help,                 "This message" );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     ComputingRisCLI ( CLI::App* computingris, const TGoF& gof )
{
if ( ! IsSubCommandUsed ( computingris )  )
    return;


//TFileName           listfiles       = GetCLIOptionFile ( computingris, __listfiles );

if ( gof.IsEmpty () /*&& listfiles.IsEmpty ()*/ ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! HasCLIOption ( computingris, __preset ) ) {

    ConsoleErrorMessage ( __preset, "No preset specified!" );
    return;
    }


string              preset          = GetCLIOptionEnum ( computingris, __preset );

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


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                     numsubjects     = 0;
int                     numconditions   = 0;
TGoGoF                  subjects;

if ( /*listfiles.IsEmpty () &&*/ gof.IsNotEmpty () ) {
                                        // files in the command-line are assumed to be from the same subject
                                        // we just have to differentiate between epochs and averages/spontaneous
    numsubjects     = 1;
    numconditions   = gof.NumFiles ();
    subjects        = gof;
    }

//else { // listfiles
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! HasCLIOption ( computingris, __inversefile ) ) {

    ConsoleErrorMessage ( __inversefile, "Missing Inverse Matrix file!" );
    return;
    }

TGoF                inversefiles ( GetCLIOptionFile ( computingris, __inversefile ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           xyzfile         = GetCLIOptionFile ( computingris, __xyzfile  );
                                        // !it is enough to provide the xyzfile to activate the default spatial filter!
SpatialFilterType   spatialfilter   = SpatialFilterDefault;

if ( CanOpenFile ( xyzfile )
  && HasCLIOption ( computingris, __spatialfilter ) ) {

    spatialfilter   = TextToSpatialFilterType ( GetCLIOptionEnum ( computingris, __spatialfilter ).c_str () );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string                  reg         = GetCLIOptionEnum ( computingris, __regularization );

RegularizationType      regularization  = reg == __regauto ? RegularizationAutoGlobal
                                        : reg == __regnone ? RegularizationNone
                                        :                    Clip ( (RegularizationType) StringToInteger ( reg.c_str () ), FirstRegularization, LastRegularization );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string                  zscore      = GetCLIOptionEnum ( computingris, __zscore );

BackgroundNormalization backnorm    = zscore == __compute   ? BackgroundNormalizationComputingZScore
                                    : zscore == __loadfile  ? BackgroundNormalizationLoadingZScoreFile
                                    :                         BackgroundNormalizationNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data types - we need some extra care here

                                        // INTERMEDIATE Epochs data type: ERP always using Vectors; Induced always using Norm; there are currently no other epochs cases
AtomType            datatypeepochs  = CRISPresets[ esicase ].GetAtomTypeEpochs ();

                                        // FINAL requested data type
if ( ! HasCLIOption ( computingris, __typeformat ) ) {

    ConsoleErrorMessage ( __typeformat, "Output format type not provided!" );
    return;
    }

string              typeformat      = GetCLIOptionEnum ( computingris, __typeformat );
AtomType            datatypefinal   = typeformat == __norm      ?   AtomTypePositive
                                    : typeformat == __vector    ?   AtomTypeVector
                                    :                               AtomTypePositive;

                                        // some silent safety overrides here - difficult to implement on the CLI, so this will run "silently"
if ( datatypefinal == AtomTypeVector   && ! CRISPresets[ esicase ].CanVect () )     datatypefinal   = AtomTypePositive;
if ( datatypefinal == AtomTypePositive && ! CRISPresets[ esicase ].CanNorm () )     datatypefinal   = AtomTypeVector;

                                        // ACTUAL processing data type + Z-Score + Envelope
AtomType            datatypeproc    = CRISPresets[ esicase ].GetAtomTypeProcessing ( datatypeepochs, datatypefinal );

                                        // check compatiblity betweeen these 2
                    datatypefinal   = CRISPresets[ esicase ].GetAtomTypeFinal ( datatypeproc, datatypefinal );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // epochs / subjects initial ranking
bool                ranking         = HasCLIFlag ( computingris, __ranking );
                                        // not available for the moment
bool                thresholding    = false;    // note that only the combo ranking + thresholding is currently allowed
                                        // Using a fixed threshold at each step
double              keepingtopdata  = 0.10;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Envelope mainly for Induced filtered data - but option is still open for some other cases for the moment
bool                envelope            = false;
FilterTypes         envelopetype        = FilterTypeNone;
double              envelopeduration    = 0;

                                        // if option exists + one more check on data type
if ( HasCLIOption ( computingris, __envelope ) && datatypefinal == AtomTypePositive ) {

    envelopeduration    = GetCLIOptionDouble  ( computingris, __envelope );

    if ( envelopeduration > 0 ) {
        envelope            = true;
        envelopetype        = RisEnvelopeMethod;
        }
    else {
        envelope            = false;
        envelopetype        = FilterTypeNone;
        envelopeduration    = 0;
        }
    }

                                        // a bit late in the game, but we can not compute induced response without the envelope
if ( CRISPresets[ esicase ].IsInduced () && ! envelope ) {

    ConsoleErrorMessage ( __envelope, "Envelope not specified for Induced case!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           roisfile        = GetCLIOptionFile ( computingris, __roisfile );

bool                roiing          = CanOpenFile ( roisfile ) // roisfile.IsNotEmpty ()
                                   && ! IsVector ( datatypeproc );  // we have to do something to forbid ROIing on non-scalar data


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tracks output options - some silent safety overrides here, too
bool                savingindividualfiles   = HasCLIFlag ( computingris, __savingsubjects       );
bool                savingepochfiles        = HasCLIFlag ( computingris, __savingsubjectsepochs ) && CRISPresets[ esicase ].IsEpochs        ();
bool                computegroupsaverages   = HasCLIFlag ( computingris, __savinggrandaverage   ) && CRISPresets[ esicase ].CanAverageGroup ();
bool                computegroupscentroids  = HasCLIFlag ( computingris, __savingtemplates      ) && CRISPresets[ esicase ].IsCentroids     ();
bool                savingzscorefactors     = HasCLIFlag ( computingris, __savingzscore         )
                                           && backnorm != BackgroundNormalizationNone;
//                                         && backnorm == BackgroundNormalizationComputingZScore;   // note that we could allow a new copy of Z-Scores in case of loading from file...

bool                isprocessing            = savingindividualfiles     
                                           || savingepochfiles        
                                           || computegroupsaverages        
                                           || computegroupscentroids
                                           || savingzscorefactors;

if ( ! isprocessing ) {

    ConsoleErrorMessage ( 0, "No output has been selected!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // CanOpenFiles done here
if ( ! inversefiles.CanOpenFiles () ) {

    ConsoleErrorMessage ( __inversefile, "Can not open Inverse Matrix file ", "\"", inversefiles[ 0 ], "\"" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           basedir;
                                        // use first file directory as the base directory
StringCopy      ( basedir,  subjects[ 0 ][ 0 ] );
RemoveFilename  ( basedir );

string              prefix          = GetCLIOptionEnum ( computingris, __prefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();

cout << "Preset:           "    << CRISPresets[ esicase ].Text << NewLine;
cout << "Number subjects:  "    << numsubjects << NewLine;
cout << "Number conds.  :  "    << numconditions << NewLine;
cout << NewLine;

cout << "XYZ file:         "    << xyzfile << NewLine;
if ( xyzfile.IsNotEmpty () )
    cout << "Spatial Filter:   "    << SpatialFilterLongName[ spatialfilter ] << NewLine;
cout << "IS file:          "    << inversefiles[ 0 ] << NewLine;
cout << "Regularization:   "    << reg << NewLine;
cout << "Standardization:  "    << BackgroundNormalizationNames[ backnorm ] << NewLine;
cout << NewLine;

if ( CRISPresets[ esicase ].IsEpochs () )
    cout << "Data Type Epochs: "    << AtomNames[ datatypeepochs ] << NewLine;
cout << "Data Type Proc:   "    << AtomNames[ datatypeproc ] << NewLine;
cout << "Data Type Final:  "    << AtomNames[ datatypefinal ] << NewLine;
cout << "Ranking:          "    << BoolToString ( ranking ) << NewLine;
cout << "Envelope:         "    << BoolToString ( envelope ) << NewLine;
if ( envelope )
    cout << "Envelope Duration:"  << envelopeduration << NewLine;
cout << NewLine;

cout << "Base Dir:         "    << basedir << NewLine;
cout << "File prefix:      "    << prefix << NewLine;
cout << NewLine;

for ( int i = 0; i < (int) gof; i++ )
    cout << "File:         "    << gof[ i ] << NewLine;

cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRis    (   esicase,
                    subjects,                  
                    numsubjects,            numconditions,
                    
                    inversefiles,           regularization,         backnorm,
                    datatypeepochs,         datatypefinal,
                    RisCentroidMethod,

                    spatialfilter,          xyzfile,
                    ranking,
                    thresholding,           keepingtopdata,
                    envelope,               envelopetype,           envelopeduration,
                    roiing,                 roisfile,               RisRoiMethod,

                    savingindividualfiles,  savingepochfiles,       savingzscorefactors,
                    computegroupsaverages,  computegroupscentroids,

                    basedir,                prefix.c_str (),
                    Silent
                );


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
