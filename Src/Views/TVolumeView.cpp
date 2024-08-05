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

#include    "TVolumeView.h"

#include    "Dialogs.Input.h"
#include    "Math.Histo.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "OpenGL.h"
#include    "Volumes.TTalairachOracle.h"
#include    "GlobalOptimize.Tracks.h"

#include    "TExportTracks.h"
#include    "TExportVolume.h"

#include    "TSolutionPointsDoc.h"

#include    "TPreprocessMrisDialog.h"
#include    "CoregistrationMrisUI.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle        Taloracle;

extern  TPreprocessMrisStruct       MriPreprocTransfer;


const char  EditingToolsString[ NumEditingTools ][ 256 ] =
        {
        "Not editing",

        "Cutting Sphere"                                        NewLine 
        "Changing radius: <Right click> + horiz. mouse move"    NewLine 
        "Deleting content: <Middle click>"                      NewLine 
        "Keeping content: <Control> + <Middle click>"           NewLine 
        "Filling content:    <Shift> + <Middle click>",

        "Blurring Sphere"                                       NewLine 
        "Changing radius: <Right click> + horiz. mouse move"    NewLine 
        "Blurring content: <Middle click>",

        "Cutting Cylinder"                                      NewLine
        "Changing radius: <Right click> + horiz. mouse move"    NewLine 
        "Changing length: <Right click> + vert. mouse move"     NewLine 
        "Deleting content: <Middle click>"                      NewLine
        "Keeping content: <Control> + <Middle click>"           NewLine 
        "Filling content:    <Shift> + <Middle click>",

        "Surface Cutting Sphere"                                NewLine
        "Changing radius: <Right click> + horiz. mouse move"    NewLine
        "Deleting content: <Middle click>"                      NewLine
        "Keeping content: <Control> + <Middle click>"           NewLine 
        "Filling content:    <Shift> + <Middle click>",

        "Cutting Plane"                                         NewLine
        "Deleting content: <Middle click>",
        };


//----------------------------------------------------------------------------

    TVolumeState::TVolumeState ()    
{
RenderingMode   = 0;
Clip[ 0 ]       =
Clip[ 1 ]       = 
Clip[ 2 ]       = 
Clip[ 3 ]       = 
Clip[ 4 ]       = 
Clip[ 5 ]       = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TVolumeView, TBaseView)

    EV_WM_SIZE,
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
    EV_VN_VIEWUPDATED,

    EV_COMMAND          ( IDB_2OBJECT,                  Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,                   CmOrient ),
    EV_COMMAND          ( IDB_MAGNIFIER,                CmMagnifier ),
    EV_COMMAND          ( IDB_SURFACEMODE,              CmSetRenderingMode ),

    EV_COMMAND_AND_ID   ( IDB_ISOMANUAL,                CmIsoMode ),
    EV_COMMAND          ( IDB_COLORIZESURFACE,          CmSetSurfaceMode ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANECORONAL,          CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANETRANSVERSE,       CmSetCutPlane ),
    EV_COMMAND_AND_ID   ( IDB_CUTPLANESAGITTAL,         CmSetCutPlane ),

    EV_COMMAND          ( CM_NEWBACKTHRESH,             CmNewIsoSurfaceCut ),
    EV_COMMAND          ( CM_MRIDOWNSAMPLING,           CmNewDownsampling ),

    EV_COMMAND_AND_ID   ( CM_FILTERMEAN,                CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERGAUSSIAN,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERANISOGAUSSIAN,       CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYADD,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYSUB,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYMULT,       CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYDIV,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYOPPOSITE,   CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYINVERT,     CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYABS,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYREMAP,      CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERINTENSITYNORM,       CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKEEPVALUE,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMIN,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMAX,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMINMAX,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERRANGE,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMEDIAN,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMAXMODE,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMEANSUB,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLOGMEANSUB,          CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMEANDIV,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLOGMEANDIV,          CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMAD,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSD,                  CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSDINV,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMADSD,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCOV,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLOGCOV,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSNR,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMCOV,                CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLOGMCOV,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMODE,                CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERENTROPY,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERPERCENTFULL,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCUTBELOWFULL,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSAV,                 CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCOMPACTNESS,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERTHRESHOLD,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERTHRESHOLDABOVE,      CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERTHRESHOLDBELOW,      CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSYMMETRIZE,          CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERBINARIZE,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERTOMASK,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERIREVERT,             CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLESSNEIGH,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMORENEIGH,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERRELAX,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKEEPBIGGEST,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKEEPCOMPACT,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKEEPBIGGEST2,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKEEPCOMPACT2,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCLUSTERSTOREGIONS,   CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERGRADIENT,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCANNY,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERLAPLACIAN,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERHESSIANEIGENMAX,     CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERHESSIANEIGENMIN,     CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKCURVATURE,          CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERKCCURVATURE,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERERODE,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERDILATE,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTEROPEN,                CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERCLOSE,               CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMORPHGRADIENT,       CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMORPHGRADIENTINT,    CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMORPHGRADIENTEXT,    CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERTHINNING,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERWATERFALLRIDGES,     CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERWATERFALLVALLEYS,    CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERBIASFIELD,           CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSEGMENTCSF,          CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSEGMENTGREY,         CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSEGMENTWHITE,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERSEGMENTTISSUES,      CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERMASKTOSP,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_SKULLSTRIPPING,            CmFilter ),
    EV_COMMAND_AND_ID   ( CM_CUTBRAINSTEM,              CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERHISTOCOMPUTE,        CmOutputHistogram ),
    EV_COMMAND_AND_ID   ( CM_FILTERHISTOCOMPUTELOG,     CmOutputHistogram ),
    EV_COMMAND_AND_ID   ( CM_FILTERHISTOCOMPUTETXT,     CmOutputHistogram ),
    EV_COMMAND_AND_ID   ( CM_FILTERCDFCOMPUTE,          CmOutputHistogram ),
    EV_COMMAND_AND_ID   ( CM_FILTERHISTOEQUALIZE,       CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERHISTOEQUALIZEBRAIN,  CmFilter ),
//  EV_COMMAND_AND_ID   ( CM_FILTERHISTOCOMPACT,        CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERRANK,                CmFilter ),
    EV_COMMAND_AND_ID   ( CM_FILTERRANKRAMP,            CmFilter ),
//  EV_COMMAND_AND_ID   ( CM_FILTERRESIZE,              CmFilter ),

    EV_COMMAND_AND_ID   ( CM_OPERATIONAND,              CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONOR,               CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONXOR,              CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONADD,              CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONSUB,              CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONMULT,             CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONDIV,              CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONCLEAR,            CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONFILL,             CmOperation ),
    EV_COMMAND_AND_ID   ( CM_OPERATIONREPLACE,          CmOperation ),

    EV_COMMAND_AND_ID   ( IDB_FLIPX,                    CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPY,                    CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPZ,                    CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPXY,                   CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPYZ,                   CmFlip ),
    EV_COMMAND_AND_ID   ( IDB_FLIPXZ,                   CmFlip ),

    EV_COMMAND          ( CM_SETORIENTATION,    CmSetOrientation ),
    EV_COMMAND          ( CM_RESETORIENTATION,  CmResetOrientation ),
    EV_COMMAND_ENABLE   ( CM_RESETORIENTATION,  CmResetOrientationEnable ),
    EV_COMMAND          ( CM_MRINEWORIGIN,      CmSetNewOrigin ),

    EV_COMMAND_AND_ID   ( IDB_SHOWMIN,          CmShowMinMax ),
    EV_COMMAND_ENABLE   ( IDB_SHOWMIN,          CmIsScalarEnable ),
    EV_COMMAND_AND_ID   ( IDB_SHOWMAX,          CmShowMinMax ),
    EV_COMMAND          ( IDB_FINDMINMAX,       CmSetFindMinMax ),

    EV_COMMAND_AND_ID   ( IDB_ISINCBRIGHT,      CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_ISDECBRIGHT,      CmSetBrightness ),
    EV_COMMAND_AND_ID   ( IDB_ISINCCONTRAST,    CmSetContrast ),
    EV_COMMAND_AND_ID   ( IDB_ISDECCONTRAST,    CmSetContrast ),
    EV_COMMAND          ( IDB_FIXEDSCALE,       CmSetScalingAdapt ),
    EV_COMMAND          ( IDB_SPCOLOR,          CmNextColorTable ),
    EV_COMMAND_ENABLE   ( IDB_ISINCBRIGHT,      CmSetIntensityLevelEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISDECBRIGHT,      CmSetIntensityLevelEnable ),
    EV_COMMAND_ENABLE   ( CM_SPUSERSCALE,       CmSetIntensityLevelEnable ),

    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE0,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE1,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE2,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE3,  CmSetShiftDepthRange ),
    EV_COMMAND_AND_ID   ( CM_SHIFTDEPTHRANGE4,  CmSetShiftDepthRange ),

    EV_COMMAND          ( IDB_PLANEMODE,        CmSetSliceMode ),
    EV_COMMAND_AND_ID   ( IDB_LESSSLICES,       CmSetNumSlices ),
    EV_COMMAND_AND_ID   ( IDB_MORESLICES,       CmSetNumSlices ),
    EV_COMMAND_AND_ID   ( IDB_FIRSTSLICEFWD,    CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_FIRSTSLICEBWD,    CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_LASTSLICEFWD,     CmMoveSlice ),
    EV_COMMAND_AND_ID   ( IDB_LASTSLICEBWD,     CmMoveSlice ),
    EV_COMMAND_ENABLE   ( IDB_LESSSLICES,       CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_MORESLICES,       CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_FIRSTSLICEFWD,    CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_FIRSTSLICEBWD,    CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_LASTSLICEFWD,     CmSliceModeEnable ),
    EV_COMMAND_ENABLE   ( IDB_LASTSLICEBWD,     CmSliceModeEnable ),

    EV_COMMAND          ( IDB_EDITING,          CmSetEditing ),

    EV_COMMAND          ( CM_MRICOREGISTRATION, CmCoregistration ),

    EV_COMMAND          ( CM_NORMALIZEMRI,      CmPreprocessMris ),

END_RESPONSE_TABLE;

                                        // allow a contrast range from 0.5 to 1
#define     ToContrast(SC)          0.10 + 9.90 * SC * SC
#define     ToScalingContrast(C)    sqrt ( ( C - 0.10 ) / 9.90 )
                                        // new contrast formula with color table with cut-off slope
//#define     ToContrast(SC)          (SC)
//#define     ToScalingContrast(C)    (C)


        TVolumeView::TVolumeView ( TVolumeDoc& doc, TWindow* parent, TLinkManyDoc* group )
        : TBaseView ( doc, parent, group ), MRIDoc ( &doc )
{
GODoc               = 0;                // force it to no-group -> 1 window is enough

                                        // view will initiate the first computation of isosurface tesselation
if ( MRIDoc->GetTessSurfacePos ()->IsEmpty ()
  && MRIDoc->GetTessSurfaceNeg ()->IsEmpty () )

    MRIDoc->SetIsoSurface ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RenderingMode       = MriOpaque;
CurrentDisplaySpace = DisplaySpace3D;

Zoom                = CartoolApplication->AnimateViews ? 0.001 : 1;

                                        // focus on the data limits
SetBoxParameters ();

SearchAndSetIntensity ();

SetColorTable ( MRIDoc->GetAtomType ( AtomTypeUseCurrent ) );


CappingMode         = MriBrainCapping;  // not used anymore, always this value
Orientation         = DefaultVolumeOrientation - 1;
IsoMode             = MriIsoFixed;
CurrIsoDownsampling = 0;
DepthRange.UserShift= 0;

Editing             = EditingToolNone;
EditingRadius       = MRIDoc->GetBounding()->MaxSize () * 0.10;
EditingDepth        = MRIDoc->GetBounding()->MaxSize () * 0.10;
EditingNow          = false;            //flag to avoid reentrant calls
                                        // colorizing the isosurface
SetDefaultSurfaceMode   ();
ComputeSurfaceColoring  ();

ScalingContrast     = ToScalingContrast ( MRIDoc->IsMask () ? 1 : MRIDoc->IsRis () ? 10 : 0.5 );

SliceMode           = SliceModeNone;

ShowMin             = MriMinMaxNone;
ShowMax             = MriMinMaxNone;

FindMinMax          = false;

CurrentOperation    = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do this quite early, as it sets some main variables
//SetSliceMatrices ( MRIDoc, ModelCenter, ModelRadius );


Slices[ SliceModeCor - 1 ] = TSliceSelection ( MRIDoc->GetFirstSlice ( CorronalIndex   () ), MRIDoc->GetLastSlice ( CorronalIndex   () ) );
Slices[ SliceModeTra - 1 ] = TSliceSelection ( MRIDoc->GetFirstSlice ( TransverseIndex () ), MRIDoc->GetLastSlice ( TransverseIndex () ) );
Slices[ SliceModeSag - 1 ] = TSliceSelection ( MRIDoc->GetFirstSlice ( SagittalIndex   () ), MRIDoc->GetLastSlice ( SagittalIndex   () ) );


if ( MRIDoc->IsFullHead () ) {          // make the slices a little closer to the center
    Slices[ SliceModeCor - 1 ].ShiftFirstSlice (  3 );
    Slices[ SliceModeCor - 1 ].ShiftLastSlice  ( -3 );
    Slices[ SliceModeTra - 1 ].ShiftFirstSlice (  3 );
    Slices[ SliceModeTra - 1 ].ShiftLastSlice  ( -3 );
    Slices[ SliceModeSag - 1 ].ShiftFirstSlice (  3 );
    Slices[ SliceModeSag - 1 ].ShiftLastSlice  ( -3 );
    }


SlicesPerX      = 20;
SlicesPerY      = 20;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init viewpoints
//double              maxsize     = MRIDoc->GetBounding()->MaxSize () * 1.1; // BoxDiameter () * 0.707;
//Viewpoints.Set ( ViewpointsTopBackLeftSummary, MRIDoc->GetBounding()->MaxSize () * 1.1, MRIDoc->GetStandardOrientation ( LocalToPIR ) );
//Viewpoints.Set ( ViewpointsLeftBackRight,      MRIDoc->GetBounding()->MaxSize () * 1.1, MRIDoc->GetStandardOrientation ( LocalToPIR ) );
//Viewpoints.Set ( ViewpointsFrontBack,          MRIDoc->GetBounding()->MaxSize () * 1.1, MRIDoc->GetStandardOrientation ( LocalToPIR ) );



#define     MaterialOutAmbient      0.15,  0.15,  0.15,  1.00
#define     MaterialOutDiffuse      0.80,  0.80,  0.80,  1.00

MaterialOut = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      MaterialOutAmbient,
                                      MaterialOutDiffuse,
                                      0.20,  0.20,  0.20,  1.00,
                                      0.00,  0.00,  0.00,  0.00,
                                     30.00 );

/*
#define     MaterialInAmbient       0.30,  0.30,  0.00,  1.00
#define     MaterialInDiffuse       0.85,  0.85,  0.45,  1.00

MaterialIn  = TGLMaterial<GLfloat> ( GL_BACK,
                                      MaterialInAmbient,
                                      MaterialInDiffuse,
                                      0.25,  0.25,  0.25,  1.00,
                                      0.00,  0.00,  0.00,  0.00,
                                     30.00 );
*/

#define     MRIALPHA                0.40
#define     MaterialTrAmbient       0.00,  0.00,  0.00,  0.00
#define     MaterialTrDiffuse       0.55,  0.55,  0.55,  MRIALPHA

MaterialTr = TGLMaterial<GLfloat> ( GL_FRONT_AND_BACK,
                                      MaterialTrAmbient,
                                      MaterialTrDiffuse,
                                      1.00,  1.00,  1.00,  1.00,
                                      0.00,  0.00,  0.00,  0.00,
                                     40.00 );

/*
ClipPlane[ 0 ]  = TGLClipPlane ( GL_CLIP_PLANE0, -1,  0,  0, ModelCenter.X, b->XMin() - 1 - MRIDoc->GetOrigin ().X,
                                                                            b->XMax() + 1 - MRIDoc->GetOrigin ().X  );
ClipPlane[ 1 ]  = TGLClipPlane ( GL_CLIP_PLANE1,  0, -1,  0, ModelCenter.Y, b->YMin() - 1 - MRIDoc->GetOrigin ().Y,
                                                                            b->YMax() + 1 - MRIDoc->GetOrigin ().Y  );
ClipPlane[ 2 ]  = TGLClipPlane ( GL_CLIP_PLANE2,  0,  0, -1, ModelCenter.Z, b->ZMin() - 1 - MRIDoc->GetOrigin ().Z,
                                                                            b->ZMax() + 1 - MRIDoc->GetOrigin ().Z  );
*/

ClipPlane[ 0 ].SetNone ();
ClipPlane[ 1 ].SetNone ();
ClipPlane[ 2 ].SetNone ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temp code to reload markers
TFileName           markerfilename;

StringCopy      ( markerfilename, MRIDoc->GetDocPath () );
AddExtension    ( markerfilename, FILEEXT_MRK );

Markers.ReadFile ( markerfilename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );
Attr.W      = Attr.H /** WindowHeightToWidthRatio*/;
StandSize   = TSize ( MRIWindowSizeW, MRIWindowSizeH );


SetViewMenu ( new TMenuDescr ( IDM_MRI ) );


NumControlBarGadgets    = MRIGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [MRIGLVIEW_CBG_NUM];

CreateBaseGadgets ();

