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

#include    "TCreateRoisDialog.h"
#include    "GenerateRois.h"

#include    "Strings.Utils.h"
#include    "TRois.h"

#include    "Volumes.TTalairachOracle.h"

#include    "TTracksView.h"
#include    "TElectrodesView.h"
#include    "TSolutionPointsView.h"
#include    "TVolumeView.h"
#include    "TSecondaryView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TCreateRoisStruct           CreateRoisTransfer;
TCreateRoisDialog*          CreateRoisDlg       = 0;


extern  TTalairachOracle    Taloracle;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
    TCreateRoisStruct::TCreateRoisStruct ()
{
ClearString ( InputFile );

UseRoisMriFile  = BoolToCheck ( false );
ClearString ( RoisMriFile );
ClearString ( RoisLabelsFile );

Automatic       = BoolToCheck ( true  );
Interactive     = BoolToCheck ( false );

AutoTalairach   = BoolToCheck ( false );
AutoAAL         = BoolToCheck ( true  );
AutoOtherRois   = BoolToCheck ( false );
ClearString ( GenerateRoisFile );

ClearString ( RoiName );
ClearString ( Selection );

ClearString ( MessageInteractive );

StringCopy ( NumRois, "0" );
RoisSummary.Clear ();

ClearString ( BaseFileName );
OpenAuto        = BoolToCheck ( true  );
}


//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TCreateRoisDialog, TBaseDialog )

    EV_WM_DROPFILES,
    EV_WM_TIMER,

    EV_COMMAND                  ( IDC_PROCESSAUTOMATIC,         CmSetProcessing ),
    EV_COMMAND                  ( IDC_PROCESSINTERACTIVE,       CmSetProcessing ),

    EV_EN_CHANGE                ( IDC_SELECTION,                CmSelectionChange ),

    EV_COMMAND_ENABLE           ( IDC_TRACKSFILE,               CmChangeInputFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSETRACKSFILE,         CmChangeInputFileEnable ),

    EV_COMMAND                  ( IDC_USEMRIFILE,               CmUseRoisMriFile ),
    EV_COMMAND_ENABLE           ( IDC_MRIFILE,                  CmChangeRoisMriFileEnable ),
//  EV_COMMAND_ENABLE           ( IDC_BROWSEMRIFILE,            CmChangeRoisMriFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_ROISLABELSFILE,           CmRoisLabelsFileEnable ),

    EV_COMMAND                  ( IDC_TALAIRACHTOROIS,          CmAutomaticChanged ),
    EV_COMMAND_ENABLE           ( IDC_TALAIRACHTOROIS,          CmTalairachEnable ),
    EV_COMMAND                  ( IDC_AALTOROIS,                CmAutomaticChanged ),
    EV_COMMAND_ENABLE           ( IDC_AALTOROIS,                CmAutomaticEnable ),
    EV_COMMAND                  ( IDC_MRISPTOROIS,              CmAutomaticChanged ),
    EV_COMMAND_ENABLE           ( IDC_MRISPTOROIS,              CmAutomaticEnable ),
    EV_COMMAND_ENABLE           ( IDC_GENERATEROISFILE,         CmGenerateRoisFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEGENERATEROISFILE,   CmAutomaticEnable ),

    EV_COMMAND_ENABLE           ( IDC_SELECTION,                CmInteractiveEnable ),
    EV_COMMAND_ENABLE           ( IDC_ROINAME,                  CmInteractiveEnable ),
    EV_COMMAND_ENABLE           ( IDC_NUMROIS,                  CmInteractiveEnable ),
    EV_COMMAND_ENABLE           ( IDC_ROISSUMMARY,              CmInteractiveEnable ),

    EV_COMMAND                  ( IDC_ADDSELECTION,             CmAddSelection ),
    EV_COMMAND_ENABLE           ( IDC_ADDSELECTION,             CmAddSelectionEnable ),
    EV_COMMAND                  ( IDC_REMOVESELECTION,          CmRemoveSelection ),
    EV_COMMAND_ENABLE           ( IDC_REMOVESELECTION,          CmRemoveSelectionEnable ),

    EV_COMMAND                  ( IDOK,                         CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),
    EV_COMMAND                  ( IDCANCEL,                     CmCancel ),

    EV_COMMAND                  ( IDC_BROWSETRACKSFILE,         CmBrowseInputFile ),
    EV_COMMAND                  ( IDC_BROWSEMRIFILE,            CmBrowseRoisMriFile ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEROISLABELSFILE,     CmBrowseTextFile ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEGENERATEROISFILE,   CmBrowseTextFile ),
    EV_COMMAND                  ( IDC_UPDIRECTORY,              CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEROISFILE,           CmBrowseRoiFile ),

