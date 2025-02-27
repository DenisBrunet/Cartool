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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class               TTracksDoc;

                                        // Class handles simultaneously: PosMin & PosMax interval, simultaneously with their counterparts PosPivot & PosExtend
class   TTFCursor
{
public:
                    TTFCursor ();
                    TTFCursor ( const TTracksDoc* eegdoc, long limitmin, long limitmax, long initmin = 0, long initmax = 0 );


    const TTracksDoc*   EEGDoc;         // storing which EEG this cursor belongs to, as we may need to retrieve some more infos from it
    UINT                SentTo;         // used to control TTFCursor transmissions to views
    UINT                SentFrom;

                                        // allow convertion through another frequency
    void            GetPos              ( long& min, long& max )        const   { min = PosMin; max = PosMax; }
    long            GetPosMin           ()                              const   { return PosMin; }
    long            GetPosMax           ()                              const   { return PosMax; }
    long            GetPosMiddle        ()                              const   { return ( PosMin + PosMax + 1 ) / 2; }
    long            GetFixedPos         ()                              const   { return PosPivot; }
    long            GetExtendingPos     ()                              const   { return PosExtend; }
    long            GetLength           ()                              const   { return PosMax - PosMin + 1; }
    long            GetLimitMin         ()                              const   { return LimitMin; }
    long            GetLimitMax         ()                              const   { return LimitMax; }
    long            GetLimitLength      ()                              const   { return LimitMax - LimitMin + 1; }

                                        // also reset the Extending feature
    void            SetPos              ( long pos );
    void            SetPos              ( long min, long max );
    void            ShiftPos            ( long delta );
    void            SetLength           ( long length );
    void            SetFixedPos         ( long pos );
    void            SetExtendingPos     ( long pos );
    void            ShiftExtendingPos   ( long delta );
    void            StartExtending      ()                                      { Extending = true; }
    void            StopExtending       ()                                      { Extending = false; }

    bool            IsSplitted          ()                              const   { return PosMin != PosMax; }
    bool            IsExtending         ()                              const   { return Extending; }
    bool            ExtendingDir        ()                              const   { return PosExtend > PosPivot; }

                                        // some useful conversion routines
    long            TranslateCursorTF   ( const TTFCursor* tfc, long tf ) const;// translate TF from one cursor to another one, via intermediate micro-seconds conversions
    double          RelativeTFToAbsoluteMicroseconds    ( long   tf )   const;  // TF to absolute micro-seconds
    long            AbsoluteMicrosecondsToRelativeTF    ( double us )   const;  // absolute micro-seconds to TF


                    TTFCursor           ( const TTFCursor& op  );
    TTFCursor&      operator    =       ( const TTFCursor& op2 );               // assignation, copy the position through micro-second conversions

    bool            operator    ==      ( const TTFCursor& op2 )        const;  // equal if both cursors return the same position in micro-seconds
    bool            operator    !=      ( const TTFCursor& op2 )        const   { return  ! ( *this == op2 ); }


private:

    long            LimitMin;
    long            LimitMax;
    long            PosMin;
    long            PosMax;

    bool            Extending;
    long            PosPivot;
    long            PosExtend;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