ControlBarGadgets[MRIGLVIEW_CBG_SEP1]       = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_ISOMANUAL]  = new TButtonGadget(IDB_ISOMANUAL, IDB_ISOMANUAL, TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[MRIGLVIEW_CBG_COLORIZESURFACE]    = new TButtonGadget(IDB_COLORIZESURFACE, IDB_COLORIZESURFACE, TButtonGadget::NonExclusive, true, SurfaceMode ? TButtonGadget::Down : TButtonGadget::Up, false);
ControlBarGadgets[MRIGLVIEW_CBG_EDITING]    = new TButtonGadget(IDB_EDITING, IDB_EDITING, TButtonGadget::NonExclusive, true, Editing ? TButtonGadget::Down : TButtonGadget::Up, false);

ControlBarGadgets[MRIGLVIEW_CBG_SEP1A]      = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_CUTPLANECORONAL]    = new TButtonGadget(IDB_CUTPLANECORONAL, IDB_CUTPLANECORONAL, TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[MRIGLVIEW_CBG_CUTPLANETRANSVERSE] = new TButtonGadget(IDB_CUTPLANETRANSVERSE, IDB_CUTPLANETRANSVERSE, TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);
ControlBarGadgets[MRIGLVIEW_CBG_CUTPLANESAGITTAL]   = new TButtonGadget(IDB_CUTPLANESAGITTAL, IDB_CUTPLANESAGITTAL, TButtonGadget::NonExclusive, true, TButtonGadget::Up, false);

ControlBarGadgets[MRIGLVIEW_CBG_SEP2]       = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_PLANEMODE]     = new TButtonGadget(IDB_PLANEMODE, IDB_PLANEMODE, TButtonGadget::NonExclusive, false, TButtonGadget::Up );
ControlBarGadgets[MRIGLVIEW_CBG_LESSSLICES]    = new TButtonGadget(IDB_LESSSLICES,   IDB_LESSSLICES,     TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_MORESLICES]    = new TButtonGadget(IDB_MORESLICES,   IDB_MORESLICES,     TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_FIRSTSLICEBWD] = new TButtonGadget(IDB_FIRSTSLICEBWD,IDB_FIRSTSLICEBWD,  TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_FIRSTSLICEFWD] = new TButtonGadget(IDB_FIRSTSLICEFWD,IDB_FIRSTSLICEFWD,  TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_LASTSLICEBWD]  = new TButtonGadget(IDB_LASTSLICEBWD, IDB_LASTSLICEBWD,   TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_LASTSLICEFWD]  = new TButtonGadget(IDB_LASTSLICEFWD, IDB_LASTSLICEFWD,   TButtonGadget::Command);

ControlBarGadgets[MRIGLVIEW_CBG_SEP3]          = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_SHMIN]         = new TButtonGadget(IDB_SHOWMIN,      IDB_SHOWMIN,        TButtonGadget::NonExclusive);
ControlBarGadgets[MRIGLVIEW_CBG_SHMAX]         = new TButtonGadget(IDB_SHOWMAX,      IDB_SHOWMAX,        TButtonGadget::NonExclusive);
ControlBarGadgets[MRIGLVIEW_CBG_FINDMINMAX]    = new TButtonGadget(IDB_FINDMINMAX,   IDB_FINDMINMAX,     TButtonGadget::NonExclusive);

ControlBarGadgets[MRIGLVIEW_CBG_SEP4]       = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_DECBR]      = new TButtonGadget(IDB_ISDECBRIGHT,  IDB_ISDECBRIGHT,    TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_INCBR]      = new TButtonGadget(IDB_ISINCBRIGHT,  IDB_ISINCBRIGHT,    TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_DECCR]      = new TButtonGadget(IDB_ISDECCONTRAST,IDB_ISDECCONTRAST,  TButtonGadget::Command);
ControlBarGadgets[MRIGLVIEW_CBG_INCCR]      = new TButtonGadget(IDB_ISINCCONTRAST,IDB_ISINCCONTRAST,  TButtonGadget::Command);

ControlBarGadgets[MRIGLVIEW_CBG_SEP5]       = new TSeparatorGadget( ButtonSeparatorWidth );
ControlBarGadgets[MRIGLVIEW_CBG_FXDSCL]     = new TButtonGadget(IDB_FIXEDSCALE,   IDB_FIXEDSCALE,     TButtonGadget::NonExclusive);
ControlBarGadgets[MRIGLVIEW_CBG_COLOR]      = new TButtonGadget(IDB_SPCOLOR,      IDB_SPCOLOR,        TButtonGadget::Command);


if ( ! ValidView () )
    NotOK();                // do not create the window (cancel from doc)
}


//----------------------------------------------------------------------------
                                        // these parameters depend on the bounding box
void    TVolumeView::SetBoxParameters ()
{
                                        // reset these variables
const TBoundingBox<double>*     bdata   = MRIDoc->GetBounding ();

ModelCenter = bdata->GetCenter ();
                                        // take the radius of the whole data box
ModelRadius = DepthPositionRatio * MRIDoc->GetSize ()->AbsRadius ();

MRIDoc->ToAbs ( ModelCenter );


SetSliceMatrices ( MRIDoc, ModelCenter, ModelRadius );


TGLClipPlane        savedclipplane0  = ClipPlane[ 0 ];
TGLClipPlane        savedclipplane1  = ClipPlane[ 1 ];
TGLClipPlane        savedclipplane2  = ClipPlane[ 2 ];


TPointDouble        origin          = MRIDoc->GetOrigin ();

                                        // introduce a slight shift, in case of Mask which can cut in a perfect plane -> artefacts
ClipPlane[ 0 ]  = TGLClipPlane ( GL_CLIP_PLANE0, -1,  0,  0, ModelCenter.X + 1e-3, bdata->XMin() - origin.X, bdata->XMax() - origin.X );
ClipPlane[ 1 ]  = TGLClipPlane ( GL_CLIP_PLANE1,  0, -1,  0, ModelCenter.Y + 1e-3, bdata->YMin() - origin.Y, bdata->YMax() - origin.Y );
ClipPlane[ 2 ]  = TGLClipPlane ( GL_CLIP_PLANE2,  0,  0, -1, ModelCenter.Z + 1e-3, bdata->ZMin() - origin.Z, bdata->ZMax() - origin.Z );

                                        // if origin seems OK, override the initial position at the origin (0,0,0 in absolute coordinates)
if ( MRIDoc->IsValidOrigin ( origin ) ) {

    ClipPlane[ 0 ].SetPosition ( 0 );
    ClipPlane[ 1 ].SetPosition ( 0 );
    ClipPlane[ 2 ].SetPosition ( 0 );
    }

                                        // select the right planes according to the projection
InitClipPlanes ( ClipPlane );

                                        // restore actual clippings
ClipPlane[ 0 ].Set ( savedclipplane0 );
ClipPlane[ 1 ].Set ( savedclipplane1 );
ClipPlane[ 2 ].Set ( savedclipplane2 );


Fog         = TGLFog<GLfloat>   ( ModelRadius * FogDefaultNear,
                                  ModelRadius * FogDefaultFar,
                                  GLBASE_FOGCOLOR_NORMAL );

                                        // Full limits
const TBoundingBox<int>*    bsize   = MRIDoc->GetSize ();

                                        // Update axis, in case origin has changed
double              headh           = bsize->MeanSize() * 0.06;
double              headl           = bsize->MeanSize() * 0.015;
double              arrowtailmarginpastbox      = 0;        // if origin is below box, make the arrow point outside the bounding box            - default: 0
double              arrowheadmarginwithinbox    = headh;    // if origin is within box, make the arrow point outside the bounding box           - default: 0 to remain inside the box
double              arrowheadmarginpastbox      = headl;    // if origin is above box, give it a little more to avoid intersecting arrow heads  - default: 0


Xaxis       = TGLArrow<GLfloat>   ( bsize->XMin() - AtLeast ( arrowtailmarginpastbox, origin.X ),
                                    0, 
                                    0,
                                    AtLeast ( arrowheadmarginpastbox, bsize->XMax() - origin.X ) + arrowheadmarginwithinbox, 
                                    0, 
                                    0,
                                    headh, headl,
                                    1.00,  0.20,  0.20,  1.00 );

Yaxis       = TGLArrow<GLfloat>   ( 0, 
                                    bsize->YMin() - AtLeast ( arrowtailmarginpastbox, origin.Y ),
                                    0,
                                    0, 
                                    AtLeast ( arrowheadmarginpastbox, bsize->YMax() - origin.Y ) + arrowheadmarginwithinbox, 
                                    0,
                                    headh, headl,
                                    0.20,  1.00,  0.20,  1.00 );

Zaxis       = TGLArrow<GLfloat>   ( 0,
                                    0,
                                    bsize->ZMin() - AtLeast ( arrowtailmarginpastbox, origin.Z ),
                                    0, 
                                    0, 
                                    AtLeast ( arrowheadmarginpastbox, bsize->ZMax() - origin.Z ) + arrowheadmarginwithinbox, 
                                    headh, headl,
                                    0.20,  0.20,  1.00,  1.00 );

OriginRadius    = 0.5;
}


//----------------------------------------------------------------------------
                                        // Default color tables according to data type
void    TVolumeView::SetColorTable ( AtomType datatype )
{
                                        // disable all tables
ColorTable.SetTableAllowed ( NoTables );

                                        // Positive and Vectorial data
if      ( IsAbsolute ( datatype ) ) {

    ColorTable.SetTableAllowed ( AbsColorTable_BlackWhite );
    ColorTable.SetTableAllowed ( AbsColorTable_WhiteBlack );
    ColorTable.SetTableAllowed ( AbsColorTable_BlackYellowWhite );
    ColorTable.SetTableAllowed ( AbsColorTable_BlackRed );
    ColorTable.SetTableAllowed ( AbsColorTable_GrayGreenYellowRed );
    ColorTable.SetTableAllowed ( AbsColorTable_GrayYellow );
    ColorTable.SetTableAllowed ( AbsColorTable_MagentaBlueCyanGrayGreenYellowRed );
    ColorTable.SetTableAllowed ( AbsColorTable_SingleBump );  // trial


//  if ( ColorTableIndex[ datatype ] == UnknownTable ) {    // forgetting about current preset, too?
                                        // special cases
        if ( MRIDoc->IsRoiMask () && MRIDoc->GetMaxValue () > 1
          || MRIDoc->IsBlob () 
          || MRIDoc->IsRis ()                           )

            ColorTableIndex[ datatype ] = AbsColorTable_MagentaBlueCyanGrayGreenYellowRed;
        else
            ColorTableIndex[ datatype ] = AbsColorTable_BlackWhite;
//      }

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

else { // if ( IsScalar ( datatype ) )  // all other cases
                                        // ?Note: purely negative volumes could have their own type of color scale?
    ColorTable.SetTableAllowed ( SignedColorTable_BlueWhiteRed );
    ColorTable.SetTableAllowed ( SignedColorTable_CyanBlackYellow );
    ColorTable.SetTableAllowed ( SignedColorTable_MagentaBlueCyanGrayGreenYellowRed );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = SignedColorTable_BlueWhiteRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }

}


//----------------------------------------------------------------------------
                                        // Default isosurface coloring
void    TVolumeView::SetDefaultSurfaceMode ()
{
SurfaceMode     = MRIDoc->IsRoiMask () 
               || MRIDoc->IsBlob () 
               || MRIDoc->IsRis () 
               || MRIDoc->IsScalar ( AtomTypeUseCurrent );
}


//----------------------------------------------------------------------------
void    TVolumeView::SearchAndSetIntensity ()
{
                                        // set colortable limits
double              maxvalue        = MRIDoc->GetAbsMaxValue ();


SetScalingLimits   ( maxvalue * 1e-3, maxvalue * 10 );


SetScalingContrast ( ToScalingContrast ( 0.40 ) );


if (   MRIDoc->IsMask () 
    || MRIDoc->IsBlob () 
  /*|| MRIDoc->IsSegmented ()*/ 
   )
                                        // exact scaling
    SetScaling ( MRIDoc->GetMinValue (), MRIDoc->GetMaxValue () );

else {
                                        // pushing a bit on the brightness
//    SetScaling ( MRIDoc->GetMinValue () * ( 0.95 + ScalingContrast * -0.05 ),
//                 MRIDoc->GetMaxValue () * ( 0.95 + ScalingContrast * -0.05 ) );

                                        // cut at max, minus the trailing end
    THistogram          H ( *MRIDoc->GetData(), /*MRIDoc->GetData()*/ 0, 10000, 0, 0, 1, (HistogramOptions) ( HistogramCDFOptions | HistogramAbsolute ) );

    double              scalinginit     = maxvalue * 0.95;
    double              cdfmax          = 0.999; // MRIDoc->IsRis () ? 0.999 : 0.995;
                                        // searching backward
    for ( int i = H.GetDim () - 1; i >= 0; i-- )

        if ( H[ i ] <= cdfmax ) {

            scalinginit     = H.ToReal ( RealUnit, i );

            break;
            }

    SetScaling ( -scalinginit, scalinginit );
    }

}


void    TVolumeView::SetupWindow ()
{
TBaseView::SetupWindow ();

if ( CartoolApplication->AnimateViews )
    AnimFx.Start ( TimerStartup, 10, 20 );
}


bool    TVolumeView::ModifyPickingInfo ( TPointFloat& Picking, char *buff )
{
ClearString ( buff );

if ( SliceMode )
    return true;

/*
if ( SliceMode ) {
    TBoundingBox<double>*   b       = MRIDoc->GetBounding();
    int             msx             = MRIDoc->SliceRect( SliceToSliceType[ SliceMode ] ).Width () * SliceMargin;
    int             msy             = MRIDoc->SliceRect( SliceToSliceType[ SliceMode ] ).Height() * SliceMargin;

//    glTranslatef ( ( - sliceperx / 2.0 + 0.5 + slicex ) * msx,
//                   ( - slicepery / 2.0 + 0.5 + slicey ) * msy, 0 );

    if      ( SliceMode == SliceModeCor ) {
        Picking.Y   = ( ( (int) ( Picking.Y + ( 0.5 * msx - 0.5 ) + 0.5 ) ) % msx ) / SliceMargin;
        Picking.Z   = ( ( (int) ( Picking.Z + ( 0.5 * msy - 0.5 ) + 0.5 ) ) % msy ) / SliceMargin;
        }
    else if ( SliceMode == SliceModeTra ) {
        Picking.X   = ( ( (int) ( Picking.X + ( 0.5 * msx - 0.5 ) + 0.5 ) ) % msx ) / SliceMargin;
        Picking.Z   = ( ( (int) ( Picking.Z + ( 0.5 * msy - 0.5 ) + 0.5 ) ) % msy ) / SliceMargin;
        }
    else { // if ( SliceMode == SliceModeSag )
        Picking.X   = ( ( (int) ( Picking.X + ( 0.5 * msx - 0.5 ) + 0.5 ) ) % msx ) / SliceMargin;
        Picking.Y   = ( ( (int) ( Picking.Y + ( 0.5 * msy - 0.5 ) + 0.5 ) ) % msy ) / SliceMargin;
        }

    if      ( Picking.X < b->XMin() )  Picking.X = b->XMin();
    else if ( Picking.X > b->XMax() )  Picking.X = b->XMax();
    if      ( Picking.Y < b->YMin() )  Picking.Y = b->YMin();
    else if ( Picking.Y > b->YMax() )  Picking.Y = b->YMax();
    if      ( Picking.Z < b->ZMin() )  Picking.Z = b->ZMin();
    else if ( Picking.Z > b->ZMax() )  Picking.Z = b->ZMax();

    } // if SliceMode
*/

                                        // MRI title can get quite long...
//StringCopy  ( buff, MRIDoc->GetTitle(), " = ", FloatToString ( MRIDoc->GetData()->GetValueChecked ( Picking.X + MRIDoc->GetOrigin ()[ 0 ], Picking.Y + MRIDoc->GetOrigin ()[ 1 ], Picking.Z + MRIDoc->GetOrigin ()[ 2 ], InterpolateNearestNeighbor ), 2 ) );


//SetOvershootingOption ( interpolate, Array, LinearDim, true );

TPointFloat     pickorg     = Picking + MRIDoc->GetOrigin ();
MriType         value       = MRIDoc->GetData()->GetValueChecked ( pickorg.X, pickorg.Y, pickorg.Z, InterpolateNearestNeighbor );


StringCopy  ( buff, "Value                      = " );

if ( IsInteger ( value ) )  IntegerToString ( StringEnd ( buff ), value );
else                        FloatToString   ( StringEnd ( buff ), value, 2 );

                                        // any label corresponding to the value?
if ( MRIDoc->HasLabels () ) {

    StringAppend    ( buff, NewLine );
    StringAppend    ( buff, "Label                      = ", MRIDoc->GetLabel ( value ) );
    }


return true;
}


bool    TVolumeView::SetDocTitle ( LPCTSTR /*docname*/, int /*index*/ )
{
char                buff[ 1024 ];

StringCopy      ( buff, MRIDoc->GetTitle(), "   Isosurface=", FloatToString ( MRIDoc->GetIsoSurfaceCut() ) );

GetParentO()->SetCaption ( buff );

return false;
}


bool    TVolumeView::IsRenderingMode ( int renderingmode )
{
if      ( renderingmode == RenderingOpaque      )   return RenderingMode == MriSliceOpaque   || RenderingMode == MriOpaque;
else if ( renderingmode == RenderingTransparent )   return RenderingMode == MriSliceOvercast || RenderingMode == MriTransparent;
else                                                return false;
}


//----------------------------------------------------------------------------
void    TVolumeView::DrawMinMax ( TPointFloat& pos, bool colormin, bool showminmaxcircle, double scale )
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
/*                                        // The regular function will use its local volume variable Volume
void    TVolumeView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
GLPaint ( how, renderingmode, otherclipplane, &GLVolume );
}

                                        // Specific function that allows the caller to provide its own & local object, for correct textures rendering
void    TVolumeView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane, TGLVolume *volume )
{
*/

void    TVolumeView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
                                        // might replace this back to Volume
TGLVolume*          volume          = &GLVolume;

                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode


//if ( renderingmode == MriNone )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGLClipPlane*       clipplane;

                                        // take clipping from parameters, if both caller and owner share some clipping
if ( ( (bool) otherclipplane[ 0 ] || (bool) otherclipplane[ 1 ] || (bool) otherclipplane[ 2 ] )
  && ( (bool) ClipPlane     [ 0 ] || (bool) ClipPlane     [ 1 ] || (bool) ClipPlane     [ 2 ] )
  || ( how & GLPaintForceClipping ) )

    clipplane   = otherclipplane;
else                                    // otherwise, use local clipping
    clipplane   = ClipPlane;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Volume&                     vol     = *MRIDoc->GetData ();
const TBoundingBox<double>* b       =  MRIDoc->GetBounding ();
double                      mrisize = b->GetMeanExtent ();


GLfloat             mrimin[ 4 ];        // get MRI box
GLfloat             mrimax[ 4 ];

b->GetMin ( mrimin );
b->GetMax ( mrimax );

mrimin[ 3 ]         = mrimax[ 3 ]   = 1;

MRIDoc->ToAbs ( mrimin );
MRIDoc->ToAbs ( mrimax );

GLfloat             xshift          = - MRIDoc->GetOrigin ()[ 0 ];
GLfloat             yshift          = - MRIDoc->GetOrigin ()[ 1 ];
GLfloat             zshift          = - MRIDoc->GetOrigin ()[ 2 ];


bool                lininterpol     = ! MRIDoc->IsMask ();
int                 clipmode        = 0;
char                buff[ 32 ];

TGLCoordinates<GLfloat> v;
int                 quality;
bool                slicemode       = SliceMode && ( how & GLPaintOwner );
TSliceSelection    *slch            = slicemode ? &Slices[ SliceMode - 1 ] : 0;
int                 slicei;
int                 nviews          = slicemode ? slch->GetTotalSlices() : 1;
//bool                manyviewpoints = Viewpoints.Mode && !SliceMode; // && ( how & GLPaintOwner );
//TOneViewpoint    *viewpoint;
//int                 nviews        = slicemode ? slch->GetTotalSlices() : manyviewpoints ? Viewpoints.NumViewpoints : 1;
int                 slicex;
int                 slicey;

double              minValue;
double              maxValue;
TPointFloat         minPos;
TPointFloat         maxPos;
bool                showminmaxcircle= max ( ShowMin, ShowMax ) == MriMinMaxCircle;  // a bit tricky: allows only for 1 type of Min/max display: circle or cross

double              msx;
double              msy;
GetSliceSize ( MRIDoc, msx, msy );
msx *= SliceMargin;
msy *= SliceMargin;

int                 sliceperx       = slicemode ? SlicesPerX : 1;
int                 slicepery       = slicemode ? SlicesPerY : 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find max position
maxPos      = MRIDoc->GetMaxPosition ();
maxValue    = vol ( maxPos );
MRIDoc->ToAbs ( maxPos );


minPos      = MRIDoc->GetMinPosition ();
minValue    = vol ( minPos );
MRIDoc->ToAbs ( minPos );


GalMaxValue = -DBL_MAX;
GalMinValue =  DBL_MAX;

if ( maxValue > GalMaxValue ) {
    GalMaxValue     = maxValue;
    GalMaxPos       = maxPos;
    }

if ( minValue < GalMinValue ) {
    GalMinValue     = minValue;
    GalMinPos       = minPos;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scanning the clipping planes
TPointFloat         clip (  clipplane[ 0 ].GetAbsPosition (), 
                            clipplane[ 1 ].GetAbsPosition (), 
                            clipplane[ 2 ].GetAbsPosition () );
int                 ncutplanes      = (bool) clipplane[ 0 ] + (bool) clipplane[ 1 ] + (bool) clipplane[ 2 ];
int                 drawi[ 3 ]      = { -1, -1, -1 };
int                 ndraws          = AtLeast ( 1, ncutplanes );

if ( ncutplanes ) {

    int             n               = 0;
    if ( (bool) clipplane[ 0 ] )    drawi[ n++ ] = 0;
    if ( (bool) clipplane[ 1 ] )    drawi[ n++ ] = 1;
    if ( (bool) clipplane[ 2 ] )    drawi[ n   ] = 2;
    }

                                        // re-position clip planes on min or max
if ( FindMinMax && ( how & GLPaintOwner ) && ! ( RButtonDown && ControlKey ) ) {

    if ( IsSignedData () ) {

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

                                        // in editing mode, we prefer to have discrete, more precise steps
GLfloat             cx      = clipplane[ 0 ].GetAbsPosition ( Editing ? 1 : 0 );
GLfloat             cy      = clipplane[ 1 ].GetAbsPosition ( Editing ? 1 : 0 );
GLfloat             cz      = clipplane[ 2 ].GetAbsPosition ( Editing ? 1 : 0 );

                                        // quick way, better is to use StepClipPlane flag with clipplane.GLize
//if ( Editing ) {
//    if ( (bool) clipplane[ 0 ] )    clipplane[ 0 ].SetAbsPosition ( cx );
//    if ( (bool) clipplane[ 1 ] )    clipplane[ 1 ].SetAbsPosition ( cy );
//    if ( (bool) clipplane[ 2 ] )    clipplane[ 2 ].SetAbsPosition ( cz );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGLColor<GLfloat>   glcol;
TGLColor<GLfloat>   glcol0 = ColorTable[ ColorTable.GetZeroIndex () ];
glcol0[ 3 ] = 1;

                                        // save current rotation & translation (from here or outside)
if ( slicemode )
    MatModelView.CopyModelView ();


glFrontFace ( GL_CW );                  // specifies the right orientation


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute scaling factors
if ( ScalingAuto != ScalingAutoOff && ! ( RButtonDown && ! ControlKey && MouseAxis == MouseAxisHorizontal ) ) {

    if ( MRIDoc->IsMask () 
      || MRIDoc->IsBlob () 
      || MRIDoc->IsRis () 
      || MRIDoc->IsSegmented () ) // exact scaling

        SetScaling ( MRIDoc->GetMinValue (),
                     MRIDoc->GetMaxValue (), 
                     ScalingAuto != ScalingAutoAsymmetric );
    else                                          // force a bit on the brightness
//        SetScaling ( MRIDoc->GetMinValue () * ( 0.83 + ScalingContrast * -0.05 ),
//                     MRIDoc->GetMaxValue () * ( 0.83 + ScalingContrast * -0.05 ) );

    SetScaling ( MRIDoc->GetMinValue () * ( 1.00 + ScalingContrast * -0.05 ),
                 MRIDoc->GetMaxValue () * ( 1.00 + ScalingContrast * -0.05 ), 
                 ScalingAuto != ScalingAutoAsymmetric );

                                        // new contrast formula with color table with cut-off slope
//  SetScaling ( MRIDoc->GetMinValue (),
//               MRIDoc->GetMaxValue (), 
//               ScalingAuto != ScalingAutoAsymmetric );
    }

                                        // init limits of ColorTable
//#define           CAPALPHA        0.45

double              contrast        = ToContrast ( ScalingContrast );
                                        // Mask with max value 1 gives a weird alpha distribution (all 0's, 1 final non-null) which make the colortable erratic
                                        // We can simply shift a bit the isosurface cut by half a point, which doesn't harm discrete mask
double              isocut          = MRIDoc->GetIsoSurfaceCut () - ( MRIDoc->IsMask () && MRIDoc->GetIsoSurfaceCut () > 0.5 ? 0.5 : 0 );
                                        // When brightness scaling reaches isocut, everything disappears - this is an ugly fix, but at least it prevents the slice to disappear!
Maxed ( ScalingPMax, isocut * 1.05 );

                                        // Opaque 2D and 3D slices
if      ( SliceMode || renderingmode == MriSliceOpaque ) {

    if ( isocut == 0 )
                                                                    // slightly negative alpha will allow to draw the full slices, background included
        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, -0.5,             0 );

    else if ( MRIDoc->IsMask () )

        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, AlphaGreater,     isocut );

    else

        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, AlphaValueLinear, MRIDoc->GetAbsMaxValue () );
    }

                                        // Transparent 3D slices
else if ( renderingmode == MriSliceOvercast ) {

    if ( MRIDoc->IsBlob () 
      || MRIDoc->IsRis  () )

        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, AlphaSquare );

    else
                                        // AlphaGreater has another problem: when brightness scaling reaches isosurfacecut, everything disappears
        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, AlphaGreater, isocut );
    }
                                        // Other cases
