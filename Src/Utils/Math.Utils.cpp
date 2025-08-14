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
//
//----------------------------------------------------------------------------


#include    <stdio.h>
#include    <stdlib.h>
#include    <iostream>
#include    <iomanip>
#include    <io.h>
#include    <float.h>
//#include    <exception>

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "MemUtil.h"

#include    "Strings.Utils.h"
#include    "TBaseDialog.h"
#include    "Math.Random.h"
#include    "Math.Utils.h"

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // its long double version
//int     _matherrl ( struct exceptionl *a )
/*
                                        // math library error handling (but not floating-point exception, like / 0 )
int     _matherr ( class exception* a )
{
//DBGM ( a->name, "matherr" );


if      ( StringIs ( a->what (), "exp" ) ) {

    if ( a->type == OVERFLOW ) {
        a->retval   = DBL_MAX;
        return  1;
        }

    else if ( a->type == UNDERFLOW ) {
        a->retval   = 0;
        return  1;
        }

    // others to implement: DOMAIN, SING, PLOSS, TLOSS

    else {
        a->retval   = 0;
        return  1;
        }
    } // exp


else if ( StringIs ( a->name, "sqrt" ) ) {

    if ( a->type == DOMAIN ) {
//      if ( _isnan ( a->arg1 ) )       // how do we test for a f...ing NaN?
            a->retval   = 0;
//      else
//          a->retval   = sqrt ( - a->arg1 );
        return  1;
        }

    else {
        a->retval   = 0;
        return  1;
        }
    } // sqrt


else
    return  0;
}
*/

