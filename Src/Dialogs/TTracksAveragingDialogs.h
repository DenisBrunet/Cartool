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

#include    "System.h"

#include    "Strings.TStrings.h"
#include    "Time.TTimer.h"
#include    "TTracksFilters.h"

#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
// Here are declared 4 + 1 dialogs to control the interactive ERP processing:
//  - 3 dialogs to set the inputs files, parameters and types of output
//  - 1 base dialog common to these 3
//  - 1 stand-alone dialog that controls the interactive averaging process
//----------------------------------------------------------------------------

enum        {
            AvgPresetErpEegSurface,
            AvgPresetErpEegIntra,
            AvgPresetErpFreqPower,
            AvgPresetErpFreqFftapprox,
            AvgPresetErpRisScal,

            AvgPresetSeparator1,

            AvgPresetGrandaverageErpSurface,
            AvgPresetGrandaverageErpIntra,
            AvgPresetGrandaverageFreqPower,
            AvgPresetGrandaverageFreqFftapprox,
            AvgPresetGrandaverageRisScal,
            NumAvgPresets
            };

extern const char   AvgPresets[ NumAvgPresets ][ 128 ];


constexpr double    BadEpochExtraMarginMilliseconds     = 100.0;
                                        // Half decrease from lowest frequency summit (biggest half of a half period):  acos ( 0.50 ) / ( 2 * Pi ) = 1 / 6
constexpr double    HighPassAdditionalMargin            = 1.0 / 6.0;

                                        // Various temp markers names used during interactive computation
constexpr char*     MessageEpochSeemsBad    = "BAD EPOCH?";
constexpr char*     MessageEpochSeemsOk     = "EPOCH OK?";
constexpr char*     MessageEpochBadEpoch    = "BAD EPOCH REJECTED";
constexpr char*     MessageEpochBadTva      = "TVA REJECTED";
constexpr char*     MessageEpochBadAuto     = "AUTO REJECTED";
constexpr char*     MessageEpochBadUser     = "USER REJECTED";
constexpr char*     MessageEpochOkUser      = "USER ACCEPTED";


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TTracksAveragingFilesStruct
{
public:
                        TTracksAveragingFilesStruct ();


    TComboBoxData       ComboEeg;
    TComboBoxData       ComboSes;
    TComboBoxData       ComboTva;
    TComboBoxData       ComboTrg;

    TCheckBoxData       UseEvents;
    TCheckBoxData       UseTriggers;
    TCheckBoxData       UseMarkers;

    TEditData           XyzDocFile   [ EditSizeText ];
    TEditData           BaseFileName [ EditSizeText ];


    int                 GetNumFiles ()          { return ComboEeg.GetStrings().GetItemsInContainer(); }
};


//----------------------------------------------------------------------------

class   TTracksAveragingParamsStruct
{
public:
                        TTracksAveragingParamsStruct    ();


    TComboBoxData       Presets;

    TCheckBoxData       CheckEl;
    TEditData           ElThreshold  [ EditSizeValue ];
    TEditData           ElExcluded   [ EditSizeText ];

    TCheckBoxData       CheckAux;
    TEditData           AuxThreshold [ EditSizeValue ];
    TEditData           AuxOverride  [ EditSizeText ];

    TRadioButtonData    TriggerFixedTF;
    TRadioButtonData    Trigger;
    TRadioButtonData    TriggerOffset;
    TRadioButtonData    TriggerReactionTime;
    TEditData           TriggerOffsetValue      [ EditSizeValue ];
    TEditData           TriggerOffsetValueMs    [ EditSizeValue ];
    TEditData           TriggerFixedTFValue     [ EditSizeValue ];
    TEditData           TriggerFixedTFValueMs   [ EditSizeValue ];

    TEditData           DurationPre     [ EditSizeValue ];
    TEditData           DurationPreMs   [ EditSizeValue ];
    TEditData           DurationPost    [ EditSizeValue ];
    TEditData           DurationPostMs  [ EditSizeValue ];
    TEditData           DurationTotal   [ EditSizeValue ];
    TEditData           DurationTotalMs [ EditSizeValue ];

    TRadioButtonData    SignedData;
    TRadioButtonData    PositiveData;
    TRadioButtonData    NoRef;
    TRadioButtonData    AvgRef;
    TRadioButtonData    AccountPolarity;
    TRadioButtonData    IgnorePolarity;

