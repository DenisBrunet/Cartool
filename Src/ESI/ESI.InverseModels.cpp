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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "ESI.InverseModels.h"

#include    "TArray1.h"
#include    "TVector.h"
#include    "Files.WriteInverseMatrix.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TExportTracks.h"
#include    "TSelection.h"
#include    "Geometry.TPoints.h"
#include    "GlobalOptimize.Points.h"

#include    "ESI.SolutionPoints.h"
#include    "ESI.LeadFields.h"

#include    "TInverseMatrixDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOff

void    ComputeInverseCenter    (   
                                InverseCenterType           how,
                                const TFitModelOnPoints*    surfacemodel,
                                const TPoints*              headpoints,
                                const TPoints*              solpoints,
                                const TPoints*              xyzpoints,
                                TPointFloat&                centertranslate,
                                bool                        showprogress
                                )
{
centertranslate.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the current MRI center, i.e. doing nothing !just for test, as it doesn't guarantee anything at all!
if      ( how == InverseCenterMri )

    centertranslate = 0.0;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the Head model center, which is based on the big head model
else if ( how == InverseCenterHeadModel )

    centertranslate = surfacemodel->GetTranslation ();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the simplified Head model center
else if ( how == InverseCenterBfeHead ) {


    TPoints                 headpoints2 ( *headpoints );

                                        // keep a shape like top of head + on the back, avoiding any clipping (guillotine)
//  headpoints2.KeepTopHeadPoints   ( *MriFull, &mriabstoguillotine );
                                        // less points is good
    headpoints2.DownsamplePoints    ( /*HeadModelNumPoints*/ 707 );


    TFitModelOnPoints   gohead      ( headpoints2 );

    const TBoundingBox<double>& bounding    = gohead.Bounding;

                                        // Compute an approximate head model with 6 parameters
    gohead.Set ( GOStepsDefault );

                                        // Best Fitting Ellipsoid - Better, an ellipsoid is a good approximation of the brain
                                        // scalings
    gohead.AddGroup ();
    gohead.AddDim   ( ScaleX, bounding.GetRadius ( 0 ) * 0.75, bounding.GetRadius ( 0 ) * 1.25 );
    gohead.AddDim   ( ScaleY, bounding.GetRadius ( 1 ) * 0.75, bounding.GetRadius ( 1 ) * 1.25 );
    gohead.AddDim   ( ScaleZ, bounding.GetRadius ( 2 ) * 0.75, bounding.GetRadius ( 2 ) * 1.25 );

                                        // rotation
    gohead.AddGroup ();
    gohead.AddDim   ( RotationX, -15, 15 );

                                        // translations - off if points have been forcefully recentered
    gohead.AddGroup ();
  //gohead.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.05, bounding.GetRadius ( 0 ) * 0.05 );
    gohead.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.05, bounding.GetRadius ( 1 ) * 0.05 );
    gohead.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.05, bounding.GetRadius ( 2 ) * 0.05 );


    gohead.GetSolution  (   FitModelPointsMethod,   FitModelPointsHow, 
                            GODefaultConvergence,   0, 
                            showprogress ? "Inverse Center (BFE Head)" : 0 
                        );


#if defined(InvMatrixMoreOutputs)
    TFileName               _file;
    StringCopy  ( _file, CrisTransfer.BaseFileName, "\\BFE of Head.xyz" );
    gohead.WriteFile ( _file );
#endif


    centertranslate = gohead.GetTranslation ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the center of BFS of Electrodes - logical, but highly dependent on the shape of the xyz (neck...)
else if ( how == InverseCenterBfsXyz ) {

    TFitModelOnPoints   goxyz ( *xyzpoints );

    const TBoundingBox<double>& bounding    = goxyz.Bounding;

    goxyz.Set ( GOStepsDefault );

                                        // 4 parameters model
                                        // scaling
    goxyz.AddGroup ();
    goxyz.AddDim   ( Scale, bounding.Radius () * 0.75, bounding.Radius () * 1.25 );
                                        // translations
    goxyz.AddGroup ();
//  goxyz.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.10, bounding.GetRadius ( 0 ) * 0.10 );
    goxyz.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.10, bounding.GetRadius ( 1 ) * 0.10 );
    goxyz.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.10, bounding.GetRadius ( 2 ) * 0.10 );


    goxyz.GetSolution   (   FitModelPointsMethod,   FitModelPointsHow, 
                            GODefaultConvergence,   0, 
                            showprogress ? "Inverse Center (BFS XYZ)" : 0 
                        );


#if defined(InvMatrixMoreOutputs)
    TFileName           _file;
    StringCopy  ( _file, CrisTransfer.BaseFileName, "\\BFS of XYZ.xyz" );
    goxyz.WriteFile ( _file );
#endif

                                        // extract translation part from this model
    centertranslate = goxyz.GetTranslation ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the center of BFE of Electrodes
else if ( how == InverseCenterBfeXyz ) {
                                        // center as center of BFE of XYZ
    TFitModelOnPoints   goxyz ( *xyzpoints );

    const TBoundingBox<double>& bounding    = goxyz.Bounding;

    goxyz.Set ( GOStepsDefault );

                                        // 7 parameters model
                                        // scaling
    goxyz.AddGroup ();
    goxyz.AddDim   ( ScaleX, bounding.GetRadius ( 0 ) * 0.75, bounding.GetRadius ( 0 ) * 1.25 );
    goxyz.AddDim   ( ScaleY, bounding.GetRadius ( 1 ) * 0.75, bounding.GetRadius ( 1 ) * 1.25 );
    goxyz.AddDim   ( ScaleZ, bounding.GetRadius ( 2 ) * 0.75, bounding.GetRadius ( 2 ) * 1.25 );
                                            // rotations
    goxyz.AddGroup ();
    goxyz.AddDim   ( RotationX, -15, 15 );
//  goxyz.AddDim   ( RotationY, -15, 15 );
//  goxyz.AddDim   ( RotationZ, -15, 15 );
                                        // translations
    goxyz.AddGroup ();
//  goxyz.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.10, bounding.GetRadius ( 0 ) * 0.10 );
    goxyz.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.10, bounding.GetRadius ( 1 ) * 0.10 );
    goxyz.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.10, bounding.GetRadius ( 2 ) * 0.10 );

//                                        // For Potatoidal match, one can add:
//                                        // pinch along the X axis on the Y and Z axis
//    goxyz.AddGroup ();
//    goxyz.AddDim   ( SinusPinchYtoX, 0.00, -0.10 );
//    goxyz.AddDim   ( SinusPinchYtoZ, 0.00, -0.10 );
//                                        // flattening front, back, left-right, up-down
//    goxyz.AddGroup ();
//    goxyz.AddDim   ( FlattenYPos, 0.00, 0.50 );
//    goxyz.AddDim   ( FlattenYNeg, 0.00, 0.50 );
//
//    goxyz.AddGroup ();
//    goxyz.AddDim   ( FlattenZPos, 0.00, 0.50 );
//    goxyz.AddDim   ( FlattenX,    0.00, 0.50 );


    goxyz.GetSolution   (   FitModelPointsMethod,   FitModelPointsHow, 
                            GODefaultConvergence,   0, 
                            showprogress ? "Inverse Center (BFE XYZ)" : 0 
                        );


#if defined(InvMatrixMoreOutputs)
    TFileName           _file;
    StringCopy  ( _file, CrisTransfer.BaseFileName, "\\BFE of XYZ.xyz" );
    goxyz.WriteFile ( _file );
#endif

                                        // extract translation part from this model
    centertranslate = goxyz.GetTranslation ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the center of BFS of SPs
else if ( how == InverseCenterBfsSp ) {

    TFitModelOnPoints   gosp ( *solpoints );

    const TBoundingBox<double>& bounding    = gosp.Bounding;

    gosp.Set ( GOStepsDefault );

                                    // 4 parameters model
                                    // scaling
    gosp.AddGroup ();
    gosp.AddDim   ( Scale, bounding.Radius () * 0.75, bounding.Radius () * 1.25 );
                                    // translations
    gosp.AddGroup ();
//  gosp.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.10, bounding.GetRadius ( 0 ) * 0.10 );
    gosp.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.10, bounding.GetRadius ( 1 ) * 0.10 );
    gosp.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.10, bounding.GetRadius ( 2 ) * 0.10 );


    gosp.GetSolution    (   BFSContainModelPointsMethod,    BFSContainModelPointsHow, 
                            GODefaultConvergence,           0, 
                            showprogress ? "Inverse Center (BFS SP)" : 0 
                        );


#if defined(InvMatrixMoreOutputs)
    TFileName           _file;
    StringCopy  ( _file, CrisTransfer.BaseFileName, "\\BFS of SP.xyz" );
    gosp.WriteFile ( _file );
#endif

                                        // extract translation part from this model
    centertranslate = gosp.GetTranslation ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the center of BFE of SPs
