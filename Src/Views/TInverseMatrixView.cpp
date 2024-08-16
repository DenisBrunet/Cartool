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

#include    "TInverseMatrixView.h"

#include    "OpenGL.h"

#include    "TInverseMatrixDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TInverseMatrixView, TBaseView)

//  EV_WM_GETMINMAXINFO,
//  EV_WM_SIZE,
//  EV_WM_KEYDOWN,
//  EV_WM_KEYUP,

    EV_COMMAND_AND_ID   ( CM_INVERSEAVERAGEBEFORE,  CmSetAveragingBefore ),
    EV_COMMAND_AND_ID   ( CM_INVERSEAVERAGEAFTER,   CmSetAveragingBefore ),
    EV_COMMAND_ENABLE   ( CM_INVERSEAVERAGEBEFORE,  CmPrecedenceBeforeEnable ),
    EV_COMMAND_ENABLE   ( CM_INVERSEAVERAGEAFTER,   CmPrecedenceAfterEnable  ),

END_RESPONSE_TABLE;


        TInverseMatrixView::TInverseMatrixView ( TInverseMatrixDoc& doc, TWindow* parent, TLinkManyDoc *group )
      : TBaseView ( doc, parent, group ), ISDoc ( &doc )
{
GODoc       = 0;                        // force it to no-group -> 1 window is enough
                                        // let's have a smart start-up size
Attr.W      = max ( BFont->GetStringWidth ( "Atomic result is Vectorial,  single float" ),
                    BFont->GetStringWidth ( (char *) ISDoc->GetTitle () ) )
              + 4 * BFont->GetAvgWidth ();

Attr.H      = 6.5 * BFont->GetHeight ();

StandSize   = TSize ( crtl::GetWindowWidth ( this ), crtl::GetWindowHeight ( this ) );

SetViewMenu ( new TMenuDescr (IDM_IS) );


BackColor.Set ( 0, GLBASE_BACKCOLOR_PRINTING );
BackColor.Set ( 1, GLBASE_BACKCOLOR_PRINTING );


if ( ! ValidView() )
    NotOK();                // do not create the window (cancel from doc)
}


//----------------------------------------------------------------------------
void    TInverseMatrixView::CreateGadgets ()
{
NumControlBarGadgets    = ISGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();
}


//----------------------------------------------------------------------------
void    TInverseMatrixView::GLPaint ( int how, int /*renderingmode*/, TGLClipPlane* /*clipplane*/ )
{
if ( ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

    char                buff[ 256 ];
    GLdouble            xpos            = 2 * BFont->GetAvgWidth ();
    GLdouble            ypos            = PaintRect.Height() - BFont->GetHeight () / 2;
    GLdouble            zpos            = -0.5;

                                        // drawing setup
//  GLSmoothEdgesOn ();
    GLColoringOn    ();
    GLWriteDepthOff ();


    glColor4f ( GLBASE_GROUPCOLOR_IS );
    sprintf ( buff, "%s", ISDoc->GetTitle () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


    ypos -= 1.5 * BFont->GetHeight ();
    glColor4f ( 0.00, 0.00, 0.00, 1.0 );
    sprintf ( buff, "%0d  Rows (Solution Points)", ISDoc->GetNumSolPoints () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


    ypos -= BFont->GetHeight ();
    glColor4f ( 0.00, 0.00, 0.00, 1.0 );
    sprintf ( buff, "%0d  Columns (Electrodes)", ISDoc->GetNumElectrodes () );
    BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


    ypos -= BFont->GetHeight ();
    glColor4f ( 0.00, 0.00, 0.00, 1.0 );
    if ( ISDoc->GetNumRegularizations () )
        sprintf ( buff, "%0d  Regularization%s", ISDoc->GetNumRegularizations (), StringPlural ( ISDoc->GetNumRegularizations () ) );
    else
        StringCopy ( buff, "No Regularization" );
    BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


    ypos -= BFont->GetHeight ();
    glColor4f ( 0.00, 0.00, 0.00, 1.0 );
    if ( ISDoc->IsVector ( AtomTypeUseCurrent ) )
        sprintf ( buff, "Atomic result is Vectorial,  " );
    else
        sprintf ( buff, "Atomic result is Scalar,  " );

    StringAppend ( buff, "Single Float" );

    BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


//  GLSmoothEdgesOff();
    GLColoringOff   ();
    GLWriteDepthOn  ();
    }
}


//----------------------------------------------------------------------------
void    TInverseMatrixView::Paint ( TDC &dc, bool /*erase*/ , TRect &rect )
{

PrePaint ( dc, rect, 0, 0, 0 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//AntialiasingPaint ();                    // not relevant because text only
GLPaint ( GLPaintNormal, RenderingUnchanged, 0 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TInverseMatrixView::CmSetAveragingBefore ( owlwparam w )
{
ISDoc->SetAveragingBefore ( w == CM_INVERSEAVERAGEBEFORE ? AverageBeforeInverse : AverageAfterInverse );

ISDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG );
}


void    TInverseMatrixView::CmPrecedenceBeforeEnable ( TCommandEnabler& tce )
{
tce.SetCheck ( ISDoc->GetAveragingBefore () == AverageBeforeInverse );
}


void    TInverseMatrixView::CmPrecedenceAfterEnable ( TCommandEnabler& tce )
{
tce.SetCheck ( ISDoc->GetAveragingBefore () == AverageAfterInverse );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
