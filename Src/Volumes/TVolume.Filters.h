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

#include    "TVolumeRegions.h"
#include    "TFilters.Ranking.h"

namespace crtl {

//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::Filter ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
if      ( filtertype == FilterTypeGaussian
       || filtertype == FilterTypeHighPassLight
       || filtertype == FilterTypeMean              )   FilterLinear            ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeFastGaussian      )   FilterFastGaussian      ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeAnisoGaussian     )   FilterAnisoGaussian     (                           params, showprogress );

else if ( filtertype == FilterTypeMin
       || filtertype == FilterTypeMax
       || filtertype == FilterTypeMinMax
       || filtertype == FilterTypeRange
       || filtertype == FilterTypeMedian
       || filtertype == FilterTypeMaxMode
       || filtertype == FilterTypeMeanSub
       || filtertype == FilterTypeLogMeanSub
       || filtertype == FilterTypeMeanDiv
       || filtertype == FilterTypeLogMeanDiv
       || filtertype == FilterTypeMAD
       || filtertype == FilterTypeSD
       || filtertype == FilterTypeSDInv
       || filtertype == FilterTypeCoV
       || filtertype == FilterTypeLogCoV
       || filtertype == FilterTypeLogSNR
       || filtertype == FilterTypeMCoV
       || filtertype == FilterTypeLogMCoV
       || filtertype == FilterTypeMADSDInv
       || filtertype == FilterTypeModeQuant
       || filtertype == FilterTypeEntropy           )   FilterStat              ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeRank
       || filtertype == FilterTypeRankRamp          )   FilterRank              ( filtertype,               params               );

else if ( filtertype == FilterTypeThreshold
       || filtertype == FilterTypeThresholdAbove
       || filtertype == FilterTypeThresholdBelow
       || filtertype == FilterTypeThresholdBinarize )   FilterThreshold         ( filtertype,               params               );
else if ( filtertype == FilterTypeBinarize          )   FilterBinarize          (                           params               );
else if ( filtertype == FilterTypeRevert            )   FilterRevert            (                           params               );
else if ( filtertype == FilterTypeIntensityMult
       || filtertype == FilterTypeIntensityDiv      )   FilterIntensityScale    ( filtertype,               params               );
else if ( filtertype == FilterTypeIntensityNorm     )   FilterIntensityNorm     (                           params               );
else if ( filtertype == FilterTypeIntensityAdd
       || filtertype == FilterTypeIntensitySub      )   FilterIntensityAdd      ( filtertype,               params               );
else if ( filtertype == FilterTypeIntensityRemap    )   FilterIntensityRemap    (                           params               );
else if ( filtertype == FilterTypeKeepValue         )   FilterKeepValue         (                           params               );
else if ( filtertype == FilterTypeIntensityOpposite )   FilterIntensityOpposite (                           params               );
else if ( filtertype == FilterTypeIntensityInvert   )   FilterIntensityInvert   (                           params               );
else if ( filtertype == FilterTypeIntensityAbs      )   FilterIntensityAbsolute (                           params               );
else if ( filtertype == FilterTypeToMask            )   FilterToMask            (                           params, showprogress );
else if ( filtertype == FilterTypeSymmetrize        )   FilterSymmetrize        (                           params               );

else if ( filtertype == FilterTypeHistoEqual        )   FilterHistoEqual        (                           params, showprogress );
else if ( filtertype == FilterTypeHistoEqualBrain   )   FilterHistoEquBrain     (                           params, showprogress );
else if ( filtertype == FilterTypeHistoCompact      )   FilterHistoCompact      (                           params, showprogress );

else if ( filtertype == FilterTypeErode             )   FilterErode             (                           params, showprogress );
else if ( filtertype == FilterTypeDilate            )   FilterDilate            (                           params, showprogress );
else if ( filtertype == FilterTypeOpen              )   FilterOpen              (                           params, showprogress );
else if ( filtertype == FilterTypeClose             )   FilterClose             (                           params, showprogress );
else if ( filtertype == FilterTypeMorphGradient     )   FilterMorphGradient     ( filtertype,               params, showprogress );
else if ( filtertype == FilterTypeMorphGradientInt  )   FilterMorphGradient     ( filtertype,               params, showprogress );
else if ( filtertype == FilterTypeMorphGradientExt  )   FilterMorphGradient     ( filtertype,               params, showprogress );
else if ( filtertype == FilterTypeThinning          )   FilterThinning          (                           params, showprogress );
else if ( filtertype == FilterTypeWaterfallRidges
       || filtertype == FilterTypeWaterfallValleys  )   FilterWaterfall         ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeLessNeighbors
       || filtertype == FilterTypeMoreNeighbors     )   FilterNeighbors         ( filtertype,               params, showprogress );
else if ( filtertype == FilterTypeRelax             )   FilterRelax             (                           params, showprogress );
else if ( filtertype == FilterTypeKeepBiggestRegion )   KeepRegion              ( SortRegionsCount,         Truncate ( GetLinearDim () * 0.001 ), INT_MAX, Neighbors26, 0,    showprogress );
else if ( filtertype == FilterTypeKeepCompactRegion )   KeepRegion              ( SortRegionsCompactCount,  Truncate ( GetLinearDim () * 0.001 ), INT_MAX, Neighbors26, 0,    showprogress );
else if ( filtertype == FilterTypeKeepBiggest2Regions ) KeepRegion              ( SortRegionsCount,         Truncate ( GetLinearDim () * 0.001 ), INT_MAX, Neighbors26, 0.85, showprogress );
else if ( filtertype == FilterTypeKeepCompact2Regions ) KeepRegion              ( SortRegionsCompactCount,  Truncate ( GetLinearDim () * 0.001 ), INT_MAX, Neighbors26, 0.85, showprogress );
else if ( filtertype == FilterTypeClustersToRegions   ) ClustersToRegions       (                           params,                           Neighbors26,       showprogress );

else if ( filtertype == FilterTypePercentFullness
       || filtertype == FilterTypeCutBelowFullness
       || filtertype == FilterTypeSAV
       || filtertype == FilterTypeCompactness       )   FilterShapeFeature  ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeGradient
       || filtertype == FilterTypeCanny
       || filtertype == FilterTypeLaplacian
       || filtertype == FilterTypeHessianEigenMax
       || filtertype == FilterTypeHessianEigenMin
       || filtertype == FilterTypeKCurvature
       || filtertype == FilterTypeKCCurvature       )   FilterPartDeriv     ( filtertype,               params, 0, showprogress );

else if ( filtertype == FilterTypeBiasField         )   FilterBiasField     (                           params, 0, showprogress );
else if ( filtertype == FilterTypeSegmentCSF        )   SegmentCSF          (                           params, showprogress );
else if ( filtertype == FilterTypeSegmentGrey       )   SegmentGreyMatter   (                           params, showprogress );
else if ( filtertype == FilterTypeSegmentWhite      )   SegmentWhiteMatter  (                           params, showprogress );
else if ( filtertype == FilterTypeSegmentTissues    )   SegmentTissues      (                           params, showprogress );
else if ( filtertype == FilterTypeSkullStripping    )   SkullStripping      (                           params, showprogress );
else if ( filtertype == FilterTypeBrainstemRemoval  )   BrainstemRemoval    (                           params, showprogress );

else if ( filtertype == FilterTypePeeling           )   FilterPeeling       (                           params, showprogress );

//else if ( filtertype == FilterTypeResize          )   FilterResize        (                           params, showprogress );

else
    ShowMessage ( "Filter not implemented for TVolume<TypeD>!", FilterPresets[ filtertype ].Text, ShowMessageWarning );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterDim1 ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<TypeD>          line ( Dim1 );


for ( int i2 = 0; i2 < Dim2; i2++ )
for ( int i3 = 0; i3 < Dim3; i3++ ) {

    for ( int i1 = 0; i1 < Dim1; i1++ )
        line ( i1 ) = GetValue ( i1, i2, i3 );


    line.Filter ( filtertype, params, showprogress );


    for ( int i1 = 0; i1 < Dim1; i1++ )
        GetValue ( i1, i2, i3 ) = line ( i1 );
    }
}


template <class TypeD>
void    TVolume<TypeD>::FilterDim2 ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<TypeD>          line ( Dim2 );


for ( int i1 = 0; i1 < Dim1; i1++ )
for ( int i3 = 0; i3 < Dim3; i3++ ) {

    for ( int i2 = 0; i2 < Dim2; i2++ )
        line ( i2 ) = GetValue ( i1, i2, i3 );


    line.Filter ( filtertype, params, showprogress );


    for ( int i2 = 0; i2 < Dim2; i2++ )
        GetValue ( i1, i2, i3 ) = line ( i2 );
    }
}


template <class TypeD>
void    TVolume<TypeD>::FilterDim3 ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<TypeD>          line ( Dim3 );


for ( int i1 = 0; i1 < Dim1; i1++ )
for ( int i2 = 0; i2 < Dim2; i2++ ) {

    for ( int i3 = 0; i3 < Dim3; i3++ )
        line ( i3 ) = GetValue ( i1, i2, i3 );


    line.Filter ( filtertype, params, showprogress );


    for ( int i3 = 0; i3 < Dim3; i3++ )
        GetValue ( i1, i2, i3 ) = line ( i3 );
    }
}


//----------------------------------------------------------------------------
                                        // Fastest Gaussian filtering, with SKISPM:
// "An efficient algorithm for Gaussian blur using finite-state machines"
// Frederick M. Waltz and John W. V. Miller
// SPIE Conf. on Machine Vision Systems for Inspection and Metrology VII
// Originally published Boston, Nov. 1998

                                        // Size parameter is an odd, discrete width in voxels, like 3, 5, 7 etc..
                                        // We repeat elementary and fast 3x3x3 filters as many times as needed to reach the required width
                                        // Border padding is currently set with 0
