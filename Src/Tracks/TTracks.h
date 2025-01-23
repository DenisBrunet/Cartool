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

#include    "CartoolTypes.h"
#include    "TArray2.h"
#include    "Files.TOpenDoc.h"
#include    "Files.ReadFromHeader.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TArray1;
class                               TStrings;
class                               FctParams;


//----------------------------------------------------------------------------
                                        // Specializing a TArray2 to hold temporal tracks
                                        //  - First  index corresponds to tracks (like electrode)
                                        //  - Second index corresponds to time
template <class TypeD>
class   TTracks :   public  TArray2<TypeD>
{
public:
                    TTracks ()                      : TArray2 ()               {}
                    TTracks ( int dim1, int dim2 )  : TArray2 ( dim1, dim2 )   {}
                    TTracks ( double diameter, ArraySizeType sizeconstraint ); // used to set a Kernel filter
                    TTracks ( const char *file )                                { ReadFile ( file ); }


    TRealIndex      Index1;             // Convert real position to index for dimension 1
    TRealIndex      Index2;             // Convert real position to index for dimension 2


    TypeD           GetSumValues        ()          const;
    double          GetSumAbsValues     ()          const;
    double          GetSumAbsRow        ( int row ) const;
    double          GetSumAbsCol        ( int col ) const;
    double          GetMaxAbsRow        ( int row ) const;
//  double          GetMaxAbsCol        ( int col ) const;
    void            NormalizeArea       ();
    void            NormalizeMax        ();
    void            NormalizeRowsArea   ();
    void            NormalizeRowsMax    ();
    void            NormalizeColsArea   ();
    void            NormalizeColsMax    ();
    void            NormalizeTemplates  ();     // Convert to template: centered and norm = 1

    void            ComputeGFP          ( TArray1<double>& gfp, AtomType datatype );


    void            AverageDim2         ( int n = 0 );
    void            Filter              ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterDim1          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterDim2          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            Invert              ();
    int             GetNumSet           ()  const;
    void            RegionGrowing       ( int xcenter, int ycenter, TypeD maxthresh, int &xbox1, int &xbox2, int &ybox1, int &ybox2, bool checknegative = true );  // returned part is set negative


    void            ReadFile            ( const char *file );                               // ep txt bin sef
    void            WriteFile           ( char *file, TStrings*    tracknames = 0 ) const;  // ep txt bin sef spi (first 3 rows)


    using  TArray2::operator    =;

    TTracks<TypeD>& operator    +=      ( const TTracks<TypeD> &op2 );
    TTracks<TypeD>& operator    -=      ( const TTracks<TypeD> &op2 );
    TTracks<TypeD>& operator    *=      ( const TTracks<TypeD> &op2 );
    TTracks<TypeD>& operator    /=      ( const TTracks<TypeD> &op2 );
    TTracks<TypeD>& operator    *=      ( double op2 );
    TTracks<TypeD>& operator    /=      ( double op2 );
    TTracks<TypeD>& operator    -=      ( double op2 );
    TTracks<TypeD>  operator    +       ( const TTracks<TypeD> &op2 )   const;
    TTracks<TypeD>  operator    -       ( const TTracks<TypeD> &op2 )   const;
    TTracks<TypeD>  operator    *       ( const TTracks<TypeD> &op2 )   const;
    TArray1<TypeD>  operator    *       ( const TArray1<TypeD> &op2 )   const;    // Matrix * Column
    TTracks<TypeD>  operator    *       ( double op2 )                  const;
    TTracks<TypeD>  operator    /       ( double op2 )                  const;


protected:

