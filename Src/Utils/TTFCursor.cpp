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

#include    "Math.Utils.h"

#include    "TTracksDoc.h"

#include    "TTFCursor.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTFCursor::TTFCursor ()
{
EEGDoc              = 0;
SentFrom            = 0;
SentTo              = 0;

LimitMin            = 0;
LimitMax            = 0;
PosMin              = 0;
PosMax              = 0;

PosPivot            = 0;
PosExtend           = 0;
Extending           = false;
}


        TTFCursor::TTFCursor    (   const TTracksDoc*   eegdoc, 
                                    long                limitmin,   long            limitmax, 
                                    long                initmin,    long            initmax 
                                )
      : EEGDoc ( eegdoc )
{
SentFrom            = 0;
SentTo              = 0;


LimitMin            = limitmin;
LimitMax            = limitmax;
CheckOrder  ( LimitMin, LimitMax );


CheckOrder  ( initmin, initmax );

if ( IsInsideLimits ( initmin, LimitMin, LimitMax ) )   PosMin  = initmin;
else                                                    PosMin  = LimitMin;

if ( IsInsideLimits ( initmax, LimitMin, LimitMax ) )   PosMax  = initmax;
else                                                    PosMax  = PosMin;


PosPivot            = PosMin;
PosExtend           = PosMin;
Extending           = false;
}


        TTFCursor::TTFCursor ( const TTFCursor &op )
{
EEGDoc              = op.EEGDoc;
SentFrom            = op.SentFrom;
SentTo              = op.SentTo;

LimitMin            = op.LimitMin;
LimitMax            = op.LimitMax;

PosMin              = op.PosMin;
PosMax              = op.PosMax;

PosPivot            = PosMin;
PosExtend           = PosMin;
Extending           = false;
}


TTFCursor&  TTFCursor::operator= ( const TTFCursor& op2 )
{
                                        // init case only: copy everything
if ( ! EEGDoc && ! LimitMin && ! LimitMax ) {

    EEGDoc              = op2.EEGDoc;
    SentFrom            = op2.SentFrom;
    SentTo              = op2.SentTo;

    LimitMin            = op2.LimitMin;
    LimitMax            = op2.LimitMax;
    }

                                        // all cases: convert the positions and transfer
                                        // convert through micro-seconds and DateTime settings
long                pmin            = TranslateCursorTF ( &op2, op2.PosMin    );
long                pmax            = TranslateCursorTF ( &op2, op2.PosMax    );
long                fpos            = TranslateCursorTF ( &op2, op2.PosPivot  );
long                epos            = TranslateCursorTF ( &op2, op2.PosExtend );


PosMin      = Clip ( pmin, LimitMin, LimitMax );
PosMax      = Clip ( pmax, LimitMin, LimitMax );

Extending   = op2.Extending;

PosPivot    = Clip ( fpos, LimitMin, LimitMax );
PosExtend   = Clip ( epos, LimitMin, LimitMax );


return  *this;
}


//----------------------------------------------------------------------------
void    TTFCursor::SetPos ( long pos )
{
                                        // force stop extending cursor
Extending   = false;

PosMax      = PosMin    = Clip ( pos, LimitMin, LimitMax );

PosPivot    = PosExtend = PosMin;
}


void    TTFCursor::SetPos ( long min, long max )
{
                                        // force stop extending cursor
Extending   = false;

CheckOrder ( min, max );

PosMin      = Clip ( min, LimitMin, LimitMax );
PosMax      = Clip ( max, LimitMin, LimitMax );

if ( PosPivot < PosExtend )    { PosPivot = PosMin; PosExtend = PosMax; }
else                           { PosPivot = PosMax; PosExtend = PosMin; }
}


void    TTFCursor::ShiftPos ( long delta )
{
if      ( PosMin + delta < 0        )  SetPos ( 0, PosMax - PosMin );
else if ( PosMax + delta > LimitMax )  SetPos ( LimitMax - PosMax + PosMin, LimitMax );
else                                   SetPos ( PosMin + delta, PosMax + delta );
}


void    TTFCursor::SetLength ( long length )
{
if ( length <= 0 )  return;

SetPos ( PosMin, PosMin + length - 1 );
}

                                        // set the Extending feature
void    TTFCursor::SetFixedPos ( long pos )
{
                                        // force start extending cursor
Extending   = true;

PosPivot    = Clip ( pos, LimitMin, LimitMax );

PosMin      = PosMax    = PosExtend = PosPivot;
}


void    TTFCursor::SetExtendingPos ( long pos )
{
PosExtend   = Clip ( pos, LimitMin, LimitMax );

if ( ! Extending ) {
    Extending   = true;
    if ( abs ( pos - PosMin ) < abs ( pos - PosMax ) )  PosPivot    = PosMax;
    else                                                PosPivot    = PosMin;
    }

if ( PosExtend < PosPivot )     {   PosMin  = PosExtend;    PosMax  = PosPivot; }
else                            {   PosMax  = PosExtend;    PosMin  = PosPivot; }
}


void    TTFCursor::ShiftExtendingPos ( long delta )
{
SetExtendingPos ( PosExtend + delta );
}


//----------------------------------------------------------------------------
double  TTFCursor::RelativeTFToAbsoluteMicroseconds ( long tf )     const
{
return  EEGDoc->DateTime.RelUsToAbsUs ( TimeFrameToMicroseconds ( tf, EEGDoc->GetSamplingFrequency () ) );
}


long    TTFCursor::AbsoluteMicrosecondsToRelativeTF ( double us )   const
{
return  Round ( MicrosecondsToTimeFrame ( EEGDoc->DateTime.AbsUsToRelUs ( us ), EEGDoc->GetSamplingFrequency () ) ); // + ( us >= 0 ? 0.5 : - 0.5 );
}
                                        // convert the TF from one cursor to the TF of another cursor
                                        // account for any differences in sampling frequencies and time origins
long    TTFCursor::TranslateCursorTF ( const TTFCursor* tfc, long tf )  const
{
if ( EEGDoc->GetSamplingFrequency () && tfc->EEGDoc->GetSamplingFrequency () )
                                        // convert by going through micro-second conversion (DateTime)
    return  AbsoluteMicrosecondsToRelativeTF ( tfc->RelativeTFToAbsoluteMicroseconds ( tf ) );
else
    return  tf;                         // not enough info to do the conversion -> switch to plain 'ol TFs
}

                                        // test if the cursor would produce the same position
bool    TTFCursor::operator== ( const TTFCursor& op2 )  const
{
return     PosMin == TranslateCursorTF ( &op2, op2.PosMin )
        && PosMax == TranslateCursorTF ( &op2, op2.PosMax );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
