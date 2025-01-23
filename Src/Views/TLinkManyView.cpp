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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TLinkManyView.h"

#include    "OpenGL.h"

#include    "TElectrodesDoc.h"
#include    "TVolumeDoc.h"
#include    "TRisDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "TInverseMatrixDoc.h"
#include    "TRoisDoc.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TLinkManyView, TBaseView)

//  EV_WM_GETMINMAXINFO,
    EV_WM_SIZE,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
//  EV_WM_KEYUP,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONDBLCLK,

    EV_COMMAND_AND_ID   ( IDB_GTVKEEPSIZE,      CmGTV ),
    EV_COMMAND_AND_ID   ( IDB_GTVSTANDSIZE,     CmGTV ),
    EV_COMMAND_AND_ID   ( IDB_GTVFIT,           CmGTV ),
    EV_COMMAND          ( IDB_ADDTOGROUP,       CmAddToGroup ),
    EV_COMMAND          ( IDB_SEGMENT,          CmSegment ),
    EV_COMMAND_AND_ID   ( CM_SYNCALL,           CmSyncUtility ),
    EV_COMMAND_AND_ID   ( CM_DESYNCALL,         CmSyncUtility ),
    EV_COMMAND_AND_ID   ( CM_SYNCBETWEENEEG,    CmSyncUtility ),
    EV_COMMAND_AND_ID   ( CM_DESYNCBETWEENEEG,  CmSyncUtility ),
    EV_COMMAND          ( CM_COMMANDSCLONING,   CmCommandsCloning ),

    EV_VN_RELOADDATA,

END_RESPONSE_TABLE;


        TLinkManyView::TLinkManyView ( TLinkManyDoc& doc, TWindow* parent )
      : TBaseView ( doc, parent )
{
GODoc       = &doc;

StandSize   = GetBestSize ();
Attr.W      = StandSize.X ();
Attr.H      = StandSize.Y ();

SetViewMenu ( new TMenuDescr ( IDM_LM ) );
//GetViewMenu ()->CheckMenuItem ( CM_COMMANDSCLONING, MF_BYCOMMAND | MF_CHECKED );

BackColor.Set ( 0, GLBASE_BACKCOLOR_PRINTING );
BackColor.Set ( 1, GLBASE_BACKCOLOR_PRINTING );
}


//----------------------------------------------------------------------------
void    TLinkManyView::CreateGadgets ()
{
NumControlBarGadgets    = LMVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();

ControlBarGadgets[ LMVIEW_CBG_SEP1              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ LMVIEW_CBG_GTVKEEPSIZE       ]   = new TButtonGadgetDpi ( IDB_GTVKEEPSIZE,       IDB_GTVKEEPSIZE,        TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_GTVSTANDSIZE      ]   = new TButtonGadgetDpi ( IDB_GTVSTANDSIZE,      IDB_GTVSTANDSIZE,       TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_GTVFIT            ]   = new TButtonGadgetDpi ( IDB_GTVFIT,            IDB_GTVFIT,             TButtonGadget::Command );

ControlBarGadgets[ LMVIEW_CBG_SEP2              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ LMVIEW_CBG_ADDTOGROUP        ]   = new TButtonGadgetDpi ( IDB_ADDTOGROUP,        IDB_ADDTOGROUP,         TButtonGadget::Command );

ControlBarGadgets[ LMVIEW_CBG_SEP3              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ LMVIEW_CBG_SYNCALL           ]   = new TButtonGadgetDpi ( CM_SYNCALL,            CM_SYNCALL,             TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_DESYNCALL         ]   = new TButtonGadgetDpi ( CM_DESYNCALL,          CM_DESYNCALL,           TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_SYNCBETWEENEEG    ]   = new TButtonGadgetDpi ( CM_SYNCBETWEENEEG,     CM_SYNCBETWEENEEG,      TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_DESYNCBETWEENEEG  ]   = new TButtonGadgetDpi ( CM_DESYNCBETWEENEEG,   CM_DESYNCBETWEENEEG,    TButtonGadget::Command );
ControlBarGadgets[ LMVIEW_CBG_COMMANDSCLONING   ]   = new TButtonGadgetDpi ( CM_COMMANDSCLONING,    CM_COMMANDSCLONING,     TButtonGadget::NonExclusive, true, GODoc->CommandsCloning ? TButtonGadget::Down : TButtonGadget::Up, false );

ControlBarGadgets[ LMVIEW_CBG_SEP4              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ LMVIEW_CBG_SEGMENT           ]   = new TButtonGadgetDpi ( IDB_SEGMENT,           IDB_SEGMENT,            TButtonGadget::Command );
}


