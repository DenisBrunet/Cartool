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

#include    <owl/pch.h>

#include    "TTracksFiltersDialog.h"

#include    "Strings.TFixedString.h"
#include    "Files.Extensions.h"

#include    "Math.Utils.h"
#include    "TFilters.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTracksFiltersStruct::TTracksFiltersStruct ()
{
ResetAll ();
}


void    TTracksFiltersStruct::ResetAll ()
{
Baseline                = BoolToCheck ( false );

ButterworthHigh         = BoolToCheck ( false );
ClearString ( ValueButterworthLow );
ButterworthLow          = BoolToCheck ( false );
ClearString ( ValueButterworthHigh );
StringCopy  ( OrderHigh, TFilterDefaultOrderString );
StringCopy  ( OrderLow,  TFilterDefaultOrderString );

Causal                  = BoolToCheck ( false );
NonCausal               = BoolToCheck (  true );

Notches                 = BoolToCheck ( false );
ClearString ( NotchesValues );
NotchesAutoHarmonics    = BoolToCheck (  true );

SpatialFilter           = BoolToCheck ( false );
ClearString ( XyzFile );

Ranking                 = BoolToCheck ( false );

Rectification           = BoolToCheck ( false );
RectificationAbs        = BoolToCheck ( false );
RectificationPower      = BoolToCheck (  true );
Envelope                = BoolToCheck ( false );
ClearString ( EnvelopeWidth );
ClearString ( EnvelopeFreq  );

ThresholdKeepAbove      = BoolToCheck ( false );
ThresholdKeepBelow      = BoolToCheck ( false );
ClearString ( ThresholdKeepAboveValue );
ClearString ( ThresholdKeepBelowValue );

ClearString ( ValueSamplingFrequency );
FilterAuxs              = BoolToCheck (  true );

Set ();
}


void    TTracksFiltersStruct::Set ()
{
On                      = BoolToCheck (  true );
Off                     = BoolToCheck ( false );
}


void    TTracksFiltersStruct::Reset ()
{
On                      = BoolToCheck ( false );
Off                     = BoolToCheck (  true );
}


//----------------------------------------------------------------------------
double  TTracksFiltersStruct::GetButterworthLow ()    const
{
return  IsButterworthLow ()  ? StringToDouble ( ValueButterworthLow )  : 0;
}


double  TTracksFiltersStruct::GetButterworthHigh ()   const
{
return  IsButterworthHigh () ? StringToDouble ( ValueButterworthHigh ) : 0;
}


int     TTracksFiltersStruct::GetOrderHigh () const
{
return  IsButterworthHigh () ? StringToInteger ( OrderHigh ) : 0;
}


int     TTracksFiltersStruct::GetOrderLow ()  const
{
return  IsButterworthLow ()  ? StringToInteger ( OrderLow  ) : 0;
}


FilterCausality TTracksFiltersStruct::GetCausal ()    const
{
return  IsCausal () ? FilterCausal : FilterNonCausal;
}


bool    TTracksFiltersStruct::GetBaseline ()  const
{
return  IsBaseline ();
}


int     TTracksFiltersStruct::GetRectification () const
{
return  ! IsRectification ()                 ? 0
       :  CheckToBool ( RectificationAbs   ) ? IDC_RECTIFICATIONABS
       :  CheckToBool ( RectificationPower ) ? IDC_RECTIFICATIONPOWER
       :  0;
}


double  TTracksFiltersStruct::GetEnvelopeWidth () const
{
return  IsEnvelope ()   ? StringToDouble ( EnvelopeWidth ) 
                        : 0;
}


bool    TTracksFiltersStruct::GetNotches () const
{
return  IsNotches ();
}


bool    TTracksFiltersStruct::GetNotchesAutoHarmonics ()    const
{
return  IsNotchAutoHarmonics ();
}


bool    TTracksFiltersStruct::GetSpatialFiltering ()  const
{
return  IsSpatialFiltering ();
}


