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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Math.Histo.h"
#include    "TVector.h"

#include    "GlobalOptimize.Tracks.h"
#include    "GlobalOptimize.Points.h"       // GlobalOptimizeMaxPoints, geometrical transform enums

#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMapProperties::TMapProperties ()
{
Reset ();
}


        TMapProperties::TMapProperties ( TMap& map, const TElectrodesDoc* xyzdoc )
{
Reset ();


Map             = map;              // copy map
Map.Normalize ();                   // reference?

SynthMap.Resize ( Map.GetDim () );  // allocation is enough

Points          = xyzdoc->GetPoints ( DisplaySpace3D );
StandardOrient  = xyzdoc->GetStandardOrientation ( LocalToRAS );
}


void    TMapProperties::Reset ()
{
TGlobalOptimize::Reset ();


Map     .DeallocateMemory ();
SynthMap.DeallocateMemory ();

Points.Reset ();
StandardOrient.SetIdentity ();
}


//----------------------------------------------------------------------------
                                        //
double  TMapProperties::Evaluate ( TEasyStats *stat )
{

double              dr;
//double              sumsqr          = 0;
//int                 numsumsqr       = 0;


if ( stat )
    stat->Reset ();


ComputeSynthMap ( SynthMap ) ;


dr          = Map.Difference ( SynthMap );


//sumsqr      += Square ( dr );
//numsumsqr++;

                                        // or stats on electrodes?
if ( stat ) 
    stat->Add ( dr );


//return  sumsqr / numsumsqr;
return  dr;
}


//----------------------------------------------------------------------------
                                        // Returns the synth map with the current parameters
void    TMapProperties::ComputeSynthMap ( TMap& map )
{
map.MapFromDipole ( HasValue( TranslationX       ) ? GetValue ( TranslationX       ) : 0,    // default to vector in center
                    HasValue( TranslationY       ) ? GetValue ( TranslationY       ) : 0,
                    HasValue( TranslationZ       ) ? GetValue ( TranslationZ       ) : 0,
                    HasValue( RotationPrecession ) ? GetValue ( RotationPrecession ) : 0,    // default to vertex direction
                    HasValue( RotationNutation   ) ? GetValue ( RotationNutation   ) : 0, 
                    true, true,
                    Points,
                    StandardOrient );
}


//----------------------------------------------------------------------------
void    TMapProperties::GetDipole ( TVector3Float& dipole )
{
double              precession      = HasValue( RotationPrecession ) ? GetValue ( RotationPrecession ) : 0;
double              nutation        = HasValue( RotationNutation   ) ? GetValue ( RotationNutation   ) : 0;

                                        // needs +90 degrees to be in front: X points to the right, rotate to have it point forward
TMatrix44           euler ( precession + 90, nutation, 0 );

TMatrix44           transform ( euler * StandardOrient );

transform.Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

dipole.Set ( 0, 0, 1 );

                                        // transform is only for the direction of dipole
transform.Apply ( dipole );
}


