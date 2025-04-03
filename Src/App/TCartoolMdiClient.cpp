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

#include    <typeinfo>
#include    <stdio.h>

#include    <owl/pch.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Geometry.TPoint.h"
#include    "TTracks.h"

#include    "Files.Extensions.h"
#include    "TCartoolMdiClient.h"
#include    "TCartoolMdiChild.h"

#include    "CartoolTypes.h"            // ZScoreType
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TLinkManyDoc.h"
#include    "TInverseMatrixDoc.h"       // for testing map-inverse correlations

#include    "TLeadField.h"
#include    "TVolumeDoc.h"

#include    "TBaseView.h"

#include    "TExportTracksDialog.h"
#include    "TTracksAveragingDialogs.h"
#include    "TInterpolateTracksDialog.h"
#include    "TFrequencyAnalysisDialog.h"
#include    "TCreateRoisDialog.h"
#include    "TStatisticsDialog.h"
#include    "TMicroStatesSegDialog.h"
#include    "TMicroStatesFitDialog.h"
#include    "TFileCalculatorDialog.h"
#include    "TCreateInverseMatricesDialog.h"
#include    "TComputingRisDialog.h"
#include    "TPreprocessMrisDialog.h"
#include    "TRisToVolumeDialog.h"
#include    "TCoregistrationDialog.h"
#include    "TScanTriggersDialog.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTracksAveragingStruct  TAvgTransfer;
extern  TStatStruct             StatTransfer;
extern  TMicroStatesSegStruct   SegTransfer;
extern  TMicroStatesFitStruct   FitTransfer;
extern  TCreateRoisDialog*      CreateRoisDlg;
extern  TCoregistrationDialog*  CoregistrationDlg;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1(TCartoolMdiClient, TMDIClient)

    EV_WM_DROPFILES,

    EV_COMMAND          (CM_CLOSECHILDREN,              CmCloseChildren ),
    EV_COMMAND          (CM_FILENEWEMPTYLM,             CmNewEmptyLM ),
    EV_COMMAND          (CM_FILENEWLMFROMEXISTING,      CmNewLMFromExisting ),

    EV_COMMAND          (CM_FILESCONVERSIONVRBTOTVA,    FilesConversionVrbToTvaUI ),

    EV_COMMAND_AND_ID   (CM_SPLITFREQBYFREQUENCY,       SplitFreqFilesUI ),
    EV_COMMAND_AND_ID   (CM_SPLITFREQBYELECTRODE,       SplitFreqFilesUI ),
    EV_COMMAND_AND_ID   (CM_SPLITFREQBYTIME,            SplitFreqFilesUI ),
    EV_COMMAND          (CM_MERGEFREQBYFREQUENCY,       MergeTracksToFreqFilesUI ),
//  EV_COMMAND_AND_ID   (CM_SPLITMATFILES,              SplitMatFilesUI ),

//  EV_COMMAND          (CM_MLLXE,                      CmFileMultiLinkLxe),
//  EV_COMMAND          (CM_MLLIE,                      CmFileMultiLinkLie),
//  EV_COMMAND          (CM_MLLSR,                      CmFileMultiLinkLsr),

    EV_COMMAND_AND_ID   (CM_WINSIZE_INC,                CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_INCL,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_INCR,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_INCT,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_INCB,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_DEC,                CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_DECL,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_DECR,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_DECT,               CmWinAction),
    EV_COMMAND_AND_ID   (CM_WINSIZE_DECB,               CmWinAction),

    EV_COMMAND_AND_ID   (CM_DOCWIN_MIN,                 CmDocWinAction),
    EV_COMMAND_AND_ID   (CM_DOCWIN_RESTORE,             CmDocWinAction),
