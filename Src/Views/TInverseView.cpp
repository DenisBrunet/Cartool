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

#include    "TInverseView.h"

#include    "MemUtil.h"
#include    "Math.Stats.h"
#include    "Math.Resampling.h"
#include    "TArray1.h"
#include    "TArray3.h"
#include    "OpenGL.h"
#include    "TInterval.h"
#include    "TWeightedPoints.h"
#include    "Volumes.TTalairachOracle.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Files.PreProcessFiles.h"

#include    "TExportTracks.h"

#include    "TRisDoc.h"
#include    "TLinkManyDoc.h"

#include    "TVolumeView.h"
#include    "TTracksView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle    Taloracle;



//----------------------------------------------------------------------------

    TInverseVolumeState::TInverseVolumeState ()    
{
Show2DIs        = InverseRendering2DNone;
Show3DIs        = InverseRendering3DNone;
ShowVectorsIs   = InverseRenderingVectorsNone;
Show3DMri       = MriNone;
ShowSp          = PointsShowNone; 
//ClipPlane[]
ModelRotMatrix.SetIdentity ();
}


    void    TInverseState::Reset ()
{
DisplayScaling.ScalingLimitMax  = -1; 
CurrReg                         = -1; 
IsSpreading                     = 1; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TInverseView, TSecondaryView)

    EV_WM_SIZE,
    EV_WM_TIMER,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_MOUSEWHEEL,

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

    EV_VN_NEWTFCURSOR,
    EV_VN_RELOADDATA,
    EV_VN_NEWHIGHLIGHTED,
    EV_VN_VIEWUPDATED,

    EV_COMMAND          ( IDB_2OBJECT,              Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,               CmOrient ),
    EV_COMMAND          ( IDB_MAGNIFIER,            CmMagnifier ),
    EV_COMMAND          ( IDB_SHOW3DIS,             CmSetShow3DIs ),
    EV_COMMAND          ( IDB_SHOWVECTORSIS,        CmSetShowVectorsIs ),
    EV_COMMAND          ( IDB_SHOW3DMRI,            CmSetShow3DMri ),
    EV_COMMAND          ( IDB_PLANEMODE,            CmSetSliceMode ),
    EV_COMMAND_AND_ID   ( IDB_LESSSLICES,           CmSetNumSlices ),
    EV_COMMAND_AND_ID   ( IDB_MORESLICES,           CmSetNumSlices ),
    EV_COMMAND_AND_ID   ( IDB_FIRSTSLICEFWD,        CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_FIRSTSLICEBWD,        CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_LASTSLICEFWD,         CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_LASTSLICEBWD,         CmMoveSlice ),
    EV_COMMAND_ENABLE   ( IDB_LESSSLICES,           CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_MORESLICES,           CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_FIRSTSLICEFWD,        CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_FIRSTSLICEBWD,        CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_LASTSLICEFWD,         CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_LASTSLICEBWD,         CmSliceModeEnable ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANECORONAL,      CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANETRANSVERSE,   CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANESAGITTAL,     CmSetCutPlane ),
    EV_COMMAND_ENABLE   ( IDB_CUTPLANECORONAL,      CmNotSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_CUTPLANETRANSVERSE,   CmNotSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_CUTPLANESAGITTAL,     CmNotSliceModeEnable ),
    EV_COMMAND_AND_ID   ( IDB_SHOW2DIS,             CmSetShow2DIs ),
    EV_COMMAND_ENABLE   ( IDB_SHOW2DIS,             CmSetCutPlaneEnable ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPINC,           CmSetStepTF ),
    EV_COMMAND_AND_ID   ( IDB_RGESTEPDEC,           CmSetStepTF ),
    EV_COMMAND_AND_ID   ( IDB_SHOWMIN,              CmShowMinMax ),
    EV_COMMAND_AND_ID   ( IDB_SHOWMAX,              CmShowMinMax ),
    EV_COMMAND_ENABLE   ( IDB_SHOWMIN,              CmIsScalarEnable ),
    EV_COMMAND_AND_ID   ( IDB_ISINCBRIGHT,          CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_ISDECBRIGHT,          CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_ISINCCONTRAST,        CmSetContrast ),
    EV_COMMAND_AND_ID   ( IDB_ISDECCONTRAST,        CmSetContrast ),
    EV_COMMAND          ( IDB_FIXEDSCALE,           CmSetScalingAdapt ),
    EV_COMMAND          ( IDB_SPCOLOR,              CmNextColorTable ),

    EV_COMMAND_AND_ID   ( IDB_RESETREG,             CmReg ),
    EV_COMMAND_AND_ID   ( IDB_PREVREG,              CmReg ),
    EV_COMMAND_AND_ID   ( IDB_NEXTREG,              CmReg ),
    EV_COMMAND_AND_ID   ( IDB_SEARCHBESTREGCURSOR,  CmSearchRegularization ),
    EV_COMMAND_AND_ID   ( IDB_SEARCHBESTREGGLOBAL,  CmSearchRegularization ),
    EV_COMMAND_ENABLE   ( IDB_RESETREG,             CmNextRegEnable ),
    EV_COMMAND_ENABLE   ( IDB_PREVREG,              CmNextRegEnable ),
    EV_COMMAND_ENABLE   ( IDB_NEXTREG,              CmNextRegEnable ),
    EV_COMMAND_ENABLE   ( IDB_SEARCHBESTREGCURSOR,  CmNextRegEnable ),
    EV_COMMAND_ENABLE   ( IDB_SEARCHBESTREGGLOBAL,  CmNextRegEnable ),

    EV_COMMAND          ( IDB_NEXTIS,               CmNextIs ),
    EV_COMMAND_ENABLE   ( IDB_NEXTIS,               CmNextIsEnable ),
    EV_COMMAND          ( IDB_NEXTMRI,              CmNextMri ),
    EV_COMMAND_ENABLE   ( IDB_NEXTMRI,              CmNextMriEnable ),

    EV_COMMAND          ( IDB_FINDMINMAX,           CmSetFindMinMax ),
    EV_COMMAND_ENABLE   ( IDB_ISINCBRIGHT,          CmSetIntensityLevelEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISDECBRIGHT,          CmSetIntensityLevelEnable ),
    EV_COMMAND          ( IDB_SHOWSP,               CmSetShowSP ),
//  EV_COMMAND_ENABLE   ( IDB_SHOWSP,               CmIsMriEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISINCCONTRAST,        CmIsEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISDECCONTRAST,        CmIsEnable ),
    EV_COMMAND_ENABLE   ( IDB_NEXTMRI,              CmMriEnable ),
//  EV_COMMAND_ENABLE   ( IDB_SHOWMIN,              CmIsEnable ),
//  EV_COMMAND_ENABLE   ( IDB_SHOWMAX,              CmIsEnable ),
    EV_COMMAND_ENABLE   ( IDB_FIXEDSCALE,           CmIsEnable ),
    EV_COMMAND_ENABLE   ( IDB_SPCOLOR,              CmIsEnable ),
    EV_COMMAND_ENABLE   ( IDB_FINDMINMAX,           CmSlicesEnable ),

    EV_COMMAND          ( IDB_NEXTROI,              CmNextRois ),
    EV_COMMAND_ENABLE   ( IDB_NEXTROI,              CmNextRoisEnable ),

    EV_COMMAND          ( CM_SEARCHTFMAX,           CmSearchTFMax ),

    EV_COMMAND          ( CM_SHOWSEQUENCELABEL,     CmShowSequenceLabels ),

    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE0,      CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE1,      CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE2,      CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE3,      CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE4,      CmSetShiftDepthRange ),

    EV_COMMAND_AND_ID   ( CM_INVERSEAVERAGEBEFORE,  CmSetAveragingMode ),
    EV_COMMAND_AND_ID   ( CM_INVERSEAVERAGEAFTER,   CmSetAveragingMode ),
    EV_COMMAND_ENABLE   ( CM_INVERSEAVERAGEBEFORE,  CmPrecedenceBeforeEnable ),
    EV_COMMAND_ENABLE   ( CM_INVERSEAVERAGEAFTER,   CmPrecedenceAfterEnable  ),
END_RESPONSE_TABLE;


        TInverseView::TInverseView ( TTracksDoc &doc, TWindow *parent, TLinkManyDoc *group )
      : TSecondaryView ( doc, parent, group )
{
SPDoc               = 0;
ISDoc               = 0;

CurrReg             = 0;
CurrIs              = -1;
CurrRis             = -1;
MRIDocClipp         = 0;
MRIDocBackg         = 0;
CurrMri             = 0;
Rois                = 0;
CurrRois            = 0;


if ( ! ValidView () ) {
    NotOK ();
    return;
    }

                                        // look for SPs
for ( int i = 0; i < GODoc->GetNumSpDoc (); i++ )
    if ( GODoc->GetSpDoc ( i ) ) {
        SPDoc       = GODoc->GetSpDoc ( i );
        break;
        }

                                        // use the grey as the clipping, the head as current display
TVolumeDoc*         tohead;
TVolumeDoc*         tobrain;
TVolumeDoc*         togrey;
int                 headi;
int                 braini;

GODoc->GuessHeadBrainGreyMris ( tohead, tobrain, togrey, &headi, &braini );

MRIDocClipp     = togrey;
MRIDocBackg     = tobrain;
CurrMri         = braini;

                                        // group all init that is MRI dependent
InitMri ();

SetOrient ( MRIDocBackg );


OnlyRis             = EEGDoc->ExtensionIs ( FILEEXT_RIS );

if ( OnlyRis ) {                        // not allowed to GetIs, cause EEG is RIS here !

    TBaseDoc*           lastis          = CartoolDocManager->GetView ( GODoc->LastEegViewId )->BaseDoc;

    for ( int i = 0; i < GODoc->GetNumRisDoc (); i++ )

        if ( GODoc->GetRisDoc ( i ) == lastis ) {
            CurrIs  = -1;
            CurrRis = i;
            ISDoc   = GODoc->GetRisDoc ( CurrRis );
            }
    }
else {
    CurrIs  = 0;
    CurrRis = -1;
    ISDoc   = GODoc->GetIsDoc ( CurrIs );
    CurrReg = ISDoc->GetRegularizationIndex ( DefaultRegularization );
    }


if ( CurrIs == -1 && CurrRis == -1 ) {  // should never happen
    NotOK ();
    return;
    }


//MRIDocClipp->GetViewList()->AddLink ( this );
//GLObjects.Add ( MRIDocClipp->GetViewList() );



Show2DIs            = InverseRendering2DDefault;
Show3DIs            = InverseRendering3DDefault;
Show3DMri           = MriSliceOpaque;
ShowSp              = PointsShowNone;
ShowVectorsIs       = InverseRenderingVectorsDefault;
IsRoiMode           = false;
SliceMode           = SliceModeNone;
FindMinMax          = false;
Orientation         = DefaultVolumeOrientation - 1;


InvBuffS.Resize ( SPDoc->GetNumSolPoints () );
//InvBuffSSum.Resize ( SPDoc->GetNumSolPoints () );
InvBuffV.Resize ( SPDoc->GetNumSolPoints () );

SPVol.Resize    ( SPDoc->GetVolumeSize () );

SelSP               = TSelection ( SPDoc->GetNumSolPoints (), OrderSorted );
SelSP.Set ();


Highlighted         = TSelection ( SPDoc->GetNumSolPoints (), OrderSorted );
Highlighted.Reset ();
if ( CurrRis >= 0 )
    GetEegView ()->GetHighlighted ( &Highlighted );     // should be working with .ris file
Highlighted.SentTo   = 0;
Highlighted.SentFrom = GetViewId();


ShowMin             = InverseMinMaxNone;
ShowMax             = InverseMinMaxNone;


ManageRangeCursor   = EEGDoc->IsTemplates () ? MRCSequence : MRCAverage;
MRCStepTF           = MRCStepInit;


IsSpreading         = 1;


TFCursor.SetPos ( SearchAndSetIntensity ( false ) );

//EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // Maybe put that into a function? or use some OriginalAtomType?
IsSignedData        = ! ISDoc ->IsAbsolute ( AtomTypeUseCurrent );


SetColorTable ( ISDoc->GetAtomType ( AtomTypeUseCurrent ) );


#define     ScalingContrastInit     ( ScalingMinContrast + 0.070 * ScalingMaxContrast )
SetScalingContrast ( ScalingContrastInit );



Slices[ SliceModeCor - 1 ] = TSliceSelection ( MRIDocClipp->GetFirstSlice ( CorronalIndex   () ), MRIDocClipp->GetLastSlice ( CorronalIndex   () ) );
Slices[ SliceModeTra - 1 ] = TSliceSelection ( MRIDocClipp->GetFirstSlice ( TransverseIndex () ), MRIDocClipp->GetLastSlice ( TransverseIndex () ) );
Slices[ SliceModeSag - 1 ] = TSliceSelection ( MRIDocClipp->GetFirstSlice ( SagittalIndex   () ), MRIDocClipp->GetLastSlice ( SagittalIndex   () ) );


Zoom                = 1; // CartoolApplication->AnimateViews && GODoc->GetNumEegDoc () + GODoc->GetNumRisDoc() <= 4 ? 0.001 : 1;


RenderingMode       = 0;                // not used
CurrentDisplaySpace = DisplaySpace3D;
RedoSurfaceIS       = true;
DepthRange.UserShift= 0;

                                        // preset the isosurface main parameters
IsoParam.How                = IsosurfaceMarchingCube;
IsoParam.IsoValue           = 0;
IsoParam.Downsampling       = 1;
IsoParam.UseOriginalVolume  = true;     // we are providing a volume to speed up things
IsoParam.SmoothData         = FilterTypeNone;
IsoParam.SmoothGradient     = IsoParam.SmoothData == FilterTypeNone;
//IsoParam.Rescale          = SPDoc->GetVoxelSize () * ( SPDoc->IsGridAligned() ? SolutionPointsSubSampling : 1 );
IsoParam.Rescale            = SPDoc->GetVoxelSize ();
IsoParam.Origin             = MRIDocBackg->GetOrigin ();


ClipPlane[ 0 ].SetBackward ();
ClipPlane[ 1 ].SetBackward ();
ClipPlane[ 2 ].SetBackward ();


/*
MaterialIs  = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      0.40,  0.40,  0.10,  1.00,
                                      0.85,  0.85,  0.18,  0.45,
                                      0.50,  0.50,  0.50,  1.00,
                                      0.00,  0.00,  0.00,  0.00,
                                     30.00 );
*/
MaterialIs  = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 0.10,  (GLfloat) 1.00,
                                      (GLfloat) 0.85,  (GLfloat) 0.85,  (GLfloat) 0.18,  (GLfloat) 0.45,
                                      (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 0.60,  (GLfloat) 1.00,
                                      (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,  (GLfloat) 0.00,
                                     20.00 );


Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );
Attr.W      = Attr.H * MoreWidthRatio;
StandSize   = TSize ( InverseWindowSizeW, InverseWindowSizeH );


SetViewMenu ( new TMenuDescr ( IDM_INVERSE ) );

NumControlBarGadgets    = INVERSE_CBG_NUM;
ControlBarGadgets       = new TGadget * [INVERSE_CBG_NUM];

CreateBaseGadgets ();

ControlBarGadgets[ INVERSE_CBG_SEP0                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_SHOW2DIS             ]   = new TButtonGadget     ( IDB_SHOW2DIS,             IDB_SHOW2DIS,           TButtonGadget::NonExclusive, false, Show2DIs        ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ INVERSE_CBG_SHOW3DIS             ]   = new TButtonGadget     ( IDB_SHOW3DIS,             IDB_SHOW3DIS,           TButtonGadget::NonExclusive, false, Show3DIs        ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ INVERSE_CBG_SHOWVECTORSIS        ]   = new TButtonGadget     ( IDB_SHOWVECTORSIS,        IDB_SHOWVECTORSIS,      TButtonGadget::NonExclusive, false, ShowVectorsIs   ? TButtonGadget::Down : TButtonGadget::Up );

ControlBarGadgets[ INVERSE_CBG_SEP1                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_SHOW3DMRI            ]   = new TButtonGadget     ( IDB_SHOW3DMRI,            IDB_SHOW3DMRI,          TButtonGadget::NonExclusive, false, Show3DMri       ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ INVERSE_CBG_SHOWSP               ]   = new TButtonGadget     ( IDB_SHOWSP,               IDB_SHOWSP,             TButtonGadget::NonExclusive);

ControlBarGadgets[ INVERSE_CBG_SEP2                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_NEXTROI              ]   = new TButtonGadget     ( IDB_NEXTROI,              IDB_NEXTROI,            TButtonGadget::NonExclusive, true, IsRoiMode        ? TButtonGadget::Down : TButtonGadget::Up, false );
//ControlBarGadgets[ INVERSE_CBG_AVERAGEROIS        ]   = new TButtonGadget     ( IDB_AVERAGEROIS,          IDB_AVERAGEROIS,        TButtonGadget::NonExclusive, true, /*AverageRois*/false? TButtonGadget::Down : TButtonGadget::Up, false );

ControlBarGadgets[ INVERSE_CBG_SEP2A                ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_CUTPLANECORONAL      ]   = new TButtonGadget     ( IDB_CUTPLANECORONAL,      IDB_CUTPLANECORONAL,    TButtonGadget::NonExclusive, true, TButtonGadget::Down, false );
ControlBarGadgets[ INVERSE_CBG_CUTPLANETRANSVERSE   ]   = new TButtonGadget     ( IDB_CUTPLANETRANSVERSE,   IDB_CUTPLANETRANSVERSE, TButtonGadget::NonExclusive, true, TButtonGadget::Down, false );
ControlBarGadgets[ INVERSE_CBG_CUTPLANESAGITTAL     ]   = new TButtonGadget     ( IDB_CUTPLANESAGITTAL,     IDB_CUTPLANESAGITTAL,   TButtonGadget::NonExclusive, true, TButtonGadget::Down, false );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP3                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_PLANEMODE            ]   = new TButtonGadget     ( IDB_PLANEMODE,            IDB_PLANEMODE,          TButtonGadget::NonExclusive, false, TButtonGadget::Up );
ControlBarGadgets[ INVERSE_CBG_LESSSLICES           ]   = new TButtonGadget     ( IDB_LESSSLICES,           IDB_LESSSLICES,         TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_MORESLICES           ]   = new TButtonGadget     ( IDB_MORESLICES,           IDB_MORESLICES,         TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_FIRSTSLICEBWD        ]   = new TButtonGadget     ( IDB_FIRSTSLICEBWD,        IDB_FIRSTSLICEBWD,      TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_FIRSTSLICEFWD        ]   = new TButtonGadget     ( IDB_FIRSTSLICEFWD,        IDB_FIRSTSLICEFWD,      TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_LASTSLICEBWD         ]   = new TButtonGadget     ( IDB_LASTSLICEBWD,         IDB_LASTSLICEBWD,       TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_LASTSLICEFWD         ]   = new TButtonGadget     ( IDB_LASTSLICEFWD,         IDB_LASTSLICEFWD,       TButtonGadget::Command );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP4                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_SHMIN                ]   = new TButtonGadget     ( IDB_SHOWMIN,              IDB_SHOWMIN,            TButtonGadget::NonExclusive );
ControlBarGadgets[ INVERSE_CBG_SHMAX                ]   = new TButtonGadget     ( IDB_SHOWMAX,              IDB_SHOWMAX,            TButtonGadget::NonExclusive );
ControlBarGadgets[ INVERSE_CBG_FINDMINMAX           ]   = new TButtonGadget     ( IDB_FINDMINMAX,           IDB_FINDMINMAX,         TButtonGadget::NonExclusive );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP5                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_DECBR                ]   = new TButtonGadget     ( IDB_ISDECBRIGHT,          IDB_ISDECBRIGHT,        TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_INCBR                ]   = new TButtonGadget     ( IDB_ISINCBRIGHT,          IDB_ISINCBRIGHT,        TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_DECCR                ]   = new TButtonGadget     ( IDB_ISDECCONTRAST,        IDB_ISDECCONTRAST,      TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_INCCR                ]   = new TButtonGadget     ( IDB_ISINCCONTRAST,        IDB_ISINCCONTRAST,      TButtonGadget::Command );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP6                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_FXDSCL               ]   = new TButtonGadget     ( IDB_FIXEDSCALE,           IDB_FIXEDSCALE,         TButtonGadget::NonExclusive );
ControlBarGadgets[ INVERSE_CBG_COLOR                ]   = new TButtonGadget     ( IDB_SPCOLOR,              IDB_SPCOLOR,            TButtonGadget::Command );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP7                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_RESETREG             ]   = new TButtonGadget     ( IDB_RESETREG,             IDB_RESETREG,           TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_PREVREG              ]   = new TButtonGadget     ( IDB_PREVREG,              IDB_PREVREG,            TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_NEXTREG              ]   = new TButtonGadget     ( IDB_NEXTREG,              IDB_NEXTREG,            TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_SEARCHBESTREG        ]   = new TButtonGadget     ( IDB_SEARCHBESTREGGLOBAL,  IDB_SEARCHBESTREGGLOBAL,TButtonGadget::Command );
                                                                                  