else
//      ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, CAPALPHA );
        ColorTable.SetParameters ( ScalingPMax, ScalingNMax, contrast, AlphaGreater, isocut ); // better when linked into the inverse display


if ( Outputing () )
    quality = SliceMode ? ( b->MaxSize () > 128 || nviews > 12 ? QuadFastestQuality
                                                               : QuadRegularQuality )
                        :                                        QuadBestQuality;
else
    quality = QuadFastestQuality;


if ( MRIDoc->IsMask () )                // override for mask, for all displays
    quality = QuadFastestQuality;

                                        // downgrade linked volume to not use textures (?) - does not behave correctly when embedding a transparent volume
//if ( ( how & GLPaintLinked ) && quality == QuadFastestQuality )
//    quality = QuadRegularQuality;

                                        // loading volume only if needed?
//if ( slicemode  && ( how & GLPaintOpaque )
//  || ncutplanes && renderingmode == MriSliceOpaque && ( how & GLPaintOpaque ) 
//  || CappingMode != MriNoCapping && ncutplanes
//      && ( renderingmode == MriTransparent && ( how & GLPaintTransparent )
//        || renderingmode == MriOpaque      && ( how & GLPaintOpaque      ) )  )

    volume->GLize ( &vol, &ColorTable, lininterpol, MRIDoc->GetOrigin () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop for any pair views
for ( int nv = 0; nv < nviews; nv++ ) {


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
    if ( manyviewpoints ) {
        viewpoint   = &Viewpoints[ nv ];

        glLoadIdentity ();

        glTranslatef ( viewpoint->Center.X, viewpoint->Center.Y, -ModelRadius );

        viewpoint->Orientation.GLize ();

        if ( viewpoint->CanMove )
            ModelRotMatrix.GLize ();

        glTranslated ( -ModelCenter.X, -ModelCenter.Y, -ModelCenter.Z );

        if ( ! viewpoint->Summary )
            GLFogOff            ();
        }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw a series of "2D" slices
    if ( slicemode && ( how & GLPaintOpaque ) ) {

        GLColoringOn        ();
        GLFogOff            ();
        GLAlphaAboveOn      ( MRIDoc->GetBackthresholdAlpha () );


        slicex  = nv % sliceperx;
        slicey  = ( slicepery - 1 ) - nv / sliceperx;

        glLoadIdentity ();

        glTranslatef ( ( - sliceperx / 2.0 + 0.5 + slicex ) * msx,
                       ( - slicepery / 2.0 + 0.5 + slicey ) * msy, 0 );

                                        // summary slice? -> fixed view
        if ( nv == nviews - 1 )
            SlicesSummaryMatrix[ SliceMode - 1 ].GLize ();
        else                            // restore & apply original transform
            MatModelView.GLize ();


                                        // select slice
        slicei = slch->GetSelection ()->GetValue ( nv );

                                        // regular 3D view, with the slice drawn on top
        if ( slch->IsSummarySlice ( slicei ) ) {

            if ( IsSliceModeX () )

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( minPos.X,   minPos.X,   mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( maxPos.X,   maxPos.X,   mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( si() + xshift, si() + xshift, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 1 ); }
                    }
            else if ( IsSliceModeY () )

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, minPos.Y,   minPos.Y,   mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, maxPos.Y,   maxPos.Y,   mrimin[ 2 ] - 1, mrimax[ 2 ] + 1, 3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], si() + yshift, si() + yshift, mrimin[ 2 ], mrimax[ 2 ], 1 ); }
                    }
            else

                for ( TIteratorSelectedForward si ( *slch->GetSelection () ); (bool) si; ++si ) {

                    if      ( slch->IsSummarySlice ( si() ) )   break;
                    else if ( slch->IsMinSlice     ( si() ) )   { glColor4f ( 0.00, 0.00, 1.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, minPos.Z,   minPos.Z,   3 ); }
                    else if ( slch->IsMaxSlice     ( si() ) )   { glColor4f ( 1.00, 0.00, 0.00, 1.00 ); Prim.Draw3DWireFrame ( mrimin[ 0 ] - 1, mrimax[ 0 ] + 1, mrimin[ 1 ] - 1, mrimax[ 1 ] + 1, maxPos.Z,   maxPos.Z,   3 ); }
                    else                                        { glColor4f ( SliceColor );             Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], si() + zshift, si() + zshift, 1 ); }
                    }

            } // summary slice
                                        // regular slices
        else {
            int                 s;

                                        // draw the correct plane according to orientation
            if      ( IsSliceModeX () ) {

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
                    s = slicei + xshift;

                volume->DrawPlaneX ( s, quality );

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
                    s = slicei + yshift;

                volume->DrawPlaneY ( s, quality );

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
                    s = slicei + zshift;

                volume->DrawPlaneZ ( s, quality );

                if ( ShowMin && (int) minPos.Z == s )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
                if ( ShowMax && (int) maxPos.Z == s )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );
                }


            continue;                   // to the big loop!
            } // slices


        GLColoringOff       ();
        GLFogOn             ();
        GLAlphaAboveOff     ();
        } // slicemode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw MRI as 3D slices
    if ( ncutplanes && renderingmode == MriSliceOpaque && ( how & GLPaintOpaque ) ) {

        GLColoringOn        ();
        GLAlphaAboveOn      ( MRIDoc->GetBackthresholdAlpha () );

        if ( how & GLPaintLinked )
            GLFogOff        ();


//      if ( (bool) clipplane[0] && ( !manyviewpoints || viewpoint->SliceToShow == 0 || viewpoint->Summary ) )

        if ( (bool) clipplane[0] )  volume->DrawPlaneX ( cx, quality );
        if ( (bool) clipplane[1] )  volume->DrawPlaneY ( cy, quality );
        if ( (bool) clipplane[2] )  volume->DrawPlaneZ ( cz, quality );


        GLColoringOff       ();
        GLAlphaAboveOff     ();

        if ( how & GLPaintLinked )
            GLFogOn         ();
        } // MRI slices


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Opaque or transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


    if ( slicemode )                    // the scaling in the summary matrix also affects the normals
        GLNormalizeNormalOn ();