/*// from: http://support.microsoft.com/kb/167750

#include <stdio.h>
#include <math.h>
#include <ERRNO.H>
#include <string.h>
extern int errno;

// The _matherr routine can be customized to handle the specific
//  underflow problem that is encountered.  See online help for
//  more information on _matherr().

int _matherr( struct _exception *except )
{
    // Handle errors caused by pow() function.
    if(strcmp(except->name,"pow")==0)
    {
        switch(except->type)
        {
            case _DOMAIN:
            {
                printf("Domain Error: Argument not in domain.\n");
                return 1;
                break;
            }
            case _SING:
            {
                printf("Singularity Error: Argument singularity "
                       "error.\n");
                return 1;
                break;
  }
            case _OVERFLOW:
            {
                printf("Overflow Error: Overflow range error.\n");
                return 1;
                break;
            }
            case _PLOSS:
            {
                printf("Precision Error: Partial loss of "
                       "significance.\n");
                return 1;
                break;
            }
            case _TLOSS:
            {
                printf("Precision Error: Total loss of "
                       "significance.\n");
                return 1;
                break;
  }
            case _UNDERFLOW:
  {
                printf("Underflow Error: The result is too small to "
                       "be represented.\n");
                return 1;
                break;
            }
            default:
            {
                printf("Unknown Error Occurred While Performing Math "
                       "Operation.\n");
                return 1;
                break;
  }
   }
    }
    else
        return 0;  // non-error condition
}

void main(void)
{

double x = -1.0e+307;
double y = -1.0e+307;
double z;

z = pow(x,y);

//Check for error conditions.  _matherr() is automatically
//called should there be a math related error.

// Note that errno is not set properly & _matherr() is called.
if (errno != 0)
    printf("ERROR! Errno: %i\n",errno);
else
    printf("pow( %e, %e ) = %f\n", x, y, z);
}
*/


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace crtl {

//----------------------------------------------------------------------------
double      Log ( double v )
{
return  log ( AtLeast ( DBL_MIN, v ) );
}


double      Log2 ( double v )
{
return  std::log2 ( AtLeast ( DBL_MIN, v ) );
}


double      Log10 ( double v )
{
return  std::log10 ( AtLeast ( DBL_MIN, v ) );
}


double      LogBase ( double v, double base )
{
return  std::log ( AtLeast ( DBL_MIN, v ) ) / std::log ( base );
}


int         Power2Above ( int v )
{
return  v > 0 ? 1L << RoundAbove ( Log2 ( v ) ) : 0;
}


int         Power2Rounded ( int v )
{
return  v > 0 ? 1L << Round ( Log2 ( v ) ) : 0;
}


int         Power2Truncated ( int v )
{
return  v > 0 ? 1L << Truncate ( Log2 ( v ) ) : 0;
}


bool        IsPower2 ( int v )
{
return  ( v & ( v - 1 ) ) == 0;
}


double      Power ( double v, double p )
{
                                        // force 0^0 = 0
return  v > 0 ? pow ( v, p ) : 0;
}

                                        // used for polynomial stuff
double      Power ( double v, int p )
{
if      ( p == 0 )      return  1;      // force 0^0 = 1, as this is rather used for polynomials
else                    return  pow ( v, p );
}


double      PowerRoot ( double v, double p )
{
                                        // force 0^0 = 0
return  v > 0 ? pow ( v, 1.0 / p ) : 0;
}

                                        // anti-symetrical power: if v is negative, return the negative of the absolute power
double      SignedPower ( double v, double p )
{
return  v >= 0 ? Power ( v, p ) : -Power ( -v, p );
}


double      SignedSquareRoot ( double v )
{
return  v >= 0 ? sqrt ( v ) : -sqrt ( -v );
}


//----------------------------------------------------------------------------
                                        // Also OK for negative values
bool        IsOdd ( int v )
{
return  v & 1;
}

                                        // Also OK for negative values
bool        IsEven ( int v )
{
return  ! ( v & 1 );
}


bool        IsMultiple ( int v, int of )
{
return  ! ( v % of );
}


bool        IsNotMultiple ( int v, int of )
{
return  v % of;
}

                                        // !0 case is ambiguous!
bool        IsInteger ( double v )
{
return  /*v != 0 &&*/ Fraction ( v ) == 0;
}


bool        IsFraction ( double v )
{
return  Fraction ( v ) != 0;
}


//----------------------------------------------------------------------------
int         Sign ( double v )
{
return  v == 0 ? 0 : v > 0 ? 1 : -1;
}


double      Fraction ( double v )
{
return  v - Truncate ( v );
}


int         Truncate ( double v )
{
return  (int) v;
}


double      TruncateTo ( double v, double precision )
{
return  ( Truncate ( v / precision ) ) * precision;
}


double      ModuloTo ( double v, double precision )
{
return  v - TruncateTo ( v, precision );
}


int         Round ( double v )
{
return  v < 0 ? (int) ( v - 0.5 ) : (int) ( v + 0.5 );
}

                                        // f.ex. 8.56 & 0.1 -> 8.6
double      RoundTo ( double v, double precision )
{
return  ( Round ( v / precision ) ) * precision;
}
                                        // 1 2 3 4 5 6 
                                        // 1 3 3 5 5 7
int         RoundToOdd ( int v )
{
return  2 * ( v / 2 ) + 1;
}

                                        // get an odd number of multiples of precision
double      RoundToOdd ( double v, double precision )
{
return  ( max ( 1, Round ( ( v / precision - 1 ) / 2 ) ) * 2 + 1 ) * precision;
}
                                        // 1 2 3 4 5 6 
                                        // 2 2 4 4 6 6
int         RoundToEven ( int v )
{
return  2 * ( ( v + 1 ) / 2 );
}

                                        // get an even number of multiples of precision
double      RoundToEven ( double v, double precision )
{
return  max ( 1, Round ( v / ( 2 * precision ) ) ) * ( 2 * precision );
}
                                        // round to the next higher integer if fraction, or same value if integer
                                        // -1.0 -0.99 -0.5 -0.01  0  ...  1.0  1.1  1.5  1.99  2.0
                                        // -1    0     0    0     0  ...  1    2    2    2     2 
int         RoundAbove ( double v )
{
return  IsInteger ( v ) ? v : Truncate ( v ) + ( v > 0 ? 1 : 0 );
}

                                        // round to the next lower integer if fraction, or same value if integer
int         RoundBelow ( double v )
{
return  IsInteger ( v ) ? v : Truncate ( v ) + ( v > 0 ? 0 : -1 );
}


double      RoundToAbove ( double v, double precision )
{
return  ( RoundAbove ( v / precision ) ) * precision;
}

                                        // Find a power of 10 rescaling factor for the input, as to fit into maxoutput
double      RescalingFactor ( double maxinput, double maxoutput )
{
double              digitratio      = RoundBelow ( Log10 ( abs ( maxoutput ) / abs ( maxinput ) ) );
double              rescalefactor   = Power      ( 10, digitratio );

//DBGV3 ( maxinput, maxoutput, rescalefactor, "maxinput, maxoutput -> rescalefactor" );

return  rescalefactor;
}


//----------------------------------------------------------------------------
                                        // returns angle in [0..base), base could be 2 * Pi, 360, or even 180
double      NormalizeAngle ( double angle, double base )
{
                                        // this is a floating point modulo
angle      -= TruncateTo ( angle, base );
                                        // test twice
if ( angle < 0 )    angle  += base;
if ( angle < 0 )    angle  += base;

return  angle;
}


//----------------------------------------------------------------------------
double      RelativeDifference ( double v1, double v2 )
{
                                        // comparing to the average of v1+v2
return  v1 + v2 == 0 ? 0 : abs ( v1 - v2 ) / abs ( v1 + v2 ) * 2;
}

                                        // return the worst case: the max of relative differences beween all pairs
                                        // another possibility woud be to test against the mean of the 3
double      RelativeDifference ( double v1, double v2, double v3 )
{
return  max ( RelativeDifference ( v1, v2 ), 
              RelativeDifference ( v1, v3 ), 
              RelativeDifference ( v2, v3 ) );
}


double      RelativeIntervalOverlap ( long l1, long r1, long l2, long r2, RelativeIntervalOverlapEnum how )
{
                                        // no overlap?
if ( r2 < l1 || r1 < l2 )
    return  0;

                                        // here we have partial overlap

                                        // absolute overlapping distance
double              overlap         = min ( r1, r2 ) - max ( l1, l2 ) + 1;

                                        // absolute distance to compare with
double              comparedto      = how == RelativeIntervalOverlapMin      ? max ( r1 - l1 + 1, r2 - l2 + 1 ) // returned value is min, so interval is max
                                    : how == RelativeIntervalOverlapMax      ? min ( r1 - l1 + 1, r2 - l2 + 1 ) // returned value is max, so interval is min
                                    : how == RelativeIntervalOverlapToFirst  ?            r1 - l1 + 1
                                    : how == RelativeIntervalOverlapToSecond ?                    r2 - l2 + 1
                                    :                                          max ( r1 - l1 + 1, r2 - l2 + 1 );

                                        // clip results to [0..1]
return              Clip ( overlap / NonNull ( comparedto ), 0.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // diff in [0..2]
double      DifferenceToCorrelation         ( double diff       )
{
return  1 - Square ( diff ) / 2;
}

                                        // square ( diff ) in [0..4]
double      SquareDifferenceToCorrelation   ( double squarediff )
{
return  1 - squarediff / 2;
}

                                        // corr in [-1..1]
double      CorrelationToDifference         ( double corr       )
{
return  sqrt ( 2 * ( 1 - corr ) );
}

                                        // corr in [-1..1]
double      CorrelationToSquareDifference   ( double corr       )
{
return  2 * ( 1 - corr );
}

                                        // if needed, use this formula to build other converters
double      CorrelationToACosDistance       ( double corr       )
{
return  acos ( corr ) / Pi;
}


//----------------------------------------------------------------------------
                                        // Converting average spherical data R to SD - results in [0..1]
double      RtoSphericalSD          ( double R )
{
return  Clip ( 1 - R, 0.0, 1.0 );
}

                                        // Converting average spherical data R to spherical Gaussian SD - results in [0..inf)
double      RtoSphericalGaussianSD  ( double R )
{
return  sqrt ( AtLeast ( 0.0, - 2 * Log ( R ) ) );
}


//----------------------------------------------------------------------------
double      ZScore ( double v, double mean, double sd )
{
return  ( v - mean ) / NonNull ( sd );
}


double      ZScore ( double v, double mean, double sd, bool absolute )
{
return  ( absolute ? abs ( v - mean )
                   :     ( v - mean ) ) / NonNull ( sd );
}


double      ZScoreAsym ( double v, double mean, double sdleft, double sdright )
{
v      -= mean;

return  v / NonNull ( v >= 0 ? sdright : sdleft );
}

                                        
//----------------------------------------------------------------------------
                                        // Wilson - Hilferty (1931): basically a cubic root, approximation working better for higher dof
                                        // Variance is 1 / 3^2 * 2 / dof
                                        // Mean     is abs ( 1 - Variance )
                                        // Skipping the constant, which is useless for later standardization
double      ChiSquareToNormal_WilsonHilferty ( double v /*, int dof*/ )
{
return  pow ( v /* / dof*/, 1 / 3.0 );
}
                                        // Same as above, just doing both squaring and correction at once
                                        // Skipping the constant, which is useless for later standardization
double      ChiToNormal_WilsonHilferty ( double v /*, int dof*/ )
{
return  pow ( v, 2 / 3.0 ) /* / pow ( dof, 1 / 3.0 )*/;
}


//----------------------------------------------------------------------------
double      BoxCox ( double v, double lambda )
{
return  lambda  ? ( pow ( v, lambda ) - 1 ) / lambda
                : log ( v );
}
                                        // Hernandez - Johnson (1980): using the Gamma distribution ( alpha = dof/2; beta = 0.5 )
                                        // Converges to Wilson - Hilferty for increasing dof
                                        // Using the power function instead of the actual Box-Cox function for simplicity, and avoiding confusing negative values
#define     HernandezJohnsonLambda1     0.2084
#define     HernandezJohnsonLambda2     0.2654
#define     HernandezJohnsonLambda3     0.2887
#define     HernandezJohnsonLambda4     0.3006
#define     HernandezJohnsonLambda6     0.3124


double      ChiSquare3ToNormal_HernandezJohnson ( double v )
{
return  pow /*BoxCox*/ ( v, HernandezJohnsonLambda3 );
}


double      ChiSquare6ToNormal_HernandezJohnson ( double v )
{
return  pow /*BoxCox*/ ( v, HernandezJohnsonLambda6 );
}


double      ChiSquareToNormal_HernandezJohnson ( double v, int dof )
{
#if defined(CHECKASSERT)
assert ( dof >= 1 && dof <= 6 && dof != 5 );
#endif

return  dof == 3 ?  pow /*BoxCox*/ ( v, HernandezJohnsonLambda3 )    // our most common case
      : dof == 6 ?  pow /*BoxCox*/ ( v, HernandezJohnsonLambda6 )    // our second to most common case
      : dof == 1 ?  pow /*BoxCox*/ ( v, HernandezJohnsonLambda1 )
      : dof == 2 ?  pow /*BoxCox*/ ( v, HernandezJohnsonLambda2 )
      : dof == 4 ?  pow /*BoxCox*/ ( v, HernandezJohnsonLambda4 )
      :             0;
}


double      NormalToChi3_HernandezJohnson ( double v )
{
return  pow /*BoxCox*/ ( v, 1 / ( 2 * HernandezJohnsonLambda3 ) );
}

                                        // Same as above, just doing both squaring and correction at once
double      Chi3ToNormal_HernandezJohnson ( double v )
{
return  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda3 );
}


double      Chi6ToNormal_HernandezJohnson ( double v )
{
return  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda6 );
}


double      ChiToNormal_HernandezJohnson ( double v, int dof )
{
#if defined(CHECKASSERT)
assert ( dof >= 1 && dof <= 6 && dof != 5 );
#endif

return  dof == 3 ?  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda3 )    // our most common case
      : dof == 6 ?  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda6 )    // our second to most common case
      : dof == 1 ?  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda1 )
      : dof == 2 ?  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda2 )
      : dof == 4 ?  pow /*BoxCox*/ ( v, 2 * HernandezJohnsonLambda4 )
      :             0;
}


