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

#include    "TVolume.h"
#include    "Math.TMatrix44.h"
#include    "Geometry.TPoint.h"
#include    "CartoolTypes.h"
#include    "Files.TOpenDoc.h"

#include    "GlobalOptimize.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TCacheVolume;
template <class TypeD> class        TCacheVolumes;


//----------------------------------------------------------------------------
                                        // Options to Volume Properties, used in GetSolution
enum                {
                    TransversePlaneLongest,         // searching for longest slice
                    TransversePlaneBiggestBox,      // searching for biggest box
                    TransversePlaneBiggestSurface,  // searching for biggest slice
                    TransversePlaneMNIT1,           // matching the MNI template T1 central slice
                    TransversePlaneMNIT2,           // matching the MNI template T2 central slice

                    TransversePlaneGuillotine,

                    SagittalPlaneSymmetric,         // general case
                    SagittalPlaneSymmetricT1,       // adding some constraints for T1
                    SagittalPlaneSymmetricT1Gad,    // adding some constraints for T1 + Gadolinium
//                  SagittalPlaneSymmetricT2,       // not actually needed, as general case SagittalPlaneSymmetric runs well
                    };

inline bool         IsTransversePlane    ( int how )    { return IsInsideLimits ( how, (int) TransversePlaneLongest,    (int) TransversePlaneMNIT2        ); }
inline bool         IsTransversePlaneMNI ( int how )    { return IsInsideLimits ( how, (int) TransversePlaneMNIT1,      (int) TransversePlaneMNIT2        ); }
inline bool         IsGuillotinePlane    ( int how )    { return IsInsideLimits ( how, (int) TransversePlaneGuillotine, (int) TransversePlaneGuillotine   ); }
inline bool         IsSagittalPlane      ( int how )    { return IsInsideLimits ( how, (int) SagittalPlaneSymmetric,    (int) SagittalPlaneSymmetricT1Gad ); }


// Sagittal Plane:
//  Input:  matrix  LocalOrient -> RASOrient
//          center  LocalCenter
//  Output: matrix  Rotates(RASSpace) -> RASOrient   -> LocalOrient -> Translate(+LocalCenter)   !inverted compared to input!
//          center  LocalCenter
//          - matrix is inverted compared to the input sequence
//          - matrix embeds a translation, duplicated by the center output
// 
// Transverse Plane:
//  Input:  matrix  LocalOrient -> RASOrient
//          center  LocalCenter
//  Output: matrix  Rotate(RASSpace) -> RASOrient -> LocalOrient -> Translate(+LocalCenter)   !inverted compared to input!
//          center  LocalCenter
//          - matrix is inverted compared to the input sequence
//          - matrix embeds a translation, duplicated by the center output
// 


//----------------------------------------------------------------------------
                                        // Central sagittal slices from MNI Asym 2009c template
constexpr char*     MniT1BrainCentralSliceFileName    = "mni_icbm152_t1_tal_nlin_asym_09c.Brain.SagittalSlice7.nii";
constexpr char*     MniT2BrainCentralSliceFileName    = "mni_icbm152_t2_tal_nlin_asym_09c.Brain.SagittalSlice7.nii";


//----------------------------------------------------------------------------

class  TVolumeProperties    :   public TGlobalOptimize
{
public:
                    TVolumeProperties ();
                    TVolumeProperties (   const TVolumeDoc*     mridoc,     
                                          const TMatrix44&      datatoras,
                                          double                threshold     = -1,
                                          const TMatrix44*      normtomri     = 0,  // input
                                          const TPointDouble*   center        = 0   // input
                                      );

                                        // local copy of MRI variables, still public
    Volume              Vol;
    TBoundingBox<double>Bound;
    double              MaxValue;
    double              Threshold;
    int                 ExtraContentType;

    int                 FrontBackIndex;
    int                 UpDownIndex;
    int                 LeftRightIndex;

    TMatrix44           InvStandardOrient;  // RAS -> Local
    TMatrix44           NormToMRI;          // standard oriented & centered slice -> current MRI
    TPointDouble        Center;


