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

#pragma once

#include    "TVolumeDoc.h"
#include    "TBaseView.h"
#include    "TBaseVolumeView.h"
#include    "OpenGL.h"
#include    "OpenGL.Texture3D.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    ExtraSizeMRISliceMode       = 1.10;

                                        // a little more space between each slice (multiply factor)
constexpr double    SliceMargin                 = 1.06666;

#define SliceColor                      (GLfloat) 0.33, (GLfloat) 0.33, (GLfloat) 0.33, (GLfloat) 1.00


enum    {
        MRIGLVIEW_CBG_SEP1          = NumBaseViewButtons,
        MRIGLVIEW_CBG_ISOMANUAL,
        MRIGLVIEW_CBG_COLORIZESURFACE,
        MRIGLVIEW_CBG_EDITING,
        MRIGLVIEW_CBG_SEP1A,
        MRIGLVIEW_CBG_CUTPLANECORONAL,
        MRIGLVIEW_CBG_CUTPLANETRANSVERSE,
        MRIGLVIEW_CBG_CUTPLANESAGITTAL,
        MRIGLVIEW_CBG_SEP2,
        MRIGLVIEW_CBG_PLANEMODE,
        MRIGLVIEW_CBG_LESSSLICES,
        MRIGLVIEW_CBG_MORESLICES,
        MRIGLVIEW_CBG_FIRSTSLICEFWD,
        MRIGLVIEW_CBG_FIRSTSLICEBWD,
        MRIGLVIEW_CBG_LASTSLICEFWD,
        MRIGLVIEW_CBG_LASTSLICEBWD,
        MRIGLVIEW_CBG_SEP3,
        MRIGLVIEW_CBG_SHMIN,
        MRIGLVIEW_CBG_SHMAX,
        MRIGLVIEW_CBG_FINDMINMAX,
        MRIGLVIEW_CBG_SEP4,
        MRIGLVIEW_CBG_DECBR,
        MRIGLVIEW_CBG_INCBR,
        MRIGLVIEW_CBG_DECCR,
        MRIGLVIEW_CBG_INCCR,
        MRIGLVIEW_CBG_SEP5,
        MRIGLVIEW_CBG_FXDSCL,
        MRIGLVIEW_CBG_COLOR,
        MRIGLVIEW_CBG_NUM
        };

enum    MriRendering
        {
        MriNone,
        MriSliceOvercast,
        MriSliceOpaque,
        MriTransparent,
        MriOpaque,
        MriNumRendering
        };

enum    {
        MriNoCapping = 0,
        MriPlainCapping,
        MriBrainCapping,
        MriNumCappingMode
        };

enum    {
        MriIsoFixed   = 0,
        MriIsoManual,
        MriNumIsoMode
        };

enum    MriMaxRendering
        {
        MriMinMaxNone,
        MriMinMaxCircle,
        MriMinMaxCross,
        MriNumMinMax,
        };

//----------------------------------------------------------------------------
                                        // Editing tools
#define EditingColorTool                (GLfloat) 0.90, (GLfloat) 0.20, (GLfloat) 0.20
#define EditingColorGradient            (GLfloat) 0.90, (GLfloat) 0.90, (GLfloat) 0.00

constexpr double    EditingCylinderRadiusRatio  = 0.50;
constexpr double    EditingPlaneOffset          = 0.50;
constexpr int       EditingGradientMinSizeSmall = 3;
constexpr int       EditingGradientMinSizeBig   = 32;

                                        // Interactive editing tools
enum    EditingToolsEnum
        {
        EditingToolNone,
        EditingToolSphereCenter,
        EditingToolSphereCenterBlurr,
        EditingToolCylinder,
        EditingToolSphereSurface,
        EditingToolPlane,

        NumEditingTools,
        };

                                        // 4 possible modes - not all implemented, though
enum    EditingModeEnum
        {
        EditingCutInside,
        EditingCutOutside,
        EditingFillInside,
        EditingFillOutside,

        NumEditingModes,
        };

extern  const char  EditingToolsString[ NumEditingTools ][ 256 ];


inline  bool    ToolNeedsGradient   ( EditingToolsEnum t )  {   return  t == EditingToolCylinder || t == EditingToolSphereSurface || t == EditingToolPlane; }


//----------------------------------------------------------------------------
                                        // Class to save and restore display states
class   TVolumeState
{
public:
                            TVolumeState ();


    int                     RenderingMode;

    int                     Clip     [ 6 ];
    TGLClipPlane            ClipPlane[ 3 ];

    TGLMatrix               ModelRotMatrix;
};


//----------------------------------------------------------------------------
                                        // Volume display, which could be any modality, not only MRI
                                        // Supports only scalar data, signed / positive / negative only
