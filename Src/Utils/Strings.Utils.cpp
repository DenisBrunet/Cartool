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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    <owl/checkbox.h>            // BF_CHECKED

#include    <Shlwapi.h>                 // StrStrI

#include    "Strings.Utils.h"
#include    "Strings.TSplitStrings.h"
#include    "MemUtil.h"
#include    "Files.Utils.h"
#include    "Math.Random.h"
#include    "Time.Utils.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool    StringIs ( const char* s1, const char* s2 )
{
if ( ! s1 || ! s2 )
    return false;

if ( ! *s1 && ! *s2 )
    return true;

return  ! _stricmp ( s1, s2 );
}


bool    StringIsNot ( const char* s1, const char* s2 )
{
return  ! StringIs ( s1, s2 );
}


bool    StringIsEmpty ( const char* s )
{
return  s == 0 || ( s != 0 && *s == 0 );
}


bool    StringIsNotEmpty ( const char* s )
{
return  s != 0 && *s != 0;
}


bool    StringIsSpace ( const char* s )
{
if ( ! s || ! *s )
    return  true;


for ( long i = StringLength ( s ) - 1; i >= 0; i-- )
    if ( ! IsSpaceNewline ( s[ i ] ) )
        return  false;

return  true;
}


//----------------------------------------------------------------------------
bool    isconsonant ( char c )
{
return  StringContains ( "bcdfghjklmnpqrstvwxzBCDFGHJKLMNPQRSTVWXZçñ", c, StringContainsCase );
}


bool    isvowel ( char c )
{
return  isalpha ( c ) && ! isconsonant ( c );
}

                                        // only space and tab
bool    IsSpace ( char c )
{
return c == ' ' || c == '\t';
}

                                        // space, tab, newline \n, carriage return \r
bool    IsSpaceNewline ( char c )
{
return isspace ( c );
}

                                        // space, tab, newline \n, carriage return \r, comma ',', semcolon ';'
bool    IsSpaceNewlineComma ( char c )
{
return isspace ( c ) || c == ',' || c == ';';
}

                                        // space, tab, comma ',', semcolon ';'
bool    IsSpaceComma ( char c )
{
return c == ' ' || c == '\t' || c == ',' || c == ';';
}


//----------------------------------------------------------------------------
                                        // !NO newline and the like!
bool    StringToTrue ( const char* s )
{
if ( !s || !*s )
    return false;

return  IsStringAmong ( s, "y yes ok true" );
}


bool    StringToFalse ( const char* s )
{
if ( !s || !*s )
    return false;

return  IsStringAmong ( s, "n no false" ) || StringStartsWith ( s, "none" );
}


bool    StringToBool ( char* s )
{
if ( !s || !*s )
    return false;

StringCleanup ( s );

if      ( StringToTrue  ( s ) )     return true;
else if ( StringToFalse ( s ) )     return false;
else                                return false;   // don't know? say false
}


const char* BoolToString ( bool b )
{
return  b ? "Yes" : "No";
}


const char* SignToString ( double v )
{
return  v < 0 ? "-" : "";   // positive case could optionally be "+"
}

/*
char*   BoolToString ( char* s, bool b )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing, caller still might need some string

StringCopy  ( s, b ? "Yes" : "No" );

return  s;
}


TFileName   BoolToString ( bool b )
{
TFileName           s;

StringCopy  ( s, b ? "Yes" : "No" );

return  s;
}
*/

bool            CheckToBool ( TCheckBoxData c )
{
return  c == owl::BF_CHECKED;
}


TCheckBoxData   BoolToCheck ( bool b )
{
return  b ? owl::BF_CHECKED : owl::BF_UNCHECKED;
}


TCheckBoxData   StringToCheck ( char* s )
{
return  BoolToCheck ( StringToBool ( s ) );
}


char*   StringPlural    ( bool isplural,   bool space )
{
return  isplural ? "s" : space ? " " : "";
}


char*   StringPlural    ( int  v,          bool space )
{
return  StringPlural ( v > 1, space );
}


bool    StringStartsWith ( const char* searched, const char* tosearch, int maxlength )
{
if ( !searched || !tosearch )
    return  false;

if ( !*searched && !*tosearch )
    return  true;

return  ! _strnicmp ( searched, tosearch, maxlength == -1 ? StringLength ( tosearch ) : maxlength );
}

                                        // StringContainsBackward not implemented
const char*   StringContains ( const char* searched, const char* tosearch, int flags )
{
if ( !searched || !tosearch )
    return  0;

if ( !*searched && !*tosearch )
    return  searched;


const char*         toc;

if ( flags & StringContainsCase )

    toc     = strstr ( searched, tosearch );
                                        // default is no case
else /*if ( flags & StringContainsNocase )*/ {
                                        // Windows function - see your compiler
    toc     = StrStrI ( searched, tosearch );

/*                                      // Manual implementation
                                        // local copy to change case
    char*               searchedi       = new char [ StringSize ( searched ) ];
    char*               tosearchi       = new char [ StringSize ( tosearch ) ];

    StringCopy          ( searchedi, searched );
    StringCopy          ( tosearchi, tosearch );

    StringToLowercase   ( searchedi );
    StringToLowercase   ( tosearchi );


    toc             = strstr ( searchedi, tosearchi );

    if ( toc )
        toc = searched + ( toc - searchedi );


    delete[]    searchedi;
    delete[]    tosearchi;
*/
    }


return  toc;
}

                                        // StringContainsCase / StringContainsNocase not implemented
