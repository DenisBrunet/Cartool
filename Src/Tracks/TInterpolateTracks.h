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
#include    "Math.Armadillo.h"
#include    "Math.TMatrix44.h"
#include    "Strings.TStrings.h"
#include    "Files.TFileName.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define             InterpolationTitle              "Tracks Interpolation"


enum                TracksInterpolationType
                    {
                    UnknownInterpolation,
                    Interpolation2DSurfaceSpline,                   // worst method
                    InterpolationSphericalSpline,                   // best method 1
                    InterpolationSphericalCurrentDensitySpline,     // difficult to assess, but as it is derived from method above, we can infer it is also as good
                    Interpolation3DSpline,                          // best method 2

                    NumInterpolationTypes,
                    DefaultInterpolationType    = Interpolation3DSpline
                    };

extern const char   InterpolationTypeInfix[ NumInterpolationTypes ][ 16 ];


enum                InterpolationTargetOptions
                    {
                    BackToOriginalElectrodes,
                    ToOtherElectrodes
                    };

enum                ElectrodesNormalizationOptions
                    {
                    AlreadyNormalized,
                    FiducialNormalization
                    };

enum                LandmarksOptions
                    {
                    LandmarkFront,
                    LandmarkLeft, 
                    LandmarkTop, 
                    LandmarkRight, 
                    LandmarkRear, 
                    NumLandmarks,

                    LandmarkDescription = NumLandmarks,     // used in predefined landmarks array, see TInterpolateTracksDialog
                    NumLandmarksInfo,
                    };


constexpr int       MinInterpolationDegree          = 2;
constexpr int       MaxInterpolationDegree          = 4;    // !using degree 4 is not recommended, because it seems to produce weird results with actual data!
constexpr int       DefaultInterpolationDegree      = 2;    // After simulation tests, degree 2 performs the best in overall errors, and reduces the chances of producing outliers


constexpr int       NumLegendreTermsInterpol        = 51;


constexpr int       RemovingElecMaxSize             = 64 * KiloByte;


//----------------------------------------------------------------------------
                // Surface Spline formula from:
                //      Perrin, Pernier, Bertrand, Giard & Echallier, "Mapping of Scalp Potentials by Surface Spline Interpolation", Electr & Clinical Neuroph, 1987
                //
                // Spherical Spline and Current Density formulas from:
                //      Perrin, Pernier, Bertrand & Echallier, "Spherical Splines for Scalp Potential and Current Density Mapping", Electr and Clinical Neurophys, 1989
                // Formula correction:
                //      Electroencephalography and clinical Neurophysiology, 1990
                //
                // 3D Spline formula from:
                //      Thomas C. Ferree, "Spline Interpolation of the Scalp EEG", Technical Note from Electrical Geodesics Inc., 8/22/2000


TMatrix44       ComputeFiducialTransform    (   const char*             cfront, 
                                                const char*             cleft, 
                                                const char*             ctop,
                                                const char*             cright, 
                                                const char*             crear, 
                                                const TPoints&          points,
                                                const TStrings&         pointnames
                                            );

TPoints         TransformToFiducial         (   const TPoints&          points, 
                                                const TMatrix44&        transform, 
                                                TracksInterpolationType interpolationtype 
                                            );

double          SplineInterpolationKernel   (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPointFloat&      v1,                 const TPointFloat&      v2 
                                            );

ASymmetricMatrix SplineFillA                (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPoints&          frompoints 
                                            );

void            ComputeKernels              (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPoints&          frompoints,
                                                const TPoints&          topoints,
                                                TArray2<double>&        kernel
                                            );


//----------------------------------------------------------------------------
                                        // We need to wrap up many files, points and strings into a single place for simpler initialization and use
enum    SavingEegFileTypes;
class   TTracksDoc;
class   TGoF;


class   TInterpolateTracks
{
public:
                        TInterpolateTracks  ();

                        TInterpolateTracks  (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                const char*                     frombadelectrodes,
                                                
                                                const char*                     destxyzfile,        // back to from if empty
                                                ElectrodesNormalizationOptions  destnormalized,
                                                const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,

                                                const char*                     temppath    = 0,
                                                VerboseType                     verbosey    = Interactive
                                            )   { Set ( interpolationtype, splinedegree, fromxyzfile, fromnormalized, fromfront, fromleft, fromtop, fromright, fromrear, frombadelectrodes, destxyzfile, destnormalized, destfront, destleft, desttop, destright, destrear, temppath, verbosey ); }

                        TInterpolateTracks  (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                const char*                     frombadelectrodes,

                                                const char*                     temppath    = 0,
                                                VerboseType                     verbosey    = Interactive
                                            )   { Set ( interpolationtype, splinedegree, fromxyzfile, fromnormalized, fromfront, fromleft, fromtop, fromright, fromrear, frombadelectrodes, temppath, verbosey ); }

                        TInterpolateTracks  (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                
                                                const char*                     destxyzfile,
                                                ElectrodesNormalizationOptions  destnormalized,
                                                const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,

                                                const char*                     temppath    = 0,    // could be a file, a path, or null
                                                VerboseType                     verbosey    = Interactive
                                            )   { Set ( interpolationtype, splinedegree, fromxyzfile, fromnormalized, fromfront, fromleft, fromtop, fromright, fromrear, destxyzfile, destnormalized, destfront, destleft, desttop, destright, destrear, temppath, verbosey ); }