END_RESPONSE_TABLE;


        TCreateRoisDialog::TCreateRoisDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{
InputFile           = new TEdit         ( this, IDC_TRACKSFILE, EditSizeText );
UseRoisMriFile      = new TCheckBox     ( this, IDC_USEMRIFILE );
RoisMriFile         = new TEdit         ( this, IDC_MRIFILE, EditSizeText );
RoisLabelsFile      = new TEdit         ( this, IDC_ROISLABELSFILE, EditSizeText );

Automatic           = new TRadioButton  ( this, IDC_PROCESSAUTOMATIC );
Interactive         = new TRadioButton  ( this, IDC_PROCESSINTERACTIVE );

AutoTalairach       = new TRadioButton  ( this, IDC_TALAIRACHTOROIS );
AutoAAL             = new TRadioButton  ( this, IDC_AALTOROIS );
AutoOtherRois       = new TRadioButton  ( this, IDC_MRISPTOROIS );
GenerateRoisFile    = new TEdit         ( this, IDC_GENERATEROISFILE, EditSizeText );

RoiName             = new TEdit         ( this, IDC_ROINAME, EditSizeText );
Selection           = new TEdit         ( this, IDC_SELECTION, EditSizeTextLong );

MessageInteractive  = new TEdit         ( this, IDC_MESSAGEINTERACTIVE, EditSizeText );

NumRois             = new TEdit         ( this, IDC_NUMROIS, EditSizeValue );
RoisSummary         = new TListBox      ( this, IDC_ROISSUMMARY );

BaseFileName        = new TEdit         ( this, IDC_ROISFILE, EditSizeText );
OpenAuto            = new TCheckBox     ( this, IDC_OPENAUTO );


SetTransferBuffer ( &CreateRoisTransfer );


StringCopy ( BatchFilesExt, AllTracksFilesExt );


                                        // called from a view? get the pointer
BaseView            = CartoolDocManager->GetCurrentView ();
MriView             = 0;

                                        // if secondary view, switch to its associated Track view
if ( dynamic_cast< TSecondaryView* > ( BaseView ) )
    BaseView    = dynamic_cast< TSecondaryView* > ( BaseView )->GetEegView ();

                                        // is it any sort of a legal view to call from?
if ( ! ( dynamic_cast< TTracksView*        > ( BaseView )
      || dynamic_cast< TElectrodesView*    > ( BaseView )
      || dynamic_cast< TSolutionPointsView*> ( BaseView )
      || dynamic_cast< TVolumeView*        > ( BaseView ) ) )
    BaseView    = 0;

                                        // MRI case: transfer to another pointer
if ( BaseView && dynamic_cast< TVolumeView * > ( BaseView ) ) {
    MriView     = dynamic_cast< TVolumeView * > ( BaseView );
    BaseView    = 0;
    }


CanCloseBaseView    = ! BaseView;
CanCloseMriView     = ! MriView;
EegView             = 0;
XyzView             = 0;
SpView              = 0;
Names               = 0;
Processing          = CreateRoisNone;
Rois                = 0;
SelForRoi           = 0;
}


        TCreateRoisDialog::~TCreateRoisDialog ()
{
delete  InputFile;              delete  UseRoisMriFile;         delete  RoisMriFile;            delete  RoisLabelsFile;
delete  Automatic;              delete  Interactive;
delete  AutoTalairach;          delete  AutoAAL;                delete  AutoOtherRois;
delete  GenerateRoisFile;
delete  RoiName;                delete  Selection;
delete  MessageInteractive;
delete  NumRois;                delete  RoisSummary;
delete  BaseFileName;           delete  OpenAuto;
}


//----------------------------------------------------------------------------
                                        // when quitting or switching of input files
