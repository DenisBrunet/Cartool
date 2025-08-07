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

#include    "TExportTracksDialog.h"
#include    "ReprocessTracks.h"

#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.SpreadSheet.h"
#include    "Files.Conversions.h"       // MergeTracksToFreqFiles
#include    "Files.ReadFromHeader.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TRisDoc.h"
#include    "TRoisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TExportTracksStructEx       ExportTracksTransfer;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TExportTracksStruct::TExportTracksStruct ()
{
ExportTracks        = BoolToCheck ( true  );
StringCopy ( Tracks, "*" );
UseXyzDoc           = BoolToCheck ( false );
ClearString ( XyzDocFile );

ExportRois          = BoolToCheck ( false );
ClearString ( RoisDocFile );

TimeInterval        = BoolToCheck ( true  );
TriggersToKeep      = BoolToCheck ( false );
TriggersToExclude   = BoolToCheck ( false );
StringCopy  ( TimeMin, "0" );
ClearString ( TimeMax );
EndOfFile           = BoolToCheck ( true  );
StringCopy  ( TriggersToKeepList,    "*" );
ClearString ( TriggersToExcludeList );

ClearString ( AddChannels );

NoFilters           = BoolToCheck ( true  );
CurrentFilters      = BoolToCheck ( false );
OtherFilters        = BoolToCheck ( false );

BaselineCorr        = BoolToCheck ( false );
StringCopy ( BaselineCorrPre,    "0" );
StringCopy ( BaselineCorrPost,   "0" );
BaselineCorrEof     = BoolToCheck ( false );

NoRef               = BoolToCheck ( true  );
CurrentRef          = BoolToCheck ( false );
AveRef              = BoolToCheck ( false );
OtherRef            = BoolToCheck ( false );
ClearString ( RefList );

NoRescaling         = BoolToCheck ( true  );
RescaledBy          = BoolToCheck ( false );
StringCopy ( RescaledValue, "1.0" );
GfpNormalization    = BoolToCheck ( false );

Sequence            = BoolToCheck ( true  );
Mean                = BoolToCheck ( false );

Downsample          = BoolToCheck ( false );
StringCopy ( DownsampleRatio, "2" );

FileTypes.Clear ();
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    FileTypes.AddString ( SavingEegFileTypePreset[ i ], i == PresetFileTypeDefaultEEG );

StringCopy ( InfixFilename, "Export" );

Markers             = BoolToCheck ( true  );
OpenAuto            = BoolToCheck ( true  );
Concatenate         = BoolToCheck ( false );
}


        TExportTracksStructEx::TExportTracksStructEx ()
{
DefaultSamplingFrequency= 0;
ChannelsSel             = 0;
XyzLinked               = false;
RoiLinked               = false;
}


//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TExportTracksDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_OTHERFILTERS,         CmOtherFilters ),

    EV_COMMAND                  ( IDC_BROWSEXYZDOC,         CmBrowseXyzDoc ),
    EV_COMMAND                  ( IDC_USEXYZDOC,            CmXyzChange ),
    EV_EN_CHANGE                ( IDC_XYZDOCFILE,           CmXyzChange ),
//  EV_COMMAND_ENABLE           ( IDC_TRACKS,               CmExportTracksEnable ),
    EV_COMMAND_ENABLE           ( IDC_CHANNELS,             CmTracksEnable ),
    EV_COMMAND_ENABLE           ( IDC_USEXYZDOC,            CmXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZDOCFILE,           CmXyzDocFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZDOC,         CmUseXyzDocEnable ),

    EV_COMMAND                  ( IDC_BROWSEROISFILE,       CmBrowseRoiDoc ),
    EV_EN_CHANGE                ( IDC_ROISFILE,             CmRoiChange ),
//  EV_COMMAND_ENABLE           ( IDC_ROIS,                 CmExportRoisEnable ),
    EV_COMMAND_ENABLE           ( IDC_ROISFILE,             CmRoisEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEROISFILE,       CmRoisEnable ),

    EV_COMMAND_ENABLE           ( IDC_OUTPUT1FILE,          CmProcessBatchEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMIN,              CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMAX,              CmTimeMaxEnable ),

    EV_COMMAND_ENABLE           ( IDC_TOENDOFFILE,          CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_TRIGGERSTOKEEPLIST,   CmTriggersEnable ),
    EV_COMMAND_ENABLE           ( IDC_TRIGGERSTOEXCLUDELIST,CmTriggersExclEnable ),

    EV_COMMAND_ENABLE           ( IDC_ADDCHANNELS,          CmAddChannelsEnable ),

    EV_COMMAND_ENABLE           ( IDC_REFLIST,              CmOtherRefEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESCALE_VALUE,        CmRescaledByEnable ),
//  EV_COMMAND_ENABLE           ( IDC_EXPMARKERS,           CmTimeSequenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_BASELINECORR,         CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_PRE,             CmBaseLineCorrectionEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_POST,            CmBaseLineCorrectionEofEnable ),
    EV_COMMAND_ENABLE           ( IDC_BASELINECORREOF,      CmBaseLineCorrectionEnable ),
    EV_COMMAND_ENABLE           ( IDC_DOWNSAMPLE,           CmTimeSequenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_DOWNSAMPLERATIO,      CmDownsampleRatioEnable ),

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,       CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,         CmProcessBatchEnable ),