//  if ( renderingmode == MriOpaque && ! ( (bool) clipplane[ 0 ] || (bool) clipplane[ 1 ] || (bool) clipplane[ 2 ] ) )
//      glEnable ( GL_CULL_FACE );      // speed up a bit everything - well, supposedly, but it doesn't really show up


    const TTriangleSurface* tsurfpos    = MRIDoc->GetTessSurfacePos ();
    const TTriangleSurface* tsurfneg    = MRIDoc->GetTessSurfaceNeg ();

                                        // testing which isosurface to draw, as type can vary in real-time
    int                 maxtrihullpos   = MRIDoc->GetMaxValue () > 0 && tsurfpos ? tsurfpos->GetNumTriangles () : 0;
    int                 maxtrihullneg   = MRIDoc->GetMinValue () < 0 && tsurfneg ? tsurfneg->GetNumTriangles () : 0;
    int                 stencilfilled   = -1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the isosurface
    if ( renderingmode == MriTransparent && ( how & GLPaintTransparent )
      || renderingmode == MriOpaque      && ( how & GLPaintOpaque ) ) {

                                        // set surface properties
        MaterialOut.SetDiffuse ( MaterialOutDiffuse );
//      MaterialIn. SetDiffuse ( MaterialInDiffuse  );
        MaterialTr. SetDiffuse ( MaterialTrDiffuse  );


        if ( SurfaceMode && MRIDoc->IsRoiMask () ) {
                                        // send the texture
            SurfaceVolume.GLize ( &SurfaceData, &ColorTable, false, MRIDoc->GetOrigin () );
                                        // use 3D implicit texture, generating the texture coordinates automatically
            GL3DAutoTextureOn   ( MRIDoc->GetOrigin () );

            MaterialOut.SetAmbient ( 0.85, 0.85, 0.85, 1.00 );
            MaterialOut.SetDiffuse ( 0.60, 0.60, 0.60, 1.00 );

            MaterialTr .SetAmbient ( 0.85, 0.85, 0.85, MRIALPHA );
            MaterialTr. SetDiffuse ( 0.40, 0.40, 0.40, MRIALPHA );
            }


        if ( renderingmode == MriTransparent ) {
            GLTransparencyOn();
            GLMaterialOn    ();
            GLFogOff        ();

/*          if ( VkQuery () )
                glBlendFunc ( GL_SRC_ALPHA, GL_ONE );   // effect is good, quite shiny, BUT it does not render on white background!!
            else
                glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );*/

    //      glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

            MaterialTr.GLize ();
            }
        else { // MriOpaque
            GLMaterialOn    ();

            MaterialOut.GLize ();
//          MaterialIn. GLize ();
            }

                                        // take the most benefit of the first pass
        if ( CappingMode != MriNoCapping && ncutplanes == 1 ) {
            glClearStencil  ( 0 );
            glStencilMask   ( 1 );
            glEnable        ( GL_STENCIL_TEST );
            glClear         ( GL_STENCIL_BUFFER_BIT );
            glStencilFunc   ( GL_ALWAYS, 0, 1 );
            glStencilOp     ( GL_KEEP, GL_INVERT, GL_INVERT );

            stencilfilled   = drawi[0];
            }

                                        // add a global shift
        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

                                        // draw isosurface
        glEnableClientState ( GL_VERTEX_ARRAY );
        glEnableClientState ( GL_NORMAL_ARRAY );


        for ( int n = 0; n < ndraws; n++ ) {

            if ( ncutplanes ) {
                                        // must change clipping on each iteration (OR)
                if      ( n == 0 )  clipplane[ drawi[ 0 ] ].GLize ( clipmode );
                else if ( n == 1 )  clipplane[ drawi[ 0 ] ].GLize ( clipmode | InvertedClipPlane );

                if      ( n == 1 )  clipplane[ drawi[ 1 ] ].GLize ( clipmode );
                else if ( n == 2 )  clipplane[ drawi[ 1 ] ].GLize ( clipmode | InvertedClipPlane );

                if      ( n == 2 )  clipplane[ drawi[ 2 ] ].GLize ( clipmode );
                }


            if ( maxtrihullpos ) {

                if ( SurfaceMode && ! MRIDoc->IsRoiMask () ) {
                                                // override with color of value cut
                    if ( MRIDoc->IsBinaryMask () )  ColorTable.GetColorIndex ( MRIDoc->GetMaxValue (),     glcol );
                    else                            ColorTable.GetColorIndex ( MRIDoc->GetIsoSurfaceCut(), glcol );

                    glcol[ 3 ]  = (GLfloat) MRIALPHA;

                    MaterialOut.SetDiffuse ( glcol[ 0 ], glcol[ 1 ], glcol[ 2 ], glcol[ 3 ] );
                    MaterialTr. SetDiffuse ( glcol[ 0 ], glcol[ 1 ], glcol[ 2 ], glcol[ 3 ] );

                    if ( renderingmode == MriOpaque )   MaterialOut.GLize ();
                    else                                MaterialTr .GLize ();
                    }

                glVertexPointer ( 3, GL_FLOAT, 0, tsurfpos->GetListTriangles     () );
                glNormalPointer (    GL_FLOAT, 0, tsurfpos->GetListPointsNormals () );
                glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullpos );
                }


            if ( maxtrihullneg ) {

                if ( SurfaceMode && ! MRIDoc->IsRoiMask () ) {
                                                // override with color of value cut
                    if ( MRIDoc->IsBinaryMask () )  ColorTable.GetColorIndex (  MRIDoc->GetMinValue (),     glcol );
                    else                            ColorTable.GetColorIndex ( -MRIDoc->GetIsoSurfaceCut(), glcol );

                    glcol[ 3 ]  = (GLfloat) MRIALPHA;

                    MaterialOut.SetDiffuse ( glcol[ 0 ], glcol[ 1 ], glcol[ 2 ], glcol[ 3 ] );
                    MaterialTr. SetDiffuse ( glcol[ 0 ], glcol[ 1 ], glcol[ 2 ], glcol[ 3 ] );

                    if ( renderingmode == MriOpaque )   MaterialOut.GLize ();
                    else                                MaterialTr .GLize ();
                    }


                glVertexPointer ( 3, GL_FLOAT, 0, tsurfneg->GetListTriangles     () );
                glNormalPointer (    GL_FLOAT, 0, tsurfneg->GetListPointsNormals () );
                glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullneg );
                }

                                        // stop writing to the stencil
            if ( CappingMode != MriNoCapping && ncutplanes == 1 && ! n )
                glDisable ( GL_STENCIL_TEST );
            } // for ndraws

                                        // 3D implicit texture off
        if ( SurfaceMode && MRIDoc->IsRoiMask () ) {

            GL3DAutoTextureOff  ();

            MaterialOut.SetAmbient ( MaterialOutAmbient );
            MaterialTr .SetAmbient ( MaterialTrAmbient  );
            }


        if ( renderingmode == MriTransparent ) {
            GLTransparencyOff();
            GLFogOn         ();
            }


        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }


        glVertexPointer         ( 3, GL_FLOAT, 0, 0 );
        glNormalPointer         (    GL_FLOAT, 0, 0 );

        glDisableClientState    ( GL_VERTEX_ARRAY );
        glDisableClientState    ( GL_NORMAL_ARRAY );


        if ( (bool) clipplane[0] ) clipplane[0].unGLize();
        if ( (bool) clipplane[1] ) clipplane[1].unGLize();
        if ( (bool) clipplane[2] ) clipplane[2].unGLize();
        } // draw isosurface


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  glDisable ( GL_CULL_FACE );


    if ( slicemode )
        GLNormalizeNormalOff ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init the capping
                                        // all triangles are accepted and invert 1 bit
    if ( CappingMode != MriNoCapping && ncutplanes

      && ( renderingmode == MriTransparent && ( how & GLPaintTransparent )
        || renderingmode == MriOpaque      && ( how & GLPaintOpaque )      ) ) {


        if ( CappingMode == MriBrainCapping ) {
            GLColoringOn        ();
            GLFogOff            ();
            }


        if ( renderingmode == MriTransparent ) {
            GLTransparencyOn    ();
            GLFogOff            ();
            }

                                        // add a global shift
        if ( DepthRange.UserShift ) {
            DepthRange.ShiftCloser ( DepthRange.UserShift );
            DepthRange.GLize ();
            }


        glClearStencil ( 0 );
        glStencilMask ( 1 );
        glEnable ( GL_STENCIL_TEST );

        glEnableClientState ( GL_VERTEX_ARRAY );


        auto    StencilBefore = [] {
                glClear         ( GL_STENCIL_BUFFER_BIT );
                glStencilFunc   ( GL_ALWAYS, 0, 1 );
                glStencilOp     ( GL_KEEP, GL_INVERT, GL_INVERT );
                glColorMask     ( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
                glDepthMask     ( GL_FALSE );
                glDisable       ( GL_LIGHT0 );
                glDisable       ( GL_LIGHT1 );
                glShadeModel    ( GL_FLAT );
                };

        auto    StencilAfter = [ &renderingmode ] {
                glColorMask     ( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
                glEnable        ( GL_LIGHT0 );
                glEnable        ( GL_LIGHT1 );
                glShadeModel    ( GL_SMOOTH );
                if ( renderingmode == MriOpaque )
                    glDepthMask ( GL_TRUE );
                };


        if ( (bool) clipplane[0] ) {
                                        // need to fill the stencil?
            if ( stencilfilled != 0 ) {

                StencilBefore ();

                clipplane[0].GLize ( clipmode );

                if ( maxtrihullpos ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfpos->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullpos );
                    }

                if ( maxtrihullneg ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfneg->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullneg );
                    }

                StencilAfter ();
                }
                                        // now use the stencil
            glStencilFunc ( GL_NOTEQUAL, 0, 1 );
    //      glStencilOp ( GL_KEEP, GL_KEEP, GL_KEEP );

                                        // clip
            clipplane[0].unGLize();

            if ( (bool) clipplane[1] )  clipplane[1].GLize ( clipmode | InvertedClipPlane );
            if ( (bool) clipplane[2] )  clipplane[2].GLize ( clipmode | InvertedClipPlane );


                                        // paint
            glFrontFace ( clipplane[0].IsForward() ? GL_CW : GL_CCW );
            glNormal3f  ( clipplane[0].IsForward() ? -1 : 1, 0, 0 );

            volume->DrawPlaneX ( cx, quality, true );

            if ( (bool) clipplane[1] )  clipplane[1].unGLize();
            if ( (bool) clipplane[2] )  clipplane[2].unGLize();
            } // clip0


        if ( (bool) clipplane[1] ) {
                                        // need to fill the stencil?
            if ( stencilfilled != 1 ) {

                StencilBefore ();

                clipplane[1].GLize( clipmode );

                if ( maxtrihullpos ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfpos->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullpos );
                    }

                if ( maxtrihullneg ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfneg->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullneg );
                    }

                StencilAfter ();
                }
                                        // now use the stencil
            glStencilFunc ( GL_NOTEQUAL, 0, 1 );

                                        // clip
            clipplane[1].unGLize();

            if ( (bool) clipplane[0] )  clipplane[0].GLize ( clipmode | InvertedClipPlane );
            if ( (bool) clipplane[2] )  clipplane[2].GLize ( clipmode | InvertedClipPlane );

                                        // paint
            glFrontFace ( clipplane[1].IsForward() ? GL_CCW : GL_CW );
            glNormal3f  ( 0, clipplane[1].IsForward() ? -1 : 1, 0 );

            volume->DrawPlaneY ( cy, quality, true );

            if ( (bool) clipplane[0] )  clipplane[0].unGLize();
            if ( (bool) clipplane[2] )  clipplane[2].unGLize();
            } // clip1


        if ( (bool) clipplane[2] ) {
                                        // need to fill the stencil?
            if ( stencilfilled != 2 ) {

                StencilBefore ();

                clipplane[2].GLize( clipmode );

                if ( maxtrihullpos ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfpos->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullpos );
                    }

                if ( maxtrihullneg ) {
                    glVertexPointer ( 3, GL_FLOAT, 0, tsurfneg->GetListTriangles     () );
                    glDrawArrays    ( GL_TRIANGLES, 0, 3 * maxtrihullneg );
                    }

                StencilAfter ();
                }
                                        // now use the stencil
            glStencilFunc ( GL_NOTEQUAL, 0, 1 );

                                        // clip
            clipplane[2].unGLize();

            if ( (bool) clipplane[0] )  clipplane[0].GLize ( clipmode | InvertedClipPlane );
            if ( (bool) clipplane[1] )  clipplane[1].GLize ( clipmode | InvertedClipPlane );


            glFrontFace ( clipplane[2].IsForward() ? GL_CW : GL_CCW );
            glNormal3f  ( 0, 0, clipplane[2].IsForward() ? -1 : 1 );

            volume->DrawPlaneZ ( cz, quality, true );

            if ( (bool) clipplane[0] )  clipplane[0].unGLize();
            if ( (bool) clipplane[1] )  clipplane[1].unGLize();
            } // clip2


        glFrontFace ( GL_CW );

        glDisable ( GL_STENCIL_TEST );

        glVertexPointer         ( 3, GL_FLOAT, 0, 0 );

        glDisableClientState    ( GL_VERTEX_ARRAY );


        if ( DepthRange.UserShift ) {
            DepthRange.ShiftFurther ( DepthRange.UserShift );
            DepthRange.GLize ();
            }

                                        // restore
        if ( renderingmode == MriTransparent ) {
            GLTransparencyOff   ();
            GLFogOn             ();
            }


        if ( CappingMode == MriBrainCapping ) {
            GLColoringOff       ();
            GLFogOn             ();
            }
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // outline clipping planes
    if ( ShiftAxis != ShiftNone && ( how & GLPaintOpaque ) && ( how & GLPaintOwner ) ) {

        GLLinesModeOn       ();


        if ( ShiftAxis == ShiftX ) {
            glColor4f ( 1.00, 0.00, 0.00, 1.00 );

            sprintf ( buff, "X = %g", cx );
            SFont->Print ( cx, mrimax[ 1 ], mrimax[ 2 ], buff, TA_CENTER | TA_TOP | TA_BOX, 0, 0, 0.025 );

            Prim.Draw3DWireFrame ( cx, cx, mrimin[ 1 ], mrimax[ 1 ], mrimin[ 2 ], mrimax[ 2 ], 3 );
            }

        if ( ShiftAxis == ShiftY ) {
            glColor4f ( 0.00, 1.00, 0.00, 1.00 );

            sprintf ( buff, "Y = %g", cy );
            SFont->Print ( mrimax[ 0 ], cy, mrimax[ 2 ], buff, TA_CENTER | TA_TOP | TA_BOX, 0, 0, 0.025 );

            Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], cy, cy, mrimin[ 2 ], mrimax[ 2 ], 3 );
            }

        if ( ShiftAxis == ShiftZ ) {
            glColor4f ( 0.00, 0.00, 1.00, 1.00 );

            sprintf ( buff, "Z = %g", cz );
            SFont->Print ( mrimax[ 0 ], mrimax[ 1 ], cz, buff, TA_CENTER | TA_TOP | TA_BOX, 0, 0, 0.025 );

            Prim.Draw3DWireFrame ( mrimin[ 0 ], mrimax[ 0 ], mrimin[ 1 ], mrimax[ 1 ], cz, cz, 3 );
            }


        GLLinesModeOff      ( false );
//      GLWriteDepthOff     ();     // ?
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // outline data box
    if ( ( ShowSizeBox || ShowBoundingBox ) && ( how & GLPaintOpaque ) ) {

        GLLinesModeOn       ();


        if ( ShowSizeBox ) {
            glColor4f ( 0.50, 0.50, 0.50, 0.50 );

            Prim.Draw3DBox ( - MRIDoc->GetOrigin ()[ 0 ], MRIDoc->GetSize()->XMax () - MRIDoc->GetOrigin ()[ 0 ], 
                             - MRIDoc->GetOrigin ()[ 1 ], MRIDoc->GetSize()->YMax () - MRIDoc->GetOrigin ()[ 1 ], 
                             - MRIDoc->GetOrigin ()[ 2 ], MRIDoc->GetSize()->ZMax () - MRIDoc->GetOrigin ()[ 2 ],     1 );
            }

                                        // data content
        if ( ShowBoundingBox ) {
            glColor4f ( 0.75, 0.75, 0.75, 0.50 );

            Prim.Draw3DBox ( mrimin[ 0 ], mrimax[ 0 ],
                             mrimin[ 1 ], mrimax[ 1 ],
                             mrimin[ 2 ], mrimax[ 2 ], 1 );


            GLLinesModeOff      ();
            }
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temp code for markers
    if ( (bool) Markers && ! slicemode && ( how & GLPaintOpaque ) ) {

        GLMaterialOn        ();


        TGLBillboardSphere     *tobbsphere  = Outputing() ? &BbHighSphere : &BbLowSphere;
        double                  radius      = b->MaxSize () * 0.01;
        char                    name[ 32 ];


        tobbsphere->GLize ();

        MaterialTr.GLize ();

        TextColor.GLize ( how & GLPaintOwner ? 0 : 2 );


        for ( int mi = 0; mi < (int) Markers; mi++ ) {

            TVector3Float       p   = Markers[ mi ];
                                        // draw marker
            tobbsphere->GLize ( p.X, p.Y, p.Z, radius );
                                        // print its name
            IntegerToString ( name, mi + 1 );

            TVector3Float       r   = p - ModelCenter;
            r.Normalize ();

            p  += r * 3 * radius;

            SFont->Print ( p.X, p.Y, p.Z, name, TA_CENTER | TA_CENTERY | TA_BOX );
            }


        tobbsphere->unGLize ();
        } // markers


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // outline editing tool
    if ( Editing && Picking.IsNotNull () && ( how & GLPaintTransparent ) ) {

        GLLinesModeOn       ();
        GLMaterialOn        ();
                                                // transparent, proportional sphere
        MaterialTr.GLize ();

        TGLColor<GLfloat>   editcol ( EditingColorTool,   Editing == EditingToolSphereSurface       ? (GLfloat) 0.25
                                                        : Editing == EditingToolSphereCenterBlurr   ? (GLfloat) 0.20
                                                        :                                             (GLfloat) 0.40 );

        glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, editcol );

        
        double          radius      = AtLeast ( 0.5, EditingRadius );
        double          depth       = AtLeast ( 0.5, EditingDepth  );
        TPointFloat     normal;

                                        // Many tools need a gradient
        if ( ToolNeedsGradient ( Editing ) ) {

            TPointFloat         pickingrel ( Picking );

            MRIDoc->ToRel ( pickingrel );

            int                 gdist   = AtLeast ( Editing == EditingToolPlane || Editing == EditingToolSphereSurface ? EditingGradientMinSizeBig : EditingGradientMinSizeSmall,
                                                    Round ( Editing == EditingToolCylinder ? ( radius + depth ) / 2 * EditingCylinderRadiusRatio : radius )                       );

            vol.GetGradientSmoothed (   pickingrel.X, pickingrel.Y, pickingrel.Z, EditingGradient, gdist );

            EditingGradient.Normalize ();
                                        // make it point outward
            EditingGradient.Invert ();
            }


        EditingPos1     = EditingPos2   = Picking;


        if      ( Editing == EditingToolSphereCenter
               || Editing == EditingToolSphereCenterBlurr ) {

            TGLBillboardSphere     *tobbsphere  = &BbHighSphere;

            tobbsphere->GLize ( BillboardDoubleSide );

            tobbsphere->GLize ( EditingPos1.X, EditingPos1.Y, EditingPos1.Z, radius );

            if ( Editing == EditingToolSphereCenterBlurr ) {
                tobbsphere->GLize ( EditingPos1.X, EditingPos1.Y, EditingPos1.Z, radius * 0.833 );
                tobbsphere->GLize ( EditingPos1.X, EditingPos1.Y, EditingPos1.Z, radius * 0.666 );
                }

            tobbsphere->unGLize ();
            }


        else if ( Editing == EditingToolSphereSurface && EditingGradient.IsNotNull () ) {
                                        // full radius + a slight offset outward
            EditingPos1    += EditingGradient * ( radius + EditingPlaneOffset );

            TGLBillboardSphere     *tobbsphere  = &BbHighSphere;

            tobbsphere->GLize ( BillboardDoubleSide );

            tobbsphere->GLize ( EditingPos1.X, EditingPos1.Y, EditingPos1.Z, radius );

            tobbsphere->unGLize ();

            normal      = EditingGradient * ( 2 * radius );
            }


        else if ( Editing == EditingToolCylinder && EditingGradient.IsNotNull () ) {

            normal          = EditingGradient * depth;

            EditingPos1    += normal;   // outward
            EditingPos2    -= normal;   // inward

            Prim.Draw3DCylinder ( EditingPos1, EditingPos2, radius * EditingCylinderRadiusRatio );
            }


        else if ( Editing == EditingToolPlane && EditingGradient.IsNotNull () ) {
                                        // a slight offset outward
            EditingPos1    += EditingGradient * EditingPlaneOffset;
                                        // preview plane
            Prim.Draw3DPlane ( EditingPos1, EditingGradient, b->MaxSize () );
                                                    // also showing an orthogonal vector
            normal      = EditingGradient * ( b->MaxSize () * 0.20 );
            }


        if ( ToolNeedsGradient ( Editing ) ) {

            TGLColor<GLfloat>   editcol ( EditingColorGradient, (GLfloat) 0.50 );

            glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, editcol );

                                        // all those with a gradient, show it as a arrow
            double      scale   = Editing == EditingToolCylinder ? sqrt ( Square ( depth ) + Square ( radius ) ) : normal.Norm ();

            Prim.Draw3DArrow ( Picking, normal, 
//                             Clip ( scale * 0.005, 0.30, 1.20 ) );
                               Clip ( scale * 0.005, 0.35, 1.20 ), AtLeast ( 1.0, scale * 0.20 ) );
            }

        else {
                                        // all the others, add a centered, fixed cross
            glColor4f ( EditingColorGradient, 1.00 );

            DepthRange.ShiftCloser ();
            DepthRange.GLize ();

            Prim.Draw3DCross ( Picking, 2, 2, true );

            DepthRange.ShiftFurther ();
            DepthRange.GLize ();
            }


        GLLinesModeOff      ();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transparent
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Show Min/Max in 3D
                                // not needed for the summary slice, in SliceMode?
    if ( ( ShowMin || ShowMax ) && ! SliceMode && ( how & GLPaintOpaque ) ) {

        GLFogOff        ();

        if ( ShowMin )  DrawMinMax  ( minPos, true,  showminmaxcircle, mrisize );
        if ( ShowMax )  DrawMinMax  ( maxPos, false, showminmaxcircle, mrisize );

//      GLFogOn         (); // ?
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw MRI as 3D slices
    if ( ncutplanes && renderingmode == MriSliceOvercast && ( how & GLPaintTransparent ) ) {

        GLLinesModeOn       ( /*how & GLPaintLinked*/ );

        MaterialTr.GLize ();

        if ( (bool) clipplane[0] )  volume->DrawPlaneX ( cx, quality );
        if ( (bool) clipplane[1] )  volume->DrawPlaneY ( cy, quality );
        if ( (bool) clipplane[2] )  volume->DrawPlaneZ ( cz, quality );

        GLLinesModeOff      ( /*how & GLPaintLinked*/ );
        } // MRI slices


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    GLFogOn         ();
    } // for nview

}


