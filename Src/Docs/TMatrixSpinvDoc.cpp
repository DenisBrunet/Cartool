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

#include    "TMatrixSpinvDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;
using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMatrixSpinvDoc::TMatrixSpinvDoc ( TDocument *parent )
      : TInverseMatrixDoc ( parent )
{
}


bool    TMatrixSpinvDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;

float               f;


switch ( what ) {
    case ReadNumSolPoints :
        ifs.read ( (char *) &f,  sizeof ( f ) );
        *((int *)answer) = f;
        return   true ;

    case ReadNumElectrodes :
        ifs.seekg ( sizeof ( float ), ios::cur );
        ifs.read ( (char *) &f,  sizeof ( f ) );
        *((int *)answer) = f;
        return   true;
    }


return false;
}


bool    TMatrixSpinvDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );


if ( GetDocPath () ) {

    TInStream*          is              = InStream ( ofRead | ofBinary );

    if ( ! is ) return false;


    SetDirty ( false );


    float               f;

    is->read ( (char *) &f,  sizeof ( f ) );
    NumSolPoints        = f;

    is->read ( (char *) &f,  sizeof ( f ) );
    NumElectrodes       = f;

    SetAtomType ( AtomTypeVector );
    NumRegularizations  = 0;
    SetDefaultVariables ();

                                        // matrix allocation
    M.resize ( 1 );

    M[ 0 ].Resize ( GetNumLines (), NumElectrodes );

                                        // read data
    is->read ( (char *) M[ 0 ].GetArray (), SingleMatrixMemorySize () );


    delete is;
    }
else {                                  // create a IS file
    SetDirty ( false );
    return  false;
    }


//char    buff[ 256 ];
//DBGM2 ( GetContentTypeName ( buff ), GetAtomTypeName ( AtomTypeUseCurrent ), "Content Type,  Atom Type" );

return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