//----------------------------------------------------------------------------
double      Vector3PowerToNormal ( double v )
{
//return  ChiSquareToNormal_WilsonHilferty    ( v );
return  ChiSquare3ToNormal_HernandezJohnson ( v );
}


double      Vector3NormToNormal ( double v )
{
//return  ChiToNormal_WilsonHilferty    ( v );
return  Chi3ToNormal_HernandezJohnson ( v );
}


double      NormalToVector3Norm ( double v )
{
return  NormalToChi3_HernandezJohnson ( v );
}


double      Vector6NormToNormal ( double v )
{
//return  ChiToNormal_WilsonHilferty    ( v );
return  Chi6ToNormal_HernandezJohnson ( v );
}


double      VectorNormToNormal ( double v, int dof )
{
//return  ChiToNormal_WilsonHilferty       ( v );
return  ChiToNormal_HernandezJohnson       ( v, dof );
}


//double      NormalToChiSquare ( double v, int dof )
//{
//return  dof * Cube ( v );
//}
//
//
//double      NormalToVectorNorm ( double v, int dof )
//{
//                                        // Normal back to Chi Square, then revert the square of norm
//return  sqrt ( NormalToChiSquare ( v, dof ) );
//}


//----------------------------------------------------------------------------
                                        // Gaussian to Normal distribution ( width = sqrt ( 2 ) * sigma, sigma = width / sqrt ( 2 ) )
                                        // !this depends on how we write the Gaussian function, i.e. if there is a 1/2 or not!