bool    TTracksFiltersStruct::GetRanking ()   const
{
return  IsRanking ();
}


double  TTracksFiltersStruct::GetThresholdAbove ()    const
{
return  IsThresholdAbove () ? StringToDouble ( ThresholdKeepAboveValue ) : 0;
}


double  TTracksFiltersStruct::GetThresholdBelow ()    const
{
return  IsThresholdBelow () ? StringToDouble ( ThresholdKeepBelowValue ) : 0;
}


//----------------------------------------------------------------------------
void    TTracksFiltersStruct::SetButterworthLow ( const char* t )
{
ButterworthLow          = BoolToCheck (  true );
StringCopy ( ValueButterworthLow, t );
Set ();
}


void    TTracksFiltersStruct::SetButterworthHigh ( const char* t )
{
ButterworthHigh         = BoolToCheck (  true );
StringCopy ( ValueButterworthHigh, t );
Set ();
}

                                        // !Identical order for a single low / high pass filter - it will be x2 for pass / stop band!
void    TTracksFiltersStruct::SetOrderHigh ( const char* t )
{
StringCopy ( OrderHigh, t );
Set ();
}


void    TTracksFiltersStruct::SetOrderLow ( const char* t )
{
StringCopy ( OrderLow,  t );
Set ();
}


void    TTracksFiltersStruct::SetCausal ( const char* t )
{
bool                causal          = StringStartsWith ( t, "Causal" );

Causal                  = BoolToCheck (   causal );
NonCausal               = BoolToCheck ( ! causal );

Set ();
}


void    TTracksFiltersStruct::SetBaseline ( const char* t )
{
Baseline                = BoolToCheck ( StringStartsWith ( t, "DC removed" ) || StringStartsWith ( t, "Baseline" ) );
Set ();
}


void    TTracksFiltersStruct::SetRectification ( const char* t )
{
Rectification           = BoolToCheck ( false );
RectificationAbs        = BoolToCheck ( false );
RectificationPower      = BoolToCheck ( false );

if      ( StringContains ( t, "Absolute" )
       || StringContains ( t, "Abs"      ) ) {

    Rectification       = BoolToCheck (  true );
    RectificationAbs    = BoolToCheck (  true );
    Set ();
    }
else if ( StringContains ( t, "Power"   )
       || StringContains ( t, "Squared" ) ) {

    Rectification       = BoolToCheck (  true );
    RectificationPower  = BoolToCheck (  true );
    Set ();
    }
}


void    TTracksFiltersStruct::SetEnvelopeWidth ( const char* t )
{
Envelope                = BoolToCheck ( true );

StringCopy      ( EnvelopeWidth, t );
                                        // convert to equivalent frequency
FloatToString   ( EnvelopeFreq,  MillisecondsToFrequency ( StringToDouble ( EnvelopeWidth ) ) );

Set ();
}


void    TTracksFiltersStruct::SetNotches ( const char* t )
{
Notches                 = BoolToCheck (  true );
StringCopy ( NotchesValues, t );
Set ();
}


void    TTracksFiltersStruct::SetNotchesAutoHarmonics ( bool set )
{
NotchesAutoHarmonics      = BoolToCheck ( set );
//Set (); // not a filter per se, so it doesn't trigger the whole filtering
}


void    TTracksFiltersStruct::SetSpatialFiltering ( const char* t )
{
SpatialFilter           = BoolToCheck (  true );
StringCopy ( XyzFile, t );
Set ();
}


void    TTracksFiltersStruct::SetRanking ( const char* t )
{
Ranking                 = BoolToCheck ( StringStartsWith ( t, "Rank" ) );
Set ();
}


void    TTracksFiltersStruct::SetThresholdAbove ( const char* t )
{
ThresholdKeepAbove      = BoolToCheck (  true );
StringCopy ( ThresholdKeepAboveValue, t );
Set ();
}


