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


#include    <cmath>						// math functions, constants
#include    <complex>
#include    <limits.h>                  // INT_MAX, UCHAR_MAX...
#include    <limits>                    // numeric_limits

#include    "TSelection.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "TVector.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Little / Big Endian byte swapping
                                        // Functions have an optional flag, in case files does not need the actual swapping
template <typename TypeD>
inline  TypeD       SwapBytes       (   TypeD    v,  bool    swap = true )
{
if ( swap ) {
    if      ( sizeof ( TypeD ) == 1 )   ;
    else if ( sizeof ( TypeD ) == 2 )   v   = (TypeD) _byteswap_ushort ( v );
    else if ( sizeof ( TypeD ) == 4 )   v   = (TypeD) _byteswap_ulong  ( v );
    else if ( sizeof ( TypeD ) == 8 )   v   = (TypeD) _byteswap_uint64 ( v );
    }

return  v;
};

                                        // !Modifies the argument!
template <typename TypeD>
inline  TypeD       SwappedBytes    (   TypeD&      v,  bool    swap = true )
{
if ( swap ) {
    if      ( sizeof ( TypeD ) == 1 )   ;
    else if ( sizeof ( TypeD ) == 2 )   v   = (TypeD) _byteswap_ushort ( v );
    else if ( sizeof ( TypeD ) == 4 )   v   = (TypeD) _byteswap_ulong  ( v );
    else if ( sizeof ( TypeD ) == 8 )   v   = (TypeD) _byteswap_uint64 ( v );
    }

return  v;
};


//----------------------------------------------------------------------------
                                        // Range utilities

                                        // Permutate anything with a copy & assignation operator
template <class TypeD>
inline  void    Permutate   ( TypeD &a, TypeD &b )                              { TypeD  t ( a ); a = b; b = t; }

                                        // Check that 2 values are <, permutate them otherwise
template <class TypeD>
inline  void    CheckOrder  ( TypeD &low, TypeD &high )                         { if ( low > high ) Permutate ( low, high ); }

template <class TypeD>
inline  void    CheckOrder  ( TypeD &low, TypeD &mid, TypeD &high )             { CheckOrder ( mid, high ); CheckOrder ( low, mid ); CheckOrder ( mid, high );  }


                                        // Clip input value against interval limits, input parameter remains untouched
template <typename TypeD>
inline  TypeD   Clip        ( TypeD  v, TypeD limitmin, TypeD limitmax )        { return v < limitmin ? limitmin : v > limitmax ? limitmax : v; }

                                        // Clip input value against interval limits, input parameter is modified
                                        // returns false if variable was originally outside limits
template <typename TypeD>
inline  bool    Clipped     ( TypeD &v, TypeD limitmin, TypeD limitmax )
{
if      ( v < limitmin )    { v = limitmin; return false; }
else if ( v > limitmax )    { v = limitmax; return false; }
else                                        return true;
}
                                        // Clip input range against interval limits, input parameter is modified
                                        // returns false if interval was originally outside limits
template <typename TypeD>
inline  bool    Clipped     ( TypeD &rangemin, TypeD &rangemax, TypeD limitmin, TypeD limitmax )
{
                                        // Check each side independently
bool                check1          = Clipped ( rangemin, limitmin, limitmax );

bool                check2          = Clipped ( rangemax, limitmin, limitmax );
                                        // Check final ordering, by security
CheckOrder ( rangemin, rangemax );
                                        // false if any border failed
return  check1 && check2;
}

                                        // true if within limits, does not modify input variable
template <typename TypeD>
inline  bool    IsInsideLimits  ( TypeD  v, TypeD limitmin, TypeD limitmax )                    { return v >= limitmin && v <= limitmax; }

                                        // same but testing an interval
template <typename TypeD>
inline  bool    IsInsideLimits  ( TypeD  vmin, TypeD  vmax, TypeD limitmin, TypeD limitmax )    { return vmin >= limitmin && vmin <= limitmax && vmax >= limitmin && vmax <= limitmax; }

                                        // test if the 2 intervals overlap - intervals values are inclusive
template <typename TypeD>
inline  bool    IsNotOverlappingInterval    ( TypeD min1, TypeD max1, TypeD min2, TypeD max2 )  { return     min1 > max2 || max1 < min2; }


template <typename TypeD>
inline  bool    IsOverlappingInterval       ( TypeD min1, TypeD max1, TypeD min2, TypeD max2 )  { return ! ( min1 > max2 || max1 < min2 ); }


                                        // Force using functions / STL and NOT macros from minwindef.h