char*               StringContains   ( char* searched, char* tosearch, int flags )
{
return  (char *) StringContains ( (const char* ) searched, (const char* ) tosearch, flags ); 
}

                                        // Testing a single char
const char*   StringContains ( const char* searched, const char tosearch, int flags )
{
if ( !searched || !*searched )
    return  0;


if      ( flags & StringContainsBackward )      return  strrchr ( searched, tosearch );
                                        // default is forward
else  /*( flags & StringContainsForward  )*/    return  strchr  ( searched, tosearch );
}


char*   StringContains ( char* searched, char tosearch, int flags )
{
return  (char *) StringContains ( (const char* ) searched, (const char) tosearch, flags ); 
}


bool    StringEndsWith ( const char* searched, const char* tosearch )
{
if ( !searched || !tosearch )
    return false;

if ( !*searched && !*tosearch )
    return true;

return  StringLength ( searched ) >= StringLength ( tosearch ) ? ! _strnicmp ( StringEnd ( searched ) - StringLength ( tosearch ), tosearch, StringLength ( tosearch ) )
                                                               : false;
}


bool    IsStringAmong ( const char* search, const char* among )
{
if ( search == 0 || among == 0 )
    return false;


TSplitStrings       splitamong ( among, UniqueStrings, SplitStringsSeparators );


return  splitamong.Contains ( search );
}

                                        // !case sensitive!
bool    IsCharAmong ( const char search, const char* among )
{
if ( search == EOS || among == 0 )
    return false;


return  strchr ( among, search );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    ClearString ( char* s, long size )
{
if ( ! s )
    return;

if ( size > 0 )     ClearVirtualMemory ( s, size );
else               *s = EOS;
}


void    ClearString ( UINT8* s, long size )
{
if ( ! s )
    return;

if ( size > 0 )     ClearVirtualMemory ( s, size );
else               *s = EOS;
}


void    SetString ( char* s, char fill, long size )
{
if ( ! s && size > 0 )
    return;
                                        // fill memory
SetVirtualMemory ( s, size, fill );
                                        // set EOS
*( s + size )   = EOS;
}


long    StringLength ( const char* s )
{
if ( ! s )
    return  0;
                                        // although strlen returns a size_t, this has been acknowledged as a mistake, it needs to be signed for proper arithmetic
return  (long) strlen ( s );
}


long    StringLength ( const UCHAR* s )
{
return  StringLength ( (const char*) s ); 
}


long    StringSize ( const char* s )
{
return  StringLength ( s ) + 1;         // accounting for the null terminator
}


char*           StringEnd ( char* s )
{
if ( ! s )
    return  0;

return  s + strlen ( s );
}


const char*     StringEnd ( const char* s )
{
if ( ! s )
    return  0;

return  s + strlen ( s );
}


char*   LastChar ( char* s, long countingfromend )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing

if ( ! *s )                             // empty string
    return  s;                          // no last char...

                                        // past the string
if ( countingfromend <= 0 )
    return  s + strlen ( s );


if ( countingfromend > strlen ( s ) )
    return  s;


return  s + strlen ( s ) - countingfromend;
}


const char*   LastChar ( const char* s, long countingfromend )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing

if ( ! *s )                             // empty string
    return  s;                          // no last char...


                                        // past the string
if ( countingfromend <= 0 )
    return  s + strlen ( s );


if ( countingfromend > strlen ( s ) )
    return  s;


return  s + strlen ( s ) - countingfromend;
}


char*   StringClip ( char* s, long length )
{
if ( StringLength ( s ) > length )

    s[ length ] = EOS;

return  s;
}

                                        // indexes start from 0
char*   StringClip ( char* s, long from, long to )
{
CheckOrder ( from, to );


long                l               = StringLength ( s );

                                        // out of range?
if ( to < 0 || from >= l ) {
    ClearString ( s );
    return  s;
    }

                                        // range encompasses the whole string?
if ( from <= 0 && to >= l - 1 )
    return  s;

                                        // here we have from & to in legal range, with an actual clipping

if ( from == 0 )
    StringClip ( s,           to - from + 1 );
else
    StringCopy ( s, s + from, to - from + 1 );


return  s;
}


