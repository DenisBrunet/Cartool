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

#include    "Math.Utils.h"
#include    "Time.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr ULONG     DefaultDelayForPolling          =  100;
constexpr ULONG     DefaultDelayBeforeAcceleration  = 1200;
constexpr ULONG     DefaultDelayAfterNoAcceleration = 3000;
constexpr double    DefaultMinSpeed                 =    1;
constexpr double    DefaultMaxSpeed                 =  100;
constexpr double    DefaultMinAcceleration          =    1;

                                        // Class to handle user interaction with some controls that need acceleration
                                        // Public fields can be updated freely by any derived class
class   TAcceleration
{
public:

    inline          TAcceleration ();


    ULONG           DelayForPolling;            // interval between 2 calls below which the acceleration will start
    ULONG           DelayBeforeAcceleration;    // nothing happens before
    ULONG           DelayAfterNoAcceleration;   // nothing accelerates after
    double          MinSpeed;                   // limits of the Speed variable
    double          MaxSpeed;
    double          MinAcceleration;
    double          Speed;                      // current speed (whatever it means depends on the user...)


    inline void     Reset ();
    inline void     StopAccelerating ();       // stops the current acceleration

    inline double   GetSpeed ();                // polls timer, computes acceleration, then returns the speed
    inline ULONG    GetTimeSinceLastPolled ();  //


private:

    ULONG           TimeStart;                  // of current acceleration
    ULONG           TimeLastPoll;               // last time it has been polled
    bool            Accelerating;               // are we currently in an acceleration / faster phase?
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TAcceleration::TAcceleration ()
{
Reset ();
}


void    TAcceleration::Reset ()
{
DelayForPolling             = DefaultDelayForPolling;
DelayBeforeAcceleration     = DefaultDelayBeforeAcceleration;
DelayAfterNoAcceleration    = DefaultDelayAfterNoAcceleration;
MinSpeed                    = DefaultMinSpeed;
MaxSpeed                    = DefaultMaxSpeed;
MinAcceleration             = DefaultMinAcceleration;

Speed                       = MinSpeed;
TimeStart                   = 0;
TimeLastPoll                = 0; // GetWindowsTimeInMillisecond ();
Accelerating                = false;
}


void    TAcceleration::StopAccelerating ()
{
Accelerating    = false;
}


//----------------------------------------------------------------------------
                                        // Tried to make this cooking formula as straighforward as possible, but user interactions are not simple at all
double  TAcceleration::GetSpeed ()
{
ULONG               now             = GetWindowsTimeInMillisecond ();


if ( Accelerating 
  || now - TimeLastPoll < DelayForPolling ) {
                                        // have we waited enough before accelerating?
    if ( now - TimeStart > DelayBeforeAcceleration )

        Accelerating    = true;


    if ( Accelerating ) {

        double      acc     = now - TimeStart > DelayAfterNoAcceleration ? MaxSpeed /    2.5    // boosted acceleration such as to arrive quickly to max speed
                                                                         : MaxSpeed / 1000.0;   // regular acceleration

        Maxed ( acc, MinAcceleration );

        Speed += acc;
        }
    }
else {                                  // slow motion
    Accelerating    = false;
    Speed           = MinSpeed;
    TimeStart       = now;              // update new starting time
    }


TimeLastPoll    = now;                  // update last polling time

                                        // check limits, we care for the customer!
Clipped ( Speed, MinSpeed, MaxSpeed );

return Speed;
}


//----------------------------------------------------------------------------
ULONG   TAcceleration::GetTimeSinceLastPolled ()
{
return  GetWindowsTimeInMillisecond () - TimeLastPoll;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
