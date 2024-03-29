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

#include    "armadillo"                 // Armadillo is a header-only library - It is NOT NEEDED NOR RECOMMENDED to use vcpkg as it will forcefully install OpenBlas, lapack etc... libraries which we DO NOT NEED NOR WANT

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // For faster processing, Armadillo strongly recommends to set these, f.ex. from the compiler's flags:
#if !defined (ARMA_USE_LAPACK)
#define     ARMA_USE_LAPACK
#endif

#if !defined (ARMA_USE_BLAS)
#define     ARMA_USE_BLAS
#endif

#if !defined (ARMA_BLAS_NOEXCEPT)
#define     ARMA_BLAS_NOEXCEPT
#endif
                                        // !Critical for MKL to work properly!
#if !defined (ARMA_BLAS_LONG_LONG)
#define     ARMA_BLAS_LONG_LONG
#endif

#if !defined (ARMA_LAPACK_NOEXCEPT)
#define     ARMA_LAPACK_NOEXCEPT
#endif

#if !defined (ARMA_DONT_USE_FORTRAN_HIDDEN_ARGS)
#define     ARMA_DONT_USE_FORTRAN_HIDDEN_ARGS
#endif

#if !defined (ARMA_DONT_CHECK_NONFINITE)
#define     ARMA_DONT_CHECK_NONFINITE
#endif

#if !defined (ARMA_DONT_USE_OPENMP)
#define     ARMA_DONT_USE_OPENMP
#endif
                                        // For sparse matrices, Armadillo strongly recommend to set this, f.ex. in the compiler flags:
#if !defined (ARMA_USE_NEWARP)
#define     ARMA_USE_NEWARP
#endif
                                        // For release version only:
#if !defined (_DEBUG) && !defined (ARMA_NO_DEBUG)
#define     ARMA_NO_DEBUG
#endif
                                        // We assume and rely that all created objects are indeed initialized to 0
#undef      ARMA_DONT_ZERO_INIT


//----------------------------------------------------------------------------
                                        // Choosing a general type for Cartool matrices
using   AReal       = float;
using   AComplex    = std::complex<AReal>;


//----------------------------------------------------------------------------
                                        // Common matrices - Constructor will gracefully init them all to 0
using   AMatrix                 = arma::Mat<AReal>;     // dynamic matrix
using   ASymmetricMatrix        = AMatrix;              // no specialized symmetric matrix
using   ADiagonalMatrix         = AMatrix;              // no specialized diagonal matrix
using   AVector                 = arma::Col<AReal>;     // in geometrical notation
using   AComplexVector          = arma::Col<AComplex>;             


using   ASparseMatrix           = arma::SpMat<AReal>;   // dynamic sparse matrix
using   ASparseSymmetricMatrix  = ASparseMatrix;        // no specialized symmetric matrix

                                // 3x3 matrices                                    
using   AMatrix33               = arma::Mat<AReal>::fixed<3,3>;
using   ADiagonalMatrix33       = AMatrix33;            // no specialized diagonal matrix
using   AVector3                = arma::Col<AReal>::fixed<3>;

                                        // These ones used defines:
#define                                 AMatrixIdentity(D)              ASymmetricMatrix    ( D, D, arma::fill::eye )
#define                                 AMatrixOnes(R,C)                AMatrix             ( R, C, arma::fill::ones )
#define                                 AMatrixZero(R,C)                AMatrix             ( R, C, arma::fill::zeros )
//#define                               ADiagonalMatrixZero(D)          ADiagonalMatrix     ( D, D, arma::fill::zeros )
#define                                 AVectorFromMemory(V,TOMEM,N)    AVector     V       ( (TOMEM), (const arma::uword) (N), false, true )
#define                                 AVectorFromTArray1(V,A)         AVectorFromMemory(V,(A).GetArray (),(A).GetDim())


//----------------------------------------------------------------------------
                                        // All libraries diverge on the meaning of basic functions - lets make clear what is what:

                                        // Resetting to 0
#define AReset                          zeros
                                        // Releasing memory
#define ARelease                        reset
                                        // Resize & resetting memory