void    TCreateRoisDialog::CleanUp ()
{
Timer.Stop ();                          // stop this ASAP!


if ( Rois ) {                           // clear all structures
    delete  Rois;
    Rois        = 0;
    }

if ( SelForRoi ) {
    delete  SelForRoi;
    SelForRoi   = 0;
    }

                                        // also clear some parts of dialog
if ( this->GetHandle () )
    TransferData ( tdGetData );

CreateRoisTransfer.RoisSummary.Clear ();
StringCopy  ( CreateRoisTransfer.NumRois, "0" );
ClearString ( CreateRoisTransfer.MessageInteractive );

if ( this->GetHandle () )
    TransferData ( tdSetData );


if ( ! CartoolApplication->Closing && BaseView && BaseView->BaseDoc )    // unlock view
    BaseView->BaseDoc->AllowClosing ();

if ( ! CartoolApplication->Closing && MriView  && MriView ->BaseDoc )    // unlock view
    MriView ->BaseDoc->AllowClosing ();


                                        // optionally closing files
if ( CanCloseBaseView && BaseView && BaseView->BaseDoc->CanClose ( true ) ) {

    CartoolDocManager->CloseDoc ( BaseView->BaseDoc );

    BaseView    = 0;

    EegView     = 0;
    XyzView     = 0;
    SpView      = 0;
    Names       = 0;
    }


if ( CanCloseMriView && MriView && MriView->BaseDoc->CanClose ( true ) ) {

    CartoolDocManager->CloseDoc ( MriView->BaseDoc );

    MriView     = 0;
    }
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::SetupWindow ()
{
TBaseDialog::SetupWindow ();

Timer       = TTimer ( GetHandle () );

SetProcessing ( false );


InputFile           ->ResetCaret;
RoisMriFile         ->ResetCaret;
RoisLabelsFile      ->ResetCaret;
GenerateRoisFile    ->ResetCaret;
BaseFileName        ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmSetProcessing ()
{
SetProcessing ( false );
}


void    TCreateRoisDialog::SetProcessing ( bool docleanup )
{
if ( docleanup )
    CleanUp ();


TransferData ( tdGetData );


 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Choose Processing type
Processing  = CreateRoisNone;

                                        // test either BaseView xor MriView or none
                                        // and the input fields

if ( CreateRoisTransfer.IsInteractive () ) {

    if      ( BaseView ) {              // BaseView specified will force our choice
        if      ( dynamic_cast< TTracksView*         > ( BaseView ) )                           Processing  = CreateRoisInteractiveTracks;
        else if ( dynamic_cast< TElectrodesView*     > ( BaseView ) )                           Processing  = CreateRoisInteractiveXyz;
        else if ( dynamic_cast< TSolutionPointsView* > ( BaseView ) )                           Processing  = CreateRoisInteractiveSp;
                                        // file was open, don't close when done
        if ( docleanup )
            CanCloseBaseView    = false;
        }
    else {                              // no view given, guess from current entries
        if      ( IsExtensionAmong ( CreateRoisTransfer.InputFile, AllTracksFilesExt      ) )   Processing  = CreateRoisInteractiveTracks;
        else if ( IsExtensionAmong ( CreateRoisTransfer.InputFile, AllCoordinatesFilesExt ) )   Processing  = CreateRoisInteractiveXyz;
        else if ( IsExtensionAmong ( CreateRoisTransfer.InputFile, AllSolPointsFilesExt   ) )   Processing  = CreateRoisInteractiveSp;
        }

    } // interactive


else { // automatic

    if      ( BaseView ) {              // BaseView specified will force our choice

        if ( dynamic_cast< TSolutionPointsView* > ( BaseView ) ) {

            if      ( CreateRoisTransfer.IsAutoSpMri () && CreateRoisTransfer.IsRoisMriFile ()
                   && IsExtensionAmong ( CreateRoisTransfer.RoisMriFile, AllMriFilesExt ) )
                                                                                                Processing  = CreateRoisTransfer.IsAutoAAL () ? CreateRoisAutomaticSpAAL
                                                                                                                                              : CreateRoisAutomaticSpMri;
            else if ( CreateRoisTransfer.IsAutoTalairach () && CanTalairach () )                Processing  = CreateRoisAutomaticSpTalairach;
            }
                                        // file was open, don't close when done
        if ( docleanup )
            CanCloseBaseView    = false;
        }

    else if ( MriView ) {               // MriView specified will force our choice

        if      ( IsExtensionAmong ( CreateRoisTransfer.InputFile, AllSolPointsFilesExt ) )     Processing  = CreateRoisAutomaticSpMri;
                                        // file was open, don't close when done
        if ( docleanup )
            CanCloseMriView     = false;
        }

    else {                              // no view given, guess from current entries

        if ( IsExtensionAmong ( CreateRoisTransfer.InputFile, AllSolPointsFilesExt   ) )

            if      ( CreateRoisTransfer.IsAutoSpMri () && CreateRoisTransfer.IsRoisMriFile ()
                   && IsExtensionAmong ( CreateRoisTransfer.RoisMriFile, AllMriFilesExt ) )
                                                                                                Processing  = CreateRoisTransfer.IsAutoAAL () ? CreateRoisAutomaticSpAAL
                                                                                                                                              : CreateRoisAutomaticSpMri;
            else if ( CreateRoisTransfer.IsAutoTalairach () /*&& CanTalairach ()*/ )            Processing  = CreateRoisAutomaticSpTalairach;
        }

    } // automatic


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                       // Open necessary files

if ( BaseView == 0 && CanOpenFile ( CreateRoisTransfer.InputFile ) && Processing != CreateRoisNone ) {
                                        // open file & get view
    CanCloseBaseView    = ! CartoolDocManager->IsOpen ( CreateRoisTransfer.InputFile );
                                        // open doc
    TBaseDoc   *basedoc = CartoolDocManager->OpenDoc ( CreateRoisTransfer.InputFile, dtOpenOptions );
                                        // and get da view
    BaseView            = basedoc->GetViewList ();

                                        // can tailor the view?
//    if ( Processing != CreateRoisAutomaticSpMri )   // ?
    if ( CreateRoisTransfer.IsInteractive () )         // <- rather

        if      ( dynamic_cast< TElectrodesView* > ( BaseView ) ) {

            BaseView->SendMessage ( WM_COMMAND, IDB_FLATVIEW,    0 );   // 2D display
            BaseView->SendMessage ( WM_COMMAND, IDB_ORIENT,      0 );   // orientation is not correct after going 2D
            BaseView->SendMessage ( WM_COMMAND, IDB_SHOWELEC,    0 );   // showing bigger electrodes (easier to click on)..
            BaseView->SendMessage ( WM_COMMAND, IDB_SHOWELNAMES, 0 );   // & electrodes names
            }

        else if ( dynamic_cast< TSolutionPointsView* > ( BaseView ) ) {

            BaseView->SendMessage ( WM_COMMAND, IDB_SURFACEMODE, 0 );
            }

    SetFocus ();
    }


if ( MriView  == 0 && CreateRoisTransfer.IsRoisMriFile () ) {
                                        // open file & get view
    CanCloseMriView     = ! CartoolDocManager->IsOpen ( CreateRoisTransfer.RoisMriFile );
                                        // open doc
    TBaseDoc   *basedoc = CartoolDocManager->OpenDoc ( CreateRoisTransfer.RoisMriFile, dtOpenOptions );
                                        // and get da view
    MriView             = dynamic_cast< TVolumeView * > ( basedoc->GetViewList () );

    SetFocus ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Put locks on
if ( BaseView && BaseView->BaseDoc )
    BaseView->BaseDoc->PreventClosing ();

if ( MriView  && MriView->BaseDoc )
    MriView ->BaseDoc->PreventClosing ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update main input file in Dialog
if ( BaseView )
    StringCopy ( CreateRoisTransfer.InputFile,     BaseView->BaseDoc->GetDocPath (), EditSizeText - 1 );

if ( MriView )
    StringCopy ( CreateRoisTransfer.RoisMriFile,   MriView ->BaseDoc->GetDocPath (), EditSizeText - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some more consistency checkings
if ( Processing != CreateRoisNone ) {

    if ( StringIsNotEmpty ( CreateRoisTransfer.InputFile ) ) {

        StringCopy      ( CreateRoisTransfer.BaseFileName, CreateRoisTransfer.InputFile );
        RemoveExtension ( CreateRoisTransfer.BaseFileName );
        }


    if ( Processing == CreateRoisAutomaticSpTalairach ) {

        if ( ! CanTalairach () ) {

            CreateRoisTransfer.AutoTalairach   = BoolToCheck ( false );
            CreateRoisTransfer.AutoAAL         = BoolToCheck ( false );
            CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( true  );

//          Processing = CreateRoisAutomaticSpMri;
            Processing = CreateRoisTransfer.IsRoisFile () ? CreateRoisAutomaticSpMri : CreateRoisNone;
            }
        }

    } // if some processing


                                        // update buttons if we forced switching
if ( Processing == CreateRoisAutomaticSpMri ) {

    CreateRoisTransfer.AutoTalairach   = BoolToCheck ( false );
    CreateRoisTransfer.AutoAAL         = BoolToCheck ( false );
    CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( true  );
    }
else if ( Processing == CreateRoisAutomaticSpAAL ) {

    CreateRoisTransfer.AutoTalairach   = BoolToCheck ( false );
    CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( false );
    CreateRoisTransfer.AutoAAL         = BoolToCheck ( true  );
    }


TransferData ( tdSetData );

BaseFileName->ResetCaret;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create working variables

                                        // handy potential conversions of BaseView
EegView     = Processing == CreateRoisInteractiveTracks     ? dynamic_cast< TTracksView*        > ( BaseView ) : 0;
XyzView     = Processing == CreateRoisInteractiveXyz        ? dynamic_cast< TElectrodesView*    > ( BaseView ) : 0;
SpView      = Processing == CreateRoisInteractiveSp
           || Processing == CreateRoisAutomaticSpMri
           || Processing == CreateRoisAutomaticSpAAL
           || Processing == CreateRoisAutomaticSpTalairach  ? dynamic_cast< TSolutionPointsView*> ( BaseView ) : 0;


if      ( EegView ) {
    if ( SelForRoi )    delete  SelForRoi;
    SelForRoi   = new TSelection ( EegView->GetEEGDoc ()->GetNumElectrodes (), OrderSorted );
    Names       = EegView->GetElectrodesNames ();
    }

else if ( XyzView ) {
    if ( SelForRoi )    delete  SelForRoi;
    SelForRoi   = new TSelection ( XyzView->GetXYZDoc ()->GetNumElectrodes (), OrderSorted );
    Names       = XyzView->GetElectrodesNames ();
    }

else if ( SpView ) {
    if ( SelForRoi )    delete  SelForRoi;
    SelForRoi   = new TSelection ( SpView->GetSPDoc ()->GetNumSolPoints (), OrderSorted );
    Names       = SpView ->GetSPNames ();
    }


                                        // Init the interactive dialog
if ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView ) && SelForRoi ) {

    SelForRoi->SentFrom = BaseView->LinkedViewId;

    Timer.Start ( TimerRefresh, 250, 1 );

    UpdateSelection ( true );
    }

}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmChangeInputFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CanCloseBaseView );
}


