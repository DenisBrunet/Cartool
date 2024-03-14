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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-
                                        // For LAPACK
#define     LAPACK_COMPLEX_STRUCTURE
                                        // skipping FORTRAN string length optional parameters
#define     __EMSCRIPTEN__

#include    "mkl_lapacke.h"

#include    "Math.Utils.h"
#include    "Math.Armadillo.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Special case: symmetric definite matrices
                                        // providing D should be faster
void        AEigenvaluesArma    ( const ASymmetricMatrix& M, AVector& D )
{
                                        // ascending signed values order
arma::eig_sym ( D, M );
}


//----------------------------------------------------------------------------
void        AEigenvaluesEigenvectorsArma    ( const ASymmetricMatrix& M, AVector& D, AMatrix& V )
{
                                        // ascending signed values order
arma::eig_sym ( D, V, M, "std" /*"dc"*/ );
}


//----------------------------------------------------------------------------
                                        // Special case: symmetric 3x3 definite positive matrices
void        AEigenvalues33Arma  ( const AMatrix33& M, AVector3& D )
{
                                        // ascending signed values order
arma::eig_sym ( D, M );
}

/*                                      // OpenBLAS
#include    <lapacke_config.h>
#include    <lapack.h>
                                        // Faster local variables (see arma::eig_sym)
                                        // !M will be overwritten!
void        AEigenvalues33OpenBLAS ( AMatrix33& M, AVector3& D )
{
                                        // The total amount of local variables is 5x16 bytes
lapack_int          N               = 3;
lapack_int          info            = 0;
lapack_int          buffersize      = 17;
AReal               buffer[ 17 ];

    
LAPACK_ssyev_base   (   "N",            "U",    // eigenvalues only, lower triangle results
                        &N,
//                      A.memptr (),    &N,     // triangular results in A
                        M.memptr (),    &N,     // !overwriting results on input!
                        D.memptr(), 
                        buffer,         &buffersize, 
                        &info 
                    );
}
*/

void        AEigenvalues33MKL ( AMatrix33& M, AVector3& D )
{
//lapack_int    info    =
LAPACKE_ssyev   (   LAPACK_COL_MAJOR,
                    'N',            'U',    // eigenvalues only, upper triangle matrix
                    3,
                    M.memptr (),
                    3,                      // lda, Leading Dimension A = step to jump to the next column, >= N
                    D.memptr()
                );
}
                                        // Analytic solution - Tested
                                        // Ref: O. Smith, "Eigenvalues of a Symmetric 3x3 Matrix", Comm. ACM 1961
                                        // ascending signed values order
void        AEigenvalues33Smith  ( const AMatrix33& M, AVector3& D )
{
double              m               = arma::trace ( M ) / 3;

AMatrix33           B               ( M - m * AMatrixIdentity ( 3 ) );

double              q               = arma::det ( B ) / 2;
                                        // take benefit from the symmetry
double              p               = (         Square ( B ( 0, 0 ) ) + Square ( B ( 1, 1 ) ) + Square ( B ( 2, 2 ) ) 
                                        + 2 * ( Square ( B ( 0, 1 ) ) + Square ( B ( 0, 2 ) ) + Square ( B ( 1, 2 ) ) ) 
                                      ) / 6;
                                        // we need to study a bit this factor
double              p3q2            = AtLeast ( 0.0, Cube ( p ) - Square ( q ) );

                                        // prevent problems from arctangent
if ( p3q2 < DoubleFloatEpsilon && q < DoubleFloatEpsilon ) {
                                        // according to article, if p = q = 0
    D ( 0 )     = 
    D ( 1 )     = 
    D ( 2 )     = m;

    return;
    }
                                        // here parameters should be OK
double              phi             = atan2 ( sqrt ( p3q2 ), q ) / 3;

                    p               = sqrt ( p );
double              cosphi          = cos ( phi );
double              sinphi          = sin ( phi ); // sqrt ( 1 - cosphi * cosphi ); // phi is in 0..Pi/2 range, so corresponding sine is positive
                                        // ascending signed values order
D ( 0 )     = m -     p * ( cosphi + SqrtThree * sinphi );
D ( 1 )     = m -     p * ( cosphi - SqrtThree * sinphi );
D ( 2 )     = m + 2 * p *   cosphi;
}

                                        // Analytic solution - Tested
                                        // Ref: Deledalle, Denis, Tabti, Tupin, "Closed-form expressions of the eigen decomposition of 2x2 and 3x3 Hermitian matrices", HAL open science
                                        // ascending signed values order