END_RESPONSE_TABLE;


        TExportTracksDialog::TExportTracksDialog ( TWindow* parent, TResId resId, TTracksDoc* doc  )
      : TBaseDialog ( parent, resId, doc )
{
                                        // can also export .data and .seg
StringCopy ( BatchFilesExt, AllTracksFilesExt " " AllSegDataExt );


ExportTracks            = new TRadioButton ( this, IDC_TRACKS );
Tracks                  = new TEdit ( this, IDC_CHANNELS, EditSizeTextLong );
UseXyzDoc               = new TCheckBox ( this, IDC_USEXYZDOC );
XyzDocFile              = new TEdit ( this, IDC_XYZDOCFILE, EditSizeText );

ExportRois              = new TRadioButton ( this, IDC_ROIS );
RoisDocFile             = new TEdit ( this, IDC_ROISFILE, EditSizeText );

TimeInterval            = new TRadioButton ( this, IDC_TIMEINTERVAL );
TriggersToKeep          = new TRadioButton ( this, IDC_TRIGGERSTOKEEP );
TriggersToExclude       = new TRadioButton ( this, IDC_TRIGGERSTOEXCLUDE );
TimeMin                 = new TEdit ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax                 = new TEdit ( this, IDC_TIMEMAX, EditSizeValue );
EndOfFile               = new TCheckBox ( this, IDC_TOENDOFFILE );
TriggersToKeepList      = new TEdit ( this, IDC_TRIGGERSTOKEEPLIST, EditSizeText );
TriggersToExcludeList   = new TEdit ( this, IDC_TRIGGERSTOEXCLUDELIST, EditSizeText );

AddChannels             = new TEdit ( this, IDC_ADDCHANNELS, EditSizeText );

NoFilters               = new TRadioButton ( this, IDC_NOFILTERS );
CurrentFilters          = new TRadioButton ( this, IDC_CURRENTFILTERS );
OtherFilters            = new TRadioButton ( this, IDC_OTHERFILTERS );

NoRef                   = new TRadioButton ( this, IDC_NOREF );
CurrentRef              = new TRadioButton ( this, IDC_CURRENTREF );
AveRef                  = new TRadioButton ( this, IDC_AVEREF );
OtherRef                = new TRadioButton ( this, IDC_OTHERREF );
RefList                 = new TEdit ( this, IDC_REFLIST, EditSizeText );

BaselineCorr            = new TCheckBox ( this, IDC_BASELINECORR );
BaselineCorrPre         = new TEdit     ( this, IDC_BLCE_PRE, EditSizeValue );
BaselineCorrPost        = new TEdit     ( this, IDC_BLCE_POST, EditSizeValue );
BaselineCorrEof         = new TCheckBox ( this, IDC_BASELINECORREOF );

NoRescaling             = new TRadioButton ( this, IDC_RESCALE_NONE );
RescaledBy              = new TRadioButton ( this, IDC_RESCALE_CSTE );
RescaledValue           = new TEdit ( this, IDC_RESCALE_VALUE, EditSizeValue );
GfpNormalization        = new TRadioButton ( this, IDC_RESCALE_GFP );

Sequence                = new TRadioButton ( this, IDC_SEQUENCE );
Mean                    = new TRadioButton ( this, IDC_MEAN );

Downsample              = new TCheckBox ( this, IDC_DOWNSAMPLE );
DownsampleRatio         = new TEdit     ( this, IDC_DOWNSAMPLERATIO, EditSizeValue );
DownsampleRatio->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );

FileTypes               = new TComboBox ( this, IDC_FILETYPES );
InfixFilename           = new TEdit ( this, IDC_INFIXFILENAME, EditSizeText );

Markers                 = new TCheckBox ( this, IDC_EXPMARKERS );
OpenAuto                = new TCheckBox ( this, IDC_OPENAUTO );
Concatenate             = new TCheckBox ( this, IDC_OUTPUT1FILE );

                                        // are we entering batch?
BatchProcessing         = doc == 0;


SetTransferBuffer ( dynamic_cast <TExportTracksStruct*> ( &ExportTracksTransfer ) );

                                        // transfer buffer could be either just initialized, already used, or modified in TTracksView from a given TTracksDoc
                                        // so we better do some safety checks, and set some default values if empty:
if ( StringIsEmpty ( ExportTracksTransfer.TimeMin ) ) {
    StringCopy  ( ExportTracksTransfer.TimeMin, "0" );
//  ClearString ( ExportTracksTransfer.TimeMax );
    ExportTracksTransfer.EndOfFile      = BoolToCheck ( true  );
    }


if ( StringIsEmpty ( ExportTracksTransfer.TriggersToKeepList ) )
    StringCopy  ( ExportTracksTransfer.TriggersToKeepList, "*" );


if ( StringIsEmpty ( ExportTracksTransfer.RescaledValue ) )
    StringCopy  ( ExportTracksTransfer.RescaledValue, "1.0" );


if ( StringIsEmpty ( ExportTracksTransfer.DownsampleRatio ) )
    StringCopy  ( ExportTracksTransfer.DownsampleRatio, "2" );

                                        // force RIS output selection if input is also RIS
if  ( EEGDoc ) {
    if      ( dynamic_cast<TRisDoc*> ( EEGDoc ) )

        ExportTracksTransfer.FileTypes.Select ( PresetFileTypeDefaultRIS );
                                        // force to default EEG format if input is not RIS and file output is RIS(?)
    else if ( ExportTracksTransfer.FileTypes.GetSelIndex () == PresetFileTypeDefaultRIS )

        ExportTracksTransfer.FileTypes.Select ( PresetFileTypeDefaultEEG );
    }


if ( StringIsEmpty ( ExportTracksTransfer.InfixFilename ) )
    StringCopy  ( ExportTracksTransfer.InfixFilename, "Export" );

                                        // some override, done on each call
if ( BatchProcessing ) {

    ExportTracksTransfer.OpenAuto           = BoolToCheck ( false );
    }