//----------------------------------------------------------------------------
void    TVolumeView::Paint ( TDC &dc, bool /*erase*/, TRect &rect )
{

if ( SliceMode ) {
    double              msx,    msy;
    GetSliceSize ( MRIDoc, msx, msy );

    msx *= SliceMargin;
    msy *= SliceMargin;

    PrePaint ( dc, rect, SlicesPerX * msx / 2 * ExtraSizeMRISliceMode,
                         SlicesPerY * msy / 2 * ExtraSizeMRISliceMode,
                         ModelRadius );
    }
else {
    double              radius  = MRIDoc->GetBounding ()->MaxSize () / 2 * ExtraSize3D;

//  radius  = MRIDoc->GetSize ()->MaxSize () / 2 * ExtraSize3D * 0.90;  // fixed scale

    PrePaint ( dc, rect, radius, radius, ModelRadius );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this matrix for the min/max display
if ( max ( ShowMin, ShowMax ) == MriMinMaxCircle ) {
    AntiModelRotMatrix  = ModelRotMatrix;
    AntiModelRotMatrix.Invert ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );

//glShadeModel ( GL_SMOOTH );
//glShadeModel ( GL_FLAT );
//glShadeModel ( !( LButtonDown || MButtonDown || RButtonDown ) || Outputing() ? GL_SMOOTH : GL_FLAT );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
if ( clipbox.Width() == PaintRect.Width() && clipbox.Height() == PaintRect.Height() ) {
    glDisable ( GL_SCISSOR_TEST );
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
else {
    glScissor ( clipbox.Left(), rect.Top() - clipbox.Top(), clipbox.Width(), clipbox.Height() );
    glEnable ( GL_SCISSOR_TEST );
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fixed lights
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
if ( /*! Outputing() &&*/ ! SliceMode && ShowAxis /*&& ( ! Viewpoints || Viewpoints[ Viewpoints.NumViewpoints - 1 ].Summary )*/ )
    DrawAxis ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Fog.unGLize();

                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the scaling of the colors & min/max
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
    if ( ShowOrientation /*&& ! Viewpoints*/ )
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
        TVector3Double      voxsize ( MRIDoc->GetVoxelSize () );


                                        // Data type
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetContentTypeName ( buff ), TA_LEFT | TA_TOP );

        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, BaseDoc->GetAtomTypeName ( AtomTypeUseCurrent ), TA_LEFT | TA_TOP );

                                        // Size
        sprintf ( buff, "Volume size:  %0d  %0d  %0d [Voxel]", MRIDoc->GetSize ()->GetXExtent (), MRIDoc->GetSize ()->GetYExtent (), MRIDoc->GetSize ()->GetZExtent () );
        ypos   -= 1.5 * ydelta;
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

        sprintf ( buff, "Content size:  %.1lf  %.1lf  %.1lf [Voxel]", MRIDoc->GetBounding ()->GetXExtent (), MRIDoc->GetBounding ()->GetYExtent (), MRIDoc->GetBounding ()->GetZExtent () );
        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

        sprintf ( buff, "Origin:  %.2f  %.2f  %.2f [Voxel]", MRIDoc->GetOrigin ()[ 0 ], MRIDoc->GetOrigin ()[ 1 ], MRIDoc->GetOrigin ()[ 2 ] );
        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );

        sprintf ( buff, "Voxel size:  %.3f  %.3f  %.3f [mm]", voxsize[ 0 ], voxsize[ 1 ], voxsize[ 2 ] );
        ypos   -= ydelta;
        SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );



        if ( ShowMax ) {
            glColor4f ( 1.00, 0.00, 0.00, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Max:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Value = %0.3g", GalMaxValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%4d      , %4d      , %4d       [Voxel]", (int) GalMaxPos.X, (int) GalMaxPos.Y, (int) GalMaxPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMaxPos.X * voxsize[ 0 ], GalMaxPos.Y * voxsize[ 1 ], GalMaxPos.Z * voxsize[ 2 ] );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


                                        // 3.2) If no depth-shift trick -> transform & print
            if ( GetGeometryTransform () && (bool) MRIDoc->GetDisplaySpaces () ) {

                TPointFloat             p ( GalMaxPos );

                                        // forward the bounding, to help guess any downsampling
                GetGeometryTransform ()->ConvertTo ( p, & MRIDoc->GetDisplaySpaces ()[ 0 ].Bounding );


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
            } // ShowMax


        if ( IsSignedData () && ShowMin ) {
            glColor4f ( 0.00, 0.00, 1.00, 1.00 );
            ypos   -= 1.5 * ydelta;
            SFont->Print ( xpos, ypos, zpos, "Min:", TA_LEFT | TA_TOP );
            TextColor.GLize ( Outputing() ? 1 : 0 );


            sprintf ( buff, "Value = %0.3g", GalMinValue );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%4d      , %4d      , %4d       [Voxel]", (int) GalMinPos.X, (int) GalMinPos.Y, (int) GalMinPos.Z );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


            sprintf ( buff, "%7.2f, %7.2f, %7.2f [mm]", GalMinPos.X * voxsize[ 0 ], GalMinPos.Y * voxsize[ 1 ], GalMinPos.Z * voxsize[ 2 ] );
            ypos   -= ydelta;
            SFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP );


                                        // 3.2) If no depth-shift trick -> transform & print
            if ( GetGeometryTransform () && (bool) MRIDoc->GetDisplaySpaces () ) {

                TPointFloat             p ( GalMinPos );

                                        // forward the bounding, to help guess any downsampling
                GetGeometryTransform ()->ConvertTo ( p, & MRIDoc->GetDisplaySpaces ()[ 0 ].Bounding );


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

    if ( Editing ) {

        char                buff [ 256 ];
        double              xpos            = PaintRect.Width () * 0.25;
        double              ypos            = PrintBottomPos + 6 * PrintVerticalStep ( BFont );
        double              zpos            = PrintDepthPosBack;

        glColor4f ( EditingColorTool, 1.00 );
        BFont->SetBoxColor ( 1.00, 1.00, 0.00, 1.00 );

        StringCopy ( buff, "Editing mode     : ", EditingToolsString[ Editing ] );

        BFont->Print ( xpos, ypos, zpos, buff, TA_LEFT | TA_TOP | TA_BOX | TA_TOFRONT );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // bonus display
#if defined (_DEBUG)

    if ( VkQuery () ) {
        char        buff[ 256 ];

        sprintf ( buff, "Texture 3D: %s,  Palette Texture 3D: %s,  Texture 3D Loaded  : %s", BoolToString ( GLVolume.Texture[ 0 ].IsTexture3DEnable () ), BoolToString ( GLVolume.Texture[ 0 ].IsPalettedTextureEnable () ), BoolToString ( GLVolume.Texture[ 0 ].IsTextureLoaded () ) );
        SFont->Print ( PrintLeftPos, PrintBottomPos + 1.5 * PrintVerticalStep ( SFont ), PrintDepthPosBack, buff,  TA_LEFT | TA_BOTTOM );
        }
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ResetWindowCoordinates ();
    } // window mode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );
}


//----------------------------------------------------------------------------
void    TVolumeView::HintsPaint ()
{
                                        // draw additional user interaction hints
if ( LButtonDown || MButtonDown || RButtonDown ) {
//   && ( abs ( LastMouseMove.X() ) + abs ( LastMouseMove.Y() ) ) > 10 ) {


    SetWindowCoordinates ();
    GLBlendOn           ();

    glColor4f ( GLBASE_CURSORHINTCOLOR );
    BFont->SetBoxColor ( GLBASE_CURSORHINTBACKCOLOR );

//  glTranslated ( PaintRect.Width() / 2, PaintRect.Height() / 2, 0 );
    glTranslated ( MousePos.X(), PaintRect.Height() - MousePos.Y(), 0 );
    glScaled ( CursorHintSize, CursorHintSize, 1 );


    if ( RButtonDown && ! ControlKey &&  MouseAxis == MouseAxisVertical && IsoMode != MriIsoFixed ) {

        if ( LastMouseMove.Y() > 0 ) {  // expand isosurface
            Prim.DrawCircle ( 0, 0, 0, 1.10, 1.50, 0, 360 );
            Prim.DrawCircle ( 0, 0, 0, 0.70, 0.90, 0, 360 );
            Prim.DrawCircle ( 0, 0, 0, 0.40, 0.50, 0, 360 );
            }
        else {                      // shrink isosurface
            Prim.DrawCircle ( 0, 0, 0, 1.40, 1.50, 0, 360 );
            Prim.DrawCircle ( 0, 0, 0, 1.00, 1.20, 0, 360 );
            Prim.DrawCircle ( 0, 0, 0, 0.40, 0.80, 0, 360 );
            }

        char                buff[ 32 ];
        sprintf ( buff, "Isosurface %g", MRIDoc->GetIsoSurfaceCut () );
        BFont->Print ( 2, 0, 1, buff, TA_LEFT | TA_CENTERY | TA_BOX );
        }

                                        // 2D slices give an erroneous Talairach
    else if ( MButtonDown && SliceMode ) {

        if ( Picking.IsNotNull () )
            BFont->Print ( 0, -1, 1, "No coordinates available in slice mode.", TA_CENTER | TA_TOP | TA_BOX );
        }


    else if ( CurrentDisplaySpace != DisplaySpaceNone ) {

        GLBlendOff          ();
        ResetWindowCoordinates ();

        TBaseView::HintsPaint ();

        return;
        } // 3D stuff


    GLBlendOff          ();
    ResetWindowCoordinates ();          // this will also restore a clean ModelView
    }
}


//----------------------------------------------------------------------------
void    TVolumeView::EvKeyDown (uint key, uint repeatCount, uint flags)
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

    case VK_DELETE:
        Markers.RemoveLast ();

        char                markerfilename[ MaxPathShort ];
        StringCopy      ( markerfilename, MRIDoc->GetDocPath () );
        AddExtension    ( markerfilename, FILEEXT_MRK );

        Markers.WriteFile ( markerfilename );

        if ( Markers.IsEmpty () )
            DeleteFileExtended ( markerfilename );

        Invalidate ( false );
        break;

    case 'O':
        CmOrient ();
        break;

    case 'R':
        CmSetRenderingMode ();
        break;

    case 'S':
        CmSetSurfaceMode ();
        break;

    case 'T':
        TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;

    case 'A':
        TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;

    case 'C':
        CmNextColorTable ();
        break;

    case 'D':
        CmSetScalingAdapt ();
        break;

    case 'E':
        if ( ! EditingNow )
            CmSetEditing ();
        break;

    case 'F':
        CmSetFindMinMax ();
        break;

    case 'I':
        CmIsoMode ( IDB_ISOMANUAL );
        break;

    case 'M':
        if ( MButtonDown && Picking.IsNotNull () ) {

            Markers.Add ( Picking );

            char                markerfilename[ MaxPathShort ];
            StringCopy      ( markerfilename, MRIDoc->GetDocPath () );
            AddExtension    ( markerfilename, FILEEXT_MRK );

            Markers.WriteFile ( markerfilename );
            }
        else if ( ShiftKey && IsSignedData () )
            CmShowMinMax ( IDB_SHOWMIN );
        else
            CmShowMinMax ( IDB_SHOWMAX );
        break;

    case 'X':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANECORONAL );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANECORONAL, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateY ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

    case 'Y':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANETRANSVERSE );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANETRANSVERSE, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateX ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

    case 'Z':
        if ( ControlKey )
            CmSetCutPlane ( IDB_CUTPLANESAGITTAL );
        else  if ( VkKey ( VK_ADD ) || VkKey ( VK_SUBTRACT ) )
            CmShiftCutPlane ( IDB_CUTPLANESAGITTAL, VkKey ( VK_ADD ) );
        else {
            ModelRotMatrix.RotateZ ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
            Invalidate ( false );
            }
        break;

    case '2':
        CmSetSliceMode ();
        break;

    case VK_ADD:
        CmSetNumSlices ( IDB_MORESLICES );
        break;

    case VK_SUBTRACT:
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
        if (ControlKey)
            CmMoveSlice ( IDB_LASTSLICEBWD );
        else
            CmMoveSlice ( IDB_FIRSTSLICEBWD );
        break;

    case VK_NEXT:
        if (ControlKey)
            CmMoveSlice ( IDB_LASTSLICEFWD );
        else
            CmMoveSlice ( IDB_FIRSTSLICEFWD );
         break;

    case VK_ESCAPE:
        if      ( CaptureMode == CaptureOperation ) {
            CmOperation ( 0 );
            CurrentOperation    = 0;
            }
        else if ( Editing && ! EditingNow ) {
            Editing     = (EditingToolsEnum) ( NumEditingTools - 1 );
            CmSetEditing ();
            }
        else
            TBaseView::EvKeyDown ( key, repeatCount, flags );
        break;

//  default:
//      TBaseView::EvKeyDown ( key, repeatCount, flags );
//      break;
    }
}


void    TVolumeView::EvSize ( uint sizeType, const TSize &size )
{
SetItemsInWindow ();

TBaseView::EvSize ( sizeType, size );
}


//----------------------------------------------------------------------------
void    TVolumeView::EvLButtonDown ( uint i, const TPoint &p )
{
MousePos   = p;

                                        // GL biz?
if      ( CaptureMode == CaptureGLLink || CaptureMode == CaptureGLMagnify ) {

    GLLButtonDown ( i, p );

    return;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( /*ControlKey ||*/  VkKey ( MouseMiddeClicksurrogate ) ) {  // make an alias Ctrl-Left = Middle

    EvMButtonDown ( i, p );

    return;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( CaptureMode == CaptureOperation ) {

    TBaseView*          view            = ClientToBaseView ( p );
    TVolumeDoc*         tomri           = view ? dynamic_cast< TVolumeDoc * > ( view->BaseDoc ) : 0;
                                        // close capture
    CmOperation ( 0 );
                                        // not a MRI window, or itself?
    if ( ! tomri || view == this )
        return;


    if ( *MRIDoc->GetSize () != *tomri->GetSize () ) {
        ShowMessage ( "Oops, you can not operate on volumes with different sizes for the moment!", "Volume Operation", ShowMessageWarning );
        return;
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    char                opext[ 32 ];

    if      ( CurrentOperation == CM_OPERATIONAND       )   {   MRIDoc->GetData ()->ApplyMaskToData ( *tomri->GetData () );
                                                                StringCopy  ( opext, "AND" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONOR        )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationOr  ); // OR will be Max of 2 competing values
                                                                StringCopy  ( opext, "OR" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONXOR       )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationXor );
                                                                StringCopy  ( opext, "XOR" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONADD       )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationAdd        );
                                                                StringCopy  ( opext, "Add" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONSUB       )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationSub        );
                                                                StringCopy  ( opext, "Sub" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONMULT      )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationMultiply   );
                                                                StringCopy  ( opext, "Mult" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONDIV       )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandData, OperandData, OperationDivide     );
                                                                StringCopy  ( opext, "Div" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONCLEAR     )   {   MRIDoc->GetData ()->ClearMaskToData ( *tomri->GetData () );
                                                                StringCopy  ( opext, "Remove" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONFILL      )   {   MRIDoc->GetData ()->ApplyDataToMask ( *tomri->GetData () );
                                                                StringCopy  ( opext, "Fill" );
                                                            }
    else if ( CurrentOperation == CM_OPERATIONREPLACE   )   {   MRIDoc->GetData ()->BinaryOp        ( *tomri->GetData (), OperandMask, OperandData, OperationOr  ); // OR but with priority to new volume
                                                                StringCopy  ( opext, "Insert" );
                                                            }
    else                                                    {   ShowMessage ( "Unsupported Operation!", "Volume Operation", ShowMessageWarning );
                                                                StringCopy  ( opext, "Op" );
                                                            }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // We can change the type of content in some cases
    if ( CurrentOperation == CM_OPERATIONFILL   ) {
                                        // filled with original content
//      MRIDoc->ExtraContentType    = tomri->ExtraContentType;  
        StringCopy ( MRIDoc->GetNiftiIntentName (), tomri->GetNiftiIntentName () );
        }

                                        // now can clear this flag
    CurrentOperation    = 0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update state of volume and view
    MRIDoc->ResetThenSetMri ( MriInitDetect );
                                        // update view
    SetBoxParameters ();

    SearchAndSetIntensity ();

    SetColorTable ( MRIDoc->GetAtomType ( AtomTypeUseCurrent ) );

    GLVolume.unGLize ();

    SetDefaultSurfaceMode ();
    ButtonGadgetSetState ( IDB_COLORIZESURFACE, SurfaceMode );
                                        // refresh texture!
    ComputeSurfaceColoring ();

    Invalidate ( false );


   //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // Nicely updating the file name
    TFileName           operandfilename ( tomri ->GetDocPath (), TFilenameExtendedPath );
    TFileName           operandfilenameshort;

    GetFilename         ( operandfilename );

    StringShrink        ( operandfilename, operandfilenameshort, 32 ); 

    TFileName           newfilename     ( MRIDoc->GetDocPath (), TFilenameExtendedPath );

    RemoveExtension     ( newfilename );

    StringAppend        ( newfilename,  ".", StringToUppercase ( opext ), "(", operandfilenameshort, ")" );
                                            // force change extension, maybe? - problem is f.ex. Analyze can have any orientation, which will kind of mess up with Nifti
    AddExtension        ( newfilename,  DefaultMriExt );

    CheckNoOverwrite    ( newfilename );
                                            // set to new name
    MRIDoc->SetDocPath  ( newfilename );
                                            // and force save to file(?)
    MRIDoc->Commit ( true );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else {
    LButtonDown = true;

    CaptureMode = CaptureLeftButton;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }
}


