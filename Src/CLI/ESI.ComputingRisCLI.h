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

DefineCLIOptionEnum     ( computingris,     "",     __preset,               "EEG Preset" RequiredString ";  IndSubjectsEpochs needs: --envelope" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __preset1, __preset2, __preset3, __preset4, __preset5, __preset6, __preset7, __preset8 } ) ) );

DefineCLIOptionFile     ( computingris,     "",     __listfiles,            "A .csv or .txt file containing the input files" );

//ExcludeCLIOptions       ( computingris,     __listfiles,        __files );    // does not seem to work across different CLI11 subcommands


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __inversefile,          "Inverse Matrix file" RequiredString );
//->Required ();    // interferes with --help


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __xyzfile,              "Electrodes coordinates file for Spatial Filter" /*__xyzfile_descr*/ );

                                        // !it is enough to provide the xyzfile to activate the default spatial filter!
DefineCLIOptionEnum     ( computingris,     "",     __spatialfilter,        "Spatial filter" );
NeedsCLIOption          ( computingris,     __spatialfilter,    __xyzfile )
->CheckOption           ( CLI::IsMember ( vector<string> ( SpatialFilterShortName + SpatialFilterOutlier, SpatialFilterShortName + NumSpatialFilterTypes ) ) )
->DefaultString         ( SpatialFilterShortName[ SpatialFilterDefault ] )
->ZeroOrOneArgument;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( computingris,     "",     __typeformat,           "Final file type" RequiredString )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __norm, __vector } ) ) );
// !no default enum!

DefineCLIOptionEnum     ( computingris,     __reg,  __regularization,       "Regularization level" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __regnone, __regauto, __reg0, __reg1, __reg2, __reg3, __reg4, __reg5, __reg6, __reg7, __reg8, __reg9, __reg10, __reg11, __reg12 } ) ) )
->DefaultString         ( __regauto );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( computingris,     "",     __zscore,               "Background activity standardization" )
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