else {
                                        // single current doc
    ExportTracksTransfer.Concatenate        = BoolToCheck ( false );

//  ExportTracksTransfer.OpenAuto           = true;

                                        // this seems more annoying than helpful..
//  ExportTracksTransfer.FileTypes.Select ( IsExtension ( EEGDoc->GetTitle (), FILEEXT_EEGEP  )  ?  PresetFileTypeEp
//                                        : IsExtension ( EEGDoc->GetTitle (), FILEEXT_EEGEPH )  ?  PresetFileTypeEph
//                                        : IsExtension ( EEGDoc->GetTitle (), FILEEXT_EEGSEF )  ?  PresetFileTypeSef
//                                        : IsExtension ( EEGDoc->GetTitle (), FILEEXT_EEGBV  )  ?  PresetFileTypeBV
//                                        : IsExtension ( EEGDoc->GetTitle (), FILEEXT_EEGEDF )  ?  PresetFileTypeEdf
//                                        : IsExtension ( EEGDoc->GetTitle (), FILEEXT_RIS    )  ?  PresetFileTypeRis
//                                        : EEGDoc->GetNumTimeFrames() < EegMaxPointsDisplay / 2 ?  PresetFileTypeEph 
//                                        :                                                         PresetFileTypeDefaultEEG 
//                                        );
    }

                                        // RESET THIS FALLBACK FIELD BEFORE EACH CALL, for both in single file and batch mode processing
ExportTracksTransfer.DefaultSamplingFrequency = 0;


ConcatInputTime     = 0;
ConcatOutputTime    = 0;
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::SetupWindow ()
{
TDialog::SetupWindow ();


if ( ! BatchProcessing ) {              // in case files were provided before call (ProcessCurrent)

    if ( CheckToBool ( ExportTracks->GetCheck() ) )
        CmXyzChange  ();

    if ( CheckToBool ( ExportRois->GetCheck() ) )
        CmRoiChange  ();
    }
}


//----------------------------------------------------------------------------
        TExportTracksDialog::~TExportTracksDialog ()
{
delete  ExportTracks;
delete  Tracks;                 delete  UseXyzDoc;              delete  XyzDocFile;
delete  ExportRois;             delete  RoisDocFile;
delete  TimeInterval;           delete  TriggersToKeep;         delete  TriggersToExclude;
delete  TimeMin;                delete  TimeMax;                delete  EndOfFile;
delete  TriggersToKeepList;     delete  TriggersToExcludeList;
delete  AddChannels;
delete  NoFilters;              delete  CurrentFilters;         delete  OtherFilters;
delete  BaselineCorr;
delete  BaselineCorrPre;        delete  BaselineCorrPost;       delete  BaselineCorrEof;
delete  NoRef;                  delete  CurrentRef;             delete  AveRef;
delete  OtherRef;               delete  RefList;
delete  NoRescaling;            delete  RescaledBy;             delete  RescaledValue;          delete  GfpNormalization;
delete  Sequence;               delete  Mean;
delete  Downsample;             delete  DownsampleRatio;
delete  FileTypes;
delete  InfixFilename;
delete  Markers;                delete  OpenAuto;               delete  Concatenate;
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt     );
TGoF                lmfiles         ( drop, FILEEXT_LM              );
TGoF                tracksfiles     ( drop, BatchFilesExt           );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                roifiles        ( drop, AllRoisFilesExt         );

char                buff[ KiloByte ];
StringCopy ( buff, BatchFilesExt, " " SpreadSheetFilesExt " " FILEEXT_LM " " AllCoordinatesFilesExt " " AllRoisFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) xyzfiles ) {

    if ( ExportTracksTransfer.XyzLinked )
        ShowMessage (   "You can not change the coordinates file now,"      NewLine 
                        "as it is already provided by the current link."    NewLine 
                        NewLine 
                        "Close the link file first if you wish to use another coordinates file.", 
                        ExportTracksTitle, ShowMessageWarning );

    else {
        if ( ! CheckToBool ( ExportTracksTransfer.ExportTracks ) ) {
            ExportTracksTransfer.ExportTracks   = BoolToCheck ( true  );
            ExportTracksTransfer.ExportRois     = BoolToCheck ( false );
            }

        for ( int i = 0; i < (int) xyzfiles; i++ ) {

            ExportTracksTransfer.UseXyzDoc      = BoolToCheck ( true  );
            StringCopy ( ExportTracksTransfer.XyzDocFile, xyzfiles[ i ] );
            TransferData ( tdSetData ); // this will activate any change detection function
//            Transfer ( (void *) &ExportTracksTransfer, tdSetData );
            }
        }
    } // if numxyz


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) roifiles ) {
    if ( ExportTracksTransfer.RoiLinked )
        ShowMessage (   "You can not change the ROIs file now,"             NewLine 
                        "as it is already provided by the current link."    NewLine 
                        NewLine 
                        "Close the link file first if you wish to use another ROIs file.", 
                        ExportTracksTitle, ShowMessageWarning );

    else {
        if ( ! CheckToBool ( ExportTracksTransfer.ExportRois ) ) {
            ExportTracksTransfer.ExportTracks   = BoolToCheck ( false );
            ExportTracksTransfer.ExportRois     = BoolToCheck ( true  );
            }

        for ( int i = 0; i < (int) roifiles; i++ ) {

            StringCopy ( ExportTracksTransfer.RoisDocFile, roifiles[ i ] );
            TransferData ( tdSetData ); // this will activate any change detection function
//            Transfer ( (void *) &ExportTracksTransfer, tdSetData );
            }
        }
    } // if numroi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ( (bool) tracksfiles || (bool) spreadsheetfiles || (bool) lmfiles ) && ! BatchProcessing )

    ShowMessage ( BatchNotAvailMessage, ExportTracksTitle, ShowMessageWarning );


