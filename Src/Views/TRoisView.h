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
#include    "TRoisDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    {
        ROISGLVIEW_CBG_NUM  = NumBaseViewButtons
        };


class   TRoisView   :   public  TBaseView
{
public:
                        TRoisView ( TRoisDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group = 0 );


    static const char*  StaticName          ()          { return "&ROIs Display"; }
    const char*         GetViewName         ()          { return StaticName(); }

    void                CreateGadgets       ();

    void                GLPaint             ( int how, int renderingmode, TGLClipPlane *otherclipplane );


protected:

    TRoisDoc*           ROIDoc;

    bool                ValidView           ()          { return true; } // ISDoc->GetNumElectrodes(); }

    void                Paint               ( owl::TDC& dc, bool erase, owl::TRect& rect );

                                    // OwlNext wants explicit message handlers from derived class
    void                EvGetMinMaxInfo     ( MINMAXINFO& minmaxinfo )                                  {           TBaseView::EvGetMinMaxInfo    ( minmaxinfo );                 }
    void                EvSize              ( owl::uint sizetype, const owl::TSize& size )              {           TBaseView::EvSize             ( sizetype, size );             }
    void                EvKeyDown           ( owl::uint key, owl::uint repeatCount, owl::uint flags )   {           TBaseView::EvKeyDown          ( key, repeatCount, flags );    }
    void                EvKeyUp             ( owl::uint key, owl::uint repeatCount, owl::uint flags )   {           TBaseView::EvKeyUp            ( key, repeatCount, flags );    }
    bool                EvEraseBkgnd        ( HDC hdc )                                                 { return    TBaseView::EvEraseBkgnd       ( hdc );                        }
    void                EvSetFocus          ( HWND hwnd )                                               {           TBaseView::EvSetFocus         ( hwnd );                       }
    void                EvKillFocus         ( HWND hwnd )                                               {           TBaseView::EvKillFocus        ( hwnd );                       }
    bool                VnViewDestroyed     ( TBaseView* view )                                         { return    TBaseView::VnViewDestroyed    ( view );                       }


    DECLARE_RESPONSE_TABLE (TRoisView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
