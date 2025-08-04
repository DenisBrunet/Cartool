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

#include    "TMicroStates.h"
#include    "TMicroStatesFitDialog.h"

#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "TArray4.h"
#include    "TVector.h"
#include    "Files.TVerboseFile.h"
#include    "Files.SpreadSheet.h"
#include    "Math.Histo.h"

#include    "CartoolTypes.h"            // SkippingEpochsType
#include    "TMarkers.h"
#include    "TExportTracks.h"
#include    "BadEpochs.h"

#include    "TFreqCartoolDoc.h"         // TFreqFrequencyName

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Save selected groups/maps/variables to a .csv file
void        WriteVariablesToFile    (   const char*         filename,       TArray3<double>&    var,
                                        const TSelection&   groupsel,       const TSelection&   mapsel,         const TSelection&   varout,
                                        int                 subji,          int                 relgroupoffset,
                                        const char*         subjname,
                                        bool                linesassamples, bool                variablesshortnames 
                                    )
{
if ( StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char                buff                        [ 2 * MaxPathShort ];
char                LocaleListSeparator [ 5 ];

GetLocaleListSeparator ( LocaleListSeparator );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create for first subject, append on the next ones
ofstream            ofv ( TFileName ( filename, TFilenameExtendedPath ), subji == 0 ? ios::out : ios::out | ios::app );

ofv << StreamFormatFixed;


                                        // Output first line of headers
if ( subji == 0 ) {

    if ( linesassamples ) {             // Cartool default
                                        // first column name
        ofv << FitSubjectNameLong;

                                        // then all combinations of variables names
        for ( TIteratorSelectedForward absgi ( groupsel ); (bool) absgi; ++absgi )
        for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    )
        for ( TIteratorSelectedForward vi    ( varout );   (bool) vi;    ++vi    ) {

            StringCopy      ( buff, variablesshortnames ? FitGroupNameShort : FitGroupNameLong );
            StringAppend    ( buff, IntegerToString ( absgi() + 1 ) );
            StringAppend    ( buff, "_" );

            StringAppend    ( buff, variablesshortnames ? FitMapNameShort   : FitMapNameLong   );
            StringAppend    ( buff, IntegerToString ( si()    + 1 ) );
            StringAppend    ( buff, "_" );

            StringAppend    ( buff, FitVarNames[ variablesshortnames ][ vi() ] );

            ofv << LocaleListSeparator;
            ofv << buff;
            } // vi, si, absgi

        ofv << NewLine;
        }

    else {                              // R default - do we really want short names in that case?
        ofv << FitSubjectNameLong;
        ofv << LocaleListSeparator;
        ofv << FitGroupNameLong;
        ofv << LocaleListSeparator;
        ofv << FitMapNameLong;

        for ( TIteratorSelectedForward vi    ( varout );   (bool) vi;    ++vi    ) {
            ofv << LocaleListSeparator;
            ofv << FitVarNames[ /*false*/ variablesshortnames ][ vi() ];
            }

        ofv << NewLine;
        }

    } // if subji


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( linesassamples )
    ofv << subjname;

                                        // actually all groups of current subject
int                 relg            = relgroupoffset;
for ( TIteratorSelectedForward absgi ( groupsel ); (bool) absgi; ++absgi, relg++ ) {

                                        // we MUST have valid data across all groups
                                        // this is tested at data initialization time, so only paired / within-subjects groups with compatible
                                        // files can qualify for multiple groups (of files) at once
                                        // otherwise the fitting MUST be done 1 group at a time

    for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

        if ( ! linesassamples ) {
                                        // output the 3 indexes of the current variable
            ofv << subjname;
            ofv << LocaleListSeparator;
            ofv << FitGroupNameLong << ( absgi() + 1 ); 
            ofv << LocaleListSeparator;
            ofv << FitMapNameLong   << ( si() + 1 );
            }


        for ( TIteratorSelectedForward vi    ( varout );   (bool) vi;    ++vi    ) {
                                        // use different formatting for integer and float
            if ( vi() == fitfonset || vi() == fitloffset || vi() == fitnumtf || vi() == fittfbcorr || vi() == fittfmaxgfp )

                ofv << LocaleListSeparator << (int) var ( relg, vi(), si() );
            else
                ofv << LocaleListSeparator <<       var ( relg, vi(), si() );
                
            } // v


        if ( ! linesassamples )
            ofv << NewLine;
        } // s


    } // for relg/absgi()


if ( linesassamples )
    ofv << NewLine;
}


//----------------------------------------------------------------------------
                                        // Save selected groups/maps/variables to a .csv file