void    TCreateRoisDialog::CmUseRoisMriFile ()
{
TransferData ( tdGetData );


if ( CreateRoisTransfer.IsRoisFile () ) {

    if ( MriView  == 0 && CanOpenFile ( CreateRoisTransfer.RoisMriFile ) && IsExtensionAmong ( CreateRoisTransfer.RoisMriFile, AllMriFilesExt ) ) {
                                        // open file & get view
        CanCloseMriView     = ! CartoolDocManager->IsOpen ( CreateRoisTransfer.RoisMriFile );
                                        // open doc
        TBaseDoc   *basedoc = CartoolDocManager->OpenDoc ( CreateRoisTransfer.RoisMriFile, dtOpenOptions );
                                        // and get da view
        MriView             = dynamic_cast< TVolumeView * > ( basedoc->GetViewList () );

        SetFocus ();
        }

//  CreateRoisTransfer.AutoTalairach   = BoolToCheck ( false );
//  CreateRoisTransfer.AutoAAL         = BoolToCheck ( false );
//  CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( true  );
    }

else {
    if ( CanCloseMriView && MriView && MriView->BaseDoc->CanClose ( true ) ) {

        CartoolDocManager->CloseDoc ( MriView->BaseDoc );

        MriView     = 0;
        }

//  if ( CanTalairach () ) {
//      CreateRoisTransfer.AutoAAL         = BoolToCheck ( false );
//      CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( false );
//      CreateRoisTransfer.AutoTalairach   = BoolToCheck ( true  );
//      }

    if ( ! CanTalairach () ) {
        CreateRoisTransfer.AutoTalairach   = BoolToCheck ( false );
        CreateRoisTransfer.AutoAAL         = BoolToCheck ( false );
        CreateRoisTransfer.AutoOtherRois   = BoolToCheck ( true  );
        }
    }


TransferData ( tdSetData );


SetProcessing ( false );
}


