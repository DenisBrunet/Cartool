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

#include    "Strings.Grep.h"
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.Utils.h"
#include    "TList.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TStringGrep::TStringGrep ()
{
RegExpCompiled  = 0;

Reset ();
}


        TStringGrep::TStringGrep ( const char* regexp, GrepOption options )
{
RegExpCompiled  = 0;

Reset ();

Set ( regexp, options );
}


        TStringGrep::TStringGrep ( const char* searchregexp, const char* replaceregexp, GrepOption options )
{
RegExpCompiled  = 0;

Reset ();

Set ( searchregexp, replaceregexp, options );
}


        TStringGrep::~TStringGrep ()
{
Reset ();
}


void    TStringGrep::Reset ()
{
Options     = GrepOptionNone;
                                        // reset searched pattern
if ( RegExpCompiled ) {

    ::pcre_free ( RegExpCompiled );

    RegExpCompiled  = 0;
    }

ClearString ( ReplaceRegexp, PCRE_REPLACEDLENGTH );

ResetResults ();
}


void    TStringGrep::ResetResults ()
{
NumMatches      = 0;

for ( int i = 0; i < PCRE_MAXRETURNMATCHES; i++ )
    ToMatch[ i ]    = EOS;
}


//----------------------------------------------------------------------------
                                        // Transform a Shell simpler wild-chars syntax into its Perl regexp equivalent
void    TStringGrep::ShellToPerl ( char* regexp )   const
{
if ( StringIsEmpty ( regexp ) )
    return;


StringReplace ( regexp, ".", "\\."  );  // "file.ext" becomes "files\.ext"
StringReplace ( regexp, "*", ".*"   );  // !will also replace "\*", which is not nice, but shouldn't be given as input!
StringReplace ( regexp, "?", "."    );  // !will also replace "\?", which is not nice, but shouldn't be given as input!

StringPrepend ( regexp, "^" );
StringAppend  ( regexp, "$" );
}


//----------------------------------------------------------------------------
bool    TStringGrep::Set ( const char* regexp, GrepOption options )
{
Reset ();

if ( StringIsEmpty ( regexp ) )
    return  false;

Options     = options;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // make a local copy of regexp, maybe modified according to syntax

                                        // Shell syntax is simpler and aims at files
if ( IsFlag ( options, GrepOptionSyntaxShell ) ) {
                                        // allocate a big enough temp regexp
    TArray1<char>       convregexp ( 2 * StringSize ( regexp ) + 2 );

    StringCopy      ( convregexp, regexp );

    ShellToPerl     ( convregexp );

                                                // switch to Perl, while also forcing case insensitive to behave more like MS-DOS
    ResetFlags      ( options, GrepOptionSyntaxShell );

    SetFlags        ( options, (GrepOption) ( GrepOptionSyntaxPerl | GrepOption_PCRE_CASELESS ) );


    return  Set ( convregexp, options );
    } // if GrepOptionSyntaxShell


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // if GrepOptionSyntaxPerl, or nothing specified (it shouldn't, but let it land nicely to Perl)

                                        // some of the options
//  PCRE_ANCHORED       -- Like adding ^ at start of pattern.
//  PCRE_CASELESS       -- Like m//i
//  PCRE_DOLLAR_ENDONLY -- Make $ match end of string regardless of \n's
//                         No Perl equivalent.
//  PCRE_DOTALL         -- Makes . match newlines too.  Like m//s
//  PCRE_EXTENDED       -- Like m//x
//  PCRE_EXTRA          --
//  PCRE_MULTILINE      -- Like m//m
//  PCRE_UNGREEDY       -- Set quantifiers to be ungreedy.  Individual quantifiers
//                         may be set to be greedy if they are followed by "?".
//  PCRE_UTF8           -- Work with UTF8 strings.


    const char*         pcreErrorStr;
    int                 pcreErrorOffset;


    KeepFlags   ( options, GrepOption_PCRE_MASK );

                                        // compile regexp - make sure to clear-up the options from irrelevant flags
    RegExpCompiled  = ::pcre_compile ( regexp, options, &pcreErrorStr, &pcreErrorOffset, 0 );


     if ( RegExpCompiled == 0 ) {
    //    DBGM2 ( regexp, pcreErrorStr, "pcre Compiling Error" );
        Reset ();
        return  false;
        }

/*                                      // try optimizing the compiled regexp - not used as it is not very clear if it fails optimizating or it there is an error
    ::pcre_extra*       pcreExtra;

    pcreExtra       = ::pcre_study ( RegExpCompiled, 0, &pcreErrorStr );

                                        // error or no optimization possible?
    if ( pcreErrorStr != 0 ) {
    //    DBGM ( "Optimization failed", "pcre Compiling Error" );
        Reset ();
        return  false;
        }

    // don't forget  ::pcre_free ( pcreExtra )  somewhere
*/

    return  true;
    } // if GrepOptionSyntaxPerl

}


