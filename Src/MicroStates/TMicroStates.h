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

#include    "TArray1.h"
#include    "TVector.h"
#include    "TMaps.h"
#include    "Math.Stats.h"
#include    "Math.Random.h"
#include    "TSelection.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Files.TOpenDoc.h"

#include    "CartoolTypes.h"            // TDataFormat EpochsType SkippingEpochsType PolarityType CentroidType
#include    "TTracksDoc.h"
#include    "TLabeling.h"               // files that include this will also need TLabeling.h anyway

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A few general flags used to control some of the micro-states analysis behavior
constexpr int           SmoothingDefaultHalfSize    =  3;               // Default smoothing half-window size - usually for samnpling rates in the 250[Hz] range
constexpr double        SmoothingDefaultBesag       = 20;               // Default smoothing Besag / lambda factor (was 10 for a very long time)
                                            
constexpr bool          AllowEmptyClusterFiles      = true;             // Choosing how to write empty clusters on files: ignoring / writing empty file / writing a null map
                                            
constexpr bool          SavingNormalizedClusters    = false;            // Controlling if data clusters are written normalized or with original power


constexpr CentroidType  EEGCentroidMethod           = MeanCentroid;     // always, we want to keep it linear
constexpr CentroidType  MEGCentroidMethod           = MeanCentroid;     // always, we want to keep it linear
constexpr CentroidType  ESICentroidMethod           = MeanCentroid;     // maps can be quite empty after optional thresholding, a Median might be too radical while a Mean will still output something
//constexpr CentroidType  ESICentroidMethod         = MedianCentroid;   // Median gives sharper shapes/contours, counterpart is it basically forces to store all the data, and is more time-consuming to compute

                                        // Flag used to generate all known criteria for tests
//#define             UseAllCriteria


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Specific to micro-states analysis track names 
                                        // Kept as defines and not constexpr for ease of concatenation
#define             PostfixCriterionRobust              "R"
#define             PostfixKLCartool                    "C"
#define             PostfixCriterionDerivative1         "'"
#define             PostfixCriterionDerivative2         "''"


#define             TrackNameSeg                        "Seg"
#define             TrackNamePolarity                   "Polarity"
#define             TrackNameGEV                        "GEV"
#define             TrackNameEV                         "EV"
#define             TrackNameCorr                       "Corr"
#define             TrackNameCrossValidation            "1-CV"
#define             TrackNameClust                      "Clust"
#define             TrackNameMaps                       "Maps"
#define             TrackNameKrzanowskiLai              "KL"
#define             TrackNameKrzanowskiLaiC             TrackNameKrzanowskiLai PostfixKLCartool
#define             TrackNameCalinskiHarabasz           "CalinHarab"
#define             TrackNameDunn                       "Dunn"
#define             TrackNameDunnRobust                 TrackNameDunn PostfixCriterionRobust
#define             TrackNameCIndex                     "CIndex"
#define             TrackNameDaviesBouldin              "DaviesBould"
#define             TrackNameMarriott                   "Marriott"
#define             TrackNamePointBiserial              "PtBiserial"
#define             TrackNameSilhouettes                "Silhouettes"
#define             TrackNameTraceW                     "TraceW"
#define             TrackNameGamma                      "Gamma"
#define             TrackNameGPlus                      "G(+)"
#define             TrackNameFreyVanGroenewoud          "FreyVanGr"
#define             TrackNameWforKL                     "SumWPD2"
#define             TrackNameTau                        "Tau"
#define             TrackNameHartigan                   "Hartigan"
#define             TrackNameRatkowski                  "Ratkowski"
#define             TrackNameMcClain                    "McClain"
#define             TrackNameAllScores                  "AllScores"
#define             TrackNameAllVotes                   "AllVotes"
#define             TrackNameAllRanks                   "AllRanks"
#define             TrackNameAllInAll                   "AllInAll"
#define             TrackNameMeanRanks                  "MeanCrit"
#define             TrackNameArgMaxHisto                "ProbMaxCrit"
#define             TrackNameArgMax                     "MedianArgMax"
#define             TrackNameMetaCriterion              "MetaCrit"

                                        // Default variables for .seg files
enum                SegVarType
                    {
                    SegVarGfp,
                    SegVarPolarity,
                    SegVarSegment,
                    SegVarGev,
                    SegVarCorr,

                    NumSegVariables,
                    };

