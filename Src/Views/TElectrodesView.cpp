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

#include    <owl/pch.h>

#include    "TElectrodesView.h"

#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TArray2.h"
#include    "OpenGL.h"

#include    "TExportTracks.h"

#include    "TLinkManyDoc.h"
#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TElectrodesView, TBaseView)

//  EV_WM_NOTIFY,
//  EV_WM_GETMINMAXINFO,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
//  EV_WM_KEYUP,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONUP,
    EV_WM_MBUTTONDOWN,
    EV_WM_MBUTTONUP,
    EV_WM_RBUTTONDOWN,
    EV_WM_RBUTTONUP,
    EV_WM_MOUSEMOVE,
//  EV_WM_CAPTURECHANGED,
    EV_WM_TIMER,
    EV_WM_MOUSEWHEEL,

    EV_VN_NEWBADSELECTION,
    EV_VN_NEWHIGHLIGHTED,

    EV_COMMAND          ( IDB_2OBJECT,          Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,           CmOrient ),
    EV_COMMAND          ( IDB_MAGNIFIER,        CmMagnifier ),
    EV_COMMAND          ( IDB_SHOWELEC,         CmSetShowElectrodes ),
    EV_COMMAND          ( IDB_SHOWELNAMES,      CmSetShowElectrodesNames ),
    EV_COMMAND          ( IDB_SHOWTRIANGLES,    CmSetShowTriangles ),

    EV_COMMAND_AND_ID   ( IDB_FLIPX,            CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPY,            CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPZ,            CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPXY,           CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPYZ,           CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPXZ,           CmFlip ),

    EV_COMMAND          ( IDB_SURFACEMODE,      CmSetShowTriangles ),
    EV_COMMAND          ( IDB_FLATVIEW,         CmSetFlatView ),
    EV_COMMAND_ENABLE   ( IDB_FLATVIEW,         CmSetFlatViewEnable ),

    EV_COMMAND_AND_ID   ( IDB_CUTPLANEX,        CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANEY,        CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANEZ,        CmSetCutPlane ),

    EV_COMMAND_AND_ID   ( CM_EXPORTXYZKEEPSEL,  CmExportXyz ),
    EV_COMMAND_AND_ID   ( CM_EXPORTXYZREMOVESEL,CmExportXyz ),
    EV_COMMAND          ( CM_EXPORTXYZDELAUNAY, CmExportDelaunay ),

    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE0,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE1,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE2,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE3,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE4,  CmSetShiftDepthRange ),

