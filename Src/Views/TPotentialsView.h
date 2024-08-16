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

#include    "TTracks.h"
#include    "TTracksDoc.h"
#include    "TLinkManyDoc.h"

#include    "TSecondaryView.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    PotentialsSpaceBetween      = 1.1;

constexpr double    ExtraSizeMapSequence        = 1.15;
constexpr double    ExtraDepthFlatView          = 50;

constexpr int       PotentialsNumMinMax         = 3;


enum    {
        POTENTIALS_CBG_SEP1A        = NumBaseSecondViewButtons,
        POTENTIALS_CBG_SHOWELEC,
//      POTENTIALS_CBG_SHOWTRI,
        POTENTIALS_CBG_SHOWELNAMES,
        POTENTIALS_CBG_SHOWTRACKS,
        POTENTIALS_CBG_SEP2,
        POTENTIALS_CBG_FLATVIEW,
//      POTENTIALS_CBG_SHOWZERO,
        POTENTIALS_CBG_SHOWPMMINMAX,
        POTENTIALS_CBG_SEP3,
        POTENTIALS_CBG_DECBR,
        POTENTIALS_CBG_INCBR,
        POTENTIALS_CBG_DECCR,
        POTENTIALS_CBG_INCCR,
        POTENTIALS_CBG_SEP4,
        POTENTIALS_CBG_FXDSCL,
        POTENTIALS_CBG_SPCOLOR,
        POTENTIALS_CBG_SEP5,
        POTENTIALS_CBG_NEXTXYZ,
        POTENTIALS_CBG_NUM
        };

                                        // Controlling the potentials surface rendering
enum    PotentialsRendering
        {
        PotentialsRenderingNone,
        PotentialsRenderingSpheres,
        PotentialsRenderingTransparent,
        PotentialsRenderingOvercast,
        PotentialsRenderingSolid,

        NumPotentialsRenderings
        };


//----------------------------------------------------------------------------

class   TElectrodesDoc;


class   TPotentialsView :   public  TSecondaryView
{
public:
                            TPotentialsView ( TTracksDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group=0 );


    static const char*      StaticName              ()                      { return "&Potentials Display"; }
    const char*             GetViewName             ()                      { return StaticName(); }

    void                    CreateGadgets           ();


    bool                    VnNewTFCursor           ( TTFCursor *tfcursor );
    bool                    VnReloadData            ( int what );
    virtual bool            VnNewHighlighted        ( TSelection *sel );

    void                    GLPaint                 ( int how, int renderingmode, TGLClipPlane *otherclipplane );
    bool                    IsRenderingMode         ( int renderingmode )   const;


    const TBaseDoc*         GetGeometryDoc          ()              const   { return XYZDoc ? XYZDoc : BaseDoc; }   // Potentials display has no geometry info in itself, let's delegate to the background XYZ instead
    bool                    ModifyPickingInfo       ( TPointFloat& Picking, char *buff );
    int                     GetCurrXyz              ()              const   { return CurrXyz; }


protected:

    TElectrodesDoc*         XYZDoc;
    int                     CurrXyz;

    TTracks<float>          EegBuff;
    TSelection              SelTracks;
    TSelection              Highlighted;
    TSelection              BadTracks;
    TSelection              AuxTracks;
    int                     slicesPerX;
    int                     slicesPerY;
    bool                    ShowZero;
    int                     ShowMinMax;
    int                     ShowElectrodes;
    bool                    ShowTracks;
    bool                    ShowTriangles;

    int                     GalMinIndex;
    int                     GalMaxIndex;
    double                  GalMinValue;
    double                  GalMaxValue;
    TPointFloat             GalMinPos;
    TPointFloat             GalMaxPos;


                                        // OpenGL
    TPointFloat             ProjModelCenter;
    GLfloat                 ProjModelRadius;    // used for depth / fog
    TGLMaterial<GLfloat>    MaterialEl;
    TGLMaterial<GLfloat>    MaterialEl2;
    TGLMaterial<GLfloat>    MaterialElBad;
    TGLMaterial<GLfloat>    MaterialElSel;


    void                    GetTracks               ( int tf1, int tf2 );
    bool                    ValidView               ()  const       { return GODoc && GODoc->GetNumXyzDoc() && EEGDoc->GetNumTimeFrames(); }
    long                    SearchAndSetIntensity   ( bool precise = false );
    void                    Average                 ();
    void                    SetItemsInWindow        ();
    void                    FitItemsInWindow        ( int numitems, owl::TSize itemsize, int &byx, int &byy, owl::TRect *winrect = 0 );
    void                    InitXyz                 ();

    void                    Paint                   ( owl::TDC& dc, bool erase, owl::TRect& rect );
    void                    SetupWindow             ();

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

    bool                    VnNewBadSelection       ( TSelection *sel );
    bool                    VnViewUpdated           ( TBaseView *view );

    void                    Cm2Object               ()          { TBaseView::Cm2Object   ();                }
    void                    CmMagnifier             ()          { TBaseView::CmMagnifier ();                }
    void                    CmShowSequenceLabels    ()          { TSecondaryView::CmShowSequenceLabels (); }

    void                    CmOrient                ();
    void                    CmSetBrightness         ( owlwparam wparam );
    void                    CmSetContrast           ( owlwparam wparam );
    void                    CmSetIntensityLevelEnable ( owl::TCommandEnabler &tce );
    void                    CmSetShowElectrodes     ();
    void                    CmSetShowTriangles      ();
    void                    CmSetShowElectrodesNames();
    void                    CmNextColorTable        ();
    void                    CmSetShowZero           ();
    void                    CmSetShowMinMax         ();
    void                    CmSetStepTF             ( owlwparam wparam );
    void                    CmNextXyz               ();
    void                    CmXyzEnable             ( owl::TCommandEnabler &tce );
    void                    CmSetScalingAdapt       ();
    void                    CmSetInterpolation      ( owlwparam wparam );
//  void                    CmSetExtrapolation      ( owlwparam wparam );
    void                    CmSearchTFMax           ();
    void                    CmSetManageRangeCursor  ( owlwparam wparam );
    void                    CmSetRenderingMode      ();
    void                    CmSetFlatView           ();
    void                    CmSetFlatViewEnable     ( owl::TCommandEnabler &tce );
    void                    CmShowAll               ( owlwparam w );
    void                    CmShowColorScale        ();
    void                    CmShowAxis              ();
    void                    CmSetShiftDepthRange    ( owlwparam wparam );
    void                    CmSetShowTracks         ();

    DECLARE_RESPONSE_TABLE (TPotentialsView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