//  EV_COMMAND_AND_ID   (CM_DOCWIN_CLOSE,               CmDocWinAction),
//  EV_COMMAND_AND_ID   (CM_DOCWIN_TILE,                CmDocWinAction),
    EV_COMMAND_AND_ID   (CM_ALLWIN_MIN,                 CmAllWinAction),
    EV_COMMAND_AND_ID   (CM_ALLWIN_RESTORE,             CmAllWinAction),
    EV_COMMAND_AND_ID   (CM_ALLWIN_PANTILE,             CmAllWinAction),
    EV_COMMAND_AND_ID   (CM_GTV_RESTORE,                CmGroupWinAction),
    EV_COMMAND_AND_ID   (CM_GTV_MIN,                    CmGroupWinAction),
    EV_COMMAND_AND_ID   (IDB_GTVKEEPSIZE,               CmGroupWinAction),
    EV_COMMAND_AND_ID   (IDB_GTVSTANDSIZE,              CmGroupWinAction),
    EV_COMMAND_AND_ID   (IDB_GTVFIT,                    CmGroupWinAction),
    EV_COMMAND_ENABLE   (CM_DOCWIN_MIN,                 CmDocWinActionEnable),
    EV_COMMAND_ENABLE   (CM_DOCWIN_RESTORE,             CmDocWinActionEnable),