if ( (bool) tracksfiles && BatchProcessing )
    BatchProcessDropped ( tracksfiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) spreadsheetfiles && BatchProcessing )
    for ( int i = 0; i < (int) spreadsheetfiles; i++ )
        BatchProcessSplitFile ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) lmfiles && BatchProcessing )
    for ( int i = 0; i < (int) lmfiles; i++ )
        BatchProcessLmFile ( lmfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
/*
void    TExportTracksDialog::CmExportTracksEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! ExportTracksTransfer.RoiLinked );
}
*/

void    TExportTracksDialog::CmTracksEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ExportTracks->GetCheck() ) );
}


void    TExportTracksDialog::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ExportTracks->GetCheck() )
         && ! ExportTracksTransfer.XyzLinked );
}


void    TExportTracksDialog::CmXyzDocFileEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ExportTracks->GetCheck() )
          && CheckToBool ( UseXyzDoc   ->GetCheck() ) );
}


void    TExportTracksDialog::CmUseXyzDocEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ExportTracks->GetCheck() )
          && CheckToBool ( UseXyzDoc   ->GetCheck() )
          && ! ExportTracksTransfer.XyzLinked );
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::CmBrowseXyzDoc ()
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( ExportTracksTransfer.XyzDocFile ) )
    return;


TransferData ( tdSetData );             // this will activate CmXyzChange (but not SetText ( .. ) )
//Transfer ( (void *) &ExportTracksTransfer, tdSetData );

XyzDocFile->ResetCaret;
}

                                        // !Not directly setting ExportTracksTransfer buffer - Reading is OK, though!
void    TExportTracksDialog::CmXyzChange ()
{
                                        // extra-care to avoid re-entrant code
static bool         busy            = false;

if ( busy )
    return;

busy    = true;

                                        // this is subtly better than disabling, showing it's active, but not editable
XyzDocFile->SetReadOnly ( ExportTracksTransfer.XyzLinked );


if (   BatchProcessing 
  || ! CheckToBool ( ExportTracks->GetCheck() ) 
  ||   StringIsEmpty ( ExportTracksTransfer.XyzDocFile ) 
  || ! ExportTracksTransfer.ChannelsSel ) {

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check XYZ document is correct
                                        // then update tracks text from selection
char                buff[ EditSizeTextLong ];

                                        // set default text
ExportTracksTransfer.ChannelsSel->ToText ( buff, EEGDoc->GetElectrodesNames(), AuxiliaryTracksNames );
Tracks->SetText ( buff );


TFileName           xyzfile;
                                        // no XYZ business?
XyzDocFile->GetText ( xyzfile, EditSizeText );

if ( StringIsEmpty ( xyzfile ) || ! CheckToBool ( UseXyzDoc->GetCheck () ) ) {

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test file silently
if ( ! CanOpenFile ( xyzfile ) ) {

    XyzDocFile->SetText     ( "" );
    UseXyzDoc ->SetCheck    ( BoolToCheck ( false ) );

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ok, now try to open as document
TOpenDoc<TElectrodesDoc>    XYZDoc ( xyzfile, OpenDocHidden );

                                        // either incorrect file, or not a .xyz one
if ( ! XYZDoc.IsOpen () ) {

    XyzDocFile->SetText     ( "" );
    UseXyzDoc ->SetCheck    ( BoolToCheck ( false ) );

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now check for compatibily
if ( XYZDoc->GetNumElectrodes () != EEGDoc->GetNumElectrodes() ) {

    StringCopy  ( buff, "File and Coordinates don't have the same dimension: "  NewLine
                        NewLine 
                        "File:"         Tab Tab, IntegerToString ( EEGDoc->GetNumElectrodes() ),    NewLine 
                        "Coordinates:"      Tab, IntegerToString ( XYZDoc->GetNumElectrodes () ) );

    ShowMessage ( buff, ExportTracksTitle, ShowMessageWarning );

    XYZDoc.Close ();

    XyzDocFile->SetText     ( "" );
    UseXyzDoc ->SetCheck    ( BoolToCheck ( false ) );

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // XYZ is good here - use it to translate current selection
ExportTracksTransfer.ChannelsSel->ToText ( buff, XYZDoc->GetElectrodesNames(), AuxiliaryTracksNames );
Tracks->SetText ( buff );

XYZDoc.Close ();

busy    = false;
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::CmBrowseRoiDoc ()
{
static GetFileFromUser  getfile ( "ROIs File", AllRoisFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( ExportTracksTransfer.RoisDocFile ) )
    return;


TransferData ( tdSetData );             // this will activate CmRoiChange (but not SetText ( .. ) )
//Transfer ( (void *) &ExportTracksTransfer, tdSetData );

RoisDocFile->ResetCaret;
}

/*
void    TExportTracksDialog::CmExportRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! ExportTracksTransfer.RoiLinked );
}
*/

void    TExportTracksDialog::CmRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ExportRois->GetCheck() ) );
}

                                        // !Not directly setting ExportTracksTransfer buffer - Reading is OK, though!
void    TExportTracksDialog::CmRoiChange ()
{
                                        // extra-care to avoid re-entrant code
static bool         busy            = false;

if ( busy )
    return;

busy    = true;

                                        // this is subtly better than disabling, showing it's active, but not editable
RoisDocFile->SetReadOnly ( ExportTracksTransfer.RoiLinked );


//FileTypes->SetSelIndex ( PresetFileTypeDefaultEEG );


if (   BatchProcessing 
  || ! CheckToBool   ( ExportRois->GetCheck () )
  ||   StringIsEmpty ( ExportTracksTransfer.RoisDocFile ) ) {

    SetBaseFilename ();

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check ROIs document is correct
                                        // then update tracks text from selection
TFileName           roifile;

                                        // no ROIs business?
RoisDocFile->GetText ( roifile, EditSizeText );

if ( StringIsEmpty ( roifile ) ) {

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test file silently
if ( ! CanOpenFile ( roifile ) ) {

    RoisDocFile->SetText ( "" );

    SetBaseFilename ();

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ok, now try to open as document
TOpenDoc<TRoisDoc>  RoisDoc ( roifile, OpenDocHidden );

                                        // either incorrect file, or not a .rois one
if ( ! RoisDoc.IsOpen () ) {

    RoisDocFile->SetText ( "" );

    SetBaseFilename ();

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for compatibily
if ( RoisDoc->ROIs->GetDimension () != EEGDoc->GetNumElectrodes() ) {
    
    char        buff[ 256 ];

    StringCopy  ( buff, "File and ROIs don't have the same dimension: " NewLine 
                        NewLine 
                        "File:" Tab, IntegerToString ( EEGDoc->GetNumElectrodes() ),    NewLine 
                        "ROIs:" Tab, IntegerToString ( RoisDoc->ROIs->GetDimension () ) );

    ShowMessage ( buff, ExportTracksTitle, ShowMessageWarning );

    RoisDoc.Close ();

    RoisDocFile->SetText ( "" );

    SetBaseFilename ();

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ROIs are good here!
RoisDoc.Close ();

SetBaseFilename ();

busy    = false;
}


//----------------------------------------------------------------------------
                                        // generate a smart base name
void    TExportTracksDialog::SetBaseFilename ()
{
                                        // don't overwrite existing name
if ( StringIsNotEmpty ( ExportTracksTransfer.InfixFilename ) )
    return;

                                        // extra-care to avoid re-entrant code
static bool         busy            = false;

if ( busy )
    return;

busy    = true;


if ( ! ( CheckToBool ( ExportTracksTransfer.ExportRois ) && StringIsNotEmpty ( ExportTracksTransfer.RoisDocFile ) ) ) {

    if ( StringIsSpace ( ExportTracksTransfer.InfixFilename ) ) {

        StringCopy ( ExportTracksTransfer.InfixFilename, "Export" );
        TransferData ( tdSetData );     // !this call will refresh the dialog, hence calling CmXyzChange and CmRoiChange!
        }

    busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           roiname;
TFileName           match;

RoisDocFile->GetText ( roiname, EditSizeText );

GetFilename     ( roiname );

StringShrink    ( roiname, match, min ( (int) ( 4 + StringLength ( roiname ) / 2 ), 17 ) );


/*
TFileName           basename;

StringCopy ( basename, "Export" );

int                 intimemin       = GetInteger  ( TimeMin );
int                 intimemax       = GetInteger  ( TimeMax );

if ( CheckToBool ( TimeInterval->GetCheck() ) ) {
    if ( ! CheckToBool ( EndOfFile->GetCheck() ) )
        StringAppend ( basename, ".", IntegerToString ( intimemin ), " to ", IntegerToString ( intimemax ) );
    }
else
    StringAppend ( basename, ".Triggers" );
*/

StringCopy ( ExportTracksTransfer.InfixFilename, match );
TransferData ( tdSetData );     // !this call will refresh the dialog, hence calling CmXyzChange and CmRoiChange!

busy    = false;
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::CmIntervalEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TimeInterval->GetCheck() ) );
}


void    TExportTracksDialog::CmTimeMaxEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TimeInterval->GetCheck() ) && ! CheckToBool ( EndOfFile->GetCheck() ) );
}


void    TExportTracksDialog::CmTriggersEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TriggersToKeep->GetCheck() ) );
}


void    TExportTracksDialog::CmTriggersExclEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TriggersToExclude->GetCheck() ) );
}