    bool                IsOpen              ()  const   { return InterpolationType != UnknownInterpolation && FromOrigPoints.IsNotEmpty () && FromOrigPointsNames.IsNotEmpty () && DestPoints.IsNotEmpty () && DestPointsNames.IsNotEmpty (); }
    void                Reset               ();         // resetting fields
    void                FilesCleanUp        ()  const;  // actually deleting temp files

                                        // Full initialization for all cases: bad electrodes and/or switching electrodes models
    bool                Set                 (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                const char*                     frombadelectrodes,
                                                
                                                const char*                     destxyzfile,        // back to from if empty
                                                ElectrodesNormalizationOptions  destnormalized,
                                                const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,

                                                const char*                     temppath    = 0,    // could be a file, a path, or null
                                                VerboseType                     verbosey    = Interactive
                                            );
                                        // Simpler initialization for only bad electrodes interpolation
    bool                Set                 (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                const char*                     frombadelectrodes,

                                                const char*                     temppath    = 0,
                                                VerboseType                     verbosey    = Interactive
                                            );
                                        // Simpler initialization to go from one electrodes model to another one
    bool                Set                 (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                                const char*                     fromxyzfile,
                                                ElectrodesNormalizationOptions  fromnormalized,
                                                const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                                
                                                const char*                     destxyzfile,
                                                ElectrodesNormalizationOptions  destnormalized,
                                                const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,

                                                const char*                     temppath    = 0,    // could be a file, a path, or null
                                                VerboseType                     verbosey    = Interactive
                                            );


    bool                InterpolateTracks   (   const TTracksDoc*   eegdoc,
                                                const char*         infixfilename,  const char*         fileoutext, char*               fileout,
                                                VerboseType         verbosey    = Interactive
                                            );
                                        // Wrapper to method above
    bool                InterpolateTracks   (   const char*         fileeeg,
                                                const char*         infixfilename,  const char*         fileoutext, char*               fileout,
                                                VerboseType         verbosey    = Interactive
                                            );

                                        // We can give read-only access to these guys
    const TPoints&      GetFromOrigPoints       ()  const   { return FromOrigPoints;        }
    const TPoints&      GetFromPoints           ()  const   { return FromPoints;            }
    const TPoints&      GetDestPoints           ()  const   { return DestPoints;            }
    const TPoints&      GetFrom_To_FidPoints    ()  const   { return From_To_FidPoints;     }
    const TPoints&      GetDest_To_FidPoints    ()  const   { return Dest_To_FidPoints;     }
    const TStrings&     GetFromOrigPointsNames  ()  const   { return FromOrigPointsNames;   }
    const TStrings&     GetFromPointsNames      ()  const   { return FromPointsNames;       }
    const TStrings&     GetDestPointsNames      ()  const   { return DestPointsNames;       }


protected:

    TracksInterpolationType         InterpolationType;
    int                             SplineDegree;   // Following article convention, Polynomial Degree = Spline Degree - 1
    InterpolationTargetOptions      TargetSpace;    // Specifying the target space: original vs another electrodes space

                                                    // We have to handle quite a few different files:
    TFileName           FromOrigXyzFile;
    TFileName           FromXyzFile;
    TFileName           FromXyzExclFile;
    TFileName           FromXyz_To_FidFile;
    TFileName           FromXyz_To_DestXyzFile;

    TFileName           DestXyzFile;
    TFileName           DestXyz_To_FidFile;
    TFileName           DestXyz_To_FromXyzFile;

                                                    // We also need various list of points:
    TPoints             FromOrigPoints;             // Original 'From' points
    TStrings            FromOrigPointsNames;
    std::string         FromBadElectrodes;
    TPoints             FromPoints;                 // Points without the bad electrodes
    TStrings            FromPointsNames;
    TPoints             From_To_FidPoints;          // Points without the bad electrodes, normalized - Those used for computation

    ElectrodesNormalizationOptions  FromNormalized;
    TStrings                        FromFiducials;  // String list for front, left, top, right and rear landmarks


    TPoints             DestPoints;                 // 'To' points
    TStrings            DestPointsNames;
    TPoints             Dest_To_FidPoints;          // 'To' points, normalized - Those used for computation

    ElectrodesNormalizationOptions  DestNormalized;
    TStrings                        DestFiducials;  // String list for front, left, top, right and rear landmarks

                                                    // All transform matrices
    TMatrix44           From_To_Fid;                // Used for actual spline computation
    TMatrix44           Fid_To_From;
    TMatrix44           Dest_To_Fid;
    TMatrix44           Dest_To_From;               // Only used for visual checks
    TMatrix44           From_To_Dest;

                                                    // Variables used for actual computation - set once
    ASymmetricMatrix    A;                          // Spline matrix A
    TArmaLUSolver       LU;                         // LU decomposition solver
    TArray2<double>     Kernel;
    TPoints             Dest_To_FidPoints_Power[ MaxInterpolationDegree ];
    TArray1<int>        ToHasSamePositionAsFrom;


    bool                SetTransformMatrices    ();
    bool                SetSpline               ();

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