    void            FilterLinear        ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterStat          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterFastGaussian  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TTracks<TypeD>::TTracks ( double diameter, ArraySizeType sizeconstraint )
{
int                 size            = DiameterToKernelSize ( diameter, sizeconstraint );

Dim1            = size;
Dim2            = size;

LinearDim       = Dim1 * Dim2;
Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TTracks<TypeD>::GetSumValues () const
{
if ( IsNotAllocated () )
    return  0;


double              sum             = 0;

for ( int i = 0; i < LinearDim; i++ )
    sum    += Array[ i ];

return  (TypeD) sum;
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TTracks<TypeD>::GetSumAbsValues ()  const
{
if ( IsNotAllocated () )
    return  0;


double              sum             = 0;

for ( int i = 0; i < LinearDim; i++ )
    sum    += fabs ( (double) Array[ i ] );

return  sum;
}


template <class TypeD>
double  TTracks<TypeD>::GetSumAbsRow ( int row )    const
{
if ( IsNotAllocated () )
    return  0;


double              sum             = 0;


for ( int i2 = 0; i2 < Dim2; i2++ )
    sum    += fabs ( (double) GetValue ( row, i2 ) );

return  sum;
}


template <class TypeD>
double  TTracks<TypeD>::GetSumAbsCol ( int col )    const
{
if ( IsNotAllocated () )
    return  0;


double              sum             = 0;


for ( int i1 = 0; i1 < Dim1; i1++ )
    sum    += fabs ( (double) GetValue ( i1, col ) );

return  sum;
}


template <class TypeD>
double  TTracks<TypeD>::GetMaxAbsRow ( int row )    const
{
if ( IsNotAllocated () )
    return  0;


double              maxvalue        = fabs ( GetValue ( row, 0 ) );


for ( int i2 = 1; i2 < Dim2; i2++ )

    Maxed ( maxvalue, fabs ( (double) GetValue ( row, i2 ) ) );


return  maxvalue;
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeArea ()
{
*this /= NonNull ( GetSumAbsValues () );
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeMax ()
{
*this /= NonNull ( GetMaxValue () );
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeRowsArea ()
{
double              sum;


for ( int i1 = 0; i1 < Dim1; i1++ ) {

    sum     = NonNull ( GetSumAbsRow ( i1 ) );

    for ( int i2 = 0; i2 < Dim2; i2++ )
        GetValue ( i1, i2 )    /= sum;
    }
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeRowsMax ()
{
double              maxv;


for ( int i1 = 0; i1 < Dim1; i1++ ) {

    maxv    = 0;

    for ( int i2 = 0; i2 < Dim2; i2++ )
        Maxed ( maxv, (double) fabs ( GetValue ( i1, i2 ) ) );

    maxv    = NonNull ( maxv );

    for ( int i2 = 0; i2 < Dim2; i2++ )
        GetValue ( i1, i2 )    /= maxv;
    }
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeColsArea ()
{
double              sum;


for ( int i2 = 0; i2 < Dim2; i2++ ) {

    sum     = NonNull ( GetSumAbsCol ( i2 ) );

    for ( int i1 = 0; i1 < Dim1; i1++ )
        GetValue ( i1, i2 )    /= sum;
    }
}


template <class TypeD>
void    TTracks<TypeD>::NormalizeColsMax ()
{
double              maxv;


for ( int i2 = 0; i2 < Dim2; i2++ ) {

    maxv    = 0;

    for ( int i1 = 0; i1 < Dim1; i1++ )
        Maxed ( maxv, (double) fabs ( GetValue ( i1, i2 ) ) );

    maxv    = NonNull ( maxv );

    for ( int i1 = 0; i1 < Dim1; i1++ )
        GetValue ( i1, i2 )    /= maxv;
    }
}


//----------------------------------------------------------------------------
                                        // Convert to template: centered and norm = 1
template <class TypeD>
void    TTracks<TypeD>::NormalizeTemplates ()
{
TVector<TypeD>      map ( Dim1 );
//TMap                map ( Dim1 );

                                        // for all time frame
for ( int i2 = 0; i2 < Dim2; i2++ ) {

    map.GetColumn ( *this, i2 );

                                        // Average Reference, then Norm = 1
    map.Normalize ( true );

                                        // copy back
    for ( int el = 0; el < Dim1; el++ )
        GetValue ( el, i2 ) = map[ el ];
    }
}


//----------------------------------------------------------------------------
                                        // 
template <class TypeD>
void    TTracks<TypeD>::ComputeGFP ( TArray1<double>& gfp, AtomType datatype )
{
TVector<TypeD>      map ( Dim1 );

gfp.Resize ( Dim2 );


for ( int i2 = 0; i2 < Dim2; i2++ ) {

    map.GetColumn ( *this, i2 );

    gfp[ i2 ]   = map.GlobalFieldPower ( ! IsAbsolute ( datatype ), IsVector ( datatype ) );
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::AverageDim2 ( int n )
{
if ( IsNotAllocated () )
    return;

if ( n == 0 || n > Dim2 )
    n = Dim2;


double              sum;

for ( int i1 = 0; i1 < LinearDim; i1 += Dim2 ) {

    sum     = 0;

    for ( int i2 = 0, i12 = i1; i2 < n; i2++, i12++ )
        sum += Array[ i12 ];

    Array[ i1 ] = (TypeD) ( sum / n );
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
int     TTracks<TypeD>::GetNumSet ()    const
{
int                 num             = 0;

for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] != 0 )
        num++;

return  num;
}


template <class TypeD>
void    TTracks<TypeD>::Invert ()
{
for ( int i = 0; i < LinearDim; i++ )
    Array[ i ]  = (TypeD) - Array[ i ]; // !will complain if used with unsigned data types!
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::FilterDim1 ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<double>     line ( Dim1 );          // !filters will operate in double!


for ( int i2 = 0; i2 < Dim2; i2++ ) {

    for ( int i1 = 0; i1 < Dim1; i1++ )
        line ( i1 ) = GetValue ( i1, i2 );  // will operate with any input data type


    line.Filter ( filtertype, params, showprogress );


    for ( int i1 = 0; i1 < Dim1; i1++ )
        GetValue ( i1, i2 ) = (TypeD) line ( i1 );
    }
}


template <class TypeD>
void    TTracks<TypeD>::FilterDim2 ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<double>     line ( Dim2 );          // !filters will operate in double!


for ( int i1 = 0; i1 < Dim1; i1++ ) {

    for ( int i2 = 0; i2 < Dim2; i2++ )
        line ( i2 ) = GetValue ( i1, i2 );  // will operate with any input data type


    line.Filter ( filtertype, params, showprogress );


    for ( int i2 = 0; i2 < Dim2; i2++ )
        GetValue ( i1, i2 ) = (TypeD) line ( i2 );
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::Filter ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
if      ( filtertype == FilterTypeGaussian
       || filtertype == FilterTypeHighPassLight
       || filtertype == FilterTypeMean              )   FilterLinear        ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeFastGaussian      )   FilterFastGaussian  ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeMin
       || filtertype == FilterTypeMax
       || filtertype == FilterTypeMinMax
       || filtertype == FilterTypeRange
       || filtertype == FilterTypeMedian
//     || filtertype == FilterTypeMeanSub
//     || filtertype == FilterTypeMAD
       || filtertype == FilterTypeSD
       || filtertype == FilterTypeSDInv
       || filtertype == FilterTypeCoV
       || filtertype == FilterTypeLogSNR
//     || filtertype == FilterTypeMCoV
//     || filtertype == FilterTypeMADSDInv
//     || filtertype == FilterTypeModeQuant
//     || filtertype == FilterTypeEntropy
                                                    )   FilterStat          ( filtertype,               params, showprogress );

else
    ShowMessage ( "Filter not implemented for TTracks<TypeD>!", FilterPresets[ filtertype ].Text, ShowMessageWarning );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::FilterLinear ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
double              diameter        = filtertype == FilterTypeHighPassLight ? 3 : params ( FilterParamDiameter );

if ( diameter < 2 )                     // no other voxels involved other than center?
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // see:  http://en.wikipedia.org/wiki/Sampled_Gaussian_kernel#The_sampled_Gaussian_kernel

                                        // create Kernel mask, always of odd size
TTracks<double>     Kf ( diameter, OddSize );   // the floating point version, used to compute the Kernel
TPointInt           Ko ( Kf.GetDim1 () / 2, Kf.GetDim2 () / 2, 0 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( Kf.GetDim1 () ) ? 2 * Square ( 0.5 ) : 0 ) + 1e-6;
TPointInt           kp;
double              k;


int                 gaussnumSD      = 2;
TGaussian           gauss ( 0, ( diameter - 1 ) / 2, gaussnumSD );

                                        // compute the Kernel in floating point
for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
for ( int yk = 0; yk < Kf.GetDim2 (); yk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, 0 );

    if      ( filtertype == FilterTypeGaussian      )       k   = gauss ( kp.Norm () );
    else if ( filtertype == FilterTypeHighPassLight )       k   = ( kp.X || kp.Y ? -1 : 4 ) * gauss ( kp.Norm () ); // very rough, only for 3x3x3
    else /*if ( filtertype == FilterTypeMean        )*/     k   = 1;

                                        // clip Kernel outside Euclidian norm!
    Kf ( xk, yk )   = kp.Norm2 () <= r2 ? k : 0;
    }

                                        // normalize Kernel
double              sumk            = 0;

for ( int i = 0; i < Kf.GetLinearDim (); i++ )
    sumk       += Kf.GetValue ( i ); // Kf[ i ];

Kf     /= sumk;


//DBGV5 ( diameter, Kf.GetDim1 (), Kf.GetDim2 (), Ko.X, Ko.Y, "diameter  Kernel: Dim1 Dim2 Originxy" );
//for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
//    for ( int yk = 0; yk < Kf.GetDim2 (); yk++ )
//        DBGV3 ( xk, yk, Kf ( xk, yk ) * 1000, "x,y  K" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TTracks<TypeD>      temp ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2 );


temp.Insert ( *this, Ko );


//for ( int x = 0; x < Dim1; x++ )
//    for ( int y = 0; y < Dim2; y++ )
//        temp ( x + Ko.X, y + Ko.Y ) = GetValue ( x, y );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );

/*TypeD*/ double    v;

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();

    for ( int y = 0; y < Dim2; y++ ) {

        v       = 0;

                                        // scan kernel
        for ( int xki = 0, xk = x; xki < Kf.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y; yki < Kf.GetDim2 (); yki++, yk++ )

            v      += Kf ( xki, yki ) * temp ( xk, yk );


        Array[ IndexesToLinearIndex ( x, y ) ]  = (TypeD) v;
        } // for y
    }
}


//----------------------------------------------------------------------------
                                        // Fastest Gaussian filtering, with SKISPM:
// "An efficient algorithm for Gaussian blur using finite-state machines"
// Frederick M. Waltz and John W. V. Miller
// SPIE Conf. on Machine Vision Systems for Inspection and Metrology VII
// Originally published Boston, Nov. 1998

                                        // Size parameter is an odd, discrete width in voxels, like 3, 5, 7 etc..
                                        // We repeat elementary and fast 3x3 filters as many times as needed to reach the required width
                                        // Border padding is currently set with 0
template <class TypeD>
void    TTracks<TypeD>::FilterFastGaussian ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
int                 numrepeat;


if ( params ( FilterParamDiameter ) < 0 )
    numrepeat       = (int) fabs ( params ( FilterParamDiameter ) );  // !if negative value, force use as the number of repetitions!
else
    numrepeat       = params ( FilterParamDiameter ) > 2 ? Round ( ( Power ( params ( FilterParamDiameter ) - 2, 1.35 ) - 1 ) / 2 ) : 0;    // ad-hoc formula after comparison with our plain Gaussian


if ( numrepeat < 1 )                    // not big enough
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border + conversion to higher precision
                                        // border is set to 0
int                 tdim1           = Dim1 + 2 * numrepeat;
int                 tdim2           = Dim2 + 2 * numrepeat;
TTracks<TypeD>      temp ( tdim1, tdim2 );

                                        // insert with origin shift + conversion
for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
    temp ( x + numrepeat, y + numrepeat )   = GetValue ( x, y );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate SKIPSM buffers
TArray1<TypeD>      Sx0        ( tdim2 );   // 2 rows   Y
TArray1<TypeD>      Sx1        ( tdim2 );
TypeD               Sy0;                    // 2 previous values
TypeD               Sy1;
TypeD               tmp1;
TypeD               tmp2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? numrepeat *  ( tdim1 - 1 ) : 0 );


for ( int n = 0; n < numrepeat; n++ ) {

    Sx0.ResetMemory ();
    Sx1.ResetMemory ();

    for ( int x = 1; x < tdim1; x++ ) {

        Gauge.Next ();

        Sy0     = 0;
        Sy1     = 0;

        for ( int y = 1; y < tdim2; y++ ) {

            tmp1            = temp ( x, y );    // |1|

            tmp2            = Sy0 + tmp1;       // |1 1|
            Sy0             = tmp1;

            tmp1            = Sy1 + tmp2;       // |1 2 1|
            Sy1             = tmp2;

            tmp2            = Sx0[ y ] + tmp1;
            Sx0[ y ]        = tmp1;

            temp ( x - 1, y - 1 )   = (TypeD) ( ( Sx1[ y ] + tmp2 ) / 16.0 );   // no rounding errors with temp buffer?
            Sx1[ y ]        = tmp2;

            } // for y
        } // for x
    } // for numrepeat


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy & clip results
for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
    Array[ IndexesToLinearIndex ( x, y ) ]  = temp ( x + numrepeat, y + numrepeat );

}


//----------------------------------------------------------------------------
                                        // Anything stat-like filter
template <class TypeD>
void    TTracks<TypeD>::FilterStat ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
double              diameter        = params ( FilterParamDiameter );
//double              histomaxbins    = Round ( params ( FilterParamNumModeQuant ) );
//int                 filterresult    = params ( FilterParamResultType );


if ( diameter < 2 )                     // no other voxels involved other than center?
    return;

//if ( filtertype == FilterTypeModeQuant && histomaxbins < 2 )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Log filters need some offset to avoid values <= 0
bool                logfilters      = filtertype == FilterTypeLogCoV
                                   || filtertype == FilterTypeLogMCoV
                                   || filtertype == FilterTypeLogMeanSub
                                   || filtertype == FilterTypeLogMeanDiv
                                   || filtertype == FilterTypeLogSNR;

double              logoffset       = logfilters ? params ( FilterParamLogOffset ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, always of odd size
TTracks<bool>       K  ( diameter, OddSize );
TPointInt           Ko ( K.GetDim1 () / 2, K.GetDim2 () / 2, 0 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( K.GetDim1 () ) ? 2 * Square ( 0.5 ) : 0 ) + 1e-6;
TPointInt           kp;


                                        // compute the Kernel
for ( int xk = 0; xk < K.GetDim1 (); xk++ )
for ( int yk = 0; yk < K.GetDim2 (); yk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, 0 );
                                    // clip Kernel outside Euclidian norm!
    K ( xk, yk )    = kp.Norm2 () <= r2;
//  K ( xk, yk )    = sqrt ( kp.X * kp.X * 1.00 + kp.Y * kp.Y * 1.15 + kp.Z * kp.Z * 1.30 ) <= r2;  // trial of elliptic Kernel
    }

//int                 kset            = K.GetNumSet ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TTracks<TypeD>      temp ( Dim1 + Ko.X * 2, Dim2 + Ko.Y * 2 );

temp.Insert ( *this, Ko );


TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );


TEasyStats          stat ( K.GetLinearDim () );
double              v;
//double              avg;
//double              sd;
//int                 maxvalue        = max ( 1, (int) GetMaxValue () );

//double              histodown       = filtertype == FilterTypeEntropy ? 1 : histomaxbins / maxvalue;
//THistogram          histo ( maxvalue * histodown + 1 );
//int                 histominfreq    = max ( 3, Round ( Power ( kset, 1 / 3.0 ) * 1.0 ) );

                                        // entropy is a count of grey levels: can not exceed histo size, and # of insertion
//double              entropyrescale  = UCHAR_MAX / (double) max ( UCHAR_MAX, min ( maxvalue + 1, kset ) );
                                        // rescale to max char
//double              covrescale      = UCHAR_MAX / (double) sqrt ( kset );
//double              covrescale      = UCHAR_MAX / Power ( kset, 0.25 );



for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();

    for ( int y = 0; y < Dim2; y++ ) {

        stat .Reset ();
//      histo.Reset ();

                                        // scan kernel
        for ( int xki = 0, xk = x; xki < K.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y; yki < K.GetDim2 (); yki++, yk++ )

            if ( K ( xki, yki ) ) {
                stat .Add ( temp ( xk, yk ) );
//              histo.Add ( temp ( xk, yk ) * histodown + 0.5 );
                }

        stat.Sort ( true );


        if      ( filtertype == FilterTypeMin               )   v   = stat.Min ();
        else if ( filtertype == FilterTypeMax               )   v   = stat.Max ();
        else if ( filtertype == FilterTypeRange             )   v   = stat.Range ();
        else if ( filtertype == FilterTypeMedian            )   v   = stat.Median ();
        else if ( filtertype == FilterTypeSD                )   v   = stat.SD ();
        else if ( filtertype == FilterTypeSDInv             )   v   = ( v = stat.SD () ) != 0 ? NoMore ( 1e10, 1 / v ) : 0; // capping the max(?)
                                        // give more resolution to lower values (Gamma in option?)
        else if ( filtertype == FilterTypeCoV               )   v   = stat.CoefficientOfVariation (); // * covrescale;
//      else if ( filtertype == FilterTypeSNR               )   v   = stat.SNR (); // * 2;
        else if ( filtertype == FilterTypeLogSNR            )   v   = Log10 ( logoffset + stat.SNR () );    // much better with a log

                                        // Sharp Window: take the one closest, the min or the max
//      else if ( filtertype == FilterTypeMinMax            )   v   = stat.MinMax ( GetValue ( x, y ) );
                                        // complex compatible formula:
        else if ( filtertype == FilterTypeMinMax            )   v   = fabs ( GetValue ( x, y ) - stat.Min () ) < fabs ( GetValue ( x, y ) - stat.Max () ) ? stat.Min () : stat.Max ();


        Array[ IndexesToLinearIndex ( x, y ) ]      = (TypeD) v;
        } // for y
    }
}


//----------------------------------------------------------------------------
                                        // Grow from a seed point
                                        // returned part is set negative
                                        // !working with signed data type only!
template <class TypeD>
void    TTracks<TypeD>::RegionGrowing ( int xcenter, int ycenter, TypeD maxthresh, int &xbox1, int &xbox2, int &ybox1, int &ybox2, bool checknegative )
{
                                        // 1) There shouldn't remain any negative part, so optionally remove it
if ( checknegative )
    for ( int i = 0; i < LinearDim; i++ )
        if ( Array[ i ] < 0 )   Array[ i ]  = 0;


                                        // 2) Seed from the given point - marked parts are set negative (we already removed the real negative parts, so no confusion is possible)
GetValue ( xcenter, ycenter )   = - GetValue ( xcenter, ycenter );


xbox1       = xcenter;                  // current bounding box
xbox2       = xcenter;
ybox1       = ycenter;
ybox2       = ycenter;
int                 xi1;                // bounding box for current iteration
int                 xi2;
int                 yi1;
int                 yi2;
TypeD /*double*/    v00;
TypeD /*double*/    v0n;
TypeD /*double*/    v0p;
TypeD /*double*/    vn0;
TypeD /*double*/    vp0;
bool                stillgrowing;

                                        // 3) Repeat growing until no changes
do {

    stillgrowing    = false;

    xi1             = xbox1;
    xi2             = xbox2;
    yi1             = ybox1;
    yi2             = ybox2;

    for ( int x = xi1; x <= xi2; x++ )
    for ( int y = yi1; y <= yi2; y++ ) {

        v00     = GetValue ( x, y );

        if ( v00 >= 0 )     continue;   // not part of the region

                                        // here x,y is part of the region - search for any neighbor to expand
                                        // use 4 neighbors

        v00     = -v00;                 // restore to the real positive value

                                                                              // don't grow to higher parts!
        if ( y > 0        && ( v0n = GetValue ( x    , y - 1 ) ) >= maxthresh && v0n <= v00 /* * 1.05 */ )    { GetValue ( x, y - 1 ) = - v0n;    if ( y - 1 < ybox1 ) ybox1 = y - 1;     stillgrowing = true; }
        if ( y < Dim2 - 1 && ( v0p = GetValue ( x    , y + 1 ) ) >= maxthresh && v0p <= v00 /* * 1.05 */ )    { GetValue ( x, y + 1 ) = - v0p;    if ( y + 1 > ybox2 ) ybox2 = y + 1;     stillgrowing = true; }
        if ( x > 0        && ( vn0 = GetValue ( x - 1, y     ) ) >= maxthresh && vn0 <= v00 /* * 1.05 */ )    { GetValue ( x - 1, y ) = - vn0;    if ( x - 1 < xbox1 ) xbox1 = x - 1;     stillgrowing = true; }
        if ( x < Dim1 - 1 && ( vp0 = GetValue ( x + 1, y     ) ) >= maxthresh && vp0 <= v00 /* * 1.05 */ )    { GetValue ( x + 1, y ) = - vp0;    if ( x + 1 > xbox2 ) xbox2 = x + 1;     stillgrowing = true; }
        }

    } while ( stillgrowing );

}


//----------------------------------------------------------------------------
template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator+= ( const TTracks<TypeD> &op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop
    for ( int i = 0; i < LinearDim; i++ )
        Array[ i ] += op2.Array[ i ];
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = IndexesToLinearIndex ( d1, d2 ); d2 < mindim2; d2++, i++ )
        Array[ i ] += op2[ d1 ][ d2 ];
    }

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator-= ( const TTracks<TypeD> &op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop
    for ( int i = 0; i < LinearDim; i++ )
        Array[ i ] -= op2.Array[ i ];
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = IndexesToLinearIndex ( d1, d2 ); d2 < mindim2; d2++, i++ )
        Array[ i ] -= op2[ d1 ][ d2 ];
    }

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator*= ( const TTracks<TypeD> &op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop
    for ( int i = 0; i < LinearDim; i++ )
        Array[ i ] *= op2.Array[ i ];
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = IndexesToLinearIndex ( d1, d2 ); d2 < mindim2; d2++, i++ )
        Array[ i ] *= op2[ d1 ][ d2 ];
    }

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator/= ( const TTracks<TypeD> &op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop
    for ( int i = 0; i < LinearDim; i++ )
        Array[ i ] /= NonNull ( op2.Array[ i ] );
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = IndexesToLinearIndex ( d1, d2 ); d2 < mindim2; d2++, i++ )
        Array[ i ] /= NonNull ( op2[ d1 ][ d2 ] );
    }

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator*= ( double op2 )
{
for ( int i = 0; i < LinearDim; i++ )
    Array[ i ] *= op2;

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator/= ( double op2 )
{
if ( op2 == 0 )
    return *this;

for ( int i = 0; i < LinearDim; i++ )
    Array[ i ] /= op2;

return  *this;
}


template <class TypeD>
TTracks<TypeD>& TTracks<TypeD>::operator-= ( double op2 )
{
if ( op2 == 0 )
    return *this;

for ( int i = 0; i < LinearDim; i++ )
    Array[ i ] -= op2;

return  *this;
}


template <class TypeD>
TTracks<TypeD>  TTracks<TypeD>::operator+ ( const TTracks<TypeD> &op2 )     const
{
/*
if ( LinearDim != op2.LinearDim )
    return *this;

TTracks<TypeD>  temp ( Dim1, Dim2 );

for ( int i=0; i < LinearDim; i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] + op2.Array[ i ] );
*/

int     mindim1 = min ( Dim1, op2.Dim1 );
int     mindim2 = min ( Dim2, op2.Dim2 );

TTracks<TypeD>  temp ( mindim1, mindim2 );


if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )
        temp.Array[ i ] = (TypeD) ( Array[ i ] + op2.Array[ i ] );
else {

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
        temp[ d1 ][ d2 ] = (TypeD) ( (*this)[ d1 ][ d2 ] + op2[ d1 ][ d2 ] );
    }

return  temp;
}


