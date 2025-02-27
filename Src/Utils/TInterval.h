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

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// An integer interval with constraints:
//   - bounded to limits
//   - the length returned includes the bounding points
//   - can also force the current positions to not exceed a given length (useful for buffers)

class   TInterval
{
public:

    inline                  TInterval           ();
    inline                  TInterval           ( long limitmin, long limitmax, unsigned long limitlength = 0, long defmin = -1, long defmax = -1 );


    inline void             Reset               ();

    inline void             SetMin              ( long mi,         bool keeplength = false );
    inline void             AddMin              ( long deltami,    bool keeplength = false );
    inline void             SetMax              ( long ma,         bool keeplength = false );
    inline void             AddMax              ( long deltama,    bool keeplength = false );
    inline void             SetMinMax           ( long mi, long ma );
    inline void             SetMinLength        ( long mi, unsigned long le,           bool keeplength = false );
    inline void             SetMaxLength        ( long ma, unsigned long le,           bool keeplength = false );
    inline void             SetLength           ( unsigned long le, bool frommin,      bool keeplength = false );
    inline void             SetLength           ( unsigned long le, long aroundcenter, bool keeplength = false );


    long                    GetMin              ()          const   { return Min; }
    long                    GetMax              ()          const   { return Max; }
    unsigned long           GetLength           ()          const   { return Length; }
    long                    GetMiddle           ()          const   { return ( Min + Max ) / 2; }
    long                    GetLimitMin         ()          const   { return LimitMin; }
    long                    GetLimitMax         ()          const   { return LimitMax; }
    unsigned long           GetLimitLength      ()          const   { return LimitLength; }     // limit of current the length
    unsigned long           GetTotalLength      ()          const   { return (unsigned long) ( LimitMax - LimitMin + 1 ); }

    long                    ClipLimit           ( long  p ) const   { return crtl::Clip    ( p, LimitMin, LimitMax ); }
    inline void             ClippedLimit        ( long &p ) const   {        crtl::Clipped ( p, LimitMin, LimitMax ); }


    inline bool             operator    ==      ( const TInterval& op2 )    const;
//  inline TInterval&       operator    |       ( const TInterval& op2 )    const;


protected:

    long                    Min;
    long                    Max;
    unsigned long           Length;
    long                    LimitMin;
    long                    LimitMax;
    unsigned long           LimitLength;

    inline unsigned long    LengthFromMinMax    ()          const   { return (unsigned long) ( Max - Min + 1 ); }
    inline void             ClipLength          ()                  { if ( Length > LimitLength ) SetMinLength ( Min, LimitLength, true ); }    // provide a default clipping
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // default constructor, just a nice init
        TInterval::TInterval ()
{
Reset ();
}

                                        // constructor with boundaries
        TInterval::TInterval ( long limitmin, long limitmax, unsigned long limitlength, long defmin, long defmax )
{
Reset ();

                                        // set limits plus checking
LimitMin        = limitmin;
LimitMax        = limitmax;

CheckOrder ( LimitMin, LimitMax ); 

                                        // default limit in length is the whole extend
LimitLength     = LimitMax - LimitMin + 1;
                                        // if specified, overload default limit length
if ( limitlength > 0 )
    Mined ( LimitLength, limitlength );

                                        // set default init position, or use optional parameter(s)
Min     = defmin >= 0 ? defmin : LimitMin + LimitLength / 2;
Max     = defmax >= 0 ? defmax : Min;

Clipped ( Min, Max, LimitMin, LimitMax );
                                        // set length
Length          = LengthFromMinMax ();
                                        // and check it
SetLength ( Length, true, true );
}


void    TInterval::Reset ()
{
LimitMin        = 0;
LimitMax        = 0;
LimitLength     = 1;
Min             = 0;
Max             = 0;
Length          = 1;
}


//----------------------------------------------------------------------------
                                        // set the min, optionally shifting the max to keep length
                                        // can shift the min to fulfill the keeplength
void    TInterval::SetMin ( long mi, bool keeplength )
{
Min   = mi;

ClippedLimit ( Min );

if ( keeplength )
    SetLength ( Length, true, true );
else {
    if ( Min > Max )          Min = Max;// reset to length 1
    Length = LengthFromMinMax ();
    }
}


void    TInterval::AddMin ( long deltami, bool keeplength )
{
SetMin ( Min + deltami, keeplength );
}
                                        // set the max, optionaly shifting the min to keep length
                                        // can shift the max to fulfill the keeplength
void    TInterval::SetMax ( long ma, bool keeplength )
{
Max   = ma;

ClippedLimit ( Max );

if ( keeplength )
    SetLength ( Length, false, true );
else {
    if ( Max < Min )          Max = Min;// reset to length 1
    Length = LengthFromMinMax ();
    }
}


void    TInterval::AddMax ( long deltama, bool keeplength )
{
SetMax ( Max + deltama, keeplength );
}

