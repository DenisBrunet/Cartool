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
#include    "Strings.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                            CrisLeadFieldPresets;
class                           TPoints;
class                           TSelection;
class                           TDipole;
class                           TFitModelOnPoints;
class                           OneLeadFieldPreset;
template <class TypeD> class    TArray1;
template <class TypeD> class    TArray3;


//----------------------------------------------------------------------------
                                        // Lead Field properties
enum        LeadFieldFlag
            {
            NoLeadField                     = 0x000000,

                                                            // Geometrical model
            GeometryLSMAC                   = 0x100000,     // LSMAC spherization

                                                            // Target species
            Human                           = 0x010000,
            Macaque                         = 0x020000,
            Mouse                           = 0x040000,

                                                            // Electromagnetic model
            IsotropicAry                    = 0x001000,     // Isotropic Ary approximation (always 3-Shell)
            IsotropicAryLegacy              = 0x002000,     // Isotropic Ary approximation (always 3-Shell) older but well-tested parameters
            IsotropicNShellSpherical        = 0x004000,     // Isotropic N-Shell exact spherical equations with Legendre polynomials (Zhang 1995)

                                                            // More spherization options
            SkullRadiusFixedRatio           = 0x000010,     // constant ratio all the time; if radius increases, so does the skull too
            SkullRadiusModulatedRatio       = 0x000020,     // ratio adjusted according to radius, as to remain constant in absolute
            SkullRadiusGiven                = 0x000040,     // tissues thicknesses are provided, either coming from MRI estimates or Tissues file
            };

                                        // We need to detail many parameters for each known Lead Field
class       OneLeadFieldPreset
{
public:
    CrisLeadFieldPresets    Code;               // preset ID for TCreateInverseMatricesDialog
    char                    Text [ 128 ];       // for UI
    char                    Title[  32 ];       // for Progress Bar
    LeadFieldFlag           Flags;              // Lead Field actual properties
    int                     NumLayers;          // what it says
    double                  OuterSkullRadius;   // optional - used for fixed / modulated radius - !set to 0 if not relevant!
    double                  InnerSkullRadius;



    bool                    IsUndefined                 ()  const   { return    StringIsEmpty ( Text );                         }

    bool                    IsGeometryLSMAC             ()  const   { return    IsFlag ( Flags, GeometryLSMAC               );  }

    bool                    IsHuman                     ()  const   { return    IsFlag ( Flags, Human                       );  }
    bool                    IsMacaque                   ()  const   { return    IsFlag ( Flags, Macaque                     );  }
    bool                    IsMouse                     ()  const   { return    IsFlag ( Flags, Mouse                       );  }

    bool                    IsIsotropicSphericalAry     ()  const   { return    IsFlag ( Flags, IsotropicAry                );  }   // always 3-Shell BTW
    bool                    IsIsotropicNShellSpherical  ()  const   { return    IsFlag ( Flags, IsotropicNShellSpherical    );  }

    bool                    IsSkullRadiusFixedRatio     ()  const   { return    IsFlag ( Flags, SkullRadiusFixedRatio       );  }
    bool                    IsSkullRadiusModulatedRatio ()  const   { return    IsFlag ( Flags, SkullRadiusModulatedRatio   );  }
    bool                    IsSkullRadiusGiven          ()  const   { return    IsFlag ( Flags, SkullRadiusGiven            );  }

                                        // Here we list the exact combinations that are implemented
    bool                    IsIsotropic3ShellSphericalAry() const   { return    IsIsotropicSphericalAry    () && NumLayers == 3;            }
    bool                    IsIsotropic3ShellSpherical  ()  const   { return    IsIsotropicNShellSpherical () && NumLayers == 3;            }
    bool                    IsIsotropic4ShellSpherical  ()  const   { return    IsIsotropicNShellSpherical () && NumLayers == 4;            }
    bool                    IsIsotropic6ShellSpherical  ()  const   { return    IsIsotropicNShellSpherical () && NumLayers == 6;            }

    void                    GetTissuesSelection         (   TSelection&     seltissues  )   const;
    void                    GetLayersConductivities     (   double          skullcond,  double&             skullcompactcond,   double&     skullspongycond,
                                                            TSelection&     seltissues, TArray1<double>&    sigma   )   const;
};


//----------------------------------------------------------------------------

void            InterpolateLeadFieldLinear      (   AMatrix&            K,              const TPoints&      inputsolpoint,  
                                                    TPoints&            outputsolpoint, TSelection&         spsrejected     );
void            CheckNullLeadField              (   const AMatrix&      K,              TSelection&         spsrejected     );

void            RejectPointsFromLeadField       (   AMatrix&            K,              const TSelection&   spsrejected     );


//----------------------------------------------------------------------------
                                        // Ary math functions

                                        // We can optimize the number of Legendre polynomials according to the input value: higher values need more terms
constexpr int       NumLegendreTermsAryMin  = 15;
constexpr int       NumLegendreTermsAryMax  = 30;
                                        // lowest limit for reliable use of Legendre polynomials