//----------------------------------------------------------------------------
                                        // The searched and replaced string can use up to 9 capture groups
                                        // search should have syntax like "My (\\w*)" to capture the next word
                                        // replaced the can use group 1   "Is \\1"
                                        // We allow group 0 \\0 as the whole match
bool    TStringGrep::Set ( const char* searchregexp, const char* replaceregexp, GrepOption options )
{
Set ( searchregexp, options );

StringCopy  ( ReplaceRegexp, replaceregexp, PCRE_REPLACEDLENGTH - 1 );

return  IsValid ();
}


//----------------------------------------------------------------------------
                                        // Optionally returning all matches
                                        // matches ( 0 ) always contain the whole match
                                        // matches ( 1 ) is first group, matches ( 2 ) is second group, etc.. if syntax asked for groups
                                        // it seems it also return an additional group with the trail from the last group
int     TStringGrep::Matched    (   const char*     s,  
                                    TStrings*       matches
                                )
{
ResetResults ();


if ( matches )
    matches->Reset ();

                                        // not empty regexp, but failed to compile?
if ( ! IsValid () )
    return  0;

                                        // empty regexp is always true
//if ( StringIsEmpty ( regexp ) )
//    return  true;

                                        // get to last part, if requested AND if it exists
const char*         tofilename      = IsFlag ( Options, GrepOptionFileNameOnly ) ? ToFileName ( s ) : 0;
                                        // searching file name if it exists, or original string otherwise
const char*         tosearch        = StringIsEmpty ( tofilename ) ? s : tofilename;

                                        // non-empty regexp and empty input? let the function sort it out, the expression might allow it!
//if ( StringIsEmpty ( tosearch ) )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // should be a multiple of 3, !the last 1/3 part is used internally and will NOT return matches!
int                 pcreResult;


pcreResult      = ::pcre_exec ( RegExpCompiled,         // the compiled pattern
                                0,                      // extra data, if pattern was studied
                                tosearch,
                                StringLength ( tosearch ),
                                0,                      // offset in search string, can be used to repeat a search within the same string
                                0 /*pcreOptions*/,      // default options
                                pcreOutputVector,       // output vector for substring information
                                PCRE_MAXOVECTOR         // number of elements in the output vector
                            );


if ( pcreResult < 0 ) {
/*                                      // no match or errors?
    switch ( pcreResult ) {

        case PCRE_ERROR_NOMATCH:
            DBGM ( "String didn't match", "pcre Execution Error" );
            break;

        case PCRE_ERROR_NOMEMORY:
            DBGM ( "Memory error", "pcre Execution Error" );
            break;

        default:
            DBGM ( "Error", "pcre Execution Error" );
            break;
        }
*/
    }
else {                                  // 1 or more matches
//    DBGM3 ( "Match!", tosearch, regexp, "pcre Execution" );
//    DBGV ( NumMatches, tosearch + pcreOutputVector[ 0 ] /*"TStringGrep::Matched.NumMatches"*/ );

    NumMatches      = pcreResult;

    for ( int i = 0; i < NumMatches; i++ )
                                        // starting pointers INSIDE string
        ToMatch[ i ]    = tosearch + pcreOutputVector[ 2 * i ];


    if ( matches ) {
                                        // loop through the matched substrings
        char                substring[ KiloByte ];
  
        for ( int i = 0; i < NumMatches; i++ ) {
                                        // first string is the full match; successive strings are optional groups retrieved from the expression
            StringCopy      ( substring, tosearch + pcreOutputVector[ 2 * i ], pcreOutputVector[ 2 * i + 1 ] - pcreOutputVector[ 2 * i ] );

            matches->Add    ( substring );

//            DBGM ( substring, "pcre SubString" );
            }  
        }
    } // pcreResult > 0 


return  NumMatches;
}


//----------------------------------------------------------------------------
                                        // Should have been initialized with Set ( search, replace, options )