//  EV_COMMAND_ENABLE   (CM_DOCWIN_CLOSE,               CmDocWinActionEnable),
//  EV_COMMAND_ENABLE   (CM_DOCWIN_TILE,                CmDocWinActionEnable),
    EV_COMMAND_ENABLE   (CM_ALLWIN_MIN,                 CmAllWinActionEnable),
    EV_COMMAND_ENABLE   (CM_ALLWIN_RESTORE,             CmAllWinActionEnable),
    EV_COMMAND_ENABLE   (CM_ALLWIN_PANTILE,             CmAllWinActionEnable),
    EV_COMMAND_ENABLE   (CM_GTV_RESTORE,                CmGroupWinActionEnable),
    EV_COMMAND_ENABLE   (CM_GTV_MIN,                    CmGroupWinActionEnable),
    EV_COMMAND_ENABLE   (IDB_GTVKEEPSIZE,               CmGroupWinActionEnable),
    EV_COMMAND_ENABLE   (IDB_GTVSTANDSIZE,              CmGroupWinActionEnable),
    EV_COMMAND_ENABLE   (IDB_GTVFIT,                    CmGroupWinActionEnable),

    EV_COMMAND          (CM_INTERPOLATE,                CmInterpolate ),
    EV_COMMAND          (CM_AVERAGING,                  CmInteractiveAveraging ),
    EV_COMMAND          (CM_SEGMENTEEG,                 CmSegmentEeg ),
    EV_COMMAND          (CM_FITTING,                    CmFitting),

    EV_COMMAND          (CM_EXPORTTRACKS,               CmExportTracks ),
    EV_COMMAND          (CM_FREQANALYSIS,               CmFreqAnalysis ),
    EV_COMMAND          (CM_SCANTRIGGERS,               CmScanTriggers ),

    EV_COMMAND_AND_ID   (CM_STATISTICSTRACKS,           CmStatistics ),
    EV_COMMAND_AND_ID   (CM_STATISTICSRIS,              CmStatistics ),
    EV_COMMAND_AND_ID   (CM_STATISTICSFREQ,             CmStatistics ),

    EV_COMMAND          (CM_AVERAGEXYZ,                 BuildTemplateElectrodesUI ),
    EV_COMMAND          (CM_DOWNSAMPLINGELECTRODES,     DownsamplingElectrodesUI ),
    EV_COMMAND          (CM_FILENEWXYZ,                 ExtractElectrodesFromKriosUI ),

    EV_COMMAND          (CM_CREATEROIS,                 CmCreateRois ),
    EV_COMMAND          (CM_FILECALCULATOR,             CmFileCalculator ),
    EV_COMMAND          (CM_CREATEINVERSESOLUTIONS,     CmCreateInverseSolutions ),
    EV_COMMAND          (CM_COREGISTRATION,             CmCoregistration ),
    EV_COMMAND          (CM_MRICOREGISTRATION,          CoregistrationMrisUI ),

    EV_COMMAND_AND_ID   (CM_SKULLSTRIPPING,             BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_CUTBRAINSTEM,               BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_FILTERBIASFIELD,            BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_FILTERSEGMENTCSF,           BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_FILTERSEGMENTGREY,          BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_FILTERSEGMENTWHITE,         BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_FILTERSEGMENTTISSUES,       BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_DOWNSAMPLEMRI,              BatchProcessMrisUI ),
    EV_COMMAND_AND_ID   (CM_HEADCLEANUP,                BatchProcessMrisUI ),
    EV_COMMAND          (CM_NORMALIZEMRI,               CmPreprocessMris ),
    EV_COMMAND          (CM_TEMPLATEMRI,                ComputingTemplateMriUI ),
    EV_COMMAND          (CM_MERGINGMRIMASKS,            MergingMriMasksUI ),
    EV_COMMAND          (CM_FILTERMASKTOSP,             BrainToSolutionPointsUI ),

    EV_COMMAND          (CM_CORRELATEFILES,             CorrelateFilesUI ),
    EV_COMMAND_AND_ID   (CM_COMPUTECENTROIDEEG,         ComputeCentroidFilesUI ),
    EV_COMMAND_AND_ID   (CM_COMPUTECENTROIDRIS,         ComputeCentroidFilesUI ),

    EV_COMMAND_AND_ID   (CM_BATCHAVERAGEEEG,            BatchAveragingFilesUI ),
    EV_COMMAND_AND_ID   (CM_BATCHAVERAGEERRORDATA,      BatchAveragingFilesUI ),
    EV_COMMAND_AND_ID   (CM_BATCHAVERAGERIS,            BatchAveragingFilesUI ),
    EV_COMMAND_AND_ID   (CM_BATCHAVERAGEFREQ,           BatchAveragingFilesUI ),

    EV_COMMAND          (CM_COMPUTINGRIS,               CmComputeRis ),
    EV_COMMAND          (CM_RISTOVOLUME,                CmRisToVolume ),
    EV_COMMAND          (CM_RISTOPOINTS,                RisToCloudVectorsUI ),

    EV_COMMAND_AND_ID   (CM_GENERATEDATAEEG,            GenerateDataUI),
    EV_COMMAND_AND_ID   (CM_GENERATEDATARIS,            GenerateDataUI),
    EV_COMMAND          (CM_GENERATERANDOMDATA,         GenerateRandomDataUI),
    EV_COMMAND_AND_ID   (CM_ANALYZEGENERATEDATAEEG,     AnalyzeGeneratedDataUI),
    EV_COMMAND_AND_ID   (CM_ANALYZEGENERATEDATARIS,     AnalyzeGeneratedDataUI),
    EV_COMMAND          (CM_ANALYZEGENERATEDATAALTSRC,  GenerateOscillatingDataUI),

    EV_COMMAND_AND_ID   ( CM_PCA,                       PCA_ICA_UI ),
//  EV_COMMAND_AND_ID   ( CM_ICA,                       PCA_ICA_UI ), // off, as it does not work properly

    EV_COMMAND          (CM_TOOLSHELP,                  CmToolsHelp ),

END_RESPONSE_TABLE;


//----------------------------------------------------------------------------
        TCartoolMdiClient::~TCartoolMdiClient ()
{

BeforeClosing ();

                                        // Any modeless dialog needs to be destroyed?
if ( CreateRoisDlg && ! CartoolApplication->Closing ) {
    delete  CreateRoisDlg;
    CreateRoisDlg   = 0;
    }

if ( CoregistrationDlg && ! CartoolApplication->Closing ) {
    delete  CoregistrationDlg;
    CoregistrationDlg   = 0;
    }


//Destroy ();
}


