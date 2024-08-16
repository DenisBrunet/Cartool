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

#include    "TRoisView.h"

#include    "OpenGL.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TRoisView, TBaseView)

    EV_WM_GETMINMAXINFO,
    EV_WM_SIZE,
    EV_WM_ERASEBKGND,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
    EV_WM_KEYUP,

    EV_VN_VIEWDESTROYED,

END_RESPONSE_TABLE;


        TRoisView::TRoisView ( TRoisDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TBaseView ( doc, parent, group ), ROIDoc ( &doc )
{
GODoc       = 0;                        // force it to no-group -> 1 window is enough
                                        // let's have a smart start-up size
Attr.W      = max ( BFont->GetStringWidth ( "Max blablabla in the window" ),
                    BFont->GetStringWidth ( (char *) ROIDoc->GetTitle () ) )
              + 8 * BFont->GetAvgWidth ();

Attr.H      = 6.5 * BFont->GetHeight ();

StandSize   = TSize ( crtl::GetWindowWidth ( this ), crtl::GetWindowHeight ( this ) );

SetViewMenu ( new TMenuDescr ( IDM_ROI ) );


BackColor.Set ( 0, GLBASE_BACKCOLOR_PRINTING );
BackColor.Set ( 1, GLBASE_BACKCOLOR_PRINTING );


if ( ! ValidView () )
    NotOK();                // do not create the window (cancel from doc)
}


//----------------------------------------------------------------------------
void    TRoisView::CreateGadgets ()
{
NumControlBarGadgets    = ROISGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();
}


//----------------------------------------------------------------------------
void    TRoisView::GLPaint ( int how, int /*renderingmode*/, TGLClipPlane* /*clipplane*/ )
{
if ( ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

    char        buff[ 128 ];
    GLdouble    xpos = PaintRect.Width() / 2;
    GLdouble    ypos = PaintRect.Height() - BFont->GetHeight () / 2;
    GLdouble    zpos = -0.5;


                                        // drawing setup
//  GLSmoothEdgesOn ();
    GLColoringOn    ();
    GLWriteDepthOff ();


    glColor4f ( GLBASE_ROI_NAME );
    BFont->Print ( xpos, ypos, zpos, ROIDoc->ROIs->GetName (), TA_CENTER | TA_TOP );


    glColor4f ( GLBASE_ROI_INFO );

    ypos   -= 1.5 * BFont->GetHeight ();
    sprintf ( buff, "Type: %s", ROIDoc->ROIs->GetType () == RoiIndex ? "List of Indexes" : "List of 3D Coordinates" );
    BFont->Print ( xpos, ypos, zpos, buff, TA_CENTER | TA_TOP );


    ypos   -= BFont->GetHeight ();
    sprintf ( buff, "Dimension: %d", ROIDoc->ROIs->GetDimension () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_CENTER | TA_TOP );


    ypos   -= BFont->GetHeight ();
    sprintf ( buff, "Number of ROIs: %d", ROIDoc->ROIs->GetNumRois () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_CENTER | TA_TOP );


    ypos   -= BFont->GetHeight ();
    sprintf ( buff, "Elements actually used: %d", ROIDoc->ROIs->GetDimension () - ROIDoc->ROIs->GetAtomsNotSelected()->NumSet () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_CENTER | TA_TOP );


//  GLSmoothEdgesOff();
    GLColoringOff   ();
    GLWriteDepthOn  ();
    }
}


//----------------------------------------------------------------------------
void    TRoisView::Paint( TDC &dc, bool /*erase*/ , TRect &rect )
{

PrePaint ( dc, rect, 0, 0, 0 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//AntialiasingPaint ();                    // not needed because text only
GLPaint ( GLPaintNormal, RenderingUnchanged, 0 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
