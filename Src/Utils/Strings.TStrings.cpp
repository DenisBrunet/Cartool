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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.TStrings.h"
#include    "Strings.Grep.h"
#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TStrings&    TStrings::operator= ( const TStrings &op2 )
{
if ( &op2 == this )
    return  *this;

Set ( (TStrings &) op2 );

return  *this;
}


//----------------------------------------------------------------------------
bool    TStrings::operator== ( const TStrings &op )   const
{
                                        // different lengths -> different string lists
if ( Strings.Num () != op.NumStrings () )

    return  false;

                                        // now compare each element one by one
for ( int i = 0; i < (int) Strings; i++ ) 
                                        // early exit?
    if ( StringIsNot ( Strings[ i ], op[ i ] ) )

        return  false;


return  true;
}


bool    TStrings::operator!= ( const TStrings &op )   const
{
return  ! operator== ( op );
}


//----------------------------------------------------------------------------
long    TStrings::GetMinStringLength ()  const
{
if ( IsEmpty () )
    return  0;


long                lenmin          = LONG_MAX;
                                        // get shortest string
for ( int i = 0; i < (int) Strings; i++ ) 
    Mined ( lenmin, StringLength ( Strings[ i ] ) );


return  lenmin;
}


long    TStrings::GetMaxStringLength ()  const
{
if ( IsEmpty () )
    return  0;


long                lenmax          = 0;
                                        // get longest string
for ( int i = 0; i < (int) Strings; i++ ) 
    Maxed ( lenmax, StringLength ( Strings[ i ] ) );


return  lenmax;
}


int     TStrings::GetLongestStringIndex ()    const
{
if ( IsEmpty () )
    return  -1;


int                 imax            = -1;
long                lenmax          = 0;
                                        // get longest string
for ( int i = 0; i < (int) Strings; i++ )
                                        // test will also work for empty strings, i.e. length = 0
    if ( StringLength ( Strings[ i ] ) >= lenmax ) {

        lenmax  = StringLength ( Strings[ i ] );
        imax    = i;
        }

return  imax;
}


//----------------------------------------------------------------------------
void    TStrings::Reset ()
{
if ( IsEmpty () )
    return;


for ( int i = 0; i < (int) Strings; i++ )

    delete[] ( Strings[ i ] );


Strings.Reset ( false );                // then the pointers


                                        // when allocation will be an object, and not an array of char:
//                                      // delete content & structure
//Strings.Reset ( true );
}


//----------------------------------------------------------------------------
                                        // Pre-allocating a number of strings with known max size (includes EOS)
void    TStrings::Set ( int numstrings, long stringsize )
{
Reset ();

for ( int i = 0; i < numstrings; i++ )
    Add ( "", stringsize );
}


void    TStrings::Set ( const TStrings&    strings )
{
if ( &strings == this )
    return;

Reset ();

for ( int i = 0; i < (int) strings; i++ )
    Add ( strings[ i ] );
}


void    TStrings::Set ( const vector<string>&    strings )
{
for ( int i = 0; i < strings.size (); i++ )
    Add ( strings[ i ].c_str () );
}


void    TStrings::Set ( const char* strings, const char* separators )
{
Reset ();

TSplitStrings       split ( strings, NonUniqueStrings, separators );

for ( int i = 0; i < (int) split; i++ )
    Add ( split[ i ] );
}

                                        // OWL strings
void    TStrings::Set ( const owl::TStringArray& strings )
{
Reset ();

for ( UINT i = 0; i < strings.GetItemsInContainer(); i++ )
    Add ( strings[ i ].c_str () );
}

                                        // "C"-like array of chars
void    TStrings::Set ( const TArray2<char>& arraychar )
{
int                 numstrings      = arraychar.GetDim1 ();
int                 stringsize      = arraychar.GetDim2 ();

Set ( numstrings, stringsize );

if ( stringsize == 0 )
    return;

for ( int i = 0; i < numstrings; i++ )
    StringCopy ( Strings[ i ], arraychar[ i ], stringsize - 1 );
}