void    TCartoolMdiClient::SetupWindow ()
{
TMDIClient::SetupWindow();

DragAcceptFiles ( true );
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::RefreshWindows ()
{
int                 nw              = NumChildren ();


for ( TWindow* tow = GetFirstChild (); tow != 0 && nw != 0; tow = tow->Next (), nw-- ) {

    TCartoolMdiChild*   tomdih      = dynamic_cast<TCartoolMdiChild*> ( tow );

    if ( tomdih && ! IsWindowMinimized ( tomdih ) )

//      tomdih->GetClientWindow ()->Invalidate ();
        tomdih->GetClientWindow ()->RedrawWindow ( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN );
    }
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::EvDropFiles ( TDropInfo )
{
Parent->ForwardMessage ();  // to the application
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::BeforeClosing ()
{
try {
                                        // close all lm files first
    bool            foundlm;

    do {
        foundlm         = false;
                                        // repeat as many times as there are lm docs open
                                        // it can still complain when there are files used in multiple lm, though
                                        // each time we close a lm, it also closes many other docs, so the whole list is not up-to-date anymore, hence restarting
        for ( TBaseDoc* doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )

            if ( StringIs ( doc->GetTemplate()->GetDefaultExt(), FILEEXT_LM ) ) {

                foundlm     = true;

                CartoolDocManager->CloseDoc ( doc );

                UpdateApplication;

                break;
                }
        } while ( foundlm );
    }

catch ( ... ) {}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

try {
                                        // EP and EPH files can make use of SD files, so close these EP files first
    bool            foundep;

    do {
        foundep         = false;

        for ( TBaseDoc *doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )

            if ( StringIs ( doc->GetTemplate()->GetDefaultExt(), FILEEXT_EEGEPH )
              || StringIs ( doc->GetTemplate()->GetDefaultExt(), FILEEXT_EEGEP  ) ) {

                foundep     = true;

                CartoolDocManager->CloseDoc ( doc );

                UpdateApplication;

                break;
                }
        } while ( foundep );
    }

catch ( ... ) {}
}


void    TCartoolMdiClient::CmCloseChildren ()
{
BeforeClosing ();

TMDIClient::CmCloseChildren ();
}


bool    TCartoolMdiClient::CanClose ()
{
return  TMDIClient::CanClose ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmDocWinActionEnable ( TCommandEnabler &tce )
{
tce.Enable ( CartoolDocManager->GetCurrentDoc () != 0 );      // at least something to manage
}


void    TCartoolMdiClient::CmAllWinActionEnable ( TCommandEnabler &tce )
{
tce.Enable ( CartoolDocManager->DocList.Next ( 0 ) != 0 );    // at least something to manage
}


void    TCartoolMdiClient::CmGroupWinActionEnable ( TCommandEnabler &tce )
{
const TDocument*    currentdoc      = CartoolDocManager->GetCurrentDoc ();

if ( currentdoc == 0 ) {

    tce.Enable ( false );
    return;
    }


TBaseView*          currentview     = CartoolDocManager->GetCurrentView ();
TLinkManyDoc*       currentGOD      = currentview ? currentview->GODoc : 0;

tce.Enable ( currentGOD != 0 );
}


//----------------------------------------------------------------------------
                                        // Converts the rect to client coordinates
void    TCartoolMdiClient::ScreenToClient ( TRect& r )
{
                                        // done in 2 steps with the official TWindow::ScreenToClient, which works only on a TPoint&
TWindow::ScreenToClient ( *reinterpret_cast<TPoint*> ( &r.left  ) );    // (left,  top   )
TWindow::ScreenToClient ( *reinterpret_cast<TPoint*> ( &r.right ) );    // (right, bottom)
}

//----------------------------------------------------------------------------
                                        // Current, single window actions
void    TCartoolMdiClient::CmWinAction ( owlwparam w )
{
TBaseView*          view            = CartoolDocManager->GetCurrentView ();

if ( ! view )
    return;


TRect               r               = view->GetParentO ()->GetWindowRect ().Normalized ();

ScreenToClient ( r );

                                        
double              scale           = w == CM_WINSIZE_DEC 
                                   || w == CM_WINSIZE_DECL 
                                   || w == CM_WINSIZE_DECR 
                                   || w == CM_WINSIZE_DECT 
                                   || w == CM_WINSIZE_DECB  ? PowerRoot ( 2, -4 )   // done 4 times, factor will halve..
                                                            : PowerRoot ( 2,  4 );  // ..or double the original window size

                                        // If left-right keys are pressed, only apply on X dimension, and conversely for up-down keys for Y dimension
int                 width           = w == CM_WINSIZE_INCT 
                                   || w == CM_WINSIZE_INCB 
                                   || w == CM_WINSIZE_DECT 
                                   || w == CM_WINSIZE_DECB ?         r.Width  () 
                                                           : Round ( r.Width  () * scale );
int                 height          = w == CM_WINSIZE_INCL 
                                   || w == CM_WINSIZE_INCR 
                                   || w == CM_WINSIZE_DECL 
                                   || w == CM_WINSIZE_DECR ?         r.Height () 
                                                           : Round ( r.Height () * scale );

                                        // Also pressing left or right will modify the size on the left or the right, and conversely for the up and down
int                 top             = w == CM_WINSIZE_INCT 
                                   || w == CM_WINSIZE_DECT ? r.Top  () - ( height - r.Height () ) 
                                                           : r.Top  ();
int                 left            = w == CM_WINSIZE_INCL 
                                   || w == CM_WINSIZE_DECL ? r.Left () - ( width  - r.Width  () ) 
                                                           : r.Left ();


view->WindowSetPosition ( left, top, width, height );


return;
}


//----------------------------------------------------------------------------
                                        // All windows actions from a given document
void    TCartoolMdiClient::CmDocWinAction ( owlwparam w )
{
TBaseDoc*           currentdoc      = CartoolDocManager->GetCurrentBaseDoc ();

if ( currentdoc == 0 )
    return;


if      ( w == CM_DOCWIN_CLOSE ) {

    if ( currentdoc->CanClose () )
		CartoolDocManager->CloseDoc ( currentdoc );
    }

else if ( w == CM_DOCWIN_TILE ) {

    TPoint          nl ( 0, 0 );
    TPoint          nc ( 0, 0 );

    for ( TBaseView* view = currentdoc->GetViewList (); view != 0; view = currentdoc->NextView ( view ) ) {

        TWindow*    tow     = view->GetParentO ();

        if ( IsWindowMinimized ( tow ) )    continue;


        if ( nl.Y () + GetWindowHeight ( tow ) <= GetWindowHeight ( this ) ) {

            WindowSetFrameOrigin ( tow, nl.X(), nl.Y() );

            nl += TSize ( 0, GetWindowHeight ( tow ) );

            if ( GetWindowRight ( tow ) > nc.X() )
                nc  = TPoint ( GetWindowRight ( tow ) + 1, nc.Y () );
            }
        else {
            nl  = nc;
            nc += TSize ( GetWindowWidth ( tow ), 0 );

            WindowSetFrameOrigin ( tow, nl.X(), nl.Y() );

            nl += TSize ( 0, GetWindowHeight ( tow ) );
            }
        } // for view
    }

else { // minimize / restore

    TBaseView*          currentview     = CartoolDocManager->GetCurrentView ();
    TLinkManyDoc*       currentGOD      = currentview ? currentview->GODoc : 0;

    for ( TBaseView* view = currentdoc->GetViewList (); view != 0; view = currentdoc->NextView ( view ) ) {

        if ( view->GODoc != currentGOD ) continue;

        if      ( w == CM_DOCWIN_MIN     )  view->WindowMinimize ();
        else if ( w == CM_DOCWIN_RESTORE )  view->WindowRestore  ();
        }
    }
}


//----------------------------------------------------------------------------
                                        // All windows actions
void    TCartoolMdiClient::CmAllWinAction ( owlwparam w )
{
TPoint              nl ( 0, 0 );
TPoint              nc ( 0, 0 );


for ( TBaseDoc*  doc  = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
for ( TBaseView* view = doc->GetViewList (); view != 0; view = doc->NextView ( view ) ) {

    if      ( w == CM_ALLWIN_MIN )

        view->WindowMinimize ();

    else if ( w == CM_ALLWIN_RESTORE )

        view->WindowRestore ();

    else if ( w == CM_ALLWIN_PANTILE ) {

        TWindow*    tow     = view->GetParentO ();

        if ( IsWindowMinimized ( tow ) )    continue;


        if ( nl.Y () + GetWindowHeight ( tow ) <= GetWindowHeight ( this ) ) {

            WindowSetFrameOrigin ( tow, nl.X(), nl.Y() );

            nl += TSize ( 0, GetWindowHeight ( tow ) );

            if ( GetWindowRight ( tow ) > nc.X() )
                nc  = TPoint ( GetWindowRight ( tow ) + 1, nc.Y() );
            }
        else {
            nl  = nc;
            nc += TSize ( GetWindowWidth ( tow ), 0 );

            WindowSetFrameOrigin ( tow, nl.X(), nl.Y() );

            nl += TSize ( 0, GetWindowHeight ( tow ) );
            }
        }

    else if ( w == CM_ALLWIN_RELOADGADGETS ) {

        view->DestroyGadgets ();

        view->CreateGadgets  ();
        }
    } // for doc, view
}


//----------------------------------------------------------------------------
                                        // All windows actions from the same TLinkManyDoc group
void    TCartoolMdiClient::CmGroupWinAction ( owlwparam w )
{
TBaseView*          currentview     = CartoolDocManager->GetCurrentView ();
TLinkManyDoc*       currentGOD      = currentview ? currentview->GODoc : 0;

if ( currentview == 0 || currentGOD == 0 )
    return;


if      ( w == IDB_GTVKEEPSIZE  )           currentview->GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_Move     ,   GroupTilingViews_Insert ) );
else if ( w == IDB_GTVSTANDSIZE )           currentview->GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_StandSize,   GroupTilingViews_Insert ) );
else if ( w == IDB_GTVFIT       )           currentview->GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_BestFitSize, GroupTilingViews_Insert ) );