class   TVolumeView :           public  TBaseView,
                                public  TBaseVolumeView
{
public:
                                TVolumeView ( TVolumeDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group = 0 );


    static const char*          StaticName          ()              { return "&Volume Display"; }
    const char*                 GetViewName         ()              { return StaticName(); }

    void                        CreateGadgets       ();


    TVolumeDoc *                GetMRIDoc           ()              { return MRIDoc; }


    void                        GLPaint             ( int how, int renderingmode, TGLClipPlane *otherclipplane );
//  void                        GLPaint             ( int how, int renderingmode, TGLClipPlane *otherclipplane, TGLVolume *volume );    // we need an additional volume object as parameter, used by the inverse display
    void                        HintsPaint          ();
    bool                        IsRenderingMode     ( int renderingmode );

    bool                        ModifyPickingInfo   ( TPointFloat& Picking, char *buff );


    TGLVolume                   GLVolume; // giving full access to other views


protected:

    TVolumeDoc*                 MRIDoc;

                                        // OpenGL
    TGLMaterial<GLfloat>        MaterialOut;
//  TGLMaterial<GLfloat>        MaterialIn;
    TGLMaterial<GLfloat>        MaterialTr;
    int                         CappingMode;
    MriMaxRendering             ShowMin;
    MriMaxRendering             ShowMax;
    bool                        FindMinMax;
    int                         IsoMode;
    int                         CurrIsoDownsampling;
//  TManyViewpoints             Viewpoints;

    double                      GalMinValue;
    double                      GalMaxValue;
    TPointFloat                 GalMinPos;
    TPointFloat                 GalMaxPos;
    TGLMatrix                   AntiModelRotMatrix;

                                        // Interactive editing
    EditingToolsEnum            Editing;
    double                      EditingRadius;
    double                      EditingDepth;
    bool                        EditingNow;
    TPointFloat                 EditingGradient;    // sharing a few variables between display and tools application
    TPointFloat                 EditingPos1;
    TPointFloat                 EditingPos2;

                                        // Used for surface texturing
    bool                        SurfaceMode;
    Volume                      SurfaceData;
    TGLVolume                   SurfaceVolume;
    void                        ComputeSurfaceColoring ( bool fast = false );

    TVolumeState                SavedState;
    owlwparam                   CurrentOperation;

    TPoints                     Markers;    // temporary hack (well, this has been here for years already...)

                                        // Interactive editing
    void                        ApplyEditingTool    ( EditingToolsEnum tool, EditingModeEnum how, TPointFloat pos1, TPointFloat pos2, double radius, double depth, double feather );


    bool                        ValidView               ()          { return MRIDoc->GetData(); }
    void                        SetBoxParameters        ();
    void                        SearchAndSetIntensity   ();
    void                        SetIsoSurfaceCut        ( bool automatic, double bt = 0 );
    void                        SetItemsInWindow        ();
    void                        SetColorTable           ( AtomType datatype );
    void                        SetDefaultSurfaceMode   ();
    bool                        IsSignedData            ()  const   { return MRIDoc->IsScalar ( AtomTypeUseCurrent ); }

    void                        Paint                   ( owl::TDC& dc, bool erase, owl::TRect &rect );
    void                        SetupWindow             ();
    bool                        SetDocTitle             ( LPCTSTR docname, int index );
    void                        DrawMinMax              ( TPointFloat& pos, bool colormin, bool showminmaxcircle, double scale );

    void                        EvSetFocus              ( HWND hwnd )                           {   TBaseView::EvSetFocus  ( hwnd );    }
    void                        EvKillFocus             ( HWND hwnd )                           {   TBaseView::EvKillFocus ( hwnd );    }
    void                        EvKeyDown               ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                        EvLButtonDown           ( owl::uint, const owl::TPoint &p );
    void                        EvLButtonUp             ( owl::uint, const owl::TPoint &p );
    void                        EvMButtonDown           ( owl::uint, const owl::TPoint &p );
    void                        EvMButtonUp             ( owl::uint, const owl::TPoint &p );
    void                        EvRButtonDown           ( owl::uint, const owl::TPoint &p );
    void                        EvRButtonUp             ( owl::uint, const owl::TPoint &p );
    void                        EvMouseMove             ( owl::uint, const owl::TPoint &p );
    void                        EvSize                  ( owl::uint, const owl::TSize& );
    void                        EvTimer                 ( owl::uint timerId );
    void                        EvMouseWheel            ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

    bool                        VnViewUpdated           ( TBaseView *view );

    void                        Cm2Object               ()          { TBaseView::Cm2Object   (); }
    void                        CmMagnifier             ()          { TBaseView::CmMagnifier (); }

    void                        CmSetRenderingMode      ();
    void                        CmSetCutPlane           ( owlwparam wparam );
    void                        CmShiftCutPlane         ( owlwparam wparam, bool forward );
//  void                        CmSetVolumeEnable       ( owl::TCommandEnabler &tce );
    void                        CmFlip                  ( owlwparam wparam );
//  void                        CmFlipEnable            ( owl::TCommandEnabler &tce );
    void                        CmSetNewOrigin          ();
    void                        CmNewIsoSurfaceCut      ();
    void                        CmNewDownsampling       ();
    void                        CmFilter                ( owlwparam w );
    void                        CmOperation             ( owlwparam w );
    void                        CmOutputHistogram       ( owlwparam w );
    void                        CmShowMinMax            ( owlwparam wparam );
    void                        CmIsScalarEnable        ( owl::TCommandEnabler &tce );
    void                        CmSetFindMinMax         ();
    void                        CmSetBrightness         ( owlwparam wparam );
    void                        CmSetContrast           ( owlwparam wparam );
    void                        CmNextColorTable        ();
    void                        CmSetScalingAdapt       ();
    void                        CmSetIntensityLevelEnable ( owl::TCommandEnabler &tce );
    void                        CmIsoMode               ( owlwparam wparam );
    void                        CmSetShiftDepthRange    ( owlwparam wparam );
    void                        CmSetSliceMode          ();
    void                        CmSetNumSlices          ( owlwparam wparam );
    void                        CmMoveSlice             ( owlwparam wparam );
    void                        CmSliceModeEnable       ( owl::TCommandEnabler &tce );
    void                        CmOrient                ();
    void                        CmSetOrientation        ();
    void                        CmResetOrientation      ();
    void                        CmResetOrientationEnable( owl::TCommandEnabler &tce );
    void                        CmSetSurfaceMode        ();
    void                        CmSetEditing            ();
    void                        CmCoregistration        ();
    void                        CmPreprocessMris        ();

    DECLARE_RESPONSE_TABLE (TVolumeView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