#if !defined (NOMINMAX)
#define         NOMINMAX
#undef          min
#undef          max
#endif


template <typename TypeD>
inline  TypeD   max         ( TypeD v1, TypeD v2 )                                              { return    std::max ( v1, v2 ); }

template <typename TypeD>
inline  TypeD   max         ( TypeD v1, TypeD v2, TypeD v3 )                                    { return    std::max ( std::max ( v1, v2 ), v3 ); }

template <typename TypeD>
inline  TypeD   max         ( TypeD v1, TypeD v2, TypeD v3, TypeD v4 )                          { return    std::max ( std::max ( v1, v2 ), std::max ( v3, v4 ) ); }

template <typename TypeD>
inline  TypeD   max         ( TypeD v1, TypeD v2, TypeD v3, TypeD v4, TypeD v5 )                { return    std::max ( crtl::max ( v1, v2, v3 ), std::max ( v4, v5 ) ); }


template <typename TypeD>
inline  TypeD   min         ( TypeD v1, TypeD v2 )                                              { return    std::min ( v1, v2 ); }

template <typename TypeD>
inline  TypeD   min         ( TypeD v1, TypeD v2, TypeD v3 )                                    { return    std::min ( std::min ( v1, v2 ), v3 ); }

template <typename TypeD>
inline  TypeD   min         ( TypeD v1, TypeD v2, TypeD v3, TypeD v4 )                          { return    std::min ( std::min ( v1, v2 ), std::min ( v3, v4 ) ); }

template <typename TypeD>
inline  TypeD   min         ( TypeD v1, TypeD v2, TypeD v3, TypeD v4, TypeD v5 )                { return    std::min ( crtl::min ( v1, v2, v3 ), std::min ( v4, v5 ) ); }

                                        // apply a max to first value
template <typename TypeD>
inline  void    Maxed       ( TypeD &vmax, TypeD v )
{
if ( vmax < v )     vmax = v;
}

                                        // apply a min to first value
template <typename TypeD>
inline  void    Mined       ( TypeD &vmin, TypeD v )
{
if ( vmin > v )     vmin = v;
}
                                        // just to increase the semantic of the operation by having a more meaningful name
                                        // also because a minimum answer is taken with a max function, which is confusing
template <typename TypeD>
inline  TypeD   AtLeast     ( TypeD v1, TypeD v2 )                                  
{
return  std::max ( v1, v2 );
}


template <typename TypeD>
inline  TypeD   NoMore      ( TypeD v1, TypeD v2 )                                  
{
return  std::min ( v1, v2 );
}
                                        // same as above - should be using a templated alias
template <typename TypeD>
inline  TypeD   AtMost      ( TypeD v1, TypeD v2 )                                  
{
return  std::min ( v1, v2 );
}

                                        // Normalize with given center, sd left and sd right
template <typename TypeD>
inline  void    ZScoreNormalized ( TypeD &v, double mean, double sdl, double sdr )
{
v       = ( v - mean ) / NonNull ( v < mean ? sdl : sdr );
}

template <typename TypeD>
inline  void    ZScoreNormalized ( TypeD &v, double mean, double sd )
{
v       = ( v - mean ) / NonNull ( sd );
}


template <typename TypeD>
inline  double  NormVector3 ( const TypeD* tov )
{
return  sqrt ( tov[ 0 ] * tov[ 0 ] + tov[ 1 ] * tov[ 1 ] + tov[ 2 ] * tov[ 2 ] ); 
}


template <typename TypeD>
inline  double  NormVector3 ( TypeD x, TypeD y, TypeD z )
{
return  sqrt ( x * x + y * y + z * z );
}


template <typename TypeD>
inline  double  Norm2Vector3 ( const TypeD* tov )
{
return  tov[ 0 ] * tov[ 0 ] + tov[ 1 ] * tov[ 1 ] + tov[ 2 ] * tov[ 2 ];
}


template <typename TypeD>
inline  double  Norm2Vector3 ( TypeD x, TypeD y, TypeD z )
{
return  x * x + y * y + z * z;
}


//----------------------------------------------------------------------------
                                        // Returns the position at the (strict) max value - in case of equality, it will return the first position
                                        // runs from fromi to toi included