void    TCreateRoisDialog::CmAutomaticChanged ()
{
SetProcessing ( false );
}


void    TCreateRoisDialog::CmChangeRoisMriFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
//tce.Enable ( CanCloseMriView );

tce.Enable ( CreateRoisTransfer.IsRoisFile () );
}


void    TCreateRoisDialog::CmRoisLabelsFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsRoisFile () && CreateRoisTransfer.IsAutoOtherRois () );
}


void    TCreateRoisDialog::CmInteractiveEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsInteractive () );
}


void    TCreateRoisDialog::CmAutomaticEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsAutomatic () );
}


void    TCreateRoisDialog::CmGenerateRoisFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsAutoTalairach () || CreateRoisTransfer.IsAutoOtherRois () );
}


void    TCreateRoisDialog::CmTalairachEnable ( TCommandEnabler &tce )
{
tce.Enable ( CreateRoisTransfer.IsAutomatic () && CanTalairach () );
}


bool    TCreateRoisDialog::CanTalairach ()  const
{
return  Taloracle.IsAllocated ()
     &&    ! BaseView
        || ( BaseView && BaseView->GetGeometryTransform () && StringIs ( BaseView->GetGeometryTransform ()->Name, "Talairach" ) );

}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::EvTimer ( uint timerId )
{
switch ( timerId ) {

    case TimerRefresh :                 // only when IsInteractive

        UpdateApplication;
                                        // we don't increment nor reset the timer, so it keeps flashing forever
                                        // refresh only if focus is on eeg
        if ( GetFocus () == BaseView->GetHandle () )

            UpdateSelection ( true );

        break;
    }
}


void    TCreateRoisDialog::CmSelectionChange ()
{
//if ( GetHandle () )
if ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView ) )
    UpdateSelection ( false );
}


void    TCreateRoisDialog::UpdateSelection ( bool getselection )
{
if ( ! BaseView || ! SelForRoi )
    return;


                                        // use GetText & SetText so not to trigger a Selection change!
if ( getselection ) {

    if      ( EegView ) {

        EegView->GetHighlighted ( SelForRoi );

        EegView->GetEEGDoc ()->ClearPseudo ( *SelForRoi );
                                        // just store the selection in the Edit field, so the user can think twice
        SelForRoi->ToText  ( CreateRoisTransfer.Selection, Names, AuxiliaryTracksNames );
        }

    else if ( XyzView ) {

        XyzView->GetHighlighted ( SelForRoi );
                                        // just store the selection in the Edit field, so the user can think twice
        SelForRoi->ToText  ( CreateRoisTransfer.Selection, Names );
        }

    else if ( SpView ) {

        SpView->GetHighlighted ( SelForRoi );
                                        // just store the selection in the Edit field, so the user can think twice
        SelForRoi->ToText  ( CreateRoisTransfer.Selection, Names );
        }


    Selection->SetText ( CreateRoisTransfer.Selection );
    Selection->ResetCaret;

    } // getselection

                                        // put selection
else {

    if ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView ) ) {

        Selection->GetText ( CreateRoisTransfer.Selection, EditSizeTextLong );

        SelForRoi->Reset ();
                                        // Set silently, as to shut up incomplete names seen as "errors" while typing
        SelForRoi->Set ( CreateRoisTransfer.Selection, Names, false );

        if      ( EegView )     EegView->VnNewHighlighted ( SelForRoi );
        else if ( XyzView )     XyzView->VnNewHighlighted ( SelForRoi );
        else if ( SpView  )     SpView ->VnNewHighlighted ( SelForRoi );
        }

    } // put selection

}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmAddSelectionEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView )
          && ( ! Rois || Rois->GetAtomsNotSelected()->NumSet () != 0 ) && SelForRoi && SelForRoi->NumSet () != 0 );
}