    TCheckBoxData       SetFilters;
    TCheckBoxData       FilterData;
    TCheckBoxData       FilterDisplay;

    TCheckBoxData       BaselineCorr;
    TEditData           BaselineCorrPre     [ EditSizeValue ];
    TEditData           BaselineCorrPreMs   [ EditSizeValue ];
    TEditData           BaselineCorrPost    [ EditSizeValue ];
    TEditData           BaselineCorrPostMs  [ EditSizeValue ];

    TCheckBoxData       SkipBadEpochs;
    TEditData           SkipMarkers     [ EditSizeText ];

    TCheckBoxData       AcceptAll;
};


//----------------------------------------------------------------------------

class   TTracksAveragingOutputsStruct
{
public:
                        TTracksAveragingOutputsStruct   ();


    TCheckBoxData       MergeTriggers;
    TCheckBoxData       SplitTriggers;
    TCheckBoxData       SaveAverage;
    TCheckBoxData       SaveSum;
    TCheckBoxData       SaveSD;
    TCheckBoxData       SaveSE;
    TCheckBoxData       SaveEpochsMultiFiles;
    TCheckBoxData       SaveEpochsSingleFile;

    TComboBoxData       FileTypes;

    TCheckBoxData       SaveExcluded;
    TCheckBoxData       SaveAux;

    TCheckBoxData       OpenAuto;
    TCheckBoxData       OriginMarker;
};


//----------------------------------------------------------------------------
                                        // Grouping all parameters together
class   TTracksAveragingStruct  :   public  TTracksAveragingFilesStruct,
                                    public  TTracksAveragingParamsStruct,
                                    public  TTracksAveragingOutputsStruct
{
public:
                            TTracksAveragingStruct  ();


    int                     LastDialogId;
    bool                    AllFilesOk;

    TTracksFilters<float>   Filters;
//  double                  DefaultSamplingFrequency;
    double                  SamplingFrequency;
    int                     MaxNumTF;

};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Common to all dialogs
class   TGoGoF;
class   TTracksView;


class   TTracksAveragingBaseDialog  :   public  TBaseDialog
{
public:
                    TTracksAveragingBaseDialog  ( owl::TWindow* parent, int resId );
                   ~TTracksAveragingBaseDialog  ();


protected:
    void            ProcessGroups       ( TGoGoF* gogof, int gofi1, int gofi2, void* usetransfer );


    void            CmOk                ();
    void            CmOkEnable          ( owl::TCommandEnabler &tce );
    void            CmToDialog          ( owlwparam w );

    DECLARE_RESPONSE_TABLE ( TTracksAveragingBaseDialog );
};


//----------------------------------------------------------------------------
                                        // Panel that controls all files input/output settings
class   TTracksAveragingFilesDialog :   public  TTracksAveragingBaseDialog
{
public:
                    TTracksAveragingFilesDialog ( owl::TWindow* parent, int resId );
                   ~TTracksAveragingFilesDialog ();


protected:
    owl::TComboBox      *ComboEeg;
    owl::TComboBox      *ComboSes;
    owl::TComboBox      *ComboTva;
    owl::TComboBox      *ComboTrg;

    owl::TCheckBox      *UseEvents;
    owl::TCheckBox      *UseTriggers;
    owl::TCheckBox      *UseMarkers;

    owl::TEdit          *XyzDocFile;
    owl::TEdit          *BaseFileName;

    int                  SelIndex[ 4 ];      // 4 columns
    owl::TComboBox      *ComboWithFocus;


    bool                CheckFilesCompatibility ( bool silent );
    void                SetBaseFilename         ();
    void                GuessTriggerType        ();


    void                CmAddToList             ();
    void                CmRemoveFromList        ();
    void                CmClearLists            ();
    void                CmSortLists             ();
    void                CmReadParams            ();
    void                ReadParams              ( char *filename = 0 );
    void                CmWriteParams           ();

    void                CmReplaceField          ();
    void                CmMyOk                  ();
    void                CmBrowseEeg             ();
    void                CmBrowseTva             ();
    void                CmBrowseXyz             ();
    void                EvCbnSelChange          ();
    void                EvCbnSetFocus           ();
    void                CmReplaceFieldEnable    ( owl::TCommandEnabler &tce );
    void                EvXyzFileChange         ();
//  void                EvEnKillFocus           ();
    void                EvDropFiles             ( owl::TDropInfo drop );
//  void                CmToNextDialogEnable    ( owl::TCommandEnabler &tce );
    void                CmUpOneDirectory        ();

