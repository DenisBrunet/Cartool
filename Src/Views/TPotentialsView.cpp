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

#include    "TPotentialsView.h"

#include    "MemUtil.h"
#include    "Math.Resampling.h"
#include    "Dialogs.TSuperGauge.h"
#include    "System.h"

#include    "TArray1.h"
#include    "TArray2.h"
#include    "OpenGL.h"

#include    "TElectrodesDoc.h"

#include    "TTracksView.h"
#include    "TElectrodesView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//DEFINE_RESPONSE_TABLE2 (TPotentialsView, TSecondaryView, TBaseView)
DEFINE_RESPONSE_TABLE1 (TPotentialsView, TSecondaryView)

    EV_WM_KEYDOWN,
    EV_WM_KEYUP,
    EV_WM_LBUTTONDBLCLK,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONUP,
    EV_WM_MBUTTONDOWN,
    EV_WM_MBUTTONUP,
    EV_WM_RBUTTONDOWN,
    EV_WM_RBUTTONUP,
    EV_WM_MOUSEMOVE,
    EV_WM_SIZE,
    EV_WM_TIMER,
    EV_WM_MOUSEWHEEL,

    EV_VN_NEWTFCURSOR,
    EV_VN_RELOADDATA,
    EV_VN_NEWHIGHLIGHTED,
    EV_VN_NEWBADSELECTION,
    EV_VN_VIEWUPDATED,

    EV_COMMAND          ( IDB_2OBJECT,          Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,           CmOrient ),
    EV_COMMAND          ( IDB_MAGNIFIER,        CmMagnifier ),
    EV_COMMAND          ( IDB_SURFACEMODE,      CmSetRenderingMode ),

    EV_COMMAND          ( IDB_SHOWELEC,         CmSetShowElectrodes ),
    EV_COMMAND          ( IDB_SHOWTRIANGLES,    CmSetShowTriangles ),
    EV_COMMAND          ( IDB_SHOWELNAMES,      CmSetShowElectrodesNames ),

    EV_COMMAND          ( IDB_SHOWZERO,         CmSetShowZero ),
    EV_COMMAND          ( IDB_SHOWPMMINMAX,     CmSetShowMinMax ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPINC,       CmSetStepTF ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPDEC,       CmSetStepTF ),

    EV_COMMAND          ( IDB_NEXTXYZ,          CmNextXyz ),
    EV_COMMAND_ENABLE   ( IDB_NEXTXYZ,          CmXyzEnable ),
    EV_COMMAND          ( IDB_FIXEDSCALE,       CmSetScalingAdapt ),
    EV_COMMAND_AND_ID   ( CM_INTERPOL_N,        CmSetInterpolation ),
    EV_COMMAND_AND_ID   ( CM_INTERPOL_Y,        CmSetInterpolation ),
    EV_COMMAND          ( CM_SEARCHTFMAX,       CmSearchTFMax ),

    EV_COMMAND_AND_ID   ( IDB_SPINCBRIGHT,      CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_SPDECBRIGHT,      CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_SPINCCONTRAST,    CmSetContrast ),
    EV_COMMAND_AND_ID   ( IDB_SPDECCONTRAST,    CmSetContrast ),
    EV_COMMAND          ( IDB_SPCOLOR,          CmNextColorTable ),
    EV_COMMAND_ENABLE   ( IDB_SPINCBRIGHT,      CmSetIntensityLevelEnable ),
    EV_COMMAND_ENABLE   ( IDB_SPDECBRIGHT,      CmSetIntensityLevelEnable ),
    EV_COMMAND_ENABLE   ( CM_SPUSERSCALE,       CmSetIntensityLevelEnable ),

    EV_COMMAND          ( IDB_FLATVIEW,         CmSetFlatView ),
    EV_COMMAND_ENABLE   ( IDB_FLATVIEW,         CmSetFlatViewEnable ),
    EV_COMMAND          ( IDB_SHOWTRACKS,       CmSetShowTracks ),
    EV_COMMAND          ( CM_SHOWSEQUENCELABEL, CmShowSequenceLabels ),

    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE0,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE1,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE2,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE3,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE4,  CmSetShiftDepthRange ),
END_RESPONSE_TABLE;


        TPotentialsView::TPotentialsView ( TTracksDoc &doc, TWindow *parent, TLinkManyDoc *group )
      : TSecondaryView ( doc, parent, group )
{
if ( ! ValidView() ) {
    NotOK();
    return;
    }


CurrXyz             = 0;
XYZDoc              = GODoc ? GODoc->GetXyzDoc ( CurrXyz ) : 0;
                                        // group all init that is XYZ dependent
CurrentDisplaySpace = DisplaySpace3D;

InitXyz ();

SetOrient ( XYZDoc );

                                        // just set 1 TF, GetTracks will reallocate on-demand
EegBuff.Resize ( EEGDoc->GetTotalElectrodes(), 1 );


SelTracks           = TSelection ( EEGDoc->GetTotalElectrodes(), OrderSorted );
if ( EEGDoc->GetNumElectrodes() != EEGDoc->GetTotalElectrodes(), OrderSorted )
    SelTracks.Reset ( EEGDoc->GetNumElectrodes(), EEGDoc->GetTotalElectrodes() - 1 );


ManageRangeCursor   = EEGDoc->IsTemplates () ? MRCSequence : MRCAverage;


TFCursor.SetPos ( SearchAndSetIntensity ( false ) );

//EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

GetTracks ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // get aux from Eeg
AuxTracks           = EEGDoc->GetAuxTracks ();


//SetScalingContrast ( sqrt ( ( 1.0 - 0.125 ) / 5.875 ) );    // reverse formula to have a neutral contrast
SetScalingContrast ( sqrt ( ( 0.5 - 0.125 ) / 5.875 ) );    // reverse formula to have a slightly undercontrast


ShowZero            = false;
ShowMinMax          = 0;
ShowElectrodes      = PointsShowNone;
ShowTracks          = false;
ShowTriangles       = false;
ShowLabels          = ShowLabelNone;
ShowTextColor       = TextColorLight;

Orientation         = DefaultPotentialOrientation - 1;

slicesPerX          = 20;
slicesPerY          = 20;

Highlighted         = TSelection ( EEGDoc->GetTotalElectrodes (), OrderSorted );
Highlighted.Reset ();
GetEegView ()->GetHighlighted ( &Highlighted );
Highlighted.SentTo   = 0;
Highlighted.SentFrom = GetViewId();


RenderingMode       = PotentialsRenderingSolid;
DepthRange.UserShift= 0;

Zoom                = 1; // CartoolApplication->AnimateViews && GODoc->GetNumEegDoc () <= 4 ? 0.001 : 1;



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


SetColorTable ( EEGDoc->GetAtomType ( AtomTypeUseCurrent ) );


Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );
Attr.W      = Attr.H * MoreWidthRatio;
StandSize   = TSize ( MapsWindowSizeW, MapsWindowSizeH );


SetViewMenu ( new TMenuDescr ( IDM_POTENTIALS ) );

NumControlBarGadgets= POTENTIALS_CBG_NUM;
ControlBarGadgets   = new TGadget * [ POTENTIALS_CBG_NUM ];

CreateBaseGadgets ();

ControlBarGadgets[ POTENTIALS_CBG_SEP1A         ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ POTENTIALS_CBG_SHOWELEC      ]   = new TButtonGadget ( IDB_SHOWELEC,     IDB_SHOWELEC,       TButtonGadget::NonExclusive );
//ControlBarGadgets[ POTENTIALS_CBG_SHOWTRI     ]   = new TButtonGadget ( IDB_SHOWTRIANGLES,IDB_SHOWTRIANGLES,  TButtonGadget::NonExclusive );
ControlBarGadgets[ POTENTIALS_CBG_SHOWELNAMES   ]   = new TButtonGadget ( IDB_SHOWELNAMES,  IDB_SHOWELNAMES,    TButtonGadget::NonExclusive, true, ShowLabels ? TButtonGadget::Down : TButtonGadget::Up, false );
ControlBarGadgets[ POTENTIALS_CBG_SHOWTRACKS    ]   = new TButtonGadget ( IDB_SHOWTRACKS,   IDB_SHOWTRACKS,     TButtonGadget::NonExclusive );

ControlBarGadgets[ POTENTIALS_CBG_SEP2          ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ POTENTIALS_CBG_FLATVIEW      ]   = new TButtonGadget ( IDB_FLATVIEW,     IDB_FLATVIEW,       TButtonGadget::NonExclusive, true, TButtonGadget::Up, false );
//ControlBarGadgets[ POTENTIALS_CBG_SHOWZERO    ]   = new TButtonGadget ( IDB_SHOWZERO,     IDB_SHOWZERO,       TButtonGadget::NonExclusive );
ControlBarGadgets[ POTENTIALS_CBG_SHOWPMMINMAX  ]   = new TButtonGadget ( IDB_SHOWPMMINMAX, IDB_SHOWPMMINMAX,   TButtonGadget::NonExclusive );

