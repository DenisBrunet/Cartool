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

#include    "Geometry.TPoint.h"
#include    "Geometry.TPoints.h"
#include    "Math.TMatrix44.h"
#include    "Geometry.TDipole.h"

#include    "GlobalOptimize.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Explored parameters, for geometrical transforms
enum {
                    UnknownParameterType,

                    Scale,
                    ScaleX,
                    ScaleY,
                    ScaleZ,
                    RotationX,
                    RotationY,
                    RotationZ,
                    TranslationX,
                    TranslationY,
                    TranslationZ,
//                  PinchXtoYZ,
//                  PinchYtoXZ,
//                  PinchZtoXY,
                    PinchXtoY,
                    PinchXtoZ,
                    PinchYtoX,
                    PinchYtoZ,
                    PinchZtoX,
                    PinchZtoY,
                    ShearYtoX,
                    ShearZtoX,
                    ShearXtoY,
                    ShearZtoY,
                    ShearXtoZ,
                    ShearYtoZ,
//                  SinusPinchXtoYZ,
//                  SinusPinchYtoXZ,
//                  SinusPinchZtoXY,
                    SinusPinchXtoY,
                    SinusPinchXtoZ,
                    SinusPinchYtoX,
                    SinusPinchYtoZ,
                    TopBumpX,
                    TopBumpY,
                    TopLateralBumpX,
                    TopLateralBumpY,
                    FlattenX,
                    FlattenXPos,
                    FlattenXNeg,
                    FlattenY,
                    FlattenYPos,
                    FlattenYNeg,
                    FlattenZ,
                    FlattenZPos,
                    FlattenZNeg,
                    InflateLowZtoX,
//                  InflateLowZtoY,
//                  InflateLowZtoYHollow,
                    InflateLowZtoYPos,
                    InflateLowZtoYPosHollow,
                    InflateLowZtoYNeg,
                    InflateLowZtoYNegHollow,
                    DeflateLowZtoXYPos,
                    DeflateLowZtoXYNeg,
                    RotationPrecession,
                    RotationNutation,
                    RotationRotation,

                    FitVolumeFromIntensityRescale,
//                  FitVolumeFromIntensityOffset,   // intensity Offset does not seem to be enough to match 2 histograms, so leave it for now..

                    FitVolumeShearShiftX,
                    FitVolumeShearShiftY,
                    FitVolumeShearShiftZ,
                    FitVolumeNormCenterRotateX,
                    FitVolumeNormCenterRotateZ,
                    FitVolumeShearXtoY,
                    FitVolumeShearXtoZ,
                    FitVolumeShearYtoX,
                    FitVolumeShearYtoZ,
                    FitVolumeShearZtoX,
                    FitVolumeShearZtoY,

//                  FitVolumePerspectiveZtoXYDelta,
//                  FitVolumePerspectiveZtoXDelta,
//                  FitVolumePerspectiveZtoYDelta,

                    NumGeometricalTransforms
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Options to model, used in GetSolution
enum                FitModelDistance
                    {
                                        // model is a surface that FITS the points
                    FitModelNorm1,
                    FitModelWeightedNorm1,
                    FitModelNorm2,
                    FitModelWeightedNorm2,
                                        // model is a container AROUND the points
                    ContainModelNorm1,
                    ContainModelNorm2,

                    NumPointsFitting
                    };

extern char         StringPointsFitting[ NumPointsFitting ][ 16 ];


constexpr int       GlobalOptimizeMaxPoints     = 65536;

                                        // Recommended parameters for BFS and BFE surface fitting
constexpr GOMethod          FitModelPointsMethod            = GlobalNelderMead;
constexpr FitModelDistance  FitModelPointsHow               = FitModelNorm1;
                                        // Recommended parameters for BFS containing points
constexpr GOMethod          BFSContainModelPointsMethod     = GlobalNelderMead;
constexpr FitModelDistance  BFSContainModelPointsHow        = FitModelNorm1;
                                        // Recommended parameters for BFE containing points, 1 rotation, 2 translations
constexpr GOMethod          BFEContainModelPointsMethod     = CyclicalCrossHairScan;
constexpr FitModelDistance  BFEContainModelPointsHow        = FitModelNorm1;
                                        // Recommended parameters for BFE containing points, 3 rotations, 3 translations
//constexpr GOMethod        BFEContainModelPointsMethod     = GlobalNelderMead;
//constexpr FitModelDistance  BFEContainModelPointsHow      = FitModelNorm2;
                                        // Recommended parameters for Head Model surface fitting
constexpr GOMethod          HeadModelPointsMethod           = CyclicalCrossHairScan;
constexpr FitModelDistance  HeadModelPointsHow              = FitModelNorm1;
//constexpr FitModelDistance  HeadModelPointsHow            = FitModelWeightedNorm1;    // also good


//----------------------------------------------------------------------------

class  TFitModelOnPoints    :   public  TGlobalOptimize
{
public:
                    TFitModelOnPoints ();
                    TFitModelOnPoints ( const TPoints& points );