template <class TypeD>
inline  int     ArgMax ( TypeD* data, int fromi, int toi )
{
if ( data == 0 )
    return  -1;

CheckOrder ( fromi, toi );


int                 arg             =       fromi;
TypeD               maxvalue        = data[   arg ];

                                        // 
for ( int i = fromi + 1; i <= toi; i++ )

    if ( data[ i ] > maxvalue ) {

        maxvalue    = data[ i ];
        arg         = i;
        }


return  arg;
}


//----------------------------------------------------------------------------
                                        // Safely setting / resetting bits within flag-like type
template <typename TypeD>
inline  TypeD   ResetFlags  (   TypeD&  var,    TypeD   clearmask   ) 
{
var     = (TypeD) ( var & ~clearmask );

return  var;
}


template <typename TypeD>
inline  TypeD   KeepFlags   (   TypeD&  var,    TypeD   keepmask   ) 
{
var     = (TypeD) ( var & keepmask );

return  var;
}

                                        // !No checks done on vars!
template <typename TypeD>
inline  TypeD   CombineFlags(   TypeD   flags1, TypeD   flags2 ) 
{
return  (TypeD) ( flags1 | flags2 );
}

template <typename TypeD>
inline  TypeD   CombineFlags(   TypeD   flags1, TypeD   flags2 , TypeD   flags3 ) 
{
return  (TypeD) ( flags1 | flags2 | flags3 );
}

                                        // alternating set / reset
template <typename TypeD>
inline  TypeD   XorFlags    (   TypeD&  var,    TypeD   xorflags    ) 
{
var     = (TypeD) ( var ^ xorflags );

return  var;
}


template <typename TypeD>
inline  TypeD   SetFlags    (   TypeD&  var,    TypeD   setflags    ) 
{
var     = (TypeD) ( var | setflags );

return  var;
}

                                        // Setting with clearing field first
template <typename TypeD>
inline  TypeD   SetFlags    (   TypeD&  var,    TypeD   clearmask,      TypeD   setflags    ) 
{
ResetFlags  ( var,  clearmask   );

SetFlags    ( var,  setflags    );

return  var;
}

                                        // Testing any combination of flag(s) (OR)
template <typename TypeD>
inline  bool    IsFlag      (   const TypeD&    var,    TypeD   testflags   ) 
{
return  var & testflags;
}

                                        // Testing for an exact flags combination (==)
template <typename TypeD>
inline  bool    IsFlag      (   const TypeD&    var,    TypeD   keepmask,   TypeD   testflags   ) 
{
return  ( var & keepmask ) == testflags;
}


//----------------------------------------------------------------------------
                                        // returns the space needed for printing the integer part explicitly
                                        // +-0.YY -> 1  +-X.YY -> 1  +-XX.YY -> 2  +-XXX.YY ->3
