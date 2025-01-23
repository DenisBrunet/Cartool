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
#include    "TTFCursor.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // default window size
constexpr double    BASESECONDVIEW_COLORSCALEPOSV   = 0.88;
constexpr double    BASESECONDVIEW_COLORSCALEWIDTH  = 0.4;
constexpr double    BASESECONDVIEW_COLORSCALEHEIGHT = 0.025;


enum    {
        MRCAverage          = 0x1,
        MRCSequence         = 0x2,
        MRCSequenceOverlay  = 0x4,
        MRCAnySequence      = MRCSequence | MRCSequenceOverlay,
        MRCAnimation        = 0x8
        };

constexpr int       MRCStepInit                 = 8;

                                        // common buttons
enum    {
        BaseSecondViewButtonSeparatorA  = NumBaseViewButtons,
        BaseSecondViewButtonSyncViews,
        BaseSecondViewButtonSeparatorB,
        BaseSecondViewButtonRangeAverage,
        BaseSecondViewButtonRangeSequence,
        BaseSecondViewButtonRangeAnimation,
        BaseSecondViewButtonSeparatorC,
        BaseSecondViewButtonRangeStepInc,
        BaseSecondViewButtonRangeStepDec,

        NumBaseSecondViewButtons
        };


//----------------------------------------------------------------------------
                                        // Secondary views depends on a primary EEG view that will feed them some data to display
                                        // Currently secondary views are potentials (maps) or ESI / Inverse
                                        // These views also have extended synchronization capabilities, they can be all synced
                                        // within a group of documents (through TLMDoc), or even between any arbitrary (secondary) windows
class   TTracksView;


class   TSecondaryView  :   public  TBaseView
{
public:
                    TSecondaryView  ( TTracksDoc &doc, owl::TWindow *parent = 0, TLinkManyDoc *group=0 );


    virtual bool    VnViewSync      ( TTFCursor *tfcursor );
                                        // dynamically return the current eeg view associated
    TTracksView *   GetEegView      ();


protected:

    TTracksDoc*     EEGDoc;

    TTFCursor       TFCursor;
    TTFCursor       SavedTFCursor;
    int             ManageRangeCursor;
    int             MRCNumTF;
    int             MRCStepTF;
    bool            MRCSequenceOrder;
    bool            MRCDoAnimation;
    bool            ShowSequenceLabels;


    inline bool     IsMRCSequence           ()  const           { return ( ManageRangeCursor & MRCAnySequence ); }


    virtual void    UpdateCaption           ();   // with time of cursor
    void            CreateBaseGadgets       ();

    void            EvKeyDown               ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void            EvSetFocus              ( HWND);
    void            EvKillFocus             ( HWND hwnd )       { TBaseView::EvKillFocus ( hwnd ); }
    void            EvPaletteChanged        ( HWND );
    void            EvLButtonDown           ( owl::uint, const owl::TPoint& p );

    virtual void    CmSyncViews             ();
    virtual void    CmSetManageRangeCursor  ( owlwparam wparam ) = 0;
    virtual void    CmSetStepTF             ( owlwparam wparam );
    virtual void    CmSpecifyScaling        ();
    virtual void    CmShowSequenceLabels    ();

                                        // Transmitting messages to the EegView - not used anymore(?)
//  void            CmToEeg                 ()                              { CartoolDocManager->GetView ( LinkedViewId )->ForwardMessage ( false ); }
//  void            CmToEegEnable           ( owl::TCommandEnabler &tce )   { CartoolDocManager->GetView ( LinkedViewId )->EvCommandEnable (tce); }

    DECLARE_RESPONSE_TABLE(TSecondaryView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