void    TExportTracksDialog::CmAddChannelsEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( ExportRois->GetCheck() ) );
}


void    TExportTracksDialog::CmOtherRefEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( OtherRef->GetCheck() ) );
}


void    TExportTracksDialog::CmRescaledByEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( RescaledBy->GetCheck() ) );
}


void    TExportTracksDialog::CmBaseLineCorrectionEnable ( TCommandEnabler &tce )
{
tce.Enable (   CheckToBool ( BaselineCorr->GetCheck() )
        && ! ( CheckToBool ( TriggersToKeep->GetCheck() ) || CheckToBool ( TriggersToExclude->GetCheck() ) ) );
}


void    TExportTracksDialog::CmBaseLineCorrectionEofEnable ( TCommandEnabler &tce )
{
tce.Enable (   CheckToBool ( BaselineCorr   ->GetCheck() )
          && ! ( CheckToBool ( TriggersToKeep->GetCheck() ) || CheckToBool ( TriggersToExclude->GetCheck() ) )
          && ! CheckToBool ( BaselineCorrEof->GetCheck() ) );
}


void    TExportTracksDialog::CmTimeSequenceEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TimeInterval->GetCheck() )
          && CheckToBool ( Sequence    ->GetCheck() ) );
}


void    TExportTracksDialog::CmDownsampleRatioEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TimeInterval->GetCheck() )
          && CheckToBool ( Sequence    ->GetCheck() )
          && CheckToBool ( Downsample  ->GetCheck() ) );
}


