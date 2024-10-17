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

#include    "Files.TGoF.h"

#include    "TMicroStates.h"

#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
// Here we have the definition of the 2 Dialogs needed to control the EEG Micro-States segmentation:
//  - One for the input / output files options
//  - One for the processing parameters
//----------------------------------------------------------------------------

constexpr char*     SegmentationTitle           = "Segmentation";


constexpr int       SegmentMinNumData           = 100;
constexpr int       SegmentMinSampleSize        = 100;
constexpr int       SegmentMinNumResampling     = 5;
constexpr int       SegmentMaxNumResampling     = 1000;
constexpr double    SegmentMinCoverage          = 0.95;
constexpr double    SegmentResamplingRandomTrialsRatio  = 0.50;

                                        // With current filters (40[Hz]?), approximate Gfp Peaks sampling frequency is about 34[Hz]
constexpr double    GfpPeaksSamplingFrequency   = 34.0;
constexpr int       GfpPeaksDownsamplingRatio   = 6;

                                        // Define the preferred file extensions
constexpr char*     SegmentationEegTemplateExt  = FILEEXT_EEGEP;
constexpr char*     SegmentationRisTemplateExt  = FILEEXT_RIS;
constexpr char*     SegmentationEegFileExt      = FILEEXT_EEGSEF;
constexpr char*     SegmentationRisFileExt      = FILEEXT_RIS;


//----------------------------------------------------------------------------
                                        // Presets for Files:

                                        // Enumerate all presets
enum        SegPresetFilesEnum
            {
            SegPresetFilesERP,

            SegPresetFileSeparator1,
            SegPresetFilesRestingStatesSubjAllData,
            SegPresetFilesRestingStatesSubjResampling,

            SegPresetFileSeparator2,
            SegPresetFilesRestingStatesGroupAllData,
            SegPresetFilesRestingStatesGroupResampling,

            NumSegPresetFiles,

            SegPresetFilesDefault           = SegPresetFilesERP,
            };

                                        // Flags associated to files
enum        TypeFilesFlags
            {
            SegPresetFilesNone              = 0x00000000,

            SegPresetWholeTimeRange         = 0x00000001,
            SegPresetEpochs                 = 0x00000002,
            SegPresetResampling             = 0x00000004,

            SegPresetPrioritySampleSize     = 0x00000010,
            SegPresetPriorityNumResampling  = 0x00000020,
            };

                                        // Structure used to fully specify each file preset parameters
class       SegPresetFilesSpec
{
public:
    SegPresetFilesEnum      Code;                   // redundant but included for clarity
    char                    Text [ 128 ];           // for dialog
    int                     NumResampling;
    double                  SampleSizeSeconds;      // sample size, store in seconds
    double                  SampleRatio;            // sample ratio of the available data
    double                  ResamplingCoverage;
    TypeFilesFlags          FilesFlags;
};
                                        // !NOT const, as we will update some fields!
extern  SegPresetFilesSpec  SegPresetsFiles[ NumSegPresetFiles ];


inline  bool    IsSegSubjectFilePreset      ( SegPresetFilesEnum preseti )  { return IsInsideLimits ( preseti, SegPresetFilesRestingStatesSubjAllData,  SegPresetFilesRestingStatesSubjResampling  ); }
inline  bool    IsSegGroupFilePreset        ( SegPresetFilesEnum preseti )  { return IsInsideLimits ( preseti, SegPresetFilesRestingStatesGroupAllData, SegPresetFilesRestingStatesGroupResampling ); }
inline  bool    IsSegResamplingFilePreset   ( SegPresetFilesEnum preseti )  { return preseti == SegPresetFilesRestingStatesSubjResampling || preseti == SegPresetFilesRestingStatesGroupResampling; }
inline  bool    IsSegRestingStatesFilePreset( SegPresetFilesEnum preseti )  { return IsInsideLimits ( preseti, SegPresetFilesRestingStatesSubjAllData,  SegPresetFilesRestingStatesGroupResampling ); }


//----------------------------------------------------------------------------
                                        // Presets for Parameters:

                                        // Enumerate all presets
