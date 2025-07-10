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

#include    "ESI.ComputingRis.h"

#include    "Dialogs.TSuperGauge.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Files.Conversions.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.BatchAveragingFiles.h"
#include    "Files.Extensions.h"
#include    "Files.PreProcessFiles.h"   // GetInverseInfix

#include    "TInverseMatrixDoc.h"
#include    "TFreqDoc.h"

#include    "ComputeCentroidFiles.h"
#include    "TComputingRisDialog.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // checking the Z-Score file is actually compatible with the inverse
bool    IsZScoreFileCompatible  (   const TInverseMatrixDoc*    isdoc,
                                    const char*                 zscorefile
                                )
{
if ( isdoc == 0 || StringIsEmpty ( zscorefile ) )
    return  false;


int                 numspzscorefile;
int                 numfactorszscorefile;

ReadFromHeader ( zscorefile, ReadNumTimeFrames, &numspzscorefile      );
ReadFromHeader ( zscorefile, ReadNumElectrodes, &numfactorszscorefile );


return  ( numfactorszscorefile == 2 || numfactorszscorefile == 9 )  // mean + SD or 9 parameters
       && numspzscorefile == isdoc->GetNumSolPoints ();             // exact same number of correction factors as solution points
}


//----------------------------------------------------------------------------