//----------------------------------------------------------------------------
bool    TExportTracksDialog::CmProcessEnable ()
{
if ( CheckToBool ( ExportTracks->GetCheck() )
  && ! Tracks->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( ExportTracks->GetCheck() )
  && CheckToBool ( UseXyzDoc->GetCheck() )
  && ! XyzDocFile->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( ExportRois->GetCheck() )
  && ! RoisDocFile->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( TriggersToKeep->GetCheck() )
  && ! TriggersToKeepList->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( TriggersToExclude->GetCheck() )
  && ! TriggersToExcludeList->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( OtherRef->GetCheck() )
  && ! RefList->GetLineLength ( 0 ) )
    return  false;


if ( CheckToBool ( BaselineCorr->GetCheck() )
  && ! (      BaselineCorrPre->GetLineLength ( 0 )
         && ( CheckToBool ( BaselineCorrEof->GetCheck() ) || BaselineCorrPost->GetLineLength ( 0 ) /*&& StringToDouble ( TAvgTransfer.BaselineCorrPre ) <= StringToDouble ( TAvgTransfer.BaselineCorrPost )*/ )
       )
   )
    return  false;


if ( CheckToBool ( RescaledBy->GetCheck() )
  && ! RescaledValue->GetLineLength ( 0 ) )


if ( CheckToBool ( TimeInterval->GetCheck() ) && CheckToBool ( Sequence->GetCheck() ) && CheckToBool ( Downsample->GetCheck() ) ) {
    char        buff[EditSizeValue];
    DownsampleRatio->GetText ( buff, EditSizeText );

    if ( StringToInteger ( buff ) <= 0 )
        return  false;
    }


return  true;
}


void    TExportTracksDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