char*   StringCopy ( char* to, const char* from, long maxlength )
{
if ( ! to || ! from )
    return  to ? to : "";


if ( to == from ) {
    
    if ( maxlength > 0 )                // optional maxlength?
        StringClip ( to, maxlength );

    return  to;
    }

                                        // either from is empty, or maxlength is 0?
if ( ! *from || maxlength == 0 )

    ClearString ( to );

else                                    // actually something to copy

    if ( maxlength == -1 )              // full copy

        strcpy  ( to, from );

    else {                              // cropped copy
        strncpy ( to, from, maxlength );
                                        // make sure we are null-terminated!
        to[ maxlength ]     = EOS;
        }


return  to;
}


char*   StringCopy ( char* to, const char* from, const char* tail1, const char* tail2, const char* tail3, const char* tail4, const char* tail5, const char* tail6, const char* tail7, const char* tail8 )
{
if ( ! to || ! from )
    return  to ? to : "";


StringCopy   ( to, from );

StringAppend ( to, tail1, tail2, tail3, tail4, tail5, tail6, tail7, tail8 );


return  to;
}


char*   StringAppend ( char* to, const char* tail1, const char* tail2, const char* tail3, const char* tail4, const char* tail5, const char* tail6, const char* tail7, const char* tail8 )
{
if ( ! to )
    return  "";


if ( StringIsNotEmpty ( tail1 ) )   strcat ( to, tail1 );
if ( StringIsNotEmpty ( tail2 ) )   strcat ( to, tail2 );
if ( StringIsNotEmpty ( tail3 ) )   strcat ( to, tail3 );
if ( StringIsNotEmpty ( tail4 ) )   strcat ( to, tail4 );
if ( StringIsNotEmpty ( tail5 ) )   strcat ( to, tail5 );
if ( StringIsNotEmpty ( tail6 ) )   strcat ( to, tail6 );
if ( StringIsNotEmpty ( tail7 ) )   strcat ( to, tail7 );
if ( StringIsNotEmpty ( tail8 ) )   strcat ( to, tail8 );

/*
                                        // any faster?
if ( StringIsNotEmpty ( tail1 ) )
    CopyVirtualMemory ( StringEnd ( to ), tail1, StringSize ( tail1 ) );

if ( StringIsNotEmpty ( tail2 ) )
    CopyVirtualMemory ( StringEnd ( to ), tail2, StringSize ( tail2 ) );

if ( StringIsNotEmpty ( tail3 ) )
    CopyVirtualMemory ( StringEnd ( to ), tail3, StringSize ( tail3 ) );
*/

return  to;
}


char*   StringAppend ( char* to, long tail )
{
char                tailstring[ 256 ];

return  StringAppend ( to, IntegerToString ( tailstring, tail ) );
}


char*   StringAppend ( char* to, double tail )
{
char                tailstring[ 256 ];

return  StringAppend ( to, IntegerToString ( tailstring, tail ) );
}


char*   StringPrepend ( char* to, const char* head )
{
if ( ! to )
    return  "";


if ( StringIsNotEmpty ( head ) ) {

    long                sizehead        = StringLength ( head );
    long                sizeto          = StringLength ( to );
    long                newsize         = sizehead + sizeto;

                     // Dest            Source
    MoveVirtualMemory ( to + sizehead,  to,                             sizeto   );  // shift content
    CopyVirtualMemory ( to,             const_cast< char*  > ( head ),  sizehead );  // insert head

    to[ newsize ]   = EOS;              // new EOS
    }


return  to;
}


char*   AppendSeparator ( char* to, const char* separator )
{
if ( ! to )
    return  "";

                                        // empty separator?
if ( StringIsEmpty  ( separator ) )
    return  to;

                                        // don't append to an empty string, separators need something before, like "AAA" + ", ", we don't want to return ", "
if ( StringIsEmpty  ( to ) )
    return  to;

                                        // just in case, don't duplicate any trailing separator, in case function was called multiple times
if ( StringEndsWith ( to, separator ) )
    return  to;

                                        // OK, it seems we are good here
StringAppend        ( to, separator );


return  to;
}

                                        // tobereplaced MUST contain something, it will NOT replace empty strings with something else
void    StringReplace ( char* s, const char* tobereplaced, const char* replacedwith )
{
if ( !s || !tobereplaced || !replacedwith )
    return;

if ( !*s || !*tobereplaced )
    return;

                                        // difference of length in the 2 strings
                                        // user should provide sufficient space in the strings, if the second one is larger!
long                lentoreplace    = StringLength ( tobereplaced );
long                lenreplacewith  = StringLength ( replacedwith );
long                delta           = lenreplacewith - lentoreplace;
char*               toc             = s;

do {
    toc     = StringContains ( toc, const_cast<char*> ( tobereplaced ) );

    if ( !toc )                         // no more occurence?
        break;

                                        // go to end of tobereplaced
    toc    += lentoreplace;

    if ( delta != 0 )                   // something to move? (including trailing \0)
        MoveVirtualMemory ( toc + delta, toc, StringSize ( toc ) );

                                        // go to beginning of tobereplaced
    toc    -= lentoreplace;

                                        // poke in the new string
    for ( int i = 0; i < lenreplacewith; i++, toc++ )
        *toc = replacedwith[ i ];

    } while ( *toc );
}