//constexpr char*   SegFile_VariableNames_Abs       = TrackNameRMS " " TrackNameDISPlus " " TrackNameSeg " " TrackNameGEV " " TrackNameCorr;
//constexpr char*   SegFile_VariableNames_Signed    = TrackNameGFP " " TrackNameDIS " " TrackNameSeg " " TrackNameGEV " " TrackNameCorr;        // Older versions
constexpr char*     SegFile_VariableNames_Signed    = TrackNameGFP " " TrackNamePolarity " " TrackNameSeg " " TrackNameGEV " " TrackNameCorr;


//----------------------------------------------------------------------------

enum        AnalysisType
            {
            UnknownAnalysis,
            AnalysisERP,
            AnalysisRestingStatesIndiv,
            AnalysisRestingStatesGroup,

            NumAnalysisTypes
            };

enum        ModalityType
            {
            UnknownModality,
            ModalityEEG,
            ModalityMEG,
            ModalityESI,

            NumModalityTypes
            };

enum        SamplingTimeType
            {
            UnknownSamplingTime,
            SamplingTimeWhole,
            SamplingTimeResampling,

            NumSamplingTimeTypes
            };

enum        ClusteringType
            {
            UnknownClustering,
            ClusteringKMeans,
            ClusteringTAAHC,

            NumClusteringType
            };

extern const char   ClusteringString[ NumClusteringType ][ 128 ];

enum        MapOrderingType 
            {
            MapOrderingNone,
            MapOrderingTemporally,
            MapOrderingTopographically,
            MapOrderingSolutionPoints,
            MapOrderingAnatomically,
            MapOrderingFromTemplates,
            MapOrderingContextual,      // Segmentation will pick the one that fits best

            NumMapOrering
            };

extern const char   MapOrderingString[ NumMapOrering ][ 64 ];


enum        MicroStatesOutFlags
            {
            NoMicroStatesOutFlags   = 0x00000000,
                                                    // Directories
            CommonDirectory         = 0x00000001,
            DeleteIndivDirectories  = 0x00000002,
                                                    // Files to output
            WriteSegFiles           = 0x00000010,
            WriteTemplatesFiles     = 0x00000020,
            WriteClustersFiles      = 0x00000040,
            WriteDispersionFiles    = 0x00000080,
            WriteSyntheticFiles     = 0x00000100,
            WriteCorrelationFiles   = 0x00000200,
            WriteStatDurationsFiles = 0x00000400,
            WriteSegFrequencyFiles  = 0x00000800,
                                                    // Data format
            WriteEmptyClusters      = 0x00001000,
            WriteNormalizedClusters = 0x00002000,
            VariablesShortNames     = 0x00010000,
            VariablesLongNames      = 0x00020000,
            SheetLinesAsSamples     = 0x00100000,
            SheetColumnsAsFactors   = 0x00200000,
            SaveOneFilePerGroup     = 0x00400000,
            SaveOneFilePerVariable  = 0x00800000,
                                                    // Files ownership
            OwningFiles             = 0x10000000,
            DeleteTempFiles         = 0x20000000,
            };


//----------------------------------------------------------------------------
                                        // All sorts of temp variables needed by the clustering / criteria / fitting