#define AResizeZero                     zeros
                                        // Resize WITHOUT resetting memory
#define AResizeFast                     set_size
                                        // Resize AND keeping current content
#define AResizeContent                  resize


//----------------------------------------------------------------------------
                                        // For dense symmetric/hermitian matrix
                                        // ascending signed values order
void                AEigenvaluesArma                ( const ASymmetricMatrix& M, AVector& D );          // Wrapper around Armadillo

void                AEigenvaluesEigenvectorsArma    ( const ASymmetricMatrix& M, AVector& D, AMatrix& V );  // Wrapper around Armadillo

void                AEigenvalues33Arma              ( const AMatrix33& M, AVector3& D );                // Wrapper around Armadillo
//void              AEigenvalues33OpenBLAS          (       AMatrix33& M, AVector3& D );                // Faster wrapper around OpenBLAS - !M will be overwritten!
void                AEigenvalues33MKL               (       AMatrix33& M, AVector3& D );                // Wrapper on MKL, a bit faster than Armadillo - !M will be overwritten!
void                AEigenvalues33Smith             ( const AMatrix33& M, AVector3& D );                // Even faster
void                AEigenvalues33DDTT              ( const AMatrix33& M, AVector3& D );                // Fastest one

void                AEigenvaluesEigenvectors33Arma  ( const AMatrix33& M, AVector3& D, AMatrix33& V );  // Wrapper around Armadillo
void                AEigenvaluesEigenvectors33DDTT  ( const AMatrix33& M, AVector3& D, AMatrix33& V );  // Fastest one - vectors can exceptionally differ from Eigen/Armadillo


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These matrices are not actually symmetrical internally, so we have this little utility to mirro one triangular part of the matrix from the other one
void                ASymmetrize                 (   ASymmetricMatrix&   M,      bool    filluppertriangular     );

                                        // Centering matrix
ASymmetricMatrix    ACenteringMatrix            (   int                 dim );

                                        // Operations that need some Eigenvalues/vectors decomposition
ASymmetricMatrix    APseudoInverseSymmetric     (   ASymmetricMatrix    K   );  // !copy is intended, so caller can directly use an expression!
AMatrix33           AMatrixInverseSquareRoot    (   const AMatrix33&    m33 );
AMatrix33           AMatrixSquareRoot           (   const AMatrix33&    m33 );


//----------------------------------------------------------------------------
                                        // Thread-safe class, so only 1 object will be needed even in parallel code
                                        // !no copy nor assignation implemented for the moment!
class   TArmaLUSolver
{
public:
                    TArmaLUSolver ()                                                    {   Reset ();   }
                    TArmaLUSolver ( const ASymmetricMatrix& A )                         {   Set ( A );  }

    AMatrix         L;
    AMatrix         U;
    AMatrix         P;
    std::vector<int>Permutation;            // LUT equivalent to matrix P

    void            Reset                   ();
    void            Set                     ( const ASymmetricMatrix& A );

    inline int      ApplyP                  ( int col )                         const   { return Permutation [ col ]; } // translate a column index to a row index
    inline void     Solve                   ( AVector& X, const AVector& B )    const   { X =    arma::solve ( trimatu ( U ), arma::solve ( trimatl ( L ), P * B, arma::solve_opts::fast ), arma::solve_opts::fast ); }
    inline AVector  Solve                   (             const AVector& B )    const   { return arma::solve ( trimatu ( U ), arma::solve ( trimatl ( L ), P * B, arma::solve_opts::fast ), arma::solve_opts::fast ); }
    inline AMatrix  Solve                   (             const AMatrix& B )    const   { return arma::solve ( trimatu ( U ), arma::solve ( trimatl ( L ), P * B, arma::solve_opts::fast ), arma::solve_opts::fast ); }
    inline void     SolveWithoutPermutation ( AVector& X, const AVector& B )    const   { X =    arma::solve ( trimatu ( U ), arma::solve ( trimatl ( L ),     B, arma::solve_opts::fast ), arma::solve_opts::fast ); }
};


//----------------------------------------------------------------------------

}
