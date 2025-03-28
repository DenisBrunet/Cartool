/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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
#include    "Files.TVerboseFile.h"
#include    "Dialogs.Input.h"

#include    "TTracksDoc.h"
#include    "TTracksView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TScanTriggersStruct         ScanTriggersTransfer;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TScanTriggersStruct::TScanTriggersStruct ()
{
StringCopy      ( Channels, TrackNameGFP );
ClearString     ( TimeMin );
ClearString     ( TimeMax );
EndOfFile               = BoolToCheck ( true  );

ScanStability           = BoolToCheck ( false );

ScanExtrema             = BoolToCheck ( true  );
ScanMin                 = BoolToCheck ( false );
ScanMax                 = BoolToCheck ( true  );

ScanThreshold           = BoolToCheck ( false );
ThresholdAbove          = BoolToCheck ( true  );
IntegerToString ( ThresholdAboveValue, 0 );
ThresholdBelow          = BoolToCheck ( false );
IntegerToString ( ThresholdBelowValue, 0 );
ThresholdStrict         = BoolToCheck ( true  );
ThresholdSlope          = BoolToCheck ( false );

ScanTemplate            = BoolToCheck ( false );
ClearString     ( ScanTemplateFile );
IntegerToString ( ScanTemplateThreshold, 50 );

IntegerToString ( StabMin,    1 );
IntegerToString ( StabMax, 1000 );
IntegerToString ( Gap,        1 );

OneMarkerPerTrack       = BoolToCheck ( true  );
MergeMarkers            = BoolToCheck ( false );
IntegerToString ( MergeMarkersRange, 0 );

PrefixMarker            = BoolToCheck ( true  );
StringCopy      ( PrefixMarkerString, "Max" );
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

    EV_COMMAND_ENABLE           ( IDC_CHANNELS,             CmChannelsEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMAX,              CmTimeMaxEnable ),

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

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,       CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,         CmProcessBatchEnable ),

