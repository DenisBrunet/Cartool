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

#include    "TExportTracks.h"

#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*         ExportTracksTitle           = "Export Tracks";


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TExportTracksStruct
{
public:
                        TExportTracksStruct ();


    TRadioButtonData    ExportTracks;
    TEditData           Tracks                  [ EditSizeTextLong ];
    TCheckBoxData       UseXyzDoc;
    TEditData           XyzDocFile              [ EditSizeText ];

    TRadioButtonData    ExportRois;
    TEditData           RoisDocFile             [ EditSizeText ];

    TRadioButtonData    TimeInterval;
    TRadioButtonData    TriggersToKeep;
    TRadioButtonData    TriggersToExclude;
    TEditData           TimeMin                 [ EditSizeValue ];
    TEditData           TimeMax                 [ EditSizeValue ];
    TCheckBoxData       EndOfFile;
    TEditData           TriggersToKeepList      [ EditSizeText ];
    TEditData           TriggersToExcludeList   [ EditSizeText ];

    TEditData           AddChannels             [ EditSizeText ];

    TRadioButtonData    NoFilters;
    TRadioButtonData    CurrentFilters;
    TRadioButtonData    OtherFilters;

    TRadioButtonData    NoRef;
    TRadioButtonData    CurrentRef;
    TRadioButtonData    AveRef;
    TRadioButtonData    OtherRef;
    TEditData           RefList                 [ EditSizeText ];

    TCheckBoxData       BaselineCorr;
    TEditData           BaselineCorrPre         [ EditSizeValue ];
    TEditData           BaselineCorrPost        [ EditSizeValue ];
    TCheckBoxData       BaselineCorrEof;

    TRadioButtonData    NoRescaling;
    TRadioButtonData    RescaledBy;
    TEditData           RescaledValue           [EditSizeValue];
    TRadioButtonData    GfpNormalization;

    TRadioButtonData    Sequence;
    TRadioButtonData    Mean;
    TCheckBoxData       Downsample;
    TEditData           DownsampleRatio         [ EditSizeValue ];

    TComboBoxData       FileTypes;
    TEditData           InfixFilename           [ EditSizeText ];

    TCheckBoxData       Markers;
    TCheckBoxData       OpenAuto;
    TCheckBoxData       Concatenate;
    };


class   TExportTracksStructEx:  public  TExportTracksStruct
{
public:
                        TExportTracksStructEx ();


    TTracksFilters<float>   Filters;                // saved here to be accessible across multiple calls
    double              DefaultSamplingFrequency;   // used as an ultimate fallback value
    TSelection*         ChannelsSel;                // from process current
    bool                XyzLinked;                  // from lm
    bool                RoiLinked;                  // from lm
};


EndBytePacking

//----------------------------------------------------------------------------

class                   TTracksDoc;


class   TExportTracksDialog :   public  TBaseDialog
{
public:
                        TExportTracksDialog         ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc );
                       ~TExportTracksDialog         ();


    void                BatchProcess                ()                                                      final;
    void                BatchProcessCurrent         ()                                                      final;
    void                ProcessCurrent              ( void *usetransfer = 0, const char *moreinfix = 0 )    final;


protected:

    owl::TRadioButton   *ExportTracks;
    owl::TEdit          *Tracks;
    owl::TCheckBox      *UseXyzDoc;
    owl::TEdit          *XyzDocFile;

    owl::TRadioButton   *ExportRois;
    owl::TEdit          *RoisDocFile;

    owl::TRadioButton   *TimeInterval;
    owl::TRadioButton   *TriggersToKeep;
    owl::TRadioButton   *TriggersToExclude;
    owl::TEdit          *TimeMin;
    owl::TEdit          *TimeMax;
    owl::TCheckBox      *EndOfFile;
    owl::TEdit          *TriggersToKeepList;
    owl::TEdit          *TriggersToExcludeList;

    owl::TEdit          *AddChannels;

    owl::TRadioButton   *NoFilters;
    owl::TRadioButton   *CurrentFilters;
    owl::TRadioButton   *OtherFilters;

    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *CurrentRef;
    owl::TRadioButton   *AveRef;
    owl::TRadioButton   *OtherRef;
    owl::TEdit          *RefList;

    owl::TCheckBox      *BaselineCorr;
    owl::TEdit          *BaselineCorrPre;
    owl::TEdit          *BaselineCorrPost;
    owl::TCheckBox      *BaselineCorrEof;

    owl::TRadioButton   *NoRescaling;
    owl::TRadioButton   *RescaledBy;
    owl::TEdit          *RescaledValue;
    owl::TRadioButton   *GfpNormalization;

    owl::TRadioButton   *Sequence;
    owl::TRadioButton   *Mean;
    owl::TCheckBox      *Downsample;
    owl::TEdit          *DownsampleRatio;

    owl::TComboBox      *FileTypes;
    owl::TEdit          *InfixFilename;

    owl::TCheckBox      *Markers;
    owl::TCheckBox      *OpenAuto;
    owl::TCheckBox      *Concatenate;


    TExportTracks       ExpFile;
    long                ConcatInputTime;
    long                ConcatOutputTime;


    void                SetupWindow                 ()  final;
    void                EvDropFiles                 ( owl::TDropInfo drop );
    void                SetBaseFilename             ();


    void                CmBrowseXyzDoc              ();
    void                CmXyzChange                 ();
    void                CmTracksEnable              ( owl::TCommandEnabler &tce );
//  void                CmExportTracksEnable        ( owl::TCommandEnabler &tce );
    void                CmXyzEnable                 ( owl::TCommandEnabler &tce );
    void                CmXyzDocFileEnable          ( owl::TCommandEnabler &tce );
    void                CmUseXyzDocEnable           ( owl::TCommandEnabler &tce );

    void                CmBrowseRoiDoc              ();
    void                CmRoiChange                 ();
//  void                CmExportRoisEnable          ( owl::TCommandEnabler &tce );
    void                CmRoisEnable                ( owl::TCommandEnabler &tce );

    void                CmAddChannelsEnable         ( owl::TCommandEnabler &tce );

    void                CmIntervalEnable            ( owl::TCommandEnabler &tce );
    void                CmTimeMaxEnable             ( owl::TCommandEnabler &tce );
    void                CmTriggersEnable            ( owl::TCommandEnabler &tce );
    void                CmTriggersExclEnable        ( owl::TCommandEnabler &tce );
    bool                CmProcessEnable             ();
    void                CmProcessCurrentEnable      ( owl::TCommandEnabler &tce );
    void                CmProcessBatchEnable        ( owl::TCommandEnabler &tce );
    void                CmOtherFilters              ();
    void                CmOtherRefEnable            ( owl::TCommandEnabler &tce );
    void                CmRescaledByEnable          ( owl::TCommandEnabler &tce );
    void                CmBaseLineCorrectionEnable  ( owl::TCommandEnabler &tce );
    void                CmBaseLineCorrectionEofEnable( owl::TCommandEnabler &tce );
    void                CmTimeSequenceEnable        ( owl::TCommandEnabler &tce );
    void                CmDownsampleRatioEnable     ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TExportTracksDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
