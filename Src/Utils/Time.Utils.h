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

#include    <sysinfoapi.h>                // GetTickCount64
#include    <profileapi.h>                // QueryPerformanceCounter
#include    <synchapi.h>                  // Sleep

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Number of milliseconds that have elapsed since Windows was started
                                        // (GetCurrentTime/GetTickCount is the same function, clock() wraps around GetTickCount)
                                        // Time resolution is between 10..16 [ms] as of 2022
constexpr auto GetWindowsTimeInMillisecond  = GetTickCount64;

                                        // Resolution of 1 [us]
                                        // Thread-safe - still note that used for ordering threads, caller has to factor a +-1 [us] uncertainty (as of 2022)
inline int64_t  GetTicks ()
{
LARGE_INTEGER   currentticks;

return  QueryPerformanceCounter ( &currentticks ) ? currentticks.QuadPart : 0;
}

                                        // Only the low part of ticks
inline DWORD    GetLowTicks ()
{
LARGE_INTEGER   currentticks;

return  QueryPerformanceCounter ( &currentticks ) ? currentticks.LowPart : 0;
}

                                        // Active waiting
//inline void   WaitMilliseconds    ( ULONG ms )    { for ( long t = GetWindowsTimeInMillisecond (); GetWindowsTimeInMillisecond () - t < ms; ); }
                                        // Windows way of waiting
inline void     WaitMilliseconds    ( ULONG ms )    { Sleep ( ms ); }


//----------------------------------------------------------------------------
                                        // !Keeping results in double precision, so we can convert time back and forth to time frames without loss - user is free to Truncate or Round the results, though!
inline double   TimeFrameToMinutes      ( double tf,  double samplingfrequency );
inline double   TimeFrameToSeconds      ( double tf,  double samplingfrequency );
inline double   TimeFrameToMilliseconds ( double tf,  double samplingfrequency );
inline double   TimeFrameToMicroseconds ( double tf,  double samplingfrequency );


inline double   DaysToTimeFrame         ( double d,   double samplingfrequency );   
inline double   HoursToTimeFrame        ( double h,   double samplingfrequency );   
inline double   MinutesToTimeFrame      ( double m,   double samplingfrequency );
inline double   SecondsToTimeFrame      ( double s,   double samplingfrequency );
inline double   MillisecondsToTimeFrame ( double ms,  double samplingfrequency );
inline double   MicrosecondsToTimeFrame ( double mus, double samplingfrequency );
inline double   NanosecondsToTimeFrame  ( double ns,  double samplingfrequency );


//----------------------------------------------------------------------------

inline double   FrequencyToMilliseconds ( double f );                               // duration of a full cycle, in [ms]
inline double   FrequencyToTimeFrame    ( double f,   double samplingfrequency );   // duration of a full cycle, in [TF]
inline double   MillisecondsToFrequency ( double ms );                              // frequency from a full cycle
inline double   MicrosecondsToFrequency ( double mus );                             // frequency from a full cycle


//----------------------------------------------------------------------------
                                        // Is leap year (in Gregorian calendar)?
inline bool     IsLeapYear              ( int year );
                                        // returns the nth day of the year (1..366)
inline int      GetNumberOfDays         ( int year );
                                        // returns the nth day of the year (1..366)
inline int      GetDayOfTheYear         ( int year, int month, int day );

                                        // Does display need to go below the milli-second precision?
bool            IsInteger               ( double v );

inline bool     IsMicrosecondPrecision  ( double samplingfrequency )    { return  samplingfrequency > 1000.0 || ! IsInteger ( TimeFrameToMilliseconds ( 1, samplingfrequency ) ); }


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
double          TimeFrameToMinutes ( double tf, double samplingfrequency )
{
return  samplingfrequency > 0 ? tf / samplingfrequency / 60 : tf;
}


double          TimeFrameToSeconds ( double tf, double samplingfrequency )
{
return  samplingfrequency > 0 ? tf / samplingfrequency : tf;
}


double          TimeFrameToMilliseconds ( double tf, double samplingfrequency )
{
return  samplingfrequency > 0 ? tf / samplingfrequency * 1e3 : tf;
}


double          TimeFrameToMicroseconds ( double tf, double samplingfrequency )
{
return  samplingfrequency > 0 ? tf / samplingfrequency * 1e6 : tf;
}


//----------------------------------------------------------------------------
double          DaysToTimeFrame ( double d, double samplingfrequency )
{
return  samplingfrequency > 0 ?  d * ( 24 * 60 * 60 ) * samplingfrequency : d;
}


double          HoursToTimeFrame ( double h, double samplingfrequency )
{
return  samplingfrequency > 0 ?  h * ( 60 * 60 ) * samplingfrequency : h;
}


double          MinutesToTimeFrame ( double m, double samplingfrequency )
{
return  samplingfrequency > 0 ?  m * 60 * samplingfrequency : m;
}


double          SecondsToTimeFrame ( double s, double samplingfrequency )
{
return  samplingfrequency > 0 ?  s * samplingfrequency : s;
}


double          MillisecondsToTimeFrame ( double ms, double samplingfrequency )
{
return  samplingfrequency > 0 ? ms / 1e3 * samplingfrequency : ms;
}


double          MicrosecondsToTimeFrame ( double mus, double samplingfrequency )
{
return  samplingfrequency > 0 ? mus / 1e6 * samplingfrequency : mus;
}


double          NanosecondsToTimeFrame ( double ns, double samplingfrequency )
{
return  samplingfrequency > 0 ? ns / 1e9 * samplingfrequency : ns;
}


//----------------------------------------------------------------------------
double          FrequencyToMilliseconds ( double f )
{
return  f == 0 ? BigDoubleFloat : 1e3 / f;
}


double          FrequencyToTimeFrame ( double f, double samplingfrequency )
{
return  f == 0                  ?   /*BigDoubleFloat*/ (double) LONG_MAX
      : samplingfrequency > 0   ?   samplingfrequency / f 
      :                             1 / f;
}


double          MillisecondsToFrequency ( double ms )
{
return  ms == 0 ? 0 /*BigDoubleFloat*/ : 1e3 / ms;
}


double          MicrosecondsToFrequency ( double mus )
{
return  mus == 0 ? 0 /*BigDoubleFloat*/ : 1e6 / mus;
}


//----------------------------------------------------------------------------
                                        // Is leap year in Gregorian calendar?
bool        IsLeapYear ( int year )
{
return  ! ( year % 4 ) && ( ( year % 100 ) ? true : ! ( year % 400 ) );
}

                                        // returns the nth day of the year (1..366)
int         GetNumberOfDays ( int year )
{
return  IsLeapYear ( year ) ? 366 : 365;
}

                                        // returns the nth day of the year (1..366)
int         GetDayOfTheYear ( int year, int month, int day )
{
if      ( month == 1 )  return  day;
else if ( month == 2 )  return  31 + day;
else {
                                        // # of days from 1 march
    int             ordinal     = Truncate ( 30.6 * month - 91.4 ) + day;

    return  ordinal + ( IsLeapYear ( year ) ? 60 : 59 );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
