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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TSecondaryView.h"

#include    "TLinkManyDoc.h"

#include    "TTracksView.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TSecondaryView, TBaseView)

//    EV_WM_GETMINMAXINFO,
//    EV_WM_SIZE,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_LBUTTONDOWN,
    EV_WM_PALETTECHANGED,
    EV_VN_VIEWSYNC,

    EV_COMMAND          ( IDB_SYNCVIEWS,        CmSyncViews ),

    EV_COMMAND_AND_ID   ( IDB_RANGEAVE,         CmSetManageRangeCursor ),
    EV_COMMAND_AND_ID   ( IDB_RANGESEQ,         CmSetManageRangeCursor ),
    EV_COMMAND_AND_ID   ( IDB_RANGEANI,         CmSetManageRangeCursor ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPINC,       CmSetStepTF ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPDEC,       CmSetStepTF ),
    EV_COMMAND          ( CM_SPUSERSCALE,       CmSpecifyScaling ),

//  EV_COMMAND          ( CM_SHOWINFOS,         CmShowInfos ),
//  EV_COMMAND          ( CM_SHOWCOLORSCALE,    CmShowColorScale ),
//  EV_COMMAND          ( CM_SHOWAXIS,          CmShowAxis ),
//  EV_COMMAND          ( CM_SHOWORIENTATION,   CmShowOrientation ),
//  EV_COMMAND_AND_ID   ( CM_SHOWALL,           CmShowAll ),
//  EV_COMMAND_AND_ID   ( CM_HIDEALL,           CmShowAll ),


//  EV_COMMAND          ( IDB_EXTTRACKSV,   CmToEeg ),      // not used anymore(?)
//  EV_COMMAND          ( IDB_COMPTRACKSV,  CmToEeg ),
//  EV_COMMAND          ( IDB_EXTTRACKSH,   CmToEeg ),
//  EV_COMMAND          ( IDB_COMPTRACKSH,  CmToEeg ),
//  EV_COMMAND          ( IDB_FORWARD,      CmToEeg ),
//  EV_COMMAND          ( IDB_BACKWARD,     CmToEeg ),
//  EV_COMMAND          ( IDB_FASTFORWARD,  CmToEeg ),
//  EV_COMMAND          ( IDB_FASTBACKWARD, CmToEeg ),
//  EV_COMMAND          ( IDB_PAGEFORWARD,  CmToEeg ),
//  EV_COMMAND          ( IDB_PAGEBACKWARD, CmToEeg ),
//  EV_COMMAND          ( IDB_LESSTRACKS,   CmToEeg ),
//  EV_COMMAND          ( IDB_MORETRACKS,   CmToEeg ),
//  EV_COMMAND          ( IDB_LESSPSEUDOTRACKS,     CmToEeg ),
//  EV_COMMAND          ( IDB_MOREPSEUDOTRACKS,     CmToEeg ),
//  EV_COMMAND          ( IDB_UP,           CmToEeg ),
//  EV_COMMAND          ( IDB_DOWN,         CmToEeg ),
//  EV_COMMAND          ( IDB_TRACKSSUPER,  CmToEeg ),
//  EV_COMMAND          ( IDB_VERTUNITS,    CmToEeg ),
//  EV_COMMAND          ( IDB_HORIZUNITS,   CmToEeg ),
//  EV_COMMAND          ( IDB_RANGECURSOR,  CmToEeg ),
//  EV_COMMAND          ( IDB_SHOWMARKERS,  CmToEeg ),
//  EV_COMMAND          ( IDB_PREVMARKER,   CmToEeg ),
//  EV_COMMAND          ( IDB_NEXTMARKER,   CmToEeg ),
//  EV_COMMAND_ENABLE   ( IDB_SHOWMARKERS,  CmToEegEnable ),
//  EV_COMMAND_ENABLE   ( IDB_PREVMARKER,   CmToEegEnable ),
//  EV_COMMAND_ENABLE   ( IDB_NEXTMARKER,   CmToEegEnable ),
//  EV_COMMAND          ( IDB_STYLEWINDOW,  CmToEeg ),
//  EV_COMMAND          ( IDB_FILTER,       CmToEeg ),
//  EV_COMMAND_ENABLE   ( IDB_FILTER,       CmToEegEnable ),
//  EV_COMMAND          ( IDB_SHOWSD,       CmToEeg ),
//  EV_COMMAND_ENABLE   ( IDB_SHOWSD,       CmToEegEnable ),
//  EV_COMMAND          ( IDB_ADDMARKER,    CmToEeg ),
//  EV_COMMAND          ( IDB_SPLITWINDOW,  CmToEeg ),
//  EV_COMMAND          ( IDB_2EEGS,        CmToEeg ),
//  EV_COMMAND          ( IDB_SYNCZERO,     CmToEeg ),
//  EV_COMMAND          ( IDB_FLIPVERT,     CmToEeg ),
//  EV_COMMAND          ( IDB_FILLING,      CmToEeg ),