void    TTracksFiltersStruct::SetThresholdBelow ( const char* t )
{
ThresholdKeepBelow      = BoolToCheck (  true );
StringCopy ( ThresholdKeepBelowValue, t );
Set ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TTracksFiltersDialog, TBaseDialog)

    EV_WM_DROPFILES,

    EV_BN_CLICKED	            ( IDC_BUTTERWORTHHIGH,      EvButterworthHighChanged ),
    EV_BN_CLICKED	            ( IDC_BUTTERWORTHLOW,       EvButterworthLowChanged ),
    EV_EN_CHANGE                ( IDC_BUTTERWORTHHIGHVALUE, CheckEnvelopeWidth ),
    EV_EN_CHANGE                ( IDC_FILTERORDERHIGH,      UpdateOrdersHighToLow ),
    EV_EN_CHANGE                ( IDC_FILTERORDERLOW,       UpdateOrdersLowToHigh ),
    EV_COMMAND_ENABLE           ( IDC_BUTTERWORTHHIGHVALUE, CmButterworthHighEnable ),
    EV_COMMAND_ENABLE           ( IDC_BUTTERWORTHLOWVALUE,  CmButterworthLowEnable ),
    EV_COMMAND_ENABLE           ( IDC_FILTERORDERHIGH,      CmButterworthHighEnable ),
    EV_COMMAND_ENABLE           ( IDC_FILTERORDERLOW,       CmButterworthLowEnable ),

    EV_BN_CLICKED	            ( IDC_CAUSAL,               EvCausalChanged ),
    EV_BN_CLICKED	            ( IDC_NONCAUSAL,            EvCausalChanged ),
    EV_COMMAND_ENABLE           ( IDC_CAUSAL,               CmCausalEnable ),
    EV_COMMAND_ENABLE           ( IDC_NONCAUSAL,            CmCausalEnable ),

    EV_BN_CLICKED	            ( IDC_ENVELOPE,             EvEnvelopeChanged ),
    EV_BN_CLICKED	            ( IDC_NOTCH,                EvNotchChanged ),
    EV_BN_CLICKED	            ( IDC_SPATIALFILTER,        EvSpatialFilteringChanged ),
    EV_BN_CLICKED	            ( IDC_RANKING,              EvRankingChanged ),
    EV_BN_CLICKED	            ( IDC_THRESHOLDABOVE,       EvThresholdAboveChanged ),
    EV_BN_CLICKED	            ( IDC_THRESHOLDBELOW,       EvThresholdBelowChanged ),
    EV_BN_CLICKED	            ( IDC_FILTERSOFF,           CmMyOk ),

    EV_COMMAND                  ( IDC_BROWSEXYZFILE,        CmBrowseCoordinatesFile ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZFILE,        CmBrowseCoordinatesEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZFILE,              CmBrowseCoordinatesEnable ),

    EV_COMMAND_ENABLE           ( IDC_RECTIFICATIONABS,     CmRectificationEnable ),
    EV_COMMAND_ENABLE           ( IDC_RECTIFICATIONPOWER,   CmRectificationEnable ),
    EV_COMMAND_ENABLE           ( IDC_ENVELOPE,             CmEnvelopeEnable ),
    EV_COMMAND_ENABLE           ( IDC_ENVELOPEWIDTH,        CmEnvelopeWidthEnable ),
    EV_COMMAND_ENABLE           ( IDC_ENVELOPEFREQ,         CmEnvelopeWidthEnable ),
    EV_EN_CHANGE                ( IDC_ENVELOPEWIDTH,        UpdateEnvelopeWidthToFreq ),
    EV_EN_CHANGE                ( IDC_ENVELOPEFREQ,         UpdateEnvelopeFreqToWidth ),

    EV_COMMAND_ENABLE           ( IDC_NOTCHVALUES,          CmNotchEnable ),
    EV_COMMAND_ENABLE           ( IDC_NOTCHAUTOHARMONICS,   CmNotchEnable ),

    EV_COMMAND_ENABLE           ( IDC_THRESHOLDABOVEVALUE,  CmThresholdAboveEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDBELOWVALUE,  CmThresholdBelowEnable ),
    EV_COMMAND_ENABLE           ( IDC_AUXSAMPLINGFREQUENCY, CmAuxSamplingFrequencyEnable ),
    EV_COMMAND_ENABLE           ( IDC_BUTTERWORTHHIGH,      CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_BUTTERWORTHLOW,       CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_BASELINE,             CmBaseLineEnable ),
    EV_COMMAND_ENABLE           ( IDC_NOTCH,                CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDABOVE,       CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDBELOW,       CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPATIALFILTER,        CmOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_RANKING,              CmOtherEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                     CmOKEnable ),
    EV_COMMAND                  ( IDOK,                     CmMyOk ),
