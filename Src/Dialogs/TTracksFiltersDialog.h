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

inline double   FrequenciesToEnvelopeWidth ( double fmin /*, double fmax*/ )
{
if ( fmin <= 0 )    return  0;

double              width           = ( 1000.0 /                            // 1 cycle in [ms] of..
//                                               ( ( fmin + fmax ) / 2 )    //   average frequency
                                                 fmin                       //   lowest frequency
                                      ) / 2                                 // once absolute, half a cycle in [ms]
//                                      * 6;                                // at least 6 absolute cycles / bumps
                                        * 2;                                // 2 seems enough?
return  width;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // TTracksFiltersStruct is doing the interface between  TTracksFiltersDialog  dialog
                                        // but also with TTracksFilters, which needs sometime to call the dialog, too.

enum    FilterCausality;

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TTracksFiltersStruct
{
public:
                        TTracksFiltersStruct ();


    TCheckBoxData       Baseline;

    TCheckBoxData       ButterworthHigh;
    TEditData           ValueButterworthHigh[ EditSizeValue ];
    TEditData           OrderHigh           [ EditSizeValue ];
    TCheckBoxData       ButterworthLow;
    TEditData           ValueButterworthLow [ EditSizeValue ];
    TEditData           OrderLow            [ EditSizeValue ];

    TRadioButtonData    Causal;
    TRadioButtonData    NonCausal;

    TCheckBoxData       Notches;
    TEditData           NotchesValues       [ EditSizeText ];
    TCheckBoxData       NotchesAutoHarmonics;

    TCheckBoxData       SpatialFilter;
    TEditData           XyzFile             [ EditSizeText ];

    TCheckBoxData       Ranking;

    TCheckBoxData       Rectification;
    TRadioButtonData    RectificationAbs;
    TRadioButtonData    RectificationPower;
    TCheckBoxData       Envelope;
    TEditData           EnvelopeWidth       [ EditSizeValue ];
    TEditData           EnvelopeFreq        [ EditSizeValue ];

    TCheckBoxData       ThresholdKeepAbove;
    TEditData           ThresholdKeepAboveValue [ EditSizeValue ];
    TCheckBoxData       ThresholdKeepBelow;
    TEditData           ThresholdKeepBelowValue [ EditSizeValue ];

    TEditData           ValueSamplingFrequency[ EditSizeValue ];
    TCheckBoxData       FilterAuxs;
    TRadioButtonData    On;
    TRadioButtonData    Off;

                                        // retrieve info from structure
    bool            IsOn                    ()      const   { return CheckToBool ( On  ); }
    bool            IsOff                   ()      const   { return CheckToBool ( Off ); }
    bool            IsButterworthHigh       ()      const   { return IsOn () && CheckToBool ( ButterworthHigh        ); }
    bool            IsButterworthLow        ()      const   { return IsOn () && CheckToBool ( ButterworthLow         ); }
    bool            IsBandPass              ()      const   { return IsButterworthHigh () && IsButterworthLow ();       }
    bool            IsCausal                ()      const   { return IsOn () && CheckToBool ( Causal                 ); }
    bool            IsBaseline              ()      const   { return IsOn () && CheckToBool ( Baseline               ) && ! IsCausal (); }  // Causal has no DC
    bool            IsNotches               ()      const   { return IsOn () && CheckToBool ( Notches                ); }
    bool            IsNotchAutoHarmonics    ()      const   { return IsNotches ()       && CheckToBool ( NotchesAutoHarmonics   ); }
    bool            IsSpatialFiltering      ()      const   { return IsOn () && CheckToBool ( SpatialFilter          ) /*&& StringIsNotEmpty ( XyzFile )*/; }
    bool            IsRanking               ()      const   { return IsOn () && CheckToBool ( Ranking                ); }
    bool            IsRectification         ()      const   { return IsOn () && CheckToBool ( Rectification          ); }
    bool            IsEnvelope              ()      const   { return IsOn () && CheckToBool ( Envelope               ); }
    bool            IsThresholdAbove        ()      const   { return IsOn () && CheckToBool ( ThresholdKeepAbove     ); }
    bool            IsThresholdBelow        ()      const   { return IsOn () && CheckToBool ( ThresholdKeepBelow     ); }


    double          GetButterworthLow       ()      const;
    double          GetButterworthHigh      ()      const;
    int             GetOrderHigh            ()      const;
    int             GetOrderLow             ()      const;
    FilterCausality GetCausal               ()      const;
    bool            GetBaseline             ()      const;
    bool            GetNotches              ()      const;
    bool            GetNotchesAutoHarmonics ()      const;
    bool            GetSpatialFiltering     ()      const;
    bool            GetRanking              ()      const;
    int             GetRectification        ()      const;
    double          GetEnvelopeWidth        ()      const;
    double          GetThresholdAbove       ()      const;
    double          GetThresholdBelow       ()      const;


    void            ResetAll                ();
    void            Set                     ();
    void            Reset                   ();

    void            SetButterworthLow       ( const char* t );
    void            SetButterworthHigh      ( const char* t );
    void            SetOrderHigh            ( const char* t );
    void            SetOrderLow             ( const char* t );
    void            SetCausal               ( const char* t );
    void            SetBaseline             ( const char* t );
    void            SetNotches              ( const char* t );
    void            SetNotchesAutoHarmonics ( bool set );
    void            SetSpatialFiltering     ( const char* t );
    void            SetRanking              ( const char* t );
    void            SetRectification        ( const char* t );
    void            SetEnvelopeWidth        ( const char* t );
    void            SetThresholdAbove       ( const char* t );
    void            SetThresholdBelow       ( const char* t );
    };


EndBytePacking

//----------------------------------------------------------------------------
                                        // TTracksFiltersDialog is the UI called by TTracksDoc, showing the recommended filters
                                        // and their correct sequence to the user.
class   TTracksFiltersDialog    :   public  TBaseDialog
{
public:
                        TTracksFiltersDialog ( owl::TWindow* parent, owl::TResId resId, TTracksFiltersStruct& transfer, double samplfreq );
                       ~TTracksFiltersDialog ();


protected:
    owl::TCheckBox      *Baseline;

    owl::TCheckBox      *ButterworthHigh;
    owl::TEdit          *ValueButterworthHigh;
    owl::TEdit          *OrderHigh;
    owl::TCheckBox      *ButterworthLow;
    owl::TEdit          *ValueButterworthLow;
    owl::TEdit          *OrderLow;

    owl::TRadioButton   *Causal;
    owl::TRadioButton   *NonCausal;

    owl::TCheckBox      *Notches;
    owl::TEdit          *NotchesValues;
    owl::TCheckBox      *NotchesAutoHarmonics;

    owl::TCheckBox      *SpatialFilter;
    owl::TEdit          *XyzFile;

    owl::TCheckBox      *Ranking;

    owl::TCheckBox      *Rectification;
    owl::TRadioButton   *RectificationAbs;
    owl::TRadioButton   *RectificationPower;
    owl::TCheckBox      *Envelope;
    owl::TEdit          *EnvelopeWidth;
    owl::TEdit          *EnvelopeFreq;

    owl::TCheckBox      *ThresholdKeepAbove;
    owl::TEdit          *ThresholdKeepAboveValue;
    owl::TCheckBox      *ThresholdKeepBelow;
    owl::TEdit          *ThresholdKeepBelowValue;

    owl::TEdit          *ValueSamplingFrequency;
    owl::TCheckBox      *FilterAuxs;
    owl::TRadioButton   *On;
    owl::TRadioButton   *Off;


    double              SamplingFrequency;
    int                 LastButton;             // used for real-time testing, smart change of focus
    bool                LockUpdate;             // semaphor to avoid cyclical auto-updates


    void                EvDropFiles                 ( owl::TDropInfo drop );

    void                CmButterworthLowEnable      ( owl::TCommandEnabler &tce );
    void                CmButterworthHighEnable     ( owl::TCommandEnabler &tce );

    void                CmRectificationEnable       ( owl::TCommandEnabler &tce );

    void                CmEnvelopeEnable            ( owl::TCommandEnabler &tce );
    void                CmEnvelopeWidthEnable       ( owl::TCommandEnabler &tce );
    void                EvEnvelopeChanged           ();
    void                UpdateEnvelopeWidthToFreq   ();
    void                UpdateEnvelopeFreqToWidth   ();

    void                CmCausalEnable              ( owl::TCommandEnabler &tce );
    void                CmBaseLineEnable            ( owl::TCommandEnabler &tce );
    void                CmNotchEnable               ( owl::TCommandEnabler &tce );
    void                CmThresholdAboveEnable      ( owl::TCommandEnabler &tce );
    void                CmThresholdBelowEnable      ( owl::TCommandEnabler &tce );
    void                EvButterworthLowChanged     ();
    void                EvButterworthHighChanged    ();
    void                EvCausalChanged             ();
    void                EvNotchChanged              ();

    void                EvSpatialFilteringChanged   ();
    void                EvRankingChanged            ();
    void                CmBrowseCoordinatesFile     ();
    void                SetCoordinatesFile          ( char *file );
    void                CmBrowseCoordinatesEnable   ( owl::TCommandEnabler &tce );

    void                EvThresholdAboveChanged     ();
    void                EvThresholdBelowChanged     ();
    void                CmAuxSamplingFrequencyEnable( owl::TCommandEnabler &tce );
    void                CmOtherEnable               ( owl::TCommandEnabler &tce );
    void                CmOKEnable                  ( owl::TCommandEnabler &tce );
    void                CmMyOk                      ();
    void                CheckEnvelopeWidth          ();
    void                UpdateOrdersHighToLow       ();
    void                UpdateOrdersLowToHigh       ();

    DECLARE_RESPONSE_TABLE (TTracksFiltersDialog);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