template <class TypeD>
TTracks<TypeD>  TTracks<TypeD>::operator- ( const TTracks<TypeD> &op2 )     const
{
/*
if ( LinearDim != op2.LinearDim )
    return *this;

TTracks<TypeD>  temp ( Dim1, Dim2 );

for ( int i=0; i < LinearDim; i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] - op2.Array[ i ] );
*/

int     mindim1 = min ( Dim1, op2.Dim1 );
int     mindim2 = min ( Dim2, op2.Dim2 );

TTracks<TypeD>  temp ( mindim1, mindim2 );


if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )
        temp.Array[ i ] = (TypeD) ( Array[ i ] - op2.Array[ i ] );
else {

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
        temp[ d1 ][ d2 ] = (TypeD) ( (*this)[ d1 ][ d2 ] - op2[ d1 ][ d2 ] );
    }

return  temp;
}


template <class TypeD>
TTracks<TypeD>  TTracks<TypeD>::operator* ( const TTracks<TypeD> &op2 )     const
{
TTracks<TypeD>  temp ( Dim1, Dim2 );

for ( int i=0; i < LinearDim; i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] * op2.Array[ i ] );


return  temp;
}

                                        // Matrix multiplication
template <class TypeD>
TArray1<TypeD>  TTracks<TypeD>::operator* ( const TArray1<TypeD> &op2 )     const
{
TArray1<TypeD>  temp ( Dim1 );


for ( int i = 0; i < Dim1; i++ )
for ( int j = 0; j < Dim2; j++ )
    temp ( i )     += (TypeD) ( GetValue ( i, j ) * op2 ( j ) );


return  temp;
}


