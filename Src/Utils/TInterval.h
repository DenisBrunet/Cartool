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

    inline              TInterval       ();
    inline              TInterval       ( long limin, long limax, unsigned long liml = 0 );


    inline void         SetMin          ( long mi,         bool keeplength = false );
    inline void         AddMin          ( long deltami,    bool keeplength = false );
    inline void         SetMax          ( long ma,         bool keeplength = false );
    inline void         AddMax          ( long deltama,    bool keeplength = false );
    inline void         SetMinMax       ( long mi, long ma );
    inline void         SetMinLength    ( long mi, unsigned long le,           bool keeplength = false );
    inline void         SetMaxLength    ( long ma, unsigned long le,           bool keeplength = false );
    inline void         SetLength       ( unsigned long le, bool frommin,      bool keeplength = false );
    inline void         SetLength       ( unsigned long le, long aroundcenter, bool keeplength = false );


    long                GetMin          ()  const   { return Min; }
    long                GetMax          ()  const   { return Max; }
    unsigned long       GetLength       ()  const   { return Length; }
    long                GetMiddle       ()  const   { return ( Min + Max ) / 2; }
    long                GetLimitMin     ()  const   { return LimitMin; }
    long                GetLimitMax     ()  const   { return LimitMax; }
    unsigned long       GetLimitLength  ()  const   { return LimitLength; }     // limit of current the length
    unsigned long       GetTotalLength  ()  const   { return (unsigned long) ( LimitMax - LimitMin + 1 ); }


    inline bool         operator    ==  ( const TInterval& op2 )    const;
//  inline TInterval&   operator    |   ( const TInterval& op2 )    const;


protected:

    long            Min;
    long            Max;
    unsigned long   Length;
    long            LimitMin;
    long            LimitMax;
    unsigned long   LimitLength;

    inline unsigned long    LengthFromMinMax    ()          const;  // just a formula
    inline void             ClipLimit           ( long &p ) const;  // clip within Limits
    inline void             ClipLength          ();                 // clip length

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // default constructor, just a nice init
        TInterval::TInterval ()
{
LimitMin        = LimitMax  = 0;
LimitLength     = 1;
Min             = Max       = 0;
Length          = LengthFromMinMax ();
}

                                        // constructor with boundaries
        TInterval::TInterval ( long limin, long limax, unsigned long liml )
{
                                        // set limits plus checking
LimitMin        = limin;
LimitMax        = limax;

CheckOrder ( LimitMin, LimitMax ); 

                                        // default limit in length is the whole extend
LimitLength     = LimitMax - LimitMin + 1;
                                        // if specified, overload default limit length
if ( liml > 0 )
    LimitLength = min ( liml, LimitLength );

                                        // set default init position
Min             = 
Max             = LimitMin + LimitLength / 2; // LimitMin;

                                        // set length
Length          = LengthFromMinMax ();
}


//----------------------------------------------------------------------------
unsigned long   TInterval::LengthFromMinMax ()  const
{
return (unsigned long) ( Max - Min + 1 );
}

                                        // variable has to be within limits
void    TInterval::ClipLimit ( long &p )    const
{
crtl::Clipped ( p, LimitMin, LimitMax );
}

                                        // length has to be less than a limit
void    TInterval::ClipLength ()
{
if ( Length > LimitLength )             // provide a default clipping
    SetMinLength ( Min, LimitLength, true );
}


//----------------------------------------------------------------------------
                                        // set the min, optionally shifting the max to keep length
                                        // can shift the min to fulfill the keeplength
void    TInterval::SetMin ( long mi, bool keeplength )
{
Min   = mi;

ClipLimit ( Min );

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

ClipLimit ( Max );

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
                                        // permutate?
if ( mi < ma )        { Min = mi; Max = ma; }
else                  { Min = ma; Max = mi; }

ClipLimit ( Min );
ClipLimit ( Max );
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

ClipLimit ( Min );

SetLength ( le, true, keeplength );
}
                                        // set the max and the length, then adjusting the min
                                        // the max can also be shifted to fulfill the keeplength
void    TInterval::SetMaxLength ( long ma, unsigned long le, bool keeplength )
{
Max   = ma;

ClipLimit ( Max );

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
    ClipLimit ( Max );
    Length = LengthFromMinMax ();
                                        // strict length ?
    if ( keeplength && Length != le ) {

        Max = LimitMax;                 // adjust from max
        Min = Max - le + 1;
        ClipLimit ( Min );
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
    ClipLimit ( Min );
    Length = LengthFromMinMax ();
                                        // strict length ?
    if ( keeplength && Length != le ) {

        Min = LimitMin;                 // adjust from min
        Max = Min + le - 1;
        ClipLimit ( Max );
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