enum        SegmentCriteriaVariables
            {      
            segclust,
            segmaps,
            segGEV,
                                        // Temp, internal variables used for criteria
            segSumWPD2,                 // For KL
            segWCD,
            segWCD2,
            segBCD2,
            segWPD,
            segBPD,
            segAPD,
            segWPD2,
            segWPdD2,
                                        // Best Clustering various criteria
                                        // Many are available in 3 flavors (except for those criteria that are already built on some sort of derivatives, like KL):
                                        //  - Original criterion
                                        //  - Its second derivative - a very common practice
                                        //  - Its second derivative on ranked criterion - aka robust second derivative
            segCalinskiHarabasz,
            segCalinskiHarabaszDeriv,
            segCalinskiHarabaszDerivRobust,
            segCIndex,
            segCIndexDeriv,
            segCIndexDerivRobust,
            segCrossValidation,
            segCrossValidationDeriv,
            segCrossValidationDerivRobust,
            segDaviesBouldin,
            segDaviesBouldinDeriv,
            segDaviesBouldinDerivRobust,
            segDunn,
            segDunnDeriv,
            segDunnDerivRobust,
            segDunnRobust,
            segDunnRobustDeriv,
            segDunnRobustDerivRobust,
            segFreyVanGroenewoud,
            segFreyVanGroenewoudDeriv,
            segFreyVanGroenewoudDerivRobust,
            segGamma,
            segGammaDeriv,
            segGammaDerivRobust,
            segGPlus,
            segHartigan,
            segHartiganDeriv,
            segHartiganDerivRobust,
            segKrzanowskiLai,           // Original formula
            segKrzanowskiLaiC,          // Cartool  formula
            segKrzanowskiLaiRobust,     // Original formula + robust stats
            segKrzanowskiLaiCRobust,    // Cartool  formula + robust stats
            segMarriott,                // !Currently off!
            segMcClain,
            segMcClainDeriv,
            segMcClainDerivRobust,
            segPointBiserial,
            segPointBiserialDeriv,
            segPointBiserialDerivRobust,
            segRatkowski,
            segRatkowskiDeriv,
            segRatkowskiDerivRobust,
            segSilhouettes,
            segSilhouettesDeriv,
            segSilhouettesDerivRobust,
            segTau,
            segTraceW,
                                        // Merging criteria together:
            segMeanRanks,
            segHistoArgLocalMax,
//          segMedianArgMaxCurve,
//          segMerge3Curves,
                                        // ..and finally the Meta-Criterion
            segMetaCrit,

            segnumvar,

            segTempCritMin  = segSumWPD2,
            segTempCritMax  = segWPdD2,
            segCritMin      = segCalinskiHarabasz,
            segCritMax      = segTraceW,
            };


extern const char   SegmentCriteriaVariablesNames[ segnumvar ][ 32 ];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Used in ComputeW, needed to compute the various clustering criteria
enum        WFlag 
            {
            WFromCentroid       = 0x01,
            WPooled             = 0x02,
            WPooledDeterminant  = 0x04,

            WDistance           = 0x10,
            WSquareDistance     = 0x20,
//          WDimSquareDistance  = 0x40,
            };


                                        // Some utility functions used for clustering criteria

//constexpr int     KLFilterSize                = 1;
constexpr int       KLFilterSize                = 0;    // There aren't any KL criterion filter anymore


void                SmoothCurve         (   double*         data,       int             ncl1,   int         ncl2, 
                                            FilterTypes     filtertype, FctParams&      params  
                                        );

void                GetMinMax           (   const double*   data,       int             ncl1,   int         ncl2, 
                                            double&         minv,       double&         maxv, 
                                            bool            ignorenull 
                                        );

                                        // for side effects like derivative, we need some more clustering
constexpr int       CriterionMargin     = 1;


enum                NormalizeCurveFlag
                    {
                    NormalizeNone,      // not taken into account
                    NormalizeTo0,       // set to 0
                    NormalizeToE,       // set to Epsilon > 0
                    NormalizeTo1,       // set to 1
                    };

constexpr double    NormalizedEpsilon   = 0.01;

void                NormalizeCriterion      (   double*             data,
                                                int                 ncl1,       int                 ncl2, 
                                                NormalizeCurveFlag  minflag,    NormalizeCurveFlag  maxflag,    bool ignorenull 
                                            );

void                StandardizeCriterion    (   double*             data, 
                                                int                 ncl1,       int                 ncl2,
                                                bool                ignorenull 
                                            );


enum                RankCriterionFlag
                    {
                    RankCriterionLinear     = 1,    // rank -> linear weight
                    RankCriterionSquare     = 2,    // rank -> square linear weight
                    RankCriterionInverse    = 3,    // rank -> inverse linear weight
                    };

void                RankCriterion           (   const double*       datai,      double*             datao,
                                                int                 ncl1,       int                 ncl2, 
                                                RankCriterionFlag   flag
                                            );


enum                CriterionDerivativeEnum
                    {
                    CriterionDerivativePrime        = 0x0001,
                    CriterionDerivativeSecond       = 0x0002,

                    CriterionDerivativeOnData       = 0x0010,
                    CriterionDerivativeOnRanks      = 0x0020,
                    };