ControlBarGadgets[ INVERSE_CBG_SEP8                 ]   = new TSeparatorGadget  ( ButtonSeparatorWidth );
ControlBarGadgets[ INVERSE_CBG_NEXTIS               ]   = new TButtonGadget     ( IDB_NEXTIS,               IDB_NEXTIS,             TButtonGadget::Command );
ControlBarGadgets[ INVERSE_CBG_NEXTMRI              ]   = new TButtonGadget     ( IDB_NEXTMRI,              IDB_NEXTMRI,            TButtonGadget::Command );
}


//----------------------------------------------------------------------------
                                        // Calling GetInvSol with all local variables
                                        // Also loading both the scalar and vectorial buffers if needed
void    TInverseView::GetInverse    (   int     tf1,    int     tf2     )
{

if ( CurrIs >= 0 ) {
                                        // EEG -> Matrix -> Inverse
    ISDoc->GetInvSol ( CurrReg, tf1, tf2, InvBuffS, GetEegView (), Rois );

    } // Matrix x EEG case

else {
                                        // Read from the RisDoc
    TRisDoc*            RisDoc          = GODoc->GetRisDoc ( CurrRis );

    RisDoc->GetTracks   (   tf1,        tf2, 
                            RisBuff,    0, 
                            AtomTypeUseCurrent, 
                            NoPseudoTracks, 
                            ReferenceUsingCurrent 
                        );

                                        // Read from Tracks' own buffer
                                        // !Doesn't work in case of mixing EEG+IS and RIS - Will be OK if/when we remove the feature to allow switching inverse within the same view!
//  GetEegView ()->GetTracks ( tf1, tf2, RisBuff );

                                        // copy to appropriate buffer
    InvBuffS.ResetMemory ();

    for ( int sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ )
    for ( int tfi0 = 0, tf = tf1; tf <= tf2; tfi0++, tf++ )

        InvBuffS[ sp ] +=  RisBuff ( sp , tfi0 );

    InvBuffS   /= tf2 - tf1 + 1;
    } // RIS case


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read the vectors WITHOUT filtering, for all cases
if ( ShowVectorsIs != InverseRenderingVectorsNone )
                                        // also reading a vectorial version
    ISDoc->GetInvSol ( CurrReg, tf1, tf2, InvBuffV, GetEegView (), Rois );

}


//----------------------------------------------------------------------------
void    TInverseView::InitMri ()
{
const TBoundingBox<double>*     b   = MRIDocBackg->GetBounding ();

ModelCenter = b->GetCenter ();
                                        // take the radius of the whole data box
ModelRadius = DepthPositionRatio * MRIDocBackg->GetSize ()->AbsRadius ();

MRIDocBackg->ToAbs ( ModelCenter );


SetSliceMatrices ( MRIDocBackg, ModelCenter, ModelRadius );


TGLClipPlane    savedclipplane0  = ClipPlane[ 0 ];
TGLClipPlane    savedclipplane1  = ClipPlane[ 1 ];
TGLClipPlane    savedclipplane2  = ClipPlane[ 2 ];

ClipPlane[ 0 ]  = TGLClipPlane ( GL_CLIP_PLANE0, -1,  0,  0, ModelCenter.X + 1e-3, b->XMin() - MRIDocBackg->GetOrigin ().X,
                                                                                   b->XMax() - MRIDocBackg->GetOrigin ().X  );
ClipPlane[ 1 ]  = TGLClipPlane ( GL_CLIP_PLANE1,  0, -1,  0, ModelCenter.Y + 1e-3, b->YMin() - MRIDocBackg->GetOrigin ().Y,
                                                                                   b->YMax() - MRIDocBackg->GetOrigin ().Y  );
ClipPlane[ 2 ]  = TGLClipPlane ( GL_CLIP_PLANE2,  0,  0, -1, ModelCenter.Z + 1e-3, b->ZMin() - MRIDocBackg->GetOrigin ().Z,
                                                                                   b->ZMax() - MRIDocBackg->GetOrigin ().Z  );

                                        // select the right planes according to the projection
InitClipPlanes ( ClipPlane );
                                        // restore actual clippings
ClipPlane[ 0 ].Set ( savedclipplane0 );
ClipPlane[ 1 ].Set ( savedclipplane1 );
ClipPlane[ 2 ].Set ( savedclipplane2 );



Fog         = TGLFog<GLfloat>   ( ModelRadius * FogDefaultNear,
                                  ModelRadius * FogDefaultFar,
                                  GLBASE_FOGCOLOR_NORMAL );


                                        // now, use biggest boundary
const TBoundingBox<int>*    bs  = MRIDocBackg->GetSize ();

Xaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    bs->XMax() - MRIDocBackg->GetOrigin ()[ 0 ] + bs->MaxSize () * 0.08, 0, 0,
                                    bs->MeanSize() * 0.06, bs->MeanSize() * 0.015,
                                    (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Yaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, bs->YMax() - MRIDocBackg->GetOrigin ()[ 1 ] + bs->MaxSize () * 0.08, 0,
                                    bs->MeanSize() * 0.06, bs->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 0.20,  (GLfloat) 1.00 );

Zaxis       = TGLArrow<GLfloat>   ( 0, 0, 0,
                                    0, 0, bs->ZMax() - MRIDocBackg->GetOrigin ()[ 2 ] + bs->MaxSize () * 0.08,
                                    bs->MeanSize() * 0.06, bs->MeanSize() * 0.015,
                                    (GLfloat) 0.20,  (GLfloat) 0.20,  (GLfloat) 1.00,  (GLfloat) 1.00 );

OriginRadius    = 0.20 * SPDoc->GetPointRadius ();
}


//----------------------------------------------------------------------------
bool    TInverseView::ValidView ()
{
return  GODoc
     && GODoc->GetNumSpDoc () && ( ( GODoc->GetNumEegDoc () && GODoc->GetNumIsDoc () ) || GODoc->GetNumRisDoc () )
     && GODoc->GetNumMriDoc ()
     && EEGDoc->GetNumTimeFrames ();
}


void    TInverseView::SetupWindow ()
{
TBaseView::SetupWindow ();

                                        // This needs an active window
VnReloadData ( EV_VN_RELOADDATA_DOCPOINTERS );  // update XYZDoc & Rois

CmNextRois ();

                                        // same as in TInverseMatrixDoc
AveragingPrecedence = AverageDefault;
                                        // for consistency between this view and the matrices, force all matrices precedence
                                        // will send a  vnReloadData / EV_VN_RELOADDATA_EEG  message
CmSetAveragingMode ( AveragingPrecedence == AverageBeforeInverse ? CM_INVERSEAVERAGEBEFORE : CM_INVERSEAVERAGEAFTER );


//if ( CartoolApplication->AnimateViews && GODoc->GetNumEegDoc () + GODoc->GetNumRisDoc() <= 4 ) {
//
//    Show2DIs     = InverseRendering2DNone;
//    AnimFx.Set ( TimerStartup, 10, 30 );
//    }
//
//else {
                                        // landing on the same transform as the animation
    for ( int i = 1; i < 30; i++ )
        ModelRotMatrix.RotateXYZ ( 33.0 / 29.0, -66.0 / 29.0, -33.0 / 29.0, MultiplyLeft );
}


//----------------------------------------------------------------------------
                                        // color tables vary according to IS type
