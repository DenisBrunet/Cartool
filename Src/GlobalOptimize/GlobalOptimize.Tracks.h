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


#include    "TArray1.h"
#include    "TArray2.h"
#include    "Math.TMatrix44.h"

#include    "TMaps.h"

#include    "GlobalOptimize.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class  TMapProperties   : public TGlobalOptimize
{
public:
                    TMapProperties ();
                    TMapProperties ( TMap& map, const TElectrodesDoc* xyzdoc );


    void            Reset ();

    double          Evaluate    ( TEasyStats *stat = 0 );

    void            ComputeSynthMap ( TMap& map );
    void            GetDipole       ( TVector3Float& dipole );


protected:

    TMap            Map;
    TMap            SynthMap;

    TPoints         Points;
    TMatrix44       StandardOrient;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Notes:
                                        //   1 group <-> 1 function/patch to evaluate
                                        //   FunctionType parameter must be set in FixedParams
//----------------------------------------------------------------------------
                                        // Options to mixture, used in GetSolution
enum {
                    FunctionsMixture    = 0x0001,   // Decomposing a signal with Kernel Density functions of many sorts

                    LeastDistances      = 0x0010,   // one of these 2 is mandatory
                    LeastSquares        = 0x0020,

                    LeastOverlaps       = 0x0100,   // optional and non-exclusive
                    LeastSlopes         = 0x0200,   // optional and non-exclusive
                    MoreCompact         = 0x0400,   // optional and non-exclusive
                    KeepOrdering        = 0x0800,   // optional and non-exclusive
                    PositiveHeights     = 0x1000,   // optional and non-exclusive
                    FitMinIndex         = 0x2000,   // optional and non-exclusive
                    FitMaxIndex         = 0x4000,   // optional and non-exclusive
                    };


//----------------------------------------------------------------------------

constexpr int       FunctionsMixtureAdditionalTracks    = 2;
constexpr int       MaxPolynomialDegree                 = 30;

                                        // Explored parameters, for Mixture of Functions
enum {
                    FunctionCenter,
                    FunctionHeight,
                    FunctionWidth,
                    FunctionWidthLeft,
                    FunctionWidthRight,
                    FunctionSkew,
                    FunctionOffset,

                    GammaShape,
                    GammaScale,

                    ExponentialRate,
//                  ExponentialBase,

                    MonomialFactor,

                    PolynomialFactor0,
                    LastPolynomialFactor            = PolynomialFactor0 + MaxPolynomialDegree,

                    NumFunctionParams
                    };


//----------------------------------------------------------------------------
                                        // Fixed parameters keys, for Mixture of Functions
enum {
                    FunctionType,                       // mandatory
                    FitMinIndexFactor,                  // optional: restrain range of date to fit
                    FitMaxIndexFactor,                  // optional: restrain range of date to fit
                    LeastOverlapsFactor,                // optional
                    LeastSlopesFactor,                  // optional
                    MoreCompactFactor,                  // optional
                    KeepOrderingFactor,                 // optional
                    PositiveHeightsFactor,              // optional
                    SplineKernel,                       // optional
                    SplineNode,                         // optional

                    StudentDegree,                      // mandatory for Student

                    CosinePower,                        // mandatory for Raised Cosine

                    MonomialPower,                      // mandatory for Monomial

                    NumFixedParamsTTrackAnalysis
                    };

                                        // Fixed parameters, FunctionType values
enum {
                    FunctionTypeIdentity    = 0,
                    FunctionTypeGaussian,               // regular Gaussian
                    FunctionTypeGaussianAsym,           // asymmetrical Gaussian
                    FunctionTypeGeneralizedGaussian,    // Gaussian + Skew
                    FunctionTypeStudent,                // like a Gaussian, but with thicker trails
                    FunctionTypeRaisedCosine,           // like a Gaussian, but with finite support
                    FunctionTypeExponential,
                    FunctionTypeGamma,
                    FunctionTypeMonomial,
                    FunctionTypePolynomial,
                    FunctionTypeSpectrumExponential,
                    FunctionTypeSpectrumInverse,
                    NumFunctionTypes,
                    };

                                        // Fixed parameters, SplineKernel values
enum {
                    SupportKernelNone       = 0,        // no Kernel & no support, function is done on whole range
                    SupportKernelUniform,               // constant (antisymmetric)
                    SupportKernelTriangular,            // triangular (or linear) (antisymmetric)
                    SupportKernelQuartic,               // ^2^2
                    SupportKernelTriweight,             // ^2^3
                    SupportKernelTricube,               // ^3^3
                    SupportKernelRaisedCosine,          // Hanning style (antisymmetric)
                    NumSupportKernels
                    };


//----------------------------------------------------------------------------

class  TTrackAnalysis   : public TGlobalOptimize
{
public:
                    TTrackAnalysis ();
                    TTrackAnalysis ( const TArray1<double>& data );


    void            Reset ();
    void            Set ( int numsteps, int numsubsteps, double zoomfactor )    { TGlobalOptimize::Set ( numsteps, numsubsteps, zoomfactor ); } // compiler will freak out without this
    void            Set ( const TArray1<double>&    data );

    int             GetNumFunctions ()          { return NumGroups; }

    double          Evaluate                    ( TEasyStats *stat = 0 );
    double          EvaluateFunctionsMixture    ( TEasyStats *stat = 0 );
    double          EvaluateFunctions           ( double x );                   // sum of all functions
    double          EvaluateFunctions           ( double x, int gi1, int gi2 ); // sum of some functions
    double          EvaluateFunction            ( double x, int functioni );    // functions indexes start from 1


    void            GetFunctionsMixture ( TVector<float>  &mixt );
    void            GetFunctionsMixture ( TVector<double> &mixt );
    void            GetFunctionsMixture ( TTracks<double> &mixt );

//  void            ShowProgress ();


protected:

    TVector<double> Data;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