ControlBarGadgets[ POTENTIALS_CBG_SEP3          ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ POTENTIALS_CBG_DECBR         ]   = new TButtonGadget ( IDB_ISDECBRIGHT,  IDB_SPDECBRIGHT,    TButtonGadget::Command );
ControlBarGadgets[ POTENTIALS_CBG_INCBR         ]   = new TButtonGadget ( IDB_ISINCBRIGHT,  IDB_SPINCBRIGHT,    TButtonGadget::Command );
ControlBarGadgets[ POTENTIALS_CBG_DECCR         ]   = new TButtonGadget ( IDB_ISDECCONTRAST,IDB_SPDECCONTRAST,  TButtonGadget::Command );
ControlBarGadgets[ POTENTIALS_CBG_INCCR         ]   = new TButtonGadget ( IDB_ISINCCONTRAST,IDB_SPINCCONTRAST,  TButtonGadget::Command );

ControlBarGadgets[ POTENTIALS_CBG_SEP4          ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ POTENTIALS_CBG_FXDSCL        ]   = new TButtonGadget ( IDB_FIXEDSCALE,   IDB_FIXEDSCALE,     TButtonGadget::NonExclusive );
ControlBarGadgets[ POTENTIALS_CBG_SPCOLOR       ]   = new TButtonGadget ( IDB_SPCOLOR,      IDB_SPCOLOR,        TButtonGadget::Command );

ControlBarGadgets[ POTENTIALS_CBG_SEP5          ]   = new TSeparatorGadget ( ButtonSeparatorWidth );
ControlBarGadgets[ POTENTIALS_CBG_NEXTXYZ       ]   = new TButtonGadget ( IDB_NEXTXYZ,      IDB_NEXTXYZ,        TButtonGadget::Command );
}


