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

#include    <owl/date.h>                // TDate

#include    "Math.Utils.h"
#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"
#include    "Time.Utils.h"              // IsLeapYear

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    MicrosecondsPerHour     = 3600e6;
constexpr double    MicrosecondsPerDay      = 24 * MicrosecondsPerHour;

                                        // Upgrading owl::TDate class
class   TDateTime :     public owl::TDate
{
public:

    inline          TDateTime ();
                                        // the caller must make sure that there is no 'long' overflow
                                        // therefore reporting the too high variables to the next ones (at least partly)
    inline          TDateTime ( long y, long mo, long d, long h, long m, long s, long ms, long us );


    bool            RelativeTime;           // force to relative time (starting from 0)
    double          RelativeTimeOffset;     // the shift for the relative time, in microseconds (== shifting the origin)
    bool            MicrosecondPrecision;   // the caller sets how much precision he wants

                                            // get time in microseconds, return formatted string
    inline char*    GetStringData       ( double us, char *s, bool interval = false )   const;
    inline char*    GetStringTime       ( double us, char *s, bool interval = false )   const;
    inline char*    GetStringDate       ( char *s )                                     const;
                                            // converts to frequency
    inline char*    GetStringFrequency  ( double us, char *s, bool interval = false )   const;

                                            // get date and time according to current settings (absolute, relative...)
    inline int      GetYear        ()           const   { return    OriginDateAvailable ? Year()       : 0; }
    inline int      GetMonth       ()           const   { return    OriginDateAvailable ? Month()      : 0; }
    inline int      GetDay         ()           const   { return    OriginDateAvailable ? DayOfMonth() : 0; }
    inline int      GetHour        ( double us = 0, bool interval = false, bool sign = true )   const;
    inline int      GetMinute      ( double us = 0, bool interval = false, bool sign = true )   const;
    inline int      GetSecond      ( double us = 0, bool interval = false, bool sign = true )   const;
    inline int      GetMillisecond ( double us = 0, bool interval = false, bool sign = true )   const;
    inline int      GetMicrosecond ( double us = 0, bool interval = false, bool sign = true )   const;
    inline double   GetAbsoluteTimeOffset   ()  const   { return    AbsoluteTimeOffset; }

    inline bool     IsOriginTimeAvailable ()    const   { return    OriginTimeAvailable; }
    inline bool     IsOriginDateAvailable ()    const   { return    OriginDateAvailable; }


    inline bool     IsLeapYear      ()          const   { return    crtl::IsLeapYear      ( GetYear () );                           }
    inline int      GetNumberOfDays ()          const   { return    crtl::GetNumberOfDays ( GetYear () );                           }
    inline int      GetDayOfTheYear ()          const   { return    crtl::GetDayOfTheYear ( GetYear (), GetMonth (), GetDay () );   }

                                            // convert to microseconds, according to current settings
    inline double   RelUsToAbsUs ( double us, bool interval = false )   const;
    inline double   AbsUsToRelUs ( double us, bool interval = false )   const;

    inline void     Show            ( const char* title = 0 )   const;


    inline                 TDateTime       ( const TDateTime& op  );
    inline TDateTime&      operator    =   ( const TDateTime& op2 );


protected:

    long            Hour;                   // time origin
    long            Minute;
    long            Second;
    long            Millisecond;
    long            Microsecond;
    double          AbsoluteTimeOffset;     // this time converted to micro-seconds

    bool            OriginTimeAvailable;    // a valid time origin has been provided
    bool            OriginDateAvailable;    // a valid date origin has been provided

                                            // conversion routine, taking a cumulated time in microseconds
    inline int      ToHour          ( double us )   const;
    inline int      ToMinute        ( double us )   const;
    inline int      ToSecond        ( double us )   const;
    inline int      ToMillisecond   ( double us )   const;
    inline int      ToMicrosecond   ( double us )   const;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TDateTime::TDateTime ()
      : TDate ()
{
RelativeTime            = true;
RelativeTimeOffset      = 0;
MicrosecondPrecision    = false;

OriginTimeAvailable     = false;
OriginDateAvailable     = false;

Hour                    = 0;
Minute                  = 0;
Second                  = 0;
Millisecond             = 0;
Microsecond             = 0;
AbsoluteTimeOffset      = 0;
}