DefineCLIOptionFile     ( computingris,     "",     __roisfile,             __roisfile_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( computingris,     "",     __inputdir,             __inputdir_descr )
->TypeOfOption          ( __inputdir_type )
->CheckOption           ( CLI::ExistingDirectory ); // could be incomplete, but it helps a bit, though

DefineCLIOptionFile     ( computingris,     "",     __outputdir,            __outputdir_descr )
->TypeOfOption          ( __outputdir_type );

DefineCLIOptionString   ( computingris,     "",     __prefix,               __prefix_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Options with short version
DefineCLIFlag           ( computingris,     __savingsubjectsS,          __savingsubjects,           "Saving subjects" );
DefineCLIFlag           ( computingris,     __savingsubjectsepochsS,    __savingsubjectsepochs,     "Saving subjects epochs" );
DefineCLIFlag           ( computingris,     __savinggrandaverageS,      __savinggrandaverage,       "Saving Grand Averages" );
DefineCLIFlag           ( computingris,     __savingtemplatesS,         __savingtemplates,          "Saving Templates" );
DefineCLIFlag           ( computingris,     __savingzscoreS,            __savingzscore,             "Saving Z-Score Factors" );

ExcludeCLIOptions       ( computingris,     __savingtemplates,      __savingsubjects );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( computingris,     __h,    __help,                 __help_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating positional files option seems OK
DefineCLIOptionFiles    ( computingris, -1, "",     __files,                __files_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     ComputingRisCLI ( CLI::App* computingris )
{
if ( ! IsSubCommandUsed ( computingris )  )
    return;


TFileName           inputdir        = GetCLIOptionDir   ( computingris, __inputdir );
TGoF                gof             = GetCLIOptionFiles ( computingris, __files,     __inputdir );
TFileName           listfiles       = GetCLIOptionFile  ( computingris, __listfiles, __inputdir );

if ( gof.IsEmpty () && listfiles.IsEmpty () ) {

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

int                 numsubjects     = 0;
int                 numconditions   = 0;
TGoGoF              subjects;
TGoF                csvinverses;

                                        // files on the command-line AND in a csv files are mutually exclusive
if      ( listfiles.IsEmpty () && gof.IsNotEmpty () ) {
                                        // files in the command-line are assumed to be from the same subject
                                        // we just have to differentiate between epochs and averages/spontaneous
    numsubjects     = 1;
    numconditions   = gof.NumFiles ();
    subjects        = gof;
    }

else if ( listfiles.IsNotEmpty () && gof.IsEmpty () ) {
                                        // in this case, we can recover some individual inverses
    if ( ! ReadCsvRis ( listfiles, subjects, csvinverses ) ) {

        ConsoleErrorMessage ( __listfiles, "The provided list of files does not seem to be correct: ", "\"", listfiles, "\"" );
        return;
        }

    numsubjects     = NumSubjects   ( subjects );
    numconditions   = NumConditions ( subjects );
    }

else { // if ( listfiles.IsNotEmpty () && gof.IsNotEmpty () ) {
                                        // not allowed to mix command-line and csv file
    ConsoleErrorMessage ( __listfiles, "Can not mix a list of files with files on the command-line!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                cliinverse;

if ( HasCLIOption ( computingris, __inversefile ) )
                                        // try to read a single inverse file parameter
    cliinverse  = TGoF ( GetCLIOptionFile ( computingris, __inversefile, __inputdir ) );

                                        // comment this to allow a single inverse file parameter to override those from the list of files
//if ( HasInverses ( csvinverses ) && HasInverses ( cliinverse ) ) {
//
//    ConsoleErrorMessage ( __listfiles, "Can not mix Inverse Matrix files from a list of files, and another one from the command-line!" );
//    return;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do we have inverses now?
TGoF                inverses        = cliinverse .IsNotEmpty () ? cliinverse    // prioritize command-line override
                                    : csvinverses.IsNotEmpty () ? csvinverses   // else use the list of files
                                    :                             0;

if ( ! HasInverses ( inverses ) ) {

    ConsoleErrorMessage ( __inversefile, "Missing Inverse Matrix file(s)!" );
    return;
    }


bool                singleinverse       = IsSingleInverse       ( inverses );
bool                matchinginverses    = AreMatchingInverses   ( inverses, subjects );

                                        // these are the only legit cases
if ( ! ( singleinverse || matchinginverses ) ) {

    ConsoleErrorMessage ( __inversefile, "Number of Inverse Matrix file(s) does not seem correct!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasCLIOption ( computingris, __xyzfile ) && esicase == ComputingRisPresetFreq ) {

    ConsoleErrorMessage ( __xyzfile, "Can not use a Spatial Filter for the Frequency case!" );
    return;
    }


TFileName           xyzfile         = GetCLIOptionFile ( computingris, __xyzfile, __inputdir );
SpatialFilterType   spatialfilter   = SpatialFilterNone;


if ( CanOpenFile ( xyzfile ) && esicase != ComputingRisPresetFreq ) {
                                        // !it is enough to provide the xyzfile to activate the default spatial filter!
    if ( HasCLIOption ( computingris, __spatialfilter ) )

        spatialfilter   = TextToSpatialFilterType ( GetCLIOptionEnum ( computingris, __spatialfilter ).c_str () );
    else 
        spatialfilter   = SpatialFilterDefault;
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
                                        // not available for the moment - also only the combo ranking + thresholding is currently possible
bool                thresholding    = false; // ranking;

double              keepingtopdata  = ESICentroidTopData;


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

TFileName           roisfile        = GetCLIOptionFile ( computingris, __roisfile, __inputdir );

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
if ( ! inverses.CanOpenFiles () ) {

    ConsoleErrorMessage ( __inversefile, "Some error occured while reading the provided Inverse Matrix file(s)..." );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if empty, function will use some default
TFileName           outputdir       = GetCLIOptionDir   ( computingris, __outputdir );

string              prefix          = GetCLIOptionEnum  ( computingris, __prefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();


TVerboseFile        verbose ( "cout", VerboseFileDefaultWidth );
char                buff[ KiloByte ];

verbose.NextTopic ( "Data Preprocessing:" );
{
verbose.Put ( "Spatial Filter:", SpatialFilterLongName[ spatialfilter ] );
if ( spatialfilter != SpatialFilterNone )
    verbose.Put ( "Electrodes Coordinates file:", xyzfile );
}


verbose.NextTopic ( "Inverse Processing:" );
{
verbose.Put ( "Preset:", CRISPresets[ esicase ].Text );

verbose.NextLine ();

verbose.Put ( "Inverse Matrix case:", matchinginverses ? "Individual Inverse Matrices" : numsubjects > 1 ? "Common Inverse Matrix" : "Single Inverse Matrix" );
verbose.Put ( "Inverse Matrix file:", matchinginverses ? "See Input File section" : inverses[ 0 ] );
verbose.Put ( "Regularization level:", RegularizationToString ( regularization, buff, false ) );

verbose.NextLine ();

if ( CRISPresets[ esicase ].IsEpochs () ) {
    if      ( IsPositive ( datatypeepochs ) )   verbose.Put ( "Epochs data type:", "Positive Data (Norm of Vectors)" );
    else if ( IsVector   ( datatypeepochs ) )   verbose.Put ( "Epochs data type:", "3D Vectorial Data" );
    }


if ( CRISPresets[ esicase ].IsEpochs () ) {
    if      ( IsPositive ( datatypeepochs ) )        verbose.Put ( "Epochs data type:", "Positive Data (Norm of Vectors)" );
    else if ( IsVector   ( datatypeepochs ) )        verbose.Put ( "Epochs data type:", "3D Vectorial Data" );
    }

if      ( IsPositive ( datatypeproc  ) )        verbose.Put ( "Proc. data type:", "Positive Data (Norm of Vectors)" );
else if ( IsVector   ( datatypeproc  ) )        verbose.Put ( "Proc. data type:", "3D Vectorial Data" );

if      ( IsPositive ( datatypefinal ) )        verbose.Put ( "Final data type:", "Positive Data (Norm of Vectors)" );
else if ( IsVector   ( datatypefinal ) )        verbose.Put ( "Final data type:", "3D Vectorial Data" );
}


verbose.NextTopic ( "Inverse Postprocessing:" );
{
verbose.Put ( "Standardizing results:", BackgroundNormalizationNames[ backnorm ] );

verbose.Put ( "Ranking results:", ranking );

verbose.Put ( "Thresholding results:", thresholding );
//if ( thresholding )
//    verbose.Put ( "Threshold level:", threshold, 2 );

verbose.Put ( "Envelope of results:", envelope );
if ( envelope ) {
    verbose.Put ( "Envelope method:", FilterPresets[ envelopetype ].Text );
    verbose.Put ( "Envelope lowest frequency:", MillisecondsToFrequency ( envelopeduration ), 2, " [Hz]" );
    verbose.Put ( "Envelope duration:", envelopeduration, 2, " [ms]" );
    }

verbose.Put ( "Applying ROIs:", roiing );
if ( roiing ) {
    verbose.Put ( "ROIs file:", roisfile );
    verbose.Put ( "ROIs method:", FilterPresets[ RisRoiMethod ].Text );
    }
}


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Input directory:",       inputdir.IsEmpty () ? "None" : inputdir );
verbose.Put ( "Number of subjects:",    numsubjects   );
verbose.Put ( "Number of conditions:",  numconditions );


for ( int si = 0; si < subjects.NumGroups (); si++ ) {

    verbose.NextLine ();
    verbose.Put ( esicase     == ComputingRisPresetErpGroupMeans ?  "Group #:"  // more precise
                :                                                   "Subject #:", si + 1 );
    verbose.Put ( "Number of EEG files:", subjects[ si ].NumFiles () );


    if ( CRISPresets[ esicase ].IsEpochs () ) {

        TGoGoF              splitgogof;
        TStrings            splitnames;

        subjects[ si ].SplitByNames ( "." InfixEpoch, splitgogof, &splitnames );

        verbose.Put ( "Number of groups of epochs:", splitgogof.NumGroups () );

        for ( int gofi = 0; gofi < splitgogof.NumGroups (); gofi++ )
        for ( int fi = 0; fi < splitgogof[ gofi ].NumFiles (); fi++ )
            verbose.Put ( fi ? "" : StringCopy ( buff, "Epochs Group #", IntegerToString ( gofi + 1 ), ":" ), splitgogof[ gofi ][ fi ] );
        }
    else 
        for ( int fi = 0; fi < subjects[ si ].NumFiles (); fi++ )
            verbose.Put ( "", subjects[ si ][ fi ] );


    if      ( matchinginverses )    verbose.Put ( "Individual Inverse Matrix file:", inverses[ si ] );
    else if ( numsubjects > 1  )    verbose.Put ( "Common Inverse Matrix file:",     inverses[  0 ] );
    else                            verbose.Put ( "Inverse Matrix file:",            inverses[  0 ] );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Output directory:",                      outputdir.IsEmpty () ? "None" : outputdir );
verbose.Put ( "Output file prefix:",                    prefix   .empty   () ? "None" : prefix    );

verbose.NextLine ();

verbose.Put ( "Saving every individual ris files:",     savingindividualfiles   );
verbose.Put ( "Saving every epoch ris files:",          savingepochfiles        );
verbose.Put ( "Computing each groups' averages:",       computegroupsaverages   );
verbose.Put ( "Computing each groups' centroids:",      computegroupscentroids  );
verbose.Put ( "Saving standardization factors files:",  savingzscorefactors     );
}


verbose.Close ();

cout << NewLine;
cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRis    (   esicase,
                    subjects,                  
                    numsubjects,            numconditions,
                    
                    inverses,               regularization,         backnorm,
                    datatypeepochs,         datatypefinal,
                    ESICentroidMethod,

                    spatialfilter,          xyzfile,
                    ranking,
                    thresholding,           keepingtopdata,
                    envelope,               envelopetype,           envelopeduration,
                    roiing,                 roisfile,               RisRoiMethod,

                    savingindividualfiles,  savingepochfiles,       savingzscorefactors,
                    computegroupsaverages,  computegroupscentroids,

                    outputdir,
                    prefix.c_str (),
                    Silent
                );


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