void    TPotentialsView::InitXyz ()
{
                                        // bad tracks setup
                                        // get any setup from Electrodes
BadTracks           = XYZDoc->GetBadElectrodes ();
BadTracks.SentTo    = 0;
BadTracks.SentFrom  = GetViewId();

if ( (bool) BadTracks )
                                        // priority to Electrode setup
    XYZDoc->NotifyFriendViews ( FriendshipId, vnNewBadSelection, (TParam2) &BadTracks, this );
else                                    // otherwise, ask EEG
    GetEegView ()->GetBadTracks ( &BadTracks );
// or    BadTracks = EEGDoc->GetBadTracks ();



const TBoundingBox<double>*     b   = XYZDoc->GetBounding ( DisplaySpace3D        );
const TBoundingBox<double>*     bp  = XYZDoc->GetBounding ( DisplaySpaceProjected );


ModelCenter         = b->GetCenter ();
                                       // take the radius of the whole data box
ModelRadius         = DepthPositionRatio * b->AbsRadius ();


ProjModelCenter     = bp->GetCenter ();

ProjModelCenter.Z   = bp->ZMin();
                                        // dramatically increase depth to prevent clipping in height display
ProjModelRadius     = DepthPositionRatio * bp->AbsRadius() * ExtraDepthFlatView;



Fog         = TGLFog<GLfloat>     ( ModelRadius * FogXyzNear,
                                    ModelRadius * FogXyzFar,
                                    GLBASE_FOGCOLOR_NORMAL );


Xaxis       = TGLArrow<GLfloat>   ( 0,              0,               0,
                                    b->XMax(),      0,               0,
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Yaxis       = TGLArrow<GLfloat>   ( 0,              0,               0,
                                    0,              b->YMax(),       0,
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Zaxis       = TGLArrow<GLfloat>   ( 0,              0,               0,
                                    0,              0,               b->ZMax(),
                                    b->MeanSize() * 0.06, b->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 1.00 );


OriginRadius    = 0.20 * XYZDoc->GetPointRadius ( CurrentDisplaySpace != DisplaySpaceNone ? CurrentDisplaySpace : DisplaySpace3D );
}


void    TPotentialsView::SetupWindow ()
{
TBaseView::SetupWindow ();

//if ( CartoolApplication->AnimateViews && GODoc->GetNumEegDoc () <= 4 )
//    AnimFx.Set ( TimerStartup, 10, 30 );
}


//----------------------------------------------------------------------------
void    TPotentialsView::GetTracks  (   int     tf1,    int     tf2     )
{
                                        // Calling document - note the parameters are not up-to-date
//EEGDoc->GetTracks ( tf1, tf2, EegBuff, 0, false );

                                        // Calling the Tracks display to allow a quick copy of its own buffer
GetEegView ()->GetTracks ( tf1, tf2, EegBuff );
}


//----------------------------------------------------------------------------
bool    TPotentialsView::ModifyPickingInfo ( TPointFloat& Picking, char *buff )     const
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
StringAppend    ( buff, NewLine );

StringAppend    ( buff, "Value               = " );
FloatToString   ( StringEnd ( buff ), EegBuff ( el , 0 ) );


return true;
}


//----------------------------------------------------------------------------
                                        // !Does not change the cursor position!
long    TPotentialsView::SearchAndSetIntensity ( bool precise )
{
                                        // find the optimal scaling for first display
float               minValue;
float               maxValue;
float               absmaxValue;
long                maxtf           = 0;


if ( precise ) {
                                        // find TF with max value
    TDownsampling       downtf ( 0, EEGDoc->GetNumTimeFrames() - 1, 10000 );

    TSuperGauge         Gauge ( "Scanning Levels", downtf.NumDownData );

    float               v;
    minValue        =  FLT_MAX;
    maxValue        = -FLT_MAX;
    absmaxValue     =  0;

    for ( long tf = downtf.From ; tf <= downtf.To; tf += downtf.Step ) {

        Gauge.Next ();

        GetTracks ( tf, tf );

        for ( int e = 0; e < XYZDoc->GetNumElectrodes (); e++ ) {
                                        // don't account for bad tracks
            if ( BadTracks[ e ] || AuxTracks[ e ] ) continue;

            v           = EegBuff ( e, 0 );

            Mined ( minValue, v );
            Maxed ( maxValue, v );

            if ( fabs ( v ) > absmaxValue ) {
                absmaxValue = fabs ( v );
                maxtf       = tf;
                }
            }
        }
    }
else {                                  // just take the max gfp position as a reasonable estimate
    maxtf       = EEGDoc->GetMaxGfpTF ();

    GetTracks ( maxtf, maxtf );

    minValue    = EEGDoc->GetMinValue    ();
    maxValue    = EEGDoc->GetMaxValue    ();
    absmaxValue = EEGDoc->GetAbsMaxValue ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can set the absolute limits
SetScalingLimits ( absmaxValue * 1e-3, absmaxValue * 10 );


minValue    = ScalingAuto == ScalingAutoAsymmetric ? minValue : - absmaxValue;  // ?test if positive value?
maxValue    = ScalingAuto == ScalingAutoAsymmetric ? maxValue :   absmaxValue;

                                        // saturate, and set as new level
minValue   /= 2.5;
maxValue   /= 2.5;


SetScaling  ( minValue, maxValue, ScalingAuto != ScalingAutoAsymmetric );

                                        // !reload previous data!
GetTracks   ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


return  maxtf;
}


//----------------------------------------------------------------------------
void    TPotentialsView::CmSearchTFMax ()
{
                                        // modify cursor position
TFCursor.SetPos ( SearchAndSetIntensity ( true ) );


MRCNumTF        = 1;

AnimTF.Stop ();

MRCDoAnimation  = false;


EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );


GetTracks   ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


SetItemsInWindow ();

UpdateCaption ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
bool    TPotentialsView::IsRenderingMode ( int renderingmode )  const
{
if      ( renderingmode == RenderingOpaque      )   return RenderingMode == PotentialsRenderingSpheres     || RenderingMode == PotentialsRenderingSolid;
else if ( renderingmode == RenderingTransparent )   return RenderingMode == PotentialsRenderingTransparent || RenderingMode == PotentialsRenderingOvercast;
else                                                return false;
}


//----------------------------------------------------------------------------
void    TPotentialsView::GLPaint ( int how, int renderingmode, TGLClipPlane* /*otherclipplane*/ )
{
                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
TGLClipPlane*       clipplane;

                                        // take clipping from parameters, if both caller and owner share some clipping
if ( ( (bool) otherclipplane[ 0 ] || (bool) otherclipplane[ 1 ] || (bool) otherclipplane[ 2 ] )
  && ( (bool) ClipPlane     [ 0 ] || (bool) ClipPlane     [ 1 ] || (bool) ClipPlane     [ 2 ] )
  || ( how & GLPaintForceClipping ) )

    clipplane   = otherclipplane;
else                                    // otherwise, use local clipping
    clipplane   = ClipPlane;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
int             xmin    = -XYZDoc->GetRadius();
int             xmax    =  XYZDoc->GetRadius();
int             ymin    = -XYZDoc->GetRadius();
int             ymax    =  XYZDoc->GetRadius();
int             zmin    = -XYZDoc->GetRadius();
int             zmax    =  XYZDoc->GetRadius();
double          maxmax  = XYZDoc->GetRadius();
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                         tf;
int                         itf;
int                         minIndex;
int                         maxIndex;
double                      minValue;
double                      maxValue;
TPointFloat                 minPos;
TPointFloat                 maxPos;
double                      v;
int                         e;
TGLColor<GLfloat>           glcol;
int                         slicex, slicey;
//float                     ms1             = XYZDoc->GetBounding()->MaxSize ();
float                       ms1             = XYZDoc->GetCluster ( 0 ).Bounding.MaxSize ();
float                       ms2             = XYZDoc->GetBounding ( CurrentDisplaySpace )->MaxSize () * PotentialsSpaceBetween;
float                       cs              = ms1 * 0.012;
double                      pr              = XYZDoc->GetPointRadius ( CurrentDisplaySpace );
double                      equ             = XYZDoc->GetProjEquator ();
float                       x;
float                       y;
float                       z;
GLfloat                     orginwindow  [ 3 ];
GLfloat                     deltainwindow[ 3 ];
double                      trilimit        = 0;
//double                    trisize;
double                      contrast;
char                        buff[ 16 ];
double                      timepos;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracksView      *EEGView         = GetEegView ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GalMaxValue = -DBL_MAX;
GalMinValue =  DBL_MAX;

deltainwindow[ 0 ]  = ms1 / 10;
deltainwindow[ 1 ]  = deltainwindow[ 2 ]= 0;

orginwindow[ 0 ]    = orginwindow[ 1 ]  = orginwindow[ 2 ] = 0;

//orginwindow[0] = ( - (double) slicesPerX / 2 - 0.5 ) * ms2;
//orginwindow[1] = ( - (double) slicesPerY / 2 - 0.5 ) * ms2;
//orginwindow[2] = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get the right rendering object
TTriangleSurface*   elrend          = XYZDoc->GetSurfaceRendering ( CurrentDisplaySpace );

//int               numpoints       = elrend->GetNumPoints();
int                 numtri          = elrend->GetNumTriangles();
TPointFloat*        litri           = elrend->GetListTriangles ();
//const TPointFloat*litrinorm       = elrend->GetListTrianglesNormals ();
//const TPointFloat*lipoinorm       = elrend->GetListPointsNormals ();
const int*          liidx           = elrend->GetListTrianglesIndexes ();

                                        // !Entering UCA (Ugly Code Area) - You've been warned!
auto                TriangleLoop    = [ & ] () {
                                        // init starting point on indexes and triangle vertices
const int*          toi1            =          liidx;
const int*          toi2            =          liidx + 1;
const int*          toi3            =          liidx + 2;
GLfloat*            tot1            = (float*) litri;
GLfloat*            tot2            = (float*) litri + 3;
GLfloat*            tot3            = (float*) litri + 6;

for ( int t = 0; t < numtri; t++ ) {

    if ( ! (    BadTracks[ *toi1 ]
             || BadTracks[ *toi2 ]
             || BadTracks[ *toi3 ] ) ) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( CurrentDisplaySpace == DisplaySpaceHeight ) {

            double      v;

            v   = EegBuff ( *toi1 , tf );
            if ( v >= 0 )   tot1[ 2 ]   =   v / ScalingPMax * ms2 / 3;
            else            tot1[ 2 ]   = - v / ScalingNMax * ms2 / 3;

            v   = EegBuff ( *toi2 , tf );
            if ( v >= 0 )   tot2[ 2 ]   =   v / ScalingPMax * ms2 / 3;
            else            tot2[ 2 ]   = - v / ScalingNMax * ms2 / 3;

            v   = EegBuff ( *toi3 , tf );
            if ( v >= 0 )   tot3[ 2 ]   =   v / ScalingPMax * ms2 / 3;
            else            tot3[ 2 ]   = - v / ScalingNMax * ms2 / 3;
            } // DisplaySpaceHeight


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // animation?        
        if ( (bool) AnimFx && ( AnimFx.GetTimerId () == TimerToFlat || AnimFx.GetTimerId () == TimerTo3D ) ) {

                                        // memory offset between 3D and 2D triangles lists
            long        dr =   ( (char*) XYZDoc->GetSurfaceRendering ( DisplaySpaceProjected )->GetListTriangles () ) 
                             - ( (char*) XYZDoc->GetSurfaceRendering ( DisplaySpace3D        )->GetListTriangles () );

                                        // offset 3D -> 2D, or 2D -> 3D
            if ( AnimFx.GetTimerId() == TimerToFlat )   dr  = -dr;

                                        // pointer to the other triangles
            GLfloat*    totdr   = (GLfloat*) ( (char*) tot1 + dr );

                                        // normalized weights
            double      w1      = AnimFx.GetPercentageCompletion ();
            double      w2      = 1 - w1;

            GLfloat     v1[ 3 ];
            GLfloat     v2[ 3 ];
            GLfloat     v3[ 3 ];
                                        // linearly interpolating between the 3D and 2D triangles
            v1[ 0 ]     = w1 * tot1[ 0 ] + w2 * totdr[ 0 ];
            v1[ 1 ]     = w1 * tot1[ 1 ] + w2 * totdr[ 1 ];
            v1[ 2 ]     = w1 * tot1[ 2 ] + w2 * totdr[ 2 ];
            v2[ 0 ]     = w1 * tot2[ 0 ] + w2 * totdr[ 3 ];
            v2[ 1 ]     = w1 * tot2[ 1 ] + w2 * totdr[ 4 ];
            v2[ 2 ]     = w1 * tot2[ 2 ] + w2 * totdr[ 5 ];
            v3[ 0 ]     = w1 * tot3[ 0 ] + w2 * totdr[ 6 ];
            v3[ 1 ]     = w1 * tot3[ 1 ] + w2 * totdr[ 7 ];
            v3[ 2 ]     = w1 * tot3[ 2 ] + w2 * totdr[ 8 ];
    
            GLColorTrianglefv ( v1, v2, v3,
                                EegBuff ( *toi1 , tf ), EegBuff ( *toi2 , tf ), EegBuff ( *toi3 , tf ),
                                ColorTable, trilimit, 5 );
            } // AnimFx
        else
                                        // no animation, use the real triangles
            GLColorTrianglefv ( tot1, tot2, tot3,
                                EegBuff ( *toi1 , tf ), EegBuff ( *toi2 , tf ), EegBuff ( *toi3 , tf ),
                                ColorTable, trilimit, 5 );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( CurrentDisplaySpace == DisplaySpaceHeight )

            tot1[ 2 ]   = tot2[ 2 ]   = tot3[ 2 ]  = 0;
        } // ! BadTracks

                            // !increment at the end to avoid buffer overflowing with +3 or +9!
    if ( t < numtri - 1 ) {
        tot1   += 9;    tot2   += 9;    tot3   += 9;
        toi1   += 3;    toi2   += 3;    toi3   += 3;
        }
    } // for t

}; // TriangleLoop


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save current rotation & translation (from here or outside)
MatModelView.CopyModelView ();


timepos     = GetCurrentWindowSize ( how & GLPaintOwner )           // global window size
            * GetCurrentZoomFactor ( how & GLPaintOwner ) * 0.45    // zoom factor
            * ms2 * 0.5;                                            // object size factor


                                        // add a global shift
if ( DepthRange.UserShift ) {
    DepthRange.ShiftCloser ( DepthRange.UserShift );
    DepthRange.GLize ();
    }


tf  = ( ManageRangeCursor == MRCAnimation ) && ! (bool) AnimTF ? MRCNumTF - 1 : 0;

for ( itf = 0; tf < MRCNumTF; tf += MRCStepTF, itf++ ) {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find min/max
    minValue    =  DBL_MAX;
    maxValue    = -DBL_MAX;

    for ( e = 0; e < XYZDoc->GetNumElectrodes (); e++ ) {
                                        // don't account for bad tracks
        if ( BadTracks[ e ] || AuxTracks[ e ] ) continue;

        v   = EegBuff ( e , tf );

        if ( v > maxValue ) {
            maxValue    = v;
            maxIndex    = e;
            XYZDoc->GetCoordinates ( maxIndex, maxPos, CurrentDisplaySpace );
           }

        if ( v < minValue ) {
            minValue    = v;
            minIndex    = e;
            XYZDoc->GetCoordinates ( minIndex, minPos, CurrentDisplaySpace );
            }
        }

                                        // safety clipping for signed data
//  if ( XYZDoc->IsScalar ( AtomTypeUseCurrent ) ) {
//      if ( maxValue < 0 ) maxValue = 0;
//      if ( minValue > 0 ) minValue = 0;
//      }

                                        // to avoid some errors, just in case
    if ( maxValue == minValue ) {
        maxValue += 1e-6;
        minValue -= 1e-6;
        }

    if ( maxValue > GalMaxValue ) {
        GalMaxIndex     = maxIndex;
        GalMaxValue     = maxValue;
        GalMaxPos       = maxPos;
        }

    if ( minValue < GalMinValue ) {
        GalMinIndex     = minIndex;
        GalMinValue     = minValue;
        GalMinPos       = minPos;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute scaling factors
    if ( ScalingAuto != ScalingAutoOff && ! ( RButtonDown && !ControlKey && MouseAxis == MouseAxisHorizontal ) ) {
        if ( minValue == 0 )    minValue = -1e-12;
        if ( maxValue == 0 )    maxValue =  1e-12;

        #define     SPSaturate  0.83
        SetScaling ( minValue * ( SPSaturate + ScalingContrast * 0.15 ),
                     maxValue * ( SPSaturate + ScalingContrast * 0.15 ), 
                     ScalingAuto == ScalingAutoSymmetric );
        }

                                        // contrast factor for the color computation
//  contrast    = 1 + 5 * ScalingContrast * ScalingContrast;
    contrast    = 0.125 + 5.875 * ScalingContrast * ScalingContrast;    // allow a down contrast from 0.5 to 1

                                        // init limits of ColorTable
    ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, RenderingMode == PotentialsRenderingOvercast    ? AlphaSquare :
                                                                   RenderingMode == PotentialsRenderingTransparent ? 0.5         : 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set current position & scaling
    slicex  = itf % slicesPerX;
    slicey  = ( slicesPerY - 1 ) - itf / slicesPerX;

                                        // reset matrix
    glLoadIdentity();
                                        // to add a shift for sequences
    if ( ManageRangeCursor == MRCSequence ) {
//        double  seqscale = 1 / (double) min ( slicesPerX, slicesPerY );
//        if ( seqscale == 1 )
//            seqscale = 0.8;
//        glScaled ( seqscale, seqscale, 1 );

        glTranslatef ( ( slicex - (double) slicesPerX / 2 + 0.5 ) * ms2,
                       ( slicey - (double) slicesPerY / 2 + 0.5 ) * ms2, 0 );
        }
    else if ( ManageRangeCursor == MRCSequenceOverlay ) {
        glTranslatef ( ( slicex - (double) slicesPerX / 2 + 0.5 ) / slicesPerX * ms2 / (double) PaintRect.Height() * PaintRect.Width(), 0, 0 );
        }
    else {
        glTranslatef ( ( slicex - (double) slicesPerX / 2 + 0.5 ) * ms2,
                       ( slicey - (double) slicesPerY / 2 + 0.5 ) * ms2, 0 );
        }


                                        // restore & apply original transform
    MatModelView.GLize ( ApplyToCurrent );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // print the label
   if ( ShowSequenceLabels && ( ManageRangeCursor == MRCSequence || ManageRangeCursor == MRCAnimation )
     && NotSmallWindow () && timepos > 35
     && ( how & GLPaintOwner ) && ( how & GLPaintOpaque ) ) {

        GLColoringOn        ();
    //  GLWriteDepthOff     ();
    //  GLBlendOn           ();

        TextColor.GLize ( 2 );
        SFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

        sprintf ( buff, "%s %0d", EEGDoc->IsTemplates () ? "Template" : "TF", TFCursor.GetPosMin() + tf + EEGDoc->GetStartingTimeFrame () );

        SFont->Print ( 0, 0, 0, buff, TA_LEFT | TA_BOTTOM | TA_BOX, -timepos, -timepos, 0 );


        GLColoringOff       ();
    //  GLWriteDepthOn      ();
    //  GLBlendOff          ();
        } // print time


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the limit for triangle size decomposition
    if ( ! trilimit ) {

        trilimit  = 75 / Zoom / Zoom
                  * XYZDoc->GetMedianDistance ( CurrentDisplaySpace )       // account for electrode resolution: small interval -> increase division
                  / min ( PaintRect.Height(), PaintRect.Width() )           // bigger window -> higher precision
                  * ( ManageRangeCursor != MRCSequenceOverlay ? 2 * max ( slicesPerX, slicesPerY ) : slicesPerX * 0.33 ); // sequence -> less precision

                                        // be more precise while copying
        if ( Outputing () )
//          trilimit /= 2;
            trilimit  = 0;

        if ( CaptureMode == CaptureGLMagnify )
            trilimit /= 2;
                                        // be less precise while animating
        else {
            if ( (bool) AnimFx )
                trilimit *= 2;
            else if ( (bool) AnimTF )
                trilimit *= 2;
            }
        } // ! trilimit


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The opaque stuff should be drawn at first
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the axis unclipped - code is here for multiple TFs
/*
    if ( ! Outputing() && CurrentDisplaySpace == DisplaySpace3D && ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

        GLColoringOn        ();
        GLFogOff            ();

        Xaxis.GLize();
        Yaxis.GLize();
        Zaxis.GLize();

        GLColoringOff       ();
        }
*/
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( CurrentDisplaySpace != DisplaySpace3D || ( how & GLPaintLinked ) )
        GLFogOff            ();
    else
        GLFogOn             ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the electrodes colored by the potential
    if ( RenderingMode == PotentialsRenderingSpheres  && ( how & GLPaintOpaque ) ) {

        GLMaterialOn        ();

        MaterialEl.GLize ();

        TGLBillboardSphere *tobbsphere = Outputing() /*|| VkKey ( 'A' )*/ ? &BbHighSphere : &BbLowSphere;

        tobbsphere->GLize ();               // will do all the job


        for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

            if ( BadTracks[ e ] )   continue;

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
            else
                XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );


            if ( CurrentDisplaySpace == DisplaySpaceHeight ) {
                double  v;
                v   = EegBuff (e,tf);
                if ( v >= 0 )   z =   v / ScalingPMax * ms2 / 3;
                else            z = - v / ScalingNMax * ms2 / 3;
                }

    //      GetColorIndex ( ((TTracksView *) ( CartoolDocManager->GetView ( LinkedViewId ) ) )->EegBuff (e,tf), glcol ); // using Eeg view buffer
            ColorTable.GetColorIndex ( EegBuff ( e , tf ), glcol );
            glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, glcol );

                                        // override colors - result is not exactly the same as MaterialEl.GLize though
            if ( how & GLPaintOwner ) {
                if      ( Highlighted[ e ] )    glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, MaterialElSel.GetDiffuse () );
                else if ( BadTracks  [ e ] )    glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, MaterialElBad.GetDiffuse () );
                }


//            x *= 1 + fabs ( EegBuff (e,tf) ) / 20;
//            y *= 1 + fabs ( EegBuff (e,tf) ) / 20;
//            z *= 1 + fabs ( EegBuff (e,tf) ) / 20;
//            x *= 1 + fabs ( palindex - color0 ) / colordelta / 15;
//            y *= 1 + fabs ( palindex - color0 ) / colordelta / 15;
//            z *= 1 + fabs ( palindex - color0 ) / colordelta / 15;

                                        // slightly bigger than biggest electrode to avoid graphic conflict
            tobbsphere->GLize ( x, y, z, pr * ( PointRadiusRatio[ PointsBig ] + 0.05 ) );
            }

        tobbsphere->unGLize ();
        } // PotentialsRenderingSpheres


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the surface
    if ( RenderingMode == PotentialsRenderingSolid && ( how & GLPaintOpaque ) ) {

//      GLSmoothEdgesOn     ();
        GLColoringOn        ();

        glFrontFace ( GL_CW );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan triangles
//      if ( VkKey ( 'W' ) ) glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

        glBegin ( GL_TRIANGLES );

        TriangleLoop ();

        glEnd();


//      GLSmoothEdgesOff    ();
        GLColoringOff       ();

//      glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

//        if ( how & GLPaintLinked )
//            glDisable ( GL_POLYGON_OFFSET_FILL );
        } // PotentialsRenderingSolid


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the electrodes position
    if ( ShowElectrodes && ( how & GLPaintOpaque ) ) {

        GLMaterialOn        ();

        double          r           = pr * PointRadiusRatio[ ShowElectrodes ];


        TGLBillboardSphere *tobbsphere = Outputing() /*|| VkKey ( 'A' )*/ ? &BbHighSphere : &BbLowSphere;

        tobbsphere->GLize ();               // will do all the job


        for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

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
            else {
                XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );

                if ( CurrentDisplaySpace == DisplaySpaceHeight )
                    z   = ( EegBuff ( e , tf ) >= 0 ? EegBuff ( e , tf ) / ScalingPMax : - EegBuff ( e , tf ) / ScalingNMax ) * ms2 / 3;
                }

            if ( how & GLPaintOwner )
                if      ( Highlighted[ e ] )    MaterialElSel.GLize ();
                else if ( BadTracks  [ e ] )    MaterialElBad.GLize ();
                else                            MaterialEl   .GLize ();
            else                                MaterialEl2  .GLize ();

            tobbsphere->GLize ( x, y, z, Highlighted[ e ] ? HighlightRadiusBoost[ ShowElectrodes ] * r : r );
            }

        tobbsphere->unGLize ();
        } // ShowElectrodes


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw electrodes names
    if ( ShowLabels && ( how & GLPaintTransparent ) ) {

        GLColoringOn        ();
//      GLWriteDepthOff     ();
//      GLBlendOn           ();

                                        // font color
//      TextColor.GLize ( how & GLPaintOwner ? Outputing() ^ ( ShowTextColor == TextColorDark )? 1 : 0
//                                            : Outputing() ^ ( ShowTextColor == TextColorDark )? 3 : 2 );

        TextColor.GLize ( how & GLPaintOwner ? 0 : 2 );

        SFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

        char                label[ ElectrodeNameSize ];
        char                buff [ ElectrodeNameSize ];


        for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

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
            else {
                XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );

                if ( CurrentDisplaySpace == DisplaySpaceHeight )
                    z   = ( EegBuff ( e , tf ) >= 0 ? EegBuff ( e , tf ) / ScalingPMax : - EegBuff ( e , tf ) / ScalingNMax ) * ms2 / 3;

                }