        TDateTime::TDateTime ( const TDateTime& op )
      : TDate ( op )
{
RelativeTime            = op.RelativeTime;
RelativeTimeOffset      = op.RelativeTimeOffset;
MicrosecondPrecision    = op.MicrosecondPrecision;

OriginTimeAvailable     = op.OriginTimeAvailable;
OriginDateAvailable     = op.OriginDateAvailable;

Hour                    = op.Hour;
Minute                  = op.Minute;
Second                  = op.Second;
Millisecond             = op.Millisecond;
Microsecond             = op.Microsecond;
AbsoluteTimeOffset      = op.AbsoluteTimeOffset;
}


TDateTime&  TDateTime::operator= ( const TDateTime& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


TDate::operator= ( op2 );

RelativeTime            = op2.RelativeTime;
RelativeTimeOffset      = op2.RelativeTimeOffset;
MicrosecondPrecision    = op2.MicrosecondPrecision;

OriginTimeAvailable     = op2.OriginTimeAvailable;
OriginDateAvailable     = op2.OriginDateAvailable;

Hour                    = op2.Hour;
Minute                  = op2.Minute;
Second                  = op2.Second;
Millisecond             = op2.Millisecond;
Microsecond             = op2.Microsecond;
AbsoluteTimeOffset      = op2.AbsoluteTimeOffset;


return  *this;
}


//----------------------------------------------------------------------------
                                        // Can work either with a correct date and time
                                        // or, it can report ms to s to m to h to days etc... correctly
                                        // as number of seconds since a date, f.ex.
        TDateTime::TDateTime ( long y, long mo, long d, long h, long m, long s, long ms, long us )
      : TDate ( d, mo, y )
{
OriginDateAvailable     = y && mo && d;
OriginTimeAvailable     = ( h || m || s || ms || us );

RelativeTime            = ! OriginTimeAvailable;
RelativeTimeOffset      = 0;
MicrosecondPrecision    = false;

                                        // convert to cumulated signed value (in microseconds)
AbsoluteTimeOffset      = ( ( ( ( h * (double) 60 ) + m ) * (double) 60 + s ) * (double) 1000 + ms ) * (double) 1000 + us;
//DBGV ( AbsoluteTimeOffset, "all to us" );

                                        // split into time values, results always positive
Microsecond             = ToMicrosecond ( AbsoluteTimeOffset );
Millisecond             = ToMillisecond ( AbsoluteTimeOffset );
Second                  = ToSecond      ( AbsoluteTimeOffset );
Minute                  = ToMinute      ( AbsoluteTimeOffset );
Hour                    = ToHour        ( AbsoluteTimeOffset );


int                 moredays        = AbsoluteTimeOffset / MicrosecondsPerDay;

if ( moredays ) {
//  Hour               %= 24;
    AbsoluteTimeOffset -= moredays * MicrosecondsPerDay;
    *this              += moredays;
    }

                                        // Restore sign if needed
if ( AbsoluteTimeOffset < 0 ) {
    Hour        *= -1;
    Minute      *= -1;
    Second      *= -1;
    Millisecond *= -1;
    Microsecond *= -1;
    }

/*
Microsecond             = 0;            // old version, just in case
Millisecond             =                     ms % 1000;
Second                  =               ( s + ms / 1000 ) % 60;
Minute                  =       ( m + ( ( s + ms / 1000 ) / 60 ) ) % 60;
Hour                    = h + ( ( m + ( ( s + ms / 1000 ) / 60 ) ) / 60 );
*/

//DBGV5 ( h, m, s, ms, us, "in: h m s ms us" );
//DBGV5 ( Hour, Minute, Second, Millisecond, Microsecond, "out: h m s ms us" );
}


//----------------------------------------------------------------------------
void    TDateTime::Show ( const char* title )   const
{
char                buff[ 256 ];
char                buffd[ 256 ];
char                bufft[ 256 ];

StringCopy  ( buff, GetStringDate ( buffd ), "\n", GetStringTime ( 0, bufft ) );

ShowMessage ( buff, StringIsEmpty ( title ) ? "Date & Time" : title );
}


//----------------------------------------------------------------------------
char   *TDateTime::GetStringDate ( char *s )    const
{
if ( OriginDateAvailable )  StringCopy ( s, AsString().c_str() );
else                        ClearString ( s );

return  s;
}


//----------------------------------------------------------------------------
                                        // Convert, but no special temporal formatting
char   *TDateTime::GetStringData ( double us, char *s, bool interval )  const
{
us  = RelUsToAbsUs ( us, interval ) / 1e6;

//if ( Fraction ( us ) == 0 )     sprintf ( s, "%lg",   us ); // cleaner display without fraction
//else                            sprintf ( s, "%.3lf", us ); // 3 digits seem enough(?)
                                        // settle down to a general purpose formatting
FloatToString   ( s, us, 3 );

return  s;
}


//----------------------------------------------------------------------------
char   *TDateTime::GetStringTime ( double us, char *s, bool interval )  const
{
                                        // convert to relative string?
if ( RelativeTime || interval || !OriginTimeAvailable ) {
                                        // convert to microseconds
    us  = RelUsToAbsUs ( us, interval );

    if ( MicrosecondPrecision ) {
        if      ( fabs ( us )        < 1000 )   sprintf ( s, "%s%0dµs",                   SignToString ( us ), ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 1e3  < 1000 )   sprintf ( s, "%s%0dms%03d",               SignToString ( us ), ToMillisecond ( us ), ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 1e6  < 60   )   sprintf ( s, "%s%0ds%03d.%03d",           SignToString ( us ), ToSecond      ( us ), ToMillisecond ( us ), ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 60e6 < 60   )   sprintf ( s, "%s%0d:%02d.%03d.%03d",      SignToString ( us ), ToMinute      ( us ), ToSecond      ( us ), ToMillisecond ( us ), ToMicrosecond ( us ) );
        else                                    sprintf ( s, "%s%0d:%02d:%02d.%03d.%03d", SignToString ( us ), ToHour        ( us ), ToMinute      ( us ), ToSecond      ( us ), ToMillisecond ( us ), ToMicrosecond ( us ) );
        }
    else { // millisecond precision
        if      ( fabs ( us ) / 1e3  < 1000 )   sprintf ( s, "%s%0dms",                   SignToString ( us ), ToMillisecond ( us ) );
        else if ( fabs ( us ) / 1e6  < 60   )   sprintf ( s, "%s%0ds%03d",                SignToString ( us ), ToSecond      ( us ), ToMillisecond ( us ) );
        else if ( fabs ( us ) / 60e6 < 60   )   sprintf ( s, "%s%0d:%02d.%03d",           SignToString ( us ), ToMinute      ( us ), ToSecond      ( us ), ToMillisecond ( us ) );
        else                                    sprintf ( s, "%s%0d:%02d:%02d.%03d",      SignToString ( us ), ToHour        ( us ), ToMinute      ( us ), ToSecond      ( us ), ToMillisecond ( us ) );
        }
    }

else {                                  // convert to absolute string
                                        // convert to microseconds
    us  = RelUsToAbsUs ( us, false );

    if ( MicrosecondPrecision )                // print the sign explicitly, in case hour is 0 and not the rest, the sign would not appear!
        sprintf ( s, "%s%0d:%02d:%02d.%03d.%03d", SignToString ( us ), ToHour ( us ), ToMinute ( us ), ToSecond ( us ), ToMillisecond ( us ), ToMicrosecond ( us ) );

    else // millisecond precision
        sprintf ( s, "%s%0d:%02d:%02d.%03d",      SignToString ( us ), ToHour ( us ), ToMinute ( us ), ToSecond ( us ), ToMillisecond ( us ) );
    }

return  s;
}


//----------------------------------------------------------------------------
char   *TDateTime::GetStringFrequency ( double us, char *s, bool interval )     const
{
                                        // convert to relative string?
if ( RelativeTime || interval || !OriginTimeAvailable ) {
                                        // convert to microseconds
    us  = RelUsToAbsUs ( us, interval );

    sprintf ( s, "%.2f Hz", us / 1e6 );
    }

else {                                  // convert to absolute string
                                        // convert to microseconds
    us  = RelUsToAbsUs ( us, false );

    sprintf ( s, "%.2f Hz", us / 1e6 );
    }

return  s;
}


//----------------------------------------------------------------------------
                                        // add the offset to present origin, result can be signed (in microseconds)
double  TDateTime::RelUsToAbsUs ( double us, bool interval )    const
{
if      ( interval     )    return  fabs ( us );
else if ( RelativeTime )    return  us + RelativeTimeOffset;
else                        return  us + AbsoluteTimeOffset;
//else                      return  us + ( ( ( ( Hour * (double) 60 ) + Minute ) * (double) 60 + Second ) * (double) 1000 + Millisecond ) * (double) 1000 + Microsecond;
}