void    TStrings::SetOnly ( const char* string )
{
Reset ();

Add ( string );
}


//----------------------------------------------------------------------------
                                        // Will copy while specifically handling the allocated space
                                        // In all cases, the new string will be null padded if needed
                                        // StringFixedSize  will interpret length as a SIZE, so only legnth-1 chars will be actually copied
void    TStrings::Add ( const char* string, StringOptions how, long length )
{
                                        // size accounts for the null character, while length is the number of visible chars
long                size            = IsFlag ( how, StringAutoSize            )   ? StringSize ( string )           // exactly the space needed
                                    : IsFlag ( how, StringAutoSizePlusMargin  )   ? StringSize ( string ) + length  //     "    "    "     "    + some margin
                                    : IsFlag ( how, StringFixedSize           )   ? AtLeast ( 1L, length )          // fixed size no matter what - will clip the results
                                    :                                               StringSize ( string );          // default is auto

char*               toc             = new char [ size ];

                                        // we allocated more than the copied part, let reset the new string in full
if ( IsFlag ( how, StringAutoSizePlusMargin )
  || IsFlag ( how, StringFixedSize          ) 
  || string == 0                              )

    ClearString ( toc, size );


if ( string == 0 )
                                        // nothing to be copied if null pointer    
    ;

else if ( IsFlag ( how, StringFixedSize ) ) {
                                        // will add a null character at the end of buffer
    if ( length > 0 )
                                        // !length of 0 will call with 0-1 = -1, which will copy the whole string!
        StringCopy  ( toc, string, length - 1 );
    }
else                                
    StringCopy  ( toc, string );


Strings.Append  ( toc );
}


//----------------------------------------------------------------------------
void    TStrings::Add ( const char* string )
{
Add ( string, StringAutoSize );
}


void    TStrings::Add ( const char* string, long length )
{
Add ( string, StringFixedSize, length );
}


void    TStrings::Add ( const TStrings& stringlist )
{
TListIterator<char>     iterator;

foreachin ( stringlist, iterator )

    Add ( iterator () );
}


void    TStrings::AddNoDuplicates ( const char* string )
{
if ( string && ! Contains ( string ) )

    Add ( string );
}


void    TStrings::Add ( int v, int width )
{
char*               toc             = new char [   1                                        // sign
                                                 + max ( width, NumIntegerDigits ( v ) )    // integer part width itself
                                                 + 1                                        // EOS
                                                ];

IntegerToString ( toc, v, width );

Strings.Append  ( toc );
}


void    TStrings::Add ( double v, int width, int precision )
{
char                buff[ 256 ];
                                        // this one is hard to guess - so not guessing it, just compute it
FloatToString ( buff, v, width, precision );

char*               toc             = new char [ StringSize ( buff ) ];

StringCopy      ( toc, buff );

Strings.Append  ( toc );
}


void    TStrings::Add ( double v, int precision )
{
char                buff[ 256 ];
                                        // this one is hard to guess - so not guessing it, just compute it
FloatToString ( buff, v, precision );

char*               toc             = new char [ StringSize ( buff ) ];

StringCopy      ( toc, buff );

Strings.Append  ( toc );
}


void    TStrings::Add ( double v )
{
char                buff[ 256 ];
                                        // this one is hard to guess - so not guessing it, just compute it
FloatToString ( buff, v );

char*               toc             = new char [ StringSize ( buff ) ];

StringCopy      ( toc, buff );

Strings.Append  ( toc );
}


//----------------------------------------------------------------------------
void    TStrings::CopyTo ( TArray2<char> &stringsarray ) const
{
                                        // make sure we have enough room
stringsarray.Resize ( max ( NumStrings (),                   stringsarray.GetDim1 () ), 
                      max ( (int) GetMaxStringLength () + 1, stringsarray.GetDim2 () ) );

                                        // make sure all space is cleared
stringsarray.ResetMemory ();

                                        // copy is now safe
for ( int i = 0; i < (int) Strings; i++ )

    StringCopy ( stringsarray[ i ], Strings[ i ] );
}