//----------------------------------------------------------------------------
void    TVector<float>::FitDipole ( TElectrodesDoc* xyzdoc, TMap* map, TVector3Float* dipole )
{

double              radius          = xyzdoc->GetBounding ( DisplaySpace3D )->AbsRadius ();


TMapProperties      gosag ( (TMap) *this, xyzdoc );
            

gosag.AddGroup      ( 2 * 3 + 1, 1, 2.0 / 3.0 );
gosag.AddDim        ( RotationPrecession, -180, 180 );
gosag.AddDim        ( RotationNutation,   -180, 180 );


gosag.AddGroup      ( 2 * 3 + 1, 1, 2.0 / 3.0 );
gosag.AddDim        ( TranslationX, -0.66 * radius, 0.66 * radius );
gosag.AddDim        ( TranslationY, -0.66 * radius, 0.66 * radius );
gosag.AddDim        ( TranslationZ, -0.66 * radius, 0.66 * radius );


gosag.GetSolution   (   CyclicalCrossHairScan,  0, 
                        1e-3,                   0,
                        0
                    );
//gosag.GetSolution (   WeakestGroupBoxScan,    0, 
//                      1e-3,                   0, 
//                      0
//                  );

                                        // results as caller request
if ( map )
    gosag.ComputeSynthMap   ( *map );


if ( dipole )
    gosag.GetDipole         ( *dipole );


//if ( VkQuery () ) {
//    DBGV5 ( nclusters, nc + 1, nummaps, gosag[ 0 ][ 0 ].Value, gosag[ 0 ][ 1 ].Value, "#clusters: #map / #total -> Precession Nutation" );
//    DBGV3 ( gosag[ 1 ][ 0 ].Value, gosag[ 1 ][ 1 ].Value, gosag[ 1 ][ 2 ].Value, "translations" );
//    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTrackAnalysis::TTrackAnalysis ()
{
Reset ();
}


        TTrackAnalysis::TTrackAnalysis ( const TArray1<double>& data )
{
Set ( data );
}


void    TTrackAnalysis::Reset ()
{
TGlobalOptimize::Reset ();

Data.DeallocateMemory ();
}

                                        // For the moment, used for Mixture
void    TTrackAnalysis::Set ( const TArray1<double>& data )
{
Reset ();

Data    = data;
}


//----------------------------------------------------------------------------
                                        // routing to the right function
double  TTrackAnalysis::Evaluate ( TEasyStats *stat )
{
if      ( How & FunctionsMixture )      return  EvaluateFunctionsMixture ( stat );
else                                    return  0;
}


//----------------------------------------------------------------------------
double  TTrackAnalysis::EvaluateFunctions ( double x )
{
double              sumg            = 0;

for ( int g = 1; g <= GetNumFunctions (); g++ )

    sumg       += EvaluateFunction ( x, g );

return  sumg;
}


double  TTrackAnalysis::EvaluateFunctions ( double x, int gi1, int gi2 )
{
double              sumg            = 0;

for ( int g = gi1; g <= gi2; g++ )

    sumg       += EvaluateFunction ( x, g );

return  sumg;
}


//----------------------------------------------------------------------------
double  TTrackAnalysis::EvaluateFunction ( double x, int g )
{
                                        // groups/functions are numbered starting from 1
g--;

                                        // convert gaussian number to a variable index
int                 variableindex   = g * NumFunctionParams;
auto&               fparams         = Groups[ g ].FixedParams;
int                 functiontype    = fparams[ FunctionType ];
int                 kerneltype      = fparams[ SplineKernel ];
double              kernelw;
double              v;

                                        // make sure parameters are correct
#if defined(CHECKASSERT)
assert ( functiontype >= 0 && functiontype < NumFunctionTypes  );
assert ( kerneltype   >= 0 && kerneltype   < NumSupportKernels );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Kernel boils down to a weighting function across the data range

if ( kerneltype != SupportKernelNone ) {
                                        // - normalized coordinates to -1..1 (well, as all _Kernels_ are symmetrical, we take abs right now)
                                        // - don't clip the U, some Kernels (like Gaussian) might not be restricted to a close support
                                        // - the caller sets the Centers of the Kernel, the range will be computed from either the previous / the next center
    double              u               = x - fparams[ SplineNode ];

                                        // check for boundaries AND if neighbor is part of the spline game (is a node), which allows for a spline mixed with other functions
    if      ( u < 0 ) {
        if ( g > 0                      && Groups[ g - 1 ].FixedParams[ SplineKernel ] != SupportKernelNone )
            u       = fabs ( u / ( fparams[ SplineNode ] - Groups[ g - 1 ].FixedParams[ SplineNode ] ) );   // there is a connected node on the left
        else
            u       = fabs ( u / ( fparams[ SplineNode ] - Groups[ g + 1 ].FixedParams[ SplineNode ] ) );   // there is either nothing on the left, or not a node
        }
    else if ( u > 0 ) {
        if ( g < GetNumFunctions () - 1 && Groups[ g + 1 ].FixedParams[ SplineKernel ] != SupportKernelNone )
            u       = fabs ( u / ( fparams[ SplineNode ] - Groups[ g + 1 ].FixedParams[ SplineNode ] ) );   // there is a connected node on the right
        else
            u       = fabs ( u / ( fparams[ SplineNode ] - Groups[ g - 1 ].FixedParams[ SplineNode ] ) );   // there is either nothing on the right, or not a node
        }


    switch ( kerneltype ) {

        case    SupportKernelUniform:       kernelw     = u <= 1 ? 1                                    : 0; break;
        case    SupportKernelTriangular:    kernelw     = u <= 1 ? 1 - u                                : 0; break;
        case    SupportKernelQuartic:       kernelw     = u <= 1 ? Square ( 1 - Square ( u ) )          : 0; break;
        case    SupportKernelTriweight:     kernelw     = u <= 1 ? Cube   ( 1 - Square ( u ) )          : 0; break;
        case    SupportKernelTricube:       kernelw     = u <= 1 ? Cube   ( 1 - Cube   ( u ) )          : 0; break;
        case    SupportKernelRaisedCosine:  kernelw     = u <= 1 ? ( cos ( u * Pi ) + 1 ) / 2           : 0; break;
        }

    } // if kerneltype
else
    kernelw = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 degree;
double              center;
double              width;
double              height;
double              exprate;
//double            expbase;


switch ( functiontype ) {

    case    FunctionTypeIdentity:
                                        // return closest value
                                        // better: return an interpolated value?
    v       = Data[ Clip ( Round ( x ), 0, (int) Data - 1 ) ];

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeGaussian:
                                        // plain Gaussian
    v       = Gaussian            ( x,
                                   GetValue ( variableindex + FunctionCenter ),
                                   GetValue ( variableindex + FunctionWidth  ),     // function will take absolute value, if negative
                                   GetValue ( variableindex + FunctionHeight ) );
    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeGaussianAsym:
                                        // Generalized Gaussian, which could also be a plain Gaussian, too
    v       = AsymmetricalGaussian ( x,
                                   GetValue ( variableindex + FunctionCenter     ),
                                   GetValue ( variableindex + FunctionWidthLeft  ),
                                   GetValue ( variableindex + FunctionWidthRight ),
                                   GetValue ( variableindex + FunctionHeight     ) );
    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeGeneralizedGaussian:
                                        // Generalized Gaussian, which could also be a plain Gaussian, too
    v       = GeneralizedGaussian ( x,
                                   GetValue ( variableindex + FunctionCenter ),
                                   GetValue ( variableindex + FunctionWidth  ),     // function will take absolute value, if negative
                                   GetValue ( variableindex + FunctionHeight ),
                                    HasValue( variableindex + FunctionSkew   ) ? GetValue ( variableindex + FunctionSkew ) : 0 );
    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeStudent:

    v       = Student             ( x,
                                   GetValue ( variableindex + FunctionCenter ),
                                   GetValue ( variableindex + FunctionWidth  ),     // function will take absolute value, if negative
                                   GetValue ( variableindex + FunctionHeight ),
                                   Round ( fparams[ StudentDegree ] )           );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeRaisedCosine:

    v       = RaisedCosine        ( x,
                                   GetValue ( variableindex + FunctionCenter ),
                                   GetValue ( variableindex + FunctionWidth  ),     // function will take absolute value, if negative
                                   GetValue ( variableindex + FunctionHeight ),
                                   fparams[ CosinePower ]                       );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeExponential:

    v       = Exponential         ( x,
                                    HasValue( variableindex + FunctionCenter  ) ? GetValue ( variableindex + FunctionCenter ) : 0,
                                    HasValue( variableindex + FunctionWidth   ) ? GetValue ( variableindex + FunctionWidth  ) : 1,    // function will take absolute value, if negative
                                    HasValue( variableindex + FunctionHeight  ) ? GetValue ( variableindex + FunctionHeight ) : 1,
                                   GetValue ( variableindex + ExponentialRate ) );

    break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeSpectrumExponential:

//  center  = HasValue( variableindex + FunctionCenter  ) ? GetValue ( variableindex + FunctionCenter  ) : 0;
    width   = HasValue( variableindex + FunctionWidth   ) ? GetValue ( variableindex + FunctionWidth   ) : 1;
    height  = HasValue( variableindex + FunctionHeight  ) ? GetValue ( variableindex + FunctionHeight  ) : 1;
    exprate =                                               GetValue ( variableindex + ExponentialRate );
//  expbase = GetValue ( variableindex + ExponentialBase );

    if ( exprate <= 0 ) {
        v   = 0;
        break;
        }

//  x      -=  center;
//  if ( x <= 0 )
//      return  height;

    if ( width <= 0 ) {
        v   = x <= 0 ? height : 0;
        break;
        }

    v       = height * Power ( 10 /*expbase*/, - Power ( x / width, exprate ) );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeSpectrumInverse:

    center  = HasValue( variableindex + FunctionCenter  ) ? GetValue ( variableindex + FunctionCenter  ) : 0;
//  width   = HasValue( variableindex + FunctionWidth   ) ? GetValue ( variableindex + FunctionWidth   ) : 1;
    height  = HasValue( variableindex + FunctionHeight  ) ? GetValue ( variableindex + FunctionHeight  ) : 1;
    exprate =                                               GetValue ( variableindex + ExponentialRate );

    if ( exprate <= 0 ) {
        v   = 0;
        break;
        }

//  if ( width <= 0 ) {
//      v   = x == 0 ? height : 0;
//      break;
//      }

    x      += center;

    if ( x <= 0 )
        x   = 0.5;

//  v       = height * Power ( x / width, - exprate );
    v       = height * Power ( x, - exprate );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeGamma:

    v       = Gamma               ( x,
                                    HasValue( variableindex + FunctionCenter  ) ? GetValue ( variableindex + FunctionCenter ) : 0,
                                    HasValue( variableindex + FunctionWidth   ) ? GetValue ( variableindex + FunctionWidth  ) : 1,    // function will take absolute value, if negative
                                    HasValue( variableindex + FunctionHeight  ) ? GetValue ( variableindex + FunctionHeight ) : 1,
                                   GetValue ( variableindex + GammaShape ),
                                   GetValue ( variableindex + GammaScale ) );

    //      + ( HasValue( variableindex + FunctionOffset ) ? GetValue ( variableindex + FunctionOffset ) : 0 );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypeMonomial:
                                        // note: power is an integer
    degree          = Round ( fparams[ MonomialPower ] );


    if ( HasValue( variableindex + FunctionCenter ) )
        x      -= GetValue ( variableindex + FunctionCenter );


    v       = GetValue ( variableindex + MonomialFactor ) * ( degree != 0 ? Power ( x, degree ) : 1 );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case    FunctionTypePolynomial:
                                        // optional center shift
    if ( HasValue( variableindex + FunctionCenter ) )
        x      -= GetValue ( variableindex + FunctionCenter );


    v       = 0;
                                        // all polynomial factors are optional, which can make some monomial instead
    for ( degree = 0; degree <= MaxPolynomialDegree; degree++ )
        if ( HasValue( variableindex + PolynomialFactor0 + degree ) )
            v      += GetValue ( variableindex + PolynomialFactor0 + degree ) * ( degree ? Power ( x, degree ) : 1 );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    default:
                                        // unknown function
    v       = 0;

    break;

    } // switch functiontype

                                        // apply Kernel weight to value
return  kernelw * v;
}

/*
return  Gaussian ( x,
                   GetValue ( variableindex + FunctionCenter ),
                   max ( GetValue ( variableindex + FunctionWidth  ), 1e-60 ),
                   max ( GetValue ( variableindex + FunctionHeight ), 0.0   ) );
                                        // parameters clipping is done in the Nelder-Mead function
return  Gaussian ( x,
                   Clip ( GetValue ( variableindex + FunctionCenter ), *ToMin[ variableindex + FunctionCenter ], *ToMax[ variableindex + FunctionCenter ] ),
                   Clip ( GetValue ( variableindex + FunctionWidth  ), *ToMin[ variableindex + FunctionWidth  ], *ToMax[ variableindex + FunctionWidth  ] ),
                   Clip ( GetValue ( variableindex + FunctionHeight ), *ToMin[ variableindex + FunctionHeight ], *ToMax[ variableindex + FunctionHeight ] ) );
*/

//----------------------------------------------------------------------------
double  TTrackAnalysis::EvaluateFunctionsMixture ( TEasyStats *stat )
{
//int                 numdata             = (int) Data;
int                 lastdata                = (int) Data - 1;
int                 minindex                = How & FitMinIndex     ? Groups[ 0 ].FixedParams[ FitMinIndexFactor        ] : 0;
int                 maxindex                = How & FitMaxIndex     ? Groups[ 0 ].FixedParams[ FitMaxIndexFactor        ] : lastdata;
int                 numdata                 = maxindex - minindex + 1;
double              delta;
double              sumg                    = 0;
double              v;
double              vmax;
double              vmax2;
double              leastoverlapsfactor     = How & LeastOverlaps   ? Groups[ 0 ].FixedParams[ LeastOverlapsFactor      ] : 0;
double              leastslopesfactor       = How & LeastSlopes     ? Groups[ 0 ].FixedParams[ LeastSlopesFactor        ] : 0;
double              morecompactfactor       = How & MoreCompact     ? Groups[ 0 ].FixedParams[ MoreCompactFactor        ] : 0;
double              keeporderingfactor      = How & KeepOrdering    ? Groups[ 0 ].FixedParams[ KeepOrderingFactor       ] : 0;
double              positiveheightsfactor   = How & PositiveHeights ? Groups[ 0 ].FixedParams[ PositiveHeightsFactor    ] : 0;
double              datam;
double              datap;
double              evalm;
double              evalp;

TEasyStats          statdr ( numdata );

if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between the mixture and the data
for ( int i = minindex; i <= maxindex; i++ ) {

    if ( i != minindex )                // save previous evaluation
        evalm       = sumg;


    sumg            = 0;
    vmax            = 0;
    vmax2           = 0;

                                        // we scan each group ourselves, to manually track the max's
    for ( int g = 1; g <= GetNumFunctions (); g++ ) {

        v           = EvaluateFunction ( i, g );

        sumg       += v;


        if ( How & LeastOverlaps ) {
                                        // keep max and second-to-max contributions
            v           = fabs ( v );
            if      ( v > vmax  )  { vmax2 = vmax; vmax = v; }
            else if ( v > vmax2 )    vmax2 = v;
            }
        }


    if ( i == minindex )                // no previous evaluation
        evalm       = sumg;

                                        // distance from data to mixture
    delta           = fabs ( Data[ i ] - sumg );


                                        // some optional penalties:
    if ( How & LeastOverlaps ) {
                                        // max is the main contributor, we want second contributor (max2) to be as low as possible to avoid too much overlap
                                        // common penalty is about 0.10
        delta          += vmax2 * leastoverlapsfactor;

                                        // add a penalty to all other contributors but the max -> force a good separation, though detrimental to the final fit quality!
//      delta          += fabs ( sumg - vmax ) * leastoverlapsfactor;
//      delta          *= ( 1 + vmax2 / ( vmax ? vmax : 1 ) );
        } // LeastOverlaps


    if ( How & LeastSlopes ) {
                                        // slope matching

                                        // get neighbors (not totally optimal)
        datam   = Data[ max ( i - 1, 0        ) ];
        datap   = Data[ min ( i + 1, lastdata ) ];
//      evalm                           // saved from the previous computation
        evalp   = EvaluateFunctions ( min ( i + 1, maxindex ) );

                                        // add penalty from slope difference - also, compensate for the edges missing data
        delta          += fabs ( ( datap - datam ) - ( evalp - evalm ) ) * ( i == minindex || i == maxindex ? 2 : 1 ) * leastslopesfactor;
        } //


/*    if ( How & LeastCurvature ) {
                                        // neighbor evaluation
        double              eval0           = sumg;
        double              evalm           = i > minindex    ? EvaluateFunctions ( i - 1 ) : eval0;
        double              evalp           = i < maxindex    ? EvaluateFunctions ( i + 1 ) : eval0;

                                        // add penalty from high curvature
        delta          += fabs ( 2 * eval0 - evalm - evalp ) * 0.10;
        } */

                                        // could be moved out of the loop..
    if ( How & MoreCompact ) {
        for ( int g = 0; g < GetNumFunctions (); g++ )
                                        // convert back to a "distance", as this is what we cumulate here (not surfaces)
            delta      += sqrt ( GetValue ( g * NumFunctionParams + FunctionHeight )
                               * GetValue ( g * NumFunctionParams + FunctionWidth  ) / numdata ) * morecompactfactor;

                                         // does not work, but why?!
//          delta      += sqrt ( fabs ( GetValue ( g * NumFunctionParams + FunctionHeight )
//                                    * GetValue ( g * NumFunctionParams + FunctionWidth  ) ) / numdata ) * morecompactfactor;
        }

                                        // we might want to keep the function's center ordered
    if ( How & KeepOrdering ) {
        for ( int g = 1; g < GetNumFunctions (); g++ )
            if ( GetValue ( g * NumFunctionParams + FunctionCenter ) < GetValue ( ( g - 1 ) * NumFunctionParams + FunctionCenter ) )
                                        // make the penalty proportional to the past relative distance
//              delta      *= 1 + ( GetValue ( ( g - 1 ) * NumFunctionParams + FunctionCenter )
//                                - GetValue (   g       * NumFunctionParams + FunctionCenter ) ) / numdata * keeporderingfactor;
                delta      += ( GetValue ( ( g - 1 ) * NumFunctionParams + FunctionCenter )
                              - GetValue (   g       * NumFunctionParams + FunctionCenter ) ) / numdata * keeporderingfactor;
        }

                                        // we might want to keep the height positives
    if ( How & PositiveHeights ) {
        for ( int g = 0; g < GetNumFunctions (); g++ )
            if ( GetValue ( g * NumFunctionParams + FunctionHeight ) < 0 )
                delta      -= GetValue ( g * NumFunctionParams + FunctionHeight ) / numdata * positiveheightsfactor;
        }


                                        // give more weight to high value data
//  delta  *= Data[ i ]; 

                                        // store to buffer
    statdr.Add ( delta );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from 4 to 1 SD, when below convergence threshold OutliersPrecision
                                        // # of SD, % of data: 1 68%, 2 95%, 3 99%
double              limitd          = CurrentPrecision < OutliersPrecision ? statdr.Average () + ( CurrentPrecision / OutliersPrecision * 3 + 1 ) * statdr.SD () : DBL_MAX;
//double            limitd          = CurrentPrecision < OutliersPrecision ? statdr.Average () + ( CurrentPrecision / OutliersPrecision ) * ( statdr.Max () - statdr.Average () ) : DBL_MAX;
//double            limitd          = CurrentPrecision < OutliersPrecision ? statdr.Max () / 2 + ( CurrentPrecision / OutliersPrecision ) * ( statdr.Max () / 2 ) : DBL_MAX;

double              sumsqr          = 0;
int                 numsumsqr       = 0;

                                        // some points might have been rejected
OmpParallelForSum ( sumsqr, numsumsqr )

for ( int i = 0; i < (int) statdr; i++ ) {

    double          d           = statdr[ i ];

    if ( d > limitd )
        continue;


    if ( How & LeastSquares )
        d      *= d;

    sumsqr     += d;

    numsumsqr++;

    if ( stat && d != 0 )
        stat->Add ( d );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;
}


//----------------------------------------------------------------------------
void    TTrackAnalysis::GetFunctionsMixture ( TVector<double> &mixt )
{
if ( mixt.IsNotAllocated () )
    mixt.Resize ( (int) Data );

                                        // we can copy any existing scaling from original track...
mixt.Index1     = Data.Index1;


for ( int i = 0; i < (int) Data; i++ )

    mixt[ i ]   = EvaluateFunctions ( i );
}


void    TTrackAnalysis::GetFunctionsMixture ( TVector<float> &mixt )
{
if ( mixt.IsNotAllocated () )
    mixt.Resize ( (int) Data );

                                        // we can copy any existing scaling from original track...
mixt.Index1     = Data.Index1;


for ( int i = 0; i < (int) Data; i++ )

    mixt[ i ]   = EvaluateFunctions ( i );
}


void    TTrackAnalysis::GetFunctionsMixture ( TTracks<double> &mixt )
{
if ( mixt.IsNotAllocated () )
    mixt.Resize ( GetNumFunctions () + FunctionsMixtureAdditionalTracks, (int) Data );

                                        // we can copy any existing scaling from original track...
mixt.Index2     = Data.Index1;


double              sumg;
double              v;


for ( int i = 0; i < (int) Data; i++ ) {

    sumg        = 0;

    for ( int g = 1; g <= GetNumFunctions (); g++ ) {

        v                   = EvaluateFunction ( i, g );

        sumg               += v;

        mixt ( g - 1, i )   = v;
        }


    mixt ( GetNumFunctions ()    , i )  = sumg;
    mixt ( GetNumFunctions () + 1, i )  = Data[ i ];
    }
}


//----------------------------------------------------------------------------
/*
void    TTrackAnalysis::ShowProgress ()
{

if ( Iteration > 100 )
    return;


TTracks<double>     gaussmix ( GetNumFunctions () + 2, (int) Data );
TExportTracks       expfile;


GetFunctionsMixture ( gaussmix );


sprintf ( expfile.Filename, "E:\\Data\\TracksAnalysis.GaussiansMixture%0d.%03d.sef", GetNumFunctions (), Iteration );
expfile.SetAtomType ( AtomTypeScalar );
expfile.NumTracks           = gaussmix.GetDim1 ();
expfile.NumTime             = gaussmix.GetDim2 ();
expfile.Write ( gaussmix, true );
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