template <class TypeD>
void    TVolume<TypeD>::FilterFastGaussian    (   FilterTypes     filtertype, FctParams& params, bool    showprogress    )
{
int                 numrepeat;


if ( params ( FilterParamDiameter ) < 0 )
    numrepeat       = (int) fabs ( params ( FilterParamDiameter ) );    // !if negative value, force use as the number of repetitions!
else
    numrepeat       = params ( FilterParamDiameter ) > 2 ? Round ( ( Power ( params ( FilterParamDiameter ) - 2, 1.35 ) - 1 ) / 2 ) : 0;    // ad-hoc formula after comparison with our regular Gaussian


if ( numrepeat < 1 )                    // not big enough
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border + conversion to higher precision
                                        // border is set to 0
int                 tdim1           = Dim1 + 2 * numrepeat;
int                 tdim2           = Dim2 + 2 * numrepeat;
int                 tdim3           = Dim3 + 2 * numrepeat;
TVolume<TypeD>      temp ( tdim1, tdim2, tdim3 );

//                                        // insert with origin shift + conversion
//for ( int x = 0; x < Dim1; x++ )
//for ( int y = 0; y < Dim2; y++ )
//for ( int z = 0; z < Dim3; z++ )
//    temp ( x + numrepeat, y + numrepeat, z + numrepeat )    = GetValue ( x, y, z );

temp.Insert ( *this, TPointInt ( numrepeat, numrepeat, numrepeat ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Smarter with 1D and 2D buffers, less errors
                                        // allocate SKIPSM buffers

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? numrepeat : 0 );

TArray2<double>     SX0 ( tdim2, tdim3 );   // 2 slices YZ
TArray2<double>     SX1 ( tdim2, tdim3 );
TArray1<double>     SY0        ( tdim3 );   // 2 rows   Z
TArray1<double>     SY1        ( tdim3 );


for ( int n = 0; n < numrepeat; n++ ) {

    Gauge.Next ();

    for ( int x = 1; x < tdim1; x++ ) {

        SY0.ResetMemory ();
        SY1.ResetMemory ();


        for ( int y = 1; y < tdim2; y++ ) {

            double      SZ0     = 0;        // 2 previous values
            double      SZ1     = 0;

            for ( int z = 1; z < tdim3; z++ ) {

                double          tmp1;
                double          tmp2;

                tmp1            = temp ( x, y, z );     // |1|

                tmp2            = SZ0 + tmp1;           // |1 1|
                SZ0             = tmp1;

                tmp1            = SZ1 + tmp2;           // |1 2 1|
                SZ1             = tmp2;

                tmp2            = SY0[ z ] + tmp1;
                SY0[ z ]        = tmp1;

                tmp1            = SY1[ z ] + tmp2;
                SY1[ z ]        = tmp2;

                tmp2            = SX0[ y ][ z ] + tmp1;
                SX0[ y ][ z ]   = tmp1;

                temp ( x - 1, y - 1, z - 1 )    = (TypeD) ( ( SX1[ y ][ z ] + tmp2 ) / 64.0 );
                SX1[ y ][ z ]   = tmp2;
                } // for z
            } // for y
        } // for x
    } // for numrepeat
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Faster (parallelized) with repeated 1D operations on each axis
                                        // Errors cumulate, but are less than 1e-6 when compared with method above

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress );

                                        // allocate threads here, but not in parallel parts yet
OmpParallelBegin
                                        // here we know the actual number of threads
if ( showprogress )
    Gauge.SetRange ( 0, 3 * numrepeat * GetNumThreads () );

for ( int n = 0; n < numrepeat; n++ ) {

    Gauge.Next ();

    OmpFor

    for ( int y = 1; y < tdim2; y++ )
    for ( int z = 1; z < tdim3; z++ ) {

        double      S0      = 0;
        double      S1      = 0;

        for ( int x = 1; x < tdim1; x++ ) {

            double  tmp1    = temp ( x, y, z );         // |1|

            double  tmp2    = S0 + tmp1;                // |1 1|
            S0              = tmp1;

            temp ( x - 1, y, z )    = ( S1 + tmp2 ) / 4.0;

            S1              = tmp2;
            } // for x
        }


    Gauge.Next ();

    OmpFor

    for ( int x = 1; x < tdim1; x++ )
    for ( int y = 1; y < tdim2; y++ ) {

        double      S0      = 0;
        double      S1      = 0;

        for ( int z = 1; z < tdim3; z++ ) {

            double  tmp1    = temp ( x, y, z );         // |1|

            double  tmp2    = S0 + tmp1;                // |1 1|
            S0              = tmp1;

            temp ( x, y, z - 1 )    = ( S1 + tmp2 ) / 4.0;

            S1              = tmp2;
            } // for z
        }


    Gauge.Next ();

    OmpFor

    for ( int x = 1; x < tdim1; x++ )
    for ( int z = 1; z < tdim3; z++ ) {

        double      S0      = 0;
        double      S1      = 0;

        for ( int y = 1; y < tdim2; y++ ) {

            double  tmp1    = temp ( x, y, z );         // |1|

            double  tmp2    = S0 + tmp1;                // |1 1|
            S0              = tmp1;

            temp ( x, y - 1, z )    = ( S1 + tmp2 ) / 4.0;

            S1              = tmp2;
            } // for z
        }
    } // for numrepeat

OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy & clip results
for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ )

    Array[ IndexesToLinearIndex ( x, y, z ) ]   = temp ( x + numrepeat, y + numrepeat, z + numrepeat );
}


//----------------------------------------------------------------------------
                                        // Border padding with 0's