void        AEigenvalues33DDTT  ( const AMatrix33& M, AVector3& D )
{
const AReal&        a               = M ( 0, 0 );
const AReal&        b               = M ( 1, 1 );
const AReal&        c               = M ( 2, 2 );
const AReal&        d               = M ( 1, 0 );
const AReal&        e               = M ( 2, 1 );
const AReal&        f               = M ( 2, 0 );

                                        // already diagonal?
if ( d == 0 && e == 0 && f == 0 ) {
                                        // !ordering is not assessed!
    D ( 0 )         = a;
    D ( 1 )         = b;
    D ( 2 )         = c;
    return;
    }


double              x1              = a * a + b * b + c * c
                                    - a * ( b + c ) - b * c
                                    + 3 * ( d * d + e * e + f * f );

double              abc             = 2 * a - b - c;
double              bac             = 2 * b - a - c;
double              cab             = 2 * c - a - b;

double              x2              = - abc * bac * cab
                                      +  9 * ( cab * d * d + bac * f * f + abc * e * e )
                                      - 54 * d * e * f;

double              phi             = x2 ?  atan2 ( sqrt ( AtLeast ( 0.0, 4 * x1 * x1 * x1 - x2 * x2 ) ), x2 )
                                    :       HalfPi;

double              tr              = a + b + c;
                    x1              = sqrt ( AtLeast ( 0.0,  x1 ) );
                                        // !ordering differs from paper!
                    D ( 0 )         = ( tr - 2 * x1 * cos (   phi        / 3 ) ) / 3;
                    D ( 2 )         = ( tr + 2 * x1 * cos ( ( phi - Pi ) / 3 ) ) / 3;
                    D ( 1 )         = ( tr + 2 * x1 * cos ( ( phi + Pi ) / 3 ) ) / 3;
}


//----------------------------------------------------------------------------
void        AEigenvaluesEigenvectors33Arma  ( const AMatrix33& M, AVector3& D, AMatrix33& V )
{
                                        // ascending signed values order
                                        // skipping the Divide & Conquer method which is helpful only for big matrices
arma::eig_sym ( D, V, M, "std" );
}

                                        // Analytic solution - !Eigenvalues are 100% OK, but Eigenvectors can sometimes differ from Armadillo!
void        AEigenvaluesEigenvectors33DDTT ( const AMatrix33& M, AVector3& D, AMatrix33& V )
{
                                        // Eigenvalues first..
AEigenvalues33DDTT  ( M, D );

                                        // ..then Eigenvectors
//const AReal&      a               = M ( 0, 0 );   // this guy is not invited to the party...
const AReal&        b               = M ( 1, 1 );
const AReal&        c               = M ( 2, 2 );
const AReal&        d               = M ( 1, 0 );
const AReal&        e               = M ( 2, 1 );
const AReal&        f               = M ( 2, 0 );

                                        // already diagonal?
if ( d == 0 && e == 0 && f == 0 ) {
                                        // !ordering is not assessed!
    V   = AMatrixIdentity ( 3 );
    return;
    }

                                        // as suggested in the article
//if ( f == 0 )       f               = SingleFloatEpsilon; // in that case, value need to be copied into a temp
//
//                                      // original formula
//auto                ToEigenvector   = [ &b, &c, &d, &e, &f ] ( const AReal& l, auto& v )
//{
//double              m               = ( d * ( c - l ) - e * f ) / NonNull ( f * ( b - l ) - d * e );
//                    v[ 0 ]          = ( l - c - e * m ) / f;
//                    v[ 1 ]          = m;
//                    v[ 2 ]          = 1;
//
//v   = arma::normalise ( v );
//};

                                        // equivalent formula without division errors
auto                ToEigenvector   = [ &b, &c, &d, &e, &f ] ( const AReal& l, auto& v )
{
                                        // splitting m into numerator and denominator parts  m = mn / md
double              mn              = d * ( c - l ) - e * f;
double              md              = f * ( b - l ) - d * e;
                                        // propagating *f and *md to other terms gives
                    v[ 0 ]          = ( l - c ) * md - e * mn;
                    v[ 1 ]          = mn * f;
                    v[ 2 ]          = md * f;

v   = arma::normalise ( v );
};

                                        // !eigenvalues oerdering differs from original paper BUT eigenvectors formulas are all symmetrical so we don't really care at the end of the day!
ToEigenvector   ( D ( 0 ), V.col ( 0 ) );
ToEigenvector   ( D ( 1 ), V.col ( 1 ) );
ToEigenvector   ( D ( 2 ), V.col ( 2 ) );
}


