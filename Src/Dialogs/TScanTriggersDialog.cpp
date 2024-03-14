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

#include    <owl/pch.h>

#include    "TScanTriggersDialog.h"

#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"

#include    "TTracksDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TScanTriggersStruct::TScanTriggersStruct ()
{
ClearString ( Channels );
ClearString ( TimeMin );
ClearString ( TimeMax );

ScanStability           = BoolToCheck ( false );

ScanExtrema             = BoolToCheck ( false );
ScanMin                 = BoolToCheck ( false );
ScanMax                 = BoolToCheck ( true  );

ScanThreshold           = BoolToCheck ( true  );
ThresholdAbove          = BoolToCheck ( true  );
StringCopy ( ThresholdAboveValue, "0" );
ThresholdBelow          = BoolToCheck ( false );
StringCopy ( ThresholdBelowValue, "0" );
ThresholdStrict         = BoolToCheck ( true  );
ThresholdSlope          = BoolToCheck ( false );

ScanTemplate            = BoolToCheck ( false );
ClearString ( ScanTemplateFile );
StringCopy ( ScanTemplateThreshold, "50" );

StringCopy ( StabMin, "1" );
StringCopy ( StabMax, "1000" );
StringCopy ( Gap, "1" );

OneMarkerPerTrack       = BoolToCheck ( true  );
MergeMarkers            = BoolToCheck ( false );
StringCopy ( MergeMarkersRange, "0" );

PrefixMarker            = BoolToCheck ( true  );
StringCopy ( PrefixMarkerString, "Thresh" );
TrackName               = BoolToCheck ( true  );
NoValue                 = BoolToCheck ( true  );
TrackValue              = BoolToCheck ( false );
TrackRelativeIndex      = BoolToCheck ( false );
MergedCount             = BoolToCheck ( false );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 ( TScanTriggersDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_BROWSELSFILE,         CmBrowseTemplateFileName ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

    EV_COMMAND_ENABLE           ( IDC_SCANTEMPLATEFILE,     CmTemplateEnable ),
    EV_COMMAND_ENABLE           ( IDC_SCANTEMPLATETHRESHOLD,CmTemplateEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSELSFILE,         CmTemplateEnable ),

    EV_COMMAND_ENABLE           ( IDC_THRESHOLDABOVE,       CmThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDABOVEVALUE,  CmThresholdAboveEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDBELOW,       CmThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDBELOWVALUE,  CmThresholdBelowEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDSTRICT,      CmThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDSMARTSLOPE,  CmThresholdEnable ),

    EV_COMMAND_ENABLE           ( IDC_SCANMIN,              CmExtremaEnable ),
    EV_COMMAND_ENABLE           ( IDC_SCANMAX,              CmExtremaEnable ),

    EV_COMMAND_ENABLE           ( IDC_STABMIN,              CmDurationEnable ),
    EV_COMMAND_ENABLE           ( IDC_STABMAX,              CmDurationEnable ),
    EV_COMMAND_ENABLE           ( IDC_CHANNELS,             CmChannelsEnable ),

    EV_COMMAND_ENABLE           ( IDC_ONEMARKERPERTRACK,    CmNotTemplateEnable ),
    EV_COMMAND_ENABLE           ( IDC_MERGEMARKERS,         CmNotTemplateEnable ),
    EV_COMMAND_ENABLE           ( IDC_MERGEMARKERSRANGE,    CmMergeMarkersRangeEnable ),

    EV_COMMAND_ENABLE           ( IDC_PREFIXMARKERSTRING,   CmPrefixMarkerEnable ),
    EV_COMMAND_ENABLE           ( IDC_TRACKRELATIVEINDEX,   CmNotTemplateEnable ),
    EV_COMMAND_ENABLE           ( IDC_MERGEDCOUNT,          CmMergeMarkersRangeEnable ),
    EV_COMMAND_ENABLE           ( IDC_TRACKNAME,            CmOneMarkerPerTrackEnable ),
    EV_COMMAND_ENABLE           ( IDC_TRACKVALUE,           CmOneMarkerPerTrackEnable ),

    EV_COMMAND                  ( IDC_SCANSTABILITY,        EvMethodChanged ),
    EV_COMMAND                  ( IDC_SCANTHRESHOLD,        EvMethodChanged ),
    EV_COMMAND                  ( IDC_SCANTEMPLATE,         EvMethodChanged ),
    EV_COMMAND                  ( IDC_SCANEXTREMA,          EvMethodChanged ),
    EV_COMMAND                  ( IDC_SCANMIN,              EvMethodChanged ),
    EV_COMMAND                  ( IDC_SCANMAX,              EvMethodChanged ),
    EV_COMMAND                  ( IDC_ONEMARKERPERTRACK,    EvMethodChanged ),
    EV_COMMAND                  ( IDC_MERGEMARKERS,         EvMethodChanged ),

END_RESPONSE_TABLE;


        TScanTriggersDialog::TScanTriggersDialog ( TWindow* parent, TResId resId, TScanTriggersStruct& transfer, TTracksDoc* doc )
      : TBaseDialog ( parent, resId ), EEGDoc ( doc ), scantransfer ( transfer )
{
Channels                = new TEdit         ( this, IDC_CHANNELS, EditSizeTextLong );
TimeMin                 = new TEdit         ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax                 = new TEdit         ( this, IDC_TIMEMAX, EditSizeValue );

ScanStability           = new TRadioButton  ( this, IDC_SCANSTABILITY );

ScanExtrema             = new TRadioButton  ( this, IDC_SCANEXTREMA );
ScanMin                 = new TCheckBox     ( this, IDC_SCANMIN );
ScanMax                 = new TCheckBox     ( this, IDC_SCANMAX );

ScanThreshold           = new TRadioButton  ( this, IDC_SCANTHRESHOLD );
ThresholdAbove          = new TCheckBox     ( this, IDC_THRESHOLDABOVE );
ThresholdAboveValue     = new TEdit         ( this, IDC_THRESHOLDABOVEVALUE, EditSizeValue );
ThresholdBelow          = new TCheckBox     ( this, IDC_THRESHOLDBELOW );
ThresholdBelowValue     = new TEdit         ( this, IDC_THRESHOLDBELOWVALUE, EditSizeValue );
ThresholdStrict         = new TRadioButton  ( this, IDC_THRESHOLDSTRICT );
ThresholdSlope          = new TRadioButton  ( this, IDC_THRESHOLDSMARTSLOPE );

ScanTemplate            = new TRadioButton  ( this, IDC_SCANTEMPLATE );
ScanTemplateFile        = new TEdit         ( this, IDC_SCANTEMPLATEFILE, EditSizeText );
ScanTemplateThreshold   = new TEdit         ( this, IDC_SCANTEMPLATETHRESHOLD, EditSizeValue );
ScanTemplateThreshold->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );

StabMin                 = new TEdit         ( this, IDC_STABMIN, EditSizeValue );
StabMax                 = new TEdit         ( this, IDC_STABMAX, EditSizeValue );
Gap                     = new TEdit         ( this, IDC_GAP, EditSizeValue );

OneMarkerPerTrack       = new TRadioButton  ( this, IDC_ONEMARKERPERTRACK );
MergeMarkers            = new TRadioButton  ( this, IDC_MERGEMARKERS );
MergeMarkersRange       = new TEdit         ( this, IDC_MERGEMARKERSRANGE, EditSizeValue );

PrefixMarker            = new TCheckBox     ( this, IDC_PREFIXMARKER );
PrefixMarkerString      = new TEdit         ( this, IDC_PREFIXMARKERSTRING, EditSizeText );
TrackName               = new TCheckBox     ( this, IDC_TRACKNAME );
NoValue                 = new TRadioButton  ( this, IDC_NOVALUE );
TrackValue              = new TRadioButton  ( this, IDC_TRACKVALUE );
TrackRelativeIndex      = new TRadioButton  ( this, IDC_TRACKRELATIVEINDEX );
MergedCount             = new TRadioButton  ( this, IDC_MERGEDCOUNT );


SetTransferBuffer ( &transfer, sizeof ( transfer ) );
}


        TScanTriggersDialog::~TScanTriggersDialog ()
{
delete  Channels;               delete  TimeMin;                delete  TimeMax;
delete  ScanStability;
delete  ScanExtrema;            delete  ScanMin;                delete  ScanMax;
delete  ScanThreshold;
delete  ThresholdAbove;         delete  ThresholdAboveValue;    delete  ThresholdBelow;    delete  ThresholdBelowValue;
delete  ThresholdStrict;        delete  ThresholdSlope;
delete  ScanTemplate;           delete  ScanTemplateFile;       delete  ScanTemplateThreshold;
delete  StabMin;                delete  StabMax;
delete  Gap;
delete  OneMarkerPerTrack;      delete  MergeMarkers;           delete  MergeMarkersRange;
delete  PrefixMarker;           delete  PrefixMarkerString;
delete  TrackName;
delete  NoValue;                delete  TrackValue;             delete  TrackRelativeIndex;
delete  MergedCount;
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, AllEegRisFilesExt   );

char                buff[ 256 ];
StringCopy ( buff, AllEegRisFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) tracksfiles; i++ )
    SetTemplateFileName ( tracksfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::CmBrowseTemplateFileName ()
{
SetTemplateFileName ( 0 );
}


void    TScanTriggersDialog::SetTemplateFileName ( char *file )
{
static GetFileFromUser  getfile ( "Template File", AllErpEegRisFilesFilter, 2, GetFileRead );

if ( StringIsEmpty ( file ) ) {
    if ( ! getfile.Execute ( scantransfer.ScanTemplateFile ) )
        return;
    }
else {
    StringCopy ( scantransfer.ScanTemplateFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch to template
scantransfer.ScanStability  = BoolToCheck ( false );
scantransfer.ScanThreshold  = BoolToCheck ( false );
scantransfer.ScanTemplate   = BoolToCheck ( true  );
scantransfer.ScanExtrema    = BoolToCheck ( false );

TransferData ( tdSetData );

EvMethodChanged ();


ScanTemplateFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


int                 min = StringToInteger ( scantransfer.TimeMin );
int                 max = StringToInteger ( scantransfer.TimeMax );
const char*         cha =                   scantransfer.Channels;
int                 gap = StringToInteger ( scantransfer.Gap );


if ( min > max || min < 0 || max >= EEGDoc->GetNumTimeFrames () ) {
    tce.Enable ( false );
    return;
    }

if ( gap < 1 ) {
    tce.Enable ( false );
    return;
    }


if      ( CheckToBool ( scantransfer.ScanStability ) ) {

    if ( *cha == 0 ) {
        tce.Enable ( false );
        return;
        }

    int     smin = StringToInteger ( scantransfer.StabMin );
    int     smax = StringToInteger ( scantransfer.StabMax );

    if ( smin > smax || smin < 1 ) {
        tce.Enable ( false );
        return;
        }

    tce.Enable ( true );
    }

else if ( CheckToBool ( scantransfer.ScanThreshold ) ) {

    if ( *cha == 0 ) {
        tce.Enable ( false );
        return;
        }

    if ( ! CheckToBool ( scantransfer.ThresholdAbove ) && ! CheckToBool ( scantransfer.ThresholdBelow ) ) {
        tce.Enable ( false );
        return;
        }

    double  thra = StringToDouble ( scantransfer.ThresholdAboveValue );
    double  thrb = StringToDouble ( scantransfer.ThresholdBelowValue );

    if ( CheckToBool ( scantransfer.ThresholdAbove ) && CheckToBool ( scantransfer.ThresholdBelow )
      && thra > thrb ) {
        tce.Enable ( false );
        return;
        }

    int     smin = StringToInteger ( scantransfer.StabMin );
    int     smax = StringToInteger ( scantransfer.StabMax );

    if ( smin > smax || smin < 1 ) {
        tce.Enable ( false );
        return;
        }

    tce.Enable ( true );
    }

else if ( CheckToBool ( scantransfer.ScanTemplate ) ) {

//    int     threh = StringToInteger ( scantransfer.ScanTemplateThreshold );

    if ( *scantransfer.ScanTemplateFile == 0 ) {
        tce.Enable ( false );
        return;
        }

    tce.Enable ( true );
    }

else if ( CheckToBool ( scantransfer.ScanExtrema ) ) {
    tce.Enable ( CheckToBool ( scantransfer.ScanMin )
              || CheckToBool ( scantransfer.ScanMax ) );
    }
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::EvMethodChanged ()
{
TransferData ( tdGetData );

                                        // update prefix string
if      ( CheckToBool ( scantransfer.ScanStability ) )  StringCopy ( scantransfer.PrefixMarkerString, "Stab" );
else if ( CheckToBool ( scantransfer.ScanThreshold ) )  StringCopy ( scantransfer.PrefixMarkerString, "Thresh" );
else if ( CheckToBool ( scantransfer.ScanTemplate  ) )  StringCopy ( scantransfer.PrefixMarkerString, "Corr" );

else if ( CheckToBool ( scantransfer.ScanExtrema   ) ) {
    if      ( CheckToBool ( scantransfer.ScanMin ) && CheckToBool ( scantransfer.ScanMax ) )
                                                        StringCopy ( scantransfer.PrefixMarkerString, "MinMax" );
    else if ( CheckToBool ( scantransfer.ScanMin ) )    StringCopy ( scantransfer.PrefixMarkerString, "Min" );
    else if ( CheckToBool ( scantransfer.ScanMax ) )    StringCopy ( scantransfer.PrefixMarkerString, "Max" );
    else                                                ClearString ( scantransfer.PrefixMarkerString );
    }

else                                                    ClearString ( scantransfer.PrefixMarkerString );

                                        // no relative index in template scan
if ( CheckToBool ( scantransfer.ScanTemplate  ) && CheckToBool ( scantransfer.TrackRelativeIndex  ) ) {
    scantransfer.NoValue                = BoolToCheck ( false );
    scantransfer.TrackValue             = BoolToCheck ( true  );
    scantransfer.TrackRelativeIndex     = BoolToCheck ( false );
    scantransfer.MergedCount            = BoolToCheck ( false );
    }


                                        // reset some radio buttons
if ( CheckToBool ( scantransfer.OneMarkerPerTrack ) ) {
    if ( CheckToBool ( scantransfer.MergedCount ) ) {
        scantransfer.NoValue                = BoolToCheck ( true  );
        scantransfer.TrackValue             = BoolToCheck ( false );
        scantransfer.TrackRelativeIndex     = BoolToCheck ( false );
        scantransfer.MergedCount            = BoolToCheck ( false );
        }
    }
else if ( CheckToBool ( scantransfer.MergeMarkers ) ) {
    if ( CheckToBool ( scantransfer.TrackValue ) ) {
        scantransfer.NoValue                = BoolToCheck ( true  );
        scantransfer.TrackValue             = BoolToCheck ( false );
        scantransfer.TrackRelativeIndex     = BoolToCheck ( false );
        scantransfer.MergedCount            = BoolToCheck ( false );
        }
    }


TransferData ( tdSetData );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::CmNotTemplateEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( ! CheckToBool ( scantransfer.ScanTemplate ) );
}


void    TScanTriggersDialog::CmOneMarkerPerTrackEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.OneMarkerPerTrack ) );
}


void    TScanTriggersDialog::CmMergeMarkersRangeEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( ! CheckToBool ( scantransfer.ScanTemplate ) && CheckToBool ( scantransfer.MergeMarkers ) );
}


void    TScanTriggersDialog::CmPrefixMarkerEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.PrefixMarker ) );
}


void    TScanTriggersDialog::CmThresholdEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanThreshold ) );
}


void    TScanTriggersDialog::CmDurationEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanStability ) || CheckToBool ( scantransfer.ScanThreshold ) );
}


void    TScanTriggersDialog::CmTemplateEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanTemplate ) );
}


void    TScanTriggersDialog::CmExtremaEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanExtrema ) );
}


void    TScanTriggersDialog::CmThresholdAboveEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanThreshold  )
          && CheckToBool ( scantransfer.ThresholdAbove ) );
}


void    TScanTriggersDialog::CmThresholdBelowEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CheckToBool ( scantransfer.ScanThreshold  )
          && CheckToBool ( scantransfer.ThresholdBelow ) );
}


void    TScanTriggersDialog::CmChannelsEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( ! CheckToBool ( scantransfer.ScanTemplate ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