void    TInverseView::SetColorTable ( AtomType datatype )
{
                                        // disable all tables
ColorTable.SetTableAllowed ( NoTables );

                                        // Positive and Vectorial data
if      ( IsAbsolute ( datatype ) ) {

    ColorTable.SetTableAllowed ( AbsColorTable_BlackYellowWhite );
    ColorTable.SetTableAllowed ( AbsColorTable_BlackRed );
    ColorTable.SetTableAllowed ( AbsColorTable_GrayGreenYellowRed );
    ColorTable.SetTableAllowed ( AbsColorTable_GrayYellow );
    ColorTable.SetTableAllowed ( AbsColorTable_MagentaBlueCyanGrayGreenYellowRed );
    ColorTable.SetTableAllowed ( AbsColorTable_BlackYellowWhiteMRIcro );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = AbsColorTable_GrayGreenYellowRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

else if ( IsAngular ( datatype ) )
                                        // not used for the moment
    TBaseView::SetColorTable ( datatype );

else { // if ( IsScalar ( datatype ) )      // all other cases

    ColorTable.SetTableAllowed ( SignedColorTable_BlueWhiteRed );
    ColorTable.SetTableAllowed ( SignedColorTable_CyanBlackYellow );
    ColorTable.SetTableAllowed ( SignedColorTable_MagentaBlueCyanGrayGreenYellowRed );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = SignedColorTable_MagentaBlueCyanGrayGreenYellowRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

}


//----------------------------------------------------------------------------
bool    TInverseView::ModifyPickingInfo ( TPointFloat& Picking, char *buff )
{
ClearString ( buff );

if ( /*SliceMode ||*/ 
    ! (    Show2DIs 
        || ShowVectorsIs 
        || ShowSp 
        || Show3DIs == InverseRendering3DSolPoints 
        || Show3DIs == InverseRendering3DIsoOnion  ) )

    return  false;


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
StringAppend    ( buff, NewLine );
                                    // could use the inverse name?
StringAppend    ( buff, "Value                       = " /*ISDoc->GetInverseTitle ()*/ );
FloatToString   ( StringEnd ( buff ), InvBuffS[ sp ] );


return true;
}


//----------------------------------------------------------------------------
void    TInverseView::SetScaling ( double scaling )
{
TDisplayScaling::SetScaling ( scaling );
                                        // has to redo the isosurface
RedoSurfaceIS       = true;
}


void    TInverseView::SetScaling ( double negv, double posv, bool forcesymetric )
{
TDisplayScaling::SetScaling ( negv, posv, forcesymetric );
                                        // has to redo the isosurface
//RedoSurfaceIS       = true;
}


void    TInverseView::UpdateScaling ()
{
TDisplayScaling::UpdateScaling ();
                                        // has to redo the isosurface
RedoSurfaceIS       = true;
}


bool    TInverseView::IsRenderingMode ( int renderingmode )
{
if      ( renderingmode == RenderingOpaque )

    return Show3DIs      != InverseRendering3DNone      ?   Show3DIs == InverseRendering3DSolPoints   || Show3DIs == InverseRendering3DIsosurface
         : Show2DIs      != InverseRendering2DNone      ?   Show2DIs == InverseRendering2DOpaque
         : ShowVectorsIs != InverseRenderingVectorsNone ?   true
         :                                                  false;

else if ( renderingmode == RenderingTransparent )

    return Show3DIs      != InverseRendering3DNone      ?   Show3DIs == InverseRendering3DIsoOnion
         : Show2DIs      != InverseRendering2DNone      ?   Show2DIs == InverseRendering2DTransparent || Show2DIs == InverseRendering2DOvercast
         :                                                  false;
else

    return                                                  false;
}

                                        // number of IS planes visibles
int     TInverseView::NumVisiblePlanes ()
{
if ( Show2DIs == InverseRendering2DNone )
    return  0;
else
    return  (bool) ClipPlane[ 0 ] + (bool) ClipPlane[ 1 ] + (bool) ClipPlane[ 2 ];
}


//----------------------------------------------------------------------------
void    TInverseView::DrawMinMax ( TPointFloat& pos, bool colormin, bool showminmaxcircle, double scale )
{
if ( showminmaxcircle ) {
    GLBlendOn       ();
    GLColoringOn    ();
    GLWriteDepthOff ();
    GLTestDepthOff  ();
    }


if ( showminmaxcircle ) {
    glPushMatrix ();
    glTranslated ( pos.X, pos.Y, pos.Z );

//  TGLMatrix           AntiModelRotMatrix ( ModelRotMatrix );  // now in the class
//  AntiModelRotMatrix.Invert ();       // revert current rotations, so we're back to XY so that DrawCircle is in the right orientation
    AntiModelRotMatrix.GLize ();

    if ( colormin )     glColor4f ( GLBASE_MINCOLOR, 0.5 );
    else                glColor4f ( GLBASE_MAXCOLOR, 0.5 );

    Prim.DrawCircle ( 0, 0, 0, scale * 0.05, scale * 0.065, 0, 360 );

    glPopMatrix ();
    }
else
    DrawCross ( pos, scale * 0.015, false );


if ( showminmaxcircle ) {
    GLBlendOff      ();
    GLColoringOff   ();
    GLWriteDepthOn  ();
    GLTestDepthOn   ();
    }
}


//----------------------------------------------------------------------------
void    TInverseView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode


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
TPointFloat         clip ( clipplane[ 0 ].GetAbsPosition (), clipplane[ 1 ].GetAbsPosition (), clipplane[ 2 ].GetAbsPosition () );
int                 ncutplanes      = (bool) clipplane[ 0 ] + (bool) clipplane[ 1 ] + (bool) clipplane[ 2 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TVolumeDoc*           MRIDoc          = MRIDocBackg;
                                        // catch the first view of each doc - should test for returned value 0
TVolumeView*                MRIView         = dynamic_cast<TVolumeView*        > ( MRIDocBackg->GetViewList () );
TSolutionPointsView*        SPView          = dynamic_cast<TSolutionPointsView*> ( SPDoc      ->GetViewList () );

TGLColorTable*              mricolortable   = MRIView ? MRIView->GetColorTable () : 0;

TGLVolume*                  volumemri       = MRIView ? &MRIView->GLVolume : 0;


const TPointFloat*          tosp;
const TPointFloat*          solpoint        = SPDoc->GetPoints ();
const TPointDouble&         spvoxsize       = SPDoc->GetVoxelSize ();
Volume&           			spvol           = SPVol;

//float                     ms              = MRIDoc->GetBounding()->BoxDiameter() * 0.75;
double                      msx,    msy;
GetSliceSize ( MRIDoc, msx, msy );
msx *= SliceMargin;
msy *= SliceMargin;

double                      worldsizex      = SlicesPerX * SeqPerX + (double) ( SeqPerX - 1 ) * InverseSpaceBetween;
double                      worldsizey      = SlicesPerY * SeqPerY + (double) ( SeqPerY - 1 ) * InverseSpaceBetween;

//double                      pr            = SPDoc->GetPointRadius ( CurrentDisplaySpace );
//double                      r             = pr * PointRadiusRatio[ ShowSp ];
float                       md              = SPDoc->GetMedianDistance () * SolutionPointsMinDistanceRatio;

int                         minIndex        = 0;
int                         maxIndex        = 0;
double                      minValue        = 0;
double                      maxValue        = 0;
double                      meanValue       = 0;
TPointFloat                 minPos;
TPointFloat                 maxPos;
bool                        showminmaxcircle= max ( ShowMin, ShowMax ) == InverseMinMaxCircle;   // a bit tricky: allows only for 1 type of Min/max display: circle or cross
double                      v;
int                         slicex;
int                         slicey;
int                         sliceseqx;
int                         sliceseqy;
double                      timepos;

int                         color0          = ColorTable.GetZeroIndex  ();
int                         colordelta      = ColorTable.GetDeltaIndex ();
int                         palindex;
double                      contrast;
TGLColor<GLfloat>           glcol;

int                         qualitymri;
int                         qualityis;

                                        // origin shift
TPointFloat                 shift ( -MRIDoc->GetOrigin ()[ 0 ], -MRIDoc->GetOrigin ()[ 1 ], -MRIDoc->GetOrigin ()[ 2 ] );
                                        // origin of interpolation
TPointFloat                 itpmin ( 0, 0, 0 );
TPointFloat                 itpmax ( spvol.GetDim1 () - 1, spvol.GetDim2 () - 1, spvol.GetDim3 () - 1 );
                                        // remove any optional margin from the isosurface
TPointFloat                 spmin  ( IsoParam.UseOriginalVolume ? 0.0 : TesselationVolumeMargin );


SPDoc->VolumeToAbsolute ( itpmin );
SPDoc->VolumeToAbsolute ( itpmax );
SPDoc->VolumeToAbsolute ( spmin  );

spmin      -= shift;


char                        buff[ 16 ];


const TBoundingBox<double>* b               = MRIDoc->GetBounding();
GLfloat                     mrimin[ 4 ];        // get MRI box
GLfloat                     mrimax[ 4 ];
double                      mrisize         = b->GetMeanExtent ();

b->GetMin ( mrimin );
b->GetMax ( mrimax );
mrimin[ 3 ]         = mrimax[ 3 ]   = 1;

MRIDoc->ToAbs ( mrimin );
MRIDoc->ToAbs ( mrimax );


const Volume&               mribackgdata    = *MRIDoc->GetData ();


#if defined(DisplayInverseInterpolation4NN)
const TArray3<TWeightedPoints4>*    toip4nn     =  SPDoc->GetInterpol4NN ();
double                      scalingpmax;
double                      scalingnmax;
#else
const TArray3<ushort>&      toip1nn         = *SPDoc->GetInterpol1NN ();
double                      scalingpmax;
double                      scalingnmax;
#endif
TSliceSelection            *slch            = SliceMode ? &Slices[ SliceMode - 1 ] : 0;
int                         slicei;

TGLCoordinates<GLfloat>     p;


glFrontFace ( GL_CCW );                 // specifies the right orientation

                                        // save current rotation & translation (from here or outside)
MatModelView.CopyModelView ();


timepos     = GetCurrentWindowSize ( how & GLPaintOwner )           // global window size
            * GetCurrentZoomFactor ( how & GLPaintOwner ) * 0.45    // zoom factor
            * 0.5;                                                  // object size factor


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GalMaxValue = Lowest  ( GalMaxValue );
GalMinValue = Highest ( GalMaxValue );


if ( Outputing () ) {
    qualitymri  = SliceMode ? QuadFastestQuality
                            : QuadRegularQuality;
#if defined(DisplayInverseInterpolation4NN)
    qualityis   = SliceMode ? QuadRegularQuality
                            : QuadBestQuality;
#else
    qualityis   = QuadFastestQuality;
#endif
    }
else {
    qualitymri  = QuadFastestQuality;
    qualityis   = QuadFastestQuality;
    }

                                        // override for mask, for all displays
if ( MRIDoc->IsMask () )
    qualitymri = QuadFastestQuality;

                                        // init for drawing slices
                                        // borrow the color table from the volume display
//mricolortable->Alpha = AlphaValueLinear;// !as Alpha is set each time by MRIDoc, we have to be a bit pushy here!

if ( SliceMode && volumemri )
    volumemri->GLize ( &mribackgdata, mricolortable, ! MRIDoc->IsMask (), MRIDoc->GetOrigin () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 tf              = ( ManageRangeCursor == MRCAnimation ) && ! (bool) AnimTF ? MRCNumTF - 1 : 0;


for ( int itf = 0; tf < MRCNumTF; tf += MRCStepTF, itf++ ) {

    sliceseqx   = itf % SeqPerX;
    sliceseqy   = ( SeqPerY - 1 ) - itf / SeqPerX;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sequence/animation case: ask for the inverse at each time (buffer size pb)
    if ( ManageRangeCursor != MRCAverage )
        GetInverse ( TFCursor.GetPosMin() + tf, TFCursor.GetPosMin() + tf );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find min / max values & positions
    maxValue    = Lowest  ( maxValue );
    minValue    = Highest ( minValue );
    meanValue   =  0;


    for ( TIteratorSelectedForward spi ( SelSP ); (bool) spi; ++spi ) {

        v           = InvBuffS[ spi() ];

        meanValue  += v;

        if ( v > maxValue ) {
            maxIndex    = spi();
            maxValue    = v;
            maxPos      = solpoint[ spi() ];
            }

        if ( v < minValue ) {
            minIndex    = spi();
            minValue    = v;
            minPos      = solpoint[ spi() ];
            }
        }

    meanValue  /= NonNull ( SelSP.NumSet () );


    if ( Rois ) {                       // get a mean position from the right Rois

        minPos.Reset ();
        maxPos.Reset ();

        const TRoi*         roi         = Rois->IndexToRoi ( minIndex );

        for ( TIteratorSelectedForward spi ( roi->Selection ); (bool) spi; ++spi )
            minPos += solpoint[ spi() ];

        minPos /= roi->Selection ? (int) roi->Selection : 1;


        roi = Rois->IndexToRoi ( maxIndex );

        for ( TIteratorSelectedForward spi ( roi->Selection ); (bool) spi; ++spi )
            maxPos += solpoint[ spi() ];

        maxPos /= roi->Selection ? (int) roi->Selection : 1;
        }

                                    // max sould be positive, min negative
//  if ( ISDoc->IsScalar ( AtomTypeUseCurrent ) ) {
//      if ( minValue > 0 )   { minValue = 0; minPos.Reset (); }
//      if ( maxValue < 0 )   { maxValue = 0; maxPos.Reset (); }
//      }

                                    // to avoid some errors, just in case
    if ( maxValue == minValue ) {
        maxValue   += DoubleFloatEpsilon;
        minValue   -= DoubleFloatEpsilon;
        }

//  if ( minValue == 0 )    minValue = -1e-12;
//  if ( maxValue == 0 )    maxValue =  1e-12;

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
                                        // re-position clip planes on min or max
    if ( FindMinMax && ( how & GLPaintOwner ) && ! ( RButtonDown && ControlKey ) ) {

        if ( IsSignedData ) {

            if      ( ShowMax && ShowMin || ( ! ShowMax && ! ShowMin ) ) // choose the best one

                if ( fabs ( maxValue ) > fabs ( minValue ) )
                    clip = maxPos;
                else
                    clip = minPos;

            else if ( ShowMin )

                clip = minPos;

            else // otherwise, always maxPos
                clip = maxPos;

            } // IsSignedData

        else // positive data
            clip = maxPos;

//      clip.X = (int) ( clip.X + 0.5 );
//      clip.Y = (int) ( clip.Y + 0.5 );
//      clip.Z = (int) ( clip.Z + 0.5 );

        if ( ! SliceMode ) {
            if ( (bool) clipplane[ 0 ] )    clipplane[ 0 ].SetAbsPosition ( clip.X );
            if ( (bool) clipplane[ 1 ] )    clipplane[ 1 ].SetAbsPosition ( clip.Y );
            if ( (bool) clipplane[ 2 ] )    clipplane[ 2 ].SetAbsPosition ( clip.Z );
            }
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // looks like ranking?
    bool        istemplate  = RelativeDifference ( maxValue, 1 ) < 1e-3 /* && RelativeDifference ( 1 / minValue, SPDoc->GetNumSolPoints () ) < 0.01 */
                           || ( CurrRis >= 0 && GODoc->GetRisDoc ( CurrRis )->IsTemplates () );

                                        // compute scaling factors
    if ( ScalingAuto != ScalingAutoOff
      && ! ( RButtonDown && ! ControlKey && MouseAxis == MouseAxisHorizontal ) ) {  // not in force manual bypass

        if ( minValue == 0 )    minValue    = -1e-12;
        if ( maxValue == 0 )    maxValue    =  1e-12;


        if ( istemplate )

            SetScaling ( minValue - ( minValue - meanValue ) * 0.005, 
                         maxValue - ( maxValue - meanValue ) * 0.005,
                         ScalingAuto == ScalingAutoSymmetric );

        else

            SetScaling ( minValue - ( minValue - meanValue ) * 0.10, 
                         maxValue - ( maxValue - meanValue ) * 0.10,
                         ScalingAuto == ScalingAutoSymmetric );
        }

                                        // contrast factor for the color computation
                                        // give a little more contrast for ranked data, for easier comparison to regular inverse - this is a trade-off, though
    contrast    = 0.1 + 10000 * Cube ( istemplate ? 2 * ScalingContrast : ScalingContrast );

                                        // new contrast formula with color table with cut-off slope
//    contrast    = ScalingContrast;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the whole IS volume

    spvol.ResetMemory ();


#if defined(DisplayInverseInterpolation4NN)
                                        // 4NN display
    const TWeightedPoints4* toi4        = toip4nn->GetArray ();

    scalingpmax     = ScalingPMax;
    scalingnmax     = ScalingNMax;

                                        // low spreading (Epifocus) should be given very precise contrast
//  scalingpmax    *= IsSpreading < .33 ? 0.999 : 0.90 + ScalingContrast * 0.08;
//  scalingnmax    *= IsSpreading < .33 ? 0.999 : 0.90 + ScalingContrast * 0.08;

                                        // kind of working: contrast is modulated by the CDF, changing the "speed" of cut
//  double                  eq          = 1 - ValuesEqualize[ ScalingContrast * 100 ] / 100;
//  scalingpmax    *= 0.90 + eq * 0.10;
//  scalingnmax    *= 0.90 + eq * 0.10;

                                        // interpolation and result volumes have same dimensions
    for ( int i = 0; i < (int) spvol; i++, toi4++ )

        if ( toi4->IsAllocated () )
                                        // real value
            spvol ( i ) = (   toi4->w1 * InvBuffS[ toi4->i1 ]
                            + toi4->w2 * InvBuffS[ toi4->i2 ]
                            + toi4->w3 * InvBuffS[ toi4->i3 ]
                            + toi4->w4 * InvBuffS[ toi4->i4 ] ) / TWeightedPoints4SumWeights;

#else
                                        // 1NN display

    scalingpmax     = ScalingPMax * ( /*filteris ? 1 - sqrt ( ScalingContrast ) * 0.08 :*/ 1 );
    scalingnmax     = ScalingNMax * ( /*filteris ? 1 - sqrt ( ScalingContrast ) * 0.08 :*/ 1 );

                                        // low spreading (Epifocus) should be given very precise contrast
//  scalingpmax    *= IsSpreading < .33 ? 0.999 : 0.90 + ScalingContrast * 0.08;
//  scalingnmax    *= IsSpreading < .33 ? 0.999 : 0.90 + ScalingContrast * 0.08;

                                        // interpolation and result volumes have same dimensions
    for ( int i = 0; i < (int) spvol; i++ )
                                        // special index to indicate no value
        if ( toip1nn ( i ) != UndefinedInterpolation1NN )
                                        // real value
            spvol ( i ) = InvBuffS[ toip1nn ( i ) ];

#endif

                                        // ?Note: there is a slight glitch if data is signed, in opaque 2D slices, then the real values at 0 will be clipped out. It shows as small dots near 0?
    if      ( Show2DIs == InverseRendering2DOvercast )

        ColorTable.SetParameters ( scalingpmax, scalingnmax, contrast, AlphaTable /*AlphaLinear*/ /*AlphaLinearSaturated*/ );

    else if ( Show2DIs == InverseRendering2DTransparent )

        ColorTable.SetParameters ( scalingpmax, scalingnmax, contrast, AlphaBool, 0.75 );

    else

        ColorTable.SetParameters ( scalingpmax, scalingnmax, contrast, AlphaBool );


#if defined(DisplayInverseInterpolation4NN)
    VolumeIs.GLize ( &spvol, &ColorTable, true  );  // interpolation with contextual quality
#else
    VolumeIs.GLize ( &spvol, &ColorTable, false );  // NN with texture quality
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop for any pair views

    int                 nviews          = SliceMode ? slch->GetTotalSlices () : 1;

  for ( int nv = 0; nv < nviews; nv++ ) {

    slicex  = sliceseqx * SlicesPerX                      + nv % SlicesPerX;
    slicey  = sliceseqy * SlicesPerY + ( SlicesPerY - 1 ) - nv / SlicesPerX;

    glLoadIdentity ();

    glTranslatef ( ( - worldsizex / 2 + 0.5 + slicex + (double) sliceseqx * InverseSpaceBetween ) * msx,
                   ( - worldsizey / 2 + 0.5 + slicey + (double) sliceseqy * InverseSpaceBetween ) * msy, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // print the label
    if ( ShowSequenceLabels && ( ManageRangeCursor == MRCSequence || ManageRangeCursor == MRCAnimation )
      && NotSmallWindow () && timepos * SlicesPerY * msy > 35
      && nv % SlicesPerX == 0 && nv / SlicesPerX == SlicesPerY - 1  // left bottom corner of slices
      && ( how & GLPaintOwner  ) 
      && ( how & GLPaintOpaque ) ) {

        GLBlendOn       ();

        TextColor.GLize ( 2 );
        SFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

        StringCopy ( buff, EEGDoc->IsTemplates () ? "Template" : "TF", " ", IntegerToString ( TFCursor.GetPosMin() + tf + EEGDoc->GetStartingTimeFrame () ) );

        SFont->Print ( 0, 0, 0, buff, TA_LEFT | TA_BOTTOM | TA_BOX, -timepos * msx, -timepos * msy, 0 );

        GLBlendOff      ();
        } // print time


                                        // summary slice? -> fixed view
    if ( SliceMode && nv == nviews - 1 )
        SlicesSummaryMatrix[ SliceMode - 1 ].GLize ();
    else                                // restore & apply original transform
        MatModelView.GLize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing IS slices
    auto    DrawPlaneIsX    = [ & ] ( float cx ) {

        if ( cx >= itpmin.X && cx <= itpmax.X ) {

            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

            GLBlendOn       ();
            GLWriteDepthOff ();

            glTranslatef    ( itpmin.X, itpmin.Y, itpmin.Z );
            glScaled        ( spvoxsize.X, spvoxsize.Y, spvoxsize.Z );

            VolumeIs.DrawPlaneX ( SPDoc->AbsoluteToVolumeX ( cx ), qualityis );

            glScaled        ( 1 / spvoxsize.X, 1 / spvoxsize.Y, 1 / spvoxsize.Z );
            glTranslatef    ( - itpmin.X, - itpmin.Y, - itpmin.Z );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();

            GLBlendOff      ();
            GLWriteDepthOn  ();
            }
        };


    auto    DrawPlaneIsY    = [ & ] ( float cy ) {

        if ( cy >= itpmin.Y && cy <= itpmax.Y ) {

            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

            GLBlendOn       ();
            GLWriteDepthOff ();

            glTranslatef    ( itpmin.X, itpmin.Y, itpmin.Z );
            glScaled        ( spvoxsize.X, spvoxsize.Y, spvoxsize.Z );

            VolumeIs.DrawPlaneY ( SPDoc->AbsoluteToVolumeY ( cy ), qualityis );

            glScaled        ( 1 / spvoxsize.X, 1 / spvoxsize.Y, 1 / spvoxsize.Z );
            glTranslatef    ( - itpmin.X, - itpmin.Y, - itpmin.Z );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();

            GLBlendOff      ();
            GLWriteDepthOn  ();
            }
        };


    auto    DrawPlaneIsZ    = [ & ] ( float cz ) {

        if ( cz >= itpmin.Z && cz <= itpmax.Z ) {

            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

            GLBlendOn       ();
            GLWriteDepthOff ();

            glTranslatef    ( itpmin.X, itpmin.Y, itpmin.Z );
            glScaled        ( spvoxsize.X, spvoxsize.Y, spvoxsize.Z );

            VolumeIs.DrawPlaneZ ( SPDoc->AbsoluteToVolumeZ ( cz ), qualityis );

            glScaled        ( 1 / spvoxsize.X, 1 / spvoxsize.Y, 1 / spvoxsize.Z );
            glTranslatef    ( - itpmin.X, - itpmin.Y, - itpmin.Z );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();

            GLBlendOff      ();
            GLWriteDepthOn  ();
            }
        };


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw a series of "2D" slices

    if ( SliceMode && ( how & GLPaintOpaque ) ) {

        GLColoringOn        ();
        GLFogOff            ();
        GLAlphaAboveOn      ( MRIDoc->GetBackthresholdAlpha () );

                                        // to override SPDoc clipplanes
        if ( ShowSp != PointsShowNone ) {
            clipplane[ 0 ].SetNone ();
            clipplane[ 1 ].SetNone ();
            clipplane[ 2 ].SetNone ();

            clipplane[ SliceModeToXYZ[ SliceMode ] ].SetBackward ();
            }

                                        // select slice
        slicei = slch->GetSelection ()->GetValue ( nv );


        if ( slch->IsSummarySlice ( slicei ) ) {

                                        // draw only a series of frames on top of the summary slice, which is drawn outside this loop, as a regular display
            if ( IsSliceModeX () )

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( minPos.X,   minPos.X,   mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( maxPos.X,   maxPos.X,   mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( si() + shift.X, si() + shift.X, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 1 ); }
                    }
            else if ( IsSliceModeY () )

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, minPos.Y,   minPos.Y,   mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, maxPos.Y,   maxPos.Y,   mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], si() + shift.Y, si() + shift.Y, mrimin[ 2 ], mrimax[ 2 ], 1 ); }
                    }
            else

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, minPos.Z,   minPos.Z,   3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, maxPos.Z,   maxPos.Z,   3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], si() + shift.Z, si() + shift.Z, 1 ); }
                    }

            } // summary slice

        else {                          // regular slices
            int                 s;
                                        // draw the slices according to parameters, plus a frame on Min / Max slices
            if ( IsSliceModeX () ) {

                if ( slch->IsMinSlice ( slicei ) ) {
                    s = minPos.X;

                    glColor4f ( 0.00, 0.00, 1.00, 1.00 );

                    Prim.Draw3DWireFrame ( s, s, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 2 );
                    }
                else if ( slch->IsMaxSlice ( slicei ) ) {
                    s = maxPos.X;

                    glColor4f ( 1.00, 0.00, 0.00, 1.00 );

                    Prim.Draw3DWireFrame ( s, s, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 2 );
                    }
                else
                    s = slicei + shift.X;


//              if ( Show3DMri )                VolumeMri.DrawPlaneX ( s, qualitymri );
                if ( Show3DMri && volumemri )   volumemri->DrawPlaneX ( s, qualitymri );

                if ( Show2DIs  )                DrawPlaneIsX ( s );

                if ( ShowSp != PointsShowNone ) {

                    clipplane[ 0 ].SetAbsPosition ( s );
                    SPView->GLPaint ( GLPaintLinked | ( AnimFx || AnimTF ? GLPaintFast : 0 ) | GLPaintOpaqueOrTransparent /*GLPaintOpaque*/ | GLPaintForceClipping, ShowSp, clipplane );

                    GLColoringOn        ();
                    }

                if ( ShowMin && (int) minPos.X == s )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
                if ( ShowMax && (int) maxPos.X == s )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );
                }

            else if ( IsSliceModeY () ) {

                if ( slch->IsMinSlice ( slicei ) ) {
                    s = minPos.Y;

                    glColor4f ( 0.00, 0.00, 1.00, 1.00 );

                    Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], s, s, mrimin[ 2 ], mrimax[ 2 ], 3 );
                    }
                else if ( slch->IsMaxSlice ( slicei ) ) {
                    s = maxPos.Y;

                    glColor4f ( 1.00, 0.00, 0.00, 1.00 );

                    Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], s, s, mrimin[ 2 ], mrimax[ 2 ], 3 );
                    }
                else
                    s = slicei + shift.Y;