/*
            if ( CurrentDisplaySpace == DisplaySpace3D  )
                x += ( x - ModelCenter.X ) * 0.13,
                y += ( y - ModelCenter.Y ) * 0.13,
                z += ( z - ModelCenter.Z ) * 0.13;

            if ( z == 0 )
                z = XYZDoc->GetBounding ( CurrentDisplaySpace /*DisplaySpaceProjected* / )->Radius() / 15;
*/

            if      ( ShowLabels == ShowLabelText  )    StringCopy ( label, XYZDoc->GetElectrodeName ( e ) );
            else if ( ShowLabels == ShowLabelIndex )    StringCopy ( label, "#", IntegerToString ( buff, e + 1 ) );

//            SFont->Print ( x, y, z, label,
//                           TA_CENTER | ( CurrentDisplaySpace != DisplaySpace3D && ShowElectrodes && RenderingMode != PotentialsRenderingSpheres ? TA_TOP : TA_CENTERY ) | TA_BOX, 0, CurrentDisplaySpace != DisplaySpace3D && ShowElectrodes && RenderingMode != PotentialsRenderingSpheres ? -6 * Zoom * Zoom : 0 );

            SFont->Print ( x, y, z, label,
                           TA_CENTER | ( ShowElectrodes ? TA_TOP : TA_CENTERY ) | TA_BOX,
                           0, ShowElectrodes ? -6 * Zoom * Zoom : 0, 0.025 / ( CurrentDisplaySpace == DisplaySpace3D ? 1 : ExtraDepthFlatView ) );
            }


        if ( CurrentDisplaySpace == DisplaySpace3D )

            for ( int c = 0; c < XYZDoc->GetNumClusters(); c++ ) {
                
                const TElectrodesCluster&   clu     = XYZDoc->GetCluster ( c );
                float                       dx;
                float                       dy;
                float                       dz;

                dx = clu.Bounding.GetXExtent();
                dy = clu.Bounding.GetYExtent();
                dz = clu.Bounding.GetZExtent();

                if      ( dx > dy && dx > dz )
                    x = clu.Bounding.XMin() + dx / 2,
                    y = clu.Bounding.YMin() - dx / 10,
                    z = clu.Bounding.ZMin() - dx / 10;
                else if ( dy > dx && dy > dz )
                    x = clu.Bounding.XMin() - dy / 10,
                    y = clu.Bounding.YMin() + dy / 2,
                    z = clu.Bounding.ZMin() - dy / 10;

                else if ( dz > dx && dz > dy )
                    x = clu.Bounding.XMin() - dz / 10,
                    y = clu.Bounding.YMin() - dz / 10,
                    z = clu.Bounding.ZMin() + dz / 2;

                SFont->Print ( x, y, z, (char *) clu.Name, TA_CENTER | TA_TOP | TA_BOX );
                }


        GLColoringOff       ();
