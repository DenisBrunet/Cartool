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

#include    <owl/pch.h>

#include    "TSpiDoc.h"

#include    "Math.Utils.h"
#include    "Files.Stream.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TSpiDoc::TSpiDoc ( TDocument* parent )
   	  : TSolutionPointsDoc ( parent )
{
}


//----------------------------------------------------------------------------
bool	TSpiDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail () )
    return false;

switch ( what ) {

    case ReadNumSolPoints :
        *((int *)answer) = AtLeast ( 0, CountLines ( file ) );
        return  true;

    }


return false;
}


//----------------------------------------------------------------------------
bool	TSpiDoc::Open ( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );


SetDirty ( false );


if ( GetDocPath () ) {

    TPoints&            Points          = GetPoints ( DisplaySpace3D );

    Points .ReadFile ( (char *) GetDocPath (), &SPNames );


//    double      md      = Points.GetMedianDistance ();
//    for ( int sp = 0; sp < GetNumSolPoints (); sp++ )
//        Points[ sp ].RoundTo ( md );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // testing if names exist: test first line
    ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ) );
    char                buff  [ 256 ];

    ifs.getline ( buff, 256 );

    HasNames            = sscanf ( buff, "%*f %*f %*f %s", buff ) == 1;

//  DBGV ( HasNames, GetTitle () );
    }
else {                                  // can not create a SPI file
    return  false;
    }


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