void    StringDelete     ( char* s, const char* tobedeleted )
{
StringReplace   ( s, tobedeleted, "" );
}


void    StringRepeat ( char* s, char torepeat, UINT num )
{
if ( !s || !torepeat )
    return;

ClearString ( s );

if ( num <= 0 )
    return;


for ( UINT i = 0; i < num; i++ )
    s[ i ]  = torepeat;

s[ num ]    = EOS;
}


//----------------------------------------------------------------------------
                                        // remove beginning and trailing spaces
void    StringCleanup ( char* s )
{
if ( !s || !*s )
    return;

                                        // costly op - done only once - here we have length > 0
long                length          = StringLength ( s );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove trailing spaces
for ( char* toc = s + length - 1; length > 0 && IsSpaceNewline ( *toc ); length--, toc-- )
    *toc = EOS;

                                        // result is empty (string was all spaces)?
if ( length == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove beginning spaces
long                firstnonspacei;

for ( firstnonspacei = 0; firstnonspacei < length; firstnonspacei++ )
    if ( ! IsSpaceNewline ( s[ firstnonspacei ] ) )
        break;

                                        // no beginning spaces
if ( firstnonspacei == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this will also move the trailing \0
MoveVirtualMemory ( s, s + firstnonspacei, length - firstnonspacei + 1 );
}


//----------------------------------------------------------------------------

void    StringNoSpace    ( char* s )
{
ReplaceChars ( s, " \t\n", "" );
}


void    ReplaceChars ( char* s, const char* tobereplaced, const char* replacedwith )
{
if ( !s || !tobereplaced || !replacedwith )
    return;

if ( !*s || !*tobereplaced )
    return;


int                 inc;

for ( const char* tosearch = tobereplaced, *toreplace = replacedwith; *tosearch; tosearch++, toreplace += *toreplace ? 1 : 0 )

    for ( char* tos = s; *tos; tos += inc ) {

        inc     = 1;

        if ( *tos == *tosearch )
            if ( *toreplace )
                *tos = *toreplace;
            else {
                MoveVirtualMemory ( tos, tos + 1, StringSize ( tos + 1 ) );
                inc     = 0;
                }
        }
}


void    KeepChars ( char* s, const char* tobekept )
{
if ( !s || !tobekept )
    return;

if ( !*s || !*tobekept ) {
    ClearString ( s );
    return;
    }


for ( int i = 0; i < StringLength ( s ); i++ )
    if ( ! StringContains ( tobekept, s[ i ] ) )
        s[ i ]  = ' ';
}


char*   JumpToChars ( char* s, char* reachchars )
{
if ( !s || !reachchars )
    return  0;

if ( !*s || !*reachchars )
    return  0;

          // if failed: length of string -> *s == 0 -> returns 0
return  s + strcspn ( s, reachchars );
}


void    ClipToChars ( char* s, char* reachchars )
{
if ( !s || !reachchars )
    return;

if ( !*s || !*reachchars )
    return;


char*               toc             = JumpToChars ( s, reachchars );

if ( toc )
    *toc    = EOS;
}


int     CountChar   ( const char* s, char searched )
{
if ( StringIsEmpty ( s ) || !searched )
    return  0;


int                 count           = 0;

for ( int i = 0; i < StringLength ( s ); i++ )
    if ( s[ i ] == searched )
        count++;

return  count;
}


int     StringNumLines ( const char* s )
{
if ( StringIsEmpty ( s ) )
    return  0;

return  CountChar ( s, '\n' ) + 1;
}


char*   StringToUppercase ( char* s )
{
if ( StringIsEmpty ( s ) )
    return  "";

return  _strupr ( s );
}


char*   StringToLowercase ( char* s )
{
if ( StringIsEmpty ( s ) )
    return  "";

return  _strlwr ( s );
}


void    StringCamelCase ( char* string )
{
if ( !string || !*string )
    return;


bool        begin   = true;

for ( int i = 0; i < StringLength ( string ); i++ ) {

    if ( IsSpaceNewline ( string[ i ] ) ) {
        begin   = true;
        continue;
        }

    if ( begin )
        string[ i ] = (char) toupper ( string[ i ] );
    else
        string[ i ] = (char) tolower ( string[ i ] );

    begin   = false;
    }
}


//----------------------------------------------------------------------------
int     StringToInteger ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  0;

return  atoi ( string );
}


long    StringToLong ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  0;

return  atol ( string );
}


double  StringToDouble ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  0;

return  atof ( string );
}


float   StringToFloat ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  0;

return  atof ( string );
}


//----------------------------------------------------------------------------
char*   IntegerToString ( char* s, int i, int width )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing, caller still might need some string

                                        // optional width, padded with '0'
sprintf ( s, "%0*d", width, i );

return  s;
}


TFileName   IntegerToString ( int i, int width )
{
TFileName           s;

IntegerToString ( s, i, width );

return  s;
}