//----------------------------------------------------------------------------
const char* TStrings::Contains ( const char* string )    const
{
if ( IsEmpty () || StringIsEmpty ( string ) )
    return  0;


for ( int i = 0; i < (int) Strings; i++ ) 
    if ( StringIs ( Strings[ i ], string ) )    // compare content of string
        return  Strings[ i ];


return  0;
}

                                        // Test for partial fit of Strings only
const char* TStrings::PartiallyContains ( const char* string )    const
{
if ( IsEmpty () || StringIsEmpty ( string ) )
    return  0;


for ( int i = 0; i < (int) Strings; i++ ) 
    if ( StringContains ( string, Strings[ i ] ) )
        return  Strings[ i ];


return  0;
}


bool    TStrings::ContainsAny ( const TStrings &strings )    const
{
if ( IsEmpty () || strings.IsEmpty () )
    return  false;


for ( int i = 0; i < (int) strings; i++ ) 
    if ( Contains ( strings[ i ] ) )
        return  true;


return  false;
}


bool    TStrings::HasNoDuplicates ()     const
{
if ( IsEmpty () )
    return  true;


for ( int i = 0;     i < (int) Strings; i++ ) 
for ( int j = i + 1; j < (int) Strings; j++ )

    if ( StringIs ( Strings[ i ], Strings[ j ] ) )
         return false;


return  true;
}


bool    TStrings::HasOnlyDuplicates ()   const
{
if ( IsEmpty () )
    return  true;


for ( int i = 0;     i < (int) Strings; i++ ) 
for ( int j = i + 1; j < (int) Strings; j++ )

    if ( StringIsNot ( Strings[ i ], Strings[ j ] ) )
         return false;


return  true;
}


void    TStrings::RemoveDuplicates ()
{
if ( IsEmpty () )
    return;

TStrings            copy ( *this );

Reset ();

for ( int i = 0; i < (int) copy; i++ )

    AddNoDuplicates ( copy[ i ] );
}


//----------------------------------------------------------------------------
bool    TStrings::AllStringsGrep ( const char* regexp, GrepOption options )    const
{
if ( IsEmpty () )
    return  false;


TStringGrep         greppy ( regexp, options );


for ( int i = 0; i < (int) Strings; i++ )
    if ( ! greppy.Matched ( Strings[ i ] ) )
         return false;


return  true;
}


bool    TStrings::SomeStringsGrep ( const char* regexp, GrepOption options )   const
{
if ( IsEmpty () )
    return  false;


TStringGrep         greppy ( regexp, options );


for ( int i = 0; i < (int) Strings; i++ ) 
    if ( greppy.Matched ( Strings[ i ] ) )
         return true;


return  false;
}


//----------------------------------------------------------------------------
void    TStrings::StringsReplace ( const char* tobereplaced, char* replacedwith )
{
for ( int i = 0; i < (int) Strings; i++ ) 
    StringReplace ( Strings[ i ], tobereplaced, replacedwith );
}


//----------------------------------------------------------------------------
int     TStrings::GetIndex ( const char* string )    const
{
if ( IsEmpty () )
    return  -1;


for ( int i = 0; i < (int) Strings; i++ ) 
    if ( StringIs ( Strings[ i ], string ) ) // compare content of string
        return  i;

return  -1;
}


//----------------------------------------------------------------------------
void    TStrings::Show ( const char* title ) const
{
if ( IsEmpty () ) {
    ShowMessage ( "- Empty List -", StringIsEmpty ( title ) ? "String List" : title );
    return;
    }


char                buff      [ 32 ];
char                localtitle[ 256 ];


for ( int i = 0; i < (int) Strings; i++ ) {

    StringCopy  ( localtitle, StringIsEmpty ( title ) ? "String List" : title, " / Item#", IntegerToString ( buff, i + 1 ) );

    ShowMessage ( Strings[ i ], localtitle );
    }

}