//              if ( Show3DMri )                VolumeMri.DrawPlaneY ( s, qualitymri );
                if ( Show3DMri && volumemri )   volumemri->DrawPlaneY ( s, qualitymri );

                if ( Show2DIs  )                DrawPlaneIsY ( s );

                if ( ShowSp != PointsShowNone ) {

                    clipplane[ 1 ].SetAbsPosition ( s );
                    SPView->GLPaint ( GLPaintLinked | ( AnimFx || AnimTF ? GLPaintFast : 0 ) | GLPaintOpaqueOrTransparent /*GLPaintOpaque*/ | GLPaintForceClipping, ShowSp, clipplane );

                    GLColoringOn        ();
                    }

                if ( ShowMin && (int) minPos.Y == s )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
                if ( ShowMax && (int) maxPos.Y == s )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );
                }

            else { // if ( IsSliceModeZ () )

                if ( slch->IsMinSlice ( slicei ) ) {
                    s = minPos.Z;

                    glColor4f ( 0.00, 0.00, 1.00, 1.00 );

                    Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], s, s, 3 );
                    }
                else if ( slch->IsMaxSlice ( slicei ) ) {
                    s = maxPos.Z;

                    glColor4f ( 1.00, 0.00, 0.00, 1.00 );

                    Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], s, s, 3 );
                    }
                else
                    s = slicei + shift.Z;


//              if ( Show3DMri )                VolumeMri.DrawPlaneZ ( s, qualitymri );
                if ( Show3DMri && volumemri )   volumemri->DrawPlaneZ ( s, qualitymri );

                if ( Show2DIs  )                DrawPlaneIsZ ( s );

                if ( ShowSp != PointsShowNone ) {

                    clipplane[ 2 ].SetAbsPosition ( s );
                    SPView->GLPaint ( GLPaintLinked | ( AnimFx || AnimTF ? GLPaintFast : 0 ) | GLPaintOpaqueOrTransparent /*GLPaintOpaque*/ | GLPaintForceClipping, ShowSp, clipplane );

                    GLColoringOn        ();
                    }

                if ( ShowMin && (int) minPos.Z == s )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
                if ( ShowMax && (int) maxPos.Z == s )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );
                }

            continue;                   // nviews
            } // regular slices

                                        // restore clipplanes
        if ( ShowSp != PointsShowNone )
            InitClipPlanes ( clipplane );


        GLColoringOff       ();
//      GLFogOn             ();
        GLAlphaAboveOff     ();
        } // SliceMode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( how & GLPaintOwner )
        if ( ManageRangeCursor == MRCSequence || SliceMode )
            GLFogOff            ();
        else
            GLFogOn             ();


    if ( SliceMode )                    // the scaling in the summary matrix also affects the normals
        GLNormalizeNormalOn ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the solution points colored by the potential
    if ( Show3DIs == InverseRendering3DSolPoints && ( how & GLPaintOpaque ) ) {

        GLMaterialOn        ();

        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

        TGLBillboardSphere *tobbsphere = Outputing() /*|| VkKey ( 'A' )*/ ? &BbHighSphere : &BbLowSphere;

        tobbsphere->GLize ();           // will do all the job

        MaterialIs.GLize ();

        glcol.Alpha = 1.0;
        double      s;

                                        // draw here
        for ( TIteratorSelectedForward spi ( SelSP ); (bool) spi; ++spi ) {

            tosp    = solpoint + spi();


            palindex = ColorTable.GetColorIndex ( InvBuffS[ spi() ], glcol );


            if ( palindex == color0 )
                continue;

                                        // lower ratio makes more SP / wider potatoes
            #define     IsRatio3DBerry      0.18
            #define     IsRatio3DVector     0.20
            #define     IsRatio3DPotato     0.22


            s   = fabs ( (double) ( palindex - color0 ) / colordelta );

            if ( s < IsRatio3DBerry )
                continue;

            s   = pow ( s, 0.75 );      // make small berries a little bigger


            glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, glcol );

//          tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, md * ( s - IsRatio3DBerry ) / ( 1 - IsRatio3DBerry ) );
                                        // make the berries bigger and appear to pop
            tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, md * s );

            } // for spi


        tobbsphere->unGLize ();

        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }
        } // InverseRendering3DSolPoints


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw inverse as small vectors
                                        // Test is done on the scalar values, which can be filtered and/or signed data, so "negative" vectors can be shown
                                        // Vectors themselves are the original ones, unfiltered
    if ( ShowVectorsIs && ( how & GLPaintOpaque ) ) {

        double  ws  = GetCurrentWindowSize ( how & GLPaintOwner );
        double  zf  = GetCurrentZoomFactor ( how & GLPaintOwner );


        GLSmoothEdgesOn     ();


        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

        TGLBillboardSphere *tobbsphere = Outputing() ? &BbHighSphere : &BbLowSphere;

        tobbsphere->GLize ();           // will do all the job

        MaterialIs.GLize ();

        double      s;


        for ( TIteratorSelectedForward spi ( SelSP ); (bool) spi; ++spi ) {

            tosp    = solpoint + spi();

                                        // keep only vectors from any clipping plane, similarly to Solution Points
            if ( ShowVectorsIs == InverseRenderingVectors2D
                 && ! (    (bool) clipplane[0] && fabs ( tosp->X - clip.X ) <= md 
                        || (bool) clipplane[1] && fabs ( tosp->Y - clip.Y ) <= md 
                        || (bool) clipplane[2] && fabs ( tosp->Z - clip.Z ) <= md ) )

                  continue;


            palindex = ColorTable.GetColorIndex ( InvBuffS[ spi() ], glcol );

                                        // force opaque
            glcol.Alpha = 1.0;

            if ( palindex == color0 )
                continue;


            s   = fabs ( (double) ( palindex - color0 ) / colordelta );

            if ( s < IsRatio3DVector )
                continue;


                                        // sphere at origin
            s  *= md;

            if ( Outputing () ) {
                GLMaterialOn        (); // better, but a bit slower
                glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, glcol );
                }
            else {
                GLColoringOn        (); // faster, seen from afar it looks the same
                glcol.GLize ();
                }

            tobbsphere->GLize ( tosp->X, tosp->Y, tosp->Z, s * 0.25 );


                                        // line
            if ( Outputing () ) {
                GLColoringOn        ();
                glcol.GLize ();
                }
                                        // line width
//            glLineWidth ( s * 2.0 + 0.5 );
            glLineWidth ( s * 0.4 * sqrt ( zf * ws ) + 0.5 );
//            glLineWidth ( s * 2.0 * zf * ws * 0.05 + 0.5 );

                                        // line length
            s  *= 1.5 / ScalingPMax;

            glBegin ( GL_LINES );
            glVertex3f  ( tosp->X, tosp->Y, tosp->Z );
            glVertex3f  ( tosp->X + InvBuffV[ spi() ].X * s, tosp->Y + InvBuffV[ spi() ].Y * s, tosp->Z + InvBuffV[ spi() ].Z * s );
            glEnd   ();


/*                                      // a triangle always facing the screen - pb in case of facing toward screen -> see nothing!
            GLColoringOn        ();
            glcol.GLize ();

            s  *= md;
            double  s2 = s / 5;

            s  *= 1.25 / ScalingPMax;

            double  pr  = tobbsphere->Right.X * InvBuffV[ spi() ].X + tobbsphere->Right.Y * InvBuffV[ spi() ].Y + tobbsphere->Right.Z * InvBuffV[ spi() ].Z;
            double  pu  = tobbsphere->Up   .X * InvBuffV[ spi() ].X + tobbsphere->Up   .Y * InvBuffV[ spi() ].Y + tobbsphere->Up   .Z * InvBuffV[ spi() ].Z;

            double  norm    = InvBuffV[ spi() ].Norm ();

            pr /= norm;
            pu /= norm;

            glTranslatef ( tosp->x, tosp->y, tosp->z );

            glBegin ( GL_TRIANGLES );
            glVertex3f  ( s2 * ( pr * tobbsphere->Up.X - pu * tobbsphere->Right.X ),
                          s2 * ( pr * tobbsphere->Up.Y - pu * tobbsphere->Right.Y ),
                          s2 * ( pr * tobbsphere->Up.Z - pu * tobbsphere->Right.Z )  );

            glVertex3f  ( - s2 * ( pr * tobbsphere->Up.X - pu * tobbsphere->Right.X ),
                          - s2 * ( pr * tobbsphere->Up.Y - pu * tobbsphere->Right.Y ),
                          - s2 * ( pr * tobbsphere->Up.Z - pu * tobbsphere->Right.Z )  );


            glVertex3f  ( s * InvBuffV[ spi() ].X, s * InvBuffV[ spi() ].Y, s * InvBuffV[ spi() ].Z );

            glEnd   ();

            glTranslatef ( - tosp->x, - tosp->y, - tosp->z );

            GLColoringOff       ();*/
            } // for spi


        GLSmoothEdgesOff    ();
        glLineWidth ( 1 );

        tobbsphere->unGLize ();

        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }
        } // ShowVectorsIs


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the 3D isosurface of IS
    if ( Show3DIs == InverseRendering3DIsosurface  && ( how & GLPaintOpaque ) ) {

        double          isocutpos       = scalingpmax * ( 1 - powl ( ( 1.0 - IsRatio3DPotato ), ColorTable.GetContrast () ) ); // same contrast formula from ColorTable
        double          isocutneg       = scalingnmax * ( 1 - powl ( ( 1.0 - IsRatio3DPotato ), ColorTable.GetContrast () ) ); // same contrast formula from ColorTable

                                        // need to build the surface?
        if ( RedoSurfaceIS || ManageRangeCursor == MRCSequence ) { // || true ) { //|| (bool) AnimTF ) {

            SurfaceISPos[ 0 ].Reset ();
            SurfaceISNeg[ 0 ].Reset ();


            if ( maxValue >  DoubleFloatEpsilon ) {

                IsoParam.IsoValue   = isocutpos;

                SurfaceISPos[ 0 ].IsosurfaceFromVolume ( spvol, IsoParam );
                } // positive isosurface

                                        // do a negative part if needed
            if ( minValue < -DoubleFloatEpsilon ) {

                IsoParam.IsoValue   = isocutneg;

                SurfaceISNeg[ 0 ].IsosurfaceFromVolume ( spvol, IsoParam );
                } // negative isosurface


            RedoSurfaceIS   = false;
            } // if RedoSurfaceIS

                                        // any shift to give the illusion it is "on top"
        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

//      GLSmoothEdgesOn ();
        GLMaterialOn    ();

//      glEnable ( GL_CULL_FACE );      // off due to negative isosurface which is reverted
        glFrontFace ( GL_CW );

        glEnableClientState ( GL_VERTEX_ARRAY );
        glEnableClientState ( GL_NORMAL_ARRAY );


        glTranslatef ( spmin.X, spmin.Y, spmin.Z );

                                        // do first part
        if ( SurfaceISPos[ 0 ].IsNotEmpty () ) {
                                        // get the outer color
            ColorTable.GetColorIndex ( isocutpos, glcol );

            MaterialIs.SetDiffuse ( glcol.Red, glcol.Green, glcol.Blue, 1 );
            MaterialIs.GLize ();

            glVertexPointer ( 3, GL_FLOAT, 0, SurfaceISPos[ 0 ].GetListTriangles     () );
            glNormalPointer (    GL_FLOAT, 0, SurfaceISPos[ 0 ].GetListPointsNormals () );

            glDrawArrays    ( GL_TRIANGLES, 0, SurfaceISPos[ 0 ].GetNumPoints () );
            }

                                        // do the second part?
        if ( SurfaceISNeg[ 0 ].IsNotEmpty () ) {

//          glFrontFace ( GL_CCW );     // at the moment, the isosurface returns a "reverted" orientation - also reverse the normals!

                                        // get the outer color
            ColorTable.GetColorIndex ( isocutneg, glcol );

            MaterialIs.SetDiffuse ( glcol.Red, glcol.Green, glcol.Blue, 1 );
            MaterialIs.GLize ();

            glVertexPointer ( 3, GL_FLOAT, 0, SurfaceISNeg[ 0 ].GetListTriangles     () );
            glNormalPointer (    GL_FLOAT, 0, SurfaceISNeg[ 0 ].GetListPointsNormals () );

            glDrawArrays    ( GL_TRIANGLES, 0, SurfaceISNeg[ 0 ].GetNumPoints () );
            }


        glTranslatef ( -spmin.X, -spmin.Y, -spmin.Z );


        glVertexPointer         ( 3, GL_FLOAT, 0, 0 );
        glNormalPointer         (    GL_FLOAT, 0, 0 );

        glDisableClientState    ( GL_VERTEX_ARRAY );
        glDisableClientState    ( GL_NORMAL_ARRAY );

        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }


//      GLSmoothEdgesOff();
//      GLMaterialOff   ();
//      glDisable ( GL_CULL_FACE );
        } // InverseRendering3DIsosurface


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//    Fog.unGLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the MRI, either opaque or transparent
                                        // Trick: call the MRI view, which will do it the best way
    // put a link to this view to prevent closing?
    if ( Show3DMri && MRIView ) {

        MRIView->GLPaint ( GLPaintLinked
                         | ( qualitymri != QuadBestQuality || LeftKey || RightKey ? GLPaintFast : 0 )  // not used anymore for volumes, textures are doing the job
                         | GLPaintOpaqueOrTransparent
                         | GLPaintForceClipping,
                           Show3DMri, clipplane );
                                        // it seems we don't need the local VolumeMri anymore, as the volume display correctly manages textures in different contexts now..
//                         Show3DMri, clipplane,
//                         &VolumeMri );    // transmit the local variable, which is linked texture-wised to this window, as the Volume object from volume display does not belong to here
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque or transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw MRI & IS planes
                                        // the IS is drawn in transparency, but always on top of an opaque background
//    if ( ncutplanes && ( Show2DIs ) && ( how & GLPaintOpaque ) ) {
    if ( ! SliceMode && ncutplanes
       && (    ( Show2DIs == InverseRendering2DOpaque      && ( how & GLPaintOpaque      ) )
            || ( Show2DIs == InverseRendering2DOvercast    && ( how & GLPaintTransparent ) )
            || ( Show2DIs == InverseRendering2DTransparent && ( how & GLPaintTransparent ) ) ) ) {


        GLColoringOn        ();
        GLAlphaAboveOn      ( MRIDoc->GetBackthresholdAlpha () );

        if ( ManageRangeCursor == MRCSequence || SliceMode )
            GLFogOff        ();


        bool    needclip    = NumVisiblePlanes () >= 2 && Show3DMri && ! ( Show3DMri == MriSliceOpaque || Show3DMri == MriSliceOvercast );
        bool    cl0         = needclip && (bool) clipplane[0];
        bool    cl1         = needclip && (bool) clipplane[1];
        bool    cl2         = needclip && (bool) clipplane[2];


        if ( (bool) clipplane[0] ) {
            if ( cl1 )  clipplane[ 1 ].GLize ( InvertedClipPlane );
            if ( cl2 )  clipplane[ 2 ].GLize ( InvertedClipPlane );
                                        // draw the IS on top
            if ( Show2DIs )
                DrawPlaneIsX ( clip.X );

            if ( cl1 )  clipplane[ 1 ].unGLize();
            if ( cl2 )  clipplane[ 2 ].unGLize();
            } // clip0


        if ( (bool) clipplane[1] ) {
            if ( cl0 )  clipplane[ 0 ].GLize ( InvertedClipPlane );
            if ( cl2 )  clipplane[ 2 ].GLize ( InvertedClipPlane );
                                        // draw the IS on top
            if ( Show2DIs )
                DrawPlaneIsY ( clip.Y );

            if ( cl0 )  clipplane[ 0 ].unGLize();
            if ( cl2 )  clipplane[ 2 ].unGLize();
            } // clip1


        if ( (bool) clipplane[2] ) {
            if ( cl0 )  clipplane[ 0 ].GLize ( InvertedClipPlane );
            if ( cl1 )  clipplane[ 1 ].GLize ( InvertedClipPlane );
                                        // draw the IS on top
            if ( Show2DIs )
                DrawPlaneIsZ ( clip.Z );

            if ( cl0 )  clipplane[ 0 ].unGLize();
            if ( cl1 )  clipplane[ 1 ].unGLize();
            } // clip2


        GLColoringOff       ();
        GLAlphaAboveOff     ();

//      if ( how & GLPaintLinked )
//          GLFogOn         ();
        } // MRI & IS planes


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // outline clipping planes
    if ( ShiftAxis != ShiftNone && ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

        GLLinesModeOn       ();


        if ( ShiftAxis == ShiftX ) {
            glColor4f ( 1.00, 0.00, 0.00, 1.00 );

            sprintf ( buff, "X = %g", clip.X );
            SFont->Print ( clip.X, mrimax[ 1 ], mrimax[ 2 ], buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

            Prim.Draw3DWireFrame ( clip.X, clip.X, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 3 );
            }

        if ( ShiftAxis == ShiftY ) {
            glColor4f ( 0.00, 1.00, 0.00, 1.00 );

            sprintf ( buff, "Y = %g", clip.Y );
            SFont->Print ( mrimax[ 0 ], clip.Y, mrimax[ 2 ], buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

            Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], clip.Y, clip.Y, mrimin[ 2 ], mrimax[ 2 ], 3 );
            }

        if ( ShiftAxis == ShiftZ ) {
            glColor4f ( 0.00, 0.00, 1.00, 1.00 );

            sprintf ( buff, "Z = %g", clip.Z );
            SFont->Print ( mrimax[ 0 ], mrimax[ 1 ], clip.Z, buff, TA_CENTER | TA_TOP | TA_BOX, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.025 );

            Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], clip.Z, clip.Z, 3 );
            }

                           // conditionally restoring fog
        GLLinesModeOff      ( ! ( ManageRangeCursor == MRCSequence || SliceMode ) );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // draw the solution points
    if ( ShowSp != PointsShowNone && SPView && ( how & GLPaintOpaque )
      && SPView->IsRenderingMode ( RenderingOpaque ) ) {

        SPView->GLPaint ( GLPaintLinked
                        | ( AnimFx || AnimTF ? GLPaintFast : 0 )
                        | GLPaintOpaqueOrTransparent /*GLPaintOpaque*/
                        | GLPaintForceClipping,
                          ShowSp, clipplane );

//      GLColoringOn        ();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Show Min/Max in 3D
                                // not needed for the summary slice, in SliceMode?
    if ( ( ShowMin || ShowMax ) && ! SliceMode && ( how & GLPaintOpaque ) ) {

        if ( ShowMin )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
        if ( ShowMax )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );
        } // ShowMin || ShowMax


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the 3D isosurface of IS
    if ( Show3DIs == InverseRendering3DIsoOnion && ( how & GLPaintTransparent ) ) {

        GLTransparencyOn    ();
        GLColoringOn        ();


        double          ocut;
//      int             numonion    = Truncate ( sqrt ( 1 - ScalingContrast ) * ( InverseNumOnionLayers - 2 ) ) + 2;
        int             numonion    = Truncate ( ( 1 - ScalingContrast ) * ( InverseNumOnionLayers - 2 ) ) + 2;

        double          isocutpos[ InverseNumOnionLayers ];
        double          isocutneg[ InverseNumOnionLayers ];

                                        // ?store isocutpos isocutneg into global variable?
        for ( int onion = 0; onion < numonion; onion++ ) {

            ocut      = (double) onion / ( numonion - 1 );

            isocutpos[ onion ]  = scalingpmax * ( 1 - sqrt ( ocut ) * powl ( ( 1.0 - IsRatio3DPotato ), ColorTable.GetContrast () ) );
            isocutneg[ onion ]  = scalingnmax * ( 1 - sqrt ( ocut ) * powl ( ( 1.0 - IsRatio3DPotato ), ColorTable.GetContrast () ) );
            } // for onion

                                        // need to build the surface?
        if ( RedoSurfaceIS || ManageRangeCursor == MRCSequence ) { // || true ) { //|| (bool) AnimTF ) {

            for ( int onion = 0; onion < InverseNumOnionLayers; onion++ ) {
                SurfaceISPos[ onion ].Reset ();
                SurfaceISNeg[ onion ].Reset ();
                } // for onion


            if ( maxValue >  DoubleFloatEpsilon ) {

                for ( int onion = 0; onion < numonion; onion++ ) {

                    IsoParam.IsoValue   = isocutpos[ onion ];

                    SurfaceISPos[ onion ].IsosurfaceFromVolume ( spvol, IsoParam );
                    }
                } // positive isosurface

                                        // do a negative part if needed
            if ( minValue < -DoubleFloatEpsilon ) {

                for ( int onion = 0; onion < numonion; onion++ ) {

                    IsoParam.IsoValue   = isocutneg[ onion ];

                    SurfaceISNeg[ onion ].IsosurfaceFromVolume ( spvol, IsoParam );
                    }
                } // negative isosurface


            RedoSurfaceIS   = false;
            }

                                        // any shift to give the illusion it is "on top"
        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

        glFrontFace ( GL_CW );

        glEnableClientState ( GL_VERTEX_ARRAY );


        glTranslatef ( spmin.X, spmin.Y, spmin.Z );

                                        // draw the isosurfaces
        for ( int onion = 0; onion < numonion; onion++ ) {
                                        // do first part
            if ( SurfaceISPos[ onion ].IsNotEmpty () ) {

                ColorTable.GetColorIndex ( isocutpos[ onion ], glcol );

                glcol.Alpha = Square ( isocutpos[ onion ] / scalingpmax ) * 0.10 + 0.01;

                glcol.GLize ();

                glVertexPointer ( 3, GL_FLOAT,  0, SurfaceISPos[ onion ].GetListTriangles () );

                glDrawArrays    ( GL_TRIANGLES, 0, SurfaceISPos[ onion ].GetNumPoints () );
                }

                                        // do the second part?
            if ( SurfaceISNeg[ onion ].IsNotEmpty () ) {

                ColorTable.GetColorIndex ( isocutneg[ onion ], glcol );

                glcol.Alpha = Square ( isocutneg[ onion ] / scalingnmax ) * 0.10 + 0.01;

                glcol.GLize ();

                glVertexPointer ( 3, GL_FLOAT,  0, SurfaceISNeg[ onion ].GetListTriangles () );

                glDrawArrays    ( GL_TRIANGLES, 0, SurfaceISNeg[ onion ].GetNumPoints () );
                }
            }


        glTranslatef ( -spmin.X, -spmin.Y, -spmin.Z );

        glVertexPointer         ( 3, GL_FLOAT, 0, 0 );

        glDisableClientState    ( GL_VERTEX_ARRAY );

        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }


        GLTransparencyOff   ();
        GLColoringOff       ();
        } // InverseRendering3DIsoOnion


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( SliceMode )
        GLNormalizeNormalOff();

  } // for nv


    VolumeIs.unGLize ();                // unload texture each time
                                        // otherwise, the texture could store current TF, and calling GLize with optional parameter TF -> != TF -> reload

    } // for tf

}


//----------------------------------------------------------------------------
void    TInverseView::Paint ( TDC &dc, bool /*erase*/ , TRect &rect )
{

if ( ManageRangeCursor == MRCSequence || SliceMode ) {
    double              msx,    msy;
    GetSliceSize ( MRIDocBackg, msx, msy );

    msx *= SliceMargin;
    msy *= SliceMargin;

    PrePaint ( dc, rect, ( SlicesPerX * SeqPerX + (double) ( SeqPerX - 1 ) * InverseSpaceBetween ) * msx / 2 * ExtraSizeMRISliceMode,
                         ( SlicesPerY * SeqPerY + (double) ( SeqPerY - 1 ) * InverseSpaceBetween ) * msy / 2 * ExtraSizeMRISliceMode,
                         ModelRadius );
    }
else {
    double              radius  = MRIDocBackg->GetBounding ()->MaxSize () / 2 * ExtraSize3D;

    PrePaint ( dc, rect, radius, radius, ModelRadius );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this matrix for the min/max display
if ( max ( ShowMin, ShowMax ) == InverseMinMaxCircle ) {
    AntiModelRotMatrix  = ModelRotMatrix;
    AntiModelRotMatrix.Invert ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

Light0.GLize ();
Light1.GLize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for a correct Fog, push everything below 0
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
if ( /*! Outputing() &&*/ ShowAxis && ManageRangeCursor != MRCSequence && ! SliceMode )
    DrawAxis ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.unGLize();

                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the scaling of the colors & min/max
                                        // draw current IS file
    if ( ( Show3DIs || Show2DIs || ShowVectorsIs ) && ShowColorScale )

        ColorTable.Draw     (   PaintRect,
                                LineColor[ Outputing() ? 1 : 0 ],   TextColor[ Outputing() ? 1 : 0 ],
                                ColorMapWidth,                      ColorMapHeight,
                                GalMinValue,                        GalMaxValue,
                                true,                               ScalingAuto == ScalingAutoOff,      3,      /*"[µA/mm³]"*/ 0,   // !not always, could be Z values, or rank!
                                SFont
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw orientation
    if ( ShowOrientation )
        DrawOrientation ( MRIDocBackg->GetBoxSides() );


//    if ( VkQuery () ) {
//        SFont->Print ( PrintLeftPos, PrintTopPos( PaintRect.Height () ) + 1 * PrintVerticalStep ( SFont ), PrintDepthPosBack, VolumeIs .Texture.Loaded  ? "IS  Texture 3D Loaded  : yes"  : "IS  Texture 3D Loaded  : no",  TA_LEFT | TA_BOTTOM );
//        SFont->Print ( PrintLeftPos, PrintTopPos( PaintRect.Height () ) + 0 * PrintVerticalStep ( SFont ), PrintDepthPosBack, VolumeMri.Texture.Loaded  ? "MRI Texture 3D Loaded  : yes"  : "MRI Texture 3D Loaded  : no",  TA_LEFT | TA_BOTTOM );
//        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // show informations
    if ( ShowInfos /*&& ! Outputing ()*/ ) {
        TextColor.GLize ( Outputing() ? 1 : 0 );

        double              xpos            = PrintLeftPos;
        double              ypos            = PrintTopPos( PaintRect.Height () );
        double              zpos            = PrintDepthPosBack;
        double              ydelta          = PrintVerticalStep ( SFont );


        SFont->Print ( xpos, ypos, zpos, ISDoc->GetInverseTitle (), TA_LEFT | TA_TOP );

        if ( ISDoc->GetNumRegularizations () > 0 ) {
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, (*ISDoc->GetRegularizationsNames ())[ CurrReg ], TA_LEFT | TA_TOP );
            }

        if ( Rois ) {
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, (char *) Rois->GetName (), TA_LEFT | TA_TOP );
            }


        char                buff [ 256 ];
//      TPointDouble       &spvoxsize       = SPDoc->GetVoxelSize ();

        if ( ! ( ManageRangeCursor & MRCAnySequence ) && ( FindMinMax || ShowMax ) ) {

            glColor4f ( 1.00, 0.00, 0.00, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Max:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Solution Point  %s = %0.3g", SPDoc->GetSPName ( GalMaxIndex ), GalMaxValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMaxPos.X, GalMaxPos.Y, GalMaxPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


                                        // 3.2) If no depth-shift trick -> transform & print
            if ( GetGeometryTransform () && (bool) SPDoc->GetDisplaySpaces () ) {

                TPointFloat             p ( GalMaxPos );

                                        // forward the bounding, to help guess any downsampling
                GetGeometryTransform ()->ConvertTo ( p, & SPDoc->GetDisplaySpaces ()[ 0 ].Bounding );


                if ( p.IsNotNull () ) { // not foolproof, but...

                    sprintf ( buff, "%4d      , %4d      , %4d       [%s]", Truncate ( p.X ), Truncate ( p.Y ), Truncate ( p.Z ), GetGeometryTransform ()->Name );
                    ypos   -= ydelta;
                    SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

                                        // special case for Talairach: add the labels
                    if ( StringIs ( GetGeometryTransform ()->Name, "Talairach" ) ) {
                        Taloracle.PositionToString ( p, buff, true );
                        ypos   -= ydelta;
                        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );
                        ypos   -= 2 * ydelta;
                        }
                    }
                }
            } // ShowMax


        if ( ! ( ManageRangeCursor & MRCAnySequence ) && ( FindMinMax && IsSignedData || ShowMin ) ) {

            glColor4f ( 0.00, 0.00, 1.00, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Min:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Solution Point  %s = %0.3g", SPDoc->GetSPName ( GalMinIndex ), GalMinValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMinPos.X, GalMinPos.Y, GalMinPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


                                        // 3.2) If no depth-shift trick -> transform & print
            if ( GetGeometryTransform () && (bool) SPDoc->GetDisplaySpaces () ) {

                TPointFloat             p ( GalMinPos );

                                        // forward the bounding, to help guess any downsampling
                GetGeometryTransform ()->ConvertTo ( p, & SPDoc->GetDisplaySpaces ()[ 0 ].Bounding );


                if ( p.IsNotNull () ) { // not foolproof, but...

                    sprintf ( buff, "%4d      , %4d      , %4d       [%s]", Truncate ( p.X ), Truncate ( p.Y ), Truncate ( p.Z ), GetGeometryTransform ()->Name );
                    ypos   -= ydelta;
                    SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

                                        // special case for Talairach: add the labels
                    if ( StringIs ( GetGeometryTransform ()->Name, "Talairach" ) ) {
                        Taloracle.PositionToString ( p, buff, true );
                        ypos   -= ydelta;
                        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );
//                      ypos   -= 2 * ydelta;
                        }
                    }
                }
            } // ShowMin

        } // ShowInfos


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ResetWindowCoordinates ();
    } // window mode

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TInverseView::HintsPaint ()
{
                                        // draw additional user interaction hints
if ( LButtonDown || MButtonDown || RButtonDown ) {

                                        // 2D slices give an erroneous Talairach
    if ( MButtonDown && SliceMode ) {

        if ( Picking.IsNotNull () ) {

            SetWindowCoordinates ();
            GLBlendOn       ();

            glColor4f ( GLBASE_CURSORHINTCOLOR );
            BFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

            glTranslated ( MousePos.X(), PaintRect.Height() - MousePos.Y(), 0 );
            glScaled ( CursorHintSize, CursorHintSize, 1 );

            BFont->Print ( 0, -1, 1, "No coordinates available in slice mode.", TA_CENTER | TA_TOP | TA_BOX );

            GLBlendOff      ();
            ResetWindowCoordinates ();          // this will also restore a clean ModelView
            }
        }

    else if ( CurrentDisplaySpace != DisplaySpaceNone ) {
        TBaseView::HintsPaint ();
        return;
        } // 3D stuff
    }
}


//----------------------------------------------------------------------------
void    TInverseView::EvKeyDown ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {
    case 'G':
        CmMagnifier ();
        break;

    case 'L':
        Cm2Object ();
        break;

    case 'O':
        CmOrient ();
        break;

    case 'C':
        CmNextColorTable ();
        break;

    case 'D':
        CmSetScalingAdapt ();
        break;

    case 'F':
        CmSetFindMinMax ();
        break;

    case 'U':
        if ( ControlKey && ShiftKey )
            CmSearchRegularization ( /*IDB_SEARCHBESTREGCURSOR*/ IDB_SEARCHBESTREGGLOBAL );
        else
            CmReg ( ControlKey ? IDB_RESETREG : ShiftKey ? IDB_PREVREG : IDB_NEXTREG );
        break;

    case 'I':
        CmNextIs ();
        break;

    case 'M':
        if ( ShiftKey && IsSignedData )
            CmShowMinMax ( IDB_SHOWMIN );
        else
            CmShowMinMax ( IDB_SHOWMAX );
        break;

    case 'N':
        CmNextRois ();
        break;

    case 'P':
        if ( ControlKey )
            CmSetShowSP ();
        break;

    case 'R':
        CmNextMri ();
        break;

    case 'T':
        CmSetShowVectorsIs ();
        break;

    case 'V':
        ButtonGadgetSetState   ( IDB_RANGEAVE, true );
        CmSetManageRangeCursor ( IDB_RANGEAVE );
        break;

    case 'W':
        CmSetShow2DIs ( IDB_SHOW2DIS );
        break;

    case 'X':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANECORONAL );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANECORONAL, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateY ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            //Invalidate ( false );
            ShowNow ();
            }
        break;

    case 'Y':