END_RESPONSE_TABLE;


        TSecondaryView::TSecondaryView ( TTracksDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TBaseView ( doc, parent, group ), EEGDoc ( &doc )
{
LinkedViewId            = GODoc ? GODoc->LastEegViewId : 0;  // messages sent/received to

TFCursor                = TTFCursor ( EEGDoc, 0, EEGDoc->GetNumTimeFrames () - 1 );
TFCursor.SentTo         = LinkedViewId;
TFCursor.SentFrom       = GetViewId();

SavedTFCursor           = TTFCursor ( EEGDoc, 0, EEGDoc->GetNumTimeFrames () - 1 );
SavedTFCursor.SentTo    = LinkedViewId;
SavedTFCursor.SentFrom  = GetViewId();

EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this ); // set the eeg view to this cursor


ManageRangeCursor   = MRCAverage;
MRCNumTF            = 1;
MRCStepTF           = MRCStepInit;
MRCSequenceOrder    = true;
MRCDoAnimation      = false;

ShowSequenceLabels  = true;
}


//----------------------------------------------------------------------------
void    TSecondaryView::CreateBaseGadgets ()
{
                                        // buttons common to all views
TBaseView::CreateBaseGadgets ();
                                        // then buttons common to all secondary views
ControlBarGadgets[ BaseSecondViewButtonSeparatorA       ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ BaseSecondViewButtonSyncViews        ]   = new TButtonGadgetDpi ( IDB_SYNCVIEWS,     IDB_SYNCVIEWS,      TButtonGadget::Command);

ControlBarGadgets[ BaseSecondViewButtonSeparatorB       ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ BaseSecondViewButtonRangeAverage     ]   = new TButtonGadgetDpi ( IDB_RANGEAVE,      IDB_RANGEAVE,       TButtonGadget::Exclusive,     false,  ManageRangeCursor == MRCAverage ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ BaseSecondViewButtonRangeSequence    ]   = new TButtonGadgetDpi ( IDB_RANGESEQ,      IDB_RANGESEQ,       TButtonGadget::Exclusive,     false,  IsMRCSequence () ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ BaseSecondViewButtonRangeAnimation   ]   = new TButtonGadgetDpi ( IDB_RANGEANI,      IDB_RANGEANI,       TButtonGadget::Exclusive,     false,  ManageRangeCursor == MRCAnimation ? TButtonGadget::Down : TButtonGadget::Up );

ControlBarGadgets[ BaseSecondViewButtonSeparatorC       ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ BaseSecondViewButtonRangeStepInc     ]   = new TButtonGadgetDpi ( IDB_RGESTEPINC,    IDB_RGESTEPINC,     TButtonGadget::Command);
ControlBarGadgets[ BaseSecondViewButtonRangeStepDec     ]   = new TButtonGadgetDpi ( IDB_RGESTEPDEC,    IDB_RGESTEPDEC,     TButtonGadget::Command);
}


//----------------------------------------------------------------------------
                                        // Maybe put into overloaded function SetDocTitle
void    TSecondaryView::UpdateCaption ()
{
GetEegView ()->CursorToTitle ( &TFCursor, Title );

//GetEegView ()->UpdateCaption ();
//StringCopy ( Title, GetEegView ()->GetTitle () ); // the easiest, but there is a lattency with the actual cursor...

GetParentO()->SetCaption ( Title );
}


//----------------------------------------------------------------------------
TTracksView  *TSecondaryView::GetEegView ()
{
return  dynamic_cast<TTracksView *> ( CartoolDocManager->GetView ( LinkedViewId ) );
}


//----------------------------------------------------------------------------
void    TSecondaryView::EvKeyDown (uint key, uint repeatCount, uint flags)
{
switch ( key ) {

    case 'A':
        ButtonGadgetSetState ( IDB_RANGEANI, true );
        CmSetManageRangeCursor ( IDB_RANGEANI );
        break;

    case 'S':
        ButtonGadgetSetState ( IDB_RANGESEQ, true );
        CmSetManageRangeCursor ( IDB_RANGESEQ );
        break;

    case 'V':
        ButtonGadgetSetState ( IDB_RANGEAVE, true );
        CmSetManageRangeCursor ( IDB_RANGEAVE );
        break;

    case VK_ADD:
        if ( ControlKey )
            CmSetStepTF ( IDB_RGESTEPDEC );
        break;

    case VK_SUBTRACT:
        if ( ControlKey )
            CmSetStepTF ( IDB_RGESTEPINC );
        break;

    case VK_ESCAPE:
        if ( CaptureMode == CaptureTFSync )
            CmSyncViews ();
        else
            TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;

                                        // always forward these messages
    case VK_CONTROL:
    case VK_SHIFT  :

        if ( LinkedViewId )               // always forward to EEG view (like for Shift-Tab)
            dynamic_cast<TTracksView *> ( CartoolDocManager->GetView ( LinkedViewId ) )->EvKeyDown ( key, repeatCount, flags );
        break;


    default:
        if ( LinkedViewId                 // forward only relevant messages (navigation)
          && ( key == VK_LEFT || key == VK_RIGHT 
            || key == VK_NEXT || key == VK_PRIOR
            || key == VK_HOME || key == VK_END
            || key == VK_TAB  || key == VK_SPACE
            || key == VK_F3   /*|| key == 'F' && ControlKey*/ ) )

            dynamic_cast<TTracksView *> ( CartoolDocManager->GetView ( LinkedViewId ) )->EvKeyDown ( key, repeatCount, flags );
        break;
    }
}