bool    ComputingRis    (   ComputingRisPresetsEnum esicase,
                            const TGoGoF&       subjects,                  
                            int                 numsubjects,            int                 numconditions,
                            
                            const TGoF&         inversefiles,           RegularizationType  regularization,     BackgroundNormalization     backnorm,
                            AtomType            datatypeepochs,         AtomType            datatypefinal,
                            CentroidType        centroidsmethod,

                            SpatialFilterType   spatialfilter,          const char*         xyzfile,
                            bool                ranking,
                            bool                thresholding,           double              keepingtopdata,
                            bool                envelope,               FilterTypes         envelopetype,       double          envelopeduration,
                            bool                roiing,                 const char*         roifile,            FilterTypes     roimethod,

                            bool                savingindividualfiles,  bool                savingepochfiles,   bool            savingzscorefactors,
                            bool                computegroupsaverages,  bool                computegroupscentroids,
                            const char*         basedir,                const char*         fileprefix,
                            VerboseType         verbosey
                        )
{
                                        // well, we do need some data...
if ( subjects.IsEmpty () )
    return false;


if ( spatialfilter != SpatialFilterNone )
    if ( StringIsEmpty ( xyzfile ) || esicase == ComputingRisPresetFreq )
        spatialfilter = SpatialFilterNone;


if ( inversefiles.IsEmpty ()  || StringIsEmpty ( basedir ) )
    return false;

                                        // force silent if not in interactive mode
if ( verbosey == Interactive && CartoolObjects.CartoolApplication->IsNotInteractive () )
    verbosey = Silent;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numgroups           = subjects.NumGroups ();
int                 numfiles            = subjects.NumFiles ( 0, numgroups - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Full check - could take some time, skipped at that point
//bool                freqgroups          = subjects.AllFreqsGroup ();
//bool                freqtypecomplex     = false;
////bool              freqgtypereal       = false;
//int                 numfreqs            = 0;
//
//if ( freqgroups ) {
//
//    FreqsCompatibleClass        fc;
//
//    subjects.AllFreqsAreCompatible ( fc );
//
//    freqtypecomplex     = freqgroups && IsFreqTypeComplex ( (FrequencyAnalysisType) fc.FreqType );
////  freqgtypereal       = freqgroups && IsFreqTypeReal    ( (FrequencyAnalysisType) fc.FreqType );
//    }

                                        // fully testing all files should be the responsibility of caller
                                        // here we just wish to retrieve these infos / values, assuming all files are the same
bool                freqgroups          = IsExtensionAmong ( subjects[ 0 ][ 0 ], AllFreqFilesExt );
int                 freqtype            = FreqUnknown;
bool                freqtypecomplex     = false;
//bool              freqgtypereal       = false;
int                 numfreqs            = 0;

if ( freqgroups ) {

    if ( ! ReadFromHeader ( TFileName ( subjects[ 0 ][ 0 ] ), ReadFrequencyType,  &freqtype ) )
        return false;

    freqtypecomplex     = IsFreqTypeComplex ( (FrequencyAnalysisType) freqtype );
//  freqgtypereal       = IsFreqTypeReal    ( (FrequencyAnalysisType) freqtype );

    if ( ! ReadFromHeader ( TFileName ( subjects[ 0 ][ 0 ] ), ReadNumFrequencies, &numfreqs ) )
        return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Testing if we the rigth amount of inverse files
//bool              singleinverse       = IsSingleInverse       ( inversefiles );
bool                matchinginverses    = AreMatchingInverses   ( inversefiles, subjects );

int                 stepinverse         = 1;

                                        // Opening the (first) inverse matrix, which is good enough here
TOpenDoc<TInverseMatrixDoc> isdoc ( inversefiles[ 0 ], OpenDocHidden );
char                        ispostfilename[ 256 ];

if ( ! isdoc.IsOpen () )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data types - we need some care here

                                        // don't output vectorial results for frequencies (complex case)
if ( IsVector ( datatypefinal ) && esicase == ComputingRisPresetFreq )
    datatypefinal   = AtomTypePositive;

                                        // Actual processing data type + Z-Score + Envelope
AtomType                datatypeproc    = CRISPresets[ esicase ].IsEpochs () ? datatypeepochs : datatypefinal;

                                        // Can not save to Vector if processing is Norm
if ( ! IsVector ( datatypeproc ) && IsVector ( datatypefinal ) )
    datatypefinal   = datatypeproc;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // epochs / subjects initial ranking
bool                dataranking         = ranking                                                               // spare ranking if not saving any other files than centroid
                                        && ( computegroupsaverages || savingindividualfiles || savingepochfiles || ! computegroupscentroids );


bool                datathresholding    = dataranking && thresholding;  // currently only the combo ranking + thresholding is allowed - might change if needed, though

double              datathreshold       = 1 - keepingtopdata;           // actual threshold used at each step

                                        // resetting parameter?
if ( datathreshold <= 0 ) {

    datathresholding    = false;
    datathreshold       = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Centroids parameters

                                        // Default: Vectorial data -> vectorial centroid; Norm data -> norm centroid
                                        // !In case of epochs + centroids, final centroids are currently of centroidsdatatype, not datatypefinal!
AtomType            centroidsdatatype       = datatypeproc;
                                        // ?Norm seems to be the best choice, even for ERPs: clusters have too much dispersion anyway? - Used for Clusters (&Epochs) and both Within and Across Subjects
//AtomType          centroidsdatatype       = AtomTypePositive;
                                        // Polarities matter only for Spontaneous, Vectorial data

PolarityType        centroidspolarity       = CRISPresets[ esicase ].IsSpontaneous () && IsVector ( centroidsdatatype ) ?   PolarityEvaluate 
                                                                                                                        :   PolarityDirect;   // ERP cases DO care for polarities, even from Epochs - although we don't compute centroids in these cases

ReferenceType       centroidsref            = ReferenceNone;
                                        // average of sources looks better with ranking - "vectorial" ranking is allowed

bool                centroidsranking        = ranking && computegroupscentroids;

bool                centroidsthresholding   = centroidsranking && thresholding;

double              centroidsthreshold      = 1 - keepingtopdata;

                                        // resetting parameter?
if ( centroidsthreshold <= 0 ) {

    centroidsthresholding   = false;
    centroidsthreshold      = 0;
    }

                                        // normalize only if not ranking
bool                centroidsnormalized     = ! centroidsranking;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TracksContentType   contenttype         = esicase == ComputingRisPresetErpGroupMeans    ? TracksContentERP
                                        : esicase == ComputingRisPresetErpIndivMeans    ? TracksContentERP
                                        : esicase == ComputingRisPresetErpIndivEpochs   ? TracksContentEEGRecording // !Epochs are raw EEG!
                                        : esicase == ComputingRisPresetErpSegClusters   ? TracksContentERP
                                        : esicase == ComputingRisPresetErpFitClusters   ? TracksContentERP

                                        : esicase == ComputingRisPresetIndIndivEpochs   ? TracksContentEEGRecording // !Epochs are raw EEG!

                                        : esicase == ComputingRisPresetSpont            ? TracksContentEEGRecording
                                        : esicase == ComputingRisPresetSpontClusters    ? TracksContentEEGRecording

                                        : esicase == ComputingRisPresetFreq             ? TracksContentFrequency
                                        :                                                 TracksContentUnknown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

roiing      = roiing && StringIsNotEmpty ( roifile ) && ! IsVector ( datatypeproc );    // we have to do something to forbid ROIing on non-scalar data

TRois*              rois            = roiing ? new TRois ( roifile ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Z-Score options

ZScoreType          zscoremethod            = SetZScore ( ZScoreNone, ZScoreAllData /*ZScoreMaxData*/ );

if ( backnorm != BackgroundNormalizationNone )

//  DataTypeToZScoreEnum ( datatypeproc, TracksContentUnknown /*contenttype*/, 0, zscoremethod );   // !asking user, for tests!
    DataTypeToZScoreEnum ( datatypeproc, contenttype, 0, zscoremethod );


if ( backnorm == BackgroundNormalizationLoadingZScoreFile ) {
                                        // preparing the Grep post fix to be compatible with current parameters & inverse
    char                isinfix[ 256 ];
    ClearString ( ispostfilename );


    if ( spatialfilter != SpatialFilterNone )

        StringAppend ( ispostfilename, ".", PostfixSpatialFilter );

                                        // add infix inverse name
    StringAppend            ( ispostfilename, ".", GetInverseInfix ( isdoc, regularization, datatypeproc, isinfix ) );

    StringGrepNeutral       ( ispostfilename );

    StringAppend            ( ispostfilename, "\\..+", InfixStandardizationFactorsGrep );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Envelope mainly for Induced filtered data - but option is still open for the others
envelope        = envelope && ! IsVector ( datatypeproc );

if ( envelope && envelopeduration <= 0 ) {
    envelope            = false;
    envelopetype        = FilterTypeNone;
    envelopeduration    = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // More tracks output options
bool                computingindividualfiles= savingindividualfiles
                                           || savingepochfiles
                                           || computegroupsaverages
                                           || computegroupscentroids;


                    savingzscorefactors     = savingzscorefactors
                                           && backnorm != BackgroundNormalizationNone;
//                                         && backnorm == BackgroundNormalizationComputingZScore;       // note that we could allow a new copy of Z-Scores in case of loading from file...

                                        // all processings:
bool                isprocessing            = savingindividualfiles     
                                           || savingepochfiles        
                                           || computegroupsaverages        
                                           || computegroupscentroids
                                           || savingzscorefactors;

if ( ! isprocessing )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseFileName;
TFileName           localfileprefix;
TFileName           VerboseFile;
TFileName           CentroidsFile;


StringCopy      ( localfileprefix, fileprefix );

if ( StringIsNotEmpty ( localfileprefix ) )
    StringAppend ( localfileprefix, "." );


StringCopy      ( BaseFileName,             basedir,                "\\",               localfileprefix );

StringCopy      ( VerboseFile,              BaseFileName,           "Computing RIS",    "." FILEEXT_VRB );

CheckNoOverwrite( VerboseFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff[ KiloByte ];
TVerboseFile        verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Computing Results of Inverse Solution" );


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
verbose.Put ( "Inverse Matrix file:", matchinginverses ? "See Input File section" : isdoc->GetDocPath () );
verbose.Put ( "Regularization level:", RegularizationToString ( regularization, buff, false ) );

verbose.NextLine ();
                                        // TracksContentNames is not precise enough, we do it manually here
StringCopy      ( buff, contenttype == TracksContentEEGRecording? ( CRISPresets[ esicase ].IsEpochs () ? "Raw EEG" : "Spontaneous EEG" )
                      : contenttype == TracksContentERP         ?   "ERP"
                      : contenttype == TracksContentFrequency   ?   "Frequencies"
                      :                                             "Other"         );
verbose.Put ( "Type of input data:", buff );

if ( CRISPresets[ esicase ].IsEpochs () ) {
    if      ( IsPositive ( datatypeepochs ) )   verbose.Put ( "Epochs data type:", "Positive Data (Norm of Vectors)" );
    else if ( IsVector   ( datatypeepochs ) )   verbose.Put ( "Epochs data type:", "3D Vectorial Data" );
    }

if      ( IsPositive ( datatypefinal ) )        verbose.Put ( "Final data type:", "Positive Data (Norm of Vectors)" );
else if ( IsVector   ( datatypefinal ) )        verbose.Put ( "Final data type:", "3D Vectorial Data" );

if ( computegroupscentroids ) {
    verbose.NextLine ();
//  verbose.Put ( "Computing each groups' centroids:",      computegroupscentroids  );
    verbose.Put ( "Centroids method:",          CentroidNames[ centroidsmethod ] );

    if      ( IsPositive ( centroidsdatatype ) )    verbose.Put ( "Centroids data type:", "Positive Data (Norm of Vectors)" );
    else if ( IsVector   ( centroidsdatatype ) )    verbose.Put ( "Centroids data type:", "3D Vectorial Data" );
    }
}


verbose.NextTopic ( "Inverse Postprocessing:" );
{
verbose.Put ( "Standardizing results:", BackgroundNormalizationNames[ backnorm ] );
if ( backnorm != BackgroundNormalizationNone )
    verbose.Put ( "Standardization method:", ZScoreEnumToString ( zscoremethod ) );

verbose.Put ( "Ranking results:", dataranking );

verbose.Put ( "Thresholding results:", datathresholding );
if ( datathresholding )
    verbose.Put ( "Threshold level:", datathreshold, 2 );

verbose.Put ( "Envelope of results:", envelope );
if ( envelope ) {
    verbose.Put ( "Envelope method:", FilterPresets[ envelopetype ].Text );
    verbose.Put ( "Envelope lowest frequency:", MillisecondsToFrequency ( envelopeduration ), 2, " [Hz]" );
    verbose.Put ( "Envelope duration:", envelopeduration, 2, " [ms]" );
    }

verbose.Put ( "Applying ROIs:", roiing );
if ( roiing ) {
    verbose.Put ( "ROIs file:", roifile );
    verbose.Put ( "ROIs method:", FilterPresets[ roimethod ].Text );
    }

if ( computegroupscentroids ) {
    verbose.NextLine ();
//  verbose.Put ( "Centroids reference:",       ReferenceNames[ centroidsref ] );
    verbose.Put ( "Centroids Polarity test:",   IsPositive ( centroidsdatatype ) ? "Not relevant" : PolarityNames[ centroidspolarity ] );
    verbose.Put ( "Centroids ranking:",         centroidsranking );
    verbose.Put ( "Centroids thresholding:",    centroidsthresholding );
    if ( centroidsthresholding )
        verbose.Put ( "Centroids threshold level:", centroidsthreshold, 2 );
    verbose.Put ( "Centroids normalization:",   centroidsnormalized );
    }
}


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Number of subjects:",   numsubjects   );
verbose.Put ( "Number of conditions:", numconditions );


for ( int si = 0; si < numgroups; si++ ) {

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


    if      ( matchinginverses )    verbose.Put ( "Individual Inverse Matrix file:", inversefiles[ si ] );
    else if ( numsubjects > 1  )    verbose.Put ( "Common Inverse Matrix file:",     inversefiles[  0 ] );
    else                            verbose.Put ( "Inverse Matrix file:",            inversefiles[  0 ] );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Saving every individual ris files:",     savingindividualfiles   );
verbose.Put ( "Saving every epoch ris files:",          savingepochfiles        );
verbose.Put ( "Computing each groups' averages:",       computegroupsaverages   );
verbose.Put ( "Computing each groups' centroids:",      computegroupscentroids  );
verbose.Put ( "Saving standardization factors files:",  savingzscorefactors     );

verbose.NextLine ();
verbose.Put ( "Verbose file (this):", VerboseFile );
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                GaugeRisEnum
                    {
                    gaugeriscompglobal,
                    gaugerisfreqgroups,
                    gaugeriscomppreproc,
                    gaugeriscompavg,
                    };

                                        // shrinking size to keep the progress bar under observation
//WindowSetGaugeSize ( CartoolObjects.CartoolMainWindow ); 


TSuperGauge         Gauge;

if ( verbosey == Interactive ) {

    Gauge.Set           ( ComputingRisTitle, SuperGaugeLevelInter );

    Gauge.AddPart       ( gaugeriscompglobal,   1,                                          esicase == ComputingRisPresetFreq ?  1 :  1 );
    Gauge.AddPart       ( gaugerisfreqgroups,   3 * numsubjects,                            esicase == ComputingRisPresetFreq ? 20 :  0 );
                       // number of subjects will be repeated for all freqs
    Gauge.AddPart       ( gaugeriscomppreproc,  AtLeast ( 1, numfreqs ) * numsubjects,      esicase == ComputingRisPresetFreq ? 69 : 89 );
    Gauge.AddPart       ( gaugeriscompavg,      numgroups,                                  esicase == ComputingRisPresetFreq ? 10 : 10 );


    if ( /*numgroups > 1 ||*/ numfiles > 10 )
                                            // batch can be long, hide Cartool until we are done
        WindowMinimize ( CartoolObjects.CartoolMainWindow );
    }

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoGoF				gogofpersubject;
TGoGoF		        gogofallsubjectspreproc;
TGoGoF				gogofpercondition;
TGoGoF		        risgogof;
TGoF                gofmean;
//TGoGoF              gogofsd;
TGoF                baselistpreproc;
TGoF                centroidsgof;
TGoF                zscoregofout;
TGoF                zscoregofin;
BackgroundNormalization actualbacknorm;
bool                    actualsavingzscorefactors;
bool                newfiles;
int                 numexpandedfreqs    = 0;
TGoMaps             centroids;
//#define             ShowGroupsShuffling


if ( computegroupscentroids )

    for ( int i = 0; i < numsubjects; i++ )
                                        // 1 file per subject, containing all conditions
        centroids.Add ( new TMaps ( numconditions, isdoc->GetNumSolPoints () * ( IsVector ( centroidsdatatype ) ? 3 : 1 ) ) );


int                 gofi1           = 0;
int                 gofi2           = numgroups - 1;


#ifdef ShowGroupsShuffling
subjects.Show ( gofi1, gofi2, "0) Original Groups" );
#endif // ShowGroupsShuffling

gogofpersubject     = subjects;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Frequency case:
                                        // we need to split the data per frequency, therefore generating new files and a new GoF
if ( esicase == ComputingRisPresetFreq ) {

    TGoGoF              onesubjallfreqs;
    TGoGoF              onesubjallfreqsbycond;
    TGoGoF              onesubjallfreqsbycondrealimag;

                                        // we build a new list of files, for each frequency
    gogofpersubject.Reset ();

                                        // for each subject
    for ( int absg = 0; absg < subjects.NumGroups (); absg++ ) {

        Gauge.Next ( gaugerisfreqgroups );

                                        // we will have all freqs for cond1, then all freqs for cond2, etc...
        subjects[ absg ].SplitFreqFiles ( SplitFreqByFrequency, &onesubjallfreqs, false );
                                        // results: first GoF   SiC1F1R, SiC1F1I, SiC1F2R, SiC1F2I, etc...
                                        //          next  GoF   SiC2F1R, SiC2F1I, SiC2F2R, SiC2F2I, etc...


        Gauge.Next ( gaugerisfreqgroups );

                                        // we need to reorganize the results again
        onesubjallfreqs.ConditionsToSubjects ( 0, onesubjallfreqs.NumGroups () - 1, onesubjallfreqsbycond );
                                        // results: first GoF   SiC1F1R, SiC2F1R
                                        //          next  GoF   SiC1F1I, SiC2F1I
                                        //          next  GoF   SiC1F2R, SiC2F2R
                                        //          next  GoF   SiC1F2I, SiC2F2I
                                        //          ...


        if ( freqtypecomplex ) {
                                        // merge real and imaginary into a single GoF
                                        // better for any normalization and standardization
                                        // also we can give the whole real+imag data to PreProcessFiles
            int                 numgroups           = onesubjallfreqsbycond.NumGroups ();
            int                 numfilespergroup    = onesubjallfreqsbycond[ 0 ].NumFiles ();   // this is constant across all groups

            onesubjallfreqsbycondrealimag.Reset ();

                                        // divide by 2 the number of GoFs
            for ( int g2i = 0, gi = 0; g2i < numgroups; g2i+=2, gi++ ) {

                onesubjallfreqsbycondrealimag.Add ( new TGoF );

                for ( int fi = 0; fi < numfilespergroup; fi++ ) {
                                        // interleave real - imag - real - imag etc...
                    onesubjallfreqsbycondrealimag[ gi ].Add ( onesubjallfreqsbycond[ g2i     ][ fi ] );
                    onesubjallfreqsbycondrealimag[ gi ].Add ( onesubjallfreqsbycond[ g2i + 1 ][ fi ] );
                    }
                }
                                        // results: first GoF   SiC1F1R, SiC1F1I, SiC2F1R, SiC2F1I
                                        //          next  GoF   SiC1F2R, SiC1F2I, SiC2F2R, SiC2F2I
                                        //          ...

                                        // all subjects, all freqs concatenated
            gogofpersubject.Add ( onesubjallfreqsbycondrealimag );

                                        // Note: all frequencies of all subjects will end up in the same GoF
                                        // so we need a way to remember how to group the frequencies together PER SUBJECT,
                                        // and not all subjects at once. Simply count how freqs there are, and later on merge
                                        // by that amount of files.
                                        // Also, frequency files should have the same number of frequency...
            if ( numexpandedfreqs == 0 )
                numexpandedfreqs    = onesubjallfreqsbycondrealimag.NumGroups ();
            } // if freqtypecomplex

        else { // if ( freqgtypereal )
                                        // all subjects, all freqs concatenated
            gogofpersubject.Add ( onesubjallfreqsbycond );

            if ( numexpandedfreqs == 0 )
                numexpandedfreqs    = onesubjallfreqsbycond.NumGroups ();
            }

        } // for subjects


                                        // the list of inverses itself is not expanded, so we have to keep track of the ratio EEG / Inverse for proper stepping
    if ( matchinginverses )

        stepinverse     = gogofpersubject.NumGroups () / subjects.NumGroups ();


    #ifdef ShowGroupsShuffling
    gogofpersubject.Show ( "1Bis) Frequency Groups Reordered per Subject" );
    #endif // ShowGroupsShuffling

    } // if esicase == ComputingRisPresetFreq
else
    Gauge.FinishPart ( gaugerisfreqgroups );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Trying to retrieve all z-score factors files first
zscoregofin.Reset ();

                                        // this will fill-up zscoregofin with as many entries as groups
if ( backnorm == BackgroundNormalizationLoadingZScoreFile ) {

    TGoF                loadzscorefiles;
    TFileName           f;


    for ( int absg = 0; absg < gogofpersubject.NumGroups (); absg++ ) {
                                        // look within any directory of any of the file until 1 file looks what we want
                                        // in case of multiple files exist, the latest one is taken
        loadzscorefiles.GrepGoF ( gogofpersubject[ absg ], ".*", ispostfilename, AllZScoreFilesExt, false );

                                        // Asking user if no Z-Score file is found(?)
        if ( loadzscorefiles.IsEmpty () && verbosey == Interactive ) {

            StringCopy      ( f, "Standardization factors file is missing for  '", ToFileName ( gogofpersubject[ absg ][ 0 ] ), "'  , please provide another one:" );

            GetFileFromUser     getfiles ( f, AllZScoreFilesFilter, 1, GetFileRead );

            if ( getfiles.Execute () )

                loadzscorefiles = getfiles;
            }

                                        // checking again
        if      ( loadzscorefiles.IsEmpty () ) {

            StringCopy      ( f, "Standardization factors file missing for  '", ToFileName ( gogofpersubject[ absg ][ 0 ] ), "'" );
                                        // !NOT a valid file, just for the verbose output and to keep the number of files in sync!
            zscoregofin.Add ( f );
            }
                                        // checking the Z-Score file is actually compatible with our inverse
        else if ( ! IsZScoreFileCompatible ( isdoc, loadzscorefiles[ 0 ] ) ) {

            StringCopy      ( f, "Incompatible Standardization factors file  '", ToFileName ( loadzscorefiles[ 0 ] ), "'" );
                                        // !NOT a valid file, just for the verbose output and to keep the number of files in sync!
            zscoregofin.Add ( f );
            }
        else            
                                        // we good finally!
            zscoregofin.Add ( loadzscorefiles[ 0 ] );

        } // for gogofpersubject

    } // if BackgroundNormalizationLoadingZScoreFile


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through all subjects, of all sets of groups

gogofallsubjectspreproc .Reset ();
centroidsgof            .Reset ();
TArray1<RegularizationType>     usedregularizations;
RegularizationType              usedregularization;


for ( int absg = 0; absg < gogofpersubject.NumGroups (); absg++ ) {

    Gauge.CurrentPart   = gaugeriscomppreproc;


    CartoolObjects.CartoolApplication->SetMainTitle ( "RIS Computation of", gogofpersubject[ absg ][ 0 ], Gauge );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loading Z-Score Factors?
    actualbacknorm              = backnorm;
    actualsavingzscorefactors   = savingzscorefactors;


    if ( actualbacknorm == BackgroundNormalizationLoadingZScoreFile && ! CanOpenFile ( zscoregofin[ absg ] ) ) {
                                        // if file doesn't exist, switch to computing the Z-Score for this current group
        actualbacknorm              = BackgroundNormalizationComputingZScore;
                                        // also force saving the Z-Score Factors - not sure about that, it shows to the user that something different happened, and the results of the Z-Scoring, too
        actualsavingzscorefactors   = true;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Splitting epochs is done per subject: one subject can have n files of 1 epoch, another subject 1 file of n epochs
    bool                splitepochs     =    CRISPresets[ esicase ].IsEpochs ()
                                                                                    // we can avoid splitting files when not in this case
                                          && computingindividualfiles               
                                                                                    // only 1 file of n epochs to trigger the splitting
//                                        && (    gogofpersubject[ absg ].AllStringsGrep ( InfixEpochsGrep, GrepOptionDefaultFiles )    // !this is testing for file names, NOT markers!
//                                             || gogofpersubject[ absg ].AllStringsGrep ( InfixConcatGrep, GrepOptionDefaultFiles )    // !this is testing for file names, NOT markers!
//                                           );
                                                                                    // allows n files of 1 epoch to mix with 1 file of n epochs and trigger the splitting - but at the expense of duplicating all single epochs files
                                          && gogofpersubject[ absg ].AllStringsGrep ( InfixEpochConcatGrep, GrepOptionDefaultFiles );

    TGoF                gofsplitepochs;


    if ( splitepochs ) {
                                        // !The returned epochs can be any size, as specified by the markers themselves!
                                        // !The Batch Averaging below also seems to not really care, taking the first file size for all epochs!
                                        // ?We could add a fall-back option to split into blocks of known size?
        gogofpersubject[ absg ].SplitByEpochs ( InfixEpochConcatGrep, -1, localfileprefix, gofsplitepochs );

                                        // here we can actually test all epochs are equally long - a bit late in the game, though
//      TracksCompatibleClass       CompatEpochs;
//
//      gofsplitepochs.AllTracksAreCompatible ( CompatEpochs );
//
//      if  ( CompatEpochs.NumTF == CompatibilityNotConsistent ) {
//          ShowMessage ( "Epochs don't seem to have the same number of samples/time range!" NewLine 
//                        "Proceeding anyway, but the averaging will have some problems...", ComputingRisTitle, ShowMessageWarning );
//          }


        if ( gofsplitepochs.IsEmpty () )
            splitepochs = false;
        else                            // !Replacing subjects with split epochs!
            gogofpersubject[ absg ] = gofsplitepochs;

        } // if splitepochs


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Switching inverse matrix?
                                        // Be careful for frequency case where we expanded all freqs into tracks
    if ( matchinginverses && IsMultiple ( absg, stepinverse ) )

        isdoc.Open ( inversefiles[ absg / stepinverse ], OpenDocHidden );
    
    
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                                    // datatypeepochs or datatypefinal
    PreProcessFiles (   gogofpersubject[ absg ],    datatypeproc,
                        NoDualData,                 0,
                        spatialfilter,              xyzfile,                // optional spatial filter
                        true,                       isdoc,              regularization,     &usedregularization,
                        freqtypecomplex,                                    // merge only complex frequency files
                        false /*gfpnormalize*/,
                        actualbacknorm,             zscoremethod,       actualbacknorm == BackgroundNormalizationLoadingZScoreFile ? zscoregofin[ absg ] : 0,
                        dataranking,
                        datathresholding,           datathreshold,
                        envelopetype,               envelopeduration,
                        rois,                       roimethod,              // optional rois, 0 otherwise
                        EpochWholeTime,             0,                  0,
                        NoGfpPeaksDetection,        0,
                        NoSkippingBadEpochs,        0,                  0,
                        basedir,                    localfileprefix,        // base file name / directory, optional file prefix
                        -1,                        -1,                      // no filename clipping
                        false,                      0,                      // no temp dir
                        computingindividualfiles,   risgogof,           0,          baselistpreproc,    newfiles,
                        actualsavingzscorefactors,  &zscoregofout,
                        &Gauge 
                    );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can get rid of these temp EEG split epochs
    if ( splitepochs )

        gofsplitepochs.DeleteFiles ( TracksBuddyExt );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // This group layout contains multiple epochs for multiple conditions, try splitting that & average
    if ( CRISPresets[ esicase ].IsEpochs () ) {
//  if ( splitepochs ) {

        TGoGoF              splitgogof;
        TFileName           meanfile;
        TGoF                mergedgof;
        TStringGrep         grepepoch ( InfixEpochConcatGrep, GrepOptionDefault );  // !handling file names ourselves here!
        TStrings            matched;
        TFileName           newmeanfile;

                // only 1 output TGoF  Default to split by our epoch name 
        risgogof[ 0 ].SplitByNames (  "." InfixEpoch, splitgogof );


        #ifdef ShowGroupsShuffling
//      risgogof[ 0 ].Show ( "All epochs together" );       // tedious output...
//      splitgogof   .Show ( "Epochs split per Conditions" );
        #endif // ShowGroupsShuffling


        for ( int condi = 0; condi < splitgogof.NumGroups (); condi++ ) {

            usedregularizations.ResizeDelta ( 1 );

            usedregularizations[ (int) usedregularizations - 1 ]    = usedregularization;

            if ( savingindividualfiles || computegroupsaverages ) {
                                        // saving only the mean, this is per subject / per condition
                if ( esicase == ComputingRisPresetFreq )    BatchAveragingFreq      (   splitgogof[ condi ],
                                                                                        FreqFFTComplex, PolarityDirect, // not important as long as not FreqFFTApproximation
                                                                                        meanfile,       0,
                                                                                        false,          true    
                                                                                    );
                else {                                                                                           // writing vectorial, as should be
                    if ( IsVector ( datatypeepochs ) )      BatchAveragingVectorial (   splitgogof[ condi ],    
                                                                                        meanfile,       0,              0,
                                                                                        0,              0,              0,
                                                                                        false,          true
                                                                                    );
//                                                                                                               // !force writing vectorial sum as norm!
//                  if ( IsVector ( datatypeepochs ) )      BatchAveragingVectorial (   splitgogof[ condi ],    
//                                                                                      0,              meanfile,       0,
//                                                                                      0,              0,              0,
//                                                                                      false,  true    
//                                                                                  );
//                                                                                                              // !saving either as vectorial or norm - for debugging!
//                  if ( IsVector ( datatypeepochs ) )      BatchAveragingVectorial (   splitgogof[ condi ],
//                                                                                      IsVector ( datatypefinal ) ? (char*) meanfile : (char*) 0, IsVector ( datatypefinal ) ? (char*) 0 : (char*) meanfile,   0,
//                                                                                      0,              0,              0,
//                                                                                      false,          true
//                                                                                  );

                    else                                    BatchAveragingScalar    (   splitgogof[ condi ],
                                                                                        meanfile,       0,              0,
                                                                                        0,              0,
                                                                                        false,          true
                                                                                    );
                    }
                } // batch averaging


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case of centroids from epochs
            if ( computegroupscentroids ) {

                TMaps               mapscentroid;

                ComputeCentroidFiles    (   splitgogof[ condi ],    AllFilesOneCentroid,
                                            centroidsmethod,
                                            centroidsdatatype,
                                            centroidspolarity, 
                                            centroidsref, 
                                            centroidsranking,
                                            centroidsthresholding,  centroidsthreshold,
                                            centroidsnormalized,
                                            SpatialFilterNone,      0, 
                                            mapscentroid,   // we have 1 template per epoch (of the same condition)
                                            false
                                        );
                                        // transfer single centroid
                centroids[ absg ][ condi ]  = mapscentroid[ 0 ];
                } // centroids


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do a bit of file name clean-up
            StringCopy          ( newmeanfile, meanfile );
                                        // look for some remnant like ".Epoch 00." in the FILE NAME ONLY
            if ( grepepoch.Matched  ( ToFileName ( newmeanfile ), &matched ) )
                                        // then remove it
                StringReplace       ( ToFileName ( newmeanfile ), matched ( 0 ), "." );
                                        // files were in a temp directory, so move them up for better visibility
            if ( splitepochs )
                RemoveLastDir       ( newmeanfile );

            if ( StringIsNot ( newmeanfile, meanfile ) ) {

                CheckNoOverwrite    ( newmeanfile );
                                        // and rename
                MoveFileExtended    ( meanfile, newmeanfile );
                }

                                        // remember only the Mean
            mergedgof.Add       ( newmeanfile );
            } // for splitgogof


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Need to reapply these processing after averaging
        if (    ( savingindividualfiles || computegroupsaverages ) 
             && ( dataranking || datathresholding )                )

            ProcessResults  (   mergedgof,          datatypeepochs,     ReferenceNone,
                                dataranking, 
                                datathresholding,   datathreshold, 
                                false /*normalize*/
                            );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( actualsavingzscorefactors ) {
                                        // we need to move 1 directory up the last 2 files (ris and sef)
            const char*         oldzscorefile1  = zscoregofout [ (int) zscoregofout - 2 ];
            TFileName           newzscorefile1  = oldzscorefile1;

            RemoveLastDir       ( newzscorefile1 );
                                        // look for some remnant like ".Epoch 00." in the FILE NAME ONLY
            if ( grepepoch.Matched  ( ToFileName ( newzscorefile1 ), &matched ) )
                                        // then remove it
                StringReplace       ( ToFileName ( newzscorefile1 ), matched ( 0 ), "." );

            CheckNoOverwrite    ( newzscorefile1 );

            MoveFileExtended    ( oldzscorefile1, newzscorefile1 );


            const char*         oldzscorefile2  = zscoregofout [ (int) zscoregofout - 1 ];
            TFileName           newzscorefile2  = oldzscorefile2;

            RemoveLastDir       ( newzscorefile2 );
                                        // look for some remnant like ".Epoch 00." in the FILE NAME ONLY
            if ( grepepoch.Matched  ( ToFileName ( newzscorefile2 ), &matched ) )
                                        // then remove it
                StringReplace       ( ToFileName ( newzscorefile2 ), matched ( 0 ), "." );

            CheckNoOverwrite    ( newzscorefile2 );

            MoveFileExtended    ( oldzscorefile2, newzscorefile2 );

                                        // finally update files list
            zscoregofout.RemoveLast ( 2 );
            zscoregofout.Add    ( newzscorefile1 );
            zscoregofout.Add    ( newzscorefile2 );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( ! savingepochfiles )
                                        // first delete these files
            risgogof.DeleteFiles ( FILEEXT_MRK /*TracksBuddyExt*/ );

                                        // finally trying to remove the directory, only if empty
        TFileName           tempdir ( risgogof[ 0 ][ 0 ] );
        RemoveFilename  ( tempdir );

        if ( IsEmptyDirectory ( tempdir ) )

            risgogof.NukeDirectories ();


//          else
            // saving to a TGoGoF for verbose?


//                                      // Average of vectorial Z-Scored epochs is no longer Z-Score'd
//                                      // At least rescale all tracks so that the global max mode is back to 1?
//      if ( IsVector ( datatypeepochs ) 
//        && actualbacknorm != BackgroundNormalizationNone )
//
//          RescaleZScore (   mergedgof   );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have reduced dimensionality, we can proceed with the regular case
        gogofallsubjectspreproc.Add ( &mergedgof, true, MaxPathShort );

        } // IsEpochs

    else { // ! IsEpochs
                                        // regular case of centroids
        if ( computegroupscentroids ) {

            ComputeCentroidFiles    (   risgogof[ 0 ],      OneFileOneCentroid,
                                        centroidsmethod,
                                        centroidsdatatype,
                                        centroidspolarity, 
                                        centroidsref, 
                                        centroidsranking, 
                                        centroidsthresholding,  centroidsthreshold,
                                        centroidsnormalized,
                                        SpatialFilterNone,  0, 
                                        centroids[ absg ],
                                        false
                                    );
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // saving actual regularizations
        if ( (bool) risgogof ) {

            for ( int absg = 0; absg < risgogof[ 0 ].NumFiles (); absg++ ) {

                usedregularizations.ResizeDelta ( 1 );

                usedregularizations[ (int) usedregularizations - 1 ]    = usedregularization;
                }
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( computegroupsaverages || savingindividualfiles )
                                        // cumulate results, actually only 1 GoF here (no epochs)
                                        // 1 gof = 1 subject, all conditions
            gogofallsubjectspreproc.Add ( risgogof, MaxPathShort );

        else
                                        // !We can get rid of these big files right now!
            risgogof[ 0 ].DeleteFiles ();

        } // ! IsEpochs


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Results of PreProcessFiles are done here
    risgogof.Reset ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
    if ( computegroupscentroids ) {

        StringCopy      ( CentroidsFile,            BaseFileName,       "Centroids" );
        StringAppend    ( CentroidsFile,            ".Subj ",           IntegerToString ( absg + 1, NumIntegerDigits ( numsubjects - 1 ) ) );
        AddExtension    ( CentroidsFile,            FILEEXT_RIS );

        centroidsgof.Add ( CentroidsFile );

                                        // save file early - user can see the current progress, and in case of crash, there will still be intermediate results
        centroids[ absg ].WriteFile ( CentroidsFile, IsVector ( centroidsdatatype ), 0 );

        } // computegroupscentroids
*/
    } // for gof per subject


#ifdef ShowGroupsShuffling
gogofallsubjectspreproc.Show ( "2) Groups Preprocessed per Subject" );
#endif // ShowGroupsShuffling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore original groups organization
Gauge.Next ( gaugeriscompglobal );

                                        // Frequency case:
                                        // results: first GoF   SiC1F1ris, SiC2F1ris
                                        //          next  GoF   SiC1F2ris, SiC2F2ris
                                        //          ...

                                        // 1 gof = 1 condition, all subjects
gogofallsubjectspreproc.SubjectsToConditions ( gogofpercondition );

gogofallsubjectspreproc.Reset ();


#ifdef ShowGroupsShuffling
gogofpercondition.Show ( "3) Groups Reordered per Condition" );
#endif // ShowGroupsShuffling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Revert the frequency splitting, so merge the ris into a big freq file
if ( esicase == ComputingRisPresetFreq ) {

    TGoGoF              rismergedgogof;
    TGoF                gof1subj;
    TFileName           returnfreqfile;

                                        // go through every condition
    for ( int absg = 0; absg < gogofpercondition.NumGroups (); absg++ ) {


        rismergedgogof.Add ( new TGoF );

                                        // gather all freqs from 1 subject
        for ( int fi1 = 0; fi1 < gogofpercondition[ absg ].NumFiles (); fi1 += numexpandedfreqs ) {

            Gauge.Next ( gaugerisfreqgroups );


            gof1subj.Reset ();

            for ( int fi0 = 0, fi2 = fi1; fi0 < numexpandedfreqs; fi0++, fi2++ )
                gof1subj.Add ( gogofpercondition[ absg ][ fi2 ] );

            MergeTracksToFreqFiles ( gof1subj, FreqESI, returnfreqfile, false );

            rismergedgogof[ absg ].Add ( returnfreqfile );
            } // for fi1

        } // for gogofpercondition

                                        // getting rid of the individual, per-frequency, ris files
    gogofpercondition.DeleteFiles ();
                                        // new groups of GoF, finally with the same structure as per the EEG case
    gogofpercondition   = rismergedgogof;


    #ifdef ShowGroupsShuffling
    gogofpercondition.Show ( "3Bis) Frequency Groups Reordered per Condition" );
    #endif // ShowGroupsShuffling

    } // if esicase == ComputingRisPresetFreq
else
    Gauge.FinishPart ( gaugerisfreqgroups );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here gogofpercondition has the final results, for the current set of group(s)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally compute the average per group
gofmean.Reset ();
//  gofsd  .Reset ();


if ( computegroupsaverages ) {

    for ( int absg = 0; absg < gogofpercondition.NumGroups (); absg++ ) {

        Gauge.Next ( gaugeriscompavg );

        TFileName           meanfile;
        TFileName           newmeanfile;


        if ( esicase == ComputingRisPresetFreq )    BatchAveragingFreq      (   gogofpercondition[ absg ],
                                                                                FreqFFTComplex, PolarityDirect, // not important as long as not FreqFFTApproximation
                                                                                meanfile,       0,
                                                                                false,          true
                                                                            );
        else {                          // vectorial results allowed
            if ( IsVector ( datatypefinal ) )       BatchAveragingVectorial (   gogofpercondition[ absg ],
                                                                                meanfile,       0,              0,
                                                                                0,              0,              0,
                                                                                false,          true
                                                                            );
                                        // For RIS: note that the SDm of the Mean is SDm = SD / sqrt ( #files ) - the resulting Mean is therefore not N(1,1/3) but N(1,1/(3*sqrt(#))
                                        // RIS Vectorial will be straightforwardly averaged as Norm
            else                                    BatchAveragingScalar    (   gogofpercondition[ absg ],
                                                                                meanfile,       0,              0,
                                                                                0,              0,
                                                                                false,          true
                                                                            );
            }


        StringCopy          ( newmeanfile,  BaseFileName, InfixGroup " ", IntegerToString ( gofi1 + absg + 1 ), "." InfixMean );
        AddExtension        ( newmeanfile,  ToExtension ( meanfile ) );

        if ( StringIsNot ( newmeanfile, meanfile ) ) {

            CheckNoOverwrite    ( newmeanfile );

            MoveFileExtended    ( meanfile,     newmeanfile );
            }

        gofmean.Add         ( newmeanfile  );
//      gofsd  .Add         ( &sdfile2 );
        }

                                        // Need to reapply these processing after averaging
    if ( dataranking || datathresholding )

        ProcessResults  (   gofmean,            datatypefinal,     ReferenceNone,
                            dataranking, 
                            datathresholding,   datathreshold, 
                            false /*normalize*/
                        );

//                                      // Average of vectorial Z-Scored data is no longer Z-Score'd
//                                      // At least rescale all tracks so that the global max mode is back to 1?
//  if ( IsVector ( datatypefinal ) 
//    && actualbacknorm != BackgroundNormalizationNone )
//
//      RescaleZScore (   gofmean   );

    } // if computegroupsaverages
else
    Gauge.FinishPart ( gaugeriscompavg );


                                        // finally done with these files
if ( ! savingindividualfiles )

    gogofpercondition.DeleteFiles ( TracksBuddyExt );


#ifdef ShowGroupsShuffling
if ( computegroupsaverages )
    gofmean.Show ( "4) Groups Averaged per Conditions" );
#endif // ShowGroupsShuffling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Centroids have been cumulated in the centroids variable - time to save them all on files
if ( computegroupscentroids ) {

    TMaps               centroidspercond ( numsubjects, centroids.GetDimension () );

    for ( int ci = 0; ci < numconditions; ci++ ) {

        for ( int si = 0; si < numsubjects;   si++ )

            centroidspercond[ si ]   = centroids[ si ][ ci ];


        StringCopy          ( CentroidsFile,            BaseFileName,       "Centroids" );
        StringAppend        ( CentroidsFile,            ".Cond ",           IntegerToString ( ci + 1, NumIntegerDigits ( numconditions - 1 ) ) );
        AddExtension        ( CentroidsFile,            FILEEXT_RIS );

        CheckNoOverwrite    ( CentroidsFile );

        centroidsgof.Add    ( CentroidsFile );

                                        // ?Maybe force convert and write to datatypefinal?
        centroidspercond.WriteFile ( CentroidsFile, IsVector ( centroidsdatatype ), 0 );
        } // numsubjects

                                        // complimentary computing the mean of all subjects + putting them in sequence within a single file
//  if ( numsubjects > 1 ) {
                                        // ?In case of Epochs + Centroids, should convert to datatypefinal beforehand? otherwise we could have centroids of different types than average
        TMaps               meancentroids   = centroids.ComputeCentroids ( centroidsmethod, centroidsdatatype, centroidspolarity );

                                        // Need to reapply these processing after averaging (still OK for 1 group, as the threshold factor was computed for this step aynway)
        ProcessResults  (   meancentroids,          centroidsdatatype,      centroidsref,
                            centroidsranking, 
                            centroidsthresholding,  centroidsthreshold, 
                            centroidsnormalized
                        );

        StringCopy          ( CentroidsFile,            BaseFileName,       "Centroids" );
        StringAppend        ( CentroidsFile,            ".Subj Mean" );
        AddExtension        ( CentroidsFile,            FILEEXT_RIS );

        CheckNoOverwrite    ( CentroidsFile );
                                        // shamelessly putting the name in this list for the verbose
        centroidsgof.Add    ( CentroidsFile );

        meancentroids.WriteFile ( CentroidsFile, IsVector ( centroidsdatatype ), 0 );
//      } // numsubjects > 1

    } // computegroupscentroids


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // final verbose output files
{
                                        // showing how the data was grouped?
//verbose.NextLine ( 2 );
//verbose.Put ( "From group:", gofi1 + 1 );
//verbose.Put ( "To   group:", gofi2 + 1 );
//verbose.Put ( "Number of groups of RIS files:", gofi2 - gofi1 + 1 );

verbose.NextLine ();


if ( (bool) zscoregofin ) {

    verbose.NextLine ();

    for ( int fi = 0; fi < (int) zscoregofin; fi++ ) {

        verbose.Put ( fi ? "" : "Reloading standardization factors files:", zscoregofin[ fi ] );
        }

    verbose.NextLine ();
    }


if ( (bool) zscoregofout ) {

    for ( int fi = 0; fi < (int) zscoregofout; fi++ ) {

        verbose.Put ( fi ? "" : "Writing standardization factors files:", zscoregofout[ fi ] );
        }

    verbose.NextLine ();
    }


if ( savingindividualfiles ) {
                                        // 1 gof = 1 condition, all subjects
    for ( int si = 0; si < (int) gogofpercondition; si++ ) {

        verbose.NextLine ();
        verbose.Put ( "Subject #:", si + 1 );
        verbose.Put ( "Number of ris files:", gogofpercondition[ si ].NumFiles () );

        for ( int fi = 0; fi < gogofpercondition[ si ].NumFiles (); fi++ ) {
                                        // file within a given condition
            verbose.Put ( "File:", gogofpercondition[ si ][ fi ] );
                                        // show the exact regularization used for the file
            verbose.Put ( "Regularization:", usedregularizations[ fi * numconditions + si ] );
            }

        }

    verbose.NextLine ();
    } // savingindividualfiles


if ( computegroupsaverages ) {

    verbose.Put ( "Number of average files:", gofmean.NumFiles () );

                                        // showing the mean and SD separately
    for ( int gofi = 0; gofi < (int) gofmean; gofi++ )
        verbose.Put ( gofi == 0 ? "Average file:" : "", gofmean[ gofi ] );

//      for ( int gofi = 0; gofi < (int) gofsd; gofi++ )
//          verbose.Put ( gofi == 0 ? "SD file:" : "", gofsd[ gofi ] );

    verbose.NextLine ();
    } // computegroupsaverages


if ( computegroupscentroids ) {

    for ( int fi = 0; fi < (int) centroidsgof; fi++ ) {

        verbose.Put ( fi ? "" : "Centroids files:", centroidsgof[ fi ] );
        }

    verbose.NextLine ();
    } // computegroupscentroids


verbose.NextLine ();
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( esicase == ComputingRisPresetFreq )
                                        // getting rid of the temporary split freqs
    gogofpersubject.DeleteFiles ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UpdateApplication;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( rois )
    delete  rois;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

WindowMaximize ( CartoolObjects.CartoolMainWindow );

Gauge.FinishParts ();

//CartoolObjects.CartoolApplication->ResetMainTitle ();
CartoolObjects.CartoolApplication->SetMainTitle ( ComputingRisTitle, localfileprefix, Gauge );

Gauge.HappyEnd ();


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
