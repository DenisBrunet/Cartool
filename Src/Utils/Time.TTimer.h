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

#include    "WinUser.h"                 // SetTimer

#include    "System.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrapper around Windows timer
class   TTimer
{
public:
                    TTimer ( HWND hw = 0 )                          : Hwnd ( hw ), TimerId (  0 ), Interval (  0 ), MaxCount (    0 ), Count ( 0 )  {}
                    TTimer ( HWND hw, int id, int it, int maxc )    : Hwnd ( hw ), TimerId ( id ), Interval ( it ), MaxCount ( maxc ), Count ( 0 )  {}


    bool            IsFirstCall ()              const   { return Hwnd && Count == 0; }

    int             GetTimerId  ()              const   { return TimerId;            }
    int             GetCount    ()              const   { return Count;              }
    int             GetMaxCount ()              const   { return MaxCount;           }
    int             GetInterval ()              const   { return Interval;           }
    double          GetPercentageCompletion ()  const   { return              Count   / (double) MaxCount; }
    double          GetPercentageRemaining  ()  const   { return ( MaxCount - Count ) / (double) MaxCount; }


    inline void     Stop        ();
    inline void     Start       ();
    inline void     Start       ( int id, int it, int maxc );
    void            SetInterval ( int it )              { Interval = it;             }

    inline          operator    bool    ()      const   { return Hwnd && Count > 0 && Count <= MaxCount; }  // true if timer is all set AND running

    inline TTimer&  operator    ++      ( int );


protected:

    HWND            Hwnd;               // store the window to time
    int             TimerId;            // identifies this timer
    int             Count;              // counts the # of iteration, better hide it
    int             MaxCount;           // # of loop Timer
    int             Interval;           // in ms
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------


void        TTimer::Stop    ()
{
/*if ( (bool) *this )*/

    KillTimer ( Hwnd, TimerId );

Count       = 0;
}


void        TTimer::Start   ()
{
Stop ();

Count       = 1; 

SetTimer ( Hwnd, TimerId, Interval, 0 );
}


void        TTimer::Start   ( int id, int it, int maxc )
{
Stop ();

TimerId     = id; 
Interval    = it; 
MaxCount    = maxc; 
Count       = 0; 

Start ();
}


TTimer&     TTimer::operator++ ( int )
{
if ( VkEscape () )

    Stop ();

else {
    Count++;

    SetTimer ( Hwnd, TimerId, Interval, 0 );
    }

return *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