enum        SegPresetParamsEnum
            {
                                        // EEG stands for signed data, so it could be MEG Magnetometer too...
            SegPresetEegSurfaceErpTAAHC,
            SegPresetEegSurfaceErpKMeans,
            SegPresetEegSurfaceSpontKMeans,
            SegPresetEegSurfaceSpontKMeansAll,
            SegPresetEegSurfaceCollectionKMeans,

            SegPresetSeparator1,
            SegPresetEegIntraErpTAAHC,
            SegPresetEegIntraErpKMeans,
            SegPresetEegIntraSpontKMeans,
            SegPresetEegIntraSpontKMeansAll,
            SegPresetEegIntraCollectionKMeans,

                                        // MEG stands for Positive Data here (norm of gradiometer)...
            SegPresetSeparator2,
            SegPresetMegSurfaceErpTAAHC,
            SegPresetMegSurfaceErpKMeans,
            SegPresetMegSurfaceSpontKMeans,
            SegPresetMegSurfaceSpontKMeansAll,
            SegPresetMegSurfaceCollectionKMeans,

            SegPresetSeparator3,
            SegPresetRisScalErpTAAHC,
            SegPresetRisScalErpKMeans,
            SegPresetRisScalSpontKMeans,
            SegPresetRisScalSpontKMeansAll,
            SegPresetRisScalCollectionKMeans,

                                        // to be further refined, think of cell counts...
//          SegPresetSeparator4,
//          SegPresetPosTAAHC,
//          SegPresetPosKMeans,

            NumSegPresetParams,

                                        // useful duplicates
            SegPresetParamsDefault      = SegPresetEegSurfaceErpTAAHC,
            SegPresetESIRange1          = SegPresetRisScalErpTAAHC,
            SegPresetESIRange2          = SegPresetRisScalCollectionKMeans,
            };

                                        // Flags associated to parameters
enum        TypeClusteringFlags
            {
            SegPresetClusteringNone         = 0x00000000,
                                        // For Segmentation only:
            SegPresetERP                    = 0x10000000,   // ERP (EEG or ESI)
            SegPresetRSIndiv                = 0x20000000,   // Resting States / Spontaneous Individual (EEG or ESI)
            SegPresetRSGroup                = 0x40000000,   // Resting States / Spontaneous Group (EEG or ESI)
                                        // For Segmentation only:
            SegPresetKMeans                 = 0x00000001,
            SegPresetTAAHC                  = 0x00000002,
                                        // For Segmentation only:
            SegPresetAllData                = 0x00000010,
            SegPresetGfpPeaksOnly           = 0x00000020,

            SegPresetPositiveData           = 0x00000100,
            SegPresetSignedData             = 0x00000200,
            SegPresetVectorData             = 0x00000400,

            SegPresetAccountPolarity        = 0x00001000,
            SegPresetIgnorePolarity         = 0x00002000,

            SegPresetNoRef                  = 0x00010000,
            SegPresetAveRef                 = 0x00020000,

            SegPresetSpatialFilterOn        = 0x00040000,
            SegPresetSpatialFilterOff       = 0x00080000,

            SegPresetZScoreOn               = 0x00100000,
            SegPresetZScoreOff              = 0x00200000,
            SegPresetZScoreDontCare         = 0x00400000,

            SegPresetEnvelopeOn             = 0x01000000,
            SegPresetEnvelopeOff            = 0x02000000,

            SegPresetGfpNormOff             = 0x04000000,
            SegPresetGfpNormOn              = 0x08000000,
            };


enum        TypePostProcFlags
            {
            SegPresetPostProcNone           = 0x00000000,
                                        // For Segmentation only:
            SegPresetSequentializeOn        = 0x00000001,
            SegPresetSequentializeOff       = 0x00000002,

            SegPresetSmoothingOn            = 0x00000010,
            SegPresetSmoothingOff           = 0x00000020,
            SegPresetSmoothingDontCare      = 0x00000040,   // some options could be let "as is"

            SegPresetRejectSmallOn          = 0x00000100,
            SegPresetRejectSmallOff         = 0x00000200,
            SegPresetRejectSmallDontCare    = 0x00000400,   // some options could be let "as is"
                                        // For Segmentation only:
            SegPresetMergeCorrOn            = 0x00001000,
            SegPresetMergeCorrOff           = 0x00002000,
            SegPresetMergeCorrDontCare      = 0x00004000,   // some options could be let "as is"
                                        // For Fitting only:
            SegPresetVariablesNoTime        = 0x10000000,   // restricted variables when time is not relevant
            SegPresetVariablesCyclic        = 0x20000000,   // time relevant, but cyclic as in spontaneous EEG
            SegPresetVariablesAll           = 0x40000000,
            };

                                        // Structure used to fully specify each preset computation parameters
