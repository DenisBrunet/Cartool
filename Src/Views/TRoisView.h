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
    const char*         GetViewName         ()  final   { return StaticName(); }

    void                CreateGadgets       ()  final;

    void                GLPaint             ( int how, int renderingmode, TGLClipPlane *otherclipplane )    final;


protected:

    TRoisDoc*           ROIDoc;

    bool                ValidView           ()  final   { return true; } // ISDoc->GetNumElectrodes(); }

    void                Paint               ( owl::TDC& dc, bool erase, owl::TRect& rect )  final;

                                    // OwlNext wants explicit message handlers from derived class
    using    TBaseView::EvGetMinMaxInfo;
    using    TBaseView::EvSize;
    using    TBaseView::EvKeyDown;
    using    TBaseView::EvKeyUp;
    using    TBaseView::EvEraseBkgnd;
    using    TBaseView::EvSetFocus;
    using    TBaseView::EvKillFocus;
    using    TBaseView::VnViewDestroyed;


    DECLARE_RESPONSE_TABLE (TRoisView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
