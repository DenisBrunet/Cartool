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

#include    <typeinfo>

#include    <owl/pch.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TBaseView.h"

#include    "TCartoolMdiChild.h"

#include    "MemUtil.h"

#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TList.h"
#include    "TInterval.h"
#include    "Strings.Utils.h"
#include    "Volumes.TTalairachOracle.h"

#include    "TLinkManyDoc.h"
#include    "TVolumeDoc.h"

#include    "TTracksView.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle    Taloracle;


const double    AntiAliasingJitter[ NumAntiAliasingJitter ][ 2 ] = 
        { 
            {   0.5625, 0.4375  }, 
            {   0.0625, 0.9375  }, 
            {   0.3125, 0.6875  }, 
            {   0.6875, 0.8125  }, 
            {   0.8125, 0.1875  }, 
            {   0.9375, 0.5625  }, 
            {   0.4375, 0.0625  }, 
            {   0.1875, 0.3125  } 
        };


const double    PointRadiusRatio    [ PointsNumRendering ]  = {
        1.00,
        0.125,
        0.25,
        0.50,
        1.00 
        };

                                        // give more radius boost for small points, less boost for big points
const double    HighlightRadiusBoost[ PointsNumRendering ]  = {
        1.00,
        2.00,
        2.00,
        1.50,
        1.10 
        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Only one window can capture the mouse, which should be the original message sender
void        TBaseView::CaptureMouse     (   CaptureMouseEnum    how     )
{
                                        // Global lock on mouse capture to prevent cloned messages to steal from each others
static bool         LockedCapture   = false;

                                        // capture if available
if      ( how == Capture && ! LockedCapture
       || how == ForceCapture               ) {

    LockedCapture   = true;

    SetCapture     ();
    }
                                        // release if captured
else if ( how == Release &&   LockedCapture
       || how == ForceRelease               ) {

    LockedCapture   = false;

    ReleaseCapture ();
    }
                                       
}


//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1(TBaseView, TWindowView)

    EV_WM_GETMINMAXINFO,
    EV_WM_SIZE,
    EV_WM_ERASEBKGND,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
    EV_WM_KEYUP,

    EV_VN_VIEWDESTROYED,

    EV_COMMAND          ( CM_EDITUNDO,          CmEditUndo ),
    EV_COMMAND_ENABLE   ( CM_EDITUNDO,          CmEditUndoEnable ),
    EV_COMMAND          ( CM_EDITCOPYBM,        GLEditCopyBitmap ),
    EV_COMMAND          ( CM_GEOMETRYTRANSFORM, CmGeometryTransform ),
    EV_COMMAND_ENABLE   ( CM_GEOMETRYTRANSFORM, CmGeometryTransformEnable ),

    EV_COMMAND          ( CM_SHOWINFOS,         CmShowInfos ),
    EV_COMMAND          ( CM_SHOWCOLORSCALE,    CmShowColorScale ),
    EV_COMMAND          ( CM_SHOWAXIS,          CmShowAxis ),
    EV_COMMAND          ( CM_SHOWORIENTATION,   CmShowOrientation ),
    EV_COMMAND          ( CM_SHOWSIZEBOX,       CmShowSizeBox ),
    EV_COMMAND          ( CM_SHOWBOUNDINGBOX,   CmShowBoundingBox ),
    EV_COMMAND_AND_ID   ( CM_SHOWALL,           CmShowAll ),
    EV_COMMAND_AND_ID   ( CM_HIDEALL,           CmShowAll ),

END_RESPONSE_TABLE;


        TBaseView::TBaseView ( TBaseDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TWindowView ( doc, parent ), BaseDoc ( &doc )
{
if ( group )    GODoc   = group;        // given at creation
else            GODoc   = BaseDoc->LastGroupDoc; // or get it from the doc, whatever 0 or real doc


StringCopy      ( Title, BaseDoc->GetTitle() ); // set title to default document's


ResetFriendship ();                     // no sharing upon init
BumpNextViewId ();                      // in case the program creates views itself


ControlBarGadgets       = 0;
NumControlBarGadgets    = 0;
LinkedViewId            = 0;


ControlKey              = false;
ShiftKey                = false;
//AltKey                = false;
LeftKey                 = false;
RightKey                = false;
UpKey                   = false;
DownKey                 = false;

                                        // OpenGL renders only in the client area
Attr.Style             |=  WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
WindowClientOffset      = TSize ( 0, 0 );
StandSize               = TSize ( 250, 100 );   // default standard size


MouseDC                 = 0;
LButtonDown             = false;
MButtonDown             = false;
RButtonDown             = false;
CaptureMode             = CaptureNone;
MouseAxis               = MouseAxisNone;
MousePos                = TPoint ( 0, 0 );
LastMouseMove           = TPoint ( 0, 0 );


RenderingMode           = 0;
ViewportOrgSize[0]      = ViewportOrgSize[1]= 0;
ViewportOrgSize[2]      = ViewportOrgSize[3]= 1;

ModelRadius             = 0;
Zoom                    = 1;


for ( int cti = 0; cti < NumAtomTypes; cti++ )
    ColorTableIndex[ cti ]  = UnknownTable;


ShowLabels              = ShowLabelNone;
ShowTextColor           = TextColorLight;
ShowInfos               = true;
ShowAxis                = true;
ShowColorScale          = true;
ShowOrientation         = true;
Orientation             = DefaultVolumeOrientation - 1;
ShowSizeBox             = false;
ShowBoundingBox         = false;

ShiftAxis               = ShiftNone;
Magnifier[0]            = Magnifier[1]    = 0.5;
Magnifier[2]            = 3;
CurrentDisplaySpace     = DisplaySpaceNone;


BackColor.Set ( 0, GLBASE_BACKCOLOR_NORMAL );
BackColor.Set ( 1, GLBASE_BACKCOLOR_PRINTING );

FogColor. Set ( 0, GLBASE_FOGCOLOR_NORMAL );
FogColor. Set ( 1, 0.50, 0.50, 0.50, 1.00 );

LineColor.Set ( 0, 0.80, 0.80, 0.80, 1.00 );
LineColor.Set ( 1, 0.15, 0.15, 0.15, 1.00 );
LineColor.Set ( 2, 0.66, 0.66, 1.00, 1.00 );

TextColor.Set ( 0, 1.00, 1.00, 1.00, 1.00 );
TextColor.Set ( 1, 0.00, 0.00, 0.00, 1.00 );
TextColor.Set ( 2, 0.75, 0.75, 1.00, 1.00 );
TextColor.Set ( 3, 0.20, 0.20, 0.60, 1.00 );


Light0 = TGLLight<GLfloat> ( GL_LIGHT0,
                              0.10,  0.10,  0.10,  1.00,
                              0.66,  0.66,  0.66,  1.00,
                              1.00,  1.00,  1.00,  1.00,
                             -5.00, -2.00,  0.00,  0.00 );

Light1 = TGLLight<GLfloat> ( GL_LIGHT1,
                              0.25,  0.25,  0.25,  1.00,
                              1.00,  1.00,  1.00,  1.00,
                              1.00,  1.00,  1.00,  1.00,
                              3.00,  4.00,  5.00,  0.00 );

                                        // default origin radius set to 0, which will prevent from drawing
                                        // derived classes set it to their liking, according to object's scaling
OriginRadius    = 0;
                                        // Fog, axis: not set here as they depend on the actual geometry extend
}


        TBaseView::~TBaseView ()
{
if ( MouseDC ) {
    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
    }

                                        // tell other views not to point to this anymore
BaseDoc->NotifyViews ( vnViewDestroyed, (TParam2) this, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove from the control bar
CartoolApplication->RemoveGadgets ();


if ( NumControlBarGadgets > 0 && ControlBarGadgets ) {  
                                        // then destroy the buttons
    for ( int g = 0; g < NumControlBarGadgets; g++ )
        delete ControlBarGadgets[ g ];

    delete[]  ControlBarGadgets;
    ControlBarGadgets   = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // automatically remove links to this
TListIterator<TBaseView>    iterator;

if ( ! CartoolApplication->Closing )
    foreachin ( UsedBy, iterator ) {
                                        // remove, and refresh
        iterator ()->Using.Remove ( this );
        iterator ()->Invalidate ( false );
        }
UsedBy.Reset ( false );

                                        // don't use these windows anymore
if ( ! CartoolApplication->Closing )                // maybe the windows pointed to are already destroyed?
    foreachin ( Using, iterator )
        iterator ()->UsedBy.Remove ( this );
Using.Reset ( false );
}


//----------------------------------------------------------------------------
//void    TBaseView::GetWindowClass ( WNDCLASS &wc )
//{
////wc.style    |= CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
//wc.style    |= CS_OWNDC;
//
//TWindow::GetWindowClass ( wc );
//}


//----------------------------------------------------------------------------
                                        // Actual creation, windows handle exists here
void    TBaseView::SetupWindow ()
{
TWindowView::SetupWindow ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CmOrient ();


GetParentO()->SetCaption ( Doc->GetTitle() );


TRect               wr              =   GetParentO()->GetWindowRect();
//TRect             cr              =   GetWindowRect();
TRect               cr              =   GetParentO()->GetClientRect();

//CartoolMdiClient->ScreenToClient ( wr );
//CartoolMdiClient->ScreenToClient ( cr );

                                        // offset origin with my parent window
//WindowClientOffset = TSize ( cr.Left() - wr.Left(), cr.Top() - wr.Top() );
WindowClientOffset = TSize ( wr.Width() - cr.Width(), wr.Height() - cr.Height() );


AnimTF  = TTimer ( GetHandle () );
AnimFx  = TTimer ( GetHandle () );
}


bool    TBaseView::SetDocTitle ( LPCTSTR /*docname*/, int /*index*/ )
{
GetParentO()->SetCaption ( /*BaseDoc->GetTitle()*/ Title ); // override with our own title

return false;
}

//----------------------------------------------------------------------------
                                        // Common buttons on the left side
void    TBaseView::CreateBaseGadgets ()
{
ControlBarGadgets[ BaseViewButtonSeparatorA     ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ BaseViewButtonToObject       ]   = new TButtonGadget ( IDB_2OBJECT,        IDB_2OBJECT,        TButtonGadget::NonExclusive, true, TButtonGadget::Up, false );
ControlBarGadgets[ BaseViewButtonRendering      ]   = new TButtonGadget ( IDB_SURFACEMODE,    IDB_SURFACEMODE,    TButtonGadget::Command );
ControlBarGadgets[ BaseViewButtonOrientation    ]   = new TButtonGadget ( IDB_ORIENT,         IDB_ORIENT,         TButtonGadget::Command );
ControlBarGadgets[ BaseViewButtonMagnifier      ]   = new TButtonGadget ( IDB_MAGNIFIER,      IDB_MAGNIFIER,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false );
}


//----------------------------------------------------------------------------
void    TBaseView::UpdateCaptionUsing ( char *buff )
{
ClearString ( buff );
                                        // add infos on graphical links
if ( (bool) Using ) {

    TListIterator<TBaseView>    iterator;

    foreachin ( Using, iterator ) {

        StringAppend ( buff, " + " );

//      StringCopy ( StringEnd ( buff ), dynamic_cast< TTracksView* > ( iterator () )->GetEEGDoc()->GetTitle() );
        StringCopy ( StringEnd ( buff ), dynamic_cast< TTracksDoc* > ( iterator ()->BaseDoc )->GetTitle() );
//      StringCopy ( StringEnd ( buff ), iterator ()->BaseDoc->GetTitle() );
        }
    }
}


//----------------------------------------------------------------------------
TBaseView*  TBaseView::ClientToBaseView ( const TPoint& p )
{
TPoint              pmodif ( p );

ClientToScreen ( pmodif );

HWND                hwnd            = WindowFromPoint ( pmodif );
TWindow*            towin           = CartoolApplication->GetWindowPtr ( hwnd );

if ( towin == 0 )
    return 0;

if ( typeid ( *towin ) == typeid ( TCartoolMdiChild ) )
    towin = towin->GetFirstChild ();

return  dynamic_cast<TBaseView *> ( towin );
}


//----------------------------------------------------------------------------
                                        // Default color tables according to data type
void    TBaseView::SetColorTable ( AtomType datatype )
{
                                        // disable all tables
ColorTable.SetTableAllowed ( NoTables );

                                        // Positive and Vectorial data
if      ( IsAbsolute ( datatype ) ) {

    ColorTable.SetTableAllowed ( AbsColorTable_GrayGreenYellowRed );
    ColorTable.SetTableAllowed ( AbsColorTable_GrayYellow );
    ColorTable.SetTableAllowed ( AbsColorTable_MagentaBlueCyanGrayGreenYellowRed );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = AbsColorTable_GrayGreenYellowRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

else if ( IsAngular ( datatype ) ) {

    ColorTable.SetTableAllowed ( CyclicColorTable_WhiteBlackWhite );
    ColorTable.SetTableAllowed ( CyclicColorTable_GreenBlueMagentaRedGreen );
    ColorTable.SetTableAllowed ( CyclicColorTable_YellowCyanBlueMagentaYellow );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = CyclicColorTable_YellowCyanBlueMagentaYellow;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

else { // if ( IsScalar ( datatype ) )      // all other cases

    ColorTable.SetTableAllowed ( SignedColorTable_BlueWhiteRed );
    ColorTable.SetTableAllowed ( SignedColorTable_NeuroscanColors );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = SignedColorTable_BlueWhiteRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }
}


//----------------------------------------------------------------------------
                                        // !Provided ID is a VIEW ID, NOT a Friendship ID!
bool    TBaseView::IsFriendView ( UINT viewid )   const   
{
TBaseView*          toview          = CartoolDocManager->GetView ( viewid );

return  toview && IsFriendView ( toview ); 
}

                                        // Actually transitive, will make nay friends of this new friend also friends
bool    TBaseView::SetFriendView ( const TBaseView* view )
{
if ( view == 0 ) 
    return  false;

                                        // already a buddy?
if ( IsFriendView ( view ) )
    return  true;


uint                friendid1 =       FriendshipId;
uint                friendid2 = view->FriendshipId;

                                        // now replace one ID by the other one -> all friends now!
for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
for ( TBaseView* v = doc->GetViewList (); v != 0; v = doc->NextView ( v ) )

    if ( v->IsFriendship ( friendid2 ) )

        v->SetFriendship ( friendid1 );


return true;
}

                                        // Does more than ResetFriendship which is only to window itself, as this function can propagate cancellation
bool    TBaseView::CancelFriendship ()  // (so sad)
{
if ( HasOtherFriendship () ) {
                                        // this view recovers its own ID
    ResetFriendship ();
    }

else {                                  // modify friend views and replace their friendship ID
    uint                friendid        = 0;

    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* v = doc->GetViewList (); v != 0; v = doc->NextView ( v ) ) {
                                        // is it.. me?
        if ( v->GetViewId () == GetViewId () )     
            continue;

        if ( IsFriendView ( v ) ) {

            if ( friendid == 0 )        // set to first encountered view
                friendid    = v->GetViewId ();
                                        // disconnecting this view from me
            v->SetFriendship ( friendid );
            }
        }
    }


return true;
}


//----------------------------------------------------------------------------
                                        // user range is between 0x0400 and 0x7FFF, within an application (see WM_USER and WM_APP)
                                        // so inserting this flag shouldn't hurt anything
static constexpr uint   ForwardMessageFlag      = 0x8000;

                                        // Buttons, Menus, other commands
TResult TBaseView::EvCommand ( uint id, THandle hWndCtl, uint notifyCode )
{
if ( GetCommandsCloning ()
  && CaptureMode != CaptureGLLink
  && CaptureMode != CaptureTFSync 
    ) {

    if ( IsFlag ( notifyCode, ForwardMessageFlag ) )    // is it already a forwarded message that we generated?

        ResetFlags ( notifyCode, ForwardMessageFlag );  // remove flag to prevent infinite forwarding, then proceed to default EvCommand processing

                                        // check for legal messages to pass
    else if ( id != IDB_2OBJECT
           && id != IDB_SYNCVIEWS
           && id != CM_EDITCOPYBM
           && id != CM_EDITCOPY
           && id != CM_INTERPOLATE
           && id != CM_AVERAGING
           && id != CM_SEGMENTEEG
           && id != CM_FITTING
           && id != CM_EXPORTTRACKS
           && id != CM_FREQANALYSIS
           && id != CM_GEOMETRYTRANSFORM
           && id != CM_DOWNSAMPLEMRI
           && id != CM_NORMALIZEMRI 
           && id != CM_CREATEROIS 
           && id != CM_COREGISTRATION
           && id != CM_DOWNSAMPLINGELECTRODES
           && id != CM_AVERAGEXYZ 
           && id != CM_SKULLSTRIPPING
           && id != CM_CUTBRAINSTEM
           && id != CM_STATISTICSTRACKS
           && id != CM_STATISTICSRIS
           && id != CM_STATISTICSFREQ
           && id != CM_HELPCONTENTS
           && id != CM_HELPABOUT
           && id != CM_EEGTIMERELATIVETO
           && id != CM_EEGTIMERELATIVE
           && id != CM_EEGTIMEABSOLUTE
//         && id != CM_EEGUSERSCALE
           && id != CM_GOTOTIME
           && id < MaxUserCommandId     ) { // above are general commands


//      DBGV ( id, "EvCommand Forward" );

                                        // !call original sender first!
        TResult     senderresult    = TWindowView::EvCommand ( id, hWndCtl, notifyCode );

                                        // spread to windows of same group, same type
        for ( TBaseDoc*  doc  = CartoolDocManager->DocListNext ( 0 ); doc; doc = CartoolDocManager->DocListNext ( doc ) )
        for ( TBaseView* view = doc->GetViewList ( GODoc ); view; view = doc->NextView ( view, GODoc ) )

            if ( view != this
              && typeid ( *view ) == typeid ( *this ) )

                view->EvCommand ( id, hWndCtl, CombineFlags ( notifyCode, ForwardMessageFlag ) );   // adding out own flag to track forwarding messages upon reception


//      GetParentO()->SetFocus ();


        return  senderresult;
        } // correct message

    } // CommandsCloning

                                        // default processing
return  TWindowView::EvCommand ( id, hWndCtl, notifyCode );
}

                                        // Keyboard, Mouse, all other events
TResult TBaseView::WindowProc ( uint msg, TParam1 p1, TParam2 p2 )   // TWindow
{
if ( GetCommandsCloning ()
  && CaptureMode != CaptureGLLink
  && CaptureMode != CaptureTFSync 
    ) {

    if ( IsFlag ( msg, ForwardMessageFlag ) )   // is it already a forwarded message that we generated?

        ResetFlags ( msg, ForwardMessageFlag ); // remove flag to prevent infinite forwarding, then proceed to default WindowProc processing

    else {
                                        // check for an EEG in 3D mode
        bool    leftok = ! dynamic_cast<TTracksView *> ( this ) || dynamic_cast<TTracksView *> ( this )->CurrentDisplaySpace == DisplaySpace3D;

                                        // check for legal messages to pass
        if ( ( msg == WM_KEYDOWN && p1 != VK_TAB     // don't allow anything dealing with time
                                 && ( p1 != VK_RIGHT || VkShift () )    // (horizontal zoom) can't use ShiftKey, it may not have been already processed by all windows
                                 && ( p1 != VK_LEFT  || VkShift () )
                                 && p1 != VK_PRIOR
                                 && p1 != VK_NEXT
                                 && p1 != VK_HOME
                                 && p1 != VK_END
                                 && p1 != VK_MULTIPLY   // window resizing
                                 && p1 != VK_DIVIDE
                                 && p1 != VK_RETURN
                                 && p1 != 'L' )         // don't forward linking

                                        // keyboard, always forwarded
          || ( ( msg == WM_KEYDOWN || msg == WM_KEYUP ) && ( p1 == VK_CONTROL || p1 == VK_SHIFT /*|| p1 == VK_MENU*/ ) )

                                        // also forward focus, to update keyboard state - !turned off to avoid all EEG in LM to insert their own gadgets - it seems we are good without it!
//        || ( msg == WM_SETFOCUS )

                                        // mouse stuff
          || ( (   msg == WM_LBUTTONDOWN
                || msg == WM_LBUTTONUP   ) && (                leftok                || CaptureMode == CaptureGLMagnify )
                || msg == WM_RBUTTONDOWN
                || msg == WM_RBUTTONUP
                || msg == WM_MOUSEWHEEL
                || msg == WM_MOUSEMOVE     && ( LButtonDown && leftok || RButtonDown || CaptureMode == CaptureGLMagnify ) )
           ) {

//          DBGV3 ( msg, p1, p2, "WindowProc Forward" );
//          if ( msg == WM_RBUTTONDOWN )    DBGV2 ( LOWORD ( p2 ), HIWORD ( p2 ), "RDown" );    // TPoint

                                        // !CALLING ORIGINAL SENDER FIRST - useful for setting filters from user f.ex.!
            TResult     senderresult    = TWindowView::WindowProc ( msg, p1, p2 );

                                        // THEN spreading to other windows of the same group & same type
            for ( TBaseDoc*  doc  = CartoolDocManager->DocListNext ( 0 ); doc; doc = CartoolDocManager->DocListNext ( doc ) )
            for ( TBaseView* view = doc->GetViewList ( GODoc ); view; view = doc->NextView ( view, GODoc ) )

                if ( view != this
                  && typeid ( *view ) == typeid ( *this ) )

                    view->SendMessage ( CombineFlags ( msg, ForwardMessageFlag ), p1, p2 );     // adding out own flag to track forwarding messages upon reception


    //      GetParentO()->SetFocus ();


            return  senderresult;
            } // if correct message

        } // else not ForwardMessageFlag

    } // CommandsCloning

                                        // default processing
return  TWindowView::WindowProc ( msg, p1, p2 );
}


//----------------------------------------------------------------------------
void    TBaseView::ButtonGadgetSetState ( TGadget *tog, bool down )
{
TButtonGadget*      tobg            = dynamic_cast<TButtonGadget*> ( tog );

if ( ! tobg )
    return;

tobg->SetButtonState ( down ? TButtonGadget::Down : TButtonGadget::Up );


/*                                      // We used to force updating, but OwlNext seems to have fixed the bug?
if ( tobg->GetButtonType () == TButtonGadget::Exclusive ) {

//    TGadget            *nextg           = CartoolApplication->ControlBar->NextGadget ( *tog );
//                                        // do some business to show it properly (bug?)
//    CartoolApplication->ControlBar->Remove ( *tog );
//    CartoolApplication->ControlBar->Insert ( *tog, TGadgetWindow::Before, nextg );
//    CartoolApplication->ControlBar->LayoutSession ();

    CartoolApplication->ControlBar->Invalidate ();  // ?instead?
    }
*/
}


void    TBaseView::ButtonGadgetSetState ( int Id, bool down )
{
                                        // scan all buttons for the correct Id
for ( int g = 0; g < NumControlBarGadgets; g++ )

    if ( ControlBarGadgets[ g ]->GetId () == Id ) {

        ButtonGadgetSetState ( ControlBarGadgets[ g ], down );

        return;
        }
}


bool    TBaseView::GetCommandsCloning ()    const
{
return  GODoc && GODoc->CommandsCloning;
}


//----------------------------------------------------------------------------
void    TBaseView::FitItemsInWindow ( int numitems, TSize itemsize, int &byx, int &byy, TRect *winrect )
{
if ( numitems <= 0 ) {
    byx = byy = 1;
    return;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no optional window? get the caller's
if ( winrect == 0 )     winrect     = &GetClientRect ();

                                        // get geometries of object and window
double              wx              = winrect->Width  ();
double              wy              = winrect->Height ();
double              ox              = itemsize.X ();
double              oy              = itemsize.Y ();


                                        // ratio of the ( X/Y ratios )
double              ratio           = NonNull ( fabs (           ( wx / NonNull ( wy ) )
                                                       / NonNull ( ox / NonNull ( oy ) ) ) );

bool                ishoriz         = ratio > 1.0;

                                        // first guess of the # of items in a normalized X
                                        // force a rounding to favor more X's when in vertical mode
//byx     = sqrt ( numitems * ratio ) + ( ishoriz ? -0.00 : +0.50 );
//byx     = sqrt ( numitems * ratio ) + ( ishoriz ? -0.50 : +0.50 );// * ( VkQuery () ? 1 : 0 );
//byx     = sqrt ( numitems * ratio ) + ( ishoriz ? -0.75 * ( 1 - 1 / ratio ) : 0.75 * ( 1 - ratio ) ) * ( VkQuery () ? 1 : 0 );

byx     = NonNull ( sqrt ( numitems * ratio ) + ( ishoriz ? 0 : 0.75 * ( 1 - ratio ) ) );
                                        // deduce first Y size
byy     = NonNull ( numitems / byx );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check the number of items really fits into our  byx * byy  tiling

if ( byx * byy == numitems )            // perfect match?
    return;

                                        // total count is not enough?
while ( byx * byy < numitems ) {
    byx +=   ishoriz;
    byy += ! ishoriz;
    }

                                        // total count could be reduced?
while ( ( byx - ishoriz ) * ( byy - ! ishoriz ) >= numitems ) {
    byx -=   ishoriz;
    byy -= ! ishoriz;
    }

                                        // a final check: if a single line/row, set to exact # of items
if      ( byy == 1 )    byx = numitems;
else if ( byx == 1 )    byy = numitems;
}


//----------------------------------------------------------------------------
bool    TBaseView::CanClose ()
{
TBaseView*          view;

if ( CartoolApplication->Closing )      // Everybody is quitting the ship!
    return  true;

                                        // Civilized way
if ( GODoc ) {

    int                 ngodviews       = 0;
                                        // special case of eeg tracks
    if ( typeid ( *this ) == typeid ( TTracksView  ) ) {

        for ( view = BaseDoc->GetViewList(); view != 0; view = BaseDoc->NextView ( view ) )

            if ( view->GODoc == GODoc
                && typeid ( *view ) == typeid ( TTracksView ) )

                ngodviews++;
        }
    else { // ! TTracksView

        for ( view = BaseDoc->GetViewList(); view != 0; view = BaseDoc->NextView (view) )
            if ( view->GODoc == GODoc )

                ngodviews++;
        }

    return  ngodviews > 1;              // can close this window if more than one view of this group remain
    }
else

    return TWindowView::CanClose ();
}


//----------------------------------------------------------------------------
void    TBaseView::PrePaint ( TDC& dc, TRect&, double objectradiusx, double objectradiusy, double objectradiusz )
{
GetClientRect ( PaintRect );

                                        // OpenGL inits
if ( GLrc.IsOpen () )   GLrc.GLize ( dc );          // use context
else                    GLrc.Set   ( dc, GLpfd );   // (re)create window context if needed

//GLDisplayError ();
//GLDisplayVersion ();

                                        // where to draw?
glDrawBuffer ( ! GLpfd.IsDoubleBuffer () || Outputing () && GLpfd.IsSoftware () ? GL_FRONT : GL_BACK );

                                        // from where to read from (copy/paste)?
glReadBuffer ( ! GLpfd.IsDoubleBuffer () || Outputing () && GLpfd.IsSoftware () ? GL_FRONT : GL_BACK );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a few common OpenGL setup
GLfloat*            tob             = BackColor.Get ( Outputing () );

glClearColor ( tob[ 0 ], tob[ 1 ], tob[ 2 ], tob[ 3 ] );

Fog.SetColor ( FogColor[ Outputing () ] );


if ( objectradiusx ) {                  // 3D mode

    GLWriteDepthOn      ();
    GLTestDepthOn       ();
    GLMaterialOn        ();
//  GLFogOn             ();
    glClear             ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
else {                                  // 2D window mode

    GLWriteDepthOff     ();
    GLTestDepthOff      ();
    GLColoringOn        ();
    GLFogOff            ();
    glClear             ( GL_COLOR_BUFFER_BIT );
    }


GLNormalizeNormalOff();
glDisable           ( GL_DITHER );
glEdgeFlag          ( GL_TRUE );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // saving current viewport
ViewportOrgSize[ 0 ]    = 0;
ViewportOrgSize[ 1 ]    = 0;
ViewportOrgSize[ 2 ]    = PaintRect.Width ();
ViewportOrgSize[ 3 ]    = PaintRect.Height ();


if ( CaptureMode == CaptureGLMagnify ) {
                                        // magnification is done at the viewport level
    double              m           = 1;

    if ( (bool) AnimFx ) {

        if      ( AnimFx.GetTimerId() == TimerMagnifierIn  )    m = AnimFx.GetPercentageCompletion ();
        else if ( AnimFx.GetTimerId() == TimerMagnifierOut )    m = AnimFx.GetPercentageRemaining  ();

        m      = Square ( m );
        }

                                        // new size and shift
    ViewportOrgSize[ 0 ]    = m * ( ( Magnifier[ 0 ] - 1 ) * ViewportOrgSize[ 2 ] ) * ( Magnifier[ 2 ] - 1 );
    ViewportOrgSize[ 1 ]    = m * ( ( Magnifier[ 1 ] - 1 ) * ViewportOrgSize[ 3 ] ) * ( Magnifier[ 2 ] - 1 );
    ViewportOrgSize[ 2 ]    = ViewportOrgSize[ 2 ] * ( m * Magnifier[ 2 ] + ( 1 - m ) );
    ViewportOrgSize[ 3 ]    = ViewportOrgSize[ 3 ] * ( m * Magnifier[ 2 ] + ( 1 - m ) );

                                        // check if size exceeds the max allowed
    if ( ViewportOrgSize[ 2 ] > ViewportMaxSize[ 0 ] || ViewportOrgSize[ 3 ] > ViewportMaxSize[ 1 ] ) {
                                        // compute relative ratio to max size allowed ( < 1 if ok, > 1 otherwise)
        double      rx          = 1 + (double) ( ViewportOrgSize[ 2 ] - ViewportMaxSize[ 0 ] ) / ViewportMaxSize[ 0 ];
        double      ry          = 1 + (double) ( ViewportOrgSize[ 3 ] - ViewportMaxSize[ 1 ] ) / ViewportMaxSize[ 1 ];
                                        // take biggest ratio, at least one is above 1
        double      r           = max ( rx, ry );
                                        // shrink size homogenously in both direction
        ViewportOrgSize[ 2 ]   /= r;
        ViewportOrgSize[ 3 ]   /= r;
                                        // and update the translation shift accordingly
        ViewportOrgSize[ 0 ]    = m * ( ( Magnifier[ 0 ] - 1 ) * PaintRect.Width ()  ) * ( Magnifier[ 2 ] / r - 1 );
        ViewportOrgSize[ 1 ]    = m * ( ( Magnifier[ 1 ] - 1 ) * PaintRect.Height () ) * ( Magnifier[ 2 ] / r - 1 );
        }

    } // CaptureGLMagnify


glViewport ( ViewportOrgSize[ 0 ], ViewportOrgSize[ 1 ], ViewportOrgSize[ 2 ], ViewportOrgSize[ 3 ] );

//GLDisplayError ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setup projection
glMatrixMode    ( GL_PROJECTION );
glLoadIdentity  ();


if ( objectradiusx ) {                        // 3D mode?

    double      viewratio   = (double) PaintRect.Height () / NonNull ( PaintRect.Width () )
                            / NonNull ( objectradiusy )    * objectradiusx;

    if ( viewratio == 0 )   viewratio = 1;

    double      ratiox      = viewratio < 1 ? 1 / viewratio : 1;
    double      ratioy      = viewratio < 1 ? 1             : viewratio;

    double      newzoom     = Square ( Zoom );

                                        // viewing box: +- radius in x and y
                                        // from 0 to ( 2 * ClippingMargin * radiusz ) in z
    glOrtho ( - objectradiusx * ratiox / newzoom, objectradiusx * ratiox / newzoom,
              - objectradiusy * ratioy / newzoom, objectradiusy * ratioy / newzoom,
                0,                                objectradiusz * 2                 );
    }
else {                                  // 2D window coordinates
    gluOrtho2D ( 0, PaintRect.Width(), 0, PaintRect.Height() );
    }

//GLDisplayError ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ready for model transformation
                                        // default is to be in modelview matrix mode
glMatrixMode    ( GL_MODELVIEW );
glLoadIdentity  ();
}


//----------------------------------------------------------------------------
                                        // Basically looping into GLPaint with slight offsets, rendering are then averaged together
void    TBaseView::AntialiasingPaint ()
{
                                        // Regular drawing, or no accumulation buffer either
if ( ! ( GLpfd.GetAccumBits ( true ) && ( Outputing () /*|| ( ShiftKey && VkKey ( 'A' ) )*/ ) ) ) {

                                        // regular rendering
    NestedPaints ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Antialiasing drawing
//#define     GaugeAntialiasing

#if defined ( GaugeAntialiasing )
                                        // Old code - seems reliable though                                        
//TGauge*             gauge       = new TGauge    (   CartoolMdiClient, "Optimizing Snapshot  %3d%%", 
//                                                    1234,
//                                                    Left ( CartoolMdiClient ) + ( Width  ( CartoolMdiClient ) - GaugeWidth  ) / 2,
//                                                    Top  ( CartoolMdiClient ) + ( Height ( CartoolMdiClient ) - GaugeHeight ) / 2,
//                                                    GaugeWidth,
//                                                    GaugeHeight, 
//                                                    true 
//                                                );
//gauge->Create ();

                                        // New code, using TSuperGauge for consistenty - to be tested for reliability
TSuperGauge         Gauge ( "Optimizing Snapshot", NumAntiAliasingJitter );

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve the current projection (we already have a copy of the viewport..)
TGLMatrix           M;

M.CopyProjection ();
                                        // get a pixel width back to through current projection
double              pixeldx         = 2.0 / ( M[ 0 ] ? M[ 0 ] : 1.0 ) / NonNull ( ViewportOrgSize[ 2 ] );
double              pixeldy         = 2.0 / ( M[ 5 ] ? M[ 5 ] : 1.0 ) / NonNull ( ViewportOrgSize[ 3 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glClear ( GL_ACCUM_BUFFER_BIT );


for ( int i = 0; i < NumAntiAliasingJitter; i++ ) {


#if defined ( GaugeAntialiasing )

//  gauge->SetValue ( Percentage ( i + 1, NumAntiAliasingJitter ) );
//
//  gauge->UpdateWindow ();
            
    Gauge.Next ();

//  SetFocus ();

#endif

                                        // compute the translation
    M[ 12 ]  = ( AntiAliasingJitter[ i ][ 0 ] - 0.5 ) * pixeldx;
    M[ 13 ]  = ( AntiAliasingJitter[ i ][ 1 ] - 0.5 ) * pixeldy;

    glMatrixMode ( GL_PROJECTION );
    glPushMatrix ();
    glTranslated ( M[ 12 ], M[ 13 ], 0 );   // we poke into the projection matrix

    glMatrixMode ( GL_MODELVIEW );
    if ( i < NumAntiAliasingJitter - 1 )    // save the transformation, as successive calls really mess up the display!
        glPushMatrix ();

                                    // clear buffers, draw, & cumulate
    glClear     ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    NestedPaints ();
    glAccum ( GL_ACCUM, 1.0 / NumAntiAliasingJitter );

                                        // revert everything back
    glMatrixMode ( GL_PROJECTION );
    glPopMatrix ();

    glMatrixMode ( GL_MODELVIEW );
    if ( i < NumAntiAliasingJitter - 1 )
        glPopMatrix ();                 // last call: let the current transformation alive for any further processings

//  glFlush ();

    } // for jitter


glAccum ( GL_RETURN, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined ( GaugeAntialiasing )

//delete gauge;

Gauge.Reset ();
                                        // restore focus to actual window
SetFocus ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
}


//----------------------------------------------------------------------------
void    TBaseView::NestedPaints ()
{
                                        // special treatment if we are linking to other views
if ( (int) Using ) {

    int     maxpass = (int) Using + 1;
    int     pass    = 0;

    DepthRange.DepthFromMultipass ( pass++, maxpass );

    DepthRange.GLize ();

                                        // paint first the opaque
    GLPaint ( GLPaintOwner | GLPaintOpaque, RenderingUnchanged, ClipPlane );

                                        // paint the others
    if ( ! CartoolApplication->Closing )

        for ( int i = 0; i < (int) Using; i++ ) {

            DepthRange.DepthFromMultipass ( pass++, maxpass );

            DepthRange.GLize ();

            Using[ i ]->GLPaint ( GLPaintLinked | GLPaintOpaque, RenderingUnchanged, ClipPlane );
            }


    DepthRange.unGLize ();              // reset depth range

                                        // then paint the transparent
    GLPaint ( GLPaintOwner | GLPaintTransparent, RenderingUnchanged, ClipPlane );

                                        // paint the others
    if ( ! CartoolApplication->Closing )

        for ( int i = 0; i < (int) Using; i++ )

            Using[ i ]->GLPaint ( GLPaintLinked | GLPaintTransparent, RenderingUnchanged, ClipPlane );
    }
else {
    DepthRange.unGLize ();              // reset depth range
                                        // 1 pass drawing
    GLPaint ( GLPaintNormal, RenderingUnchanged, ClipPlane );
    }

//GLDisplayError ();
}


//----------------------------------------------------------------------------
void    TBaseView::PostPaint ( TDC &dc )
{
                                        // last call to draw something related to the window itself
if ( ! Outputing () && GetCapture() == *this )
    HintsPaint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show FPS, for all windows, captured or not
if ( VkQuery () ) {

    static ulong        start           = GetWindowsTimeInMillisecond ();
    static ulong        end             = start;
    static char         bufffps [ 16 ]  = "";
    static int          count           = 0;
//  double              ydeltaB         = PrintVerticalStep ( BFont );
//  double              ydeltaS         = PrintVerticalStep ( SFont );
    double              xpos            = PrintLeftPos;
    double              ypos            = PrintBottomPos; // + ydeltaB;
    double              zpos            = PrintDepthPosFront;


    count++;
    end     = GetWindowsTimeInMillisecond ();

    SetWindowCoordinates ();

                                        // Speed
    if ( end - start >= 1000 ) {
        ClearString ( bufffps );
        count = 0;
        start = end;
        }
    else if ( end - start >= 250 ) {
        StringCopy  ( bufffps, "FPS ", IntegerToString ( (int) ( (double) count * 1000.0 / ( end - start ) + 0.5 ), 2 ) );
        count = 0;
        start = end;
        }

    glColor3f ( 1.00, 0.00, 0.00 );
    BFont->Print ( xpos, ypos, zpos, bufffps, TA_LEFT | TA_BOTTOM );
//  ypos   += ydeltaB;

                                        // Also keyboard state(?)
//  static char         buffkst [ 64 ]  = "";
//  StringCopy  ( buffkst, "Control", ControlKey ? "ON" : "OFF", "  " "Shift", ShiftKey ? "ON" : "OFF" );
//  BFont->Print ( xpos, ypos, zpos, buffkst, TA_LEFT | TA_BOTTOM );
//  ypos   += ydeltaB;

                                      // And data type(?)
//    SFont->Print ( xpos, ypos, zpos, BaseDoc->GetContentTypeName ( buffstr ), TA_LEFT | TA_BOTTOM );
//    ypos   += ydeltaS;
//
//    SFont->Print ( xpos, ypos, zpos, BaseDoc->GetAtomTypeName ( AtomTypeUseCurrent ), TA_LEFT | TA_BOTTOM );
//    ypos   += ydeltaS;
//
//    if ( dynamic_cast< TTracksDoc* > ( BaseDoc ) ) {
//        SFont->Print ( xpos, ypos, zpos, dynamic_cast< TTracksDoc* > ( BaseDoc )->GetDim2TypeName (), TA_LEFT | TA_BOTTOM );
////      ypos   += ydeltaS;
//        }


    ResetWindowCoordinates ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OpenGL finish
//glFlush();                            // flush all commands, and returns before execution
//glFinish();                           // flush all commands, but wait until execution is finished

                                        // show da window
if ( ! GLpfd.IsDoubleBuffer () )
    glFlush ();
else
    SwapBuffers( dc );

                                        // copying back to front
//glReadBuffer (GL_BACK);
//glDrawBuffer (GL_FRONT);
//glCopyPixels ( ViewportOrgSize[0], ViewportOrgSize[1], ViewportOrgSize[2], ViewportOrgSize[3], GL_COLOR );


//GLDisplayError ();
                                        // comment this to prevent unloading context, if it is needed by other functions
GLrc.unGLize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // views depending on this one should be updated
TListIterator<TBaseView>    iterator;

if ( (bool) UsedBy && ! CartoolApplication->Closing )

    foreachin ( UsedBy, iterator ) {

//      UsedBy()->ShowNow ();
//      UsedBy()->Invalidate ();
                                        // this code is to reduce the number of Paint
        if      ( ! iterator ()->GetCommandsCloning () )    iterator ()->Invalidate (); // help cumulate Paint coming from cloned windows
        else if ( ! IsFriendView ( iterator () ) )          iterator ()->ShowNow ();    // for faster interaction
        }
}


//----------------------------------------------------------------------------
void    TBaseView::HintsPaint ()
{
                                        // draw additional user interaction hints
if ( LButtonDown || MButtonDown || RButtonDown ) {


    if ( ! MButtonDown ) {
        SetWindowCoordinates ();
        glTranslated ( MousePos.X(), PaintRect.Height() - MousePos.Y(), 0 );
//      glTranslated ( PaintRect.Width() / 2, PaintRect.Height() / 2, 0 );
        glScaled ( CursorHintSize, CursorHintSize, 1 );
        }

    GLBlendOn       ();

    glColor4f          ( GLBASE_CURSORHINTCOLOR );
    BFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( LButtonDown ) {
                                        // Zooming?
        if ( ControlKey ) {

            if ( LastMouseMove.Y() > 0 ) {  // zoom out
                Prim.DrawArrow (  1.25, 0, 0, 0.50, 0.25, 1.00, 0.50,   0 );
                Prim.DrawArrow ( -1.25, 0, 0, 0.50, 0.25, 1.00, 0.50, 180 );
                Prim.DrawArrow ( 0,  1.25, 0, 0.50, 0.25, 1.00, 0.50,  90 );
                Prim.DrawArrow ( 0, -1.25, 0, 0.50, 0.25, 1.00, 0.50, -90 );
                }
            else {                      // zoom in
                Prim.DrawArrow ( -1.25, 0, 0, 0.50, 0.25, 1.00, 0.50,   0 );
                Prim.DrawArrow (  1.25, 0, 0, 0.50, 0.25, 1.00, 0.50, 180 );
                Prim.DrawArrow ( 0, -1.25, 0, 0.50, 0.25, 1.00, 0.50,  90 );
                Prim.DrawArrow ( 0,  1.25, 0, 0.50, 0.25, 1.00, 0.50, -90 );
                }
            }
                                        // rotating
        else {
            if ( ModelRotAngle[2] ) {   // && !ModelRotAngle[0] && !ModelRotAngle[1] ) {
                                        // rotation along Z axis
                Prim.DrawCircle ( 0, 0, 0, 1.00, 1.50, 0.80 * 180, 2.20 * 180 );

                Prim.DrawTriangle (  cos ( 0.80 * Pi ) * 1.25, sin ( 0.80 * Pi ) * 1.25, 0, 0.80 * 180 - 90 );
                Prim.DrawTriangle (  cos ( 2.20 * Pi ) * 1.25, sin ( 2.20 * Pi ) * 1.25, 0, 2.20 * 180 + 90 );
                }
            else {                      // panning
                Prim.DrawTriangle (  0.00,  1.00, 0,  90 );
                Prim.DrawTriangle (  0.00, -1.00, 0, -90 );
                Prim.DrawTriangle (  1.00,  0.00, 0,   0 );
                Prim.DrawTriangle ( -1.00,  0.00, 0, 180 );
                }

            if ( VkMenu () )            // add a "block", if moving by steps
                Prim.DrawRectangle ( 0, 0, 0, 0.5, 0.5, 0 );
            }
        } // if LButtonDown

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else if ( MButtonDown ) {

        if ( Picking.IsNotNull () ) {

            char                    buff     [ 1 * KiloByte ];
            char                    buffclip [ 1 * KiloByte ];
            char                    buffmodif[ 1 * KiloByte ]; // rather be generous on this one
            TClipboardEx            clipboard;

                                        // !DO NOT allocate the clipboard while in real time, as its demanding memory allocation will crash the window sooner or later!
                                        // Instead: When releasing the Middle button, children windows call Invalidate(), which will in turn call HintPaints one last time - This is when to perform the Clipboard access
//          if ( ! VkKey ( VkMButton () ) )// ?Is it really doing something here?
                clipboard.Set ( *this, 1 * KiloByte );


                                        // 1) Actual view can complete with additional information, and also update Picking
            if ( (bool) Using ) {
                char                buffusing[ 256 ];

                                        // cycle once in all the links, as to get the common Picking position to all
                for ( int i = 0; i < (int) Using; i++ )
                    Using[ i ]->ModifyPickingInfo ( Picking, buffusing );

                                        // then build up the information string
                ModifyPickingInfo ( Picking, buffmodif );

                GetLastWord ( buffmodif, buffclip );

                for ( int i = 0; i < (int) Using; i++ ) {
                    Using[ i ]->ModifyPickingInfo ( Picking, buffusing );
                    StringAppend ( buffmodif, "\n", buffusing );
                    }
                }
            else {
                ModifyPickingInfo ( Picking, buffmodif );
                                        // the last word is the most interesting...
                GetLastWord ( buffmodif, buffclip );
                }


                                        // 2) Print current coordinates from Picking
            if ( BaseDoc->IsContentType ( ContentTypeVolume ) ) {

                TVolumeDoc*     MRIDoc      = dynamic_cast< TVolumeDoc* > ( BaseDoc );

                StringCopy      ( buff, "Coordinates [voxel]: ",            FloatToString ( Picking.X, 2 ), 
                                                                    ", ",   FloatToString ( Picking.Y, 2 ), 
                                                                    ", ",   FloatToString ( Picking.Z, 2 )
                                );

                StringAppend    ( buff, "\n" );

                StringAppend    ( buff, "Coordinates [mm]  : ",             FloatToString ( Picking.X * MRIDoc->GetVoxelSize ().X, 2 ), 
                                                                    ", ",   FloatToString ( Picking.Y * MRIDoc->GetVoxelSize ().Y, 2 ), 
                                                                    ", ",   FloatToString ( Picking.Z * MRIDoc->GetVoxelSize ().Z, 2 )
                                );


                if ( (bool) clipboard ) {

                    StringCopy      ( clipboard,            FloatToString ( Picking.X, 2 ), 
                                                    "\t",   FloatToString ( Picking.Y, 2 ), 
                                                    "\t",   FloatToString ( Picking.Z, 2 )
                                    );

                    StringAppend    ( clipboard,    "\t",   FloatToString ( Picking.X * MRIDoc->GetVoxelSize ().X, 2 ), 
                                                    "\t",   FloatToString ( Picking.Y * MRIDoc->GetVoxelSize ().Y, 2 ), 
                                                    "\t",   FloatToString ( Picking.Z * MRIDoc->GetVoxelSize ().Z, 2 ) 
                                    );
                    }
                }
            else {                      // output (optionally modified) coordinates

                StringCopy      ( buff, "Coordinates: ",            FloatToString ( Picking.X, 2 ), 
                                                            ", ",   FloatToString ( Picking.Y, 2 ), 
                                                            ", ",   FloatToString ( Picking.Z, 2 )
                                );

                if ( (bool) clipboard )

                    StringCopy      ( clipboard,            FloatToString ( Picking.X, 2 ), 
                                                    "\t",   FloatToString ( Picking.Y, 2 ), 
                                                    "\t",   FloatToString ( Picking.Z, 2 )
                                    );
                }

                                        // 3) Print geometrically converted Picking, if any transform (Talairach f.ex.)

                                        // 3.1) Check if this window is depth-shift tricking
            bool    legalgeometry   = ! DepthRange.UserShift;

                                        // second, if any linked object also has a shift, then geometry is tricked
            if ( (bool) Using && legalgeometry )
                for ( int i = 0; i < (int) Using; i++ )
                    legalgeometry &= Using[ i ]->GetDepthRange ()->UserShift == 0;


                                        // 3.2) If no depth-shift trick -> transform & print
            if ( GetGeometryTransform () && (bool) GetDisplaySpaces () && legalgeometry ) {

                TPointFloat             pick ( Picking );

                                        // forward the bounding, to help guess any downsampling
                GetGeometryTransform ()->ConvertTo ( pick, & GetDisplaySpaces ()[ 0 ].Bounding );


                if ( pick.IsNotNull () ) {  // not foolproof, but...

                    StringAppend    ( buff, "\n" );

                    StringAppend    ( buff, "Coordinates [", GetGeometryTransform ()->Name, "]: " );

                    StringAppend    ( buff,         IntegerToString ( pick.X ),
                                            ", ",   IntegerToString ( pick.Y ), 
                                            ", ",   IntegerToString ( pick.Z )
                                    );


                    if ( (bool) clipboard )

                        StringAppend    ( clipboard,    "\t",   IntegerToString ( pick.X ),
                                                        "\t",   IntegerToString ( pick.Y ), 
                                                        "\t",   IntegerToString ( pick.Z )
                                        );

                                        // special case for Talairach: add the labels
                    if ( StringIs ( GetGeometryTransform ()->Name, "Talairach" ) ) {

                        StringAppend    ( buff, "\n" );

                        Taloracle.PositionToString ( pick, StringEnd ( buff ), true );

                        if ( (bool) clipboard ) {

                            StringAppend    ( clipboard, "\t" );

                            Taloracle.PositionToString ( pick, StringEnd ( clipboard ), false );
                            }
                        }
                    else { // not Talairach - not tested

                        if ( (bool) clipboard )

                            StringAppend ( clipboard, "\t\t\t\t\t" );
                        }
                    }
                else { // out of limits

                    StringAppend    ( buff, "\n" );

                    StringAppend    ( buff, "Coordinates [", GetGeometryTransform ()->Name, "]: ", "Out of limits" );


                    if ( (bool) clipboard )

                        StringAppend ( clipboard, "\t\t\t\t\t\t\t\t" );
                    }
                }
            else { // no geometry

                if ( (bool) clipboard )

                    StringAppend ( clipboard, "\t\t\t\t\t\t\t\t" );
                }


                                        // 4) Add additional information from 1) here
            if ( StringIsNotEmpty ( buffmodif ) ) {

                StringAppend    ( buff, "\n", buffmodif );

                if ( (bool) clipboard && StringIsNotEmpty ( buffclip ) )

                    StringAppend ( clipboard, "\t", buffclip );
                else
                    StringAppend ( clipboard, "\t" );
                }

                                        // Here, we are done with the string

                                        // send to clipboard
           if ( (bool) clipboard ) {
                clipboard.AddNewLine ();
                clipboard.Send ( CF_TEXT );
                }


                                        // 5) Draw a 3D mark, in the real space
            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

//            Prim.Draw3DCross ( Picking, 1.0, 3, true );

            double          er      = GetDisplaySpaces () ? GetDisplaySpaces ()[ 0 /*(bool) FlatView*/ ].Bounding.MaxSize () / 60 * 3 : 1.0;


            Prim.Draw3DCross ( Picking, er, 3, true );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();

                                        // 6) Finally print the text, as to overlap the cross
            GLBlendOn       ();
            BFont->Print ( Picking.X, Picking.Y, Picking.Z, buff, TA_LEFT | TA_TOP | TA_BOX | TA_TOFRONT, -11 * BFont->GetAvgWidth (), -24 );

            } // if Picking

        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else if ( RButtonDown ) {
        char buff[ 32 ];

        if ( MouseAxis == MouseAxisHorizontal ) {
//          sprintf ( buff, "%g   %g", ScalingNMax, ScalingPMax );
//          BFont->Print ( 0, -2, 1, buff, TA_TOP | TA_CENTER | TA_BOX );

            Prim.DrawBrightness ( 0, 0, 0, 1 );

            if      ( LastMouseMove.X() < 0 )
                Prim.DrawRectangle ( -2.50, 0.00, 0, 0.50, 0.12, 0 );
            else if ( LastMouseMove.X() > 0 )
                Prim.DrawPlus (  2.50, 0.00, 0, 0.50 );
            }
        else if ( MouseAxis == MouseAxisVertical ) {
//          sprintf ( buff, "%g", ScalingContrast );
            sprintf ( buff, "%g", ColorTable.GetContrast () );

            BFont->Print ( 2, 0, 1, buff, TA_LEFT | TA_CENTERY | TA_BOX );

            Prim.DrawContrast ( 0, 0, 0, 1 );

            if      ( LastMouseMove.Y() > 0 )
                Prim.DrawRectangle ( 0.00, -1.50, 0, 0.50, 0.12, 0 );
            else if ( LastMouseMove.Y() < 0 )
                Prim.DrawPlus (  0.00, 1.75, 0, 0.50 );
            }

        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( ! MButtonDown )
        ResetWindowCoordinates ();      // this will also restore a clean ModelView

    GLBlendOff      ();
    }
}


//----------------------------------------------------------------------------
                                        // does all we need to temporarily enter window coordinates mode
void    TBaseView::SetWindowCoordinates ( bool savestate )
{
GLWindowCoordinatesOn ( PaintRect.Width (), PaintRect.Height (), savestate );
}

                                        // restore the previous state before entering window coordinates mode
void    TBaseView::ResetWindowCoordinates ( bool restorestate )
{
GLWindowCoordinatesOff ( restorestate );
}


double  TBaseView::GetCurrentWindowSize ( bool locally )
{
if ( locally )
    return  min ( ViewportOrgSize[ 2 ], ViewportOrgSize[ 3 ] );
else {
    GLint        Viewport[ 4 ];

    glGetIntegerv ( GL_VIEWPORT, Viewport );

    return  min ( Viewport[ 2 ], Viewport[ 3 ] );
    }
}


double  TBaseView::GetCurrentZoomFactor ( bool locally )
{
if ( locally )
    return  max ( MatProjection [ 0 ], MatProjection [ 5 ] );
else {
    TGLMatrix       proj;

    proj.CopyProjection ();

    return  max ( proj [ 0 ], proj [ 5 ] );
    }
}


//----------------------------------------------------------------------------

void    TBaseView::DrawAxis ()
{
GLSmoothEdgesOn ();
GLColoringOn    ();
//glLineWidth ( 0.5 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw a little sphere at origin
if ( OriginRadius > 0 ) {

    TGLBillboardSphere             *tobbsphere  = &BbLowSphere;; // Outputing() ? &BbHighSphere : &BbLowSphere;
                                        // transparent, sowe can see what is actually below
    glColor4f ( (GLfloat) 0.80, (GLfloat) 0.80, (GLfloat) 0.80, (GLfloat) 0.60 );

    tobbsphere->GLize ();
                                        // not too big, be discreet
    tobbsphere->GLize ( 0, 0, 0, OriginRadius );

    tobbsphere->unGLize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // slightly pushing the axis forward, in case it perfectly intersects a clipping plane
DepthRange.ShiftCloser ( MultipasDepthShift );
DepthRange.GLize ();

                                        // then draw the axis + axis letter
constexpr GLfloat   AxisShiftPrint      = 5;

Xaxis.GLize ();
SFont->Print (  Xaxis.To.X + AxisShiftPrint, 
                Xaxis.To.Y, 
                Xaxis.To.Z, 
                "X", TA_CENTER | TA_CENTERY );

Yaxis.GLize ();
SFont->Print (  Yaxis.To.X, 
                Yaxis.To.Y + AxisShiftPrint, 
                Yaxis.To.Z, 
                "Y", TA_CENTER | TA_CENTERY );

Zaxis.GLize ();
SFont->Print (  Zaxis.To.X, 
                Zaxis.To.Y, 
                Zaxis.To.Z + AxisShiftPrint, 
                "Z", TA_CENTER | TA_CENTERY );


DepthRange.ShiftFurther ( MultipasDepthShift );
DepthRange.GLize ();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLSmoothEdgesOff ();
GLColoringOff    ();
//glLineWidth ( 1.0 );
}


//----------------------------------------------------------------------------
                                        // return the intersection in 2D with window border
                                        // border1 is the negative direction, border2 the positive
void    TBaseView::AxisToBorder (   TGLCoordinates<GLfloat> &dir,
                                    TGLCoordinates<GLfloat> &border1, int &textattr1,
                                    TGLCoordinates<GLfloat> &border2, int &textattr2 )
{

GLApplyModelMatrix ( dir, (GLdouble *) ModelRotMatrix );

double              dx              =  dir.X;
double              dy              = -dir.Y;
double              c               = dy * PaintRect.Width() / 2 + dx * PaintRect.Height() / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( dx ) {

    if ( dx > 0 ) {
        border1.X = 0;                      border1.Y = ( c - dy * border1.X ) / dx;    border1.Z = 0;  textattr1 = TA_LEFT  | TA_BOTTOM /*TA_CENTERY*/;
        border2.X = PaintRect.Width() - 1;  border2.Y = ( c - dy * border2.X ) / dx;    border2.Z = 0;  textattr2 = TA_RIGHT | TA_BOTTOM /*TA_CENTERY*/;
        }
    else {
        border2.X = 0;                      border2.Y = ( c - dy * border2.X ) / dx;    border1.Z = 0;  textattr2 = TA_LEFT  | TA_BOTTOM /*TA_CENTERY*/;
        border1.X = PaintRect.Width() - 1;  border1.Y = ( c - dy * border1.X ) / dx;    border2.Z = 0;  textattr1 = TA_RIGHT | TA_BOTTOM /*TA_CENTERY*/;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( dy &&
  ( !dx || dx && ( border1.Y < 0 || border2.Y < 0 || border1.Y > PaintRect.Height() || border2.Y > PaintRect.Height() ) ) ) {

    if ( dy < 0 ) {
        border1.Y = 0;                      border1.X = ( c - dx * border1.Y ) / dy;    border1.Z = 0;  textattr1 = TA_CENTER | TA_BOTTOM;
        border2.Y = PaintRect.Height() - 1; border2.X = ( c - dx * border2.Y ) / dy;    border2.Z = 0;  textattr2 = TA_CENTER | TA_TOP;
        }
    else {
        border2.Y = 0;                      border2.X = ( c - dx * border2.Y ) / dy;    border2.Z = 0;  textattr2 = TA_CENTER | TA_BOTTOM;
        border1.Y = PaintRect.Height() - 1; border1.X = ( c - dx * border1.Y ) / dy;    border1.Z = 0;  textattr1 = TA_CENTER | TA_TOP;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

textattr1   |= TA_TOBACK;
textattr2   |= TA_TOBACK;
}


//----------------------------------------------------------------------------
void    TBaseView::DrawOrientation ( const TOrientationType *boxsides )
{
                                        // above 30 degrees, don't show
                                        // -> 60 degrees of full visibility, 30 degrees off when in-between 2 orientations
constexpr double            SinMaxAngleProj     = 0.5;

//TOrientationType         *boxsides = BaseDoc->GetBoxSides();
TGLCoordinates<GLfloat>     dir;
TGLCoordinates<GLfloat>     border1;
TGLCoordinates<GLfloat>     border2;
int                         textattr1;
int                         textattr2;
                                        // no override?
if ( boxsides == 0 )
    boxsides = BaseDoc->GetBoxSides ();

TextColor.GLize ( Outputing () ? 1 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // intersection with X
dir.X   = 1;    
dir.Y   = 0;    
dir.Z   = 0;

AxisToBorder ( dir, border1, textattr1, border2, textattr2 );


if ( fabs ( dir.Z ) < SinMaxAngleProj ) {
                                        // make room for lowest flag
    if ( ! border1.Y )      border1.Y  += SFont->GetHeight ();
    if ( ! border2.Y )      border2.Y  += SFont->GetHeight ();

    SFont->Print ( border1.X, border1.Y + 1,                    -0.5, OrientationLabels [ boxsides[ XMinSide ] ], textattr1 );
    SFont->Print ( border2.X, border2.Y + 1,                    -0.5, OrientationLabels [ boxsides[ XMaxSide ] ], textattr2 );

    SFont->Print ( border1.X, border1.Y - SFont->GetHeight (),  -0.5, "X-", textattr1 );
    SFont->Print ( border2.X, border2.Y - SFont->GetHeight (),  -0.5, "X+", textattr2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // intersection with Y
dir.X   = 0;    
dir.Y   = 1;    
dir.Z   = 0;

AxisToBorder ( dir, border1, textattr1, border2, textattr2 );

if ( fabs ( dir.Z ) < SinMaxAngleProj ) {
                                        // make room for lowest flag
    if ( ! border1.Y )      border1.Y  += SFont->GetHeight ();
    if ( ! border2.Y )      border2.Y  += SFont->GetHeight ();

    SFont->Print ( border1.X, border1.Y + 1,                    -0.5, OrientationLabels [ boxsides[ YMinSide ] ], textattr1 );
    SFont->Print ( border2.X, border2.Y + 1,                    -0.5, OrientationLabels [ boxsides[ YMaxSide ] ], textattr2 );

    SFont->Print ( border1.X, border1.Y - SFont->GetHeight (),  -0.5, "Y-", textattr1 );
    SFont->Print ( border2.X, border2.Y - SFont->GetHeight (),  -0.5, "Y+", textattr2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // intersection with Z
dir.X   = 0;    
dir.Y   = 0;    
dir.Z   = 1;

AxisToBorder ( dir, border1, textattr1, border2, textattr2 );

if ( fabs ( dir.Z ) < SinMaxAngleProj ) {
                                        // make room for lowest flag
    if ( ! border1.Y )      border1.Y  += SFont->GetHeight ();
    if ( ! border2.Y )      border2.Y  += SFont->GetHeight ();

    SFont->Print ( border1.X, border1.Y + 1,                    -0.5, OrientationLabels [ boxsides[ ZMinSide ] ], textattr1 );
    SFont->Print ( border2.X, border2.Y + 1,                    -0.5, OrientationLabels [ boxsides[ ZMaxSide ] ], textattr2 );

    SFont->Print ( border1.X, border1.Y - SFont->GetHeight (),  -0.5, "Z-", textattr1 );
    SFont->Print ( border2.X, border2.Y - SFont->GetHeight (),  -0.5, "Z+", textattr2 );
    }
} // draw orientation


//----------------------------------------------------------------------------
void        TBaseView::DrawCross ( const TPointFloat& p, double r, bool colormin )
{
if ( DepthRange.UserShift )
    DepthRange.ShiftCloser ( DepthRange.UserShift );


DepthRange.ShiftCloser ();
DepthRange.GLize ();


glColor3f ( 1.0, 1.0, 1.0 );

Prim.Draw3DCross ( p, 3 * r, 5, true );


DepthRange.ShiftCloser ();
DepthRange.GLize ();
if ( colormin )     glColor3f ( GLBASE_MINCOLOR );
else                glColor3f ( GLBASE_MAXCOLOR );

Prim.Draw3DCross ( p, 2.8 * r, 2, true );


DepthRange.ShiftFurther ();
DepthRange.ShiftFurther ();


if ( DepthRange.UserShift )
    DepthRange.ShiftFurther ( DepthRange.UserShift );

DepthRange.GLize ();
}


//----------------------------------------------------------------------------
                                        // This is not going to be pleasant to read, I warn ya
void    TBaseView::GLEditCopyBitmap ()
{
TClipboard          clipboard ( *this );

if ( ! clipboard.EmptyClipboard () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // have a dc
UseThisDC           pdc ( *this );

TSize               size            = GetClientRect ().Size ();
TBitmap*            bitmap          = new TBitmap ( pdc, size.X (), size.Y (), false );
int                 bytesperpixel   = pdc.GetDeviceCaps ( BITSPIXEL ) / 8;
int                 bytesperline    = bytesperpixel * size.X ();


if ( IsOdd ( bytesperline ) )
    bytesperline    = ( bytesperline & ~0x1 ) + 2;

int                 numbytes        = bytesperline * size.Y();

                                        // old style of context, with palette color tables, are not OK!
if ( bytesperpixel < 3 ) {

    ShowMessage (   "Cartool does not handle the current screen color depth,"   NewLine 
                    "Please set your screen to 24 or 32 bits per pixel!"        NewLine 
                    "If not possible, consider upgrading your graphic card.", 
                    "Screen Color Depth Error", ShowMessageWarning, this );

    delete  bitmap;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//EvSize ( SIZE_RESTORED, size );       // old way: does the same job as above, but, magically, it appears to be more stable (not losing focus)

                                        // delete existing context and recreate a fresh one
//GLrc.Set ( pdc, GLpfd );
                                        // or, we can re-use existing context(?)
GLrc.GLize ( pdc );


TMemory             pPixelDataMemory ( numbytes );
uchar*              pPixelData      = (uchar *) pPixelDataMemory.GetMemoryAddress ();

                                        // now, load the GL buffer with a nice painting
CartoolApplication->Bitmapping    = true;

                                        // some paintings can take some time, so set the cursor to wait
HCURSOR             hPrevCursor = ::SetCursor ( ::LoadCursor ( 0, IDC_WAIT ) );

                                        // paint with flag on
//Paint ( pdc, true, TRect (0,0,0,0) );
//Paint ( pdc, true, PaintRect );
//ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN );
Paint ( pdc, true, PaintRect );         // call this silly window directly, otherwise we may lose the window if too long painting!


//UpdateApplication;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not too fast!
WaitMilliseconds ( 250 );

                                        // re-open the GL context that we have just been using to access the bitmaps
GLrc.GLize ( pdc );


//GLDisplayError ();                    // you don't trust the window?


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glPixelStorei ( GL_PACK_ALIGNMENT, 2 );
//glPixelStorei ( GL_PACK_ROW_LENGTH, size.X() );

                                        // read the front buffer
GLint               orb;                // save this before
glGetIntegerv   ( GL_READ_BUFFER, &orb );
glReadBuffer    ( GL_FRONT );           // we are going to read the front buffer (Paint has finished, and swapped the result to the front)

                                        // read in reverse Y order
for ( int y = 0; y < size.Y (); y++ )

    if ( bytesperpixel == 3 )   glReadPixels ( 0, size.Y () - y - 1, size.X (), 1, GL_BGR_EXT,  GL_UNSIGNED_BYTE, pPixelData + y * bytesperline );
    else                        glReadPixels ( 0, size.Y () - y - 1, size.X (), 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pPixelData + y * bytesperline );


glFinish ();                            // done

                                        // restore to default
glReadBuffer ( orb );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // free the GL context
//glFinish ();


//GLrc.Reset ();
GLrc.unGLize ();

                                        // back to regular display
CartoolApplication->Bitmapping    = false;
//UpdateApplication;


Invalidate ();                          // sending message (slow) is totally fine here
//ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN );
//Paint ( pdc, true, PaintRect );

                                        // send the bitmap to the clipbaord
bitmap->SetBitmapBits ( numbytes, pPixelData );
bitmap->TBitmap::ToClipboard ( clipboard );


if ( hPrevCursor )
    ::SetCursor ( hPrevCursor );


delete  bitmap;
}


//----------------------------------------------------------------------------
bool    TBaseView::EvEraseBkgnd ( HDC )
{
return true;
}


//----------------------------------------------------------------------------
void    TBaseView::EvGetMinMaxInfo ( MINMAXINFO& minmaxinfo )
{
minmaxinfo.ptMaxSize        = TPoint( 1600, 1200 );
minmaxinfo.ptMaxTrackSize   = TPoint( 1600, 1200 );

//TWindowView::EvGetMinMaxInfo ( minmaxinfo );
}


//----------------------------------------------------------------------------
                                        // Type: SIZE_MAXIMIZED	/ SIZE_MINIMIZED / SIZE_RESTORED
                                        // size: new client size
void    TBaseView::EvSize ( uint sizetype, const TSize& size )
{
                                        // some computers need to refresh their mind when resizing... / looks it used to be a bug in a driver, rather
//GLrc.Reset ();                        // next call to PrePaint will create a fresh context


if ( sizetype == SIZE_MINIMIZED ) {
    TWindow::EvSize ( sizetype, size );
    return;
    }

WindowSetSize ( size.X() + WindowClientOffset.X(), size.Y() + WindowClientOffset.Y() );
                                        // not useful anymore?
//::WindowSetSize ( this, size.X(), size.Y() );

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TBaseView::EvSetFocus ( HWND hwnd )
{
TWindowView::EvSetFocus ( hwnd );


CartoolApplication->LastActiveBaseView  = this;

                                        // Updating keys that might have changed BETWEEN focus switches
                                        // VkKey is a lower level, real-time polling, which does not make use of messages
ControlKey              = VkControl ();
ShiftKey                = VkShift ();
//AltKey                = VkMenu ();
LeftKey                 = VkKey ( VK_LEFT  );
RightKey                = VkKey ( VK_RIGHT );
UpKey                   = VkKey ( VK_UP    );
DownKey                 = VkKey ( VK_DOWN  );

                                        // update the last group with the one of this window
BaseDoc->LastGroupDoc = GODoc;

                                        // insert my buttons
CartoolApplication->InsertGadgets ( ControlBarGadgets, NumControlBarGadgets );
}


void    TBaseView::EvKillFocus ( HWND hwnd )
{
//CaptureMouse ( Release );
//CaptureMode = CaptureNone;

if ( MouseDC ) {
    CaptureMouse ( Release );

    delete  MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
    }

LButtonDown             = false;
MButtonDown             = false;
RButtonDown             = false;


ControlKey              = false;
ShiftKey                = false;
LeftKey                 = false;
RightKey                = false;
UpKey                   = false;
DownKey                 = false;

                                        // remove my buttons
CartoolApplication->RemoveGadgets ();


TWindowView::EvKillFocus ( hwnd );
}


void    TBaseView::SetFocusBack ( UINT backto )
{
LinkedViewId    = backto;

GetWindow()->GetParentO()->SetFocus();
}


//----------------------------------------------------------------------------
void    TBaseView::EvKeyDown ( uint key, uint /*repeatCount*/, uint /*flags*/ )
{
switch ( key ) {

    case VK_ESCAPE:
        if ( CaptureMode == CaptureGLLink )
            Cm2Object ();
        break;


    case VK_CONTROL:
        ControlKey  = true;
        break;

    case VK_SHIFT:
        ShiftKey    = true;
        break;


    case VK_LEFT:
        LeftKey     = true;
        break;

    case VK_RIGHT:
        RightKey    = true;
        break;

    case VK_UP:
        UpKey       = true;
        break;

    case VK_DOWN:
        DownKey     = true;
        break;


    case VK_MULTIPLY:
        
        if      ( LeftKey   )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_INCL );
        else if ( RightKey  )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_INCR );
        else if ( UpKey     )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_INCT );
        else if ( DownKey   )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_INCB );
        else                    CartoolMdiClient->CmWinAction ( CM_WINSIZE_INC  );
        break;

    case VK_DIVIDE:
        
        if      ( LeftKey   )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_DECL );
        else if ( RightKey  )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_DECR );
        else if ( UpKey     )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_DECT );
        else if ( DownKey   )   CartoolMdiClient->CmWinAction ( CM_WINSIZE_DECB );
        else                    CartoolMdiClient->CmWinAction ( CM_WINSIZE_DEC  );
        break;
    }
}


void    TBaseView::EvKeyUp ( uint key, uint /*repeatCount*/, uint /*flags*/ )
{
switch ( key ) {

    case VK_CONTROL:
        ControlKey  = false;
        break;

    case VK_SHIFT:
        ShiftKey    = false;
        break;


    case VK_LEFT:
        LeftKey     = false;
        break;

    case VK_RIGHT:
        RightKey    = false;
        break;

    case VK_UP:
        UpKey       = false;
        break;

    case VK_DOWN:
        DownKey     = false;
        break;

    }
}


void    TBaseView::EvTimer ( uint timerId )
{
switch ( timerId ) {

    case TimerMagnifierIn :
    case TimerMagnifierOut:
        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx || GetCommandsCloning () ) {

            AnimFx.Stop ();
                                        // clear flag
            if ( AnimFx.GetTimerId() == TimerMagnifierOut ) {
                CaptureMode = CaptureNone;
                ButtonGadgetSetState ( IDB_MAGNIFIER, CaptureMode == CaptureGLMagnify );
                }
            }

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;
    }
}


//----------------------------------------------------------------------------
void    TBaseView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& )
{
if ( ControlKey ) {

    if ( zDelta < 0 )       Zoom       += sqrt ( Zoom + 1 ) * 0.025;
    else                    Zoom       -= sqrt ( Zoom + 1 ) * 0.025;

    Maxed ( Zoom, 0.1 );

//  Invalidate ( false );
    ShowNow ();                         // for more interactive linked windows display
    }
}


//----------------------------------------------------------------------------
void    TBaseView::GLLButtonDown ( uint, const TPoint &p )
{
MousePos            = p;
LastMouseMove       = TPoint ( 0, 0 );


if ( CaptureMode == CaptureGLLink ) {

    TBaseView    *view    = ClientToBaseView ( p );

    if ( ! view ) {
        Cm2Object ();                   // end of process
        return;
        }

    if ( view != this )
                                        // already pointed to?
        if ( Using.IsInside ( view ) ) {
            view->UsedBy.Remove ( this );
            Using.Remove ( view );
            Invalidate ( false );
//            ShowNow ();
            }
        else {
            if ( ! UsedBy.IsInside ( view ) ) {
                view->UsedBy.Append ( this );
                Using.Append ( view );
                Invalidate ( false );
//                ShowNow ();

//              BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );   // it seems not needed...
                }
            else {
                Cm2Object ();           // finish
                ShowMessage ( "Can not create a loop of links between windows.", "Linking error", ShowMessageWarning );
                }
            }
    else {

        Cm2Object();                    // clean-up

        TListIterator<TBaseView>    iterator;

        foreachin ( Using, iterator )
            iterator ()->UsedBy.Remove ( this );

        Using.Reset ( false );
        Invalidate ( false );
        //ShowNow ();
        }
    }

else if ( CaptureMode == CaptureGLMagnify ) {
                                        // clean-up
    CmMagnifier ();
    }
}


void    TBaseView::GLMouseMove ( uint, const TPoint &p )
{
if      ( CaptureMode == CaptureGLMagnify ) {

    Magnifier[0] = 1 - (double) p.X() / GetClientRect().Width();
    Magnifier[1] =     (double) p.Y() / GetClientRect().Height();

    if      ( Magnifier[0] < -0.1 ) Magnifier[0] = -0.1;
    else if ( Magnifier[0] >  1.1 ) Magnifier[0] =  1.1;
    if      ( Magnifier[1] < -0.1 ) Magnifier[1] = -0.1;
    else if ( Magnifier[1] >  1.1 ) Magnifier[1] =  1.1;

//    UpdateApplication;                // to smooth zooming animation & mouse move - seems ok?? not always with sync windows?

//  Invalidate ( false );
    ShowNow ();
    }

else if ( LButtonDown ) {
                                        // Zooming?
    if ( ControlKey ) {

        double  dy = p.Y() - MousePos.Y();

        Zoom   += dy / GetClientRect().Height();
        if ( Zoom <= 0.1 )
            Zoom = 0.1;
        }
                                        // rotating
    else {

        double  winsize = min ( GetClientRect().Width(), GetClientRect().Height() );
        double  dx      = p.X() - MousePos.X();
        double  dy      = p.Y() - MousePos.Y();

        double  cx      = MousePos.X() - GetClientRect().Width()  / 2;
        double  cy      = MousePos.Y() - GetClientRect().Height() / 2;


        double  nc      = sqrt ( cx * cx + cy * cy );
        if ( nc == 0 ) nc = 1;
        cx /= nc; cy /= nc;

        if ( winsize == 0 ) winsize = 1;

        double  spcd    =   dx * cx + dy * cy;
        double  spocd   = - dx * cy + dy * cx;

                                        // "unproject" to a sphere to get the non-linear feeling of 3D
        double  dcos    = nc / winsize / 2 * 1.8;
        if ( dcos > 1 )
            dcos = 1;
        dcos = acos ( 1 - dcos );

        ModelRotAngle[0] =   dy    / winsize * dcos * 360;
        ModelRotAngle[1] =   dx    / winsize * dcos * 360;
        ModelRotAngle[2] = - spocd / winsize * 180 / 1.414;


        if ( nc < 0.33 * winsize
          || fabs ( spocd ) < fabs ( spcd ) )
//          || dx * dx + dy * dy < 2 && dx && dy )
            ModelRotAngle[2] = 0;       // cancel Z axis
        else
            ModelRotAngle[0] = ModelRotAngle[1] = 0;    // cancel non-Z axis

                                        // step the angles
        constexpr double    SteppedAngle    = 45;
        if ( VkMenu () ) {
            ModelRotAngle[0] = (int) ( ModelRotAngle[0] / SteppedAngle * 2    ) * SteppedAngle;
            ModelRotAngle[1] = (int) ( ModelRotAngle[1] / SteppedAngle * 2    ) * SteppedAngle;
            ModelRotAngle[2] = (int) ( ModelRotAngle[2] / SteppedAngle * 1.10 ) * SteppedAngle;
            }

        if ( ! ( ModelRotAngle[0] || ModelRotAngle[1] || ModelRotAngle[2] ) )
            return;

        ModelRotMatrix.RotateXYZ ( ModelRotAngle[0], ModelRotAngle[1], ModelRotAngle[2], MultiplyLeft );

/*
        if ( fabs ( ModelRotAngle[0] ) < 2 && fabs ( ModelRotAngle[1] ) < 2 && fabs ( ModelRotAngle[2] ) < 2 ) {
            count = 0;
            glClear ( GL_ACCUM_BUFFER_BIT );
            }
*/
        }

//  MousePos  = p;
//  Invalidate ( false );
    ShowNow ();
    }

else if ( MButtonDown ) {               // picking

                                        // we don't have a current dc, create a temp one
    GLrc.GLize ( UseThisDC ( *this ) );


    Picking.Set ( p.X(), p.Y(), 0 );
                                        // then ask for underlying object coordinates
//  if ( ! GLGetObjectCoord ( Picking, ViewportOrgSize ) )
    if ( ! GLGetObjectCoord ( Picking, ViewportOrgSize, MatProjection ) )
        Picking.Reset ();


    GLrc.unGLize ();


//  Invalidate ( false );
    ShowNow ();
    }


LastMouseMove       = TPoint ( p.X() - MousePos.X(), p.Y() - MousePos.Y() );
MousePos            = p;
}


//----------------------------------------------------------------------------
void    TBaseView::CmEditUndo ()
{
BaseDoc->Revert ();

CartoolMdiClient->RefreshWindows ();
}


void    TBaseView::CmEditUndoEnable ( TCommandEnabler &tce )
{
tce.Enable ( BaseDoc->IsDirty() );
}


void    TBaseView::CmGeometryTransform ()
{
if ( ! GetGeometryTransform () )
    return;


static GetFileFromUser  getfiles ( "Open Files", AllSolPointsFilesTxtFilter, 1, GetFileMulti );


if ( ! getfiles.Execute () )
    return;

                                        // loop through the files
for ( int i = 0; i < (int) getfiles; i++ )

    GetGeometryTransform ()->ConvertTo ( getfiles[ i ], GetDisplaySpaces () ? & GetDisplaySpaces ()[ 0 ].Bounding : 0 );


TSuperGauge         Gauge ( "Geometry Transform" );
Gauge.HappyEnd ();
}


void    TBaseView::CmGeometryTransformEnable ( TCommandEnabler &tce )
{
tce.Enable ( GetGeometryTransform() );
}


void    TBaseView::Cm2Object ()
{
if ( CaptureMode == CaptureNone ) {

    ::SetCursor ( *CartoolApplication->CursorLinkView );

    CaptureMouse ( Capture );

    MouseDC     = new TClientDC ( *this );

    CaptureMode = CaptureGLLink;
    }
else if ( CaptureMode == CaptureGLLink ) { // cancel the link manoeuvre

    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
    }

ButtonGadgetSetState ( IDB_2OBJECT, CaptureMode == CaptureGLLink );
}


void    TBaseView::CmOrient ()
{
                                        // unaltered orthogonal projection?
if ( ModelRotMatrix.IsOrthogonal ( 1e-6 ) )
                                        // switch to next orientation
    Orientation = ( Orientation + ( ShiftKey ? NumOrient - 1 : 1 ) ) % NumOrient;

else                                    // user has rotated the object, we offer to simply refresh the current orientation
    ;                                   // so don't actually update the current state


SetOrient ( BaseDoc );

Invalidate ( false );
//ShowNow ();
}


void    TBaseView::SetOrient ( TBaseDoc *doc )
{
// should use current Current3DSpace...
                                        // get the predefined orientation
ModelRotMatrix.SetOrientation ( Orientation );
                                        // standardizing orientation applies first
ModelRotMatrix *= ( doc ? doc : BaseDoc )->GetStandardOrientation ( LocalToPIR );
}


void    TBaseView::CmMagnifier ()
{
if ( CaptureMode == CaptureNone ) {

    ::SetCursor ( *CartoolApplication->CursorMagnify );

    CaptureMouse ( Capture );

    MouseDC     = new TClientDC ( *this );

    CaptureMode = CaptureGLMagnify;

    AnimFx.Start ( TimerMagnifierIn, 10, 10 );
                                        // generate a pseudo mouse move, then cancel the unwanted shift
                                        // so to position the view with the mouse
    mouse_event ( MOUSEEVENTF_MOVE,  1, 0, 0, 0 );
    mouse_event ( MOUSEEVENTF_MOVE, -1, 0, 0, 0 );
    }
else if ( CaptureMode == CaptureGLMagnify ) { // cancel

    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

//  CaptureMode = CaptureNone;

    AnimFx.Start ( TimerMagnifierOut, 10, 10 );

//  Invalidate ( false );
    }

ButtonGadgetSetState ( IDB_MAGNIFIER, CaptureMode == CaptureGLMagnify );
}


//----------------------------------------------------------------------------
void    TBaseView::CmShowInfos ()
{
ShowInfos   = ! ShowInfos;

Invalidate ( false );
}


void    TBaseView::CmShowAxis ()
{
ShowAxis    = ! ShowAxis;

Invalidate ( false );
}


void    TBaseView::CmShowColorScale ()
{
ShowColorScale  = ! ShowColorScale;

Invalidate ( false );
}


void    TBaseView::CmShowOrientation ()
{
ShowOrientation  = ! ShowOrientation;

Invalidate ( false );
}


void    TBaseView::CmShowSizeBox ()
{
ShowSizeBox         = ! ShowSizeBox;

Invalidate ( false );
}


void    TBaseView::CmShowBoundingBox ()
{
ShowBoundingBox     = ! ShowBoundingBox;

Invalidate ( false );
}


void    TBaseView::CmShowAll ( owlwparam w )
{
if ( w == CM_HIDEALL ) {
    ShowInfos           = false;
    ShowAxis            = false;
    ShowColorScale      = false;
    ShowOrientation     = false;
    ShowSizeBox         = false;
    ShowBoundingBox     = false;
    }
else {
    ShowInfos           = true;
    ShowAxis            = true;
    ShowColorScale      = true;
    ShowOrientation     = true;
    ShowSizeBox         = true;
    ShowBoundingBox     = true;
    }

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TBaseView::CmSetShiftDepthRange ( owlwparam w )
{
DepthRange.UserShift = (double) ( w - CM_SHIFTDEPTHRANGE0 ) / ( CM_SHIFTDEPTHRANGE4 - CM_SHIFTDEPTHRANGE0 )
                       * MaxShiftDepthRange;

if ( w == CM_SHIFTDEPTHRANGE0 )
    DepthRange.unGLize ();
}


int     TBaseView::NextRois ( int currrois, int dimrois )
{
if ( ! ( GODoc && GODoc->GetNumRoiDoc () ) )
    return  -1;


if ( currrois == -1 ) {

    currrois    = ShiftKey ? GODoc->GetNumRoiDoc () : -1;

    do {
        currrois   += ShiftKey ? -1 : 1;

        if ( currrois < 0 || currrois >= GODoc->GetNumRoiDoc () )
            return  -1;
        } while ( GODoc->GetRoisDoc ( currrois )->ROIs->GetDimension () != dimrois );

    return  currrois;
    }

else {
                                        // get next candidate
    int     oldrois = currrois;

    do {
        currrois    = ( currrois + ( ShiftKey ? GODoc->GetNumRoiDoc () : 1 ) ) % ( GODoc->GetNumRoiDoc () + 1 );

        if ( currrois == oldrois )
            currrois = GODoc->GetNumRoiDoc ();

        } while ( currrois < GODoc->GetNumRoiDoc () && GODoc->GetRoisDoc ( currrois )->ROIs->GetDimension () != dimrois );

    return  currrois == GODoc->GetNumRoiDoc () ? -1 : currrois;
    }
}


//----------------------------------------------------------------------------
bool    TBaseView::VnViewDestroyed ( TBaseView *view )
{
if ( view )
    if ( view->GetViewId() == LinkedViewId ) {
        LinkedViewId    = 0;
        }

return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