class       SegPresetSpec
{
public:
    SegPresetParamsEnum     Code;                   // redundant but included for clarity
    char                    Text [ 128 ];           // for dialog
    int                     RandomTrials;           // number of random trials
    int                     MinClusters;
    int                     MaxClusters;
    int                     CorrelationThreshold;   // [-100..100), use -100 to turn off
    int                     SmoothingSize;          // Maybe another field for Rejct Size? Also be careful in case of Envelope, is it still needed?
    TypeClusteringFlags     ClusteringFlags;
    TypePostProcFlags       PostProcFlags;
};

extern const SegPresetSpec  SegPresets[ NumSegPresetParams ];


inline  bool    IsSegEEGPreset          ( SegPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, SegPresetEegSurfaceErpTAAHC, SegPresetEegIntraCollectionKMeans ); }
inline  bool    IsSegEEGSurfacePreset   ( SegPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, SegPresetEegSurfaceErpTAAHC, SegPresetEegSurfaceCollectionKMeans ); }
inline  bool    IsSegEEGIntraPreset     ( SegPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, SegPresetEegIntraErpTAAHC,   SegPresetEegIntraCollectionKMeans ); }
inline  bool    IsSegMEGPreset          ( SegPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, SegPresetMegSurfaceErpTAAHC, SegPresetMegSurfaceCollectionKMeans ); }
inline  bool    IsSegESIPreset          ( SegPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, SegPresetESIRange1,          SegPresetESIRange2 ); }
inline  bool    IsSegERPPreset          ( SegPresetParamsEnum preseti )     { return SegPresets [ preseti ].ClusteringFlags & SegPresetERP; }
inline  bool    IsSegRSPreset           ( SegPresetParamsEnum preseti )     { return SegPresets [ preseti ].ClusteringFlags & ( SegPresetRSIndiv | SegPresetRSGroup ); }
inline  bool    IsSegGroupPreset        ( SegPresetParamsEnum preseti )     { return SegPresets [ preseti ].ClusteringFlags & SegPresetRSGroup; }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TMicroStatesSegFilesStruct 
{
public:
                        TMicroStatesSegFilesStruct ();


    TEditData           NumGroups       [ EditSizeValue ];
    TListBoxData        GroupsSummary;

    TRadioButtonData    NoEpochs;
    TRadioButtonData    ListEpochs;
    TComboBoxData       EpochFrom;
    TComboBoxData       EpochTo;
    TRadioButtonData    ResamplingEpochs;
    TEditData           NumResamplingEpochs [ EditSizeValue ];
    TEditData           ResamplingEpochsSize[ EditSizeValue ];
    TEditData           ResamplingCoverage  [ EditSizeValue ];

    TComboBoxData       PresetsFiles;

    TEditData           BaseFileName    [ EditSizeText ];
    TCheckBoxData       WriteSeg;
    TCheckBoxData       WriteMaps;
    TCheckBoxData       WriteClusters;
    TCheckBoxData       WriteSynth;

    TCheckBoxData       ForceSingleFiles;
    TCheckBoxData       BestDir;
    TCheckBoxData       DeleteIndivDirs;
    };


class  TMicroStatesSegParamsStruct 
{
public:
                        TMicroStatesSegParamsStruct ();


    TComboBoxData       PresetsParams;

    TRadioButtonData    PositiveData;
    TRadioButtonData    SignedData;
    TRadioButtonData    VectorData;

    TRadioButtonData    NoRef;
    TRadioButtonData    AveRef;

    TRadioButtonData    AccountPolarity;
    TRadioButtonData    IgnorePolarity;

    TRadioButtonData    KMeans;
    TRadioButtonData    TAAHC;

    TEditData           RandomTrials    [ EditSizeValue ];

    TEditData           MinClusters     [ EditSizeValue ];
    TEditData           MaxClusters     [ EditSizeValue ];