void    TExportTracksDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( ! BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::CmOtherFilters ()
{
//WindowMinimize ();

                                        // run dialog to ask the user
if ( TTracksFiltersDialog   (   CartoolMainWindow, IDD_TRACKSFILTERS, 
                                ((TExportTracksStructEx *) GetTransferBuffer () )->Filters.FiltersParam,    BatchProcessing ? 0 : EEGDoc->GetSamplingFrequency () 
                            ).Execute() == IDCANCEL ) {

    OtherFilters  ->SetCheck ( BoolToCheck ( false ) );
    CurrentFilters->SetCheck ( BoolToCheck ( true  ) );
//  WindowRestore ();
    return;
    }

                                        // chew the results in a proper way
((TExportTracksStructEx *) GetTransferBuffer () )->Filters.SetFromStruct ( BatchProcessing ? 0 : EEGDoc->GetSamplingFrequency (), BatchProcessing ? Silent : Interactive );

//WindowRestore ();
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::ProcessCurrent ( void* usetransfer, const char* freqinfix )
{
if ( EEGDoc == 0 )
    return;


TExportTracksStructEx*  transfer    = usetransfer ? (TExportTracksStructEx *) usetransfer : &ExportTracksTransfer;

if ( ! transfer )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                isfrequency         =    IsExtensionAmong ( EEGDoc->GetDocPath (), AllFreqFilesExt ) 
                                          || StringIsNotEmpty ( freqinfix );    // this is the current frequency string, when processing frequency files

int                 numregel            = EEGDoc->GetNumElectrodes   ();
int                 numtotalel          = EEGDoc->GetTotalElectrodes ();
long                numtimeframes       = EEGDoc->GetNumTimeFrames   ();
long                lasttimeframe       = numtimeframes - 1;
bool                haspseudos          = EEGDoc->HasPseudoElectrodes ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tracks parameters
TracksOptions          tracksoptions    = CheckToBool ( transfer->ExportTracks ) ? ProcessTracks
                                        : CheckToBool ( transfer->ExportRois   ) ? ProcessRois
                                        :                                          ProcessUnknown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can be done only once if in BatchProcess..
TOpenDoc<TElectrodesDoc>    XYZDoc;

bool                letopenxyz      = transfer->XyzLinked || BatchProcessing;
auto                CloseXyz        = [ &XYZDoc,  letopenxyz  ] ()    { XYZDoc .Close ( letopenxyz  ? CloseDocLetOpen : CloseDocRestoreAsBefore ); };

                                        // see if a coordinates file has been provided
if ( CheckToBool ( transfer->UseXyzDoc  )
  && CanOpenFile ( transfer->XyzDocFile ) ) {

    XYZDoc.Open ( transfer->XyzDocFile, OpenDocHidden );

                                        // test for XYZ compatibility
    if ( ! ( XYZDoc.IsOpen () && XYZDoc->GetNumElectrodes () == EEGDoc->GetNumElectrodes () ) ) {

        CloseXyz ();

        if ( BatchProcessing )
            return;                     // that means, skip the current file silently
        else
            CmCancel ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can be done only once if in BatchProcess..
TOpenDoc<TRoisDoc>  RoisDoc;

bool                letopenrois     = transfer->RoiLinked || BatchProcessing;
auto                CloseRois       = [ &RoisDoc, letopenrois ] ()    { RoisDoc.Close ( letopenrois ? CloseDocLetOpen : CloseDocRestoreAsBefore ); };

                                        // see if a Roi file has been provided
if ( tracksoptions == ProcessRois ) {

    if ( CanOpenFile ( transfer->RoisDocFile ) ) {

        RoisDoc.Open ( transfer->RoisDocFile, OpenDocHidden );

                                        // test for ROIs compatibility
        if ( ! ( RoisDoc.IsOpen () && RoisDoc->ROIs->GetDimension () == EEGDoc->GetNumElectrodes() ) ) {

            CloseXyz  ();
            CloseRois ();

            if ( BatchProcessing )
                return;                 // that means, skip the current file silently
            else
                CmCancel ();
            }
        }
    else {                              // there is a problem with the roi file, get out
        CloseXyz;
        CmCancel ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters
TimeOptions         timeoptions     = CheckToBool ( transfer->TimeInterval      ) ? ExportTimeInterval
                                    : CheckToBool ( transfer->TriggersToKeep    ) ? ExportTimeTriggers
                                    : CheckToBool ( transfer->TriggersToExclude ) ? ExcludeTimeTriggers
                                    :                                               UnknownTimeProcessing;


bool                endoffile       = CheckToBool ( transfer->EndOfFile );
long                timemin         = timeoptions == ExportTimeInterval ? StringToInteger ( transfer->TimeMin )
                                                                        : 0;
long                timemax         = timeoptions == ExportTimeInterval ? ( endoffile ? lasttimeframe /*Highest ( timemax )*/
                                                                                      : StringToInteger ( transfer->TimeMax ) )
                                                                        : 0;

Clipped ( timemin, timemax, (long) 0, lasttimeframe );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sequence parameters                                             HasAnyFilter ()  makes implicit use of  AreFiltersActivated ()
FiltersOptions      filtersoptions  = CheckToBool ( transfer->CurrentFilters ) && EEGDoc  ->GetFilters ()->HasAnyFilter () ? UsingCurrentFilters
                                    : CheckToBool ( transfer->OtherFilters   ) && transfer->Filters.       HasAnyFilter () ? UsingOtherFilters
                                    :                                                                                        NotUsingFilters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting data type, reference, and reading parameters
AtomType         	datatype        = EEGDoc->GetAtomType ( AtomTypeUseOriginal );

                                        // Pick the right reference
                                        // Note that there is no check on RefList anymore, allowing to account for any added channels - this will be handled by ReprocessTracks
ReferenceType       ref;

if      ( CheckToBool ( transfer->CurrentRef ) )    ref = ReferenceUsingCurrent;
else if ( CheckToBool ( transfer->AveRef     ) )    ref = ReferenceAverage;
else if ( CheckToBool ( transfer->OtherRef   ) )    ref = ReferenceArbitraryTracks;
else                                                ref = ReferenceAsInFile;

                                        // Allow any sort of re-referencing
//CheckReference ( ref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                baselinecorr        = CheckToBool ( transfer->BaselineCorr ) 
                                       && timeoptions == ExportTimeInterval;
bool                baselinecorreof     = CheckToBool ( transfer->BaselineCorrEof );

long                baselinecorrpre     = baselinecorr ? StringToInteger ( transfer->BaselineCorrPre  ) 
                                                       : 0;
long                baselinecorrpost    = baselinecorr ? baselinecorreof ? lasttimeframe 
                                                                         : StringToInteger ( transfer->BaselineCorrPost )
                                                       : 0;

                                        // check over current limits
if ( baselinecorr ) {
                                        // this is not beyond limit, this is totally wrong parameters
    if ( baselinecorrpre < 0             && baselinecorrpost < 0
      || baselinecorrpre > lasttimeframe && ( baselinecorreof || baselinecorrpost > lasttimeframe ) )

        baselinecorr    = false;

    else
        Clipped ( baselinecorrpre,  baselinecorrpost,   (long) 0, lasttimeframe );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RescalingOptions    rescalingoptions    = CheckToBool ( transfer->NoRescaling       ) ? NotRescaled
                                        : CheckToBool ( transfer->RescaledBy        ) ? ConstantRescaling
                                        : CheckToBool ( transfer->GfpNormalization  ) ? GfpRescaling        // was gfpnormalization
                                        :                                               NotRescaled;

double              rescalingfactor     = rescalingoptions == ConstantRescaling ? StringToDouble ( transfer->RescaledValue ) : 0;

if ( rescalingoptions == GfpRescaling && ! haspseudos )
    rescalingoptions    = NotRescaled;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sequence parameters
SequenceOptions        sequenceoptions  = CheckToBool ( transfer->Sequence ) ? SequenceProcessing
                                        : CheckToBool ( transfer->Mean     ) ? AverageProcessing
                                        :                                      UnknownSequenceProcessing;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                downsample          = CheckToBool ( transfer->Downsample ) 
                                       && timeoptions     == ExportTimeInterval 
                                       && sequenceoptions == SequenceProcessing;
int                 downsampleratio     = downsample ? StringToInteger ( transfer->DownsampleRatio ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output file type
SavingEegFileTypes  filetype            = isfrequency   ? PresetFileTypeSef                     // these are temp files actually, better use binary ones
                                                        : (SavingEegFileTypes) transfer->FileTypes.GetSelIndex ();

                                        // works for all cases of time processing: time interval, keeping or excluding triggers
bool                outputmarkers       = CheckToBool ( transfer->Markers );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Concatenate parameters
ConcatenateOptions  concatenateoptions  = CheckToBool ( transfer->Concatenate ) && NumBatchFiles () > 1 ? ConcatenateTime
                                                                                                        : NoConcatenateTime;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // silencing in these cases
ExecFlags           execflags           = ExecFlags ( (    isfrequency 
                                                        || concatenateoptions == ConcatenateTime 
                                                        || NumBatchFiles () > 1 ? Silent : Interactive ) 
                                                    | DefaultOverwrite );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ReprocessTracks (
                EEGDoc,
                tracksoptions,      transfer->Tracks,
                XYZDoc,
                RoisDoc,
                timeoptions,
                timemin,            timemax,
                transfer->TriggersToKeepList,   transfer->TriggersToExcludeList,
                transfer->AddChannels,
                filtersoptions,     filtersoptions == UsingOtherFilters ? &transfer->Filters : 0,   ExportTracksTransfer.DefaultSamplingFrequency,
                ref,                transfer->RefList,
                baselinecorr,
                baselinecorrpre,    baselinecorrpost,
                rescalingoptions,   rescalingfactor,
                sequenceoptions,
                downsampleratio,
                0,                  // outputdir not specified from dialog
                transfer->InfixFilename,
                freqinfix,
                filetype,
                outputmarkers,
                concatenateoptions, concatenateoptions == ConcatenateTime ? &ConcatInputTime : 0,   concatenateoptions == ConcatenateTime ? &ConcatOutputTime : 0,
                ExpFile,
                &BatchFileNames,
                GetBatchFileIndex (),
                execflags
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                openauto            = CheckToBool ( transfer->OpenAuto )
                                       && ! isfrequency
                                       && CartoolApplication->IsInteractive ();

                                        // complimentary opening the file for the user
if ( openauto )
    if ( concatenateoptions == NoConcatenateTime || IsBatchLastCall () )
        CartoolDocManager->OpenDoc ( ExpFile.Filename, dtOpenOptions );

                                        // just to be clean
if ( concatenateoptions == NoConcatenateTime || IsBatchLastCall () )
    ExpFile.Markers.ResetMarkers ();


CloseXyz  ();
CloseRois ();


//Gauge.FinishParts ();
//CartoolApplication->SetMainTitle ( ExportTracksTitle, ExpFile.Filename, Gauge );


CartoolApplication->SetMainTitle ( ExportTracksTitle, ExpFile.Filename );

UpdateApplication ();
}


//----------------------------------------------------------------------------
                                        // Overriding function, for special case of frequency file
void    TExportTracksDialog::BatchProcessCurrent ()
{
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


TFreqDoc*           FreqDoc         = dynamic_cast<TFreqDoc*> ( EEGDoc );

if ( FreqDoc == 0 )
                                        // regular tracks - move along
    ProcessCurrent ( &ExportTracksTransfer );

else {                                  // loop for each frequency, and export each of them
    int                 numf            = FreqDoc->GetNumFrequencies ();
    int                 oldf            = FreqDoc->GetCurrentFrequency ();

    TSuperGauge         Gauge ( "Frequency", numf, SuperGaugeLevelInter );


    TGoF                filestomerge;
    TFileName           mergedfreqfile;


    for ( int f = 0; f < numf; f++ ) {

        Gauge.Next ();


        FreqDoc->SetCurrentFrequency ( f );

        ProcessCurrent ( &ExportTracksTransfer, FreqDoc->GetFrequencyName ( f ) );

                                        // the output file is there - it would be more elegant if the function would return it explicitly, though
        filestomerge.Add ( ExpFile.Filename );
        }

                                        // merge back for the user...
    MergeTracksToFreqFiles ( filestomerge, FreqDoc->GetFreqType (), mergedfreqfile, ExecFlags ( Interactive | DefaultOverwrite ) );

                                        // getting rid of the individual, per-frequency, ris files
    filestomerge.DeleteFiles ();

                                        // getting rid of all these marker files
    filestomerge.StringsReplace ( "." FILEEXT_EEGSEF, "." FILEEXT_EEGSEF "." FILEEXT_MRK );

    filestomerge.DeleteFiles ();

                                        // getting rid of all these verbose files
    filestomerge.StringsReplace ( "." FILEEXT_EEGSEF "." FILEEXT_MRK, "." FILEEXT_EEGSEF "." FILEEXT_VRB );
                                        // still sparing one
    filestomerge.RemoveFirst ();

    filestomerge.DeleteFiles ();

                                        // opening was forbidden during the tracks loop
    if ( CheckToBool ( ExportTracksTransfer.OpenAuto ) )

        mergedfreqfile.Open ();

                                        // restore old current frequency
    FreqDoc->SetCurrentFrequency ( oldf );  
    } // FreqDoc


SetProcessPriority ();
}


//----------------------------------------------------------------------------
void    TExportTracksDialog::BatchProcess ()
{
                                        // does not process lm and csv files, contrary to Drag & Drop

int                 numinfiles      = NumBatchFiles ();
bool                concatenation   = CheckToBool ( ExportTracksTransfer.Concatenate ) && numinfiles > 1;



if ( numinfiles > MaxFilesToOpen && ! concatenation )
    ExportTracksTransfer.OpenAuto           = BoolToCheck ( false );


                                        // concatenate needs some checking first
if ( concatenation ) {

                                        // freq files not allowed
    for ( int i = 0; i < numinfiles ; i++ )

        if ( IsExtensionAmong ( BatchFileNames[ i ], AllFreqFilesExt ) ) {

            ShowMessage (   "You are trying to concatenate Frequency files," NewLine 
                            "which is not possible at the moment.", 
                            ExportTracksTitle, ShowMessageWarning );
            return;
            }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check if files are all compatibles
    int             numel;
    int             numel0;

    for ( int i = 0; i < numinfiles ; i++ ) {

        if ( i == 0 )
            ReadFromHeader ( BatchFileNames[ i ], ReadNumElectrodes, &numel0 );

        ReadFromHeader ( BatchFileNames[ i ], ReadNumElectrodes, &numel );


        if ( numel != numel0 ) {

            ShowMessage (   "You are trying to concatenate files"   NewLine 
                            "with different number of tracks."      NewLine
                            NewLine 
                            "Check your files and try your chance again!", 
                            ExportTracksTitle, ShowMessageWarning );
            return;
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OK, now get a common file name
    TFileName           match;

    BatchFileNames.GetCommonString ( match, true );

                                        // compose the base name
    PrefixFilename ( match, "All " );

                                        // extension will be completed by the first processing
    if ( IsAbsoluteFilename ( match ) ) {

        StringCopy ( ExpFile.Filename, match );

        if ( StringIsNotEmpty ( ExportTracksTransfer.InfixFilename ) )
            StringAppend ( ExpFile.Filename, ".", ExportTracksTransfer.InfixFilename );
        }
    else
       ClearString ( ExpFile.Filename );

//  DBGM ( ExpFile.Filename, "Concat to file" );

                                        // will be updated by each file
    ConcatInputTime     = 0;
    ConcatOutputTime    = 0;

    } // create common filename
else
   ClearString ( ExpFile.Filename );



TBaseDialog::BatchProcess ();


                                        // tell the user about the good news
if ( ! CheckToBool ( ExportTracksTransfer.OpenAuto ) ) {
    TSuperGauge         Gauge ( ExportTracksTitle );
    Gauge.HappyEnd ();
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