//----------------------------------------------------------------------------
char*   FloatToString ( char* s, double f, int width, int precision )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing

                                        // FOLLOW the formatting, including width and precision of 0
sprintf ( s, "%*.*lf", width, precision, RoundTo ( f, Power10 ( -precision ) ) );

                                        // smart version - maybe a bit too unpredictable when using default values of 0?
//if      ( width == 0 && precision == 0 )    sprintf ( s, "%lg", f );
//else if ( width != 0 && precision == 0 )    sprintf ( s, "%*lg", width, f );
//else if ( width == 0 && precision != 0 )    sprintf ( s, "%.*lf", precision, RoundTo ( f, Power10 ( -precision ) ) );
//else if ( width != 0 && precision != 0 )    sprintf ( s, "%*.*lf", width, precision, RoundTo ( f, Power10 ( -precision ) ) );

return  s;
}


char*   FloatToString ( char* s, double f, int precision )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing

                                        // after point digit
                                        // !enforce the rounding ourselves, not relying on sprintf to either truncate or round!
sprintf ( s, "%.*lf", precision, RoundTo ( f, Power10 ( -precision ) ) );

return  s;
}


char*   FloatToString ( char* s, double f )
{
if ( ! s )                              // null pointer
    return  "";                         // to avoid crashing

sprintf ( s, "%lg", f );                // use general formatting, which optimally adapts to the data

return  s;
}


TFileName   FloatToString ( double f, int width, int precision )
{
TFileName           s;

FloatToString ( s, f, width, precision );

return  s;
}


TFileName   FloatToString ( double f, int precision )
{
TFileName           s;

FloatToString ( s, f, precision );

return  s;
}


TFileName   FloatToString ( double f )
{
TFileName           s;

FloatToString ( s, f );

return  s;
}


//----------------------------------------------------------------------------
bool    IsInteger ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  false;

                                        // either successful, or string "0", otherwise false
return  StringToInteger ( string ) != 0 || StringIs ( string, "0" );
}


bool    IsFloat ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  false;

                                        // either successful, or string "0", otherwise false
return  StringToDouble ( string ) != 0 || StringContains ( string, "0" );
}

                                        // scan the string for the first recognized value
double  GetFirstNumber ( const char* string )
{
if ( StringIsEmpty ( string ) )
    return  0;


TSplitStrings       splits ( string, UniqueStrings );
double              scandouble;


for ( int i = 0; i < (int) splits; i++ ) {

//    DBGM ( splits[ i ], string );

    if ( sscanf ( splits[ i ], "%lf",    &scandouble ) == 1 )   return  scandouble;
    if ( sscanf ( splits[ i ], "%lf%%",  &scandouble ) == 1 )   return  scandouble;
    if ( sscanf ( splits[ i ], "%lf%*s", &scandouble ) == 1 )   return  scandouble;
    }


return  0;
}


//----------------------------------------------------------------------------
                                        // Returns a floating point value, so we can have fractions of TF
                                        // !string parameter is modified!

                                        //      Duration specifiers:
                                        // time frames:     "Time Frame" "Time Frames" "TimeFrame" "TimeFrames" "TF" or nothing
                                        // hours:           h hour hours
                                        // minutes:         m min minute minutes ' ´
                                        // seconds:         s sec second seconds '' ´´ "
                                        // milliseconds:    ms msec millisec millisecond milliseconds

                                        //      Syntax and example for 500Hz:
                                        // 123          -> 123 TF
                                        // 123 TF       -> 123 TF
                                        // 10  [ms]     -> 5 TF
                                        // 1   [s]      -> 500 TF
                                        // 1.5 [s]      -> 750 TF
                                        // 1   [m]      -> 30000 TF
                                        // 1.5 [m]      -> 45000 TF
                                        // 1   [h]      -> 1800000 TF
                                        // 0.1 [h]      -> 180000 TF
                                        // XX:          -> XX minutes
                                        // XX:YY        -> XX minutes YY seconds
                                        // XXsYY        -> XX seconds YY milliseconds
                                        // XXmYY        -> XX minutes YY seconds
                                        // XXhYY        -> XX hours YY minutes
                                        // XX:YY:       -> XX minutes YY seconds
                                        // XXsYYms      -> XX seconds YY milliseconds
                                        // XXmYYs       -> XX minutes YY seconds
                                        // XXmYYsZZms   -> XX minutes YY seconds ZZ milliseconds
                                        // XXhYYm       -> XX hours YY minutes
                                        // XX:YY:ZZ     -> XX hours YY minutes ZZ seconds (note that XX, YY or ZZ can be floats, like 0.1:00:00!)
                                        // XXhYYmZZ     -> XX hours YY minutes ZZ seconds
                                        // XXhYYmZZs    -> XX hours YY minutes ZZ seconds
                                        // XXhYYmZZsMM  -> XX hours YY minutes ZZ seconds MM milliseconds
                                        // XXhYYmZZsMMms-> XX hours YY minutes ZZ seconds MM milliseconds