bool    TStringGrep::SearchAndReplace   ( char* s )
{
TStrings            matches;

if ( ! Matched  ( s, &matches ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // addressing the replaced string only
char                replaced[ PCRE_REPLACEDLENGTH ];

StringCopy  ( replaced, ReplaceRegexp, PCRE_REPLACEDLENGTH - 1 );

                                        // now search for any capture group
char                group[ 4 ];
                                        // currently limited to 9 capture groups
                                        // also allows \0 as whole capture
for ( int groupi = 0; groupi <= NoMore ( 9, NumMatches - 1 ); groupi++ ) {
    
    StringCopy      ( group, "\\", IntegerToString ( groupi ) );
                                        // replaces ALL instances of string group
    StringReplace   ( replaced, group, matches[ groupi ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // addressing the whole resulting string
size_t              searchlength    = StringLength ( matches[ 0 ] );
size_t              replacelength   = StringLength ( replaced     );
char*               startsearch     = s + pcreOutputVector[ 0 ];
char*               postsearch      = startsearch + searchlength;
char*               postreplace     = startsearch + replacelength;


if ( searchlength != replacelength )
                                        // will work for all cases - also moves the final EOS char
    MoveVirtualMemory   ( postreplace, postsearch,  StringSize ( postsearch ) );


if ( replacelength != 0 )
                                        // poke new characters, without the EOS char
    CopyVirtualMemory   ( startsearch, replaced,    replacelength );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // for a single call, allocates an object just for the evaluation
bool    StringGrep ( const char* text, const char* regexp, GrepOption options )
{
return  TStringGrep ( regexp, options ).Matched ( text );
}


//----------------------------------------------------------------------------
                                        // Neutralize a string to not interfere with a Grep
void    StringGrepNeutral ( char* s )
{
if ( s == 0 )
    return;

                                        // exhaustive list(?)
TSplitStrings       specialgrep ( "[ ] ( ) { } ^ $ . + * ? & -", UniqueStrings );
char                newpart     [ 256 ];

                                        // 1) transforms 1 backslash into 2 (editor gets in the way, hence the doubling)
StringReplace   ( s, "\\",  "\\\\" );   

                                        // 2) loop through all the others special chars
for ( int i = 0; i < (int) specialgrep; i++ ) {

    StringCopy      ( newpart,  "\\",               specialgrep[ i ] );
    StringReplace   ( s,        specialgrep[ i ],   newpart          );
    }

}


//----------------------------------------------------------------------------
                                        // convert the usual list of extensions to a grepa-able one
void    ExtensionsToGrep ( char* grepexts, const char* exts )
{
if ( ! exts || ! grepexts )
    return;

ClearString ( grepexts );

if ( StringIsEmpty ( exts ) )
    return;

                                        // transform extensions into pipe-separate extensions
StringCopy      ( grepexts, exts );
                                        // get rid of beginning and ending spaces
StringCleanup   ( grepexts );
                                        // replace all these possible separators (although space is the usual one) with a "|"
ReplaceChars    ( grepexts, " ,;\t\n", "|||||" );

                                        // now if there are some pipes, we need round brackets around them
if ( StringContains ( (const char*) grepexts, "|" ) ) {
    StringPrepend ( grepexts, "(" );
    StringAppend  ( grepexts, ")" );
    }

                                        // add a real "." before the extension(s)
StringPrepend ( grepexts, "\\." );

                                        // finally, extensions are always at the end of the file
StringAppend  ( grepexts, "$" );
}


//----------------------------------------------------------------------------
                                        // strings can have any typical separators, even with duplication, without porblem
void    ListToGrep ( char* grepablestrings, const char* strings )
{
if ( ! grepablestrings || ! strings )
    return;

ClearString ( grepablestrings );

if ( StringIsEmpty ( strings ) )
    return;


                                        // split with many types of separators, or duplicated spaces f.ex.
TSplitStrings       splits ( strings, UniqueStrings );


StringCopy          ( grepablestrings, "(" );

for ( int i = 0; i < (int) splits; i++ ) {
                                        // strings can have special characters, lets convert them
                                        // tokens were allocated with a bit of extra space
    StringGrepNeutral   ( splits[ i ] );
                                        // grep separators
    StringAppend        ( grepablestrings, i ? "|" : "" );
                                        // grep-friendly string
    StringAppend        ( grepablestrings, splits[ i ] );
    }

StringAppend        ( grepablestrings, ")" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