void    TVolumeView::EvLButtonUp ( uint i, const TPoint &p )
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


LButtonDown     = false;

CaptureMouse ( Release );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TVolumeView::EvMButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos    = p;

MButtonDown = true;

CaptureMode = CaptureGLSelect;

CaptureMouse ( Capture );

EvMouseMove ( i, p );
}


void    TVolumeView::EvMButtonUp (uint, const TPoint &/*p*/ )
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
void    TVolumeView::EvRButtonDown ( uint, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );

MousePos   = p;
}


void    TVolumeView::EvRButtonUp (uint, const TPoint &/*p*/)
{
MouseAxis   = MouseAxisNone;


if ( CaptureMode == CaptureRightButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


if ( IsoMode != MriIsoFixed && ! ControlKey /*&& MouseAxis == MouseAxisVertical*/ )
    ComputeSurfaceColoring ();          // because while pressed, we use a faster scheme

RButtonDown = false;
ShiftAxis   = ShiftNone;

CaptureMouse ( Release );

Invalidate ( false );


if ( ScalingAuto != ScalingAutoOff )   // releasing the user overriding scaling -> restore all
    BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::EvMouseMove ( uint i, const TPoint &p )
{
int                 dx              = p.X () - MousePos.X ();
int                 dy              = p.Y () - MousePos.Y ();
int                 adx             = abs ( dx );
int                 ady             = abs ( dy );


if ( RButtonDown ) {

    if ( ControlKey ) {
                                        // move planes
        if ( ! ( (bool) ClipPlane[0] || (bool) ClipPlane[1] || (bool) ClipPlane[2] ) )
            return;

        double  k   = (double) MRIDoc->GetSize ()->MaxSize () / GetClientRect ().Height () * 1.7;
    //    if ( 5 * abs ( dy ) > GetClientRect().Height() )
    //        k *= 3;

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


        if ( Editing ) {
                                        // use integer clipping only
            GLfloat             p       = ClipPlane[ ShiftAxis ].GetAbsPosition ( 1 );
            ClipPlane[ ShiftAxis ].SetAbsPosition ( p );
            }


    /*
                                            // 3 planes are quite ambiguous
        if ( ClipPlane[0] && ClipPlane[1] && ClipPlane[2] ) {
                                            // who has got the max participation?
            int     best= fabs(rat[ 0 ]) > fabs(rat[ 1 ]) ? fabs(rat[ 0 ]) > fabs(rat[ 2 ]) ? 0 : 2 : fabs(rat[ 2 ]) > fabs(rat[ 1 ]) ? 2 : 1;
                                            // shift it
            ClipPlane[ best ].Shift ( rat[best] * k );
            }
        else {                              // moving 2 planes at the same time is more pleasant
            bool    best[3];
            best[0] = fabs(rat[ 0 ]) > fabs(rat[ 1 ]) || fabs(rat[ 0 ]) > fabs(rat[ 2 ]);
            best[1] = fabs(rat[ 1 ]) > fabs(rat[ 0 ]) || fabs(rat[ 1 ]) > fabs(rat[ 2 ]);
            best[2] = fabs(rat[ 2 ]) > fabs(rat[ 0 ]) || fabs(rat[ 2 ]) > fabs(rat[ 1 ]);

            if ( ClipPlane[0] && best[0] )
                ClipPlane[0].Shift ( rat[ 0 ] * k );
            if ( ClipPlane[1] && best[1] )
                ClipPlane[1].Shift ( rat[ 1 ] * k );
            if ( ClipPlane[2] && best[2] )
                ClipPlane[2].Shift ( rat[ 2 ] * k );
            }
    */
        }

    else if ( Editing ) {
                                        // !not all tools can make use of all mouse adjustments!
        if      ( Editing == EditingToolSphereCenter 
               || Editing == EditingToolSphereCenterBlurr
               || Editing == EditingToolSphereSurface
               || Editing == EditingToolCylinder      && adx > ady ) {

            if ( dx > 0 )   EditingRadius *= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.5 : 0.1 );
            else            EditingRadius /= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.5 : 0.1 );

            Clipped ( EditingRadius, (double) 0.5, (double) MRIDoc->GetSize ()->MaxSize () / 2 );
            }

        else if ( Editing == EditingToolCylinder && adx < ady ) {

            if ( dy > 0 )   EditingDepth /= 1 + (double) 2 * min ( ady, MouseMoveScale ) / MouseMoveScale * ( ady > MouseMoveScaleFast ? 0.5 : 0.1 );
            else            EditingDepth *= 1 + (double) 2 * min ( ady, MouseMoveScale ) / MouseMoveScale * ( ady > MouseMoveScaleFast ? 0.5 : 0.1 );

            Clipped ( EditingDepth, (double) 0.5, (double) MRIDoc->GetSize ()->MaxSize () );
            }

        } // Editing

    else {                              // change color scaling

        if ( adx >= MinMouseMove || ady >= MinMouseMove ) {
            if      ( adx > ady * ( MouseAxis == MouseAxisVertical   ? 3 : 1 ) )
                MouseAxis   = MouseAxisHorizontal;
            else if ( ady > adx * ( MouseAxis == MouseAxisHorizontal ? 3 : 1 ) )
                MouseAxis   = MouseAxisVertical;
            }

        if ( MouseAxis == MouseAxisNone )
            return;


        if ( MouseAxis == MouseAxisVertical ) {

            if ( IsoMode == MriIsoManual ) {
                                        // using a shift proportional to max data, as the latter can be of any range
                double  bt  = fabs ( MRIDoc->GetIsoSurfaceCut () )
                            - (double) dy * ( ady < MouseMoveScale ? 0.5 : 1.5 ) * MRIDoc->GetAbsMaxValue () / 1000;
                
                SetIsoSurfaceCut ( false, AtLeast ( 0.0, bt ) );
                }
            else {
                ScalingContrast -= (double) dy / 100;

                CmSetContrast ( 0 );
                }
            }
        else if ( MouseAxis == MouseAxisHorizontal ) {
                                        // brightness & contrast
            if ( dx > 0 )   ScalingLevel /= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.5 : 0.1 );
            else            ScalingLevel *= 1 + (double) min ( adx, MouseMoveScale ) / MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.5 : 0.1 );

            CmSetBrightness ( 0 );
            }

//      MousePos   = p;
//      return;
        }

	LastMouseMove       = TPoint ( dx, dy );
	MousePos            = p;

    Invalidate ( false );   // behaves a bit better when doing isosurfaces
//  ShowNow ();
    }


else if ( Editing && ! EditingNow && ! ( LButtonDown || RButtonDown ) ) {
                                        // put a lock to following code
    EditingNow      = true;

                                        // we don't have a current dc, create a temp one
    GLrc.GLize ( UseThisDC ( *this ) );


    if ( GLrc.IsOpen () ) {
                                        // set current position
        Picking.Set ( p.X(), p.Y(), 0 );

                                        // then ask for underlying object coordinates
        if ( ! GLGetObjectCoord ( Picking, ViewportOrgSize ) )

            Picking.Reset ();

        else if ( MButtonDown ) {
                                        // this would be better controlled via buttons
            EditingModeEnum     how     = ControlKey ?  EditingCutOutside 
                                        : ShiftKey   ?  EditingFillInside 
                                        :               EditingCutInside;   // EditingFillOutside not implemented yet

                                        // !function needs a valid OpenGL context!
            ApplyEditingTool    (   Editing,        how,
                                    EditingPos1,    EditingPos2, 
                                    EditingRadius,  EditingDepth, 
                                    1 
                                );
            }


        GLrc.unGLize ();
        }
    else
        Picking.Reset ();


//  ShowNow ();                         // too fast?
    Invalidate ( false );               // seems safer

                                        // remove lock
    EditingNow      = false;
    }

else
    GLMouseMove ( i, p );


if ( ! Editing )
    SetDocTitle ( 0, 0 );
}


//----------------------------------------------------------------------------
void    TVolumeView::EvTimer ( uint timerId )
{
double              ta1;


switch ( timerId ) {

    case TimerStartup:

        AnimFx++;
                                        // finished?
        if ( ! AnimFx ) {

            AnimFx.Stop ();

            Zoom    = 1;
            MaterialOut.SetDiffuse ( MaterialOutDiffuse );

            ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
            return;
            }

        ta1     = AnimFx.GetPercentageCompletion();
        Zoom    = sqrt ( ta1 );
//      MaterialOut.SetDiffuse ( 0.80 / ta1,  0.80 / ta1 ,  0.80 / ta1,  1.00 / ta1 );

        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
        break;

    default:

        TBaseView::EvTimer ( timerId );
    }

}


//----------------------------------------------------------------------------
void    TVolumeView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& p )
{
TBaseView::EvMouseWheel ( modKeys, zDelta, p );
}


//----------------------------------------------------------------------------
                                        // Apply a given editing tool to the data
                                        // !It needs to be call within an active OpenGL context!