//      GLWriteDepthOn      ();
//      GLBlendOff          ();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The transparent stuff should be drawn at the end
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the min & max positions
    if ( ShowMinMax && ( how & GLPaintOpaque ) ) {

        DrawCross ( minPos, cs * ShowMinMax, true );
        DrawCross ( maxPos, cs * ShowMinMax, false );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the surface
    if ( (    RenderingMode == PotentialsRenderingTransparent 
           || RenderingMode == PotentialsRenderingOvercast   ) 
      && ( how & GLPaintTransparent ) ) {

        GLTransparencyOn    ();
        GLColoringOn        ();
        glFrontFace         ( GL_CW );

                                        // scan triangles
        if ( CurrentDisplaySpace != DisplaySpaceProjected ) {
                                        // draw in 2 passes: first back triangles, then front
            glEnable ( GL_CULL_FACE );

            glCullFace ( GL_BACK );
            glBegin ( GL_TRIANGLES );
            TriangleLoop ();
            glEnd();

            glCullFace ( GL_FRONT );
            glBegin ( GL_TRIANGLES );
            TriangleLoop ();
            glEnd();

            glDisable ( GL_CULL_FACE );
            glCullFace ( GL_BACK );
            }
        else {
            glBegin ( GL_TRIANGLES );
            TriangleLoop ();
            glEnd();
            }


        GLTransparencyOff   ();
        GLColoringOff       ();

//        if ( how & GLPaintLinked )
//            glDisable ( GL_POLYGON_OFFSET_FILL );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the 3D tracks directly from the EEG view
    if ( ShowTracks && EEGView ) {
                                        // eeg view is putting the mess in the matrix, so save it!
        glMatrixMode ( GL_MODELVIEW );
        glPushMatrix ();

        EEGView->GLPaint ( GLPaintLinked | GLPaintOpaqueOrTransparent | GLPaintForceClipping, LayoutOneTrackOneBoxXyz3D, ClipPlane );
                                        // restore transformations
        glMatrixMode ( GL_MODELVIEW );
        glPopMatrix ();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // equator circle & axis
    if ( ShowAxis && ( CurrentDisplaySpace != DisplaySpace3D && XYZDoc->IsHead () && ManageRangeCursor != MRCSequenceOverlay )
      && ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

        GLLinesModeOn       ( false );

                                        // shift only if we are on a 2D world
        if ( CurrentDisplaySpace != DisplaySpaceHeight ) {
            DepthRange.ShiftCloser ();
            DepthRange.GLize ();
            }

        LineColor.GLize ( how & GLPaintOwner ? Outputing() ? 1 : 0 : 2 );


        if ( equ ) {
            glBegin ( GL_LINE_LOOP );
            for ( double angle=0; angle < TwoPi; angle += 0.1 )
                glVertex3f ( cos ( angle ) * equ, sin ( angle ) * equ, 0 );
            glEnd();
            }

                                        // axis
        const TBoundingBox<double>*     pb = XYZDoc->GetBounding ( DisplaySpaceProjected );
        float   delta = pb->MaxSize() / 20;
        float   xmin  = pb->XMin() - delta;
        float   xmax  = pb->XMax() + delta;
        float   ymin  = pb->YMin() - delta;
        float   ymax  = pb->YMax() + delta;

        glBegin ( GL_LINES );
        glVertex3f ( xmin, 0, 0 );
        glVertex3f ( xmax, 0, 0 );
        glVertex3f ( 0, ymin, 0 );
        glVertex3f ( 0, ymax, 0 );
        glEnd();


        GLLinesModeOff      ( false );

        if ( CurrentDisplaySpace != DisplaySpaceHeight ) {
            DepthRange.ShiftFurther ();
            DepthRange.GLize ();
            }
        } // ShowAxis


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for tf

                                        // reset
if ( DepthRange.UserShift ) {
    DepthRange.ShiftFurther ( DepthRange.UserShift );
    DepthRange.GLize ();
    }
}


void    TPotentialsView::Paint ( TDC &dc, bool /*erase*/ , TRect &rect )
{

const TBoundingBox<double>*     b       = XYZDoc->GetBounding ( CurrentDisplaySpace );

if ( ManageRangeCursor == MRCSequence ) {            // switch back to regular resizing if only 1 TF in sequence
    double              radius  = b->MaxSize () / 2 * ( ( MRCNumTF - 1 ) / MRCStepTF > 0 ? ExtraSizeMapSequence : ( CurrentDisplaySpace == DisplaySpace3D ? ExtraSizeXyz3D : ExtraSize3D ) );

    PrePaint ( dc, rect, slicesPerX * radius, slicesPerY * radius, CurrentDisplaySpace != DisplaySpace3D ? ProjModelRadius : ModelRadius );
    }
else {
    double              radius  = b->MaxSize () / 2 * ( CurrentDisplaySpace == DisplaySpace3D ? ExtraSizeXyz3D : ExtraSize3D );

    PrePaint ( dc, rect, radius, radius, CurrentDisplaySpace != DisplaySpace3D ? ProjModelRadius : ModelRadius );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

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


//    if ( Center )                       // slight shift for other linked flat rendering
//        glTranslated ( -Center[0], -Center[1], -Center[2] + 1e-3 );
if ( CurrentDisplaySpace != DisplaySpace3D )
    glTranslated ( -ProjModelCenter.X, -ProjModelCenter.Y, -ProjModelCenter.Z );
else
    glTranslated ( -ModelCenter.X,     -ModelCenter.Y,     -ModelCenter.Z     );

                                        // save this matrix for external drawings
MatProjection.CopyProjection ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.GLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AntialiasingPaint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! Outputing() && CurrentDisplaySpace == DisplaySpace3D && ShowAxis )
    DrawAxis ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.unGLize();

                                        // draw the scaling of the colors & min/max of the map
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( ShowColorScale )

        ColorTable.Draw     (   PaintRect,
                                LineColor[ Outputing() ? 1 : 0 ],   TextColor[ Outputing() ? 1 : 0 ],
                                ColorMapWidth,                      ColorMapHeight,
                                GalMinValue,                        GalMaxValue,
                                true,                               ScalingAuto == ScalingAutoOff,      2,      0,
                                SFont
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw orientation
    if ( ShowOrientation )
        DrawOrientation ( CurrentDisplaySpace != DisplaySpace3D ? XYZDoc->GetProjBoxSides() : XYZDoc->GetBoxSides() );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw title
    if ( GODoc->GetNumXyzDoc() > 1 && ! Outputing() ) {
        TextColor.GLize ( Outputing() ? 1 : 0 );
        SFont->Print ( PaintRect.Width() - 2, PaintRect.Height() - 2, -0.5, (char *) XYZDoc->GetTitle(), TA_RIGHT | TA_TOP );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show informations
    if ( ShowInfos /*&& ! Outputing ()*/ ) {
        TextColor.GLize ( Outputing() ? 1 : 0 );

        double              xpos            = PrintLeftPos;
        double              ypos            = PrintTopPos( PaintRect.Height () );
        double              zpos            = PrintDepthPosBack;
        double              ydelta          = PrintVerticalStep ( SFont );


        SFont->Print ( xpos, ypos, zpos, (char *) XYZDoc->GetTitle (), TA_LEFT | TA_TOP );

        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, DisplaySpaceName[ CurrentDisplaySpace ], TA_LEFT | TA_TOP );


        char                buff [ 256 ];

        if ( ShowMinMax ) {
            glColor4f ( GLBASE_MAXCOLOR, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Max:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Electrode  %s = %0.3g", XYZDoc->GetElectrodeName ( GalMaxIndex ), GalMaxValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMaxPos.X, GalMaxPos.Y, GalMaxPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );
            } // ShowMinMax, Max part


        if ( ShowMinMax ) {
            glColor4f ( GLBASE_MINCOLOR, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Min:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Electrode  %s = %0.3g", XYZDoc->GetElectrodeName ( GalMinIndex ), GalMinValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMinPos.X, GalMinPos.Y, GalMinPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );
            } // ShowMinMax, Min part

        } // ShowInfos


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ResetWindowCoordinates ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TPotentialsView::EvKeyDown ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {
//    case '0': // currently un-implemented
//        CmSetShowZero ();
//        ButtonGadgetSetState ( IDB_SHOWZERO, ShowZero );
//        break;

    case    'G':
        CmMagnifier ();
        break;

    case    'L':
        Cm2Object ();
        break;

    case    'O':
        CmOrient ();
        break;

    case    'R':
        CmSetRenderingMode ();
        break;

    case 'C':
        CmNextColorTable ();
        break;

    case 'D':
        CmSetScalingAdapt ();
        break;

    case 'E':
        CmSetShowElectrodes ();
        break;

    case 'F':
    case '2': // for compatibility with Inverses
        CmSetFlatView ();
        break;

    case 'M':
        CmSetShowMinMax ();
        break;

    case 'N':
        CmSetShowElectrodesNames ();
        break;

    case 'T':
        CmSetShowTracks ();
        break;

//    case 'T':
//        CmSetShowTriangles ();
//        ButtonGadgetSetState ( IDB_SHOWTRIANGLES, ShowTriangles );
//        break;

    case 'X':
        ModelRotMatrix.RotateY ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
//      Invalidate ( false );
        ShowNow ();
        break;

    case 'Y':
        ModelRotMatrix.RotateX ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
//      Invalidate ( false );
        ShowNow ();
        break;

    case 'Z':
        ModelRotMatrix.RotateZ ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
//      Invalidate ( false );
        ShowNow ();
        break;

    case VK_UP:
        if ( ControlKey )
            CmSetBrightness (IDB_SPINCBRIGHT);
        else if ( ShiftKey )
            CmSetContrast (IDB_SPINCCONTRAST);
        break;

    case VK_DOWN:
        if ( ControlKey )
            CmSetBrightness (IDB_SPDECBRIGHT);
        else if ( ShiftKey )
            CmSetContrast (IDB_SPDECCONTRAST);
         break;

    case VK_RETURN:
        EvLButtonDblClk ( 0, TPoint() );
        break;

                                        // used for forwarding
    default:
        TSecondaryView::EvKeyDown ( key, repeatCount, flags );
        break;
    }
}


void    TPotentialsView::EvKeyUp ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyUp ( key, repeatCount, flags );


if ( LinkedViewId )
    GetEegView ()->EvKeyUp ( key, repeatCount, flags );
}


void    TPotentialsView::EvSize ( uint sizeType, const TSize &size )
{
SetItemsInWindow ();

TBaseView::EvSize ( sizeType, size );
}


void    TPotentialsView::SetItemsInWindow ()
{
int     n = (double) MRCNumTF / MRCStepTF;
if ( (double) MRCNumTF / MRCStepTF - n > 0 )
    n++;

if ( ManageRangeCursor == MRCSequence )
    FitItemsInWindow ( n,
                       TSize ( ElectrodesSliceSize, ElectrodesSliceSize ), slicesPerX, slicesPerY );
else if ( ManageRangeCursor == MRCSequenceOverlay ) {
    slicesPerX  = n;
    slicesPerY  = 1;
    }
else
    FitItemsInWindow ( 1,
                       TSize ( ElectrodesSliceSize, ElectrodesSliceSize ), slicesPerX, slicesPerY );
}


void    TPotentialsView::EvLButtonDblClk ( uint, const TPoint & )
{
TBaseView*      v  = CartoolDocManager->GetView ( LinkedViewId );

if ( v != 0 )
    v->SetFocusBack ( GetViewId () );   // come back to me !
}


//----------------------------------------------------------------------------
void    TPotentialsView::EvLButtonDown ( uint i, const TPoint &p )
{
MousePos   = p;

                                        // GL biz?
if      ( CaptureMode == CaptureGLLink || CaptureMode == CaptureGLMagnify ) {

    GLLButtonDown ( i, p );

    return;
    }

else if ( /*ControlKey ||*/ VkKey ( MouseMiddeClicksurrogate ) ) {  // make an alias Ctrl-Left = Middle

    EvMButtonDown ( i, p );

    return;
    }

else if ( CaptureMode == CaptureNone ) {

    LButtonDown = true;

    CaptureMode = CaptureLeftButton;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }

else
    TSecondaryView::EvLButtonDown ( i, p );
}


void    TPotentialsView::EvLButtonUp ( uint i, const TPoint &p )
{
MouseAxis           = MouseAxisNone;


if      ( CaptureMode == CaptureGLSelect ) {  // make an alias Ctrl-Left = Middle

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
void    TPotentialsView::EvMButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos    = p;

MButtonDown = true;

CaptureMode = CaptureGLSelect;

CaptureMouse ( Capture );

EvMouseMove ( i, p );

                                        // resetting everything can only be done at clicking time
if ( ( ShowElectrodes || RenderingMode == PotentialsRenderingSpheres ) && Picking.IsNull () ) {

    Highlighted.Reset ();

    XYZDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

    ShowNow ();
    }
}


void    TPotentialsView::EvMButtonUp ( uint, const TPoint & )
{
MouseAxis           = MouseAxisNone;


if      ( CaptureMode == CaptureGLSelect )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


MButtonDown = false;

CaptureMouse ( Release );

//GetParentO()->SetCaption ( BaseDoc->GetTitle() );
UpdateCaption ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TPotentialsView::EvRButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );

MousePos   = p;
}


void    TPotentialsView::EvRButtonUp ( uint, const TPoint & )
{
MouseAxis   = MouseAxisNone;


if      ( CaptureMode == CaptureRightButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


RButtonDown = false;
ShiftAxis   = ShiftNone;

CaptureMouse ( Release );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TPotentialsView::EvMouseMove ( uint i, const TPoint &p )
{
int                 dx              = p.X () - MousePos.X ();
int                 dy              = p.Y () - MousePos.Y ();
int                 adx             = abs ( dx );
int                 ady             = abs ( dy );


if ( RButtonDown ) {

    if ( adx >= MinMouseMove || ady >= MinMouseMove ) {

        if      ( adx > ady * ( MouseAxis == MouseAxisVertical   ? 3 : 1 ) )
            MouseAxis   = MouseAxisHorizontal;

        else if ( ady > adx * ( MouseAxis == MouseAxisHorizontal ? 3 : 1 ) )
            MouseAxis   = MouseAxisVertical;
        }

    if ( MouseAxis == MouseAxisNone )
        return;


//    clipfront[3] += (double) ( p.Y() - MousePos.Y() ) / 10;
    double              osl             = ScalingLevel;
    double              osc             = ScalingContrast;

    if ( MouseAxis == MouseAxisHorizontal ) {

        if ( dx > 0 )   ScalingLevel /= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.3 );
        else            ScalingLevel *= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.3 );

        SetScaling ( ScalingLevel );
        }
    else if ( MouseAxis == MouseAxisVertical ) {

        ScalingContrast -= (double) dy / 150;

        SetScalingContrast ( ScalingContrast );
        }

    if ( osl == ScalingLevel && osc == ScalingContrast )
        return;

//  Invalidate ( false );
    ShowNow ();
    }
//else if ( LButtonDown )
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

        if ( ShowElectrodes || RenderingMode == PotentialsRenderingSpheres )

            if ( ShiftKey ) {

                BadTracks.Invert ( hit );

                EEGDoc->SetBadTracks ( &BadTracks );
                }
            else { // if ( ControlKey ){      // control + middle = select
                Highlighted.Invert ( hit );

                XYZDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );
                }

        ShowNow ();
        }


    lasthit = hit;
    }


LastMouseMove       = TPoint ( dx, dy );
MousePos            = p;
}


//----------------------------------------------------------------------------
void    TPotentialsView::FitItemsInWindow ( int numitems, TSize itemsize, int &byx, int &byy, TRect *winrect )
{
TBaseView::FitItemsInWindow ( numitems, itemsize, byx, byy, winrect );
}


//----------------------------------------------------------------------------
void    TPotentialsView::CmOrient ()
{
if ( CurrentDisplaySpace != DisplaySpace3D ) {
                                        // projection resets orientation -Y / X, so we can overlap different setups
    Orientation         = OrientMinusYX;

    ModelRotMatrix.SetOrientation ( Orientation );
    }
else {
    TBaseView::CmOrient ();

    SetOrient ( XYZDoc );
    }

Invalidate ( false );
}


void    TPotentialsView::CmSetBrightness ( owlwparam wparam )
{
double  osl = ScalingLevel;

if      ( wparam == IDB_SPINCBRIGHT )   ScalingLevel /= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep ;
else if ( wparam == IDB_SPDECBRIGHT )   ScalingLevel *= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep;

SetScaling ( ScalingLevel );

if ( osl == ScalingLevel )
    return;

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetContrast ( owlwparam wparam )
{
double  osc  = ScalingContrast;

if      ( wparam == IDB_SPINCCONTRAST )     ScalingContrast += 0.10;
else if ( wparam == IDB_SPDECCONTRAST )     ScalingContrast -= 0.10;

SetScalingContrast ( ScalingContrast );

if ( osc == ScalingContrast )
    return;

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetIntensityLevelEnable ( TCommandEnabler &tce )
{
tce.Enable ( ScalingAuto == ScalingAutoOff );
}


void    TPotentialsView::CmSetShowElectrodes ()
{
if ( ShiftKey )
    ShowElectrodes   = ( ShowElectrodes + PointsNumRendering - 1 ) % PointsNumRendering;
else
    ShowElectrodes   = ( ShowElectrodes + 1 ) % PointsNumRendering;


//Invalidate ( false );
ShowNow ();

ButtonGadgetSetState ( IDB_SHOWELEC, ShowElectrodes );
}


void    TPotentialsView::CmSetShowTracks ()
{
ShowTracks  = ! ShowTracks;

if ( ShowTracks )
    GetEegView ()->Set3DMode ( CurrentDisplaySpace );

Invalidate ( false );
ButtonGadgetSetState ( IDB_SHOWTRACKS, ShowTracks );
}


void    TPotentialsView::CmSetShowTriangles ()
{
ShowTriangles   = ! ShowTriangles;
Invalidate ( false );
}


void    TPotentialsView::CmSetShowElectrodesNames ()
{
ShowLabels      = (ShowLabelEnum) ( ( ShowLabels + ( ShiftKey ? NumShowLabels - 1 : 1 ) ) % NumShowLabels );

ButtonGadgetSetState ( IDB_SHOWELNAMES, ShowLabels );

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmNextColorTable ()
{
ColorTable.NextColorTable ( ShiftKey );

ColorTableIndex[ EEGDoc->GetAtomType ( AtomTypeUseCurrent ) ]    = ColorTable.GetTableType ();

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetShowZero ()
{
ShowZero    = ! ShowZero;

Invalidate ( false );
}


void    TPotentialsView::CmSetShowMinMax ()
{
if ( ShiftKey )     ShowMinMax  = ( ShowMinMax + PotentialsNumMinMax - 1 ) % PotentialsNumMinMax;
else                ShowMinMax  = ( ShowMinMax + 1                       ) % PotentialsNumMinMax;

ButtonGadgetSetState ( IDB_SHOWPMMINMAX, ShowMinMax );

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetStepTF ( owlwparam wparam )
{
if ( wparam ) {
    int     oldStepTF = MRCStepTF;

    TSecondaryView::CmSetStepTF ( wparam );

    SetItemsInWindow ();

    if ( oldStepTF != MRCStepTF && IsMRCSequence () )
        Invalidate ( false );
    }
else {
    MRCStepTF = (double) sqrt ( MRCNumTF / 6.0 ); // until 24, step = 1 ( 24 / 6 = 4 -> 2 )

    if ( MRCStepTF == 0 || EEGDoc->IsTemplates () )
        MRCStepTF = 1;
    }
}


void    TPotentialsView::CmNextXyz ()
{
int     oldcxyz = CurrXyz;

if ( ShiftKey )
    CurrXyz = ( CurrXyz + GODoc->GetNumXyzDoc() - 1 ) % GODoc->GetNumXyzDoc();
else
    CurrXyz = ( CurrXyz + 1 ) % GODoc->GetNumXyzDoc();

if ( oldcxyz == CurrXyz )   return;

XYZDoc = GODoc->GetXyzDoc ( CurrXyz );
                                        // group all init that is MRI dependent
InitXyz ();

Invalidate ( false );

//GetEegView ()->Set3DMode ( CurrentDisplaySpace );

XYZDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_DOCPOINTERS, this, XYZDoc );
}


void    TPotentialsView::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc->GetNumXyzDoc() > 1 );
}


void    TPotentialsView::CmSetScalingAdapt ()
{
ScalingAuto     = NextState ( ScalingAuto, EEGDoc->IsPositive ( AtomTypeUseCurrent ) ? NumScalingAutoPositive : NumScalingAuto, ShiftKey );

SetScaling ( ScalingLevel );

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetInterpolation ( owlwparam w )
{
if      ( w == CM_INTERPOL_N )  RenderingMode = PotentialsRenderingSpheres;
else if ( w == CM_INTERPOL_Y )  RenderingMode = PotentialsRenderingSolid;

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetManageRangeCursor ( owlwparam wparam )
{
                                        // stop if already running
if ( (bool) AnimTF ) {
    TFCursor.SetPos ( SavedTFCursor.GetPosMax() );
    ButtonGadgetSetState ( IDB_RANGEANI, true );
    return;
    }

MRCNumTF       = TFCursor.GetPosMax() - TFCursor.GetPosMin() + 1;


GetTracks ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


switch  ( wparam ) {

    case    IDB_RANGEAVE:

        ManageRangeCursor   = MRCAverage;
        Average ();
        MRCNumTF            = 1;

        break;

    case    IDB_RANGESEQ:

        if      ( ManageRangeCursor == MRCSequence          )   ManageRangeCursor   = MRCSequenceOverlay;
        else if ( ManageRangeCursor == MRCSequenceOverlay   )   ManageRangeCursor   = MRCSequence;
        else                                                    ManageRangeCursor   = MRCSequence;

        CmSetStepTF ( 0 );

        break;

    case    IDB_RANGEANI:

        ManageRangeCursor   = MRCAnimation;
//      MRCDoAnimation      = false;

        AnimTF.Stop ();

        if ( TFCursor.GetLength() != 1 ) {
                                        // save cursor, new one of length 1
//          MRCDoAnimation      = true;
            MRCNumTF            = 1;
            SavedTFCursor       = TFCursor;
            TFCursor.SetPos ( TFCursor.GetPosMin() - 1 );

            AnimTF.Start ( TimerTFCursor, MRCStepTF <= 4 ? 20 : 2 * MRCStepTF + 20, SavedTFCursor.GetLength() );
            }

        break;
    }


SetItemsInWindow ();

Invalidate ( false );

//ButtonGadgetSetState ( IDB_RANGEAVE, ManageRangeCursor == MRCAverage   );
//ButtonGadgetSetState ( IDB_RANGESEQ, IsMRCSequence ()                  );
//ButtonGadgetSetState ( IDB_RANGEANI, ManageRangeCursor == MRCAnimation );
}


//----------------------------------------------------------------------------
void    TPotentialsView::EvTimer ( uint timerId )
{
switch ( timerId ) {

    case TimerTFCursor:

        AnimTF++;
                                        // finished?     TWindow
        if ( TFCursor.GetPosMax() >= SavedTFCursor.GetPosMax() ) {

            AnimTF.Stop ();

//          MRCDoAnimation = false;
                                        // reload everything
            TFCursor    = SavedTFCursor;
            MRCNumTF    = TFCursor.GetPosMax() - TFCursor.GetPosMin() + 1;
            }
        else {                          // allow to change speed
            AnimTF.SetInterval ( MRCStepTF <= 4 ? 20 : 2 * MRCStepTF + 20 );

            TFCursor.ShiftPos  ( MRCStepTF < 4 ? 1 << ( 4 - MRCStepTF ) : 1 );
            }

        GetTracks ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

        UpdateCaption ();
        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

        EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

        break;


    case TimerStartup:

        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx ) {

            AnimFx.Stop ();

            Zoom    = 1;
            Orientation         = DefaultPotentialOrientation - 1;
            CmOrient ();

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        Zoom    = sqrt ( AnimFx.GetPercentageCompletion() );
        ModelRotMatrix.RotateX ( - 360.0 / ( AnimFx.GetMaxCount() - 1 ), MultiplyLeft );

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
void    TPotentialsView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& p )
{
TBaseView::EvMouseWheel ( modKeys, zDelta, p );
}


//----------------------------------------------------------------------------
void    TPotentialsView::CmSetRenderingMode ()
{
RenderingMode   = NextState ( RenderingMode, (int) NumPotentialsRenderings, ShiftKey );

SetItemsInWindow ();

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetFlatView ()
{
CurrentDisplaySpace     = NextState ( CurrentDisplaySpace, (int) GetDisplaySpaces (), ShiftKey );


ButtonGadgetSetState ( IDB_FLATVIEW, CurrentDisplaySpace );

if ( (bool) AnimFx && AnimFx.GetTimerId () == TimerStartup )
    Zoom = 1;


AnimFx.Start ( CurrentDisplaySpace != DisplaySpace3D ? TimerToFlat : TimerTo3D, 10, 10 );


if ( CurrentDisplaySpace == DisplaySpace3D )
    Orientation         = DefaultPotentialOrientation - 1; // would be better to recall the last 3D orientation

CmOrient ();


SetItemsInWindow ();

ShowTracks  = ! ShowTracks;
CmSetShowTracks ();

//Invalidate ( false );
ShowNow ();
}


void    TPotentialsView::CmSetFlatViewEnable ( TCommandEnabler &tce )
{
tce.Enable ( true );
}


void    TPotentialsView::CmShowColorScale ()
{
ShowColorScale  = ! ShowColorScale;

SetItemsInWindow ();

Invalidate ( false );
}


void    TPotentialsView::CmShowAxis ()
{
TBaseView::CmShowAxis ();

SetItemsInWindow ();

Invalidate ( false );
}


void    TPotentialsView::CmShowAll ( owlwparam w )
{
TBaseView::CmShowAll ( w );

SetItemsInWindow ();

Invalidate ( false );
}


void    TPotentialsView::CmSetShiftDepthRange ( owlwparam w )
{
TBaseView::CmSetShiftDepthRange ( w );

SetItemsInWindow ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TPotentialsView::Average ()
{
                                        // compute the average of the TF's
if ( TFCursor.IsSplitted() )
    EegBuff.AverageDim2 ( MRCNumTF );
}


bool    TPotentialsView::VnNewTFCursor ( TTFCursor *tfc )
{
#ifdef CARTOOL_DEBUGMESSAGES
DBGV4 ( tfc->SentFrom, tfc->SentTo, tfc->GetPosMin (), tfc->GetPosMax (), "TPotentialsView::VnNewTFCursor  From To  Min Max" );
#endif // CARTOOL_DEBUGMESSAGES


if ( ! IsFriendView ( tfc->SentTo ) )
    return false;                       // not for me !

                                        // no need to update
if ( *tfc == TFCursor )     return false;


long    oldMRCNumTF = TFCursor.GetLength ();


TFCursor            = *tfc;             // transfer positions
TFCursor.SentTo     = LinkedViewId;
TFCursor.SentFrom   = GetViewId ();


MRCNumTF       = TFCursor.GetLength ();


GetTracks ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


if ( ManageRangeCursor == MRCAverage ) {
    Average ();
    MRCNumTF   = 1;
    }
else if ( IsMRCSequence () ) {
    if ( MRCNumTF != oldMRCNumTF )      // reset only in case user changed the selection size
        CmSetStepTF ( 0 );              // set a default spacing
    }


UpdateCaption ();

AnimTF.Stop ();

SetItemsInWindow ();

ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

                                        // send the cursor to the EEG view also, if it is not the original sender
if ( tfc->SentFrom != LinkedViewId )
    EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

return false;
}


bool    TPotentialsView::VnReloadData (int what)
{
switch ( what ) {

  case EV_VN_RELOADDATA_REF:
  case EV_VN_RELOADDATA_EEG:

    MRCNumTF       = TFCursor.GetPosMax() - TFCursor.GetPosMin() + 1;

    GetTracks ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

    SearchAndSetIntensity ( false );

    SetColorTable ( EEGDoc->GetAtomType ( AtomTypeUseCurrent ) );

    if ( ManageRangeCursor == MRCAverage ) {
        Average ();
        MRCNumTF   = 1;
        }

    SetItemsInWindow ();

//  Invalidate ( false );
    ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

    break;


  case EV_VN_RELOADDATA_TRG:

    break;


  case EV_VN_RELOADDATA_CAPTION:

    UpdateCaption ();

    break;
    }

return true;
}


bool    TPotentialsView::VnNewHighlighted ( TSelection *sel )
{
    // message can be either for electrodes or SPs                                if 0, accept all
if ( abs ( sel->Size () - Highlighted.Size () ) > NumPseudoTracks || sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                        // not for me !

//if ( ! IsFriendView ( hl->SentTo ) )
//    return false;                       // not for me !


if ( Highlighted != *sel ) {
    Highlighted  = *sel;

    if ( ShowElectrodes || RenderingMode == PotentialsRenderingSpheres )
        ShowNow ();
    }


return  true;
}


bool    TPotentialsView::VnNewBadSelection ( TSelection *sel )
{
  // if 0, accept all
if ( sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                        // not for me !

if ( BadTracks != *sel ) {

    BadTracks  = *sel;

    ShowNow ();
    }

return  true;
}


bool    TPotentialsView::VnViewUpdated ( TBaseView *view )
{
if ( !view )
    return true;

                                        // listen to EEG view, and try to follow the mode if in 3D
if ( ShowTracks && view == GetEegView () ) {
    int     current3dset = GetEegView ()->GetCurrentSpace ();
                                        // the EEG view does not (yet?) handle DisplaySpaceHeight
    if ( current3dset != DisplaySpaceNone && ! ( current3dset == DisplaySpaceProjected && CurrentDisplaySpace != DisplaySpace3D ) )
        CurrentDisplaySpace     = current3dset;
    }


//Invalidate ( false );
ShowNow ();

return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
