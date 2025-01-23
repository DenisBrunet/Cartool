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

#include    <owl/pch.h>

#include    "TTracksViewScrollbar.h"

#include    "Math.Utils.h"

#include    "TTracksView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1(TTracksViewScrollbar, TScrollBar)

    EV_WM_SETFOCUS,

END_RESPONSE_TABLE;


        TTracksViewScrollbar::TTracksViewScrollbar ( TTracksView* eegview, TWindow* parent, int id, int x, int y, int w, int h, bool isHScrollBar, TModule* module )
      : TScrollBar ( parent, id, x, y, w, h, isHScrollBar, module ), EEGView ( eegview )
{
PositionToTimeFrames        = 1.0;
RedrawEeg                   = true;

DelayForPolling             = TracksScrollBarDelayForPolling;
DelayBeforeAcceleration     = TracksScrollBarDelayBeforeAcceleration;
DelayAfterNoAcceleration    = TracksScrollBarDelayAfterNoAcceleration;
MinSpeed                    = TracksScrollBarMinSpeed;
MaxSpeed                    = TracksScrollBarMaxSpeed;
MinAcceleration             = TracksScrollBarMinAcceleration;
}


//----------------------------------------------------------------------------
                                        // alias LineRight
void    TTracksViewScrollbar::SBLineDown ()
{
//if ( ! VkLButton () )
    StopAccelerating ();

                                        // allow greater latency for big display
DelayForPolling             = 100 + (double) EEGView->CDPt.GetLength() / 5;
DelayBeforeAcceleration     = TracksScrollBarDelayBeforeAcceleration;
DelayAfterNoAcceleration    = TracksScrollBarDelayAfterNoAcceleration;
MinSpeed                    = AtLeast ( (ulong) 1,                          EEGView->CDPt.GetLength()         * 5 / 100 );
MaxSpeed                    = AtLeast ( (ulong) EEGView->CDPt.GetLength(),  EEGView->CDPt.GetTotalLength()    * 5 / 100 );
MinAcceleration             = TracksScrollBarMinAcceleration;


GetSpeed ();

                                        // modify CDP and scroll
RedrawEeg   = false;
//EEGView->ScrollCursor ( GetLineMagnitude() * PositionToTimeFrames + 0.5, true );
EEGView->ScrollDisplay ( Speed );

RedrawEeg   = false;
//SetPosition ( EEGView->CDPt.GetMin() / PositionToTimeFrames + 0.5 );
SetPosition ( EEGView->CDPt.GetMin() / PositionToTimeFrames + 0.5 );
}

                                        // alias LineLeft
void    TTracksViewScrollbar::SBLineUp ()
{
//if ( ! VkLButton () )
    StopAccelerating ();

                                        // allow greater latency for big display
DelayForPolling             = 100 + (double) EEGView->CDPt.GetLength() / 5;
DelayBeforeAcceleration     = TracksScrollBarDelayBeforeAcceleration;
DelayAfterNoAcceleration    = TracksScrollBarDelayAfterNoAcceleration;
MinSpeed                    = AtLeast ( (ulong) 1,                          EEGView->CDPt.GetLength()         * 5 / 100 );
MaxSpeed                    = AtLeast ( (ulong) EEGView->CDPt.GetLength(),  EEGView->CDPt.GetTotalLength()    * 5 / 100 );
MinAcceleration             = TracksScrollBarMinAcceleration;


GetSpeed ();

                                        // modify CDPt and scroll
RedrawEeg   = false;
EEGView->ScrollDisplay ( -Speed );

RedrawEeg   = false;
SetPosition ( EEGView->CDPt.GetMin() / PositionToTimeFrames + 0.5 );
}


