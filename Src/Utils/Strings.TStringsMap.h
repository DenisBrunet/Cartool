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
                                        // Associates string pairs, but currently used for ("integer",string) pairs, integers being stored as strings
                                        // Uses 2 string list for storage; for a faster solution, use some std::map
                                        // Certainly should be using some std::unordered_map

                                        // Note:
                                        // - There is no restriction to the # of pairs, and # of pairs for a given first value
                                        // - First element is not bound to be an integer, but calling ValueToKey will convert it to an integer (probably 0)
class               TAALRoi;


class  TStringsMap
{
public:

    bool            IsEmpty     ()  const       { return  Key.IsEmpty    (); }
    bool            IsNotEmpty  ()  const       { return  Key.IsNotEmpty (); }

    int             GetNumPairs ()  const       { return    (int) Key; }
    int             GetMaxKey   ()  const ;     // from all pairs


    void            Add         ( const char* file  );                  // add each line as a series of pairs "first token"-"all tokens"
    void            Add         ( const TAALRoi* aal, int numlines );   // add from the AAL array
    void            Add         ( int min, int max );                   // add a range of indexes


    bool            ValueToKey  ( const char* value, int&  key   )  const;
    char*           KeyToValue  ( int         key,   char* value )  const;
    const char*     GetValue    ( int         key )                 const;


protected:

    TStrings        Key;
    TStrings        Value;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