//----------------------------------------------------------------------------
void    TLinkManyView::SetupWindow ()
{
TBaseView::SetupWindow ();

                                        // do not retile if it's one more view TMdiClient
if ( GODoc->GetViewList() == this ) {
                                        // for myself, ready for next call
    SetStandSize ();
                                        // default tiling
    GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_BestFitSize, GroupTilingViews_RightSide ) );
    }
}


//----------------------------------------------------------------------------
                                        // count total number of lines
                                        // and max string length
TSize   TLinkManyView::GetBestSize ()
{
GLfloat             width           = 0;
GLfloat             height;


for ( int i=0; i < GODoc->GetNumEegDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetEegDoc( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumRoiDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetRoisDoc( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumXyzDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetXyzDoc( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumIsDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetIsDoc ( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumRisDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetRisDoc( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumSpDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetSpDoc ( i )->GetTitle() ) );

for ( int i=0; i < GODoc->GetNumMriDoc(); i++ )
    Maxed ( width, BFont->GetStringWidth ( (char *) GODoc->GetMriDoc( i )->GetTitle() ) );


width  += 4 * BFont->GetAvgWidth ();

Clipped ( width, (GLfloat) MinLMWindowWidth, (GLfloat) MaxLMWindowWidth );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( GetNumLines () == 0 )

    height  = 3 * BFont->GetHeight ();

else {
    height  =  ( GetNumLines () + 1 ) * ( BFont->GetHeight () + BFont->GetLineSpacing () );

    height  = AtLeast ( 2 * BFont->GetHeight (), height );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return TSize ( width, height );
}


//----------------------------------------------------------------------------
                                        // Resizing is mostly constrained to show at least all the lines
void    TLinkManyView::EvSize ( uint sizetype, const TSize &size )
{
if ( ! IsWindowMaximized () ) {

    TSize               bestsize        = GetBestSize ();
                                        // only allowing the actual width to be smaller than the optimal
    TSize               safesize        = TSize ( NoMore ( bestsize.X (), size.X() ), bestsize.Y () );

    TBaseView::EvSize ( sizetype, safesize );
                                        // setting the parent
    WindowSetSize ( safesize.X() + WindowClientOffset.X(), safesize.Y() + WindowClientOffset.Y() );
                                        // setting the child, too
    crtl::WindowSetSize ( this,  safesize.X(), safesize.Y() );
    }
}


//----------------------------------------------------------------------------
int    TLinkManyView::GetNumLines ()
{
return  GODoc->GetNumEegDoc ()
      + GODoc->GetNumRoiDoc () 
      + GODoc->GetNumXyzDoc () 
      + GODoc->GetNumIsDoc  ()
      + GODoc->GetNumRisDoc () 
      + GODoc->GetNumSpDoc  ()  
      + GODoc->GetNumMriDoc ();
}


//----------------------------------------------------------------------------
void    TLinkManyView::ViewIndexToPosition ( int i, TGLCoordinates<float> &p )
{
p.X     = 2 * BFont->GetAvgWidth ();
p.Y     = PaintRect.Height() - ( i + 1 ) * ( BFont->GetHeight () + BFont->GetLineSpacing () );
p.Z     = -0.5;
}


TBaseDoc*   TLinkManyView::PositionToDoc ( const TPoint &p )
{
int                 viewindex       = (double) p.Y () / ( BFont->GetHeight () + BFont->GetLineSpacing () ) - 0.5;

if ( viewindex < 0 || viewindex >= GetNumLines () )
    return 0;


for ( int i=0; i < GODoc->GetNumEegDoc(); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetEegDoc( i );

for ( int i=0; i < GODoc->GetNumRoiDoc(); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetRoisDoc( i );

for ( int i=0; i < GODoc->GetNumXyzDoc(); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetXyzDoc( i );

for ( int i=0; i < GODoc->GetNumIsDoc (); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetIsDoc( i );

for ( int i=0; i < GODoc->GetNumRisDoc(); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetRisDoc( i );

for ( int i=0; i < GODoc->GetNumSpDoc (); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetSpDoc( i );

for ( int i=0; i < GODoc->GetNumMriDoc(); i++, viewindex-- )
    if ( ! viewindex )
        return GODoc->GetMriDoc( i );


return  0;
}


//----------------------------------------------------------------------------
void    TLinkManyView::GLPaint ( int how, int /*renderingmode*/, TGLClipPlane* /*clipplane*/ )
{

if ( ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

    int                     viewindex   = 0;
    TGLCoordinates<float>   p;
//  int                     paragraph   = 0;
//  #define     deltaparagraph          5

                                        // drawing setup
//  GLSmoothEdgesOn ();
    GLColoringOn    ();
    GLWriteDepthOff ();


    glColor4f ( GLBASE_GROUPCOLOR_EEG );
    for ( int i=0; i < GODoc->GetNumEegDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetEegDoc( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }

//  if ( GODoc->GetNumEegDoc() )
//      paragraph += deltaparagraph;


    glColor4f ( GLBASE_GROUPCOLOR_ROI );
    for ( int i=0; i < GODoc->GetNumRoiDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetRoisDoc( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    glColor4f ( GLBASE_GROUPCOLOR_XYZ );
    for ( int i=0; i < GODoc->GetNumXyzDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetXyzDoc( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    glColor4f ( GLBASE_GROUPCOLOR_IS );
    for ( int i=0; i < GODoc->GetNumIsDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetIsDoc ( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    glColor4f ( GLBASE_GROUPCOLOR_RIS );
    for ( int i=0; i < GODoc->GetNumRisDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetRisDoc( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    glColor4f ( GLBASE_GROUPCOLOR_SP );
    for ( int i=0; i < GODoc->GetNumSpDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetSpDoc ( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    glColor4f ( GLBASE_GROUPCOLOR_MRI );
    for ( int i=0; i < GODoc->GetNumMriDoc(); i++, viewindex++ ) {
        ViewIndexToPosition ( viewindex, p );
        BFont->Print ( p.X, p.Y, p.Z, (char *) GODoc->GetMriDoc( i )->GetTitle(), TA_LEFT | TA_CENTERY );
        }


    if ( GetNumLines () == 0 ) {
        glColor4f ( GLBASE_GROUPCOLOR_OTHER );
        ViewIndexToPosition ( 0, p );
        BFont->Print ( PaintRect.Width() / 2, p.Y, p.Z, "<Drag & Drop files or .lm here>", TA_CENTER | TA_TOP );
        }


//  GLSmoothEdgesOff();
    GLColoringOff   ();
    GLWriteDepthOn  ();
    } // GLPaintOpaque && GLPaintOwner

}


//----------------------------------------------------------------------------
void    TLinkManyView::Paint( TDC &dc, bool /*erase*/ , TRect &rect )
{

PrePaint ( dc, rect, 0, 0, 0 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//AntialiasingPaint ();                    // not relevant, as window draws only text
GLPaint ( GLPaintNormal, RenderingUnchanged, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TLinkManyView::EvKeyDown (uint key, uint repeatCount, uint flags)
{
switch ( key ) {

    case    'A':

        if      (   ControlKey &&   ShiftKey )  CmSyncUtility ( CM_DESYNCBETWEENEEG );
        else if (   ControlKey && ! ShiftKey )  CmSyncUtility ( CM_SYNCBETWEENEEG   );
        else if ( ! ControlKey &&   ShiftKey )  CmSyncUtility ( CM_DESYNCALL        );
        else if ( ! ControlKey && ! ShiftKey )  CmSyncUtility ( CM_SYNCALL          );

        break;

    case    'C':
        CmCommandsCloning ();
        break;

    case    'F':
        CmGTV ( IDB_GTVFIT );
        break;

    case    'S':
        if ( ControlKey )
            CmSegment ();
        else
            CmGTV ( IDB_GTVSTANDSIZE );
        break;

    case    'T':
        CmGTV ( IDB_GTVKEEPSIZE );
        break;

    case    VK_ADD:
        CmAddToGroup ();
        break;

    case VK_ESCAPE:
        if ( CaptureMode == CaptureAddToGroup )
            CmAddToGroup ();
        else
            TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;

    default:
        TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;
    }
}


//----------------------------------------------------------------------------
void    TLinkManyView::EvLButtonDown ( uint, const TPoint &p )
{
if ( CaptureMode == CaptureAddToGroup ) {

    TBaseView*          view        = ClientToBaseView ( p );

    if ( ! view ) {

        CmAddToGroup ();                // stop adding to group mode

        return;
        }

    if ( GODoc->AddToGroup ( view->BaseDoc ) ) {

        StandSize   = GetBestSize ();

        EvSize ( 0, StandSize );

//      EEGDoc->NotifyFriendViews ( TFCursor->SentTo, vnNewTFCursor, (TParam2) TFCursor, this );

        CmAddToGroup ();                // ?For whatever reason, capture mode has been lost at that point, let's re-activate it until user explicitly gets out of it!
        }
    else
        CmAddToGroup ();                // stop adding to group mode
    }
}


//----------------------------------------------------------------------------
void    TLinkManyView::EvLButtonDblClk ( uint, const TPoint &p )
{
TBaseDoc*           doc             = PositionToDoc ( p );

if ( ! doc )
    return;


TBaseView*          view;
bool                wasiconic       = false;

for ( view = doc->GetViewList (); view != 0; view = doc->NextView ( view ) ) {
                                        // act upon non-proprietary views, or views belonging only to this group
    if ( view->GODoc && view->GODoc != GODoc ) continue;

    wasiconic = view->IsWindowMinimized ();

    if ( wasiconic )    view->WindowRestore  ();
    else                view->WindowMinimize ();
    }


if ( ! wasiconic )
    GetParentO()->SetFocus ();
}


//----------------------------------------------------------------------------
void    TLinkManyView::CmGTV ( owlwparam w )
{
if      ( w == IDB_GTVKEEPSIZE )    GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_Move     ,   GroupTilingViews_Insert ) );
else if ( w == IDB_GTVSTANDSIZE )   GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_StandSize,   GroupTilingViews_Insert ) );
else if ( w == IDB_GTVFIT )         GODoc->GroupTileViews ( CombineFlags ( GroupTilingViews_BestFitSize, GroupTilingViews_Insert ) );
}


//----------------------------------------------------------------------------
void    TLinkManyView::CmAddToGroup ()
{
if ( CaptureMode == CaptureNone ) {

    ::SetCursor ( *CartoolApplication->CursorAddToGroup );

    CaptureMouse ( Capture );

    MouseDC     = new TClientDC ( *this );

    CaptureMode = CaptureAddToGroup;
    }
else if ( CaptureMode == CaptureAddToGroup ) { // cancel the sync manoeuvre

    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
    }
    
ButtonGadgetSetState ( IDB_ADDTOGROUP, CaptureMode == CaptureAddToGroup );
}


//----------------------------------------------------------------------------
                                        // Old shortcut, used to run segmentation on all EEG from a TLinkManyDoc
void    TLinkManyView::CmSegment ()
{
GODoc->Segment ();
}


//----------------------------------------------------------------------------
void    TLinkManyView::CmSyncUtility ( owlwparam w )
{
GODoc->SyncUtility ( w );

GetParentO ()->SetFocus ();
}


//----------------------------------------------------------------------------
void    TLinkManyView::CmCommandsCloning ()
{
GODoc->CommandsCloning = ! GODoc->CommandsCloning;

ButtonGadgetSetState ( CM_COMMANDSCLONING, GODoc->CommandsCloning );

//GetViewMenu ()->CheckMenuItem ( CM_COMMANDSCLONING, MF_BYCOMMAND | ( GODoc->CommandsCloning ? MF_CHECKED : MF_UNCHECKED ) );
}


//----------------------------------------------------------------------------
bool    TLinkManyView::VnReloadData ( int what )
{
switch ( what ) {

  case EV_VN_RELOADDATA_DOCPOINTERS:
    Invalidate ( false );
    break;
    }

return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
