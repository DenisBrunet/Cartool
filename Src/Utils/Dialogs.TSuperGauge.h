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

#include    <owl/gauge.h>

#include    "System.h"
#include    "WindowingUtils.h"
//#include  "Time.TTimer.h"
#include    "TCartoolDocManager.h"      // TCartoolObjects

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Progress Bars made easier, even without access to application / main windows

#define             GaugeWidth                  CartoolApplication->MmToPixels ( 60 )
#define             GaugeHeight                 CartoolApplication->MmToPixels (  8 )

                                        // Barely used anymore
void                SetGaugeRel ( owl::TGauge &G, int N, int D );


//----------------------------------------------------------------------------

enum                SuperGaugeStyle
                    {
                    SuperGaugeCount             = 0x01,
                    SuperGaugePercentage        = 0x02,

                    SuperGaugeLinear            = 0x10,
                    SuperGaugeAccelerated       = 0x20,

                    SuperGaugeLinearCount       = SuperGaugeCount      | SuperGaugeLinear,
                    SuperGaugeLinearPercentage  = SuperGaugePercentage | SuperGaugeLinear,
                    SuperGaugeDefault           = SuperGaugeLinearPercentage,
                    };


constexpr int       SuperGaugeMaxTitle          = 256;

constexpr int       SuperGaugeDefaultPart       = -1;
constexpr int       SuperGaugeDefaultOccupy     = -1;

constexpr ULONGLONG SuperGaugeRefreshDelayTitle     =   50;
constexpr ULONGLONG SuperGaugeRefreshDelayWindow    = 1000;

enum                SuperGaugeLevels
                    {
                    SuperGaugeLevelSub          = -1,   // below the regular level                              - usually for debugging
                    SuperGaugeLevelDefault,             // default level                                        - usually for processing
                    SuperGaugeLevelInter,               // intermediate level, just visually above the default  - usually for longer processing that will use sub-processing gauges
                    SuperGaugeLevelBatch,               // highest level                                        - usually for batch main progress
                    };

enum                SuperGaugeUpdate
                    {
                    SuperGaugeNoTitle,
                    SuperGaugeUpdateTitle,
                    };

                                        // Class that encapsulates all the nitty-gritty parts of progress bar handling.
                                        // It also offers an optional level value, which allows to stack multiple progress bars
                                        // for nested processings.
class   TSuperGauge :   public  TCartoolObjects
{
public:
                    TSuperGauge ();
                    TSuperGauge ( const char* title, int range = 100, SuperGaugeLevels level = SuperGaugeLevelDefault, SuperGaugeStyle style = SuperGaugeDefault );
                   ~TSuperGauge ();


    int             CurrentPart;                    // set by caller, when passed to a function that doesn't know which part to update


    bool            IsAlive         ()  const                   { return    Gauge != 0 && IsMainThread ();  }           // gauge is in use, and this is the main thread
    bool            IsNotAlive      ()  const                   { return    ! IsAlive ();                   }           // gauge is not in use, or not the main thread
    bool            IsDone          ()  const                   { return    DisplayedValue == TotalRange;   }           // counter reached max - to avoid possible rounding problems, another way would be to explcitly check all Count are at max


    void            Reset           ();
    void            Set             ( const char *title, int range, SuperGaugeLevels level = SuperGaugeLevelDefault, SuperGaugeStyle style = SuperGaugeDefault );   // single part
    void            Set             ( const char *title,            SuperGaugeLevels level = SuperGaugeLevelDefault, SuperGaugeStyle style = SuperGaugeDefault );   // no part defined yet
    void            AddPart         ( int part, int range, double occupy = SuperGaugeDefaultOccupy );
    void            AdjustOccupy    ( bool ignorenullparts );               // check & force sum of Occupy to be 100
    void            FinishPart      ( int part = SuperGaugeDefaultPart );   // finish/fill this part
    void            FinishParts     ();                                     // finish all parts, convenient to make sure the gauge is done
    int             GetNumParts     ()  const                   { return    NumParts; }

    int             GetTotalRange   ()  const;
    int             GetTotalCount   ()  const;
    double          GetTotalOccupy  ( bool ignorenullparts )    const;
    int             GetValue        ()  const                   { return    DisplayedValue; }
    SuperGaugeLevels    GetLevel    ()  const                   { return    (SuperGaugeLevels) Center.Z; }

    bool            IsStylePercentage   ()  const               { return    Style & SuperGaugePercentage; }
    bool            IsStyleCount        ()  const               { return    Style & SuperGaugeCount;      }


    void            Next        ( int part = SuperGaugeDefaultPart, SuperGaugeUpdate update = SuperGaugeNoTitle, int step = 1 );   // increment current counter & update - !accounts for multithreading!
    void            SetValue    ( int part, int value );                                        // directly set the counter value & update - use SuperGaugeDefaultPart for CurrentPart
    int             GetRange    ( int part = SuperGaugeDefaultPart );
    void            SetRange    ( int part, int range );                                        // udpate the range, in case we know it only later - use SuperGaugeDefaultPart for CurrentPart
    void            Blink       ();                                                             // blinks the gauge value if called regularly
    void            EndBlink    ();                                                             // shouldn't be needed, just to end the blinking correctly
    void            Actualize   ( int displayvalue = -1 );                                      // encapsulate gauge: set it and refresh display
    void            ActualizePosition   ();
    int             GetWindowLeft   ()  const                   { return Center.X - Extent.X / 2; }
    int             GetWindowTop    ()  const                   { return Center.Y - Extent.Y * (int) ( Center.Z + 0.5 ); }  // levels go downward
    int             GetWindowWidth  ()  const                   { return Extent.X; }
    int             GetWindowHeight ()  const                   { return Extent.Y; }


    void            Finished    ();                         // briefly show the max gauge, then shut it downs
    void            HappyEnd    ();                         // briefly flash the gauge, then shut it downs (takes longer than Finished)


    void            WindowRestore   ()                          { if ( IsAlive () ) crtl::WindowRestore  ( Gauge ); }
    void            WindowHide      ()                          { if ( IsAlive () ) crtl::WindowHide     ( Gauge ); }


                                                            // these can be confusing
//                  operator    bool            ()              { return    IsAlive (); }
//                  operator    int             ()              { return    DisplayedValue; }


protected:

    owl::TGauge*    Gauge;
    static int      GaugeId;                                // static, so that we can bump different Id's one after the other for each new gauge

    char            Title[ SuperGaugeMaxTitle ];            // just the meaningful part
    int             Style;
    TPointInt       Center;                                 // center in XY; Center.Z will store the level
    TPointInt       Extent;                                 // size in XY

    TArray1<int>    Range;
    TArray1<int>    Count;
    TVector<double> Occupy;
    int             NumParts;
    ULONGLONG       TitleLastTimeCalled;                    // we might want to restrain the number of calls to refresh
    ULONGLONG       WindowLastTimeCalled;                   // we might want to restrain the number of calls to refresh

    int             DisplayedValue;
    int             TotalRange;


    void            UpdateVariables ();
    void            Show            ( int value );          // actually set a value on the gauge, then update


//  TTimer          Timer;
//  void            EvTimer ( UINT timerId );
//  DECLARE_RESPONSE_TABLE(TSuperGauge);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
