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

#include    "Files.TGoF.h"

#include    "TMicroStates.h"
#include    "TMicroStatesSegDialog.h"   // ClusteringFlags TypePostProcFlags

#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
// Here we have the definition of the 3 Dialogs needed to control the EEG Micro-States back-fitting:
//  - One for the input files options
//  - One for the processing parameters
//  - One for the output options
//----------------------------------------------------------------------------

constexpr char*     FittingTitle                = "Fitting";

                                        // File infixes specific to Fitting
                                        // Kept as defines and not constexpr for ease of concatenation
#define             InfixSegmentation           "Segmentations"
#define             InfixCorrelation            "Correlations"
#define             InfixDurations              "Histograms Durations"
#define             InfixSegFrequency           "Segments Frequency"
#define             InfixMarkov                 "Markov Chains"
#define             InfixMarkovTransMatrix      "MTM"
#define             InfixJointStateProb         "JSP"
#define             InfixStepAhead              "Ahead"
#define             InfixObserved               "Observed"
#define             InfixExpected               "Expected"
#define             InfixCount                  "Count"
#define             InfixProb                   "Prob"


//----------------------------------------------------------------------------
                                        // Presets:

                                        // Enumerate all presets
enum        FitPresetParamsEnum
            {
            FitPresetEegSurfaceErp,
            FitPresetEegSurfaceSpont,
            FitPresetEegSurfaceSpontGfpPeaks,
            FitPresetEegSurfaceSpontTemplates,

            FitPresetSeparator1,
            FitPresetEegIntraErp,
            FitPresetEegIntraSpont,
            FitPresetEegIntraSpontGfpPeaks,
            FitPresetEegIntraSpontTemplates,

            FitPresetSeparator2,
            FitPresetMegSurfaceErp,
            FitPresetMegSurfaceSpont,
            FitPresetMegSurfaceSpontGfpPeaks,
            FitPresetMegSurfaceSpontTemplates,

            FitPresetSeparator3,
            FitPresetRisScalErp,
            FitPresetRisScalSpont,
            FitPresetRisScalSpontGfpPeaks,
            FitPresetRisScalSpontTemplates,

            NumFitPresets,

                                        // useful duplicates
            FitPresetParamsDefault      = FitPresetEegSurfaceErp,
            FitPresetESIRange1          = FitPresetRisScalErp,
            FitPresetESIRange2          = FitPresetRisScalSpontTemplates,
            };

                                        // Structure used to fully specify each preset computation parameters
class       FitPresetSpec
{
public:
    FitPresetParamsEnum     Code;                   // redundant but included for clarity
    char                    Text [ 128 ];           // for dialog
    int                     CorrelationThreshold;   // [-100..100), use -100 to turn off
    int                     SmoothingSize;          // & RejectSmall size too
    TypeClusteringFlags     ClusteringFlags;
    TypePostProcFlags       PostProcFlags;
};

extern const FitPresetSpec  FitPresets[ NumFitPresets ];


inline  bool    IsFitEEGPreset  ( FitPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, FitPresetEegSurfaceErp, FitPresetEegIntraSpontTemplates ); }
inline  bool    IsFitMEGPreset  ( FitPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, FitPresetMegSurfaceErp, FitPresetMegSurfaceSpontTemplates ); }
inline  bool    IsFitESIPreset  ( FitPresetParamsEnum preseti )     { return IsInsideLimits ( preseti, FitPresetESIRange1, FitPresetESIRange2 ); }
inline  bool    IsFitERPPreset  ( FitPresetParamsEnum preseti )     { return FitPresets [ preseti ].ClusteringFlags & SegPresetERP; }
inline  bool    IsFitRSPreset   ( FitPresetParamsEnum preseti )     { return FitPresets [ preseti ].ClusteringFlags & ( SegPresetRSIndiv | SegPresetRSGroup ); }


//----------------------------------------------------------------------------
                                        // Default missing value in output tables
constexpr double    FittingMissingValue         = -1;

                                        // Kept as defines and not constexpr for ease of concatenation
#define             FitSubjectNameLong          "Subject"
#define             FitGroupNameLong            "Group"
#define             FitGroupNameShort           "G"
#define             FitMapNameLong              "Map"
#define             FitMapNameShort             "M"
#define             FitFromMapNameLong          "FromMap"
#define             FitFromMapNameShort         "FM"
#define             FitToMapNameLong            "ToMap"
#define             FitToMapNameShort           "TM"
#define             FitUnlabeledNameLong        "Unlabel"
#define             FitUnlabeledNameShort       "U"


