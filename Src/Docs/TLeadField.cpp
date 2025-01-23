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

#include    "TLeadField.h"

#include    "TArray2.h"
#include    "Strings.Utils.h"
#include    "TExportTracks.h"

#include    "TRisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;
using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TLeadField::TLeadField ()
{
Reset ();
}


        TLeadField::TLeadField ( char *file )
{
Reset ();

Set ( file );
}


        TLeadField::~TLeadField ()
{
Reset ();
}


//----------------------------------------------------------------------------
void    TLeadField::Reset ()
{
TDataFormat::Reset ();

FilePath.Reset ();

Dim1                = 0;
Dim2                = 0;
}


//----------------------------------------------------------------------------
                                        // Check & open file, browse header & store dimensions
                                        // does NOT actually read the matrix itself
bool    TLeadField::Set ( char *file )
{
if ( ! ( StringIsNotEmpty ( file ) && IsExtensionAmong ( file, AllLeadFieldFilesExt ) ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for file corruption, sometimes, shit happens...

// or use CanOpenFile ?

size_t              filesize        = GetFileSize ( file /*, true*/ );

if ( filesize == 0 ) {
    Reset ();
    return  false;
    }


FilePath.Set ( file, TFilenameExtendedPath );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Lead Field is always of vectorial type
SetAtomType ( AtomTypeVector );
ContentType         = ContentTypeLeadField;
ExtraContentType    = 0;
//StringCopy ( ExtraContentTypeNames[ 0 ], "", ContentTypeMaxChars - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                risformat       = FilePath.IsExtension ( FILEEXT_RIS );
bool                lftformat       = FilePath.IsExtension ( FILEEXT_LFT );
bool                lfformat        = FilePath.IsExtension ( FILEEXT_LF  );
bool                csvformat       = FilePath.IsExtension ( FILEEXT_CSV );
//bool              binformat       = FilePath.IsExtension ( FILEEXT_BIN );
bool                FileBin         = ! csvformat;


ifstream            ifs ( FilePath, FileBin ? ios::binary | ios::in : ios::in );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get LF size from file
if      ( risformat ) {

    TRisHeader          risheader;

    ifs.read ( (char *) (&risheader), sizeof ( risheader ) );

    if ( ! IsMagicNumber ( risheader.Magic, RISBIN_MAGICNUMBER1 )
        || risheader.IsInverseScalar ) {

        Reset ();
        return  false;
        }

    Dim1                = risheader.NumTimeFrames;
    Dim2                = risheader.NumSolutionPoints;
    } //  FILEEXT_RIS


else if ( lftformat ) {

    int32               magicnumber;
    ifs.read ( (char *) &magicnumber, sizeof ( magicnumber ) );

//  if ( magicnumber != 1 ) {
//      Reset ();
//      return  false;
//      }

    int32               i32;

    ifs.read ( (char *) &i32, sizeof ( i32 ) );
    Dim1    = i32;
    ifs.read ( (char *) &i32, sizeof ( i32 ) );
    Dim2    = i32;

    int32               sourcedim;
    ifs.read ( (char *) &sourcedim, sizeof ( sourcedim ) );
                                        // we expect vectorial data
    if ( sourcedim != 3 ) {
        Reset ();
        return  false;
        }

    } //  FILEEXT_LFT


else if ( lfformat ) {
                                // read header
    int32               i32;

    ifs.read ( (char *) &i32, sizeof ( i32 ) );
    Dim1    = i32;
    ifs.read ( (char *) &i32, sizeof ( i32 ) );
    Dim2    = i32 / AtomSize ();
    } // FILEEXT_LF


else if ( csvformat ) {

    Dim1    = CountLines    ( FilePath );
    Dim2    = CountPerLine  ( FilePath ) / 3;   // multiplexed X,Y,Z

    if ( Dim1 <= 0 || Dim2 <= 0 ) {
        Reset ();
        return  false;
        }
    } // FILEEXT_CSV


/*else if ( binformat ) {   // not supported anymore
                                        // get the # of input solpoints
//    numsolp     = numleadfieldsolpoints;

                                        // 1) Detect if file format is float or double - working
    float               f;
    double              d;
    bool                isfloat;
    int                 typesize;
    TEasyStats          statsf;
    TEasyStats          statsd;

                                        // at least 100 SPs...
    for ( int i = 0; i < 100; i++ ) {

        ifs.seekg ( i * AtomSize () * sizeof ( f ), ios::beg );
        ifs.read  ( (char *) &f,  sizeof ( f ) );
        statsf.Add ( fabs ( f ) );

        ifs.seekg ( i * AtomSize () * sizeof ( d ), ios::beg );
        ifs.read  ( (char *) &d,  sizeof ( d ) );
        statsd.Add ( fabs ( d ) );
        }

//    statsf.Show ( "LeadField BIN type: float" );
//    statsd.Show ( "LeadField BIN type: double" );

                                        // the data type with the lowest variation is the winner
    isfloat     = statsf.CoefficientOfVariation () < statsd.CoefficientOfVariation ();
    typesize    = isfloat ? sizeof ( float ) : sizeof ( double );

                                        // 2) Guess the right combination of dimensions - !NOT working!
    int                 product             = filesize / ( AtomSize () * typesize );
    int                 guess2;
    char                buff[ 256 ];
    float               f3[ 3 ];
    double              d3[ 3 ];
//    double              delta[ 3 ];
    double              delta;


    for ( int guess1 = 16; guess1 <= 1024; guess1++ ) {
                                        // el should be an integral divider
        if ( product % guess1 != 0 )
            continue;

        guess2  = product / guess1;

//        DBGV3 ( product, guess1, guess2, "product, guess1, guess2" );


        statsf.Reset ();

        for ( int i = 0; i < guess2 - 1; i++ ) {

            ifs.seekg ( ( ( guess1 - 1 ) * guess2 + i ) * ( AtomSize () * typesize ), ios::beg );
            if ( isfloat )  ifs.read  ( (char *) &f3,  AtomSize () * sizeof ( *f3 ) );
            else            ifs.read  ( (char *) &d3,  AtomSize () * sizeof ( *d3 ) );

            delta   = isfloat ? max ( fabs ( f3[ 0 ] ), fabs ( f3[ 1 ] ), fabs ( f3[ 2 ] ) ) : max ( fabs ( d3[ 0 ] ), fabs ( d3[ 1 ] ), fabs ( d3[ 2 ] ) );
//            delta   = isfloat ? sqrt ( Square ( f3[ 0 ] ) + Square ( f3[ 1 ] ) + Square ( f3[ 2 ] ) ) : sqrt ( Square ( d3[ 0 ] ) + Square ( d3[ 1 ] ) + Square ( d3[ 2 ] ) );


            ifs.seekg ( ( i + 1 ) * ( AtomSize () * typesize ), ios::beg );
            if ( isfloat )  ifs.read  ( (char *) &f3,  AtomSize () * sizeof ( *f3 ) );
            else            ifs.read  ( (char *) &d3,  AtomSize () * sizeof ( *d3 ) );

            delta   = fabs ( delta - ( isfloat ? max ( fabs ( f3[ 0 ] ), fabs ( f3[ 1 ] ), fabs ( f3[ 2 ] ) ) : max ( fabs ( d3[ 0 ] ), fabs ( d3[ 1 ] ), fabs ( d3[ 2 ] ) ) ) );
//            delta   = fabs ( delta - ( isfloat ? sqrt ( Square ( f3[ 0 ] ) + Square ( f3[ 1 ] ) + Square ( f3[ 2 ] ) ) : sqrt ( Square ( d3[ 0 ] ) + Square ( d3[ 1 ] ) + Square ( d3[ 2 ] ) ) ) );

            statsf.Add ( delta );
            }

        sprintf ( buff, "guess1 = %0d, guess2 = %0d", guess1, guess2 );
        statsf.Show ( buff );

//    for ( int sp = 10; el <= 1024; el++ )
        }


//    Dim1    = Dim2    = filesize / ( AtomSize () * ( isfloat ? sizeof ( float ) : sizeof ( double ) ) * numsolp );

                                        // we can deduce numel
//    numel             = filesize / ( AtomSize () * ( isfloat ? sizeof ( float ) : sizeof ( double ) ) * numsolp );

                                // go back to file beginning, there isn't any header
    ifs.seekg ( 0, ios::beg );
    } // FILEEXT_BIN
*/


else {                                  // not a recognized file type
    Reset ();
    return  false;
    }


return  true;
}


//----------------------------------------------------------------------------
                                        // Allocate & retrieve the actual matrix
bool    TLeadField::ReadFile ( TArray2<float> &K )
{
                                        // to avoid duplicating reading the file & setting the matrix, allocate a temp Matrix type var
AMatrix             K2;

if ( ! ReadFile ( K2 ) )
    return  false;

                                        // then transfer
int                 numel           = K2.n_rows;
int                 numsolp3        = K2.n_cols;


K.Resize ( numel, numsolp3 );


for ( int el = 0; el < numel;    el++ )
for ( int sp = 0; sp < numsolp3; sp++ )

    K ( el, sp )    = K2 ( el, sp );

return  true;
}


//----------------------------------------------------------------------------
                                        // Allocate & retrieve the actual matrix
bool    TLeadField::ReadFile ( AMatrix& K )
{
if ( ! IsOpen () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate matrix
int                 numel           = GetNumElectrodes     ();
int                 numsolp         = GetNumSolutionPoints ();
int                 numsolp3        = 3 * numsolp;


K.AResizeFast ( numel, numsolp3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                risformat       = FilePath.IsExtension ( FILEEXT_RIS );
bool                lftformat       = FilePath.IsExtension ( FILEEXT_LFT );
bool                lfformat        = FilePath.IsExtension ( FILEEXT_LF  );
bool                csvformat       = FilePath.IsExtension ( FILEEXT_CSV );
//bool              binformat       = FilePath.IsExtension ( FILEEXT_BIN );
bool                FileBin         = ! csvformat;


ifstream            ifs ( FilePath, FileBin ? ios::binary | ios::in : ios::in );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( risformat ) {
                                        // skip header
    ifs.seekg ( sizeof ( TRisHeader ), ios::beg );

    float               v;
                                        // .ris is float, K is AReal (float or double)
    for ( int el  = 0; el  < numel;    el++  )
    for ( int sp3 = 0; sp3 < numsolp3; sp3++ ) {

        ifs.read ( (char *) &v, sizeof ( v ) );

        K ( el, sp3 )   = (AReal) v;
        }
    } // FILEEXT_RIS


else if ( lfformat ) {
                                        // skip header
    ifs.seekg ( 2 * sizeof ( int32 ), ios::beg );

    double              v;

    for ( int el  = 0; el  < numel;    el++  )
    for ( int sp3 = 0; sp3 < numsolp3; sp3++ ) {

        ifs.read ( (char *) &v, sizeof ( v ) );

        K ( el, sp3 )   = (AReal) v;
        }
    } // FILEEXT_LF


else if ( lftformat ) {
                                        // skip header
    ifs.seekg ( 4 * sizeof ( int32 ), ios::beg );

                                        // here it is always 3
    int                 sourcedim       = 3;

                                // format is compressed: first get the max per column (sol point) and per dimension
    TArray2<float>      maxsp ( numsolp, sourcedim );
                                // one row for x, y, z
    for ( int v3 = 0; v3 < sourcedim; v3++ )
    for ( int sp = 0; sp < numsolp;   sp++ )

        ifs.read ( (char *) &maxsp ( sp, v3 ), maxsp.AtomSize () );

                                // then read short ints & rescale
    int16               i16;

    for ( int el = 0; el < numel;     el++ )
    for ( int v3 = 0; v3 < sourcedim; v3++ )
    for ( int sp = 0; sp < numsolp;   sp++ ) {

        ifs.read ( (char *) &i16, sizeof ( i16 ) );

        K ( el, 3 * sp + v3 )   = (AReal) ( maxsp ( sp, v3 ) * ( (double) i16 / SHRT_MAX ) );
        }
    } // FILEEXT_LFT


else if ( csvformat ) {

    char        buff [ 256 ];

    for ( int el  = 0; el  < numel;    el++  )
    for ( int sp3 = 0; sp3 < numsolp3; sp3++ ) 
                                        // will handle space, tabs, newlines and comma separators
        K ( el, sp3 )   = (AReal) StringToFloat ( GetToken ( &ifs, buff ) );
    } // FILEEXT_CSV


else                                    // shouldn't happen!
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Utility method that doesn't need the object to be already open
bool    TLeadField::WriteFile   (   const AMatrix&      K,  const char*     file,   WriteLeadFieldOptions   option  )
{
                                                            // ! NOT AllLeadFieldFilesExt !
if ( ! ( StringIsNotEmpty ( file ) && IsExtensionAmong ( file, AllWriteLeadFieldFilesExt ) ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;


if      ( IsExtension ( file, FILEEXT_LF ) ) {
                                        // Options not relevant here

    ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

    int32               i32;

    i32     = numel;
    ofs.write ( (char *) &i32, sizeof ( i32 ) );
    i32     = numsolp3;
    ofs.write ( (char *) &i32, sizeof ( i32 ) );

    double              v;

    for ( int el  = 0; el  < numel;    el++  )
    for ( int sp3 = 0; sp3 < numsolp3; sp3++ ) {

        v       = K ( el, sp3 );

        ofs.write ( (char *) &v, sizeof ( v ) );
        }
    } // FILEEXT_LF

                                        // !NOT recommended because matrix ordering can differ!
else if ( IsExtension ( file, FILEEXT_BIN ) ) {
                                        // Options not relevant here

    ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
                                        // doesn't care if float or double
    ofs.write ( (char *) K.memptr (), sizeof ( AReal ) * K.size () );

    } // FILEEXT_BIN


else if ( IsExtension ( file, FILEEXT_RIS ) ) {
                                        // save the lead field as a ris, so that we can visualize  Electrode -> SPs
                                        // as the whole vectorial data are written, it can be used for re-loading a Lead Field, too
    TExportTracks       expkris;

    StringCopy ( expkris.Filename, file );


    if      ( option == ComponentsAutomatic ) {

        expkris.SetAtomType ( AtomTypeVector );
        expkris.NumTime         = numel;
        expkris.NumTracks       = numsolp;

        for ( int el = 0; el < numel;   el++ )
        for ( int sp = 0; sp < numsolp; sp++ )

            expkris.Write ( TVector3Float ( K ( el, 3 * sp     ),
                                            K ( el, 3 * sp + 1 ),
                                            K ( el, 3 * sp + 2 ) ) );
        }

    else if ( option == ComponentsNorm      ) {

        expkris.SetAtomType ( AtomTypeScalar );
        expkris.NumTime         = numel;
        expkris.NumTracks       = numsolp;

        for ( int el = 0; el < numel;   el++ )
        for ( int sp = 0; sp < numsolp; sp++ )

            expkris.Write ( (float) sqrt ( Square ( K ( el, 3 * sp     ) )  // scalar formula
                                         + Square ( K ( el, 3 * sp + 1 ) )
                                         + Square ( K ( el, 3 * sp + 2 ) ) ) );
        }

    else if ( option == ComponentsSplit      ) {
                                        // useful for investigations, but not practical use
        expkris.SetAtomType ( AtomTypeScalar );
        expkris.NumTime         = numel;
        expkris.NumTracks       = numsolp3;

        for ( int el = 0; el < numel;   el++ )
        for ( int sp3 = 0; sp3 < numsolp3; sp3++ )

            expkris.Write ( K ( el, sp3 ) );    // each component is scalar
        }

    } // FILEEXT_RIS


else if ( IsExtensionAmong ( file, FILEEXT_EEGEP " " FILEEXT_EEGSEF ) ) {
                                        // save the lead field as an ep, so we can visualize  SP -> Electrodes
    TExportTracks       expkep;

    StringCopy ( expkep.Filename, file );

    if      ( option == ComponentsAutomatic 
           || option == ComponentsNorm      ) {

        expkep.SetAtomType ( AtomTypeScalar );
        expkep.NumTracks        = numel;
        expkep.NumTime          = numsolp;

        for ( int sp = 0; sp < numsolp; sp++ )
        for ( int el = 0; el < numel;   el++ )

            expkep.Write ( (float) sqrt ( Square ( K ( el, 3 * sp     ) )
                                        + Square ( K ( el, 3 * sp + 1 ) )
                                        + Square ( K ( el, 3 * sp + 2 ) ) ) );
        }

    else if ( option == ComponentsSplit ) {

        expkep.SetAtomType ( AtomTypeScalar );
        expkep.NumTracks        = numel;
        expkep.NumTime          = numsolp3;

        for ( int sp3 = 0; sp3 < numsolp3; sp3++ )
        for ( int el = 0; el < numel;   el++ )

            expkep.Write ( K ( el, sp3 ) );
        }

    } // FILEEXT_EP FILEEXT_EEGSEF

                                        // matrix as text
else if ( IsExtension ( file, FILEEXT_TXT ) ) {

    ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ) );
                                        // rely on Armadillo to do that nicely
    ofs << K;
    }

else                                    // not a recognized file type
    return  false;


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