void                CriterionToDerivative   (   const double*       datai,          double*         datao,          int         datasize,
                                                int                 minclusters,    int             maxclusters,
                                                CriterionDerivativeEnum     flag,
                                                double              scale
                                            );


//----------------------------------------------------------------------------
                                        // Holds common variables and functions for both the segmentation and the fitting
enum        EpochsType;
enum        SpatialFilterType;
enum        GfpPeaksDetectType;
enum        SkippingEpochsType;
enum        ResamplingType;
enum        AtomType;
enum        PolarityType;
enum        ReferenceType;
enum        CentroidType;
class       TInterval;


class   TMicroStates    :   public virtual  TDataFormat     // !could be inherited by different sub-children!
{
public:
                    TMicroStates ();
    virtual        ~TMicroStates ();


    TMaps           Data;               // All EEG data concatenated as TMap's, in a single array
    TVector<double> Gfp;                // Gfp -> ( Segment output, Fitting Variables & output )
    TVector<double> Dis;                // Dis -> ( Segment output, Fitting output )
    TVector<double> Norm;               // Norm of the input data - used in: SigmaMu2, SigmaD2, GEV, CV, Smoothing


    void            AllocateVariables       ( const TGoF& gof, AtomType datatype );
    void            AllocateVariables       ( const TGoGoF& gogof, int gofi1, int gofi2, int filei, AtomType datatype );
    void            AllocateVariables       ( const TGoGoF& gogof, int gofi1, int gofi2, int filei1, int filei2, AtomType datatype );
    void            DeallocateVariables     ();
    void            ReadData                ( const TGoF& gof, AtomType datatype, ReferenceType dataref, bool showprogress );
    void            ReadData                ( const TGoGoF& gogof, int gofi1, int gofi2, int filei, AtomType datatype, ReferenceType dataref, bool showprogress );
    void            ReadData                ( const TGoGoF& gogof, int gofi1, int gofi2, int filei1, int filei2, AtomType datatype, ReferenceType dataref, bool showprogress );
    void            PreprocessMaps          ( TMaps& maps, bool forcezscorepos, AtomType datatype, PolarityType polarity, ReferenceType dataref, bool ranking, ReferenceType processingref, bool normalizing, bool computeandsavenorm );

                                        // Clustering methods themselves:
    int             SegmentKMeans           ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity, int numrandomruns, CentroidType centroid, bool ranking );
    int             SegmentTAAHC            ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity, CentroidType centroid, bool ranking, TMaps& savedmaps, TLabeling& savedlabels, TMaps& tempmaps, int maxclusters );

                                        // Full segmentation processing:
    bool            Segmentation        (   TGoF&               gof,                                                    // to be processed
                                            const TGoF&         origgof,                                                // the originals before preprocessing
                                            DualDataType        dualdata,           TGoF*                   gofalt,     // alternative dataset (RIS or EEG)
                                            AnalysisType        analysis,           ModalityType            modality,           SamplingTimeType    samplingtime,   // last parameter for info only
                                        // following preprocessing parameters are here just for info/verbose - they should have been applied beforehand
                                            EpochsType          epochs,
                                            SkippingEpochsType  badepochs,          const char*             listbadepochs,
                                            GfpPeaksDetectType  gfppeaks,           const char*             listgfppeaks,
                                            ResamplingType      resampling,         int                     numresamples,       int                 resamplingsize,
                                            SpatialFilterType   spatialfilter,      const char*             xyzfile,
//                                          ZScoreType          timezscore,

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
                                            const char*         basefilename,       const char*             outputcommondir,
                                            bool                complimentaryopen
                                            );
                                        // Simplified wrapper to Segmentation:
    bool            Segmentation        (   TGoF&               gof,
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
                                            );


    int             ComputeMetaCriterion (  TSelection&         critsel,            TSelection&             critselrank,        TSelection&         critselmax,
                                            int                 reqminclusters,     int                     reqmaxclusters,
                                            TArray2<double>&    var
                                            );


    bool            BackFitting         (   const char*         templatefile,
                                            TGoGoF&             gogof,              int                     numwithinsubjects,
                                            AnalysisType        analysis,           ModalityType            modality,
                                            TStrings            epochfrom,          TStrings                epochto,            TStrings            epochmaps,  // !copies!

                                            SpatialFilterType   spatialfilter,      const char*             XyzFile,
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
                                        );
                                        // Simplified wrapper to BackFitting:
    bool            BackFitting         (   const char*         templatefile,
                                            TGoGoF&             gogof,              int                     numwithinsubjects,
                                            AnalysisType        analysis,           ModalityType            modality,
                                            AtomType            datatype,           PolarityType            polarity,           ReferenceType       dataref,
                                            bool                dolimitcorr,        double                  limitcorr,
                                            bool                smoothing,          int                     smoothinghalfsize,  double              smoothinglambda,
                                            bool                rejectsmall,        int                     rejectsize,
                                            const TSelection&   varout,
                                            MicroStatesOutFlags outputflags,
                                            const char*         basefilename
                                        );

                                        // Segments post-processing
    void            GetTemporalOrdering     ( int nclusters, const TLabeling& labels, TArray2<int>& ordering )                                                      const;
    void            GetOrderingFromTemplates( int nclusters, const TMaps& maps, const TMaps& templatesmaps, PolarityType polarity, TArray2<int>& ordering )         const;
    void            GetTopographicalOrdering( int nclusters, const TMaps& maps, const TElectrodesDoc* xyzdoc, TArray2<int>& ordering )                              const;
    void            GetAnatomicalOrdering   ( int nclusters, const TMaps& maps, const TSolutionPointsDoc* spdoc, const TVolumeDoc* mridoc, TArray2<int>& ordering ) const;


    void            WriteSegFile        (   int nclusters, const TMaps& maps, const TLabeling& labels, const char*  filename  )   const;


