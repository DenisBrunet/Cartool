/************************************************************************\
© 2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "Strings.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrapping a fixed, C-style string which can therefor be stacked, like returned objects
                                        // Parameter N is the SIZE of the array, hence max string length will be (N-1)
                                        // When testing with other strings, do not forget to account for any possible clipping
template <long N>
class   TFixedString
{
public:

    inline                  TFixedString            ()                                          { Clear ();                                     }
    inline                  TFixedString            ( const TFixedString& str )                 { ::StringCopy ( String, str.String, N - 1 );   }
    inline                  TFixedString            ( const char*         str )                 { ::StringCopy ( String, str,        N - 1 );   }


    inline bool             IsEmpty                 ()                          const           { return *String == EOS;            }
    inline bool             IsNotEmpty              ()                          const           { return *String != EOS;            }
    inline bool             IsFull                  ()                          const           { return Length () == N - 1;        }
    inline bool             IsNotFull               ()                          const           { return Length () <  N - 1;        }


    inline TFixedString&    Append                  ( const TFixedString& str )                 { return Append ( str.String );     }
    inline TFixedString&    Append                  ( const char*         str );
    inline void             Clear                   ()                                          { ::ClearString  ( String, N );     }   // reset the whole array
    inline TFixedString&    Copy                    ( const TFixedString& str )                 { ::StringCopy   ( String, str.String, N - 1 ); return  *this; }
    inline TFixedString&    Copy                    ( const char*         str )                 { ::StringCopy   ( String, str,        N - 1 ); return  *this; }
    inline long             Length                  ()                          const           { return std::strlen ( String );    }
    inline long             Size                    ()                          const           { return N;                         }


    inline TFixedString&    operator    =          ( const TFixedString& op2 )                  { return &op2 != this ? Copy ( op2 ) : *this;   }
    inline TFixedString&    operator    =          ( const char*         op2 )                  { return                Copy ( op2 );           }


          char&             operator    []          ( long i )                                  { return String[ i                           ]; }   // !no boundary checks!
    const char&             operator    []          ( long i )                  const           { return String[ i                           ]; }
          char&             operator    ()          ( long i )                                  { return String[ Clip ( i, (long) 0, N - 1 ) ]; }   // !with boundary checks!
    const char&             operator    ()          ( long i )                  const           { return String[ Clip ( i, (long) 0, N - 1 ) ]; }

                                                    // !default is case sensitive!
    int                     Compare                 ( const TFixedString& op, StringFlags flags = CaseSensitive )   const   { return ::StringCompare ( String, op.String, flags ); }
    int                     Compare                 ( const char*         op, StringFlags flags = CaseSensitive )   const   { return ::StringCompare ( String, op,        flags ); }    // !char* op is NOT truncated to N-1, which could have side-effects!

    bool                    operator    ==          ( const TFixedString& op  ) const           { return Compare ( op ) == 0;       }   // !default is case sensitive!
    bool                    operator    !=          ( const TFixedString& op  ) const           { return Compare ( op ) != 0;       }
    bool                    operator    <           ( const TFixedString& op  ) const           { return Compare ( op ) <  0;       }
    bool                    operator    <=          ( const TFixedString& op  ) const           { return Compare ( op ) <= 0;       }
    bool                    operator    >           ( const TFixedString& op  ) const           { return Compare ( op ) >  0;       }
    bool                    operator    >=          ( const TFixedString& op  ) const           { return Compare ( op ) >= 0;       }

    bool                    operator    ==          ( const char*         op  ) const           { return Compare ( op ) == 0;       }   // !default is case sensitive!
    bool                    operator    !=          ( const char*         op  ) const           { return Compare ( op ) != 0;       }
    bool                    operator    <           ( const char*         op  ) const           { return Compare ( op ) <  0;       }
    bool                    operator    <=          ( const char*         op  ) const           { return Compare ( op ) <= 0;       }
    bool                    operator    >           ( const char*         op  ) const           { return Compare ( op ) >  0;       }
    bool                    operator    >=          ( const char*         op  ) const           { return Compare ( op ) >= 0;       }

    friend bool             operator    ==          ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) == 0;  }   // inverting operator and operands - !also avoiding casting the char* to a TFixedString, which could truncate it!
    friend bool             operator    !=          ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) != 0;  }
    friend bool             operator    <           ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) >  0;  }
    friend bool             operator    <=          ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) >= 0;  }
    friend bool             operator    >           ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) <  0;  }
    friend bool             operator    >=          ( const char* opl, const TFixedString& opr ){ return opr.Compare ( opl ) <= 0;  }


    TFixedString&           operator    +=          ( const TFixedString& op2 )                 { return Append ( op2 );                        }
    TFixedString&           operator    +=          ( const char*         op2 )                 { return Append ( op2 );                        }
    TFixedString            operator    +           ( const TFixedString& op2 ) const           { return TFixedString ( *this ).Append ( op2 ); }   // concatenate to the right
    TFixedString            operator    +           ( const char*         op2 ) const           { return TFixedString ( *this ).Append ( op2 ); }   // concatenate to the right
    friend TFixedString     operator    +           ( const char* opl, const TFixedString& opr ){ return TFixedString ( opl   ).Append ( opr ); }   // concatenate to the left


    friend std::ostream&    operator    <<          ( std::ostream& os, const TFixedString<N>& str )    { os << str.String; return os; }


                            operator          char* ()                                          { return String;    }
                            operator    const char* ()                          const           { return String;    }
    explicit                operator    long        ()                          const           { return Length (); }
    explicit                operator    int         ()                          const           { return Length (); }


protected:

    char                    String[ N ];            // array size, INCLUDING null char terminator

};


template <long N>
TFixedString<N>&    TFixedString<N>::Append ( const char* str )           
{
long            currlen     = Length ();

if ( currlen < N - 1 )
    ::StringCopy   ( String + currlen, str, N - 1 - currlen );

return *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