enum        FitVarType
            {
            fitfonset,
            fitloffset,
            fitnumtf,
            fittfcentroid,
            fitmeancorr,
            fitgev,
            fitbcorr,
            fittfbcorr,
            fitgfptfbcorr,
            fitmaxgfp,
            fittfmaxgfp,
            fitmeangfp,
            fitmeanduration,
            fittimecoverage,
            fitsegdensity,

            fitnumvar  
            };


extern const char   FitVarNames[ 2 ][ fitnumvar ][ 32 ];


//----------------------------------------------------------------------------

enum        GaugeFitEnum
            {
            gaugefitglobal,
            gaugefitinit,
            gaugefitlabeling,
            gaugefitrejectsmall,
            gaugefitextractvariables,
            gaugefitsegments,
            gaugefitwritecorr,
            gaugefitwriteseg,
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TMicroStatesFitFilesStruct
{
public:
                        TMicroStatesFitFilesStruct ();


    TEditData           TemplateFileName[ EditSizeText ];

    TEditData           NumGroups[ EditSizeValue ];
    TListBoxData        GroupsSummary;
    TEditData           NumWithinSubjects[ EditSizeValue ];
    TEditData           MaxWithinSubjects[ EditSizeValue ];

    TComboBoxData       EpochFrom;
    TComboBoxData       EpochTo;
    TComboBoxData       EpochMaps;

    TEditData           BaseFileName[ EditSizeText ];

    TCheckBoxData       WriteSegFiles;
    TCheckBoxData       WriteClusters;
    TCheckBoxData       DeletePreProcFiles;

                                        // from TemplateFileName to BaseFileName
    void                ComposeBaseFilename     ( char* bigmatch )  const;
};


class  TMicroStatesFitParamsStruct
{
public:
                        TMicroStatesFitParamsStruct ();


    TComboBoxData       PresetsParams;

    TCheckBoxData       SpatialFilter;
    TEditData           XyzFile         [ EditSizeText ];

    TRadioButtonData    NormalizationNone;
    TRadioButtonData    NormalizationGfp;

    TCheckBoxData       SkipBadEpochs;
    TRadioButtonData    SkipBadEpochsAuto;
    TRadioButtonData    SkipBadEpochsList;
    TEditData           SkipMarkers     [ EditSizeText ];

    TRadioButtonData    PositiveData;
    TRadioButtonData    SignedData;
    TRadioButtonData    VectorData;

    TRadioButtonData    NoRef;
    TRadioButtonData    AveRef;

    TRadioButtonData    AccountPolarity;
    TRadioButtonData    IgnorePolarity;

    TCheckBoxData       FitSingleSeg;

    TCheckBoxData       ClipCorrelation;
    TEditData           MinCorrelation  [ EditSizeValue ];

    TCheckBoxData       Smoothing;
    TEditData           WindowSize      [ EditSizeValue ];
    TEditData           BesagFactor     [ EditSizeValue ];

    TCheckBoxData       RejectSmall;
    TEditData           RejectSize      [ EditSizeValue ];

                                                                                                        // !GetSelIndex does not like a const object!
    const FitPresetParamsEnum   GetPresetParamsIndex    ()  const   { return (const FitPresetParamsEnum) ((TComboBoxData)PresetsParams).GetSelIndex (); }
    bool                        IsFitESIPreset          ()  const   { return crtl::IsFitESIPreset ( GetPresetParamsIndex () ); }
};


class  TMicroStatesFitResultsStruct
{
public:
                        TMicroStatesFitResultsStruct ();


    TCheckBoxData       FOnset;
    TCheckBoxData       LOffset;
    TCheckBoxData       NumTf;
    TCheckBoxData       TfCentroid;
    TCheckBoxData       MeanCorr;
    TCheckBoxData       Gev;
    TCheckBoxData       BestCorr;
    TCheckBoxData       TfBestCorr;
    TCheckBoxData       GfpTfBestCorr;
    TCheckBoxData       MaxGfp;
    TCheckBoxData       TfMaxGfp;
    TCheckBoxData       MeanGfp;
    TCheckBoxData       MeanDuration;
    TCheckBoxData       TimeCoverage;
    TCheckBoxData       SegDensity;

    TCheckBoxData       LinesAsSamples;
    TCheckBoxData       ColumnsAsFactors;
    TCheckBoxData       OneFilePerGroup;
    TCheckBoxData       OneFilePerVariable;
    TCheckBoxData       ShortVariableNames;

    TCheckBoxData       MarkovSegment;
    TEditData           MarkovNumTransitions[ EditSizeValue ];

    TCheckBoxData       WriteStatDurations;
    TCheckBoxData       WriteCorrFiles;
    TCheckBoxData       WriteSegFrequency;
};

                                        // Joining the 3 structures together
class   TMicroStatesFitStruct   :   public  TMicroStatesFitFilesStruct,
                                    public  TMicroStatesFitParamsStruct,
                                    public  TMicroStatesFitResultsStruct
{
public:
                        TMicroStatesFitStruct   ();


    int                 LastDialogId;


    void                SetVariablesOn          ();
    void                SetVariablesOff         ();
    void                SetNonCompetitive       ();
    void                SetVariables            ( const FitPresetSpec& preset );
};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Common to all fitting dialogs
class   TMicroStatesFitDialog   :   public  TBaseDialog,
                                    public  TMicroStates
{
public:
                        TMicroStatesFitDialog   ( owl::TWindow* parent, owl::TResId resId );
//                     ~TMicroStatesFitDialog   ();


    static TGoGoF       GoGoF;


protected:

    bool                XyzAndEegMatch;
    bool                GroupsAllRis;


    int                 MaxFilesPerGroup;
    int                 MaxGroupsWithinSubjects;
    bool                AllGroupsEqual;
    void                CheckXyzFile ();


    void                SetMaxFilesPerGroup     ( int gofi1 = -1, int gofi2 = -1 );
    void                SetMaxGroupsWithinSubjects ( bool updateNumWithinSubjects = true );
    void                SetAllGroupsEqual       ();


    void                CmOk                    ();
    virtual void        CmOkEnable              ( owl::TCommandEnabler &tce );
    void                CmToDialog              ( owlwparam w );

    virtual void        CmVariableEnable        ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE ( TMicroStatesFitDialog );
};


//----------------------------------------------------------------------------
class   TMicroStatesFitFilesDialog  :   public  TMicroStatesFitDialog
{
public:
                        TMicroStatesFitFilesDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TMicroStatesFitFilesDialog ();


protected:
    owl::TEdit          *TemplateFileName;

    owl::TEdit          *NumGroups;
    owl::TListBox       *GroupsSummary;

public:                                 // SetMaxGroupsWithinSubjects needs access
    owl::TEdit          *NumWithinSubjects;
    owl::TEdit          *MaxWithinSubjects;

protected:
    owl::TComboBox      *EpochFrom;
    owl::TComboBox      *EpochTo;
    owl::TComboBox      *EpochMaps;

    owl::TEdit          *BaseFileName;

    owl::TCheckBox      *WriteSegFiles;
    owl::TCheckBox      *WriteClusters;
    owl::TCheckBox      *DeletePreProcFiles;


    void                SetupWindow             ()  final;

    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmOkEnable       ( tce ); }
    void                CmVariableEnable        ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmVariableEnable ( tce ); }

    void                SetEnumEpochs           ( int maxtf = 0 );
    bool                CheckTracksGroup        ( const TGoF& gof );
    void                AddFileToGroup          ( const char* filename, bool first );
    void                AddGroupSummary         ( int gofi );
    void                SetBaseFilename         ();


    void                EvDropFiles             ( owl::TDropInfo drop );

    void                CmBrowseTemplateFileName();
    void                SetTemplateFileName     ( const char* file );
    void                CmUpOneDirectory        ();
    void                CmBrowseOutputFileName  ();

    void                CmClearEpochs           ();
    void                CmAddEpoch              ();
    void                CmRemoveEpoch           ();
    void                CmAddEpochEnable        ( owl::TCommandEnabler &tce );

    void                CmAddGroup              ();
    void                CmRemoveGroup           ();
    void                CmClearGroups           ();
    void                CmSortGroups            ();
    void                CmReadParams            ();
    void                ReadParams              ( const char* filename = 0 );
    void                CmWriteParams           ();

    DECLARE_RESPONSE_TABLE ( TMicroStatesFitFilesDialog );
};