//x       = Gaussian ( x, center, SqrtTwo * sigma, 1.0 / sqrt ( TwoPi * sigma * sigma ) );
double      GaussianWidthToSigma    ( double w )   { return w / SqrtTwo; }
double      GaussianSigmaToWidth    ( double s )   { return s * SqrtTwo; }

                                        // General mathematical case, with Gaussian with -1/2 in exponential
                                        // to cut at different height, like a 10th of height, change to log ( height )
double      GaussianFWHMToSigma     ( double w )   { return w / ( 2 * sqrt ( 2 * log ( (double) 2 ) ) ); }
double      GaussianSigmaToFWHM     ( double s )   { return s * ( 2 * sqrt ( 2 * log ( (double) 2 ) ) ); }

                                        // Gaussian with WIDTH and height parameter - integral is therefore NOT 1
double      Gaussian ( double x, double center, double width, double height )
{
if ( width == 0 )
    return  x == center ? height : 0;

x       = ( x - center ) / width;

return  height * exp ( - Square ( x ) );
}

                                        // Normal distribution, with mean and SIGMA parameter - integral is 1
double      Normal ( double x, double mean, double sigma )
{
if ( sigma == 0 )
    return  x == mean ? 1 : 0;

return  exp ( - Square ( ( x - mean ) / sigma ) / 2 ) / ( SqrtTwoPi * sigma );
}

/*
double      AsymmetricalNormal ( double x, double mean, double sigmaleft, double sigmaright )
{
return  Normal ( x, mean, x < mean ? sigmaleft : sigmaright );

//                                        // Forcing the "Gaussian" to have the same height left and right - not sure about the formula
//if ( sigmaleft == 0 && x <= mean || sigmaright == 0 && x >= mean )
//    return  x == mean ? 1 : 0;
//                                        // use either of the left or right sigma
//double              sigma           = x < mean ? sigmaleft : sigmaright;
//                                        // average area of the 2 left and right Gaussians
//double              area            = SqrtTwoPi * ( sigmaleft + sigmaright ) / 2;
//
//return  exp ( - Square ( ( x - mean ) / sigma ) / 2 ) / area;
}
*/
                                        // Gaussian with skew factor (Wikipedia formula is not correct as for 2012)
// rgm2.lab.nig.ac.jp/RGM2/func.php?rd_id=lmom:cdfgno
// Inverted the skew parameter, so that it gives the regular behavior "positive skewness -> flatter on the right"
// skew = 0 -> regular Gaussian
double      GeneralizedGaussian ( double x, double center, double width, double height, double skew )
{
if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x      -= center;

                                        // test range
if ( skew > 0 && x <= - width / skew
  || skew < 0 && x >= - width / skew )
    return  0;

                                        // coordinate change
double              y           = skew != 0 ? log ( 1 + skew * x / width ) / skew
                                            :                  x / width;
      // Normal ( y )
return  height * exp ( - Square ( y ) );
}


double      AsymmetricalGaussian ( double x, double center, double widthleft, double widthright, double height )
{
return  Gaussian ( x, center, x < center ? widthleft : widthright, height );
}

                                        // Gaussian CDF
// Implementation with 16 bits precision, as in:
// West, Graeme (2009), "Better approximations to cumulative normal functions", Wilmott Magazine: 70–76. (www.wilmott.com/pdfs/090721_west.pdf)
double      GaussianCDF ( double x, double center, double width )
{
if ( width == 0 )
    return  x >= center ? 1 : 0;

width   = abs ( width );
                                        // normalize (use sigma, not width)
x       = ( x - center ) / GaussianWidthToSigma ( width );

                                        // normal to CDF
double              xabs            = abs ( x );
double              cdf;
double              e;
double              b;


if ( xabs > 37 )
    cdf         = 0;

else {
    e           = exp ( -xabs * xabs / 2 );

    if ( xabs < 7.07106781186547 ) {
        b       = 3.52624965998911e-2 * xabs +   0.700383064443688;
        b       = b                   * xabs +   6.37396220353165;
        b       = b                   * xabs +  33.912866078383;
        b       = b                   * xabs + 112.079291497871;
        b       = b                   * xabs + 221.213596169931;
        b       = b                   * xabs + 220.206867912376;

        cdf     = e * b;

        b       = 8.83883476483184e-2 * xabs +   1.75566716318264;
        b       = b                   * xabs +  16.064177579207;
        b       = b                   * xabs +  86.7807322029461;
        b       = b                   * xabs + 296.564248779674;
        b       = b                   * xabs + 637.333633378831;
        b       = b                   * xabs + 793.826512519948;
        b       = b                   * xabs + 440.413735824752;

        cdf    /= b;
        }
    else {
        b       = xabs + 0.65;
        b       = xabs + 4 / b;
        b       = xabs + 3 / b;
        b       = xabs + 2 / b;
        b       = xabs + 1 / b;

        cdf     = e / b / 2.506628274631;
        }
    }


if ( x > 0 )
    cdf         = 1 - cdf;

return  cdf;
}

                                        // Gaussian with skew factor CDF