void    TVolumeView::ApplyEditingTool   (   EditingToolsEnum    tool,       EditingModeEnum     how,
                                            TPointFloat         pos1,       TPointFloat         pos2, 
                                            double              radius,     double              depth,
                                            double              feather
                                        )
{
if ( tool == EditingToolNone || MRIDoc->GetData ()->IsNotAllocated () )
    return;


MriType             fillingvalue    = how == EditingFillInside || how == EditingFillOutside? MRIDoc->GetAbsMaxValue () : 0;

bool                invert          = how == EditingCutOutside
                                    && ! (   tool == EditingToolSphereCenterBlurr       // not implemented
                                          || tool == EditingToolPlane             );


Maxed ( radius,     0.5 );
Maxed ( depth,      0.5 );
Maxed ( feather,    0.0 );


if ( MRIDoc->IsMask () )    feather     = 0;    // masks shouldn't have any feathering, as it produces intermediate values
else                        feather    /= 2.0;  // feather is applied half on each side


Volume&             vol             = *MRIDoc->GetData ();

Volume              savedvol;
if ( invert )       savedvol        = *MRIDoc->GetData ();

                                        // these were the absolute positions set during the display of tools
                                        // we need them in relative now for actually poking in the volume
MRIDoc->ToRel ( pos1 );
MRIDoc->ToRel ( pos2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Many tools need a valid gradient
if ( ToolNeedsGradient ( tool ) && EditingGradient.IsNull () )   
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do it!
if      ( tool == EditingToolSphereCenter
       || tool == EditingToolSphereSurface      )   vol.DrawSphere      ( pos1, radius, feather, fillingvalue, OperationSet );
else if ( tool == EditingToolSphereCenterBlurr  )   vol.BlurrSphere     ( pos1, radius );
else if ( tool == EditingToolCylinder           )   vol.DrawCylinder    ( pos1, pos2, radius * EditingCylinderRadiusRatio, feather, fillingvalue, OperationSet );
else if ( tool == EditingToolPlane              )   vol.ErasePlane      ( pos1, EditingGradient, feather );

                                        // subtract results
if ( invert )

    for ( int i = 0; i < vol.GetLinearDim (); i++ )
        vol[ i ]    = savedvol[ i ] - vol[ i ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLVolume.unGLize ();

ComputeSurfaceColoring ();

MRIDoc->SetIsoSurface ();

//BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );

MRIDoc->SetDirty ( true );
}


//----------------------------------------------------------------------------
                                        // updates for all views to recompute setup
bool    TVolumeView::VnViewUpdated ( TBaseView* /*view*/ )
{
//if ( ! view )
//    return true;

SetBoxParameters ();

SearchAndSetIntensity ();

SetColorTable ( MRIDoc->GetAtomType ( AtomTypeUseCurrent ) );

GLVolume.unGLize ();

SetDefaultSurfaceMode ();
ButtonGadgetSetState ( IDB_COLORIZESURFACE, SurfaceMode );
                                        // we could also reset these 2 guys - currently, GLPaint just ignores the irrelevant ones
//MRIDoc->GetTessSurfacePos ()->Reset ();
//MRIDoc->GetTessSurfaceNeg ()->Reset ();

SetDocTitle ( 0, 0 );

Invalidate ( false );

return true;
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetRenderingMode ()
{
int                 orm             = RenderingMode;


RenderingMode   = NextState ( RenderingMode, (int) MriNumRendering, ShiftKey );

                                        // check cutting planes
//if ( RenderingMode == MriSliceOpaque || orm == MriSliceOpaque ) {
if ( RenderingMode == MriSliceOpaque   && orm           != MriSliceOvercast
  || RenderingMode == MriSliceOvercast && orm           != MriSliceOpaque
  || orm           == MriSliceOpaque   && RenderingMode != MriSliceOvercast
  || orm           == MriSliceOvercast && RenderingMode != MriSliceOpaque ) {


                                        // leaving oriented clipping? store the direction values
    if ( ( RenderingMode == MriSliceOpaque || RenderingMode == MriSliceOvercast ) ) {

        SavedState.Clip[ 3 ]    = (int) ClipPlane[ 0 ];
        SavedState.Clip[ 4 ]    = (int) ClipPlane[ 1 ];
        SavedState.Clip[ 5 ]    = (int) ClipPlane[ 2 ];

                                        // set to unoriented
        if ( (bool) ClipPlane[ 0 ] )    ClipPlane[0].SetBackward ();
        if ( (bool) ClipPlane[ 1 ] )    ClipPlane[1].SetBackward ();
        if ( (bool) ClipPlane[ 2 ] )    ClipPlane[2].SetBackward ();

                                        // if no planes, set all of them, at least to see something!
        if ( ! ( (bool) ClipPlane[ 0 ] || (bool) ClipPlane[ 1 ] || (bool) ClipPlane[ 2 ] ) ) {
            ClipPlane[0].SetBackward ();
            ClipPlane[1].SetBackward ();
            ClipPlane[2].SetBackward ();
            }
        }
    else {                              // re-entering oriented clipping? restore directions if apropriate

        if ( SavedState.Clip[ 3 ] || SavedState.Clip[ 4 ] || SavedState.Clip[ 5 ] ) {

            if ( (bool) ClipPlane[ 0 ] && SavedState.Clip[ 3 ] )    ClipPlane[0].Set ( SavedState.Clip[ 3 ] );
            if ( (bool) ClipPlane[ 1 ] && SavedState.Clip[ 4 ] )    ClipPlane[1].Set ( SavedState.Clip[ 4 ] );
            if ( (bool) ClipPlane[ 2 ] && SavedState.Clip[ 5 ] )    ClipPlane[2].Set ( SavedState.Clip[ 5 ] );
            }
        else {
            ClipPlane[0].SetNone ();
            ClipPlane[1].SetNone ();
            ClipPlane[2].SetNone ();
            }
        }
    }


ButtonGadgetSetState ( IDB_CUTPLANECORONAL,    ClipPlane[ CtsToXyz ( 0 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANETRANSVERSE, ClipPlane[ CtsToXyz ( 1 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANESAGITTAL,   ClipPlane[ CtsToXyz ( 2 ) ] );


SetItemsInWindow();

Invalidate ( false );

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetCutPlane ( owlwparam wparam )
{
int                 p               = CtsToXyz ( wparam - IDB_CUTPLANECORONAL );
bool                singleside      = false; // RenderingMode == MriSliceOpaque || RenderingMode == MriSliceOvercast;   // allowing flat slices to be shown on each side, more compatible with clipped volume
ClipPlaneDirection  num             = singleside ? NumClipPlaneSym : NumClipPlane;

                                        // toggle to next state
                                                                                // inverting sequence for transverse and coronal cuts
ClipPlane[ p ].Set ( NextState ( ClipPlane[ p ].GetMode (), num, (bool) ( ShiftKey ^ ( p != 0 ) ) ) );


ButtonGadgetSetState ( wparam, ClipPlane[ p ] );

//if ( SliceMode )
    SetItemsInWindow();

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

        SetOrient ( BaseDoc );
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

        SetOrient ( BaseDoc ); 
                                        // cumulating additional rotations if more planes

                                        // main rotation
        if ( tra )              ModelRotMatrix.RotateX ( 45 * TrueToPlus ( traforward ), MultiplyLeft );
                                        // cosmetic rotation - not mandatory, just disambiguate a bit the perspective
        if ( tra )              ModelRotMatrix.RotateY ( -10, MultiplyLeft );
        }

    else if ( tra ) {

        Orientation = traforward ? OrientTransverseTop : OrientTransverseBottom;   

        SetOrient ( BaseDoc ); 
        }


    if ( noclipping ) {
                                        // in case user just switched off all planes, restore to predefined orientation instead of remaining in the last one
        if      ( sagold  )  Orientation = OrientSagittalLeft;
        else if ( corold  )  Orientation = OrientCoronalFront;
        else if ( traold  )  Orientation = OrientTransverseTop;

        SetOrient ( BaseDoc ); 
        }

                                        // remember for next call
    sagold  = sag;
    corold  = cor;
    traold  = tra;

#undef      ClipSag
#undef      ClipCor
#undef      ClipTra
//  }


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmShiftCutPlane ( owlwparam wparam, bool forward )
{
int     p       = CtsToXyz ( wparam - IDB_CUTPLANECORONAL );

                                        // don't shift if no planes
if ( ! (bool) ClipPlane[ p ] )
    return;
                                        // shift it
ClipPlane[ p ].Shift ( forward ? 1 : -1 );
ShowNow ();
}

/*
void    TVolumeView::CmSetVolumeEnable ( TCommandEnabler &tce )
{
tce.Enable ( !( RenderingMode == MriSliceOpaque || RenderingMode == MriSliceOvercast ) );
}
*/

//----------------------------------------------------------------------------
                                        // TODO: Moving these operations with all other filters
void    TVolumeView::CmFlip ( owlwparam wparam )
{
MRIDoc->Flip ( wparam );

SetBoxParameters ();

//SearchAndSetIntensity ();
//
//SetColorTable ( MRIDoc->GetAtomType ( AtomTypeUseCurrent ) );

GLVolume.unGLize ();
                                        // refresh texture!
ComputeSurfaceColoring ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmCoregistration ()
{
                                        // Try to find another open MRI, and see how we can deal with it
TDocument*          doc;
TVolumeDoc*         mri2            = 0;

for ( doc = CartoolDocManager->DocList.Next ( 0 ); doc != 0; doc = CartoolDocManager->DocList.Next ( doc ) ) {

    if ( doc == MRIDoc )
        continue;

    mri2    = dynamic_cast< TVolumeDoc* > ( doc );

    if ( mri2 != 0 )
        break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                inclusionhint[ 16 ];


if      ( MRIDoc->GetGeometryTransform () 
       && MRIDoc->GetGeometryTransform ()->Type != GeometryUnknown )

    StringCopy ( inclusionhint, "T" );

else if ( mri2 
       && mri2  ->GetGeometryTransform () 
       && mri2  ->GetGeometryTransform ()->Type != GeometryUnknown )

    StringCopy ( inclusionhint, "S" );

else if ( mri2 == 0 )
                                        // no transform and 1 MRI -> suggest it is a source (?)
    StringCopy ( inclusionhint, "S" );

else                                    // no transform and 2 MRIs? don't know what to say
    ClearString ( inclusionhint );


char                answer          = GetOptionFromUser ( "Is the current MRI the (S)ource or the (T)arget volume,\nor none and just (I)gnore it?" , 
                                                          MriCoregistrationTitle, "S T I", inclusionhint );

if ( answer == EOS )   return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !mri2 could totally be 0 here!
if      ( answer == 'T' )   CoregistrationMrisUI ( mri2,    MRIDoc  );
else if ( answer == 'S' )   CoregistrationMrisUI ( MRIDoc,  mri2    );
else                        CoregistrationMrisUI ( 0,       0       );
}


//----------------------------------------------------------------------------
//void    TVolumeView::CmFlipEnable ( TCommandEnabler &tce )
//{
//tce.Enable ( ! MRIDoc->IsUsedBy() );
//}


void    TVolumeView::CmSetNewOrigin ()
{
MRIDoc->SetNewOrigin ();

SetBoxParameters ();
}


void    TVolumeView::CmNewDownsampling ()
{
MRIDoc->NewIsoDownsampling ();

SetBoxParameters ();
}


//----------------------------------------------------------------------------
                                        // Catching filter IDs from menu
void    TVolumeView::CmFilter ( owlwparam w )
{
if      ( w == CM_FILTERMEAN                )   MRIDoc->Filter ( FilterTypeMean          );
else if ( w == CM_FILTERGAUSSIAN            )   MRIDoc->Filter ( FilterTypeGaussian      );
else if ( w == CM_FILTERANISOGAUSSIAN       )   MRIDoc->Filter ( FilterTypeAnisoGaussian );

else if ( w == CM_FILTERINTENSITYADD        )   MRIDoc->Filter ( FilterTypeIntensityAdd         );
else if ( w == CM_FILTERINTENSITYSUB        )   MRIDoc->Filter ( FilterTypeIntensitySub         );
else if ( w == CM_FILTERINTENSITYMULT       )   MRIDoc->Filter ( FilterTypeIntensityMult        );
else if ( w == CM_FILTERINTENSITYDIV        )   MRIDoc->Filter ( FilterTypeIntensityDiv         );
else if ( w == CM_FILTERINTENSITYOPPOSITE   )   MRIDoc->Filter ( FilterTypeIntensityOpposite    );
else if ( w == CM_FILTERINTENSITYINVERT     )   MRIDoc->Filter ( FilterTypeIntensityInvert      );
else if ( w == CM_FILTERINTENSITYABS        )   MRIDoc->Filter ( FilterTypeIntensityAbs         );
else if ( w == CM_FILTERINTENSITYREMAP      )   MRIDoc->Filter ( FilterTypeIntensityRemap       );
else if ( w == CM_FILTERINTENSITYNORM       )   MRIDoc->Filter ( FilterTypeIntensityNorm        );
else if ( w == CM_FILTERKEEPVALUE           )   MRIDoc->Filter ( FilterTypeKeepValue            );

else if ( w == CM_FILTERMIN                 )   MRIDoc->Filter ( FilterTypeMin      );
else if ( w == CM_FILTERMAX                 )   MRIDoc->Filter ( FilterTypeMax      );
else if ( w == CM_FILTERMINMAX              )   MRIDoc->Filter ( FilterTypeMinMax   );
else if ( w == CM_FILTERRANGE               )   MRIDoc->Filter ( FilterTypeRange    );
else if ( w == CM_FILTERMEDIAN              )   MRIDoc->Filter ( FilterTypeMedian   );
else if ( w == CM_FILTERMAXMODE             )   MRIDoc->Filter ( FilterTypeMaxMode  );
else if ( w == CM_FILTERMEANSUB             )   MRIDoc->Filter ( FilterTypeMeanSub  );
else if ( w == CM_FILTERLOGMEANSUB          )   MRIDoc->Filter ( FilterTypeLogMeanSub );
else if ( w == CM_FILTERMEANDIV             )   MRIDoc->Filter ( FilterTypeMeanDiv  );
else if ( w == CM_FILTERLOGMEANDIV          )   MRIDoc->Filter ( FilterTypeLogMeanDiv );
else if ( w == CM_FILTERMAD                 )   MRIDoc->Filter ( FilterTypeMAD      );
else if ( w == CM_FILTERMADSD               )   MRIDoc->Filter ( FilterTypeMADSDInv );
else if ( w == CM_FILTERSD                  )   MRIDoc->Filter ( FilterTypeSD       );
else if ( w == CM_FILTERSDINV               )   MRIDoc->Filter ( FilterTypeSDInv    );
else if ( w == CM_FILTERCOV                 )   MRIDoc->Filter ( FilterTypeCoV      );
else if ( w == CM_FILTERLOGCOV              )   MRIDoc->Filter ( FilterTypeLogCoV   );
else if ( w == CM_FILTERSNR                 )   MRIDoc->Filter ( FilterTypeLogSNR   );
else if ( w == CM_FILTERMCOV                )   MRIDoc->Filter ( FilterTypeMCoV     );
else if ( w == CM_FILTERLOGMCOV             )   MRIDoc->Filter ( FilterTypeLogMCoV  );
else if ( w == CM_FILTERMODE                )   MRIDoc->Filter ( FilterTypeModeQuant);
else if ( w == CM_FILTERENTROPY             )   MRIDoc->Filter ( FilterTypeEntropy  );
else if ( w == CM_FILTERPERCENTFULL         )   MRIDoc->Filter ( FilterTypePercentFullness );
else if ( w == CM_FILTERCUTBELOWFULL        )   MRIDoc->Filter ( FilterTypeCutBelowFullness );
else if ( w == CM_FILTERSAV                 )   MRIDoc->Filter ( FilterTypeSAV );
else if ( w == CM_FILTERCOMPACTNESS         )   MRIDoc->Filter ( FilterTypeCompactness );
else if ( w == CM_FILTERRANK                )   MRIDoc->Filter ( FilterTypeRank );
else if ( w == CM_FILTERRANKRAMP            )   MRIDoc->Filter ( FilterTypeRankRamp );

else if ( w == CM_FILTERTHRESHOLD           )   MRIDoc->Filter ( FilterTypeThreshold );
else if ( w == CM_FILTERTHRESHOLDABOVE      )   MRIDoc->Filter ( FilterTypeThresholdAbove );
else if ( w == CM_FILTERTHRESHOLDBELOW      )   MRIDoc->Filter ( FilterTypeThresholdBelow );
else if ( w == CM_FILTERSYMMETRIZE          )   MRIDoc->Filter ( FilterTypeSymmetrize );
else if ( w == CM_FILTERBINARIZE            )   MRIDoc->Filter ( FilterTypeBinarize );
else if ( w == CM_FILTERTOMASK              )   MRIDoc->Filter ( FilterTypeToMask   );
else if ( w == CM_FILTERIREVERT             )   MRIDoc->Filter ( FilterTypeRevert   );
else if ( w == CM_FILTERLESSNEIGH           )   MRIDoc->Filter ( FilterTypeLessNeighbors );
else if ( w == CM_FILTERMORENEIGH           )   MRIDoc->Filter ( FilterTypeMoreNeighbors );
else if ( w == CM_FILTERRELAX               )   MRIDoc->Filter ( FilterTypeRelax );
else if ( w == CM_FILTERTHINNING            )   MRIDoc->Filter ( FilterTypeThinning );
else if ( w == CM_FILTERWATERFALLRIDGES     )   MRIDoc->Filter ( FilterTypeWaterfallRidges );
else if ( w == CM_FILTERWATERFALLVALLEYS    )   MRIDoc->Filter ( FilterTypeWaterfallValleys );
else if ( w == CM_FILTERKEEPBIGGEST         )   MRIDoc->Filter ( FilterTypeKeepBiggestRegion );
else if ( w == CM_FILTERKEEPCOMPACT         )   MRIDoc->Filter ( FilterTypeKeepCompactRegion );
else if ( w == CM_FILTERKEEPBIGGEST2        )   MRIDoc->Filter ( FilterTypeKeepBiggest2Regions );
else if ( w == CM_FILTERKEEPCOMPACT2        )   MRIDoc->Filter ( FilterTypeKeepCompact2Regions );
else if ( w == CM_FILTERCLUSTERSTOREGIONS   )   MRIDoc->Filter ( FilterTypeClustersToRegions );

else if ( w == CM_FILTERHISTOEQUALIZE       )   MRIDoc->Filter ( FilterTypeHistoEqual );
else if ( w == CM_FILTERHISTOEQUALIZEBRAIN  )   MRIDoc->Filter ( FilterTypeHistoEqualBrain );
else if ( w == CM_FILTERHISTOCOMPACT        )   MRIDoc->Filter ( FilterTypeHistoCompact );

else if ( w == CM_FILTERGRADIENT            )   MRIDoc->Filter ( FilterTypeGradient );
else if ( w == CM_FILTERCANNY               )   MRIDoc->Filter ( FilterTypeCanny );
else if ( w == CM_FILTERLAPLACIAN           )   MRIDoc->Filter ( FilterTypeLaplacian );
else if ( w == CM_FILTERHESSIANEIGENMAX     )   MRIDoc->Filter ( FilterTypeHessianEigenMax );
else if ( w == CM_FILTERHESSIANEIGENMIN     )   MRIDoc->Filter ( FilterTypeHessianEigenMin );
else if ( w == CM_FILTERKCURVATURE          )   MRIDoc->Filter ( FilterTypeKCurvature );
else if ( w == CM_FILTERKCCURVATURE         )   MRIDoc->Filter ( FilterTypeKCCurvature );

else if ( w == CM_FILTERERODE               )   MRIDoc->Filter ( FilterTypeErode    );
else if ( w == CM_FILTERDILATE              )   MRIDoc->Filter ( FilterTypeDilate   );
else if ( w == CM_FILTEROPEN                )   MRIDoc->Filter ( FilterTypeOpen     );
else if ( w == CM_FILTERCLOSE               )   MRIDoc->Filter ( FilterTypeClose    );
else if ( w == CM_FILTERMORPHGRADIENT       )   MRIDoc->Filter ( FilterTypeMorphGradient );
else if ( w == CM_FILTERMORPHGRADIENTINT    )   MRIDoc->Filter ( FilterTypeMorphGradientInt );
else if ( w == CM_FILTERMORPHGRADIENTEXT    )   MRIDoc->Filter ( FilterTypeMorphGradientExt );

else if ( w == CM_FILTERBIASFIELD           )   MRIDoc->Filter ( FilterTypeBiasField );
else if ( w == CM_FILTERSEGMENTCSF          )   MRIDoc->Filter ( FilterTypeSegmentCSF );
else if ( w == CM_FILTERSEGMENTGREY         )   MRIDoc->Filter ( FilterTypeSegmentGrey );
else if ( w == CM_FILTERSEGMENTWHITE        )   MRIDoc->Filter ( FilterTypeSegmentWhite );
else if ( w == CM_FILTERSEGMENTTISSUES      )   MRIDoc->Filter ( FilterTypeSegmentTissues );
else if ( w == CM_FILTERMASKTOSP            )   MRIDoc->Filter ( FilterTypeMaskToSolutionPoints );
else if ( w == CM_SKULLSTRIPPING            )   MRIDoc->Filter ( FilterTypeSkullStripping );
else if ( w == CM_CUTBRAINSTEM              )   MRIDoc->Filter ( FilterTypeBrainstemRemoval );

//else if ( w == CM_FILTERRESIZE              )   MRIDoc->Filter ( FilterTypeResize );


SetBoxParameters ();

SearchAndSetIntensity ();

SetColorTable ( MRIDoc->GetAtomType ( AtomTypeUseCurrent ) );

GLVolume.unGLize ();
                                        // refresh texture!
ComputeSurfaceColoring ();

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmOperation ( owlwparam w )
{
if ( CaptureMode == CaptureNone ) {

    ::SetCursor ( *CartoolApplication->CursorOperation );

    CaptureMouse ( Capture );

    MouseDC     = new TClientDC ( *this );

    CaptureMode         = CaptureOperation;
    CurrentOperation    = w;
    }

else if ( CaptureMode == CaptureOperation ) {

    CaptureMouse ( Release );

    delete MouseDC;
    MouseDC     = 0;

    CaptureMode         = CaptureNone;
//  CurrentOperation    = 0;    // don't reset yet, we can still need it in EvLButtonDown
    }

}


//----------------------------------------------------------------------------
void    TVolumeView::CmShowMinMax ( owlwparam w )
{
                                        // Right now it works this way:
                                        // - If only Max is allowed, just play forward/backward the setting
                                        // - If both Min and Max are allowed, the current one pressed will overwrite the other one
                                        //   so they are in sync (display does not allow otherwise for the moment)
if ( w == IDB_SHOWMIN ) {

    if ( IsSignedData () ) {

//      ShowMin     = NextState ( ShowMin, MriNumMinMax, ShiftKey );
        ShowMin     = NextState ( ShowMin, MriNumMinMax, false );
                                            // synchronize to ShowMax
//      if ( ShowMin && ShowMax )   ShowMin = max ( ShowMin, ShowMax ); // highest of the 2 propagates
        if ( ShowMin && ShowMax )   ShowMax = ShowMin;                  // last one pressed propagates
        }

    } // IDB_SHOWMIN

else if ( w == IDB_SHOWMAX ) {

    ShowMax     = NextState ( ShowMax, MriNumMinMax, ShiftKey );
                                            // synchronize to ShowMin
//  if ( ShowMin && ShowMax )   ShowMax = max ( ShowMin, ShowMax );     // highest of the 2 propagates
    if ( ShowMin && ShowMax )   ShowMin = ShowMax;                      // last one pressed propagates

    } // IDB_SHOWMAX


if ( ! IsSignedData () )
    ShowMin = MriMinMaxNone;


ButtonGadgetSetState ( IDB_SHOWMIN, ShowMin );
ButtonGadgetSetState ( IDB_SHOWMAX, ShowMax );

                                        // update all slices
Slices[ SliceModeCor - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeCor - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );


SetItemsInWindow ();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmIsScalarEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsSignedData () );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetFindMinMax ()
{
FindMinMax      = ! FindMinMax;

ButtonGadgetSetState ( IDB_FINDMINMAX, FindMinMax );

                                        // update all slices
Slices[ SliceModeCor - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeCor - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeTra - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMinSlice ( /* ShowMin && */ IsSignedData () && FindMinMax );
Slices[ SliceModeSag - 1 ].ShowMaxSlice ( /* ShowMax && */                    FindMinMax );


SetItemsInWindow ();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmOutputHistogram ( owlwparam w )
{
double              kernelsubsampling   = MRIDoc->IsMask () ? 1 : 3;

HistogramOptions    options             = NoHistogramOptions;

SetFlags ( options, w == CM_FILTERCDFCOMPUTE        ?   HistogramCDF        : HistogramPDF          );
SetFlags ( options, MRIDoc->IsMask ()               ?   HistogramRaw        : HistogramSmooth       );
SetFlags ( options,                                     HistogramIgnoreNulls                        );
SetFlags ( options, w == CM_FILTERCDFCOMPUTE        ?   HistogramNormMax    : HistogramNormArea     );
SetFlags ( options, w == CM_FILTERHISTOCOMPUTELOG   ?   HistogramLog        : HistogramLinear       );
SetFlags ( options, MRIDoc->IsMask ()               ?   HistogramDiscrete   : HistogramContinuous   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

THistogram          H;

TFileName           file;
StringCopy ( file, MRIDoc->GetDocPath () );


if      ( w == CM_FILTERHISTOCOMPUTETXT ) {

    ReplaceExtension    ( file, InfixHistogramLong "." FILEEXT_TXT );

    CheckNoOverwrite    ( file );

    H.ComputeHistogram  (   *MRIDoc->GetData (), 
                            0,
                            0,
                            0, 
                            0,  kernelsubsampling,
                            options
                        );

    H.WriteFileVerbose  ( file, true );

    file.Open ();
    }

else if ( w == CM_FILTERHISTOCOMPUTE ) {

    ReplaceExtension    ( file, InfixHistogramLong "." FILEEXT_EEGSEF );

    CheckNoOverwrite    ( file );

    H.ComputeHistogram  (   *MRIDoc->GetData (), 
                            0, 
                            0,
                            0, 
                            0,  kernelsubsampling,
                            options
                        );

    H.WriteFile         ( file, "PDF" );
    file.Open ();
    }

else if ( w == CM_FILTERHISTOCOMPUTELOG ) {

    ReplaceExtension    ( file, InfixHistogramLong "Log" "." FILEEXT_EEGSEF );

    CheckNoOverwrite    ( file );

    H.ComputeHistogram  (   *MRIDoc->GetData (), 
                            0, 
                            0,
                            0, 
                            0,  kernelsubsampling,
                            options
                        );

    H.WriteFile         ( file, "LogPDF" );
    file.Open ();
    }

else if ( w == CM_FILTERCDFCOMPUTE ) {

    ReplaceExtension    ( file, InfixCDF "." FILEEXT_EEGSEF );

    CheckNoOverwrite    ( file );

    H.ComputeHistogram  (   *MRIDoc->GetData (), 
                            0, 
                            0,
                            0, 
                            0,  kernelsubsampling,
                            options
                        );

    H.WriteFile         ( file, "CDF" );
    file.Open ();
    }


//ShowMessage ( "Histogram has been saved to file.", file, ShowMessageDefault, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetBrightness ( owlwparam wparam )
{
double              osl             = ScalingLevel;

if      ( wparam == IDB_ISINCBRIGHT )   ScalingLevel /= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep ;
else if ( wparam == IDB_ISDECBRIGHT )   ScalingLevel *= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep;

SetScaling ( ScalingLevel );

if ( osl == ScalingLevel && wparam )
    return;

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


void    TVolumeView::CmSetContrast ( owlwparam wparam )
{
double              osc             = ScalingContrast;

if      ( wparam == IDB_ISINCCONTRAST )     ScalingContrast += 0.05;
else if ( wparam == IDB_ISDECCONTRAST )     ScalingContrast -= 0.05;

SetScalingContrast ( ScalingContrast );

if ( osc == ScalingContrast && wparam )
    return;

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmNextColorTable ()
{
ColorTable.NextColorTable ( ShiftKey );

ColorTableIndex[ MRIDoc->GetAtomType ( AtomTypeUseCurrent ) ]   = ColorTable.GetTableType ();

GLVolume.unGLize ();

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


void    TVolumeView::CmSetScalingAdapt ()
{
ScalingAuto     = NextState ( ScalingAuto, IsSignedData () ? NumScalingAuto : NumScalingAutoPositive, ShiftKey );

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );
                                        // cancelling the asymmetrical scaling, if any
if ( ScalingAuto == ScalingAutoOff )
    UpdateScaling (); 

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


void    TVolumeView::CmSetIntensityLevelEnable ( TCommandEnabler &tce )
{
tce.Enable ( ScalingAuto == ScalingAutoOff );
}


void    TVolumeView::CmSetEditing ()
{
if ( EditingNow )
    return;


Editing     = NextState ( Editing, NumEditingTools, ShiftKey );

ButtonGadgetSetState ( IDB_EDITING, Editing );

Invalidate ( false );

//BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmIsoMode ( owlwparam wparam )
{
if      ( wparam == IDB_ISOMANUAL ) {
    if ( IsoMode == MriIsoManual )
        IsoMode = MriIsoFixed;
    else
        IsoMode = MriIsoManual;
    }

ButtonGadgetSetState ( IDB_ISOMANUAL, IsoMode == MriIsoManual );

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetSurfaceMode ()
{
SurfaceMode     = ! SurfaceMode;

ButtonGadgetSetState ( IDB_COLORIZESURFACE, SurfaceMode );

ComputeSurfaceColoring ();

ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
void    TVolumeView::ComputeSurfaceColoring ( bool fast )
{
                                        // only ROI masks need this texture
if ( ! ( SurfaceMode && MRIDoc->IsRoiMask () ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const Volume&       vol             = *MRIDoc->GetData ();

                                        // allocate (once) this global buffer, plus giving it a more texture-compatible size
if ( SurfaceData.IsNotAllocated () )
    SurfaceData.Resize ( Power2Above ( vol.GetDim1 () ), Power2Above ( vol.GetDim2 () ), Power2Above ( vol.GetDim3 () ) );

                                        // clear-up texture, so that it will reload on next redraw
SurfaceVolume.unGLize ();

                                        // set data as a default texture, and return
if ( fast ) {
    SurfaceData.Insert ( *MRIDoc->GetData () );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MriType             threshold       = MRIDoc->GetIsoSurfaceCut ();

SurfaceData.ResetMemory ();

                                        // first, transfer only values above threshold
for ( int x = 0; x < vol.GetDim1 (); x++ )
for ( int y = 0; y < vol.GetDim2 (); y++ )
for ( int z = 0; z < vol.GetDim3 (); z++ )

    if ( vol ( x, y, z ) >= threshold )
        SurfaceData ( x, y, z ) = vol ( x, y, z );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // inflate data

                                        // too slow(?)
//FctParams           p;
//p ( FilterParamDiameter )     = 3;
//SurfaceData.Filter ( FilterTypeMax, p, true );

                                        // "above" part - needs more inflation on this side
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int x = SurfaceData.GetDim1 () - 1; x >= 2; x-- )

    if      ( ! SurfaceData ( x, y, z ) && SurfaceData ( x - 1, y, z ) )    SurfaceData ( x, y, z ) = SurfaceData ( x - 1, y, z );
    else if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x - 2, y, z ) )    SurfaceData ( x, y, z ) = SurfaceData ( x - 2, y, z );


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int y = SurfaceData.GetDim2 () - 1; y >= 2; y-- )

    if      ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y - 1, z ) )    SurfaceData ( x, y, z ) = SurfaceData ( x, y - 1, z );
    else if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y - 2, z ) )    SurfaceData ( x, y, z ) = SurfaceData ( x, y - 2, z );


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = SurfaceData.GetDim3 () - 1; z >= 2; z-- )

    if      ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y, z - 1 ) )    SurfaceData ( x, y, z ) = SurfaceData ( x, y, z - 1 );
    else if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y, z - 2 ) )    SurfaceData ( x, y, z ) = SurfaceData ( x, y, z - 2 );

    
                                        // "below" part
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int x = 0; x <= SurfaceData.GetDim1 () - 2; x++ )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x + 1, y, z ) )         SurfaceData ( x, y, z ) = SurfaceData ( x + 1, y, z );


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int y = 0; y <= SurfaceData.GetDim2 () - 2; y++ )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y + 1, z ) )         SurfaceData ( x, y, z ) = SurfaceData ( x, y + 1, z );


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = 0; z <= SurfaceData.GetDim3 () - 2; z++ )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y, z + 1 ) )         SurfaceData ( x, y, z ) = SurfaceData ( x, y, z + 1 );


/*                                      // then inflate the mask obtained with MRI data
                                        // use histogram for highest probability
THistogram          histoneighbors ( 256 );


for ( int x = 0; x < vol.GetDim1 (); x++ )
for ( int y = 0; y < vol.GetDim2 (); y++ )
for ( int z = 0; z < vol.GetDim3 (); z++ )

    if ( ! SurfaceData ( x, y, z ) ) {
/*
                                // 6 neighbors
//      SurfaceData ( x, y, z ) = max ( vol ( x + 1, y, z ), vol ( x - 1, y, z ),
//                                max ( vol ( x, y + 1, z ), vol ( x, y - 1, z ),
//                                max ( vol ( x, y, z + 1 ), vol ( x, y, z - 1 ) ) ) );

                                // 14 neighbors seems enough
        SurfaceData ( x, y, z ) = max ( vol ( x + 1, y, z ),       vol ( x - 1, y, z ),
                                  max ( vol ( x + 1, y + 1, z ),   vol ( x + 1, y - 1, z ),
                                  max ( vol ( x - 1, y + 1, z ),   vol ( x - 1, y - 1, z ),
                                  max ( vol ( x, y + 1, z ),       vol ( x, y - 1, z ),
                                  max ( vol ( x, y, z + 1 ),       vol ( x, y, z - 1 ),
                                  max ( vol ( x, y + 1, z + 1 ),   vol ( x, y + 1, z - 1 ),
                                  max ( vol ( x, y - 1, z + 1 ),   vol ( x, y - 1, z - 1 ) ) ) ) ) ) ) );

//      if ( SurfaceData ( x, y, z ) < threshold )
//          SurfaceData ( x, y, z ) = threshold;
        }
* /

        histoneighbors.Reset ();

        if ( x ) {
            if (                           vol ( x - 1, y,     z ) >= threshold ) histoneighbors.Add ( vol ( x - 1, y, z ) );
            if ( y                      && vol ( x - 1, y - 1, z ) >= threshold ) histoneighbors.Add ( vol ( x - 1, y - 1, z ) );
            if ( y < vol.GetDim2 () - 1 && vol ( x - 1, y + 1, z ) >= threshold ) histoneighbors.Add ( vol ( x - 1, y + 1, z ) );
            }

        if ( x < vol.GetDim1 () - 1 ) {
            if (                           vol ( x + 1, y,     z ) >= threshold ) histoneighbors.Add ( vol ( x + 1, y, z ) );
            if ( y                      && vol ( x + 1, y - 1, z ) >= threshold ) histoneighbors.Add ( vol ( x + 1, y - 1, z ) );
            if ( y < vol.GetDim2 () - 1 && vol ( x + 1, y + 1, z ) >= threshold ) histoneighbors.Add ( vol ( x + 1, y + 1, z ) );
            }


        if ( y ) {
            if (                           vol ( x, y - 1, z     ) >= threshold ) histoneighbors.Add ( vol ( x, y - 1, z ) );
            if ( z                      && vol ( x, y - 1, z - 1 ) >= threshold ) histoneighbors.Add ( vol ( x, y - 1, z - 1 ) );
            if ( z < vol.GetDim3 () - 1 && vol ( x, y - 1, z + 1 ) >= threshold ) histoneighbors.Add ( vol ( x, y - 1, z + 1 ) );
            }

        if ( y < vol.GetDim2 () - 1 ) {
            if (                           vol ( x, y + 1, z     ) >= threshold ) histoneighbors.Add ( vol ( x, y + 1, z ) );
            if ( z                      && vol ( x, y + 1, z - 1 ) >= threshold ) histoneighbors.Add ( vol ( x, y + 1, z - 1 ) );
            if ( z < vol.GetDim3 () - 1 && vol ( x, y + 1, z + 1 ) >= threshold ) histoneighbors.Add ( vol ( x, y + 1, z + 1 ) );
            }


        if      ( z                      && vol ( x, y, z - 1 ) >= threshold )    histoneighbors.Add ( vol ( x, y, z - 1 ) );
        if      ( z < vol.GetDim3 () - 1 && vol ( x, y, z + 1 ) >= threshold )    histoneighbors.Add ( vol ( x, y, z + 1 ) );


        SurfaceData ( x, y, z ) = (MriType) histoneighbors.MaximumPosition ();
        }


                                        // for safety, try to add one more voxel, stopping when reaching the border
                                        // this is if MRI was cut at the very border, then texture will show 0!
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int x = SurfaceData.GetDim1 () - 1; x >= 1; x-- )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x - 1, y, z ) ) {
        SurfaceData ( x, y, z ) = SurfaceData ( x - 1, y, z );
        break;
        }


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int y = 0; y < SurfaceData.GetDim2 (); y++ )
for ( int z = SurfaceData.GetDim3 () - 1; z >= 1; z-- )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y, z - 1 ) ) {
        SurfaceData ( x, y, z ) = SurfaceData ( x, y, z - 1 );
        break;
        }


for ( int x = 0; x < SurfaceData.GetDim1 (); x++ )
for ( int z = 0; z < SurfaceData.GetDim3 (); z++ )
for ( int y = SurfaceData.GetDim2 () - 1; y >= 1; y-- )

    if ( ! SurfaceData ( x, y, z ) && SurfaceData ( x, y - 1, z ) ) {
        SurfaceData ( x, y, z ) = SurfaceData ( x, y - 1, z );
        break;
        }
*/

}


//----------------------------------------------------------------------------
void    TVolumeView::SetIsoSurfaceCut ( bool automatic, double bt )
{
                                        // put a semaphor to avoid fast calls to re-enter the function, which can be comparatively slow
static bool         inuse           = false;

if ( inuse )        return;

inuse   = true;


MRIDoc->NewIsoSurfaceCut ( automatic, bt );

SetBoxParameters ();

ComputeSurfaceColoring ( IsoMode != MriIsoFixed && RButtonDown && ! ControlKey && MouseAxis == MouseAxisVertical );

SetDocTitle ( 0, 0 );

if ( SliceMode )                        // re-tile as the shape can change a lot...
    SetItemsInWindow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );


inuse   = false;
}


void    TVolumeView::CmNewIsoSurfaceCut ()
{
SetIsoSurfaceCut ( true );

ShowNow ();
}


void    TVolumeView::CmSetShiftDepthRange ( owlwparam w )
{
TBaseView::CmSetShiftDepthRange ( w );

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSliceModeEnable ( TCommandEnabler &tce )
{
tce.Enable ( SliceMode );
}


void    TVolumeView::CmSetSliceMode ()
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
        SavedState.RenderingMode    = RenderingMode;

        SavedState.ClipPlane[ 0 ]   = ClipPlane[ 0 ];
        SavedState.ClipPlane[ 1 ]   = ClipPlane[ 1 ];
        SavedState.ClipPlane[ 2 ]   = ClipPlane[ 2 ];
        }

                                        // set current matrix according to slice mode
    ModelRotMatrix  = SlicesRotMatrix[ SliceMode - 1 ];


    RenderingMode   = MriSliceOpaque;

                                        // select the right planes according to the projection
    InitClipPlanes ( ClipPlane );
    }