constexpr double    Shell3to1LowestRadius3  = 0.05;
                                        // step for initial linear search
constexpr double    Shell3to1StepInit       = 0.02;
                                        // convergence precision form radius3 to radius1
constexpr double    Shell3to1Convergence    = 1e-8;

                                        // !returns _ratios_ radius3/radius!
double          AryR3ToR1           ( double radius3toradius1, double Xi, double innerskullradius, double outerskullradius );
double          AryM3ToM1           ( double radius3,          double Xi, double innerskullradius, double outerskullradius );

long double     AryFn               ( double n,                       double Xi, double innerskullradius, double outerskullradius );
double          RadialRho           ( double radius1, double radius3, double Xi, double innerskullradius, double outerskullradius );
double          RadialR3ToR1        ( double radius3,                 double Xi, double innerskullradius, double outerskullradius );
double          RadialM3ToM1        ( double radius1, double radius3, double Xi, double innerskullradius, double outerskullradius );
double          TangentialRho       ( double radius1, double radius3, double Xi, double innerskullradius, double outerskullradius );
double          TangentialR3ToR1    ( double radius3,                 double Xi, double innerskullradius, double outerskullradius );
double          TangentialM3ToM1    ( double radius1, double radius3, double Xi, double innerskullradius, double outerskullradius );


//----------------------------------------------------------------------------
                                        // All known potential computation formulas

                                        // Most functions can either compute a regular potential, or the Lead Field special case
enum            PotentialFlags
                {
                ComputePotentials,      // dipole direction and moment has been set by caller, just compute the voltage
                ComputeLeadField,       // dipole direction has to be set toward electrode, returns the scaled direction (voltage is ignored)
                };


TVector3Float   DipoleElectricField         (
                                            const TVector3Float&    dipoledir, 
                                            const TVector3Float&    relpos, 
                                            double                  sigma
                                            );

double          PotentialIsotropic1ShellExactSphericalVector (   
                                            const TDipole&          dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            );

double          PotentialIsotropic1ShellApproxRealVector (
                                            const TDipole&          dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            );

double          PotentialIsotropic1ShellExactSphericalLegendre (
                                            TDipole&                dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            );

double          PotentialIsotropic3ShellApproxSphericalAry (   
                                            TDipole&                dipole,     PotentialFlags          flags,
                                            const TPointFloat&      electrodepos,
                                            const TArray1<double>&  R,          const TArray1<double>&  sigma
                                            );

double          PotentialIsotropicNShellExactSphericalLegendre (   
                                            TDipole&                dipole,     PotentialFlags          flags,
                                            const TPointFloat&      electrodepos,
                                            const TArray1<double>&  R,          const TArray1<double>&  sigma,
                                            int                     maxterms,   double                  convergence
                                            );


//----------------------------------------------------------------------------
                                        // Actual computation of the Lead Field matrix with the LSMAC model
void    ComputeLeadFieldLSMAC           (
                                            const OneLeadFieldPreset&   lfpreset,
                                            const TPoints&              xyzpoints,
                                            const TPoints&              solpoints,
                                            const TFitModelOnPoints&    surfacemodel,
                                            const TArray1<double>&      sigma,
                                            const TArray3<float>&       elradius,
                                            AMatrix&                    K
                                        );


//----------------------------------------------------------------------------
                                        // High-level wrapper: takes head + brain + electrodes + solution points files
                                        // and computes the Lead Field for a given type and age
                                        // It will estimate the skull radii by scanning the T1 volumes
bool    ComputeLeadFieldLSMAC_T1        (   const OneLeadFieldPreset&   lfpreset,
                                            double                  age,
                                            const char*             headfile,
                                            const char*             brainfile,
                                            const char*             xyzfile,
                                            const char*             spfile,
                                            AMatrix&                K,
                                            TPoints&                xyzpoints,  TStrings&       xyznames,       // in inverse space coordinates
                                            TPoints&                solpoints,  TStrings&       solpointsnames, // in inverse space coordinates
                                            TPointFloat&            mricentertoinversecenter,
                                            TArray3<float>&         tissuesradii
                                        );

                                        // High-level wrapper: takes a tissues segmented volume + electrodes + solution points files
                                        // and computes the Lead Field for a given type and age
                                        // It will compute the skull radii by scanning the segmented volume
bool    ComputeLeadFieldLSMAC_Segmentation  (   const OneLeadFieldPreset&   lfpreset,
                                                double                  age,
                                                const char*             headfile,
                                                const char*             tissuesfile,
                                                const char*             xyzfile,
                                                const char*             spfile,
                                                AMatrix&                K,
                                                TPoints&                xyzpoints,  TStrings&       xyznames,
                                                TPoints&                solpoints,  TStrings&       solpointsnames,
                                                TPointFloat&            mricentertoinversecenter,
                                                TArray3<float>&         tissuesradii
                                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