                                        // convert absolute us to relative us (starting from beginning)
double  TDateTime::AbsUsToRelUs ( double us, bool interval )    const
{
if      ( interval     )    return  fabs ( us );
else if ( RelativeTime )    return  us - RelativeTimeOffset;
else                        return  us - AbsoluteTimeOffset;
}


//----------------------------------------------------------------------------
                                        // these are for public results, results can be signed
int     TDateTime::GetHour        ( double us, bool interval, bool sign )   const
{
us  = RelUsToAbsUs ( us, interval );
return  ( sign && us < 0 ? -1 : 1 ) * ToHour ( us );
}

int     TDateTime::GetMinute      ( double us, bool interval, bool sign )   const
{
us  = RelUsToAbsUs ( us, interval );
return  ( sign && us < 0 ? -1 : 1 ) * ToMinute ( us );
}

int     TDateTime::GetSecond      ( double us, bool interval, bool sign )   const
{
us  = RelUsToAbsUs ( us, interval );
return  ( sign && us < 0 ? -1 : 1 ) * ToSecond ( us );
}

int     TDateTime::GetMillisecond ( double us, bool interval, bool sign )   const
{
us  = RelUsToAbsUs ( us, interval );
return  ( sign && us < 0 ? -1 : 1 ) * ToMillisecond ( us );
}

int     TDateTime::GetMicrosecond ( double us, bool interval, bool sign )   const
{
us  = RelUsToAbsUs ( us, interval );
return  ( sign && us < 0 ? -1 : 1 ) * ToMicrosecond ( us );
}


//----------------------------------------------------------------------------
                                        // internal conversion routines, absolute results
int     TDateTime::ToHour ( double us ) const
{
return  fmod ( fabs ( us ) / 3600e6, 24 );
}

int     TDateTime::ToMinute ( double us )   const
{
return  fmod ( fabs ( us ) / 60e6, 60 );
}

int     TDateTime::ToSecond ( double us )   const
{
return  fmod ( fabs ( us ) / 1e6, 60 );
}

int     TDateTime::ToMillisecond ( double us )  const
{
return  fmod ( fabs ( us ) / 1e3, 1000 );
}

int     TDateTime::ToMicrosecond ( double us )  const
{
return  fmod ( fabs ( us ), 1000 );
}


/*
char   *TDateTime::GetStringTime ( long ms, char *s, bool forcerelativetime )
{
if ( OriginTimeAvailable && !( RelativeTime || forcerelativetime ) ) {
    sprintf ( s, "%0dh%02d:%02d.%03d", Hour + ( Minute + ( Second + ( ( Millisecond + ms ) / 1000 ) ) / 60 ) / 60,
                                              ( Minute + ( Second + ( ( Millisecond + ms ) / 1000 ) ) / 60 ) % 60,
                                                         ( Second + ( ( Millisecond + ms ) / 1000 ) ) % 60,
                                                                      ( Millisecond + ms ) % 1000 );
    }
else {
    if      ( ms <           1000 )     sprintf ( s, "%0dms",                                                                     ms        );
    else if ( ms <      60 * 1000 )     sprintf ( s, "%0ds%03d",                                                ms / 1000,        ms % 1000 );
    else if ( ms < 60 * 60 * 1000 )     sprintf ( s, "%0d:%02d.%03d",                      ms / 60000,        ( ms / 1000 ) % 60, ms % 1000 );
    else                                sprintf ( s, "%0dh%02d:%02d.%03d", ms / 3600000, ( ms / 60000 ) % 60, ( ms / 1000 ) % 60, ms % 1000 );
    }

return  s;
}
*/
                                        // offset is in microsecond, and can go up to hours, so a double with 52 bits of mantissa seems fair enough (= 625499 hours)
                                        // optionally adding a fixed offset (origin)
/*
char   *TDateTime::GetStringTime ( double us, char *s, bool interval )
{
if ( OriginTimeAvailable && !( RelativeTime || interval ) ) {

    if ( MicrosecondPrecision )
        sprintf ( s, "%0dh%02d:%02d.%03d.%03d", ToHour ( Hour, Minute, Second, Millisecond, Microsecond + us ), ToMinute ( Minute, Second, Millisecond, Microsecond + us ), ToSecond ( Second, Millisecond, Microsecond + us ), ToMillisecond ( Millisecond, Microsecond + us, false ), ToMicrosecond ( Microsecond + us ) );

    else // millisecond precision
        sprintf ( s, "%0dh%02d:%02d.%03d",      ToHour ( Hour, Minute, Second, Millisecond, Microsecond + us ), ToMinute ( Minute, Second, Millisecond, Microsecond + us ), ToSecond ( Second, Millisecond, Microsecond + us ), ToMillisecond ( Millisecond, Microsecond + us, true ) );

    }
else {                                  // use relative time
    if ( ! interval )
        us  += RelativeTimeOffset;

    if ( MicrosecondPrecision ) {
        if      ( fabs ( us ) <                  1000 ) sprintf ( s, "%s%0dµs",                   us >= 0 ? "" : "-", ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 1000 <           1000 ) sprintf ( s, "%s%0dms%03d",               us >= 0 ? "" : "-", ToMillisecond ( 0, us, false ), ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 1000 / 1000 <      60 ) sprintf ( s, "%s%0ds%03d.%03d",           us >= 0 ? "" : "-", ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, false ), ToMicrosecond ( us ) );
        else if ( fabs ( us ) / 1000 / 1000 / 60 < 60 ) sprintf ( s, "%s%0d:%02d.%03d.%03d",      us >= 0 ? "" : "-", ToMinute ( 0, 0, 0, us ), ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, false ), ToMicrosecond ( us ) );
        else                                            sprintf ( s, "%s%0dh%02d:%02d.%03d.%03d", us >= 0 ? "" : "-", ToHour ( 0, 0, 0, 0, us ), ToMinute ( 0, 0, 0, us ), ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, false ), ToMicrosecond ( us ) );
        }
    else { // millisecond precision
        if      ( fabs ( us ) / 1000 <           1000 ) sprintf ( s, "%s%0dms",                   us >= 0 ? "" : "-", ToMillisecond ( 0, us, true ) );
        else if ( fabs ( us ) / 1000 / 1000  <     60 ) sprintf ( s, "%s%0ds%03d",                us >= 0 ? "" : "-", ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, true ) );
        else if ( fabs ( us ) / 1000 / 1000 / 60 < 60 ) sprintf ( s, "%s%0d:%02d.%03d",           us >= 0 ? "" : "-", ToMinute ( 0, 0, 0, us ), ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, true ) );
        else                                            sprintf ( s, "%s%0dh%02d:%02d.%03d",      us >= 0 ? "" : "-", ToHour ( 0, 0, 0, 0, us ), ToMinute ( 0, 0, 0, us ), ToSecond ( 0, 0, us ), ToMillisecond ( 0, us, true ) );
        }
    }

return  s;
}
*/

/*
int     TDateTime::ToHour ( long h, long m, long s, long ms, double us )
{
//return (int) ( h + floorl ( ( m + ( ( s + ( ms + floorl ( us / 1000 ) ) / 1000 ) / 60 ) ) / 60 ) );
//return h + ( m + ( s + ( ms + (ULONG) ( us / 1000 ) ) / 1000 ) / 60 ) / 60;

int     t = h + ( m + ( s + ( ms + (int) ( us / 1000 ) ) / 1000 ) / 60 ) / 60;
return  t >= 0 ? t : -t;
}

int     TDateTime::ToMinute ( long m, long s, long ms, double us )
{
//return  (int) fmod ( m + floorl ( ( s + floorl ( ( ms + floorl ( us / 1000 ) ) / 1000 ) ) / 60 ), 60 );
//return ( m + ( ( s + ( ms + (ULONG) ( us / 1000 ) ) / 1000 ) / 60 ) ) % 60;

int     t = m + ( s + ( ms + (int) ( us / 1000 ) ) / 1000 ) / 60;
return  t >= 0 ? t % 60 : -t % 60;
}

int     TDateTime::ToSecond ( long s, long ms, double us )
{
//return  (int) fmod ( s + floorl ( ( ms + floorl ( us / 1000 ) ) / 1000 ), 60 );
//return ( s + ( ms + (ULONG) ( us / 1000 ) ) / 1000 ) % 60;

int     t = s + ( ms + (int) ( us / 1000 ) ) / 1000;
return  t >= 0 ? t % 60 : -t % 60;
}

int     TDateTime::ToMillisecond ( long ms, double us, bool rounding )
{
//return  (int) fmod ( ms + floorl ( us / 1000 ), 1000 );
//return  ( ms + (ULONG) ( us / 1000 ) ) % 1000;

                                        // version without rounding
//int     t = ms + (int) ( us / 1000 );
//return  t >= 0 ? t % 1000 : -t % 1000;

                                        // modified to allow rounding in floating point
double  t = ms + us / 1000;

if ( rounding )
    t += t >= 0 ? 0.5 : -0.5;

return  t >= 0 ? (int) t % 1000 : (int) -t % 1000;
}

int     TDateTime::ToMicrosecond ( double us )
{
//return  us % 1000;
//return  (int) fmod ( us, 1000 );

return  us >= 0 ? fmod ( us, 1000 ) : fmod ( -us, 1000 );
}
*/


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