    TPoints                 Points;         // target shape
    TPointFloat             Center;         // handy to know the center and boundary limits of points
    TBoundingBox<double>    Bounding;


    void            Reset ();

    double          Evaluate                ( TEasyStats *stat = 0 );

    void            Transform               ( TPointFloat &p )  const;
    TVector3Float   GetTranslation          ()  const   { return TVector3Float ( (GLfloat) HasValue ( TranslationX ) ? GetValue ( TranslationX ) : 0, 
                                                                                 (GLfloat) HasValue ( TranslationY ) ? GetValue ( TranslationY ) : 0, 
                                                                                 (GLfloat) HasValue ( TranslationZ ) ? GetValue ( TranslationZ ) : 0 ); }

//  void            ShowProgress ();


    double          GetMaxRadius    ()                              const;
    double          GetMeanRadius   ()                              const;
    TPointFloat     ToModel         ( TPointFloat p      )          const;              // Results with original scaling
    TPoints         ToModel         ( const TPoints& listp, bool translate )    const;
    TPointFloat     Spherize        ( TPointFloat p      )          const;              // Results in radius 1
    TDipole         Spherize        ( const TDipole&     d )        const;
    TPoints         Spherize        ( const TPoints& listp )        const;
    TPointFloat     Unspherize      ( TPointFloat p, bool translate )   const;          // Results back to original space, with optional translation
    TPoints         Unspherize      ( const TPoints& listp )        const;


    void            WriteFile ( const char* file, TVector3Float* translate = 0 )  const;


private:

    void            _Transform              ( TPointFloat &p )  const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                {
                    PointsSagittal,
                    PointsTransverse,
                    PointsReorientTop,  // top == Z axis
                    };
                                        // Recommended parameter
//constexpr GOMethod  PointsPropertiesMethod        = GlobalBoxScan;
constexpr GOMethod  PointsPropertiesMethod          = WeakestDimensionCrossHairScan;
constexpr double    PointsPropertiesPrecision       = 1e-3;


//----------------------------------------------------------------------------
                                        // Finding geometrical properties on a single set of points
class  TPointsProperties    :   public  TGlobalOptimize
{
public:
                    TPointsProperties ();
                    TPointsProperties ( const TPoints& points );


    TPoints         Points;


    void            Reset ();

    double          Evaluate            ( TEasyStats *stat = 0 );
    double          EvaluateSagittal    ( TEasyStats *stat = 0 );
    double          EvaluateTransverse  ( TEasyStats *stat = 0 );
    double          EvaluateToTop       ( TEasyStats *stat = 0 );

    void            Transform           ( TPointFloat& p )                                                           const;
    void            Transform           ( TPoints& points, bool reorient = false, TPoints* auxpoints = 0 ) const;

    static void     ResolveFrontBackOrientation ( TPoints& points, TPoints* auxpoints = 0 );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Coregistering a set of points on another set
enum                {
                    ClosestPoints,              // fitting a set of points to another set of points by looking at closest fit
                    MatchingPairs,              // fitting between the same number of points
                    };
                                        // Recommended parameter
constexpr GOMethod  FitPointsOnPointPairsMethod     = GlobalNelderMead;
constexpr GOMethod  FitPointsOnPointClosestMethod   = WeakestGroupCrossHairScan; // WeakestDimensionCrossHairScan;


class  TFitPointsOnPoints   :   public  TGlobalOptimize
{
public:
                    TFitPointsOnPoints ();
                    TFitPointsOnPoints ( const TPoints& frompoints, const TPoints& topoints, const TMatrix44* mriabstoguillotine = 0 );


    TPoints         FromPoints;         // shape to adjust
    TPoints         ToPoints;           // to target shape


    void            Reset ();

    double          Evaluate                 ( TEasyStats *stat = 0 );
    double          EvaluatePoints           ( TEasyStats *stat = 0 );
    double          EvaluatePairs            ( TEasyStats *stat = 0 );

    double          GetMinimumDistance  ( const TPointFloat& p );
    double          GetClosestPoint     ( TPointFloat& p );
    void            Transform           ( TPointFloat &p,  bool scaling = true );
    void            Transform           ( TPoints& points, bool scaling = true );

//  void            ShowProgress ();


protected:

    TMatrix44       MriAbsToGuillotine; // there can be a cutting plane, to eliminate electrodes below the neck cut

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







