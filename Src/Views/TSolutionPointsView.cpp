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

#include    "TSolutionPointsView.h"

#include    "Dialogs.Input.h"

#include    "OpenGL.h"

#include    "TTracksDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TSolutionPointsView, TBaseView)

    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONUP,
    EV_WM_MBUTTONDOWN,
    EV_WM_MBUTTONUP,
    EV_WM_RBUTTONDOWN,
    EV_WM_RBUTTONUP,
    EV_WM_MOUSEMOVE,
    EV_WM_TIMER,
    EV_WM_MOUSEWHEEL,

    EV_VN_NEWHIGHLIGHTED,

    EV_COMMAND          ( IDB_2OBJECT,          Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,           CmOrient ),
    EV_COMMAND          ( IDB_MAGNIFIER,        CmMagnifier ),
    EV_COMMAND          ( IDB_SURFACEMODE,      CmSetRenderingMode ),
    EV_COMMAND          ( CM_EXPTXT,            CmExportText ),

    EV_COMMAND_AND_ID   ( IDB_CUTPLANEX,        CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANEY,        CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANEZ,        CmSetCutPlane ),
    EV_COMMAND          ( IDB_SOLPSHOWNAMES,    CmSetShowNames ),

    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE0,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE1,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE2,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE3,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE4,  CmSetShiftDepthRange ),