END_RESPONSE_TABLE;


        TElectrodesView::TElectrodesView ( TElectrodesDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TBaseView ( doc, parent, group ), XYZDoc ( &doc )

{
//GODoc             = 0;                    // force it to no-group -> 1 window is enough
LinkedViewId        = GODoc ? GODoc->LastEegViewId : 0;  // messages sent/received to


ShowElectrodes      = XYZDoc->GetNumElectrodes () <= 512 ? PointsNormal : PointsShowNone;
ShowLabels          = ShowLabelNone;
ShowTextColor       = TextColorLight;


Orientation         = DefaultPotentialOrientation - 1;


mtgNum              = 0;
mtgFrom = mtgTo     = 0;
mtgCurrent          = 0;


Bad                 = TSelection ( XYZDoc->GetNumElectrodes (), OrderSorted );
Bad.Reset();
Bad.SentTo          = 0;
Bad.SentFrom        = 0;                // set to 0, as to be widely accepted

                                        // get any setup from the original document
Bad                 = XYZDoc->GetBadElectrodes();


Highlighted         = TSelection ( XYZDoc->GetNumElectrodes (), OrderSorted );
Highlighted.Reset ();
Highlighted.SentTo  = 0;
Highlighted.SentFrom= 0;                // set to 0, as to be widely accepted


RenderingMode       = XyzSolidSmooth;
CurrentDisplaySpace = DisplaySpace3D;
DepthRange.UserShift= 0;


const TBoundingBox<double>*     b3d     = XYZDoc->GetBounding ( DisplaySpace3D        );
const TBoundingBox<double>*     b2d     = XYZDoc->GetBounding ( DisplaySpaceProjected );


ModelCenter         = b3d->GetCenter ();
                                       // take the radius of the whole data box
ModelRadius         = DepthPositionRatio * AtLeast ( 1.0, b3d->AbsRadius () );


ProjModelCenter     = b2d->GetCenter ();

ProjModelCenter.Z   = b2d->ZMin();

ProjModelRadius     = DepthPositionRatio * AtLeast ( 1.0, b2d->AbsRadius() );

                                                                               // slight offset for init
ClipPlane[ 0 ]      = TGLClipPlane ( GL_CLIP_PLANE0, -1,  0,  0, ModelCenter.X + 0.10, b3d->XMin() - 1, b3d->XMax() + 1 );
ClipPlane[ 1 ]      = TGLClipPlane ( GL_CLIP_PLANE1,  0, -1,  0, ModelCenter.Y + 0.10, b3d->YMin() - 1, b3d->YMax() + 1 );
ClipPlane[ 2 ]      = TGLClipPlane ( GL_CLIP_PLANE2,  0,  0, -1, ModelCenter.Z + 0.10, b3d->ZMin() - 1, b3d->ZMax() + 1 );

ClipPlane[ 0 ].SetNone ();
ClipPlane[ 1 ].SetNone ();
ClipPlane[ 2 ].SetNone ();


Zoom                = CartoolApplication->AnimateViews ? 0.001 : 1;

MaterialEl  = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.40,  (GLfloat) 0.40,  (GLfloat) 0.10,  (GLfloat) 1.00,
                                      (GLfloat) 0.85,  (GLfloat) 0.85,  (GLfloat) 0.18,  (GLfloat) 0.45,
                                      (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

MaterialEl2 = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.25,  (GLfloat) 0.25,  (GLfloat) 0.60,  (GLfloat) 1.00,
                                      (GLfloat) 0.40,  (GLfloat) 0.40,  (GLfloat) 1.00,  (GLfloat) 0.45,
                                      (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 0.50,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

MaterialElBad = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 1.00,
                                      (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 0.45,
                                      (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

MaterialElSel = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.30,  (GLfloat) 0.10,  (GLfloat) 0.30,  (GLfloat) 1.00,
                                      (GLfloat) 0.90,  (GLfloat) 0.10,  (GLfloat) 0.90,  (GLfloat) 0.45,
                                      (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     30.00 );

//MaterialTri = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
//                                      (GLfloat) 0.25,  (GLfloat) 0.25,  (GLfloat) 0.15,  (GLfloat) 1.00,
//                                      (GLfloat) 0.35,  (GLfloat) 0.35,  (GLfloat) 0.15,  (GLfloat) 0.50,
//                                      (GLfloat) 0.70,  (GLfloat) 0.70,  (GLfloat) 0.70,  (GLfloat) 1.00,
//                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
//                                     30.00 );

MaterialTri = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 0.05,  (GLfloat) 1.00,
                                      (GLfloat) 0.55,  (GLfloat) 0.55,  (GLfloat) 0.22,  (GLfloat) 0.50,
                                      (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     15.00 );

//MaterialTri2 = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
//                                      (GLfloat) 0.15,  (GLfloat) 0.15,  (GLfloat) 0.25,  (GLfloat) 1.00,
//                                      (GLfloat) 0.15,  (GLfloat) 0.15,  (GLfloat) 0.40,  (GLfloat) 0.50,
//                                      (GLfloat) 0.70,  (GLfloat) 0.70,  (GLfloat) 0.70,  (GLfloat) 1.00,
//                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
//                                     30.00 );

MaterialTri2 = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.15,  (GLfloat) 0.15,  (GLfloat) 0.25,  (GLfloat) 1.00,
                                      (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 0.55,  (GLfloat) 0.50,
                                      (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     15.00 );


Fog         = TGLFog<GLfloat>   ( ModelRadius * FogXyzNear,
                                  ModelRadius * FogXyzFar,
                                  GLBASE_FOGCOLOR_NORMAL );


Xaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    b3d->XMax() + b3d->MaxSize () * 0.08, 0, 0,
                                    b3d->MeanSize() * 0.06, b3d->MeanSize() * 0.015,
                                    (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Yaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, b3d->YMax() + b3d->MaxSize () * 0.08, 0,
                                    b3d->MeanSize() * 0.06, b3d->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Zaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, 0, b3d->ZMax() + b3d->MaxSize () * 0.08,
                                    b3d->MeanSize() * 0.06, b3d->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 1.00 );

OriginRadius    = 0.20 * XYZDoc->GetPointRadius ( CurrentDisplaySpace );


Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );
Attr.W      = Attr.H /* * WindowHeightToWidthRatio*/;
StandSize   = TSize ( XYZWindowSize, XYZWindowSize );


SetViewMenu ( new TMenuDescr (IDM_XYZ) );


if ( ! ValidView() )
    NotOK();                            // do not create the window (cancel from doc)
}


//----------------------------------------------------------------------------
void    TElectrodesView::CreateGadgets ()
{
NumControlBarGadgets    = XYZGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();

ControlBarGadgets[ XYZGLVIEW_CBG_SEP1           ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ XYZGLVIEW_CBG_SHOWELEC       ]   = new TButtonGadgetDpi ( IDB_SHOWELEC,      IDB_SHOWELEC,       TButtonGadget::NonExclusive, true, TButtonGadget::Down, false);
ControlBarGadgets[ XYZGLVIEW_CBG_SHOWELNAMES    ]   = new TButtonGadgetDpi ( IDB_SHOWELNAMES,   IDB_SHOWELNAMES,    TButtonGadget::NonExclusive, true, ShowLabels ? TButtonGadget::Down : TButtonGadget::Up, false);
ControlBarGadgets[ XYZGLVIEW_CBG_FLATVIEW       ]   = new TButtonGadgetDpi ( IDB_FLATVIEW,      IDB_FLATVIEW,       TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
                                                                             
ControlBarGadgets[ XYZGLVIEW_CBG_SEP2           ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ XYZGLVIEW_CBG_CUTPLANEX      ]   = new TButtonGadgetDpi ( IDB_CUTPLANEX,     IDB_CUTPLANEX,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[ XYZGLVIEW_CBG_CUTPLANEY      ]   = new TButtonGadgetDpi ( IDB_CUTPLANEY,     IDB_CUTPLANEY,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[ XYZGLVIEW_CBG_CUTPLANEZ      ]   = new TButtonGadgetDpi ( IDB_CUTPLANEZ,     IDB_CUTPLANEZ,      TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
}


//----------------------------------------------------------------------------
        TElectrodesView::~TElectrodesView()
{
if ( MouseDC ) {
    delete  MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;

    delete[]    mtgFrom;
    delete[]    mtgTo;
    }
}


//----------------------------------------------------------------------------
void    TElectrodesView::SetupWindow ()
{
TBaseView::SetupWindow ();

if ( CartoolApplication->AnimateViews )
    AnimFx.Start ( TimerStartup, 10, 30 );
}


//----------------------------------------------------------------------------
bool    TElectrodesView::ModifyPickingInfo ( TPointFloat& Picking, char *buff )
{
ClearString ( buff );

if ( ! XYZDoc )
    return false;


//double              nearlimit       = Clip ( XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * PointNearFactor, PointRadiusRatio[ ShowElectrodes ] * 1.50, XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * 0.49 );
double              nearlimit       = XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * PointNearFactor;
int                 el              = XYZDoc->NearestElement ( Picking, CurrentDisplaySpace, nearlimit );

                                        // not close enough to an electrode?
if ( el < 0 )
    return false;


if ( CurrentDisplaySpace != DisplaySpace3D ) {
    XYZDoc->GetCoordinates ( el, Picking, false );
    sprintf ( buff, "3D Coordinates: %.2f, %.2f, %.2f\n", Picking.X, Picking.Y, Picking.Z );
    }


XYZDoc->GetCoordinates ( el, Picking, CurrentDisplaySpace );

                                        // don't show the electrode name file(?)
StringAppend    ( buff, "Electrode Name: " /*XYZDoc->GetTitle()*/, XYZDoc->GetElectrodeName ( el ) );
StringAppend    ( buff, NewLine );

StringAppend    ( buff, "Electrode Index: #" );
IntegerToString ( StringEnd ( buff ), el + 1 );


return true;
}


//----------------------------------------------------------------------------
bool    TElectrodesView::IsRenderingMode ( int renderingmode )
{
if      ( renderingmode == RenderingOpaque )

    return RenderingMode == XyzLines || RenderingMode == XyzSolidFacet || RenderingMode == XyzSolidSmooth;

else if ( renderingmode == RenderingTransparent )

    return RenderingMode == XyzTransparentFacet || RenderingMode == XyzTransparentSmooth;
else
    return false;
}


//----------------------------------------------------------------------------
void    TElectrodesView::GLPaint ( int how, int renderingmode, TGLClipPlane* otherclipplane )
{
                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode


if ( CurrentDisplaySpace != DisplaySpace3D ) {
    if      ( renderingmode == XyzSolidSmooth       )   renderingmode = XyzSolidFacet;
    else if ( renderingmode == XyzTransparentSmooth )   renderingmode = XyzTransparentFacet;
    }


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
int                 drawi[ 3 ]      = { -1, -1, -1 };
int                 ndraws          = AtLeast ( 1, ncutplanes );

if ( ncutplanes ) {

    int             n               = 0;
    if ( (bool) clipplane[ 0 ] )    drawi[ n++ ] = 0;
    if ( (bool) clipplane[ 1 ] )    drawi[ n++ ] = 1;
    if ( (bool) clipplane[ 2 ] )    drawi[ n   ] = 2;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//float             md              = XYZDoc->GetMedianDistance ( CurrentDisplaySpace );
double              pr              = XYZDoc->GetPointRadius ( CurrentDisplaySpace );
double              r               = pr * PointRadiusRatio[ ShowElectrodes ];
float               x;
float               y;
float               z;

int                 xmin            = XYZDoc->GetBounding( CurrentDisplaySpace )->XMin();
int                 xmax            = XYZDoc->GetBounding( CurrentDisplaySpace )->XMax();
int                 ymin            = XYZDoc->GetBounding( CurrentDisplaySpace )->YMin();
int                 ymax            = XYZDoc->GetBounding( CurrentDisplaySpace )->YMax();
int                 zmin            = XYZDoc->GetBounding( CurrentDisplaySpace )->ZMin();
int                 zmax            = XYZDoc->GetBounding( CurrentDisplaySpace )->ZMax();

double              meddistance     = XYZDoc->GetMedianDistance ( CurrentDisplaySpace );
double              mind            = meddistance * ElectrodeMinDistanceRatio / 4;

double              p;
char                buff[ 16 ];

                                        // Get the right rendering object
TTriangleSurface*   elrend          = XYZDoc->GetSurfaceRendering ( CurrentDisplaySpace );

int                 numpoints       = elrend->GetNumPoints();
const TPointFloat*  litri           = elrend->GetListTriangles ();
const TPointFloat*  litrinorm       = elrend->GetListTrianglesNormals ();
const TPointFloat*  lipoinorm       = elrend->GetListPointsNormals ();
//const int*        liidx           = elrend->GetListTrianglesIndexes ();

if ( numpoints == 0 )
    renderingmode   = XyzNone;

                                        // add a global shift
if ( DepthRange.UserShift ) {
    DepthRange.ShiftCloser ( DepthRange.UserShift );
    DepthRange.GLize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the triangulation
if ( renderingmode == XyzLines             && ( how & GLPaintOpaque )
  || renderingmode == XyzSolidFacet        && ( how & GLPaintOpaque )
  || renderingmode == XyzSolidSmooth       && ( how & GLPaintOpaque ) ) {

    glFrontFace ( GL_CW );

    if ( renderingmode == XyzLines )
        LineColor.GLize ( how & GLPaintOwner ? Outputing() ? 1 : 0 : 2 );
    else
        if ( how & GLPaintOwner )
            MaterialTri.GLize ();
        else
            MaterialTri2.GLize ();


    for ( int n = 0; n < ndraws; n++ ) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if ( ncutplanes ) {
                                    // must change clipping on each iteration (OR)
            if      ( n == 0 )  clipplane[ drawi[ 0 ] ].GLize ( CurrentClipPlane  );
            else if ( n == 1 )  clipplane[ drawi[ 0 ] ].GLize ( InvertedClipPlane );

            if      ( n == 1 )  clipplane[ drawi[ 1 ] ].GLize ( CurrentClipPlane  );
            else if ( n == 2 )  clipplane[ drawi[ 1 ] ].GLize ( InvertedClipPlane );

            if      ( n == 2 )  clipplane[ drawi[ 2 ] ].GLize ( CurrentClipPlane  );
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing setup
        if      ( renderingmode == XyzLines ) {

            GLLinesModeOn       ( false );

    //      glLineWidth ( 0.25 );

            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

            glEnable        ( GL_POLYGON_OFFSET_LINE );
            glPolygonMode   ( GL_FRONT_AND_BACK, GL_LINE );
            glPolygonOffset ( -1.5, -4 );

            glBegin ( GL_POLYGON );
            }
        else if ( renderingmode == XyzSolidFacet ) {
            GLMaterialOn        ();

            glBegin ( GL_TRIANGLES );
            }
        else if ( renderingmode == XyzSolidSmooth ) {
            GLMaterialOn        ();

            glBegin ( GL_TRIANGLES );
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing
        for ( int t=0; t < numpoints; ) {

            if ( renderingmode == XyzSolidSmooth ) {

                GLSmoothTrianglefv ( (GLfloat *) litri     + 3 * t, (GLfloat *) litri     + 3 * ( t + 1 ), (GLfloat *) litri     + 3 * ( t + 2 ),
                                     (GLfloat *) lipoinorm + 3 * t, (GLfloat *) lipoinorm + 3 * ( t + 1 ), (GLfloat *) lipoinorm + 3 * ( t + 2 ),
                                     4 );
                t += 3;
                }
            else {
                if ( renderingmode == XyzSolidFacet )
                    glNormal3fv ( (GLfloat *) litrinorm + t );

                glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                glVertex3fv ( (GLfloat *) litri + 3 * t++ );

                                            // finish the current triangle
                if ( renderingmode == XyzLines ) {
                    glEnd();
                    glBegin ( GL_POLYGON );
                    }
                }
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing ending
        if      ( renderingmode == XyzLines ) {
            glEnd();

            GLLinesModeOff      ( false );

    //      glLineWidth ( 1.0 );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();

            glDisable       ( GL_POLYGON_OFFSET_LINE );
            glPolygonMode   ( GL_FRONT_AND_BACK, GL_FILL );
            }
        else if ( renderingmode == XyzSolidFacet ) {
            glEnd();
            }
        else if ( renderingmode == XyzSolidSmooth ) {
            glEnd();

            glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
            }

        } // for ndraws


    if ( (bool) clipplane[0] ) clipplane[0].unGLize();
    if ( (bool) clipplane[1] ) clipplane[1].unGLize();
    if ( (bool) clipplane[2] ) clipplane[2].unGLize();

//    if ( how & GLPaintLinked )
//        glDisable ( GL_POLYGON_OFFSET_FILL );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the electrode positions
if ( ShowElectrodes && ( how & GLPaintOpaque ) ) {

    GLMaterialOn        ();

    TGLBillboardSphere *tobbsphere = Outputing() /*|| VkKey ( 'A' )*/ ? &BbHighSphere : &BbLowSphere;

    tobbsphere->GLize ();               // will do all the job


    for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

        XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );


        if ( CurrentDisplaySpace == DisplaySpace3D && ncutplanes 
            && ! (    clipplane[ 0 ].IsForward () && x < cx + mind || clipplane[ 0 ].IsBackward () && x > cx - mind
                   || clipplane[ 1 ].IsForward () && y < cy + mind || clipplane[ 1 ].IsBackward () && y > cy - mind
                   || clipplane[ 2 ].IsForward () && z < cz + mind || clipplane[ 2 ].IsBackward () && z > cz - mind ) )

                continue;


        if ( (bool) AnimFx && ( AnimFx.GetTimerId () == TimerToFlat || AnimFx.GetTimerId () == TimerTo3D ) ) {
            float   x1,  y1,  z1;
            float   x2,  y2,  z2;
            double  w1, w2;

            if ( AnimFx.GetTimerId () == TimerToFlat  )
                w2 = AnimFx.GetPercentageCompletion(), w1 = AnimFx.GetPercentageRemaining();
            else
                w1 = AnimFx.GetPercentageCompletion(), w2 = AnimFx.GetPercentageRemaining();

            XYZDoc->GetCoordinates ( e, x1, y1, z1, false );
            XYZDoc->GetCoordinates ( e, x2, y2, z2, true );

            x = w1 * x1 + w2 * x2;
            y = w1 * y1 + w2 * y2;
            z = w1 * z1 + w2 * z2;
            }


        if ( how & GLPaintOwner ) {
            if      ( Highlighted[ e ] )    MaterialElSel.GLize ();
            else if ( Bad        [ e ] )    MaterialElBad.GLize ();
            else                            MaterialEl   .GLize ();
            }
        else                                MaterialEl2  .GLize ();


        tobbsphere->GLize ( x, y, z, Highlighted[ e ] ? HighlightRadiusBoost[ ShowElectrodes ] * r : r );
        }  // for electrode


    tobbsphere->unGLize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque or transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // outline clipping planes
if ( CurrentDisplaySpace == DisplaySpace3D 
  && ( how & GLPaintOwner ) && ( how & GLPaintOpaque )
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
// The transparent stuff should be drawn at the end
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the triangulation
if ( renderingmode == XyzTransparentFacet  && ( how & GLPaintTransparent )
  || renderingmode == XyzTransparentSmooth && ( how & GLPaintTransparent ) ) {

    GLTransparencyOn    ();
    GLMaterialOn        ();
    glFrontFace         ( GL_CW );

    if ( how & GLPaintOwner )
        MaterialTri.GLize ();
    else
        MaterialTri2.GLize ();


    for ( int n = 0; n < ndraws; n++ ) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if ( ncutplanes ) {
                                    // must change clipping on each iteration (OR)
            if      ( n == 0 )  clipplane[ drawi[ 0 ] ].GLize ( CurrentClipPlane  );
            else if ( n == 1 )  clipplane[ drawi[ 0 ] ].GLize ( InvertedClipPlane );

            if      ( n == 1 )  clipplane[ drawi[ 1 ] ].GLize ( CurrentClipPlane  );
            else if ( n == 2 )  clipplane[ drawi[ 1 ] ].GLize ( InvertedClipPlane );

            if      ( n == 2 )  clipplane[ drawi[ 2 ] ].GLize ( CurrentClipPlane  );
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing
        if ( CurrentDisplaySpace == DisplaySpace3D ) {
                                        // draw in 2 passes: first back triangles, then front
            glEnable ( GL_CULL_FACE );

            glCullFace ( GL_BACK );
            glBegin ( GL_TRIANGLES );

            for ( int t=0; t < numpoints; ) {
                if ( renderingmode == XyzTransparentSmooth ) {

                    GLSmoothTrianglefv ( (GLfloat *) litri     + 3 * t, (GLfloat *) litri     + 3 * ( t + 1 ), (GLfloat *) litri     + 3 * ( t + 2 ),
                                         (GLfloat *) lipoinorm + 3 * t, (GLfloat *) lipoinorm + 3 * ( t + 1 ), (GLfloat *) lipoinorm + 3 * ( t + 2 ),
                                         4 );
                    t += 3;
                    }
                else {
                    glNormal3fv ( (GLfloat *) litrinorm + t );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    }
                }

            glEnd();

            glCullFace ( GL_FRONT );
            glBegin ( GL_TRIANGLES );

            for ( int t=0; t < numpoints; ) {
                if ( renderingmode == XyzTransparentSmooth ) {

                    GLSmoothTrianglefv ( (GLfloat *) litri     + 3 * t, (GLfloat *) litri     + 3 * ( t + 1 ), (GLfloat *) litri     + 3 * ( t + 2 ),
                                         (GLfloat *) lipoinorm + 3 * t, (GLfloat *) lipoinorm + 3 * ( t + 1 ), (GLfloat *) lipoinorm + 3 * ( t + 2 ),
                                         4 );
                    t += 3;
                    }
                else {
                    glNormal3fv ( (GLfloat *) litrinorm + t );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    }
                }

            glEnd();
            glDisable ( GL_CULL_FACE );
            glCullFace ( GL_BACK );
            }
        else {
            glBegin ( GL_TRIANGLES );

            for ( int t=0; t < numpoints; ) {
                if ( renderingmode == XyzTransparentSmooth ) {
                    glNormal3fv ( (GLfloat *) lipoinorm + 3 * t );
                    glVertex3fv ( (GLfloat *) litri     + 3 * t++ );
                    glNormal3fv ( (GLfloat *) lipoinorm + 3 * t );
                    glVertex3fv ( (GLfloat *) litri     + 3 * t++ );
                    glNormal3fv ( (GLfloat *) lipoinorm + 3 * t );
                    glVertex3fv ( (GLfloat *) litri     + 3 * t++ );
                    }
                else {
                    glNormal3fv ( (GLfloat *) litrinorm + t );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    glVertex3fv ( (GLfloat *) litri + 3 * t++ );
                    }
                }

            glEnd();
            }

        } // for ndraws


    if ( (bool) clipplane[0] ) clipplane[0].unGLize();
    if ( (bool) clipplane[1] ) clipplane[1].unGLize();
    if ( (bool) clipplane[2] ) clipplane[2].unGLize();

    GLTransparencyOff   ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // equator circle & axis
if ( CurrentDisplaySpace != DisplaySpace3D && XYZDoc->IsHead () && ShowAxis
  && ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

    GLLinesModeOn       ( false );

    DepthRange.ShiftCloser ();
    DepthRange.GLize ();

    LineColor.GLize ( how & GLPaintOwner ? Outputing() ? 1 : 0 : 2 );

                                        // equator?
    double              equ             = XYZDoc->GetProjEquator ();

    if ( equ ) {
        glBegin ( GL_LINE_LOOP );

        for ( double angle = 0; angle < TwoPi; angle += 0.1 )
            glVertex3f ( cos ( angle ) * equ, sin ( angle ) * equ, 0 );

        glEnd();
        }

                                        // axis
    const TBoundingBox<double>* pb      = XYZDoc->GetBounding ( DisplaySpaceProjected );
    float               delta           = pb->MaxSize () / 20;
    float               xmin            = pb->XMin () - delta;
    float               xmax            = pb->XMax () + delta;
    float               ymin            = pb->YMin () - delta;
    float               ymax            = pb->YMax () + delta;

    glBegin ( GL_LINES );
    glVertex3f ( xmin, 0, 0 );
    glVertex3f ( xmax, 0, 0 );
    glVertex3f ( 0, ymin, 0 );
    glVertex3f ( 0, ymax, 0 );
    glEnd();


    GLLinesModeOff      ( false );

    DepthRange.ShiftFurther ();
    DepthRange.GLize ();
    } // ShowAxis


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw electrode names
if ( ShowLabels && ( how & GLPaintTransparent ) ) {

    GLColoringOn        ();
//  GLWriteDepthOff     ();
//  GLBlendOn           ();


//  TextColor.GLize ( how & GLPaintOwner ? Outputing() ^ ( ShowTextColor == TextColorDark )? 1 : 0
//                                       : Outputing() ^ ( ShowTextColor == TextColorDark )? 3 : 2 );
//  TextColor.GLize ( how & GLPaintOwner ? ShowTextColor == TextColorLight ? 0 : 1 : 2 );

    TextColor.GLize ( how & GLPaintOwner ? 0 : 2 );

    SFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

    char                label[ ElectrodeNameSize ];
    char                buff [ ElectrodeNameSize ];


    for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

        XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );


        if ( CurrentDisplaySpace == DisplaySpace3D && ncutplanes 
            && ! (    clipplane[ 0 ].IsForward () && x < cx + mind || clipplane[ 0 ].IsBackward () && x > cx - mind
                   || clipplane[ 1 ].IsForward () && y < cy + mind || clipplane[ 1 ].IsBackward () && y > cy - mind
                   || clipplane[ 2 ].IsForward () && z < cz + mind || clipplane[ 2 ].IsBackward () && z > cz - mind ) )

                continue;


        if ( (bool) AnimFx && ( AnimFx.GetTimerId () == TimerToFlat || AnimFx.GetTimerId () == TimerTo3D ) ) {
            float   x1,  y1,  z1;
            float   x2,  y2,  z2;
            double  w1, w2;

            if ( AnimFx.GetTimerId() == TimerToFlat  )
                w2 = AnimFx.GetPercentageCompletion(), w1 = AnimFx.GetPercentageRemaining();
            else
                w1 = AnimFx.GetPercentageCompletion(), w2 = AnimFx.GetPercentageRemaining();

            XYZDoc->GetCoordinates ( e, x1, y1, z1, false );
            XYZDoc->GetCoordinates ( e, x2, y2, z2, true );

            x = w1 * x1 + w2 * x2;
            y = w1 * y1 + w2 * y2;
            z = w1 * z1 + w2 * z2;
            }

//          if ( CurrentDisplaySpace != DisplaySpace3D ) {
//              z = XYZDoc->GetBounding ( DisplaySpaceProjected )->Radius() / 15;
/*
            if ( ShowElectrodes ) {
                                        // shift radially on the plane
                r  = sqrt ( x * x + y * y );
                if ( r == 0 ) {
                    x += glh;
                    }
                else {
                    x += (double) x / r * glh;
                    y += (double) y / r * glh * 1.2;
                    }
                }
            }
*/
/*
        if ( CurrentDisplaySpace == DisplaySpace3D ) {
                                        // shift outward radially
//          x *= 1.07, y *= 1.07, z *= 1.07;
            x += ( x - ModelCenter.X ) * 0.10;
            y += ( y - ModelCenter.Y ) * 0.10;
            z += ( z - ModelCenter.Z ) * 0.10;
            }

        if ( z == 0 )
            z = XYZDoc->GetBounding ( DisplaySpaceProjected )->Radius() / 15;
*/

        if      ( ShowLabels == ShowLabelText  )    StringCopy ( label, XYZDoc->GetElectrodeName ( e ) );
        else if ( ShowLabels == ShowLabelIndex )    StringCopy ( label, "#", IntegerToString ( buff, e + 1 ) );

//      SFont->Print ( x, y, z, label,
//                     TA_CENTER | ( CurrentDisplaySpace != DisplaySpace3D && ShowElectrodes ? TA_TOP : TA_CENTERY ) | TA_BOX, 0, CurrentDisplaySpace != DisplaySpace3D && ShowElectrodes ? -6 * Zoom * Zoom : 0 );

                                        // draw the names with a screen coordinates shift, including toward viewer
                                        // -> always on top, whatever the rotation!
        SFont->Print ( x, y, z, label,
                       TA_CENTER | ( ShowElectrodes ? TA_TOP : TA_CENTERY ) | TA_BOX, (GLfloat) 0, (GLfloat) ( ShowElectrodes ? -6 * Zoom * Zoom : 0 ), (GLfloat) 0.025 );
        }


    if ( CurrentDisplaySpace == DisplaySpace3D )

        for ( int c = 0; c < XYZDoc->GetNumClusters (); c++ ) {

            const TElectrodesCluster&   clu     = XYZDoc->GetCluster ( c );
            float                       dx;
            float                       dy;
            float                       dz;

            if ( clu.NumElectrodes <= 1 )
                continue;

            dx = clu.Bounding.GetXExtent ();
            dy = clu.Bounding.GetYExtent ();
            dz = clu.Bounding.GetZExtent ();

            if      ( dx >= dy && dx >= dz )
                x = clu.Bounding.XMin() + dx / 2,
                y = clu.Bounding.YMin() - dx / 10,
                z = clu.Bounding.ZMin() - dx / 10;
            else if ( dy >= dx && dy >= dz )
                x = clu.Bounding.XMin() - dy / 10,
                y = clu.Bounding.YMin() + dy / 2,
                z = clu.Bounding.ZMin() - dy / 10;

            else if ( dz >= dx && dz >= dy )
                x = clu.Bounding.XMin() - dz / 10,
                y = clu.Bounding.YMin() - dz / 10,
                z = clu.Bounding.ZMin() + dz / 2;

    //        x = clu.Bounding.XMin();
    //        y = clu.Bounding.YMin();
    //        z = clu.Bounding.ZMin();

            SFont->Print ( x, y, z, (char *) clu.Name, TA_CENTER | TA_TOP | TA_BOX );
            }


    GLColoringOff       ();
//  GLWriteDepthOn      ();
//  GLBlendOff          ();
    }

                                        // reset
if ( DepthRange.UserShift ) {
    DepthRange.ShiftFurther ( DepthRange.UserShift );
    DepthRange.GLize ();
    }
}


//----------------------------------------------------------------------------
void    TElectrodesView::Paint ( TDC &dc, bool /*erase*/ , TRect &rect )
{

const TBoundingBox<double>* b   = XYZDoc->GetBounding ( CurrentDisplaySpace );
double              radius      = AtLeast ( 1.0, b->MaxSize () / 2 * ( CurrentDisplaySpace == DisplaySpace3D ? ExtraSizeXyz3D : ExtraSize3D ) );

PrePaint ( dc, rect, radius, radius, CurrentDisplaySpace != DisplaySpace3D ? ProjModelRadius : ModelRadius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

//glShadeModel ( GL_FLAT );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glFrontFace ( GL_CCW );                 // specifies the right orientation


Light0.GLize ();
Light1.GLize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for a correct Fog, push everything below 0
if ( CurrentDisplaySpace != DisplaySpace3D )
    glTranslated ( 0, 0, -ProjModelRadius );
else
    glTranslated ( 0, 0, -ModelRadius );


ModelRotMatrix.GLize ();


if ( CurrentDisplaySpace != DisplaySpace3D )
    glTranslated ( -ProjModelCenter.X, -ProjModelCenter.Y, -ProjModelCenter.Z );
else
    glTranslated ( -ModelCenter.X,     -ModelCenter.Y,     -ModelCenter.Z     );

                                        // save this matrix for external drawings
MatProjection.CopyProjection ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CurrentDisplaySpace != DisplaySpace3D )
    GLFogOff        ();
else
    Fog.GLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AntialiasingPaint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the axis unclipped
if ( /*! Outputing() &&*/ CurrentDisplaySpace == DisplaySpace3D && ShowAxis )
    DrawAxis ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.unGLize();

                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw orientation
    if ( ShowOrientation )
        DrawOrientation ( CurrentDisplaySpace != DisplaySpace3D ? XYZDoc->GetProjBoxSides() : XYZDoc->GetBoxSides() );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show informations
    if ( ShowInfos /*&& ! Outputing ()*/ ) {
        TextColor.GLize ( Outputing() ? 1 : 0 );

        double              xpos            = PrintLeftPos;
        double              ypos            = PrintTopPos( PaintRect.Height () );
        double              zpos            = PrintDepthPosBack;
        double              ydelta          = PrintVerticalStep ( SFont );
        char                buff [ 256 ];
//      double             *voxsize         = XYZDoc->GetVoxelSize ();

                                        // Dimension info
        sprintf ( buff, "%0d  electrodes, in  %0d  cluster(s)", XYZDoc->GetNumElectrodes (), XYZDoc->GetNumClusters () );
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

                                        // Data type
        ypos   -= 1.5 * ydelta;
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetContentTypeName ( buff ), TA_LEFT | TA_TOP );

        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetAtomTypeName ( AtomTypeUseCurrent ), TA_LEFT | TA_TOP );

        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, DisplaySpaceName[ CurrentDisplaySpace ], TA_LEFT | TA_TOP );

                                        // Size
        sprintf ( buff, "Content size:  %.1lf  %.1lf  %.1lf [mm]", XYZDoc->GetBounding ( CurrentDisplaySpace )->GetXExtent (), XYZDoc->GetBounding ( CurrentDisplaySpace )->GetYExtent (), XYZDoc->GetBounding ( CurrentDisplaySpace )->GetZExtent () );
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
void    TElectrodesView::EvKeyDown (uint key, uint repeatCount, uint flags)
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {
    case    'G':
        CmMagnifier ();
        break;

    case    'L':
        Cm2Object ();
        break;

    case    'O':
        CmOrient ();
        break;

    case 'E':
        CmSetShowElectrodes ();
        break;

    case 'F':
    case '2': // for compatibility with Inverses
        CmSetFlatView ();
        break;

    case 'N':
        CmSetShowElectrodesNames ();
        break;

    case 'R':
        CmSetShowTriangles ();
        break;

    case 'T':
        RenderingMode = XyzLines;
        Invalidate ( false );
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
void    TElectrodesView::EvLButtonDown ( uint i, const TPoint &p )
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
void    TElectrodesView::EvLButtonUp ( uint i, const TPoint &p )
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
void    TElectrodesView::EvMButtonDown ( uint i, const TPoint &p )
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

    XYZDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

    ShowNow ();
    }
}


//----------------------------------------------------------------------------
void    TElectrodesView::EvMButtonUp (uint, const TPoint &/*p*/ )
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
void    TElectrodesView::EvRButtonDown ( uint, const TPoint& p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos   = p;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );
}


//----------------------------------------------------------------------------
void    TElectrodesView::EvRButtonUp (uint, const TPoint& )
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
void    TElectrodesView::EvMouseMove ( uint i, const TPoint &p )
{
int     dx      = p.X() - MousePos.X();
int     dy      = p.Y() - MousePos.Y();
//int     adx     = abs ( dx );
//int     ady     = abs ( dy );


if ( CurrentDisplaySpace == DisplaySpace3D && RButtonDown ) {

    if ( ! ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) )
        return;

    double  k   = (double) XYZDoc->GetBounding( DisplaySpace3D )->MaxSize() / GetClientRect().Height() * 1.7;

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


if ( MButtonDown ) {

    static int          lasthit         = -1;
    static ulong        before          = 0;
    ulong               now             = GetWindowsTimeInMillisecond ();


    if ( now - before < 50 || Picking.IsNull () )
        return;


//  double              nearlimit       = Clip ( XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * PointNearFactor, PointRadiusRatio[ ShowElectrodes ] * 1.50, XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * 0.49 );
    double              nearlimit       = XYZDoc->GetMedianDistance ( CurrentDisplaySpace ) * PointNearFactor;
    int                 hit             = XYZDoc->NearestElement ( Picking, CurrentDisplaySpace, nearlimit );

    if ( hit >= 0 ) {

        if ( lasthit != -1 && lasthit == hit && now - before < MouseMoveHitDelay )
            return;

        before  = now;

        if ( ShiftKey ) {
            Bad.Invert ( hit );

            XYZDoc->SetBadElectrodes ( Bad );
            }
        else { // if ( ControlKey ) {     // control + middle = select
            Highlighted.Invert ( hit );

            XYZDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );
            }

        ShowNow ();
        }


    lasthit = hit;
    }
}


//----------------------------------------------------------------------------
void    TElectrodesView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& p )
{
TBaseView::EvMouseWheel ( modKeys, zDelta, p );
}


//----------------------------------------------------------------------------
void    TElectrodesView::EvTimer ( uint timerId )
{
double              ta1;


switch ( timerId ) {

    case TimerStartup:

        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx ) {

            AnimFx.Stop ();

            Zoom    = 1;
//          Orientation         = DefaultPotentialOrientation;
//          CmOrient ();
            MaterialTri.SetDiffuse ( (GLfloat) 0.55,  (GLfloat) 0.55,  (GLfloat) 0.22,  (GLfloat) 0.50 );

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        ta1     = AnimFx.GetPercentageCompletion();
        Zoom    = sqrt ( ta1 );
//      ModelRotMatrix.RotateX ( 360.0 / AnimFx.GetMaxCount (), MultiplyLeft );
        MaterialTri.SetDiffuse ( 0.55 / ta1,  0.55 / ta1 ,  0.22 / ta1,  0.50 / ta1 );

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;


    case TimerToFlat:
    case TimerTo3D:

        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx ) {

            AnimFx.Stop ();

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;


    default:

        TBaseView::EvTimer ( timerId );
    }
}


//----------------------------------------------------------------------------
bool    TElectrodesView::VnNewBadSelection ( const TSelection *sel )
{
//if ( sel->SentFrom != LinkedViewId )
//    return true;                        // not for me !


if ( Bad != *sel ) {

    Bad         = *sel;
                                        // propagate to document, that will notify views
    XYZDoc->SetBadElectrodes ( Bad, false );

    Invalidate ( false );
    }

return  true;
}


//----------------------------------------------------------------------------
bool    TElectrodesView::VnNewHighlighted ( const TSelection *sel )
{
    // message can be either for electrodes or SPs                                if 0, accept all
if ( abs ( sel->Size () - Highlighted.Size () ) > NumPseudoTracks ) // || sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                        // not for me !


if ( Highlighted != *sel ) {
    Highlighted = *sel;

    if ( ShowElectrodes )
        ShowNow ();
    }


return  true;
}


//----------------------------------------------------------------------------
void    TElectrodesView::CmOrient ()
{
if ( CurrentDisplaySpace != DisplaySpace3D ) {
    Orientation         = OrientMinusYX;

    ModelRotMatrix.SetOrientation ( Orientation );
    }
else
    TBaseView::CmOrient ();

Invalidate ( false );
}


void    TElectrodesView::CmSetShowElectrodes ()
{
if ( ShiftKey )
    ShowElectrodes   = ( ShowElectrodes + PointsNumRendering - 1 ) % PointsNumRendering;
else
    ShowElectrodes   = ( ShowElectrodes + 1 ) % PointsNumRendering;


ButtonGadgetSetState ( IDB_SHOWELEC, ShowElectrodes );
Invalidate ( false );
}


void    TElectrodesView::CmSetShowElectrodesNames ()
{
ShowLabels      = (ShowLabelEnum) ( ( ShowLabels + ( ShiftKey ? NumShowLabels - 1 : 1 ) ) % NumShowLabels );

ButtonGadgetSetState ( IDB_SHOWELNAMES, ShowLabels );

Invalidate ( false );
}


void    TElectrodesView::CmSetShowTriangles ()
{
if ( ShiftKey )
    RenderingMode   = ( RenderingMode + XyzNumRendering - 1 ) % XyzNumRendering;
else
    RenderingMode   = ( RenderingMode + 1 ) % XyzNumRendering;

Invalidate ( false );
}


void    TElectrodesView::CmSetFlatView ()
{
/*
if ( ShiftKey )
    CurrentDisplaySpace     = ( CurrentDisplaySpace + (int) GetDisplaySpaces () - 1 ) % (int) GetDisplaySpaces ();
else
    CurrentDisplaySpace     = ( CurrentDisplaySpace + 1                             ) % (int) GetDisplaySpaces ();
*/
                                        // just toggle between 3D and simple projection (height not useful)
CurrentDisplaySpace     = ! CurrentDisplaySpace;

ButtonGadgetSetState ( IDB_FLATVIEW, CurrentDisplaySpace );

if ( (bool) AnimFx && AnimFx.GetTimerId () == TimerStartup ) {
    Zoom = 1;
    return;
    }

AnimFx.Start ( CurrentDisplaySpace != DisplaySpace3D ? TimerToFlat : TimerTo3D, 10, 10 );

                                        // reset to usual point of view
//if ( CurrentDisplaySpace == DisplaySpaceProjected ) {
//    ModelRotMatrix[0]   = 0;    ModelRotMatrix[1]   = -1;   ModelRotMatrix[2]   = 0;    ModelRotMatrix[3]   = 0;
//    ModelRotMatrix[4]   = 1;    ModelRotMatrix[5]   = 0;    ModelRotMatrix[6]   = 0;    ModelRotMatrix[7]   = 0;
//    ModelRotMatrix[8]   = 0;    ModelRotMatrix[9]   = 0;    ModelRotMatrix[10]  = 1;    ModelRotMatrix[11]  = 0;
//    }


if ( CurrentDisplaySpace == DisplaySpace3D )
    Orientation         = DefaultPotentialOrientation - 1; // would be better to recall the last 3D orientation

CmOrient ();


//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TElectrodesView::CmSetCutPlane ( owlwparam wparam )
{
int                 p               = wparam - IDB_CUTPLANEX;
ClipPlaneDirection  num             = (ClipPlaneDirection) NumClipPlane;

                                        // toggles to next state
ClipPlane[ p ].Set ( NextState ( ClipPlane[ p ].GetMode (), num, (bool) ( ShiftKey ^ ( p != 0 ) ) ) );


ButtonGadgetSetState ( wparam, ClipPlane[ p ] );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TElectrodesView::CmSetFlatViewEnable ( TCommandEnabler &tce )
{
tce.Enable ( true );
}


void    TElectrodesView::CmSetShiftDepthRange ( owlwparam w )
{
TBaseView::CmSetShiftDepthRange ( w );

Invalidate ( false );
}


void    TElectrodesView::CmFlip ( owlwparam wparam )
{
XYZDoc->Flip ( wparam );
}


//void    TElectrodesView::CmFlipEnable ( TCommandEnabler &tce )
//{
//tce.Enable ( ! XYZDoc->IsPointedBy() );
//}


//----------------------------------------------------------------------------
void    TElectrodesView::CmExportXyz ( owlwparam w )
{
if ( ! (bool) Highlighted ) {
    ShowMessage ( "Well, select at least some electrodes beforehand!", "Export Selected Electrodes", ShowMessageWarning );
    return;
    }


static GetFileFromUser  getoutputfile ( "Output Coordinates File", AllCoordinatesFilesFilter, 1, GetFileWrite );

if ( ! getoutputfile.Execute () )
    return;


XYZDoc->ExtractToFile ( getoutputfile, Highlighted, w ==  CM_EXPORTXYZREMOVESEL );


TSuperGauge         Gauge ( "Saving Coordinates File" );
Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
void    TElectrodesView::CmExportDelaunay ()
{
                                        // retrieve triangulation info, as a list of indexes
TArray2<int>        neighindex;

XYZDoc->GetNeighborhoodIndexes ( neighindex );

if ( neighindex.IsNotAllocated () ) 
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retrieve all normalized distances across all electrodes
TTracks<double>     neighdistances;

XYZDoc->GetNeighborhoodDistances ( neighdistances );

if ( neighdistances.IsNotAllocated () ) 
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // merge the triangulation and the distances
TTracks<double>     neighweights ( neighdistances );
int                 e2;

neighweights.ResetMemory ();


                                        // then transfer to smaller array
for ( int e1 = 0; e1 < XYZDoc->GetNumElectrodes (); e1++ ) {

                                        // currently, central weight is 1
    neighweights ( e1, e1 ) = 1;


    for ( int ni = 0; ni < neighindex ( e1, 0 ); ni++ ) {

        e2      = neighindex ( e1, ni + 1 );

                                        // this shouldn't happen, but better not output some confusing weights
        if ( neighdistances ( e1, e2 ) == 0 )
            continue;

                                        // actual weights are currently the inverse normalized distance
        neighweights ( e1, e2 )     = neighweights ( e2, e1 )   = 1 / NonNull ( neighdistances ( e1, e2 ) );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


TFileName           file;


StringCopy      ( file, XYZDoc->GetDocPath () );
RemoveExtension ( file );
StringAppend    ( file, ".ElectrodesDistances", "." FILEEXT_EEGSEF );
neighdistances.WriteFile ( file );


StringCopy      ( file, XYZDoc->GetDocPath () );
RemoveExtension ( file );
StringAppend    ( file, ".TriangulationWeights", "." FILEEXT_EEGSEF );
neighweights.WriteFile ( file );



StringCopy      ( file, XYZDoc->GetDocPath () );
RemoveExtension ( file );
StringAppend    ( file, ".TriangulationIndexes", "." FILEEXT_TXT );

                                        // directly writing the indexes
ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ) );
ofs << StreamFormatGeneral;

for ( int e1 = 0; e1 < XYZDoc->GetNumElectrodes (); e1++ ) {

    ofs << StreamFormatInt8 << ( e1 + 1 );
    ofs << "\t" << StreamFormatInt8 << neighindex ( e1, 0 );

    for ( int ni = 0; ni < neighindex ( e1, 0 ); ni++ ) {

        e2      = neighindex ( e1, ni + 1 );

        ofs << "\t" << StreamFormatInt8 << ( e2 + 1 );
        }

    ofs << "\n";
    }



TSuperGauge         Gauge;
Gauge.Set           ( "Exporting Delaunay", SuperGaugeLevelInter );
Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