END_RESPONSE_TABLE;


        TTracksFiltersDialog::TTracksFiltersDialog  (   TWindow*                parent,     TResId      resId, 
                                                        TTracksFiltersStruct&   transfer,   double      samplfreq  )
      : TBaseDialog ( parent, resId )
{
SamplingFrequency   = samplfreq;


Baseline                    = new TCheckBox     ( this, IDC_BASELINE, 0 );

ButterworthHigh             = new TCheckBox     ( this, IDC_BUTTERWORTHHIGH, 0 );
ValueButterworthHigh        = new TEdit         ( this, IDC_BUTTERWORTHHIGHVALUE, EditSizeValue );
OrderHigh                   = new TEdit         ( this, IDC_FILTERORDERHIGH, EditSizeValue );
ButterworthLow              = new TCheckBox     ( this, IDC_BUTTERWORTHLOW, 0 );
ValueButterworthLow         = new TEdit         ( this, IDC_BUTTERWORTHLOWVALUE, EditSizeValue );
OrderLow                    = new TEdit         ( this, IDC_FILTERORDERLOW, EditSizeValue );
ValueButterworthHigh->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat   ) );
ValueButterworthLow ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat   ) );
OrderHigh           ->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );
OrderLow            ->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );

Causal                      = new TRadioButton  ( this, IDC_CAUSAL, 0 );
NonCausal                   = new TRadioButton  ( this, IDC_NONCAUSAL, 0 );

Notches                     = new TCheckBox     ( this, IDC_NOTCH, 0 );
NotchesValues               = new TEdit         ( this, IDC_NOTCHVALUES, EditSizeText );
NotchesAutoHarmonics        = new TCheckBox     ( this, IDC_NOTCHAUTOHARMONICS, 0 );
NotchesValues->SetValidator ( new TFilterValidator ( ValidatorPositiveFloats ) );

SpatialFilter               = new TCheckBox     ( this, IDC_SPATIALFILTER );
XyzFile                     = new TEdit         ( this, IDC_XYZFILE, EditSizeText );

Ranking                     = new TCheckBox     ( this, IDC_RANKING );

Rectification               = new TCheckBox     ( this, IDC_RECTIFICATION, 0 );
RectificationAbs            = new TRadioButton  ( this, IDC_RECTIFICATIONABS, 0 );
RectificationPower          = new TRadioButton  ( this, IDC_RECTIFICATIONPOWER, 0 );
Envelope                    = new TCheckBox     ( this, IDC_ENVELOPE, 0 );
EnvelopeWidth               = new TEdit         ( this, IDC_ENVELOPEWIDTH, EditSizeValue );
EnvelopeFreq                = new TEdit         ( this, IDC_ENVELOPEFREQ, EditSizeValue );
EnvelopeWidth->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );
EnvelopeFreq ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );

ThresholdKeepAbove          = new TCheckBox     ( this, IDC_THRESHOLDABOVE, 0 );
ThresholdKeepAboveValue     = new TEdit         ( this, IDC_THRESHOLDABOVEVALUE, EditSizeValue );
ThresholdKeepBelow          = new TCheckBox     ( this, IDC_THRESHOLDBELOW, 0 );
ThresholdKeepBelowValue     = new TEdit         ( this, IDC_THRESHOLDBELOWVALUE, EditSizeValue );
ThresholdKeepAboveValue->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );
ThresholdKeepBelowValue->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );

ValueSamplingFrequency      = new TEdit         ( this, IDC_AUXSAMPLINGFREQUENCY, EditSizeValue );
FilterAuxs                  = new TCheckBox     ( this, IDC_FILTERAUXS, 0 );
On                          = new TRadioButton  ( this, IDC_FILTERSON, 0 );
Off                         = new TRadioButton  ( this, IDC_FILTERSOFF, 0 );


SetTransferBuffer ( &transfer );

LastButton      = 0;

LockUpdate      = false;


if ( samplfreq > 0 )
    FloatToString ( transfer.ValueSamplingFrequency, samplfreq );
}


        TTracksFiltersDialog::~TTracksFiltersDialog ()
{
delete  Baseline;
delete  ButterworthHigh;            delete  ValueButterworthHigh;           delete  OrderHigh;
delete  ButterworthLow;             delete  ValueButterworthLow;            delete  OrderLow;
delete  Causal;                     delete  NonCausal;
delete  Notches;                    delete  NotchesValues;                  delete  NotchesAutoHarmonics;
delete  SpatialFilter;              delete  XyzFile;
delete  Ranking;
delete  Rectification;              delete  RectificationAbs;               delete  RectificationPower;              
delete  Envelope;                   delete  EnvelopeWidth;                  delete  EnvelopeFreq;
delete  ThresholdKeepAbove;         delete  ThresholdKeepAboveValue;
delete  ThresholdKeepBelow;         delete  ThresholdKeepBelowValue;
delete  ValueSamplingFrequency;     delete  FilterAuxs;
delete  On;                         delete  Off;
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::CmButterworthHighEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( ButterworthHigh->GetCheck() ) ) {
    tce.Enable ( true );

    if ( LastButton == IDC_BUTTERWORTHHIGH )
        SetControlFocus ( *ValueButterworthHigh ),
        LastButton = 0;
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmButterworthLowEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( ButterworthLow->GetCheck() ) ) {
    tce.Enable ( true );

    if ( LastButton == IDC_BUTTERWORTHLOW )
        SetControlFocus ( *ValueButterworthLow ),
        LastButton = 0;
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmBaseLineEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( NonCausal->GetCheck() ) ) {
    tce.Enable ( true );
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmRectificationEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( Rectification  ->GetCheck() ) ) {
    tce.Enable ( true );
    }
else
    tce.Enable ( false );
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::CmEnvelopeEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() ) ) {

    tce.Enable ( true );

    if ( LastButton == IDC_ENVELOPE ) {
        SetControlFocus ( *EnvelopeWidth );
        LastButton = 0;
        }
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmEnvelopeWidthEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( Envelope       ->GetCheck() ) ) {
    tce.Enable ( true );
    }
else
    tce.Enable ( false );
}

                                        // These 2 functions cross-update width and frequency
void    TTracksFiltersDialog::UpdateEnvelopeWidthToFreq ()
{
                                        // check we don't enter in an infinite loop of updates
if ( LockUpdate )   return;

char                buff[ EditSizeValue ];

EnvelopeWidth->GetText ( buff, EditSizeValue );

double              v               = StringToDouble ( buff );
                                        // send new content without launching a feedback loop to itself
LockUpdate  = true;

if ( v == 0 )   EnvelopeFreq->Clear   ();
else            EnvelopeFreq->SetText ( FloatToString ( MillisecondsToFrequency ( v ) ) );

LockUpdate  = false;
}