void        WriteHeaderToFile   (   const char*             filename,       ofstream&       ofm,
                                    int                     gofi1,          int             numwithinsubjects,      int             nclusters,
                                    const TArray2<bool>&    jspmapsel,
                                    int                     subji,
                                    const char*             subjname,
                                    bool                    variablesshortnames 
                                )
{
if ( StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char                buff                [ 256 ];
char                LocaleListSeparator [ 5 ];

GetLocaleListSeparator ( LocaleListSeparator );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create for first subject, append on the next ones
ofm.open ( filename, subji == 0 ? ios::out : ios::out | ios::app );


ofm << StreamFormatFixed;


                                        // Output first line of headers
if ( subji == 0 ) {
                                        // first column name
    ofm << FitSubjectNameLong;

                                            // first line with variables names
    for ( int relg = 0, absg = gofi1 + relg; relg < numwithinsubjects; relg++, absg++ )
    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
    for ( int destmap = 0; destmap < nclusters + 1; destmap++ ) {

        if ( ! jspmapsel ( frommap, destmap ) )
            continue;

        StringCopy      ( buff, variablesshortnames ? FitGroupNameShort   : FitGroupNameLong   );
        StringAppend    ( buff, IntegerToString ( absg + 1 ) );
        StringAppend    ( buff, "_" );

        StringAppend    ( buff, variablesshortnames ? FitFromMapNameShort : FitFromMapNameLong );
//      StringAppend    ( buff, IntegerToString ( frommap + 1 )  );
                                        // substitute the last map with "Unlabel"
        StringAppend    ( buff, frommap < nclusters ? IntegerToString ( frommap + 1 ) : ( variablesshortnames ? FitUnlabeledNameShort : FitUnlabeledNameLong ) );
        StringAppend    ( buff, "_" );

        StringAppend    ( buff, variablesshortnames ? FitToMapNameShort   : FitToMapNameLong   );
//      StringAppend    ( buff, IntegerToString ( destmap + 1 )  );
                                        // substitute the last map with "Unlabel"
        StringAppend    ( buff, destmap < nclusters ? IntegerToString ( destmap + 1 ) : ( variablesshortnames ? FitUnlabeledNameShort : FitUnlabeledNameLong ) );

        
        ofm << LocaleListSeparator;
        ofm << buff;
        } // relg, frommap, destmap

    ofm << NewLine;
    } // subji == 0


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ofm << subjname;
} 


//----------------------------------------------------------------------------
bool    TMicroStates::BackFitting       (   const char*         templatefile,
                                            TGoGoF&             gogof,              int                     numwithinsubjects,
                                            AnalysisType        analysis,           ModalityType            modality,
                                            TStrings            epochfrom,          TStrings                epochto,            TStrings            epochmaps,  // !copies!

                                            SpatialFilterType   spatialfilter,      const char*             xyzfile,
                                            SkippingEpochsType  badepochs,          const char*             listbadepochs,

                                            AtomType            datatype,           PolarityType            polarity,           ReferenceType       dataref,
                                            bool                dolimitcorr,        double                  limitcorr,

                                            bool                gfpnormalize,
                                            bool                noncompetitive,

                                            bool                smoothing,          int                     smoothinghalfsize,  double              smoothinglambda,
                                            bool                rejectsmall,        int                     rejectsize,

                                            bool                markov,             int                     markovtransmax,

                                            const TSelection&   varout,
                                            MicroStatesOutFlags outputflags,
                                            const char*         basefilename
                                        )
{
if ( StringIsEmpty ( templatefile ) || gogof.IsEmpty () || StringIsEmpty ( basefilename ) )
    return  false;

if ( analysis == UnknownAnalysis 
  || modality == UnknownModality )
    return  false;

//if ( varout.IsNoneSet () )
//    return  false;


if ( spatialfilter != SpatialFilterNone && StringIsEmpty ( xyzfile ) )
    spatialfilter   = SpatialFilterNone;

if ( dolimitcorr && limitcorr <= MinCorrelationThreshold ) {
    dolimitcorr     = false;
    limitcorr       = IgnoreCorrelationThreshold;
    }

if ( smoothing && ( smoothinghalfsize <= 0 || smoothinglambda <= 0 ) ) {
    smoothing           = false;
    smoothinghalfsize   = 0;
    smoothinglambda     = 0;
    }

if ( rejectsmall && rejectsize <= 0 )
    rejectsmall = false;

if ( markov && markovtransmax <= 0 )
    markov      = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Decode output flags
bool                variablesshortnames         = IsFlag ( outputflags, VariablesShortNames         );

bool                writeexcelfiles             = IsFlag ( outputflags, SheetLinesAsSamples         );
bool                writeRfiles                 = IsFlag ( outputflags, SheetColumnsAsFactors       );
bool                saveonefilepergroup         = IsFlag ( outputflags, SaveOneFilePerGroup         );
bool                saveonefilepervariable      = IsFlag ( outputflags, SaveOneFilePerVariable      );

bool                writesegfiles               = IsFlag ( outputflags, WriteSegFiles               );
bool                writeclustersfiles          = IsFlag ( outputflags, WriteClustersFiles          );
bool                writeemptyclusters          = IsFlag ( outputflags, WriteEmptyClusters          );
bool                writenormalizedclusters     = IsFlag ( outputflags, WriteNormalizedClusters     );
bool                writestatdurationsfiles     = IsFlag ( outputflags, WriteStatDurationsFiles     );
bool                writecorrelationfiles       = IsFlag ( outputflags, WriteCorrelationFiles       );
bool                writesegfrequencyfiles      = IsFlag ( outputflags, WriteSegFrequencyFiles      );

bool                owningfiles                 = IsFlag ( outputflags, OwningFiles                 );
bool                deletetempfiles             = IsFlag ( outputflags, DeleteTempFiles             );


if ( ! owningfiles && deletetempfiles )
    deletetempfiles  = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                alldatris       = gogof.AllExtensionsAre    ( AllRisFilesExt );
bool                allmapris       = IsExtensionAmong          ( templatefile, AllRisFilesExt );
bool                allris          = alldatris && allmapris;

bool                alldatrisv      = alldatris && gogof.AllExtensionsAre ( AllRisFilesExt, AtomTypeVector );
char                isscalar;
bool                allmaprisv      = allmapris && ReadFromHeader ( templatefile, ReadInverseScalar, &isscalar ) && ! isscalar;
bool                allrisv         = alldatrisv && allmaprisv;

                                        // Check ESI parameters - noESI computation here, either we have EEG or RIS
bool                isesipreset     = modality == ModalityESI && allris;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // downgrade vectorial type to scalar if not all data are vectorial
if ( IsVector ( datatype ) && ! allrisv )                   datatype    = AtomTypePositive;

                                        // this also shouldn't happen...
if ( IsVector ( datatype ) && spatialfilter != SpatialFilterNone )
    spatialfilter   = SpatialFilterNone;


CheckReference ( dataref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                competitive         = ! noncompetitive;

                                        // Markov Chains parameters
bool                markovsavefreq      = markov;
bool                markovsavecsv       = markov;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDir;
//TFileName         BaseDirMore;
TFileName           BaseDirSegmentation;
TFileName           BaseDirCorrelation;
TFileName           BaseDirStatDurations;
TFileName           BaseDirSegFrequency;
TFileName           BaseDirMarkov;
TFileName           BaseDirDataCluster;
//TFileName         BaseDirSubjects;

TFileName           BaseFileName;
//TFileName         BaseFileNameMore;
TFileName           BaseFileNameSegmentation;
TFileName           BaseFileNameCorrelation;
TFileName           BaseFileNameStatDurations;
TFileName           BaseFileNameSegFrequency;
TFileName           BaseFileNameMarkov;
TFileName           BaseFileNameDataCluster;
TFileName           fileoutprefix;
TFileName           buff;


StringCopy      ( BaseDir,              basefilename                                 );
//StringCopy    ( BaseDirMore,          BaseDir,        "\\",      DirMore           );
StringCopy      ( BaseDirSegmentation,  BaseDir,        "\\",      InfixSegmentation );
StringCopy      ( BaseDirCorrelation,   BaseDir,        "\\",      InfixCorrelation  );
StringCopy      ( BaseDirStatDurations, BaseDir,        "\\",      InfixDurations    );
StringCopy      ( BaseDirSegFrequency,  BaseDir,        "\\",      InfixSegFrequency );
StringCopy      ( BaseDirMarkov,        BaseDir,        "\\",      InfixMarkov       );
StringCopy      ( BaseDirDataCluster,   BaseDir,        "\\",      DirDataClusters   );
//StringCopy    ( BaseDirSubjects,      BaseDir );
//RemoveFilename( BaseDirSubjects );    // for now, assume subjects are 1 directory above


CreatePath      ( BaseDir,     false );
//CreatePath    ( BaseDirMore, false );


                                        // extract string "prefix"
StringCopy      ( fileoutprefix,            ToFileName ( basefilename ) );
                                        // clean prefix from any extension(?)
if ( IsExtensionAmong ( fileoutprefix, FILEEXT_SEG " " FILEEXT_VRB ) )
    RemoveExtension ( fileoutprefix );
                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy      ( BaseFileName,              BaseDir,                "\\",   fileoutprefix,  "." );
//StringCopy    ( BaseFileNameMore,          BaseDirMore,            "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameSegmentation,  BaseDirSegmentation,    "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameCorrelation,   BaseDirCorrelation,     "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameStatDurations, BaseDirStatDurations,   "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameSegFrequency,  BaseDirSegFrequency,    "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameMarkov,        BaseDirMarkov,          "\\",   fileoutprefix,  "." );
StringCopy      ( BaseFileNameDataCluster,   BaseDirDataCluster,     "\\",   fileoutprefix,  "." );


if ( StringLength ( BaseFileName ) > MaxPathShort - 32 ) {

//  ShowMessage ( "Base File Name is too long to generate a correct output...", BaseFileName, ShowMessageWarning );

    Gauge.Finished ();
    return  false;
    }


                                        // delete any previous files, with my prefix only!
StringCopy  ( buff,  BaseFileName,              "*" );
DeleteFiles ( buff );

//StringCopy  ( buff,  BaseFileNameMore,          "*" );
//DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameSegmentation,  "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameCorrelation,   "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameStatDurations, "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameSegFrequency,  "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameMarkov,        "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameDataCluster,   "*" );
DeleteFiles ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Move the given files to our output directory
constexpr char*     FittingPreProcGroup = "PreprocGroup";

if ( owningfiles ) {
                                        // move & rename each group of files into separate directories, including buddy files, and updates the gof
//    gogof.CopyFilesTo ( BaseDir, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );


    for ( int gofi0 = 0; gofi0 < gogof.NumGroups (); gofi0++ ) {

        StringCopy  ( buff, BaseDir, "\\", FittingPreProcGroup, IntegerToString ( gofi0 + 1 ) );

        gogof[ gofi0 ].CopyFilesTo ( buff, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
        }

    }


//for ( int gofi0 = 0; gofi0 < gogof.NumGroups (); gofi0++ ) {
//    char        buff[256 ];
//    sprintf ( buff, "GoGoF %0d on %0d: final preprocessed, #=%0d", gofi0 + 1, (int) gogof, (int) gogof[ gofi0 ] );
//    gogof.Show ( gofi0, gofi0, buff );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters for ESI data & templates
bool                ranking             = isesipreset;

                                        // Processing type and reference
ReferenceType       processingref       = GetProcessingRef ( isesipreset ? ProcessingReferenceESI : ProcessingReferenceEEG );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finish base directories creation
if ( writesegfiles          )   CreatePath  ( BaseDirSegmentation,      false );
if ( writeclustersfiles     )   CreatePath  ( BaseDirDataCluster,       false );
if ( writecorrelationfiles  )   CreatePath  ( BaseDirCorrelation,       false );
if ( writestatdurationsfiles)   CreatePath  ( BaseDirStatDurations,     false );
if ( writesegfrequencyfiles )   CreatePath  ( BaseDirSegFrequency,      false );
if ( markov                 )   CreatePath  ( BaseDirMarkov,            false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Init gauge
Gauge.Set       ( FittingTitle );

Gauge.AddPart   ( gaugefitglobal,             2,  5 );

//Gauge.AddPart ( gaugefitinit, 0, 20 );
Gauge.AddPart ( gaugefitinit,
                gogof.NumFiles ( 0, gogof.NumGroups () - 1 )
              + gogof.NumFiles ( 0, numwithinsubjects  - 1 ), 15 );

Gauge.AddPart ( gaugefitlabeling,           0, 15 );
Gauge.AddPart ( gaugefitrejectsmall,        0, 15 );
Gauge.AddPart ( gaugefitextractvariables,   0, 15 );
Gauge.AddPart ( gaugefitsegments,           0, 15 );    // will account for segments extraction, duration and Markov computations
Gauge.AddPart ( gaugefitwritecorr,          gogof.NumFiles ( 0, gogof.NumGroups () - 1 ), 10 );
Gauge.AddPart ( gaugefitwriteseg,           0, 10 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // add all files of all groups
                                        // do the processing with slightly different parameters from the input, if needed
Gauge.CurrentPart  = gaugefitinit;

                                        // doing a simple allocation (and not reading any data in) so we can init variables
AllocateVariables   (   gogof, 0, numwithinsubjects - 1, 
                        0,
                        datatype 
                    );

                                        // do we really need to update EnumEpochs here?
//SetEnumEpochs ( MaxNumTF );

                                        // find biggest time range for proper initialization
MaxNumTF    = gogof.GetMaxNumTF ();

int                 numgroups       = gogof.NumGroups ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//char              EpochString      [ 256 ];
//char              MapsString       [ 256 ];
char                GroupString      [ 256 ];
TGoF                SubjectName;
char                MainExt          [ 256 ];
//TFileName         SegFile;
TFileName           MainVerbFile;
TFileName           BaseCsvFile;
TFileName           ExcelFile;
TFileName           RFile;
TFileName           MarkovFile;
TFileName           FilePrefix;
TFileName           FileSuffix;
//TFileName         DataFile;
TFileName           TemplateFileName;
TFileName           CopyTemplateFile;
TFileName           SegFileName;
TFileName           CopySegFile;
TFileName           DurationFile;
TFileName           SegVerbFileName;


StringCopy ( MainVerbFile,              BaseFileName,                                               FILEEXT_VRB );
//StringCopy ( SegFile,                   BaseFileName,                                               FILEEXT_SEG );

                                        // it is quite convenient to have a copy of these 2 files:
                                        // copy the template file
StringCopy          ( TemplateFileName, templatefile );
CopyFileToDirectory ( TemplateFileName, BaseDir, CopyTemplateFile );
                                        // copy the segmentation file - guessed from the template file
StringCopy          ( SegFileName,      TemplateFileName );
ReplaceExtension    ( SegFileName,      FILEEXT_SEG );
CopyFileToDirectory ( SegFileName,      BaseDir, CopySegFile );


StringCopy          ( SegVerbFileName,  TemplateFileName );
ReplaceExtension    ( SegVerbFileName,  FILEEXT_VRB );



char                LocaleListSeparator [ 5 ];

GetLocaleListSeparator ( LocaleListSeparator );

                                        // setting a default file extension according to the input data type
StringCopy          ( MainExt,  isesipreset ? FILEEXT_RIS : FILEEXT_EEGSEF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // number of TF in template file -> number of clusters
int                 nclusters;
ReadFromHeader ( TemplateFileName, ReadNumTimeFrames, &nclusters );


int                 reqmaxclusters  = nclusters;


int                 lsnumel;
ReadFromHeader ( TemplateFileName, ReadNumElectrodes, &lsnumel );

if ( lsnumel != NumElectrodes ) {

//  ShowMessage (   "The number of electrodes/solution points does not match" NewLine 
//                  "between the Maps / Templates file and the Data files!", 
//                  FittingTitle, ShowMessageWarning );

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the sampling frequency from the first file
double              samplingfrequency   = gogof.GetSamplingFrequency ();

//if ( samplingfrequency == 0 )
//    DBGM ( "No Sampling Frequency were found among all your input files!", FittingTitle );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no epoch -> default is the whole time range, and using all the maps
if (   epochfrom.IsEmpty () 
    || epochto  .IsEmpty () 
    || epochmaps.IsEmpty () ) {

    epochfrom.Reset ();
    epochto  .Reset ();
    epochmaps.Reset ();

                                        // set to full time range
    epochfrom.Add ( "0"          );
    epochto  .Add ( MaxNumTF - 1 );
    epochmaps.Add ( "*"          );
    }

                                        // convert & store the epochs strings as actual values
int                 numepochs       = (int) epochfrom;
TArray1<long>       EpochsFrom ( numepochs );
TArray1<long>       EpochsTo   ( numepochs );

                                        // scanning backward, last item is the first being pushed by the user
for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {

    int         epochfromtf     = StringToInteger ( epochfrom[ epochi ] );
    int         epochtotf       = StringToInteger ( epochto  [ epochi ] );

    CheckOrder ( epochfromtf, epochtotf );

    EpochsFrom ( epochi )   = epochfromtf;
    EpochsTo   ( epochi )   = epochtotf;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now we know
int                 bigtotaloffiles = gogof.NumFiles ();

Gauge.SetRange ( gaugefitlabeling,          numepochs * bigtotaloffiles );
Gauge.SetRange ( gaugefitrejectsmall,       numepochs * bigtotaloffiles );
Gauge.SetRange ( gaugefitextractvariables,  numepochs * bigtotaloffiles );
Gauge.SetRange ( gaugefitsegments,          numepochs * bigtotaloffiles );
                                         // big seg                                  per subject seg
Gauge.SetRange ( gaugefitwriteseg,          gogof.NumGroups () / numwithinsubjects + bigtotaloffiles / numwithinsubjects );

Gauge.AdjustOccupy ( false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // non-centered data have different formulas and names

//for ( int i = 0; i < fitnumvar; i++ ) {
//                                        // change Gfp to Rms, or the other way round, according to data type
//    if ( IsAbsolute ( datatype ) )    StringReplace ( FitVarNames[ 0 ][ i ], "Gfp", "Rms" );
//    else                              StringReplace ( FitVarNames[ 0 ][ i ], "Rms", "Gfp" );
//                                        // don't touch the shorter names(?)
////  StringCopy ( FitVarNames[ 1 ][ fitgfptfbcorr ], "R" );
//    }


//TArray1<double>     GEV ( NumFiles );


TArray3<double>     var ( numwithinsubjects, fitnumvar, reqmaxclusters + 1 );   // a big array of all variables, plus one more bucket for cumulated data
double              missingvalue    = FittingMissingValue;                      // at one point, this could be an option provided by user


TSelection          groupsel ( numgroups,       OrderSorted );
TSelection          mapsel   ( reqmaxclusters,  OrderSorted );

//TStrings            mapnames;
//for ( int nc = 0; nc < reqmaxclusters; nc++ )
//    mapnames.Add ( nc + 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init and read template maps
Cartool.CartoolApplication->SetMainTitle    ( "Fitting of", /*FitTransfer.BaseFileName*/ basefilename, Gauge );

Gauge.Next ( gaugefitglobal, SuperGaugeUpdateTitle );


TMaps               templatemaps;
TLabeling           labels;
ReferenceType       templateref         = GetProcessingRef ( isesipreset ? ProcessingReferenceESI : ProcessingReferenceEEG );


templatemaps.ReadFile   (   TemplateFileName,   0,
                            datatype,           ReferenceNone 
                        );

PreprocessMaps          (   templatemaps,
                            false,
                            datatype,   polarity,   dataref,
                            ranking,
                            templateref,
                            true,
                            false
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Markov Chains parameters
TArray4<double>     JSP;                // Joint State Probability (or Joint Probability Mass Function)
TTracks<double>     ProbSeg;            // Probability of each segment
TArray1<int>        state    ( markovtransmax + 1 );
TExportTracks*      expmarkovfreq   = 0;


if ( markov ) {
                                        // allocate as many matrices as needed
                                        // !transition index starts from 1 (easier access)!
                                        // !add 1 more segment for undefined label!
    JSP     .Resize ( numwithinsubjects, markovtransmax + 1, nclusters + 1, nclusters + 1 );
    ProbSeg .Resize ( numwithinsubjects,                     nclusters + 1                );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file init
Gauge.Next ( gaugefitglobal, SuperGaugeUpdateTitle );


TVerboseFile    verbose ( MainVerbFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Back-Fitting" );


                                        // !don't use main or gfppeaks because they can be forced from to other values than the dialogs'!
verbose.NextTopic ( "Data Preprocessing:" );
{
verbose.Put ( "Spatial Filter:", SpatialFilterLongName[ spatialfilter ] );
if ( spatialfilter != SpatialFilterNone )
    verbose.Put ( "Electrodes Coordinates file:", xyzfile );
verbose.NextLine ();


verbose.Put ( "Data levels normalization:", gfpnormalize );
if ( gfpnormalize ) {
//  verbose.Put ( "Data level normalization formula:", IsAbsolute ( datatype ) ? "Mean RMS within subjects" : "Mean GFP within subjects" );
    verbose.Put ( "Normalization computation:", "Re-centering on GFP Background / Mode" );
    }
verbose.NextLine ();


verbose.Put ( "Deleting temporary preprocessed files:", deletetempfiles );
}


verbose.NextTopic ( "Time parameters:" );
{
verbose.Put ( "Number of epochs to be back-fitted:", numepochs );

for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {
    verbose.NextLine ();
    verbose.Put ( "Epoch #:",               numepochs  - epochi );
    verbose.Put ( "From Time Frame:",       EpochsFrom ( epochi ) );
    verbose.Put ( "To   Time Frame:",       EpochsTo   ( epochi ) );
    verbose.Put ( "With Maps / Templates:", epochmaps  [ epochi ] );
    }


verbose.NextLine ();
verbose.Put ( "Excluding bad epochs:", SkippingEpochsNames[ badepochs ] );
if ( badepochs == SkippingBadEpochsList )
    verbose.Put ( "Skipping markers:", listbadepochs );
}


verbose.NextTopic ( "Fitting Method:" );
{
verbose.Put ( "Number of templates:", nclusters );

verbose.NextLine ();
verbose.Put ( "Fitting process is:", noncompetitive ? "Non-competitive" : "Competitive" );

verbose.NextLine ();
verbose.Put ( "Labeling at low Correlations:", ! dolimitcorr );
if ( dolimitcorr )
    verbose.Put ( "No labeling if below Correlation:", limitcorr * 100, 2, " [%]" );


verbose.NextLine ();
if      ( IsScalar   ( datatype ) )     verbose.Put ( "Data type:", "Signed Data" );
else if ( IsPositive ( datatype ) )     verbose.Put ( "Data type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
else if ( IsVector   ( datatype ) )     verbose.Put ( "Data type:", "3D Vectorial Data" );

verbose.Put ( "Data Ranking:",      ranking );
verbose.Put ( "Data reference:",    ReferenceNames[ dataref ] );
verbose.Put ( "Data Correlation reference:", ReferenceNames[ processingref ] );
verbose.Put ( "Data polarity:", IsAbsolute ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
}


verbose.NextTopic ( "Templates:" );
{
if      ( IsScalar   ( datatype ) )     verbose.Put ( "Templates type:", "Signed Data" );
else if ( IsPositive ( datatype ) )     verbose.Put ( "Templates type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
else if ( IsVector   ( datatype ) )     verbose.Put ( "Templates type:", "3D Vectorial Data" );

verbose.Put ( "Templates Ranking:",     ranking );
verbose.Put ( "Templates reference:",   ReferenceNames[ dataref ] );
verbose.Put ( "Templates Correlation reference:", ReferenceNames[ templateref ] );
verbose.Put ( "Templates polarity:",    IsAbsolute ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
}


verbose.NextTopic ( "Temporal Postprocessings:" );
{
verbose.Put ( "Smoothing:", smoothing );
if ( smoothing ) {
    verbose.Put ( "Smoothing half window size:", smoothinghalfsize );
    verbose.Put ( "Smoothing strength (Besag factor):", smoothinglambda );
    }

verbose.NextLine ();
verbose.Put ( "Rejecting small segments:", rejectsmall );
if ( rejectsmall )
    verbose.Put ( "Rejecting segment less or equal to:", rejectsize );

verbose.NextLine ();
if ( samplingfrequency > 0 )
    verbose.Put ( "Sampling Frequency:", samplingfrequency );
else
    verbose.Put ( "Sampling Frequency:", "Missing, results will be in Time Frames" );
}


verbose.NextTopic ( "Variables Computed:" );
{
verbose.Put ( "Number of variables computed:",          (int) varout );
verbose.Put ( "One output file per group of files:",    saveonefilepergroup );
verbose.Put ( "One output file per variable:",          saveonefilepervariable );
verbose.Put ( "Short variable names:",                  variablesshortnames );

if ( (bool) varout )
    verbose.NextLine ();

if ( variablesshortnames )  StringCopy ( buff, FitGroupNameShort "*_" FitMapNameShort "*_" );
else                        StringCopy ( buff, FitGroupNameLong  "*_" FitMapNameLong  "*_" );

for ( TIteratorSelectedForward vi    ( varout );   (bool) vi;    ++vi    ) {

    if      ( vi() == fitfonset        ) verbose.Put ( "First TF of onset:",                   buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitloffset       ) verbose.Put ( "Last TF of offset:",                   buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitnumtf         ) verbose.Put ( "Number of TFs:",                       buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fittfcentroid    ) verbose.Put ( "TF of centroid:",                      buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitmeancorr      ) verbose.Put ( "Mean Correlation:",                    buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitgev           ) verbose.Put ( "Global Explained Variance:",           buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitbcorr         ) verbose.Put ( "Best Correlation value:",              buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fittfbcorr       ) verbose.Put ( "TF of the Best Correlation:",          buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitgfptfbcorr    ) verbose.Put ( "GFP of the TF of Best Correlation:",   buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitmaxgfp        ) verbose.Put ( "Max GFP:",                             buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fittfmaxgfp      ) verbose.Put ( "TF of Max GFP:",                       buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitmeangfp       ) verbose.Put ( "Mean GFP:",                            buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitmeanduration  ) verbose.Put ( "Mean Segment Duration:",               buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fittimecoverage  ) verbose.Put ( "Time Coverage:",                       buff, FitVarNames[ variablesshortnames ][ vi() ] );
    else if ( vi() == fitsegdensity    ) verbose.Put ( "Segment Count Density:",               buff, FitVarNames[ variablesshortnames ][ vi() ] );
    }
}


verbose.NextTopic ( "Markov Chains:" );
{
verbose.Put ( "Computing Joint States Probabilities:", markov );
verbose.Put ( "Computing Markov Transition Matrices:", markov );

if ( markov ) {
    verbose.Put ( "Using as a Markov state:", "Each Microstate" );
    verbose.Put ( "Number of transitions ahead:", markovtransmax );
    }
}


verbose.NextTopic ( "Resting States:" );
{
verbose.Put ( "Histograms of Segments durations:", writestatdurationsfiles );
verbose.Put ( "Computing Segments frequencies:", writesegfrequencyfiles );
verbose.Put ( "Time Course of Correlations:", writecorrelationfiles );
}


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Template file:", TemplateFileName );

verbose.NextLine ();
verbose.Put ( "Number of groups of files:", numgroups );
verbose.Put ( "# of Consecutive groups within subjects:", numwithinsubjects );


for ( int gogofi = 0; gogofi < (int) gogof; gogofi++ ) {

    verbose.NextLine ();
    verbose.Put ( "Group #:", gogofi + 1 );
    verbose.Put ( "Number of files:", gogof[ gogofi ].NumFiles () );

    for ( int fi = 0; fi < gogof[ gogofi ].NumFiles (); fi++ )
        verbose.Put ( StringCopy ( buff, "File ", IntegerToString ( fi + 1, NumIntegerDigits ( gogof[ gogofi ].NumFiles () ) ) ), gogof[ gogofi ][ fi ] );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Verbose file (this):", MainVerbFile );
verbose.Put ( "Copy of Template file:", CopyTemplateFile );
verbose.Put ( "Copy of Segmentation file:", CopySegFile );


if ( writeexcelfiles || writeRfiles ) {
    verbose.NextLine ();


    StringCopy      ( buff,          BaseFileName );
    StringAppend    ( buff,          numepochs > 1 ? "Epoch *.Maps *." : "" );
    StringAppend    ( buff,          InfixGroup, StringPlural ( numwithinsubjects > 1 && ! saveonefilepergroup ), "*." InfixCartoolWorksheet "." FILEEXT_CSV );

    if ( writeexcelfiles ) {
        verbose.Put ( "Worksheet file for Cartool:", buff );
        }


    if ( writeRfiles ) {
        StringReplace ( buff, InfixCartoolWorksheet, InfixRWorksheet );
        verbose.Put ( "Worksheet file for R:", buff );
        }

    }


if ( writesegfiles ) {
    verbose.NextLine ();

    StringCopy      ( buff,          BaseFileName, InfixGroup, StringPlural ( numwithinsubjects > 1 /*&& ! saveonefilepergroup*/ ), "*." FILEEXT_SEG );
    verbose.Put ( "Segmentation file,  all subjects:", buff );


    StringCopy      ( buff,          BaseFileNameSegmentation, InfixGroup, StringPlural ( numwithinsubjects > 1 /*&& ! saveonefilepergroup*/ ), "*." InfixSubject "*." );
    StringAppend    ( buff,          FILEEXT_SEG );

    verbose.Put ( "Segmentation files, per subject :", buff );
    }


if ( writestatdurationsfiles ) {
    StringCopy      ( buff,          BaseFileNameStatDurations );
    StringAppend    ( buff,          InfixGroup, "*.Average.HistoDuration.Lin." FILEEXT_EEGSEF );

    verbose.NextLine ();
    verbose.Put ( "Segments duration, average linear:", buff );

    StringReplace   ( buff,         ".Lin.",   ".Log." );
    verbose.Put ( "Segments duration, average log   :", buff );


    StringCopy      ( buff,          BaseFileNameStatDurations );
    StringAppend    ( buff,          InfixGroup, "*." InfixSubject "*.HistoDuration.Lin." FILEEXT_EEGSEF );

    verbose.NextLine ();
    verbose.Put ( "Segments duration, per subject linear:", buff );

    StringReplace   ( buff,         ".Lin.",   ".Log." );
    verbose.Put ( "Segments duration, per subject log   :", buff );
    }


if ( writesegfrequencyfiles ) {
    StringCopy ( buff, (char*) BaseFileNameSegFrequency, InfixGroup "*." InfixSubject "*.SegFreq.", FILEEXT_EEGSEF );

    verbose.NextLine ();
    verbose.Put ( "Segments Frequency:", buff );
    }


if ( writecorrelationfiles ) {
//  StringCopy ( buff, (char*) BaseFileNameCorrelation, InfixGroup "*." InfixSubject "*.<filename>.Corr.",   FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );
    StringCopy ( buff, (char*) BaseFileNameCorrelation, InfixGroup "*." InfixSubject "*.Corr.",    FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );

    verbose.NextLine ();
    verbose.Put ( "Time Course of Correlations data/maps:", buff );
    }


if ( markovsavefreq ) {
    StringCopy      ( buff,     BaseFileNameMarkov, InfixGroup "*." );
    StringAppend    ( buff,     InfixJointStateProb " " InfixObserved ".", InfixProb"." );
    StringAppend    ( buff,     FILEEXT_FREQ );

    verbose.NextLine ();
    verbose.Put ( "Joint States Prob, Observed, Prob:", buff );
    }


if ( markovsavecsv ) {
    StringCopy      ( FilePrefix,               BaseFileNameMarkov );
    StringAppend    ( FilePrefix,               InfixGroup "*" );

    StringCopy      ( FileSuffix,               "." InfixStepAhead, "*" );
    StringAppend    ( FileSuffix,               "." InfixCartoolWorksheet );
    AddExtension    ( FileSuffix,               FILEEXT_CSV );


    verbose.NextLine ();

    StringCopy          ( buff,     FilePrefix,     "." InfixJointStateProb " " InfixObserved, "." InfixCount,  FileSuffix );
    verbose.Put ( "Joint States Prob, Observed, Counts:", buff );

    StringCopy          ( buff,     FilePrefix,     "." InfixJointStateProb " " InfixExpected, "." InfixCount,  FileSuffix );
    verbose.Put ( "Joint States Prob, Expected, Counts:", buff );                         
                                                                                          
    StringCopy          ( buff,     FilePrefix,     "." InfixJointStateProb " " InfixObserved, "." InfixProb,   FileSuffix );
    verbose.Put ( "Joint States Prob, Observed, Prob:", buff );                           
                                                                                          
    StringCopy          ( buff,     FilePrefix,     "." InfixJointStateProb " " InfixExpected, "." InfixProb,   FileSuffix );
    verbose.Put ( "Joint States Prob, Expected, Prob:", buff );

    StringCopy          ( buff,     FilePrefix,     "." InfixMarkovTransMatrix " " InfixObserved,               FileSuffix );
    verbose.Put ( "Markov Transition Matrix, Observed:", buff );

    StringCopy          ( buff,     FilePrefix,     "." InfixMarkovTransMatrix " " InfixExpected,               FileSuffix );
    verbose.Put ( "Markov Transition Matrix, Expected:", buff );
    }


StringCopy ( buff, BaseFileNameDataCluster, "*.Cluster*", MainExt );
if ( writeclustersfiles )
    verbose.Put ( "Data clusters files:", buff );
}


verbose.NextTopic ( "Processing Summary:" );
{
(ofstream&) verbose << "Data are processed following the sequence:"                 NewLine NewLine;
(ofstream&) verbose << "    Repeating for each epoch, or the whole file if none:"   NewLine NewLine;
(ofstream&) verbose << "    * Labeling                      (always)"               NewLine;
(ofstream&) verbose << "    * Removing low correlations     (optional)"             NewLine;
(ofstream&) verbose << "    * Temporal smoothing            (optional)"             NewLine;
(ofstream&) verbose <<                                                              NewLine;
(ofstream&) verbose << "    Once on all time frames:"                               NewLine NewLine;
(ofstream&) verbose << "    * Rejection of small segments   (optional)"             NewLine;

verbose.NextLine ();
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

verbose.NextBlock ();


verbose.NextTopic ( "Correlations  Centroids x Centroids:" );
{
(ofstream&) verbose << "(multiplied by 100; -100 = opposite, 100 = identical)" NewLine NewLine;

(ofstream&) verbose << StreamFormatFixed;


verbose.ResetTable ();

verbose.TableColNames.Add ( "Map    " );
for ( int nc = 1; nc <= nclusters; nc++ ) {
    IntegerToString           ( buff, nc );
    verbose.TableColNames.Add ( buff );
    verbose.TableRowNames.Add ( buff );
    }


verbose.BeginTable ( 8 /*5*/ );


for ( int nc1 = 0; nc1 < nclusters; nc1++ ) 
for ( int nc2 = 0; nc2 < nclusters; nc2++ )

    verbose.PutTable ( Round ( 100 * Project ( templatemaps[ nc1 ], templatemaps[ nc2 ], polarity ) ) );


verbose.EndTable ();

verbose.NextLine ();
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data needed to properly save the big .seg file: all subjects & across all epochs
//TArray4<float>    bigsegdata;         // Single array, with its longer part adjusted to the longest subject, which can be quite big
TArray3<float>*     bigsegdata      = 0;// Array of array, each subject has its own array hence size

                                        // All things related to time per subject
enum                SubjTimeFramesEnum
                    {
                    FileDuration,               // total file size, in TF
                    EpochsDuration,             // duration within optional epochs, <= file size
                    EpochsDurationLabeled,      // the one above, but for the labeled   parts only
                    EpochsDurationUnlabeled,    //   "      "      "    "  "  unlabeled  "     "
                    NumDurationVars,
                    };
                                        // Conditions x Subjects x TimeVariables
TArray3<int>        SubjTimeFrames;

                                        // GEV per subject
TArray2<double>     SubjGEV;


TArray4<float>      alldurationsraw;
TArray4<float>      alldurationssmooth;
//double            alldurationsraw_IndexMin;
double              alldurationsraw_IndexRatio;
double              alldurationssmooth_IndexMin;
double              alldurationssmooth_IndexRatio;
TGoEasyStats        durstat ( writestatdurationsfiles ? reqmaxclusters : 0, writestatdurationsfiles ? 100 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through groups of GoF - groups of GoF should be complete
                                        // these groups of GoF are joined to compute common GEV and GFP (n conditions within subject)

for ( int gofi1 = 0, gofi2 = gofi1 + numwithinsubjects - 1; gofi1 < numgroups && gofi2 < numgroups; gofi1 += numwithinsubjects, gofi2 += numwithinsubjects ) {

                                        // set groups selection
    groupsel.Reset ();
    groupsel.Set   ( gofi1, gofi2 );

                                        // needed for max allocations
    int         maxfilespergroup    = gogof.GetMaxFilesPerGroup ( gofi1, gofi2 );

                                        // number of TF for the current group
    int         groupnumtf          = gogof.GetMaxNumTF ( gofi1, gofi2 );

                                        // update TMicroStates(?)
    MaxNumTF    = groupnumtf;

                                        // used for file output
    if ( numwithinsubjects > 1 )    StringCopy  ( GroupString,  InfixGroup "s", IntegerToString ( gofi1 + 1 ), "-", IntegerToString ( gofi2 + 1 ) );
    else                            StringCopy  ( GroupString,  InfixGroup,     IntegerToString ( gofi1 + 1 ) );

                                        // extracting subjects' names from file
    gogof.SimplifyFilenames ( gofi1, gofi2, SubjectName );

                                        // update the verbose with current epoch
    verbose.NextBlock ();
    verbose.NextTopic ( "Processing Group(s):" );
    {
                                        // output all file names, too?
    TFileName           buffgroup;

    for ( int gofi = gofi1; gofi <= gofi2; gofi++ ) {

        StringCopy ( buffgroup,  "Group ", IntegerToString ( gofi + 1 ), ":" );
        StringCopy ( buff, IntegerToString ( gogof[ gofi ].NumFiles () ), " file", StringPlural ( gogof[ gofi ].NumFiles () ) );

        verbose.Put ( buffgroup, buff );
        }

    verbose.Flush ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate a big buffer to hold all subjects segments
    if ( writesegfiles || writesegfrequencyfiles ) {

        bigsegdata      = new TArray3<float> [ maxfilespergroup ];

                                        // allocate the biggest size, although it could use the strict amount of TFs
        for ( int fi = 0; fi < maxfilespergroup; fi++ ) {

            bigsegdata[ fi ].Resize      ( numwithinsubjects, NumSegVariables, groupnumtf );
                                        // !Resetting memory is important, as out of boundaries and/or unlabeled parts will be set to 0!
            bigsegdata[ fi ].ResetMemory ();
            }

                                        // TArray4 version is simpler, but there are memory allocation problems...
                                        // put the highest dimension in the last index, for memory access
//      bigsegdata.Resize ( maxfilespergroup, numwithinsubjects, NumSegVariables, groupnumtf );
//                                      // !Resetting memory is important, as out of boundaries and/or unlabeled parts will be set to 0!
//      bigsegdata.ResetMemory ();
        } // if writesegfiles || writesegfrequencyfiles


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need an array of files, because we process  numwithinsubjects  groups at once
                                        // !not tested with epochs (if that means anything)!
    if ( markovsavefreq ) {

        expmarkovfreq   = new TExportTracks[ numwithinsubjects ];


        for ( int relg = 0, absg = gofi1 + relg; relg < numwithinsubjects; relg++, absg++ ) {

            StringCopy      ( expmarkovfreq[ relg ].Filename,   BaseFileNameMarkov );
            StringAppend    ( expmarkovfreq[ relg ].Filename,   InfixGroup, IntegerToString ( absg + 1 ) );
            StringAppend    ( expmarkovfreq[ relg ].Filename,   "." InfixJointStateProb " " InfixObserved, "." InfixProb );
            AddExtension    ( expmarkovfreq[ relg ].Filename,   FILEEXT_FREQ );

                                        // because UndefinedLabel
            int             numtracks       = nclusters + 1;

            expmarkovfreq[ relg ].SetAtomType ( AtomTypeScalar );
            expmarkovfreq[ relg ].NumTracks           = Square ( (double) numtracks /*nclusters*/ );
            expmarkovfreq[ relg ].NumFrequencies      = markovtransmax;
            expmarkovfreq[ relg ].NumTime             = gogof[ gofi1 ].NumFiles ();
            expmarkovfreq[ relg ].SamplingFrequency   = 0;
            expmarkovfreq[ relg ].BlockFrequency      = 0;

            StringCopy ( expmarkovfreq[ relg ].FreqTypeName, FrequencyAnalysisNames[ FreqFFTNorm2 ] );
          //StringCopy ( expmarkovfreq[ relg ].Type, FILEEXT_FREQ );

            expmarkovfreq[ relg ].ElectrodesNames.Set ( expmarkovfreq[ relg ].NumTracks, ElectrodeNameSize );
            expmarkovfreq[ relg ].FrequencyNames .Set ( expmarkovfreq[ relg ].NumFrequencies, sizeof ( TFreqFrequencyName ) );


            for ( int fi = 0; fi < markovtransmax; fi++ )
                StringCopy ( expmarkovfreq[ relg ].FrequencyNames[ fi ], /*"Transition "*/ IntegerToString ( fi + 1 ) );


            for ( int frommap = 0, mmi = 0; frommap < numtracks; frommap++        )
            for ( int destmap = 0;          destmap < numtracks; destmap++, mmi++ ) {
                                        // we have to stick to very short names here
                char*       ton     = expmarkovfreq[ relg ].ElectrodesNames[ mmi ];

                if ( frommap < nclusters )  StringCopy  ( ton, "M", IntegerToString ( frommap + 1 ) );
                else                        StringCopy  ( ton, "M"  "U" );

                                            StringAppend( ton, "To" );

                if ( destmap < nclusters )  StringAppend( ton, "M", IntegerToString ( destmap + 1 ) );
                else                        StringAppend( ton, "M"  "U" );
                }
            }

        } // if markovsavefreq


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need to store some different durations for each subjects' files
                                        // One more for max across groups per subject
    SubjTimeFrames.Resize ( numwithinsubjects + 1, maxfilespergroup + 1, NumDurationVars );

                                        // Important, must be 0
    SubjTimeFrames.ResetMemory ();


    SubjGEV.Resize ( maxfilespergroup, noncompetitive ? nclusters : 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

                                        // load 1 row of files for a set of group
        Gauge.CurrentPart  = gaugefitinit;
                                        // udpate with each processed file
        Cartool.CartoolApplication->SetMainTitle    ( "Fitting of", gogof[ gofi1 ][ subji ], Gauge );

                                        // Here the data is: Spatially filtered / ESI / GFP Normalized / Z-Score'd
        ReadData        (   gogof, gofi1, gofi2, 
                            subji, 
                            datatype, ReferenceNone,
                            Cartool.IsInteractive () 
                        );

        PreprocessMaps  (   Data,
                            false,
                            datatype,       polarity,       dataref,
                            ranking,
                            processingref,
                            true,           // always normalized
                            true            // compute Norm Gfp Dis arrays
                        );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // will also Reset the structure at the same time        
        labels.Resize ( NumTimeFrames );

     // if NumFiles != numwithinsubjects, there is a problem here - the dialog should check that, though

                                        // make an assert of the following?
//      DBGV6 ( gofi1, gofi2, subji, maxfilespergroup, fl, NumFiles, "gofi1..gofi2, subji/maxfilespergroup:  max linear file index == # files?" );
//#if defined(CHECKASSERT)
//      assert ( fl == NumFiles );
//#endif


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for each subject, reset Markov count for ALL epochs/maps
                                        // what is important is that we don't interfere across rows when doing the counting
        if ( markov ) {
            JSP     .ResetMemory ();
            ProbSeg .ResetMemory ();
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fit each epoch independently

                                        // init the labeling with -1, as we are not sure that all TFs will be covered by an epoch
        labels.Reset ();
        
                                        // fit each epoch independently, while cumulating all labels
        for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {

            long                fromtf          = EpochsFrom ( epochi );
            long                totf            = EpochsTo   ( epochi );
            long                tfmin;
            long                tfmax;

                                        // set the list of template maps to be fitted
            mapsel.Reset ();
            mapsel.Set ( epochmaps[ epochi ], true, Cartool.IsInteractive () );

                                        // was used for file output - but not anymore
//          StringCopy      ( EpochString,  "Epoch ", IntegerToString ( fromtf, 3 ), "-", IntegerToString ( totf, 3 ) );
//          
//          StringCopy      ( MapsString,   "Maps " );
//          mapsel.ToText   ( StringEnd ( MapsString ), &mapnames );    // used to shorten any enumeration like "1 2 3 4" to "1-4"


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update the verbose with current epoch
//          verbose.NextBlock ();
//          verbose.NextTopic ( "Results:" );

                                        // not useful?
//          verbose.Put ( "Epoch number:",     numepochs -  epochi );
//          verbose.Put ( "From Time Frame :", EpochsFrom ( epochi ) );
//          verbose.Put ( "To   Time Frame :", EpochsTo   ( epochi ) );
//          verbose.Put ( "Maps / Templates:", epochmaps  [ epochi ] );
//          verbose.NextLine ( 2 );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do the labeling / smoothing, per group (each file here is from the same subject)
            for ( int relg = 0; relg < NumFiles; relg++ ) {


                TimeRangeToDataRange ( relg, fromtf, totf, tfmin, tfmax );

                                        // Here: we are within 1 subject (subji) / 1 condition (relg) / 1 epoch [fromtf..totf] -> absolute data range is [tfmin..tfmax]

//              DBGV3 ( gofi1, gofi2, subji, "gofi1 gofi2 subji" );
//              DBGV6 ( fromtf, totf, tfmin, tfmax, MaxNumTF, bigsegdata.GetDim4 (), "fromtf, totf, tfmin, tfmax, MaxNumTF, bigsegdata.GetDim4 ()" );

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First do the labeling / fitting:
                                        //   - only on the current file
                                        //   - inside the time window of the epoch
                                        //   - with the template maps selected
                                        //   - then equivalent to  RejectLowCorrelation

                Gauge.Next ( gaugefitlabeling, SuperGaugeUpdateTitle );

                templatemaps.CentroidsToLabeling (  
                                            Data,
                                            tfmin,          tfmax, 
                                            reqmaxclusters,
                                            &mapsel,        labels,
                                            polarity /*PolarityDirect*/, limitcorr 
                                            );


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if ( smoothing )
                                    // It seems better to not clip the correlation here
                    SmoothingLabeling       (     
                                            tfmin,              tfmax, 
                                            reqmaxclusters, 
                                            templatemaps,       &mapsel, 
                                            labels,
                                            smoothinghalfsize,  smoothinglambda,
                                            polarity,           IgnoreCorrelationThreshold  // unlabeled data points can be turned into labeled IF unlabeled data is not majority
//                                          polarity,           limitcorr                   // unlabeled data points will remain unlabeled if below correlation threshold
                                            );


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                Gauge.Next ( gaugefitrejectsmall, SuperGaugeUpdateTitle );

                if ( rejectsmall ) {
                        
                    RejectSmallSegments     (   
                                            tfmin,          tfmax, 
                                            templatemaps,   labels,
                                            polarity,       IgnoreCorrelationThreshold,
//                                          polarity,       limitcorr,
                                            rejectsize 
                                            );

//                                  // re-do it in case the limitcorr has made some crumbles again (silly it)
//                  if ( dolimitcorr && limitcorr > MinCorrelationThreshold )
//
//                      RejectSmallSegments (   
//                                          tfmin,          tfmax,
//                                          templatemaps,   labels,
//                                          polarity,       IgnoreCorrelationThreshold, // always on second run
//                                          rejectsize 
//                                          );

                    } // if rejectsmall


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if ( badepochs != NoSkippingBadEpochs ) {

                    RejectBadEpochs     (     
                                        relg,       
                                        fromtf,     totf,
                                        gogof[ gofi1 + relg ][ subji ],
                                        badepochs,  listbadepochs,  BadEpochsToleranceDefault,
                                        labels 
                                        );  
                    }

                } // Labeling

            } // for epochs


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // labels variable has been updated with all requested epochs, and all files within subject
                                        // now we can globally extract informations from the labeling
                                        // which allows f.ex. to fit a given map in many epochs, there will be no forced reset of variables

                                        // set to biggest including interval
        long                allepochsfromtf     = Highest ( allepochsfromtf );
        long                allepochstotf       = Lowest  ( allepochstotf   );

        mapsel.Reset ();

                                        // find biggest encompassing time interval, total time frames within all epochs, and cumulate all labels across all epochs
        for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {

            Mined ( allepochsfromtf, EpochsFrom ( epochi ) );
            Maxed ( allepochstotf,   EpochsTo   ( epochi ) );
                                        // add up all template maps
            mapsel.Set ( epochmaps[ epochi ], true, Cartool.IsInteractive () );
            }

                                        // use biggest interval
        long                fromtf      = allepochsfromtf;
        long                totf        = allepochstotf;
        long                tfmin;
        long                tfmax;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have our final labeling
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Files duration
        for ( int relg = 0; relg < NumFiles; relg++ ) {

            SubjTimeFrames ( relg, subji, FileDuration )    =                   NumTF[ relg ];                                                              
                                        // max size within subject - used for joint output
            Maxed ( SubjTimeFrames ( numwithinsubjects, subji, FileDuration ),  NumTF[ relg ] );
            }
                                        // max size across subjects - for consistency (should be the same as MaxNumTF)
        Maxed ( SubjTimeFrames ( numwithinsubjects, maxfilespergroup, FileDuration ), SubjTimeFrames ( numwithinsubjects, subji, FileDuration ) );

                                        // Compute # of TF per epoch, then split into labeled and unlabeled
        for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {

            for ( int relg = 0; relg < NumFiles; relg++ ) {

                TimeRangeToDataRange ( relg, EpochsFrom ( epochi ), EpochsTo ( epochi ), tfmin, tfmax );

                                        // cumulate current subject epochs for all conditions, with ACTUAL limits
                SubjTimeFrames ( relg, subji, EpochsDuration ) += tfmax - tfmin + 1;

                                        // now split #TF within epochs into labeled and unlabeled
                for ( long tf = tfmin; tf <= tfmax; tf++ )

                    if ( labels.IsUndefined ( tf ) )    SubjTimeFrames ( relg, subji, EpochsDurationUnlabeled )++;
                    else                                SubjTimeFrames ( relg, subji, EpochsDurationLabeled   )++;

                }
            }

                                        // cumulate within subject
        for ( int relg = 0; relg < NumFiles; relg++ ) {
            SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration          )   += SubjTimeFrames ( relg, subji, EpochsDuration          );
            SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationUnlabeled )   += SubjTimeFrames ( relg, subji, EpochsDurationUnlabeled );
            SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationLabeled   )   += SubjTimeFrames ( relg, subji, EpochsDurationLabeled   );
            }

                                        // cumulate across subjects
        SubjTimeFrames ( numwithinsubjects, maxfilespergroup, EpochsDuration           )   += SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration          );
        SubjTimeFrames ( numwithinsubjects, maxfilespergroup, EpochsDurationUnlabeled  )   += SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationUnlabeled );
        SubjTimeFrames ( numwithinsubjects, maxfilespergroup, EpochsDurationLabeled    )   += SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationLabeled   );


//      if ( subji < 3 )
//      DBGV5 ( subji + 1,
//              SubjTimeFrames ( numwithinsubjects, subji, FileDuration             ), 
//              SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration           ), 
//              SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationUnlabeled  ), 
//              SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationLabeled    ), 
//              "#Subject: #TF #TFinEpochs -> #TFUnlabeled + #TFLabeled" );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // recompute a global GEV across all conditions of the same subject - actually gev is not used here
//      if ( numwithinsubjects > 1 )
//
//          gev     = ComputeGEV ( templatemaps, labels, 0, NumTimeFrames - 1 );


                                        // cumulate the denominator part of the GEV, for ALL epochs time AND across ALL files (of the same subject)
                                        // !using only the labeled part (within list of epochs, excluding bad epochs, above correlation)!
        double          gevdenom        = 0;

                                        // normalize across the whole time range
        for ( long tf = 0; tf < NumTimeFrames; tf++ )

            if ( labels.IsDefined ( tf ) )
                                        // include in  SubjTimeFrames  variable?
                gevdenom   += Square ( Norm[ tf ] );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For array access reasons, we need to temporarily convert label values so to be all >= 0.
                                        // Label values:        defined label values in [0 .. nclusters) / UndefinedLabel         = -1
                                        // Temp label values:   defined label values in [0 .. nclusters) / UndefinedLabelPositive = nclusters

        #define     UndefinedLabelPositive      nclusters

                                        // just convert UndefinedLabel to UndefinedLabelPositive
        auto        SegToIndex  = [ &UndefinedLabelPositive ] ( const LabelType& L )    { return L == UndefinedLabel ? UndefinedLabelPositive : L; };

                                        // convert back UndefinedLabelPositive to UndefinedLabel + test for map selection
        auto        IsSegOk     = [ &nclusters, &mapsel ]     ( const LabelType& L )    { return L == UndefinedLabelPositive 
                                                                                              || ( IsInsideLimits ( L, 0, nclusters - 1 ) && mapsel[ L ] ); };


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset all variables with -1 (0 can be a legal value!)
        var                 = missingvalue;

                                        // now, extract all variables from global labeling
                                        // !TODO replace all f by relg!
        for ( int relg = 0, f = relg, absg = gofi1 + relg; relg < NumFiles; relg++, f++, absg++ ) {

            Gauge.Next ( gaugefitextractvariables, SuperGaugeUpdateTitle );


            TimeRangeToDataRange ( relg, fromtf, totf, tfmin, tfmax );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert the temporal segmentation into a list of markers (for the current subject and condition/file)
            TMarkers        seglist;

                                        // In case of noncompetitive, there is nothing but a single, "big" segment
            if ( competitive ) {

                long            begintf         = tfmin;

                                        // Transform the labeling into discrete states
                for ( long tf = tfmin; tf <= tfmax; tf++ ) {

                    if ( ! ( tf == tfmax
                          || labels[ tf + 1 ] != labels[ tf ] ) )
                            continue;
                                        // here we have a new segment, which can also be UndefinedLabel

                                        // segments index starts from 0, n is for the UndefinedLabel
                    seglist.AppendMarker ( TMarker ( begintf, tf, 
                                                     (MarkerCode) SegToIndex ( labels[ begintf ] ), 
                                                     labels.IsUndefined ( begintf ) ? "Unlabeled" : IntegerToString ( buff, labels[ begintf ] + 1 ),
                                                     MarkerTypeTemp ) );

                                        // next segment will begin after current TF
                    begintf     = tf + 1;
                    } // for tf

                                        // save segments as markers
//              TFileName           FileSegToMarkers;
//              StringCopy ( FileSegToMarkers, BaseFileName, "Seg To Markers.S", IntegerToString ( buff, subji + 1 ), ".C", IntegerToString ( buff, absg + 1 ), "." FILEEXT_MRK );
//              seglist.WriteFile ( FileSegToMarkers, AllMarkerTypes );
                } // if competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Onset TF, Offset TF, Number of TFs, Time Coverage

                                        // it's valid for numtf to be 0
            for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                var ( f, fitnumtf,          si() ) = 0;
                var ( f, fittimecoverage,   si() ) = 0;
                }


            if ( noncompetitive ) {
                                        // Doesn't make any sense
                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                    var ( f, fitfonset,         si() ) = fromtf;
                    var ( f, fitloffset,        si() ) = totf;
//                  var ( f, fitnumtf,          si() ) = SubjTimeFrames ( f, subji, EpochsDuration );          // fixed value
                    var ( f, fitnumtf,          si() ) = SubjTimeFrames ( f, subji, EpochsDurationLabeled );   // variable value, according to bad / unlabeled time points
                    var ( f, fittimecoverage,   si() ) = 100;
                    }
                } // if noncompetitive

            else { // competitive

                for ( long tf = tfmin, tf0 = tfmin - OffsetTF[ f ]; tf <= tfmax; tf++, tf0++ ) {

                    if ( labels.IsUndefined ( tf ) )
                        continue;

                    LabelType   l       = labels[ tf ];

                                        // first onset: if variable not already set
                    if ( var ( f, fitfonset, l ) == missingvalue )
                        var ( f, fitfonset, l ) = tf0;
                                        // last offset: last update value
                    var ( f, fitloffset,    l ) = tf0;
                                        // number of TF
                    var ( f, fitnumtf,      l )++;
                    }

                                        // compute ratio of # TFs / duration
                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    )

                    var ( f, fittimecoverage,   si() )  = var ( f, fitnumtf, si() )
//                                                      / NonNull ( (double) SubjTimeFrames ( f, subji, EpochsDuration        ) )   // ratio proportional to the epochs duration - could be biased with the unlabeled parts
                                                        / NonNull ( (double) SubjTimeFrames ( f, subji, EpochsDurationLabeled ) )   // ratio proportional to the ACTUAL labeled time points
                                                        * 100;  // make it a percentage
                } // competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Best Correlation, TF of Best Correlation, GFP of TF of Best Correlation

            for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    )
                var ( f, fitbcorr,      si() ) = Lowest<double> ();


            if ( noncompetitive ) {

                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

                    for ( long tf = tfmin, tf0 = tfmin - OffsetTF[ f ]; tf <= tfmax; tf++, tf0++ ) {

                        if ( labels.IsUndefined ( tf ) )
                            continue;

                        double      corr    = Project ( templatemaps[ si() ], Data[ tf ], polarity );  // pick the right polarity for each map

                        if ( corr > var ( f, fitbcorr, si() ) ) {
                            var ( f, fitbcorr,      si() ) = corr;
                            var ( f, fittfbcorr,    si() ) = tf0;
                            var ( f, fitgfptfbcorr, si() ) = Gfp[ tf ];
                            }
                        } // for tf
                    } // for s
                } // if noncompetitive

            else {                      // competitive - best fit on all segments

                for ( long tf = tfmin, tf0 = tfmin - OffsetTF[ f ]; tf <= tfmax; tf++, tf0++ ) {

                    if ( labels.IsUndefined ( tf ) )
                        continue;

                    LabelType   l       = labels[ tf ];
                                        // although we know the polarity for Centroid, we still have to evaluate for Cloud
                    double      corr    = Project ( templatemaps[ l ], Data[ tf ], polarity /*labels.GetPolarity ( tf )*/ );

                    if ( corr > var ( f, fitbcorr, l ) ) {
                        var ( f, fitbcorr,      l ) = corr;
                        var ( f, fittfbcorr,    l ) = tf0;
                        var ( f, fitgfptfbcorr, l ) = Gfp[ tf ];
                        }
                    } // for tf
                } // competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // GEV, Mean Correlation

                                        // it's valid for GEV to be 0
            for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                var ( f, fitmeancorr,   si() ) = 0;
                var ( f, fitgev,        si() ) = 0;
                }
                                        // simplified formula that can be applied to either the competitive or non-competitive cases
                                        // also, does not set to -1 if not found, let it be 0
            if ( noncompetitive ) {

                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

                    for ( long tf = tfmin; tf <= tfmax; tf++ ) {

                        if ( labels.IsUndefined ( tf ) )
                            continue;

                        double      corr    = Project ( templatemaps[ si() ], Data[ tf ], polarity );  // pick the right polarity for each map
                        double      gevc    = Square ( Norm[ tf ] * corr );

                        var ( f, fitmeancorr, si() )  += corr;
                        var ( f, fitgev,      si() )  += gevc;
                        } // for tf

                                        // normalize
                    var ( f, fitmeancorr,   si() )    /= NonNull ( var ( f, fitnumtf,   si() ) );
                    var ( f, fitgev,        si() )    /= NonNull ( gevdenom );    // gevdenom summed across all conditions, within labeled parts
                    } // for s
                } // if noncompetitive

            else {                      // competitive - GEV on all segments

                for ( long tf = tfmin; tf <= tfmax; tf++ ) {

                    if ( labels.IsUndefined ( tf ) )
                        continue;

                    LabelType   l       = labels[ tf ];

                    double      corr    = Project ( templatemaps[ l ], Data[ tf ], polarity /*labels.GetPolarity ( tf )*/ );
                    double      gevc    = Square ( Norm[ tf ] * corr );

                    var ( f, fitmeancorr,   l )    += corr;
                    var ( f, fitgev,        l )    += gevc;
                    } // for tf

                                        // normalize
                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                    var ( f, fitmeancorr,   si() )    /= NonNull ( var ( f, fitnumtf,   si() ) );
                    var ( f, fitgev,        si() )    /= NonNull ( gevdenom );     // gevdenom summed across all conditions, within labeled parts
                    }

                } // competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mean and Max Gfp, TF of Centroid

                                        // not valid to be 0 if missing, but for sums, set to 0 here
            for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                var ( f, fitmeangfp,    si() ) = 0;
                var ( f, fitmaxgfp,     si() ) = 0;
                var ( f, fittfcentroid, si() ) = 0;
                }


            if ( noncompetitive ) {     // actually nonsense

                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

                    for ( long tf = tfmin, tf0 = tfmin - OffsetTF[ f ]; tf <= tfmax; tf++, tf0++ ) {

                        if ( labels.IsUndefined ( tf ) )
                            continue;


                        double      gfp     = Gfp[ tf ];

                        if ( gfp > var ( f, fitmaxgfp, si() ) ) {
                            var ( f, fitmaxgfp,    si() )  = gfp;
                            var ( f, fittfmaxgfp,  si() )  = tf0;
                            }

                        var ( f, fitmeangfp,    si() )    += gfp;
                        var ( f, fittfcentroid, si() )    += gfp * tf0;
                        }

                    var ( f, fittfcentroid, si() )        /= NonNull ( var ( f, fitmeangfp, si() ) );
                    var ( f, fitmeangfp,    si() )        /= NonNull ( var ( f, fitnumtf,   si() ) );
                    } // for s
                } // if noncompetitive

            else { // competitive

                for ( long tf = tfmin, tf0 = tfmin - OffsetTF[ f ]; tf <= tfmax; tf++, tf0++ ) {

                    if ( labels.IsUndefined ( tf ) )
                        continue;

                    LabelType   l       = labels[ tf ];
                    double      gfp     = Gfp[ tf ];

                    if ( gfp > var ( f, fitmaxgfp, l ) ) {
                        var ( f, fitmaxgfp,    l )  = gfp;
                        var ( f, fittfmaxgfp,  l )  = tf0;
                        }

                    var ( f, fitmeangfp,    l )    += gfp;
                    var ( f, fittfcentroid, l )    += gfp * tf0;
                    }

                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                    var ( f, fittfcentroid, si() )    /= NonNull ( var ( f, fitmeangfp, si() ) );
                    var ( f, fitmeangfp,    si() )    /= NonNull ( var ( f, fitnumtf,   si() ) );
                    }
                } // competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute each segment's duration

            Gauge.Next ( gaugefitsegments, SuperGaugeUpdateTitle );


            if ( noncompetitive ) {
                                        // doesn't make any sense
                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {
                                        // total duration of labeled TFs, in milliseconds
//                  var ( f, fitmeanduration,   si() )     = TimeFrameToMilliseconds ( SubjTimeFrames ( f, subji, EpochsDuration        ), samplingfrequency );
                    var ( f, fitmeanduration,   si() )     = TimeFrameToMilliseconds ( SubjTimeFrames ( f, subji, EpochsDurationLabeled ), samplingfrequency );
                                        // inverse of duration, in 1/second
//                  var ( f, fitsegdensity,     si() )     = 1 / NonNull ( TimeFrameToSeconds ( (double) SubjTimeFrames ( f, subji, EpochsDuration        ), samplingfrequency ) );
                    var ( f, fitsegdensity,     si() )     = 1 / NonNull ( TimeFrameToSeconds ( (double) SubjTimeFrames ( f, subji, EpochsDurationLabeled ), samplingfrequency ) );
                    }
                } // if noncompetitive

            else { // competitive

                enum                SegmentsDurationEnum {
                                    segduration,
                                    numsegs,
                                    numsegmentsvar
                                    };

                TArray2<double>     segments ( reqmaxclusters, numsegmentsvar );
                long                duration;
                int                 mapi;

                                        // counting segments duration per map
                segments.ResetMemory ();
                                        // initialize with invalid labels - index 0 is for current state, 1 for previous one, etc...
                state       = UndefinedLabelPositive;   // UndefinedLabel;
                                        // reset only current template maps
                if ( writestatdurationsfiles )
                    for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    )
                        durstat[ si() ].Reset ();

                                        // browse each segment - labeled and unlabeled altogether
                for ( int segi = 0; segi < (int) seglist; segi++ ) {


                    duration    = seglist[ segi ]->Length ();
                    mapi        = seglist[ segi ]->Code;        // !positive only values, UndefinedLabelPositive!

                                        // we allow to let in an UndefinedLabel segment in the list of previous segments, which will count as a single null segment
                    if ( markov ) {
                                        // 1) count each segment, including UndefinedLabel, for each condition
                                        //    this count is absolute, and does not care for mapsel
                        ProbSeg ( f, mapi )++;

                                        // 2) shift and insert new state (0 = current state, markovtransmax = previous nth segment)
                        for ( int trans = markovtransmax; trans >= 1; trans-- )

                            state[ trans ]  = state[ trans - 1 ];
                                        // then insert new segment
                        state[ 0 ]  = mapi;

                                        // 3) count transitions
                                        //    we now allow to transition to/from UndefinedLabel states
                                        //    we don't allow anymore transitioning to (and neither from) states NOT in mapsel

                                        // count all transitions from previous segments (state[ i >= 1 ]) to new one (state[ 0 ])
                                        // !"before" segments should exist, real or UndefinedLabel!
                        if ( IsSegOk ( state[ 0 ] ) )

                            for ( int trans = 1; trans <= min ( segi, markovtransmax ); trans++ )

                                if ( IsSegOk ( state[ trans ] ) )

                                    JSP ( f, trans, state[ trans ], state[ 0 ] )++;

                        } // if markov


                                        // right now, no stats for UndefinedLabel
                    if ( IsSegOk ( mapi ) && mapi != UndefinedLabelPositive /*UndefinedLabel*/ ) {
                                        // store / stats duration
//                      segments ( mapi, segduration )  += duration;                    // arithmetic mean
                        segments ( mapi, segduration )  += log ( (double) duration );   // geometric mean (duration is non-null here)
                        segments ( mapi, numsegs     )  ++;


                        if ( writestatdurationsfiles )
                                        // !rounding + results in [s]!
                            durstat[ mapi ].Add ( ( TimeFrameToMilliseconds ( duration, samplingfrequency ) + 0.5 ) / 1000 );
                        }

                    } // for tfi

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill variables
                for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

                    double          nseg            = segments ( si(), numsegs );

                                        // average segments duration
//                  double          meandur         = nseg ?       segments ( si(), segduration ) / nseg ) : 0;    // arithmetic mean
                    double          meandur         = nseg ? exp ( segments ( si(), segduration ) / nseg ) : 0;    // geometric mean
                                        // mean duration in milliseconds
                    var ( f, fitmeanduration,   si() ) = TimeFrameToMilliseconds ( meandur, samplingfrequency );

                                        // average of number of segments in time - it's a density, NOT a frequency which would be #Cycles per seconds
//                  double          dur             = SubjTimeFrames ( f, subji, EpochsDuration        );       // comparing to whole epochs duration
                    double          dur             = SubjTimeFrames ( f, subji, EpochsDurationLabeled );       // comparing to labeled parts only
                                        // #Segments per second
                    var ( f, fitsegdensity,     si() ) = dur ? nseg / TimeFrameToSeconds ( dur, samplingfrequency ) : 0;
                    }


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if ( writestatdurationsfiles ) {

                    THistogram              curve_raw;
                    THistogram              curve_smooth;

                                        // compute histogram of current subject
                    for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    ) {

                                                     // Kernel size to 1 TF                             max 5 [s] for all   no smoothing
                        curve_raw   .ComputeHistogram   (   durstat[ si() ], 
                                                            TimeFrameToSeconds ( 1, samplingfrequency ),    0,  5,
                                                            0,      1,
                                                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormArea | HistogramLinear )
                                                        );
                                                                                                                 // make it much bigger as it is log'ed later
                        curve_smooth.ComputeHistogram   (   durstat[ si() ],
                                                            TimeFrameToSeconds ( 1, samplingfrequency ),    0,  5,
                                                            0,    100,
                                                            (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormArea | HistogramLinear )
                                                        );

                                        // Optimal Kernel, but doesn't allow comparisons...
//                      durstat[ si() ].ComputeHistogram ( 0, log ( 1 ), log ( 1000 ), 0, 3, false );
//                      durstat[ si() ].ComputeHistogram ( 0, log ( 1 ), log ( 1000 ), 0, 3, true  );
                                        // Fixed Kernel - adhoc step made from some computed histogram
//                      durstat[ si() ].ComputeHistogram ( 0.075, log ( 1 ), log ( 1000 ), 0, 3, false );
//                      durstat[ si() ].ComputeHistogram ( 0.075, log ( 1 ), log ( 1000 ), 0, 3, true  );

                                        // big array not allocated? we know the curve sizes here
                                        // this will also reset to 0 only once, so we can cumulate all epochs/maps here
                        if ( alldurationsraw.IsNotAllocated () ) {
                            alldurationsraw   .Resize ( numwithinsubjects, maxfilespergroup + 1, reqmaxclusters, (int) curve_raw    );
                            alldurationssmooth.Resize ( numwithinsubjects, maxfilespergroup + 1, reqmaxclusters, (int) curve_smooth );
                                        // store conversions parameters for later save
//                          alldurationsraw_IndexMin        = curve_raw   .Index1.IndexMin;
                            alldurationsraw_IndexRatio      = curve_raw   .Index1.IndexRatio;
                            alldurationssmooth_IndexMin     = curve_smooth.Index1.IndexMin;
                            alldurationssmooth_IndexRatio   = curve_smooth.Index1.IndexRatio;
                            }

                                        // save current subject & cumulate
                        for ( int i = 0; i < (int) curve_raw; i++ ) {
                            alldurationsraw ( relg, subji,            si(), i )    = curve_raw ( i );
                            alldurationsraw ( relg, maxfilespergroup, si(), i )   += curve_raw ( i );
                            }

                        for ( int i = 0; i < (int) curve_smooth; i++ ) {
                            alldurationssmooth ( relg, subji,            si(), i ) = curve_smooth ( i );
                            alldurationssmooth ( relg, maxfilespergroup, si(), i )+= curve_smooth ( i );
                            }

                        } // for cluster

                                        // last subject? normalize the average histograms & store again
                    if ( subji == maxfilespergroup - 1 ) {

                        for ( int s = 0; s < reqmaxclusters; s++ ) {

                            for ( int i = 0; i < (int) curve_raw; i++ )
                                curve_raw    ( i )  = alldurationsraw    ( relg, maxfilespergroup, s, i );

                            for ( int i = 0; i < (int) curve_smooth; i++ )
                                curve_smooth ( i )  = alldurationssmooth ( relg, maxfilespergroup, s, i );

                            curve_raw   .NormalizeArea ();
                            curve_smooth.NormalizeArea ();

                            for ( int i = 0; i < (int) curve_raw; i++ )
                                alldurationsraw    ( relg, maxfilespergroup, s, i ) = curve_raw    ( i );

                            for ( int i = 0; i < (int) curve_smooth; i++ )
                                alldurationssmooth ( relg, maxfilespergroup, s, i ) = curve_smooth ( i );
                            } // for cluster

                        } // if last subject

                    } // writestatdurationsfiles

                } // competitive


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset variables to missing values, if needed
            for ( TIteratorSelectedForward si    ( mapsel );   (bool) si;    ++si    )

                if ( var ( f, fitnumtf,      si() ) == 0 ) {

                    var ( f, fitfonset,         si() ) = missingvalue;
                    var ( f, fitloffset,        si() ) = missingvalue;
                    var ( f, fittfcentroid,     si() ) = missingvalue;
                    var ( f, fitmeancorr,       si() ) = missingvalue;
                    var ( f, fitbcorr,          si() ) = missingvalue;
                    var ( f, fittfbcorr,        si() ) = missingvalue;
                    var ( f, fitgfptfbcorr,     si() ) = missingvalue;
                    var ( f, fittfmaxgfp,       si() ) = missingvalue;
                                        // these values could hold 0 as a legal value?
//                  var ( f, fitnumtf,          si() ) = missingvalue;
//                  var ( f, fitgev,            si() ) = missingvalue;
//                  var ( f, fitmeanduration,   si() ) = missingvalue;
//                  var ( f, fittimecoverage,   si() ) = missingvalue;
//                  var ( f, fitmaxgfp,         si() ) = missingvalue;
//                  var ( f, fitmeangfp,        si() ) = missingvalue;
//                  var ( f, fitsegdensity,     si() ) = missingvalue;
                    }
/*              else {                  // Convert the skewed variables with a log - or doing that in the statistics?
                    var ( f, fitnumtf,          si() ) = log ( var ( f, fitnumtf,          si() ) );
                    var ( f, fitgev,            si() ) = log ( var ( f, fitgev,            si() ) );
                    var ( f, fitgfptfbcorr,     si() ) = log ( var ( f, fitgfptfbcorr,     si() ) );
                    var ( f, fitmaxgfp,         si() ) = log ( var ( f, fitmaxgfp,         si() ) );
                    var ( f, fitmeanduration,   si() ) = log ( var ( f, fitmeanduration,   si() ) );
                    var ( f, fittimecoverage,   si() ) = log ( var ( f, fittimecoverage,   si() ) );
                    var ( f, fitsegdensity,     si() ) = log ( var ( f, fitsegdensity,     si() ) );
                    }
*/

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            } // for f/relg, variables extraction


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sum up all GEV contributions to get the per subject grand total
        if ( noncompetitive ) {

            for ( int nc = 0; nc < nclusters; nc++ ) {

                SubjGEV ( subji, nc )   = 0;

                for ( int relg = 0; relg < NumFiles; relg++ )
                                        // !each map has its own GEV!
                    SubjGEV ( subji, nc )  += var ( relg, fitgev, nc );
                }
            }
        else {
            SubjGEV ( subji, 0 )        = 0;

            for ( int relg = 0; relg < NumFiles; relg++ )
            for ( int nc = 0; nc < nclusters; nc++ )

                SubjGEV ( subji, 0 )   += var ( relg, fitgev, nc );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // need to store segmentation for later use?
        if ( writesegfiles || writesegfrequencyfiles ) {

//          Gauge.Next ( gaugefitwriteseg, SuperGaugeUpdateTitle );

                                        // Note that bigsegdata has been cleared before use, so any out of boundaries (group, file, time frame..) will be nicely set to 0
            for ( int  relg = 0; relg < NumFiles /*numwithinsubjects*/; relg++ )
            for ( long tf = 0, tf2 = OffsetTF[ relg ]; tf < SubjTimeFrames ( relg, subji, FileDuration ); tf++, tf2++ ) {

                UpdateApplication;

                                        // within range of current file & within current epoch scope
                LabelType   l       = IsInsideLimits ( tf, fromtf, totf ) ? labels[ tf2 ]   // <- could be UndefinedLabel
                                                                          : UndefinedLabel;

                bigsegdata[ subji ] ( relg, SegVarGfp, tf )   = Gfp[ tf2 ];

                if ( l == UndefinedLabel ) 
                    continue;

                bigsegdata[ subji ] ( relg, SegVarPolarity, tf )    = TrueToMinus ( polarity == PolarityEvaluate && templatemaps[ l ].IsOppositeDirection ( Data[ tf2 ] ) );
                bigsegdata[ subji ] ( relg, SegVarSegment,  tf )    = l + 1;
                bigsegdata[ subji ] ( relg, SegVarGev,      tf )    = var ( relg, fitgev, l );
                bigsegdata[ subji ] ( relg, SegVarCorr,     tf )    = Project ( templatemaps[ l ], Data[ tf2 ], polarity /*labels.GetPolarity ( tf2 )*/ );;
                } // for relg, tf

            } // if writesegfiles || writesegfrequencyfiles


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional output, all templates correlations, for each file
                                        // this is done across all TFs, so we don't need a per epoch output
        if ( writecorrelationfiles ) {

//          TFilter<TMapAtomType>  filter;
//          filter.SetButterworthLow ( "10" + "Non-causal" );


            TFileName           basefilename;
            TExportTracks       corrfile;
//          TExportTracks       corrspfile;
            bool                crosscorrcenter     = true; // even though the time course correlation is only positive (blobs present or not), the correlation of their time courses can be signed (i.e. anti-correlated)


            for ( int relg = 0, absg = gofi1 + relg; relg < NumFiles; relg++, absg++ ) {

                Gauge.Next ( gaugefitwritecorr, SuperGaugeUpdateTitle );

                                        // !"map" is 1 correlation in time, so there are nclusters of them!
                long                numtimesubj         = SubjTimeFrames ( relg, subji, FileDuration );
                TMaps               corrtime ( nclusters, numtimesubj );


                for ( int m = 0; m < nclusters; m++ )
                for ( long tf = 0, tf2 = OffsetTF[ relg ]; tf < numtimesubj; tf++, tf2++ )

                                        // !Don't use labels.GetPolarity as it is only valid for the labeled map only!
                    corrtime ( m, tf )  = mapsel.IsSelected ( m ) ? Project ( templatemaps[ m ], Data[ tf2 ], polarity )

                                        // direct use of TVector functions, with proper polarity
                                        // !No Cloud Correlation available!
                                        // !No Average Reference, data have already been centered if need be!
//                  corrtime ( m, tf )  = mapsel.IsSelected ( m ) ? Data[ tf2 ].Correlation ( templatemaps[ m ], polarity, false /*true*/ /*processingref == ReferenceAverage*/ )
                                        // IF not a selected map, output a null correlation (0 for ignore polarity, -1 for polarity)
                                                                      : 0; // ( polarity == PolarityEvaluate ? 0 : -1 );

                StringCopy      ( basefilename,                 BaseFileNameCorrelation );
                StringAppend    ( basefilename,                 InfixGroup, IntegerToString ( absg + 1 ) );
                StringAppend    ( basefilename,                 ".", SubjectName[ subji ] );


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                StringCopy      ( corrfile.Filename,          basefilename, "." "CorrDataTempl" );
                AddExtension    ( corrfile.Filename,          FILEEXT_EEGSEF );

                corrfile.SetAtomType ( AtomTypeScalar );
                corrfile.NumTracks          = nclusters;    // save all maps correlation into the same result file
                corrfile.NumTime            = numtimesubj;
//              corrfile.NumTime            = min ( allepochstotf, SubjTimeFrames ( relg, subji, FileDuration ) ) - allepochsfromtf + 1;


                corrfile.Begin ();

                for ( long tf = 0; tf < numtimesubj; tf++ )
                for ( int m   = 0; m  < nclusters;   m++  ) 
                    corrfile.Write ( corrtime ( m, tf ) );

                corrfile.End ();

                } // for gofi

            } // if writecorrelationfiles

        else {
            Gauge.FinishPart ( gaugefitwritecorr );

            Cartool.CartoolApplication->SetMainTitle    ( Gauge );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // variables output into a worksheet-like file
                                        // - we can split the output per groups and per variables
                                        // - we can output Excel and/or R formated files
        if ( (bool) varout ) {

        //  sprintf ( DataFile,  "%s.%03d_%03d.%s", BaseFileName, fromtf, totf, FILEEXT_DATA );
        //  ofstream            ofd ( DataFile );


            TSelection          groupsel2 ( groupsel );

                                        // trick here: run the loop even if writing all groups at once, just that we break the loop if the latter case. This way we avoid dupliccate code.
            int                 relg            = 0;
            for ( TIteratorSelectedForward absgi ( groupsel ); (bool) absgi; ++absgi, relg++ ) {

                                        // file name cooking
                StringCopy      ( BaseCsvFile,              BaseFileName );

                if ( saveonefilepergroup ) {
                    groupsel2.SetOnly ( absgi() );
                                        // override GroupString and replace by a single group format
                    StringAppend    ( BaseCsvFile,  InfixGroup, IntegerToString ( absgi() + 1 ) );
                    }
                else
                    StringAppend    ( BaseCsvFile,  GroupString );


                if ( saveonefilepervariable ) {
                                        // loop through all variables
                    TSelection          onevar ( varout );

                    for ( TIteratorSelectedForward vi    ( varout );   (bool) vi;    ++vi    ) {

                        onevar.SetOnly ( vi() );

                        StringCopy      ( ExcelFile,                BaseCsvFile,    ".", FitVarNames[ variablesshortnames ][ vi() ] );
                        StringCopy      ( RFile,                    BaseCsvFile,    ".", FitVarNames[ variablesshortnames ][ vi() ] );
                        StringAppend    ( ExcelFile,                "." InfixCartoolWorksheet );
                        StringAppend    ( RFile,                    "." InfixRWorksheet       );
                        AddExtension    ( ExcelFile,                FILEEXT_CSV );
                        AddExtension    ( RFile,                    FILEEXT_CSV );

                        if ( writeexcelfiles )  WriteVariablesToFile ( ExcelFile, var, groupsel2, mapsel, onevar, subji, relg, SubjectName[ subji ], true,  variablesshortnames );
                        if ( writeRfiles     )  WriteVariablesToFile ( RFile,     var, groupsel2, mapsel, onevar, subji, relg, SubjectName[ subji ], false, variablesshortnames );
                        }

                    } // if saveonefilepervariable

                else { // ! saveonefilepervariable

                    StringCopy      ( ExcelFile,                BaseCsvFile );
                    StringCopy      ( RFile,                    BaseCsvFile );
                    StringAppend    ( ExcelFile,                InfixCartoolWorksheet,  "."     FILEEXT_CSV );
                    StringAppend    ( RFile,                    InfixRWorksheet,        "."     FILEEXT_CSV );

                    if ( writeexcelfiles )  WriteVariablesToFile ( ExcelFile, var, groupsel2, mapsel, varout, subji, relg, SubjectName[ subji ], true,  variablesshortnames );
                    if ( writeRfiles     )  WriteVariablesToFile ( RFile,     var, groupsel2, mapsel, varout, subji, relg, SubjectName[ subji ], false, variablesshortnames );
                    } // ! saveonefilepervariable

                                        // all groups together?
                if ( ! saveonefilepergroup )
                    break;              // no need to loop, as we write everything at once

                } // for groupsel

            } // if varout


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Markov variables output
        if ( markovsavefreq || markovsavecsv ) {

                                        // Process probabilities from data & expected, then save to files
                                        // These arrays will hold for UndefinedLabel if JSP does
            TTracks<double>         jspc    ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Join Stat Probabilities,  Observed, original counts
            TTracks<double>         jsp     ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Join Stat Probabilities,  Observed, probabilities
            TTracks<double>         jspcx   ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Join Stat Probabilities,  Expected, original counts
            TTracks<double>         jspx    ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Join Stat Probabilities,  Expected, probabilities
            TTracks<double>         mtm     ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Markov Transition Matrix, Observed, probabilities
            TTracks<double>         mtmx    ( (int) JSP.GetDim3 (), (int) JSP.GetDim4 () ); // Markov Transition Matrix, Expected, probabilities
                    
                                        // normalize Segments probabilities, which is independent of the transitions
                                        // this is better than to deduce them from the Marginal distributions of the JSP, as we don't have all beginning segments in this table (it needs valid pairs)!
            ProbSeg.NormalizeRowsArea ();

                                        // we can be specific to which transitions to output...
            TTracks<bool>           jspmapsel ( nclusters + 1, nclusters + 1 );
            
                                        // 1 file per transition, so the variables look like GroupX_MapY_MapZ which is then read correctly by Cartool
            for ( int trans = 1; trans <= markovtransmax; trans++ ) {

                                        // set the legal transitions here
                for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
                for ( int destmap = 0; destmap < nclusters + 1; destmap++ )
                                                    // don't output Unlabeled if no Correlation Thresholding
                    jspmapsel ( frommap, destmap )  = ( ! dolimitcorr && ( frommap == nclusters || destmap == nclusters ) )
                                                    // don't output diagonals for 1 transition ahead
                                                   || ( trans == 1 && frommap == destmap )                                  ? false
                                                                                                                            : true;

                ofstream            ofjspc;
                ofstream            ofjspcx;
                ofstream            ofjsp;
                ofstream            ofjspx;
                ofstream            ofmtm;
                ofstream            ofmtmx;


                if ( markovsavecsv ) {
                                        // 1 .csv per transition
                    StringCopy          ( FilePrefix,   BaseFileNameMarkov );
                    StringAppend        ( FilePrefix,   GroupString );

                    StringCopy          ( FileSuffix,   "." InfixStepAhead, IntegerToString ( trans ) );
                    StringAppend        ( FileSuffix,   "." InfixCartoolWorksheet );
                    AddExtension        ( FileSuffix,   FILEEXT_CSV );


                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixJointStateProb " " InfixObserved, "." InfixCount,  FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofjspc,   gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );

                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixJointStateProb " " InfixExpected, "." InfixCount,  FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofjspcx,  gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );


                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixJointStateProb " " InfixObserved, "." InfixProb,   FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofjsp,    gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );

                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixJointStateProb " " InfixExpected, "." InfixProb,   FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofjspx,   gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );


                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixMarkovTransMatrix " " InfixObserved,               FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofmtm,    gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );

                    StringCopy          ( MarkovFile,   FilePrefix,     "." InfixMarkovTransMatrix " " InfixExpected,               FileSuffix );
                    WriteHeaderToFile   ( MarkovFile,   ofmtmx,   gofi1,  numwithinsubjects, nclusters, jspmapsel, subji, SubjectName[ subji ], variablesshortnames );
                    } // if markovsavecsv



                for ( int relg = 0; relg < numwithinsubjects; relg++ ) {

                                        // copy current group/transition to a 2D array, then normalize to total count
                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
                    for ( int destmap = 0; destmap < nclusters + 1; destmap++ )
                                        // transfer the counts
                        jspc ( frommap, destmap )   = JSP ( relg, trans, frommap, destmap );

                                        // probabilities: normalize each count by the total count
                    jsp     = jspc;

                    jsp.NormalizeArea ();

                                        // here jsp has all existing transitions

                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // expected JSP - note that a map transiting to itself is not possible, at least for 1 seg ahead
                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
                    for ( int destmap = 0; destmap < nclusters + 1; destmap++ )

                        if ( trans == 1 )
                                        // exclude destination case M -> M, and normlize all destination prob.
                            jspx ( frommap, destmap ) = frommap == destmap ? 0 : ProbSeg ( relg, frommap ) * ( ProbSeg ( relg, destmap ) / NonNull ( 1 - ProbSeg ( relg, frommap ) ) );

                        else            // all cases are possible, no restriction on destination state
                            jspx ( frommap, destmap ) =                          ProbSeg ( relg, frommap ) *   ProbSeg ( relg, destmap );

//                    DBGV4 ( relg, trans, subji, jspx.GetSumValues (), "expected JSP:  relg, trans, subji -> total prob" );

                                        // Convert Expected probabilities to equivalent counts, so we can compare jspc and jspcx with Chi Square
                    jspcx   = jspx * jspc.GetSumAbsValues ();


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute Markov Transition Matrix
                    mtm     = jspc;

                    mtm.NormalizeRowsArea ();

//                    DBGV4 ( relg, trans, subji, mtm.GetSumValues (), "MTM:  relg, trans, subji -> total prob" );
//                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
//                        DBGV5 ( relg, trans, subji, frommap, mtm.GetSumAbsValues ( frommap ), "MTM:  relg, trans, subji, row -> total prob" );


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute Expected Markov Transition Matrix
                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
                    for ( int destmap = 0; destmap < nclusters + 1; destmap++ )

                        if ( trans == 1 )
                                        // exclude case A -> A
                            mtmx ( frommap, destmap ) = frommap == destmap ? 0 : ProbSeg ( relg, destmap ) / NonNull ( 1 - ProbSeg ( relg, frommap ) );

                        else            // all cases are possible here, even A -> A
                            mtmx ( frommap, destmap ) =                          ProbSeg ( relg, destmap );


//                  mtmx.NormalizeRowsArea ();  // not needed

//                    DBGV4 ( relg, trans, subji, mtmx.GetSumValues (), "expected MTM:  relg, trans, subji -> total prob" );
//                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
//                        DBGV5 ( relg, trans, subji, frommap, mtmx.GetSumAbsValues ( frommap ), "expected MTM:  relg, trans, subji, row -> total prob" );


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save all maps, all epochs have been done here
                    for ( int frommap = 0; frommap < nclusters + 1; frommap++ )
                    for ( int destmap = 0; destmap < nclusters + 1; destmap++ ) {

                        if ( markovsavecsv  && jspmapsel ( frommap, destmap ) ) {
                            ofjspc  << LocaleListSeparator;
                            ofjspc  << jspc ( frommap, destmap );

                            ofjspcx << LocaleListSeparator;
                            ofjspcx << jspcx ( frommap, destmap );

                            ofjsp   << LocaleListSeparator;
                            ofjsp   << jsp  ( frommap, destmap );

                            ofjspx  << LocaleListSeparator;
                            ofjspx  << jspx ( frommap, destmap );

                            ofmtm   << LocaleListSeparator;
                            ofmtm   << mtm  ( frommap, destmap );

                            ofmtmx  << LocaleListSeparator;
                            ofmtmx  << mtmx ( frommap, destmap );
                            }

                        if ( markovsavefreq )
                            expmarkovfreq[ relg ].Write ( jsp ( frommap, destmap ), subji, frommap * ( nclusters + 1 ) + destmap, trans - 1 );
                        }
                    }


                if ( markovsavecsv ) {
                    ofjspc  << NewLine;
                    ofjspcx << NewLine;
                    ofjsp   << NewLine;
                    ofjspx  << NewLine;
                    ofmtm   << NewLine;
                    ofmtmx  << NewLine;
                    }

                } // for trans

            } // if markovsavefreq || markovsavecsv


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // data split by clusters
        if ( writeclustersfiles ) {

            TExportTracks       expclusters;
            int                 tfpercluster;
            double              offset      = isesipreset && processingref != ReferenceNone ? - NoMore ( (float) 0, Data.GetMinValue () ) : 0;


            for ( int nc = 0; nc < nclusters; nc++ ) {

                tfpercluster    = labels.GetSizeOfClusters ( nc );


                StringCopy      ( expclusters.Filename, BaseFileNameDataCluster );
                StringAppend    ( expclusters.Filename, SubjectName[ subji ] );
                StringAppend    ( expclusters.Filename, "." "Cluster" );
                StringAppend    ( expclusters.Filename, IntegerToString ( buff, nc + 1, 2 ) );
                AddExtension    ( expclusters.Filename, MainExt );

                expclusters.SetAtomType ( IsVector ( datatype ) ? AtomTypeVector : AtomTypeScalar );  // for ris output

                expclusters.NumTracks          = NumElectrodes;
                expclusters.NumTime            = tfpercluster;

                                        // special case of empty cluster
                if ( tfpercluster == 0 ) {

//                    DBGV2 ( subji + 1, nc + 1, "#Subj #Cluster is Empty" );
                                        // still write something
                    if ( writeemptyclusters ) {
                                        // write a single empty map - this should be rare, and for centroids, adding null maps doesn't matter after normalization anyway
                        expclusters.NumTime     = 1;

                        expclusters.Begin ();
                        expclusters.Write ( TMap ( NumRows ) );
                        expclusters.End ();
                        }

                    continue;
                    } // null tfpercluster


                expclusters.Begin ();

                for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

                    if ( labels[ tf ] != nc )
                        continue;


                    for ( int e = 0; e < NumElectrodes; e++ )

                        if ( IsVector ( datatype ) )
                                        // with Norm == input data; without Norm == templates
                            expclusters.Write ( TVector3Float ( Data[ tf ][ 3 * e     ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ),
                                                                Data[ tf ][ 3 * e + 1 ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ),
                                                                Data[ tf ][ 3 * e + 2 ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ) ) );
                        else
                            expclusters.Write (     (float) ( ( Data[ tf ][ e         ] + offset ) * ( writenormalizedclusters ? 1 : Norm[ tf ] ) ) );
                    } // for tf

                expclusters.End ();
                } // for nc

            } // if writeclusters

        } // for subji


    DeallocateVariables ();
                                        // restore generic title
    Cartool.CartoolApplication->SetMainTitle    ( "Fitting of", /*FitTransfer.BaseFileName*/ basefilename, Gauge );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All durations across all subjects & withinsubjects groups
    if ( writestatdurationsfiles ) {

//      TVector<double>         curve;
        TVector<double>         curve_smooth    ( alldurationssmooth.GetDim4 () );  // could re-use the same curve_smooth as above, already allocated..
        TVector<double>         curve_log       ( 1001 );                           // arbitrary size for now
        TVector<int>            count           ( curve_log.GetDim1 () );
        TExportTracks           expdur;


        for ( int relg = 0, absg = gofi1 + relg; relg < numwithinsubjects; relg++, absg++ ) {

                                        // save a file per subject + average
            for ( int subji = 0; subji <= gogof[ absg ].NumFiles (); subji++ ) {

                UpdateApplication;

                                        // Set a local base file name for all things duration
                StringCopy      ( DurationFile,         BaseFileNameStatDurations );
                StringAppend    ( DurationFile,         InfixGroup, IntegerToString ( absg + 1 ) );
                StringAppend    ( DurationFile,         ".", subji < gogof[ absg ].NumFiles () ? SubjectName[ subji ] : "Average" );
                StringAppend    ( DurationFile,         ".", "HistoDuration" );


                                        // raw version
                StringCopy      ( expdur.Filename,          DurationFile );
                StringAppend    ( expdur.Filename,          ".", "Lin" );
                AddExtension    ( expdur.Filename,          FILEEXT_EEGSEF );

                expdur.SetAtomType ( AtomTypeScalar );
                expdur.NumTime              = alldurationsraw.GetDim4 ();
                expdur.NumTracks            = alldurationsraw.GetDim3 ();
                expdur.SamplingFrequency    = alldurationsraw_IndexRatio;
                                        // Note: we don't need any time offset, as the histograms have been forced to start from 0

                expdur.ElectrodesNames.Set ( expdur.NumTracks, ElectrodeNameSize );
                for ( int s = 0; s < reqmaxclusters; s++ )
                    StringCopy ( expdur.ElectrodesNames[ s ], "Map", IntegerToString ( s + 1 ) );

                for ( int tfi = 0; tfi < expdur.NumTime;   tfi++ )
                for ( int s   = 0; s   < expdur.NumTracks; s++   )

                    expdur.Write ( alldurationsraw ( relg, subji, s, tfi ), tfi, s );

                expdur.End ();


                                        // smooth version
//              StringCopy      ( expdur.Filename,          DurationFile );
//              StringAppend    ( expdur.Filename,          ".", "Lin", ".", "Smooth" );
//              AddExtension    ( expdur.Filename,          FILEEXT_EEGSEF );
//
//              expdur.NumTime              = alldurationssmooth.GetDim4 ();
//              expdur.NumTracks            = alldurationssmooth.GetDim3 ();
//              expdur.SamplingFrequency    = alldurationssmooth_IndexRatio;
//                                      // Note: we don't need any time offset, as the histograms have been forced to start from 0
//
//              for ( int tfi = 0; tfi < expdur.NumTime;   tfi++ )
//              for ( int s   = 0; s   < expdur.NumTracks; s++   )
//
//                  expdur.Write ( alldurationssmooth ( relg, subji, s, tfi ), tfi, s );
//
//              expdur.End ();


                                        // save a file per subject + average - log version
                StringCopy      ( expdur.Filename,          DurationFile );
                StringAppend    ( expdur.Filename,          ".", "Log" );
                AddExtension    ( expdur.Filename,          FILEEXT_EEGSEF );

                curve_log.Index1.IndexMin   = log ( (double) 1 );   // conveniently, it's 0
                curve_log.Index1.IndexRatio = ( curve_log.GetDim1 () - 1 ) / ( log ( (double) 1000 ) - log ( (double) 1 ) );

                expdur.NumTime              = curve_log.GetDim1 ();
                expdur.NumTracks            = alldurationsraw.GetDim3 ();
                expdur.SamplingFrequency    = curve_log.Index1.IndexRatio;
                                        // Note: we don't need any time offset, as the histograms have been forced to start from 0


                for ( int s   = 0; s   < expdur.NumTracks; s++   ) {

                    curve_log.ResetMemory ();
                    count    .ResetMemory ();

                                        // for convenience, transfer to a single curve
                    for ( int i = 0; i < (int) curve_smooth; i++ )
                        curve_smooth ( i )  = alldurationssmooth ( relg, subji, s, i );

                    curve_smooth.Index1.IndexMin    = alldurationssmooth_IndexMin;      // for correct conversions - until a better way to save these variables
                    curve_smooth.Index1.IndexRatio  = alldurationssmooth_IndexRatio;

//                  SetOvershootingOption ( interpolate, curve_smooth->Array, LinearDim );

                                        // regular loop
        //          for ( int i = 0; i < (int) curve_log; i++ ) {
                                        // with oversampling
                    for ( double v = 0; v < (int) curve_log; v += 0.25 ) {

                        int         logindex    = Truncate ( v ); // i;
                        double      logvalue    = curve_log.ToReal ( v );
                        double      linvalue    = exp ( logvalue );

        //              int         linindex    = curve_smooth.ToIndex ( linvalue / 1000 );
        //              curve_log ( logindex ) += curve_smooth.GetValue ( linindex );

                                        // data were divided by 1000 to nicely scale with "sampling frequency" display
                                        // will convert to index, then interpolate
                        curve_log ( logindex ) += curve_smooth.GetValueChecked ( linvalue / 1000, InterpolateUniformCubicBSpline );

                        count            ( logindex )++;
        //                if ( VkQuery () )  DBGV5 ( logindex, vlog, vlin, lini, curve_smooth ( lini ), "i, vlog, vlin, lini, curve_smooth ( lini )" );
                        }

                                        // average all values
                    for ( int i = 0; i < (int) curve_log; i++ )
                        curve_log ( i )    /= NonNull ( count ( i ) );


                    curve_log.NormalizeArea ();


                    for ( int tfi = 0; tfi < expdur.NumTime;   tfi++ )

                        expdur.Write ( curve_log ( tfi ), tfi, s );
                    } // for cluster

                expdur.End ();


                } // for subji


                                        // save average of all subjects, per map - raw data
//          curve.Resize ( alldurationsraw.GetDim4 () );
//          curve.Index1.IndexMin       = alldurationsraw_IndexMin;
//          curve.Index1.IndexRatio     = alldurationsraw_IndexRatio;
//
//          for ( int s = 0; s < reqmaxclusters; s++ ) {
//
//              StringCopy      ( buff,             DurationFile );
//              StringAppend    ( buff,             ".", "Lin", ".", "Average" );
//              StringAppend    ( buff,             ".", "Map", IntegerToString ( s + 1 ) );
//              AddExtension    ( buff,             FILEEXT_EEGSEF );
//
//              for ( int i = 0; i < (int) curve; i++ )
//                  curve ( i ) = alldurationsraw ( relg, maxfilespergroup, s, i );
//                                          // averaging all cumulated histograms
//              curve.NormalizeArea ();
//
//              curve.WriteFile ( buff );
//              }

            } // for relg

                                        // deallocate here, so that the next set of groups will allocate its own sizes
        alldurationsraw   .DeallocateMemory ();
        alldurationssmooth.DeallocateMemory ();
        } // if writestatdurationsfiles


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( markovsavefreq ) {
                                        // done with current set of groups
        for ( int relg = 0; relg < numwithinsubjects; relg++ )
            expmarkovfreq[ relg ].End ();

        delete[]    expmarkovfreq;
        expmarkovfreq   = 0;
        } // if markovsavefreq


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( writesegfiles ) {

        Gauge.Next ( gaugefitwriteseg, SuperGaugeUpdateTitle );

                                        // .seg for all subjects, all groups, all epochs together
        TExportTracks       expbigseg;

        StringCopy      ( expbigseg.Filename,          BaseFileName );
        StringAppend    ( expbigseg.Filename,          GroupString );
        AddExtension    ( expbigseg.Filename,          FILEEXT_SEG );


        expbigseg.SetAtomType ( AtomTypeScalar );
        expbigseg.NumClusters      = reqmaxclusters;
        expbigseg.NumFiles         = numwithinsubjects * maxfilespergroup;
        expbigseg.NumTime          = groupnumtf;
        expbigseg.NumTracks        = NumSegVariables;
        expbigseg.VariableNames.Set ( SegFile_VariableNames_Signed, " " Tab );

                                        // loop across time, groups and subjects
        for ( long tf    = 0; tf    < expbigseg.NumTime; tf++    )
        for ( int  relg  = 0; relg  < numwithinsubjects; relg++  )
        for ( int  subji = 0; subji < maxfilespergroup;  subji++ ) {

            UpdateApplication;

            for ( int vari = 0; vari < NumSegVariables; vari++ )

                if ( subji < gogof[ gofi1 + relg ].NumFiles ()
                  && tf    < SubjTimeFrames ( relg, subji, FileDuration ) )

                    expbigseg.Write ( (float) bigsegdata[ subji ] ( relg, vari, tf ) );
                else
                    expbigseg.Write ( (float) 0 );
            }


        expbigseg.End ();


                                        // .seg for each subject, with all groups and all epochs together
        TExportTracks       expseg;

        expseg.SetAtomType ( AtomTypeScalar );
        expseg.NumClusters      = reqmaxclusters;
        expseg.NumFiles         = numwithinsubjects; // maxfilespergroup;
        expseg.NumTracks        = NumSegVariables;
        expseg.VariableNames.Set ( SegFile_VariableNames_Signed, " " Tab );


        for ( int  subji = 0; subji < maxfilespergroup;  subji++ ) {

            Gauge.Next ( gaugefitwriteseg, SuperGaugeUpdateTitle );

            StringCopy      ( expseg.Filename,          BaseFileNameSegmentation );
            StringAppend    ( expseg.Filename,          GroupString );
            StringAppend    ( expseg.Filename,          ".", SubjectName[ subji ] );
            AddExtension    ( expseg.Filename,          FILEEXT_SEG );
                                        // max for all groups of a given subject
            expseg.NumTime          = SubjTimeFrames ( numwithinsubjects, subji, FileDuration );


            expseg.Begin ();

            for ( long tf    = 0; tf    < expseg.NumTime;    tf++   )
            for ( int  relg  = 0; relg  < numwithinsubjects; relg++ )
            for ( int  vari  = 0; vari  < NumSegVariables;   vari++ )

                if ( subji < gogof[ gofi1 + relg ].NumFiles ()
                  && tf    < SubjTimeFrames ( relg, subji, FileDuration ) )

                    expseg.Write ( (float) bigsegdata[ subji ] ( relg, vari, tf ) );
                else
                    expseg.Write ( (float) 0 );


            expseg.End ();
            } // for subji


                                        // add some markers to big .seg file
      /*TSegDoc          *docseg          = (TSegDoc *)*/ Cartool.CartoolDocManager->OpenDoc ( expbigseg.Filename, dtOpenOptions );

/*
        if ( docseg ) {
                                        // clear a possibly existing list
            docseg->RemoveMarkers ( AllMarkerTypes );

                                        // add 2 markers for each epoch
            for ( int epochi = numepochs - 1; epochi >= 0; epochi-- ) {

                long        fromtf  = EpochsFrom ( epochi );
//              long        totf    = EpochsTo   ( epochi );

                MarkerCode          mcode   = (MarkerCode) ( numepochs - epochi );
                char                mstr[32];

                sprintf ( mstr, "Epoch %0d", mcode );
                buff[ MarkerNameMaxLength - 1 ] = 0;

                                        // don't add if at the end
    //          if ( fromtf != 0 )
                    docseg->InsertMarker ( TMarker ( fromtf, fromtf, mcode, mstr, MarkerTypeMarker ) );
//              if ( totf != groupnumtf - 1 )
//                  docseg->InsertMarker ( TMarker ( totf,   totf,   mcode, mstr, MarkerTypeMarker ) );
                } // for epochi

                                        // clean up consecutive markers
            TMarker     marker;
            int         lastpos = -2;
            int         numt    = docseg->GetNumMarkers(); // we are sure to deal only with MarkerTypeMarker
            marker.Set ( -1 );


            for ( int i=0; i < numt; i++ ) {

                docseg->GetNextMarker ( marker );
                                        // remove a single marker if at 0
                if ( numt == 1 && marker.From == 0 ) {
                    docseg->RemoveMarkers ( 0, MaxNumTF, AllMarkerTypes );
                    break;
                    }
                else if ( ( marker.From - lastpos ) <= 1 ) {
                    docseg->RemoveMarkers ( lastpos, lastpos, AllMarkerTypes );
                    marker.From    = lastpos;
                    numt        = docseg->GetNumMarkers();
                    i--;
                    }

                lastpos = marker.From;
                } // num triggers


            docseg->CommitMarkers ();

                                        // refresh the views with what remains of markers
            docseg->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

                                        // close the seg file - actually don't, keep it open to show something to the user
    //      if ( docseg->CanClose ( true ) )
    //		    CartoolDocManager->CloseDoc ( docseg );

            } // if docseg
*/
        } // if writesegfiles
    else {
        Gauge.FinishPart ( gaugefitwriteseg );

        Cartool.CartoolApplication->SetMainTitle    ( Gauge );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( writesegfrequencyfiles ) {

        TExportTracks       freqmapfile;

        freqmapfile.SetAtomType ( AtomTypeScalar );
        freqmapfile.NumTracks           = nclusters;
        freqmapfile.SamplingFrequency   = samplingfrequency;

        freqmapfile.ElectrodesNames.Set ( freqmapfile.NumTracks, ElectrodeNameSize );
        for ( int s = 0; s < nclusters; s++ )
            StringCopy ( freqmapfile.ElectrodesNames[ s ], "Map", IntegerToString ( s + 1 ) );

                                        // save a single segfreq file for all epochs together
        for ( int relg  = 0, absg = gofi1 + relg; relg < numwithinsubjects; relg++, absg++ )
        for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

            UpdateApplication;

            StringCopy      ( freqmapfile.Filename,          BaseFileNameSegFrequency );
            StringAppend    ( freqmapfile.Filename,          InfixGroup, IntegerToString ( absg  + 1 ) );
            StringAppend    ( freqmapfile.Filename,          ".", SubjectName[ subji ] );
            StringAppend    ( freqmapfile.Filename,          ".", "SegFreq" );
            AddExtension    ( freqmapfile.Filename,          FILEEXT_EEGSEF );

            freqmapfile.NumTime             = SubjTimeFrames ( relg, subji, FileDuration );


            freqmapfile.Begin ();

                                        // poke a rectangle - easy
//          for ( int tfi = 0; tfi < freqmapfile.NumTime;   tfi++ )
//              if ( IsInsideLimits ( (int) bigsegdata[ subji ] ( relg, SegVarSegment, tfi ), 1, nclusters ) )   // by security
//                  freqmapfile.Write ( 1, tfi, bigsegdata[ subji ] ( relg, SegVarSegment, tfi ) - 1 );

                                        // NOTE: this code kind of duplicates the analysis of segments durations done in the variables extraction loop

                                        // poke a raised cosine
            long        begintf     = 0;

            for ( int tfi = 0; tfi < freqmapfile.NumTime;   tfi++ ) {

                if ( ! ( tfi >= SubjTimeFrames ( relg, subji, FileDuration ) - 1
                      || bigsegdata[ subji ] ( relg, SegVarSegment, tfi + 1 ) != bigsegdata[ subji ] ( relg, SegVarSegment, tfi ) ) )

                     continue;
                                        // here we have a segment

                long        endtf       = tfi;
                long        duration    = endtf - begintf + 1;
                int         mapi        = bigsegdata[ subji ] ( relg, SegVarSegment, begintf ) - 1;

                                        // by security
                if ( ! IsInsideLimits ( mapi, 0, nclusters - 1 ) ) {
                    begintf     = tfi + 1;
                    continue;
                    }

                                        // do special cases separately to avoid cosine rounding approximations
                if      ( duration == 1 )
                                        // 1 TF -> poke single peak
                    freqmapfile.Write ( 1, begintf, mapi );

                else if ( duration == 2 ) {
                                        // 2 TF -> poke wider single peak
                    freqmapfile.Write ( 1, begintf, mapi );
                    freqmapfile.Write ( 1, endtf,   mapi );
                    }

//              else if ( duration == 3 )
//                                      // 3 TF -> poke single central peak
//                  freqmapfile.Write ( 1, begintf + 1, mapi );

                else
                                        // poke cosine, begintf is non-null, endtf is null, so we could chain multiple cosines together
                    for ( int tf2 = begintf, bloblen = 1; tf2 <= endtf; tf2++, bloblen++ )
                        freqmapfile.Write ( ( - cos ( bloblen / (double) ( duration + 1 ) * TwoPi ) + 1 ) / 2, tf2, mapi );

                                        // next segment will begin after current TF
                begintf     = tfi + 1;
                } // for tfi


            freqmapfile.End ();
            } // for relg/subji

        } // if writesegfrequencyfiles


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update the verbose with current epoch

    verbose.NextTopic ( "Labeled vs Unlabeled TFs:" );
    {
    verbose.Put ( "Force minimum Correlation:", dolimitcorr );
    if ( dolimitcorr )
        verbose.Put ( "Min Correlation allowed for Labeling:", limitcorr * 100, 2, " [%]" );

    verbose.NextLine ( 2 );


    verbose.ResetTable ();

    verbose.TableColNames.Add ( "File"         );
    verbose.TableColNames.Add ( "EpochsTFs"    );
    verbose.TableColNames.Add ( "LabeledTFs"   );
    verbose.TableColNames.Add ( "UnlabeledTFs" );
    verbose.TableColNames.Add ( "Labeled%"     );
    verbose.TableColNames.Add ( "Unlabeled%"   );


    for ( int subji = 0; subji < maxfilespergroup; subji++ )
        verbose.TableRowNames.Add ( SubjectName[ subji ] );

    verbose.TableRowNames.Add ( "All" );


    verbose.BeginTable ( 15 );

                                        // all subjects + cumulated on additional row
    for ( int subji = 0; subji <= maxfilespergroup; subji++ ) {

        verbose.PutTable ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration          ) );
        verbose.PutTable ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationLabeled   ) );
        verbose.PutTable ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationUnlabeled ) );
        verbose.PutTable ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationLabeled   ) / (double) NonNull ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration ) ) * 100, 2 );
        verbose.PutTable ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDurationUnlabeled ) / (double) NonNull ( SubjTimeFrames ( numwithinsubjects, subji, EpochsDuration ) ) * 100, 2 );
        } // for subji


    verbose.EndTable ();

    verbose.Flush ();
    }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // GEV
    verbose.NextTopic ( "GEV:" );
    {
    verbose.Put ( "GEV is computed:", noncompetitive ? "For each template, all conditions together" : "Globally, all conditions together" );

    verbose.NextLine ( 2 );


    if ( noncompetitive ) {

        verbose.ResetTable ();

        verbose.TableColNames.Add ( "File" );
        for ( int nc = 0; nc < nclusters; nc++ )
            verbose.TableColNames.Add ( StringCopy ( buff, "GEV", IntegerToString ( nc + 1 ), "%" ) );

        for ( int subji = 0; subji < maxfilespergroup; subji++ )
            verbose.TableRowNames.Add ( SubjectName[ subji ] );
        verbose.TableRowNames.Add ( "Average" );

        verbose.BeginTable ( 8 );


        TGoEasyStats        stat ( nclusters );

        for ( int subji = 0; subji < maxfilespergroup; subji++ )
        for ( int nc = 0; nc < nclusters; nc++ ) {

            verbose.PutTable ( SubjGEV ( subji, nc ) * 100, 2 );

            stat[ nc ].Add   ( SubjGEV ( subji, nc ), ThreadSafetyIgnore );
            }

        for ( int nc = 0; nc < nclusters; nc++ )
            verbose.PutTable ( stat[ nc ].Average () * 100, 2 );


        verbose.EndTable ();
        }
    else {

        verbose.ResetTable ();

        verbose.TableColNames.Add ( "File" );
        verbose.TableColNames.Add ( "GEV%" );

        for ( int subji = 0; subji < maxfilespergroup; subji++ )
            verbose.TableRowNames.Add ( SubjectName[ subji ] );
        verbose.TableRowNames.Add ( "Average" );

        verbose.BeginTable ( 8 );


        TEasyStats          stat;

        for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

            verbose.PutTable ( SubjGEV ( subji, 0 ) * 100, 2 );

            stat.Add ( SubjGEV ( subji, 0 ), ThreadSafetyIgnore );
            }

        verbose.PutTable ( stat.Average () * 100, 2 );


        verbose.EndTable ();
        }

    verbose.Flush ();
    }

    verbose.NextLine ( 2 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( bigsegdata ) {

        for ( int fi = 0; fi < maxfilespergroup; fi++ )
            bigsegdata[ fi ].DeallocateMemory ();

        delete[]    bigsegdata;
        bigsegdata  = 0;
        }

    } // for groups of (groups of files), per numwithinsubjects


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 
if ( deletetempfiles ) {

//  gogof.DeleteFiles ( true );

    for ( int gofi0 = 0; gofi0 < gogof.NumGroups (); gofi0++ ) {

        StringCopy  ( buff, BaseDir, "\\", FittingPreProcGroup, IntegerToString ( gofi0 + 1 ) );

        NukeDirectory ( buff );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
bool                savehistovars       = true;

                                        // Save fitting data, by unfolding it into n files of Variables tracks
if ( savehistovars ) {

    TGoF                filescsv;
    TSpreadSheet        sf;
    CsvType             csvtype;
    TFileName           attr;

    int                 csvnumgroups;
    TArray1<int>        csvgroupindex;
    int                 csvnumvarspergroup;
    TStrings            csvvarnames;
    int                 csvmaxmap;
    TSelection          csvmapsel;
    TStrings            csvmapnames;
    int                 csvnumfitvars;
    TStrings            csvfitvarnames;

    TExportTracks       expvars;


    StringCopy          ( ExcelFile, BaseFileName );
    StringAppend        ( ExcelFile, "*" InfixCartoolWorksheet );
    AddExtension        ( ExcelFile, FILEEXT_CSV );
    filescsv.FindFiles  ( ExcelFile );


    for ( int i = 0; i < (int) filescsv; i++ ) {

        if ( ! sf.ReadFile ( filescsv[ i ] ) )
            continue;


        csvtype         = sf.GetType ();

                                       
        if ( ! IsCsvStatFittingCartool ( csvtype ) )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        sf.GetFittingInfo ( csvnumgroups, csvgroupindex, 0, csvnumvarspergroup, csvvarnames, csvmaxmap, csvmapsel, csvmapnames, csvnumfitvars, csvfitvarnames );
    

        if ( csvnumgroups == 0 )
            continue;
            
                                            // now feed with the groups, though coming from the same file
        for ( int gi = 0; gi < csvnumgroups; gi++ ) {


            int         numsamples          = sf.GetNumRecords ();
            int         nummaps             = (int) csvmapsel;


            StringCopy      ( GroupString,  InfixGroup, IntegerToString ( csvgroupindex ( gi ) ) );

            StringCopy      ( expvars.Filename,          BaseFileName );
            StringAppend    ( expvars.Filename,          GroupString,    "." "FittingVariables" );
            AddExtension    ( expvars.Filename,          FILEEXT_EEGSEF );


            expvars.SetAtomType ( AtomTypeScalar );
            expvars.NumTime          = nummaps * numsamples;
            expvars.NumTracks        = csvnumfitvars;

            expvars.ElectrodesNames.Resize ( expvars.NumTracks, ElectrodeNameSize );
            for ( int fvni = 0; fvni < expvars.NumTracks; fvni++ )
                StringCopy ( expvars.ElectrodesNames[ fvni ], csvfitvarnames ( fvni ), ElectrodeNameSize - 1 );
    

            for ( int s = numsamples; s >= 0; s-- ) 
            for ( int e = 0; e < csvnumvarspergroup; e++ ) {

                sf.GetRecord ( s, 1 + ( gi * csvnumvarspergroup ) + e, attr );
                                    // all maps and all subjects value sequentially
                expvars.Write ( StringToDouble ( attr ), Truncate ( e / csvnumfitvars ) * numsamples + s, e % csvnumfitvars );
                }


            expvars.End ();
            } // for group
        }

    } // savehistovars
 */  

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.FinishParts ();

Cartool.CartoolApplication->SetMainTitle    ( Gauge );

Gauge.HappyEnd ();

return  true;
}

//----------------------------------------------------------------------------

bool    TMicroStates::BackFitting       (   const char*         templatefile,
                                            TGoGoF&             gogof,              int                     numwithinsubjects,
                                            AnalysisType        analysis,           ModalityType            modality,
                                            AtomType            datatype,           PolarityType            polarity,           ReferenceType       dataref,
                                            bool                dolimitcorr,        double                  limitcorr,
                                            bool                smoothing,          int                     smoothinghalfsize,  double              smoothinglambda,
                                            bool                rejectsmall,        int                     rejectsize,
                                            const TSelection&   varout,
                                            MicroStatesOutFlags outputflags,
                                            const char*         basefilename
                                        )
{
return  BackFitting     (   templatefile,
                            gogof,              numwithinsubjects,
                            analysis,           modality,
                            TStrings ( 0, 0 ),  TStrings ( 0, 0 ),  TStrings ( 0, 0 ),  // empty objects
                            SpatialFilterNone,  0,
                            NoSkippingBadEpochs,0,
                            datatype,           polarity,           dataref,
                            dolimitcorr,        limitcorr,
                            true,
                            false,
                            smoothing,          smoothinghalfsize,  smoothinglambda,
                            rejectsmall,        rejectsize,
                            false,      0,
                            varout,
                            outputflags,
                            basefilename
                        );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
