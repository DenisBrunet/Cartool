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

#include    <pcre.h>                    // pcre regular expressions (using vcpkg)

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Converting pcre defines into types, while adding some more options
enum    GrepOption  {
                                        // Taken from pcre.h:
                    GrepOption_PCRE_CASELESS        = PCRE_CASELESS       ,     // 0x00000001
                    GrepOption_PCRE_MULTILINE       = PCRE_MULTILINE      ,     // 0x00000002
                    GrepOption_PCRE_DOTALL          = PCRE_DOTALL         ,     // 0x00000004
                    GrepOption_PCRE_EXTENDED        = PCRE_EXTENDED       ,     // 0x00000008
                    GrepOption_PCRE_ANCHORED        = PCRE_ANCHORED       ,     // 0x00000010
                    GrepOption_PCRE_DOLLAR_ENDONLY  = PCRE_DOLLAR_ENDONLY ,     // 0x00000020
                    GrepOption_PCRE_EXTRA           = PCRE_EXTRA          ,     // 0x00000040
                    GrepOption_PCRE_NOTBOL          = PCRE_NOTBOL         ,     // 0x00000080
                    GrepOption_PCRE_NOTEOL          = PCRE_NOTEOL         ,     // 0x00000100
                    GrepOption_PCRE_UNGREEDY        = PCRE_UNGREEDY       ,     // 0x00000200
                    GrepOption_PCRE_NOTEMPTY        = PCRE_NOTEMPTY       ,     // 0x00000400
                    GrepOption_PCRE_UTF8            = PCRE_UTF8           ,     // 0x00000800
                    GrepOption_PCRE_NO_AUTO_CAPTURE = PCRE_NO_AUTO_CAPTURE,     // 0x00001000
                    GrepOption_PCRE_NO_UTF8_CHECK   = PCRE_NO_UTF8_CHECK  ,     // 0x00002000
                    GrepOption_PCRE_AUTO_CALLOUT    = PCRE_AUTO_CALLOUT   ,     // 0x00004000
                    GrepOption_PCRE_PARTIAL         = PCRE_PARTIAL        ,     // 0x00008000
                    GrepOption_PCRE_DFA_SHORTEST    = PCRE_DFA_SHORTEST   ,     // 0x00010000
                    GrepOption_PCRE_DFA_RESTART     = PCRE_DFA_RESTART    ,     // 0x00020000
                    GrepOption_PCRE_FIRSTLINE       = PCRE_FIRSTLINE      ,     // 0x00040000
                    GrepOption_PCRE_DUPNAMES        = PCRE_DUPNAMES       ,     // 0x00080000
                    GrepOption_PCRE_NEWLINE_CR      = PCRE_NEWLINE_CR     ,     // 0x00100000
                    GrepOption_PCRE_NEWLINE_LF      = PCRE_NEWLINE_LF     ,     // 0x00200000
                    GrepOption_PCRE_NEWLINE_CRLF    = PCRE_NEWLINE_CRLF   ,     // 0x00300000
                    GrepOption_PCRE_NEWLINE_ANY     = PCRE_NEWLINE_ANY    ,     // 0x00400000

                    GrepOption_PCRE_MASK            = 0x007FFFFF,               // Mask for all pcre defines above

                                                                                //              Cartool addition:
                    GrepOptionNone                  = 0x00000000,               //

                    GrepOptionSyntaxPerl            = 0x10000000,               // default syntax
                    GrepOptionSyntaxShell           = 0x20000000,               // like a DOS shell - not really used, learn the Perl syntax instead!
                    GrepOptionFileNameOnly          = 0x01000000,               // in case of file names, search for the file name part only (past the last directory)
                    GrepOptionCartoolMask           = GrepOptionSyntaxPerl | GrepOptionSyntaxShell | GrepOptionFileNameOnly,


                    GrepOptionDefault               = GrepOptionSyntaxPerl      // perl syntax
                                                    | GrepOption_PCRE_CASELESS, // ignoring cases

                    GrepOptionDefaultFiles          = GrepOptionDefault
                                                    | GrepOptionFileNameOnly,   // skipping the directory part of file paths

                    GrepOptionDefaultMarkers        = GrepOptionDefault,
                    };


constexpr int       PCRE_MAXRETURNMATCHES   = 50;
constexpr int       PCRE_MAXOVECTOR         = 2 * PCRE_MAXRETURNMATCHES;
constexpr int       PCRE_REPLACEDLENGTH     = KiloByte;

                                        // A few useful grep expressions
constexpr char*     GrepEmptyString         = "^$";             // empty string - !will return true for "\n" string!
constexpr char*     GrepEmptyInput          = "^(|\\s)*$";      // empty dialog input: empty, or any mix of spaces, tabs and newlines


class               TStrings;

                                        // Wrapper around the pcre library, using the Perl syntax by default
class  TStringGrep
{
public:
                    TStringGrep     ();
                    TStringGrep     ( const char* regexp, GrepOption options );
                    TStringGrep     ( const char* searchregexp, const char* replaceregexp, GrepOption options );
                   ~TStringGrep     ();


    void            Reset           ();
    bool            Set             ( const char* regexp, GrepOption options );
    bool            Set             ( const char* searchregexp, const char* replaceregexp, GrepOption options );

    bool            IsValid         ()          const           { return  RegExpCompiled != 0; }
    bool            HasMatches      ()          const           { return  NumMatches > 0; }
    int             GetNumMatches   ()          const           { return  NumMatches; }

    int             Matched         ( const char* s, TStrings* matches = 0 );   // optional argument to return the match(es)
    bool            SearchAndReplace( char* s );

                                        // returns a pointer to beginning of a given match
//  char*           GetMatchStart   ( int i )   const           { return  IsInsideLimits ( i, 0, NumMatches - 1 ) ? const_cast<char*> ( ToMatch[ i ] ) : 0; }
    const char*     GetMatchStart   ( int i )   const           { return  IsInsideLimits ( i, 0, NumMatches - 1 ) ? ToMatch[ i ] : 0; }


                    operator bool   ()          const           { return  IsValid (); }


protected:
                                        // Full user's options
    GrepOption      Options;
                                        // pcre variables
    ::pcre*         RegExpCompiled;
    int             pcreOutputVector[ PCRE_MAXOVECTOR ];        // could be static or global, but we maintain thread safe behavior by allocating it per obejct
    char            ReplaceRegexp   [ PCRE_REPLACEDLENGTH   ];

    int             NumMatches;
    const char*     ToMatch         [ PCRE_MAXRETURNMATCHES ];  // pointers to sub-strings' matches


    void            ResetResults    ();
    void            ShellToPerl     ( char* regexp )    const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrappers to TStringGrep - recommended for single calls only
bool                StringGrep       ( const char* s, const char* regexp, GrepOption options ); // it is strongly recommended to use TStringGrep direclty, this is just a handy wrapper for a single call
void                StringGrepNeutral( char*       s);                                          // "neutralize" "." in file names f.ex.
void                ExtensionsToGrep ( char* grepexts, const char* exts );                      // convert a list of extension(s) to a Grep syntax
void                ListToGrep       ( char* grepablestrings, const char* strings );            // convert a list of strings to a Grep syntax - strings should be separated by space


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
