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
#include    "TMicroStatesSegDialog.h"   // SegPresetsFiles SegPresets

#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "Math.Utils.h"
#include    "TVector.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.TVerboseFile.h"

#include    "CartoolTypes.h"            // EpochsType SkippingEpochsType ResamplingType GfpPeaksDetectType ZScoreType CentroidType
#include    "TExportTracks.h"
#include    "TMaps.h"
#include    "Volumes.TTalairachOracle.h"

//#include    "TRisDoc.h"
#include    "TVolumeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle    Taloracle;

constexpr char*     KMeansSegmentationTitle     = "K-Means Segmentation";
constexpr char*     TAAHCSegmentationTitle      = "T-AAHC Segmentation";


//----------------------------------------------------------------------------
                                        // origgof is used only for verbose purpose, processing is done solely on gogof
bool    TMicroStates::Segmentation  (   TGoF&               gof,                                                // to be processed
                                        const TGoF&         origgof,                                            // the originals before preprocessing
                                        DualDataType        dualdata,           TGoF*                   gofalt, // alternative dataset (RIS or EEG)
                                        AnalysisType        analysis,           ModalityType            modality,           SamplingTimeType    samplingtime,   // last parameter for info only
                                        // following preprocessing parameters are here just for info/verbose - they should have been applied beforehand
                                        EpochsType          epochs,
                                        SkippingEpochsType  badepochs,          const char*             listbadepochs,
                                        GfpPeaksDetectType  gfppeaks,           const char*             listgfppeaks,
                                        ResamplingType      resampling,         int                     numresamples,       int                 resamplingsize,
                                        SpatialFilterType   spatialfilter,      const char*             xyzfile,
//                                      ZScoreType          timezscore,

                                        AtomType            datatype,           PolarityType            polarity,           ReferenceType       dataref,
                                        ClusteringType      clusteringmethod,
                                        int                 reqminclusters,     int                     reqmaxclusters,
                                        int                 numrandomtrials,    CentroidType            centroid,
                                        bool                dolimitcorr,        double                  limitcorr,

                                        bool                sequentialize,
                                        bool                mergecorr,          double                  mergecorrthresh,
                                        bool                smoothing,          int                     smoothinghalfsize,  double              smoothinglambda,
                                        bool                rejectsmall,        int                     rejectsize,
                                        MapOrderingType     mapordering,        const char*             templatesfile,

                                        MicroStatesOutFlags outputflags,
                                        const char*         basefilename,
                                        const char*         outputcommondir,
                                        bool                complimentaryopen
                                    )
{
if ( gof.IsEmpty () || StringIsEmpty ( basefilename ) )
    return  false;

if ( analysis == UnknownAnalysis 
  || modality == UnknownModality
  /*|| samplingtime == UnknownSamplingTime*/ )  // we can go easy on this one, as it is for info only...
    return  false;

if ( clusteringmethod == UnknownClustering
  || reqminclusters > reqmaxclusters       )
    return  false;


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

if ( mapordering == MapOrderingFromTemplates && ! CanOpenFile ( templatesfile ) )
    mapordering     = MapOrderingNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                isrestingstates =  analysis   == AnalysisRestingStatesIndiv
                                    || analysis   == AnalysisRestingStatesGroup;



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Decode output flags
bool                writesegfiles               = IsFlag ( outputflags, WriteSegFiles               );
bool                writetemplatesfiles         = IsFlag ( outputflags, WriteTemplatesFiles         );
bool                writeclustersfiles          = IsFlag ( outputflags, WriteClustersFiles          );
bool                writedispersionfiles        = IsFlag ( outputflags, WriteDispersionFiles        );
bool                writeemptyclusters          = IsFlag ( outputflags, WriteEmptyClusters          );
bool                writenormalizedclusters     = IsFlag ( outputflags, WriteNormalizedClusters     );
bool                writesyntheticfiles         = IsFlag ( outputflags, WriteSyntheticFiles         );

bool                commondir                   = IsFlag ( outputflags, CommonDirectory             );
bool                deleteindivdirs             = IsFlag ( outputflags, DeleteIndivDirectories      );

bool                owningfiles                 = IsFlag ( outputflags, OwningFiles                 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseDirMore;
TFileName           BaseDirPreproc;
TFileName           BaseDirDataCluster;
TFileName           BaseFileName;
TFileName           BaseFileNameMore;
TFileName           BaseFileNameDataCluster;
TFileName           fileoutprefix;
TFileName           buff;


StringCopy      ( BaseDir,                  basefilename                                );
StringCopy      ( BaseDirMore,              BaseDir,            "\\",   DirMore         );
StringCopy      ( BaseDirPreproc,           BaseDir,            "\\",   DirPreProcData  );
StringCopy      ( BaseDirDataCluster,       BaseDir,            "\\",   DirDataClusters );
                                        // should be enough, or checking the whole path, too?
CreatePath      ( BaseDir,          false );
CreatePath      ( BaseDirMore,      false );

                                        // extract string "prefix"
StringCopy      ( fileoutprefix,            ToFileName ( basefilename ) );
                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy      ( BaseFileName,             BaseDir,            "\\",   fileoutprefix );
StringCopy      ( BaseFileNameMore,         BaseDirMore,        "\\",   fileoutprefix );
StringCopy      ( BaseFileNameDataCluster,  BaseDirDataCluster, "\\",   fileoutprefix );


if ( StringLength ( BaseFileName ) > MaxPathShort - 32 ) {

//  ShowMessage (   "File name is too long to generate a correct output..." NewLine 
//                  "Try to either shorten the base file name,"             NewLine 
//                  "or move your output directory higher.", 
//                  SegmentationTitle, ShowMessageWarning );
//
//  Gauge.Finished ();
    return  false;
    }

                                        // delete any previous files, with my prefix only!
StringCopy  ( buff,  BaseFileName,              "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameMore,          "*" );
DeleteFiles ( buff );

StringCopy  ( buff,  BaseFileNameDataCluster,   "*" );
DeleteFiles ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simplified case: only deal with Z-Score data, don't manage dual copies anymore
bool                dataallzscore       = gof.AllStringsGrep ( PostfixStandardizationGrep, GrepOptionDefaultFiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // these 2 go together
if ( dualdata && ! gofalt )

    dualdata    = NoDualData;

                                        // checking a bit of consistency between dual data and main data
if ( gofalt && gofalt->NumFiles () != gof.NumFiles () ) {

    dualdata    = NoDualData;
    gofalt      = 0;
    }


//gof.Show ( "Segmentation: input" );
//if ( gofalt ) gofalt->Show ( "Segmentation: input alt" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Move the given files to our output directory
if ( owningfiles ) {

//  if ( dualgofopen ) {
//                                      // move and update both versions
//      gofzscore   .CopyFilesTo ( BaseDirPreproc, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
//      gofnonzscore.CopyFilesTo ( BaseDirPreproc, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
//                                      // gof has not been updated yet, switch to ZScore version for now
//      gof             = gofzscore;
//      }
//
//  else

                                        // move & rename single group of files, including buddy files, and updates the gof
                                        // ?should we also rename the input files to something more compact?
    gof.CopyFilesTo ( BaseDirPreproc, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );

                                        // also move the alternative RIS version
    if ( gofalt )
        gofalt->CopyFilesTo ( BaseDirPreproc, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );

    } // owningfiles


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                allris      = gof.AllExtensionsAre ( FILEEXT_RIS );
bool                allrisv     = gof.AllExtensionsAre ( FILEEXT_RIS, AtomTypeVector );

                                        // Check ESI parameters
bool                isesipreset     = modality == ModalityESI && allris;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // gauge initialization

                                        // Init gauge
Gauge.Set ( clusteringmethod == ClusteringKMeans ? KMeansSegmentationTitle 
                                                 : TAAHCSegmentationTitle  );


enum                {
                    gaugeseginit,
                    gaugesegglobal,
                    gaugesegcluster,
                    gaugesegsmoothing,
                    gaugesegsequentialize,
                    gaugesegmergecorr,
                    gaugesegrejectsmall,
                    };


Gauge.AddPart ( gaugeseginit,           gof.NumFiles (),    10 );
Gauge.AddPart ( gaugesegglobal,         0,                  05 );
Gauge.AddPart ( gaugesegcluster,        0,                  50 );
Gauge.AddPart ( gaugesegsmoothing,      0,                  10 );
Gauge.AddPart ( gaugesegsequentialize,  0,                  10 );
Gauge.AddPart ( gaugesegmergecorr,      0,                  05 );
Gauge.AddPart ( gaugesegrejectsmall,    0,                  10 );

Gauge.AdjustOccupy ( false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check if we had some preprocessing, the lazy way, by just comparing the strings between origgof and gof
bool                ispreprocessing     = StringIsNot ( origgof[ 0 ], gof[ 0 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.CurrentPart  = gaugeseginit;
                                        // if needed, processing will run with slightly different parameters from the input
                                        // read original data with appropriate data reference: filtering and Z-Score is already done here
ReadData    (   gof,
                datatype, ReferenceNone, 
                CartoolObjects.CartoolApplication->IsInteractive ()
            );
                                        // attention: due to some artefacts, splitting into epochs could end up being empty
                                        // in that case, just leave, there is nothing to segment here!
if ( NumTimeFrames == 0 ) {

//  ShowMessage (   "Current file appears to be empty," NewLine 
//                  "segmentation aborted!", 
//                  gof[ 0 ] /*SegmentationTitle*/, ShowMessageWarning );
//  
//  DeallocateVariables ();

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters for ESI data & templates

                                        // Seems to improve things in the ESI space - ?also done for Grand Clustering?
bool                ranking             = isesipreset;    // was used for ESI RS

                                        // Processing type and reference
ReferenceType       processingref       = GetProcessingRef ( isesipreset ? ProcessingReferenceESI : ProcessingReferenceEEG );


PreprocessMaps  (   Data,
                    false,
                    datatype,       polarity,       dataref,
                    ranking,
                    processingref,
                    true,               // always normalized
                    true                // compute Norm Gfp Dis arrays
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Dual Data is a parallel dataset from the one used for segmentation
                                        // It could be RIS (for EEG) or EEG (for RIS). The MEG case is possible, but it is up to the caller to set the parameters right
TMaps               DualData;
TMaps               DualMaps;
PolarityType        dualpolarity;
AtomType            dualdatatype;
ReferenceType       dualdataref;
bool                dualranking;
CentroidType        dualcentroid;


if      ( dualdata == DualRis ) {

    dualpolarity        = PolarityDirect;   // NO polarity for ESI
    dualdatatype        = DualDataPresets[ dualdata ].DataType;
    dualdataref         = ReferenceNone; // GetProcessingRef ( ProcessingReferenceESI );
    dualranking         = true;
    dualcentroid        = MedianCentroid;

                                        // load & concatenate all files
    DualData.ReadFiles  (   *gofalt, dualdatatype, ReferenceNone );

    PreprocessMaps      (   DualData,
                            false,
                            dualdatatype,       dualpolarity,       ReferenceNone,
                            dualranking,
                            dualdataref,
                            ! dualranking && writenormalizedclusters,
                            false
                        );
    } // if DualRis

else if ( dualdata == DualEeg ) {

    dualpolarity        = isrestingstates ? PolarityEvaluate : PolarityDirect;
    dualdatatype        = DualDataPresets[ dualdata ].DataType;
    dualdataref         = GetProcessingRef ( ProcessingReferenceEEG ); // or ReferenceNone?
    dualranking         = false;            // shouldn't
    dualcentroid        = MedoidCentroid;   // can not average maps from ESI with different vectorial orientations

                                        // load & concatenate all files
    DualData.ReadFiles  (   *gofalt, dualdatatype, ReferenceNone );

    PreprocessMaps      (   DualData,
                            false,
                            dualdatatype,       dualpolarity,       ReferenceNone,
                            dualranking,
                            dualdataref,
                            ! dualranking && writenormalizedclusters,
                            false
                        );
    } // if DualEeg
                                        // !NOT tested!
else if ( dualdata == DualMeg ) {

    dualpolarity        = PolarityDirect;
    dualdatatype        = DualDataPresets[ dualdata ].DataType;
    dualdataref         = ReferenceNone; // GetProcessingRef ( ProcessingRefMEG );
    dualranking         = false;
    dualcentroid        = MeanCentroid;

                                        // load & concatenate all files
    DualData.ReadFiles  (   *gofalt, dualdatatype, ReferenceNone );

    PreprocessMaps      (   DualData,
                            false,
                            dualdatatype,       dualpolarity,       ReferenceNone,
                            dualranking,
                            dualdataref,
                            ! dualranking && writenormalizedclusters,
                            false
                        );
    } // if DualMeg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Dual versions means, we need some data in one form for the discrimination, and data in another form for the output/display/understanding
bool                dualversions    = processingref != dataref;     // f.ex. input is only positive, but processing use average reference, we need to see the results with the original positive way
//                                 || IsZScore ( timezscore )       // template matching is done using Z-Score data, we might need to see the results without the Z-Score
//                                 || dualgofopen;                  // Grand Clustering case, we have both Z-Score and Non-Z-Score data, process with Z-Score and also output with Non-Z-Score

/*                                      // As dualversions currently boils down to processing ref. BEING avg. ref. and data ref. being NOT avg. ref.
                                        // we can do all the job with proc. ref., then save the maps shifted to be all positives
                                        // so that when reloading the templates later, doing the avg. ref. will restore the lost
                                        // centering, while keeping the original normalization used during the template computation
                                        // On the contrary, having dual data, i.e. with no avg. ref., then normalized and saved,
                                        // reloaded as template, with then avg. ref. (and normalized) is NOT the same due to the sequence
                                        // of ref vs normalization.
TMaps               DualData;
TMaps               DualMaps;


if ( dualversions ) {

                                        // dual gof, force using the Non-Z-Score version here !!!!!!! verbose about that !!!!!!!!!!
//  if ( dualgofopen )
//      gof     = gofnonzscore;

                                        // read original data with appropriate data reference
    ReadData    (   gogof, gofi, gofi, -1, -1, 
                    datatype, ReferenceNone,
                    false );

//  or 'DualData.ReadFiles' through a TGoMaps

                                        // save alternative version of data
    DualData        = Data;


    DualData.SetReference ( dataref, datatype );

                                        // normalizing the maps/vectors so we can average the resulting data to generate templates for display
    DualData.Normalize ();

                                        // dual gof, now force using the Z-Score version for the real processing
//  if ( dualgofopen )
//      gof     = gofzscore;

    } // if dualversions
  */

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Volume files and stuff
TFileName           SPFile;
TFileName           GreyFile;
bool                HasEsiFiles     = false;

//  GetFileFromUser     getmrifile ( "MRI Mask:", AllMriFilesFilter,       1, GetFileRead );
//  GetFileFromUser     getspifile ( "SPI Mask:", AllSolPointsFilesFilter, 0, GetFileRead );
//  HasEsiFiles     = getmrifile.Execute () && getspifile.Execute ();


if ( isesipreset ) {
                                        // SPDoc and MRIDocBackg belong to object, they will remain open until it is detroyed
                                        // also, one of these can be missing, but for some processing, we need both
    if ( SPDoc.IsOpen () )
                                        // can use already open doc?
        StringCopy      ( SPFile, SPDoc->GetDocPath () );

                                        // !Find a way to access SP File!
//    else if ( StringIsNotEmpty ( SegTransfer.ISFile ) ) {
//                                        // retrieve SP file from IS file
//        StringCopy      ( SPFile,   SegTransfer.ISFile );
//        RemoveExtension ( SPFile,   2 );    // removing extension and inverse name
//        AddExtension    ( SPFile,   FILEEXT_SPIRR );
//
//        SPDoc.Open      ( SPFile,   /*OpenDocVisible*/ OpenDocHidden );
//
//                                        // try another chance in upper directory?
//        if ( ! SPDoc.IsOpen () ) {
//            RemoveLastDir   ( SPFile );
//            SPDoc.Open      ( SPFile,   /*OpenDocVisible*/ OpenDocHidden );
//            }
//        }


    if ( MRIDocBackg.IsOpen () )
                                        // can use already open doc?
        StringCopy      ( GreyFile, MRIDocBackg->GetDocPath () );

                                        // !Find a way to access MRI File!
//    else if ( StringIsNotEmpty ( SegTransfer.ISFile ) ) {
//                                        // retrieve Grey Mask file from IS file
//        StringCopy      ( GreyFile, SegTransfer.ISFile );
//        RemoveExtension ( GreyFile, 2 );
//        StringAppend    ( GreyFile, "." InfixGrey, "." DefaultMriExt );
//
//        MRIDocBackg.Open( GreyFile, /*OpenDocVisible*/ OpenDocHidden );
//
//                                        // try another chance in upper directory?
//        if ( ! MRIDocBackg.IsOpen () ) {
//            RemoveLastDir       ( GreyFile );
//            MRIDocBackg.Open    ( GreyFile,   /*OpenDocVisible*/ OpenDocHidden );
//            }
//        }

                                        // above opening somehow failed, try to be smart, and scan directory for a set of inverse files
    if ( StringIsEmpty ( SPFile ) || StringIsEmpty ( GreyFile ) ) {

        TFileName           path;
        TFileName           grepvol;
        TFileName           grepspi;
        TGoF                gofvol;
        TGoF                gofspi;

        StringCopy ( path,      BaseDir );
        StringCopy ( grepvol,   AllMriFilesGrep         );
        StringCopy ( grepspi,   AllSolPointsFilesGrep   );


        while ( ! ( SPDoc.IsOpen () && MRIDocBackg.IsOpen () ) && ToLastDirectory ( path ) ) {
                                        // move up one directory at a time
            RemoveFilename      ( path );
                                        // we look for both MRIs and SPIs
            if ( ! gofspi.GrepFiles ( path, grepspi, GrepOptionDefaultFiles ) )     continue;

            if ( ! gofvol.GrepFiles ( path, grepvol, GrepOptionDefaultFiles ) )     continue;
                                        // found 2 groups of files

                                        // loop through all pairs of SPs/Greys that have been found - there can be more than 1 and there can be non ESI spi f.ex.
            for ( int ispi = 0; ispi < (int) gofspi && ! SPDoc.IsOpen ();       ispi++ )
            for ( int imri = 0; imri < (int) gofvol && ! MRIDocBackg.IsOpen (); imri++ ) {

                                        // compare the beginning of the names, which should be synced and identical - this is a high constraint, so we skip opening a lot of files
                if ( StringStartsWith ( gofvol [ imri ], gofspi [ ispi ], StringLength ( gofspi [ ispi ] ) - 4 ) ) {

                                        // found a pair, open SPs first
                    StringCopy          ( SPFile,   gofspi [ ispi ] );
                    SPDoc.Open          ( SPFile,   /*OpenDocVisible*/ OpenDocHidden );

                                        // now, check we have the right dimension!
                    if ( SPDoc->GetNumSolPoints () != NumElectrodes ) {
                        SPDoc.Close ();
                        continue;
                        }

                                        // we good, open Grey!
                    StringCopy          ( GreyFile, gofvol [ imri ] );
                    MRIDocBackg.Open    ( GreyFile, /*OpenDocVisible*/ OpenDocHidden );
                                                // to give a hint we did something behind the hood?
        //          StringCopy          ( SegTransfer.GreyFile, GreyFile );

                    break;
                    }

                } // for gofs

            } // while path

        } // if not spfiles and greyfiles


//  DBGM2 ( SPDoc.IsOpen () ? SPDoc->GetTitle () : "none", MRIDocBackg.IsOpen () ? MRIDocBackg->GetTitle () : "none", "SPDoc & MRIDocBackg" );

                                        // ESI files: SP and MRI Grey
    HasEsiFiles     = MRIDocBackg.IsOpen ()
                   && SPDoc      .IsOpen ();
    } // isesipreset


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we're good here
bool                writevolumes    = HasEsiFiles;

const TBoundingBox<int>*    volsize;
TExportVolume               expvol;
Volume                      templvol;


if ( writevolumes && HasEsiFiles ) {

    SPDoc->BuildInterpolation ( SPInterpolation1NN, MRIDocBackg );

                                    // allocate the rightly sized volume
    templvol        = *MRIDocBackg->GetData ();

                                    // set header
    volsize         = MRIDocBackg->GetSize ();


    expvol.VolumeFormat     = /*IsInsideLimits ( Data.GetMinValue (), Data.GetMaxValue (), (MriType) 0, (MriType) Highest<UCHAR> () ) ? AtomFormatByte : */ AtomFormatFloat;

    expvol.Dim[ 0 ]         = volsize->GetXExtent ();
    expvol.Dim[ 1 ]         = volsize->GetYExtent ();
    expvol.Dim[ 2 ]         = volsize->GetZExtent ();

    expvol.VoxelSize        = MRIDocBackg->GetVoxelSize ();

    expvol.RealSize         = MRIDocBackg->GetRealSize ();

    expvol.Origin           = MRIDocBackg->GetOrigin ();

    expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, MRIDocBackg->GetNiftiTransform () );

    expvol.NiftiIntentCode  = NiftiIntentCodeLabels;

    StringCopy  ( expvol.NiftiIntentName, NiftiIntentNameLabels, NiftiIntentNameSize - 1 );

    //if ( MRIDoc->HasKnownOrientation () ) // always saving orientation?
        MRIDocBackg->OrientationToString ( expvol.Orientation );
    } // if HasEsiFiles


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally expanding the range of clusters

                                        // special case: user asked for a single number of clusters -> this will simplify a few things
bool                computecriteria = reqminclusters != reqmaxclusters;

                                        // minclusters and maxclusters are the actual values used in the main loop
int                 minclusters     = reqminclusters;
int                 maxclusters     = reqmaxclusters;


if ( computecriteria ) {
                                        //   KLFilterSize       : in case of curve filtering
                                        //   CriterionMargin    : for difference computation and other side-effects
    minclusters    -= KLFilterSize + CriterionMargin;
    maxclusters    += KLFilterSize + CriterionMargin;
    }

                                        // can't actually go to NumTimeFrames with k-means (maybe init of maps?)
Clipped ( minclusters, maxclusters, 1, NumTimeFrames - 1 );

                                        // update request range to realistic bounds
Clipped ( reqminclusters, reqmaxclusters, 1, maxclusters - ( computecriteria ? KLFilterSize + CriterionMargin : 0 ) );

                                        // useless now that actual limits are used
//if ( maxclusters > NumTimeFrames ) {
// 
//    ShowMessage (     "Not enough TFs for the requested amount of clusters," NewLine 
//                      "segmentation aborted!", 
//                      SegmentationTitle, ShowMessageWarning );
// 
//    DeallocateVariables ();
//    return  false;
//    }


int                 numclusters     = maxclusters - minclusters + 1;

                                        // maybe time to reload the random generator
RandomUniform.Reload ();
                                        // !If one wants reproducible results, then Reload function must use an absolute seed!
//RandomUniform.Reload ( numclusters );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( writeclustersfiles )
    CreatePath  ( BaseDirDataCluster,       false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have many options at hand to reorder each and every clustering
TArray2<int>        ordering;
TMaps               templatemaps;


if ( mapordering == MapOrderingFromTemplates ) {
                                        // highest priority re-ordering option
    templatemaps.ReadFile   (   templatesfile,   
                                datatype,       ReferenceNone 
                            );

    PreprocessMaps          (   templatemaps,
                                false,
                                datatype,       polarity,       dataref,
                                ranking,
                                processingref,
                                true,
                                false
                            );
    }

                                        // all the following options are being set internally only, for the moment
else if ( mapordering == MapOrderingContextual ) {

    if      ( sequentialize )
                                        // second highest priority re-ordering
        mapordering     = MapOrderingTemporally;

    else if ( polarity   == PolarityEvaluate        // in case we do ERP without sequentialize
           || isrestingstates                   )   // but most likely the RS presets

        mapordering     = isesipreset ? HasEsiFiles ? MapOrderingAnatomically 
                                                    : MapOrderingSolutionPoints     // no anatomy provided, sorting of the poor
                                      :               MapOrderingTopographically;

    else
                                        // currently, it shouldn't land here
        mapordering     = MapOrderingNone;
    } // MapOrderingContextual

                                        // currently relies on already open XyzDoc
if ( mapordering == MapOrderingTopographically ) {

    if ( ! XyzDoc.IsOpen () && StringIsNotEmpty ( xyzfile ) /*&& spatialfilter != SpatialFilterNone*/ )

        XyzDoc.Open ( xyzfile, /*OpenDocVisible*/ OpenDocHidden );

                                        // safety check
    if ( ! XyzDoc.IsOpen () )

        mapordering     = MapOrderingNone;
    }

                                        // now can order with some default w/o these docs
//if ( mapordering == MapOrderingAnatomically && ! ( SPDoc.IsOpen () && MRIDocBackg.IsOpen () ) )
//
//    mapordering     = MapOrderingNone;

//DBGV5 ( SPDoc.IsOpen (), MRIDocBackg.IsOpen (), mapordering, HasEsiFiles, writevolumes, "SPDoc.IsOpen (), MRIDocBackg.IsOpen (), mapordering, HasEsiFiles, writevolumes" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update gauge ranges

Gauge.SetRange ( gaugesegglobal,        numclusters * 3 + 2 );

//Gauge.SetRange ( gaugesegcluster,       numclusters );
Gauge.SetRange ( gaugesegcluster,       clusteringmethod == ClusteringKMeans  ? numclusters * numrandomtrials
                                                          /*ClusteringTAAHC*/ : //  NumTimeFrames  
                                                                                  ( NumTimeFrames * ( NumTimeFrames - 1 ) ) / 2     // size of triangular matrix
                                                                                + ( NumTimeFrames - minclusters + 1 ) 
                                                                                + ( numclusters - 1 ) * ( numclusters - 2 )     );

Gauge.SetRange ( gaugesegsmoothing,     numclusters );
Gauge.SetRange ( gaugesegsequentialize, numclusters );
Gauge.SetRange ( gaugesegmergecorr,     numclusters );
Gauge.SetRange ( gaugesegrejectsmall,   numclusters );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // file names
TFileName           BaseFileNameCluster;
TFileName           BaseFileNameMoreCluster;
TFileName           MainVerbFile;
TFileName           DetailVerbFile;
TFileName           DataFile;
TFileName           DataFileMarker;
//TFileName         LinkFile;
TFileName           SegFile;
TFileName           LandFile;
TFileName           SynthFile;
TFileName           TemplateExtension;
TFileName           OtherFileExtension;
//TFileName         SegAllFile;
TFileName           CopyXyzFile;
TFileName           ZScoreInfix;


                                        // main verbose file
StringCopy  ( MainVerbFile,         BaseFileName,   "." FILEEXT_VRB  );

StringCopy  ( DataFile,             BaseFileName,   "." FILEEXT_ERRORDATA );
StringCopy  ( DataFileMarker,       DataFile,       "." FILEEXT_MRK       );
StringCopy  ( TemplateExtension,    allris ? SegmentationRisTemplateExt : SegmentationEegTemplateExt );
StringCopy  ( OtherFileExtension,   allris ? SegmentationRisFileExt     : SegmentationEegFileExt     );


//bool                separatesubjects        = false; // true;
//if ( commondir && separatesubjects ) {
//
//      StringCopy      ( buff, origgof[ 0 ] );
//      GetFilename     ( buff );
//      StringClip      ( buff, fromchars, tochars );
//                                      // make per subject sub-directories, if we wish to process subjects independently
//      StringAppend    ( outputcommondir, "\\", buff, "\\" );
//      }

                                        // "ZScore" + some optional postfix letter to specifiy if absolute, vectorial...
//StringCopy  ( ZScoreInfix, ".", ZScoreEnumToInfix ( timezscore ) );
//StringCopy  ( ZScoreInfix, "." PostfixStandardizationZScore or CovMat );   // !not sure if still useful! maybe retrieve postfix from input file?


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary copying the xyz file
if ( StringIsNotEmpty ( xyzfile ) )
    CopyFileToDirectory ( xyzfile, BaseDir, CopyXyzFile );
else
    ClearString ( CopyXyzFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a big array of all variables - 1 more slot so we can use indexes starting from 1 (slot 0 is not used)
TArray2<double>     var    ( segnumvar, maxclusters + 1 );  
                                        // which of these variables to output
TSelection          varout ( segnumvar, OrderSorted );


varout.Set ();
                                        // don't output these guys
varout.Reset ( segSumWPD2, segWPdD2 );

//varout.Reset ( segMeanRanks );        // keep showing the mean criterion
varout.Reset ( segHistoArgLocalMax );   // not showing the smoothed version of ArgMax

                                        // reset all variables
var.ResetMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set which criteria will be used, depending on the type of processing
                                        // We manage to have the same criteria for both  ( EEG / ESI ) x ( ERPs / Resting States ) cases
TSelection          critselrank ( segnumvar, OrderSorted );     // criteria used for ranking
TSelection          critselmax  ( segnumvar, OrderSorted );     // criteria used for arg max
TSelection          critsel     ( segnumvar, OrderSorted );     // both criteria merged together

                                        // Best criteria for RankAveraging
                                        // Criteria 2025
critselrank.Set ( segDaviesBouldin              );
critselrank.Set ( segPointBiserial              );
critselrank.Set ( segSilhouettes                );
                                        // maybe too "spiky" to be included in the "mean" criterion(?)
critselrank.Set ( segDaviesBouldinDerivRobust   );
critselrank.Set ( segKrzanowskiLaiC             );
critselrank.Set ( segPointBiserialDerivRobust   );
critselrank.Set ( segSilhouettesDeriv           );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Best criteria for ArgMax;

                                        // Criteria 2025 after pruning out Gamma and Dunn, which always overshoot and don't seem to contribute effectively
                                        // EEG & ESI - 7 Criteria 2025
critselmax.Set ( segDaviesBouldin               );
critselmax.Set ( segDaviesBouldinDerivRobust    );
critselmax.Set ( segKrzanowskiLaiC              );
critselmax.Set ( segPointBiserial               );
critselmax.Set ( segPointBiserialDerivRobust    );
critselmax.Set ( segSilhouettes                 );
critselmax.Set ( segSilhouettesDeriv            );


if ( IsEven ( (int) critselmax ) )
    critselmax.Set ( segMeanRanks );    // force an odd count by adding this guy


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merged set of best criteria
critsel     = critselrank;
critsel    += critselmax;


#if defined(UseAllCriteria)
                                        // bypass previous choices, select all criteria, usually for tests
critsel.Set     ( segCritMin, segCritMax );

#endif

                                        // make a copy of original criteria
TSelection          critselout ( critsel );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Needs a few more variables for the AAHC / T-AAHC methods
TMaps               TAAHCSavedMaps;
TMaps               TAAHCTempMaps;
TLabeling           TAAHCSavedLabels;


if ( clusteringmethod == ClusteringTAAHC ) {

    TAAHCSavedMaps  .Resize ( maxclusters, NumRows );
    TAAHCSavedLabels.Resize ( NumTimeFrames );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file init
TVerboseFile    mainverb ( MainVerbFile, VerboseFileDefaultWidth );

mainverb.PutTitle ( "Segmentation - Main" );
                                        // skipping a few lines so the main verbose and the detailed verboses align together
//mainverb.NextLine ( 7 );
                                        // alias verbose to the main verbose file first
                                        // then, per cluster, to the detailed one, so we can simply copy the verbose code as is
#define         verbose     mainverb


verbose.NextTopic ( "Data Preprocessing:" );
{
verbose.Put ( "Spatial Filter:", SpatialFilterLongName[ spatialfilter ] );
if ( spatialfilter != SpatialFilterNone )
    verbose.Put ( "Electrodes Coordinates file:", xyzfile );
}


verbose.NextTopic ( "Time parameters:" );
{
verbose.Put ( "Input time interval(s):", EpochsNames[ epochs ] );
if ( epochs == EpochsFromList ) {

    int         fromtf;
    int         totf;

    sscanf ( ToExtension ( basefilename ), "%d_%d", &fromtf, &totf );
    verbose.Put ( "Epoch min TF:", fromtf );
    verbose.Put ( "Epoch max TF:", totf );
    }

verbose.NextLine ();
verbose.Put ( "Excluding bad epochs:", SkippingEpochsNames[ badepochs ] );
if ( badepochs == SkippingBadEpochsList )
    verbose.Put ( "Skipping markers:", listbadepochs );

verbose.NextLine ();
verbose.Put ( "Data at GFP Peaks:", GfpPeaksDetectNames[ gfppeaks ] );
if ( gfppeaks == GfpPeaksDetectionList )
    verbose.Put ( "GFP Peaks markers:", listgfppeaks  );

verbose.NextLine ();
verbose.Put ( "Resampling data:", ResamplingNames[ resampling ] );
if ( resampling == TimeResampling ) {

    verbose.Put ( "Number of repetitions:", numresamples );
    verbose.Put ( "Sample size:", resamplingsize );
    }
}


verbose.NextTopic ( "Clustering Method:" );
{
verbose.Put ( "Min number of clusters:", reqminclusters );
verbose.Put ( "Max number of clusters:", reqmaxclusters );


verbose.NextLine ();
if      ( analysis == AnalysisERP                                                           )   verbose.Put ( "Files Preset:", "ERPs Segmentation" );
else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == UnknownSamplingTime     )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals" );
else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == SamplingTimeWhole       )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals, using whole dataset" );
else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == SamplingTimeResampling  )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals, with resampling" );
else if ( analysis == AnalysisRestingStatesGroup && samplingtime == UnknownSamplingTime     )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s)" );
else if ( analysis == AnalysisRestingStatesGroup && samplingtime == SamplingTimeWhole       )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s), using whole dataset" );
else if ( analysis == AnalysisRestingStatesGroup && samplingtime == SamplingTimeResampling  )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s), with resampling" );


verbose.NextLine ();
verbose.Put ( "Clustering method:", ClusteringString[ clusteringmethod ] );

if ( clusteringmethod == ClusteringKMeans ) {
    verbose.Put ( "Requested number of random trials:", numrandomtrials );
    }

verbose.Put ( "Labeling at low Correlations:", ! dolimitcorr );
if ( dolimitcorr )
    verbose.Put ( "No labeling if below Correlation:", limitcorr * 100, 2, " [%]" );


verbose.NextLine ();
if      ( IsScalar   ( datatype ) )     verbose.Put ( "Data type:", "Signed Data" );
else if ( IsPositive ( datatype ) )     verbose.Put ( "Data type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
else if ( IsVector   ( datatype ) )     verbose.Put ( "Data type:", "3D Vectorial Data" );

verbose.Put ( "Data Ranking:", ranking );
verbose.Put ( "Data reference:", ReferenceNames[ dataref ] );
verbose.Put ( "Data Correlation reference:", ReferenceNames[ processingref ] );
verbose.Put ( "Data polarity:", IsPositive ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
}


verbose.NextTopic ( "Templates:" );
{
verbose.Put ( "Templates computation:", CentroidNames[ centroid ], "s of Clusters" );

verbose.NextLine ();
if      ( IsScalar   ( datatype ) )     verbose.Put ( "Templates type:", "Signed Data" );
else if ( IsPositive ( datatype ) )     verbose.Put ( "Templates type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
else if ( IsVector   ( datatype ) )     verbose.Put ( "Templates type:", "3D Vectorial Data" );

verbose.Put ( "Templates Ranking:", ranking );
verbose.Put ( "Templates reference:", ReferenceNames[ dataref ] );
//verbose.Put ( "Templates Correlation reference:", ReferenceNames[ processingref ] );
verbose.Put ( "Templates polarity:", IsPositive ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
}


verbose.NextTopic ( "Temporal Postprocessing:" );
{
verbose.Put ( "Sequentializing:", sequentialize );
verbose.NextLine ();


verbose.Put ( "Merge correlated clusters:", mergecorr );
if ( mergecorr )
    verbose.Put ( "Merge clusters correlated above:", mergecorrthresh );
verbose.NextLine ();


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


verbose.Put ( "Reordering maps:", MapOrderingString[ mapordering ] );
if ( mapordering == MapOrderingFromTemplates )
    verbose.Put ( "Using templates maps:", templatesfile );
}


verbose.NextTopic ( "Alternative Templates:" );
{
verbose.Put ( "Computing alternative set of templates:", DualDataPresets[ dualdata ].Text );

if ( dualdata ) {

    verbose.NextLine ();

    verbose.Put ( "Alternative templates computation:", CentroidNames[ dualcentroid ], "s of Clusters" );

    verbose.NextLine ();

    if      ( IsScalar   ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "Signed Data" );
    else if ( IsPositive ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "Positive Data" );
    else if ( IsVector   ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "3D Vectorial Data" );

    verbose.Put ( "Alternative dataset Ranking:", dualranking );

    verbose.Put ( "Alternative dataset reference:", dualdataref == ReferenceAverage ? "Average Reference" : "As in file" );

    verbose.Put ( "Alternative dataset polarity:", IsPositive ( dualdatatype ) ? "Not relevant" : dualpolarity == PolarityEvaluate ? "Ignore" : "Account" );
    }
}


verbose.NextTopic ( "Input File(s):" );
{
verbose.Put ( "Number of input files:", NumFiles );


if ( ispreprocessing ) {
    verbose.NextLine ();
    for ( int fi = 0; fi < origgof.NumFiles (); fi++ )
        verbose.Put ( "Original File:", origgof[ fi ] );
    }


verbose.NextLine ();
for ( int fi = 0; fi < gof.NumFiles (); fi++ )
//  verbose.Put ( dualgofopen ? "Standardization File:" : "File:", gof[ fi ] );
    verbose.Put ( ispreprocessing ? "Preprocessed File" : "File:", gof[ fi ] );


if ( gofalt ) {
    verbose.NextLine ();
    for ( int fi = 0; fi < gofalt->NumFiles (); fi++ )
        verbose.Put ( "Alternative Dataset File:", (*gofalt)[ fi ] );
    }


if ( SPDoc.IsOpen () ) {
    verbose.NextLine ();
    verbose.Put ( "Solution Points File:", (char *) SPDoc->GetDocPath () );
    }

if ( MRIDocBackg.IsOpen () ) {
    verbose.NextLine ();
    verbose.Put ( "MRI Grey Mask File:", (char *) MRIDocBackg->GetDocPath () );
    }


//if ( dualgofopen ) {
//    verbose.NextLine ();
//    for ( int fi = 0; fi < gof.NumFiles (); fi++ )
//        verbose.Put ( "Non-standardized File:", gofnonzscore[ fi ] );
//    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Main verbose file (this):", MainVerbFile );
verbose.Put ( "Segmentation error file:", DataFile );
if ( StringIsNotEmpty ( CopyXyzFile ) )
    verbose.Put ( "Electrodes Coordinates file:", CopyXyzFile );
verbose.NextLine ();

StringCopy ( buff, BaseFileName, ".*." FILEEXT_VRB );
verbose.Put ( "Detailed verbose files:", buff );


StringCopy ( buff, BaseFileName, ".*.", TemplateExtension );
if ( writetemplatesfiles )
    verbose.Put ( "Templates files:", buff );


StringCopy ( buff, BaseFileName, ".*." FILEEXT_SEG );
if ( writesegfiles )
    verbose.Put ( "Segmentation files:", buff );


StringCopy ( buff, BaseFileNameDataCluster, ".*.Cluster*.", OtherFileExtension );
if ( writeclustersfiles )
    verbose.Put ( "Data clusters files:", buff );


StringCopy ( buff, BaseFileNameMore, ".*.Dispersion.", OtherFileExtension );
if ( writedispersionfiles )
    verbose.Put ( "Dispersion files:", buff );


StringCopy ( buff, BaseFileNameMore, ".*.AllTemplates.", DefaultMriExt );
if ( writevolumes )
    verbose.Put ( "Volume Templates files:", buff );


if ( writesyntheticfiles ) {

    for ( int fi = 0; fi < gof.NumFiles (); fi++ ) {

        StringCopy  ( buff, gof[ fi ] );
        GetFilename ( buff );
        StringCopy  ( SynthFile, BaseFileName, ".*.", buff, ".", OtherFileExtension );

        verbose.Put ( fi == 0 ? "Synthetic EEG files:" : "", SynthFile );
        }
    }

verbose.NextLine ();
verbose.Put ( "Use common best clustering directory:", commondir );
if ( commondir )
    verbose.Put ( "Common best clustering directory:", outputcommondir );

if ( deleteindivdirs )
    verbose.Put ( "Delete individual directories:", deleteindivdirs );
}


verbose.NextTopic ( "Processing Summary:" );
{
(ofstream&) verbose << "Data are processed following the sequence:"                     fastendl;
(ofstream&) verbose << fastendl;
(ofstream&) verbose << "    Repeated for each epoch, or done once on all time frames:"  fastendl;
(ofstream&) verbose << fastendl;
(ofstream&) verbose << "    * Mathematical clustering       (always)"                   fastendl;
(ofstream&) verbose << "    * Sequentialization             (optional)"                 fastendl;
(ofstream&) verbose << "    * Merging correlated templates  (optional)"                 fastendl;
(ofstream&) verbose << "    * Removing low correlations     (optional)"                 fastendl;
(ofstream&) verbose << "    * Temporal smoothing            (optional)"                 fastendl;
(ofstream&) verbose << "    * Rejection of small segments   (optional)"                 fastendl;

verbose.NextLine ();
}


verbose.NextBlock ();
verbose.Flush     ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 nclusters;
int                 nummaps;
double              gev;

TMaps               maps    ( 0, NumRows );     // Create an empty set of maps
TMap                mapt    ( NumRows );
TLabeling           labels  ( NumTimeFrames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // initialize number of clusters, even îf the minclusters is > 1 
for ( int ncl = 1; ncl <= maxclusters; ncl++ )
    var ( segclust, ncl )   = ncl;


                                        // run for each number of clusters
for ( nclusters = minclusters; nclusters <= maxclusters; nclusters++ ) {

//  Gauge.Next ( gaugesegcluster, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );
    Gauge.CurrentPart   = gaugesegcluster;

                                        // offer to quit while processing(?)
//  if ( VkEscape () ) {
//      if ( GetAnswerFromUser ( "Aborting segmentation now?", SegmentationTitle ) ) {
//          DeallocateVariables ();
//          return  false;
//          }
//      }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Running the mathematical clustering, with no time constraints
    if      ( clusteringmethod == ClusteringKMeans )

        nummaps = SegmentKMeans (   nclusters,          maps,               labels,
                                    polarity, 
                                    numrandomtrials,
                                    centroid,
                                    ranking
                                );


    else if ( clusteringmethod == ClusteringTAAHC )

        nummaps = SegmentTAAHC  (   nclusters,          maps,               labels,
                                    polarity,           centroid,
                                    ranking,
                                    TAAHCSavedMaps,     TAAHCSavedLabels,
                                    TAAHCTempMaps,      // !must be deallocated on first call!
                                    maxclusters 
                                );

                                        // !Note: nummaps < nclusters could happen, this is why nummaps is used for (most of) the following processings!
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Post-processings
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegsequentialize, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );
                                        // Splitting up clusters in NON-OVERLAPPING TIME CHUNKS (time was NOT involved at the clustering level)
                                        // Tends to fragment segments into smaller chunks & increase the number of centroids
                                        // It IMPROVES the clustering as we end up with more clusters
    if ( sequentialize ) 
                                        // returns: re-allocated, re-computed & unsorted maps, packed labels, exact number of maps
        nummaps     = SequentializeSegments ( nummaps, maps, labels, polarity, centroid, ranking );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegmergecorr, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );
                                        // Merging clusters which are high-correlated (time is NOT involved)
                                        // Tends to fuse segments into bigger chunks & decrease the number of centroids
                                        // It DEGRADES the clustering because it can bypass the boundaries found by the clustering
                                        // It is doing the opposite of the Sequentialize above, so it might not be a good idea to use both options together!
                                        // To be done BEFORE RejectLowCorrelation, as the new fused clusters can have new outliers
    if ( mergecorr ) 
                                        // returns: re-computed & unsorted maps, packed labels, exact number of maps
        nummaps     = MergeCorrelatedSegments ( nummaps, maps, labels, polarity, centroid, ranking, mergecorrthresh );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  Gauge.Next ( gaugeseglimitcorr, GroupGauge.IsNotAlive () );
                                        // Clipping out the parts that correlate badly to their corresponding centroids
                                        // Tends to fragment segments into smaller chunks (number of centroids should remain constant)
                                        // It IMPROVES the clustering as we end up with less outliers within clusters
    if ( dolimitcorr ) {
                                        // just in case some segments totally disappeared due to bad correlation
                                        // returns: unsorted maps, packed labels, exact number of maps
        nummaps     = RejectLowCorrelation ( 0, NumTimeFrames - 1, maps, labels, limitcorr );

                                        // we have supposedly removed outliers from the data
                                        // re-computing the centroids should benefit to the templates
        maps.Resize ( nummaps, NumRows );   

        maps.LabelingToCentroids    ( 
                                    Data,       &ToData, 
                                    nummaps, 
                                    labels, 
                                    polarity,   centroid,   ranking 
                                    );    

                                        // we have a "gnawed" into the segmentation, maybe a new time sequence emerged?
        if ( sequentialize && ! mergecorr ) 

            nummaps     = SequentializeSegments ( nummaps, maps, labels, polarity, centroid, ranking );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegsmoothing, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );
                                        // It re-labels with a penalty inversely proportional to each segments' durations
                                        // Tends to fuse segments into bigger chunks (number of centroids should remain constant, but could also decrease)
                                        // Centroids are not being modified
    if ( smoothing ) {
                                        // Always done after any correlation clipping, which can fragment big segments
                                        // Smoothing is run per file, to avoid side effects
        SmoothingLabeling   ( 
                            0,                  NumFiles - 1, 
                            0,                  MaxNumTF - 1,
                            nummaps, 
                            maps,               0,
                            labels,
                            smoothinghalfsize,  smoothinglambda,
//                          polarity,           IgnoreCorrelationThreshold  // unlabeled data points can be turned into labeled IF unlabeled data is not majority
                            polarity,           limitcorr                   // unlabeled data points will remain unlabeled if below correlation threshold
                            );

                                        // returns: unsorted maps, packed labels, exact number of maps
        nummaps     = labels.PackLabels ( maps );

                                        // It does not seem necessary to call SequentializeSegments here

                                        // Smoothing is for the labeling only, we don't want to downgrade the templates with a more approximate labeling
//      maps.Resize ( nummaps, NumRows );   
//      maps.LabelingToCentroids ( Data, nummaps, labels, polarity, centroid, ranking );    
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegrejectsmall, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );
                                        // Removes smaller segments (number of centroids should remain constant, but could also decrease)
                                        // Always done after the smoothing
                                        // Centroids are not being modified
    if ( rejectsmall ) {
                                        // returns: recomputed maps, packed labels, exact number of maps
        RejectSmallSegments (
                            0,              NumFiles - 1, 
                            0,              MaxNumTF - 1,
                            maps,           labels,
//                          polarity,       IgnoreCorrelationThreshold,
                            polarity,       limitcorr,
                            rejectsize 
                            );

                                        // re-do it in case the limitcorr has made some crumbles again (silly it)
        if ( dolimitcorr && limitcorr > MinCorrelationThreshold )

            RejectSmallSegments (
                                0,              NumFiles - 1, 
                                0,              MaxNumTF - 1,
                                maps,           labels,
                                polarity,       IgnoreCorrelationThreshold, // always on second run
                                rejectsize 
                                );

                                        // returns: unsorted maps, packed labels, exact number of maps
        nummaps     = labels.PackLabels ( maps );

                                        // we have a removed pieces of the segmentation, maybe a new sequence appeared?
        if ( sequentialize && ! mergecorr ) 

            nummaps     = SequentializeSegments ( nummaps, maps, labels, polarity, centroid, ranking );

                                        // same as per the smoothing case: don't update maps from a degraded labeling
//      maps.Resize ( nummaps, NumRows );   
//      maps.LabelingToCentroids ( Data, nummaps, labels, polarity, centroid, ranking );    
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, labeling & maps are done
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sort maps?
    if ( mapordering != MapOrderingNone ) {

        switch ( mapordering ) {

            case   MapOrderingTemporally:       // by time appearance
                GetTemporalOrdering         ( nummaps, labels, ordering );
                break;


            case   MapOrderingTopographically:  // by orientation - has already checked for XyzDoc is OK
                GetTopographicalOrdering    ( nummaps, maps, XyzDoc, ordering );
                break;


            case   MapOrderingSolutionPoints:   // index of dimension is solution point index
            case   MapOrderingAnatomically:     // with SPI + Grey
               GetAnatomicalOrdering        ( nummaps, maps, SPDoc, MRIDocBackg, ordering );
                break;


            case   MapOrderingFromTemplates:    // following given set of templates
                GetOrderingFromTemplates    ( nummaps, maps, templatemaps, polarity, ordering );
                break;
            }

                                        // apply retrieved ordering
        labels.ReorderLabels ( maps, ordering );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( polarity == PolarityEvaluate )
                                        // if polarities don't matter, reorder the succession of maps to look more consistent visually
        maps.AlignSuccessivePolarities ();  


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Last check / update for the polarity of labeling (RejectStuff can mess up polarity)
    labels.UpdatePolarities ( Data, 0, NumTimeFrames - 1, maps, polarity );

                                        // Finally compute the Global Explained Variance
    gev         = ComputeGEV ( maps, labels, 0, NumTimeFrames - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegglobal, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );

/*  if ( dualversions ) {
                                        // Compute maps with another version of the data, original or with different reference,
                                        // which are different from the version used for template matching (average ref)
                                        // Use the Labeling to re-compute the templates, but with another version of the input data

//      PolarityType        polarity        = PolarityDirect;   // !In case of vectorial data, use PolarityDirect instead if reorientation has already be done on the input data!

        DualMaps.Resize ( nummaps, DualData.GetDimension () );  
                                        // also, don't update polarity
        DualMaps.LabelingToCentroids ( DualData, nummaps, labels, polarity, centroid, avgcleanupalt, orthogonalizealt, false );
        }
*/


    if ( dualdata ) {
                                        // Compute maps with another version of the data, here the ESI
                                        // Use the Labeling to re-compute the templates, but with another version of the input data

        DualMaps.Resize ( nummaps, DualData.GetDimension () );  

                                        // !use alternative parameters, specific for ESI! - also, don't update polarity
        DualMaps.LabelingToCentroids    ( 
                                        DualData,       0, 
                                        nummaps, 
                                        labels, 
                                        dualpolarity,   dualcentroid,   dualranking, 
                                        false 
                                        );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set file name base for current number of clusters/maps

                                        // add current number of clusters to BaseFileName
//  if ( nummaps != nclusters )     sprintf ( buff, ".%02d.(%02d)", nclusters, nummaps );
//  else                            sprintf ( buff, ".%02d",        nclusters );

    sprintf         ( buff, ".%02d.(%02d)", nclusters, nummaps );

    StringCopy      ( BaseFileNameCluster,      BaseFileName,       buff );
    StringCopy      ( BaseFileNameMoreCluster,  BaseFileNameMore,   buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    var ( segclust,     nclusters )     = nclusters;
    var ( segmaps,      nclusters )     = nummaps;
    var ( segGEV,       nclusters )     = gev;


    if ( computecriteria && nummaps > 0 ) {
                                        // Compute all stats at once, also storing the averages of each stat
                                        // Currently Krzanowski-Lai and Davies-Boulding need some per cluster WCD and WPD2, which they compute on their own
        if ( nclusters > 1 )
            ComputeAllWBA ( nclusters, nummaps, maps, labels, polarity, var );

                                        // Note: a few criteria really don't work for 2 clusters, force them to 0

        var ( segCalinskiHarabasz,  nclusters )     = (    critsel[ segCalinskiHarabasz             ] 
                                                        || critsel[ segCalinskiHarabaszDeriv        ] 
                                                        || critsel[ segCalinskiHarabaszDerivRobust  ] ) 
                                                   && nclusters > 1                                     ? ComputeCalinskiHarabasz   ( nummaps )
                                                                                                        : 0;

        var ( segCIndex,            nclusters )     = (    critsel[ segCIndex                       ] 
                                                        || critsel[ segCIndexDeriv                  ] 
                                                        || critsel[ segCIndexDerivRobust            ] )
                                                   && nclusters > 1                                     ? ComputeCIndex             ( nummaps )
                                                                                                        : 0;

        var ( segCrossValidation,   nclusters )     = (    critsel[ segCrossValidation              ] 
                                                        || critsel[ segCrossValidationDeriv         ] 
                                                        || critsel[ segCrossValidationDerivRobust   ] )
                                                   && nclusters > 1                                     ? ComputeCrossValidation    ( nummaps, NumElectrodes, maps, labels, polarity, 0, NumTimeFrames - 1 )
                                                                                                        : 0;

        var ( segDaviesBouldin,     nclusters )     = (    critsel[ segDaviesBouldin                ] 
                                                        || critsel[ segDaviesBouldinDeriv           ] 
                                                        || critsel[ segDaviesBouldinDerivRobust     ] )
                                                   && nclusters > 1                                     ? ComputeDaviesBouldin      ( nummaps, maps, labels, polarity )
                                                                                                        : 0;

        var ( segDunn,              nclusters )     = (    critsel[ segDunn                         ] 
                                                        || critsel[ segDunnDeriv                    ] 
                                                        || critsel[ segDunnDerivRobust              ] ) 
                                                   && nclusters > 1                                     ? ComputeDunn               ( nummaps )
                                                                                                        : 0;

        var ( segDunnRobust,        nclusters )     = (    critsel[ segDunnRobust                   ] 
                                                        || critsel[ segDunnRobustDeriv              ] 
                                                        || critsel[ segDunnRobustDerivRobust        ] ) 
                                                   && nclusters > 1                                     ? ComputeDunnRobust         ( nummaps )
                                                                                                        : 0;

        var ( segSumWPD2,           nclusters )     =      critsel[ segKrzanowskiLai                ]
                                                        || critsel[ segKrzanowskiLaiC               ]
                                                        || critsel[ segKrzanowskiLaiRobust          ]
                                                        || critsel[ segKrzanowskiLaiCRobust         ]   ? ComputeKrzanowskiLaiW     ( nummaps, maps, labels, polarity )
                                                                                                        : 0;
                                                        
                                        // Off because not used and computation needs a big matrix
//      var ( segMarriott,          nclusters )     =     critsel[ segMarriott                      ] 
//                                                 && nclusters > 2                                     ? ComputeMarriott           ( nummaps )
//                                                                                                      : 0;

        var ( segMcClain,           nclusters )     = (    critsel[ segMcClain                      ] 
                                                        || critsel[ segMcClainDeriv                 ] 
                                                        || critsel[ segMcClainDerivRobust           ] )
                                                   && nclusters > 1                                     ? ComputeMcClain            ( nummaps )
                                                                                                        : 0;

        var ( segPointBiserial,     nclusters )     = (    critsel[ segPointBiserial                ] 
                                                        || critsel[ segPointBiserialDeriv           ] 
                                                        || critsel[ segPointBiserialDerivRobust     ] )
                                                   && nclusters > 1                                     ? ComputePointBiserial      ( nummaps,       labels )
                                                                                                        : 0;

        var ( segRatkowski,         nclusters )     = (    critsel[ segRatkowski                    ] 
                                                        || critsel[ segRatkowskiDeriv               ] 
                                                        || critsel[ segRatkowskiDerivRobust         ] )
                                                   && nclusters > 2                                     ? ComputeRatkowski          ( nummaps, maps, labels, polarity )
                                                                                                        : 0;

        var ( segSilhouettes,       nclusters )     = (    critsel[ segSilhouettes                  ] 
                                                        || critsel[ segSilhouettesDeriv             ] 
                                                        || critsel[ segSilhouettesDerivRobust       ] )
                                                   && nclusters > 1                                     ? ComputeSilhouettes        ( nummaps,       labels )
                                                                                                        : 0;

        var ( segTraceW,            nclusters )     =      critsel[ segTraceW                       ]
                                                   && nclusters > 1                                     ? ComputeTraceW             ( nummaps )
                                                                                                        : 0;


        double              Gamma;
        double              GPlusCriterion;
        double              Tau;
                                        // Gamma, G(+) and Tau criteria go together
        if ( critsel[ segGamma              ]
          || critsel[ segGammaDeriv         ]
          || critsel[ segGammaDerivRobust   ]
          || critsel[ segGPlus              ]
          || critsel[ segTau                ] )

            Gamma                                   = nclusters > 1                                     ? ComputeGamma              ( nummaps, GPlusCriterion, Tau )
                                                                                                        : 0;

        var ( segGamma,             nclusters )     = (    critsel[ segGamma                        ] 
                                                        || critsel[ segGammaDeriv                   ]
                                                        || critsel[ segGammaDerivRobust             ] ) 
                                                   && nclusters > 1                                     ? Gamma
                                                                                                        : 0;
        var ( segGPlus,             nclusters )     =      critsel[ segGPlus                        ] 
                                                   && nclusters > 1                                     ? GPlusCriterion
                                                                                                        : 0;
        var ( segTau,               nclusters )     =      critsel[ segTau                          ]
                                                   && nclusters > 1                                     ? Tau
                                                                                                        : 0;


/*                                        // Saving distance files to check the behavior of the criteria
        TFileName           _file;
        TEasyStats          _stats;

        _stats  = StatWCentroidSquareDistance;
        _stats.Sort ();
        StringCopy      ( _file,                 BaseFileNameCluster, ".WCentroidDistances2", ".sef" );
        _stats.WriteFileData ( _file );

        _stats  = StatBCentroidSquareDistance;
        _stats.Sort ();
        StringCopy      ( _file,                 BaseFileNameCluster, ".BCentroidDistances2", ".sef" );
        _stats.WriteFileData ( _file );

        _stats  = StatWPooledDistance;
        _stats.Sort ();
        StringCopy      ( _file,                 BaseFileNameCluster, ".WPooledDistances", ".sef" );
        _stats.WriteFileData ( _file );

        _stats  = StatBPooledDistance;
        _stats.Sort ();
        StringCopy      ( _file,                 BaseFileNameCluster, ".BPooledDistances", ".sef" );
        _stats.WriteFileData ( _file );
*/
        } // nummaps > 0


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Outputs
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugesegglobal, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );

                                        // outside of the requested range?
    if ( ! IsInsideLimits ( nclusters, reqminclusters, reqmaxclusters ) )
        continue;                       // don't output anything, this is for internal use only


    StringCopy      ( LandFile,                 BaseFileNameCluster, ".", TemplateExtension );
    StringCopy      ( SegFile,                  BaseFileNameCluster, "." FILEEXT_SEG  );
    StringCopy      ( DetailVerbFile,           BaseFileNameCluster, "." FILEEXT_VRB  );

                                        // write local verbose to this # of clusters
    TVerboseFile    detailverb ( DetailVerbFile, VerboseFileDefaultWidth );

    detailverb.PutTitle ( "Segmentation - Detailed" );



    detailverb.NextTopic ( "Current Results:" );

    detailverb.Put ( "Number of Clusters:",         nclusters );
    detailverb.Put ( "Number of Maps / Templates:", nummaps   );
    detailverb.Put ( "Global Explained Variance (GEV):", 100 * gev, 4 );


                                        // switch to detail verbose, so we can re-use the main verbose code
    #undef          verbose
    #define         verbose     detailverb


    verbose.NextTopic ( "Data Preprocessing:" );
    {
    verbose.Put ( "Spatial Filter:", SpatialFilterLongName[ spatialfilter ] );
    if ( spatialfilter != SpatialFilterNone )
        verbose.Put ( "Electrodes Coordinates file:", xyzfile );
    }


    verbose.NextTopic ( "Time parameters:" );
    {
    verbose.Put ( "Input time interval(s):", EpochsNames[ epochs ] );
    if ( epochs == EpochsFromList ) {

        int         fromtf;
        int         totf;

        sscanf ( ToExtension ( basefilename ), "%d_%d", &fromtf, &totf );
        verbose.Put ( "Epoch min TF:", fromtf );
        verbose.Put ( "Epoch max TF:", totf );
        }

    verbose.NextLine ();
    verbose.Put ( "Excluding bad epochs:", SkippingEpochsNames[ badepochs ] );
    if ( badepochs == SkippingBadEpochsList )
        verbose.Put ( "Skipping markers:", listbadepochs );

    verbose.NextLine ();
    verbose.Put ( "Data at GFP Peaks:", GfpPeaksDetectNames[ gfppeaks ] );
    if ( gfppeaks == GfpPeaksDetectionList )
        verbose.Put ( "GFP Peaks markers:", listgfppeaks  );

    verbose.NextLine ();
    verbose.Put ( "Resampling data:", ResamplingNames[ resampling ] );
    if ( resampling == TimeResampling ) {

        verbose.Put ( "Number of repetitions:", numresamples );
        verbose.Put ( "Sample size:", resamplingsize );
        }
    }


    verbose.NextTopic ( "Clustering Method:" );
    {
    verbose.Put ( "Min number of clusters:", reqminclusters );
    verbose.Put ( "Max number of clusters:", reqmaxclusters );


    verbose.NextLine ();
    if      ( analysis == AnalysisERP                                                           )   verbose.Put ( "Files Preset:", "ERPs Segmentation" );
    else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == UnknownSamplingTime     )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals" );
    else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == SamplingTimeWhole       )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals, using whole dataset" );
    else if ( analysis == AnalysisRestingStatesIndiv && samplingtime == SamplingTimeResampling  )   verbose.Put ( "Files Preset:", "Resting States, First Stage on Individuals, with resampling" );
    else if ( analysis == AnalysisRestingStatesGroup && samplingtime == UnknownSamplingTime     )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s)" );
    else if ( analysis == AnalysisRestingStatesGroup && samplingtime == SamplingTimeWhole       )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s), using whole dataset" );
    else if ( analysis == AnalysisRestingStatesGroup && samplingtime == SamplingTimeResampling  )   verbose.Put ( "Files Preset:", "Resting States, Second Stage on Group(s), with resampling" );


    verbose.NextLine ();
    verbose.Put ( "Clustering method:", ClusteringString[ clusteringmethod ] );

    if ( clusteringmethod == ClusteringKMeans ) {
        verbose.Put ( "Requested number of random trials:", numrandomtrials );
        }

    verbose.Put ( "Labeling at low Correlations:", ! dolimitcorr );
    if ( dolimitcorr )
        verbose.Put ( "No labeling if below Correlation:", limitcorr * 100, 2, " [%]" );


    verbose.NextLine ();
    if      ( IsScalar   ( datatype ) )     verbose.Put ( "Data type:", "Signed Data" );
    else if ( IsPositive ( datatype ) )     verbose.Put ( "Data type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
    else if ( IsVector   ( datatype ) )     verbose.Put ( "Data type:", "3D Vectorial Data" );

    verbose.Put ( "Data Ranking:", ranking );
    verbose.Put ( "Data reference:", ReferenceNames[ dataref ] );
    verbose.Put ( "Data Correlation reference:", ReferenceNames[ processingref ] );
    verbose.Put ( "Data polarity:", IsPositive ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
    }


    verbose.NextTopic ( "Templates:" );
    {
    verbose.Put ( "Templates computation:", CentroidNames[ centroid ], "s of Clusters" );

    verbose.NextLine ();
    if      ( IsScalar   ( datatype ) )     verbose.Put ( "Templates type:", "Signed Data" );
    else if ( IsPositive ( datatype ) )     verbose.Put ( "Templates type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
    else if ( IsVector   ( datatype ) )     verbose.Put ( "Templates type:", "3D Vectorial Data" );

    verbose.Put ( "Templates Ranking:", ranking );
    verbose.Put ( "Templates reference:", ReferenceNames[ dataref ] );
//  verbose.Put ( "Templates Correlation reference:", ReferenceNames[ processingref ] );
    verbose.Put ( "Templates polarity:", IsPositive ( datatype ) ? "Not relevant" : polarity == PolarityEvaluate ? "Ignore" : "Account" );
    }


    verbose.NextTopic ( "Temporal Postprocessing:" );
    {
    verbose.Put ( "Sequentializing:", sequentialize );
    verbose.NextLine ();


    verbose.Put ( "Merge correlated clusters:", mergecorr );
    if ( mergecorr )
        verbose.Put ( "Merge clusters correlated above:", mergecorrthresh );
    verbose.NextLine ();


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


    verbose.Put ( "Reordering maps:", MapOrderingString[ mapordering ] );
    if ( mapordering == MapOrderingFromTemplates )
        verbose.Put ( "Using templates maps:", templatesfile );
    }


    verbose.NextTopic ( "Alternative Templates:" );
    {
    verbose.Put ( "Computing alternative set of templates:", DualDataPresets[ dualdata ].Text );

    if ( dualdata ) {

        verbose.NextLine ();

        verbose.Put ( "Alternative templates computation:", CentroidNames[ dualcentroid ], "s of Clusters" );

        verbose.NextLine ();

        if      ( IsScalar   ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "Signed Data" );
        else if ( IsPositive ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "Positive Data" );
        else if ( IsVector   ( dualdatatype ) )     verbose.Put ( "Alternative dataset type:", "3D Vectorial Data" );

        verbose.Put ( "Alternative dataset Ranking:", dualranking );

        verbose.Put ( "Alternative dataset reference:", dualdataref == ReferenceAverage ? "Average Reference" : "As in file" );

        verbose.Put ( "Alternative dataset polarity:", IsPositive ( dualdatatype ) ? "Not relevant" : dualpolarity == PolarityEvaluate ? "Ignore" : "Account" );
        }
    }


    verbose.NextTopic ( "Input File(s):" );
    {
    verbose.Put ( "Number of input files:", NumFiles );


    if ( ispreprocessing ) {
        verbose.NextLine ();
        for ( int fi = 0; fi < origgof.NumFiles (); fi++ )
            verbose.Put ( "Original File:", origgof[ fi ] );
        }


    verbose.NextLine ();
    for ( int fi = 0; fi < gof.NumFiles (); fi++ )
    //  verbose.Put ( dualgofopen ? "Standardization File:" : "File:", gof[ fi ] );
        verbose.Put ( ispreprocessing ? "Preprocessed File" : "File:", gof[ fi ] );


    if ( gofalt ) {
        verbose.NextLine ();
        for ( int fi = 0; fi < gofalt->NumFiles (); fi++ )
            verbose.Put ( "Alternative Dataset File:", (*gofalt)[ fi ] );
        }


    if ( SPDoc.IsOpen () ) {
        verbose.NextLine ();
        verbose.Put ( "Solution Points File:", (char *) SPDoc->GetDocPath () );
        }

    if ( MRIDocBackg.IsOpen () ) {
        verbose.NextLine ();
        verbose.Put ( "MRI Grey Mask File:", (char *) MRIDocBackg->GetDocPath () );
        }


    //if ( dualgofopen ) {
    //    verbose.NextLine ();
    //    for ( int fi = 0; fi < gof.NumFiles (); fi++ )
    //        verbose.Put ( "Non-standardized File:", gofnonzscore[ fi ] );
    //    }
    }

                                        // this part is slightly different, with actual output file names
    verbose.NextTopic ( "Output Files:" );
    {
    verbose.Put ( "Main verbose file (this):", MainVerbFile );
    verbose.Put ( "Segmentation error file:", DataFile );
    if ( StringIsNotEmpty ( CopyXyzFile ) )
        verbose.Put ( "Electrodes Coordinates file:", CopyXyzFile );
    verbose.NextLine ();


    detailverb.Put ( "Detailed verbose file (this):", DetailVerbFile );


    if ( writetemplatesfiles )
        detailverb.Put ( "Templates file:", LandFile );


    if ( writesegfiles )
        detailverb.Put ( "Segmentation file:", SegFile );


    StringCopy ( buff, BaseFileNameDataCluster, ".*.Cluster*", OtherFileExtension );
    if ( writeclustersfiles )
        detailverb.Put ( "Data clusters files:", buff );


    StringCopy  ( buff, BaseFileNameMoreCluster, "." "AllTemplates", "." DefaultMriExt );
    if ( writevolumes )
        detailverb.Put ( "Volume Template file:", buff );


    if ( writesyntheticfiles ) {

        bool    first = true;

        for ( int fi = 0; fi < gof.NumFiles (); fi++ ) {

            StringCopy  ( buff, gof[ fi ] );
            GetFilename ( buff );
            StringCopy  ( SynthFile, BaseFileNameMoreCluster, ".", buff, ".", OtherFileExtension );

            detailverb.Put ( first ? "Synthetic EEG file(s):" : "", SynthFile );

            first = false;
            }
        }


    verbose.NextLine ();
    verbose.Put ( "Use common best clustering directory:", commondir );
    if ( commondir )
        verbose.Put ( "Common best clustering directory:", outputcommondir );

    if ( deleteindivdirs )
        verbose.Put ( "Delete individual directories:", deleteindivdirs );
    }


    verbose.NextTopic ( "Processing Summary:" );
    {
    (ofstream&) verbose << "Data are processed following the sequence:"                     fastendl;
    (ofstream&) verbose << fastendl;
    (ofstream&) verbose << "    Repeated for each epoch, or done once on all time frames:"  fastendl;
    (ofstream&) verbose << fastendl;
    (ofstream&) verbose << "    * Mathematical clustering       (always)"                   fastendl;
    (ofstream&) verbose << "    * Sequentialization             (optional)"                 fastendl;
    (ofstream&) verbose << "    * Merging correlated templates  (optional)"                 fastendl;
    (ofstream&) verbose << "    * Removing low correlations     (optional)"                 fastendl;
    (ofstream&) verbose << "    * Temporal smoothing            (optional)"                 fastendl;
    (ofstream&) verbose << "    * Rejection of small segments   (optional)"                 fastendl;

    verbose.NextLine ();
    }


    verbose.NextBlock ();

                                        // we're done with the shared verbose parts
    #undef  verbose

                                        // from here, we are all detailed verbose
    detailverb.NextTopic ( "Detailed Results:" );
    detailverb.NextLine ();
    (ofstream&) detailverb << StreamFormatFixed;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    detailverb.NextTopic ( "Correlation between all Templates of all files:" );
    (ofstream&) detailverb << "(multiplied by 100; -100 = opposite, 100 = identical)" fastendl fastendl;

    (ofstream&) detailverb << StreamFormatFixed;


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "Map    " /*"Map"*/ );
    for ( int nc = 1; nc <= nummaps; nc++ ) {
        sprintf ( buff, "%0d", nc );
        detailverb.TableColNames.Add ( buff );
        detailverb.TableRowNames.Add ( buff );
        }


    detailverb.BeginTable ( 8 /*5*/ );

                                        // !Now more a projection than a correlation!
    for ( int nc1 = 0; nc1 < nummaps; nc1++ )
    for ( int nc2 = 0; nc2 < nummaps; nc2++ )

        detailverb.PutTable ( Round ( 100 * Project ( maps[ nc1 ], maps[ nc2 ], polarity ) ) );
//      detailverb.PutTable ( Round ( 100 * maps[ nc1 ].Correlation ( maps[ nc2 ], polarity, false ) ) );


    detailverb.EndTable ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Correlation across clusters is also interesting
    TGoMaps             cloudsmaps ( nummaps, 0, maps.GetDimension () );
                                        // spread data into clouds per clusters (no unlabeled cluster right now)
    for ( int nc = 0; nc < nummaps; nc++ ) 
    for ( long tf = 0; tf < NumTimeFrames; tf++ )
        if ( labels[ tf ] == nc )
            cloudsmaps[ nc ].Add ( Data[ tf ] );


    detailverb.NextTopic ( "Correlations between all Clusters of all files:" );
    (ofstream&) detailverb << "(multiplied by 100; -100 = opposite, 100 = identical)" fastendl fastendl;

    (ofstream&) detailverb << StreamFormatFixed;


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "Cluster" );
    for ( int nc = 1; nc <= nummaps; nc++ ) {
        sprintf ( buff, "%0d", nc );
        detailverb.TableColNames.Add ( buff );
        detailverb.TableRowNames.Add ( buff );
        }


    detailverb.BeginTable ( 8 /*5*/ );

                                        // !Now more a projection than a correlation!
    for ( int nc1 = 0; nc1 < nummaps; nc1++ ) 
    for ( int nc2 = 0; nc2 < nummaps; nc2++ )

        detailverb.PutTable ( Round ( 100 * Project ( cloudsmaps[ nc1 ], cloudsmaps[ nc2 ], polarity ) ) );


    detailverb.EndTable ();

    detailverb.NextLine ();

    cloudsmaps.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    detailverb.NextTopic ( "Clusters Spreading:" );
    detailverb.NextLine ();


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "Map" );
    for ( int nc = 1; nc <= nummaps; nc++ ) {
        sprintf ( buff, "Seg%0d", nc );
        detailverb.TableColNames.Add ( buff );
        }

//  detailverb.TableRowNames.Add ( "SD of Distances to Centroid" );
//  detailverb.TableRowNames.Add ( "Median of Distances to Centroid" );
//  detailverb.TableRowNames.Add ( "Median of Pooled Distances" );
    detailverb.TableRowNames.Add ( "Mean Dispersion" );


    detailverb.BeginTable ( 10 );

                                        // !upgrade and put into ComputeClustersDispersion!
    TVector<double>     dispersion ( nummaps );

    ComputeClustersDispersion ( nummaps, maps, labels, polarity, dispersion, true );

    for ( int nc = 0; nc < dispersion.GetDim () ; nc++ )
        detailverb.PutTable ( dispersion ( nc ), 3 );


    detailverb.EndTable ();

    detailverb.NextLine ();

                                        // complimentary saving dispersions to file, for better visualization
    if ( writedispersionfiles ) {
        StringCopy  ( buff, BaseFileNameMoreCluster, ".Dispersion.sef" );
        dispersion.WriteFile ( buff, "Dispersion" );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // This takes a lot of space - is it really helpful?
    TSelection          mapsel  ( nummaps, OrderSorted );

                                        // correlation table per file
    detailverb.NextTopic ( "Correlation between all Maps / Templates of a single file:" );


    for ( int i = 0; i < NumFiles; i++ ) {

        detailverb.NextLine ();
        detailverb.Put ( "File:", gof[ i ] );
//      (ofstream&) detailverb << "        " << gof[ i ] << fastendl fastendl;
//      (ofstream&) detailverb << "(multiplied by 100; -100 = opposite, 100 = identical)" fastendl fastendl;
        detailverb.NextLine ();

        mapsel.Reset();

        for ( long tf = IntervalTF[ i ].GetLimitMin (); tf <= IntervalTF[ i ].GetLimitMax (); tf++ )
            if ( labels.IsDefined ( tf ) )
                mapsel.Set ( labels[ tf ] );


        detailverb.ResetTable ();

        detailverb.TableColNames.Add ( "Map" );
        for ( TIteratorSelectedForward nc1 ( mapsel ); (bool) nc1; ++nc1 ) {
            sprintf ( buff, "%0d", nc1() + 1 );
            detailverb.TableColNames.Add ( buff );
            detailverb.TableRowNames.Add ( buff );
            }


        detailverb.BeginTable ( 5 );


        for ( TIteratorSelectedForward nc1 ( mapsel ); (bool) nc1; ++nc1 )
        for ( TIteratorSelectedForward nc2 ( mapsel ); (bool) nc2; ++nc2 ) {

            corr    = maps[ nc1() ].Correlation ( maps[ nc2() ], polarity );
            detailverb.PutTable ( 100 * corr, 0 );
            }


        detailverb.EndTable ();

        detailverb.NextLine ();
        }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    detailverb.NextTopic ( "Sorted correlations between all Maps / Templates:" );
//  (ofstream&) detailverb << "(multiplied by 100; -100 = opposite, 100 = identical)" fastendl fastendl;
    detailverb.NextLine ();

                                        // compute all possible correlations between all maps
    TArray2<float>      allcorr ( ( nummaps * ( nummaps - 1 ) ) / 2, 3 );
    int                 numallcor       = 0;

    for ( int nc1 = 0; nc1 < nummaps; nc1++ )
    for ( int nc2 = nc1 + 1; nc2 < nummaps; nc2++ ) {

        allcorr ( numallcor, 0 )    = nc1;
        allcorr ( numallcor, 1 )    = nc2;
        allcorr ( numallcor, 2 )    = Project ( maps[ nc1 ], maps[ nc2 ], polarity );
        numallcor++;
        }

                                        // sort by correlations
    allcorr.SortRows ( 2, Descending );


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( ""     );    // !empty, we don't want a row title!
    detailverb.TableColNames.Add ( "Map1" );
    detailverb.TableColNames.Add ( "Map2" );
    detailverb.TableColNames.Add ( "Corr" );
        

    detailverb.BeginTable ( 10 );


    for ( int i = 0; i < numallcor; i++ ) {
        detailverb.PutTable ( (int) allcorr ( i, 0 ) + 1 );  // !index offset!
        detailverb.PutTable ( (int) allcorr ( i, 1 ) + 1 );
        detailverb.PutTable ( 100 * allcorr ( i, 2 ), 2 );
        }


    detailverb.EndTable ();

    detailverb.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute and save once for all the correlations between the segmentation and template
    TArray1<double>     correlation ( NumTimeFrames );
//  TArray1<double>     projection  ( NumTimeFrames );

    correlation     = 0;
//  projection      = 0;

    //OmpParallelFor

    for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

        if ( labels.IsDefined ( tf ) ) {

            correlation[ tf ]   = maps[ labels[ tf ] ].Correlation ( Data[ tf ], labels.GetPolarity ( tf ), false /*processingref == ReferenceAverage*/ );

//          projection [ tf ]   = Project ( maps[ labels[ tf ] ], Data[ tf ], labels.GetPolarity ( tf ) );
            }
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Remember: files have been cropped to any specified epoch during preprocessing, so NumTimeFrames IS the ACTUAL epoch duration
                                        // Index codes: 0=unlabeled, 1..nummaps=map index based 1, nummaps+1=total
    TArray1<int>        tfpercluster ( nummaps + 2 );


    for ( long tf = 0; tf < NumTimeFrames; tf++ )
        if ( labels.IsDefined ( tf ) )  tfpercluster[ labels[ tf ] + 1 ]++;
        else                            tfpercluster[ 0                ]++;
    tfpercluster[ nummaps + 1 ] = NumTimeFrames;


    detailverb.NextBlock ();
    detailverb.NextTopic ( "Time per segment:" );
    (ofstream&) detailverb << StreamFormatGeneral << StreamFormatLeft;
    detailverb.NextLine ( 2 );


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "Time"   );
    detailverb.TableColNames.Add ( "Unlab." );
    for ( int nc = 1; nc <= nummaps; nc++ ) {
        sprintf ( buff, "Seg%0d", nc );
        detailverb.TableColNames.Add ( buff );
        }
    detailverb.TableColNames.Add ( "All" );


    detailverb.TableRowNames.Add ( "Abs" );
    detailverb.TableRowNames.Add ( "%"   );


    detailverb.BeginTable ( 8 );


    for ( int nc = 0; nc < tfpercluster.GetDim1 () ; nc++ )
        detailverb.PutTable ( tfpercluster[ nc ] );


    for ( int nc = 0; nc < tfpercluster.GetDim1 (); nc++ )
        detailverb.PutTable ( 100 * (double) tfpercluster[ nc ] / NumTimeFrames, 1 );


    detailverb.EndTable ();

    detailverb.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute the relative contribution to the GEV for each cluster, using only the numerator part (we know the total GEV already)
    TVector<double>         gevpercluster;

    ComputeGevPerCluster    (   nummaps /*nclusters*/,  
                                maps,  
                                labels,  
                                gevpercluster
                            );


//  detailverb.NextBlock ();
    detailverb.NextTopic ( "Global Explained Variance per segment:" );
    (ofstream&) detailverb << StreamFormatGeneral << StreamFormatLeft;
    detailverb.NextLine ( 2 );


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "GEV" );
    for ( int nc = 1; nc <= nummaps; nc++ ) {
        sprintf ( buff, "Seg%0d", nc );
        detailverb.TableColNames.Add ( buff );
        }
    detailverb.TableColNames.Add ( "All" );


    detailverb.TableRowNames.Add ( "Abs" );
    detailverb.TableRowNames.Add ( "%"   );


    detailverb.BeginTable ( 8 );


    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( 100 * gevpercluster[ nc ], 1 );
    detailverb.PutTable ( 100 * gev, 1 );


    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( 100 * gevpercluster[ nc ] / NonNull ( gevpercluster[ nummaps ] ), 1 );    // relative share
    detailverb.PutTable ( 100, 1 );


    detailverb.EndTable ();

    detailverb.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    (ofstream&) detailverb << StreamFormatGeneral << StreamFormatLeft;


    detailverb.NextBlock ();
    detailverb.NextTopic ( "Number of Maps per file:" );
    detailverb.NextLine ( 2 );


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "File" );
    for ( int nc = 0; nc < nummaps; nc++ ) {
        sprintf ( buff, "Map%0d", nc + 1 );
        detailverb.TableColNames.Add ( buff );
        }


    for ( int fi = 0; fi < gof.NumFiles (); fi++ ) {
//      sprintf     ( buff, "File%0d", fi + 1 );

        StringCopy  ( buff, gof[ fi ] );
        GetFilename ( buff );
        detailverb.TableRowNames.Add ( buff );
        }
    detailverb.TableRowNames.Add ( "Total Maps Count"  );
    detailverb.TableRowNames.Add ( "Total Maps %"      );
    detailverb.TableRowNames.Add ( "Total Files Count" );
    detailverb.TableRowNames.Add ( "Total Files %"     );


    detailverb.BeginTable ( 8 );


    TVector<int>        mapcount    ( nummaps );
    TVector<int>        totalsum    ( nummaps );
    TVector<int>        totalexist  ( nummaps );


    for ( int i = 0; i < NumFiles; i++ ) {

        mapcount.ResetMemory ();

        for ( int tf0 = 0, tf = OffsetTF[ i ] + tf0; tf0 < NumTF[ i ];    tf0++, tf++ )
            if ( labels.IsDefined ( tf ) )
                mapcount ( labels[ tf ] )++;

                                        // cumulate all contributions across all files - a file can contribute more than once
        totalsum   += mapcount;

                                        // cumulate existing contributions only - a file can contribute only once
        for ( int nc = 0; nc < nummaps; nc++ )
            if ( mapcount ( nc ) )
                totalexist  ( nc )++;


        for ( int nc = 0; nc < nummaps; nc++ )
            detailverb.PutTable ( mapcount ( nc ) );
        } // for file

                                        // A few more rows
    detailverb.NextLine ();

    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( totalsum ( nc ) );

    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( Round ( (double) totalsum ( nc ) / NumTimeFrames * 100 ) );


    detailverb.NextLine ();

    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( totalexist ( nc ) );

    for ( int nc = 0; nc < nummaps; nc++ )
        detailverb.PutTable ( Round ( (double) totalexist ( nc ) / NumFiles * 100 ) );


    detailverb.EndTable ();

    detailverb.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    (ofstream&) detailverb << StreamFormatFixed << StreamFormatLeft;


    detailverb.NextBlock ();
    detailverb.NextTopic ( "Labeled vs Unlabeled TFs:" );


    int                 numundef        = 0;
    int                 numdef;

    for ( long tf = 0; tf < NumTimeFrames; tf++ )
        if ( labels.IsUndefined ( tf ) )
            numundef++;

    numdef      = NumTimeFrames - numundef;


    detailverb.NextLine ();
    detailverb.Put ( "Force minimum Correlation:", dolimitcorr );
    if ( dolimitcorr )
        detailverb.Put ( "Min Correlation allowed for Labeling:", limitcorr * 100, 2, " [%]" );

    detailverb.NextLine ();
    detailverb.Put ( "Total number of TFs      :", NumTimeFrames );
    detailverb.Put ( "Number of     labeled TFs:", numdef );
    detailverb.Put ( "Number of non-labeled TFs:", numundef );

    detailverb.NextLine ();
    detailverb.Put ( "Percentage of     labeled TFs:", numdef   / (double) NumTimeFrames * 100, 2, " %" );
    detailverb.Put ( "Percentage of non-labeled TFs:", numundef / (double) NumTimeFrames * 100, 2, " %" );

    detailverb.NextLine ();


    (ofstream&) detailverb << StreamFormatFixed;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save the centroids / templates maps
    Gauge.Next ( gaugesegglobal, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );


    if ( writetemplatesfiles ) {
                                        // ref is different
//      TMaps&              m           = dualversions ? DualMaps : maps;
        TMaps&              m           = maps;
        double              offset      = dualversions ? - NoMore ( (TMapAtomType) 0, maps.GetMinValue () ) : 0;


        TExportTracks       expmaps;

        StringCopy ( expmaps.Filename, LandFile );

        if ( /*dualversions || IsZScore ( timezscore ) || dualgofopen*/ dataallzscore ) // dualversions is also about different reference
            PostfixFilename ( expmaps.Filename, ZScoreInfix );
//      else
//          PostfixFilename ( expmaps.Filename, "." PostfixOriginalData );              // not anymore, we don't output the proc. ref. version, so we don't need to differentiate them

        expmaps.NumTime           = nummaps;
        expmaps.NumTracks         = NumElectrodes;
        expmaps.SetAtomType ( IsVector ( datatype ) ? AtomTypeVector : AtomTypeScalar );// for ris output


        for ( int nc = 0; nc < nummaps; nc++ )
        for ( int e = 0; e < NumElectrodes; e++ )
            if ( IsVector ( datatype ) )
                expmaps.Write ( TVector3Float ( m[ nc ][ 3 * e     ],
                                                m[ nc ][ 3 * e + 1 ],
                                                m[ nc ][ 3 * e + 2 ] ) );
            else
                expmaps.Write ( (float)         m[ nc ][ e         ] + offset );

        } // writetemplatesfiles


    if ( writetemplatesfiles && dualdata ) {
                                        // ref is different
        TMaps&              m           = DualMaps;


        TExportTracks       expmaps;

        StringCopy          ( expmaps.Filename, LandFile );
        ReplaceExtension    ( expmaps.Filename, dualdata == DualRis ? SegmentationRisTemplateExt : SegmentationEegTemplateExt );

        expmaps.NumTime           = nummaps;
        expmaps.NumTracks         = DualMaps.GetDimension ();
        expmaps.SetAtomType ( dualdatatype );

                                        // !doesn't write Vectorial version!
        for ( int nc = 0; nc < nummaps; nc++ )
        for ( int e = 0; e < expmaps.NumTracks; e++ )
            expmaps.Write ( (float)         m[ nc ][ e         ] );

        } // writetemplatesfiles dual


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // save segments as markers
    tforg       = 0;
    curreeg     = 0;
    int     lastlabel   = UndefinedLabel;
    bool    starteeg    = true;

    for ( long tf = 0; tf < NumTimeFrames; tf++ ) {
        if ( tf - tforg == NumTF[ curreeg ] ) {
            tforg  += NumTF[ curreeg ];
            curreeg++;
            starteeg = true;
            }

        if ( labels[ tf ] != lastlabel || starteeg ) {
            lastlabel = labels[ tf ];

            sprintf ( buff, "S%0d", lastlabel + 1 );
            GDoc->GetEegDoc( curreeg )->InsertMarker ( TMarker ( AbsTFToRelTF[ tf ], AbsTFToRelTF[ tf ], (MarkerCode) (lastlabel + 1), buff, MarkerTypeTrigger ) );
            }

        starteeg = false;
        }

    GDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save synthetic EEG, where data is replaced by its corresponding template + modulated in power
    if ( writesyntheticfiles ) {
                                        // ref is different
//      TMaps&              m           = dualversions ? DualMaps : maps;
        TMaps&              m           = maps;
        double              offset      = dualversions ? - NoMore ( (float) 0, maps.GetMinValue () ) : 0;
        double              globalscale = Norm.GetMeanValue (); // synth file are text files, magnetometer values can be very small and don't fit into the fixed space allocated
        double              s;
        LabelType           l;


        TExportTracks       expsynth;

        expsynth.NumTracks          = NumElectrodes;
        expsynth.SamplingFrequency  = SamplingFrequency;
        expsynth.SetAtomType ( IsVector ( datatype ) ? AtomTypeVector : AtomTypeScalar );   // for ris output


        for ( int i = 0; i < NumFiles; i++ ) {

            StringCopy  ( buff, gof[ i ] );
            GetFilename ( buff );
            StringCopy  ( expsynth.Filename, BaseFileNameMoreCluster, ".", buff, ".", OtherFileExtension );

//          if ( /*dualversions || IsZScore ( timezscore ) || */ dataallzscore )        // dualversions is also about different reference
//              PostfixFilename ( expsynth.Filename, "." PostfixStandardizationNone );  // !ZScore is already included in name!
//          else
//              PostfixFilename ( expsynth.Filename, "." PostfixOriginalData );         // not anymore, we don't output the proc. ref. version, so we don't need to differentiate them

            expsynth.NumTime        = NumTF[ i ];


            for ( int tf0 = 0, tf = OffsetTF[ i ] + tf0; tf0 < NumTF[ i ];    tf0++, tf++ )
            for ( int e   = 0;                           e   < NumElectrodes; e++         )

                if ( labels.IsDefined ( tf ) ) {

                    l       = labels[ tf ];
                    s       = Norm[ tf ] / NonNull ( globalscale );

                                        // rescale each template with actual norm of map, for a better visual result
                    if ( IsVector ( datatype ) )
                        expsynth.Write ( TVector3Float (   m[ l ][ 3 * e     ]    * s,
                                                           m[ l ][ 3 * e + 1 ]    * s,
                                                           m[ l ][ 3 * e + 2 ]    * s ) );
                    else
                        expsynth.Write ( (float)       ( ( m[ l ][ e ] + offset ) * s ) );
                    }
                else {
                    if ( IsVector ( datatype ) )
                        expsynth.Write ( TVector3Float ( 0,
                                                         0,
                                                         0 ) );
                    else
                        expsynth.Write ( (float)         0 );
                    }
            } // for file

        } // writesyntheticfiles


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    detailverb.NextBlock ();
    detailverb.NextTopic ( "Labeling:" );

    (ofstream&) detailverb << "(Corr multiplied by 100)" fastendl fastendl;


    detailverb.ResetTable ();

    detailverb.TableColNames.Add ( "" );
    detailverb.TableColNames.Add ( "File#" );
    detailverb.TableColNames.Add ( "TF" );
    detailverb.TableColNames.Add ( "Map#" );
    detailverb.TableColNames.Add ( "Sign" );
    detailverb.TableColNames.Add ( "Corr" );
    detailverb.TableColNames.Add ( TrackNameGFP );
    

    detailverb.BeginTable ( 10 );

    long                tforg           = 0;
    int                 curreeg         = 0;

    for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

        if ( tf - tforg == NumTF[ curreeg ] ) {
            tforg  += NumTF[ curreeg ];
            curreeg++;
            }

        detailverb.PutTable ( curreeg + 1 );
        detailverb.PutTable ( tf - tforg );
        detailverb.PutTable ( labels.IsUndefined ( tf ) ? 0 : labels[ tf ] + 1 );
        detailverb.PutTable ( polarity == PolarityEvaluate && labels.IsInverted ( tf ) ? -1 : 1 );
        detailverb.PutTable ( 100 * correlation[ tf ], 2 );
        detailverb.PutTable ( Gfp[ tf ], 4 );
        }


    detailverb.EndTable ();

    detailverb.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( writesegfiles )

        WriteSegFile    (   nummaps /*nclusters*/,
                            maps,
                            labels,
                            SegFile
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // data split by clusters - put that in a separate method
    if ( writeclustersfiles ) {
                                        // ref is different
//      TMaps&              d           = dualversions ? DualData : Data;
        TMaps&              d           = Data;
        double              offset      = dualversions ? - NoMore ( (float) 0, Data.GetMinValue () ) : 0;


        TExportTracks       expclusters;

                                        // export unlabeled + clustered cloud data
        for ( int nc = 0; nc < tfpercluster.GetDim1 () - 1; nc++ ) {


            StringCopy          ( expclusters.Filename, BaseFileNameDataCluster );
            StringAppend        ( expclusters.Filename, ".", IntegerToString ( nclusters, 2 ), ".(", IntegerToString ( nummaps, 2 ), ")" );

            if ( nc == 0 )  StringAppend    ( expclusters.Filename, ".Unlabeled" );
            else            StringAppend    ( expclusters.Filename, ".Cluster", IntegerToString ( nc, 2 ) );
                                        // we're saving only the processing data
//          if ( dualversions )
//              if ( dualgofopen )  StringAppend ( expclusters.Filename, ZScoreInfix );
            if ( dataallzscore )  StringAppend ( expclusters.Filename, ZScoreInfix );

            AddExtension        ( expclusters.Filename, OtherFileExtension );

            expclusters.SetAtomType ( IsVector ( datatype ) ? AtomTypeVector : AtomTypeScalar );  // for ris output

            expclusters.NumTracks          = NumElectrodes;
            expclusters.NumTime            = tfpercluster[ nc ];

                                        // special case of empty cluster
            if ( tfpercluster[ nc ] == 0 ) {

//                DBGV ( nc, "#Cluster is Empty" );
                                        // still write something
                if ( writeemptyclusters
                     && ( nc > 0 || dolimitcorr ) ) {   // accounting for the non-labeled case - dolimitcorr is curretly the only way to have unlabeled time points
                                        // write a single empty map - this should be as rare as possible
                    expclusters.NumTime     = 1;

                    expclusters.Begin ();
                    expclusters.Write ( TMap ( NumRows ) );
                    expclusters.End ();
                    }

                continue;
                } // null tfpercluster


            expclusters.Begin ();

            for ( long tf = 0; tf < NumTimeFrames; tf++ ) {
                                        // !special index coding!
                if ( ! ( nc == 0 && labels.IsUndefined ( tf )
                      || nc >  0 && labels[ tf ] == nc - 1    ) )
                    continue;
                                        // ?save data inverted or not?
                                        // !Data can be either proc. ref. or original ref.!
                mapt    = d[ tf ] * (double) labels.GetSign ( tf );


                for ( int e = 0; e < NumElectrodes; e++ )

                    if ( IsVector ( datatype ) )
                                        // !Note: adapt dual data by removing / adding Normalize operation!
                        expclusters.Write ( TVector3Float ( mapt[ 3 * e     ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ),
                                                            mapt[ 3 * e + 1 ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ),
                                                            mapt[ 3 * e + 2 ]            * ( writenormalizedclusters ? 1 : Norm[ tf ] ) ) );
                    else
                        expclusters.Write (     (float) ( ( mapt[ e         ] + offset ) * ( writenormalizedclusters ? 1 : Norm[ tf ] ) ) );
                } // for tf

            expclusters.End ();
            } // for nc

        } // if writeclusters


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( writevolumes && HasEsiFiles ) {
                                        // we need to calibrate each template individually
        TGoEasyStats        stat ( nummaps );

        for ( int nc = 0; nc < nummaps; nc++ )
        for ( int e = 0; e < NumElectrodes; e++ )
            stat[ nc ].Add ( maps[ nc ][ e ] );

                                        // global cut
        double              cutv            = 0; // 0.50; // 0.75;          // fixed threshold, once data normalized
//      double              cutv[]          = stat[ nc ].Quantile ( 0.90 ); // smarter threshold


                                        // cook current file name
        StringCopy  ( expvol.Filename, BaseFileNameMoreCluster, "." "AllTemplates", "." DefaultMriExt );


        expvol.VolumeFormat     = nummaps <= Highest<UCHAR> () ? AtomFormatByte : AtomFormatFloat;

        expvol.MaxValue         = nummaps;

        int                 spi;
        MriType             v;
        double              normv;
        double              maxv;
        TPointFloat         pvol;
        Volume*             mridata         = MRIDocBackg->GetData ();
        const TArray3<USHORT>&  toip1nn     = *SPDoc->GetInterpol1NN ();
        FctParams           p;


        templvol    = 0;


        for ( int z = 0; z < volsize->GetZExtent (); z++ )
        for ( int y = 0; y < volsize->GetYExtent (); y++ )
        for ( int x = 0; x < volsize->GetXExtent (); x++ ) {

            v   = 0;

                                        // clip to mask
            if ( mridata->GetValue ( x, y, z ) == 0 )
                continue;


            pvol.Set ( x, y, z );

            MRIDocBackg->ToAbs            ( pvol );
            SPDoc      ->AbsoluteToVolume ( pvol );

            pvol.Round ();

                                        // got a solution point?
            if ( toip1nn.WithinBoundary ( pvol ) && toip1nn ( pvol ) != UndefinedInterpolation1NN ) {

                spi     = toip1nn ( pvol );
                maxv    = -1;
                                        // look for the template with max response (once normalized?)
                for ( int nc = 0; nc < nummaps; nc++ ) {

//                  normv   = fabs ( maps[ nc ][ spi ] );                   // non-normalized
                    normv   = stat[ nc ].Normalize ( maps[ nc ][ spi ] );   // normalized to [0..1]

                    if ( normv > maxv ) {
                        maxv    = normv;   
                        v       = (MriType) ( nc + 1 );
                        }
                    }

                                        // clip to higher parts of template?
                if ( maxv < cutv )
                    v   = 0;
                }


            templvol ( x, y, z )    = v;
            } // for x


//      p ( FilterParamDiameter )     = SPDoc->GetMedianDistance ();
//      templvol.Filter ( FilterTypeMedian, p );

        expvol.Begin ();

        expvol.Write ( templvol, ExportArrayOrderZYX );

        expvol.End ();

    
    
/*                                        // Now, write each template separately, each one fully rescaled on its own   
        for ( int nc = 0; nc < nummaps; nc++ ) {
                                        // cook current file name
            StringCopy      ( expvol.Filename, BaseFileNameMoreCluster, ".Template " );
            IntegerToString ( StringEnd ( expvol.Filename ), nc + 1 );
            AddExtension    ( expvol.Filename, DefaultMriExt );

                                        // rescaling to max
            expvol.MaxValue             = 1;


            templvol    = 0;


            for ( int z = 0; z < volsize->GetZExtent (); z++ )
            for ( int y = 0; y < volsize->GetYExtent (); y++ )
            for ( int x = 0; x < volsize->GetXExtent (); x++ ) {

                v   = 0;

                                            // clip to mask
                if ( mridata->GetValue ( x, y, z ) == 0 )
                    continue;


                pvol.Set ( x, y, z );

                MRIDocBackg->ToAbs            ( pvol );
                SPDoc      ->AbsoluteToVolume ( pvol );

                pvol.Round ();


                if ( toip1nn.WithinBoundary ( pvol ) && toip1nn ( pvol ) != UndefinedInterpolation1NN ) {
                    spi     = toip1nn ( pvol );
                    v       = stat[ nc ].Normalize ( maps[ nc ][ spi ] );
                    }


                templvol ( x, y, z )    = v;
                } // for x

                                        // smooth out a bit the blocky 1NN interpolation
                                        // or use GetInterpol4NN instead...
            p ( FilterParamDiameter )     = SPDoc->GetMedianDistance () * 0.75;
            templvol.Filter ( FilterTypeFastGaussian, p );

            expvol.Begin ();

            expvol.Write ( templvol );
    
            expvol.End ();
            } // for map
    */
        } // writevolumes


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Heavy cooking here to output some synthetic informations about the whereabouts
                                        // of each template in the Talairach space
    if ( HasEsiFiles 
      && MRIDocBackg->GetGeometryTransform () 
      && StringIs ( MRIDocBackg->GetGeometryTransform ()->Name, "Talairach" )
      && (bool) SPDoc->GetDisplaySpaces () ) {


        detailverb.NextBlock ();
        detailverb.NextTopic ( "Talairach Regions per Template:" );

        detailverb.NextLine ();
        (ofstream&) detailverb << "(Contribution to regions in permillage)" fastendl;



        const TPoints&          sp          = SPDoc->GetPoints ();
        TPointFloat             p;
        TTracks<double>         talbucket ( NumTalairachCodes, NumTalairachGroups + 1 );    // Talairach groups are put in separate columns
        int                     talindex    = NumTalairachGroups;
        int                     talcodes[ NumTalairachGroups ];
        double                  v;
        double                  offset;

                                        // loop through each ESI template
        for ( int nc = 0; nc < nummaps; nc++ ) {

                                        // get probabilities proportional to each solution point
            offset  = maps[ nc ].GetMinValue ();

            for ( int e = 0; e < NumElectrodes; e++ ) {
                                        // if vectorial take the norm of the SP
                if ( IsVector ( datatype ) )
                    v   = TVector3Float ( maps[ nc ][ 3 * e     ],
                                          maps[ nc ][ 3 * e + 1 ],
                                          maps[ nc ][ 3 * e + 2 ] ).Norm ();
                else                    // if scalar, shift the negative part upward, so we have only positive values
                    v   = maps[ nc ][ e ] - offset;


                mapt[ e ]   = v;
                }

                                        // total sum of weights (probability) is 1
            mapt.Normalize1 ();


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // project the template into the 5 Talairach groups
            talbucket.ResetMemory ();


            for ( int e = 0; e < NumElectrodes; e++ ) {

                p.Set ( sp[ e ].X + 0.5, sp[ e ].Y + 0.5, sp[ e ].Z + 0.5 );
                                        // convert to Talairach
                MRIDocBackg->GetGeometryTransform ()->ConvertTo ( p, & SPDoc->GetDisplaySpaces ()[ 0 ].Bounding );
                                        // get codes for the 5 Talairach groups            
                Taloracle.PositionToCodes ( p, talcodes );

                                        // if appropriate, cumulate to each group
                for ( int tgi = 0; tgi < NumTalairachGroups; tgi++ )
                                        // group is valid if code is non null
                    if ( talcodes[ tgi ] )      talbucket ( talcodes[ tgi ], tgi )     += mapt[ e ];
                }


                                        // normalize the sum of each Talairach group independently
            talbucket.NormalizeColsArea ();


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store indexes of Talairach structures before sorting (and AFTER normalization above, we don't normalize indexes!)
            for ( int tci = 1; tci < NumTalairachCodes; tci++ )

                talbucket ( tci, talindex )     = tci;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting closer to the output
            sprintf ( buff, "Template %0d", nc + 1 );
            detailverb.NextTopic ( buff );

                                        // output the most probable regions, per Talairach group
            for ( int tgi = 0; tgi < NumTalairachGroups; tgi++ ) {
                                        // don't care for this group (grey/white matter)
                if ( tgi == 3 )
                    continue;


//              detailverb.Put ( "Talairach Label:", tci + 1 );
                detailverb.Put ( TalairachGroups[ tgi ], "" );

                                        // sort current group
                talbucket.SortRows ( tgi, Descending );

                                        // output at most the 7 biggest contributors (max group 1 length)
                for ( int i = 0; i < 7; i++ ) {
                                        // retrieve talairach index
                    int     tsi     = talbucket ( i, talindex );

                    if ( tsi == 0 || talbucket ( i, tgi ) < 0.001 )
                        break;
                                        // output region name + prob
                    sprintf ( buff, "%-36s \t%3d", TalairachLabels [ tsi ], Truncate ( 1000 * talbucket ( i, tgi ) ) );
                    detailverb.Put ( "", buff );
                    }

                detailverb.NextLine ();
                }

            } // for template

        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for nclusters


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Meta-Criterion position
int                 argmetacrit;

                                        // Save some computation by not calling the meta-criterion?
if ( computecriteria )

    argmetacrit     = ComputeMetaCriterion  (   critsel,
                                                critselrank,    critselmax,
                                                reqminclusters, reqmaxclusters,
                                                var 
                                            );

else { // ! computecriteria
                                        // single number of clusters
    argmetacrit     = reqmaxclusters;

                                        // force all criteria to this single number of clusters
    for ( int ci = segCritMin; ci < segnumvar; ci++ )

        var ( ci, argmetacrit ) = 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Write error file
Gauge.Next ( gaugesegglobal, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );


TExportTracks     expdata;

StringCopy ( expdata.Filename, DataFile );

                                        // to output only the remaining selected criteria
varout.Reset ( segCritMin, segCritMax );

varout     += critselout;

                                        // for proper output, clean out temp data that were not requested
for ( int ncl = minclusters; ncl < reqminclusters; ncl++ )
    var ( segmaps,  ncl )   =
    var ( segGEV,   ncl )   = 0;


expdata.NumFiles            = 1;
expdata.NumTime             = reqmaxclusters;   // write everything from 1
expdata.NumTracks           = (int) varout;


for ( TIteratorSelectedForward vi ( varout ); (bool) vi; ++vi )
    expdata.VariableNames.Add ( SegmentCriteriaVariablesNames[ vi() ] );

                                        // var is correctly set before reqminclusters, in case reqminclusters is not 1
for ( int ncl = 1; ncl <= reqmaxclusters; ncl++ )
for ( TIteratorSelectedForward vi ( varout ); (bool) vi; ++vi )
    expdata.Write ( (float) var ( vi(), ncl ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary adding a marker to the optimal position in error file
TMarkers            argmetacritrmk;

argmetacritrmk.AppendMarker ( TMarker ( argmetacrit - 1, argmetacrit - 1, MarkerDefaultCode, InfixBestClustering, MarkerTypeMarker ), false );

argmetacritrmk.WriteFile ( DataFileMarker );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary copying the best files to a common directory

if ( commondir ) {

    TGoF            bestfiles;

                                        // copy all files from the best clustering (template file)s), seg file, verbose file)
                                        // get the template name, but it needs to be neutralized for Grep, in case it contains '(' or '.' f.ex.
    StringCopy          ( BaseFileNameCluster, fileoutprefix );

    StringGrepNeutral   ( BaseFileNameCluster );
                                        // append the regexp for the best clustering
    sprintf ( StringEnd ( BaseFileNameCluster ), "\\.%02d\\..*", argmetacrit );


    if ( bestfiles.GrepFiles ( BaseDir, BaseFileNameCluster, GrepOptionDefaultFiles ) )
        bestfiles.CopyFilesTo ( outputcommondir, (CopyToFlags) ( CopyAndKeepOriginals | CopyAndPreserveFileNames | CopyAllKnownBuddies ) );

                                        // also copy the .error.data file
    StringCopy ( buff,  DataFile, "*" );

    if ( bestfiles.FindFiles ( buff ) )
        bestfiles.CopyFilesTo ( outputcommondir, (CopyToFlags) ( CopyAndKeepOriginals | CopyAndUpdateFileNames   | CopyAllKnownBuddies ) );

                                        // open copied error.data for the user
    if ( complimentaryopen )

        for ( int i = 0; i < (int) bestfiles; i++ )

            if ( IsExtension ( bestfiles[ i ], FILEEXT_DATA ) )

                CartoolObjects.CartoolDocManager->OpenDoc ( bestfiles[ i ], dtOpenOptions );
    } // if commondir 

else { // ! commondir
                                        // open the original error.data for the user
    if ( complimentaryopen )

        CartoolObjects.CartoolDocManager->OpenDoc ( DataFile, dtOpenOptions );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update verbose with criterion summary
Gauge.Next ( gaugesegglobal, GroupGauge.IsNotAlive () ? SuperGaugeUpdateTitle : SuperGaugeNoTitle );

                                        // This is only for the main verbose
mainverb.NextTopic ( "Clustering Quality Criteria:" );

mainverb.Put ( "Total number of criteria:", (int) critselout );

mainverb.NextLine ();
mainverb.Put ( "Criteria for Ranking meta-criterion:", (int) critselrank );
for ( TIteratorSelectedForward ci ( critselrank ); (bool) ci; ++ci )
    mainverb.Put ( "", SegmentCriteriaVariablesNames[ ci() ] );

mainverb.NextLine ();
mainverb.Put ( "Criteria for ArgMax meta-criterion:", (int) critselmax );
for ( TIteratorSelectedForward ci ( critselmax ); (bool) ci; ++ci )
    mainverb.Put ( "", SegmentCriteriaVariablesNames[ ci() ] );


mainverb.NextTopic ( "Results:" );
mainverb.NextLine ();

(ofstream&) mainverb << StreamFormatFixed;

(ofstream&) mainverb << "Rows       : number of Clusters of the clustering method"  fastendl;
(ofstream&) mainverb << "#Maps      : number of Maps / Templates finally obtained"  fastendl;
(ofstream&) mainverb << "GEV        : Global Explained Variance, multiplied by 100" fastendl;
(ofstream&) mainverb << "Criterion  : multiplied by 100"                            fastendl;
mainverb.NextLine ( 2 );


mainverb.ResetTable ();


mainverb.TableColNames.Add ( "#Clusters" );
mainverb.TableColNames.Add ( "#Maps" );
mainverb.TableColNames.Add ( "GEV" );
mainverb.TableColNames.Add ( SegmentCriteriaVariablesNames[ segMeanRanks    ] );
mainverb.TableColNames.Add ( SegmentCriteriaVariablesNames[ segMetaCrit     ] );


for ( int ncl = reqminclusters; ncl <= reqmaxclusters; ncl++ ) {
    sprintf ( buff, "%0d", ncl );
    mainverb.TableRowNames.Add ( buff );
    }


mainverb.BeginTable ( 12 );


for ( int ncl = reqminclusters; ncl <= reqmaxclusters; ncl++ ) {

    mainverb.PutTable ( (int) var ( segmaps,        ncl )    );
    mainverb.PutTable ( 100 * var ( segGEV,         ncl ), 2 );
    mainverb.PutTable ( 100 * var ( segMeanRanks ,  ncl ), 2 );
    mainverb.PutTable ( 100 * var ( segMetaCrit ,   ncl ), 2 );
    }


mainverb.EndTable ();

mainverb.NextTopic ( "Optimal Clustering:" );
mainverb.NextLine ();


//StringCopy      ( buff, "ArgMax of '", SegmentCriteriaVariablesNames[ segMeanRanks ], "' criterion:" );
//mainverb.Put    ( buff, argmaxmedian );

//StringCopy      ( buff, "ArgMax of '", SegmentCriteriaVariablesNames[ segMedianArgMaxCurve ], "' criterion:" );
//mainverb.Put    ( buff, medianargmax );

//StringCopy      ( buff, "ArgMax of '", SegmentCriteriaVariablesNames[ segMerge3Curves ], "' criterion:" );
//mainverb.Put    ( buff, argmerge3curves );

StringCopy      ( buff, "ArgMax of '", SegmentCriteriaVariablesNames[ segMetaCrit ], "' criterion:" );
mainverb.Put    ( buff, argmetacrit );

mainverb.NextLine ( 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

mainverb.Close ();

DeallocateVariables ();

if ( deleteindivdirs )
    NukeDirectory ( basefilename );
                                        // if this directory is empty, just delete it
if ( IsEmptyDirectory ( BaseDirMore ) )
    NukeDirectory ( BaseDirMore );

if ( IsEmptyDirectory ( BaseDirDataCluster ) )
    NukeDirectory ( BaseDirDataCluster );

if ( IsEmptyDirectory ( BaseDirPreproc ) )
    NukeDirectory ( BaseDirPreproc );


Gauge.FinishParts ();

if ( GroupGauge.IsNotAlive () )     CartoolObjects.CartoolApplication->SetMainTitle    ( Gauge );

Gauge.Finished ();

return  true;
}


//----------------------------------------------------------------------------

bool    TMicroStates::Segmentation  (   TGoF&               gof,
                                        AnalysisType        analysis,           ModalityType            modality,
                                        AtomType            datatype,           PolarityType            polarity,           ReferenceType       dataref,
                                        ClusteringType      clusteringmethod,
                                        int                 reqminclusters,     int                     reqmaxclusters,
                                        int                 numrandomtrials,
                                        bool                dolimitcorr,        double                  limitcorr,
                                        bool                sequentialize,
                                        bool                mergecorr,          double                  mergecorrthresh,
                                        bool                smoothing,          int                     smoothinghalfsize,  double              smoothinglambda,
                                        bool                rejectsmall,        int                     rejectsize,
                                        MapOrderingType     mapordering,        const char*             templatesfile,
                                        const char*         basefilename
                                    )
{
return  Segmentation    (   gof,    
                            gof,    
                            NoDualData,             0,
                            analysis,               modality,           UnknownSamplingTime,

                            EpochWholeTime,
                            NoSkippingBadEpochs,    0,
                            NoGfpPeaksDetection,    0,
                            NoTimeResampling,       0,      0,
                            SpatialFilterNone,      0,

                            datatype,               polarity,           dataref,
                            clusteringmethod,
                            reqminclusters,         reqmaxclusters,
                            numrandomtrials,        MeanCentroid,
                            dolimitcorr,            limitcorr,

                            sequentialize,
                            mergecorr,              mergecorrthresh,
                            smoothing,              smoothinghalfsize,  smoothinglambda,
                            rejectsmall,            rejectsize,
                            mapordering,            templatesfile,

                            (MicroStatesOutFlags) ( WriteSegFiles | WriteTemplatesFiles | WriteEmptyClusters ),
                            basefilename,           0,
                            false
                        );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
