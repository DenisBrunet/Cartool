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

#include    <owl/scrollba.h>

#include    "Time.TAcceleration.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       EegScrollbarHeight          = 12;
constexpr long      ScrollbarMaxRange           = 0xFFFF;   // OwlNext max range - this will be up to Cartool to do any rescaling...


class   TTracksView;

                                        // Upgrading our horizontal scrollbar
class   TTracksViewScrollbar    :   public  owl::TScrollBar,
                                    public  TAcceleration
{
public:
                    TTracksViewScrollbar ( TTracksView* eegview, owl::TWindow* parent, int id, int x, int y, int w, int h, bool isHScrollBar, owl::TModule* module = 0 );


    void            SBLineUp        ();
    void            SBLineDown      ();
    void            SetPosition     ( int thumbPos, bool redraw = true );

    void            UpdateIt        ();


private:
    TTracksView*    EEGView;
    double          PositionToTimeFrames;   // scaling factor data range <-> scrollbar max range
    bool            RedrawEeg;

    void            EvSetFocus      ( HWND );

    DECLARE_RESPONSE_TABLE (TTracksViewScrollbar);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
