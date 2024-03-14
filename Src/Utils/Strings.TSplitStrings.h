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

#include    "Strings.TStrings.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Splitting a string into tokens
                                        // handling the case of "" quoted tokens
                                        // Individual tokens are gracefully allocated with more space to them, allowing for some simple editing (like Regex neutralization)

constexpr char*     SplitStringsSeparators  = " \t\n,;";

                                        // Control if repeating tokens are allowed or not
enum                StringsUnicity 
                    {
                    UniqueStrings,
                    NonUniqueStrings
                    };


class   TSplitStrings
{
public:
                    TSplitStrings ();
                    TSplitStrings ( const char* str, StringsUnicity unicity, const char* separators = SplitStringsSeparators );


    bool            IsEmpty                 ()  const           { return  Tokens.IsEmpty ();                }
    int             GetNumTokens            ()  const           { return  Tokens.NumStrings ();             }
    int             GetBiggestTokenIndex    ()  const           { return  Tokens.GetLongestStringIndex ();  }
    long            GetBiggestTokenLength   ()  const           { return  Tokens.GetMaxStringLength ();     }


    void            Set                 ( const char* str, StringsUnicity unicity, const char* separators = SplitStringsSeparators );
    void            Reset               ();

    void            Add                 ( const char* str, StringsUnicity unicity, const char* separators = SplitStringsSeparators );
    void            Add                 ( const TSplitStrings& splitstring, StringsUnicity unicity );
    void            AddToken            ( const char* token )   { Tokens.Add ( token, StringAutoSizePlusMargin, strlen ( token ) ); } // directly adding a token, with extra room for safety

    void            Remove              ( const char* removestrs );
    void            RemoveDuplicates    ();


    void            ExpandWildchars     ( TStrings&    strs, StringsUnicity unicity );
    void            FilterWith          ( TStrings&    strs, bool silent = true );
    void            CompactSpaces       ();

    bool            Contains            ( const char* str )         const;
    bool            PartiallyContains   ( const char* str )         const;
    char*           ToString            ( char* str, bool verbose ) const;
    void            Show                ( const char* title = 0 )   const;
    const TStrings&     GetTokens       ()      const           { return Tokens; }


//  char*           operator    []      ( int i )               { return  IsInsideLimits ( i, 0, GetNumTokens () - 1 ) ? Tokens[ i ] : 0; }
//  const char*     operator    []      ( int i )   const       { return  IsInsideLimits ( i, 0, GetNumTokens () - 1 ) ? Tokens[ i ] : 0; }
    char*           operator    []      ( int i )               { return  i >= 0 && i <= GetNumTokens () - 1 ? Tokens[ i ] : 0; }
    const char*     operator    []      ( int i )   const       { return  i >= 0 && i <= GetNumTokens () - 1 ? Tokens[ i ] : 0; }

//  TSplitStrings&  operator    +=      ( const TSplitStrings &op2 );

                    operator    bool    ()      const           { return  (bool) Tokens; }
                    operator    int     ()      const           { return  (int)  Tokens; }


protected:

    TStrings        Tokens;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