//----------------------------------------------------------------------------
char*   TStrings::Concatenate    (   char*   concatstring,   const char* separator   )   const
{
ClearString     ( concatstring );

if ( concatstring == 0 || IsEmpty () )
    return  concatstring;


StringCopy  ( concatstring, Strings[ 0 ] );

for ( int i = 1; i < (int) Strings; i++ )

    StringAppend    ( concatstring, separator, Strings[ i ] );


return  concatstring;
}


//----------------------------------------------------------------------------
                                        // string MUST belongs to TStrings, it works by comparing pointers only
void    TStrings::RemoveRef ( const char* string )
{
if ( IsEmpty () )
    return;

                                        // !Removes by comparing pointers, NOT string!
Strings.Remove ( (char *) string );      // from list

delete[] ( string );                    // from memory
}


                                        // string does not belong to TStrings, it works by comparing content
void    TStrings::Remove ( const char* string )
{
if ( IsEmpty () )
    return;

                                        // get the reference to the inner string
for ( int i = 0; i < (int) Strings; i++ ) 

    if ( StringIs ( Strings[ i ], string ) ) {

        RemoveRef ( Strings[ i ] );

        break;
        }
}


void    TStrings::GrepKeep ( const char* regexp, GrepOption options )
{
if ( IsEmpty () )
    return;


TStringGrep         greppy ( regexp, options );

                                        // get the reference to the inner string
for ( int i = 0; i < (int) Strings; i++ ) 

    if ( ! greppy.Matched ( Strings[ i ] ) ) {

        RemoveRef ( Strings[ i ] );
        i--;
        }
}


void    TStrings::GrepRemove ( const char* regexp, GrepOption options )
{
if ( IsEmpty () )
    return;


TStringGrep         greppy ( regexp, options );

                                        // get the reference to the inner string
for ( int i = 0; i < (int) Strings; i++ ) 

    if ( greppy.Matched ( Strings[ i ] ) ) {

        RemoveRef ( Strings[ i ] );
        i--;
        }
}


void    TStrings::RemoveLast ( int num )
{
if ( num < 1 )
    return;

if ( num >= NumStrings () ) {
    Reset ();
    return;
    }

for ( ; num > 0; num-- )
    RemoveRef ( GetLast () );
}


void    TStrings::RemoveFirst ( int num )
{
if ( num < 1 )
    return;

if ( num >= NumStrings () ) {
    Reset ();
    return;
    }

for ( ; num > 0; num-- )
    RemoveRef ( GetFirst () );
}


//----------------------------------------------------------------------------
void    TStrings::RevertOrder ()
{
Strings.RevertOrder ();
}

                                        // !Do not call in multi-threads!
void    TStrings::Sort ()
{
if ( NumStrings () <= 1 )
    return;

Sort ( 0, NumStrings () - 1 );
                                        // we manually changed the content, force update indexes
Strings.UpdateIndexes ( true );
}

                                        // Works directly with atoms from the list
void    TStrings::Sort ( int l, int r )
{
if ( r <= l )   return;


int                 i               = l;
int                 j               = r;
const char*         v               = Strings[ ( l + r ) / 2 ];


do {
    while ( _stricmp ( Strings[ i ], v            ) < 0 )   i++;
    while ( _stricmp ( v,            Strings[ j ] ) < 0 )   j--;

    if ( i <= j )
        Permutate ( Strings.GetAtom ( i++ )->To, Strings.GetAtom ( j-- )->To );   // actual data remain in place, we only permutate the 'To' parts

    } while ( i <= j );


if ( l < j )    Sort ( l, j );
if ( i < r )    Sort ( i, r );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