END_RESPONSE_TABLE;


        TSolutionPointsView::TSolutionPointsView ( TSolutionPointsDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TBaseView ( doc, parent, group ), SPDoc ( &doc )

{
GODoc               = 0;                // force it to no-group -> 1 window is enough


Highlighted         = TSelection ( SPDoc->GetNumSolPoints (), OrderSorted );
Highlighted.Reset ();
Highlighted.SentTo  = 0;
Highlighted.SentFrom= 0;                // set to 0, as to be widely accepted


ShowLabels          = ShowLabelNone;
ShowTextColor       = TextColorLight;
RenderingMode       = PointsNormal;
CurrentDisplaySpace = DisplaySpace3D;
DepthRange.UserShift= 0;

Zoom                = CartoolApplication->AnimateViews ? 0.001 : 1;

const TBoundingBox<double>*     b   = SPDoc->GetBounding();

ModelCenter = b->GetCenter ();
                                       // take the radius of the whole data box
ModelRadius = DepthPositionRatio * AtLeast ( 1.0, b->AbsRadius () );

                                                                               // slight offset for init
ClipPlane[ 0 ]      = TGLClipPlane ( GL_CLIP_PLANE0, -1,  0,  0, ModelCenter.X + 0.10, b->XMin() - 1, b->XMax() + 1 );
ClipPlane[ 1 ]      = TGLClipPlane ( GL_CLIP_PLANE1,  0, -1,  0, ModelCenter.Y + 0.10, b->YMin() - 1, b->YMax() + 1 );
ClipPlane[ 2 ]      = TGLClipPlane ( GL_CLIP_PLANE2,  0,  0, -1, ModelCenter.Z + 0.10, b->ZMin() - 1, b->ZMax() + 1 );

ClipPlane[ 0 ].SetNone ();
ClipPlane[ 1 ].SetNone ();
ClipPlane[ 2 ].SetNone ();


MaterialSp    = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.33,  (GLfloat) 0.33,  (GLfloat) 0.00,  (GLfloat) 1.00,
                                      (GLfloat) 0.65,  (GLfloat) 0.65,  (GLfloat) 0.10,  (GLfloat) 0.45,
                                      (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

MaterialSp2   = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 0.50,  (GLfloat) 1.00,
                                      (GLfloat) 0.25,  (GLfloat) 0.25,  (GLfloat) 0.95,  (GLfloat) 0.45,
                                      (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

MaterialHi    = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.15,  (GLfloat) 0.00,  (GLfloat) 0.15,  (GLfloat) 1.00,
                                      (GLfloat) 0.66,  (GLfloat) 0.10,  (GLfloat) 0.66,  (GLfloat) 0.45,
                                      (GLfloat) 1.00,  (GLfloat) 1.00,  (GLfloat) 0.00,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     10.00 );

Fog         = TGLFog<GLfloat>   ( ModelRadius * FogDefaultNear,
                                  ModelRadius * FogDefaultFar * 1.5,   // easy on the fog...
                                  GLBASE_FOGCOLOR_NORMAL );


Xaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    b->XMax() + b->MaxSize () * 0.08, 0, 0,
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Yaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, b->YMax() + b->MaxSize () * 0.08, 0,
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Zaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, 0, b->ZMax() + b->MaxSize () * 0.08,
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 1.00 );

OriginRadius    = 0.20 * SPDoc->GetPointRadius ();


                                        // disable all tables
ColorTable.SetTableAllowed  ( NoTables );
                                        // then re-enable these
ColorTable.SetTableAllowed  ( SignedColorTable_MagentaBlueCyanGrayGreenYellowRed );

ColorTable.Set              ( SignedColorTable_MagentaBlueCyanGrayGreenYellowRed );


double      maxValue = SPDoc->GetNumSolPoints ();
SetScalingLimits ( maxValue * 1e-3, maxValue );
SetScalingContrast ( 0.00 );
SetScaling ( maxValue );


Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );
Attr.W      = Attr.H /** WindowHeightToWidthRatio*/;
StandSize   = TSize ( SolPointsWindowSize, SolPointsWindowSize );


SetViewMenu ( new TMenuDescr ( IDM_SP ) );


if ( ! ValidView() )
    NotOK();                            // do not create the window (cancel from doc)
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::CreateGadgets ()
{
NumControlBarGadgets    = SPGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();

ControlBarGadgets[ SPGLVIEW_CBG_SEP1        ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ SPGLVIEW_CBG_SHOWNAMES   ]   = new TButtonGadgetDpi ( IDB_SOLPSHOWNAMES, IDB_SOLPSHOWNAMES,  TButtonGadget::NonExclusive, true, ShowLabels ? TButtonGadget::Down : TButtonGadget::Up, false);

ControlBarGadgets[ SPGLVIEW_CBG_SEP2        ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ SPGLVIEW_CBG_CUTPLANEX   ]   = new TButtonGadgetDpi ( IDB_CUTPLANEX,     IDB_CUTPLANEX,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[ SPGLVIEW_CBG_CUTPLANEY   ]   = new TButtonGadgetDpi ( IDB_CUTPLANEY,     IDB_CUTPLANEY,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[ SPGLVIEW_CBG_CUTPLANEZ   ]   = new TButtonGadgetDpi ( IDB_CUTPLANEZ,     IDB_CUTPLANEZ,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::SetupWindow ()
{
TBaseView::SetupWindow ();

if ( CartoolApplication->AnimateViews )
    AnimFx.Start ( TimerStartup, 10, 15 );
}


//----------------------------------------------------------------------------
bool    TSolutionPointsView::ModifyPickingInfo ( TPointFloat& Picking, char *buff )
{
ClearString ( buff );

double              nearlimit       = Clip ( SPDoc->GetMedianDistance () * PointNearFactor, 
                                             PointRadiusRatio[ RenderingMode ] * 1.50, 
                                             SPDoc->GetMedianDistance () * 0.49 );
int                 sp              = SPDoc->GetNearestElementIndex ( Picking, nearlimit );

                                        // not close enough to a solution point?
if ( sp < 0 )
    return false;

                                        // force the coordinates of the point instead of the pointer
Picking     = SPDoc->GetPoints ()[ sp ];

                                        // could add the filename here, but most of the time, there is only one at a time...
StringCopy      ( buff, "Solution Point Name: ", SPDoc->GetSPName ( sp ) );
StringAppend    ( buff, NewLine );

StringAppend    ( buff, "Solution Point Index: #" );
IntegerToString ( StringEnd ( buff ), sp + 1 );


return true;
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode


if ( ! ( renderingmode != PointsShowNone || ShowLabels ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGLClipPlane*       clipplane;

                                        // take clipping from parameters, if both caller and owner share some clipping
if ( ( (bool) otherclipplane[ 0 ] || (bool) otherclipplane[ 1 ] || (bool) otherclipplane[ 2 ] )
  && ( (bool) ClipPlane     [ 0 ] || (bool) ClipPlane     [ 1 ] || (bool) ClipPlane     [ 2 ] )
  || ( how & GLPaintForceClipping ) )

    clipplane   = otherclipplane;
else                                    // otherwise, use local clipping
    clipplane   = ClipPlane;


                                        // Scanning the clipping planes
double              cx              = clipplane[ 0 ].GetAbsPosition ();
double              cy              = clipplane[ 1 ].GetAbsPosition ();
double              cz              = clipplane[ 2 ].GetAbsPosition ();
int                 ncutplanes      = (bool) clipplane[ 0 ] + (bool) clipplane[ 1 ] + (bool) clipplane[ 2 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numsp           = SPDoc->GetNumSolPoints ();
                                        // Fast points are now smoothed, which invokes "transparent" flags. These unfortunately interfere badly with linked display, so we need to sort out a few cases.
bool                quality         = ncutplanes                // clipping planes can splurge into quality
                                   || numsp <= 150000           // fast enough up to this amount of solid spheres
                                   || ( how & GLPaintLinked )   // if embedded, force to solid spheres, as volumes display (f.ex.) will not like the smooth "transparent" points
                                   || (bool) Using;             // if embedding, "transparent" points will not merge gracefully with other solid spheres
double              pointradius;


if ( quality )
                                        // plotting real-world spheres
    pointradius = SPDoc->GetPointRadius ();

else {
                                        // plotting screen-coordinates points -> need to know current point size
    TPointFloat         v1;
    TPointFloat         v2 ( SPDoc->GetPointRadius () );

                                        // project a centered point and the same point + our radius offset
    if ( how & GLPaintOwner ) {
        GLGetScreenCoord ( v1, ViewportOrgSize, MatProjection );
        GLGetScreenCoord ( v2, ViewportOrgSize, MatProjection );
        }
    else {  // current caller transform
        GLGetScreenCoord ( v1 );
        GLGetScreenCoord ( v2 );
        }
                                        // add-hoc formula
    pointradius = ( v2 - v1 ).Norm () * SqrtTwo;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 xmin            = SPDoc->GetBounding()->XMin();
int                 xmax            = SPDoc->GetBounding()->XMax();
int                 ymin            = SPDoc->GetBounding()->YMin();
int                 ymax            = SPDoc->GetBounding()->YMax();
int                 zmin            = SPDoc->GetBounding()->ZMin();
int                 zmax            = SPDoc->GetBounding()->ZMax();

//double            maxmax          = SPDoc->GetBounding()->MaxSize();
const TPointFloat*  tosp;
double              meddistance     = SPDoc->GetMedianDistance ();

//double            mind            = ( meddistance+ SPDoc->GetBounding()->BoxDiameter() / 50 ) / 4;
//double            mind            = SPDoc->GetBounding()->GetZExtent() * 0.015;
double              mind            = meddistance * SolutionPointsMinDistanceRatio;

double              p;
char                buff[ 16 ];


ColorTable.SetParameters ( ScalingPMax, ScalingNMax, 1.0, 1.0 );

                                        // add a global shift
if ( DepthRange.UserShift ) {
    DepthRange.ShiftCloser ( DepthRange.UserShift );
    DepthRange.GLize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the solution points
if ( renderingmode != PointsShowNone && ( how & GLPaintOpaque ) ) {

    if ( quality ) {

        GLMaterialOn        ();

//      if ( how & GLPaintOwner )
            MaterialSp.GLize ();
//      else
//          MaterialSp2.GLize ();
        }
    else
        GLColoringOn        ();


    if ( ncutplanes )
        GLFogOff            ();


    double              r               = pointradius * PointRadiusRatio[ renderingmode ];


    TGLBillboardSphere *tobbsphere = Outputing() /*|| VkKey ( 'A' )*/ ? &BbHighSphere : &BbLowSphere;

    if ( quality )
        tobbsphere->GLize ();           // will do all the job
    else
        GLSmoothPointsOn ();

                                        // draw here
    TGLColor<GLfloat>   glcol;
    tosp    = SPDoc->GetPoints ();

    for ( int sp = 0; sp < numsp; sp++, tosp++ ) {


        if ( ncutplanes 
          && ! (    (bool) clipplane[0] && fabs ( tosp->X - cx ) <= mind
                 || (bool) clipplane[1] && fabs ( tosp->Y - cy ) <= mind
                 || (bool) clipplane[2] && fabs ( tosp->Z - cz ) <= mind ) )

                continue;

                                        // full coloring
        if ( ( how & GLPaintOwner ) && ! (bool) Using ) {

            if ( Highlighted[ sp ] ) {

                if ( quality ) {

                    MaterialHi.GLize ();    // set all parameters

                    tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, r * HighlightRadiusBoost[ renderingmode ] );

                    MaterialSp.GLize ();    // restore regular parameters
                    }
                else {
                                    // approximative color
                    MaterialHi.GetDiffuse ().GLize ();

                    glPointSize ( r * HighlightRadiusBoost[ renderingmode ] );

                    glBegin ( GL_POINTS );
                    glVertex3fv ( (float *) tosp );
                    glEnd();
                    }
                } // if Highlighted
            else {

                ColorTable.GetColorIndex ( ( sp << 1 ) - numsp, glcol );

                if ( quality ) {

                    glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, glcol );

                    tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, r );
                    }
                else {

                    glcol.GLize ();

                    glPointSize ( r );

                    glBegin ( GL_POINTS );
                    glVertex3fv ( (float *) tosp );
                    glEnd();
                    }
                } // if ! Highlighted
            } // owner & full colors
        else {                      // simpler coloring for linked windows

            if ( Highlighted[ sp ] ) {

                if ( quality ) {

                    MaterialHi.GLize ();    // set all parameters

                    tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, r * HighlightRadiusBoost[ renderingmode ] );
                    }
                else {

                    MaterialHi.GetDiffuse ().GLize ();

                    glPointSize ( r * HighlightRadiusBoost[ renderingmode ] );

                    glBegin ( GL_POINTS );
                    glVertex3fv ( (float *) tosp );
                    glEnd();
                    }
                } // if Highlighted
            else {

                if ( quality ) {

                    if ( ( how & GLPaintOwner ) )       MaterialSp .GLize ();
                    else                                MaterialSp2.GLize ();

                    tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, r );
                    }
                else {

                    if ( ( how & GLPaintOwner ) )       MaterialSp .GetDiffuse ().GLize ();
                    else                                MaterialSp2.GetDiffuse ().GLize ();

                    glPointSize ( r );

                    glBegin ( GL_POINTS );
                    glVertex3fv ( (float *) tosp );
                    glEnd();
                    }
                } // if ! Highlighted
            } // linked windows

        } // for sp


    if ( quality )
        tobbsphere->unGLize ();
    else
        GLSmoothPointsOff ();

    GLFogOn             ();
//  glDisable ( GL_CULL_FACE );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw names
if ( ShowLabels && ( how & GLPaintTransparent )
  && ( numsp < 1000 
    || ncutplanes ) ) {

    GLColoringOn        ();
//  GLWriteDepthOff     ();
    GLFogOff            ();
//  GLBlendOn           ();

//  double  glh  = meddistance / 2;
//  double  glh2 = glh / 2;

    double              x,  y,  z;
    int                 alternate       = -1;

//    TextColor.GLize ( how & GLPaintOwner ? Outputing() ^ ( ShowTextColor == TextColorDark )? 1 : 0
//                                         : Outputing() ^ ( ShowTextColor == TextColorDark )? 3 : 2 );

    TextColor.GLize ( how & GLPaintOwner ? 0 : 2 );

    SFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

    char                label[ ElectrodeNameSize ];
    char                buff [ ElectrodeNameSize ];


    tosp    = SPDoc->GetPoints ();

    for ( int sp = 0; sp < numsp; sp++, tosp++ ) {

                                        // !this specific mode will show the names at the intersection of clipping planes only - not the union of the planes!
        if ( (bool) clipplane[ 0 ] && fabs ( tosp->X - cx ) > mind )    continue;
        if ( (bool) clipplane[ 1 ] && fabs ( tosp->Y - cy ) > mind )    continue;
        if ( (bool) clipplane[ 2 ] && fabs ( tosp->Z - cz ) > mind )    continue;

        x   = tosp->X;
        y   = tosp->Y;
        z   = tosp->Z;
                                        // shift "depth" to see the names on one side only
                                        // shift checkerboardly in the "plane" to avoid overlaps
/*
        if      ( (bool) clipplane[0] ) {
            x -= glh;
            if ( ((int) ( z / meddistance )) & 1 )
                y += glh2;
            else
                y -= glh2;
            }
        else if ( (bool) clipplane[1] ) {
            y -= glh;
            if ( ((int) ( z / meddistance )) & 1 )
                x += glh2;
            else
                x -= glh2;
            }
        else if ( (bool) clipplane[2] ) {
            z -= glh;
            if ( ((int) ( x / meddistance )) & 1 )
                y += glh2;
            else
                y -= glh2;
            }
*/
        if      ( (bool) clipplane[0] )     alternate = ( ((int) ( y / meddistance )) & 1 ) ^ ( ((int) ( z / meddistance )) & 1 );
        else if ( (bool) clipplane[1] )     alternate = ( ((int) ( x / meddistance )) & 1 ) ^ ( ((int) ( y / meddistance )) & 1 );
        else if ( (bool) clipplane[2] )     alternate = ( ((int) ( x / meddistance )) & 1 ) ^ ( ((int) ( y / meddistance )) & 1 );


        if      ( ShowLabels == ShowLabelText  )    StringCopy ( label, SPDoc->GetSPName ( sp ) );
        else if ( ShowLabels == ShowLabelIndex )    StringCopy ( label, "#", IntegerToString ( buff, sp + 1 ) );


//      SFont->Print ( x, y, z, label, TA_CENTER | TA_CENTERY | TA_BOX );

        SFont->Print ( x, y, z, label,
                       TA_CENTER | ( alternate == -1 ? TA_CENTERY : alternate ? TA_TOP : TA_BOTTOM ) | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

        } // for sp


    GLColoringOff       ();
//  GLWriteDepthOn      ();
    GLFogOn             ();
//  GLBlendOff          ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque or transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // outline clipping planes
if (    ( how & GLPaintOwner ) 
     && ( how & GLPaintOpaque )
     && ncutplanes ) {

    GLLinesModeOn       ();

    DepthRange.ShiftCloser ();
    DepthRange.GLize ();

    glColor4f ( 0.50, 0.50, 0.50, 1.00 );


    if ( (bool) clipplane[ 2 ] ) {
        p = clipplane[ 2 ].GetAbsPosition();

        if ( ShiftAxis == ShiftZ )
            glColor4f ( 0.00, 0.00, 1.00, 1.00 );

        sprintf ( buff, "Z = %g", p );
        SFont->Print ( xmax, ymax, p, buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

        Prim.Draw3DWireFrame ( xmin, xmax, ymin, ymax, p, p, ShiftAxis == ShiftZ ? 3 : 1 );
        }


    if ( ShiftAxis != ShiftNone )
        glColor4f ( 0.50, 0.50, 0.50, 1.00 );


    if ( (bool) clipplane[ 1 ] ) {
        p = clipplane[ 1 ].GetAbsPosition();

        if ( ShiftAxis == ShiftY )
            glColor4f ( 0.00, 1.00, 0.00, 1.00 );

        sprintf ( buff, "Y = %g", p );
        SFont->Print ( xmax, p, zmax, buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

        Prim.Draw3DWireFrame ( xmin, xmax, p, p, zmin, zmax, ShiftAxis == ShiftY ? 3 : 1 );
        }


    if ( ShiftAxis != ShiftNone )
        glColor4f ( 0.50, 0.50, 0.50, 1.00 );


    if ( (bool) clipplane[ 0 ] ) {
        p = clipplane[ 0 ].GetAbsPosition();

        if ( ShiftAxis == ShiftX ) {
            glLineWidth ( 3.0 );
            glColor4f ( 1.00, 0.00, 0.00, 1.00 );
            }

        sprintf ( buff, "X = %g", p );
        SFont->Print ( p, ymax, zmax, buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

        Prim.Draw3DWireFrame ( p, p, ymin, ymax, zmin, zmax, ShiftAxis == ShiftX ? 3 : 1 );
        }


    GLLinesModeOff      ();

    DepthRange.ShiftFurther ();
    DepthRange.GLize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // reset
if ( DepthRange.UserShift ) {
    DepthRange.ShiftFurther ( DepthRange.UserShift );
    DepthRange.GLize ();
    }
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::Paint ( TDC &dc, bool /*erase*/ , TRect &rect )
{

double              radius  = AtLeast ( 1.0, SPDoc->GetBounding()->MaxSize () / 2 * 0.85 * ExtraSize3D );

PrePaint ( dc, rect, radius, radius, ModelRadius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

//glShadeModel ( Outputing() ? GL_SMOOTH : GL_FLAT );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Light0.GLize ();
Light1.GLize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glTranslated ( 0, 0, -ModelRadius );

ModelRotMatrix.GLize ();

glTranslated ( -ModelCenter.X, -ModelCenter.Y, -ModelCenter.Z );

                                        // save this matrix for external drawings
MatProjection.CopyProjection ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.GLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AntialiasingPaint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the axis unclipped
if ( /*! Outputing() &&*/ ShowAxis )
    DrawAxis ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.unGLize();

                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw orientation
    if ( ShowOrientation )
        DrawOrientation ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show informations
    if ( ShowInfos /*&& ! Outputing ()*/ ) {
        TextColor.GLize ( Outputing() ? 1 : 0 );

        double              xpos            = PrintLeftPos;
        double              ypos            = PrintTopPos( PaintRect.Height () );
        double              zpos            = PrintDepthPosBack;
        double              ydelta          = PrintVerticalStep ( SFont );
        char                buff [ 256 ];
//      double             *voxsize         = SPDoc->GetVoxelSize ();

                                        // Dimension info
        sprintf ( buff, "%0d  points", SPDoc->GetNumSolPoints () );
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

                                        // Data type
        ypos   -= 1.5 * ydelta;
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetContentTypeName ( buff ), TA_LEFT | TA_TOP );

        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetAtomTypeName ( AtomTypeUseCurrent ), TA_LEFT | TA_TOP );

                                        // Size
                                                         // should be [mm], but currently it is [Voxel]
        sprintf ( buff, "Content size:  %.1lf  %.1lf  %.1lf [Voxel]", SPDoc->GetBounding ()->GetXExtent (), SPDoc->GetBounding ()->GetYExtent (), SPDoc->GetBounding ()->GetZExtent () );
        ypos   -= 1.5 * ydelta;
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );
        } // ShowInfos


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ResetWindowCoordinates ();
    } // window mode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvKeyDown (uint key, uint repeatCount, uint flags)
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {

    case 'G':
        CmMagnifier ();
        break;

    case 'O':
        CmOrient ();
        break;

    case 'R':
        CmSetRenderingMode ();
        break;

    case 'L':
        Cm2Object ();
        break;

    case 'N':
        CmSetShowNames ();
        break;

    case 'X':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANEX );
        else {
            ModelRotMatrix.RotateY ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

    case 'Y':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANEY );
        else {
            ModelRotMatrix.RotateX ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

    case 'Z':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANEZ );
        else {
            ModelRotMatrix.RotateZ ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

/*
    case VK_LEFT:
        ModelRotMatrix.RotateY ( -RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case VK_RIGHT:
        ModelRotMatrix.RotateY (  RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case VK_UP:
        ModelRotMatrix.RotateX ( -RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case VK_DOWN:
        ModelRotMatrix.RotateX (  RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;
*/

//  default:
//      TBaseView::EvKeyDown ( key, repeatCount, flags );
//      break;
    }
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvTimer ( uint timerId )
{
double              ta1;


switch ( timerId ) {

    case TimerStartup:

        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx ) {

            AnimFx.Stop ();

            Zoom    = 1;

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        ta1     = AnimFx.GetPercentageCompletion();
        Zoom    = sqrt ( ta1 );

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;


    default:

        TBaseView::EvTimer ( timerId );
    }
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& p )
{
TBaseView::EvMouseWheel ( modKeys, zDelta, p );
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvLButtonDown ( uint i, const TPoint &p )
{
MousePos   = p;

                                        // GL biz?
if      ( CaptureMode == CaptureGLLink || CaptureMode == CaptureGLMagnify ) {

    GLLButtonDown ( i, p );

    return;
    }

else if ( /*ControlKey ||*/  VkKey ( MouseMiddeClicksurrogate ) ) {  // make an alias Ctrl-Left = Middle

    EvMButtonDown ( i, p );

    return;
    }

else {
    LButtonDown = true;

    CaptureMode = CaptureLeftButton;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvLButtonUp ( uint i, const TPoint &p )
{
MouseAxis           = MouseAxisNone;


if      ( CaptureMode == CaptureGLSelect ) { // make an alias Ctrl-Left = Middle

    EvMButtonUp ( i, p );

    return;
    }

else if ( CaptureMode == CaptureLeftButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


LButtonDown = false;

CaptureMouse ( Release );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvMButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos    = p;

MButtonDown = true;

CaptureMode = CaptureGLSelect;

CaptureMouse ( Capture );

EvMouseMove ( i, p );

                                        // resetting everything can only be done at clicking time
if ( Picking.IsNull () ) {

    Highlighted.Reset ();

    SPDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

    ShowNow ();
    }
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvMButtonUp (uint, const TPoint &/*p*/ )
{
MouseAxis           = MouseAxisNone;


if      ( CaptureMode == CaptureGLSelect )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


MButtonDown = false;

CaptureMouse ( Release );

GetParentO()->SetCaption ( BaseDoc->GetTitle() );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvRButtonDown ( uint, const TPoint& p )
{
if ( CaptureMode != CaptureNone )
    return;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );

MousePos   = p;
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvRButtonUp (uint, const TPoint& )
{
MouseAxis   = MouseAxisNone;


if ( CaptureMode == CaptureRightButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


RButtonDown = false;
ShiftAxis   = ShiftNone;

CaptureMouse ( Release );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::EvMouseMove ( uint i, const TPoint &p )
{
int                 dx              = p.X() - MousePos.X();
int                 dy              = p.Y() - MousePos.Y();
//int               adx             = abs ( dx );
//int               ady             = abs ( dy );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( RButtonDown ) {

    if ( ! ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) )
        return;

    double  k   = (double) SPDoc->GetBounding()->MaxSize() / GetClientRect().Height() * 1.7;

    if ( fabs ( (double) dx ) + fabs ( (double) dy ) < 10 && ShiftAxis == ShiftNone )
        return;

                                        // compute the 3 ratios, for each clipping plane
    double              rat[ 3 ];

    rat[ 0 ]  = (bool) ClipPlane[0] ? ClipPlane[0].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;
    rat[ 1 ]  = (bool) ClipPlane[1] ? ClipPlane[1].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;
    rat[ 2 ]  = (bool) ClipPlane[2] ? ClipPlane[2].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;

    if ( ShiftAxis == ShiftNone ) {
                                        // ratios are all 0 -> perpendicular to the single axis
        if ( ! ( rat[ 0 ] || rat[ 1 ] || rat[ 2 ] ) )
            ShiftAxis = (ShiftAxisEnum) ( (bool) ClipPlane[1] + ( (bool) ClipPlane[2] ) * 2 );
        else                        // who has got the max participation?
            ShiftAxis = fabs(rat[ 0 ]) > fabs(rat[ 1 ]) ? fabs(rat[ 0 ]) > fabs(rat[ 2 ])   ? ShiftX 
                                                                                            : ShiftZ 
                                                        : fabs(rat[ 2 ]) > fabs(rat[ 1 ])   ? ShiftZ 
                                                                                            : ShiftY;
        }

                                        // move something!
    if ( rat[ ShiftAxis ] == 0 )
        rat[ ShiftAxis ] =  dy;

                                        // shift it
    ClipPlane[ ShiftAxis ].Shift ( rat[ ShiftAxis ] * k );


	LastMouseMove       = TPoint ( dx, dy );
	MousePos            = p;

    Invalidate ( false );
    }
else
    GLMouseMove ( i, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( MButtonDown ) {
    static int          lasthit         = -1;
    static ulong        before          = 0;
    ulong               now             = GetWindowsTimeInMillisecond ();


    if ( now - before < 50 || Picking.IsNull () )
        return;


    double              nearlimit       = Clip ( SPDoc->GetMedianDistance () * 0.50 /*PointNearFactor*/, PointRadiusRatio[ RenderingMode ] * 1.50, SPDoc->GetMedianDistance () * 0.49 );
    int                 hit             = SPDoc->GetNearestElementIndex ( Picking, nearlimit );

    if ( hit >= 0 ) {

//      if ( now - before < 50 || lasthit != -1 && lasthit == hit && ( now - before ) * ( adx + ady ) < 6000 )
        if ( lasthit != -1 && lasthit == hit && now - before < MouseMoveHitDelay )
            return;

        before  = now;

        Highlighted.Invert ( hit );

/*                                        // temp code to highlight the 26 neirghbors
        double              step                = SPDoc->GetMedianDistance ();
//      double              neighborradius      = step * Neighborhood[ Neighbors26 ].MidDistanceCut;
        double              neighborradius6     = step * Neighborhood[ Neighbors6  ].MidDistanceCut;    // get neighborhood-adaptive distance thresholds
        double              neighborradius18    = step * Neighborhood[ Neighbors18 ].MidDistanceCut;
        double              neighborradius26    = step * Neighborhood[ Neighbors26 ].MidDistanceCut;
        bool                isn6;
        bool                isn18;
        bool                isn26;
        double              d;
        int                 numsp           = SPDoc->GetNumSolPoints ();

        TPoints&            sp          = SPDoc->GetPoints ();


        for ( int spi = 0; spi < numsp; spi++ ) {

            if ( spi == hit )    continue;

            d               = ( sp[ spi ] - sp[ hit ] ).Norm ();

            isn6            = d > 0                && d <= neighborradius6;
            isn18           = d > neighborradius6  && d <= neighborradius18;
            isn26           = d > neighborradius18 && d <= neighborradius26;

            if ( isn6  )    Highlighted.Set ( spi, Highlighted[ hit ] );
            if ( isn18 )    Highlighted.Set ( spi, Highlighted[ hit ] );
            if ( isn26 )    Highlighted.Set ( spi, Highlighted[ hit ] );
            }
*/

        SPDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

        ShowNow ();
        }


    lasthit = hit;
    }
}


//----------------------------------------------------------------------------
bool    TSolutionPointsView::VnNewHighlighted ( const TSelection *sel )
{
        // message can be either for electrodes or SPs
if ( abs ( sel->Size () - Highlighted.Size () ) > NumPseudoTracks /*|| sel->SentFrom && sel->SentFrom != LinkedViewId*/ )
    return true;                        // not for me !


if ( Highlighted != *sel ) {
    Highlighted = *sel;

    if ( RenderingMode != PointsShowNone )
        ShowNow ();
    }


return  true;
}


//----------------------------------------------------------------------------
void    TSolutionPointsView::CmSetRenderingMode ()
{
if ( ShiftKey )
    RenderingMode   = ( RenderingMode + PointsNumRendering - 1 ) % PointsNumRendering;
else
    RenderingMode   = ( RenderingMode + 1 ) % PointsNumRendering;

Invalidate ( false );

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


void    TSolutionPointsView::CmSetCutPlane ( owlwparam wparam )
{
int                 p               = wparam - IDB_CUTPLANEX;
ClipPlaneDirection  num             = (ClipPlaneDirection) ( (bool) Using ? NumClipPlane : 2 );

                                        // toggles to next state
ClipPlane[ p ].Set ( NextState ( ClipPlane[ p ].GetMode (), num, ShiftKey ) );


ButtonGadgetSetState ( IDB_CUTPLANEX + p, ClipPlane[ p ] );

Invalidate ( false );
}


void    TSolutionPointsView::CmSetShiftDepthRange ( owlwparam w )
{
TBaseView::CmSetShiftDepthRange ( w );

Invalidate ( false );
}


void    TSolutionPointsView::CmSetShowNames ()
{
ShowLabels      = (ShowLabelEnum) ( ( ShowLabels + ( ShiftKey ? NumShowLabels - 1 : 1 ) ) % NumShowLabels );

ButtonGadgetSetState ( IDB_SOLPSHOWNAMES, ShowLabels );

Invalidate ( false );

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
