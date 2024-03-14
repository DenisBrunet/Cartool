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

#include    <owl/contain.h>             // TStringArray (for ListBox in dialogs)

#include    "TList.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                            GrepOption;
template <class TypeD> class    TArray2;


enum            StringOptions
                {
                StringAutoSize,             // each string has exactly its needed room
                StringAutoSizePlusMargin,   // same, but adding some margin
                StringFixedSize,            // providing exact size, clipping strings
                };


class   TStrings
{
public:
                    TStrings    ();
                    TStrings    ( int numstrings, long stringlength )                             { Set ( numstrings, stringlength ); }
                    TStrings    ( const char*         string  )                                   { SetOnly ( string ); }
                    TStrings    ( const TStrings*     strings )                                   { Set ( strings ); }
                    TStrings    ( const TList<char>&  strings )                                   { Set ( strings ); }
                    TStrings    ( const owl::TStringArray& strings )                              { Set ( strings ); }
                    TStrings    ( const TArray2<char>&  arraychar )                               { Set ( arraychar ); }

    virtual        ~TStrings    ();


    int             NumStrings              ()  const       { return    Strings.Num ();         }
    bool            IsEmpty                 ()  const       { return    Strings.IsEmpty ();     }
    bool            IsNotEmpty              ()  const       { return    Strings.IsNotEmpty ();  }
    long            GetMinStringLength      ()  const;
    long            GetMaxStringLength      ()  const;
    int             GetLongestStringIndex   ()  const;

          char*     GetFirst ()                             { return    Strings.GetFirst (); }
    const char*     GetFirst ()                 const       { return    Strings.GetFirst (); }
          char*     GetLast  ()                             { return    Strings.GetLast  (); }
    const char*     GetLast  ()                 const       { return    Strings.GetLast  (); }


    virtual void    Reset           ();

    void            Set             ( int numstrings, long stringsize );
    void            Set             ( const TStrings*    strings );
    void            Set             ( const TStrings&    strings );
    void            Set             ( const TList<char>& strings );
    void            Set             ( const char* strings, const char* separators );
    void            Set             ( const owl::TStringArray& strings );   // OWL strings
    void            Set             ( const TArray2<char>& arraychar );     // // "C"-like array of chars
    void            SetOnly         ( const char* string );


    void            Add             ( const char* string, StringOptions how, long length = 0 );   // the work horse
    virtual void    Add             ( const char* string );
    virtual void    Add             ( const char* string, long length );
    void            Add             ( const TStrings& stringlist );
    void            Add             ( int    v, int width = 0 );
    void            Add             ( double v, int width, int precision );
    void            Add             ( double v, int precision );
    void            Add             ( double v );
    void            AddNoDuplicates ( const char* string );

    void            Remove          ( const char* string );     // any string
    void            RemoveRef       ( const char* string );     // string must be one within Strings list, usually Strings[ index ]
    void            GrepKeep        ( const char* regexp, GrepOption options );
    void            GrepRemove      ( const char* regexp, GrepOption options );
    void            RemoveFirst     ( int num = 1 );
    void            RemoveLast      ( int num = 1 );

    void            RevertOrder     ();

    void            CopyTo          ( TArray2<char> &stringsarray ) const;  // copy the strings
    virtual void    Sort            ();

    char*           Concatenate     ( char* concatstring, const char* separator )   const;
    const char*     Contains        ( const char* string )                          const;
    const char*     PartiallyContains( const char* string )                         const;
    bool            ContainsAny     ( const TStrings &strings )                     const;
    bool            HasNoDuplicates ()                                              const;
    bool            HasOnlyDuplicates ()                                            const;
    void            RemoveDuplicates  ();

    bool            AllStringsGrep  ( const char* regexp, GrepOption options )      const;  // true if all strings Grep to true
    bool            SomeStringsGrep ( const char* regexp, GrepOption options )      const;  // true if at least 1 string Grep to true
    void            StringsReplace  ( const char* tobereplaced, char* replacedwith );


    int             GetIndex        ( const char* string    )   const;
    void            Show            ( const char* title = 0 )   const;


    TStrings                            ( const TStrings &op  );
    TStrings&       operator    =       ( const TStrings &op2 );


    char*           operator    []              ( int index )       { return Strings[ index ]; }
    const char*     operator    []              ( int index ) const { return Strings[ index ]; }    // access from const object forces returning a const char*
    char*           operator    ()              ( int index )       { return Strings[ index ]; }
    const char*     operator    ()              ( int index ) const { return Strings[ index ]; }    // access from const object forces returning a const char*

                    operator    int                 ()  const       { return (int)  Strings; }
                    operator    bool                ()  const       { return (bool) Strings; }
                    operator          TList<char>&  ()              { return Strings; }
                    operator    const TList<char>&  ()  const       { return Strings; }


    bool            operator    ==              ( const TStrings &op )   const;
    bool            operator    !=              ( const TStrings &op )   const;


protected:

    crtl::TList<char>   Strings;

    void            Sort  ( int l, int r );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