else {
                                        // restore 3D state
    RenderingMode   = SavedState.RenderingMode;

    ClipPlane[ 0 ]  = SavedState.ClipPlane[ 0 ];
    ClipPlane[ 1 ]  = SavedState.ClipPlane[ 1 ];
    ClipPlane[ 2 ]  = SavedState.ClipPlane[ 2 ];

    ModelRotMatrix  = SavedState.ModelRotMatrix;
    }


ButtonGadgetSetState ( IDB_CUTPLANECORONAL,     ClipPlane[ CtsToXyz ( 0 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANETRANSVERSE,  ClipPlane[ CtsToXyz ( 1 ) ] );
ButtonGadgetSetState ( IDB_CUTPLANESAGITTAL,    ClipPlane[ CtsToXyz ( 2 ) ] );
ButtonGadgetSetState ( IDB_PLANEMODE, SliceMode );


//if ( (bool) AnimFx && AnimFx.GetTimerId () == TimerStartup )
//    Zoom = 1;

SetItemsInWindow ();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetNumSlices ( owlwparam wparam )
{
if ( ! SliceMode )
    return;


if ( ! SetNumSlices ( wparam, ShiftKey ) )
    return;


SetItemsInWindow();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmMoveSlice ( owlwparam wparam )
{
if ( ! SliceMode )
    return;


MoveSlice ( MRIDoc, wparam, ShiftKey );


SetItemsInWindow();

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::SetItemsInWindow ()
{
if ( SliceMode ) {

    double              msx;
    double              msy;

    GetSliceSize ( MRIDoc, msx, msy );

    FitItemsInWindow ( Slices[ SliceMode - 1 ].GetTotalSlices(), TSize ( msx, msy ), SlicesPerX, SlicesPerY );
    }
else
    FitItemsInWindow ( 1, TSize ( 1, 1 ), SlicesPerX, SlicesPerY );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmOrient ()
{
if      ( SliceMode == SliceModeCor )   ModelRotMatrix = CTSMatrix[ 0 ]; // not SliceX but SliceCoronal
else if ( SliceMode == SliceModeTra )   ModelRotMatrix = CTSMatrix[ 1 ];
else if ( SliceMode == SliceModeSag )   ModelRotMatrix = CTSMatrix[ 2 ];
else {
    TBaseView::CmOrient ();
    }

ShowNow ();
}


//----------------------------------------------------------------------------
void    TVolumeView::CmSetOrientation ()
{
char                answer[ 256 ];

if ( ! GetOptionsFromUser ( "Specify the directions the X,Y and Z axis are pointing to by using a 3 letters code. Allowed letters are 'APLRIS', standing for Anterior Posterior Left Right Inferior Superior.\nAn example of code is 'RAS' for X->Right Y->Anterior Z->Superior:",
                            "Set Orientation Flags", "A P L R I S", "", answer, this ) )
    return;


if ( StringLength ( answer ) != 3 ) {
    ShowMessage ( "There appears to be an error in your answer:\n\n  - Just use the letters among A, P, L, R, I and S\n  - Give one letter for the X, Y and Z axis respectively, f.ex. RAS or PIR", "Set Orientation Flags", ShowMessageWarning, this );
    return;
    }

                                        // process answer
if ( ! MRIDoc->SetOrientation ( answer ) ) {
    ShowMessage ( "Can not make a proper orientation with your answer, sorry!\n\nHints: no repeated letters; no left-handed axis.", "Set Orientation Flags", ShowMessageWarning, this );
    return;
    }

//MRIDoc->SetKnownOrientation ( true );


                                        // update views
ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );

MRIDoc->SetDirty ( true );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmResetOrientation ()
{
//                                        // only if it has been forced
//if ( ! MRIDoc->HasKnownOrientation () )
//    return;

                                        // reset flag
MRIDoc->SetKnownOrientation ( false );
                                        // run the auto detection
BaseDoc->FindOrientation ();

                                        // update views
ShowNow ();

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );

MRIDoc->SetDirty ( true );
}


void    TVolumeView::CmResetOrientationEnable ( TCommandEnabler &tce )
{
tce.Enable ( true /*MRIDoc->HasKnownOrientation ()*/ );
}


//----------------------------------------------------------------------------
void    TVolumeView::CmPreprocessMris ()
{
                                        // use a common transfer buffer
//TPreprocessMrisStruct&  transfer = MriPreprocTransfer;

                                        // run the dialog, using current MRI
TPreprocessMrisDialog ( CartoolMdiClient, IDD_MRIPREPROCESSING, MRIDoc ).Execute();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