//      CmNextXyz ();
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANETRANSVERSE );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANETRANSVERSE, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateX ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            //Invalidate ( false );
            ShowNow ();
            }
        break;

    case 'Z':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANESAGITTAL );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANESAGITTAL, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateZ ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            //Invalidate ( false );
            ShowNow ();
            }
        break;

    case '2':
        CmSetSliceMode ();
        break;

    case '3':
        if ( ControlKey )
            CmSetShow3DMri ();
        else
            CmSetShow3DIs ();
        break;

    case VK_ADD:
        if ( ControlKey )
            CmSetStepTF ( IDB_RGESTEPDEC );
        else
            CmSetNumSlices ( IDB_MORESLICES );
        break;

    case VK_SUBTRACT:
        if ( ControlKey )
            CmSetStepTF ( IDB_RGESTEPINC );
        else
            CmSetNumSlices ( IDB_LESSSLICES );
        break;

    case VK_UP:
        if ( ControlKey )
            CmSetBrightness ( IDB_ISINCBRIGHT );
        else if ( ShiftKey )
            CmSetContrast ( IDB_ISINCCONTRAST );
        break;

    case VK_DOWN:
        if ( ControlKey )
            CmSetBrightness ( IDB_ISDECBRIGHT );
        else if ( ShiftKey )
            CmSetContrast ( IDB_ISDECCONTRAST );
         break;

    case VK_PRIOR:
        if ( ControlKey )
            CmMoveSlice ( IDB_LASTSLICEBWD );
        else
            CmMoveSlice ( IDB_FIRSTSLICEBWD );
        break;

    case VK_NEXT:
        if ( ControlKey )
            CmMoveSlice ( IDB_LASTSLICEFWD );
        else
            CmMoveSlice ( IDB_FIRSTSLICEFWD );
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


void    TInverseView::EvKeyUp ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyUp ( key, repeatCount, flags );


switch (key) {

    case VK_RIGHT:
    case VK_LEFT:
    case 'X':
    case 'Y':
    case 'Z':

        Invalidate ( false );
    }


if ( LinkedViewId )
    GetEegView ()->EvKeyUp ( key, repeatCount, flags );
}


void    TInverseView::EvSize ( uint sizeType, const TSize &size )
{
SetItemsInWindow ();

TBaseView::EvSize ( sizeType, size );
}


void    TInverseView::EvLButtonDblClk  ( uint, const TPoint & )
{
TBaseView*      v  = CartoolDocManager->GetView ( LinkedViewId );

if ( v != 0 )
    v->SetFocusBack ( GetViewId () );   // come back to me !
}


//----------------------------------------------------------------------------
void    TInverseView::EvLButtonDown ( uint i, const TPoint &p )
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

else if ( CaptureMode == CaptureNone ) {

    LButtonDown = true;

    CaptureMode = CaptureLeftButton;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }

else
    TSecondaryView::EvLButtonDown ( i, p );
}


void    TInverseView::EvLButtonUp ( uint i, const TPoint &p )
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
void    TInverseView::EvMButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos    = p;

MButtonDown = true;

CaptureMode = CaptureGLSelect;

CaptureMouse ( Capture );

EvMouseMove ( i, p );

                                        // resetting everything can only be done at clicking time
if ( ShowSp != PointsShowNone && Picking.IsNull () ) {

    Highlighted.Reset ();

    SPDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

    ShowNow ();
    }
}


void    TInverseView::EvMButtonUp ( uint, const TPoint & )
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
void    TInverseView::EvRButtonDown ( uint, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );

MousePos   = p;
}


void    TInverseView::EvRButtonUp ( uint, const TPoint & )
{
int                 oma             = MouseAxis;

MouseAxis   = MouseAxisNone;


if ( CaptureMode == CaptureRightButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


RButtonDown = false;
ShiftAxis   = ShiftNone;

CaptureMouse ( Release );
                                        // for clipping planes refresh
RedoSurfaceIS       = ! ControlKey && oma == MouseAxisHorizontal;    // and isosurface

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TInverseView::EvMouseMove ( uint i, const TPoint &p )
{
/*
if ( RButtonDown && VkKey ( 'J' ) ) {
    double  dy  = - ( p.Y() - MousePos.Y() );

    DepthRange.UserShift += dy / 2000;

    if ( DepthRange.UserShift < MaxShiftDepthRange ) DepthRange.UserShift = MaxShiftDepthRange;
    if ( DepthRange.UserShift > 0 ) DepthRange.UserShift = 0;

    Invalidate ( false );
    return;
    }
*/

int                 dx              = p.X () - MousePos.X ();
int                 dy              = p.Y () - MousePos.Y ();
int                 adx             = abs ( dx );
int                 ady             = abs ( dy );


if ( RButtonDown ) {

    if ( ControlKey ) {
                                        // move planes
        if ( ! ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) || SliceMode )
            return;

        double  k       = (double) MRIDocBackg->GetSize()->MaxSize() / GetClientRect().Height() * 1.7;

        if ( fabs ( (double) dx ) + fabs ( (double) dy ) < 10 && ShiftAxis == ShiftNone )
            return;

                                        // compute the 3 ratios, for each clipping plane
        double              rat[ 3 ];

        rat[ 0 ]  = ClipPlane[0] ? ClipPlane[0].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;
        rat[ 1 ]  = ClipPlane[1] ? ClipPlane[1].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;
        rat[ 2 ]  = ClipPlane[2] ? ClipPlane[2].DirectionRatio ( -dx, dy, (GLdouble *) ModelRotMatrix, true ) : 0;

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
            rat[ ShiftAxis ] = ShiftAxis == ShiftZ ? -dy : dy;  // sign change to be more intuitive as moving up in the brain ~= moving up the mouse

                                        // shift it
        ClipPlane[ ShiftAxis ].Shift ( rat[ ShiftAxis ] * k );
        }
    else {                              // change color scaling

        if ( adx >= MinMouseMove || ady >= MinMouseMove ) {

            if      ( adx > ady * ( MouseAxis == MouseAxisVertical   ? 3 : 1 ) )
                MouseAxis   = MouseAxisHorizontal;

            else if ( ady > adx * ( MouseAxis == MouseAxisHorizontal ? 3 : 1 ) )
                MouseAxis   = MouseAxisVertical;
            }

        if ( MouseAxis == MouseAxisNone )
            return;


        double              osl             = ScalingLevel;
        double              osc             = ScalingContrast;

        if ( MouseAxis == MouseAxisHorizontal ) {

            if ( dx > 0 )   ScalingLevel /= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.3 );
            else            ScalingLevel *= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.3 );

            SetScaling ( ScalingLevel );
            }
        else if ( MouseAxis == MouseAxisVertical ) {

            ScalingContrast -= (double) dy / 1000;

            SetScalingContrast ( ScalingContrast );
            }

		LastMouseMove       = TPoint ( dx, dy );
		MousePos            = p;

        if ( osl == ScalingLevel && osc == ScalingContrast )
            return;
        }

	LastMouseMove       = TPoint ( dx, dy );
	MousePos            = p;

    //Invalidate ( false );
    ShowNow ();
    }
//else if ( LButtonDown )
else {
    GLMouseMove ( i, p );
    }


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

        if ( ShowSp != PointsShowNone ) {

            Highlighted.Invert ( hit );

            SPDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );
            }

        ShowNow ();
        }


    lasthit = hit;
    }
}


//----------------------------------------------------------------------------
bool    TInverseView::VnViewUpdated ( TBaseView *view )
{
if ( !view )
    return true;

//Invalidate ( false );
ShowNow ();

return true;
}


//----------------------------------------------------------------------------
void    TInverseView::CmMriEnable ( TCommandEnabler &tce )
{
tce.Enable ( Show3DMri || ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) ); //|| Show2DIs );
}


void    TInverseView::CmNextMriEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && GODoc->GetNumMriDoc() > 1 );
}


void    TInverseView::CmIsEnable ( TCommandEnabler &tce )
{
tce.Enable ( Show3DIs || ShowVectorsIs || ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) && Show2DIs );
}


void    TInverseView::CmNextRegEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && GODoc->GetNumIsDoc() != 0
          && ISDoc->GetNumRegularizations () > 1 );
}


void    TInverseView::CmNextIsEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && ( GODoc->GetNumIsDoc() + GODoc->GetNumRisDoc() ) > 1
          && ( Show3DIs || ShowVectorsIs || ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) && Show2DIs ) );
}


void    TInverseView::CmIsMriEnable ( TCommandEnabler &tce )
{
tce.Enable ( Show2DIs || ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) );
}


void    TInverseView::CmSlicesEnable ( TCommandEnabler &tce )
{
tce.Enable ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] || SliceMode );
}


void    TInverseView::CmIsScalarEnable ( TCommandEnabler &tce )
{
tce.Enable ( true && IsSignedData ); // doing an expression solves a bug of Borland
}


void    TInverseView::CmSliceModeEnable ( TCommandEnabler &tce )
{
tce.Enable ( SliceMode );
}


void    TInverseView::CmNotSliceModeEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! SliceMode );
}


void    TInverseView::CmSetCutPlaneEnable ( TCommandEnabler &tce )
{
tce.Enable ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] );
}


void    TInverseView::CmSetIntensityLevelEnable ( TCommandEnabler &tce )
{
tce.Enable ( ( Show3DIs || ShowVectorsIs || ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) && Show2DIs )
             && ScalingAuto == ScalingAutoOff );
}