//----------------------------------------------------------------------------
class   TMicroStatesFitParamsDialog :   public  TMicroStatesFitDialog
{
public:
                        TMicroStatesFitParamsDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TMicroStatesFitParamsDialog ();


protected:
    owl::TComboBox      *Presets;

    owl::TCheckBox      *SpatialFilter;
    owl::TEdit          *XyzFile;

    owl::TRadioButton   *NormalizationNone;
    owl::TRadioButton   *NormalizationGfp;

    owl::TCheckBox      *SkipBadEpochs;
    owl::TRadioButton   *SkipBadEpochsAuto;
    owl::TRadioButton   *SkipBadEpochsList;
    owl::TEdit          *SkipMarkers;

    owl::TRadioButton   *PositiveData;
    owl::TRadioButton   *SignedData;
    owl::TRadioButton   *VectorData;

    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *AveRef;

    owl::TRadioButton   *AccountPolarity;
    owl::TRadioButton   *IgnorePolarity;

    owl::TCheckBox      *FitSingleSeg;

    owl::TCheckBox      *ClipCorrelation;
    owl::TEdit          *MinCorrelation;

    owl::TCheckBox      *Smoothing;
    owl::TEdit          *WindowSize;
    owl::TEdit          *BesagFactor;

    owl::TCheckBox      *RejectSmall;
    owl::TEdit          *RejectSize;