//----------------------------------------------------------------------------
void    TSecondaryView::EvSetFocus ( HWND hwnd )
{
                                        // in case of doc closing with multiple views, prevent all the other views from inheriting the focus, too
if ( ! EEGDoc->IsOpen () )
    return;


TBaseView::EvSetFocus ( hwnd );


if ( GODoc && LinkedViewId )
    GODoc->LastEegViewId    = LinkedViewId;   // update with MY eeg view


//EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this ); // set the eeg view to this cursor

                                        // tell everybody, to refresh positions
EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

                                        // don't know why the EEG view does not refresh?
//GetEegView ()->Invalidate ( false );
GetEegView ()->ShowNow ();


UpdateCaption ();
}


//----------------------------------------------------------------------------
void    TSecondaryView::EvLButtonDown ( uint, const TPoint& p )
{
if ( CaptureMode == CaptureTFSync ) {

    TBaseView*      view    = ClientToBaseView ( p );

    if ( view == 0 ) {

        CmSyncViews ();                 // clean-up

        return;
        }

    if ( EEGDoc->CanSync ( view ) ) {

        if ( view->GetViewId () != GetViewId () ) {
                                        // make these 2 friends
            SetFriendView ( view );

                                        // signal all friends to store the current time as their new time offset - ?not sure if this is still useful?
//          EEGDoc->NotifyFriendViews ( TFCursor.SentTo, vnViewSync, (TParam2) &TFCursor );
                                        // force my tracks view to copy my cursor
            EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );
                                        // syncing all friends on each click
            TFCursor.SentTo = FriendshipId;
            EEGDoc->NotifyFriendViews ( TFCursor.SentTo, vnNewTFCursor, (TParam2) &TFCursor, this );
            }
        else {
            CancelFriendship ();

            CmSyncViews ();
            }
        }
    else
        CmSyncViews ();
    }
}


//----------------------------------------------------------------------------
void    TSecondaryView::CmSetStepTF ( owlwparam w )
{
//int     oldStepTF   = MRCStepTF;

if ( w == IDB_RGESTEPINC ) {

    MRCStepTF  *= 2;

//  Mined ( MRCStepTF, MRCNumTF );
    }
else {
    MRCStepTF  /= 2;

    Maxed ( MRCStepTF, 1 );
    }

//if ( oldStepTF != MRCStepTF && ManageRangeCursor == MRCSequence )
//    Invalidate ( false );
//    ShowNow ();
}


//----------------------------------------------------------------------------
void    TSecondaryView::CmSyncViews ()
{
if      ( CaptureMode == CaptureNone ) {

    ButtonGadgetSetState ( IDB_SYNCVIEWS, true );

    ::SetCursor ( *CartoolApplication->CursorSyncViews );

    CaptureMouse ( Capture );

    MouseDC     = new TClientDC (*this);

    CaptureMode = CaptureTFSync;
    }

else if ( CaptureMode == CaptureTFSync ) {  // cancel the sync manoeuvre

    ButtonGadgetSetState ( IDB_SYNCVIEWS, false );

    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
                                        // syncing all friends at once when done
//  TFCursor.SentTo = FriendshipId;
//  EEGDoc->NotifyFriendViews ( TFCursor.SentTo, vnNewTFCursor, (TParam2) &TFCursor, this );
    }
}


//----------------------------------------------------------------------------
void    TSecondaryView::EvPaletteChanged ( HWND )
{
//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
bool    TSecondaryView::VnViewSync ( const TTFCursor* tfc )
{
//if ( ! IsFriendView ( tfc->SentTo ) )
//    return false;                     // not for me !

if ( tfc->SentFrom != LinkedViewId )
    EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

return false;
}


//----------------------------------------------------------------------------
void    TSecondaryView::CmSpecifyScaling ()
{
double              scaling;


if ( ! GetValueFromUser ( "Give an absolute value:", "New Scaling", scaling, "", this ) || scaling == 0 )
    return;


scaling     = fabs ( scaling );

if ( scaling != 0 )
    SetScaling ( -scaling, scaling );


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TSecondaryView::CmShowSequenceLabels ()
{
ShowSequenceLabels  = ! ShowSequenceLabels;

Invalidate ( false );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
