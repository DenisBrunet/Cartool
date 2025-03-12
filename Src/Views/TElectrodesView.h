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

#include    "TBaseDoc.h"
#include    "TElectrodesDoc.h"
#include    "TBaseView.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    ExtraSizeXyz3D              = 1.3;
constexpr double    FogXyzNear                  = FogDefaultNear;
constexpr double    FogXyzFar                   = 1.75;
                                        // points are roughly hexagonal, this is the half height of an equilateral triangle
constexpr double    ElectrodeMinDistanceRatio   = 0.433;


enum    {
        XYZGLVIEW_CBG_SEP1   = NumBaseViewButtons,
        XYZGLVIEW_CBG_SHOWELEC,
        XYZGLVIEW_CBG_SHOWELNAMES,
        XYZGLVIEW_CBG_FLATVIEW,
//      XYZWINVIEWFLAT_CBG_SEP1,
//      XYZWINVIEWFLAT_CBG_MONTAGE,
        XYZGLVIEW_CBG_SEP2,
        XYZGLVIEW_CBG_CUTPLANEX,
        XYZGLVIEW_CBG_CUTPLANEY,
        XYZGLVIEW_CBG_CUTPLANEZ,

//      XYZGLVIEW_CBG_SEP2,
//      XYZGLVIEW_CBG_FLIPX,
//      XYZGLVIEW_CBG_FLIPY,
//      XYZGLVIEW_CBG_FLIPZ,
//      XYZGLVIEW_CBG_SEP3,
//      XYZGLVIEW_CBG_FLIPXY,
//      XYZGLVIEW_CBG_FLIPYZ,
//      XYZGLVIEW_CBG_FLIPXZ,

        XYZGLVIEW_CBG_NUM
        };

enum    {
        XyzNone             = 0,
        XyzLines,
        XyzTransparentFacet,
        XyzTransparentSmooth,
        XyzSolidFacet,
        XyzSolidSmooth,
        XyzNumRendering
        };


//----------------------------------------------------------------------------
                                        // Displaying electrodes in 3D and 2D
class   TElectrodesView :   public  TBaseView
{
public:
                            TElectrodesView ( TElectrodesDoc& doc, owl::TWindow *parent = 0, TLinkManyDoc *group = 0 );
                           ~TElectrodesView ();


    static const char*      StaticName          ()                          { return "&Electrodes Display"; }
    const char*             GetViewName         ()                  final   { return StaticName (); }

    void                    CreateGadgets       ()                  final;


    TElectrodesDoc*         GetXYZDoc           ()                          { return XYZDoc; }
    void                    GetHighlighted      ( TSelection *s )   const   { s->Copy ( 0, XYZDoc->GetNumElectrodes() - 1, &Highlighted ); }
    const TStrings*         GetElectrodesNames  ()                  const   { return XYZDoc->GetElectrodesNames (); }


    void                    GLPaint             ( int how, int renderingmode, TGLClipPlane* otherclipplane )    final;
    bool                    IsRenderingMode     ( int renderingmode )                                           final;


    bool                    ModifyPickingInfo   ( TPointFloat& Picking, char* buff )                            final;
    virtual bool            VnNewHighlighted    ( const TSelection* sel );


protected:

    TElectrodesDoc*         XYZDoc;
    int                     ShowElectrodes;
    int                     mtgNum;
    int*                    mtgFrom;
    int*                    mtgTo;
    int                     mtgCurrent;
    TSelection              Bad;
    TSelection              Highlighted;

                                        // OpenGL
    TPointFloat             ProjModelCenter;
    GLdouble                ProjModelRadius;    // used for depth / fog
    TGLMaterial<GLfloat>    MaterialEl;
    TGLMaterial<GLfloat>    MaterialEl2;
    TGLMaterial<GLfloat>    MaterialElBad;
    TGLMaterial<GLfloat>    MaterialElSel;
    TGLMaterial<GLfloat>    MaterialTri;
    TGLMaterial<GLfloat>    MaterialTri2;


    bool                    ValidView           ()  final                                   { return XYZDoc->GetNumElectrodes(); }

    void                    Paint               ( owl::TDC& dc, bool erase, owl::TRect& rect )  final;
    void                    SetupWindow         ()                                              final;

    using        TBaseView::EvSetFocus;
    using        TBaseView::EvKillFocus;
    void                    EvKeyDown           ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                    EvLButtonDown       ( owl::uint, const owl::TPoint& p );
    void                    EvLButtonUp         ( owl::uint, const owl::TPoint& p );
    void                    EvMButtonDown       ( owl::uint, const owl::TPoint& p );
    void                    EvMButtonUp         ( owl::uint, const owl::TPoint& p );
    void                    EvRButtonDown       ( owl::uint, const owl::TPoint& p );
    void                    EvRButtonUp         ( owl::uint, const owl::TPoint& p );
    void                    EvMouseMove         ( owl::uint, const owl::TPoint& p );
    void                    EvTimer             ( owl::uint timerId );
//  void                    EvCaptureChanged    ( HWND );
    void                    EvMouseWheel        ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

    bool                    VnNewBadSelection   ( const TSelection *sel );

    using        TBaseView::Cm2Object;
    using        TBaseView::CmMagnifier;

    void                    CmOrient            ()  final;
    void                    CmFlip              ( owlwparam wparam );
//  void                    CmFlipEnable        ( owl::TCommandEnabler &tce );
    void                    CmSetShowElectrodes ();
    void                    CmSetShowElectrodesNames ();
    void                    CmSetShowTriangles  ();
    void                    CmSetCutPlane       ( owlwparam wparam );
    void                    CmSetFlatView       ();
    void                    CmSetFlatViewEnable ( owl::TCommandEnabler &tce );
    void                    CmSetShiftDepthRange( owlwparam w ) final;
    void                    CmExportXyz         ( owlwparam w );
    void                    CmExportDelaunay    ();

    DECLARE_RESPONSE_TABLE (TElectrodesView);
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