protected:

    TArray1<TMap*>  ToData;             // Some functions can benefit from having a handy array of pointers-to-maps all ready

                                        // Optional documents: global variables will allow to have single opening and closing in batch processing
    TOpenDoc<TElectrodesDoc>    XyzDoc; // (not currently used for the spatial filtering)
    TOpenDoc<TInverseMatrixDoc> ISDoc;
    TOpenDoc<TVolumeDoc>        MRIDocBackg;
    TOpenDoc<TSolutionPointsDoc>SPDoc;


    int             NumElectrodes;
    int             NumRows;            // = NumElectrodes if scalar data; 3 * NumElectrodes if vectorial data
    int             TotalElectrodes;
    int             NumTimeFrames;
    float           SamplingFrequency;
    TSelection      EnumEpochs;

    int             NumFiles;           // Total number of file for current processing
    int             MaxNumTF;           // Longest file duration
    TArray1<int>    NumTF;              // Duration per file
    TArray1<int>    OffsetTF;           // Beginning of data in absolute TF
    std::vector<TInterval>  IntervalTF; // Interval in absolute TF for each file
//  TArray1<long>   AbsTFToRelTF;


    TRandUniform    RandomUniform;      // used in GetRandomMaps

    TSuperGauge     Gauge;
    TSuperGauge     GroupGauge;         // global gauge, used if more than 1 group

                                        // Data handling
    void            TimeRangeToDataRange    ( int filei, long fromtf, long totf, long &tfmin, long &tfmax );
    void            ResetDissimilarityJunctions ();

                                        // sigmamu2 sigmad2 utilities
    double          ComputeSigmaMu2         ( int numelectrodes, const TMaps& maps, const TLabeling& labels, long tfmin, long tfmax );
    double          ComputeSigmaD2          ( int numelectrodes, const TLabeling& labels, long tfmin, long tfmax );
    double          ComputeGEV              ( const TMaps& maps, const TLabeling& labels, long tfmin, long tfmax ) const;
    void            ComputeGevPerCluster    ( int nclusters, const TMaps& maps, const TLabeling& labels, TVector<double>& gevpercluster ) const;

                                        // Clustering methods
    bool            SegmentKMeans_Once      ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity, double &gev, CentroidType centroid, bool ranking /*, TArray1<double> dispersion*/ );
    void            GetRandomMaps           ( int nclusters, TMaps& maps );