    TRadioButtonData    AllTFs;
    TRadioButtonData    GfpPeaksOnly;
    TRadioButtonData    GfpPeaksAuto;
    TRadioButtonData    GfpPeaksList;
    TEditData           AnalyzeMarkers  [ EditSizeText ];

    TCheckBoxData       SkipBadEpochs;
    TRadioButtonData    SkipBadEpochsAuto;
    TRadioButtonData    SkipBadEpochsList;
    TEditData           SkipMarkers     [ EditSizeText ];

    TCheckBoxData       SpatialFilter;
    TEditData           XyzFile         [ EditSizeText ];

    TCheckBoxData       ClipCorrelation;
    TEditData           MinCorrelation  [ EditSizeValue ];
    TCheckBoxData       DualDataset;

    TCheckBoxData       Sequentialize;

    TCheckBoxData       MergeCorr;
    TEditData           MergeCorrThresh [ EditSizeValue ];

    TCheckBoxData       Smoothing;
    TEditData           WindowSize      [ EditSizeValue ];
    TEditData           BesagFactor     [ EditSizeValue ];
    TCheckBoxData       RejectSmall;
    TEditData           RejectSize      [ EditSizeValue ];

                                                                                                        // !GetSelIndex does not like a const object!
    const SegPresetParamsEnum   GetPresetParamsIndex    ()  const   { return  (const SegPresetParamsEnum) ((TComboBoxData)PresetsParams).GetSelIndex (); }
    bool                        IsSegESIPreset          ()  const   { return  crtl::IsSegESIPreset ( GetPresetParamsIndex () ); }
};


                                        // Joining the 2 structures together
class   TMicroStatesSegStruct   :   public  TMicroStatesSegFilesStruct,
                                    public  TMicroStatesSegParamsStruct
{
public:
                    TMicroStatesSegStruct ();


    int             LastDialogId;
};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Common to all segmentation dialogs
enum        AtomType;
enum        PolarityType;
enum        DualDataType;
enum        ZScoreType;
enum        EpochsType;
enum        GfpPeaksDetectType;
enum        SkippingEpochsType;
enum        ResamplingType;
enum        CentroidType;
class       TStrings;


class   TMicroStatesSegDialog   :   public  TBaseDialog,
                                    public  TMicroStates
{
public:
                        TMicroStatesSegDialog ( owl::TWindow* parent, owl::TResId resId );
//                     ~TMicroStatesSegDialog ();

    static TGoGoF   GoGoF;


protected:

    bool                XyzAndEegMatch;
    bool                GroupsAllRis;
    double              SamplingFrequency;


    void                CmOk                    ();
    virtual void        CmOkEnable              ( owl::TCommandEnabler &tce );
    void                CmToDialog              ( owlwparam w );


    DECLARE_RESPONSE_TABLE ( TMicroStatesSegDialog );
};


//----------------------------------------------------------------------------
class   TMicroStatesSegFilesDialog  :   public  TMicroStatesSegDialog
{
public:
                        TMicroStatesSegFilesDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TMicroStatesSegFilesDialog ();


protected:
    owl::TEdit          *NumGroups;
    owl::TListBox       *GroupsSummary;

    owl::TRadioButton   *NoEpochs;
    owl::TRadioButton   *ListEpochs;
    owl::TComboBox      *EpochFrom;
    owl::TComboBox      *EpochTo;
    owl::TRadioButton   *ResamplingEpochs;
    owl::TEdit          *NumResamplingEpochs;
    owl::TEdit          *ResamplingEpochsSize;
    owl::TEdit          *ResamplingCoverage;

    owl::TComboBox      *PresetsFiles;

    owl::TEdit          *BaseFileName;
    owl::TCheckBox      *WriteSeg;
    owl::TCheckBox      *WriteMaps;
    owl::TCheckBox      *WriteClusters;
    owl::TCheckBox      *WriteSynth;

    owl::TCheckBox      *ForceSingleFiles;
    owl::TCheckBox      *BestDir;
    owl::TCheckBox      *DeleteIndivDirs;


    bool                LockResampling;


