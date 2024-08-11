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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Dialogs.TSuperGauge.h"

#include    "Strings.Utils.h"

#include    "TCartoolApp.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    SetGaugeRel ( owl::TGauge &G, int N, int D )
{
G.SetValue ( Round ( Percentage ( N, D ) ) );

G.UpdateWindow ();

UpdateApplication;
}


//----------------------------------------------------------------------------

//DEFINE_RESPONSE_TABLE1(TSuperGauge,TWindow)
//    EV_WM_TIMER,
//END_RESPONSE_TABLE;


int                 TSuperGauge::GaugeId    = 32001;


        TSuperGauge::TSuperGauge ()
{
Gauge           = 0;

Reset ();
}


        TSuperGauge::TSuperGauge ( const char* title, int range, SuperGaugeLevels level, SuperGaugeStyle style )
{
Gauge           = 0;
                                        // !In that case, this means caller does not want to create this progress bar!
if ( range == 0 ) {
    Reset ();
    return;
    }

Set   ( title, range, level, style );
}


        TSuperGauge::~TSuperGauge ()
{
Reset ();
}


//----------------------------------------------------------------------------
void    TSuperGauge::Reset ()
{
CurrentPart     = 0;

if ( Gauge != 0 ) {

    Gauge->Destroy ();
                                        // deleting should do the whole destruction business?
    delete  Gauge;
    Gauge       = 0;
    }

ClearString ( Title );
Style           = SuperGaugeDefault;
Center.Reset ();
Extent.Reset ();


//Range .DeallocateMemory ();           // don't, in case we are to re-use that gauge, we save some memory operations
//Count .DeallocateMemory ();
//Occupy.DeallocateMemory ();

Range .ResetMemory ();
Count .ResetMemory ();
Occupy.ResetMemory ();

NumParts        = 0;
                                        // some refreshing operations are really slow, don't do them all the time but only on intervals
TitleLastTimeCalled     =
WindowLastTimeCalled    = GetWindowsTimeInMillisecond ();


DisplayedValue  = 0;
TotalRange      = 100;
}


//----------------------------------------------------------------------------
void    TSuperGauge::Set ( const char *title, SuperGaugeLevels level, SuperGaugeStyle style )
{
Set ( title, 0, level, style );         // do it for the caller
}


void    TSuperGauge::Set ( const char *title, int range, SuperGaugeLevels level, SuperGaugeStyle style )
{
                                        // clear the way first
Reset ();

                                        // test mode has no window and will not fancy creating a dialog
if ( CartoolApplication->IsNotInteractive () || CartoolMainWindow == 0 )
    return;

                                        // called for a single range
if ( range > 0 )
    AddPart ( 0, range );


Style           = style;


StringCopy  ( Title, StringIsEmpty ( title ) ? "Progress" : title, SuperGaugeMaxTitle - 1 );

                                        // set center according to current position
                                        // !should think of a way to update this dynamically!
owl::TWindow*       windowparent    = CartoolMainWindow->GetClientWindow ();
                                        // in case there are no client window, try to grab the desktop window - does not seem to work with current TGauge, though...
//owl::TWindow*     windowparent    = CartoolApplication->IsInteractive () ? CartoolMainWindow->GetClientWindow () : CartoolApplication->GetWindowPtr ( GetDesktopWindow () );

Center.X    = GetWindowMiddleHoriz ( windowparent );
Center.Y    = GetWindowMiddleVert  ( windowparent );
Center.Z    = level;                    // 0 being visually the highest

                                        // fixed extent for now
Extent.Set ( GaugeWidth, GaugeHeight, 0 );

                                        // create this gauge, which could be replaced later
                                        // note that the font is not rescaled properly...
Gauge       = new owl::TGauge   (   windowparent,
                                    Title,
                                    GaugeId++,      // bumps to the next Id
                                    GetWindowLeft (),
                                    GetWindowTop  (),
                                    Extent.X,
                                    Extent.Y,
                                    true            // horizontal
                                );


UpdateVariables ();

Gauge->SetStep ( 1 );
                                        // actually creating the window
Gauge->Create  ();
                                        // show & refresh
Actualize ( 0 );

                                        // Timer stuff does not work - let the code here if it itches again...
                                        // launch timer
//SetTimer ( 0, 0, 500, & TSuperGauge::TimerProc );
//Timer       = TTimer ( 0, /*TimerCursor*/ 123, 500, INT_MAX );

//TWindow::Create ();
//Timer       = TTimer ( GetHandle (), /*TimerCursor*/ 123, 1000, INT_MAX );

//Timer.Set ();
                                        // this works, but it needs to dispatch messages...
//SetTimer ( 0, (UINT) &Gauge, 500, GaugeTimerProc );
//SetTimer ( 0, (UINT) this, 1000, GaugeTimerProc );
                                        // will dispatch - but Cartool loop is not called
//MSG                 msg;
//BOOL                ret;
//
//while ( ret = GetMessage ( &msg, 0, 0, 0 ) ) {
//    if ( ret != -1 ) {
//        TranslateMessage ( &msg );
//        DispatchMessage  ( &msg ); // dispatches message to window
//        }
//    }

}

                                        // trick is working, except can't get the main loop to process this message?!