//  void            GetRandomMapsPP         ( int nclusters, TMaps& maps, PolarityType polarity );
    int             SegmentTAAHC_Init       (                TMaps& maps, TLabeling& labels, PolarityType polarity, CentroidType centroid, bool ranking );

                                        // Clustering criteria
    void            ComputeAllWBA           ( int nclusters, int nummaps, const TMaps& maps, const TLabeling& labels, PolarityType polarity, TArray2<double>& var );
    void            ComputeW                ( int nc, const TMaps& maps, const TLabeling& labels, PolarityType polarity, WFlag flags, TEasyStats* statcluster, TEasyStats* statnoncluster, TEasyStats* statall, TEasyStats* indexall );
    void            ComputeClustersDispersion   ( int nummaps, TMaps& maps, TLabeling& labels, PolarityType polarity, TArray1<double>& dispersion, bool precise );

    double          ComputeCrossValidation  ( int nclusters, int numelectrodes, TMaps& maps, TLabeling& labels, PolarityType polarity, long tfmin, long tfmax );
    double          ComputeKrzanowskiLaiW   ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity );
    void            ComputeKrzanowskiLai    ( int minclusters, int maxclusters, int numwfilter, CriterionDerivativeEnum flag, TArray2<double>& var );
    double          ComputeCalinskiHarabasz ( int nclusters );
    double          ComputeDunn             ( int nclusters );
    double          ComputeDunnRobust       ( int nclusters );
    double          ComputeCIndex           ( int nclusters );
    double          ComputeDaviesBouldin    ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity );
    double          ComputeCubicClusteringCriterion ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity );
//  double          ComputeMarriott         ( int nclusters );
    double          ComputePointBiserial    ( int nclusters, TLabeling& labels );
    double          ComputeSilhouettes      ( int nclusters, TLabeling& labels );
    double          ComputeTraceW           ( int nclusters );
    double          ComputeGamma            ( int nclusters, double& GPlusCriterion, double& Tau );
    void            ComputeFreyVanGroenewoud( int minclusters, int maxclusters, TArray2<double>& var );
    void            ComputeHartigan         ( int minclusters, int maxclusters, TArray2<double>& var );
    double          ComputeRatkowski        ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity );
    double          ComputeMcClain          ( int nclusters );

                                        // Segmentation smoothing
    void            SmoothingLabeling       ( int filemin, int filemax, long fromtf, long totf, int nclusters, TMaps& maps, TSelection *mapsel, TLabeling& labels, int winsize, double lambda, PolarityType polarity, double limitcorr );
    void            SmoothingLabeling       ( long tfmin, long tfmax, int nclusters, TMaps& maps, TSelection *mapsel, TLabeling& labels, int winsize, double lambda, PolarityType polarity, double limitcorr );
    void            DistributeChunk         ( long tfmin, long tfmax, long segfrom, long segto, TMaps& maps, TLabeling& labels, PolarityType polarity, double limitcorr );
    void            RejectSmallSegments     ( int filemin, int filemax, long fromtf, long totf, TMaps& maps, TLabeling& labels, PolarityType polarity, double limitcorr, int rejectsize );
    void            RejectSmallSegments     ( long tfmin, long tfmax, TMaps& maps, TLabeling& labels, PolarityType polarity, double limitcorr, int rejectsize );
    int             RejectLowCorrelation    ( long tfmin, long tfmax, TMaps& maps, TLabeling& labels, double limitcorr );
    void            RejectBadEpochs         ( int filei, long fromtf, long totf, const char* eegfile, SkippingEpochsType badepochs, const char* rejectlist, double badepochstolerance, TLabeling& labels );
    int             SequentializeSegments   ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity, CentroidType centroid, bool ranking );
    int             MergeCorrelatedSegments ( int nclusters, TMaps& maps, TLabeling& labels, PolarityType polarity, CentroidType centroid, bool ranking, double corrthresh );

                                        // Segments post-processing
    void            PreProcOrdering         ( int nclusters, TArray2<int>& ordering )   const;
    void            PostProcOrdering        ( TArray2<int>& ordering )                  const;


private:
                                        // Used privately for Within / Across clusters distances - see ComputeAllWBA
    int             CriterionDownSampling;

    TEasyStats      StatWCentroidDistance;

    TEasyStats      StatWCentroidSquareDistance;
    TEasyStats      StatBCentroidSquareDistance;

    TEasyStats      StatWPooledDistance;
    TEasyStats      StatBPooledDistance;
    TEasyStats      StatAPooledDistance;
    TEasyStats      StatAPooledDistanceIndex;

    TEasyStats      StatWPooledSquareDistance;

//  TEasyStats      StatWPooledDetSquareDistance;   // !not used for the moment!

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
