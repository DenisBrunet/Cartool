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

#include    "TSxyzDoc.h"

#include    "Math.Utils.h"
#include    "Files.Stream.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TSxyzDoc::TSxyzDoc ( TDocument *parent )
   	  : TSolutionPointsDoc ( parent )
{
}


//----------------------------------------------------------------------------
bool	TSxyzDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail() ) return false;

char        buff[ 128 ];


switch ( what ) {

    case ReadNumSolPoints :

        int     numsp   = StringToInteger ( GetToken ( &ifs, buff ) );

        *((int *)answer) = ( numsp > 0 ) ? numsp : 0;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TSxyzDoc::Open ( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );


SetDirty ( false );


if ( GetDocPath () ) {

    HasNames            = true;

    TPoints&        Points          = GetPoints ( DisplaySpace3D );

    Points .ReadFile ( (char *) GetDocPath (), &SPNames );
    }
else {                          // can not create a SPI file
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
