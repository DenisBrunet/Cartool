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
#include    <owl/tooltip.h>

#include    "TCoregistrationDialog.h"

#include    "Strings.Utils.h"
#include    "Dialogs.TSuperGauge.h"
#include    "GlobalOptimize.Tracks.h"
#include    "Volumes.SagittalTransversePlanes.h"
#include    "Electrodes.TransformElectrodes.h"

#include    "TElectrodesView.h"
#include    "TVolumeView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

OptimizeOff

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TCoregistrationStruct       CoregTransfer;
TCoregistrationDialog*      CoregistrationDlg   = 0;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrapper to plug a TPoints into the dialog, using an intermediate temp file
bool    CoregisterXyzToMriInteractive   (   TPoints&        xyzpoints,  TStrings&       xyznames,
                                            TVolumeDoc*     mridoc,     Volume&         FullVolume, 
//                                          TMatrix44       mriabstoguillotine, 
                                            bool            init,
                                            TMatrix44&      XyzCoregToNorm
                                        )
{
                                        // write to temp file
TFileName           fileinxyz;


GetTempFilePath     ( fileinxyz );
AddExtension        ( fileinxyz, FILEEXT_XYZ );

xyzpoints.WriteFile ( fileinxyz, &xyznames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting the transfer buffer
CoregTransfer.Presets.Select ( init ? CoregisterXyzToMriScratch : CoregisterXyzToMriReload );

StringCopy  ( CoregTransfer.InputSourceFile, fileinxyz );
StringCopy  ( CoregTransfer.InputTargetFile, mridoc->GetDocPath () );

CoregTransfer.Glue      = BoolToCheck ( false );
                                        // actual base file name will be set upon dialog creation
ClearString ( CoregTransfer.BaseFileName        );
ClearString ( CoregTransfer.SavingAltElectrodes );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init the dialog modeless
CoregistrationDlg   = new TCoregistrationDialog ( CartoolObjects.CartoolMdiClient, IDD_COREGISTRATION, false );

                                        // run it modeless, so that we can switch focus to another window to tailor its view
bool                success         = CoregistrationDlg->Create ();

                                        // !we have to actively wait for completion, as the modeless dialog has already exited!
do UpdateApplication while ( CoregistrationDlg != 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // try to retrieve the output file name
TFileName           fileoutxyz;

StringCopy  ( fileoutxyz,       CoregTransfer.BaseFileName,     "." FILEEXT_XYZ       );
                                        // file exist only if user has pressed OK, Cancel will leave no file on the other hand
success     = success && CanOpenFile ( fileoutxyz );


if ( success ) {

    xyzpoints.ReadFile ( fileoutxyz );

    CartoolObjects.CartoolDocManager->CloseDoc ( fileoutxyz );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // delete everything with our temp file name
ReplaceExtension    ( fileinxyz, "*" );
DeleteFiles         ( fileinxyz );


return  success;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  CoregitrationTypeString[ NumCoregitrationType ][ 64 ] =
            {
            "Interactive Coregistration - Starting from scratch",
            "Interactive Coregistration - Reloading as is for fine-tuning",
            };


//----------------------------------------------------------------------------

        TCoregistrationStruct::TCoregistrationStruct ()
{
Presets.Clear ();
for ( int i = 0; i < NumCoregitrationType; i++ )
    Presets.AddString ( CoregitrationTypeString[ i ], i == CoregitrationTypeDefault );

ClearString     ( InputSourceFile   );
ClearString     ( InputTargetFile   );
Glue            = BoolToCheck ( false );
ClearString     ( BaseFileName        );
ClearString     ( SavingAltElectrodes );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TCoregistrationDialog, TBaseDialog )

    EV_WM_DROPFILES,

//  EV_COMMAND                  ( IDC_PROCESSAUTOMATIC,         CmSetProcessing ),
//  EV_COMMAND                  ( IDC_PROCESSINTERACTIVE,       CmSetProcessing ),

    EV_CBN_SELCHANGE            ( IDC_PRESETS,                  EvPresetsChange ),

    EV_COMMAND                  ( IDOK,                         CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),
    EV_COMMAND                  ( IDCANCEL,                     CmCancel ),

    EV_COMMAND                  ( IDC_BROWSEXYZDOC,             CmBrowseInputSourceFile ),
    EV_COMMAND                  ( IDC_BROWSEMRIFILE,            CmBrowseInputTargetFile ),
    EV_COMMAND                  ( IDC_UPDIRECTORY,              CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,       CmBrowseOutputFile ),

    EV_COMMAND_AND_ID           ( IDC_SCALEGLOBALPLUS,          CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEGLOBALMINUS,         CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEXPLUS,               CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEXMINUS,              CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEYPLUS,               CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEYMINUS,              CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEZPLUS,               CmAction ),
    EV_COMMAND_AND_ID           ( IDC_SCALEZMINUS,              CmAction ),

    EV_COMMAND_AND_ID           ( IDC_TRANSLATEXPLUS,           CmAction ),
    EV_COMMAND_AND_ID           ( IDC_TRANSLATEXMINUS,          CmAction ),
    EV_COMMAND_AND_ID           ( IDC_TRANSLATEYPLUS,           CmAction ),
    EV_COMMAND_AND_ID           ( IDC_TRANSLATEYMINUS,          CmAction ),
    EV_COMMAND_AND_ID           ( IDC_TRANSLATEZPLUS,           CmAction ),
    EV_COMMAND_AND_ID           ( IDC_TRANSLATEZMINUS,          CmAction ),

    EV_COMMAND_AND_ID           ( IDC_ROTATEXPLUS,              CmAction ),
    EV_COMMAND_AND_ID           ( IDC_ROTATEXMINUS,             CmAction ),
    EV_COMMAND_AND_ID           ( IDC_ROTATEYPLUS,              CmAction ),
    EV_COMMAND_AND_ID           ( IDC_ROTATEYMINUS,             CmAction ),
    EV_COMMAND_AND_ID           ( IDC_ROTATEZPLUS,              CmAction ),
    EV_COMMAND_AND_ID           ( IDC_ROTATEZMINUS,             CmAction ),

    EV_COMMAND_AND_ID           ( IDC_GLUE,                     CmAction ),

    EV_COMMAND_AND_ID           ( IDC_UNDOLAST,                 CmAction ),
    EV_COMMAND_AND_ID           ( IDC_UNDOALL,                  CmAction ),
    EV_COMMAND_AND_ID           ( IDC_REDOLAST,                 CmAction ),
    EV_COMMAND_AND_ID           ( IDC_REDOALL,                  CmAction ),
    EV_COMMAND_AND_ID           ( IDC_RESETTRANSFORM,           CmAction ),
    EV_COMMAND_ENABLE           ( IDC_UNDOLAST,                 CmUndoEnable ),
    EV_COMMAND_ENABLE           ( IDC_UNDOALL,                  CmUndoEnable ),
    EV_COMMAND_ENABLE           ( IDC_REDOLAST,                 CmRedoEnable ),
    EV_COMMAND_ENABLE           ( IDC_REDOALL,                  CmRedoEnable ),

END_RESPONSE_TABLE;


        TCoregistrationDialog::TCoregistrationDialog ( TWindow* parent, TResId resId, bool usingopenview, TMatrix44* xyzcoregtonorm )
      : TBaseDialog ( parent, resId )
{
Presets             = new TComboBox     ( this, IDC_PRESETS );

InputSourceFile     = new TEdit     ( this, IDC_XYZDOCFILE, EditSizeText );
InputTargetFile     = new TEdit     ( this, IDC_MRIFILE, EditSizeText );
Glue                = new TCheckBox ( this, IDC_GLUE );
BaseFileName        = new TEdit     ( this, IDC_BASEFILENAME, EditSizeText );
SavingAltElectrodes = new TEdit     ( this, IDC_SAVINGALTELEC, EditSizeText );


SetTransferBuffer ( &CoregTransfer );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Init electrodes and MRI views
BaseView            = 0;
MriView             = 0;

if ( usingopenview ) {
                                        // called from a view?
    BaseView            = CartoolDocManager->GetCurrentView ();

                                        // is it any sort of a legal view to call from?
    if ( ! ( dynamic_cast<TElectrodesView*> ( BaseView )
          || dynamic_cast<TVolumeView*>     ( BaseView ) ) )
        BaseView    = 0;

                                        // MRI case: transfer to another pointer
    if ( BaseView &&  dynamic_cast<TVolumeView*> ( BaseView ) ) {

        MriView     = dynamic_cast<TVolumeView*> ( BaseView );
        BaseView    = 0;
        }

                                        // Now look more thoroughly to all opened documents:
    int             countxyzs       = 0;
    TElectrodesDoc* lastxyzdoc      = 0;
    int             countmris       = 0;
    TVolumeDoc*     lastmridoc      = 0;

    for ( TDocument* doc = CartoolDocManager->DocList.Next ( 0 ); doc != 0; doc = CartoolDocManager->DocList.Next ( doc ) ) {
        
        if      ( dynamic_cast<TElectrodesDoc*> ( doc ) ) {
            countxyzs++;
            lastxyzdoc  = dynamic_cast<TElectrodesDoc*> ( doc );
            }
        
        else if ( dynamic_cast<TVolumeDoc*> ( doc ) ) {
            countmris++;
            lastmridoc  = dynamic_cast<TVolumeDoc*> ( doc );
            }
        }

                                        // Try to fill the missing input field when it looks the less ambiguous
    if ( BaseView == 0 && countxyzs == 1 )   BaseView    = lastxyzdoc->GetViewList ();
    if ( MriView  == 0 && countmris == 1 )   MriView     = dynamic_cast<TVolumeView*> ( lastmridoc->GetViewList () );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

XyzCoregToNorm      = xyzcoregtonorm;

if ( XyzCoregToNorm )
    XyzCoregToNorm->SetIdentity ();


CanCloseBaseView    = ! BaseView;
CanCloseMriView     = ! MriView;
XyzView             = 0;
Processing          = CoregisterNone;

TranslateStep       = 1;
Operation           = EOS;
Axis                = EOS;
Direction           = '+';

DelayForPolling             =  500;
DelayBeforeAcceleration     =  600; // set the beginning of acceleration quite early
DelayAfterNoAcceleration    = 5000;
MinSpeed                    =    1;
MaxSpeed                    =   30;
MinAcceleration             =    1;
}


        TCoregistrationDialog::~TCoregistrationDialog ()
{
CleanUp ( false );


delete  Presets;
delete  InputSourceFile;        delete  InputTargetFile;
delete  Glue;
delete  BaseFileName;           delete  SavingAltElectrodes;
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::SetupWindow ()
{
TBaseDialog::SetupWindow ();

                                        // resetting this
CoregTransfer.Glue  = CheckToBool ( false );
                                        // restoring previous content
TransferData ( tdSetData );


SetProcessing ( true, false );


InputSourceFile     ->ResetCaret;
InputTargetFile     ->ResetCaret;
BaseFileName        ->ResetCaret;
SavingAltElectrodes ->ResetCaret;


StopAccelerating ();


auto&               tooltip         = *TTooltip::Make ( this );

tooltip.AddTool (   GetHandle (),   IDC_SCALEGLOBALPLUS,   "Rescaling up globally" );
tooltip.AddTool (   GetHandle (),   IDC_SCALEGLOBALMINUS,  "Rescaling down globally" );
tooltip.AddTool (   GetHandle (),   IDC_GLUE,              "Projecting electrodes to the nearest surface - Usually done as a last step" );
tooltip.AddTool (   GetHandle (),   IDC_UPDIRECTORY,       "Move output directory up" );
//tooltip.AddTool (   GetHandle (),   IDC_SAVINGALTELEC,     "Optional sub-set list of electrodes to be additionally outputted" );   // tooltip on TEdit is blinking?
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::EvPresetsChange ()
{
                                        // we need to reset so many locks
CleanUp ( false );

SetProcessing ( false, false );
}


//----------------------------------------------------------------------------
                                        // We need to intervene here to explicitly catch the keyboard in a modeless dialog
                                        // https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgbox-programming-considerations
bool    TCoregistrationDialog::PreProcessMsg ( MSG& msg )
{
if ( GetHandle () == 0 )    return false;

                                        // returns true if message was for me
if ( ::IsDialogMessage ( GetHandle (), &msg ) ) {

                                        // also manually handling the key down messages
    if ( msg.message == WM_KEYDOWN ) {
                                        // call the guy that does not its job...
        EvKeyDown ( msg.wParam, 1, 0 );


        if      ( Operation == '*'         ) CmAction (                                             IDC_SCALEGLOBALPLUS                       );

        else if ( Operation == '/'         ) CmAction (                                             IDC_SCALEGLOBALMINUS                      );

        else if ( Operation == 'G'         ) {
                                        // currently, CmAction directly tests the checkbox
            ToggleCheck ( Glue );

                                            CmAction ( IDC_GLUE );
            }

        else if ( Operation == 'R' && Axis ) CmAction (      Axis == 'X' ?   ( Direction == '+' ?   IDC_ROTATEXPLUS     : IDC_ROTATEXMINUS    )
                                                         :   Axis == 'Y' ?   ( Direction == '+' ?   IDC_ROTATEYPLUS     : IDC_ROTATEYMINUS    )
                                                         :   Axis == 'Z' ?   ( Direction == '+' ?   IDC_ROTATEZPLUS     : IDC_ROTATEZMINUS    )
                                                         :                   0
                                                      );

        else if ( Operation == 'T' && Axis ) CmAction (      Axis == 'X' ?   ( Direction == '+' ?   IDC_TRANSLATEXPLUS  : IDC_TRANSLATEXMINUS )
                                                         :   Axis == 'Y' ?   ( Direction == '+' ?   IDC_TRANSLATEYPLUS  : IDC_TRANSLATEYMINUS )
                                                         :   Axis == 'Z' ?   ( Direction == '+' ?   IDC_TRANSLATEZPLUS  : IDC_TRANSLATEZMINUS )
                                                         :                   0
                                                      );

        else if ( Operation == 'S' && Axis ) CmAction (      Axis == 'X' ?   ( Direction == '+' ?   IDC_SCALEXPLUS      : IDC_SCALEXMINUS     )
                                                         :   Axis == 'Y' ?   ( Direction == '+' ?   IDC_SCALEYPLUS      : IDC_SCALEYMINUS     )
                                                         :   Axis == 'Z' ?   ( Direction == '+' ?   IDC_SCALEZPLUS      : IDC_SCALEZMINUS     )
                                                         :                   0
                                                      );
        }

    return  true;
    }

else
                                        // otherwise, forward message to the application
    return  TBaseDialog::PreProcessMsg ( msg );
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::EvKeyDown ( uint key, uint repeatCount, uint flags )
{
switch ( key ) {

    case 'X':
    case 'Y':
    case 'Z':
        Axis        = key;
        Operation   = EOS;
        break;


    case VK_ADD:
        Direction   = '+';
        Operation   = EOS;
        break;

    case VK_SUBTRACT:
        Direction   = '-';
        Operation   = EOS;
        break;


    case 'G':
    case 'R':
    case 'T':
    case 'S':
        Operation   = key;
        break;

    case VK_MULTIPLY:
        Operation   = '*';
        break;
    case VK_DIVIDE:
        Operation   = '/';
        break;


    default:
        Operation   = EOS;
    }
}


//----------------------------------------------------------------------------
                                        // when quitting or switching of input files
void    TCoregistrationDialog::CleanUp ( bool destroydialog )
{

if ( MriView && XyzView ) {
                                        // unplug from display
    MriView->Using. Remove ( XyzView );
    XyzView->UsedBy.Remove ( MriView );

    MriView->WindowSetPosition ( 0, 0, 800, 800 );
    }


if ( ! CartoolApplication->Closing && BaseView && BaseView->BaseDoc )    // unlock view
    BaseView->BaseDoc->AllowClosing ();

if ( ! CartoolApplication->Closing && MriView  && MriView ->BaseDoc )    // unlock view
    MriView ->BaseDoc->AllowClosing ();


                                        // force closing the electrodes file, so we can re-open it clean
if ( /*CanCloseBaseView && */ BaseView && BaseView->BaseDoc->CanClose ( true ) ) {

    CartoolDocManager->CloseDoc ( BaseView->BaseDoc );

    BaseView    = 0;

    XyzView     = 0;
    }


if ( CanCloseMriView && MriView && MriView->BaseDoc->CanClose ( true ) ) {

    MriView->BaseDoc->SetDirty ( false );

    CartoolDocManager->CloseDoc ( MriView->BaseDoc );

    MriView     = 0;
    }


UndoMatrices.Reset ( true );
RedoMatrices.Reset ( true );


 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // additional self-destruct?
if ( destroydialog ) {

    Destroy ();
                                        // needed if TCartoolMdiClient tries to test/delete dialog
    CoregistrationDlg   = 0;

    if ( CartoolMainWindow )
        CartoolMainWindow->SetFocus (); // modeless seems to lose the focus...
    }
}


//----------------------------------------------------------------------------
                                        // Always interactive for the moment
//void    TCoregistrationDialog::CmSetProcessing ()
//{
//SetProcessing ( true, false );
//}

                                        // Setting the processing type from input; opening any needed files; setting display windows; setting needed working variables
void    TCoregistrationDialog::SetProcessing ( bool computemasks, bool docleanup )
{
if ( docleanup )
    CleanUp ( false );


TransferData ( tdGetData );


 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Choose Processing type
CoregistrationType  preset          = (CoregistrationType) Presets->GetSelIndex ();
                    Processing      = CoregisterNone;


if      ( BaseView ) {              // BaseView specified will force our choice

    if      ( dynamic_cast<TElectrodesView*> ( BaseView ) )                                 Processing  = preset;
                                    // file was open, don't close when done
    if ( docleanup )
        CanCloseBaseView    = false;
    }
else {                              // no view given, guess from current entries
    if      ( IsExtensionAmong ( CoregTransfer.InputSourceFile, AllCoordinatesFilesExt ) )  Processing  = preset;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                       // Open all necessary files
if ( BaseView == 0 
  && CanOpenFile ( CoregTransfer.InputSourceFile ) 
  && Processing != CoregisterNone ) {

    bool                oldav           = CartoolApplication->AnimateViews;

    CartoolApplication->AnimateViews    = CartoolDocManager->IsOpen ( CoregTransfer.InputSourceFile );

                                        // open file & get view
    CanCloseBaseView    = ! CartoolDocManager->IsOpen ( CoregTransfer.InputSourceFile );
                                        // open doc
    TBaseDoc*   basedoc = CartoolDocManager->OpenDoc ( CoregTransfer.InputSourceFile, dtOpenOptions );
                                        // and get da view
    BaseView            = basedoc->GetViewList ();

    SetFocus ();

    CartoolApplication->AnimateViews    = oldav;
    }


if ( MriView == 0 
  && CanOpenFile ( CoregTransfer.InputTargetFile ) ) {

    bool                oldav           = CartoolApplication->AnimateViews;

    CartoolApplication->AnimateViews    = CartoolDocManager->IsOpen ( CoregTransfer.InputTargetFile );

                                        // open file & get view
    CanCloseMriView     = ! CartoolDocManager->IsOpen ( CoregTransfer.InputTargetFile );
                                        // open doc
    TBaseDoc*   basedoc = CartoolDocManager->OpenDoc ( CoregTransfer.InputTargetFile, dtOpenOptions );
                                        // and get da view
    MriView             = dynamic_cast<TVolumeView*> ( basedoc->GetViewList () );


    SetFocus ();

    CartoolApplication->AnimateViews    = oldav;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Lock documents, preventing them from closing
if ( BaseView && BaseView->BaseDoc )
    BaseView->BaseDoc->PreventClosing ();

if ( MriView  && MriView->BaseDoc )
    MriView ->BaseDoc->PreventClosing ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set       ( CoregistrationTitle " Init", BaseView ? 6 : 0, SuperGaugeLevelDefault );


if ( BaseView )

    WindowSetOrigin     (   this,
                            Gauge.GetWindowLeft () + Gauge.GetWindowWidth (),   // move dialog slightly on the right
                            GetWindowTop ( this )
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update main input file in Dialog
if ( BaseView && BaseView->BaseDoc )
    StringCopy ( CoregTransfer.InputSourceFile,  BaseView->BaseDoc->GetDocPath () );

if ( MriView  && MriView ->BaseDoc )
    StringCopy ( CoregTransfer.InputTargetFile,  MriView ->BaseDoc->GetDocPath () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some more consistency checks
if ( Processing != CoregisterNone ) {
    
    TFileName       targetdir  ( CoregTransfer.InputTargetFile );
    RemoveFilename  ( targetdir, true );

    TFileName       targetname ( CoregTransfer.InputTargetFile );
    GetFilename     ( targetname );

    TFileName       sourcename ( CoregTransfer.InputSourceFile );
    GetFilename     ( sourcename );

                                        // destination is TARGET directory
    StringCopy      ( CoregTransfer.BaseFileName, targetdir, sourcename, InfixToCoregistered, targetname );
    } // if some processing


TransferData ( tdSetData );

BaseFileName->ResetCaret;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // handy potential conversions of BaseView
XyzView     = Processing == CoregisterXyzToMriScratch
           || Processing == CoregisterXyzToMriReload    ? dynamic_cast<TElectrodesView*> ( BaseView ) : 0;

if ( ! ( XyzView && MriView ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get data boundaries
TVolumeDoc*         mridoc              = MriView->GetMRIDoc ();
TElectrodesDoc*     xyzdoc              = XyzView->GetXYZDoc ();

const TBoundingBox<double>* MriBound    = mridoc->GetBounding ();
const TBoundingBox<double>* XyzBound    = xyzdoc->GetBounding ( DisplaySpace3D );

                                        // copy the original xyz
XyzOrig         = xyzdoc->GetPoints ( DisplaySpace3D );
                                        // !private array has additional points in case it is needed for pseudo-tracks - remove them now!
XyzOrig.Resize ( xyzdoc->GetNumElectrodes () );

                                        // will be needed when applying translation actions
TranslateStep   = MriBound->GetMeanExtent () / 500;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting MRI to RAS orientation
MriCenterOrig   = mridoc->GetOrigin ();


GetNormalizationTransform   (   mridoc,         0,
                                true,           MriCenterGeom,  // compute new geometrical center
//                              false,          MriCenter,      // use actual MRI center
                                0 /*NormToMri*/,    0
                            );

                                        // avoiding as possible these costly operations
if ( computemasks ) {

    VolumeMask      = *mridoc->GetData ();

    FctParams           p;

                                        // make that a binary mask
    Gauge.Next ();
    p ( FilterParamToMaskThreshold )     = mridoc->GetBackgroundValue ();
    p ( FilterParamToMaskNewValue  )     = 1;
    p ( FilterParamToMaskCarveBack )     = true;
    VolumeMask.Filter ( FilterTypeToMask, p );

                                        // a bit of clean-up - this should be enough
    Gauge.Next ();
    p ( FilterParamDiameter )     = 1;
    VolumeMask.Filter ( FilterTypeOpen, p );

                                        // smooth out, too
    Gauge.Next ();
    p ( FilterParamDiameter )     = 10;
    p ( FilterParamNumRelax )     = 1;
    VolumeMask.Filter ( FilterTypeRelax, p );

                                        // convert mask to "gradient-like" volume
    VolumeGradient  = VolumeMask;
                                        // HEAVY smoothing
    Gauge.Next ();
    p ( FilterParamDiameter )     = 30 / NonNull ( mridoc->GetVoxelSize ().Mean () );
    VolumeGradient.Filter ( FilterTypeFastGaussian, p );


    //VolumeMask    .WriteFile ( "E:\\Data\\VolumeMask.nii",     &MriCenterOrig, &mridoc->GetVoxelSize (), &mridoc->GetRealSize () );
    //VolumeGradient.WriteFile ( "E:\\Data\\VolumeGradient.nii", &MriCenterOrig, &mridoc->GetVoxelSize (), &mridoc->GetRealSize () );

    } // computemasks
else
    Gauge.Next ( 4 );
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting xyz to RAS orientation
//if ( init ) {
                                        // Getting bounding center
    TPointDouble        XyzCenterGeom   = XyzBound->GetCenter ();

                                        // there can be some slight shifts in the centering which are not so nice, though not a big issue
    if ( XyzBound->MeanSize () > 10 )   // truncation should not happen in normalized coordinates!

        XyzCenterGeom.Truncate ();
//      XyzCenterGeom.Show ( "XYZ bounding center" );

                                        // force resetting the left-right centering, assuming it is already optimal - might depend on some preset(?)
    XyzCenterGeom[ xyzdoc->GetAxisIndex ( LeftRight ) ] = 0;
//  }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set an initial scaling
TVector3Double      InitScale;

//if ( init ) {
                                        // use an estimate
    InitScale.X     = MriBound->GetRadius ( mridoc->GetAxisIndex ( LeftRight ) ) / NonNull ( XyzBound->GetRadius ( xyzdoc->GetAxisIndex ( LeftRight ) ) );
    InitScale.Y     = MriBound->GetRadius ( mridoc->GetAxisIndex ( FrontBack ) ) / NonNull ( XyzBound->GetRadius ( xyzdoc->GetAxisIndex ( FrontBack ) ) );
//  InitScale.Z     = MriBound->GetRadius ( mridoc->GetAxisIndex ( UpDown    ) ) / NonNull ( XyzBound->GetRadius ( xyzdoc->GetAxisIndex ( UpDown    ) ) );     // too much variability on Z axis (cropped head vs full head)

                                        // use a common scale, only using the x and y scaling estimates
    InitScale.X =
    InitScale.Y =
    InitScale.Z = GeometricalMean ( InitScale.Y, InitScale.X ) * 1.06;

                                        // use a common scale for all dimensions?
//  InitScale.X =
//  InitScale.Y =
//  InitScale.Z = 0.90 * ( InitScale.X + InitScale.Y + InitScale.Z ) / 3;
//    }
//else {
//                                        // option to reload an already coregistered XYZ, so it does not need any initial scaling
//    InitScale.X     = 1;
//    InitScale.Y     = 1;
//    InitScale.Z     = 1;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // By safety, reset all these guys
Transform    .Reset ();
TransformInit.Reset ();
UndoMatrices .Reset ( true );
RedoMatrices .Reset ( true );

                                        // NOT to be done for CoregisterXyzToMriReload
if      ( Processing == CoregisterXyzToMriScratch ) {

                                        // Compositing transform: Source re-centering -> to RAS -> to MRI local orientation
    Transform.SourceToTargetOrientation = mridoc->GetStandardOrientation ( RASToLocal )
                                        * xyzdoc->GetStandardOrientation ( LocalToRAS )
                                        * TMatrix44                      ( -XyzCenterGeom );    // translation to geometrical center

                                        // Initial estimated scaling
    Transform.ScaleOnly.Scale ( InitScale, MultiplyLeft );


    Transform.RotateOnly.SetIdentity ();

                                        // Only the final adjustment to MRI
    Transform.TranslateMriOnly.Translate ( MriCenterGeom - MriCenterOrig, MultiplyLeft );
    }

else if ( Processing == CoregisterXyzToMriReload )
    
    Transform.Reset ();


TransformInit   = Transform;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

bool                guillotineok    = SetGuillotinePlane    (   mridoc, 
                                                                MriAbsToGuillotine,
                                                                0,
                                                                0 // "Guillotine Plane Search"
                                                            );

//if ( ! guillotineok )
//    MriAbsToGuillotine.SetIdentity ();

Gauge.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting windows positions & appearances
XyzView->WindowMinimize ();
                                        // set display to triangle wires
XyzView->SendMessage ( WM_COMMAND, IDB_SURFACEMODE, 0 );
XyzView->SendMessage ( WM_COMMAND, IDB_SURFACEMODE, 0 );
                                        // showing electrodes name
XyzView->SendMessage ( WM_COMMAND, IDB_SHOWELNAMES, 0 );


int                 maxsize         = GetWindowMinSide ( CartoolObjects.CartoolMainWindow->GetClientWindow () );

MriView->WindowRestore      ();
MriView->WindowSetPosition  (   0,
                                0, 
                                maxsize, 
                                maxsize
                            );

                                        // graphically plugging one into the other
MriView->Using. Append ( XyzView );
XyzView->UsedBy.Append ( MriView );
MriView->ShowNow ();

                                        // finally setting the control (this) window's position
int                 dlgleft         = GetWindowLeft     ( CartoolObjects.CartoolMainWindow->GetClientWindow () ) + maxsize;
int                 dlgtop          = GetWindowTop      ( CartoolObjects.CartoolMainWindow->GetClientWindow () ) * 2 + ( maxsize - GetWindowHeight ( this ) ) / 2;
int                 offsetleft      = IsWindowMaximized ( CartoolObjects.CartoolMainWindow ) ? 0 : GetWindowLeft ( CartoolObjects.CartoolMainWindow );
int                 offsettop       = IsWindowMaximized ( CartoolObjects.CartoolMainWindow ) ? 0 : GetWindowTop  ( CartoolObjects.CartoolMainWindow );

WindowSetOrigin             (   this,
                                offsetleft + dlgleft,
                                offsettop  + dlgtop
                            );

this->SetFocus ();

                                        // refreshing initial display
CmAction ( IDC_RESETTRANSFORM );
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmOkEnable ( TCommandEnabler &tce )
{
if (   MriView == 0
    || XyzView == 0
    || IsEmpty ( BaseFileName ) ) {

    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmOk ()
{
if (   MriView == 0
    || XyzView == 0
    || IsEmpty ( BaseFileName ) ) {

    CleanUp ( true );

    return;                             // never reached, but anyway
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check a few things before applying transform
if ( UndoMatrices.IsEmpty () ) {
                                        // nothing to undo means no transform currently in the stack
    if ( ! GetAnswerFromUser (  "It seems you have no current transform to be applied..." NewLine 
                                NewLine 
                                "Do you still wish to proceed?", 
                                CoregistrationTitle ) )
        return;
    }

else {
                                        // ask user if this is time to apply and leave, as it has happened a few times this was a false move
    if ( ! GetAnswerFromUser (  "Do you want to apply the current transform and leave?", CoregistrationTitle ) )
        return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieving parameters
TransferData ( tdGetData );


TFileName           xyztransfile;
TFileName           altxyztransfile;

if ( ! TransformElectrodes  (   CoregTransfer.InputSourceFile,
                                CoregTransfer.InputTargetFile,  // for information only

                                Transform,
                                Processing,                     // for information only

                                VolumeMask,         VolumeGradient, 
                                MriCenterOrig, 
                                MriAbsToGuillotine,
                                PostGluingInflate,

                                CoregTransfer.BaseFileName,
                                xyztransfile,
                                altxyztransfile,    CoregTransfer.SavingAltElectrodes,
                                XyzCoregToNorm                  // optional
                            ) )

    ShowMessage ( "Something went wrong while applying the coregistration transform!", CoregistrationTitle, ShowMessageWarning );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary opening any existing output files
xyztransfile   .Open ();
altxyztransfile.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CleanUp ( true );
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmCancel ()
{
CleanUp ( true );


TBaseDialog::CmCancel ();


if ( CartoolMainWindow )
    CartoolMainWindow->SetFocus ();      // modeless seems to lose the focus...
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmBrowseInputSourceFile ()
{
SetInputSourceFile ( 0 );
}


void    TCoregistrationDialog::SetInputSourceFile ( char *file )
{
static GetFileFromUser  getfile ( "Source Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CoregTransfer.InputSourceFile ) )
        return;

    TransferData ( tdSetData );
                                        // update available choices
    SetProcessing ( false, true );
    }
else {
    StringCopy ( CoregTransfer.InputSourceFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

InputSourceFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmBrowseInputTargetFile ()
{
SetInputTargetFile ( 0 );
}


void    TCoregistrationDialog::SetInputTargetFile ( char *file )
{
static GetFileFromUser  getfile ( "Target Volume File", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CoregTransfer.InputTargetFile ) )
        return;

    TransferData ( tdSetData );
                                        // update available choices
    SetProcessing ( true, true );
    }
else {
    StringCopy ( CoregTransfer.InputTargetFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

InputTargetFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmUpOneDirectory ()
{
RemoveLastDir ( CoregTransfer.BaseFileName );

BaseFileName->SetText ( CoregTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


void    TCoregistrationDialog::CmBrowseOutputFile ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( CoregTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                mrifiles        ( drop, AllMriFilesExt          );
TGoF                remainingfiles  ( drop, AllCoordinatesFilesExt " " AllMriFilesExt, 0, true );
char                buff[ 256 ];

drop.DragFinish ();

                                        // test legality to change some of the files
if ( (bool) xyzfiles && ! CanCloseBaseView && BaseView ) {

    StringCopy  ( buff, "You are already working with file:"                    NewLine
                        NewLine, 
                        BaseView->BaseDoc->GetTitle (),                         NewLine 
                        NewLine
                        "Close this Dialog, select another window (or none)"    NewLine
                        "if you want to work on another file." );

    ShowMessage ( buff, "Coregistration", ShowMessageWarning );
    return;
    }

if ( (bool) mrifiles && ! CanCloseMriView && MriView ) {

    StringCopy  ( buff, "You are already working with file:"                    NewLine
                        NewLine, 
                        MriView->BaseDoc->GetTitle (),                          NewLine 
                        NewLine
                        "Close this Dialog, select another window (or none)"    NewLine
                        "if you want to work on another file." );

    ShowMessage ( buff, "Coregistration", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) mrifiles; i++ )
    SetInputTargetFile ( mrifiles[ i ] );


for ( int i = 0; i < (int) xyzfiles; i++ )
    SetInputSourceFile ( xyzfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );


                                        // update available choices
if ( (bool) xyzfiles || (bool) mrifiles )
    SetProcessing ( (bool) mrifiles, (bool) xyzfiles || (bool) mrifiles );
}


//----------------------------------------------------------------------------
void    TCoregistrationDialog::CmUndoEnable ( TCommandEnabler &tce )
{
tce.Enable ( UndoMatrices.IsNotEmpty () );
}


void    TCoregistrationDialog::CmRedoEnable ( TCommandEnabler &tce )
{
tce.Enable ( RedoMatrices.IsNotEmpty () );
}


//----------------------------------------------------------------------------
                                        // Responding to user's button clicks
void    TCoregistrationDialog::CmAction ( owlwparam op )
{
                                        // resetting accelerarion if no input since "long enough"(?)
if ( GetTimeSinceLastPolled () > 1 /*5*/ )

    StopAccelerating ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              v               = GetSpeed ();      // value will depend on how fast / how much buttons are being clicked on
bool                revertaxis      = IsOdd ( op );     // current buttons' IDs allow this little trick


switch ( op ) {

    case    IDC_RESETTRANSFORM:

            if ( ! ( UndoMatrices.IsEmpty () && RedoMatrices.IsEmpty () )
              && ! GetAnswerFromUser ( "Are you sure you really want to erase your whole transform history?", CoregistrationTitle ) )
                return;

            Transform   = TransformInit;

            UndoMatrices.Reset ( true );
            RedoMatrices.Reset ( true );

            SetCheck ( Glue, Transform.Gluing );

            break;


    case    IDC_REDOLAST:
    case    IDC_REDOALL:
            
            while ( RedoMatrices.IsNotEmpty () ) {
                                        // recover last redo
                Transform   = *RedoMatrices.GetLast ();

                                        // update button
                SetCheck ( Glue, Transform.Gluing );

                                        // push back to last undo
                UndoMatrices.Append ( new TCoregistrationTransform ( Transform ) );


                delete  RedoMatrices.GetLast ();
                RedoMatrices.RemoveLast ();

                                        // do it only once
                if ( op == IDC_REDOLAST )
                    break;
                }

            break;

    case    IDC_UNDOLAST:
    case    IDC_UNDOALL:

            while ( UndoMatrices.IsNotEmpty () ) {
                                        // copy current state to redo
                RedoMatrices.Append ( new TCoregistrationTransform ( Transform ) );


                delete  UndoMatrices.GetLast ();
                UndoMatrices.RemoveLast ();

                                        // update with new last undo matrix
                Transform   = UndoMatrices.IsNotEmpty () ? *UndoMatrices.GetLast () : TransformInit;

                                        // update button
                SetCheck ( Glue, Transform.Gluing );

                                        // do it only once
                if ( op == IDC_UNDOLAST )
                    break;
                }

            break;

    case    IDC_SCALEGLOBALPLUS:
    case    IDC_SCALEGLOBALMINUS:
    case    IDC_SCALEXPLUS:
    case    IDC_SCALEXMINUS:
    case    IDC_SCALEYPLUS:
    case    IDC_SCALEYMINUS:
    case    IDC_SCALEZPLUS:
    case    IDC_SCALEZMINUS:

            v       = 1 + v * 0.0025;

            if ( revertaxis )
                v   = 1 / v;

            if      ( op == IDC_SCALEGLOBALPLUS || op == IDC_SCALEGLOBALMINUS   )   Transform.Scale  ( v );
            else if ( op == IDC_SCALEXPLUS      || op == IDC_SCALEXMINUS        )   Transform.ScaleX ( v );
            else if ( op == IDC_SCALEYPLUS      || op == IDC_SCALEYMINUS        )   Transform.ScaleY ( v );
            else                                                                    Transform.ScaleZ ( v );

            UndoMatrices.Append ( new TCoregistrationTransform ( Transform ) );
            RedoMatrices.Reset ( true );

            break;


    case    IDC_TRANSLATEXPLUS:
    case    IDC_TRANSLATEXMINUS:
    case    IDC_TRANSLATEYPLUS:
    case    IDC_TRANSLATEYMINUS:
    case    IDC_TRANSLATEZPLUS:
    case    IDC_TRANSLATEZMINUS:

            v      *= TranslateStep * TrueToMinus ( revertaxis );

            if      ( op == IDC_TRANSLATEXPLUS  || op == IDC_TRANSLATEXMINUS    )   Transform.TranslateX ( v );
            else if ( op == IDC_TRANSLATEYPLUS  || op == IDC_TRANSLATEYMINUS    )   Transform.TranslateY ( v );
            else                                                                    Transform.TranslateZ ( v );

            UndoMatrices.Append ( new TCoregistrationTransform ( Transform ) );
            RedoMatrices.Reset ( true );

            break;


    case    IDC_ROTATEXPLUS:
    case    IDC_ROTATEXMINUS:
    case    IDC_ROTATEYPLUS:
    case    IDC_ROTATEYMINUS:
    case    IDC_ROTATEZPLUS:
    case    IDC_ROTATEZMINUS:

            v      *= 0.25 * TrueToMinus ( revertaxis );

            if      ( op == IDC_ROTATEXPLUS     || op == IDC_ROTATEXMINUS       )   Transform.RotateX ( v );
            else if ( op == IDC_ROTATEYPLUS     || op == IDC_ROTATEYMINUS       )   Transform.RotateY ( v );
            else                                                                    Transform.RotateZ ( v );

            UndoMatrices.Append ( new TCoregistrationTransform ( Transform ) );
            RedoMatrices.Reset ( true );

            break;

    case    IDC_GLUE:
                                                                                    Transform.ToggleGlueing ();

            UndoMatrices.Append ( new TCoregistrationTransform ( Transform ) );
            RedoMatrices.Reset ( true );

            break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply compound transform from original points, instead of incremental transforms!
XyzCopy         = XyzOrig;

Transform.Apply (   XyzCopy, 
                    VolumeMask,     VolumeGradient, 
                    MriCenterOrig, 
                    MriAbsToGuillotine,
                    PostGluingInflate
                );

XyzView->GetXYZDoc ()->SetPoints ( DisplaySpace3D, XyzCopy );


MriView->ShowNow ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}

OptimizeOn