template <class TypeD>
TTracks<TypeD>  TTracks<TypeD>::operator* ( double op2 )    const
{
TTracks<TypeD>  temp ( Dim1, Dim2 );

for ( int i=0; i < LinearDim; i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] * op2 );


return  temp;
}


template <class TypeD>
TTracks<TypeD>  TTracks<TypeD>::operator/ ( double op2 )    const
{
TTracks<TypeD>  temp ( Dim1, Dim2 );

for ( int i=0; i < LinearDim; i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] / op2 );


return  temp;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::ReadFile ( const char *file )
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get file ready
bool                epformat        = IsExtensionAmong ( file, FILEEXT_TXT " " FILEEXT_EEGEP );
bool                sefformat       = IsExtensionAmong ( file, FILEEXT_EEGSEF );
bool                binformat       = IsExtensionAmong ( file, "bin" );
bool                FileBin         = ! epformat;

                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = epformat || sefformat || binformat;
assert ( formatok );
#endif

                                        // binary file is a dump, caller must allocate the object so we know what to load
#if defined(CHECKASSERT)
assert ( ! binformat || IsAllocated () );
#endif
//if ( binformat && IsNotAllocated () )
//    return;


TFileName           safefile ( file, TFilenameExtendedPath );


if ( ! CanOpenFile ( safefile, CanOpenFileRead ) )
    return;