else if ( how == InverseCenterBfeSp ) {

    TFitModelOnPoints   gosp ( *solpoints );

    const TBoundingBox<double>& bounding    = gosp.Bounding;

    gosp.Set ( GOStepsDefault );

                                        // 7 parameters model
                                        // scaling
    gosp.AddGroup ();
    gosp.AddDim   ( ScaleX, bounding.GetRadius ( 0 ) * 0.75, bounding.GetRadius ( 0 ) * 1.25 );
    gosp.AddDim   ( ScaleY, bounding.GetRadius ( 1 ) * 0.75, bounding.GetRadius ( 1 ) * 1.25 );
    gosp.AddDim   ( ScaleZ, bounding.GetRadius ( 2 ) * 0.75, bounding.GetRadius ( 2 ) * 1.25 );
                                        // rotations
    gosp.AddGroup ();
    gosp.AddDim   ( RotationX, -15, 15 );
//  gosp.AddDim   ( RotationY, -15, 15 );
//  gosp.AddDim   ( RotationZ, -15, 15 );
                                        // translations
    gosp.AddGroup ();
//  gosp.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.10, bounding.GetRadius ( 0 ) * 0.10 );
    gosp.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.10, bounding.GetRadius ( 1 ) * 0.10 );
    gosp.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.10, bounding.GetRadius ( 2 ) * 0.10 );

//                                        // For Potatoidal match, one can add:
//                                        // pinch along the X axis on the Y and Z axis
//    gosp.AddGroup ();
//    gosp.AddDim   ( SinusPinchYtoX, 0.00, -0.10 );
//    gosp.AddDim   ( SinusPinchYtoZ, 0.00, -0.10 );
//                                        // flattening front, back, left-right, up-down
//    gosp.AddGroup ();
//    gosp.AddDim   ( FlattenYPos, 0.00, 0.50 );
//    gosp.AddDim   ( FlattenYNeg, 0.00, 0.50 );
//
//    gosp.AddGroup ();
//    gosp.AddDim   ( FlattenZPos, 0.00, 0.50 );
//    gosp.AddDim   ( FlattenX,    0.00, 0.50 );


    gosp.GetSolution    (   BFEContainModelPointsMethod,    BFEContainModelPointsHow, 
                            GODefaultConvergence,           0, 
                            showprogress ? "Inverse Center (BFE SP)" : 0
                        );


#if defined(InvMatrixMoreOutputs)
    TFileName               _file;
    StringCopy  ( _file, CrisTransfer.BaseFileName, "\\BFE of SP.xyz" );
    gosp.WriteFile ( _file );
#endif

                                        // extract translation part from this model
    centertranslate = gosp.GetTranslation ();
    }

}

OptimizeOn

//----------------------------------------------------------------------------
                                        // Merging different estimates
                                        // To go from original space to optimal centered space: Points + Translation / Center - Translation
                                        // To go from optimal centered space to original space: Points - Translation / Center + Translation