//----------------------------------------------------------------------------

void        ASymmetrize (    ASymmetricMatrix&   M,      bool    filluppertriangular     )
{
int                 dim             = M.n_rows;

if ( dim == 0 || ! M.is_square () )
    return;


if ( filluppertriangular )
    for ( int i  = 1; i  < dim; i++  )
    for ( int j  = 0; j  < i;   j++  )

        M ( j, i )  = M ( i, j );

else // lower tringular
    for ( int i  = 1; i  < dim; i++  )
    for ( int j  = 0; j  < i;   j++  )

        M ( i, j )  = M ( j, i );
}


//----------------------------------------------------------------------------
                                        // Centering matrix
ASymmetricMatrix    ACenteringMatrix ( int     dim )
{
return  AMatrixIdentity ( dim ) - AMatrixOnes ( dim, dim ) / (double) dim;
}


//----------------------------------------------------------------------------

ASymmetricMatrix    APseudoInverseSymmetric (   ASymmetricMatrix    K   )
{
                                        // Armadillo provided function
return  pinv ( K );

/*
                                        // Diagonalisation
                                        // K = V * D * V.t()
AVector             D;
AMatrix             V;
                                        // ascending signed values order
AEigenvaluesEigenvectorsArma    ( K, D, V );

                                        // tolerance formula, as from Matlab pinv
double              tolerance       = GetMachineEpsilon<AReal> () * K.n_rows * D.max ();
                                        // Invert diagonal matrix, for non-zero / close to zero elements
                                        // There shouldn't be any negative eigenvalues at that point - If there are some, they will be cancelled
for ( int i = 0; i < D.n_rows; i++ )
    D ( i ) = D ( i ) > tolerance ? 1 / D ( i ) : 0;

                                        // reconstruct matrix
return  V * arma::diagmat ( D ) * V.t ();
*/
}


//----------------------------------------------------------------------------
                                        // Matrix which multiplied by itself gives inverse of Matrix
AMatrix33           AMatrixInverseSquareRoot (   const AMatrix33&    m33  )
{
                                        // eigenvector decomposition
AVector3            D;
AMatrix33           V;

AEigenvaluesEigenvectors33Arma ( m33, D, V );

                                        // we can invert AND sqrt the diagonal (eigenvalues), doing both at the same time (note that the eigenvalues should be all positive, and not too small either)
D[ 0 ]      = D[ 0 ] > 0 ? 1 / sqrt ( D[ 0 ] ) : 0;
D[ 1 ]      = D[ 1 ] > 0 ? 1 / sqrt ( D[ 1 ] ) : 0;
D[ 2 ]      = D[ 2 ] > 0 ? 1 / sqrt ( D[ 2 ] ) : 0;

                                        // building back our 3x3 matrix, with inverse of square root
return  V * arma::diagmat ( D ) * V.t ();
}

                                        // Matrix which multiplied by itself gives Matrix
AMatrix33           AMatrixSquareRoot        (   const AMatrix33&    m33  )
{
                                        // eigenvector decomposition
AVector3            D;
AMatrix33           V;

AEigenvaluesEigenvectors33Arma ( m33, D, V );

                                        // sqrt the diagonal (eigenvalues)
D[ 0 ]      = D[ 0 ] > 0 ? sqrt ( D[ 0 ] ) : 0;
D[ 1 ]      = D[ 1 ] > 0 ? sqrt ( D[ 1 ] ) : 0;
D[ 2 ]      = D[ 2 ] > 0 ? sqrt ( D[ 2 ] ) : 0;

                                        // building back our 3x3 matrix
return  V * arma::diagmat ( D ) * V.t ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TArmaLUSolver::Reset ()
{
L.ARelease ();
U.ARelease ();
P.ARelease ();

Permutation.resize ( 0 );
}


void    TArmaLUSolver::Set ( const ASymmetricMatrix& A )
{
                                        // not really needed, as all fields will be overwritten anyway
//Reset ();


lu ( L, U, P, A );


int                 asize           = A.n_rows;

Permutation.resize ( asize );

for ( int c = 0; c < asize; c++ )
for ( int r = 0; r < asize; r++ )

    if ( P ( r, c ) != 0 ) {
                                        // translates column to row
        Permutation[ c ]   = r;

        break;
        }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