END_RESPONSE_TABLE;


        TScanTriggersDialog::TScanTriggersDialog ( TWindow* parent, TResId resId, TTracksDoc* doc )
      : TBaseDialog ( parent, resId, doc )
{
                                        // can also process .data and .seg
StringCopy ( BatchFilesExt, AllEegRisFilesExt " " AllSegDataExt );


Channels                = new TEdit         ( this, IDC_CHANNELS, EditSizeTextLong );
TimeMin                 = new TEdit         ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax                 = new TEdit         ( this, IDC_TIMEMAX, EditSizeValue );
EndOfFile               = new TCheckBox     ( this, IDC_TOENDOFFILE );

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

                                        // are we entering batch?
BatchProcessing         = doc == 0;


SetTransferBuffer ( dynamic_cast <TScanTriggersStruct*> ( &ScanTriggersTransfer ) );


if ( StringIsEmpty ( ScanTriggersTransfer.TimeMin ) ) {
    StringCopy  ( ScanTriggersTransfer.TimeMin, "0" );
//  ClearString ( ScanTriggersTransfer.TimeMax );
    ScanTriggersTransfer.EndOfFile      = BoolToCheck ( true  );
    }
}


        TScanTriggersDialog::~TScanTriggersDialog ()
{
delete  Channels;               delete  TimeMin;                delete  TimeMax;            delete  EndOfFile;
delete  ScanStability;
delete  ScanExtrema;            delete  ScanMin;                delete  ScanMax;
delete  ScanThreshold;
delete  ThresholdAbove;         delete  ThresholdAboveValue;    delete  ThresholdBelow;     delete  ThresholdBelowValue;
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
TPointInt           where;
TGoF                tracksfiles     ( drop, BatchFilesExt, &where );

char                buff[ KiloByte ];
StringCopy ( buff, BatchFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) tracksfiles ) {

    if ( (int) tracksfiles == 1 && IsInsideLimits ( where.Y, 390, 520 ) )
                                        // dropping a single file at restricted position
        SetTemplateFileName ( tracksfiles[ 0 ] );

    else {
                                        // dropping multiple files for sure is batch processing
        if ( BatchProcessing )

            BatchProcessDropped ( tracksfiles );
        else

            ShowMessage ( BatchNotAvailMessage, ScanningTriggersTitle, ShowMessageWarning );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::CmBrowseTemplateFileName ()
{
SetTemplateFileName ( 0 );
}


void    TScanTriggersDialog::SetTemplateFileName ( const char* file )
{
static GetFileFromUser  getfile ( "Template File", AllErpEegRisFilesFilter, 2, GetFileRead );

if ( StringIsEmpty ( file ) ) {
    if ( ! getfile.Execute ( ScanTriggersTransfer.ScanTemplateFile ) )
        return;
    }
else {
    StringCopy ( ScanTriggersTransfer.ScanTemplateFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch to template
ScanTriggersTransfer.ScanStability  = BoolToCheck ( false );
ScanTriggersTransfer.ScanThreshold  = BoolToCheck ( false );
ScanTriggersTransfer.ScanTemplate   = BoolToCheck ( true  );
ScanTriggersTransfer.ScanExtrema    = BoolToCheck ( false );

TransferData ( tdSetData );

EvMethodChanged ();


ScanTemplateFile->ResetCaret;
}


//----------------------------------------------------------------------------
bool    TScanTriggersDialog::CmProcessEnable ()
{
int                 timemin         = StringToInteger ( ScanTriggersTransfer.TimeMin );
int                 timemax         = StringToInteger ( ScanTriggersTransfer.TimeMax );
bool                endoffile       = CheckToBool     ( ScanTriggersTransfer.EndOfFile );

if ( timemin < 0 )
    return  false;

if ( ! endoffile && (    timemin > timemax 
                      || EEGDoc && timemax >= EEGDoc->GetNumTimeFrames () ) )
    return  false;


int                 gap             = StringToInteger ( ScanTriggersTransfer.Gap );

if ( gap < 1 )
    return  false;


const char*         channels        = ScanTriggersTransfer.Channels;

if      ( CheckToBool ( ScanTriggersTransfer.ScanStability ) ) {

    if ( StringIsEmpty ( channels ) )
        return  false;


    int         smin    = StringToInteger ( ScanTriggersTransfer.StabMin );
    int         smax    = StringToInteger ( ScanTriggersTransfer.StabMax );

    if ( smin > smax || smin < 1 )
        return  false;

    return  true;
    }

else if ( CheckToBool ( ScanTriggersTransfer.ScanThreshold ) ) {

    if ( StringIsEmpty ( channels ) )
        return  false;

    if ( ! CheckToBool ( ScanTriggersTransfer.ThresholdAbove ) 
      && ! CheckToBool ( ScanTriggersTransfer.ThresholdBelow ) )

        return  false;


    double      threshabove = StringToDouble ( ScanTriggersTransfer.ThresholdAboveValue );
    double      threshbelow = StringToDouble ( ScanTriggersTransfer.ThresholdBelowValue );

    if ( CheckToBool ( ScanTriggersTransfer.ThresholdAbove ) 
      && CheckToBool ( ScanTriggersTransfer.ThresholdBelow )
      && threshabove > threshbelow )

        return  false;


    int         stabmin     = StringToInteger ( ScanTriggersTransfer.StabMin );
    int         stabmax     = StringToInteger ( ScanTriggersTransfer.StabMax );

    if ( stabmin > stabmax 
      || stabmin < 1    )

        return  false;

    return  true;
    }

else if ( CheckToBool ( ScanTriggersTransfer.ScanTemplate ) ) {

//  int         threh   = StringToInteger ( ScanTriggersTransfer.ScanTemplateThreshold );

    if ( StringIsEmpty ( ScanTriggersTransfer.ScanTemplateFile ) )
        return  false;

    return  true;
    }

else if ( CheckToBool ( ScanTriggersTransfer.ScanExtrema ) ) {

    return  CheckToBool ( ScanTriggersTransfer.ScanMin )
         || CheckToBool ( ScanTriggersTransfer.ScanMax );
    }


return  true;
}


void    TScanTriggersDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


void    TScanTriggersDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( ! BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::EvMethodChanged ()
{
TransferData ( tdGetData );

                                        // update prefix string
if      ( CheckToBool ( ScanTriggersTransfer.ScanStability ) )      StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Stab" );

else if ( CheckToBool ( ScanTriggersTransfer.ScanThreshold ) ) {

    if      ( CheckToBool ( ScanTriggersTransfer.ThresholdAbove ) 
           && CheckToBool ( ScanTriggersTransfer.ThresholdBelow ) ) StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Between" );
    else if ( CheckToBool ( ScanTriggersTransfer.ThresholdAbove ) ) StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Above" );
    else if ( CheckToBool ( ScanTriggersTransfer.ThresholdBelow ) ) StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Below" );
    else                                                            ClearString( ScanTriggersTransfer.PrefixMarkerString );
    }

else if ( CheckToBool ( ScanTriggersTransfer.ScanTemplate  ) )      StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Corr" );

else if ( CheckToBool ( ScanTriggersTransfer.ScanExtrema   ) ) {
    if      ( CheckToBool ( ScanTriggersTransfer.ScanMin ) 
           && CheckToBool ( ScanTriggersTransfer.ScanMax ) )        StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "MinMax" );
    else if ( CheckToBool ( ScanTriggersTransfer.ScanMin ) )        StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Min" );
    else if ( CheckToBool ( ScanTriggersTransfer.ScanMax ) )        StringCopy ( ScanTriggersTransfer.PrefixMarkerString, "Max" );
    else                                                            ClearString( ScanTriggersTransfer.PrefixMarkerString );
    }

else                                                                ClearString( ScanTriggersTransfer.PrefixMarkerString );


                                        // no relative index in template scan
if ( CheckToBool ( ScanTriggersTransfer.ScanTemplate  ) && CheckToBool ( ScanTriggersTransfer.TrackRelativeIndex  ) ) {
    ScanTriggersTransfer.NoValue                = BoolToCheck ( false );
    ScanTriggersTransfer.TrackValue             = BoolToCheck ( true  );
    ScanTriggersTransfer.TrackRelativeIndex     = BoolToCheck ( false );
    ScanTriggersTransfer.MergedCount            = BoolToCheck ( false );
    }


                                        // reset some radio buttons
if ( CheckToBool ( ScanTriggersTransfer.OneMarkerPerTrack ) ) {
    if ( CheckToBool ( ScanTriggersTransfer.MergedCount ) ) {
        ScanTriggersTransfer.NoValue                = BoolToCheck ( true  );
        ScanTriggersTransfer.TrackValue             = BoolToCheck ( false );
        ScanTriggersTransfer.TrackRelativeIndex     = BoolToCheck ( false );
        ScanTriggersTransfer.MergedCount            = BoolToCheck ( false );
        }
    }
else if ( CheckToBool ( ScanTriggersTransfer.MergeMarkers ) ) {
    if ( CheckToBool ( ScanTriggersTransfer.TrackValue ) ) {
        ScanTriggersTransfer.NoValue                = BoolToCheck ( true  );
        ScanTriggersTransfer.TrackValue             = BoolToCheck ( false );
        ScanTriggersTransfer.TrackRelativeIndex     = BoolToCheck ( false );
        ScanTriggersTransfer.MergedCount            = BoolToCheck ( false );
        }
    }


TransferData ( tdSetData );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::CmNotTemplateEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( ScanTemplate->GetCheck () ) );
}


void    TScanTriggersDialog::CmOneMarkerPerTrackEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( OneMarkerPerTrack->GetCheck () ) );
}


void    TScanTriggersDialog::CmMergeMarkersRangeEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( ScanTemplate->GetCheck () ) && CheckToBool ( MergeMarkers->GetCheck () ) );
}