//----------------------------------------------------------------------------
void    TTracksViewScrollbar::SetPosition ( int thumbPos, bool redraw )
{
int                 oldRealThumb    = Round ( TScrollBar::GetPosition() * PositionToTimeFrames );
                                        // update position
TScrollBar::SetPosition ( thumbPos, redraw );

/*                                        // don't keep focus, so EEG view can get messages
if ( EEGView->CartoolApplication->LastActiveBaseView )
    EEGView->CartoolApplication->LastActiveBaseView->SetFocus();
else
    EEGView->SetFocus ();
*/
/*                                        // check for create ep/dialog
if ( EEGView->CartoolMainWindow->GetLastActivePopup() )
    GetWindowPtr ( EEGView->CartoolMainWindow->GetLastActivePopup() )->SetFocus();
else
    EEGView->CartoolMainWindow->SetFocus();
*/

long                oldTFMin        = EEGView->CDPt.GetMin();
long                oldTFMax        = EEGView->CDPt.GetMax();
int                 realThumb       = Round ( TScrollBar::GetPosition() * PositionToTimeFrames );

if ( oldRealThumb == realThumb ) {
    RedrawEeg   = true;
    return;
    }


//EEGView->CDPt.SetMin ( realThumb, true );
//EEGView->UpdateBuffers ( oldTFMin, oldTFMax, EEGView->CDPt.GetMin(), EEGView->CDPt.GetMax() );

if ( RedrawEeg ) {

    EEGView->CDPt.SetMin ( realThumb, true );

    EEGView->UpdateBuffers ( oldTFMin, oldTFMax, EEGView->CDPt.GetMin(), EEGView->CDPt.GetMax() );
                                        // need to move the cursor ?
    if ( EEGView->TFCursor.GetPosMax() > EEGView->CDPt.GetMax()
      || EEGView->TFCursor.GetPosMin() < EEGView->CDPt.GetMin() ) {

        if ( EEGView->TFCursor.IsExtending() )
            EEGView->TFCursor.SetExtendingPos ( EEGView->TFCursor.GetExtendingPos() + EEGView->CDPt.GetMin() - oldTFMin );
        else
            EEGView->TFCursor.ShiftPos ( EEGView->CDPt.GetMin() - oldTFMin );

        EEGView->EEGDoc->NotifyFriendViews ( EEGView->LinkedViewId, vnNewTFCursor, (TParam2) &EEGView->TFCursor, EEGView );
        }

//  EEGView->Invalidate ( false );
    EEGView->ShowNow();
    }


RedrawEeg   = true;
                                        // not needed, we forcibly redirect the focus to the EEG all the time
//EEGView->SetFocus ();
}


//----------------------------------------------------------------------------
void    TTracksViewScrollbar::UpdateIt ()
{
                                        // scaling factor from our range to the max legal scrollbar's range
PositionToTimeFrames   = NonNull ( (double) EEGView->CDPt.GetTotalLength() / ScrollbarMaxRange );


int                 oldline         = GetLineMagnitude ();
int                 oldpage         = GetPageMagnitude ();
int                 oldpos          = GetPosition      ();
int                 newpos          = Round (   EEGView->CDPt.GetMin    ()           / PositionToTimeFrames );
int                 newpage         = Round (   EEGView->CDPt.GetLength ()           / PositionToTimeFrames );
int                 newline         = Round ( ( EEGView->CDPt.GetLength () + 3 ) / 6 / PositionToTimeFrames );

//EEGView->SetFocus (); // the last one removed

if ( oldline == newline 
  && oldpage == newpage 
  && oldpos  == newpos  )
    return;


SetRange         ( 0, ScrollbarMaxRange );  // max range allowed, all the time
SetPageMagnitude ( newpage );
SetLineMagnitude ( newline );

//SetFocus();                           // to update its window
RedrawEeg   = false;

SetPosition ( newpos );
}


//----------------------------------------------------------------------------
/*
void    TTracksViewScrollbar::EvLButtonUp ( uint, TPoint & )
{
StopAccelerating ();
EEGView->SetFocus ();
}
*/

void    TTracksViewScrollbar::EvSetFocus ( HWND hwnd )
{
//EEGView->SetFocus ();

if ( hwnd )                             // reject the focus, give it back to whoever lost it last (eeg view, control window...)
    ::SetFocus ( hwnd );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
