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

#include    "CartoolTypes.h"
#include    "Strings.TSplitStrings.h"
#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TSplitStrings::TSplitStrings ()
{
Reset ();
}


        TSplitStrings::TSplitStrings ( const char* str, StringsUnicity unicity, const char* separators )
{
Set ( str, unicity, separators );
}


//----------------------------------------------------------------------------
void    TSplitStrings::Set ( const char* str, StringsUnicity unicity, const char* separators )
{
Reset ();

Add ( str, unicity, separators );
}


void    TSplitStrings::Reset ()
{
Tokens.Reset ();
}


//----------------------------------------------------------------------------
                                        // Add everything, including '*' or regex
void    TSplitStrings::Add ( const TSplitStrings& splitstring, StringsUnicity unicity )
{
for ( int i = 0; i < splitstring.GetNumTokens (); i++ )
    AddToken ( splitstring[ i ] );


if ( unicity == UniqueStrings )
    RemoveDuplicates ();
}


//----------------------------------------------------------------------------

#define             DoubleQuoteC        '\"'
#define             DoubleQuoteS        "\""

                                        // Because some tokens might be in quote, and not some others, we need to browse tokens manually - strtok will not do the job
void    TSplitStrings::Add ( const char* str, StringsUnicity unicity, const char* separators )
{
if ( StringIsEmpty ( str ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TArray1<char>       buff ( StringSize ( str ) );
char*               begin           = EOS;      // beginning of current token - 0 if none started
bool                withinquote     = false;    // flagging if we are within quotes, where separators are not ignored

                                        // copy to temp string
StringCopy ( (char *) buff, str );

                                        // scan one char at a time
for ( char* totok = (char *) buff; *totok; totok++ ) {

    if ( ! withinquote ) {
                                        // starting a new quote?
        if      ( begin == 0 && *totok == DoubleQuoteC ) {  // note: for the moment, a quote is "allowed" inside a token

            withinquote = true;
            }
                                        // legal separators?
        else if ( IsCharAmong ( *totok, separators ) ) {
                                        // any open token?
            if ( begin != 0 ) {
                *totok  = EOS;         // poke EOS
                AddToken ( begin );
                begin   = EOS;         // finished this one
                }
            } // separator

        else { // token stuff, finally

            if ( begin == 0 )           // starting a new token?
                begin   = totok;
            }

        } // ! withinquote

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else { // withinquote
                                        // ending quote?
        if      ( *totok == DoubleQuoteC ) {
                                        // any open token?
            if ( begin != 0 ) {
                *totok  = EOS;         // poke EOS
                AddToken ( begin );
                begin   = EOS;         // finished this one
                }
                                        // here we ignore ""
            withinquote = false;
            } // DoubleQuoteC

        // separators are NOT tested here, they are allowed inside quotes

        else { // ! DoubleQuoteC

            if ( begin == 0 )           // starting a new token?
                begin   = totok;
            } // ! DoubleQuoteC

        } // withinquote

    } // for totok

                                        // any open token? allowing "ABC to be entered as ABC, even though the quote was not properly ended?
if ( begin != 0 )

    AddToken ( begin );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( unicity == UniqueStrings )
    RemoveDuplicates ();


///*if ( VkQuery () )*/  Show ();
}

//----------------------------------------------------------------------------
                                        // This is the key function, testing for an existing string
bool    TSplitStrings::Contains ( const char* str ) const
{
return  Tokens.Contains ( str ) 
     || Tokens.Contains ( "*" );        // a single wildchar '*' will match everything
}


bool    TSplitStrings::PartiallyContains ( const char* str ) const
{
return  Tokens.PartiallyContains ( str )
     || Tokens.Contains          ( "*" );   // a single wildchar '*' will match everything
}


//----------------------------------------------------------------------------
bool    TSplitStrings::Intersect ( const TSplitStrings& splitstring )    const
{
for ( int i = 0; i <             GetNumTokens (); i++ )
for ( int j = 0; j < splitstring.GetNumTokens (); j++ )

    if ( StringIs ( splitstring[ j ], Tokens[ i ] ) )

        return  true;

return  false;
}


//----------------------------------------------------------------------------
void    TSplitStrings::Show ( const char* title )   const
{
Tokens.Show ( StringIsEmpty ( title ) ? "Tokens" : title );
}


char*   TSplitStrings::ToString ( char* str, ExecFlags execflags ) const
{
if ( str == 0 )
    return str;


if ( GetNumTokens () == 0 ) {
    if ( execflags == ExpandedString )  StringCopy  ( str, "- None -" );
    else                                ClearString ( str );

    return str;
    }

                                        // get some more room, in case of expansion
char*               buff            = new char [ 3 * Tokens.GetMaxStringLength () + 16 ];

ClearString ( str );


                                        // scan all tokens
for ( int i = 0; i < GetNumTokens (); i++ ) {

    StringCopy ( buff, Tokens[ i ] );

                                        // if compact, replace any wildchar
    if ( execflags == CompactString ) {

        if ( StringIs ( buff, "*" ) )   // replace single "*" with "All"

            StringCopy ( buff, "All" );
        else                            // f.ex. "stuff*" -> "stuffAll"
            StringReplace ( buff, "*", "All" );
        }

                                        // add quotes if it contains space
    if ( execflags == ExpandedString && StringContains ( buff, ' ' ) ) {

        StringPrepend ( buff, DoubleQuoteS );
        StringAppend  ( buff, DoubleQuoteS );
        }

                                        // need a separator to global string?
    if ( execflags == ExpandedString && ! StringIsSpace ( str ) )
        StringAppend ( str, ", " );

                                        // then add to current string
    StringAppend ( str, buff );
    }


delete[]    buff;
return  str;
}


//----------------------------------------------------------------------------
void    TSplitStrings::RemoveDuplicates ()
{
if ( GetNumTokens () <= 1 )
    return;


for ( int i = 0; i < GetNumTokens () - 1; i++ ) {

    for ( int j = i + 1; j < GetNumTokens (); j++ )

        if ( StringIs ( Tokens[ j ], Tokens[ i ] ) ) {
            Tokens.RemoveRef ( Tokens[ j ] );
            i--;                        // do token i again
            break;
            }
    }
}


void    TSplitStrings::FilterWith ( const TStrings& strs, ExecFlags execflags )
{
if ( GetNumTokens () == 0 || strs.IsEmpty () )
    return;


for ( int i = 0; i < GetNumTokens (); i++ ) {
                                        // skip wildchar, fits to all strings
    if ( StringIs ( Tokens[ i ], "*" ) )
        continue;

    if ( ! strs.Contains ( Tokens[ i ] ) ) {

        if ( IsInteractive ( execflags ) )
            ShowMessage ( "Can not find this name," NewLine "check your spelling!", Tokens[ i ], ShowMessageWarning );

        Tokens.RemoveRef ( Tokens[ i ] );
        i--;
        }
    }
}


void    TSplitStrings::Remove ( const char* removestrs )
{
if ( GetNumTokens () == 0 || StringIsEmpty ( removestrs ) )
    return;


for ( int i = 0; i < GetNumTokens (); i++ )
    if ( IsStringAmong ( Tokens[ i ], removestrs ) )
        Tokens.RemoveRef ( Tokens[ i ] );
}

                                        // Remove space-y tokens (splitting might have been done with non-space-y separators)
void    TSplitStrings::CompactSpaces ()
{
if ( GetNumTokens () == 0 )
    return;


for ( int i = 0; i < GetNumTokens () - 1; i++ ) {

    if ( StringIsSpace ( Tokens[ i ] ) ) {
        Tokens.RemoveRef ( Tokens[ i ] );
        i--;                            // do this token again
        }
    }
}


void    TSplitStrings::ExpandWildchars ( const TStrings& strs, StringsUnicity unicity )
{
if ( GetNumTokens () == 0 || strs.IsEmpty () )
    return;


char*               toc;
char                buff[ 256 ];
int                 baselen;


for ( int i = 0; i < GetNumTokens (); i++ ) {
                                        // assume only one '*'
    if ( ( toc = StringContains ( Tokens[ i ], '*', StringContainsBackward ) ) == 0 )
        continue;

                                        // copy the template before the '*'
    baselen         = toc - Tokens[ i ];
    StringCopy ( buff, Tokens[ i ], baselen );

                                        // remove templated token
    Tokens.RemoveRef ( Tokens[ i ] );
    i--;

                                        // scan all strings for a template match
    TListIterator<char>     iterator;

    foreachin ( strs, iterator )
        if ( ! Contains ( iterator() ) && StringStartsWith ( iterator(), buff ) )
            AddToken ( iterator() );
    }


if ( unicity == UniqueStrings )
    RemoveDuplicates ();

//Show ( "After Removing Duplicates" );
}

/* Should also do the Assignation and Copy constructor
TSplitStrings&  TSplitStrings::operator+= ( TSplitStrings &op2 )
{
if ( ! (bool) Tokens ) {
//  Tokens      = op2.Tokens;           // copy operator does not exist!

    for ( int j = 0; j < op2.GetNumTokens (); j++ )
        AddToken ( op2.Tokens[ j ] );
    }
else {
                                        // add op2 to receiver
    for ( int j = 0; j < op2.GetNumTokens (); j++ )

        if ( ! Contains ( op2.Tokens[ j ] ) )
            AddToken ( op2.Tokens[ j ] );
    }


if ( unicity == UniqueStrings )
    RemoveDuplicates ();


return  *this;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