void    TCreateRoisDialog::CmAddSelection ()
{
Selection->GetText ( CreateRoisTransfer.Selection, EditSizeTextLong );
RoiName  ->GetText ( CreateRoisTransfer.RoiName,   EditSizeText );


if ( StringIsSpace ( CreateRoisTransfer.Selection ) )
    return;


if ( ! Rois )                           // create our temp Rois
    Rois    = new TRois ();


bool                addok           = false;


if ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView ) ) {
                                        // reprocess the text back to selection
    SelForRoi->Reset ();

    SelForRoi->Set ( CreateRoisTransfer.Selection, Names );

    if ( EegView )
        EegView->GetEEGDoc ()->ClearPseudo ( *SelForRoi );


    addok   = Rois->AddRoi ( *SelForRoi, CreateRoisTransfer.RoiName, true, true );


    if ( ! addok )
        ShowMessage (   "Your selection was either empty, or already used in other ROIs!" NewLine 
                        "Try modifying your selection and submitting again,"              NewLine 
                        "or finish your session.", 
                        "Add New Roi", ShowMessageWarning );

                                        // clear view
    SelForRoi->Reset ();

    if      ( EegView )     EegView->VnNewHighlighted ( SelForRoi );
    else if ( XyzView )     XyzView->VnNewHighlighted ( SelForRoi );
    else if ( SpView  )     SpView ->VnNewHighlighted ( SelForRoi );
    }


                                        // clear dialog selection
RoiName  ->SetText ( "" );
Selection->SetText ( "" );

if ( Rois->GetAtomsNotSelected()->NumSet () == 0 )
    MessageInteractive->SetText ( "No more elements to select from!" );
else
    MessageInteractive->SetText ( "" );



if ( addok )
    AddRoiSummary ();
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmRemoveSelectionEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView )
          && Rois && Rois->GetNumRois () );
}


