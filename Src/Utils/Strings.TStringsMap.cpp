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

#include    <fstream>

#include    "Strings.TStringsMap.h"
#include    "Strings.TSplitStrings.h"
#include    "Strings.Utils.h"
#include    "Files.Utils.h"

#include    "Volumes.AAL.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int     TStringsMap::GetMaxKey ()  const
{
if ( IsEmpty () )
    return  0;


int                 maxvalue        = INT_MIN;

for ( int i = 0; i < (int) Key; i++ )

    Maxed ( maxvalue, StringToInteger ( Key[ i ] ) );


return  maxvalue;
}


//----------------------------------------------------------------------------
void    TStringsMap::Add ( const char* file )
{
if ( StringIsEmpty ( file ) || ! CanOpenFile ( file ) )
    return;


ifstream            ifp ( TFileName ( file, TFilenameExtendedPath ) );
char                buff[ 1024 ];
char                sindex[ 256 ];
int                 line            = 0;

                                        // scan file, each line is 1 ROI
do {

    ifp.getline ( buff, 1024 );

    if ( StringIsEmpty ( buff ) || StringIsSpace ( buff ) )
        continue;


    TSplitStrings   tokens ( buff, UniqueStrings );

    if ( (int) tokens < 1 )             // we have a problem here...
        continue;

    line++;

                                        // in case the first token is not an integer, force the line number
    StringCopy ( sindex, tokens[ 0 ] );

    if ( ! IsInteger ( sindex ) ) {
        IntegerToString ( sindex, line );
        Key  .Add ( sindex );           // force identity
        Value.Add ( sindex );
        }
                                        // associate the first token to each of the following tokens, including the first one
                                        // f.ex.: "1"-"1" / "1"-"Precentral_L" / "1"-"2001"
    for ( int i = 0; i < (int) tokens; i++ ) {
        Key  .Add ( sindex      );
        Value.Add ( tokens[ i ] );
        }

    } while ( ! ifp.eof() );


//Key  .Show ( "Key" );
//Value.Show ( "Value" );
}


//----------------------------------------------------------------------------
void    TStringsMap::Add ( const TAALRoi* aal, int numlines )
{
if ( aal == 0 )
    return;


char                sindex[ 256 ];
char                svalue[ 256 ];


for ( int i = 0; i < numlines; i++ ) {
                                        // convert everything to string
    IntegerToString ( sindex, aal[ i ].Index );
                                        // 1 <-> 1
    Key  .Add ( sindex );
    Value.Add ( sindex );
                                        // 1 <-> "FAL"
    Key  .Add ( sindex );
    Value.Add ( aal[ i ].ShortName );
                                        // 1 <-> "Precentral_L"
    Key  .Add ( sindex );
    Value.Add ( aal[ i ].LongName );
                                        // 1 <-> 2001 - obsolete, value can be 0
    if ( aal[ i ].Value != 0 ) {
        Key  .Add ( sindex );
        Value.Add ( IntegerToString ( svalue, aal[ i ].Value ) );
        }
    } // for line


//Key  .Show ( "Key" );
//Value.Show ( "Value" );
}


//----------------------------------------------------------------------------
void    TStringsMap::Add ( int min, int max )
{
char                sindex[ 256 ];


for ( int i = min; i <= max; i++ ) {
                                        // convert to string
    IntegerToString ( sindex, i );

    Key  .Add ( sindex );
    Value.Add ( sindex );
    } // for line


//Key  .Show ( "Key" );
//Value.Show ( "Value" );
}


//----------------------------------------------------------------------------
bool    TStringsMap::ValueToKey ( const char* value, int& key )    const
{
key     = 0;

if ( IsEmpty () || StringIsEmpty ( value ) )
    return  false;

                                        // pairs are: "int" / string
for ( int i = 0; i < (int) Value; i++ )

    if ( StringIs ( Value[ i ], value ) ) {

        key     = StringToInteger ( Key[ i ] );

        return  true;
        }


return  false;
}


//----------------------------------------------------------------------------
const char* TStringsMap::GetValue ( int key )  const
{
if ( IsEmpty () )
    return  "";


char                value[ 256 ];
int                 keyi            = -1;

ClearString ( value );

                                        // pairs are: "int" / string
for ( int i = 0; i < (int) Key; i++ )

    if ( StringToInteger ( Key[ i ] ) == key ) {
                                        // a bit tricky: return the longest string which matched the key
        if ( StringLength ( Value[ i ] ) > StringLength ( value ) ) {

            keyi    = i;

            StringCopy ( value, Value[ i ] );
            }
        }


return  keyi >= 0 ? Value[ keyi ] : "";
}


char*   TStringsMap::KeyToValue ( int key, char* value )   const
{
ClearString ( value );

StringCopy ( value, GetValue ( key ) );

return  value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