void    TTracksFiltersDialog::UpdateEnvelopeFreqToWidth ()
{
                                        // check we don't enter in an infinite loop of updates
if ( LockUpdate )   return;

char                buff[ EditSizeValue ];

EnvelopeFreq->GetText ( buff, EditSizeValue );

double              v               = StringToDouble ( buff );

                                        // send new content without launching a feedback loop to itself
LockUpdate  = true;

if ( v == 0 )   EnvelopeWidth->Clear   ();
else            EnvelopeWidth->SetText ( FloatToString ( FrequencyToMilliseconds ( v ) ) );

LockUpdate  = false;
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::CmCausalEnable ( TCommandEnabler &tce )
{
if (   CheckToBool ( On->GetCheck() )
  && ( CheckToBool ( ButterworthHigh->GetCheck() )
    || CheckToBool ( ButterworthLow ->GetCheck() ) ) ) {
    tce.Enable ( true );
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmNotchEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( Notches->GetCheck() ) ) {
    tce.Enable ( true );

    if ( LastButton == IDC_NOTCH )
        SetControlFocus ( *NotchesValues ),
        LastButton = 0;
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmThresholdAboveEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( ThresholdKeepAbove->GetCheck() ) ) {
    tce.Enable ( true );

    if ( LastButton == IDC_THRESHOLDABOVE )
        SetControlFocus ( *ThresholdKeepAboveValue ),
        LastButton = 0;
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmThresholdBelowEnable ( TCommandEnabler &tce )
{
if ( CheckToBool ( On->GetCheck() )
  && CheckToBool ( ThresholdKeepBelow->GetCheck() ) ) {
    tce.Enable ( true );

    if ( LastButton == IDC_THRESHOLDBELOW )
        SetControlFocus ( *ThresholdKeepBelowValue ),
        LastButton = 0;
    }
else
    tce.Enable ( false );
}


void    TTracksFiltersDialog::CmOtherEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( On->GetCheck() ) );
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::EvButterworthHighChanged ()
{
if ( CheckToBool ( ButterworthHigh->GetCheck() ) ) {
    LastButton = IDC_BUTTERWORTHHIGH;

    Baseline->SetCheck ( BoolToCheck (  true ) );

    CheckEnvelopeWidth ();
    }
//else
//    Baseline->SetCheck ( BoolToCheck ( false ) );
}


void    TTracksFiltersDialog::EvButterworthLowChanged ()
{
if ( CheckToBool ( ButterworthLow->GetCheck() ) ) {
    LastButton = IDC_BUTTERWORTHLOW;

    SetControlFocus ( *ValueButterworthLow );
    }
}


void    TTracksFiltersDialog::EvCausalChanged ()
{
if ( CheckToBool ( Causal->GetCheck() ) ) {
    LastButton = IDC_CAUSAL;
                                        // cancel DC - will be done through Enabling and GetBaseLine
//    Baseline->SetCheck ( BoolToCheck ( false ) );
    }
else {
                                        // set again DC
//    EvButterworthHighChanged ();

    LastButton = IDC_NONCAUSAL;
    }
}


void    TTracksFiltersDialog::EvEnvelopeChanged ()
{
if ( CheckToBool ( Envelope->GetCheck() ) ) {
    LastButton = IDC_ENVELOPE;

    CheckEnvelopeWidth ();

    SetControlFocus ( *EnvelopeWidth );
    }
}


void    TTracksFiltersDialog::CheckEnvelopeWidth ()
{
if ( ! (    CheckToBool ( Envelope->GetCheck () )
         && CheckToBool ( ButterworthHigh->GetCheck() ) ) )
    return;


TransferData ( tdGetData );
TTracksFiltersStruct*   transfer    = (TTracksFiltersStruct *) GetTransferBuffer ();

double              highf           = StringToDouble ( transfer->ValueButterworthHigh );

                                    // don't try to guess with uncoherent parameters
if ( highf == 0 )
    return;

double              minwidth        = FrequenciesToEnvelopeWidth ( highf );

EnvelopeWidth->SetText  ( FloatToString ( minwidth, 2 ) );
                                        // convert to equivalent frequency
EnvelopeFreq ->SetText  ( FloatToString ( MillisecondsToFrequency ( minwidth ) ) );
}

                                        // These 2 functions cross-update the high and low orders
void    TTracksFiltersDialog::UpdateOrdersHighToLow ()
{
                                        // check we don't enter in an infinite loop of updates
if ( LockUpdate )   return;

char                buff[ EditSizeValue ];

OrderHigh->GetText ( buff, EditSizeValue );
                                        // send new content without launching a feedback loop to itself
LockUpdate  = true;

OrderLow ->SetText ( buff );

LockUpdate  = false;
}


void    TTracksFiltersDialog::UpdateOrdersLowToHigh ()
{
                                        // check we don't enter in an infinite loop of updates
if ( LockUpdate )   return;

char                buff[ EditSizeValue ];

OrderLow ->GetText ( buff, EditSizeValue );
                                        // send new content without launching a feedback loop to itself
LockUpdate  = true;

OrderHigh->SetText ( buff );

LockUpdate  = false;
}


void    TTracksFiltersDialog::EvNotchChanged ()
{
if ( CheckToBool ( Notches->GetCheck() ) )
    LastButton = IDC_NOTCH;
}


void    TTracksFiltersDialog::EvThresholdAboveChanged ()
{
if ( CheckToBool ( ThresholdKeepAbove->GetCheck() ) )
    LastButton = IDC_THRESHOLDABOVE;
}


void    TTracksFiltersDialog::EvThresholdBelowChanged ()
{
if ( CheckToBool ( ThresholdKeepBelow->GetCheck() ) )
    LastButton = IDC_THRESHOLDBELOW;
}


void    TTracksFiltersDialog::CmAuxSamplingFrequencyEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( On->GetCheck () ) && SamplingFrequency <= 0 );
}