void    TCreateRoisDialog::CmRemoveSelection ()
{
if ( ! Rois || Rois->GetNumRois () == 0 )
    return;


const TRoi*         roi             = Rois->GetRoi ( Rois->GetNumRois () - 1 );

                                        // restore to edit
RoiName  ->SetText ( Rois->GetRoiName ( Rois->GetNumRois () - 1 )  );
                                        // setting will trigger CmSelectionChange, thus updating the highlighted display
roi->Selection.ToText  ( CreateRoisTransfer.Selection, Names );
Selection->SetText ( CreateRoisTransfer.Selection );

                                        // forget from structure
Rois->RemoveRoi ();

                                        // update summary
NumRois->SetIntValue ( Rois->GetNumRois () );
RoisSummary->DeleteString ( 0 );
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::AddRoiSummary ()
{
if ( ! Rois )
    return;


char                buff  [ MaxPathShort ];
char                buff2 [ MaxPathShort ];
int                 r               = Rois->GetNumRois () - 1;
const TRoi*         toroi           = Rois->GetRoi ( r );


if ( (int) toroi->Selection > 20 )

    StringCopy  (   buff2,  
                    (*Names)[ toroi->Selection.FirstSelected () ], 
                    " .. ",
                    (*Names)[ toroi->Selection.LastSelected  () ] 
                );

else {
    if      ( EegView )     toroi->Selection.ToText ( buff2, Names, AuxiliaryTracksNames );
    else if ( XyzView )     toroi->Selection.ToText ( buff2, Names );
    else if ( SpView  )     toroi->Selection.ToText ( buff2, Names );
    }


StringCopy  (   buff, 
                Rois->GetRoiName ( r ), ":  ", IntegerToString ( (int) toroi->Selection ), " Element", StringPlural ( (int) toroi->Selection ),
                "  ( ", buff2, " )"
            );

RoisSummary->InsertString ( buff, 0 );


NumRois->SetIntValue ( Rois->GetNumRois () );
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


if ( CreateRoisTransfer.IsAutoTalairach () && SpView
  && CreateRoisTransfer.IsGenerateRoisFile ()
  && CanTalairach () ) {

    tce.Enable ( true );
    return;
    }


if ( CreateRoisTransfer.IsAutoSpMri () && SpView
  && CreateRoisTransfer.IsRoisMriFile ()
  && ( ! CreateRoisTransfer.IsAutoOtherRois () || CreateRoisTransfer.IsRoisLabelsFile ()  )
  && ( StringIsEmpty ( CreateRoisTransfer.GenerateRoisFile ) || CreateRoisTransfer.IsGenerateRoisFile () ) ) {

    tce.Enable ( true );
    return;
    }


if ( CreateRoisTransfer.IsInteractive () && ( EegView || XyzView || SpView )
  && Rois && Rois->GetNumRois () != 0 ) {
    tce.Enable ( true );
    return;
    }


tce.Enable ( false );
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmOk ()
{
Destroy ();

if ( CartoolMainWindow )
    CartoolMainWindow->SetFocus ();      // modeless seems to lose the focus...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      (    CreateRoisTransfer.IsInteractive () 
          && ( EegView || XyzView || SpView ) 
          && Rois                               ) {

    GenerateRois                        (
                                        Processing,
                                        *Rois,

                                          EegView ? EegView->BaseDoc 
                                        : XyzView ? XyzView->BaseDoc 
                                        : SpView  ? SpView ->BaseDoc 
                                        : 0,
                                        CreateRoisTransfer.IsRoisMriFile () && MriView                                  ? MriView->GetMRIDoc ()               : 0,
                                        CreateRoisTransfer.IsRoisLabelsFile () && CreateRoisTransfer.IsAutoOtherRois () ? CreateRoisTransfer.RoisLabelsFile   : 0,
                                        CreateRoisTransfer.IsGenerateRoisFile ()                                        ? CreateRoisTransfer.GenerateRoisFile : 0,

                                        CreateRoisTransfer.BaseFileName
                                        );
    } // IsInteractive

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( CreateRoisTransfer.IsAutomatic () && SpView ) {

    if      (  (    Processing == CreateRoisAutomaticSpMri
                 || Processing == CreateRoisAutomaticSpAAL ) 
              && MriView                                     ) {
                                        // should it also update global Rois?
        GenerateRoisFromSpAndMri        (
                                        Processing,

                                        SpView ->GetSPDoc  (),
                                        MriView->GetMRIDoc (),
                                        CreateRoisTransfer.RoisLabelsFile,

                                        CreateRoisTransfer.IsAutoOtherRois () ? CreateRoisTransfer.GenerateRoisFile : 0,

                                        CreateRoisTransfer.BaseFileName
                                        );
        } // IsAutoSpMri

    else if (    Processing == CreateRoisAutomaticSpTalairach 
              && StringIsNotEmpty ( CreateRoisTransfer.GenerateRoisFile ) ) {

        GenerateRoisFromSpAndTalairach  (
                                        Processing,

                                        SpView ->GetSPDoc  (),
                                        MriView && CreateRoisTransfer.IsRoisFile () ? MriView->GetMRIDoc () : 0,

                                        CreateRoisTransfer.GenerateRoisFile,    // mandatory

                                        CreateRoisTransfer.BaseFileName
                                        );
        }
    } // IsAutomatic


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CleanUp ();


if ( CheckToBool ( CreateRoisTransfer.OpenAuto ) ) {

    TFileName           roifilename;
    TFileName           volroifilename;

    StringCopy      ( roifilename,      CreateRoisTransfer.BaseFileName );
    AddExtension    ( roifilename,      FILEEXT_ROIS );

    StringCopy      ( volroifilename,   roifilename   );
    AddExtension    ( volroifilename,   DefaultMriExt );


    roifilename   .Open ();
    volroifilename.Open ();     // only if it exists
    } // OpenAuto

}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmCancel ()
{
CleanUp ();


TBaseDialog::CmCancel ();


if ( CartoolMainWindow )
    CartoolMainWindow->SetFocus ();      // modeless seems to lose the focus...
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmBrowseInputFile ()
{
SetInputFile ( 0 );
}


void    TCreateRoisDialog::SetInputFile ( const char* file )
{
static GetFileFromUser  getfile ( "Input File", AllTracksRoisFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CreateRoisTransfer.InputFile ) )
        return;

    TransferData ( tdSetData );

                                        // update available choices
    SetProcessing ( true );
    }
else {
    StringCopy ( CreateRoisTransfer.InputFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

InputFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmBrowseRoisMriFile ()
{
SetRoisMriFile ( 0 );
}


void    TCreateRoisDialog::SetRoisMriFile ( const char* file )
{
static GetFileFromUser  getfile ( "ROIs Volume File", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CreateRoisTransfer.RoisMriFile ) )
        return;

    CreateRoisTransfer.UseRoisMriFile  = BoolToCheck ( true  );
//  AutoTalairach  ->SetCheck ( BoolToCheck ( false ) );
//  AutoAAL        ->SetCheck ( BoolToCheck ( false ) );
//  AutoOtherRois  ->SetCheck ( BoolToCheck ( true  ) );

    TransferData ( tdSetData );

                                        // update available choices
    SetProcessing ( true );
    }
else {
    StringCopy ( CreateRoisTransfer.RoisMriFile, file );
    getfile.SetOnly ( file );

    CreateRoisTransfer.UseRoisMriFile  = BoolToCheck ( true  );
//  AutoTalairach  ->SetCheck ( BoolToCheck ( false ) );
//  AutoAAL        ->SetCheck ( BoolToCheck ( false ) );
//  AutoOtherRois  ->SetCheck ( BoolToCheck ( true  ) );
    }


TransferData ( tdSetData );

RoisMriFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmBrowseTextFile ( owlwparam w )
{
SetTextFile ( w, 0 );
}


void    TCreateRoisDialog::SetTextFile ( owlwparam w, const char* file )
{
GetFileFromUser  getfile ( w == IDC_BROWSEROISLABELSFILE ? "ROIs Labels File" : "Generated ROIs File", AllTextFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );

char*               tofilename      = w == IDC_BROWSEROISLABELSFILE ? CreateRoisTransfer.RoisLabelsFile
                                                                    : CreateRoisTransfer.GenerateRoisFile;


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( tofilename ) )
        return;
    }
else {
    StringCopy ( tofilename, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

RoisLabelsFile  ->ResetCaret;
GenerateRoisFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::CmUpOneDirectory ()
{
RemoveLastDir ( CreateRoisTransfer.BaseFileName );

BaseFileName->SetText ( CreateRoisTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


void    TCreateRoisDialog::CmBrowseRoiFile ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( CreateRoisTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateRoisDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                tracksfiles     ( drop, AllTracksFilesExt       );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                spfiles         ( drop, AllSolPointsFilesExt    );
TGoF                mrifiles        ( drop, AllMriFilesExt          );
TGoF                txtfiles        ( drop, AllTextFilesExt, &where );
TGoF                remainingfiles  ( drop, AllTracksFilesExt " " AllCoordinatesFilesExt " " AllSolPointsFilesExt " " AllMriFilesExt " " AllTextFilesExt, 0, true );
char                buff[ 256 ];

drop.DragFinish ();

                                        // test legality to change some of the files
if ( ( (bool) tracksfiles || (bool) xyzfiles || (bool) spfiles ) && ! CanCloseBaseView && BaseView ) {

    StringCopy  ( buff, "You are already working with file:"                 NewLine 
                                                                             NewLine, 
                        BaseView->BaseDoc->GetTitle (),                      NewLine 
                                                                             NewLine 
                        "Close this Dialog, select another window (or none)" NewLine 
                        "if you want to work on another file." );

    ShowMessage ( buff, "Create ROIs", ShowMessageWarning );
    return;
    }

if ( (bool) mrifiles && ! CanCloseMriView && MriView ) {

    StringCopy  ( buff, "You are already working with file:"                 NewLine 
                                                                             NewLine, 
                        MriView ->BaseDoc->GetTitle (),                      NewLine 
                                                                             NewLine 
                        "Close this Dialog, select another window (or none)" NewLine 
                        "if you want to work on another file." );

    ShowMessage ( buff, "Create ROIs", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) mrifiles; i++ ) {

    SetRoisMriFile ( mrifiles[ i ] );
                                        // guess spi file name from lf file name
    TFileName               filelabels ( mrifiles[ i ] );
                                        // There could be 2 cases(!):
                                        // file.ext.txt
    AddExtension    ( filelabels, FILEEXT_TXT );

    if ( ! filelabels.CanOpenFile () ) {
                                        // file.txt
        RemoveExtension ( filelabels, 2 );
        AddExtension    ( filelabels, FILEEXT_TXT );
        }

    if ( filelabels.CanOpenFile () )

        SetTextFile ( IDC_BROWSEROISLABELSFILE, filelabels );
    }


for ( int i = 0; i < (int) txtfiles; i++ )
    SetTextFile ( where.Y < 145 ? IDC_BROWSEROISLABELSFILE : IDC_BROWSEGENERATEROISFILE, txtfiles[ i ] );


for ( int i = 0; i < (int) tracksfiles; i++ )
    SetInputFile ( tracksfiles[ i ] );


for ( int i = 0; i < (int) xyzfiles; i++ )
    SetInputFile ( xyzfiles[ i ] );


for ( int i = 0; i < (int) spfiles; i++ )
    SetInputFile ( spfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );


                                        // update available choices
if ( (bool) tracksfiles || (bool) xyzfiles || (bool) spfiles || (bool) mrifiles || (bool) txtfiles )
    SetProcessing ( (bool) tracksfiles || (bool) xyzfiles || (bool) spfiles || (bool) mrifiles );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