    DECLARE_RESPONSE_TABLE (TTracksAveragingFilesDialog);
};


//----------------------------------------------------------------------------
                                        // Panel that controls all processing parameters
class   TTracksAveragingParamsDialog :  public  TTracksAveragingBaseDialog
{
public:
                        TTracksAveragingParamsDialog    ( owl::TWindow* parent, int resId );
                       ~TTracksAveragingParamsDialog    ();


protected:
    owl::TComboBox      *Presets;

    owl::TCheckBox      *CheckEl;
    owl::TEdit          *ElThreshold;
    owl::TEdit          *ElExcluded;

    owl::TCheckBox      *CheckAux;
    owl::TEdit          *AuxThreshold;
    owl::TEdit          *AuxOverride;

    owl::TRadioButton   *TriggerFixedTF;
    owl::TRadioButton   *Trigger;
    owl::TRadioButton   *TriggerOffset;
    owl::TRadioButton   *TriggerReactionTime;
    owl::TEdit          *TriggerOffsetValue;
    owl::TEdit          *TriggerOffsetValueMs;
    owl::TEdit          *TriggerFixedTFValue;
    owl::TEdit          *TriggerFixedTFValueMs;

    owl::TEdit          *DurationPre;
    owl::TEdit          *DurationPreMs;
    owl::TEdit          *DurationPost;
    owl::TEdit          *DurationPostMs;
    owl::TEdit          *DurationTotal;
    owl::TEdit          *DurationTotalMs;

    owl::TRadioButton   *SignedData;
    owl::TRadioButton   *PositiveData;
    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *AvgRef;
    owl::TRadioButton   *AccountPolarity;
    owl::TRadioButton   *IgnorePolarity;

    owl::TCheckBox      *SetFilters;
    owl::TCheckBox      *FilterData;
    owl::TCheckBox      *FilterDisplay;

    owl::TCheckBox      *BaselineCorr;
    owl::TEdit          *BaselineCorrPre;
    owl::TEdit          *BaselineCorrPreMs;
    owl::TEdit          *BaselineCorrPost;
    owl::TEdit          *BaselineCorrPostMs;

    owl::TCheckBox      *SkipBadEpochs;
    owl::TEdit          *SkipMarkers;

    owl::TCheckBox      *AcceptAll;


    void                SetupWindow                     ();
    char*               TfToMs                          ( int tf, char *buff );

    void                EvPresetsChange                 ();
    void                EvFiltersChanged                ();
    void                CmSetFiltersEnable              ( owl::TCommandEnabler &tce );
    void                CmFiltersEnable                 ( owl::TCommandEnabler &tce );
    void                CmTriggerOffsetEnable           ( owl::TCommandEnabler &tce );
    void                EvEditPrePostChanged            ();
    void                CmSO1TFEnable                   ( owl::TCommandEnabler &tce );
    void                CmBaseLineCorrectionEnable      ( owl::TCommandEnabler &tce );
    void                CmBaseLineCorrectionParamsEnable( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsListEnable       ( owl::TCommandEnabler &tce );
    void                EvIgnorePolarityChanged         ();
    void                EvGetFiles                      ();
    void                EvTFChanged                     ();
    void                CmCheckElEnable                 ( owl::TCommandEnabler &tce );
    void                CmCheckAuxEnable                ( owl::TCommandEnabler &tce );
    void                EvDropFiles                     ( owl::TDropInfo drop );
    void                CmReadParams                    ();
    void                ReadParams                      ( char *file );
//  void                CmToNextDialogEnable            ( owl::TCommandEnabler &tce );
    void                CmAvgRefEnable                  ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE (TTracksAveragingParamsDialog);
};


//----------------------------------------------------------------------------
                                        // Panel that controls all outputs parameters
class   TTracksAveragingOutputsDialog :  public  TTracksAveragingBaseDialog
{
public:
                        TTracksAveragingOutputsDialog   ( owl::TWindow* parent, int resId );
                       ~TTracksAveragingOutputsDialog   ();

protected:
    owl::TCheckBox      *MergeTriggers;
    owl::TCheckBox      *SplitTriggers;
    owl::TCheckBox      *SaveAverage;
    owl::TCheckBox      *SaveSum;
    owl::TCheckBox      *SaveSD;
    owl::TCheckBox      *SaveSE;
    owl::TCheckBox      *SaveEpochsMultiFiles;
    owl::TCheckBox      *SaveEpochsSingleFile;

