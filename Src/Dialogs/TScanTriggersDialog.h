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

#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*         ScanningTriggersTitle   = "Scanning Markers";


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TScanTriggersStruct
{
public:
                        TScanTriggersStruct ();


    TEditData           Channels[ EditSizeTextLong ];
    TEditData           TimeMin [ EditSizeValue ];
    TEditData           TimeMax [ EditSizeValue ];
    TCheckBoxData       EndOfFile;

    TRadioButtonData    ScanStability;

    TRadioButtonData    ScanExtrema;
    TCheckBoxData       ScanMin;
    TCheckBoxData       ScanMax;

    TRadioButtonData    ScanThreshold;
    TCheckBoxData       ThresholdAbove;
    TEditData           ThresholdAboveValue [ EditSizeValue ];
    TCheckBoxData       ThresholdBelow;
    TEditData           ThresholdBelowValue [ EditSizeValue ];
    TRadioButtonData    ThresholdStrict;
    TRadioButtonData    ThresholdSlope;

    TRadioButtonData    ScanTemplate;
    TEditData           ScanTemplateFile     [ EditSizeText ];
    TEditData           ScanTemplateThreshold[ EditSizeValue ];

    TEditData           StabMin [ EditSizeValue ];
    TEditData           StabMax [ EditSizeValue ];
    TEditData           Gap     [ EditSizeValue ];

    TRadioButtonData    OneMarkerPerTrack;
    TRadioButtonData    MergeMarkers;
    TEditData           MergeMarkersRange    [ EditSizeValue ];

    TCheckBoxData       PrefixMarker;
    TEditData           PrefixMarkerString   [ EditSizeText ];
    TCheckBoxData       TrackName;
    TRadioButtonData    NoValue;
    TRadioButtonData    TrackValue;
    TRadioButtonData    TrackRelativeIndex;
    TRadioButtonData    MergedCount;
    };


EndBytePacking

//----------------------------------------------------------------------------

class   TTracksDoc;


class   TScanTriggersDialog :    public  TBaseDialog
{
public:
                        TScanTriggersDialog         ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc );
                       ~TScanTriggersDialog         ();


    void                BatchProcessCurrent         ()  final;
    void                ProcessCurrent              ( void *usetransfer = 0, const char *moreinfix = 0 )    final;


protected:

    owl::TEdit          *Channels;
    owl::TEdit          *TimeMin;
    owl::TEdit          *TimeMax;
    owl::TCheckBox      *EndOfFile;

    owl::TRadioButton   *ScanStability;

    owl::TRadioButton   *ScanExtrema;
    owl::TCheckBox      *ScanMin;
    owl::TCheckBox      *ScanMax;

    owl::TRadioButton   *ScanThreshold;
    owl::TCheckBox      *ThresholdAbove;
    owl::TEdit          *ThresholdAboveValue;
    owl::TCheckBox      *ThresholdBelow;
    owl::TEdit          *ThresholdBelowValue;
    owl::TRadioButton   *ThresholdStrict;
    owl::TRadioButton   *ThresholdSlope;

    owl::TRadioButton   *ScanTemplate;
    owl::TEdit          *ScanTemplateFile;
    owl::TEdit          *ScanTemplateThreshold;

    owl::TEdit          *StabMin;
    owl::TEdit          *StabMax;
    owl::TEdit          *Gap;

    owl::TRadioButton   *OneMarkerPerTrack;
    owl::TRadioButton   *MergeMarkers;
    owl::TEdit          *MergeMarkersRange;

    owl::TCheckBox      *PrefixMarker;
    owl::TEdit          *PrefixMarkerString;
    owl::TCheckBox      *TrackName;
    owl::TRadioButton   *NoValue;
    owl::TRadioButton   *TrackValue;
    owl::TRadioButton   *TrackRelativeIndex;
    owl::TRadioButton   *MergedCount;


    void                EvDropFiles                 ( owl::TDropInfo drop );
    void                CmBrowseTemplateFileName    ();
    void                SetTemplateFileName         ( const char* file );

    bool                CmProcessEnable             ();
    void                CmProcessCurrentEnable      ( owl::TCommandEnabler &tce );
    void                CmProcessBatchEnable        ( owl::TCommandEnabler &tce );
    void                CmNotTemplateEnable         ( owl::TCommandEnabler &tce );
    void                CmOneMarkerPerTrackEnable   ( owl::TCommandEnabler &tce );
    void                CmMergeMarkersRangeEnable   ( owl::TCommandEnabler &tce );
    void                CmThresholdEnable           ( owl::TCommandEnabler &tce );
    void                CmDurationEnable            ( owl::TCommandEnabler &tce );
    void                CmTemplateEnable            ( owl::TCommandEnabler &tce );
    void                CmThresholdAboveEnable      ( owl::TCommandEnabler &tce );
    void                CmThresholdBelowEnable      ( owl::TCommandEnabler &tce );
    void                CmExtremaEnable             ( owl::TCommandEnabler &tce );
    void                CmChannelsEnable            ( owl::TCommandEnabler &tce );
    void                CmTimeMaxEnable             ( owl::TCommandEnabler &tce );
    void                CmPrefixMarkerEnable        ( owl::TCommandEnabler &tce );
    void                EvMethodChanged             ();

    DECLARE_RESPONSE_TABLE ( TScanTriggersDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
