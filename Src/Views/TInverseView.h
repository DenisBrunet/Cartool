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

#pragma once

#include    "TTracksDoc.h"
#include    "TVolumeDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "TInverseMatrixDoc.h"

#include    "TSecondaryView.h"
#include    "TBaseVolumeView.h"
#include    "TSolutionPointsView.h"
#include    "TVolumeView.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // default window size
constexpr double    InverseSpaceBetween         = 0.15;

constexpr int       InverseNumSavedStates       = 64;

constexpr int       InverseNumOnionLayers       = 12;


enum    {
        INVERSE_CBG_SEP0            = NumBaseSecondViewButtons,
        INVERSE_CBG_SHOW2DIS,
        INVERSE_CBG_SHOW3DIS,
        INVERSE_CBG_SHOWVECTORSIS,
        INVERSE_CBG_SEP1,
        INVERSE_CBG_SHOW3DMRI,
        INVERSE_CBG_SHOWSP,
        INVERSE_CBG_SEP2,
        INVERSE_CBG_NEXTROI,
//      INVERSE_CBG_AVERAGEROIS,
        INVERSE_CBG_SEP2A,
        INVERSE_CBG_CUTPLANECORONAL,
        INVERSE_CBG_CUTPLANETRANSVERSE,
        INVERSE_CBG_CUTPLANESAGITTAL,
        INVERSE_CBG_SEP3,
        INVERSE_CBG_PLANEMODE,
        INVERSE_CBG_LESSSLICES,
        INVERSE_CBG_MORESLICES,
        INVERSE_CBG_FIRSTSLICEFWD,
        INVERSE_CBG_FIRSTSLICEBWD,
        INVERSE_CBG_LASTSLICEFWD,
        INVERSE_CBG_LASTSLICEBWD,
        INVERSE_CBG_SEP4,
        INVERSE_CBG_SHMIN,
        INVERSE_CBG_SHMAX,
        INVERSE_CBG_FINDMINMAX,
        INVERSE_CBG_SEP5,
        INVERSE_CBG_DECBR,
        INVERSE_CBG_INCBR,
        INVERSE_CBG_DECCR,
        INVERSE_CBG_INCCR,
        INVERSE_CBG_SEP6,
        INVERSE_CBG_FXDSCL,
        INVERSE_CBG_COLOR,
        INVERSE_CBG_SEP7,
        INVERSE_CBG_RESETREG,
        INVERSE_CBG_PREVREG,
        INVERSE_CBG_NEXTREG,
        INVERSE_CBG_SEARCHBESTREG,
        INVERSE_CBG_SEP8,
        INVERSE_CBG_NEXTIS,
        INVERSE_CBG_NEXTMRI,
        INVERSE_CBG_NUM
        };


//----------------------------------------------------------------------------

enum    InverseRendering2D
        {
        InverseRendering2DNone,
        InverseRendering2DOvercast,
        InverseRendering2DTransparent,
        InverseRendering2DOpaque,

        NumInverseRendering2D,

        InverseRendering2DDefault       = InverseRendering2DOvercast
        };


enum    InverseRendering3D
        {
        InverseRendering3DNone,
        InverseRendering3DSolPoints,
        InverseRendering3DIsosurface,
        InverseRendering3DIsoOnion,

        NumInverseRendering3D,

        InverseRendering3DDefault       = InverseRendering3DNone
        };


enum    InverseRenderingVectors
        {
        InverseRenderingVectorsNone,
        InverseRenderingVectors2D,
        InverseRenderingVectors3D,

        NumInverseRenderingVectors,

        InverseRenderingVectorsDefault  = InverseRenderingVectorsNone
        };


enum    InverseMinMaxType
        {
        InverseMinMaxNone,
        InverseMinMaxCircle,
        InverseMinMaxCross,

        NumInverseMinMax,
        };


//----------------------------------------------------------------------------
                                        // Class to save and restore MRI display states
class   TInverseVolumeState
{
public:
                            TInverseVolumeState ();


    InverseRendering2D      Show2DIs;
    InverseRendering3D      Show3DIs;
    InverseRenderingVectors ShowVectorsIs;
    MriRendering            Show3DMri;
    PointsRendering         ShowSp;

    TGLClipPlane            ClipPlane[ 3 ];

    TGLMatrix               ModelRotMatrix;
};


//----------------------------------------------------------------------------
                                        // Class to save and restore Inverse display states
class   TInverseState
{
public:
                            TInverseState ()        { Reset (); }


    TDisplayScaling         DisplayScaling;
    int                     CurrReg;
    double                  IsSpreading;


    bool                    IsAllocated ()  const   { return  DisplayScaling.ScalingLimitMax >= 0; }

    void                    Reset ();
};