//----------------------------------------------------------------------------
                                        // for consistency, not really used
void    TTracksFiltersDialog::EvSpatialFilteringChanged ()
{
if ( CheckToBool ( SpatialFilter->GetCheck () ) )
    LastButton  = IDC_SPATIALFILTER;
}

                                        // for consistency, not really used
void    TTracksFiltersDialog::EvRankingChanged ()
{
if ( CheckToBool ( Ranking->GetCheck () ) )
    LastButton  = IDC_RANKING;
}


void    TTracksFiltersDialog::CmBrowseCoordinatesFile ()
{
SetCoordinatesFile ( 0 );
}


void    TTracksFiltersDialog::SetCoordinatesFile ( char *file )
{
static GetFileFromUser  getfile ( "Electrodes or Solution Points Coordinates File", AllPointsFilesFilter, 1, GetFileRead );


TransferData ( tdGetData );
TTracksFiltersStruct*   transfer    = (TTracksFiltersStruct *) GetTransferBuffer ();


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( transfer->XyzFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( transfer->XyzFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

//CheckXyzFile ();
}


void    TTracksFiltersDialog::CmBrowseCoordinatesEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( On->GetCheck () ) && CheckToBool ( SpatialFilter->GetCheck () ) );
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt );
TGoF                spfiles         ( drop, AllSolPointsFilesExt   );
TGoF                remainingfiles  ( drop, AllCoordinatesFilesExt " " AllSolPointsFilesExt, 0, true );

drop.DragFinish ();