template <class TypeD>
void    TVolume<TypeD>::FilterLinear    (   FilterTypes     filtertype, FctParams& params, bool    showprogress    )
{
double              diameter        = filtertype == FilterTypeHighPassLight ? 3 : params ( FilterParamDiameter );

if ( diameter < 2 )                     // no other voxels involved other than center?
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // see:  http://en.wikipedia.org/wiki/Sampled_Gaussian_kernel#The_sampled_Gaussian_kernel

                                        // create Kernel mask, always of odd size
TVolume<double>     Kf ( diameter, OddSize );   // the floating point version, used to compute the Kernel
TPointInt           Ko ( Kf.GetDim1 () / 2, Kf.GetDim2 () / 2, Kf.GetDim3 () / 2 );


double              r2              = Square ( diameter / 2 ) + ( IsEven ( Kf.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointInt           kp;
double              v;


int                 gaussnumSD      = 2;
TGaussian           gauss ( 0, ( diameter - 1 ) / 2, gaussnumSD );

                                        // compute the Kernel in floating point
for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
for ( int yk = 0; yk < Kf.GetDim2 (); yk++ )
for ( int zk = 0; zk < Kf.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );

    if      ( filtertype == FilterTypeGaussian      )       v   = gauss ( kp.Norm () );
    else if ( filtertype == FilterTypeHighPassLight )       v   = ( kp.X || kp.Y || kp.Z ? -1 : 4 ) * gauss ( kp.Norm () ); // very rough, only for 3x3x3
    else /*if ( filtertype == FilterTypeMean        )*/     v   = 1;

                                        // clip Kernel outside Euclidian norm!
    Kf ( xk, yk, zk )   = kp.Norm2 () <= r2 ? v : 0;
    } // for xk, yk, zk

                                        // normalize Kernel
double              sumk            = 0;

for ( int i = 0; i < Kf.GetLinearDim (); i++ )
    sumk       += Kf[ i ];

Kf         /= sumk;


//DBGV3 ( diameter, Kf.GetDim1 (), Ko.X, "diameter  Kernel: Dim1 Origin" );
//for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
//for ( int yk = 0; yk < Kf.GetDim2 (); yk++ )
//for ( int zk = 0; zk < Kf.GetDim3 (); zk++ )
//    DBGV4 ( xk, yk, zk, Kf ( xk, yk, zk ) * 1000, "x,y,z  K" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2, Dim3 + Ko.Z * 2 );

temp.Insert ( *this, Ko );

/*
int                 inindex;
int                 outindex;
int                 rowsize         = Dim3 * sizeof ( double );

for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ ) {
                                        // get index of first Z
    inindex  =      IndexesToLinearIndex ( x, y, 0 );
    outindex = temp.IndexesToLinearIndex ( x, y, Ko.Z );
                                        // direct copy of a full Z line
    CopyVirtualMemory ( &temp[ outindex ], &Array[ inindex ], rowsize );
    } // for x, y
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );


OmpParallelFor

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();


    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {

        double          v           = 0;

                                        // scan kernel
        for ( int xki = 0, xk = x; xki < Kf.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y; yki < Kf.GetDim2 (); yki++, yk++ )
        for ( int zki = 0, zk = z; zki < Kf.GetDim3 (); zki++, zk++ )

            v  += Kf ( xki, yki, zki ) * temp ( xk, yk, zk );


        Array[ IndexesToLinearIndex ( x, y, z ) ]    = (TypeD) v;
        } // for y, z
    } // for x
}


//----------------------------------------------------------------------------
                                        // Anything stat-like filter

                                        // Relationship of diameter to # of neighbors:
                                        //      2.00 ->  6 neighbors
                                        //      2.83 -> 18 neighbors
                                        //      3.47 -> 26 neighbors
template <class TypeD>
void    TVolume<TypeD>::FilterStat      (   FilterTypes     filtertype, FctParams& params, bool    showprogress    )
{
double              diameter        =                           params ( FilterParamDiameter );
double              histomaxbins    =                   Round ( params ( FilterParamNumModeQuant ) );
FilterResultsType   filterresult    = (FilterResultsType) (int) params ( FilterParamResultType );


if ( diameter < 2 )                     // no other voxels involved other than center?
    return;

if ( filtertype == FilterTypeModeQuant && histomaxbins < 2 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, always of odd size
TVolume<uchar>      K  ( diameter, OddSize );
TPointInt           Ko ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( K.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointInt           kp;


                                        // compute the Kernel
for ( int xk = 0; xk < K.GetDim1 (); xk++ )
for ( int yk = 0; yk < K.GetDim2 (); yk++ )
for ( int zk = 0; zk < K.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );
                                        // clip Kernel outside Euclidian norm!
    K ( xk, yk, zk )    = kp.Norm2 () <= r2;
//  K ( xk, yk, zk )    = sqrt ( kp.X * kp.X * 1.00 + kp.Y * kp.Y * 1.15 + kp.Z * kp.Z * 1.30 ) <= r2;  // trial for an elliptic Kernel
    } // for xk, yk, zk


//TFileName           _file;
//sprintf ( _file, "E:\\Data\\Kernel.Stat %.2f." DefaultMriExt, diameter );
//K.WriteFile ( _file, TPointDouble ( Ko ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2, Dim3 + Ko.Z * 2 );

temp.Insert ( *this, Ko );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Not all statistics need to be robust, which is longer to process
bool                histostat       = filtertype == FilterTypeModeQuant
                                   || filtertype == FilterTypeEntropy;

bool                robuststat      = histostat
                                   || filtertype == FilterTypeMedian 
                                   || filtertype == FilterTypeMaxMode
                                   || filtertype == FilterTypeMAD
                                   || filtertype == FilterTypeMCoV
                                   || filtertype == FilterTypeLogMCoV
                                   || filtertype == FilterTypeMADSDInv;


//double            maxvalue        = GetMaxValue ();
//int               histominfreq    = max ( 3, Round ( Power ( K.GetNumSet (), 1 / 3.0 ) * 1.0 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Log filters need some offset to avoid values <= 0
bool                logfilters      = filtertype == FilterTypeLogCoV
                                   || filtertype == FilterTypeLogMCoV
                                   || filtertype == FilterTypeLogMeanSub
                                   || filtertype == FilterTypeLogMeanDiv
                                   || filtertype == FilterTypeLogSNR;

double              logoffset       = logfilters ? params ( FilterParamLogOffset ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelBegin
                                        // private variables
TEasyStats          stat ( robuststat ? K.GetLinearDim () : 0 );
THistogram          histo;
double              v;

OmpFor

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();

    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {

        stat.Reset ();

                                        // scan kernel
        for ( int xki = 0, xk = x; xki < K.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y; yki < K.GetDim2 (); yki++, yk++ )
        for ( int zki = 0, zk = z; zki < K.GetDim3 (); zki++, zk++ )

            if ( K ( xki, yki, zki ) )

                stat.Add ( temp ( xk, yk, zk ), ThreadSafetyIgnore );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if      ( filtertype == FilterTypeMin               )   v   = stat.Min ();
        else if ( filtertype == FilterTypeMax               )   v   = stat.Max ();
        else if ( filtertype == FilterTypeRange             )   v   = stat.Range ();
        else if ( filtertype == FilterTypeMedian            )   v   = stat.Median ();
        else if ( filtertype == FilterTypeMaxMode           )   v   = stat.MaxModeHSM ();
        else if ( filtertype == FilterTypeSD                )   v   = stat.SD ();
        else if ( filtertype == FilterTypeSDInv             )   v   = ( v = stat.SD () ) != 0 ? NoMore ( 1e10, 1 / v ) : 0; // capping the max(?)
        else if ( filtertype == FilterTypeCoV               )   v   = stat.CoV ();
        else if ( filtertype == FilterTypeLogCoV            )   v   = Log10 ( ( logoffset + stat.SD () ) / ( logoffset + stat.Average () ) );
//      else if ( filtertype == FilterTypeSNR               )   v   = stat.SNR ();
        else if ( filtertype == FilterTypeLogSNR            )   v   = Log10 ( logoffset + stat.SNR () );    // much better with a log

                                        // Sharp Window: take the one closest, the min or the max
        else if ( filtertype == FilterTypeMinMax            )   v   = stat.MinMax ( GetValue ( x, y, z ) );

                                        // Mean Subtraction Window
        else if ( filtertype == FilterTypeMeanSub           )   v   = GetValue ( x, y, z ) - stat.Average ();
        else if ( filtertype == FilterTypeLogMeanSub        )   v   = Log10 ( logoffset + fabs ( GetValue ( x, y, z ) - stat.Average () ) );
        else if ( filtertype == FilterTypeMeanDiv           )   v   = GetValue ( x, y, z ) / NonNull ( stat.Average () );           // "real" formula
//      else if ( filtertype == FilterTypeMeanDiv           )   v   = ( 1 + GetValue ( x, y, z ) ) / ( 1 + stat.Average () ) - 1;   // formula more compatible to Log Mean Division below, with signed results
        else if ( filtertype == FilterTypeLogMeanDiv        )   v   = Log10 ( ( logoffset + GetValue ( x, y, z ) ) / ( logoffset + stat.Average () ) );
        else if ( filtertype == FilterTypeMAD               )   v   = stat.MAD ( CanAlterData );// Median Absolute Deviation (MAD)
        else if ( filtertype == FilterTypeMCoV              )   v   = stat.RobustCoV ();        // Same as Coefficient of Variation, but with Median
        else if ( filtertype == FilterTypeLogMCoV           )   v   = Log10 ( ( logoffset + stat.InterQuartileRange () ) / ( logoffset + stat.Median () ) );

                                        // combine MAD & SD, which is usefull for white matter extraction
        else if ( filtertype == FilterTypeMADSDInv ) {
                                        // store MAD & SD
            double  sd  = stat.SD  ();
            double  mad = stat.MAD ( CanAlterData );
                                        // combine MAD & SD
            v   = mad * sd;
                                        // use the SD alone if v is null; set to null otherwise (more intuitive)
            v   = v  != 0 ? NoMore ( 1e10, 1 / sqrt ( v ) ) // capping the max(?)
                : sd != 0 ? NoMore ( 1e10, 1 / sd )
                :           0;
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Quantized Mode Window
                                        // resulting value is the index of level, 0 .. #modes - 1
                                        // !Not super reliable since floating point conversion, but not used either, so...!
        else if ( filtertype == FilterTypeModeQuant ) {

            histo.ComputeHistogram ( stat, 0, 0, 0, 0, 1, (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear ) );

            double      maxmode     = histo.GetMaxValue ();
   
//            if ( maxmode < histominfreq )
//
//                v   = stat.Median ();   // fallback formula
//
//            else {
                                        // check there is a single unambigous maximum mode
                int     nummodes    = histo.GetNumModes ();
                int     countmodes  = 0;
   
                for ( int i = 1; i <= nummodes && countmodes < 2; i++ )
                    if ( histo.GetModeValue ( i ) == maxmode )
                        countmodes++;
                                        // more than 1 max mode
                if ( countmodes > 1 )
                    v   = stat.Median ();   // fallback formula
                else
                    v   = histo.GetMaxPosition ( RealUnit );
//                }
            } // FilterTypeModeQuant

                                        // Entropy
        else if ( filtertype == FilterTypeEntropy ) {
                                        // ?computing a global Kernel Density on all data first?
            histo.ComputeHistogram ( stat, 0, 0, 0, 0, 1, (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear ) );

            v   = histo.GetNumNonEmptyBins ( RealUnit );
            v  *= Log2 ( v + 1 );       // official formula has a log
            } // FilterTypeEntropy


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cooking signed results
        if      ( filterresult == FilterResultNegative )    v   = AtLeast ( 0.0, -v );
        else if ( filterresult == FilterResultPositive )    v   = AtLeast ( 0.0,  v );
        else if ( filterresult == FilterResultAbsolute )    v   = fabs ( v );
//      else if ( filterresult == FilterResultSigned   )    ;   // and default


        GetValue ( x, y, z )    = (TypeD) v;
        } // for y, z
    } // for x

OmpParallelEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterAnisoGaussian     ( FctParams& params, bool showprogress )
{
double              diameter        = params ( FilterParamDiameter );

if ( diameter < 2 )                     // no other voxels involved other than center?
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FctParams           p;


TSuperGauge         Gauge ( FilterPresets[ FilterTypeAnisoGaussian ].Text, showprogress ? Dim1 : 0 );

//int                 gaugen  = 5;
//int                 gaugei  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, always of odd size
TVolume<double>     Kf ( diameter, OddSize );   // the floating point version, used to compute the Kernel
TPointInt           Ko ( Kf.GetDim1 () / 2, Kf.GetDim2 () / 2, Kf.GetDim3 () / 2 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( Kf.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointInt           kp;
double              v;


int                 gaussnumSD      = 2;
TGaussian           gauss ( 0, ( diameter - 1 ) / 2, gaussnumSD );

                                        // compute the Kernel in floating point
for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
for ( int yk = 0; yk < Kf.GetDim2 (); yk++ )
for ( int zk = 0; zk < Kf.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );

    v   = gauss ( kp.Norm () );
//  v   = 1;
                                        // clip Kernel outside Euclidian norm!
    Kf ( xk, yk, zk )   = kp.Norm2 () <= r2 ? v : 0;
    } // for xk, yk, zk

                                        // normalize Kernel
double              sumk            = 0;

for ( int i = 0; i < Kf.GetLinearDim (); i++ )
    sumk       += Kf[ i ];

Kf         /= sumk;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2, Dim3 + Ko.Z * 2 );

temp.Insert ( *this, Ko );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TArray3< TVector3Float >    gradvect ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2, Dim3 + Ko.Z * 2 );


p ( FilterParamDiameter   ) = diameter;
p ( FilterParamResultType ) = FilterResultPositive;
                                        // correct, but will modify current data & display
//FilterPartDeriv ( FilterTypeGradient, p, &gradvect );

temp.FilterPartDeriv ( FilterTypeGradient, p, &gradvect );

//double              maxgradient     = temp.GetMaxValue ();

temp.Insert ( *this, Ko );

                                        // normalize all gradients
for ( int li = 0; li < gradvect.GetLinearDim (); li++ )
    gradvect ( li ).Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelBegin
                                        // private variables
//TPointDouble        d;
TPointDouble        go;
TPointDouble        gk;
//TPointFloat         ortho1;
//TPointFloat         ortho2;
//double              gon;

OmpFor

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();

    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {


        go  = gradvect.GetValue ( x + Ko.X, y + Ko.Y, z + Ko.Z );
//      go.Normalize ();

//        gon     = go.Norm ();
//        go     /= gon;
//        gon    /= maxgradient;

//      dir.GetOrthogonalVectors ( ortho1, ortho2 );


        double      w;
        double      sumw    = 0;
        double      v       = 0;

                                        // scan kernel
        for ( int xki = 0, xk = x; xki < Kf.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y; yki < Kf.GetDim2 (); yki++, yk++ )
        for ( int zki = 0, zk = z; zki < Kf.GetDim3 (); zki++, zk++ )

//            {
//            d.Set ( xki - Ko.X, yki - Ko.Y, zki - Ko.Z );
//            d.Normalize ();
//
//            if ( fabs ( d.ScalarProduct ( go ) ) <= 0.25 ) {
//                w       = Kf ( xki, yki, zki );
//                sumw   += w;
//                v      += w * temp ( xk, yk, zk );
//                }
//            }

            {
            gk  = gradvect.GetValue ( xk, yk, zk );
//          gk.Normalize ();

//          double  wg;
//          wg      = gon * Clip ( gk.ScalarProduct ( go ), 0.0, 1.0 ) + ( 1 - gon ) * 1;   // goes from anisotropic for high gradient, to isotropic for low gradient

            w       = Kf ( xki, yki, zki ) * Clip ( gk.ScalarProduct ( go ), 0.0, 1.0 );  // interesting
            sumw   += w;
            v      += w * temp ( xk, yk, zk );
            }


        if ( sumw )
            Array[ IndexesToLinearIndex ( x, y, z ) ]    = (TypeD) ( v / NonNull ( sumw ) );
        } // for y, z
    } // for x

OmpParallelEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterRank  ( FilterTypes filtertype, FctParams& params )
{
double              threshold       = fabs ( params ( FilterParamThreshold ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFilterRanking<TypeD>   filterrank;

filterrank.Apply ( *this, threshold );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update ranks
if ( filtertype == FilterTypeRankRamp ) {
    
    for ( int i = 0; i < LinearDim; i++ )
                                        // integral of sqrt will be linear -> histogram will be an increasing ramp - looks quite natural actually
        Array[ i ]  =  SignedSquareRoot ( Array[ i ] );

    } // FilterTypeRankRamp
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterHistoEqual ( FctParams& /*params*/, bool showprogress )
{

TSuperGauge         Gauge ( FilterPresets[ FilterTypeHistoEqual ].Text, showprogress ? LinearDim : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get CDF of the whole data, but without nulls (which is nearly equivalent to mask clipping)
THistogram          cdf (   *this,
                            0,
                            0,
                            0,
                            0,  3,      // giving an extra boost here seems to give better results
                            HistogramCDFOptions
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get equalization LUT
TVector<double>     toequ;

cdf.EqualizeCDF ( toequ );

                                        // transform
for ( int i = 0; i < LinearDim; i++ ) {

    Gauge.Next ();

    Array[ i ]  = (TypeD) toequ[ cdf.ToBin ( RealUnit, Array[ i ] ) ];
    }

}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterHistoEquBrain ( FctParams& /*params*/, bool showprogress )
{

TSuperGauge         Gauge ( FilterPresets[ FilterTypeHistoEqualBrain ].Text, showprogress ? LinearDim : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TArray1<double>     values;


GetGreyWhiteValues ( values, false );


//DBGV4 ( values[ BlackMin ], values[ BlackMax ], values[ BlackMode ], values[ BlackW ], "Black: Min Max Mode SD" );
//DBGV4 ( values[ GreyMin  ], values[ GreyMax  ], values[ GreyMode  ], values[ GreyW  ], "Grey : Min Max Mode SD" );
//DBGV4 ( values[ WhiteMin ], values[ WhiteMax ], values[ WhiteMode ], values[ WhiteW ], "White: Min Max Mode SD" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // limit the white on the right side, as there can be a long tail

double          whitemax        = values[ WhiteMode ] + GaussianWidthToSigma ( values[ WhiteW ] ) * 3;
double          ispositive      = GetMinValue () >= 0;
double          maxvalue        = GetMaxValue ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // assigning new values for grey level
                                        // using some linear mapping on 7 segments 

double          normblackmin    =  30; //  25; //  42;
double          normgreymin     =  60; //  50; //  83;
double          normgreymode    = 103; //  85; // 142;
double          normgreymax     = 120; // 100; // 167;
double          normwhitemode   = 150; // 125; // 208;
double          normwhitemax    = 180; // 150; // 250;
double          normmaxvalue    = 210; // 175;

/*
for ( int i = 0; i < LinearDim; i++ )

    if      ( Array[ i ] <= values[ BlackMin ] )    Array[ i ]  = LinearRescale (   Array[ i ], 0,                  values[ BlackMin ], 0,              normblackmin  );
    else if ( Array[ i ] <= values[ GreyMin  ] )    Array[ i ]  = LinearRescale (   Array[ i ], values[ BlackMin ], values[ GreyMin  ], normblackmin,   normgreymin   );
    else if ( Array[ i ] <= values[ GreyMode ] )    Array[ i ]  = LinearRescale (   Array[ i ], values[ GreyMin  ], values[ GreyMode ], normgreymin,    normgreymode  );
    else if ( Array[ i ] <= values[ GreyMax  ] )    Array[ i ]  = LinearRescale (   Array[ i ], values[ GreyMode ], values[ GreyMax  ], normgreymode,   normgreymax   );
    else if ( Array[ i ] <= values[ WhiteMode ] )   Array[ i ]  = LinearRescale (   Array[ i ], values[ GreyMax  ], values[ WhiteMode], normgreymax,    normwhitemode );
    else if ( Array[ i ] <= whitemax           )    Array[ i ]  = LinearRescale (   Array[ i ], values[ WhiteMode], whitemax,           normwhitemode,  normwhitemax  );
    else                                            Array[ i ]  = normwhitemax + 1;
*/

OmpParallelFor

for ( int i = 0; i < LinearDim; i++ ) {

    Gauge.Next ();

    double              v;

    if      ( Array[ i ] == 0                   )   v   = 0;    // always force remapping 0 to 0, otherwise the background will look weird
    else if ( Array[ i ] <= values[ BlackMin  ] )   v   = LinearRescaleNoise (   Array[ i ], 0,                  values[ BlackMin ], 0,              normblackmin  );
    else if ( Array[ i ] <= values[ GreyMin   ] )   v   = LinearRescaleNoise (   Array[ i ], values[ BlackMin ], values[ GreyMin  ], normblackmin,   normgreymin   );
    else if ( Array[ i ] <= values[ GreyMode  ] )   v   = LinearRescaleNoise (   Array[ i ], values[ GreyMin  ], values[ GreyMode ], normgreymin,    normgreymode  );
    else if ( Array[ i ] <= values[ GreyMax   ] )   v   = LinearRescaleNoise (   Array[ i ], values[ GreyMode ], values[ GreyMax  ], normgreymode,   normgreymax   );
    else if ( Array[ i ] <= values[ WhiteMode ] )   v   = LinearRescaleNoise (   Array[ i ], values[ GreyMax  ], values[ WhiteMode], normgreymax,    normwhitemode );
    else if ( Array[ i ] <= whitemax            )   v   = LinearRescaleNoise (   Array[ i ], values[ WhiteMode], whitemax,           normwhitemode,  normwhitemax  );
    else                                            v   = LinearRescaleNoise (   Array[ i ], whitemax,           maxvalue,           normwhitemax,   normmaxvalue  );

                                        // avoid negative value if original data was only positive
    if ( ispositive )
        Maxed ( v, 0.0 );

    Array[ i ]  = v;
    }

}


//----------------------------------------------------------------------------
                                        // Not working for float data, code was designed for uchar data, hence the discrete compaction
template <class TypeD>
void    TVolume<TypeD>::FilterHistoCompact ( FctParams& params, bool showprogress )
{
double              percentignored  = Clip ( params ( FilterParamThreshold ), (double) 0, (double) 100 ) / 100;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeHistoCompact ].Text, showprogress ? LinearDim : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get full histogram
THistogram          H   (  *this,
                            0,
                            0,
                            0,
                            0,  1, 
                            (HistogramOptions) ( HistogramPDF /*| HistogramSmooth*/ | HistogramNormNone | HistogramLinear )
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute max without bin 0!
double              lowmax          = 0;

for ( int b = 1; b < (int) H; b++ )
    Maxed ( lowmax, H[ b ] );


TArray1<int>        topack;
TArray1<int>        tounpack;


H.Pack ( topack, tounpack, percentignored * lowmax );


OmpParallelFor

for ( int i = 0; i < LinearDim; i++ ) {

    Gauge.Next ();

    Array[ i ]  = (TypeD) topack[ Array[ i ] ];
    }

}


//----------------------------------------------------------------------------
                                        // Homogenize with tomographic-like correction of sliding Histograms statistics
template <class TypeD>
bool    TVolume<TypeD>::FilterBiasField (   FctParams& params,
                                            TVolume<float>*     corr,
                                            bool                showprogress )
{
                                        // set global parameters
int                 numcorrections  = (int) Clip ( params ( FilterParamBiasFieldRepeat ), (double) 1, (double) 100 );

                                        // needs some downsampling?
int                 downsampling    = AtLeast ( 1, Round ( (double) MaxSize () / 128 ) );   // 64..128..191 -> 1, 192..256..383 -> 2 etc...
//  downsampling     = GetValue ( "Force the Downsampling ratio to:", "MRI", this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Limit the scan to the interesting inside part
TVolume<TypeD>      mask ( *this );
FctParams           p;

p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
mask.Filter ( FilterTypeToMask, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.0) Scan a set of directions, build function based on the histogram of a sliding window

TBoundingBox<int>   bounding ( *this, false, 0 );
                                        // estimated size of deepest sulcus * 2, to have thick enough histogram with enough white inside
int                 halfthick       = Round ( bounding.GetMeanExtent () * 0.15 );
//int                 halfthick       = ::GetValue ( "Half thickness:", FilterPresets[ FilterTypeBiasField ].Text );

//DBGV3 ( bounding.GetMeanExtent (), halfthick, boundarymargin, "MeanExtent -> halfthick boundarymargin" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.1) Create a set of directions that covers a sphere

#define             numicosahedron      12
#define             numdodecahedron     20

//int                 maxaxis         = numicosahedron;                 // icosahedron (12 vertices / 20 triangles / 30 edges)
int                 maxaxis         = numicosahedron + numdodecahedron; // icosahedron + dodecahedron (20 vertices / 12 triangles / 30 edges)
bool                usehalfdir      = false; // true;
int                 numaxis         = usehalfdir ? maxaxis / 2 : maxaxis;
TPoints             dir      ( maxaxis );

enum                BoundaryEnum
                    {
                    BoundaryInferior    = 0,
                    BoundarySuperior,
                    BoundaryInferiorMargin,
                    BoundarySuperiorMargin,
                    NumBoundaryArray,
                    };
TArray2<int>        boundary ( maxaxis, NumBoundaryArray );
int                 v               = 0;
double              phi             = ( 1 + sqrt ( 5.0 ) ) / 2;


int                 maxlength       =   bounding.BoxDiameter () + 2 * halfthick + 1.5;      // max bounding diameter + histogram thickness
int                 maxradius       = ( bounding.BoxDiameter () / 2 / sqrt ( 3.0 ) ) * 0.5; // mean radius * 0.5: brains are convex so it is fair enough to scan a more central part, when looking for extrma of a given axis
TPointFloat         center ( &bounding.GetCenter () );
int                 centeri         = maxlength / 2;

                                        // icosahedron
dir[ v++ ].Set (   0,   1,   phi );
dir[ v++ ].Set (   0,   1,  -phi );
dir[ v++ ].Set (   1,  phi,    0 );
dir[ v++ ].Set (   1, -phi,    0 );
dir[ v++ ].Set ( phi,    0,    1 );
dir[ v++ ].Set ( phi,    0,   -1 );
                                        // other half
for ( int i = 0; i < numicosahedron / 2; i++ )
    dir[ v++ ].Set ( - dir[ i ].X, - dir[ i ].Y, - dir[ i ].Z );


/*
                                        // dodecahedron
dir[ v++ ].Set (           1,          1,          1 );
dir[ v++ ].Set (           1,          1,         -1 );
dir[ v++ ].Set (           1,         -1,          1 );
dir[ v++ ].Set (           1,         -1,         -1 );
dir[ v++ ].Set (           0,    1 / phi,        phi );
dir[ v++ ].Set (           0,    1 / phi,       -phi );
dir[ v++ ].Set (     1 / phi,        phi,          0 );
dir[ v++ ].Set (     1 / phi,       -phi,          0 );
dir[ v++ ].Set (         phi,          0,    1 / phi );
dir[ v++ ].Set (         phi,          0,   -1 / phi );
                                        // other half
for ( int i = 0; i < numdodecahedron / 2; i++ )
    dir[ v++ ].Set ( - dir[ i ].X, - dir[ i ].Y, - dir[ i ].Z );
*/

                                        // subdivide to middle of triangles: Icosahedron -> Dodecahedron (its dual)
double              edgelength      = ( dir[ 0 ] - dir[ 2 ] ).Norm () + 1e-3;

                                        // scan for each (equilateral) triangles
for ( int i = 0;     i < numicosahedron; i++ )
for ( int j = i + 1; j < numicosahedron; j++ )

    if ( ( dir[ j ] - dir[ i ] ).Norm () <= edgelength )

        for ( int k = j + 1; k < numicosahedron; k++ )

            if ( ( dir[ k ] - dir[ i ] ).Norm () <= edgelength
              && ( dir[ k ] - dir[ j ] ).Norm () <= edgelength
              && v < maxaxis )
                dir[ v++ ]  = dir[ i ] + dir[ j ] + dir[ k ];


                                        // normalize directions
dir.Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeBiasField ].Text, showprogress ? numcorrections * numaxis : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.2) Keep only half of the all directions, as they are opposite and already scanned
TPoints             usedir;


if ( usehalfdir ) {

    usedir.Resize ( numaxis );
    v   = 0;

    for ( int i = 0; i < maxaxis; i++ )
        if ( (        dir[ i ].X   > 0
            || fabs ( dir[ i ].X ) < 1e-3 &&        dir[ i ].Y   > 0
            || fabs ( dir[ i ].X ) < 1e-3 && fabs ( dir[ i ].Y ) < 1e-3 && dir[ i ].Z > 0 )
          && v < numaxis )

            usedir[ v++ ]   = dir[ i ];

                                        // switch to subset directions
    dir     = usedir;
    }
else                                    // use all directions
    usedir  = dir;


//dir.WriteFile ( "E:\\Data\\Inhomogeneities Directions Icosahedron + Dodecahedron.spi" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.3) Re-order the directions
                                        // looking for the most orthogonal sequence of 3 consecutive vectors
TArray1<bool>       dirdone ( numaxis );
TPointFloat         dir1;
TPointFloat         dir2;
int                 mini;
double              minangle;
double              angle;


dirdone         = false;
v               = 0;

                                        // first dir
dir[ v++ ]      = usedir[ 0 ];
dir1            = usedir[ 0 ];
dirdone[ 0 ]    = true;

                                        // second dir
minangle        = 100;
mini            = -1;
for ( int i = 0; i < numaxis; i++ ) {
    if ( dirdone[ i ] )     continue;

    angle   = fabs ( usedir[ i ].ScalarProduct ( dir1 ) );

    if ( angle < minangle ) {
        minangle    = angle;
        mini        = i;
        }
    }

dir[ v++ ]      = usedir[ mini ];
dir2            = usedir[ mini ];
dirdone[ mini ] = true;

                                        // all remaining dirs
do {
    minangle        = 100;
    mini            = -1;

    for ( int i = 0; i < numaxis; i++ ) {
        if ( dirdone[ i ] )     continue;
                                        // we're fortunate that the cosine already accounts for a Pi rotation!
        angle   = max ( fabs ( usedir[ i ].ScalarProduct ( dir1 ) ), fabs ( usedir[ i ].ScalarProduct ( dir2 ) ) );

        if ( angle < minangle ) {
            minangle    = angle;
            mini        = i;
            }
        }

    dir[ v++ ]      = usedir[ mini ];
    dir1            = dir2;
    dir2            = usedir[ mini ];
    dirdone[ mini ] = true;

    } while ( v < numaxis );


//dir.WriteFile ( "E:\\Data\\Inhomogeneities Directions Icosahedron + Dodecahedron.Ortho.spi" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.4) Scan for radial limits in each axis, until a perpendicular plane touch the data

double              boundarymargin  = halfthick;    // start inside, to avoid artificial decay on border due to missing part
//double              boundarymargin  = halfthick * ::GetValue ( "Margin %:", FilterPresets[ FilterTypeBiasField ].Text ) / 100;

OmpParallelFor

for ( int axis = 0; axis < numaxis; axis++ ) {

//    Gauge.Next ( another part up to numaxis );


    boundary ( axis, BoundaryInferior )         = 0;
    boundary ( axis, BoundarySuperior )         = 0;


    TPointFloat         p1;
    TPointFloat         p2;

    dir[ axis ].GetOrthogonalVectors ( p1, p2 );

                                        // "inferior" limit
    for ( int d =  centeri;  d >= 0          && ! boundary ( axis, BoundaryInferior ); d--               )
    for ( int w = -maxradius; w <= maxradius && ! boundary ( axis, BoundaryInferior ); w += downsampling )
    for ( int h = -maxradius; h <= maxradius;                                          h += downsampling ) {

        TPointFloat         pos         = center - dir[ axis ] * d + p1 * w + p2 * h;

        if ( WithinBoundary ( pos ) && mask ( pos ) ) {
//          boundary ( axis, BoundaryInferior ) = -d;
            boundary ( axis, BoundaryInferior ) = -d + boundarymargin * 0.25;   // limit the overshoot at the edge
            break;
            }
        }
                                        // "superior" limit
    for ( int d =  centeri;  d >= 0          && ! boundary ( axis, BoundarySuperior ); d--               )
    for ( int w = -maxradius; w <= maxradius && ! boundary ( axis, BoundarySuperior ); w += downsampling )
    for ( int h = -maxradius; h <= maxradius;                                          h += downsampling ) {

        TPointFloat         pos         = center + dir[ axis ] * d + p1 * w + p2 * h;

        if ( WithinBoundary ( pos ) && mask ( pos ) ) {
//          boundary ( axis, BoundarySuperior ) = d;
            boundary ( axis, BoundarySuperior ) = d - boundarymargin * 0.25;    // limit the overshoot at the edge
            break;
            }
        }


    boundary ( axis, BoundaryInferiorMargin )   = boundary ( axis, BoundaryInferior ) + boundarymargin;
    boundary ( axis, BoundarySuperiorMargin )   = boundary ( axis, BoundarySuperior ) - boundarymargin;

//    DBGV5 ( axis + 1, boundary ( axis, BoundaryInferior ), boundary ( axis, BoundarySuperior ), boundary ( axis, BoundaryInferiorMargin ), boundary ( axis, BoundarySuperiorMargin ), "Axis -> boundaries" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.0) Scan around the brain, using our computed set of directions

TVolume<double>     correction ( Dim1, Dim2, Dim3 );
TVector<double>     axiscorr ( maxlength );

int                 hperpmax            = 1000; // this will give the granularity of one axis' correction
THistogram          Hperp ( hperpmax + 1 );
double              hperprescale;
double              hperpminperc        = 0.75; // 0.85;    // it seems we can go for a broader range(?)
double              hperpmaxperc        = 0.93; // 0.95;

TEasyStats          stat;
TEasyStats          globalquality;

//double              backvalue       = GetBackgroundValue ();

                                        // global correction = none
correction      = 1;                    // multiplicative correction
//correction    = 0;                    // additive correction


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.1) Get a measure of "size" from the original data
                                        // to be used for the final rescaling
//THistogram          Horig   (   *this,
//                                &mask,
//                                0,
//                                0,
//                                0,  1,
//                                (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
//                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.2) Loop in axis + repeat

for ( int c = 0, axis; c < numcorrections * numaxis; c++ ) {

    Gauge.Next ();

                                        // get current axis
    axis        = c % numaxis;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local rescale to have the maximum possible grey levels resolution
    stat.Reset ();

    for ( int i = 0; i < LinearDim; i++ )
        if ( mask ( i ) )
            stat.Add ( Array[ i ] * correction ( i ) );

    hperprescale    = stat.Max () ? hperpmax / stat.Max () : 1;

//    stat.Show ( "Intermediate Correction" );
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute correction for current axis
    axiscorr    = 0;

                                        // compute sliding histogram on current axis
                                        // seems better between BoundaryInferior..BoundarySuperior
    for ( int si = boundary ( axis, BoundaryInferior /*BoundaryInferiorMargin*/ ); si <= boundary ( axis, BoundarySuperior /*BoundarySuperiorMargin*/ ); si++ ) {

                                        // histogram perpendicaluar to current axis
        Hperp.Reset ();

        OmpParallelFor

        for ( int x = bounding.XMin (); x <= bounding.XMax (); x += downsampling )
        for ( int y = bounding.YMin (); y <= bounding.YMax (); y += downsampling )
        for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z += downsampling ) {

            if ( mask     ( x, y, z )       // skipping outside mask
              && GetValue ( x, y, z ) ) {   // skipping null values
                                        // slice origin
                TPointFloat     pos     = center + dir[ axis ] * si;
                                        // distance inside slice
                double          d       = fabs ( ( TPointFloat ( x, y, z ) - pos ).ScalarProduct ( dir[ axis ] ) );

                if ( d <= halfthick )
                    Hperp[ Round ( GetValue ( x, y, z ) * correction ( x, y, z ) * hperprescale ) ]++;
                }
            } // for x, y, z


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // process histogram to extract a measure of global intensity

                                        // working formula
//        Hperp.Smooth ();                                        // yep, that helps in some cases
//        Hperp.Erode ( 0.12 );                                   // don't erode too much, or we may lose tracks of the high intensities!
//        axiscorr[ centeri + si ]   = Hperp.GetLastPosition ();  // works also if already corrected

                                        // combine two measures for a more robust measure; the second part exagerates a bit the border, which is desirable for us
//        Hperp.Smooth ();                                        // yep, that helps in some cases
//        Hperp.Erode ( 0.12 );                                   // don't erode too much, or we may lose tracks of the high intensities!
//        axiscorr[ centeri + si ]   = Hperp.GetLastPosition () * 0.67 + Hperp.GetPercentilePosition ( 0.90 ) * 0.33;

                                        // do a better smoothing
        p ( FilterParamDiameter )     = GaussianSigmaToWidth ( hperpmax * 0.05 ); // shouldn't it be proportional to Kernel Density within a slice of data?
        Hperp.Filter ( FilterTypeGaussian, p );                 // we might have empty bins, so the Fast Gaussian will not perform that well


//        TFileName               _file;
//        sprintf ( _file, "E:\\Data\\Histogram.%0d.%03d.PDF.sef", axis, si );
//        if ( ( c + si ) % 37 == 0 )
//            Hperp.WriteFile ( _file, "Axis" );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // looking at the tail of the CDF
        Hperp.ToCDF ();


        stat.Reset ();
                                        // cumulate position within  hperpminperc..hperpmaxperc  range
        for ( int i = 0; i < Hperp.GetDim () && Hperp[ i ] <= hperpmaxperc; i++ )
            if ( Hperp[ i ] >= hperpminperc )
                stat.Add ( i );

                                        // Or convert to CDF and scan range myself
        axiscorr[ centeri + si ]   = stat.Mean ();


//        if ( VkQuery () ) DBGV3 ( axis, si, axiscorr[ centeri + si ], "Axis, Perp -> Corr" );
//        TFileName               _file;
//        sprintf ( _file, "E:\\Data\\Histogram.%0d.%03d.CDF.sef", axis, si );
//        if ( ( c + si ) % 37 == 0 )
//            Hperp.WriteFile ( _file, "Axis" );
        } // for sliding histogram


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill edges with cste value (linear interpolation doesn't seem to work)
                                        // seems better between BoundaryInferior..BoundarySuperior
    for ( int i = 0; i < centeri + boundary ( axis, BoundaryInferior /*BoundaryInferiorMargin*/ ); i++ )
        axiscorr[ i ]   = axiscorr[ centeri + boundary ( axis, BoundaryInferior /*BoundaryInferiorMargin*/ ) ];


    for ( int i = maxlength - 1; i > centeri + boundary ( axis, BoundarySuperior /*BoundarySuperiorMargin*/ ); i-- )
        axiscorr[ i ]   = axiscorr[ centeri + boundary ( axis, BoundarySuperior /*BoundarySuperiorMargin*/ ) ];
    
                                        // do some heavy filtering on the current axis
    p ( FilterParamDiameter )     = GaussianSigmaToWidth ( halfthick );
    axiscorr.Filter ( FilterTypeGaussian, p );      // we might have empty bins, so the Fast Gaussian will not perform that well


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get stats from axis correction
    stat.Reset ();
    for ( int i = boundary ( axis, BoundaryInferior /*BoundaryInferiorMargin*/ ); i <= boundary ( axis, BoundarySuperior /*BoundarySuperiorMargin*/ ); i++ )
        stat.Add ( axiscorr[ centeri + i ] );

                                        // store all SDs
    globalquality.Add ( stat.SD () );

                                        // !these constants are relative to the rescaling!
    bool                badstats            = stat.SD () > ( hperpmax / 17 ) * 1.5 
                                           || stat.SD () == 0;

                                        // save axis curves to file
//    TFileName           _file;
//    sprintf ( _file, "E:\\Data\\Axis Correction.%02d.sef", c + 1 );
//    axiscorr.WriteFile ( _file );
//
//    if ( VkQuery () ) DBGV4 ( axis, stat.Average (), stat.SD (), badstats, "axis  avg SD -> badstats" );

                                        // don't buy that guy, there is obviously a problem; and BTW, SD == 0 means no correction
    if ( badstats ) {
//        if ( stat.SD () )   DBGV2 ( axis, stat.SD (), "Axis correction pb: SD" );
        continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute local correction from measure
                                        // then merge it into global correction
    OmpParallelFor

    for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
    for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
    for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

//      if ( mask ( x, y, z ) ) {       // off: current axis is applied on the whole volume

            {
                                        // distance inside slice
            double      d       = centeri + ( TPointFloat ( x, y, z ) - center ).ScalarProduct ( dir[ axis ] );

            if ( d >= 0 && d < maxlength )
                correction ( x, y, z ) /= NonNull ( axiscorr[ (int) d ] );    // ?interpolating between 2 bins?
//              correction ( x, y, z ) += avgcorr - axiscorr[ (int) d ];
            }
        }  // for x, y, z

                                        // intermediate smoothing?
//  p ( FilterParamDiameter )     = 3.47;
//  correction.Filter ( FilterTypeFastGaussian, p );


                                        // compute the current mean correction only within the mask
    stat.Set ( correction, &mask, false, false );

    double          avgrescale      = NonNull ( stat.Mean () );

                                        // adjust global correction so that its mean == 1
    OmpParallelFor

    for ( int i = 0; i < LinearDim; i++ )
        if ( mask ( i ) )   correction  ( i )   /= avgrescale;
        else                correction  ( i )    = 1;


//    TFileName           _file;
//    sprintf ( _file, "E:\\Data\\Axis Correction.%02d.nii", c + 1 );
//    correction.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
    } // for c numcorrections


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Average:
                                        // 0.25 .. 2    OK
                                        // 5 .. 20      error

                                        // SD:
                                        // 0 .. 2   OK
                                        // 5        error

                                        // * 1.5 with correct range of boundary
bool                transfok        = globalquality.Average () < ( hperpmax / 64 ) * 1.5
                                   && globalquality.SD ()      < ( hperpmax / 51 ) * 1.5;

//globalquality.Show ( "Bias Field global quality" );
//DBGV4 ( globalquality.Average (), globalquality.SD (), globalquality.CoV (), transfok, "globalquality of SDs: avg, SD, CoV -> transfok" );

                                        // don't apply if transformation seem wrong
if ( ! transfok ) {
//    ShowMessage ( "Transformation seems to be erroneous, and will not be applied.", FilterPresets[ FilterTypeBiasField ].Text, ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Compute final rescaling - now off, as normalizing correction at each steps is enough
//stat.Set  ( *this, &mask, true );
//stat.Show ( "Data only" );

//stat.Set  ( correction, &mask, true );
//stat.Show ( "Correction only" );

//stat.Resize ( mask.GetNumSet () );
//for ( int i = 0; i < LinearDim; i++ )
//    if ( mask ( i ) )
////  if ( mask ( i ) && Array[ i ] ) 
//        stat.Add ( Array[ i ] * correction ( i ) );
//stat.Show ( "Data * Correction" );


/*
TVolume<double>     correctionfixed ( correction );


correction.WriteFile ( "E:\\Data\\BFC.Final Correction.nii", 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );

                                        // a bit of smoothing?
p ( FilterParamDiameter )     = 30;
correction.Filter ( FilterTypeFastGaussian, p );

for ( int i = 0; i < LinearDim; i++ )
    if ( mask ( i ) )   correction  ( i )    = correctionfixed  ( i );

correction.WriteFile ( "E:\\Data\\BFC.Final Correction.Gauss+Mask.1.nii", 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );


p ( FilterParamDiameter )     = 30;
correction.Filter ( FilterTypeFastGaussian, p );

for ( int i = 0; i < LinearDim; i++ )
    if ( mask ( i ) )   correction  ( i )    = correctionfixed  ( i );

correction.WriteFile ( "E:\\Data\\BFC.Final Correction.Gauss+Mask.2.nii", 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );


p ( FilterParamDiameter )     = 30;
correction.Filter ( FilterTypeFastGaussian, p );

for ( int i = 0; i < LinearDim; i++ )
    if ( mask ( i ) )   correction  ( i )    = correctionfixed  ( i );

correction.WriteFile ( "E:\\Data\\BFC.Final Correction.Gauss+Mask.3.nii", 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally returning the correction factors
if ( corr ) {

    corr->Resize ( correction.GetDim1 (), correction.GetDim2 (), correction.GetDim3 () );

    OmpParallelFor

    for ( int i = 0; i < LinearDim; i++ )

//      (*corr)( i )    = correction ( i ); // ?this crashes - but warum?
//      (*corr)( i )    = ( correction ( i ) > 0 ? 1 / ( correction ( i ) ) : 0 ) * 100;    // OK for output
        (*corr)( i )    = ( correction ( i ) > 0 ? 1 / ( correction ( i ) ) : 0 );          // returning the inverse of correction does work...
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Apply global correction + final rescaling
OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
//  if ( mask ( i ) )
        Array[ i ]  = Array[ i ] * correction ( i );
//      Array[ i ]  = Array[ i ] + correction ( i );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // output estimated bias field into volume, * 100
TFileName           _file;
TVolume<TypeD>      bfcvol ( *this );

OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
//  bfcvol ( i )    = mask ( i ) ? correction ( i ) ? 1 / correction ( i ) : 0 : 0;
//  bfcvol ( i )    = mask ( i ) ? 100 - correction ( i ) : 0;
    bfcvol ( i )    = correction ( i ) ? 1 / correction ( i ) : 0;

sprintf ( _file, "E:\\Data\\Bias Field Correction." DefaultMriExt );
bfcvol.WriteFile ( _file );
*/

return  true;
}


//----------------------------------------------------------------------------
                                        // Feature extraction, like edge & curvature
template <class TypeD>
void    TVolume<TypeD>::FilterPartDeriv ( FilterTypes filtertype, FctParams& params, TArray3< TVector3Float > *vectres, bool showprogress )
{
int                 size            =                     (int) params ( FilterParamDiameter ); // use an integer size, as the Differences work on integer steps
FilterResultsType   filterresult    = (FilterResultsType) (int) params ( FilterParamResultType );
int                 Cratio          =                     (int) params ( FilterParamCRatio );

                                        // force to odd size (3, 5, 7..)
if ( IsEven ( size ) )
    size++;


int                 size2           = max ( 1, size / 2 );
FctParams           p;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned, temp array
TVolume<TypeD>      temp ( *this );

p ( FilterParamDiameter )     = size;
temp.Filter ( FilterTypeFastGaussian /*FilterTypeGaussian*/, p, showprogress );

double              backvalue       = temp.GetBackgroundValue ();
//double              maxvalue        = max ( (TypeD) 1, temp.GetMaxValue () );

//Cratio      = filtertype == FilterTypeKCCurvature ? Power ( maxvalue / 12, max ( 5 - size2, 1 ) ) : 1;    // heuristic value - not tuned up

//double              mineigen1       = ::GetValue ( "Min for eigenvalue 1:", FilterPresets[ filtertype ].Text );
//double              mineigen3       = ::GetValue ( "Min for eigenvalue 3:", FilterPresets[ filtertype ].Text );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional mask for backgound values outside of head
                                        // could be an option...
TVolume<TypeD>      mask;

if ( ! ( filtertype == FilterTypeGradient || filtertype == FilterTypeCanny ) ) {

    mask        = *this;

    p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
    p ( FilterParamToMaskNewValue  )     = 1;
    p ( FilterParamToMaskCarveBack )     = true;
    mask.Filter ( FilterTypeToMask, p );

                                        // give more room on the side to allow correct differentials
    p ( FilterParamDiameter )     = size2 + 1;
    mask.Filter ( FilterTypeDilate, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );

OmpParallelBegin
                                        // private variables
TPointFloat         g1;
TPointFloat         g2;
double              v;
double              d[ NumDiffs ];
double              C;
double              Cavg;
AMatrix33           m33;
AVector3            D;

OmpFor

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();


    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {

                                        // skip background
        if ( mask.IsAllocated ()
          && temp ( x, y, z ) < backvalue && ! mask ( x, y, z ) ) {
            Array[ IndexesToLinearIndex ( x, y, z ) ]    = 0;
            continue;
            }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if ( filtertype == FilterTypeGradient ) {

            temp.GetDifferences ( x, y, z, d, size2 );

            v   = sqrt ( d[ DiffDx ] * d[ DiffDx ] + d[ DiffDy ] * d[ DiffDy ] + d[ DiffDz ] * d[ DiffDz ] );

            if ( vectres )
                vectres->GetValue ( x, y, z ).Set ( d[ DiffDx ], d[ DiffDy ], d[ DiffDz ] );
            }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( filtertype == FilterTypeCanny ) {

            temp.GetDifferences ( x, y, z, d, size2 );

            v   = sqrt ( d[ DiffDx ] * d[ DiffDx ] + d[ DiffDy ] * d[ DiffDy ] + d[ DiffDz ] * d[ DiffDz ] );
            }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( filtertype == FilterTypeLaplacian ) {

            temp.GetDifferences ( x, y, z, d, size2 );

            v   = d[ DiffDxx ] + d[ DiffDyy ] + d[ DiffDzz ];

            v   = -v;                   // !minus Laplacian actually!
            } // FilterTypeLaplacian

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( filtertype == FilterTypeHessianEigenMax
               || filtertype == FilterTypeHessianEigenMin ) {

            temp.GetDifferences ( x, y, z, d, size2 );


            if ( d[ DiffDxx ] || d[ DiffDyy ] || d[ DiffDzz ] ) {

                                        // compose Hessian matrix
                m33 ( 0, 0 )    =                   d[ DiffDxx ];
                m33 ( 1, 1 )    =                   d[ DiffDyy ];
                m33 ( 2, 2 )    =                   d[ DiffDzz ];
                m33 ( 0, 1 )    = m33 ( 1, 0 )  =   d[ DiffDxy ];
                m33 ( 0, 2 )    = m33 ( 2, 0 )  =   d[ DiffDxz ];
                m33 ( 1, 2 )    = m33 ( 2, 1 )  =   d[ DiffDyz ];

                                        // eigenvalues are not always sorted, could be negative, and third value could also be 0
                                        // ascending signed values order
                AEigenvalues33DDTT ( m33, D );

                D[ 0 ]  = fabs ( D[ 0 ] );
                D[ 1 ]  = fabs ( D[ 1 ] );
                D[ 2 ]  = fabs ( D[ 2 ] );

                                        // interesting: the Determinant of the Hessian close to 0 -> noise / background
//              double  det     = arma::det ( m33 );

                if      ( filtertype == FilterTypeHessianEigenMax   )   v   = max ( D[ 0 ], D[ 1 ], D[ 2 ] ); // / maxvalue;
                else if ( filtertype == FilterTypeHessianEigenMin   )   v   = min ( D[ 0 ], D[ 1 ], D[ 2 ] ); // / maxvalue;
    //          else if ( filtertype == FilterTypeHessianEigenLapl  )   v   = ( D[ 0 ] + D[ 1 ] + D[ 2 ] ) / maxvalue;    // Laplacian
                }
            else
                v   = 0;
            } // FilterTypeHessian

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( filtertype == FilterTypeKCurvature ) {
                                        // Divergence of the normalized gradient field (!= Laplacian)

            temp.GetGradient ( x - 1,   y,      z,      g1, size2 );
            g1.Normalize ();
            temp.GetGradient ( x + 1,   y,      z,      g2, size2 );
            g2.Normalize ();
            v   = g2.X - g1.X;

            temp.GetGradient ( x,       y - 1,  z,      g1, size2 );
            g1.Normalize ();
            temp.GetGradient ( x,       y + 1,  z,      g2, size2 );
            g2.Normalize ();
            v  += g2.Y - g1.Y;

            temp.GetGradient ( x,       y,      z - 1,  g1, size2 );
            g1.Normalize ();
            temp.GetGradient ( x,       y,      z + 1,  g2, size2 );
            g2.Normalize ();
            v  += g2.Z - g1.Z;

                                        // normalizing: 3 sums that can each be up to +-2
            v      /= -6;
            } // FilterTypeKCurvature

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( filtertype == FilterTypeKCCurvature ) {
                                        // Divergence of the principal gradient field, weighted by confidence measure

            temp.GetPrincipalGradient ( x - 1,   y,      z,      g1, size2, C );
            Cavg    = C;
            temp.GetPrincipalGradient ( x + 1,   y,      z,      g2, size2, C );
            Cavg   += C;
            v       = g2.X - g1.X;

            temp.GetPrincipalGradient ( x,       y - 1,  z,      g1, size2, C );
            Cavg   += C;
            temp.GetPrincipalGradient ( x,       y + 1,  z,      g2, size2, C );
            Cavg   += C;
            v      += g2.Y - g1.Y;

            temp.GetPrincipalGradient ( x,       y,      z - 1,  g1, size2, C );
            Cavg   += C;
            temp.GetPrincipalGradient ( x,       y,      z + 1,  g2, size2, C );
            Cavg   += C;
            v      += g2.Z - g1.Z;

                                        // compute the confidence C
            Cavg   /= 6;
            C       = 1 - expl ( - Square ( Cavg / Cratio ) / 2 );

                                        // normalizing: 3 sums that can each be up to +-2
            v      *= C / -6;
            } // FilterTypeKCCurvature

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // process value according to desired result
        if      ( filterresult == FilterResultNegative )    v   = AtLeast ( 0.0, -v );
        else if ( filterresult == FilterResultPositive )    v   = AtLeast ( 0.0,  v );
        else if ( filterresult == FilterResultAbsolute )    v   = fabs ( v );
//      else if ( filterresult == FilterResultSigned   )    ;   // and default


        Array[ IndexesToLinearIndex ( x, y, z ) ]    = (TypeD) v;
        } // for y, z
    } // for x

OmpParallelEnd
}


//----------------------------------------------------------------------------
                                        // Scan from the outside
                                        // Erase while below background
                                        // Erase while increasing, stop at decreasing
template <class TypeD>
void    TVolume<TypeD>::FilterPeelingRadius ( TPointFloat &p, TPointFloat &center, double &backthreshold, TVolume<TypeD> &temp )
{
TPointFloat         dir;
int                 iter;
int                 maxiter;
int                 previous;
int                 current;
TPointFloat         p1;
TPointFloat         p2;

//SetOvershootingOption ( interpolate, Array, LinearDim, true );

                                        // get direction from voxel to center
dir         = center - p;
                                        // set a limit to the # of iterations
maxiter     = Truncate ( dir.Norm () );

dir        /= maxiter;

dir.GetOrthogonalVectors ( p1, p2 );

//center.Show ( "center" );
//p.Show ( "border" );
//dir.Show ( "dir" );

                                        // 1) grignoting from the surface, below the threshold
for ( iter = 0; iter < maxiter; iter++, p += dir ) {

//    current     = temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );
                                        // look at all neighbors, get the max -> less agressive erosion
    current     =                (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );
    p  += p1;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p1 * -2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p1 + p2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p2 * -2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p2;


    if ( current <= backthreshold ) (*this)[ p + 0.5 ]  = 0;
    else                            break;
    }


if ( iter == maxiter )
    return;

                                        // 2) delete white->grey gradient
//current     = temp.GetValueChecked ( p.X - dir.X, p.Y - dir.Y, p.Z - dir.Z, InterpolateLinear );

for ( ; iter < maxiter; iter++, p += dir ) {

    previous    = current;
    current     = (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );
/*
    current     =                      temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );
    p  += p1;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p1 * -2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p1 + p2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p2 * -2;
    current     = max ( current, (int) temp.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) );
    p  += p2;
*/

    if      ( previous == current )  continue;
    else if ( previous > current )  (*this)[ p + 0.5 ]  = 0;
    else                            break;
    }

}

                                        // Not finalized, not used
template <class TypeD>
void    TVolume<TypeD>::FilterPeeling ( FctParams& params, bool showprogress )
{
double              backthreshold   = params ( FilterParamThreshold );

bool                safe            = true;

TPointInt           step;
TPointInt           boundmin;
TPointInt           boundmax;
TPointInt           limitmin;
TPointInt           limitmax;
TPointFloat         p;
TPointFloat         center;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned, temp array
TVolume<TypeD>      temp ( *this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( safe ) {
    boundmin.Set ( 1, 1, 1 );
    boundmax.Set ( Dim1 - 2, Dim2 - 2, Dim3 - 2 );
    }
else {
    boundmin.Set ( 0, 0, 0 );
    boundmax.Set ( Dim1 - 1, Dim2 - 1, Dim3 - 1 );
    }

//if ( step == 0 )
    step.Set ( 1, 1, 1 );


center  = TBoundingBox<int> ( *this, false, 0.1 ).GetCenter ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypePeeling ].Text, showprogress ? 7 : 0 );

                                        // scan from each side of the cube
Gauge.Next ();

for ( int x = boundmin.X; x <= boundmax.X; x += step.X )
for ( int y = boundmin.Y; y <= boundmax.Y; y += step.Y )
    FilterPeelingRadius ( TPointFloat ( (float) x, (float) y, boundmin.Z ), center, backthreshold, temp );


Gauge.Next ();

for ( int x = boundmin.X; x <= boundmax.X; x += step.X )
for ( int y = boundmin.Y; y <= boundmax.Y; y += step.Y )
    FilterPeelingRadius ( TPointFloat ( (float) x, (float) y, boundmax.Z ), center, backthreshold, temp );


Gauge.Next ();

for ( int x = boundmin.X; x <= boundmax.X; x += step.X )
for ( int z = boundmin.Z; z <= boundmax.Z; z += step.Z )
    FilterPeelingRadius ( TPointFloat ( (float) x, boundmin.Y, (float) z ), center, backthreshold, temp );


Gauge.Next ();

for ( int x = boundmin.X; x <= boundmax.X; x += step.X )
for ( int z = boundmin.Z; z <= boundmax.Z; z += step.Z )
    FilterPeelingRadius ( TPointFloat ( (float) x, boundmax.Y, (float) z ), center, backthreshold, temp );


Gauge.Next ();

for ( int y = boundmin.Y; y <= boundmax.Y; y += step.Y )
for ( int z = boundmin.Z; z <= boundmax.Z; z += step.Z )
    FilterPeelingRadius ( TPointFloat ( boundmin.X, (float) y, (float) z ), center, backthreshold, temp );


Gauge.Next ();

for ( int y = boundmin.Y; y <= boundmax.Y; y += step.Y )
for ( int z = boundmin.Z; z <= boundmax.Z; z += step.Z )
    FilterPeelingRadius ( TPointFloat ( boundmax.X, (float) y, (float) z ), center, backthreshold, temp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();
}


//----------------------------------------------------------------------------
                                        // this  Operation=  this
template <class TypeD>
void    TVolume<TypeD>::UnaryOp (   ArrayOperandType  /*typeoperand*/, 
                                    ArrayOperationType  typeoperation, 
                                    double*             p1,             double*             p2 
                                )
{
if ( IsNotAllocated () )
    return;

TypeD               minv            = Lowest  ( minv );
TypeD               maxv            = Highest ( maxv );

OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )

    if      ( typeoperation == OperationThreshold       )   Array[ i ]  = (TypeD)                               ( Array[ i ] < *p1 || Array[ i ] > *p2  ? 0     : Array[ i ] );
    else if ( typeoperation == OperationThresholdAbove  )   Array[ i ]  = (TypeD)                               ( Array[ i ] < *p1                      ? 0     : Array[ i ] );
    else if ( typeoperation == OperationThresholdBelow  )   Array[ i ]  = (TypeD)                               ( Array[ i ] > *p1                      ? 0     : Array[ i ] );
    else if ( typeoperation == OperationBinarize        )   Array[ i ]  = (TypeD)                               ( Array[ i ]                            ? *p1   : 0          );
    else if ( typeoperation == OperationRevert          )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( *p1 - Array[ i ] ),                           minv,   maxv );
    else if ( typeoperation == OperationSet             )   Array[ i ]  = (TypeD)                Clip ( (TypeD)   *p1,                                          minv,   maxv );
    else if ( typeoperation == OperationAdd             )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] + *p1 ),                           minv,   maxv );
    else if ( typeoperation == OperationSub             )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] - *p1 ),                           minv,   maxv );
    else if ( typeoperation == OperationAddIfNotNull    )   Array[ i ]  = (TypeD) ( Array[ i ] ? Clip ( (TypeD) ( Array[ i ] + *p1 ),                           minv,   maxv ) : 0 );
    else if ( typeoperation == OperationMultiply        )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] * *p1 ),                           minv,   maxv );
    else if ( typeoperation == OperationDivide          )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] / *p1 ),                           minv,   maxv );
    else if ( typeoperation == OperationRemap           )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] == *p1 ? *p2 : Array[ i ] ),       minv,   maxv );
    else if ( typeoperation == OperationInvert          )   Array[ i ]  = (TypeD)                Clip ( (TypeD) ( Array[ i ] ? 1 /* *p1 */ / Array[ i ] : 0 ),  minv,   maxv );
    else if ( typeoperation == OperationAbsolute        )   Array[ i ]  = (TypeD)                                 fabs ( Array[ i ] );
    else if ( typeoperation == OperationKeep            )   Array[ i ]  = (TypeD)                               ( Array[ i ] != *p1                     ? 0     : Array[ i ] );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterThreshold ( FilterTypes filtertype, FctParams& params )
{
if      ( filtertype == FilterTypeThreshold
       || filtertype == FilterTypeThresholdBinarize ) {

    double              mint            = params ( FilterParamThresholdMin );
    double              maxt            = params ( FilterParamThresholdMax );

    UnaryOp ( OperandData, OperationThreshold, &mint, &maxt );
    }

else if ( filtertype == FilterTypeThresholdAbove ) {

    double              mint            = params ( FilterParamThreshold );

    UnaryOp ( OperandData, OperationThresholdAbove, &mint );
    }

else if ( filtertype == FilterTypeThresholdBelow ) {

    double              maxt            = params ( FilterParamThreshold );

    UnaryOp ( OperandData, OperationThresholdBelow, &maxt );
    }


if ( filtertype == FilterTypeThresholdBinarize ) {

    double              binv            = params ( FilterParamThresholdBin );

    UnaryOp ( OperandData, OperationBinarize, &binv );
    }
}