double  StringToTimeFrame ( char* string, double samplingfrequency, bool* answerintf )
{
if ( answerintf )   *answerintf     = true;

if ( StringIsEmpty ( string ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Re-code all recognized time strings to simplified ones
                                        // !do the longest strings first!

StringReplace ( string, "TIME FRAMES",      "TF"    );
StringReplace ( string, "TIME FRAME",       "TF"    );
StringReplace ( string, "TIMEFRAMES",       "TF"    );
StringReplace ( string, "TIMEFRAME",        "TF"    );

StringReplace ( string, "MILLISECONDS",     "L"     );  // not "S" nor "M"
StringReplace ( string, "MILLISECOND",      "L"     );
StringReplace ( string, "MILLISEC",         "L"     );
StringReplace ( string, "MSEC",             "L"     );
StringReplace ( string, "MS",               "L"     );

StringReplace ( string, "SECONDS",          "S"     );
StringReplace ( string, "SECOND",           "S"     );
StringReplace ( string, "SEC",              "S"     );
StringReplace ( string, "''",               "S"     );
StringReplace ( string, "´´",               "S"     );
StringReplace ( string, "\"",               "S"     );

StringReplace ( string, "MINUTES",          "M"     );
StringReplace ( string, "MINUTE",           "M"     );
StringReplace ( string, "MIN",              "M"     );
StringReplace ( string, "'",                "M"     );
StringReplace ( string, "´",                "M"     );

StringReplace ( string, "HOURS",            "H"     );
StringReplace ( string, "HOUR",             "H"     );
StringReplace ( string, "HR",               "H"     );

                                        // These are the recognized local strings: TF L S M H :
                                        // add some space separators for TSplitStrings to do a proper job
StringReplace ( string, "TF",               " TF "  );
StringReplace ( string, "L",                " L "   );
StringReplace ( string, "S",                " S "   );
StringReplace ( string, "M",                " M "   );
StringReplace ( string, "H",                " H "   );
StringReplace ( string, ":",                " : "   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSplitStrings       split ( string, NonUniqueStrings, " \t\n" );
double              tf              = 0;

if ( answerintf )   *answerintf     = false;

                                        // We test, for each number of split elements, all possible time specifiers
                                        // What is currently not tested is between the time specifiers, that there are actual numbers

//split.Show ( "time string" );

if      ( (int) split == 0 ) {

    if ( answerintf )   *answerintf     = true;

    tf      = 0;
    }

                                        // either no frequency available, or 1 string -> always time frame
else if ( samplingfrequency == 0
       || (int) split == 1 ) {

    if ( answerintf )   *answerintf     = true;

    tf      = StringToDouble ( split[ 0 ] );

    } // 1 item
                                        // Here, samplingfrequency is > 0

                                        // "XX TF" "HH hour" "MM min" "SS sec" "LL ms"
else if ( (int) split == 2 ) {

    if ( StringIs ( split[ 1 ], "TF" ) ) {

        if ( answerintf )   *answerintf     = true;
    
        tf      = StringToDouble             ( split[ 0 ] );
        }

    else if ( StringIs ( split[ 1 ], "L" ) )

        tf      = MillisecondsToTimeFrame   ( StringToDouble ( split[ 0 ] ), samplingfrequency );
       
    else if ( StringIs ( split[ 1 ], "S" ) )

        tf      = SecondsToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency );
       
    else if ( IsStringAmong ( split[ 1 ], ": M" ) )

        tf      = MinutesToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency );
       
    else if ( StringIs ( split[ 1 ], "H" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency );
       
    } // 2 items


                                        // MM:SS" or "MM min SS" or "HH hour MM" or "SS sec LL"
else if ( (int) split == 3 ) {

    if ( IsStringAmong ( split[ 1 ], ": M" ) )

        tf      = MinutesToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    else if ( StringIs ( split[ 1 ], "H" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    else if ( StringIs ( split[ 1 ], "S" ) )

        tf      = SecondsToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MillisecondsToTimeFrame   ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    } // 3 items


                                        // "HH:MM:" (not clean) or "MM min SS sec" or "HH hour MM min" or "SS sec LL ms"
else if ( (int) split == 4 ) {

    if      ( IsStringAmong ( split[ 1 ], ": H" )
           || IsStringAmong ( split[ 3 ], ": M" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    else if ( StringIs ( split[ 1 ], "M" )
           || StringIs ( split[ 3 ], "S" ) )

        tf      = MinutesToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    else if ( StringIs ( split[ 1 ], "S" )
           || StringIs ( split[ 3 ], "L" ) )

        tf      = SecondsToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MillisecondsToTimeFrame   ( StringToDouble ( split[ 2 ] ), samplingfrequency );

    } // 4 items


                                        // "HH:MM:SS" or "HH hour MM min SS"
else if ( (int) split == 5 ) {

    if ( IsStringAmong ( split[ 1 ], ": H" )
      && IsStringAmong ( split[ 3 ], ": M" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 4 ] ), samplingfrequency );
       
    } // 5 items


                                        // "HH hour MM min SS sec" or "MM min SS sec LL ms"
else if ( (int) split == 6 ) {

    if      ( StringIs ( split[ 1 ], "H" )
           && StringIs ( split[ 3 ], "M" )
           && StringIs ( split[ 5 ], "S" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 4 ] ), samplingfrequency );

    else if ( StringIs ( split[ 1 ], "M" )
           && StringIs ( split[ 3 ], "S" )
           && StringIs ( split[ 5 ], "L" ) )

        tf      = MinutesToTimeFrame        ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency )
                + MillisecondsToTimeFrame   ( StringToDouble ( split[ 4 ] ), samplingfrequency );
       
    } // 6 items

                                        // "HH hour MM min SS sec LL"
else if ( (int) split == 7 ) {

    if      ( StringIs ( split[ 1 ], "H" )
           && StringIs ( split[ 3 ], "M" )
           && StringIs ( split[ 5 ], "S" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 4 ] ), samplingfrequency )
                + MillisecondsToTimeFrame   ( StringToDouble ( split[ 6 ] ), samplingfrequency );
    } // 7 items

                                        // "HH hour MM min SS sec LL ms"
else if ( (int) split == 8 ) {

    if      ( StringIs ( split[ 1 ], "H" )
           && StringIs ( split[ 3 ], "M" )
           && StringIs ( split[ 5 ], "S" )
           && StringIs ( split[ 7 ], "L" ) )

        tf      = HoursToTimeFrame          ( StringToDouble ( split[ 0 ] ), samplingfrequency )
                + MinutesToTimeFrame        ( StringToDouble ( split[ 2 ] ), samplingfrequency )
                + SecondsToTimeFrame        ( StringToDouble ( split[ 4 ] ), samplingfrequency )
                + MillisecondsToTimeFrame   ( StringToDouble ( split[ 6 ] ), samplingfrequency );
    } // 8 items

                                        // more than 8 items is not recognized

return  tf;
}


//----------------------------------------------------------------------------
void    RemoveLastSyllable ( char* s )
{
if ( StringIsEmpty ( s ) )
    return;


long                i               = StringLength ( s ) - 1;

                                        // first, clean-up trailing non-chars
if ( ! isalnum ( s[ i ] ) ) {

    for ( ; i >= 0; i-- )

        if ( ! isalnum ( s[ i ] ) )
            s[ i ] = EOS;
        else
            break;

    return;                             // that may already be enough
    }

                                        // search for last chars component
for ( ; i >= 0; i-- )
    if ( ! isalnum ( s[ i ] ) )
        break;

                                        // this is the last component
char*               last            = s + i + 1;
long                lasti           = StringLength ( last ) - 1;
                    i               = StringLength ( s ) - 1;


if ( lasti <= 1 ) {                     // cases: o p oo pp op po
    last[ 0 ]  = EOS;                  // remove the last 1 or 2 chars
    }
                                        // treat last series of digits as a syllable
else if ( isdigit ( last[ lasti ] ) ) {

    for ( ; lasti >= 0; lasti-- )
        if ( isdigit ( last[ lasti ] ) )
            last[ lasti ] = EOS;
        else
            break;
    }

else {
                                        // first remove last consonant(s)
    for ( ; i >= 0; i-- )
        if ( isconsonant ( s[ i ] ) )
            s[ i ] = EOS;
        else
            break;

                                        // then remove preceding vowel(s)
    for ( ; i >= 0; i-- )
        if ( isvowel ( s[ i ] ) )
            s[ i ] = EOS;
        else
            break;
    }

                                        // last step, clean-up trailing non-chars
for ( i = StringLength ( s ) - 1; i >= 0; i-- )
    if ( ! isalnum ( s[ i ] ) )
        s[ i ] = EOS;
    else
        break;
}

                                        // maxlen is the maximum length of chars, so add 1 char for EOS
char*   StringShrink ( const char* name, char* newname, int maxlen )
{
if ( StringIsEmpty ( name ) || ! newname )
    return  0;


                                        // nothing to do?
if ( StringLength ( name ) <= maxlen ) {
    StringCopy ( newname, name );
                                        // make it consistent with other processings
//  StringCamelCase ( newname );
    StringNoSpace ( newname );

//  DBGM2 ( name, newname, "Copied" );
    return  newname;
    }

                                        // We have to do some processing...

                                        // split string into tokens
TSplitStrings       splits ( name, /*UniqueStrings*/ NonUniqueStrings, " _\t\n,;:" );

                                        // make them nice
for ( int i = 0; i < (int) splits; i++ ) {
    StringCamelCase ( splits[ i ] );

    ReplaceChars ( splits[ i ], "()[]{}", "" );
//  DBGM ( splits[ i ], "Split" );
    }

splits.CompactSpaces ();

                                        // first choice: simply compacting by removing spaces
splits.ToString ( newname, false );

                                        // is it enough?
if ( StringLength ( newname ) <= maxlen ) {
//  DBGM2 ( name, newname, "Compacted" );
    return  newname;
    }
                                        // second choice:
                                        // iteratively decimate the longest token
                                        // removing the last syllable
long                imax;
long                lenmax;

do {
    imax    = splits.GetBiggestTokenIndex ();

    if ( imax < 0 )
        break;

    lenmax  = StringLength ( splits[ imax ] );

    if ( lenmax == 0 )
        break;


    RemoveLastSyllable ( splits[ imax ] );

                                        // do some clean-up
    splits.CompactSpaces ();

                                        // compact results
    splits.ToString ( newname, false );
//  DBGM ( newname, "intermediate result" );


    if ( StringLength ( newname ) <= maxlen ) {
                                        // clean-up possible mess after the EOS
        ClearVirtualMemory ( StringEnd ( newname ), maxlen - StringLength ( newname ) );

//      DBGM2 ( name, newname, "Decimated" );
        return  newname;                // we're done!
        }

    } while ( (int) splits );           // loop till there is nothing left


                                        // our last hope, let's truncate the original
StringCopy ( newname, name, maxlen );
//DBGM2 ( name, newname, "Truncated" );

return  newname;
}


//----------------------------------------------------------------------------
char*   StringRandom ( char* s, int n )
{
if ( !s || n <= 0 )
    return  0;

/*
TRandUniform        randunif;
#define             NumLegalChars       2 * 26
char                legalchars[ NumLegalChars + 1 ]     = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


for ( int i = 0; i < n; i++ )
    s[ i ]  = legalchars[ randunif ( (uint) NumLegalChars ) ];

s[ n ]  = EOS;
*/
                                        // Random syllables
TRandUniform        randunif;

char                consonants[]    = "BCDFGHJKLMNPQRSTVWXZbcdfghjklmnpqrstvwxz";
char                vowels    []    = "AEIOUYaeiouy";
int                 numconsonants   = sizeof ( consonants ) / sizeof ( char ) - 1;
int                 numvowels       = sizeof ( vowels     ) / sizeof ( char ) - 1;


for ( int i = 0; i < n; i++ )

    s[ i ]  = IsEven ( i )  ?   consonants[ randunif ( (UINT) numconsonants ) ] 
                            :   vowels    [ randunif ( (UINT) numvowels     ) ];

s[ n ]  = EOS;

return  s;
}


//----------------------------------------------------------------------------
char*   GetLastWord ( char* s, char* w )
{
if ( !s || !w )
    return  0;

ClearString ( w );

if ( !*s )
    return  w;

                                        // start from the end of string
long                i               = StringLength ( s ) - 1;

                                        // 1) skip trailing spaces
for ( ; i >= 0; i-- )
    if ( ! IsSpaceNewline ( s[ i ] ) )
        break;

                                        // no more non-space chars?
if ( i == -1 )
    return  w;

                                        // here we have the end of a non-spacey word
char*               wordend         = s + i;
char*               wordbegin       = wordend;

                                        // 2) rewind to the beginning
for ( ; i >= 0; i-- )
    if ( IsSpaceNewline ( s[ i ] ) )
        break;
    else
        wordbegin   = s + i;


StringCopy ( w, wordbegin, wordend - wordbegin + 1 );


return  w;
}

                                        // returned in a copy string
char*   SkipFirstWords ( char* s, char* sw, int numwords )
{
if ( !s || !sw )
    return  0;

StringCopy ( sw, s );

if ( !*s || numwords <= 0 )
    return  sw;


char*               toc;

                                        // first, skip beginning spaces
for ( toc = s; StringIsNotEmpty ( toc ); toc++ )
    if ( ! IsSpaceNewline ( *toc ) )
        break;

                                        // repeat for the # of words...
for ( int iw = 0; iw < numwords && StringIsNotEmpty ( toc ); iw++ ) {
                                        // skip word (non-spaces)
    for ( ; StringIsNotEmpty ( toc ); toc++ )
        if ( IsSpaceNewline ( *toc ) )
            break;
                                        // skip spaces
    for ( ; StringIsNotEmpty ( toc ); toc++ )
        if ( ! IsSpaceNewline ( *toc ) )
            break;
    } // for word

                                        // copy result
StringCopy ( sw, toc );

return  sw;
}


//----------------------------------------------------------------------------
void    GetTimeStamp ( bool date, bool time, char* text )
{
SYSTEMTIME          thedate;

GetLocalTime    ( &thedate );

ClearString ( text );


if ( date )
    sprintf ( text, "%0d/%02d/%02d", thedate.wYear, thedate.wMonth, thedate.wDay );


if ( time ) {
    if ( StringIsNotEmpty ( text ) )
        StringAppend ( text, "  " );

    sprintf ( StringEnd ( text ), "%02d:%02d", thedate.wHour, thedate.wMinute );
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