    void            Reset ();
    void            SetVolume         (   const TVolumeDoc*     mridoc,     
                                          const TMatrix44&      datatoras,
                                          double                threshold     = -1,
                                          const TMatrix44*      normtomri     = 0,  // input
                                          const TPointDouble*   center        = 0   // input
                                      );


    void            GetSolution ( GOMethod method, int how, double requestedprecision, double outliersprecision, const char* title, TEasyStats* stat = 0 );

    double          Evaluate ( TEasyStats *stat = 0 );

    inline void     EvaluateSagittalPlaneMatrix             ();
    double          EvaluateSagittalPlaneSymmetric          ( TEasyStats *stat = 0 );

    inline void     EvaluateTransversePlaneGuillotineMatrix ( TMatrix44& normtomri );
    double          EvaluateTransversePlaneGuillotine       ( TEasyStats* stat = 0 );

    inline void     EvaluateTransversePlaneMatrix           ( TMatrix44& normtomri );
    double          EvaluateTransversePlaneLongest          ( TEasyStats* stat = 0 );
    double          EvaluateTransversePlaneBiggestBox       ( TEasyStats* stat = 0 );
    double          EvaluateTransversePlaneBiggestSurface   ( TEasyStats* stat = 0 );
    double          EvaluateTransversePlaneMNI              ( TEasyStats* stat = 0 );

    void            CenterSlice ();

//  void            ShowProgress ();


private:

    TOpenDoc<TVolumeDoc>    MniSlicedoc;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Not tested
class  TFitPointsOnVolume   : public TGlobalOptimize
{
public:
                    TFitPointsOnVolume ();
                    TFitPointsOnVolume ( const TPoints& frompoints, const Volume& volume, double threshold, TMatrix44 normtomri, const TMatrix44* mriabstoguillotine = 0 );


    void            Reset ();

    double          Evaluate            ( TEasyStats* stat = 0 );

    double          GetMinimumDistance  ( TPointFloat& p );
    inline void     Transform           ( TPointFloat& p );
    void            Transform           ( TPoints& points );
    void            GetMatrix           ( TMatrix44& m );

//  void            ShowProgress ();


protected:
//  void            SetMaximumRadius ();


    TPoints         FromPoints;         // shape to adjust
    Volume          Vol;                // target

    double          Threshold;
    TMatrix44       NormToMRI;          // standard oriented & centered slice -> current MRI
    TMatrix44       MriAbsToGuillotine; // there can be a cutting plane, to eliminate electrodes below the neck cut
    TPointFloat     Org;
    double          RadiusMax;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Initialization settings
enum                RemapIntensityType 
                    {
                    RemapIntensityNone,         // mandatory
                    RemapIntensityEqualize,     // histogram equalization, for grey level matching
                    RemapIntensityEqualizeBrain,// histogram equalization, specialized for brain
                    RemapIntensityRank,         // ranking, looks like histogram equalization
                    RemapIntensityRankRamp,     // another ranking, with more high values, looking more "natural"
                    RemapIntensityBinarize,     // thresholding & binarize -> shape can be hollow
                    RemapIntensityMask,         // mask is full
                    RemapIntensityInvert,       // prototype if intensity are reverted

                    NumRemapIntensityTypes
                    };

extern  char        RemapIntensityNames[ NumRemapIntensityTypes ][ 32 ];


enum                FitVolumeType 
                    {
                    FitVolumeNone,              //
                    FitVolumeEqualSizes,        // fit exactly the 2 volumes
                    FitVolumeTargetBigger,      // weaker fit, trying to include the target into a bigger source
                    FitVolumeSourceBigger,      // weaker fit, trying to include the source into a bigger target

                    NumFitVolumeTypes,
                    };


inline  bool        IsEqualSizes    ( const FitVolumeType& f )      { return    f == FitVolumeEqualSizes;   }
inline  bool        IsTargetBigger  ( const FitVolumeType& f )      { return    f == FitVolumeTargetBigger; }
inline  bool        IsSourceBigger  ( const FitVolumeType& f )      { return    f == FitVolumeSourceBigger; }


//----------------------------------------------------------------------------
                                        // Fixed parameters:
//enum {
//                    VolumeRadius                    = 0,
//
//                    NumFixedParamsTFitVolumeOnVolume
//                    };


//----------------------------------------------------------------------------
                                        // Currently, only affine transforms is used, including shearing
                                        // but NOT Pinching / Flattening, which are non-linear - these could not be saved in a matrix
class  TFitVolumeOnVolume   : public TGlobalOptimize
{
public:
                    TFitVolumeOnVolume ();
                    TFitVolumeOnVolume ( const TVolumeDoc*  fromvolume,     RemapIntensityType      fromremap,  TMatrix44*  fromrel_fromabs, 
                                         const TVolumeDoc*  tovolume,       RemapIntensityType      toremap,    TMatrix44*  torel_toabs,
                                         FitVolumeType      flags );


