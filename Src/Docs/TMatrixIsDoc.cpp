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

#include    "TMatrixIsDoc.h"

#include    "System.h"
#include    "Files.Stream.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;
using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMatrixIsDoc::TMatrixIsDoc ( TDocument *parent )
      : TInverseMatrixDoc ( parent )
{
}


bool    TMatrixIsDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

if ( ifs.fail() ) return false;


switch ( what ) {

    case ReadMagicNumber :
        ifs.read ( (char*) answer, sizeof ( int ) );

        return  IsMagicNumber ( (const char*) answer, ISBIN_MAGICNUMBER1 )
             || IsMagicNumber ( (const char*) answer, ISBIN_MAGICNUMBER2 )
             || IsMagicNumber ( (const char*) answer, ISBIN_MAGICNUMBER3 );

    case ReadNumElectrodes :
        ifs.seekg ( sizeof ( int ), ios::cur );
        ifs.read ( (char*) answer, sizeof ( int ) );
        return   true ;

    case ReadNumSolPoints :
        ifs.seekg ( 2 * sizeof ( int ), ios::cur );
        ifs.read ( (char*) answer, sizeof ( int ) );
        return   true;

    case ReadInverseScalar :
        int             magic;
        ifs.read ( (char *) &magic, sizeof ( int ) );

        ifs.seekg ( ( IsMagicNumber ( (const char*) answer, ISBIN_MAGICNUMBER3 ) ? 4 : 3 ) * sizeof ( int ), ios::cur );
        ifs.read ( (char*) answer, sizeof ( bool ) );
        return   true;
    }


return false;
}


bool    TMatrixIsDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );


if ( GetDocPath () ) {
    TInStream*          is              = InStream ( ofRead | ofBinary );

    if ( ! is ) return false;

    int                 magic;

    SetDirty ( false );

    if ( ! ReadFromHeader ( (char *) GetDocPath(), ReadMagicNumber, &magic ) ) {

        ShowMessage ( "Can not recognize this file (unknown magic number)!", "Open file", ShowMessageWarning );
        delete is;
        return false;
        }

                                        // Read Header
    is->read ( (char *) &magic,         sizeof ( magic         ) );
    is->read ( (char *) &NumElectrodes, sizeof ( NumElectrodes ) );
    is->read ( (char *) &NumSolPoints,  sizeof ( NumSolPoints  ) );

    if ( IsMagicNumber ( magic, ISBIN_MAGICNUMBER3 ) )
        is->read ( (char *) &NumRegularizations,  sizeof ( NumRegularizations ) );
    else
        NumRegularizations  = 0;

    bool                invscal;
    is->read ( (char *) &invscal, sizeof ( invscal ) );     // bool is read as single char
    SetAtomType ( invscal ? AtomTypeScalar : AtomTypeVector );


    SetDefaultVariables ();


    if ( IsMagicNumber ( magic, ISBIN_MAGICNUMBER3 ) ) {
                                        // More header to read, overwrite the defaults
                                        // electrodes names
        for ( int el = 0; el < NumElectrodes; el++ ) {
            is->read ( ElectrodesNames[ el ], InverseMaxElectrodeName );
            ElectrodesNames[ el ][ InverseMaxElectrodeName - 1 ]  = 0;
            }

                                        // sp names
        for ( int sp = 0; sp < NumSolPoints; sp++ ) {
            is->read ( SolutionPointsNames[ sp ], InverseMaxSolutionPointName );
            SolutionPointsNames[ sp ][ InverseMaxSolutionPointName - 1 ]  = 0;
            }

                                        // regularization values
        for ( int reg = 0; reg < NumRegularizations; reg++ )
            is->read ( (char *) &RegularizationsValues[ reg ], RegularizationsValues.AtomSize () );

                                        // regularization names
        for ( int reg = 0; reg < NumRegularizations; reg++ ) {
            is->read ( RegularizationsNames[ reg ], InverseMaxRegularizationName );
            RegularizationsNames[ reg ][ InverseMaxRegularizationName - 1 ]  = 0;
            }
        } // ISBIN_MAGICNUMBER3 Header


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrix(es) allocation
    M.resize ( GetMaxRegularization () );

    for ( auto emi = M.begin (); emi != M.end(); emi++ )

        emi->Resize ( GetNumLines (), NumElectrodes );
//      emi->AResizeFast ( GetNumLines (), NumElectrodes );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read Matrix(es)
    if      ( IsMagicNumber ( magic, ISBIN_MAGICNUMBER1 ) ) {
                                        // row-major - !AReal MUST be float!
        is->read ( (char *) M[ 0 ].GetArray (), SingleMatrixMemorySize () );
        
                                        // column-major
//      AMatrix             Mt ( M[ 0 ].n_cols, M[ 0 ].n_rows );
//                                      // reading row-major
//      is->read ( (char *) Mt.memptr (), SingleMatrixMemorySize () );
//                                      // then transposing to column major
//      M[ 0 ]      = Mt.t ();
        }

    else if ( IsMagicNumber ( magic, ISBIN_MAGICNUMBER2 ) ) {
                                        // convert it
        magic = ISBIN_MAGICNUMBER1;
                                        // read and convert to float - we have no use for double matrix
        double              v;

        for ( int sp = 0; sp < GetNumLines (); sp++ )
        for ( int el = 0; el < NumElectrodes;  el++ ) {
            is->read ( (char *) &v, sizeof ( v ) );
            M[ 0 ] ( sp, el )  = v;
            }
        }

    else if ( IsMagicNumber ( magic, ISBIN_MAGICNUMBER3 ) ) {

        TSuperGauge         Gauge ( "Entering the Matrix", GetMaxRegularization () );
//      AMatrix             Mt ( M[ 0 ].n_cols, M[ 0 ].n_rows );

        for ( int reg = 0; reg < GetMaxRegularization (); reg++ ) {
                                        // loading a stack of matrices can be long, let's advertise it!
            Gauge.Next ();
                                        // row-major - !AReal MUST be float!
            is->read ( (char *) M[ reg ].GetArray (), SingleMatrixMemorySize () );
            
                                        // reading row-major
//          is->read ( (char *) Mt.memptr (), SingleMatrixMemorySize () );
//                                      // then transposing to column major
//          M[ reg ]    = Mt.t ();
            }
        }


    delete is;

    }
