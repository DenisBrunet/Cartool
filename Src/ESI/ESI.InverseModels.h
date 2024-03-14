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

#include    "Math.Armadillo.h"
#include    "Geometry.TPoint.h"
#include    "TVolume.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define         PostfixInverseTitle     " Inverse"


class           TPoints;
class           TStrings;
class           TSelection;
class           TVerboseFile;
class           TFitModelOnPoints;


enum            InverseCenterType
                {
                InverseCenterMri,       // No translation, using the MRI - do not use for a real Lead Field, as the xyz normalization will be wrong

                InverseCenterHeadModel, // Head Model center
                InverseCenterBfeHead,   // Best Fitting Ellipsoid through Head surface

                InverseCenterBfsXyz,    // Best Fitting Sphere    through Electrodes
                InverseCenterBfeXyz,    // Best Fitting Ellipsoid through Electrodes

                InverseCenterBfsSp,     // Best Fitting Sphere    through Solution Points
                InverseCenterBfeSp,     // Best Fitting Ellipsoid through Solution Points
                };

void            ComputeInverseCenter    (   
                                        InverseCenterType           how,
                                        const TFitModelOnPoints*    surfacemodel,
                                        const TPoints*              headpoints,
                                        const TPoints*              solpoints,
                                        const TPoints*              xyzpoints,
                                        TPointFloat&                centertranslate,
                                        bool                        showprogress    = false
                                        );

TPointFloat     ComputeOptimalInverseTranslation (
                                        const TPoints*              headpoints,
                                        const TPoints*              solpoints,
                                        const TPoints*              xyzpoints,
                                        bool                        showprogress
                                        );


//----------------------------------------------------------------------------
                                        // Running simulations gives this:
                                        // - Adaptive reduction from 26 to 18 is the most stable w and w/o Z-Score
                                        // - With Z-Score and Cartool Weighting, 6 neighbors is best
constexpr NeighborhoodType  InverseNeighborhood     = Neighbors26;

                                        // Laura power -> Errors Dpos 0-1, 1-2, 2-3
                                        //          1                 26   69   5
                                        //          2                 29   66   5
                                        //          3                 32   63   4
                                        // Recommendations based on original article: 3 for vectorial fields, 2 for scalar fields
//constexpr double    LauraPower              = 2.0;
                                        // After simulations with real noise, this is actually the best value
constexpr double    LauraPower              = 0.5;

constexpr int       ELoretaMaxIterations    = 15;
constexpr double    ELoretaConvergence      = 0.005;


//----------------------------------------------------------------------------
                                        // Flags for neighborhood reduction
enum                NeighborhoodReduction
                    {
                    Neighbors26To18None         = 0x00,
                    Neighbors26To18             = 0x01,
                    Neighbors26To18Strict       = 0x10,
                    Neighbors26To18Lax          = 0x20,

                                        // Adaptative reduction from 26 to 18 is the most stable w and w/o Z-Score
                    InverseNeighborhood26to18   = Neighbors26To18 | Neighbors26To18Lax,
                    };

inline  bool    IsNeighbors26To18       ( NeighborhoodReduction nr )    { return IsFlag ( nr, Neighbors26To18       ); }
inline  bool    IsNeighbors26To18Lax    ( NeighborhoodReduction nr )    { return IsFlag ( nr, Neighbors26To18Lax    ); }
inline  bool    IsNeighbors26To18Strict ( NeighborhoodReduction nr )    { return IsFlag ( nr, Neighbors26To18Strict ); }


//----------------------------------------------------------------------------

void    ComputeRegularizationFactors (
                                const ASymmetricMatrix&     KKt,            double          eigenvaluedownfactor, 
                                TVector<double>&            regulvalues,    TStrings&       regulnames 
                                );

void    ComputeResolutionMatrix (   
                                const AMatrix&      K,              const TPoints&      solpoints,
                                const char*         fileinverse,
                                RegularizationType  regularization,
                                char*               fileresmat,     char*               fileresmats,    char*               fileresmatt
                                );


//----------------------------------------------------------------------------

void    ComputeInverseMN        ( 
                                const AMatrix&      Kin,            TSelection              spsrejected, 
                                bool                regularization, const TStrings&         xyznames, 
                                const TStrings&     spnamesin,
                                const char*         filename 
                                );

void    ComputeInverseWMN       (
                                const AMatrix&      Kin,            TSelection              spsrejected, 
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin,
                                const char*         filename
                                );

void    ComputeInverseLoreta    (   
                                const AMatrix&      Kin,            TPoints&                solpointsin,        TSelection              spsrejected,
                                NeighborhoodType    neighborhood,   NeighborhoodReduction   neighbors26to18,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename,
                                TVerboseFile*       verbose = 0
                                );

void    ComputeInverseSLoreta   (   
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                );

void    ComputeInverseELoreta   (
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                );

void    ComputeInverseDale      (   
                                const AMatrix&      Kin,            TSelection              spsrejected,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename
                                );

void    ComputeInverseLaura     (   
                                const AMatrix&      Kin,            TPoints&                solpointsin,    TSelection              spsrejected,
                                NeighborhoodType    neighborhood,   double                  laurapower,     NeighborhoodReduction   neighbors26to18,
                                bool                regularization,
                                const TStrings&     xyznames,       const TStrings&         spnamesin, 
                                const char*         filename,
                                TVerboseFile*       verbose = 0
                                );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