    Volume              FromVolume;
    Volume              FromMask;
    RemapIntensityType  FromRemap;
    TBoundingBox<double>FromBound;
    TPointDouble        FromCenter;
    TPointDouble        FromVoxelSize;
    char                FromOrientation[ 4 ];

    Volume              ToVolume;
    Volume              ToMask;
    RemapIntensityType  ToRemap;
    TBoundingBox<double>ToBound;
    TPointDouble        ToCenter;
    TPointDouble        ToVoxelSize;
    char                ToOrientation[ 4 ];

    FitVolumeType       Flags;


    TMatrix44     		FromAbs_FromRel;    // absolute source voxel space to relative source voxel space
//  TMatrix44     		FromRel_FromAbs;    // relative source voxel space to absolute source voxel space
//  TMatrix44     		ToAbs_ToRel;        // absolute target voxel space to relative target voxel space
    TMatrix44     		ToRel_ToAbs;        // relative target voxel space to absolute target voxel space

    TMatrix44     		ToAbs_FromAbs;      // only the current (absolute) transform
    TMatrix44     		FromAbs_ToAbs;
    TMatrix44     		ToRel_FromRel;      // transform from voxel to voxel (relative)
    TMatrix44     		FromRel_ToRel;

    double          DistanceTargetToSource ( double dt );


    void            Reset ();
    void            SetFitVolumeOnVolume    (   const TVolumeDoc*   fromvolume,     RemapIntensityType      fromremap,   TMatrix44*  fromrel_fromabs, 
                                                const TVolumeDoc*   tovolume,       RemapIntensityType      toremap,     TMatrix44*  torel_toabs,
                                                FitVolumeType       flags );

    double          Evaluate                ( TEasyStats *stat = 0 );
    void            EvaluateMatrices        ();
//  void            TransformTargetToSource ( TPointDouble &p );    // !any update in this function should be reverted and included in the next     function!
//  void            TransformSourceToTarget ( TPointDouble &p );    // !any update in this function should be reverted and included in the previous function!
    void            TransformToTarget       ( const Volume& volume, FilterTypes filtertype, InterpolationType interpolate, int numsubsampling, int niftitransform, int niftiintentcode, const char* niftiintentname, const char *file, char *title = 0 );
    void            TransformToSource       ( const Volume& volume, FilterTypes filtertype, InterpolationType interpolate, int numsubsampling, int niftitransform, int niftiintentcode, const char* niftiintentname, const char *file, char *title = 0 );

//  void            ShowProgress ();

    static double       GetFinalQuality         ( TEasyStats& stat )    { return  RoundTo ( stat.CoV () /* / 1.183 */ * 100, 0.1 ); }   // !1.20 ratio if using an inflated Mask!
    static const char*  GetQualityOpinion       ( double quality   )    { return  quality >= 120 ? "Fantastic" 
                                                                                : quality >= 100 ? "Excellent" 
                                                                                : quality >=  90 ? "Very good" 
                                                                                : quality >=  80 ? "Good" 
                                                                                :                  "Dubtious"; }

private:

    TCacheVolumes<MriType>* FromVolumesSmoothed;    // we need a set of caches, as according to the current transform, we might need different scaling alternatively

    Volume          ToVolumeSmoothed;       // we don't need caches for this one, as steps always get smaller and smaller (no coming back and forth)
    int             ToVolumeSmoothedStep;   // remember current smoothing

    TPointDouble    FromNormExt;            // used internally to normalize points
    TMatrix44       TempMat;


    void            SetMask                 ( Volume& volume, Volume& mask );
    void            RemapVolume             ( Volume& volume, RemapIntensityType remap );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
