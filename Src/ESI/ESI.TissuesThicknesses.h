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

#include    "TVector.h"                 // TMap
#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Roche & Lillie methods

// Investigation of the critical geometric characteristics of living human skulls utilising medical image analysis techniques
// Haiyan Li, Jesse Ruan, Zhonghua Xie, Hao Wang, Wengling Liu
//                      Male                    Female
//  Frontal     6.58 mm     7.49%       7.48 mm     8.77%
//  Parietal    5.37 mm     7.38%       5.58 mm     7.97%
//  Occipital   7.56 mm     8.60%       8.17 mm     9.58%
//  Average                 7.82%                   8.77%
//  Average                             8.30%
//
//  Length      175.81 mm               170.61 mm
//  Breadth     145.35 mm               140.11 mm
//  Average                 158 mm

                                        // Skull & conductivity functions
constexpr double    SkullRocheMinAge        =  -0.5;
//constexpr double    SkullRocheMinAge        =  -1.0;    // to draw a nice curve only
constexpr double    SkullRocheMaxAge        =  20.0;
constexpr double    SkullLillieMinAge       =  20.0;
constexpr double    SkullLillieMaxAge       = 100.0;
constexpr double    SkullThicknessMinAge    = SkullRocheMinAge;
constexpr double    SkullThicknessMaxAge    = SkullLillieMaxAge;

                                        // Synthetic formula between Roche and Lillie
double      AgeToSkullThickness     ( double age );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    SkullCompactToSpongyRatio   = 3.6;
                                        // frontal:45%; parietal:66% - Note that this indeed leads to a spongy / compact conductivitiy ratio of 3.6!
constexpr double    SkullSpongyPercentage       = 0.55;
                                        // if we need to set an absolute compact table thickness, in [mm] (frontal 2.15, parietal 1.15)
constexpr double    SkullCompactThickness       = 1.7;
constexpr double    SkullCompactMinThickness    = 1.0;
constexpr double    SkullCompactMaxThickness    = 2.4;

                                        // Minimum tissues thickness - could be small, but not 0
constexpr double    MinCsfThickness             = 0.1;
constexpr double    MaxCsfThickness             = 4.0;  // will actually go up to 5
constexpr double    MinSpongySkullThickness     = 0.1;
constexpr double    MinSkullThickness           = 0.1;
constexpr double    MinScalpThickness           = 1.0;


//----------------------------------------------------------------------------
                                                    // Associated with each tissues, their inner and outer limits + thickness, both absolute and relative
enum        TissuesLimits
            {
            InnerAbs,
            OuterAbs,
            ThickAbs,

            InnerRel,
            OuterRel,
            ThickRel,

            NumTissuesLimits
            };

extern  char    TissuesLimitsString[ NumTissuesLimits ][ 8 ];

inline  bool    IsTissuesLimitAbs   ( int li )      { return  li <= ThickAbs; }
inline  bool    IsTissuesLimitRel   ( int li )      { return  li >= InnerRel; }


//----------------------------------------------------------------------------

enum                            SpatialFilterType;
class                           TPoints;
class                           TElectrodesDoc;
class                           TVerboseFile;
template <class TypeD> class    TArray3;


double          SkullThicknessToSpongy  (   
                                double      skullthickness,
                                double      spongypercentage,
                                double      compactminthickness,    double      compactmaxthickness
                                );
                                        // Single estimate
bool            EstimateSkullRadii    (   
                                const TPointFloat&      p,
                                const Volume&           fullvolume,         const Volume&           skulllimit,         const Volume&     brainlimit, 
                                double                  fullbackground,     double                  brainbackground,
                                const TPointFloat&      center,
                                float&                  innercsf,
                                float&                  innerskull,
                                float&                  outerskull
                                );
                                        // With resampling
void            EstimateSkullRadii   (
                                const TPoints&          points,
                                const Volume&           fullvolume,         const Volume&           skulllimit,         const Volume&     brainlimit, 
                                double                  fullbackground,     double                  brainbackground,
                                const TPointFloat&      center,
                                TMap&                   innercsf,
                                TMap&                   innerskull,
                                TMap&                   outerskull
                                );

bool            EstimateTissuesRadii_T1 (
                                const TPoints&          points,
                                SpatialFilterType       smoothing,          const TElectrodesDoc*   xyzdoc,
                                const Volume&           fullvolume,         const Volume&           skulllimit,         const Volume&     brainlimit, 
                                const TPointFloat       inversecenter,
                                const TPointDouble&     voxelsize,
                                bool                    adjustradius,       double                  targetskullthickness,
                                TArray3<float>&         tissuesradii
                                );


//----------------------------------------------------------------------------

bool            EstimateTissuesRadii_Segmentation ( 
                                const TPoints&          points,
                                SpatialFilterType       smoothing,          const TElectrodesDoc*   xyzdoc,
                                const Volume&           tissues,            TPointDouble            mritissuesorigin,
                                TPointDouble            mricenter,          const TPointDouble&     voxelsize,
                                TPointFloat             inversecenter,
                                bool                    adjustradius,       double                  targetskullthickness,
                                TArray3<float>&         tissuesradii,
                                TVerboseFile*           verbose     = 0
                                );


//----------------------------------------------------------------------------

void    WriteTissuesRadiiToFile         (   const TArray3<float>&   tissuesradii,       const TSelection&       seltissues,
                                            const TPoints&          xyzpoints,          const TStrings&         xyznames,
                                            const TPointFloat&      mricenter,
                                            const TPointFloat&      inversecenter,
                                            const char*             filesurfaceall
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
