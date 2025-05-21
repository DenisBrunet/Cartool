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

#include    "Math.Armadillo.h"          // AMatrix
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"

#include    "TInverseResults.h"         // InverseMaxElectrodeName

namespace crtl {

//----------------------------------------------------------------------------
                                        // Utilities for writing a matrix to file - kind of TExportInverse
inline void WriteInverseMatrixHeader    (   int                 numel,      int                 numsolp,    int             numreg,         bool                invscal,
                                            const TStrings&     xyznames,   const TStrings&     spnames,    const double*   regulvalues,    const TStrings*     regulnames,
                                            const char*         filename 
                                        );


inline void WriteInverseMatrixFile      (   const AMatrix&      M,          bool                invscal,    
                                            const TStrings&     xyznames,   const TStrings&     spnames,    const TSelection*   spsrejected,
                                            int                 numreg,     int                 regi,       const double*       regulvalues,    TStrings*       regulnames,
                                            const char*         filename 
                                        );


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

void    WriteInverseMatrixHeader    (   int                 numel,      int                 numsolp,    int             numreg,         bool                invscal,
                                        const TStrings&     xyznames,   const TStrings&     spnames,    const double*   regulvalues,    const TStrings*     regulnames,
                                        const char*         filename 
                                    )
{

ofstream            ofs ( TFileName ( filename, TFilenameExtendedPath ), ios::binary );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Header: fixed part
int                 magic           = ISBIN_MAGICNUMBER3;   // float, stack of matrices

ofs.write ( (char *) &magic,   sizeof ( magic   ) );

ofs.write ( (char *) &numel,   sizeof ( numel   ) );
ofs.write ( (char *) &numsolp, sizeof ( numsolp ) );
ofs.write ( (char *) &numreg,  sizeof ( numreg  ) );
ofs.write ( (char *) &invscal, sizeof ( invscal ) );    // bool is written as single char


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Header: variable parts
char                buff[ 256 ];
                                        // electrodes names
for ( int el = 0; el < numel; el++ ) {

    ClearString ( buff, InverseMaxElectrodeName );

    if ( el < (int) xyznames )      StringCopy ( buff, xyznames[ el ] );
    else                            StringCopy ( buff, "e", IntegerToString ( el + 1 ) );

    buff[ InverseMaxElectrodeName - 1 ]  = 0;

    ofs.write ( buff, InverseMaxElectrodeName );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sp names
for ( int sp = 0; sp < numsolp; sp++ ) {

    ClearString ( buff, InverseMaxSolutionPointName );

    if ( sp < (int) spnames )       StringCopy ( buff, spnames[ sp ] );
    else                            StringCopy ( buff, "sp", IntegerToString ( sp + 1 ) );

    buff[ InverseMaxSolutionPointName - 1 ]  = 0;

    ofs.write ( buff, InverseMaxSolutionPointName );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // regularization values
for ( int reg = 0; reg < numreg; reg++ ) {

    double      regv        = regulvalues[ reg ];

    ofs.write ( (char *) &regv, sizeof ( regv ) );
    }

                                        // regularization names
for ( int reg = 0; reg < numreg; reg++ ) {

    ClearString ( buff, InverseMaxRegularizationName );

    if ( regulnames && reg < regulnames->NumStrings () && StringIsNotEmpty ( (*regulnames)[ reg ] ) )
        StringCopy ( buff, (*regulnames)[ reg ] );
    else
        StringCopy ( buff, InfixRegularization " ", IntegerToString ( reg ) );

    buff[ InverseMaxRegularizationName - 1 ]  = 0;

    ofs.write ( buff, InverseMaxRegularizationName );
    }
}


//----------------------------------------------------------------------------
                                        // Create / append a given matrix to an .is file
void    WriteInverseMatrixFile  (   const AMatrix&          M,          bool                invscal,    
                                    const TStrings&         xyznames,   const TStrings&     spnames,    const TSelection*   spsrejected,
                                    int                     numreg,     int                 regi,       const double*       regulvalues,    TStrings*       regulnames,
                                    const char*             filename 
                                )
{
bool                writeheader     = ! numreg || numreg && regi == 0;

int                 numel           = M.n_cols;

int                 dimsp           = invscal ? 1 : 3;
int                 numrows         = M.n_rows;
int                 numsolp         = ( numrows / dimsp )
                                    + ( spsrejected ? spsrejected->NumSet () : 0 ); // adding optional rejected points to get back to the original count
                    numrows         = numsolp * dimsp;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           safepath ( filename, TFilenameExtendedPath );

if ( writeheader )

    WriteInverseMatrixHeader    (   numel,      numsolp,    numreg,         invscal, 
                                    xyznames,   spnames,    regulvalues,    regulnames, 
                                    safepath 
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // append matrix body, while converting to float type
ofstream            ofs ( safepath, ios::binary | ios::app );
float               v;
int                 sp3in           = 0;
bool                spok;

                                        // to position after the header
//ofs.seekp ( 17 + numel * InverseMaxElectrodeName + numsolp * InverseMaxSolutionPointName + numreg * InverseMaxRegularizationName, ios::beg );

                                        // !loop includes the X, Y then Z components!
for ( int sp3out = 0; sp3out < numrows; sp3out++ ) {

    spok        = spsrejected == 0 || spsrejected->IsNotSelected ( sp3out / dimsp );

    for ( int el = 0; el < numel; el++ ) {

        v       = spok ? M ( sp3in, el ) : 0;

        ofs.write ( (char *) &v, sizeof ( v ) );
        }

    if ( spok )     sp3in++;
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