//void CALLBACK   GaugeTimerProc ( HWND /*hwnd*/, UINT uMsg, UINT /*idEvent*/, DWORD /*dwTime*/ )
//{
//TSuperGauge        *that            = (TSuperGauge *) uMsg;
//
//if ( that->IsAlive () )
////    that->Gauge->UpdateWindow ();
//    that->Actualize ();
//}

//void    TSuperGauge::EvTimer ( uint timerId )
//{
//switch ( timerId ) {
//    case 123:
//        Timer++;
//        Actualize ();
//        break;
//    }
//}

                                        // right now, used when anything about the ranges has changed
void    TSuperGauge::UpdateVariables ()
{
if ( ! IsAlive () )
    return;

                                        // we need to cache this value
TotalRange      = GetTotalRange ();

                                        // now cook the title
char                titleforgauge[ 1024 ];


StringCopy  ( titleforgauge, StringIsNotEmpty ( Title ) ? Title : "Progress" );


if      ( Style & SuperGaugeCount ) {

    StringAppend    ( titleforgauge, "  %0d / " );

    IntegerToString ( StringEnd ( titleforgauge ), TotalRange );
    }

else // if ( Style & SuperGaugePercentage )

    StringAppend    ( titleforgauge, "  %02d%%" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( ! IsAlive () )
//    return;

                                        // update gauge title
Gauge->SetCaption ( titleforgauge );
//Gauge->SetWindowText ( titleforgauge );

                                        // will smartly select either the real total count, or 100
Gauge->SetRange   ( 0, TotalRange );


//if ( CurrentPart < NumParts ) DBGV6 ( NumParts, CurrentPart, Count[ CurrentPart ], Range[ CurrentPart ], Occupy[ CurrentPart ], TotalRange, "NumParts, CurrentPart, Count[ CurrentPart ], Range[ CurrentPart ], Occupy[ CurrentPart ], TotalRange" );
// could also be used to update size & position
}


//----------------------------------------------------------------------------
                                        // range  == 0 -> can be ignored by calling  AdjustOccupy ( true )
                                        // occupy == 0 -> will not contribute to  GetTotalCount
void    TSuperGauge::AddPart    (   int     part,   int     range,  double  occupy   )
{
if ( part < 0 || range < 0 )
    return;

                                        // inserting a paprt outside current limit?
if ( part >= NumParts ) {
                                        // nice: we can allocate more than 1 slot at a time
    NumParts    = part + 1;
                                        // resize while keeping existing values
    Range .Resize ( NumParts, (MemoryAllocationType) ( MemoryAuto | ResizeKeepMemory ) );
    Count .Resize ( NumParts, (MemoryAllocationType) ( MemoryAuto | ResizeKeepMemory ) );
    Occupy.Resize ( NumParts, (MemoryAllocationType) ( MemoryAuto | ResizeKeepMemory ) );
    }

                                        // Here part is within existing NumParts
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Range ( part )  = range;                // range could be updated later if the right value is not known at creation time
Count ( part )  = 0;

                                        // occupy not provided?
if ( occupy == SuperGaugeDefaultOccupy ) {
                                        // set equal occupation among all parts - note that it overwrites any existing values
    for ( int i = 0; i < NumParts; i++ )

        Occupy ( i )    = 100.0 / (double) NumParts;
    }
else
                                        // store provided value as is - can be adjusted later on see  AdjustOccupy
    Occupy ( part )     = AtLeast ( 0.0, occupy );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UpdateVariables ();
}


//----------------------------------------------------------------------------
                                        // Either real sum of range, or 100
int     TSuperGauge::GetTotalRange ()   const
{
if      ( Style & SuperGaugeCount ) {
    int                 totalrange      = 0;

    for ( int i = 0; i < NumParts; i++ )
        totalrange     += Range ( i );

    return  totalrange;
    }

else // if ( Style & SuperGaugePercentage )

    return  100;
}


//----------------------------------------------------------------------------
                                        // Cumulate all parts, returning either a real count, or a percentage 0..100
                                        // all parts are already check to lie between its boundaries
int     TSuperGauge::GetTotalCount ()   const
{
if      ( Style & SuperGaugeCount ) {

    int                 totalcount      = 0;
                                        // result in [0..totalcount]
    for ( int i = 0; i < NumParts; i++ )
                                        // returns the sum of counters
        totalcount     += Count ( i );

    return  totalcount;
    }

else { // if ( Style & SuperGaugePercentage )

    double              totalcountf     = 0;

                                        // result in [0..100]
    for ( int i = 0; i < NumParts; i++ )
                                        // null range doesn't count (literally)
        if ( Range ( i ) != 0 )
                                        // returns the weighted sum of the counters by their respective occupy weights, which should total to 100.0
            totalcountf    += ( Count ( i ) / (double) Range ( i ) ) * Occupy ( i );


    totalcountf = NoMore ( 100.0, totalcountf );

                                        // add a small epsilon, which shouldn't be a problem for about 1e6 files in 50 parts(!) - it is possible to be exact using  min ( Occupy / Range ) / 2, though
    return  Truncate ( totalcountf + 1e-10 );
    }
}


//----------------------------------------------------------------------------
                                        // Sum of occupied ranges, optionally ignoring the null ones
double  TSuperGauge::GetTotalOccupy ( bool ignorenullparts )    const
{
double              totaloccupy     = 0;


for ( int i = 0; i < NumParts; i++ )

    if ( ! ignorenullparts || ignorenullparts && Range ( i ) != 0 )

        totaloccupy    += Occupy ( i );


return  totaloccupy;
}

                                        // Done when all parts have been added, force the sum of Occupy to be 100
void    TSuperGauge::AdjustOccupy ( bool ignorenullparts )
{
if ( NumParts == 0 )
    return;


double              totaloccupy     = GetTotalOccupy ( ignorenullparts );

                                        // sum of occupy OK?
if ( totaloccupy == 100 )
    return;

                                        // normalize Occupy (including the null ranges)
Occupy     *= 100 / NonNull ( totaloccupy );
}


//----------------------------------------------------------------------------
                                        // !step will be rescaled by the number of current threads, in case we are in a parallel block!
void    TSuperGauge::Next ( int part, SuperGaugeUpdate update, int step )
{
if ( ! IsAlive () )
    return;


if ( part == SuperGaugeDefaultPart )
    part    = CurrentPart;

if ( ! IsInsideLimits ( part, 0, NumParts - 1 ) )
    return;
//#if defined(CHECKASSERT)
//assert ( IsInsideLimits ( part, 0, NumParts - 1 ) );
//#endif
                                        // !sign of step is not tested, so it is valid to actually decrease the progress bar!
                                        // !we also extrapolate the stepping with the current amount of active threads (this could be erroneous if not ALL threads are allocated to the processing being monitored)!
SetValue ( part, Count ( part ) + step * StepThread () );

                                        // this one is optional, there might be multiple gauge at once, not all should be allowed to update the title
if ( update == SuperGaugeUpdateTitle ) {
                                        // don't refresh main title too often, we don't know the absolute size of counter set by caller...
                                        // maybe this could also be generalized to Show function?
    ULONGLONG           now             = GetWindowsTimeInMillisecond ();

    if ( now - TitleLastTimeCalled < SuperGaugeRefreshDelayTitle )
        return;

    TitleLastTimeCalled = now;

    CartoolApplication->SetMainTitle ( *this );
    }
}


void    TSuperGauge::FinishPart ( int part )
{
if ( ! IsAlive () )
    return;


if ( part == SuperGaugeDefaultPart )
    part    = CurrentPart;

if ( ! IsInsideLimits ( part, 0, NumParts - 1 ) )
    return;
//#if defined(CHECKASSERT)
//assert ( IsInsideLimits ( part, 0, NumParts - 1 ) );
//#endif


SetValue ( part, Range ( part ) );
}


void    TSuperGauge::FinishParts ()
{
if ( ! IsAlive () )
    return;


for ( int i = 0; i < NumParts; i++ )

    FinishPart ( i );
}


//----------------------------------------------------------------------------
                                        // Actually store the value, with some checking in between
void    TSuperGauge::SetValue ( int part, int value )
{
if ( ! IsAlive () )
    return;


if ( part == SuperGaugeDefaultPart )
    part    = CurrentPart;

if ( ! IsInsideLimits ( part, 0, NumParts - 1 ) )
    return;
//#if defined(CHECKASSERT)
//assert ( IsInsideLimits ( part, 0, NumParts - 1 ) );
//#endif

                                        // avoid weird under- or over-flow, done per part
Count ( part )  = Clip ( value, 0, Range ( part ) );


Show ( GetTotalCount () );
}

                                        // Update the range at some point, f.ex. when the actual range is known well after the creation of the gauge / part
                                        // it should not affect the display thanks to the occupy mechanism
void    TSuperGauge::SetRange ( int part, int range )
{
if ( part == SuperGaugeDefaultPart )
    part    = CurrentPart;

if ( ! IsInsideLimits ( part, 0, NumParts - 1 ) )
    return;
//#if defined(CHECKASSERT)
//assert ( IsInsideLimits ( part, 0, NumParts - 1 ) );
//#endif

                                        // new range
Range ( part )  = AtLeast ( 0, range );

                                        // check that, too
Clipped ( Count ( part ), 0, Range ( part ) );


UpdateVariables ();


Show ( GetTotalCount () );
}


int     TSuperGauge::GetRange ( int part )
{
if ( part == SuperGaugeDefaultPart )
    part    = CurrentPart;

if ( ! IsInsideLimits ( part, 0, NumParts - 1 ) )
    return  0;
//#if defined(CHECKASSERT)
//assert ( IsInsideLimits ( part, 0, NumParts - 1 ) );
//#endif


return  Range ( part );
}


//----------------------------------------------------------------------------
                                        // refreshing the window position, in case the client has changed
void    TSuperGauge::ActualizePosition ()
{
if ( ! IsAlive () )
    return;

owl::TWindow*       windowclient    = CartoolMainWindow->GetClientWindow ();

Center.X    = GetWindowMiddleHoriz ( windowclient );
Center.Y    = GetWindowMiddleVert  ( windowclient );
}


//----------------------------------------------------------------------------
void    TSuperGauge::Actualize ( int displayvalue )
{
if ( ! IsAlive () )
    return;

                                        // retrieving and setting window position is super costly, just ask once in a while only
ULONGLONG           now             = GetWindowsTimeInMillisecond ();
                                        // using a much greater time period than for the title
if ( now - WindowLastTimeCalled > SuperGaugeRefreshDelayWindow ) {

    WindowLastTimeCalled    = now;

    TPointInt       oldcenter   ( Center );

    ActualizePosition ();

    if ( Center != oldcenter )

        WindowSetOrigin ( Gauge, GetWindowLeft (), GetWindowTop () );
    }


if ( displayvalue >= 0 )
    Gauge->SetValue ( displayvalue );

Gauge->UpdateWindow ();

UpdateApplication;
}

                                        // display value according to current setting
void    TSuperGauge::Show ( int value )
{
if      ( Style & SuperGaugeCount )
                                        // count mode is always absolute & linear
    Actualize ( value );

else { // if ( Style & SuperGaugePercentage )

    if      ( Style & SuperGaugeLinear )
                                        // linear progression, already scaled to 100
        Actualize ( value );
                                        // stepping faster is favorably perceived by the user, giving the feeling of a shorter execution time
    else if ( Style & SuperGaugeAccelerated ) {

        double              norm            = value / 100.0;
                                        // faster
//      value        = Round ( pow ( norm, 1.7 ) * 99.5 + 1.0 );
                                        // smooth speed up, but beginning does not move that much
//      value        = Round ( Round ( ( 1.0 - FastCosine ( norm * HalfPi ) ) * 99.5 + 1.0 ) );
                                        // smooth speed-up + alive at beginning
        value        = Round ( ( norm + ( 1.0 - FastCosine ( norm * HalfPi ) ) ) / 2.0 * 99.5 + 1.0 );

        Clipped ( value, 0, 100 );

        Actualize ( value );
        }

    } // SuperGaugePercentage

                                        // save value actually displayed
DisplayedValue  = value;
}
                                        // there is no timer in here, just the caller has to call the function
                                        // frequently enough for it to appear to blink
void    TSuperGauge::Blink ()
{
if ( ! IsAlive () )
    return;

                                        // blink value / value + 1
int                 value           = DisplayedValue + IsOdd ( GetWindowsTimeInMillisecond () / 1000 );
                                        // blink value - 1 / value / value + 1
//int                 value           = DisplayedValue + ( GetWindowsTimeInMillisecond () / 1000 ) % 3 - 1;
                                        // blink value / value - 1 / value / value + 1
//int                 value           = ( GetWindowsTimeInMillisecond () / 1000 ) & 0x3;
//value   = DisplayedValue + ( IsOdd ( value ) ? ( value & 0x2 ) - 1 : 0 );

Clipped ( value, 0, TotalRange );

Actualize ( value );
}


void    TSuperGauge::EndBlink ()
{
if ( ! IsAlive () )
    return;


Actualize ( DisplayedValue );
}


//----------------------------------------------------------------------------
void    TSuperGauge::Finished ()
{
if ( IsAlive () ) {

//  Gauge->Create ();                   // force show the window
    crtl::WindowRestore ( Gauge );      // or simply restore window

    Show ( TotalRange );                // set the gauge to max very shortly, then shut it down

    WaitMilliseconds ( 200 );           // have you seen it now?
    }

                                        // we're done
Reset ();
}

                                        // Blink to the user to show contentment :)
void    TSuperGauge::HappyEnd ()
{
if ( IsAlive () ) {

//  Gauge->Create ();                   // force show the window
    crtl::WindowRestore ( Gauge );      // or simply restore window

    Gauge->SetCaption ( "Done!" );      // force title
//  Gauge->SetWindowText ( "Done!" );

                                        // then repeat for a few times to create a (poor man) blinking animation
    for ( int i = 0; i < 4; i++ ) {
        Show (          0 ); WaitMilliseconds ( 150 );
        Show ( TotalRange ); WaitMilliseconds ( 200 );
        }
    }

                                        // we're done
Reset ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