else {                                  // create a IS file
    ifstream           *is;
    char                buff[ 256 ];
    bool                infloat         = true;


    SetDirty ( false );


    static GetFileFromUser  getinputfile ( "Input Matrix File", AllInputInverseFilesFilter, 1, GetFileRead );

    if ( ! getinputfile.Execute () )
        return  false;


    if ( ! GetValueFromUser ( "Number of electrodes:", "Reading Matrix", NumElectrodes, "", CartoolMainWindow ) )
        return false;


    if ( ! GetValueFromUser ( "Number of solution points:", "Reading Matrix", NumSolPoints, "", CartoolMainWindow ) )
        return false;


    if ( ! GetInputFromUser ( "(S)calar or (V)ectorial type:", "Reading Matrix", buff, "", CartoolMainWindow ) )
        return false;

    SetAtomType ( StringStartsWith ( buff, "S" ) ? AtomTypeScalar : AtomTypeVector );

    NumRegularizations  = 0;
    SetDefaultVariables ();


    if ( getinputfile.GetFileFilterIndex () == 2 ) { // binary
        if ( ! GetInputFromUser ( "(F)loat or (D)ouble:", "Reading Matrix", buff, "", CartoolMainWindow ) )
            return false;

        infloat     = StringStartsWith ( buff, "F" );
        }


    GetFileFromUser     getoutputfile ( "Output Matrix File", AllInverseFilesFilter, 1, GetFileWrite );

    TFileName           outputfile;
    StringCopy ( outputfile, (char *) getinputfile );
    ReplaceExtension ( outputfile, FILEEXT_IS );

    getoutputfile.SetOnly ( outputfile );

    if ( ! getoutputfile.Execute () )
        return  false;


    if ( ! CanOpenFile ( (char*) getoutputfile, CanOpenFileWriteAndAsk ) )
        return false;


                                        // matrix allocation
    M.resize ( GetMaxRegularization () );

    for ( auto emi = M.begin (); emi != M.end(); emi++ )

        emi->Resize ( GetNumLines (), NumElectrodes );
//      emi->AResizeFast ( GetNumLines (), NumElectrodes );


    if ( getinputfile.GetFileFilterIndex () == 1 ) {
                                        // text file
        is  = new ifstream ( TFileName ( (const char *) getinputfile, TFilenameExtendedPath ) );
                                        // read in the values
        for ( int l  = 0; l  < GetNumLines (); l++  )
        for ( int el = 0; el < NumElectrodes;  el++ )

            M[ 0 ] ( l, el )    = StringToDouble ( GetToken ( is, buff ) );
        }
    else {
                                        // binary file
        is  = new ifstream ( TFileName ( (const char *) getinputfile, TFilenameExtendedPath ), ios::binary );

        float               f;
        double              d;

        for ( int s  = 0; s  < GetNumLines (); s++  )
        for ( int el = 0; el < NumElectrodes;  el++ ) {

            if ( infloat ) {
                is->read ( (char *) &f, sizeof ( f ) );
                M[ 0 ] ( s, el )     = f;
                }
            else {
                is->read ( (char *) &d, sizeof ( d ) );
                M[ 0 ] ( s, el )     = d;
                }
            }
        }

    delete  is;


    SetDocPath ( getoutputfile );

    Commit ( true );                    // force to write it NOW
    }


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