    owl::TComboBox      *FileTypes;

    owl::TCheckBox      *SaveExcluded;
    owl::TCheckBox      *SaveAux;

    owl::TCheckBox      *OpenAuto;
    owl::TCheckBox      *OriginMarker;


    void                CmSplitTriggersEnable ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE (TTracksAveragingOutputsDialog);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TTracksAveragingControlStruct
{
public:
                    TTracksAveragingControlStruct   ();


    TEditData       CurrentFile     [ EditSizeText ];

    TEditData       TriggerCode     [ EditSizeText ];
    TEditData       TriggerPos      [ EditSizeValue ];
    TEditData       TriggerIndex    [ EditSizeValue ];

    TEditData       AcceptNumTF     [ EditSizeValue ];
    TEditData       RejectNumTF     [ EditSizeValue ];

    TEditData       NumTriggersAccepted [ EditSizeValue ];
    TEditData       NumTriggersRejected [ EditSizeValue ];

    TEditData       TrackName       [ EditSizeText ];
    TEditData       TrackMin        [ EditSizeValue ];
    TEditData       TrackMax        [ EditSizeValue ];
    TEditData       Threshold       [ EditSizeValue ];
    TEditData       AuxName         [ EditSizeText ];
    TEditData       AuxMin          [ EditSizeValue ];
    TEditData       AuxMax          [ EditSizeValue ];
    TEditData       AuxThreshold    [ EditSizeValue ];

    TCheckBoxData   TrackZoom;
    TEditData       TracksToExclude [ EditSizeText ];
    TEditData       TracksExcluded  [ EditSizeText ];
};


EndBytePacking

//----------------------------------------------------------------------------
                                        // This is the "remote control" dialog that the user interacts with during the live averaging
class   TTracksAveragingControlDialog   :   public  TBaseDialog
{
public:
                        TTracksAveragingControlDialog   ( owl::TWindow *parent, owl::TResId resId /*, TTracksAveragingControlStruct& transfer*/ );
                       ~TTracksAveragingControlDialog   ();


    owl::TEdit          *CurrentFile;

    owl::TEdit          *TriggerCode;
    owl::TEdit          *TriggerPos;
    owl::TEdit          *TriggerIndex;

    owl::TEdit          *AcceptNumTF;
    owl::TEdit          *RejectNumTF;

    owl::TEdit          *NumTriggersAccepted;
    owl::TEdit          *NumTriggersRejected;

    owl::TGroupBox      *TracksBox;
    owl::TEdit          *TrackName;
    owl::TEdit          *TrackMin;
    owl::TEdit          *TrackMax;
    owl::TEdit          *Threshold;

    owl::TEdit          *AuxName;
    owl::TEdit          *AuxMin;
    owl::TEdit          *AuxMax;
    owl::TEdit          *AuxThreshold;

    owl::TCheckBox      *TrackZoom;
    owl::TEdit          *TracksToExclude;
    owl::TEdit          *TracksExcluded;


    owlwparam           ButtonPressed;
    bool                canClose;

    void                ChannelToExcludeReadOnly( bool read )       { TracksToExclude->SetReadOnly ( read ); }


    void                ProcessGroups           ( TGoGoF* gogof, int gofi1, int gofi2, void* usetransfer );


protected:

    TTimer              Timer;
                                            // define these variables here, as they will be used outside the processing
    TTracksView*        EegView;
    TStrings            ElectrodesNames;
    TSelection          AuxTracks;
    TSelection          TempSel;
    bool                EpochState;


    void                SetupWindow             ();
    void                CloseWindow             ( int retvalue )    { if ( canClose )   TBaseDialog::CloseWindow(retvalue); }
    void                Destroy                 ( int retvalue )    { if ( canClose )   TBaseDialog::Destroy(retvalue); }
    void                EvTimer                 ( UINT timerId );

    void                CmButtonPressed         ( owlwparam wparam );
    void                CmAuxThresholdEnable    ( owl::TCommandEnabler &tce );
    void                CmAcceptUntilBadEnable  ( owl::TCommandEnabler &tce );
    void                CmRejectUntilOkEnable   ( owl::TCommandEnabler &tce );
    void                CmMyOk                  ();

    DECLARE_RESPONSE_TABLE (TTracksAveragingControlDialog);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
