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
                                        // Strings utilities

                                        // Define standard fixed strings for text conversion
template <long N>   class   TFixedString;
using               TFixedString32      = TFixedString<32>;


enum                StringFlags
                    {
                    CaseSensitive           = 0x01,
                    CaseInsensitive         = 0x02,

                    StringContainsForward   = 0x10,
                    StringContainsBackward  = 0x20,

                    StringContainsDefault   = CaseInsensitive | StringContainsForward,
                    };


//----------------------------------------------------------------------------
                                        // A few predefined convenient strings / chars
                                        // Kept as define so we can mix them in onther strings without fuss
#define             Tab                     "\t"
#define             DoubleQuote             "\""
#define             NewLine                 "\n"
#define             fastendl                "\n"
#define             EOS                     '\0'


int                 StringCompare    ( const char* s1, const char* s2, StringFlags flags = CaseInsensitive );
bool                StringIs         ( const char* s1, const char* s2, StringFlags flags = CaseInsensitive );
bool                StringIsNot      ( const char* s1, const char* s2, StringFlags flags = CaseInsensitive );
bool                StringIsEmpty    ( const char* s );
bool                StringIsNotEmpty ( const char* s );
bool                StringIsSpace    ( const char* s );
bool                StringStartsWith ( const char* searched, const char* tosearch, int maxlength = -1 );
const char*         StringContains   ( const char* searched, const char* tosearch, StringFlags flags = StringContainsDefault ); // CaseSensitive / CaseInsensitive not implemented
char*               StringContains   ( char*       searched, char*       tosearch, StringFlags flags = StringContainsDefault ); // StringContainsBackward not implemented
const char*         StringContains   ( const char* searched, const char  tosearch, StringFlags flags = StringContainsDefault ); // CaseSensitive / CaseInsensitive not implemented
char*               StringContains   ( char*       searched, char        tosearch, StringFlags flags = StringContainsDefault );
bool                StringEndsWith   ( const char* searched, const char* tosearch );
bool                IsStringAmong    ( const char* search,   const char* among );


bool                isconsonant      ( char c );
bool                isvowel          ( char c );
bool                IsSpace             ( char c );
bool                IsSpaceNewline      ( char c );
bool                IsSpaceNewlineComma ( char c );
bool                IsSpaceComma        ( char c );

char*               LastChar         ( char*       s, long countingfromend   = 1 ); // count from 1 for last char, 2 for previous to last, etc...
const char*         LastChar         ( const char* s, long countingfromend   = 1 ); // count from 1 for last char, 2 for previous to last, etc...
void                ReplaceChars     ( char* s, const char* tobereplaced, const char* replacedwith );
void                DeleteChars      ( char* s, const char* tobedeleted );
void                KeepChars        ( char* s, const char* tobekept );
char*               JumpToChars      ( char* s, char* reachchars );             // returns 0 if error
void                ClipToChars      ( char* s, char* reachchars );             // clip string to first char among the list
int                 CountChar        ( const char* s, char searched );
bool                IsCharAmong      ( const char search, const char* among );  // case sensitive


void                ClearString      ( char* s, long size = 0 );              // by default, just the first char, otherwise, also reset content
void                ClearString      ( UINT8 *s, long size = 0 );             // by default, just the first char, otherwise, also reset content
void                SetString        ( char* s, char fill, long size );
long                StringLength     ( const char*  s );
long                StringLength     ( const UCHAR *s );
long                StringSize       ( const char* s );
char*               StringEnd        ( char* s );
const char*         StringEnd        ( const char* s );
char*               StringClip       ( char* s, long length );                  // clip by length
char*               StringClip       ( char* s, long from, long to );           // clip by range of indexes (starting from 0)
char*               StringCopy       ( char* to, const char* from );
char*               StringCopy       ( char* to, const char* from, long totallength );  // totallength is the final max length
char*               StringCopy       ( char* to, const char* from, const char* tail1, const char* tail2 = 0, const char* tail3 = 0, const char* tail4 = 0, const char* tail5 = 0, const char* tail6 = 0, const char* tail7 = 0, const char* tail8 = 0 );   // at least 1 tail, otherwise use StringCopy above
char*               StringAppend     ( char* to,                   const char* tail1, const char* tail2 = 0, const char* tail3 = 0, const char* tail4 = 0, const char* tail5 = 0, const char* tail6 = 0, const char* tail7 = 0, const char* tail8 = 0 );
char*               StringAppend     ( char* to, const char* from, long totallength );  // totallength is the final max length, AFTER concatenation
char*               StringAppend     ( char* to, long   tail );
char*               StringAppend     ( char* to, double tail );
char*               StringPrepend    ( char* to, const char* head );
void                StringReplace    ( char* s, const char* tobereplaced, const char* replacedwith );
void                StringDelete     ( char* s, const char* tobedeleted );
void                StringCleanup    ( char* s );
void                StringNoSpace    ( char* s );
void                StringCamelCase  ( char* string );
void                StringRepeat     ( char* s, char torepeat, UINT num );      // repeat a single char num times
int                 StringNumLines   ( const char* s );
char*               AppendSeparator  ( char* to, const char* separator );       // smartly add separators at the end of string: "AAA" -> "AAA, "; but "" -> ""


int                 StringToInteger  ( const char* string );
long                StringToLong     ( const char* string );
float               StringToFloat    ( const char* string );
double              StringToDouble   ( const char* string );
char*               IntegerToString  ( char* s, int i, int width = 0 );
TFixedString32      IntegerToString  (          int i, int width = 0 );
char*               TimeFrameToString( char* s, int tf );
TFixedString32      TimeFrameToString(          int tf );
char*               FloatToString    ( char* s, double f, int width, int precision );   // width AND size parameter
TFixedString32      FloatToString    (          double f, int width, int precision );
char*               FloatToString    ( char* s, double f, int precision );              // precision: 7 for float, 16 for double
TFixedString32      FloatToString    (          double f, int precision );
char*               FloatToString    ( char* s, double f );                             // automatic formatting, when one does not care about size formatting
TFixedString32      FloatToString    (          double f );
double              GetFirstNumber   ( const char* string );
bool                IsInteger        ( const char* string );
bool                IsFloat          ( const char* string );
double              StringToTimeFrame( const char* string, double samplingfrequency, bool* answerintf = 0 );

char*               StringToUppercase   ( char* s );
char*               StringToLowercase   ( char* s );

void                RemoveLastSyllable ( char* s );
char*               StringShrink     ( const char* name, char* newname, int maxlen ); // name and newname can be the same variable
char*               StringRandom     ( char* s, int n );

char*               GetLastWord      ( char* s, char* w );
char*               SkipFirstWords   ( char* s, char* sw, int numwords );

void                GetTimeStamp     ( bool date, bool time, char* text );


                                        // locally repeating this typedef until we have a cleaner solution
using   TCheckBoxData       = UINT16;
                                        // to simplify dialog's input / output

bool                StringToTrue    ( const char* s );
bool                StringToFalse   ( const char* s );
bool                StringToBool    ( char* s );
TCheckBoxData       StringToCheck   ( char* s );
const char*         BoolToString    ( bool b );
//char*             BoolToString    ( char* s, bool b );
//TFixedString32    BoolToString    (          bool b );

const char*         SignToString    ( double v );

bool                CheckToBool     ( TCheckBoxData c );
TCheckBoxData       BoolToCheck     ( bool b );

char*               StringPlural    ( bool isplural,   bool space = false );
char*               StringPlural    ( int  v,          bool space = false );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