    void                SetupWindow ();

    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TMicroStatesSegDialog::CmOkEnable ( tce ); }

    void                EvPresetsChange         ();
    bool                CheckTracksGroup        ( const TGoF& gof );
    void                AddFileToGroup          ( const char* filename, bool first );
    void                AddGroupSummary         ( int gofi, bool updatebasefilename = true );
    void                SetBaseFilename         ();
    int                 GetNumData              ()  const;
    bool                IsGfpMax                ()  const;
    void                SampleSizeToNumResampling ();
    void                UpdateResamplingCoverage();
    void                UpdateNumResampling     ();
    void                UpdateSampleSize        ();
    void                SetSamplingFrequency    ();

    void                CmUpOneDirectory        ();
    void                CmBrowseBaseFileName    ();

    void                CmAddGroup              ();
    void                CmRemoveGroup           ();
    void                CmClearGroups           ();
    void                CmSortGroups            ();
    void                EvDropFiles             ( owl::TDropInfo drop );
    void                CmReadParams            ();
    void                ReadParams              ( char *filename = 0 );
    void                CmWriteParams           ();

    void                CmAddEpoch              ();
    void                CmRemoveEpoch           ();
    void                CmListEpochsEnable      ( owl::TCommandEnabler &tce );
    void                CmResamplingEpochsEnable( owl::TCommandEnabler &tce );
    
    void                CmDeleteIndivDirsEnable ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TMicroStatesSegFilesDialog );
};


//----------------------------------------------------------------------------

class   TMicroStatesSegParamsDialog :   public  TMicroStatesSegDialog
{
public:
                        TMicroStatesSegParamsDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TMicroStatesSegParamsDialog ();


protected:
    owl::TComboBox      *PresetsParams;

    owl::TRadioButton   *PositiveData;
    owl::TRadioButton   *SignedData;
    owl::TRadioButton   *VectorData;

    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *AveRef;

    owl::TRadioButton   *AccountPolarity;
    owl::TRadioButton   *IgnorePolarity;

    owl::TRadioButton   *KMeans;
    owl::TRadioButton   *TAAHC;

    owl::TEdit          *RandomTrials;

    owl::TEdit          *MinClusters;
    owl::TEdit          *MaxClusters;

    owl::TRadioButton   *AllTFs;
    owl::TRadioButton   *GfpPeaksOnly;
    owl::TRadioButton   *GfpPeaksAuto;
    owl::TRadioButton   *GfpPeaksList;
    owl::TEdit          *AnalyzeMarkers;

    owl::TCheckBox      *SkipBadEpochs;
    owl::TRadioButton   *SkipBadEpochsAuto;
    owl::TRadioButton   *SkipBadEpochsList;
    owl::TEdit          *SkipMarkers;

    owl::TCheckBox      *SpatialFilter;
    owl::TEdit          *XyzFile;

    owl::TCheckBox      *ClipCorrelation;
    owl::TEdit          *MinCorrelation;
    owl::TCheckBox      *DualDataset;

    owl::TCheckBox      *Sequentialize;

    owl::TCheckBox      *MergeCorr;
    owl::TEdit          *MergeCorrThresh;

    owl::TCheckBox      *Smoothing;
    owl::TEdit          *WindowSize;
    owl::TEdit          *BesagFactor;
    owl::TCheckBox      *RejectSmall;
    owl::TEdit          *RejectSize;


    void                SetupWindow ();

    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TMicroStatesSegDialog::CmOkEnable ( tce ); }

    void                EvPresetsChange         ();
    void                CmKMeansEnable          ( owl::TCommandEnabler &tce );
    void                CmHierarchicalEnable    ( owl::TCommandEnabler &tce );
    void                CmSmoothingEnable       ( owl::TCommandEnabler &tce );
    void                CmRejectSmallEnable     ( owl::TCommandEnabler &tce );
    void                SmoothingWindowsSizeChanged ();
    void                CmMergeCorrEnable       ( owl::TCommandEnabler &tce );
    void                CmMinCorrelationEnable  ( owl::TCommandEnabler &tce );

    void                EvDropFiles             ( owl::TDropInfo drop );

    void                CmBrowseXyzFile         ();
    void                SetXyzFile              ( char *file );
    void                CheckXyzFile            ();
    void                CmXyzEnable             ( owl::TCommandEnabler &tce );
    void                CmGfpPeaksEnable        ( owl::TCommandEnabler &tce );
    void                CmGfpPeaksListEnable    ( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsEnable   ( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsListEnable ( owl::TCommandEnabler &tce );

    void                CmNotESIEnable          ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TMicroStatesSegParamsDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