void    TScanTriggersDialog::CmPrefixMarkerEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( PrefixMarker->GetCheck () ) );
}


void    TScanTriggersDialog::CmThresholdEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanThreshold->GetCheck () ) );
}


void    TScanTriggersDialog::CmDurationEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanStability->GetCheck () ) || CheckToBool ( ScanThreshold->GetCheck () ) );
}


void    TScanTriggersDialog::CmTemplateEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanTemplate->GetCheck () ) );
}


void    TScanTriggersDialog::CmExtremaEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanExtrema->GetCheck () ) );
}


void    TScanTriggersDialog::CmThresholdAboveEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanThreshold ->GetCheck () )
          && CheckToBool ( ThresholdAbove->GetCheck () ) );
}


void    TScanTriggersDialog::CmThresholdBelowEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ScanThreshold ->GetCheck () )
          && CheckToBool ( ThresholdBelow->GetCheck () ) );
}


void    TScanTriggersDialog::CmChannelsEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( ScanTemplate->GetCheck () ) );
}


void    TScanTriggersDialog::CmTimeMaxEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( EndOfFile->GetCheck() ) );
}


//----------------------------------------------------------------------------
void    TScanTriggersDialog::ProcessCurrent ( void* usetransfer, const char* freqinfix )
{
if ( EEGDoc == 0 )
    return;


TScanTriggersStruct*    transfer    = usetransfer ? (TScanTriggersStruct*) usetransfer : &ScanTriggersTransfer;

if ( ! transfer )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numregel        = EEGDoc->GetNumElectrodes   ();
int                 numtotalel      = EEGDoc->GetTotalElectrodes ();
int                 numtimeframes   = EEGDoc->GetNumTimeFrames   ();
int                 lasttimeframe   = numtimeframes - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // select which tracks to scan
TSelection          elsel ( numtotalel, OrderSorted );
int                 numsel          = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the method type
bool                scanstability   = CheckToBool ( transfer->ScanStability );
bool                scanextrema     = CheckToBool ( transfer->ScanExtrema   );
bool                scanthreshold   = CheckToBool ( transfer->ScanThreshold );
bool                scantemplate    = CheckToBool ( transfer->ScanTemplate  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time interval
bool                endoffile       = CheckToBool     ( transfer->EndOfFile );
int                 timemin         = StringToInteger ( transfer->TimeMin );
int                 timemax         = endoffile ? lasttimeframe 
                                                : StringToInteger ( transfer->TimeMax );

Clipped ( timemin, timemax, 0, lasttimeframe );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scanstability

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scanextrema
bool                scanmin         = CheckToBool    ( transfer->ScanMin ) && scanextrema;
bool                scanmax         = CheckToBool    ( transfer->ScanMax ) && scanextrema;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scanthreshold
bool                abovethresh     = CheckToBool    ( transfer->ThresholdAbove ) && scanthreshold;
double              threshabove     = StringToDouble ( transfer->ThresholdAboveValue );
bool                belowthresh     = CheckToBool    ( transfer->ThresholdBelow ) && scanthreshold;
double              threshbelow     = StringToDouble ( transfer->ThresholdBelowValue );

bool                smartslope      = CheckToBool ( transfer->ThresholdSlope ) && scanthreshold;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scantemplate
double              templthresh     = StringToDouble ( transfer->ScanTemplateThreshold ) / 100.0;

TFileName           templatefile    = transfer->ScanTemplateFile;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // postprocessing parameters

                                        // scanstability || scanthreshold
int                 minstable       = scanstability || scanthreshold ? StringToInteger ( transfer->StabMin ) : 1;
int                 maxstable       = scanstability || scanthreshold ? StringToInteger ( transfer->StabMax ) : 1;

Clipped ( minstable, maxstable, 1, timemax - timemin + 1 );

                                        // all
int                 mingap          = StringToInteger ( transfer->Gap );

                                        // ! scantemplate
bool                onemarker       = CheckToBool ( transfer->OneMarkerPerTrack ) && ! scantemplate;
bool                mergemarkers    = CheckToBool ( transfer->MergeMarkers      ) && ! scantemplate;
int                 mergerange      = mergemarkers ? StringToInteger ( transfer->MergeMarkersRange ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scantemplate
TTracks<float>      templb;
int                 templw;             // full width
int                 templorg;           // offset to origin
int                 templtail;          // remaining part from origin (included)


if ( scantemplate ) {

    int                 numeltempl      = 0;


    if ( ! ReadFromHeader ( templatefile, ReadNumElectrodes, &numeltempl ) ) {
        ShowMessage ( "Can not read the number of electrodes!", templatefile, ShowMessageWarning );
        return;
        }

    if ( numregel != numeltempl ) {
        ShowMessage ( "Not the same number of electrodes with template file!", templatefile, ShowMessageWarning );
        return;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TOpenDoc< TTracksDoc >  TemplDoc ( templatefile, OpenDocHidden );

//    if ( ! (bool) TemplDoc->UsedBy )
//        TemplDoc->MinimizeViews ();


    templw          = TemplDoc->GetNumTimeFrames ();
    int gfpindex    = TemplDoc->GetGfpIndex ();
    templb.Resize ( TemplDoc->GetTotalElectrodes (), templw );

                                        // read the template
    TemplDoc->GetTracks (   0,      templw - 1, 
                            templb, 0, 
                            AtomTypeUseCurrent, 
                            ComputePseudoTracks, 
                            numeltempl > 1 ? ReferenceAverage : ReferenceAsInFile 
                        );


    TemplDoc.Close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // normalize template
    double      sumdata     = 0;

    for ( int e      = 0; e      < numregel; e++      )
    for ( int templi = 0; templi < templw;   templi++ )

        sumdata    += templb[ e ][ templi ] * templb[ e ][ templi ];


    if ( sumdata == 0 ) {
        ShowMessage ( "Template is empty!", "Scanning with Template", ShowMessageWarning );
        return;
        }

    templb     /= NonNull ( sqrt ( sumdata ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find relative origin in the template
    double      maxgfp = -1;

    templorg    = 0;

    for ( int templi = 0; templi < templw; templi++ )

        if ( templb[ gfpindex ][ templi ] > maxgfp ) {

            maxgfp   = templb[ gfpindex ][ templi ];
            templorg = templi;
            }


    templtail   = templw - templorg;    // tail, including the "0" position
                                        // adjust boundaries
    timemin    -= templorg;

    timemax    += templtail - 1;        // timemax already includes the "0"

    Clipped ( timemin, timemax, 0, lasttimeframe );


    if ( timemax - timemin + 1 < templw ) {
        ShowMessage ( "The time interval selected is too small compared to the Template!", "Scanning with Template", ShowMessageWarning );
        return;
        }
    } // if scantemplate

else { // ! scantemplate

    elsel.Set ( transfer->Channels, EEGDoc->GetElectrodesNames () );

    numsel  = elsel.NumSet();

    if ( numsel == 0 )
        return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // output parameters
bool                prefixmarker    = CheckToBool ( transfer->PrefixMarker       );
bool                trackname       = CheckToBool ( transfer->TrackName          ) && onemarker;
bool                trackvalue      = CheckToBool ( transfer->TrackValue         ) && onemarker;
bool                relativeindex   = CheckToBool ( transfer->TrackRelativeIndex );
bool                mergedcount     = CheckToBool ( transfer->MergedCount        ) && mergemarkers;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                markertext  [ 2 * MarkerNameMaxLength ];
char                prefix      [ KiloByte ];
char                buff        [ KiloByte ];
TFileName           templatename;


if ( scantemplate && trackname ) {
    StringCopy  ( templatename, templatefile );
    GetFilename ( templatename );
    }


StringCopy ( prefix, prefixmarker ? transfer->PrefixMarkerString : "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Done with parameters extraction
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( ScanningTriggersTitle, timemax - timemin + 1 );

                                        // prevent closing the original file, and the app !!
EEGDoc->PreventClosing ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // In case of multiple sessions, write only a single verbose file as we don't really provide per-session details
if ( ! EEGDoc->HasMultipleSessions () || EEGDoc->GetCurrentSession () == 1 ) {

    TFileName       verbosefilename ( EEGDoc->GetDocPath () );

    verbosefilename.AddExtension        ( ScanningTriggersTitle );
    verbosefilename.AddExtension        ( FILEEXT_VRB );
    verbosefilename.CheckNoOverwrite    ();


    TVerboseFile    verbose ( verbosefilename, VerboseFileDefaultWidth );

    verbose.PutTitle ( ScanningTriggersTitle );


    verbose.NextTopic ( "Files:" );
    {
    verbose.Put ( "Input   File         :", EEGDoc->GetDocPath () );

    if ( scantemplate )
        verbose.Put ( "Template file        :", templatefile );


    TFileName       fileout     = TFileName ( EEGDoc->GetDocPath () ) + ( EEGDoc->HasMultipleSessions () ? ".*." FILEEXT_MRK : "." FILEEXT_MRK );

    verbose.Put ( EEGDoc->HasMultipleSessions () ? "Output  Files        :" : "Output  File         :", fileout );

    verbose.Put ( "Verbose File (this)  :", verbosefilename );
    }


    verbose.NextTopic ( "What to Scan:" );
    {
    verbose.Put ( "Tracks         :", elsel.ToText ( buff, EEGDoc->GetElectrodesNames () ) );
    verbose.Put ( "From time frame:", timemin );
    verbose.Put ( "To   time frame:", timemax );

    if ( EEGDoc->HasMultipleSessions () )
        verbose.Put ( "Sessions       :", "All sessions" );
    }


    verbose.NextTopic ( "EEG Parameters:" );
    {
    verbose.Put ( "Auxiliary channels:", EEGDoc->GetNumAuxElectrodes   () ? EEGDoc->GetAuxTracks   ().ToText ( buff, EEGDoc->GetElectrodesNames () ) : "None" );
    verbose.Put ( "Bad channels:",       EEGDoc->GetNumBadElectrodes   () ? EEGDoc->GetBadTracks   ().ToText ( buff, EEGDoc->GetElectrodesNames () ) : "None" );
    verbose.Put ( "Valid channels:",     EEGDoc->GetNumValidElectrodes () ? EEGDoc->GetValidTracks ().ToText ( buff, EEGDoc->GetElectrodesNames () ) : "None" );

    verbose.NextLine ();
    verbose.Put ( "Current filters:",   EEGDoc->GetFilters ()->ParametersToText ( buff ) );

    verbose.NextLine ();
    auto        ref         = EEGDoc->GetReferenceType ();
    if      ( ref == ReferenceAsInFile          )       verbose.Put ( "Reference:", "No reference, as in file" );
    else if ( ref == ReferenceAverage           )       verbose.Put ( "Reference:", "Average reference" );
    else if ( ref == ReferenceArbitraryTracks   )       verbose.Put ( "Reference:", EEGDoc->GetReferenceTracks ().ToText ( buff, EEGDoc->GetElectrodesNames (), AuxiliaryTracksNames ) );
    else                                                verbose.Put ( "Reference:", "Unknown" );
    }


    verbose.NextTopic ( "Scanning Parameters:" );
    {
    verbose.Put ( "Scanning method:",   scanstability   ? "Periods of Stability"
                                      : scanextrema     ? "Local Extrema"
                                      : scanthreshold   ? "Thresholds"
                                      : scantemplate    ? "Template Matching"
                                      :                   "Unknown Method"
                );


    if      ( scanstability ) {

        verbose.Put ( "Generating markers for:", "Equal consecutive values" );
        }

    else if ( scanextrema ) {

        verbose.Put ( "Generating markers for:", scanmin && scanmax ? "Local Min or Local Max"
                                               : scanmin            ? "Local Min"
                                               :          /*scanmax*/ "Local Max"
                    );
        }

    else if ( scanthreshold ) {

        verbose.Put ( "Generating markers for:", abovethresh && belowthresh ? "Above or below threshold values"
                                               : abovethresh                ? "Above threshold values"
                                               :              /*belowthresh*/ "Below threshold values"
                    );
        if ( abovethresh )
            verbose.Put ( "Above threshold value:", threshabove, 2 );
        if ( belowthresh )
            verbose.Put ( "Below threshold value:", threshbelow, 2 );

        verbose.Put ( "Setting marker position:", smartslope ? "At the highest slope around threshold" : "At the first time point past threshold" );
        }

    else if ( scantemplate ) {

        verbose.Put ( "Generating markers for:", "Template correlating above threshold" );
        verbose.Put ( "Template file:", templatefile );
        verbose.Put ( "Min acceptable correlation:", templthresh, 2 );
        }

    if ( scanstability || scanthreshold ) {
        verbose.NextLine ();
        verbose.Put ( "Min Stability Duration:", minstable );
        verbose.Put ( "Max Stability Duration:", maxstable );
        }

    verbose.NextLine ();

    verbose.Put ( "Min gap between successive markers:", mingap, 0, " [TF]" );

    if ( ! scantemplate ) {
        verbose.Put ( "Handling of simultaneous markers:", onemarker ? "One marker per track" : "Merged into single marker" );
        if ( mergemarkers )
            verbose.Put ( "Merging within relative tolerance:", mergerange, 0, " [TF]" );
        }
    }


    verbose.NextTopic ( "Naming Markers:" );
    {
    verbose.Put ( "Beginning with prefix:", prefixmarker    ? StringCopy ( buff, "'", prefix, "'" ) // better wrap this string in quotes
                                                            : "No prefix" );

    if ( scantemplate )
        verbose.Put ( "Including template name:", trackname ? templatename 
                                                            : "No template name added" );
    else
        verbose.Put ( "Including track name:", trackname    ? "Adding track name" 
                                                            : "No track name added" );

    if ( scantemplate )
        verbose.Put ( "Ending with value:", trackvalue      ? "Adding correlation value"
                                          :                   "No value added" );
    else
        verbose.Put ( "Ending with value:", trackvalue      ? "Adding track value"
                                          : relativeindex   ? "Adding relative index" 
                                          : mergedcount     ? "Adding merged count" 
                                          :                   "No value added" );
    }

    verbose.NextLine ();
    verbose.NextLine ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for the sake of speed, use a big buffer to load everyting we need at once!
TTracks<float>      eegb ( numtotalel, ( timemax - timemin + 1 ) );
MarkersList         newmarkers;


if ( scanstability || scanthreshold || scanextrema ) {
                                        // store info for each channel
    enum            {
                    value2,             // value before -2
                    value1,             // value before -1
                    value,
                    onset,
                    count,
                    nextonset,
                    numscanvar
                    };

    TArray2<float>  chan ( numtotalel, numscanvar );

                                        // get the whole data at once, already filtered
    EEGDoc->GetTracks   (   timemin,    timemax, 
                            eegb,       0, 
                            AtomTypeUseCurrent, 
                            ComputePseudoTracks, 
                            ReferenceUsingCurrent 
                        );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset counters
    for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli ) {

        chan[ eli() ][ value2    ]  =  0;
        chan[ eli() ][ value1    ]  =  0;
        chan[ eli() ][ value     ]  =  eegb[ eli() ][ 0 ];
        chan[ eli() ][ onset     ]  = -1;
        chan[ eli() ][ count     ]  =  0;
        chan[ eli() ][ nextonset ]  = -1;
        }


    for ( int tf = timemin, tfi = 0; tf <= timemax; tf++, tfi++ ) {

        Gauge.Next ();

                                        // !binary indexes will only work up to 16 tracks, as it is stored in a short int!
        int         binarycode      = 1;

        for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli, binarycode <<= 1 ) {

            int     i               = eli();

                                        // compute the condition
            bool    update      = scanstability &&                                   eegb[ i ][ tfi ] == chan[ i ][ value ]
                               || scanthreshold && ( ! abovethresh || abovethresh && eegb[ i ][ tfi ] >= threshabove )
                                                && ( ! belowthresh || belowthresh && eegb[ i ][ tfi ] <= threshbelow );

            if ( update ) {
                                        // store duration informations, if needed

                                        // push previous values
                chan[ i ][ value2 ] = chan[ i ][ value1 ];
                chan[ i ][ value1 ] = chan[ i ][ value  ];
                                        // store current value
                chan[ i ][ value ]  = eegb[ i ][ tfi ];
                                        // increment counter
                chan[ i ][ count ]++;
                                        // TF of first occurence
                if ( chan[ i ][ onset ] < 0 )
                    chan[ i ][ onset ] = scanstability && tf > timemin ? tf - 1 : tf;
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // condition not OK, or leaving an OK condition, or last TF (finishing the job), or not concerned by duration
            if ( ! update || tf == timemax ) {

                           // no duration for Min / Max
                bool    duration    = scanextrema || chan[ i ][ count ] >= minstable && chan[ i ][ count ] <= maxstable;

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                bool    condition   = false;

                if      ( scanstability )   condition   = (bool) chan[ i ][ value ];    // stability for non-null values

                else if ( scanthreshold )   condition   = ( ! abovethresh || abovethresh && chan[ i ][ value ] >= threshabove )
                                                       && ( ! belowthresh || belowthresh && chan[ i ][ value ] <= threshbelow );

                else if ( scanextrema && tf - timemin > 2  ) {
                                        // actually, 1 TF too late here, but doesn't harm, except when scanning very small intervals (like 4 TF)

                                        // problem if there is a plateau somewhere (rare case)
                    double  delta1      = chan[ i ][ value1 ] - chan[ i ][ value2 ];
                    double  delta2      = chan[ i ][ value  ] - chan[ i ][ value1 ];

                            delta1      = delta1 > 0 ? 1 : delta1 < 0 ? -1 : 0;
                            delta2      = delta2 > 0 ? 1 : delta2 < 0 ? -1 : 0;
                                        // "laplacian of signs"
                    int     dsd         = delta2 - delta1;

                    condition   = scanmax && dsd == -2 || scanmin && dsd ==  2;

                    if ( condition ) {  // we never entered the previous "if", so we are late of 1 TF, so update these fields now
                        chan[ i ][ onset ] = tf - 2;                // onset is there
                        }
                    }
                else                        condition   = false;

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                bool    timing      = chan[ i ][ onset ] > ( chan[ i ][ nextonset ] - ( scanextrema ? 1 : 0 ) );

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if ( duration && timing && condition ) {

                                        // default position is the onset
                    int     besttf  = chan[ i ][ onset ];


                    if ( smartslope ) { // adjust the tf to the highest slope, either before or after

                                        // we need to know which test it is
                        bool    isitdown        = belowthresh && chan[ i ][ value ] <= threshbelow;
                                        // clip index to safe range
                        int     besttfi         = Clip ( besttf - timemin, 1, timemax - timemin - 1 );
                                        // slope at current best tf
                        double  delta0          = ( isitdown ? -1 : 1 ) * ( eegb[ i ][ besttfi + 1 ] - eegb[ i ][ besttfi - 1 ] );


                                        // scan after
                        int     besttfup        = besttf;
                        double  bestdeltaup     = 0;
                        double  delta1          = delta0;

                        for ( int tf2i = besttfi, tf2 = timemin + tf2i; tf2 <= timemax - 1; tf2i++, tf2++ ) {

                            double  delta2      = ( isitdown ? -1 : 1 ) * ( eegb[ i ][ tf2i + 1 ] - eegb[ i ][ tf2i - 1 ] );

//                            DBGV3 ( tf2, delta1, delta2, "UP: tf2, delta1, delta2" );

                            if ( delta2 >= delta1 ) {
                                bestdeltaup     = delta2;
                                besttfup        = tf2;
                                delta1          = delta2;
                                }
                            else
                                break;
                            }

                                        // scan before
                        int     besttfdown      = besttf;
                        double  bestdeltadown   = 0;
                                delta1          = delta0;

                        for ( int tf2i = besttfi, tf2 = timemin + tf2i; tf2 >= timemin + 1; tf2i--, tf2-- ) {

                            double  delta2  = ( isitdown ? -1 : 1 ) * ( eegb[ i ][ tf2i + 1 ] - eegb[ i ][ tf2i - 1 ] );

//                            DBGV3 ( tf2, delta1, delta2, "DOWN: tf2, delta1, delta2" );

                            if ( delta2 >= delta1 ) {
                                bestdeltadown   = delta2;
                                besttfdown      = tf2;
                                delta1          = delta2;
                                }
                            else
                                break;
                            }

                                        // take the one side with highest slope
                        besttf      = besttfup == besttfdown      ? besttf
                                    : bestdeltaup > bestdeltadown ? besttfup : besttfdown;

//                        DBGV4 ( chan[ i ][ onset ], besttfup, besttfdown, besttf, "tf, besttfup, besttfdown, besttf" );
                        } // smartslope


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                    MarkerCode  markercode = (MarkerCode) binarycode;

                                        // cook marker name
                    ClearString ( markertext );

                    if ( StringIsNotEmpty ( prefix ) )
                        StringCopy   ( markertext, prefix );

                    if ( trackname )
                        StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", EEGDoc->GetElectrodeName ( i ) );

                    if ( trackvalue )
                        StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", FloatToString ( buff, chan[ i ][ value ] ) );

                    if ( relativeindex )
                        StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", IntegerToString ( buff, binarycode ) );

                    StringShrink ( markertext, markertext, MarkerNameMaxLength - 1 );


                                        // everything OK, add marker
                    newmarkers.Append ( new TMarker (   besttf, besttf,
                                                        markercode,
                                                        markertext,
                                                        mergemarkers ? MarkerTypeTemp : MarkerTypeMarker ) );

                                        // set next possible onset, for this track
                    chan[ i ][ nextonset ]  = tf + mingap - 1;
                    } // if adding marker


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // push previous values
                chan[ i ][ value2 ] = chan[ i ][ value1 ];
                chan[ i ][ value1 ] = chan[ i ][ value  ];
                                        // store current value
                chan[ i ][ value ]  = eegb[ i ][ tfi ];
                                        // reset counter
                chan[ i ][ count ]  = 0;
                                        // reset TF of first occurence
                chan[ i ][ onset ]  = -1;
                } // current track & TF not qualified

            } // for track

        } // for tf

    } // scanstability || scanthreshold || scanextrema


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( scantemplate ) {
                                        // get the whole data at once, already filtered
    EEGDoc->GetTracks   (   timemin,    timemax, 
                            eegb,       0, 
                            AtomTypeUseCurrent, 
                            ComputePseudoTracks, 
                            numregel > 1 ? ReferenceAverage : ReferenceAsInFile 
                        );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int             maxtf       = timemin;
    double          maxcorr     = Lowest ( maxcorr );
    bool            corrabove   = false;

                                        // tfi starts at beginning of template (not templorg)
    for ( int tf = timemin, tfi = 0; tf <= timemax - templw + 1; tf++, tfi++ ) {

        Gauge.Next ();

                                        // a bit of screen refreshing, tamplate scanning is inherently slower, it is nice to animate something!
        if ( Truncate ( ( tfi * 4.0 ) / ( timemax - timemin ) ) != Truncate ( ( ( tfi + 1 ) * 4.0 ) / ( timemax - timemin ) ) ) {
            Invalidate ( false );
//          ShowNow ();
//          UpdateApplication;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute correlation
        double      sumdata     = 0;
        double      corrdata    = 0;

        for ( int e      = 0; e      < numregel; e++      )
        for ( int templi = 0; templi < templw;   templi++ ) {

            corrdata += eegb[ e ][ tfi + templi ] * templb[ e ][       templi ];
            sumdata  += eegb[ e ][ tfi + templi ] * eegb  [ e ][ tfi + templi ];
            }

        corrdata   /= NonNull ( sqrt ( sumdata ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( corrdata < templthresh ) {

            if ( corrabove ) {          // a previous found?

                MarkerCode  markercode     = (MarkerCode) Round ( maxcorr * 100 );

                                        // cook marker name
                ClearString ( markertext );

                if ( StringIsNotEmpty ( prefix ) )
                    StringCopy   ( markertext, prefix );

                if ( trackname )        // replace track name by template file name
                    StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", templatename );

                if ( trackvalue )       // replace track value by correlation value
                    StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", IntegerToString ( buff, Round ( maxcorr * 100 ), 2 ) );

                StringShrink ( markertext, markertext, MarkerNameMaxLength - 1 );


                                        // everything OK, add marker
                newmarkers.Append ( new TMarker ( maxtf + templorg, maxtf + templorg, markercode, markertext, MarkerTypeMarker ) );

                                        // jump (update the 2 indexes)
                tf          = max ( maxtf           + mingap - 1, tf  );
                tfi         = max ( maxtf - timemin + mingap - 1, tfi );

                maxtf       = tf;
                maxcorr     = Lowest ( maxcorr );
                corrabove   = false;
                }

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        corrabove = true;               // ok, we found at least something

        if ( corrdata > maxcorr ) {     // but keep only the best
            maxtf   = tf;
            maxcorr = corrdata;
            }

        } // for tf

    } // scantemplate


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // post-processing
if ( mergemarkers ) {
                                        // newmarkers list was not forced sort, but it should actually BE sorted
    MarkersList         mergedmarkers;

    int                 numtemp         = newmarkers.Num ();

    int                 markeri2;
                                        // scan temp list for overlaps
    for ( int markeri = 0; markeri < numtemp; markeri++ ) {

                                        // go beyond limit, to avoid missing the last marker
        for ( markeri2 = markeri; markeri2 <= numtemp; markeri2++ ) {

                                        // test only From, we know there is no extent
            if ( markeri2 < numtemp && abs ( newmarkers[ markeri2 ]->From - newmarkers[ markeri ]->From ) <= mergerange )
                continue;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan the range of markers
            MarkerCode  markercode      = 0;
            double      position        = 0;

            for ( int markeri3 = markeri; markeri3 < markeri2; markeri3++ ) {

                position    += newmarkers[ markeri3 ]->From;

                if ( mergedcount )  markercode++;                               // either a count
                else                markercode |= newmarkers[ markeri3 ]->Code; // or a binary combination
                }

            position        /= markeri2 - markeri;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cook marker name
            ClearString ( markertext );

            if ( StringIsNotEmpty ( prefix ) )
                StringCopy   ( markertext, prefix );

            if ( relativeindex || mergedcount )
                StringAppend ( markertext, StringIsEmpty ( markertext ) ? "" : "_", IntegerToString ( buff, markercode ) );

            StringShrink ( markertext, markertext, MarkerNameMaxLength - 1 );

                                        // OK to add
            mergedmarkers.Append ( new TMarker ( position, position, markercode, markertext, MarkerTypeMarker ) );

                                        // step over any merged markers
            markeri = markeri2 - 1;


            break;                      // the second loop
            } // for markeri2

        } // for markeri

    EEGDoc->InsertMarkers   ( mergedmarkers );
    } // mergemarkers

else {

    EEGDoc->InsertMarkers   ( newmarkers    );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time to save to file!
EEGDoc->CommitMarkers ();


Gauge.Finished ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show the markers
TTracksView*        eegview         = dynamic_cast<TTracksView*> ( EEGDoc->GetViewList () );


//if ( eegview ) {
    //ShowTags            = EEGDoc->GetNumMarkers ( DisplayMarkerType );
    //ButtonGadgetSetState    ( IDB_SHOWMARKERS, ShowTags );
    //eegview->SetMarkerType ( MarkerTypeMarker );
    //}

EEGDoc->NotifyViews     ( vnReloadData, EV_VN_RELOADDATA_TRG );
                                        // clear highlighted tracks
//if ( eegview )
//    eegview->Highlighted.Reset ();
//EEGDoc->NotifyDocViews  ( vnNewHighlighted, (TParam2) &Highlighted );

if ( eegview )
    eegview->Invalidate ( false );

EEGDoc->AllowClosing ();


CartoolApplication->SetMainTitle ( ScanningTriggersTitle, EEGDoc->GetDocPath () );

UpdateApplication;
}


//----------------------------------------------------------------------------
                                        // Overriding function, for special case of frequency file
void    TScanTriggersDialog::BatchProcessCurrent ()
{
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );

ProcessCurrent ( &ScanTriggersTransfer );

SetProcessPriority ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