double      GeneralizedGaussianCDF ( double x, double center, double width, double skew )
{
if ( width == 0 )
    return  x >= center ? 1 : 0;

width   = abs ( width );

x      -= center;

                                        // test range
if ( skew > 0 && x <= - width / skew
  || skew < 0 && x >= - width / skew )
    return  0;

                                        // coordinate change
double              y           = skew != 0 ? log ( 1 + skew * x / width ) / skew
                                            :                  x / width;

return  GaussianCDF ( y, 0, 1 );
}

                                        //
double      Student ( double x, double center, double width, double height, int degree )
{
if ( degree < 1 )
    return  0;

if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x       = ( x - center ) / width;

return  height * Power ( 1 + Square ( x ) / degree, - ( degree + 1 ) / 2.0 );
}


//----------------------------------------------------------------------------
                                        // Returns a z-scored value z, the Fischer transform of a correlation r.
                                        // This is a variance-stabilizing transform, to be used before analysis of variance or regression techniques.
                                        // Note that it can also be applied to Spearman correlation.
                                        // The variance of r is much smaller near +-1, this formula expands near +-1 to compensate
double      PearsonToFisher ( double corr )
{
                                        // Problem with values +-1, data that we do encounter, which would produce infinite results.
                                        // Clipping the input, f.ex to 0.99, is not an option, as we therefore collapse all data
                                        // from 0.99 to 1.00 together, 0.99 = 0.995 = 0.998 etc...
                                        // The other approach is to artificially downscale by a small factor the input data
                                        // so as to limit the output range. The scaling factor can be smartly picked so that the
                                        // output range is f.ex. between [-3..3] or [-6..6]

                                        // With rescaling factor of tanh ( NumFisherSD ), the output range is [-NumFisherSD..NumFisherSD]
corr   *= NumFisherSDTanh;

return  log ( ( 1 + corr ) / ( 1 - corr ) ) / 2;

                                        // Just incase, the other way round:
//corr    = ( exp ( 2 * fisher ) - 1 ) / ( exp ( 2 * fisher ) + 1 ) / NumFisherSD;
}

                                        // Converts from Pearson to Kendall Tau, which compares the relative orders of ranked values,
                                        // for bivariate Normal distribution. It "streches" a bit the values near +1 and -1.
                                        // See: Lindskog, McNeil, Schmock "Kendall's Tau for Elliptical Distributions", 2001
double      PearsonToKendall ( double corr )
{
return      asin ( corr ) / HalfPi;
}


//----------------------------------------------------------------------------
double      RaisedCosine ( double x, double center, double width, double height, double power )
{
if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x   = ( x - center ) / width;

return  x <= -1 || x >= 1 ? 0
                          : height * Power ( ( FastCosine ( x * Pi ) + 1 ) / 2, power );
}

                                        // v in [0..1] -> Hanning in [0..1..0]
double      Hanning ( double v )
{
return  Clip ( ( 1 - cos ( Clip ( v, 0.0, 1.0 ) * TwoPi ) ) / 2, 0.0, 1.0 );
}


constexpr double    numcyclesinhanning  = 2; // 2 * ( SqrtTwo * 6 / TwoPi ); // = 2.70 - from litterature

                                        // Formula that does a good job for cleaning borders of wavelets, using 2 full cycles on each side (instead of 1)
double      HanningBorder ( int blocksize, int freqi )
{
                                        // Hanning window size, one at each edge of the data
double              sthwd           = Clip ( numcyclesinhanning * blocksize / (double) freqi, 0.0, blocksize / 2.0 );    

return  sthwd;
}

                                        // Another way to write it
double      HanningBorder ( double samplingfrequency, double freq, int blocksize )
{
                                        // Hanning window size, one at each edge of the data
double              sthwd           = Clip ( numcyclesinhanning * samplingfrequency / freq, 0.0, blocksize / 2.0 );    

return  sthwd;
}


//----------------------------------------------------------------------------
                                        // v in [-1..1], k controls the steepness and the inflexion direction
                                        // k > 0, inflexion is horizontal, steeper for k->0
                                        //      |
                                        //   ___/
                                        //  /
                                        //  |
                                        // 
                                        // k < 0, inflexion is vertical, steeper for k->0
                                        //     __
                                        //    /
                                        //    |  
                                        //   /
                                        // --
double      BoundedSigmoid ( double v, double k )
{
                                        // forbid k in [-1..0]
                                        // also makes the k parameter symetric -0.2 is like 0.2, except reverted
if ( k < 0 )
    k--;

return  k * abs ( v ) / ( k - abs ( v ) + 1 ) * Sign ( v );
}


//----------------------------------------------------------------------------
double      Exponential ( double x, double center, double width, double height, double rate )
{
if ( rate <= 0 )
    return  0;

if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x       = ( x - center ) / width;

return  // x <= 0 ? 0                               // official definition
                    height * exp ( - rate * x );    // height is equivalent to a center shift (?)
}

                                                                           // k             Theta