ifstream            ifs ( safefile, FileBin ? ios::binary | ios::in : ios::in );

if ( ifs.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get dimensions
int                 dim1;
int                 dim2;


if ( epformat ) {
                                        // multiplexed data, lines == time == last dimension
    dim1            = CountPerLine ( safefile );
    dim2            = CountLines   ( safefile );

    if ( dim1 <= 0 || dim2 <= 0 )
        return;
    }

else if ( binformat ) {
                                        // already allocated
    dim1            = Dim1;
    dim2            = Dim2;
    }

else if ( sefformat ) {

    ReadFromHeader ( safefile, ReadNumElectrodes,         (char *) &dim1 );
    ReadFromHeader ( safefile, ReadNumTimeFrames,         (char *) &dim2 );
    }

                                        // allocate
Resize ( dim1, dim2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Read data
ResetMemory ();


if      ( epformat ) {

    char                buff[ 256 ];

    for ( int y = 0; y < Dim2; y++ )
    for ( int x = 0; x < Dim1; x++ )

        GetValue ( x, y )   = StringToDouble ( GetToken ( &ifs, buff ) );

    } // epformat

else if ( binformat ) {
                                        // memory dump
//  for ( int i = 0; i < LinearDim; i++ )
//      ifs.read ( (char *) &Array[ i ], AtomSize () );

    ifs.read ( (char *) Array, MemorySize () );

    } // binformat

else if ( sefformat ) {
                                        // load into a temp float buffer    
    TOpenDoc< TTracksDoc >  docsef ( safefile, OpenDocHidden );
    TTracks<float>          tempsef ( Dim1, Dim2 );

    docsef->ReadRawTracks ( 0, Dim2 - 1, tempsef );

                                        // then copy & convert to current type
    for ( int i = 0; i < LinearDim; i++ )
        
        Array[ i ]  = (TypeD) tempsef.GetValue ( i );

    } // binformat

}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracks<TypeD>::WriteFile ( char *file, TStrings*    tracknames ) const
{
if ( /*IsNotAllocated () ||*/ StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                epformat        = IsExtensionAmong ( file, FILEEXT_TXT " " FILEEXT_EEGEP );
bool                sefformat       = IsExtensionAmong ( file, FILEEXT_EEGSEF );
bool                risformat       = IsExtensionAmong ( file, FILEEXT_RIS );
bool                binformat       = IsExtensionAmong ( file, "bin" );
bool                spiformat       = IsExtensionAmong ( file, FILEEXT_SPIRR );
bool                FileBin         = ! ( epformat || spiformat );

                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = epformat || sefformat || risformat || binformat || spiformat;
assert ( formatok );
#endif


ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ), FileBin ? ios::out | ios::binary : ios::out );

if ( ofs.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( sefformat || risformat ) {

    TExportTracks       expfile;

    StringCopy   ( expfile.Filename,    file );

    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = Dim1;
    expfile.NumTime             = Dim2;
    expfile.SamplingFrequency   = Index2.IndexRatio;                                           // will "rescale" to actual variable values
    expfile.DateTime            = TDateTime ( 0, 0, 0, 0, 0, 0, Truncate ( Index2.IndexMin * 1000 ), 0 );   // will set the offset correctly

    if ( tracknames && tracknames->NumStrings () >= Dim1 )
        expfile.ElectrodesNames = *tracknames;

    expfile.Begin   ();

    for ( int j = 0; j < Dim2; j++ )
    for ( int i = 0; i < Dim1; i++ )
        expfile.Write ( (float) GetValue ( i, j ) );

    expfile.End     ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( binformat ) {
                                        // !dumps as the original data type!
//  ofs.write ( (char *) Array, MemorySize () );

    for ( int i = 0; i < LinearDim; i++ )
        ofs.write ( (char *) &Array[ i ], AtomSize () );

    } // binformat

else if ( epformat ) {

    ofs << StreamFormatScientific;

    for ( int j = 0; j < Dim2; j++ ) {

        for ( int i = 0; i < Dim1; i++ )
//          ofs << StreamFormatFloat64 << (double) GetValue ( i, j ) << "\t";
            ofs << StreamFormatFloat32 << (float)  GetValue ( i, j ) << "\t";

        ofs << "\n";
        } // for j
    } // epformat

else if ( spiformat ) {


    ofs << StreamFormatFixed;

    for ( int j = 0; j < Dim2; j++ ) {

        for ( int i = 0; i < NoMore ( (const int) 3, Dim1 ); i++ )
            ofs << StreamFormatFloat32 << (float) GetValue ( i, j ) << "\t";

        for ( int i = Dim1; i < 3 ; i++ )
            ofs << StreamFormatFloat32 << 0 << "\t";


        ofs << ( j + 1 );
        ofs << "\n";
        } // for j
    } // spiformat

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