//----------------------------------------------------------------------------
                                        // Class used to display the ESI / sources localization / inverse solutions
                                        // It functionally borrows a lot to the plain volumes display, with additional
                                        // capabilities:
                                        //  - Superimposing the results as 2D layers on top of the MRI slices
                                        //  - 3D display of the results, as small spheres or isosurfaces
                                        //  - It depends on a master EEG view to receive some data, as per TPotentialsView
                                        //  - Views can sync across others
class   TInverseView    :   public  TSecondaryView,
                            public  TBaseVolumeView
{
public:
                            TInverseView ( TTracksDoc &doc, owl::TWindow *parent = 0, TLinkManyDoc *group=0 );


    static const char*      StaticName              ()                      { return "&Sources Localization"; }
    const char*             GetViewName             ()                      { return StaticName(); }


    void                    GLPaint                 ( int how, int renderingmode, TGLClipPlane *otherclipplane );
    void                    HintsPaint              ();
    bool                    IsRenderingMode         ( int renderingmode );


    const TBaseDoc*         GetGeometryDoc          ()              const   { return MRIDocBackg ? MRIDocBackg : BaseDoc; }     // ESI has no geometry info in itself, let's delegate to the background MRI instead
    bool                    ModifyPickingInfo       ( TPointFloat& Picking, char *buff );


protected:

    TSolutionPointsDoc*     SPDoc;
    TInverseResults*        ISDoc;
    TVolumeDoc*             MRIDocClipp;        // the mask for interpolation
    TVolumeDoc*             MRIDocBackg;        // a background MRI for cosmetic effect
    int                     CurrReg;
    int                     CurrIs;
    int                     CurrRis;
    bool                    OnlyRis;
    int                     CurrMri;
    TRois*                  Rois;
    int                     CurrRois;


    TVector<float>          InvBuffS;
//  TVector<float>          InvBuffSSum;
    TArray1<TVector3Float>  InvBuffV;
    TArray2<float>          RisBuff;
    Volume                  SPVol;              // local volume buffer for isosurface computation


    TSelection              SelSP;              // currently displayed solution points
    TSelection              Highlighted;

    bool                    IsSignedData;
    InverseMinMaxType       ShowMin;
    InverseMinMaxType       ShowMax;
    InverseRendering2D      Show2DIs;
    InverseRendering3D      Show3DIs;
    InverseRenderingVectors ShowVectorsIs;
    MriRendering            Show3DMri;
    PointsRendering         ShowSp;
    bool                    IsRoiMode;
    bool                    FindMinMax;
    double                  IsSpreading;        // how much variation is in the data
    TAveragingPrecedence    AveragingPrecedence;


    int                     GalMinIndex;
    int                     GalMaxIndex;
    double                  GalMinValue;
    double                  GalMaxValue;
    TPointFloat             GalMinPos;
    TPointFloat             GalMaxPos;
    TGLMatrix               AntiModelRotMatrix;


    TInverseVolumeState     SavedState;                                 // volume display state
    TInverseState           SavedIsState    [ InverseNumSavedStates ];  // inverse display state

                                        // OpenGL
    TGLMaterial<GLfloat>    MaterialIs;
    TTriangleSurface        SurfaceISPos    [ InverseNumOnionLayers ];
    TTriangleSurface        SurfaceISNeg    [ InverseNumOnionLayers ];
    TTriangleSurfaceIsoParam  IsoParam;
    long                    RedoSurfaceIS;
//  TGLVolume               VolumeMri;  // it seems we don't need this cache variable now, we can safely call the volume display
    TGLVolume               VolumeIs;

                                        // TDisplayScaling
    void                    SetScaling              ( double scaling );
    void                    SetScaling              ( double negv, double posv, bool forcesymetric = true );

    bool                    ValidView               ();
    inline int              NumVisiblePlanes        ();
    long                    SearchAndSetIntensity   ( bool precise = false ); // returns the TF position
    void                    SetSliceSize            ();
    void                    SetShowSlices           ( bool newtype );
    void                    SetTotalSlices          ();
    void                    SetItemsInWindow        ();
    void                    FitItemsInWindow        ( int numitems, owl::TSize itemsize, int &byx, int &byy, owl::TRect *winrect = 0 );
    void                    UpdateScaling           ();
    void                    SetColorTable           ( AtomType datatype );
    void                    InitMri                 ();
    void                    ReloadRoi               ();
//  void                    FilterIs                ();
    void                    GetInverse              ( int tf1, int tf2 );

    void                    Paint                   ( owl::TDC& dc, bool erase, owl::TRect& rect );
    void                    SetupWindow             ();
    void                    DrawMinMax              ( TPointFloat& pos, bool colormin, bool showminmaxcircle, double scale );

    bool                    VnViewUpdated           ( TBaseView *view );

    void                    EvSetFocus              ( HWND hwnd )                               {           TBaseView::EvSetFocus         ( hwnd );                       }
    void                    EvKillFocus             ( HWND hwnd )                               {           TBaseView::EvKillFocus        ( hwnd );                       }
    void                    EvKeyDown               ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                    EvKeyUp                 ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                    EvLButtonDblClk         ( owl::uint, const owl::TPoint & );
    void                    EvLButtonDown           ( owl::uint, const owl::TPoint &p );
    void                    EvLButtonUp             ( owl::uint, const owl::TPoint &p );
    void                    EvMButtonDown           ( owl::uint, const owl::TPoint &p );
    void                    EvMButtonUp             ( owl::uint, const owl::TPoint &p );
    void                    EvRButtonDown           ( owl::uint, const owl::TPoint &p );
    void                    EvRButtonUp             ( owl::uint, const owl::TPoint &p );
    void                    EvMouseMove             ( owl::uint, const owl::TPoint &p );
    void                    EvSize                  ( owl::uint, const owl::TSize& );
    void                    EvTimer                 ( owl::uint timerId );
    void                    EvMouseWheel            ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

    void                    Cm2Object               ()  { TBaseView::Cm2Object   ();                }
    void                    CmMagnifier             ()  { TBaseView::CmMagnifier ();                }
    void                    CmShowSequenceLabels    ()  { TSecondaryView::CmShowSequenceLabels (); }

    void                    CmSetCutPlane           ( owlwparam wparam );
    void                    CmShiftCutPlane         ( owlwparam wparam, bool forward );
    void                    CmSetCutPlaneEnable     ( owl::TCommandEnabler &tce );
    void                    CmOrient                ();

    void                    CmSetShow2DIs           ( owlwparam wparam );
    void                    CmSetShow3DIs           ();
    void                    CmSetShowVectorsIs      ();
    void                    CmSetShow3DMri          ();
    double                  GetRegularizationRatio  ( int reg1, int reg2 );
    void                    CmReg                   ( owlwparam w );
    void                    CmNextIs                ();
    void                    CmNextMri               ();
    void                    CmSetSliceMode          ();
    void                    CmSetNumSlices          ( owlwparam wparam );
    void                    CmMoveSlice             ( owlwparam wparam );
    void                    CmSetStepTF             ( owlwparam wparam );
    void                    CmShowMinMax            ( owlwparam wparam );
    void                    CmSetBrightness         ( owlwparam wparam );
    void                    CmSetContrast           ( owlwparam wparam );
    void                    CmSetManageRangeCursor  ( owlwparam wparam );
    void                    CmSetScalingAdapt       ();
    void                    CmNextColorTable        ();
    void                    CmSetFindMinMax         ();
    void                    CmSearchTFMax           ();
    void                    CmSearchRegularization  ( owlwparam w );
    void                    CmSetShowSP             ();
    void                    CmShowAll               ( owlwparam w );
    void                    CmShowColorScale        ();
    void                    CmShowOrientation       ();
    void                    CmSetShiftDepthRange    ( owlwparam wparam );
    void                    CmSetIntensityLevelEnable ( owl::TCommandEnabler &tce );
    void                    CmIsEnable              ( owl::TCommandEnabler &tce );
    void                    CmMriEnable             ( owl::TCommandEnabler &tce );
    void                    CmNextMriEnable         ( owl::TCommandEnabler &tce );
    void                    CmNextRegEnable         ( owl::TCommandEnabler &tce );
    void                    CmNextIsEnable          ( owl::TCommandEnabler &tce );
    void                    CmIsMriEnable           ( owl::TCommandEnabler &tce );
    void                    CmSlicesEnable          ( owl::TCommandEnabler &tce );
    void                    CmIsScalarEnable        ( owl::TCommandEnabler &tce );
    void                    CmSliceModeEnable       ( owl::TCommandEnabler &tce );
    void                    CmNotSliceModeEnable    ( owl::TCommandEnabler &tce );
    void                    CmSetAveragingMode      ( owlwparam w );
    void                    CmNextRois              ();
    void                    CmNextRoisEnable        ( owl::TCommandEnabler &tce );
    void                    CmPrecedenceBeforeEnable( owl::TCommandEnabler &tce );
    void                    CmPrecedenceAfterEnable ( owl::TCommandEnabler &tce );

    virtual bool            VnNewTFCursor           ( TTFCursor *tfcursor );
    virtual bool            VnReloadData            ( int what );
    virtual bool            VnNewHighlighted        ( TSelection *sel );

    DECLARE_RESPONSE_TABLE (TInverseView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