double      Gamma ( double x, double center, double width, double height, int shape, double scale )
{
if ( shape <= 0 || scale <= 0 )
    return  0;

if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x       = ( x - center ) / width;

return  x <= 0 ? 0                              // official definition
               :   height
                 * Power ( x, shape - 1 )       // k: 1=exponential, 2=log-normal, ++=Gaussian
                 * exp   ( - x / scale );
}


//----------------------------------------------------------------------------
                                        // Cubic Hermite Spline
                                        // (interpolation is done between x0 and x1)
double      CubicHermiteSpline ( double xp, double x0, double x1, double x2, double t )
{
double              t2              = t * t;
double              t3              = t * t2;

                                        // using the 4 basis functions:
return (   (     -t3 + 2 * t2 - t     ) * xp
         + (  3 * t3 - 5 * t2     + 2 ) * x0
         + ( -3 * t3 + 4 * t2 + t     ) * x1
         + (      t3 -     t2         ) * x2 ) / 2;
}

                                        // Uniform Cubic B-Spline - Blurrier than Cubic, but maybe we lack a preprocessing step?
                                        // (interpolation is done between x0 and x1)
double      UniformCubicBSpline ( double xp, double x0, double x1, double x2, double t )
{
double              t2              = t * t;
double              t3              = t * t2;

                                        // using the 4 basis functions:
return (   (     -t3 + 3 * t2 - 3 * t + 1 ) * xp
         + (  3 * t3 - 6 * t2         + 4 ) * x0
         + ( -3 * t3 + 3 * t2 + 3 * t + 1 ) * x1
         + (      t3                       ) * x2 ) / 6;
}

/*                                      // Keys cubic convolution interpolation, a = -1
                                        // support [-2..2]
double      Keys ( double t )
{
t       = abs ( t );

double              t2              = t * t;
double              t3              = t * t2;

return  ( t < 1 ?  t3 - 2 * t2         + 1
          t < 2 ? -t3 + 5 * t2 - 8 * t + 4
                :  0                       ) / 3.5;
}
*/

//----------------------------------------------------------------------------
double      MexicanHat ( double x, double center, double width, double height )
{
if ( width == 0 )
    return  x == center ? height : 0;

width   = abs ( width );

x      -= center;

x       = - Square ( x / width );

return  height * ( 2.0 / ( sqrt ( 3 * width ) * Power ( Pi, 0.25 ) ) ) * ( 1 + x ) * exp ( x / 2 );
}

                                        // a != 0
int         SolveQuadratic ( double a, double b, double c, double &x1, double &x2 )
{
double              det             = Square ( b ) - 4 * a * c;

if ( det < 0 )      return  0;

det     = sqrt ( det );

x1      = ( - b - det ) / ( 2 * a );
x2      = ( - b + det ) / ( 2 * a );

CheckOrder  ( x1, x2 );

return  det == 0 ? 1 : 2;
}

                                        // returned values x1 <= x2
int         GaussiansIntersections  (   double      c1,     double      w1,     double      h1, 
                                        double      c2,     double      w2,     double      h2, 
                                        double&     x1,     double&     x2 
                                    )
{
                                        // init with some reasonable values, as some cases below can fail
x1  = min ( c1, c2 );
x2  = max ( c1, c2 );


double              w1invpow        = w1 ? 1 / Square ( w1 ) : BigSingleFloat;
double              w2invpow        = w2 ? 1 / Square ( w2 ) : BigSingleFloat;
double              a               = w2invpow - w1invpow;
double              b               = 2 * ( c1 * w1invpow - c2 * w2invpow );
double              c               = Square ( c2 ) * w2invpow - Square ( c1 ) * w1invpow + log ( h1 ) - log ( h2 );


int                 numsol;

if ( a == 0 ) {

    if ( b == 0 )
        numsol      = 0;

    else {
        numsol      = 1;

        x1          = x2        = - c / b;
        }
    }
else

    numsol          = SolveQuadratic ( a, b, c, x1, x2 );


if ( numsol < 2 )
    return  numsol;

                                        // one of the two solutions can be dummy, like close to 0
double              y1              = Gaussian ( x1, c2, w2, h2 );
double              y2              = Gaussian ( x2, c2, w2, h2 );

if ( abs ( y2 / h2 ) < 1e-3 )  numsol--;                // ignore second intersection

if ( abs ( y1 / h2 ) < 1e-3 )
    if ( numsol == 2 )          { x1  = x2; numsol--; } // move second intersection to first
    else                        numsol  = 0;            // that is bad news!


return  numsol;
}
                                        // returns the normalized distance between 2 Gaussians
                                        // 0.5:overlapped  2:separated
                                        // || u1 - u2 || / ( max ( s1, s2 ) * sqrt ( dimensions ) )
double      GaussiansDistance       ( double c1, double w1, double c2, double w2 )
{
double              dc              = abs ( c1 - c2 );

                    w1              = GaussianWidthToSigma ( w1 );
                    w2              = GaussianWidthToSigma ( w2 );
                                        // there are different ways to account for the 2 widths
//double              sigma           = max ( w1, w2 );
double              sigma           = ( w1 + w2 ) / 2;
//double              sigma           = sqrt ( ( Square ( w1 ) + Square ( w2 ) ) / 2 );
//double              sigma           = sqrt ( w1 * w2 );

return  sigma ? dc / sigma
              : dc ? DBL_MAX : 0;
}

                                        // cacheradius is the half size of the array, or how far we want to go for the Gaussian
