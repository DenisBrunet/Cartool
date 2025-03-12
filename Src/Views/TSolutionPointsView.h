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

#include    "TBaseView.h"
#include    "TSolutionPointsDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    {
        SPGLVIEW_CBG_SEP1   = NumBaseViewButtons,
        SPGLVIEW_CBG_SHOWNAMES,
        SPGLVIEW_CBG_SEP2,
        SPGLVIEW_CBG_CUTPLANEX,
        SPGLVIEW_CBG_CUTPLANEY,
        SPGLVIEW_CBG_CUTPLANEZ,
        SPGLVIEW_CBG_NUM
        };

                                        // Solution Points display
class   TSolutionPointsView :   public  TBaseView
{
public:
                                TSolutionPointsView ( TSolutionPointsDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group = 0 );


    static const char*          StaticName          ()                          { return "&Solution Points"; }
    const char*                 GetViewName         ()                  final   { return StaticName (); }

    void                        CreateGadgets       ()                  final;


    TSolutionPointsDoc*         GetSPDoc            ()                          { return SPDoc; }
    void                        GetHighlighted      ( TSelection* s )   const   { s->Copy ( 0, SPDoc->GetNumSolPoints() - 1, &Highlighted ); }
    const TStrings*             GetSPNames          ()                  const   { return SPDoc->GetSPNames (); }


    void                        GLPaint             ( int how, int renderingmode, TGLClipPlane* otherclipplane )    final;
    bool                        IsRenderingMode     ( int renderingmode )                                           final   { return renderingmode == RenderingOpaque; }

    bool                        ModifyPickingInfo   ( TPointFloat& Picking, char *buff )                            final;
    virtual bool                VnNewHighlighted    ( const TSelection *sel );


protected:

    TSolutionPointsDoc*         SPDoc;
    TSelection                  Highlighted;

                                        // OpenGL
    TGLMaterial<GLfloat>        MaterialSp;
    TGLMaterial<GLfloat>        MaterialSp2;
    TGLMaterial<GLfloat>        MaterialHi;


    void                        SetupWindow         ();
    void                        Paint               ( owl::TDC& dc, bool erase, owl::TRect& rect )  final;
    using            TBaseView::EvSetFocus;
    using            TBaseView::EvKillFocus;
    void                        EvKeyDown           ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                        EvLButtonDown       ( owl::uint, const owl::TPoint &p );
    void                        EvLButtonUp         ( owl::uint, const owl::TPoint &p );
    void                        EvMButtonDown       ( owl::uint, const owl::TPoint &p );
    void                        EvMButtonUp         ( owl::uint, const owl::TPoint &p );
    void                        EvRButtonDown       ( owl::uint, const owl::TPoint &p );
    void                        EvRButtonUp         ( owl::uint, const owl::TPoint &p );
    void                        EvMouseMove         ( owl::uint, const owl::TPoint &p );
    void                        EvTimer             ( owl::uint timerId );
    void                        EvMouseWheel        ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

    using            TBaseView::Cm2Object;
    using            TBaseView::CmOrient;
    using            TBaseView::CmMagnifier;

    void                        CmSetRenderingMode  ()  final;
    void                        CmSetCutPlane       ( owlwparam wparam );
    void                        CmSetCutPlaneMode   ();
    void                        CmSetShiftDepthRange( owlwparam wparam )    final;
    void                        CmSetShowNames      ();
    void                        CmExportText        ()                          { SPDoc->ExportText(); }

    DECLARE_RESPONSE_TABLE (TSolutionPointsView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
