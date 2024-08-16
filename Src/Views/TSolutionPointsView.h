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
    const char*                 GetViewName         ()                  const   { return StaticName (); }

    void                        CreateGadgets       ();


    TSolutionPointsDoc*         GetSPDoc            ()                          { return SPDoc; }
    void                        GetHighlighted      ( TSelection* s )   const   { s->Copy ( 0, SPDoc->GetNumSolPoints() - 1, &Highlighted ); }
    const TStrings*             GetSPNames          ()                  const   { return SPDoc->GetSPNames (); }


    void                        GLPaint             ( int how, int renderingmode, TGLClipPlane* otherclipplane );
    bool                        IsRenderingMode     ( int renderingmode )       { return renderingmode == RenderingOpaque; }

    bool                        ModifyPickingInfo   ( TPointFloat& Picking, char *buff );
    virtual bool                VnNewHighlighted    ( TSelection *sel );


protected:

    TSolutionPointsDoc*         SPDoc;
    TSelection                  Highlighted;

                                        // OpenGL
    TGLMaterial<GLfloat>        MaterialSp;
    TGLMaterial<GLfloat>        MaterialSp2;
    TGLMaterial<GLfloat>        MaterialHi;


    void                        SetupWindow         ();
    void                        Paint               ( owl::TDC& dc, bool erase, owl::TRect& rect );
    void                        EvSetFocus          ( HWND hwnd )                               { TBaseView::EvSetFocus  ( hwnd );  }
    void                        EvKillFocus         ( HWND hwnd )                               { TBaseView::EvKillFocus ( hwnd );  }
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

    void                        Cm2Object           ()                          { TBaseView::Cm2Object   (); }
    void                        CmOrient            ()                          { TBaseView::CmOrient    (); }
    void                        CmMagnifier         ()                          { TBaseView::CmMagnifier (); }

    void                        CmSetRenderingMode  ();
    void                        CmSetCutPlane       ( owlwparam wparam );
    void                        CmSetCutPlaneMode   ();
    void                        CmSetShiftDepthRange( owlwparam wparam );
    void                        CmSetShowNames      ();
    void                        CmExportText        ()                          { SPDoc->ExportText(); }

    DECLARE_RESPONSE_TABLE (TSolutionPointsView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