template <class TypeD>
void    TVolume<TypeD>::FilterBinarize ( FctParams& params )
{
double              binv            = params ( FilterParamBinarized );

UnaryOp ( OperandData, OperationBinarize, &binv );
}

                                        // Revert sequence of values min..max -> max..min
template <class TypeD>
void    TVolume<TypeD>::FilterRevert ( FctParams& /*params*/ )
{
double              maxv            = GetMaxValue ();

UnaryOp ( OperandData, OperationRevert, &maxv );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterIntensityScale ( FilterTypes filtertype, FctParams& params )
{
double              scaling         = params ( FilterParamOperand );

if ( scaling == 1 )
    return;

UnaryOp ( OperandData, filtertype == FilterTypeIntensityMult ? OperationMultiply : OperationDivide, &scaling );
}

                                        // Rescaling any existing negative and positive parts to -1 and 1 respectively
template <class TypeD>
void    TVolume<TypeD>::FilterIntensityNorm ( FctParams& /*params*/ )
{
double              maxv            = GetMaxValue ();
double              minv            = GetMinValue ();

                                        // rescaling any negative part?
if ( minv < 0 && minv != -1 ) {

    minv    = fabs ( minv );

    OmpParallelFor

    for ( int i = 0; i < LinearDim; i++ )

        if ( Array[ i ] < 0 )   Array[ i ] /= minv;
    }

                                        // rescaling any positive part?
if ( maxv > 0 && maxv != 1 ) {

    OmpParallelFor

    for ( int i = 0; i < LinearDim; i++ )

        if ( Array[ i ] > 0 )   Array[ i ] /= maxv;
    }
}


template <class TypeD>
void    TVolume<TypeD>::FilterIntensityInvert ( FctParams& /*params*/ )
{
UnaryOp ( OperandData, OperationInvert );
}


template <class TypeD>
void    TVolume<TypeD>::FilterIntensityAdd ( FilterTypes filtertype, FctParams& params )
{
double              adding          = params ( FilterParamOperand );

if ( adding == 0 )
    return;

UnaryOp ( OperandData, filtertype == FilterTypeIntensityAdd ? OperationAdd : OperationSub, &adding );
}


template <class TypeD>
void    TVolume<TypeD>::FilterIntensityOpposite ( FctParams& /*params*/ )
{
double              zero            = 0;

UnaryOp ( OperandData, OperationRevert, &zero );
}


template <class TypeD>
void    TVolume<TypeD>::FilterIntensityAbsolute ( FctParams& /*params*/ )
{
UnaryOp ( OperandData, OperationAbsolute );
}


template <class TypeD>
void    TVolume<TypeD>::FilterIntensityRemap ( FctParams& params )
{
double              from            = params ( FilterParamRemapFrom );
double              to              = params ( FilterParamRemapTo   );

UnaryOp ( OperandData, OperationRemap, &from, &to );
}


template <class TypeD>
void    TVolume<TypeD>::FilterKeepValue ( FctParams& params )
{
double              keep            = params ( FilterParamKeepValue );

UnaryOp ( OperandData, OperationKeep, &keep );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