TPointFloat ComputeOptimalInverseTranslation (
                                const TPoints*              headpoints,
                                const TPoints*              solpoints,
                                const TPoints*              xyzpoints,
                                bool                        showprogress
                                )
{
                                        // Using multiple estimates for safety
TPoints             centertranslates;
TPointFloat         centertranslate;

                                        // Most of these processing are already parallelized, so it does not really make sense to use parallel sections here
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // at that point, this one doesn't exist yet
//  ComputeInverseCenter    (   InverseCenterHeadModel,
//                              &SurfaceModel,
//                              0,
//                              0,
//                              0,
//                              centertranslate,
//                              showprogress
//                          );
//
//  centertranslates.Add ( centertranslate );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( headpoints ) {

    ComputeInverseCenter    (   InverseCenterBfeHead,
                                0,
                                headpoints,
                                0,
                                0,
                                centertranslate,
                                showprogress
                            );
  
    centertranslates.Add ( centertranslate );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( xyzpoints ) {

    ComputeInverseCenter    (   InverseCenterBfsXyz,
                                0,
                                0,
                                0,
                                xyzpoints,
                                centertranslate,
                                showprogress
                            );

    centertranslates.Add ( centertranslate );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // low amount of SP doesn't give such a nice estimate
if ( solpoints && solpoints->GetNumPoints () > 2000 ) {

    ComputeInverseCenter    (   InverseCenterBfsSp,
                                0,
                                0,
                                solpoints,
                                0,
                                centertranslate,
                                showprogress
                            );

    centertranslates.Add ( centertranslate );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // get a robust estimate
centertranslate = centertranslates.GetMedianPoint ( false );


//    centertranslates    .Show ( "all centertranslates" );
//    centertranslate     .Show ( "centertranslate" );
//    GetValueFromUser    ( "New centertranslate.Z:", "", centertranslate.Z, "", this );

return  centertranslate;
}


//----------------------------------------------------------------------------
                                        // ratio is the fraction of the biggest eigenvalue used as steps of regularization factors
                                        // case Real == float
void    ComputeRegularizationFactors (
                                const ASymmetricMatrix&     KKt,            double          eigenvaluedownfactor, 
                                TVector<double>&            regulvalues,    TStrings&       regulnames 
                                )
{
AVector             D;
                                        // get eigenvalues of matrix, which will give us the magnitude of the values
AEigenvaluesArma ( KKt, D );
                                        // biggest eigenvalue
double              biggesteigen    = D.max ();


//TFileName           _file;
//sprintf ( _file, "%s\\KKt Eigenvalues Symmetric.txt", CrisTransfer.BaseFileName );
//ofstream    ofs ( _file );
//ofs << StreamFormatFloat32 << D;

                                        // Safety measure - at that point it wouldn't really matter anyway...
if ( IsNotAProperNumber ( biggesteigen ) )  biggesteigen = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate the # of regularization, including one without any
regulvalues.Resize ( NumSavedRegularizations );
regulnames .Set    ( NumSavedRegularizations, InverseMaxRegularizationName );


for ( int reg = 0; reg < regulvalues.GetDim1 (); reg++ ) {

    regulvalues[ reg ]  = reg * ( biggesteigen / eigenvaluedownfactor );

    StringCopy  ( regulnames[ reg ], InfixRegularization " ", IntegerToString ( reg ) );

//    DBGV ( regulvalues[ reg ], regulnames[ reg ] );
    }
}


//----------------------------------------------------------------------------
void    ComputeResolutionMatrix (   
                                const AMatrix&      K,              const TPoints&      solpoints,
                                const char*         fileinverse,
                                RegularizationType  regularization,
                                char*               fileresmat,     char*               fileresmats,    char*               fileresmatt
                                )
{
int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;

bool                isresmat        = StringIsNotEmpty ( fileresmat  );
bool                isresmats       = StringIsNotEmpty ( fileresmats );
bool                isresmatt       = StringIsNotEmpty ( fileresmatt );


if ( ! (    numel > 0 && numsolp > 0
         && StringIsNotEmpty ( fileinverse ) 
         && ( isresmat || isresmats || isresmatt ) 
         && ! solpoints.IsEmpty ()                  ) )
    return;



TOpenDoc<TInverseMatrixDoc> isdoc ( fileinverse, OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         gauge;

gauge.Set ( "Resolution Matrix" );

gauge.AddPart ( 0,          2 * numsolp,    95 );
gauge.AddPart ( 1,          3,               5 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracks<float>          resmat ( isresmat || isresmatt  ? numsolp : 0, isresmat || isresmatt    ? numsolp : 0 );    // big matrix, showing the Point Spread Function
TTracks<float>          resmats( isresmats              ? numsolp : 0, isresmats                ? 1       : 0 );    // smaller one, summarizing the spreading per each SP
//TTracks<float>        resmatt( isresmatt              ? numsolp : 0, isresmatt                ? numsolp : 0 );    // big matrix transposed, showing the Kernels

OmpParallelBegin

TArray1<TVector3Float>  invx   ( numsolp );
TArray1<TVector3Float>  invy   ( numsolp );
TArray1<TVector3Float>  invz   ( numsolp );
TVector<float>          inv    ( numsolp );

OmpFor

for ( int spi = 0; spi < numsolp; spi++ ) {

    gauge.Next ( 0 );

                                // we need to multiply the 3 components of each solution point
    isdoc->MultiplyMatrix ( regularization, K, 3 * spi    , invx );
    isdoc->MultiplyMatrix ( regularization, K, 3 * spi + 1, invy );
    isdoc->MultiplyMatrix ( regularization, K, 3 * spi + 2, invz );

                                // gather all x, y, and z results
    for ( int spi2 = 0; spi2 < numsolp; spi2++ )

        inv[ spi2 ] = sqrt ( invx[ spi2 ].Norm2 () + invy[ spi2 ].Norm2 () + invz[ spi2 ].Norm2 () );

                                // store the whole columns / row in the big matrix
    if ( isresmat || isresmatt )
        inv.SetColumn ( resmat,  spi );


//  if ( isresmatt )
//      inv.SetRow    ( resmatt, spi );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                // sum-up all weighted radii
    gauge.Next ( 0 );

    if ( isresmats ) {

        double      sumwr       = 0;
        double      sumw        = 0;

        for ( int spi2 = 0; spi2 < numsolp; spi2++ ) {

                sumwr  += Square ( inv[ spi2 ] * ( solpoints[ spi2 ] - solpoints[ spi ] ).Norm () );
                sumw   += Square ( inv[ spi2 ] );
                }

        resmats ( spi, 0 )  = sqrt ( sumwr / NonNull ( sumw ) );
        } // if resmats

                                    // !to see where is the actual SP!
    if ( isresmat || isresmatt )
        resmat ( spi, spi ) = -1;

    } // dor spi

OmpParallelEnd

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( 1 );

if ( isresmat )
    resmat.WriteFile ( fileresmat );


gauge.Next ( 1 );

if ( isresmats )
    resmats.WriteFile ( fileresmats );


gauge.Next ( 1 );

if ( isresmatt ) {

    resmat.Transpose ();

    resmat.WriteFile ( fileresmatt );
    }
}


//----------------------------------------------------------------------------
                                        // ComputeInverseLoreta optimization creates weird crashes - let's turn this off for all
OptimizeOff

//----------------------------------------------------------------------------
                                        // Minimum Norm (aka Inverso Stupido)
void    ComputeInverseMN        ( 
                                const AMatrix&      Kin,            TSelection              spsrejected, 
                                bool                regularization, const TStrings&         xyznames, 
                                const TStrings&     spnamesin,
                                const char*         filename 
                                )
{
enum                {
                    gaugemnglobal,
                    gaugemnpseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set     ( InfixInverseMN PostfixInverseTitle );

gauge.AddPart ( gaugemnglobal,         2,                                                      02 );
gauge.AddPart ( gaugemnpseudoinverse,  regularization ? NumSavedRegularizations * 1 + 1 : 1,   98 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
gauge.Next ( gaugemnglobal );


AMatrix             K ( Kin );


RejectPointsFromLeadField ( K, spsrejected );


//DBGV5 ( Kin.Nrows (), Kin.Ncols () / 3, (int) spsrejected, K.Nrows (), K.Ncols () / 3, "Kin #rejected -> K" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = K.n_rows;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugemnglobal );


AMatrix             Kt              = K.t ();
ASymmetricMatrix    KKt             = K * Kt;

K.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AMatrix             J;


if ( regularization ) {

    gauge.Next ( gaugemnpseudoinverse );


    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;


    ComputeRegularizationFactors ( KKt, EigenvalueToRegularizationFactorMN, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // centering matrix
    ASymmetricMatrix    H           = ACenteringMatrix ( numel );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    for ( int reg = 0; reg < numreg; reg++ ) {

        gauge.Next ( gaugemnpseudoinverse );

        J               = Kt * APseudoInverseSymmetric ( KKt + regulvalues[ reg ] * H );

        WriteInverseMatrixFile  (   J,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg
    } // if regularization

else { // no regularization

    gauge.Next ( gaugemnpseudoinverse );

    J               = Kt * APseudoInverseSymmetric ( KKt );

    WriteInverseMatrixFile  (   J,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inversemn


//----------------------------------------------------------------------------
                                        // Weighted Minimum Norm (aka Weighted Inverso Stupido)
void    ComputeInverseWMN       (
                                const AMatrix&      Kin,            TSelection              spsrejected, 
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin,
                                const char*         filename
                                )
{
enum                {
                    gaugewmnglobal,
                    gaugewmnpseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set     ( InfixInverseWMN PostfixInverseTitle );

gauge.AddPart ( gaugewmnglobal,         3,                                                      03 );
gauge.AddPart ( gaugewmnpseudoinverse,  regularization ? NumSavedRegularizations * 1 + 1 : 1,   97 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
gauge.Next ( gaugewmnglobal );


AMatrix             K ( Kin );


RejectPointsFromLeadField ( K, spsrejected );


//DBGV5 ( Kin.Nrows (), Kin.Ncols () / 3, (int) spsrejected, K.Nrows (), K.Ncols () / 3, "Kin #rejected -> K" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute weights minimum norm
gauge.Next ( gaugewmnglobal );


/*                                      // Compute inverse of square weights
ADiagonalMatrix     W1inv ( numsolp3 );

OmpParallelFor

for ( int sp = 0; sp < numsolp3; sp++ ) {

    double          sumk        = 0;

    for ( int el = 0; el < numel; el++ )
        sumk   += Square ( K ( el, sp ) );

    sumk        = sqrt ( sumk ) / numel;

    W1inv ( sp ,sp )  = sumk ? 1 / sumk : 0;
    }
*/

ADiagonalMatrix     W1inv ( numsolp3, numsolp3 );

                                        // Laura way
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumkx       = 0;
    double          sumky       = 0;
    double          sumkz       = 0;
    double          sumk;

    for ( int el = 0; el < numel; el++ ) {
        sumkx  += Square ( K ( el, 3 * sp     ) );
        sumky  += Square ( K ( el, 3 * sp + 1 ) );
        sumkz  += Square ( K ( el, 3 * sp + 2 ) );
        }

    sumkx       = sqrt ( sumkx ) / numel;
    sumky       = sqrt ( sumky ) / numel;
    sumkz       = sqrt ( sumkz ) / numel;
    sumk        = ( sumkx + sumky + sumkz ) / 3;

    W1inv ( 3 * sp    , 3 * sp     ) =
    W1inv ( 3 * sp + 1, 3 * sp + 1 ) =
    W1inv ( 3 * sp + 2, 3 * sp + 2 ) = sumk ? 1 / sumk : 0;
    }

/*                                        // Cartool way
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumk        = 0;

    for ( int el = 0; el < numel; el++ )

        sumk   += Square ( K ( el, 3 * sp     ) ) 
               +  Square ( K ( el, 3 * sp + 1 ) ) 
               +  Square ( K ( el, 3 * sp + 2 ) );

    sumk        = sqrt ( sumk / ( 3 * numel ) );

    W1inv ( 3 * sp    , 3 * sp     ) =
    W1inv ( 3 * sp + 1, 3 * sp + 1 ) =
    W1inv ( 3 * sp + 2, 3 * sp + 2 ) = sumk ? 1 / sumk : 0;
    }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transpose K & apply weights
gauge.Next ( gaugewmnglobal );


AMatrix             W2Kt            = W1inv * K.t ();

ASymmetricMatrix    KW2Kt           = K * W2Kt;


K    .ARelease ();
W1inv.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AMatrix             J;


if ( regularization ) {

    gauge.Next ( gaugewmnpseudoinverse );


    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;


    ComputeRegularizationFactors ( KW2Kt, EigenvalueToRegularizationFactorWMN, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // centering matrix
    ASymmetricMatrix    H           = ACenteringMatrix ( numel );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    for ( int reg = 0; reg < numreg; reg++ ) {

        gauge.Next ( gaugewmnpseudoinverse );

        J               = W2Kt * APseudoInverseSymmetric ( KW2Kt + regulvalues[ reg ] * H );

        WriteInverseMatrixFile  (   J,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg
    } // if regularization

else { // no regularization

    gauge.Next ( gaugewmnpseudoinverse );
                                        // Weighted Minimum Norm Moore-Penrose pseudo inverse
    J               = W2Kt * APseudoInverseSymmetric ( KW2Kt );

    WriteInverseMatrixFile  (   J,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inversewmn


//----------------------------------------------------------------------------

void    ComputeInverseLoreta    (   
                                const AMatrix&      Kin,            TPoints&                solpointsin,        TSelection              spsrejected,
                                NeighborhoodType    neighborhood,   NeighborhoodReduction   neighbors26to18,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename,
                                TVerboseFile*       verbose
                                )
{
enum                {
                    gaugeloretaglobal     = 0,
                    gaugeloretakronecker,
                    gaugeloretapseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set     ( InfixInverseLoreta PostfixInverseTitle );

gauge.AddPart ( gaugeloretaglobal,          9,                                                      33 );
gauge.AddPart ( gaugeloretakronecker,       3 * 3,                                                  33 );
gauge.AddPart ( gaugeloretapseudoinverse,   regularization ? NumSavedRegularizations * 2 + 1 : 2,   34 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
AMatrix             K ( Kin );

                                        // duplicate original full list
TPoints             solpoints ( solpointsin );
TStrings            spnames   ( spnamesin   );

                                        // add points not suitable for LORETA to spsrejected
RejectSingleNeighbors ( solpoints, neighborhood, spsrejected );

                                        // force modify our local list
RejectPointsFromList ( solpoints, spnames, spsrejected );


RejectPointsFromLeadField ( K, spsrejected );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeloretaglobal );

int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;

TPoints             points ( solpoints );   // duplicate list to array, for faster access
double              step            = points.GetMedianDistance ();
int                 maxneighbors    =        Neighborhood[ neighborhood ].NumNeighbors;
double              neighborradius  = step * Neighborhood[ neighborhood ].MidDistanceCut;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scan for # of neighbors
gauge.Next ( gaugeloretaglobal );

//if ( neighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 ) ) {

TVector<int>        NumNeighbors;

GetNumGreyNeighbors ( points, neighborradius, NumNeighbors );

//NumNeighbors.WriteFile ( "E:\\Data\\NumNeighbors.Loreta26.1.sef", "NumNeighbors" );


TVector<int>        numnn ( /*maxneighbors*/ Neighborhood[ Neighbors26 ].NumNeighbors + 1 );
TFileName           buff;


//numnn   = 0;
//
//for ( int i = 0; i < numsolp; i++ )
//    numnn[ NumNeighbors[ i ] ]++;
//
//numnn.WriteFile ( "E:\\Data\\NumNeighbors.Loreta26.1.Histo.txt", "NumNeighbors" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Fill Laplacian matrix
                                        // Official formula + limit # of neighbors to near 18
gauge.Next ( gaugeloretaglobal );

TVector<int>        NumNeighbors2 ( numsolp );  // do a new count for neighborhood
double              neighborradius6     = step * Neighborhood[ Neighbors6  ].MidDistanceCut;
double              neighborradius18    = step * Neighborhood[ Neighbors18 ].MidDistanceCut;
double              neighborradius26    = step * Neighborhood[ Neighbors26 ].MidDistanceCut;
bool                postpone26          = neighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 );


ASymmetricMatrix    A1              = AMatrixZero ( numsolp, numsolp );
                                        // Laplacian with equal weights
double              wlapl6          = neighborhood == Neighbors6 ? 1.0 / 6 : neighborhood == Neighbors18 ? 1.0 / 18 : neighborhood == Neighbors26 ?  1.0 / 26 : 0;
double              wlapl18         =                                        neighborhood == Neighbors18 ? 1.0 / 18 : neighborhood == Neighbors26 ?  1.0 / 26 : 0;
double              wlapl26         =                                                                                 neighborhood == Neighbors26 ?  1.0 / 26 : 0;
                                        // Laplacian with non-equal weights - results don't look good at all(?)
//double            wlapl6          = neighborhood == Neighbors6 ? 1.0 / 6 : neighborhood == Neighbors18 ? 2.0 / 24 : neighborhood == Neighbors26 ?  4.0 / 56 : 0;
//double            wlapl18         =                                        neighborhood == Neighbors18 ? 1.0 / 24 : neighborhood == Neighbors26 ?  2.0 / 56 : 0;
//double            wlapl26         =                                                                                 neighborhood == Neighbors26 ?  1.0 / 56 : 0;

                                        // first round to add neighbors, within a first limit
                                        // NOT parallelized because of potential side-effects
for ( int i = 0; i < numsolp; i++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
    if ( NumNeighbors2 ( i ) >= maxneighbors )
        continue;

                                        // scan strict upper triangle part
    for ( int j = i + 1; j < numsolp; j++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
        if ( NumNeighbors2 ( j ) >= maxneighbors )
            continue;


        double          d               = ( points[ j ] - points[ i ] ).Norm ();

        bool            isn6            = d > 0                && d <= neighborradius6;
        bool            isn18           = d > neighborradius6  && d <= neighborradius18;
        bool            isn26           = d > neighborradius18 && d <= neighborradius26;

                                     // exception: cube corners handled later
        if ( d <= neighborradius && ! ( postpone26 && isn26 ) ) {

                                        // weight each contribution according to location
            if      ( neighborhood == Neighbors6  )     A1 ( i, j ) = A1 ( j, i )   =          wlapl6                              ;
            else if ( neighborhood == Neighbors18 )     A1 ( i, j ) = A1 ( j, i )   = ( isn6 ? wlapl6 :         wlapl18 )          ;
            else if ( neighborhood == Neighbors26 )     A1 ( i, j ) = A1 ( j, i )   = ( isn6 ? wlapl6 : isn18 ? wlapl18 : wlapl26 );


            NumNeighbors2 ( i )++;
            NumNeighbors2 ( j )++;
            } // if radius OK
        } // for j
    } // for i

                                        // optional second pass: if less than 18 neighbors, go 26 neighbors by adding the 8 corners of the cube
if ( postpone26 ) {
                                        // take a snapshot of initially already full (18 neighbors) points, so to avoid adding to reasonably full SP
    TVector<bool>       neighfull18 ( numsolp );

    for ( int i = 0; i < numsolp; i++ )
        neighfull18 ( i )   = NumNeighbors[ i ] >= Neighborhood[ Neighbors18 ].NumNeighbors;

                                        // NOT parallelized because of potential side-effects
    for ( int i = 0; i < numsolp; i++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
        if ( NumNeighbors2 ( i ) >= maxneighbors )
            continue;

                                        // if already enough neighbors, don't try adding more
        if ( neighfull18 ( i ) )
            continue;

                                        // scan complete matrix here, as the current point (i) might connect to another one (j) which is already full
        for ( int j = 0; j < numsolp; j++ ) {

            if ( j == i )   continue;

                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
            if ( NumNeighbors2 ( j ) >= maxneighbors )
                continue;

                                        // strict: if potential neighbor is full, don't upgrade current point even if detrimental
            if ( IsNeighbors26To18Strict ( neighbors26to18 ) && neighfull18 ( j ) )
                continue;


            double              d               = ( points[ j ] - points[ i ] ).Norm ();

            bool                isn26           = d > neighborradius18 && d <= neighborradius26;

                                        // between 18 and 26 radius, and not already added?
            if ( isn26 && A1 ( i, j ) == 0 ) {

                A1 ( i, j ) = A1 ( j, i )   = wlapl26;

                                    // this allows to still add a few neighbors to points that have been bypassed before (>18)
                NumNeighbors2 ( i )++;
                NumNeighbors2 ( j )++;
                } // if radius OK
            } // for j
        } // for i
    } // if neighbors26to18

                                        // done, this is our new real neighborhood
NumNeighbors    = NumNeighbors2;

//NumNeighbors.WriteFile ( "E:\\Data\\NumNeighbors.Loreta26.2.sef", "NumNeighbors" );

//#if defined(InvMatrixMoreOutputs)
//TExportTracks     expneigh;
//StringCopy          ( expneigh.Filename, "E:\\Data\\Neighborhood Matrix.Loreta.A1.sef" );
//CheckNoOverwrite    ( expneigh.Filename );
//expneigh.SetAtomType ( AtomTypeScalar );
//expneigh.NumTracks        = numsolp;
//expneigh.NumTime          = numsolp;
//
//for ( int i = 0; i < numsolp; i++ )
//for ( int j = 0; j < numsolp; j++ )
//    expneigh.Write ( A1 ( i, j ) );
//#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check # of neighbors
numnn   = 0;

for ( int i = 0; i < numsolp; i++ )
    numnn[ NumNeighbors[ i ] ]++;

//numnn.WriteFile ( "E:\\Data\\NumNeighbors.Loreta26.2.Histo.txt", "NumNeighbors" );

                                        // save connection stats to verbose
if ( verbose != 0 ) {

    verbose->NextBlock ();
    verbose->NextTopic ( "LORETA Solution Points Connection:" );

                                            // title
    int                 fieldwidth      = 18;

    (ofstream&) *verbose << StreamFormatGeneral;
    (ofstream&) *verbose << StreamFormatText << setw ( fieldwidth ) << "# of Connections";
    (ofstream&) *verbose << StreamFormatText << setw ( fieldwidth ) << "# of SPs";
    (ofstream&) *verbose << "\n";

    (ofstream&) *verbose << "------------------";
    (ofstream&) *verbose << "------------------";
    (ofstream&) *verbose << "\n";

                                            // data
    for ( int sp = 0; sp < (int) numnn; sp++ ) {

        (ofstream&) *verbose << StreamFormatFixed;

        (ofstream&) *verbose << setw ( fieldwidth ) << setprecision ( 0 ) << sp;
        (ofstream&) *verbose << setw ( fieldwidth ) << setprecision ( 0 ) << numnn[ sp ];
        (ofstream&) *verbose << "\n";
        }


    //for ( int i = 0; i < (int) numnn; i++ ) {
    //    sprintf ( buff, "%0d neighbors -> %0d Solution Points", i, numnn[ i ] );
    //    ShowMessage ( buff, "LORETA Inverse Solution", ShowMessageWarning, this );
    //    }


    if ( numnn[ 0 ] > 0 ) {
                                            // This shouldn't happen, just in case, we reset the Lead Field at these positions

        sprintf ( buff, "%0d Solution Points have no neighbors, therefore the Inverse matrix will be reset for these points.", numnn[ 0 ] );
    //    ShowMessage ( buff, "LORETA Inverse Solution", ShowMessageWarning, this );

                                            // save this to verbose!
        verbose->NextLine ( 1 );
        verbose->NextTopic ( "LORETA Inverse Solution Warning:" );
        (ofstream&) *verbose << buff << "\n";

        verbose->NextLine ();
        for ( int sp = 0; sp < numsolp; sp++ )
            if ( ! NumNeighbors[ sp ] ) {

                for ( int el = 0; el < numel; el++ ) {
                    K ( el, 3 * sp     )    =
                    K ( el, 3 * sp + 1 )    =
                    K ( el, 3 * sp + 2 )    = 0;
                    } // for el

                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].X << "\t";
                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].Y << "\t";
                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].Z << "\t";
                (ofstream&) *verbose << StreamFormatText    << spnames  [ sp ];
                (ofstream&) *verbose << "\n";
                }
        verbose->NextLine ();
        }

    verbose->Flush ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Normalize Laplacian B
gauge.Next ( gaugeloretaglobal );

ADiagonalMatrix     DiagA1inv ( numsolp, numsolp );


OmpParallelFor

for ( int i = 0; i < numsolp; i++ ) {
                                        // sum of all weigths from neighbors
    for ( int j = 0; j < numsolp; j++ )

        DiagA1inv ( i, i )  += A1 ( i, j );
                                        // inverse of sum of weights, for normalization
    DiagA1inv ( i, i )  = DiagA1inv ( i, i ) ? 1 / DiagA1inv ( i, i ) : 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeloretaglobal );
                                        // formula from "Review of methods for solving the EEG inverse problem" - 1999
                                        // seems to be working
ASymmetricMatrix    A0              = 0.5 * ( AMatrixIdentity ( numsolp ) + ASymmetricMatrix ( DiagA1inv ) ) * A1;

                                        // formula from Roberto's website "Reply to Comments Made by R. Grave De Peralta Menendez and S.I. Gonzalez Andino. International Journal of Bioelectromagnetism 1999"
                                        // does not seem to work properly
//ASymmetricMatrix    A0              = ASymmetricMatrix ( DiagA1inv ) * A1;

DiagA1inv.ARelease ();
A1.       ARelease ();


gauge.Next ( gaugeloretaglobal );

ASymmetricMatrix    B               = ( A0 - AMatrixIdentity ( numsolp ) ); // / Power( d, 2 ), but distance is implicitely normalized here
//ASymmetricMatrix  B               = A1 - AMatrixIdentity ( numsolp );     // no reweighting - looks good, less smooth and more like Laura

A0.       ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute weights minimum norm
gauge.Next ( gaugeloretaglobal );

ADiagonalMatrix     W1 ( numsolp, numsolp );
                                        // Laura way
                                        // This is the version used for a long time
                                        // Without Z-Score, it performs better than the Cartool way
                                        // With Z-Score, 26to18 neighbors is equivalent as the Cartool way
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumkx       = 0;
    double          sumky       = 0;
    double          sumkz       = 0;
    double          sumk;

    for ( int el = 0; el < numel; el++ ) {
        sumkx  += Square ( K ( el, 3 * sp     ) );
        sumky  += Square ( K ( el, 3 * sp + 1 ) );
        sumkz  += Square ( K ( el, 3 * sp + 2 ) );
        }

    sumkx       = sqrt ( sumkx ) / numel;
    sumky       = sqrt ( sumky ) / numel;
    sumkz       = sqrt ( sumkz ) / numel;
    sumk        = ( sumkx + sumky + sumkz ) / 3;

    W1 ( sp, sp ) = sumk;
    }

/*                                        // Cartool way - no change without Z-Score, a bit better with Z-Score in the distance measure
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumk        = 0;

    for ( int el = 0; el < numel; el++ )

        sumk   += Square ( K ( el, 3 * sp     ) ) 
               +  Square ( K ( el, 3 * sp + 1 ) ) 
               +  Square ( K ( el, 3 * sp + 2 ) );

    sumk        = sqrt ( sumk / ( 3 * numel ) );

    W1inv ( sp, sp ) = sumk ? 1 / sumk : 0;
    }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Kronecker done before W1
AMatrix             W1BtBW1Kt;
AMatrix             KW1BtBW1Kt;
AMatrix             J;

gauge.Next ( gaugeloretaglobal );

W1BtBW1Kt       = ( W1inv * KP ( BtBinv, IdentityMatrix ( 3 ) ) * W1inv ) * K.t ();

gauge.Next ( gaugeloretaglobal );

KW1BtBW1Kt      = K * W1BtBW1Kt;

gauge.Next ( gaugeloretaglobal );

J               = W1BtBW1Kt * PseudoInverse ( KW1BtBW1Kt );

WriteInverseMatrixFile  (   J,          false, 
                            xyznames,   spnamesin,  0, 
                            0,          0,          0,              0, 
                            filename 
                        );

return;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeloretaglobal );

ASymmetricMatrix    W1BtBW1         = W1 * B.t () * B * W1;

B. ARelease ();
W1.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // To avoid the immondeous Kronecker inflation
                                        // we split the problem by dimension, then reconstruct the expected result
AMatrix             W1BtBW1Kt  ( numsolp3, numel );

TArmaLUSolver       W1BtBW1_LU ( W1BtBW1 );

OmpParallelFor

for ( int dim = 0; dim < 3; dim++ ) {

    gauge.Next ( gaugeloretakronecker );
                                        // get sub-Lead Field
    AMatrix             Ktd        ( numsolp,  numel );

    for ( int sp = 0; sp < numsolp; sp++ )
    for ( int el = 0; el < numel; el++ )
        Ktd ( sp, el )  = K ( el, 3 * sp + dim );

                                        // intermediate station, breath...
    gauge.Next ( gaugeloretakronecker );

    AMatrix             W1BtBW1Ktd      = W1BtBW1_LU.Solve ( Ktd );

                                        // inject current dimension into the full matrix, simulating the Kronecker
    gauge.Next ( gaugeloretakronecker );

    for ( int sp = 0; sp < numsolp; sp++ )
    for ( int el = 0; el < numel; el++ )
        W1BtBW1Kt ( 3 * sp + dim, el )  = W1BtBW1Ktd ( sp, el );
    } // for dim


W1BtBW1.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeloretaglobal );

ASymmetricMatrix    KW1BtBW1Kt      = K * W1BtBW1Kt;

AMatrix             J;


if ( regularization ) {

    gauge.Next ( gaugeloretapseudoinverse );

    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;


    ComputeRegularizationFactors ( KW1BtBW1Kt, EigenvalueToRegularizationFactorLORETA, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // centering matrix
    ASymmetricMatrix    H           = ACenteringMatrix ( numel );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    for ( int reg = 0; reg < numreg; reg++ ) {

        gauge.Next ( gaugeloretapseudoinverse );

        J               = W1BtBW1Kt * APseudoInverseSymmetric ( KW1BtBW1Kt + regulvalues[ reg ] * H );


        gauge.Next ( gaugeloretapseudoinverse );

        WriteInverseMatrixFile  (   J,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg

    } // if regularization

else { // no regularization
    gauge.Next ( gaugeloretapseudoinverse );

    J               = W1BtBW1Kt * APseudoInverseSymmetric ( KW1BtBW1Kt );


    gauge.Next ( gaugeloretapseudoinverse );

    WriteInverseMatrixFile  (   J,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inverseloreta


//----------------------------------------------------------------------------
void    ComputeInverseSLoreta   (   
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                )
{
enum                {
                    gaugesloretaglobal,
                    gaugesloretapseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set ( InfixInverseSLoreta PostfixInverseTitle );

gauge.AddPart ( gaugesloretaglobal,         4,                                                      04 );
gauge.AddPart ( gaugesloretapseudoinverse,  regularization ? NumSavedRegularizations * 4 + 1 : 4,   96 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
gauge.Next ( gaugesloretaglobal );


AMatrix             K ( Kin );

                                        // duplicate original full list
TStrings            spnames   ( spnamesin   );


RejectPointsFromLeadField ( K, spsrejected );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugesloretaglobal );

                                        // centering matrix
ASymmetricMatrix    H           = ACenteringMatrix ( numel );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need these guys a few times
gauge.Next ( gaugesloretaglobal );

AMatrix             Kt              = K.t ();


gauge.Next ( gaugesloretaglobal );

ASymmetricMatrix    KKt             = K * Kt;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AMatrix             T;
AMatrix             Sj;


if ( regularization ) {

    gauge.Next ( gaugesloretapseudoinverse );

    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;
                                        // sLORETA is the smoothest of all possible MN inverse. As a consequence, its norm
                                        // decreases much faster than the other matrices, looking super-smooth / deep very quickly.
                                        // The problem is that it makes the optimal L-corner search more difficult, so we
                                        // need to "zoom in" the regularization factors to spread a bit the searched curve.
                                        // The current 2.5 factor was estimated by comparing the reg curve with standard LORETA.
    ComputeRegularizationFactors ( KKt, EigenvalueToRegularizationFactorSLORETA, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    for ( int reg = 0; reg < numreg; reg++ ) {

        gauge.Next ( gaugesloretapseudoinverse );
                                        // simple minimum norm inverse matrix
        T               = Kt * APseudoInverseSymmetric ( KKt + regulvalues[ reg ] * H );


        gauge.Next ( gaugesloretapseudoinverse );
                                        // Estimated variance, = Resolution Matrix
                                        // we could extract 3 rows and 3 cols from T and K to have Sj33 directly
        Sj              = T * K;


        gauge.Next ( gaugesloretapseudoinverse );

        OmpParallelBegin

        AMatrix33       Sj33inv;

        OmpFor

        for ( int sp3 = 0; sp3 < numsolp3; sp3 += 3 ) {

                                        // extract (reference to) 3x3 diagonal part
            const AMatrix33&    Sj33    = Sj.submat ( sp3, sp3, sp3 + 2, sp3 + 2 );

                                        // to standardize with the standard deviation, we need the inverse square root of a matrix
            Sj33inv     = AMatrixInverseSquareRoot ( Sj33 );

                                        // rescale submatrix of 3 rows for current solution point
            T.rows ( sp3, sp3 + 2 )     = Sj33inv * T.rows ( sp3, sp3 + 2 );
            } // for sp

        OmpParallelEnd

        gauge.Next ( gaugesloretapseudoinverse );

        WriteInverseMatrixFile  (   T,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg

    } // if regularization

else { // no regularization
    gauge.Next ( gaugesloretapseudoinverse );

    T               = Kt * APseudoInverseSymmetric ( KKt );


    gauge.Next ( gaugesloretapseudoinverse );
                                        // Estimated variance, = Resolution Matrix
                                        // we could extract 3 rows and 3 cols from T and K to have Sj33 directly
    Sj              = T * K;


    gauge.Next ( gaugesloretapseudoinverse );

    OmpParallelBegin

    AMatrix33       Sj33inv;

    OmpFor

    for ( int sp3 = 0; sp3 < numsolp3; sp3 += 3 ) {

                                        // extract 3x3 diagonal part
        const AMatrix33&    Sj33    = Sj.submat ( sp3, sp3, sp3 + 2, sp3 + 2 );

                                        // to standardize with the standard deviation, we need the inverse square root of a matrix
        Sj33inv     = AMatrixInverseSquareRoot ( Sj33 );

                                        // rescale submatrix of 3 rows for current solution point
        T.rows ( sp3, sp3 + 2 )     = Sj33inv * T.rows ( sp3, sp3 + 2 );
        } // for sp

    OmpParallelEnd

    gauge.Next ( gaugesloretapseudoinverse );

    WriteInverseMatrixFile  (   T,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inversesloreta


//----------------------------------------------------------------------------
void    ComputeInverseELoreta   (
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                )
{
enum                {
                    gaugeeloretaglobal,
                    gaugeeloretapseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set ( InfixInverseELoreta PostfixInverseTitle );

gauge.AddPart ( gaugeeloretaglobal,         5,                                                                          05 );
gauge.AddPart ( gaugeeloretapseudoinverse,  regularization  ? NumSavedRegularizations * ( 4 + ELoretaMaxIterations ) 
                                                            :                           ( 4 + ELoretaMaxIterations ),   95 );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
gauge.Next ( gaugeeloretaglobal );


AMatrix             K ( Kin );

                                        // duplicate original full list
TStrings            spnames   ( spnamesin   );


RejectPointsFromLeadField ( K, spsrejected );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeeloretaglobal );

                                        // centering matrix
ASymmetricMatrix    H           = ACenteringMatrix ( numel );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need these guys a few times
gauge.Next ( gaugeeloretaglobal );

AMatrix             Kt              = K.t ();


gauge.Next ( gaugeeloretaglobal );

ASymmetricMatrix    KKt             = K * Kt;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numreg          = regularization ? NumSavedRegularizations : 1;

ASymmetricMatrix    Winv;
AMatrix             WinvKt  ( numsolp3, numel );
ASymmetricMatrix    KWinvKt;
ASymmetricMatrix    M;
AMatrix             T;
double              sumdiff;
double              sumold;
double              error;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugeeloretaglobal );

TVector<double>     regulvalues;
TStrings            regulnames;


if ( regularization )

    ComputeRegularizationFactors ( KKt, EigenvalueToRegularizationFactorELORETA, regulvalues, regulnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Main loop across Tikhonov regularizations - or at least one without any
for ( int reg = 0; reg < numreg; reg++ ) {

    gauge.Next ( gaugeeloretapseudoinverse );
                                        // init weights, inverse of Id is also Id
    Winv    = AMatrixIdentity ( numsolp3 );
                                        // therefore we can init with this
    KWinvKt = KKt;

                                        // set a limit to the number of iterations
    error   = DBL_MAX;

    for ( int iter = 0; iter < ELoretaMaxIterations; iter++ ) {

        gauge.Next ( gaugeeloretapseudoinverse );

        if ( iter != 0 && error < ELoretaConvergence )
            continue;                   // finish this part of the gauge

        sumdiff     = 0;
        sumold      = 0;


        if ( regularization )   M   = APseudoInverseSymmetric ( KWinvKt + regulvalues[ reg ] * H );
        else                    M   = APseudoInverseSymmetric ( KWinvKt                          );

                                        // We can split the workload by blocks
        OmpParallelBegin

        AMatrix         Kj3t;
        AMatrix33       Wj;
        AMatrix33       Winvj;

        OmpForSum ( sumdiff, sumold )

        for ( int sp3 = 0; sp3 < numsolp3; sp3 += 3 ) {

                                        // extract 1 columns of K (with x,y,z components)
            const AMatrix&  Kj3     = K.cols ( sp3, sp3 + 2 );
                                        // we need it transposed a few times
            Kj3t    = Kj3.t ();
                                        // Compute new Winv
            Wj	    = Kj3t * M * Kj3;
                                        // !Inverting AND square root at the same time!
            Winvj   = AMatrixInverseSquareRoot ( Wj );

                                        // compute error before overwriting Winv
            {
            sumdiff+=     Square ( Winv ( sp3    , sp3     ) - Winvj ( 0, 0 ) )
                    + 2 * Square ( Winv ( sp3    , sp3 + 1 ) - Winvj ( 0, 1 ) )
                    + 2 * Square ( Winv ( sp3    , sp3 + 2 ) - Winvj ( 0, 2 ) )
                    +     Square ( Winv ( sp3 + 1, sp3 + 1 ) - Winvj ( 1, 1 ) )
                    + 2 * Square ( Winv ( sp3 + 1, sp3 + 2 ) - Winvj ( 1, 2 ) )
                    +     Square ( Winv ( sp3 + 2, sp3 + 2 ) - Winvj ( 2, 2 ) );

            sumold +=     Square ( Winv ( sp3    , sp3     ) )
                    + 2 * Square ( Winv ( sp3    , sp3 + 1 ) )
                    + 2 * Square ( Winv ( sp3    , sp3 + 2 ) )
                    +     Square ( Winv ( sp3 + 1, sp3 + 1 ) )
                    + 2 * Square ( Winv ( sp3 + 1, sp3 + 2 ) )
                    +     Square ( Winv ( sp3 + 2, sp3 + 2 ) );
            }

                                        // inverse of block diagonal matrix is diagonal of inverse of sub-blocks
            Winv.submat ( sp3, sp3, sp3 + 2, sp3 + 2 )  = Winvj;

                                        // Update ( Winv * Kt )
            WinvKt.rows ( sp3, sp3 + 2 )    = Winvj * Kj3t;
            }

        OmpParallelEnd
                                        // Compute ( K * Winv * Kt )
        KWinvKt	    = K * WinvKt;

                                        // relative error between norms
        error   = sqrt ( sumdiff / NonNull ( sumold ) );

//      if ( VkQuery () ) DBGV3 ( reg+1, iter+1, error * 100, "reg, iteration -> Avg%Error" );
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here Winv and KWinvKt are up-to-date (but not M)
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    gauge.Next ( gaugeeloretapseudoinverse );

    if ( regularization )   M   = APseudoInverseSymmetric ( KWinvKt + regulvalues[ reg ] * H );
    else                    M   = APseudoInverseSymmetric ( KWinvKt                          );


    gauge.Next ( gaugeeloretapseudoinverse );

    T           = Winv * Kt * M;


    gauge.Next ( gaugeeloretapseudoinverse );

    WriteInverseMatrixFile  (   T,          false, 
                                xyznames,   spnamesin,  &spsrejected,
                                regularization ? numreg             : 0,
                                regularization ? reg                : 0, 
                                regularization ? &regulvalues[ 0 ]  : 0, 
                                regularization ? &regulnames        : 0, 
                                filename 
                            );

    } // for reg

} // inverseeloreta


//----------------------------------------------------------------------------
void    ComputeInverseDale      (   
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                )
{
enum                {
                    gaugedaleglobal,
                    gaugedalepseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set ( InfixInverseDale PostfixInverseTitle );

gauge.AddPart ( gaugedaleglobal,         4,                                                      04 );
gauge.AddPart ( gaugedalepseudoinverse,  regularization ? NumSavedRegularizations * 4 + 1 : 4,   96 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
gauge.Next ( gaugedaleglobal );


AMatrix             K ( Kin );

                                        // duplicate original full list
TStrings            spnames   ( spnamesin   );


RejectPointsFromLeadField ( K, spsrejected );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugedaleglobal );

                                        // centering matrix
ASymmetricMatrix    H           = ACenteringMatrix ( numel );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need these guys a few times
gauge.Next ( gaugedaleglobal );

AMatrix             Kt              = K.t ();


gauge.Next ( gaugedaleglobal );

ASymmetricMatrix    KKt             = K * Kt;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AMatrix             T;
AMatrix             SjDale;             // variance according to Dale formula, focusing only on the noise part
double              regvalue;


if ( regularization ) {

    gauge.Next ( gaugedalepseudoinverse );

    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;


    ComputeRegularizationFactors ( KKt, EigenvalueToRegularizationFactorDale, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    for ( int reg = 0; reg < numreg; reg++ ) {

                                        // we need a tiny bit of noise, even for the no-regularization case, otherwise the diagonal matrix is empty
        regvalue        = reg > 0 ? regulvalues[ reg ] : regulvalues[ 1 ] / 10 /*SingleFloatEpsilon*/;


        gauge.Next ( gaugedalepseudoinverse );
                                        // simple minimum norm inverse matrix
        T               = Kt * APseudoInverseSymmetric ( KKt + regvalue * H );


        gauge.Next ( gaugedalepseudoinverse );
                                        // Dale formula, a priori noise model
        SjDale          = T * ( ( regvalue * H ) * T.t () );


        gauge.Next ( gaugedalepseudoinverse );

        OmpParallelBegin

        ADiagonalMatrix33   Diag;

        OmpFor

        for ( int sp3 = 0; sp3 < numsolp3; sp3 += 3 ) {

                                      // Dale - power should be 0.25, but 0.30 looks less superficial
            Diag ( 0, 0 ) = SjDale ( sp3    , sp3     ) ? 1 / powl ( SjDale ( sp3    , sp3     ), 0.25 /*0.30*/ ) : 1;
            Diag ( 1, 1 ) = SjDale ( sp3 + 1, sp3 + 1 ) ? 1 / powl ( SjDale ( sp3 + 1, sp3 + 1 ), 0.25 /*0.30*/ ) : 1;
            Diag ( 2, 2 ) = SjDale ( sp3 + 2, sp3 + 2 ) ? 1 / powl ( SjDale ( sp3 + 2, sp3 + 2 ), 0.25 /*0.30*/ ) : 1;

                                        // Dale rescale submatrix of 3 rows for current solution point
            T.rows ( sp3, sp3 + 2 )     = Diag * T.rows ( sp3, sp3 + 2 );
            } // for sp

        OmpParallelEnd

        gauge.Next ( gaugedalepseudoinverse );

        WriteInverseMatrixFile  (   T,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg

    } // if regularization

else { // no regularization - actually this case shouldn't be used, as there is no theoretical noise...

                                        // we need a tiny bit of noise, even for the no-regularization case, otherwise the diagonal matrix is empty
    regvalue        = SingleFloatEpsilon;


    gauge.Next ( gaugedalepseudoinverse );

//  T               = Kt * APseudoInverseSymmetric ( KKt );
    T               = Kt * APseudoInverseSymmetric ( KKt + regvalue * H );


    gauge.Next ( gaugedalepseudoinverse );
                                        // Dale formula, a priori noise model
    SjDale          = T * ( ( regvalue * H ) * T.t () );


    gauge.Next ( gaugedalepseudoinverse );

    OmpParallelBegin

    ADiagonalMatrix33   Diag;

    OmpFor

    for ( int sp3 = 0; sp3 < numsolp3; sp3 += 3 ) {

                                      // Dale - power should be 0.25, but 0.30 looks less superficial
        Diag ( 0, 0 ) = SjDale ( sp3    , sp3     ) ? 1 / powl ( SjDale ( sp3    , sp3     ), 0.25 /*0.30*/ ) : 1;
        Diag ( 1, 1 ) = SjDale ( sp3 + 1, sp3 + 1 ) ? 1 / powl ( SjDale ( sp3 + 1, sp3 + 1 ), 0.25 /*0.30*/ ) : 1;
        Diag ( 2, 2 ) = SjDale ( sp3 + 2, sp3 + 2 ) ? 1 / powl ( SjDale ( sp3 + 2, sp3 + 2 ), 0.25 /*0.30*/ ) : 1;

                                        // Dale rescale submatrix of 3 rows for current solution point
        T.rows ( sp3, sp3 + 2 )     = Diag * T.rows ( sp3, sp3 + 2 );
        } // for sp

    OmpParallelEnd

    gauge.Next ( gaugedalepseudoinverse );

    WriteInverseMatrixFile  (   T,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inversedale


//----------------------------------------------------------------------------

void    ComputeInverseLaura     (   
                                const AMatrix&      Kin,            TPoints&                solpointsin,    TSelection              spsrejected,
                                NeighborhoodType    neighborhood,   double                  laurapower,     NeighborhoodReduction   neighbors26to18,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename,
                                TVerboseFile*       verbose
                                )

{
enum                {
                    gaugelauraglobal     = 0,
                    gaugelaurakronecker,
                    gaugelaurapseudoinverse,
                    };


TSuperGauge         gauge;

gauge.Set     ( InfixInverseLaura PostfixInverseTitle );

gauge.AddPart ( gaugelauraglobal,           7,                                                      33 );
gauge.AddPart ( gaugelaurakronecker,        3 * 3,                                                  33 );
gauge.AddPart ( gaugelaurapseudoinverse,    regularization ? NumSavedRegularizations * 2 + 1 : 2,   34 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local points of Lead Field, with points removed
AMatrix             K ( Kin );

                                        // duplicate original full list
TPoints             solpoints ( solpointsin );
TStrings            spnames   ( spnamesin   );

                                        // add points not suitable for LAURA to spsrejected
RejectSingleNeighbors ( solpoints, neighborhood, spsrejected );

                                        // force modify our local list
RejectPointsFromList ( solpoints, spnames, spsrejected );


RejectPointsFromLeadField ( K, spsrejected );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugelauraglobal );

int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;

TPoints             points ( solpoints );   // duplicate list to array, for faster access
double              step            = points.GetMedianDistance ();
int                 maxneighbors    =        Neighborhood[ neighborhood ].NumNeighbors;
double              neighborradius  = step * Neighborhood[ neighborhood ].MidDistanceCut;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scan for # of neighbors
gauge.Next ( gaugelauraglobal );

//if ( neighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 ) ) {

TVector<int>        NumNeighbors;

GetNumGreyNeighbors ( points, neighborradius, NumNeighbors );


TVector<int>        numnn ( /*maxneighbors*/ Neighborhood[ Neighbors26 ].NumNeighbors + 1 );
TFileName           buff;


//numnn   = 0;
//
//for ( int i = 0; i < numsolp; i++ )
//    numnn[ NumNeighbors[ i ] ]++;
//
//numnn.WriteFile ( "E:\\Data\\Number of Neighbors.Laura26.txt", false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Fill neighborhood matrix A
                                        // Official formula + limit # of neighbors to near 18
gauge.Next ( gaugelauraglobal );

TVector<int>        NumNeighbors2 ( numsolp );  // do a new count for neighborhood
double              neighborradius6     = step * Neighborhood[ Neighbors6  ].MidDistanceCut;
double              neighborradius18    = step * Neighborhood[ Neighbors18 ].MidDistanceCut;
double              neighborradius26    = step * Neighborhood[ Neighbors26 ].MidDistanceCut;
bool                postpone26          = neighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 );


ASymmetricMatrix    A               = AMatrixZero ( numsolp, numsolp );

                                        // first round to add neighbors, within a first limit
                                        // NOT parallelized because of potential side-effects
for ( int i = 0; i < numsolp; i++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
    if ( NumNeighbors2 ( i ) >= maxneighbors )
        continue;

                                        // scan strict upper triangle part
    for ( int j = i + 1; j < numsolp; j++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
        if ( NumNeighbors2 ( j ) >= maxneighbors )
            continue;


        double              d               = ( points[ j ] - points[ i ] ).Norm ();

        bool                isn6            = d > 0                && d <= neighborradius6;
        bool                isn18           = d > neighborradius6  && d <= neighborradius18;
        bool                isn26           = d > neighborradius18 && d <= neighborradius26;

                                     // exception: cube corners handled later
        if ( d <= neighborradius && ! ( postpone26 && isn26 ) ) {

            if ( neighborhood == Neighbors6  &&   isn6 
              || neighborhood == Neighbors18 && ( isn6 || isn18 )
              || neighborhood == Neighbors26 && ( isn6 || isn18 || isn26 ) ) {

                                    // don't forget to normalize the distance into discrete SP steps!
                d               = 1.0 / Power ( d / step, laurapower );

                A ( i, j )      = A ( j, i )    = - d;
                A ( i, i )     += d;
                A ( j, j )     += d;


                NumNeighbors2 ( i )++;
                NumNeighbors2 ( j )++;
                } // if neighbor OK
            } // if radius OK
        } // for j
    } // for i


                                        // optional second pass: if less than 18 neighbors, go 26 neighbors by adding the 8 corners of the cube
if ( postpone26 ) {
                                        // take a snapshot of initially already full (18 neighbors) points, so to avoid adding to reasonably full SP
    TVector<bool>       neighfull18 ( numsolp );

    for ( int i = 0; i < numsolp; i++ )
        neighfull18 ( i )   = NumNeighbors[ i ] >= Neighborhood[ Neighbors18 ].NumNeighbors;

                                        // NOT parallelized because of potential side-effects
    for ( int i = 0; i < numsolp; i++ ) {
                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
        if ( NumNeighbors2 ( i ) >= maxneighbors )
            continue;

                                        // if already enough neighbors, don't try adding more
        if ( neighfull18 ( i ) )
            continue;

                                        // scan complete matrix here, as the current point (i) might connect to another one (j) which is already full
        for ( int j = 0; j < numsolp; j++ ) {

            if ( j == i )   continue;

                                        // neighborhood is already full (security check, in case of irregular SPs and the distance check is not enough)
            if ( NumNeighbors2 ( j ) >= maxneighbors )
                continue;

                                        // strict: if potential neighbor is full, don't upgrade current point even if detrimental
            if ( IsNeighbors26To18Strict ( neighbors26to18 ) && neighfull18 ( j ) )
                continue;


            double              d               = ( points[ j ] - points[ i ] ).Norm ();

            bool                isn26           = d > neighborradius18 && d <= neighborradius26;

                                        // between 18 and 26 radius, and not already added?
            if ( isn26 && A ( i, j ) == 0 ) {

                                        // don't forget to normalize the distance into discrete SP steps!
                d               = 1.0 / Power ( d / step, laurapower );

                A ( i, j )      = A ( j, i )    = - d;
                A ( i, i )     += d;
                A ( j, j )     += d;

                                        // this allows to still add a few neighbors to points that have been bypassed before (>18)
                NumNeighbors2 ( i )++;
                NumNeighbors2 ( j )++;
                } // if radius OK
            } // for j
        } // for i
    } // if neighbors26to18

                                        // done, this is our new real neighborhood
NumNeighbors    = NumNeighbors2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check # of neighbors
numnn   = 0;

for ( int i = 0; i < numsolp; i++ )
    numnn[ NumNeighbors[ i ] ]++;

//numnn.WriteFile ( "E:\\Data\\Number of Neighbors.Laura26to18.txt", false );

                                        // save connection stats to verbose
if ( verbose != 0 ) {

    verbose->NextBlock ();
    verbose->NextTopic ( "LAURA Solution Points Connection:" );

                                            // title
    int                 fieldwidth      = 18;

    (ofstream&) *verbose << StreamFormatGeneral;
    (ofstream&) *verbose << StreamFormatText << setw ( fieldwidth ) << "# of Connections";
    (ofstream&) *verbose << StreamFormatText << setw ( fieldwidth ) << "# of SPs";
    (ofstream&) *verbose << "\n";

    (ofstream&) *verbose << "------------------";
    (ofstream&) *verbose << "------------------";
    (ofstream&) *verbose << "\n";

                                            // data
    for ( int sp = 0; sp < (int) numnn; sp++ ) {

        (ofstream&) *verbose << StreamFormatFixed;

        (ofstream&) *verbose << setw ( fieldwidth ) << setprecision ( 0 ) << sp;
        (ofstream&) *verbose << setw ( fieldwidth ) << setprecision ( 0 ) << numnn[ sp ];
        (ofstream&) *verbose << "\n";
        }


    //for ( int i = 0; i < (int) numnn; i++ ) {
    //    sprintf ( buff, "%0d neighbors -> %0d Solution Points", i, numnn[ i ] );
    //    ShowMessage ( buff, "LAURA Inverse Solution", ShowMessageWarning, this );
    //    }


    if ( numnn[ 0 ] > 0 ) {
                                            // This shouldn't happen, just in case, we reset the Lead Field at these positions

        sprintf ( buff, "%0d Solution Points have no neighbors, therefore the Inverse matrix will be reset for these points.", numnn[ 0 ] );
    //    ShowMessage ( buff, "LAURA Inverse Solution", ShowMessageWarning, this );

                                            // save this to verbose!
        verbose->NextLine ( 1 );
        verbose->NextTopic ( "LAURA Inverse Solution Warning:" );
        (ofstream&) *verbose << buff << "\n";

        verbose->NextLine ();
        for ( int sp = 0; sp < numsolp; sp++ )
            if ( ! NumNeighbors[ sp ] ) {

                for ( int el = 0; el < numel; el++ ) {
                    K ( el, 3 * sp     )    =
                    K ( el, 3 * sp + 1 )    =
                    K ( el, 3 * sp + 2 )    = 0;
                    } // for el

                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].X << "\t";
                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].Y << "\t";
                (ofstream&) *verbose << StreamFormatFloat32 << solpoints[ sp ].Z << "\t";
                (ofstream&) *verbose << StreamFormatText    << spnames  [ sp ];
                (ofstream&) *verbose << "\n";
                }
        verbose->NextLine ();
        }

    verbose->Flush ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Normalize matrix A: distribute the weights across all non-null neighbors
gauge.Next ( gaugelauraglobal );


OmpParallelFor

for ( int i = 0; i < numsolp; i++ ) {
                                        // get average of center
    double                  d               = NumNeighbors[ i ] ? A ( i, i ) / NumNeighbors[ i ] : 0;
                                        // normalize center, for all cases
    A ( i, i )      = maxneighbors;

    if ( d /*NumNeighbors[ i ]*/ )
                                        // normalize neighbors
        for ( int j = i + 1; j < numsolp; j++ )
            A ( i, j )  = A ( j, i )   /= d;
//  else
//      DBGV ( i + 1, "SP with no neighbor" );
    }


//#if defined(InvMatrixMoreOutputs)
//TExportTracks     expneigh;
//sprintf ( expneigh.Filename, "%s\\More\\Neighborhood Matrix.Laura.sef", CrisTransfer.BaseFileName );
//expneigh.SetAtomType ( AtomTypeScalar );
//expneigh.NumTracks        = numsolp;
//expneigh.NumTime          = numsolp;
//
//for ( int i = 0; i < numsolp; i++ )
//for ( int j = 0; j < numsolp; j++ )
//    expneigh.Write ( A[ i ][ j ] );
//#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute weights minimum norm
gauge.Next ( gaugelauraglobal );

ADiagonalMatrix     W1 ( numsolp, numsolp );
                                        // Laura way
                                        // This is the version used for a long time
                                        // Without Z-Score, it performs better than the Cartool way
                                        // With Z-Score, 26to18 neighbors is equivalent as the Cartool way
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumkx       = 0;
    double          sumky       = 0;
    double          sumkz       = 0;
    double          sumk;

    for ( int el = 0; el < numel; el++ ) {
        sumkx  += Square ( K ( el, 3 * sp     ) );
        sumky  += Square ( K ( el, 3 * sp + 1 ) );
        sumkz  += Square ( K ( el, 3 * sp + 2 ) );
        }

    sumkx       = sqrt ( sumkx ) / numel;
    sumky       = sqrt ( sumky ) / numel;
    sumkz       = sqrt ( sumkz ) / numel;
    sumk        = ( sumkx + sumky + sumkz ) / 3;

    W1 ( sp, sp ) = sumk;
    }

/*                                        // Cartool way - no change without Z-Score, a bit better with Z-Score in the distance measure
OmpParallelFor

for ( int sp = 0; sp < numsolp; sp++ ) {

    double          sumk        = 0;

    for ( int el = 0; el < numel; el++ )

        sumk   += Square ( K ( el, 3 * sp     ) ) 
               +  Square ( K ( el, 3 * sp + 1 ) ) 
               +  Square ( K ( el, 3 * sp + 2 ) );

    sumk        = sqrt ( sumk / ( 3 * numel ) );

    W1inv ( sp, sp ) = sumk ? 1 / sumk : 0;
    }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute M
                                        // The official way
gauge.Next ( gaugelauraglobal );

ASymmetricMatrix    W1AtAW1         = W1 * A.t () * A * W1;

A. ARelease ();
W1.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // To avoid the immondeous Kronecker inflation
                                        // we split the problem by dimension, then reconstruct the expected result

AMatrix             WjKt    ( numsolp3, numel );

TArmaLUSolver       W1AtAW1_LU ( W1AtAW1 );


OmpParallelFor

for ( int dim = 0; dim < 3; dim++ ) {
                                        // get sub-Lead Field
    gauge.Next ( gaugelaurakronecker );

    AMatrix             Ktd        ( numsolp,  numel );

    for ( int sp = 0; sp < numsolp; sp++ )
    for ( int el = 0; el < numel; el++ )
        Ktd ( sp, el )  = K ( el, 3 * sp + dim );

                                        // intermediate station, breath...
                                        // The official way
    gauge.Next ( gaugelaurakronecker );

    AMatrix             WjKtd           = W1AtAW1_LU.Solve ( Ktd );

                                        // inject current dimension into the full matrix, simulating the Kronecker
    gauge.Next ( gaugelaurakronecker );

    for ( int sp = 0; sp < numsolp; sp++ )
    for ( int el = 0; el < numel; el++ )
        WjKt ( 3 * sp + dim, el )  = WjKtd ( sp, el );

    } // for dim


W1AtAW1.ARelease ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugelauraglobal );

ASymmetricMatrix    KWjKt           = K * WjKt;

AMatrix             J;


if ( regularization ) {

    gauge.Next ( gaugelaurapseudoinverse );

    int                 numreg          = NumSavedRegularizations;
    TVector<double>     regulvalues;
    TStrings            regulnames;


    ComputeRegularizationFactors ( KWjKt, EigenvalueToRegularizationFactorLAURA, regulvalues, regulnames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !article doesn't use H, but I!
//  ASymmetricMatrix    H;
//
//  ComputeCenteringMatrix  ( H, numel );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matrices with Tikhonov regularization
    ASymmetricMatrix    I               = AMatrixIdentity ( numel );


    for ( int reg = 0; reg < numreg; reg++ ) {

        gauge.Next ( gaugelaurapseudoinverse );

        J               = WjKt * APseudoInverseSymmetric ( KWjKt + regulvalues[ reg ] * I );


        gauge.Next ( gaugelaurapseudoinverse );

        WriteInverseMatrixFile  (   J,          false, 
                                    xyznames,   spnamesin,  &spsrejected, 
                                    numreg,     reg,        &regulvalues[ 0 ],  &regulnames, 
                                    filename 
                                );
        } // for reg

    } // if regularization

else { // no regularization
    gauge.Next ( gaugelaurapseudoinverse );
                                        // The official way, but with regularization = 0, it's total crap
//  J               = WjKt * ( K * WjKt ).i ();
                                        // "A la Loreta" way, for regularization != 0 it's exactly the same results as inverse, plus consistent result for regularization = 0
    J               = WjKt * APseudoInverseSymmetric ( KWjKt );


    gauge.Next ( gaugelaurapseudoinverse );

    WriteInverseMatrixFile  (   J,          false, 
                                xyznames,   spnamesin,  &spsrejected, 
                                0,          0,          0,              0, 
                                filename 
                            );
    }

} // inverselaura


//----------------------------------------------------------------------------

OptimizeOn

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
