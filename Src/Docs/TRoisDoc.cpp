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

#include    <owl/pch.h>

#include    "MemUtil.h"

#include    "TRoisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-


using   namespace   owl;
using   namespace   std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TRoisDoc::TRoisDoc ( TDocument *parent )
      : TBaseDoc ( parent )
{
ContentType         = ContentTypeRoi;

ROIs                = 0;
}


        TRoisDoc::~TRoisDoc ()
{
if ( ROIs )
    delete  ROIs, ROIs = 0;
}


bool	TRoisDoc::InitDoc ()
{
if ( ! IsOpen () )
    return  Open ( ofRead, 0 );

return  true;
}


bool    TRoisDoc::Commit     ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return  true;

SetDirty ( false );

return  true;
}


bool    TRoisDoc::Revert ( bool clear )
{
if ( ! TFileDocument::Revert(clear) )
    return false;

if ( ! clear ) {
    Close();
    Open ( ofRead );
    }

SetDirty ( false );
return true;
}


//----------------------------------------------------------------------------
bool	TRoisDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );
if ( ifs.fail() ) return false;

char        buff[ KiloByte ];

                                        // read magic number
ifs.getline ( buff, KiloByte );
if ( ! StringStartsWith ( buff, ROITXT_MAGICNUMBER1 ) )
    return false;
                                        // ROITXT_MAGICNUMBER1 is a list of indexes
//Type    = RoiIndex;


switch ( what ) {
    case ReadNumElectrodes :
    case ReadNumSolPoints  :

        ifs.getline ( buff, KiloByte );
        sscanf ( buff, "%d", (int *) answer );
        return  true;

    case ReadNumRois :

        ifs.getline ( buff, KiloByte );
        ifs.getline ( buff, KiloByte );
        sscanf ( buff, "%d", (int *) answer );
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool    TRoisDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath() ) {

    if ( ROIs )
        delete  ROIs;

    ROIs    = new TRois ( (char *) GetDocPath () );
    }
else {
    return false;
    }


return true;
}


bool    TRoisDoc::Close ()
{
if ( ! IsOpen() )   return true;

return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