    void                SetupWindow             ()  final;

    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmOkEnable       ( tce ); }
    void                CmVariableEnable        ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmVariableEnable ( tce ); }

    void                EvDropFiles             ( owl::TDropInfo drop );

    void                EvPresetsChange         ();
    void                CmSmoothingEnable       ( owl::TCommandEnabler &tce );
    void                CmRejectSmallEnable     ( owl::TCommandEnabler &tce );
    void                SmoothingWindowsSizeChanged ();

    void                CmReferenceEnable       ( owl::TCommandEnabler &tce );
    void                CmPolarityEnable        ( owl::TCommandEnabler &tce );
    void                CmMinCorrelationEnable  ( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsEnable   ( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsListEnable ( owl::TCommandEnabler &tce );

    void                CmCheckXyzFile          ();
    void                CmBrowseXyzFile         ();
    void                SetXyzFile              ( const char* file );
    void                CmXyzEnable             ( owl::TCommandEnabler &tce );

    void                CmNotESIEnable          ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TMicroStatesFitParamsDialog );
};


//----------------------------------------------------------------------------
class   TMicroStatesFitResultsDialog    :   public  TMicroStatesFitDialog
{
public:
                        TMicroStatesFitResultsDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TMicroStatesFitResultsDialog ();


protected:
    owl::TCheckBox      *FOnset;
    owl::TCheckBox      *LOffset;
    owl::TCheckBox      *NumTf;
    owl::TCheckBox      *TfCentroid;
    owl::TCheckBox      *MeanCorr;
    owl::TCheckBox      *Gev;
    owl::TCheckBox      *BestCorr;
    owl::TCheckBox      *TfBestCorr;
    owl::TCheckBox      *GfpTfBestCorr;
    owl::TCheckBox      *MaxGfp;
    owl::TCheckBox      *TfMaxGfp;
    owl::TCheckBox      *MeanGfp;
    owl::TCheckBox      *MeanDuration;
    owl::TCheckBox      *TimeCoverage;
    owl::TCheckBox      *SegDensity;

    owl::TCheckBox      *LinesAsSamples;
    owl::TCheckBox      *ColumnsAsFactors;
    owl::TCheckBox      *OneFilePerGroup;
    owl::TCheckBox      *OneFilePerVariable;
    owl::TCheckBox      *ShortVariableNames;

    owl::TCheckBox      *MarkovSegment;
    owl::TEdit          *MarkovNumTransitions;

    owl::TCheckBox      *WriteStatDurations;
    owl::TCheckBox      *WriteCorrFiles;
    owl::TCheckBox      *WriteSegFrequency;


    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmOkEnable       ( tce ); }
    void                CmVariableEnable        ( owl::TCommandEnabler &tce )   { TMicroStatesFitDialog::CmVariableEnable ( tce ); }

    void                CmAllOnOff              ( owlwparam w );
    void                CmReferenceEnable       ( owl::TCommandEnabler &tce );
    void                CmPolarityEnable        ( owl::TCommandEnabler &tce );
    void                CmMinCorrelationEnable  ( owl::TCommandEnabler &tce );

    void                CmMarkovEnable          ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TMicroStatesFitResultsDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