template <typename TypeD>
inline  int     NumIntegerDigits ( TypeD v )
{
return  v == 0 ? 1 : AtLeast ( 1, Truncate ( Log10 ( fabs ( (double) v ) ) ) + 1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Stand-alone mathematical functions
constexpr double    MinLogValue         = 1e-30;
                                        // used for error convergence
constexpr double    SingleFloatEpsilon  = 1e-6;
constexpr double    DoubleFloatEpsilon  = 1e-15;
                                        // used when big numbers are needed
constexpr double    BigSingleFloat      = 1e38;
constexpr double    BigDoubleFloat      = 1e308;


template <typename T>
constexpr double    GetMachineEpsilon ()
{
return  std::numeric_limits<T>::epsilon ();
}


template <typename T>
constexpr T     Highest ()
{
return  std::numeric_limits<T>::max ();
}


template <typename T>
constexpr T     Lowest ()
{
return  -std::numeric_limits<T>::max ();
}

                                        // parameter's type will constrain the template, so initilization will adapt if variable's type changes
template <typename T>
constexpr T     Highest ( const T& )
{
return  Highest<T> ();
}


template <typename T>
constexpr T     Lowest ( const T& )
{
return  Lowest<T> ();
}


//----------------------------------------------------------------------------

double          Log                     ( double v );   // Natural log (ln) - this function will securely check for data > 0
double          Log2                    ( double v );   // Power of 2 log
double          Log10                   ( double v );
double          LogBase                 ( double v, double base );
int             Power2Above             ( int v );
int             Power2Rounded           ( int v );
int             Power2Truncated         ( int v );
bool            IsPower2                ( int v );


double          Power                   ( double v, double p );
double          Power                   ( double v, int p );
double          SignedPower             ( double v, double p ); // anti-symetrical power: if v is negative, return the negative of the absolute power
double          PowerRoot               ( double v, double p );
double          SignedSquareRoot        ( double v );           // anti-symetrical power: if v is negative, return the negative of the absolute square root
inline double   Square                  ( double v )                                { return  pow  ( v, 2 ); }
inline long double  Square              ( long double v )                           { return  powl ( v, 2 ); }
inline double   SignedSquare            ( double v )                                { return  v >= 0 ? pow ( v, 2 ) : -Power ( -v, 2 ); }   // anti-symetrical power: if v is negative, return the negative of the absolute square
inline double   Cube                    ( double v )                                { return  pow ( v, 3 ); }
inline double   Power10                 ( double p )                                { return  pow ( 10, p ); }
inline double   SquareRoot              ( double v )                                { return  v > 0 ? sqrt ( v ) : 0; }
inline double   CubicRoot               ( double v )                                { return  v > 0 ? cbrt ( v ) : 0; }
inline double   GeometricalMean         ( double v1, double v2 )                    { return  sqrt ( ( fabs ( v1 ) + 1 ) * ( fabs ( v2 ) + 1 ) ) - 1; }

                                        // Percentage of Variance to SNR, and vice-versa
inline  double  PercentageToSnr         ( double percentage )   {  return  10 * Log10 ( 1 / percentage ); }
inline  double  SnrToPercentage         ( double snr        )   {  return  1 / Power10 ( snr / 10 );      }


bool            IsOdd                   ( int v );
bool            IsEven                  ( int v );
bool            IsInteger               ( double v );
bool            IsFraction              ( double v );
int             Sign                    ( double v );   // > 0 -> +1, < 0 -> -1
inline int      TrueToPlus              ( bool   b )                                { return  b ? +1 : -1; }    // true  -> +1, false -> -1
inline int      TrueToMinus             ( bool   b )                                { return  b ? -1 : +1; }    // true  -> -1, false -> +1
inline int      FalseToPlus             ( bool   b )                                { return  b ? -1 : +1; }    // false -> +1, true  -> -1, == TrueToMinus
inline int      FalseToMinus            ( bool   b )                                { return  b ? +1 : -1; }    // false -> -1, true  -> +1, == TrueToPlus

template <typename T>
inline bool     IsNaN                   ( const T& v )                              { return  isnan ( v ); }
template <typename T>
inline bool     IsInfinity              ( const T& v )                              { return  isinf ( v ); }
template <typename T>
inline bool     IsNotAProperNumber      ( const T& v )                              { return  ! isfinite ( v ); }   // NaN / infinite / HUGE_VAL


template <typename T>
inline T        NonNull                 ( T      v )                                { return  v != 0 ? v : 1; }             // v non-null -> v, else 1
template <typename T>
inline T        AbsNonNull              ( T      v )                                { return  v > 0 ? v : v < 0 ? -v : 1; } // v non-null -> |v|, else 1
double          Fraction                ( double v );
int             Truncate                ( double v );
double          TruncateTo              ( double v, double precision );
double          ModuloTo                ( double v, double precision );
int             Round                   ( double v );
double          RoundTo                 ( double v, double precision );
int             RoundToOdd              ( int    v );
double          RoundToOdd              ( double v, double precision );
int             RoundToEven             ( int    v );
double          RoundToEven             ( double v, double precision );
int             RoundAbove              ( double v );   // 1.0 -> 1  1.1/1.2/../1.99 -> 2
int             RoundBelow              ( double v );   // 
double          RoundToAbove            ( double v, double precision );
double          RescalingFactor         ( double maxinput, double maxoutput );

double          RelativeDifference      ( double v1, double v2 );   // results are always positive
double          RelativeDifference      ( double v1, double v2, double v3 );

double          NormalizeAngle          ( double angle, double base );  // returns angle in [0..base)

enum            RelativeIntervalOverlapEnum
                {
                RelativeIntervalOverlapMin,         // compared to widest interval, giving the smallest possible value
                RelativeIntervalOverlapMax,         // compared to narrowest interval, giving the biggest possible value
                RelativeIntervalOverlapToFirst,     // compared to first interval
                RelativeIntervalOverlapToSecond,    // compared to second interval
                };

double          RelativeIntervalOverlap ( long l1, long r1, long l2, long r2, RelativeIntervalOverlapEnum how );    // 0:no overlap  1:total overlap or inclusion else:highest percentage of coverage

inline  double  Percentage              ( double numerator, double denominator )    { return  denominator != 0 ? Clip ( numerator / denominator, 0.0, 1.0 ) * 100 : 0; }


inline  int     IndexesToLinearIndex    ( int i1, int i2, int dim2 )                { return i1 * dim2 + i2; }
inline  void    LinearIndexToIndexes    ( int li, int& i1, int& i2, int dim2 )      { i1 = li / dim2; i2 = li % dim2; }

                                        // Returning the left/right position with proper mirroring - Always returning within boundaries
inline  int     LeftMirroring           ( int fromtf, int delta, int maxtf )        { return    NoMore  ( maxtf,         abs (           fromtf - delta )   );   }
inline  int     RightMirroring          ( int fromtf, int delta, int maxtf )        { return    AtLeast ( 0,     maxtf - abs ( maxtf - ( fromtf + delta ) ) ); }


//----------------------------------------------------------------------------
                                        // Conversion distance <-> correlation
double          DifferenceToCorrelation         ( double diff       );
double          SquareDifferenceToCorrelation   ( double squarediff );
double          CorrelationToDifference         ( double corr       );
double          CorrelationToSquareDifference   ( double corr       );
double          CorrelationToACosDistance       ( double corr       );
                                        // Spherical Standard Deviation from average of spherical vectors R
double          RtoSphericalSD                  ( double R );
double          RtoSphericalGaussianSD          ( double R );

                                        
//----------------------------------------------------------------------------
                                        // Exponential distribution
double          Exponential             ( double x, double center, double width, double height, double rate );
                                        // Gamma distribution
double          Gamma                   ( double x, double center, double width, double height, int shape, double scale );

                                        // Gaussian (!width not sigma!)
double          Gaussian                ( double x, double center, double width, double height );
                                        // Normal probability distribution
double          Normal                  ( double x, double mean, double sigma );
//double        AsymmetricalNormal      ( double x, double mean, double sigmaleft, double sigmaright ); // prototype, not tested
                                        // used to convert to the Gaussian function above
double          GaussianWidthToSigma    ( double w );
double          GaussianSigmaToWidth    ( double s );
double          GaussianFWHMToSigma     ( double w );
double          GaussianSigmaToFWHM     ( double s );
                                        // Gaussian Cumulated Distribution Function (!width not sigma!)
double          GaussianCDF             ( double x, double center, double width );
                                        // Gaussian with skew factor, note that skew = 0 -> Gaussian
double          GeneralizedGaussian     ( double x, double center, double width, double height, double skew );
                                        // Gaussian with skew factor, note that skew = 0 -> Gaussian
double          AsymmetricalGaussian    ( double x, double center, double widthleft, double widthright, double height );
                                        // Gaussian Cumulated Distribution Function (!width not sigma!)
double          GeneralizedGaussianCDF  ( double x, double center, double width, double skew );
                                        // returned values x1 <= x2
int             GaussiansIntersections  ( double c1, double w1, double h1, double c2, double w2, double h2, double &x1, double &x2 );
                                        // returns the normalized distance between 2 Gaussians (width parameter, not sigma)
double          GaussiansDistance       ( double c1, double w1, double c2, double w2 );
                                        // allocate & compute a Gaussian used for cache / Kernel
void            GetGaussianCache        ( double gaussianwidth, TArray1<double> &cache, int cacheradius );

                                        // Hanning style
double          RaisedCosine            ( double x, double center, double width, double height, double power );
                                        // Student-like function, factors are not the offical ones
double          Student                 ( double x, double center, double width, double height, int degree );
                                        // transform a covariance / correlation distribution to a Normal one
constexpr double    NumFisherSD         = 3.0;
constexpr double    NumFisherSDTanh     = 0.99505475368673045133188018525549;
double          PearsonToFisher         ( double corr );

double          PearsonToKendall        ( double corr );

double          ZScore                  ( double v, double mean, double sd );
double          ZScore                  ( double v, double mean, double sd, bool absolute );
double          ZScoreAsym              ( double v, double mean, double sdleft, double sdright );

                                        // v in [0..1] (will be clipped anyway)
double          Hanning                 ( double v );

                                        // v in [-1..1], returned values in [-1..1], k>0 inflexion is horizontal, k<0 inflexion is vertical
double          BoundedSigmoid          ( double v, double k );

                                        // Wilson - Hilferty (1931) formula to convert from a Chi Square to a Normal Distribution
double          ChiSquareToNormal_WilsonHilferty    ( double v /*, int dof*/ );
double          ChiToNormal_WilsonHilferty          ( double v /*, int dof*/ );
                                        // Hernandez - Johnson formula (1980)
double          ChiSquareToNormal_HernandezJohnson  ( double v, int dof );
double          ChiToNormal_HernandezJohnson        ( double v, int dof );
double          BoxCox                  ( double v, double lambda );
                                        // Centralizing the transformation (re-expression) from Chi to Normal distribution
double          Vector3PowerToNormal    ( double v );
double          Vector3NormToNormal     ( double v );
double          NormalToVector3Norm     ( double v );
double          Vector6NormToNormal     ( double v );
double          VectorNormToNormal      ( double v, int dof );

                                        // Reverting the formula
//double          NormalToChiSquare       ( double v, int dof );
                                        // reverting the above transform
//double          NormalToVectorNorm      ( double v );

                                        // Cubic Hermit Spline, equivalent to Keys with a=-0.5
                                        // interpolation is done between x0 and x1
double          CubicHermiteSpline      ( double xp, double x0, double x1, double x2, double t );
                                        // Uniform Cubic B-Spline - Blurrier than Cubic, but maybe we lack a preprocessing step?
                                        // interpolation is done between x0 and x1
double          UniformCubicBSpline     ( double xp, double x0, double x1, double x2, double t );
                                        // Mexican Hat function / Ricker wavelet - +- 1 sigma is the positive part, then +-3 sigma the main negative
double          MexicanHat              ( double x, double center, double width, double height );
                                        // classical way with determinant, and a != 0
int             SolveQuadratic          ( double a, double b, double c, double &x1, double &x2 );

                                        // Just a handy function to do a linear mapping
double          LinearRescale           (   double  x,  
                                            double  xmin,   double  xmax,
                                            double  toymin, double  toymax,
                                            bool    clipborders
                                        );
                                        // Same as above, but if the mapping "stretches", add some noise to have all intermediate values
double          LinearRescaleNoise      (   double  x,  
                                            double  xmin,   double  xmax,
                                            double  toymin, double  toymax
                                        );


//----------------------------------------------------------------------------
                                        // Direct and optimized, iterative Legendre functions
long double     Legendre                        ( long double x, int n );
long double     NextLegendre                    ( long double& Pnm2, long double& Pnm1, long double& Pn, long double x, int n );

long double     AssociatedLegendreOrder1        ( long double x, int l );
long double     NextAssociatedLegendreOrder1    ( long double& Pnm2, long double& Pnm1, long double& Pn, long double x, int l );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // 2 * Pi / 1e-3 = 6283 -> 1e-3 precision
constexpr int       PredefinedArraySizeTrigo    = 6283;
constexpr int       PredefinedArraySizeLanczos  = 6283;


//----------------------------------------------------------------------------
                                        // Handy predefined constants with enough precision
constexpr double    Pi                          = 3.1415926535897932384626433832795;
constexpr double    TwoPi                       = 6.2831853071795864769252867665590;
constexpr double    FourPi                      = 12.566370614359172953850573533118;
constexpr double    SqrtTwoPi                   = 2.506628274631000502415765284811;
constexpr double    HalfPi                      = 1.5707963267948966192313216916398;
constexpr double    SqrtTwo                     = 1.4142135623730950488016887242097;
constexpr double    SqrtThree                   = 1.7320508075688772935274463415059;

constexpr int       Lanczos2Size                = 2;
constexpr int       Lanczos2Size2               = 2 * Lanczos2Size;
constexpr int       Lanczos3Size                = 3;
constexpr int       Lanczos3Size2               = 2 * Lanczos3Size;


double          FastCosine      ( double angle );
double          FastSine        ( double angle );
double          FastLanczos2    ( double x );   // x: [-2..2]
double          FastLanczos3    ( double x );   // x: [-3..3]

                                        // returned in radian, always positive
double          ArcTangent      ( double y, double x );
double          Cotangent       ( double angle );


inline double   DegreesToRadians ( double degrees )     { return  degrees * ( Pi  / 180 ); }
inline double   RadiansToDegrees ( double radians )     { return  radians * ( 180 / Pi  ); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A Gaussian variable
class  TGaussian
{
public:
                    TGaussian ();
                    TGaussian ( double center, double radius, double numsd );

                                                            // # of SD to fit into radius
    void            Set ( double center, double radius, double numsd );

    double          operator    ()      ( double v );

protected:
    double          Center;
    double          Exponent;
    double          Constant;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