                                        // except boundary, set the min and max
void    TInterval::SetMinMax ( long mi, long ma )
{
Min     = mi;
Max     = ma;

CheckOrder ( Min, Max );

ClippedLimit ( Min );
ClippedLimit ( Max );
                                        // set new length & check
Length = LengthFromMinMax ();
ClipLength ();
}


//----------------------------------------------------------------------------
                                        // set the min and the length, then adjusting the max
                                        // the min can also be shifted to fulfill the keeplength
void    TInterval::SetMinLength ( long mi, unsigned long le, bool keeplength )
{
Min   = mi;

ClippedLimit ( Min );

SetLength ( le, true, keeplength );
}
                                        // set the max and the length, then adjusting the min
                                        // the max can also be shifted to fulfill the keeplength
void    TInterval::SetMaxLength ( long ma, unsigned long le, bool keeplength )
{
Max   = ma;

ClippedLimit ( Max );

SetLength ( le, false, keeplength );
}

                                        // set length either from min or max, and adjust length or keep it strict if possible
void    TInterval::SetLength ( unsigned long le, bool frommin, bool keeplength )
{
                                        // check length limit
if      ( le <= 0          )    le = 1;
else if ( le > LimitLength )    le = LimitLength;

if ( frommin ) {

    Max = Min + le - 1;
    ClippedLimit ( Max );
    Length = LengthFromMinMax ();
                                        // strict length ?
    if ( keeplength && Length != le ) {

        Max = LimitMax;                 // adjust from max
        Min = Max - le + 1;
        ClippedLimit ( Min );
        Length = LengthFromMinMax ();
                                        // still can not make it?
        if ( Length != le ) {
            Min    = LimitMin;
            Length = LengthFromMinMax ();   // here Length is always less than LimitLength
            }
        }
    }
else {                                  // from max

    Min = Max - le + 1;
    ClippedLimit ( Min );
    Length = LengthFromMinMax ();
                                        // strict length ?
    if ( keeplength && Length != le ) {

        Min = LimitMin;                 // adjust from min
        Max = Min + le - 1;
        ClippedLimit ( Max );
        Length = LengthFromMinMax ();
                                        // still can not make it?
        if ( Length != le ) {
            Max    = LimitMax;
            Length = LengthFromMinMax ();   // here Length is always less than LimitLength
            }
        }
    }
}

                                        // set length either from min or max, and adjust length or keep it strict if possible
void    TInterval::SetLength ( unsigned long le, long aroundcenter, bool keeplength )
{
SetMinLength ( aroundcenter - le / 2, le, keeplength );
}


bool    TInterval::operator== ( const TInterval& op2 )  const
{
return     Min == op2.Min
        && Max == op2.Max ;
}

/*
TInterval&  TInterval::operator| ( const TInterval& op2 )   const
{
if ( op2.LimitMin < LimitMin )  LimitMin = op2.LimitMin;
if ( op2.LimitMax > LimitMax )  LimitMax = op2.LimitMax;

if ( op2.Min < Min )    Min = op2.Min;
if ( op2.Max > Max )    Max = op2.Max;

Length = LengthFromMinMax ();
ClipLength ();

return  *this;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