void    TInverseView::CmNextRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && GODoc->GetNumRoiDoc () );
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetShow2DIs ( owlwparam wparam )
{
if ( wparam == IDB_SHOW2DIS ) {

    Show2DIs    = NextState ( Show2DIs, NumInverseRendering2D, ShiftKey );

    ButtonGadgetSetState ( IDB_SHOW2DIS, Show2DIs );
    }

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetShow3DIs ()
{
int     os3dis = Show3DIs;

Show3DIs    = NextState ( Show3DIs, NumInverseRendering3D, ShiftKey );


if ( Show3DIs )
    RedoSurfaceIS = true;


                                        // 3D IS is more accurate than 2D IS
/*
if ( Show3DIs && ! os3dis && ! SliceMode ) { // so if not in slice mode remove 2D IS
    Show2DIs    = InverseRendering2DNone;
    ButtonGadgetSetState ( IDB_SHOW2DIS, Show2DIs );
    }
*/
                                        // leaving 3D display -> restore 2D IS
if ( ! Show3DIs && os3dis )

    if ( ! Show2DIs && ! ShowVectorsIs ) {

        Show2DIs    = InverseRendering2DDefault;

        ButtonGadgetSetState ( IDB_SHOW2DIS, Show2DIs );
        }


ButtonGadgetSetState ( IDB_SHOW3DIS, Show3DIs );

SetItemsInWindow();

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetShowVectorsIs ()
{
ShowVectorsIs   = NextState ( ShowVectorsIs, NumInverseRenderingVectors, ShiftKey );

ButtonGadgetSetState ( IDB_SHOWVECTORSIS, ShowVectorsIs );


if ( ShowVectorsIs && ManageRangeCursor == MRCAverage )

    GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // set a default display?
if ( ! Show3DIs && ! Show2DIs && ! ShowVectorsIs ) {

    Show2DIs    = InverseRendering2DDefault;

    ButtonGadgetSetState ( IDB_SHOW2DIS, Show2DIs );
    }


//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetShow3DMri ()
{
Show3DMri   = NextState ( Show3DMri, MriNumRendering, ShiftKey );

ButtonGadgetSetState ( IDB_SHOW3DMRI, Show3DMri );

SetItemsInWindow();

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
double  TInverseView::GetRegularizationRatio ( int reg1, int reg2 )
{
int                 sp;
double              maxreg1         = 0;
double              maxreg2         = 0;

                                        // get data with reg1
ISDoc->GetInvSol ( reg1, TFCursor.GetPosMin(), TFCursor.GetPosMax(), InvBuffS, GetEegView (), Rois );

                                        // and save the max
for ( sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ )
    Maxed ( maxreg1, fabs ( (double) InvBuffS[ sp ] ) );

                                        // get data with reg2
ISDoc->GetInvSol ( reg2, TFCursor.GetPosMin(), TFCursor.GetPosMax(), InvBuffS, GetEegView (), Rois );

                                        // and save the max
for ( sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ )
    Maxed ( maxreg2, fabs ( (double) InvBuffS[ sp ] ) );


return  maxreg1 && maxreg2 ? maxreg1 / maxreg2 : 1;
}


void    TInverseView::CmReg ( owlwparam w )
{
int                 oldreg          = CurrReg;

if      ( w == IDB_PREVREG )    CurrReg = max ( 0, CurrReg - 1 ); //  ( CurrReg + ISDoc->GetMaxRegularization () - 1 ) % ISDoc->GetMaxRegularization ();
else if ( w == IDB_NEXTREG )    CurrReg = min ( ISDoc->GetMaxRegularization () - 1, CurrReg + 1 ); // ( CurrReg + 1 ) % ISDoc->GetMaxRegularization ();
else if ( w == IDB_RESETREG )   CurrReg = 0;
else return;


if ( oldreg == CurrReg )    return;

                                        // adapt spreading?
//SearchAndSetIntensity ( false );


if ( ScalingAuto == ScalingAutoOff ) {
                                        // adjust the current scaling to keep the same visual aspect
    ScalingLevel   *= GetRegularizationRatio ( CurrReg, oldreg );

    SetScaling ( ScalingLevel );
    }


if ( ManageRangeCursor == MRCAverage )  // else it is updated during the paint
    GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // has to redo the isosurface
RedoSurfaceIS       = true;

ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmNextIs ()
{
int                 oldcis          = CurrIs;
int                 oldcris         = CurrRis;

                                        // saved current state + shift from is to ris
if ( CurrIs < 0 ) { // currently in ris?

    SavedIsState[ InverseNumSavedStates - CurrRis - 1 ].DisplayScaling  = *dynamic_cast<TDisplayScaling *> ( this );


    CurrRis     = NextState ( CurrRis, GODoc->GetNumRisDoc (), ShiftKey );


    if ( ! CurrRis && GODoc->GetNumIsDoc () && ! OnlyRis ) {    // switch to is?
        CurrRis = -1;
        CurrIs  = 0;
        ISDoc   = GODoc->GetIsDoc  ( CurrIs );
        }
    else {
        ISDoc   = GODoc->GetRisDoc ( CurrRis );
        }
    }

else { // currently in is

    SavedIsState[ CurrIs ].DisplayScaling   = *dynamic_cast<TDisplayScaling *> ( this );
    SavedIsState[ CurrIs ].CurrReg          = CurrReg;
    SavedIsState[ CurrIs ].IsSpreading      = IsSpreading;


    CurrIs      = NextState ( CurrIs, GODoc->GetNumIsDoc (), ShiftKey );


    if ( ! CurrIs && GODoc->GetNumRisDoc () ) {     // switch to ris?
        CurrRis = 0;
        CurrIs  = -1;
        ISDoc   = GODoc->GetRisDoc ( CurrRis );
        }
    else {
        ISDoc   = GODoc->GetIsDoc  ( CurrIs );
        }
    }


if ( oldcis == CurrIs && oldcris == CurrRis )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore saved state
if ( CurrIs >= 0 ) {

    if ( SavedIsState[ CurrIs ].IsAllocated () ) {
        *dynamic_cast<TDisplayScaling *> ( this )   = SavedIsState[ CurrIs ].DisplayScaling;
        CurrReg                                     = SavedIsState[ CurrIs ].CurrReg;
        IsSpreading                                 = SavedIsState[ CurrIs ].IsSpreading;
        }
    else { // first call, inherit from current display

        if ( CurrReg == 0 )
            CurrReg     = ISDoc->GetRegularizationIndex ( DefaultRegularization );

        CurrReg     = min ( CurrReg, ISDoc->GetMaxRegularization () - 1 );

        SearchAndSetIntensity ();

//      SetScalingContrast ( ScalingContrastInit ); // don't - we prefer to clone the current contrast
        }
    }

else if ( CurrRis >= 0 ) {

    if ( SavedIsState[ InverseNumSavedStates - CurrRis - 1 ].IsAllocated () ) {
        *dynamic_cast<TDisplayScaling *> ( this )   = SavedIsState[ InverseNumSavedStates - CurrRis - 1 ].DisplayScaling;
        }
    else { // first call, inherit from current display

        SearchAndSetIntensity ();

//      SetScalingContrast ( ScalingContrastInit ); // don't - we prefer to clone the current contrast
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update color tables?
bool                oldIsSignedData     = IsSignedData;


IsSignedData        = ! ISDoc ->IsAbsolute ( AtomTypeUseCurrent );

                                        // don't reset current color table if not needed
if ( IsSignedData != oldIsSignedData )

    SetColorTable ( ISDoc->GetAtomType ( AtomTypeUseCurrent ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );

UpdateScaling ();


if ( ManageRangeCursor == MRCAverage )  // else it is updated during the paint
    GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmNextMri ()
{
int     oldcmri = CurrMri;

CurrMri     = NextState ( CurrMri, GODoc->GetNumMriDoc (), ShiftKey );


if ( oldcmri == CurrMri )   return;

MRIDocBackg = GODoc->GetMriDoc ( CurrMri );

InitMri ();

SetItemsInWindow();

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmNextRois ()
{
int     nextrois    = TBaseView::NextRois ( IsRoiMode ? CurrRois : -1, SPDoc->GetNumSolPoints () );


if ( nextrois == -1 ) {

    IsRoiMode   = false;

    CurrRois    = 0;
    if ( Rois ) delete Rois, Rois = 0;
    }
else {
    IsRoiMode   = true;

    CurrRois    = nextrois;
    if ( Rois ) delete Rois, Rois = 0;
                                        // create a local copy of next rois
    Rois        = new TRois ( (char *) GODoc->GetRoisDoc ( CurrRois )->GetDocPath () );
    }


if ( IsRoiMode ) {
    Rois->Set ();                       // set all rois
                                        // sum up all rois into a single selection
    Rois->CumulateInto ( SelSP );
    }
else
    SelSP.Set ();

                                        // reload data!
GetInverse ( TFCursor.GetPosMin (), TFCursor.GetPosMax () );


if ( ControlBarGadgets ) {              // test window has been initialized
  //EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );
    GODoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

    ButtonGadgetSetState ( IDB_NEXTROI, IsRoiMode );

    Invalidate ( false );
    }
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetSliceMode ()
{
int     osm = SliceMode;

SliceMode   = NextState ( SliceMode, NumSliceMode, ShiftKey );


if ( osm )                              // save current matrix for this slice mode
    SlicesRotMatrix[ osm - 1 ] = ModelRotMatrix;
else                                    // save regular matrix
    SavedState.ModelRotMatrix = ModelRotMatrix;


if ( SliceMode ) {

    if ( ! osm ) {                      // first time we switch to slices?
                                        // save 3D state
        SavedState.Show2DIs         = Show2DIs;
        SavedState.Show3DIs         = Show3DIs;
        SavedState.ShowVectorsIs    = ShowVectorsIs;
        SavedState.Show3DMri        = Show3DMri;
        SavedState.ShowSp           = ShowSp;

        SavedState.ClipPlane[ 0 ]   = ClipPlane[ 0 ];
        SavedState.ClipPlane[ 1 ]   = ClipPlane[ 1 ];
        SavedState.ClipPlane[ 2 ]   = ClipPlane[ 2 ];


        if ( ! Show2DIs )               // force back the 2D IS
            Show2DIs        = InverseRendering2DDefault;

        if ( ShowVectorsIs )            // remove vectors IS
            ShowVectorsIs   = InverseRenderingVectorsNone;

        if ( Show3DIs )                 // remove 3D IS
            Show3DIs        = InverseRendering3DNone;

        if ( ShowSp != PointsShowNone ) // remove SP
            ShowSp          = PointsShowNone;
        }

                                        // set current matrix according to slice mode
    ModelRotMatrix  = SlicesRotMatrix[ SliceMode - 1 ];

                                        // force MRI to be in slices
    Show3DMri       = MriSliceOpaque;

                                        // select the right planes according to the projection
    InitClipPlanes ( ClipPlane );
    }
else {
                                        // restore 3D state
    Show2DIs        = SavedState.Show2DIs;
    Show3DIs        = SavedState.Show3DIs;
    ShowVectorsIs   = SavedState.ShowVectorsIs;
    Show3DMri       = SavedState.Show3DMri;
    ShowSp          = SavedState.ShowSp;

    ClipPlane[ 0 ]  = SavedState.ClipPlane[ 0 ];
    ClipPlane[ 1 ]  = SavedState.ClipPlane[ 1 ];
    ClipPlane[ 2 ]  = SavedState.ClipPlane[ 2 ];

    ModelRotMatrix  = SavedState.ModelRotMatrix;
    }

                                        // update da' bunch o' buttons
ButtonGadgetSetState ( IDB_SHOW2DIS,            Show2DIs  );
ButtonGadgetSetState ( IDB_SHOW3DIS,            Show3DIs  );
ButtonGadgetSetState ( IDB_SHOWVECTORSIS,       ShowVectorsIs );
ButtonGadgetSetState ( IDB_SHOW3DMRI,           Show3DMri );
ButtonGadgetSetState ( IDB_SHOWSP,              ShowSp    );
ButtonGadgetSetState ( IDB_CUTPLANECORONAL,     ClipPlane[ CtsToXyz ( 0 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANETRANSVERSE,  ClipPlane[ CtsToXyz ( 1 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANESAGITTAL,    ClipPlane[ CtsToXyz ( 2 ) ] );
ButtonGadgetSetState ( IDB_PLANEMODE,           SliceMode );


//if ( (bool) AnimFx && AnimFx.GetTimerId () == TimerStartup )
//    Zoom = 1;

SetItemsInWindow();

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetNumSlices ( owlwparam wparam )
{
if ( ! SliceMode )
    return;


if ( ! SetNumSlices ( wparam, ShiftKey ) )
    return;


SetItemsInWindow();

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmMoveSlice ( owlwparam wparam )
{
if ( ! SliceMode )
    return;


MoveSlice ( MRIDocClipp, wparam, ShiftKey );


SetItemsInWindow ();

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetCutPlane ( owlwparam wparam )
{
if ( SliceMode )                        // cutting planes are set by the program
    return;


int                 p               = CtsToXyz ( wparam - IDB_CUTPLANECORONAL );

//bool                singleside      = Show3DMri == MriSliceOpaque || Show3DMri == MriSliceOvercast;
bool                singleside      = false; // RenderingMode == MriSliceOpaque || RenderingMode == MriSliceOvercast;   // allowing flat slices to be shown on each side, more compatible with clipped volume
ClipPlaneDirection  num             = (ClipPlaneDirection) ( Show3DMri && ! singleside ? NumClipPlane : NumClipPlaneSym );

                                        // toggle to next state
                                                                                // inverting sequence for transverse and coronal cuts
ClipPlane[ p ].Set ( NextState ( ClipPlane[ p ].GetMode (), num, (bool) ( ShiftKey ^ ( p != 0 ) ) ) );


ButtonGadgetSetState ( wparam, ClipPlane[ p ] );

SetItemsInWindow ();


                                        // automatic smart reorientation?
//if ( ModelRotMatrix.IsOrthogonal ( SingleFloatEpsilon ) ) {

#define     ClipSag     ClipPlane[ 0 ]
#define     ClipCor     ClipPlane[ 1 ]
#define     ClipTra     ClipPlane[ 2 ]

                                        // used when toggling planes off
    static bool sagold          = false;
    static bool corold          = false;
    static bool traold          = false;
                                        // current states
    bool        sag             = (bool) ClipSag;
    bool        cor             = (bool) ClipCor;
    bool        tra             = (bool) ClipTra;
    bool        noclipping      = ! ( sag || cor || tra );

    bool        sagforward      = ClipSag.IsForward (); // ^ singleside;
    bool        corforward      = ClipCor.IsForward (); // ^ singleside;
    bool        traforward      = ClipTra.IsForward (); // ^ singleside;


    if      ( sag ) {

        Orientation = sagforward ? OrientSagittalRight : OrientSagittalLeft;       

        SetOrient ( MRIDocBackg );
                                        // cumulating additional rotations if more planes

                                        // cosmetic rotation - not mandatory, just disambiguate a bit the perspective
//      if ( tra && ! cor )     ModelRotMatrix.RotateY ( 10 * TrueToPlus ( (! sagforward) ^ traforward ), MultiplyLeft );
        if ( tra && ! cor )     ModelRotMatrix.RotateY ( 10 * TrueToPlus ( sagforward ), MultiplyLeft );
                                        // main rotation
        if ( cor )              ModelRotMatrix.RotateY ( 45 * TrueToPlus ( sagforward ^ corforward ), MultiplyLeft );
                                        // main rotation
        if ( tra )              ModelRotMatrix.RotateX ( 45 * TrueToPlus ( traforward ), MultiplyLeft );
                                        // cosmetic rotation - not mandatory, just disambiguate a bit the perspective
        if ( cor && ! tra )     ModelRotMatrix.RotateX ( 10, MultiplyLeft );
        }

    else if ( cor ) {

        Orientation = corforward ? OrientCoronalFront  : OrientCoronalBack;

        SetOrient ( MRIDocBackg ); 
                                        // cumulating additional rotations if more planes

                                        // main rotation
        if ( tra )              ModelRotMatrix.RotateX ( 45 * TrueToPlus ( traforward ), MultiplyLeft );
                                        // cosmetic rotation - not mandatory, just disambiguate a bit the perspective
        if ( tra )              ModelRotMatrix.RotateY ( -10, MultiplyLeft );
        }

    else if ( tra ) {

        Orientation = traforward ? OrientTransverseTop : OrientTransverseBottom;   

        SetOrient ( MRIDocBackg ); 
        }


    if ( noclipping ) {
                                        // in case user just switched off all planes, restore to predefined orientation instead of remaining in the last one
        if      ( sagold  )  Orientation = OrientSagittalLeft;
        else if ( corold  )  Orientation = OrientCoronalFront;
        else if ( traold  )  Orientation = OrientTransverseTop;

        SetOrient ( MRIDocBackg ); 
        }

                                        // remember for next call
    sagold  = sag;
    corold  = cor;
    traold  = tra;

#undef      ClipSag
#undef      ClipCor
#undef      ClipTra
//  }


//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmShiftCutPlane ( owlwparam wparam, bool forward )
{
int     p       = CtsToXyz ( wparam - IDB_CUTPLANECORONAL );

                                        // don't shift if no planes
if ( ! (bool) ClipPlane[ p ] )
    return;
                                        // shift it
ClipPlane[ p ].Shift ( forward ? 1 : -1 );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetBrightness ( owlwparam wparam )
{
double  osl = ScalingLevel;

if      ( wparam == IDB_ISINCBRIGHT )   ScalingLevel /= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep ;
else if ( wparam == IDB_ISDECBRIGHT )   ScalingLevel *= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep;

SetScaling ( ScalingLevel );

if ( osl == ScalingLevel )
    return;

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetContrast ( owlwparam wparam )
{
double  osc  = ScalingContrast;

if      ( wparam == IDB_ISINCCONTRAST )     ScalingContrast += 0.05;
else if ( wparam == IDB_ISDECCONTRAST )     ScalingContrast -= 0.05;

SetScalingContrast ( ScalingContrast );

if ( osc == ScalingContrast )
    return;

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmSearchRegularization ( owlwparam w )
{
                                        // are there any/enough regularizations?
if ( ISDoc->GetNumRegularizations () < 3 )
    return;


int                 oldreg          = CurrReg;


TDownsampling       downtf  (   w == IDB_SEARCHBESTREGCURSOR ? TFCursor.GetPosMin () : 0,
                                w == IDB_SEARCHBESTREGCURSOR ? TFCursor.GetPosMax () : EEGDoc->GetNumTimeFrames () - 1, // NoMore ( (long) EegMaxPointsDisplay, EEGDoc->GetNumTimeFrames () - 1 );
                                DownsamplingTargetSizeReg 
                            );


int                 newreg          = ISDoc->GetGlobalRegularization ( 0, GetEegView (), downtf.From, downtf.To, downtf.Step );

if ( oldreg == newreg )     
    return;


CurrReg     = newreg;


if ( ScalingAuto == ScalingAutoOff ) {
                                        // adjust the current scaling to keep the same visual aspect
    ScalingLevel   *= GetRegularizationRatio ( CurrReg, oldreg );

    SetScaling ( ScalingLevel );
    }


if ( ManageRangeCursor == MRCAverage )  // else it is updated during the paint
    GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // has to redo the isosurface
RedoSurfaceIS       = true;

SetItemsInWindow ();

UpdateCaption ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TInverseView::CmShowMinMax ( owlwparam wparam )
{
                                        // Right now it works this way:
                                        // - If only Max is allowed, just play forward/backward the setting
                                        // - If both Min and Max are allowed, the current one pressed will overwrite the other one
                                        //   so they are in sync (display does not allow otherwise for the moment)
if ( wparam == IDB_SHOWMIN ) {

    if ( IsSignedData ) {

//      ShowMin     = NextState ( ShowMin, NumInverseMinMax, ShiftKey );
        ShowMin     = NextState ( ShowMin, NumInverseMinMax, false );
                                            // synchronize to ShowMax
//      if ( ShowMin && ShowMax )   ShowMin = max ( ShowMin, ShowMax ); // highest of the 2 propagates
        if ( ShowMin && ShowMax )   ShowMax = ShowMin;                  // last one pressed propagates
        }

    } // IDB_SHOWMIN

else if ( wparam == IDB_SHOWMAX ) {

    ShowMax     = NextState ( ShowMax, NumInverseMinMax, ShiftKey );
                                            // synchronize to ShowMin
//  if ( ShowMin && ShowMax )   ShowMax = max ( ShowMin, ShowMax );     // highest of the 2 propagates
    if ( ShowMin && ShowMax )   ShowMin = ShowMax;                      // last one pressed propagates

    } // IDB_SHOWMAX


if ( ! IsSignedData )
    ShowMin = InverseMinMaxNone;


ButtonGadgetSetState ( IDB_SHOWMIN, ShowMin );
ButtonGadgetSetState ( IDB_SHOWMAX, ShowMax );

                                        // update all slices
Slices[ SliceModeCor - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeCor - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );


SetItemsInWindow ();

ShowNow ();
}


void    TInverseView::CmSetFindMinMax ()
{
FindMinMax      = ! FindMinMax;

ButtonGadgetSetState ( IDB_FINDMINMAX, FindMinMax );

                                        // update all slices
Slices[ SliceModeCor - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeCor - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData && FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMaxSlice ( /* ShowMax && */                 FindMinMax );


SetItemsInWindow ();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetManageRangeCursor ( owlwparam wparam )
{
                                        // stop if already running
if ( (bool) AnimTF ) {
    TFCursor.SetPos ( SavedTFCursor.GetPosMax() );
    RedoSurfaceIS = true;
    ButtonGadgetSetState ( IDB_RANGEANI, true );
    return;
    }


MRCNumTF       = TFCursor.GetPosMax() - TFCursor.GetPosMin() + 1;


switch  ( wparam ) {
    case    IDB_RANGEAVE:
        ManageRangeCursor   = MRCAverage;
        MRCNumTF   = 1;
        GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );
        break;

    case    IDB_RANGESEQ:
        ManageRangeCursor   = MRCSequence;

        CmSetStepTF ( 0 );

        break;

    case    IDB_RANGEANI:
        ManageRangeCursor   = MRCAnimation;

        AnimTF.Stop ();

        if ( ( Show2DIs || Show3DIs || ShowVectorsIs || FindMinMax ) && TFCursor.GetLength() != 1 ) {
            MRCNumTF        = 1;
            SavedTFCursor   = TFCursor;
            TFCursor.SetPos ( TFCursor.GetPosMin() - 1 );

            AnimTF.Start ( TimerTFCursor, MRCStepTF <= 4 ? 20 : 2 * MRCStepTF + 20, SavedTFCursor.GetLength() );
            }
        break;
    }
                                        // also by security
RedoSurfaceIS = true;

//CheckPairView ();
SetItemsInWindow ();

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::EvTimer ( uint timerId )
{
switch ( timerId ) {

    case TimerTFCursor:

        AnimTF++;
                                        // finished?
        if ( TFCursor.GetPosMax() >= SavedTFCursor.GetPosMax() ) {

            AnimTF.Stop ();
                                        // reload everything
            TFCursor    = SavedTFCursor;
            MRCNumTF    = TFCursor.GetPosMax() - TFCursor.GetPosMin() + 1;
            }
        else {                          // allow to change speed
            AnimTF.SetInterval ( MRCStepTF <= 4 ? 20 : 2 * MRCStepTF + 20 );

            TFCursor.ShiftPos ( MRCStepTF < 4 ? 1 << ( 4 - MRCStepTF ) : 1 );
            }

        GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMin() );
        RedoSurfaceIS = true;

        UpdateCaption ();
        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

        EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );
        break;


    case TimerStartup:

        AnimFx++;
                                        // finished?
        if ( ! (bool) AnimFx ) {

            AnimFx.Stop ();

            Show2DIs    = InverseRendering2DDefault;

            Zoom        = 1;

            Orientation = DefaultVolumeOrientation;
            CmOrient ();

            for ( int i = 1; i < AnimFx.GetMaxCount (); i++ )
                ModelRotMatrix.RotateXYZ ( 33.0 / ( AnimFx.GetMaxCount() - 1 ), -66.0 / ( AnimFx.GetMaxCount() - 1 ), -33.0 / ( AnimFx.GetMaxCount() - 1 ), MultiplyLeft );

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        Zoom        = sqrt ( AnimFx.GetPercentageCompletion() );
        ModelRotMatrix.RotateXYZ ( 33.0 / ( AnimFx.GetMaxCount() - 1 ), -66.0 / ( AnimFx.GetMaxCount() - 1 ), -33.0 / ( AnimFx.GetMaxCount() - 1 ), MultiplyLeft );

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;


    default:

        TBaseView::EvTimer ( timerId );
    }
}


//----------------------------------------------------------------------------
void    TInverseView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& p )
{
TBaseView::EvMouseWheel ( modKeys, zDelta, p );
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetScalingAdapt ()
{
ScalingAuto     = NextState ( ScalingAuto, IsSignedData ? NumScalingAuto : NumScalingAutoPositive, ShiftKey );

SetScaling ( ScalingLevel );

RedoSurfaceIS   = true;

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmSetShowSP ()
{
ShowSp      = NextState ( ShowSp, PointsNumRendering, ShiftKey );

ButtonGadgetSetState ( IDB_SHOWSP, ShowSp );

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmShowColorScale ()
{
ShowColorScale  = ! ShowColorScale;

SetItemsInWindow ();

//Invalidate ( false );
ShowNow ();
}


void    TInverseView::CmShowOrientation ()
{
ShowOrientation  = ! ShowOrientation;

SetItemsInWindow ();

Invalidate ( false );
}


void    TInverseView::CmShowAll ( owlwparam w )
{
TBaseView::CmShowAll ( w );

SetItemsInWindow ();

Invalidate ( false );
}


void    TInverseView::CmSetShiftDepthRange ( owlwparam w )
{
TBaseView::CmSetShiftDepthRange ( w );

SetItemsInWindow();

ShowNow ();
}


//----------------------------------------------------------------------------
long    TInverseView::SearchAndSetIntensity ( bool precise )
{
                                        // find the optimal scaling for first display
float               minValue;
float               maxValue;
float               absmaxValue;
long                maxtf           = 0;
float               v;


if ( precise ) {
                                        // find TF with max value
    TDownsampling       downtf ( 0, EEGDoc->GetNumTimeFrames() - 1, 10000 );

    TSuperGauge         Gauge ( "Scanning Levels", downtf.NumDownData );

    minValue        =  FLT_MAX;
    maxValue        = -FLT_MAX;
    absmaxValue     =  0;

    for ( long tf = downtf.From ; tf <= downtf.To; tf += downtf.Step ) {

        Gauge.Next ();

        GetInverse ( tf, tf );

        for ( int sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ ) {

            v           = InvBuffS[ sp ];

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

    GetInverse ( maxtf, maxtf );

    minValue        =  FLT_MAX;
    maxValue        = -FLT_MAX;
    absmaxValue     =  0;
                                        // then scan only the values at this TF (much faster)
    for ( int sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ ) {

        v           = InvBuffS[ sp ];

        Mined ( minValue, v );
        Maxed ( maxValue, v );

        if ( fabs ( v ) > absmaxValue )
            absmaxValue = fabs ( v );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // Not currently used (2019-05-30) - remove later if confirmed
TEasyStats          stats;
                                        // we just want these data
GetInverse ( EEGDoc->GetMaxGfpTF (), EEGDoc->GetMaxGfpTF () );

for ( sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ )

    if ( InvBuffS[ sp ] != 0 )

        stats.Add ( fabs ( InvBuffS[ sp ] ) );

                                        // also look for data dispersion
IsSpreading     = stats.CoefficientOfVariation ();

//DBGV ( IsSpreading, "IsSpreading" );
*/

/*                                      // Histogram of inverse data at max Gfp position
THistogram          cdf ( ... );
TVector<double>     ValuesEqualize;

for ( sp = 0; sp < SPDoc->GetNumSolPoints (); sp++ )

    if ( InvBuffS[ sp ] != 0 )

        histo[ NoMore ( 100.0, fabs ( InvBuffS[ sp ] ) / ( absmaxValue - DoubleFloatEpsilon ) * 101 ) ]++;

                                        // get equalization LUT
histo.EqualizeCDF ( ValuesEqualize );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can set the absolute limits
SetScalingLimits ( absmaxValue * 1e-2, absmaxValue * 5 );


minValue    = ScalingAuto == ScalingAutoAsymmetric ? minValue : - absmaxValue;  // ?test if positive value?
maxValue    = ScalingAuto == ScalingAutoAsymmetric ? maxValue :   absmaxValue;

                                        // saturate, and set as new level
//minValue /= IsSignedData ? 1.5 : 1.25;
//maxValue /= IsSignedData ? 1.5 : 1.25;


if ( ScalingAuto == ScalingAutoOff )
    SetScaling ( minValue, maxValue, ScalingAuto != ScalingAutoAsymmetric );

                                        // !reload previous data!
GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


return  maxtf;
}


void    TInverseView::CmSearchTFMax ()
{
                                        // modify cursor position
TFCursor.SetPos ( SearchAndSetIntensity ( true ) );


MRCNumTF        = 1;

AnimTF.Stop ();

MRCDoAnimation  = false;


EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );


GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );


SetItemsInWindow ();

UpdateCaption ();

Invalidate ( false );
}


void    TInverseView::CmNextColorTable ()
{
ColorTable.NextColorTable ( ShiftKey );

ColorTableIndex[ ISDoc->GetAtomType ( AtomTypeUseCurrent ) ]    = ColorTable.GetTableType ();

//Invalidate ( false );
ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::SetItemsInWindow ()
{
if ( SliceMode ) {
    double              msx,    msy;
    GetSliceSize ( MRIDocBackg, msx, msy );

    FitItemsInWindow ( Slices[ SliceMode - 1 ].GetTotalSlices(), TSize ( msx, msy ), SlicesPerX, SlicesPerY );

//  DBGV2 ( SliceMode, Slices[ SliceMode - 1 ].GetTotalSlices(), "SliceMode  #slices" );
    }
else
    FitItemsInWindow ( 1, TSize ( 1, 1 ), SlicesPerX, SlicesPerY );
}


void    TInverseView::FitItemsInWindow ( int numitems, TSize itemsize, int &byx, int &byy, TRect *winrect )
{
TRect   wr;

SeqPerX = SeqPerY = 1;

if ( ManageRangeCursor == MRCSequence ) {
                                        // ignore real window size, put it into a squared window
    wr  = TRect ( 0, 0, 10000, 10000 );

    TBaseView::FitItemsInWindow ( numitems, itemsize, byx, byy, &wr );
                                        // now spread the big squared window into a sequence inside the real window
    wr  = GetWindowRect();
    wr.SetWH ( 0, 0, wr.Width(), (double) wr.Height() );

    int     n = (double) MRCNumTF / MRCStepTF;

    if ( (double) MRCNumTF / MRCStepTF - n > 0 )
        n++;

    TBaseView::FitItemsInWindow ( n, TSize ( byx * itemsize.X(), byy * itemsize.Y() ), SeqPerX, SeqPerY, &wr );
    }
else
    TBaseView::FitItemsInWindow ( numitems, itemsize, byx, byy, winrect );
}


void    TInverseView::CmSetStepTF ( owlwparam wparam )
{
if ( wparam ) {
    int     oldStepTF = MRCStepTF;

    TSecondaryView::CmSetStepTF ( wparam );

    SetItemsInWindow ();

    if ( oldStepTF != MRCStepTF && IsMRCSequence () )
        Invalidate ( false );
//      ShowNow ();
    }
else {
    MRCStepTF = (double) sqrt ( MRCNumTF / 6.0 ); // until 24, step = 1 ( 24 / 6 = 4 -> 2 )

    if ( MRCStepTF == 0 || EEGDoc->IsTemplates () )
        MRCStepTF = 1;
    }
}


void    TInverseView::CmOrient ()
{
if      ( SliceMode == SliceModeCor )   ModelRotMatrix = CTSMatrix[ 0 ];
else if ( SliceMode == SliceModeTra )   ModelRotMatrix = CTSMatrix[ 1 ];
else if ( SliceMode == SliceModeSag )   ModelRotMatrix = CTSMatrix[ 2 ];
else {
    TBaseView::CmOrient ();

    SetOrient ( MRIDocBackg );
    }

Invalidate ( false );
//ShowNow ();
}


//----------------------------------------------------------------------------
void    TInverseView::CmSetAveragingMode ( owlwparam w )
{
AveragingPrecedence     = w == CM_INVERSEAVERAGEBEFORE ? AverageBeforeInverse : AverageAfterInverse;


for ( int i = 0; i < GODoc->GetNumRisDoc (); i++ )

    GODoc->GetRisDoc ( i )->SetAveragingBefore ( AveragingPrecedence );   // actually useless, ris files are already computed


for ( int i = 0; i < GODoc->GetNumIsDoc (); i++ )

    GODoc->GetIsDoc ( i )->SetAveragingBefore ( AveragingPrecedence );


GODoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG );
}


void    TInverseView::CmPrecedenceBeforeEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( AveragingPrecedence == AverageBeforeInverse );
}


void    TInverseView::CmPrecedenceAfterEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( AveragingPrecedence == AverageAfterInverse );
}


//----------------------------------------------------------------------------
bool    TInverseView::VnNewTFCursor ( TTFCursor *tfc )
{
if ( ! IsFriendView ( tfc->SentTo ) )
    return false;                     // not for me !


if ( *tfc == TFCursor )    return false;


long    oldMRCNumTF = TFCursor.GetLength ();


TFCursor            = *tfc;             // transfer positions
TFCursor.SentTo     = LinkedViewId;
TFCursor.SentFrom   = GetViewId();


MRCNumTF   = TFCursor.GetLength ();


if ( ManageRangeCursor == MRCAverage ) {
    MRCNumTF   = 1;

//  HCURSOR  hPrevCursor = !ISDoc->GetAveragingMode() && TFCursor->GetLength () > 10 ? ::SetCursor ( ::LoadCursor ( 0, IDC_WAIT ) ) : 0;

    GetInverse ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

//  if ( hPrevCursor )  ::SetCursor ( hPrevCursor );
    }
else if ( IsMRCSequence () ) {
    if ( MRCNumTF != oldMRCNumTF )      // reset only in case user changed the selection size
        CmSetStepTF ( 0 );              // set a default spacing
    }


UpdateCaption ();

AnimTF.Stop ();


SetItemsInWindow ();
RedoSurfaceIS = true;

//if ( Show2DIs )
    ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

                                        // send the cursor to the EEG view also, if it is not the original sender
if ( tfc->SentFrom != LinkedViewId )
    EEGDoc->QueryViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

return false;
}


void    TInverseView::ReloadRoi ()
{
if ( ! IsRoiMode )
    return;                             // but don't change anything

                                        // this in contrary is weird
if ( ! ( GODoc && GODoc->GetNumRoiDoc () ) ) {

    IsRoiMode   = false;

    if ( Rois ) delete Rois, Rois = 0;
    CurrRois    = 0;
    return;
    }

Rois    = new TRois ( (char *) GODoc->GetRoisDoc ( CurrRois )->GetDocPath () );

Rois->Set ();
}


bool    TInverseView::VnReloadData ( int what )
{
switch ( what ) {

  case EV_VN_RELOADDATA_REF:
  case EV_VN_RELOADDATA_EEG:

    SearchAndSetIntensity ();

    IsSignedData        = ! ISDoc ->IsAbsolute ( AtomTypeUseCurrent );

    SetColorTable ( ISDoc->GetAtomType ( AtomTypeUseCurrent ) );

    SetItemsInWindow ();

    RedoSurfaceIS = true;

//  Invalidate ( false );
    ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );

    break;


  case EV_VN_RELOADDATA_CAPTION:

    UpdateCaption ();

    break;


  case EV_VN_RELOADDATA_DOCPOINTERS:

    ReloadRoi ();

    if ( ! Rois )
        break;

    Invalidate ( false );
    break;

    }


return true;
}


bool    TInverseView::VnNewHighlighted ( TSelection *sel )
{
    // message can be either for electrodes or SPs                                if 0, accept all
if ( abs ( sel->Size () - Highlighted.Size () ) > NumPseudoTracks || sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                        // not for me !

//if ( ! IsFriendView ( hl->SentTo ) )
//    return false;                       // not for me !


if ( Highlighted != *sel ) {

    Highlighted  = *sel;

    if ( ShowSp || Show3DIs == InverseRendering3DSolPoints || ShowVectorsIs )
//      ShowNow ();                     // too early: TLinkManyDoc::NotifyDocViews notifies EEG and RIS first, SP not updated yet
        Invalidate ( false );           // this one is OK, SP View has been updated now

    }

return  true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
