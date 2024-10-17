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

#include    <iostream>
#include    <iomanip>
#include    <io.h>

#include    "Strings.Utils.h"
#include    "Strings.TStrings.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // XML utilities, the quick and lazy way: reading an input stream directly, without loading the full tree in memory

                                        // count elements within a scope, optionally restarting from beginning
                                        // leaves the position unchanged upon returning
inline int          XMLCountElement ( std::istream& is, char* element, char* withinscope, bool frombeginning = false );
                                        // position stream at the beginning just after <element>, while remaining within an optional scope
                                        // rewind to calling position if element is not to be found, as to avoid reading the whole file
                                        // one improvement would be to also have a beginning scope
inline bool         XMLGotoElement  ( std::istream& is, char* element, char* withinscope, TStrings* attributes = 0, bool frombeginning = false );
                                        // read element, while remaining within an optional scope
                                        // one drawback is that it moves the file pointer forward, so call/written order matters
inline bool         XMLGetElement   ( std::istream& is, char* element, char* withinscope, char* result, TStrings* attributes = 0, bool frombeginning = false );

//inline bool       XMLFlushScope   ( std::istream& is, char* withinscope );
//inline bool       XMLNextElement  ( std::istream& is, char* element, char* withinscope, char* result, TStrings* attributes, bool frombeginning );


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
int         XMLCountElement ( std::istream& is, char* element, char* withinscope, bool frombeginning )
{
                                        // reset position?
if ( frombeginning )
    is.seekg ( 0, ios::beg );
                                        // remember position, so as to restore it upon leaving
long                ispos           = is.tellg ();


int                 numelements     = 0;

do {

    if ( XMLGotoElement ( is, element, withinscope, 0 ) )
        ++numelements;
    else
        break;

    } while ( is.good () );

                                        // restore from where we were at call time
is.seekg ( ispos, ios::beg );


return  numelements;
}


//----------------------------------------------------------------------------
bool        XMLGotoElement ( std::istream& is, char* element, char* withinscope, TStrings* attributes, bool frombeginning )
{
char                buff        [ 1024 ];

                                        // reset position?
if ( frombeginning )
    is.seekg ( 0, ios::beg );

if ( attributes )
    attributes->Reset ();


long                currentpos      = is.tellg ();


do {
                                        // go to next beginning tag "<"
    is.ignore  (     100000, '<' );
                                        // read up to the end of beginning tag ">"
    is.getline ( buff, 1024, '>' );

                                        // reached the beginning of element tag?
    if ( StringStartsWith ( buff, element ) ) {
//        DBGM ( buff, "XMLGotoElement" );

        if ( attributes ) {
                                        // do the job for the caller: split by spaces, return list
            TSplitStrings   attr ( buff + StringLength ( element ), NonUniqueStrings, " =\t\n" );
//            attr.Show ( "XMLGotoElement / Attributes" );

            attributes->Set ( attr.GetTokens () );
            }

        return  true;
        }
                                        // reached the given scope?
    else if ( StringIsNotEmpty ( withinscope )
           && buff[ 0 ] == '/' && StringStartsWith ( buff + 1, withinscope ) ) {
//           && buff[ 0 ] == '/' && StringContains ( withinscope, buff + 1, StringContainsCase | StringContainsForward ) ) {    // ?does not work?
                                        // searched element not found
        is.seekg ( currentpos, ios::beg );  // if missing / failed, restore position from entry call, otherwise the position is at the end of withinscope

        return  false;
        }


    } while ( is.good () );


is.seekg ( currentpos, ios::beg );      // if missing / failed, restore position from entry call, otherwise the position is at the end of withinscope

return  false;
}


//----------------------------------------------------------------------------
bool        XMLGetElement ( std::istream& is, char* element, char* withinscope, char* result, TStrings* attributes, bool frombeginning )
{
ClearString ( result );

                                        // go to tag
if ( ! XMLGotoElement ( is, element, withinscope, attributes, frombeginning ) ) {
//  StringCopy ( result, "NOT FOUND" );
    return  false;
    }


char                tempresult  [ 65 * KiloByte ];
char                buff        [ 1024 ];

ClearString ( tempresult );
                                        // loop to read until the end of the _requested_ tag
                                        // which means, if this is a parent tag, result will contain all children tags too
do {
                                        // read up to the next tag "<"
    is.getline ( buff, 1024, '<');

    StringAppend ( tempresult, buff );

                                        // read end of tag
    is.getline ( buff, 1024, '>');

//    DBGM ( buff, "XMLGetElement - reading" );

                                        // reached the terminating tag?
    if ( buff[ 0 ] == '/' && StringStartsWith ( buff + 1, element ) ) {
                                        // replace XML single EOL with C \n
        char            EOL[ 2 ];
        EOL[ 0 ]    = 0x0D;
        EOL[ 1 ]    = 0;
        StringReplace ( tempresult, EOL, "\n" );

//        DBGM ( tempresult, "XMLGetElement" );

        StringCopy ( result, tempresult );
        return  true;
        }
                                        // reached the given scope?
    else if ( StringIsNotEmpty ( withinscope )
           && buff[ 0 ] == '/' && StringStartsWith ( buff + 1, withinscope ) ) {
                                        // searched element not found
//      StringCopy ( result, "NOT FOUND" );
        return  false;
        }

                                        // not the end -> add what has been read to temp result
    StringAppend ( tempresult, "<", buff, ">" );

    } while ( is.good () );


return  false;
}


//----------------------------------------------------------------------------
/*                                      // NOT TESTED
                                        // Finish up current scope
bool        XMLFlushScope ( std::istream& is, char* withinscope )
{
if ( StringIsEmpty ( withinscope ) )
    return  false;


char                buff        [ 1024 ];


do {
                                        // go to next beginning tag "<"
    is.ignore  (     100000, '<' );
                                        // read up to the end of beginning tag ">"
    is.getline ( buff, 1024, '>' );

                                        // reached the end of given scope?
    if ( buff[ 0 ] == '/' && StringContains ( withinscope, buff + 1, StringContainsCase | StringContainsForward ) ) {

        return  true;
        }

    } while ( is.good () );


return  false;
}


bool        XMLNextElement ( std::istream& is, char* element, char* withinscope, char* result, TStrings* attributes, bool frombeginning )
{
                                        // get to the end of current scope
if ( ! XMLFlushScope ( is, withinscope ) )
    return  false;

return  XMLGetElement ( is, element, withinscope, result, attributes, frombeginning );
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