void        GetGaussianCache ( double gaussianwidth, TArray1<double> &cache, int cacheradius )
{
if ( cacheradius < 1 )
    return;

                                        // Windowing / Kernel function to smooth out the spindles density
cache.Resize ( 2 * cacheradius + 1 );

                                        // normalized windowing function
for ( int tfi = - cacheradius, tfi0 = 0; tfi <= cacheradius; tfi++, tfi0++ ) {
                                        // Gaussian with given width, height of 1
    cache[ tfi0 ]   = Gaussian ( tfi, 0, gaussianwidth, 1 );
                                        // Hanning, fatter
//  cache[ tfi0 ]   = ( cos ( Pi * (double) tfi / cacheradius ) + 1 ) / 2;
                                        // Blackman, slimmer
//  cache[ tfi0 ]   = 0.42 - 0.50 * cos ( Pi * ( (double) tfi / cacheradius + 1 ) ) + 0.08 * cos ( TwoPi * ( (double) tfi / cacheradius + 1 ) );
    }

                                        // optionnally
//cache.Kf.NormalizeArea ();
}


//----------------------------------------------------------------------------
                                        // clipping if out of bound
double  LinearRescale   (   double  x,  
                            double  xmin,   double  xmax,
                            double  toymin, double  toymax,
                            bool    clipborders
                        )
{
if ( clipborders ) {
    if ( x < xmin )     return  toymin;
    if ( x > xmax )     return  toymax;
    }

return  ( x - xmin ) / ( xmax   - xmin   )
                     * ( toymax - toymin ) + toymin;
}


double  LinearRescaleNoise  (   double  x,  
                                double  xmin,   double  xmax,
                                double  toymin, double  toymax
                             )
{
if ( x < xmin )     return  toymin;
if ( x > xmax )     return  toymax;


double              ratio       = ( toymax - toymin ) / ( xmax   - xmin   );
double              y           = ( x - xmin ) * ratio + toymin;

                                                  // always force a little bit of noise
TRandNormal         randnormal ( 0, GaussianWidthToSigma ( AtLeast ( 1.0, ratio ) ) /*GaussianWidthToSigma ( ratio )*/ );

                                        // if expanding the data, add some noise on top
//if ( ratio > 1 )

    y   +=  randnormal ();


return  y;
}


//----------------------------------------------------------------------------
                                        // Un-optimized, regular Legendre polynomial computation
long double     Legendre ( long double x, int n )
{
if      ( n == 0 )  return  1;
else if ( n == 1 )  return  x;


long double         Pnm1            = 1;    // previous value li-1
long double         Pn              = x;    // current value  li
long double         Pnp1;                   // next value     li+1

                                        // loop until n-1, as it computes the next value (n-1)+1 = n
for ( int li = 1; li < n; li++ ) {
                                        // next value
    Pnp1    = ( ( 2 * li + 1 ) * x * Pn - li * Pnm1 ) / ( li + 1 );
                                        // shift
    Pnm1    = Pn;
    Pn      = Pnp1;
    }


return  Pnp1;
}
                                        // given n-2 and n-1 values, return next value n
                                        // init starting from either 0 or 1 are OK, otherwise caller HAS to call iteratively
long double     NextLegendre ( long double& Pnm2, long double& Pnm1, long double& Pn, long double x, int n )
{
if      ( n == 0 )  { Pnm2  = 0; Pnm1  = 0; Pn  = 1; }
else if ( n == 1 )  { Pnm2  = 0; Pnm1  = 1; Pn  = x; }
else                { Pn    = ( ( 2 * n - 1 ) * x * Pnm1 - ( n - 1 ) * Pnm2 ) / n; }

                                        // shift to be ready for next call
Pnm2    = Pnm1;
Pnm1    = Pn;


return  Pn;
}


//----------------------------------------------------------------------------
                                        // Un-optimized, regular associated Legendre polynomial computation
                                        // P1l  for order m = 1
                                        // l >= 1
long double     AssociatedLegendreOrder1 ( long double x, int l )
{
if      ( l == 1 )  return  -         sqrt ( 1 - x * x );
else if ( l == 2 )  return  - 3 * x * sqrt ( 1 - x * x );


long double         lnm1            = -         sqrt ( 1 - x * x ); // previous value li-1
long double         ln0             = - 3 * x * sqrt ( 1 - x * x ); // current value  li
long double         lnp1;                                           // next value     li+1

                                        // loop until l-1, as it computes the next value (l-1)+1 = l
for ( int li = 2; li < l; li++ ) {
                                        // next value
    lnp1    = ( ( 2 * li + 1 ) * x * ln0 - ( li + 1 ) * lnm1 ) / li;
                                        // shift
    lnm1    = ln0;
    ln0     = lnp1;
    }


return  lnp1;
}
                                        // given n-2 and n-1 values, return next value n
                                        // init starting from either 1 or 2 are OK, otherwise caller HAS to call iteratively