else {
    for ( TBaseDoc*  doc  = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( currentGOD ); view != 0; view = doc->NextView ( view, currentGOD ) ) {

        if      ( w == CM_GTV_MIN     )     view->WindowMinimize ();
        else if ( w == CM_GTV_RESTORE )     view->WindowRestore  ();
        }
    }
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmNewEmptyLM ()
{
TDocTemplate*       lmtpl           = CartoolDocManager->MatchTemplate ( FILEFILTER_LM );

TFileName           file;
                                        // Always using the same empty file name...
StringCopy ( file, EmptyLmFilename );

if ( CanOpenFile ( file ) )
    DeleteFileExtended ( file );

CartoolDocManager->CreateDoc ( lmtpl, file );
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmNewLMFromExisting ()
{
TDocTemplate*       lmtpl           = CartoolDocManager->MatchTemplate ( FILEFILTER_LM );

TFileName           file;
                                        // Always using the same empty file name...
StringCopy ( file, AllOpenedLmFilename );

if ( CanOpenFile ( file ) )
    DeleteFileExtended ( file );

TLinkManyDoc*       lmdoc           = dynamic_cast<TLinkManyDoc *> ( CartoolDocManager->CreateDoc ( lmtpl, file ) );

if ( lmdoc == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case for Grey MRI - insert on the top
for ( TBaseDoc* doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) ) {

    TVolumeDoc*         mridoc      = dynamic_cast<TVolumeDoc*> ( doc );

    if ( mridoc 
      && mridoc->IsBinaryMask ()
      && lmdoc->AddToGroup ( mridoc, false ) ) {
                                        // hide these guys
        mridoc->MinimizeViews ();
        lmdoc->RefreshWindows ( false );
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case for Brain MRI - insert second to top
for ( TBaseDoc* doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) ) {

    TVolumeDoc*         mridoc      = dynamic_cast<TVolumeDoc*> ( doc );

    if ( mridoc 
      && mridoc->IsSegmented () && ! mridoc->IsMask ()
      && lmdoc->AddToGroup ( mridoc, false ) ) {
                                        // hide these guys
        mridoc->MinimizeViews ();
        lmdoc->RefreshWindows ( false );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now loop into existing docs and forward them to the new .lm file
                                        // the one files already linked will be gracefully skipped
for ( TBaseDoc* doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) ) {
                                        // don't include itself!
    if ( doc == lmdoc )     continue;

    if ( lmdoc->AddToGroup ( doc, false )
      && IsExtensionAmong ( doc->GetDocPath (),     AllCoordinatesFilesExt 
                                                " " AllSolPointsFilesExt 
                                                " " AllMriFilesExt 
                                                " " AllInverseFilesExt 
                                                " " AllRoisFilesExt         ) ) {
                                        // hide these guys
        doc  ->MinimizeViews ();

        lmdoc->RefreshWindows ( false );
        }

    if ( IsExtensionAmong ( doc->GetDocPath (), AllLmFilesExt ) )
                                        // minimize already opened .lm files, which will interfere with the forthcoming retiling
        doc  ->MinimizeViews ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

lmdoc->RefreshWindows ( false );

lmdoc->NotifyDocViews ( vnViewUpdated );

lmdoc->SyncUtility ( CM_SYNCALL );

CmGroupWinAction ( IDB_GTVFIT );
}


//----------------------------------------------------------------------------
                                        // Called from menu  Tools | Help
void    TCartoolMdiClient::CmToolsHelp ()
{
TFileName           helpaddress     = TFileName ( WebPageCartoolHelp ) + "/all-processings.html";

ShellExecute    ( NULL, "open", helpaddress, NULL, NULL, SW_SHOWNORMAL );
}


//----------------------------------------------------------------------------
                                        // Menu interface handlers
//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmFileCalculator ()
{
TFileCalculatorDialog ( this, IDD_CALCULATOR ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmComputeRis ()
{
TComputingRisDialog ( this, IDD_RISCOMPUTATION ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmCoregistration ()
{
if ( CoregistrationDlg )
    delete  CoregistrationDlg;

                                        // init the dialog modeless
CoregistrationDlg   = new TCoregistrationDialog ( CartoolMdiClient, IDD_COREGISTRATION );
                                        // run it modeless, so that we can switch focus to another window to tailor its view
CoregistrationDlg->Create ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmCreateInverseSolutions ()
{
TCreateInverseMatricesDialog ( this, IDD_INVERSEMATRICES ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmCreateRois ()
{
if ( CreateRoisDlg )
    delete  CreateRoisDlg;

                                        // init the dialog modeless
CreateRoisDlg   = new TCreateRoisDialog ( CartoolMdiClient, IDD_CREATEROIS );
                                        // run it modeless, so that we can switch focus to another window for picking stuff
CreateRoisDlg->Create ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmExportTracks ()
{
TExportTracksDialog ( this, IDD_EXPORTTRACKS, 0 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmFreqAnalysis ()
{
TFrequencyAnalysisDialog ( this, IDD_FREQANALYSIS, 0 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmInteractiveAveraging ()
{
                                        // For this processing, we prefer not to be disturbed with any other windows
CmCloseChildren ();


if      ( TAvgTransfer.LastDialogId == IDD_AVERAGE1 )   TTracksAveragingFilesDialog     ( this, IDD_AVERAGE1 ).Execute ();
else if ( TAvgTransfer.LastDialogId == IDD_AVERAGE2 )   TTracksAveragingParamsDialog    ( this, IDD_AVERAGE2 ).Execute ();
else                                                    TTracksAveragingOutputsDialog   ( this, IDD_AVERAGE3 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmInterpolate ()
{
TInterpolateTracksDialog ( this, IDD_INTERPOLATE, 0 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmPreprocessMris ()
{
TPreprocessMrisDialog ( this, IDD_MRIPREPROCESSING ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmRisToVolume ()
{
TRisToVolumeDialog ( this, IDD_RISTOVOLUME, 0 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmScanTriggers ()
{
TScanTriggersDialog ( this, IDD_SCANMARKERS, 0 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmStatistics ( owlwparam w )
{
//CmCloseChildren();

if      ( StatTransfer.LastDialogId == IDD_STATISTICS1 )   TStatisticsFilesDialog  ( this, IDD_STATISTICS1, w ).Execute ();
else                                                       TStatisticsParamsDialog ( this, IDD_STATISTICS2    ).Execute ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmSegmentEeg ()
{
//CmCloseChildren();

if      ( SegTransfer.LastDialogId == IDD_SEGMENT1 )    TMicroStatesSegFilesDialog  ( this, IDD_SEGMENT1 ).Execute ();
else                                                    TMicroStatesSegParamsDialog ( this, IDD_SEGMENT2 ).Execute ();
}


//----------------------------------------------------------------------------
void    TCartoolMdiClient::CmFitting ()
{
                                        // calling from a .seg file?
TTracksDoc*         currenttracksdoc    = dynamic_cast<TTracksDoc*> ( CartoolDocManager->GetCurrentBaseDoc () );

                                        // then go through a few hoops for user's convenience...
if ( currenttracksdoc && currenttracksdoc->IsContentType ( ContentTypeSeg ) ) {

    TFileName           filetempl;

    StringCopy    ( filetempl, currenttracksdoc->GetDocPath () );
                                        // file template could be either this:
    StringReplace ( filetempl, "." FILEEXT_SEG, "." FILEEXT_EEGEP );

    if ( ! GetFirstFile ( filetempl ) ) {
                                        // ..or this:
        ReplaceExtension ( filetempl, FILEEXT_RIS );

        if ( ! GetFirstFile ( filetempl ) )
            ClearString ( filetempl );  // nothing found
        }


    if ( filetempl.IsNotEmpty () ) {
                                        // we can finally set the Fitting dialog with the current template file
        TFileName           basefilename;

        StringCopy ( FitTransfer.TemplateFileName, filetempl, EditSizeText - 1 );
                                        // cook the base file name
        FitTransfer.ComposeBaseFilename ( basefilename );
                                        // then copy it to the dialog
        StringCopy ( FitTransfer.BaseFileName, basefilename, EditSizeText - 1 );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( FitTransfer.LastDialogId == IDD_FITTING1 )    TMicroStatesFitFilesDialog  ( this, IDD_FITTING1 ).Execute ();
else if ( FitTransfer.LastDialogId == IDD_FITTING2 )    TMicroStatesFitParamsDialog ( this, IDD_FITTING2 ).Execute ();
else                                                    TMicroStatesFitResultsDialog( this, IDD_FITTING3 ).Execute ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*                                      // Boilerplate for when conversion will be actually implemented
void    TCartoolMdiClient::SplitMatFilesUI ( owlwparam )
{
static GetFileFromUser  getfiles ( "Opening Matlab .mat file:", AllMatlabFilesFilter, 1, GetFileMulti );

if ( ! getfiles.Execute () )
    return;


TSuperGauge         Gauge ( "Splitting Matlab files", (int) getfiles + 1 );

for ( int i = 0; i < (int) getfiles; i++ ) {

    Gauge.Next ();

    SplitMatFiles ( getfiles[ i ] );
    }

Gauge.HappyEnd ();
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