TTracksFiltersStruct*   transfer    = (TTracksFiltersStruct *) GetTransferBuffer ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ ) {

    SetCoordinatesFile ( xyzfiles[ i ] );

    SpatialFilter->SetCheck ( BoolToCheck ( StringIsNotEmpty ( transfer->XyzFile ) /*XyzAndEegMatch*/ ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spfiles; i++ ) {

    SetCoordinatesFile ( spfiles[ i ] );

    SpatialFilter->SetCheck ( BoolToCheck ( StringIsNotEmpty ( transfer->XyzFile ) /*XyzAndEegMatch*/ ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
void    TTracksFiltersDialog::CmOKEnable ( TCommandEnabler &tce )
{
                                        // Forced off -> enables Ok button (other buttons have their own enabling functions, and are also off)
if ( ! CheckToBool ( On->GetCheck () ) ) {
    tce.Enable ( true );
    return;
    }


TransferData ( tdGetData );
TTracksFiltersStruct*   transfer    = (TTracksFiltersStruct *) GetTransferBuffer ();


double              samplf          = StringToDouble ( transfer->ValueSamplingFrequency );
double              highf           = StringToDouble ( transfer->ValueButterworthHigh   );
double              lowf            = StringToDouble ( transfer->ValueButterworthLow    );
int                 orderh          = StringToInteger ( transfer->OrderHigh );
int                 orderl          = StringToInteger ( transfer->OrderLow  );


if ( CheckToBool ( ButterworthHigh->GetCheck() )
  && ! ( samplf > 0 && highf > 0 && IsInsideLimits ( orderh, ButterworthHighPassMinOrder, ButterworthHighPassMaxOrder ) && IsEven ( orderh ) ) ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( ButterworthLow->GetCheck() ) 
  && ! ( samplf > 0 && lowf > 0 && IsInsideLimits ( orderl, ButterworthLowPassMinOrder, ButterworthLowPassMaxOrder ) && IsEven ( orderl ) ) ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( ButterworthLow->GetCheck() ) && CheckToBool ( ButterworthHigh->GetCheck() ) 
  && ! ( highf < lowf && orderh == orderl ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( Notches->GetCheck() ) && ( samplf <= 0 || NotchesValues ->GetLineLength ( 0 ) == 0 ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( SpatialFilter->GetCheck() ) && ( StringIsEmpty ( transfer->XyzFile ) ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( Envelope->GetCheck() )
  && (    samplf <= 0 
       || ! EnvelopeWidth->GetLineLength ( 0 ) ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( ThresholdKeepAbove->GetCheck() ) && ThresholdKeepAboveValue->GetLineLength ( 0 ) == 0 ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( ThresholdKeepBelow->GetCheck() ) && ThresholdKeepBelowValue->GetLineLength ( 0 ) == 0 ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


void    TTracksFiltersDialog::CmMyOk ()
{
/*
char        buff[256];
double      v;

SamplingFrequency   = StringToDouble (  ((TTracksFiltersStruct*) TransferBuffer)->ValueSamplingFrequency );

double  maxfh   = SamplingFrequency / 2;
double  minfh   = SamplingFrequency / DataWidth;
double  maxfl   = maxfh;
double  minfl   = minfh;


if ( CheckToBool ( ButterworthLow->GetCheck() ) ) {
    ValueButterworthLow->GetLine ( buff, EditSizeValue, 0 );
    v   = StringToDouble ( buff );

    if ( v < minfl || v > maxfl ) {
        sprintf ( buff, "Butterworth low frequency should remain between\n%0.3f  and  %0.3f", minfl, maxfl );
        ShowMessage ( buff, "Filtering parameter", ShowMessageWarning );
        return;
        }
    }

if ( CheckToBool ( ButterworthHigh->GetCheck() ) ) {
    ValueButterworthHigh->GetLine ( buff, EditSizeValue, 0 );
    v   = StringToDouble ( buff );

    if ( v < minfh || v > maxfh ) {
        sprintf ( buff, "Butterworth high frequency should remain between\n%0.3f  and  %0.3f", minfh, maxfh );
        ShowMessage ( buff, "Filtering parameter", ShowMessageWarning );
        return;
        }
    }
*/

TDialog::CmOk();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