long double     NextAssociatedLegendreOrder1 ( long double& Pnm2, long double& Pnm1, long double& Pn, long double x, int l )
{
if      ( l == 1 )  { Pnm2  = 0; Pnm1  = 0;                            Pn  = -         sqrt ( 1 - x * x ); }
else if ( l == 2 )  { Pnm2  = 0; Pnm1  = -         sqrt ( 1 - x * x ); Pn  = - 3 * x * sqrt ( 1 - x * x ); }
else                { Pn    = ( ( 2 * l - 1 ) * x * Pnm1 - l * Pnm2 ) / ( l - 1 ); }

                                        // rotate to be ready for next call
Pnm2    = Pnm1;
Pnm1    = Pn;


return  Pn;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Have some predefined arrays now...
static  /*const*/   TPredefinedArray    PredefinedSinus     ( PredefinedArraySinus,     TwoPi,      PredefinedArraySizeTrigo   );
static  /*const*/   TPredefinedArray    PredefinedCosinus   ( PredefinedArrayCosinus,   TwoPi,      PredefinedArraySizeTrigo   );
static  /*const*/   TPredefinedArray    PredefinedLanczos2  ( PredefinedArrayLanczos2,  2.0,        PredefinedArraySizeLanczos );
static  /*const*/   TPredefinedArray    PredefinedLanczos3  ( PredefinedArrayLanczos3,  3.0,        PredefinedArraySizeLanczos );
//static/*const*/   TPredefinedArray    PredefinedLog       ( PredefinedArrayLog,       3.0,        3 * 10000 );
//static/*const*/   TPredefinedArray    PredefinedPower_25  ( PredefinedArrayPower,     2.0,        2 * 10000,  0.25 );   // not working correctly
//static/*const*/   TPredefinedArray    PredefinedPower_10  ( PredefinedArrayPower,     2.0,        2 * 10000,  0.10 );


        TPredefinedArray::TPredefinedArray ( PredefinedArrayType type, double datarange, int storagesize, double param )
{
Size    = storagesize;

                                        // one more slot for circular end
Resize ( Size + 1 );

                                        // conversions index <-> real value
Index1.IndexMin     = 0;
Index1.IndexRatio   = Size / datarange;


double              t;


for ( int i = 0; i <= Size; i++ )

    switch ( type ) {

    case    PredefinedArraySinus:
        Array[ i ]  = sin ( ToReal ( i ) );
        break;

    case    PredefinedArrayCosinus:
        Array[ i ]  = cos ( ToReal ( i ) );
        break;

    case    PredefinedArrayLanczos2:
                            // [0..2]
        t           = Pi * ToReal ( i );
        Array[ i ]  = t ? 2 * sin ( t ) * sin ( t / 2 ) / Square ( t ) : 1;
        break;

    case    PredefinedArrayLanczos3:
                            // [0..3]
        t           = Pi * ToReal ( i );
        Array[ i ]  = t ? 3 * sin ( t ) * sin ( t / 3 ) / Square ( t ) : 1;
        break;

    case    PredefinedArrayLog:
        t           = ToReal ( i );
        Array[ i ]  = t == 0 ? 2 * log ( ToReal ( 1 ) ) : log ( t );
        break;

    case    PredefinedArrayPower:
        t           = ToReal ( i );
        Array[ i ]  = Power ( t, param );
        break;

    }

}


//----------------------------------------------------------------------------

double          FastSine ( double angle )
{
int                 i               = ( (int) PredefinedSinus.ToIndex ( angle ) ) % PredefinedSinus.Size;
                             // ! of negative values for %
return  PredefinedSinus[ i >= 0 ? i : i + PredefinedSinus.Size ];
}


double          FastCosine ( double angle )
{
int                 i               = ( (int) PredefinedCosinus.ToIndex ( angle ) ) % PredefinedCosinus.Size;
                               // ! of negative values for %
return  PredefinedCosinus[ i >= 0 ? i : i + PredefinedCosinus.Size ];
}

                                        // x: [-2..2]
double          FastLanczos2 ( double x )
{
                                        // Lanczos is symmetrical, and up to 2
int                 i               = PredefinedLanczos2.ToIndex ( abs ( x ) );

return  i <= PredefinedLanczos2.Size ? PredefinedLanczos2[ i ] : 0;
}

                                        // x: [-3..3]
double          FastLanczos3 ( double x )
{
                                        // Lanczos is symmetrical, and up to 3
int                 i               = PredefinedLanczos3.ToIndex ( abs ( x ) );

return  i <= PredefinedLanczos3.Size ? PredefinedLanczos3[ i ] : 0;
}


//----------------------------------------------------------------------------
                                        // returned in radian, always positive [0..2Pi]
double      ArcTangent ( double y, double x )
{
                                        // undefined case
if ( x == 0 && y == 0 )
    return  0;
                                        // result in [-Pi .. Pi]
double              arctan          = atan2 ( y, x );
                                        // result always in [0 .. 2Pi]
return  arctan < 0 ? arctan + TwoPi : arctan;
}


double      ArcTangent ( const std::complex<float>& c )
{
return  ArcTangent ( c.imag (), c.real () );
}


double      Cotangent ( double angle )
{
return  1.0 / tan ( angle );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A Gaussian variable
        TGaussian::TGaussian ()
{
Center              = 0;
Exponent            = 0;
Constant            = 1;
}


        TGaussian::TGaussian ( double center, double radius, double numsd )
{
Set ( center, radius, numsd );
}

                                        // FWHM = 2.35482 x Spread  (2*sqrt(2*ln(2)))
void    TGaussian::Set ( double center, double radius, double numsd )
{
radius              = abs ( radius );

double              Spread          = radius / ( numsd ? numsd : 1 );

if ( Spread == 0 )  Spread  = 1;

Center              = center;
Exponent            = -1.0 /      ( 2     * Square ( Spread ) );
Constant            =  1.0 / sqrt ( TwoPi * Square ( Spread ) );
}


double  TGaussian::operator() ( double v )
{
return  Constant * exp ( Exponent * Square ( v - Center ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







