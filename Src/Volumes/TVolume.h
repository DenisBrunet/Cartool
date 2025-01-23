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

#include    "Files.Format.nifti1.h"
#include    "Volumes.AnalyzeNifti.h"    // NiftiOrientation, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault

#include    "TTracks.h"
#include    "TArray3.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                                MriContentType;
enum                                AtomFormatType;
class                               TMatrix44;
class                               THistogram;
class                               TEasyStats;
template <class TypeD> class        TArray1;
template <class TypeD> class        TArray2;
template <class TypeD> class        TBoundingBox;
enum                                VolumeRegionsSort;
class                               TVolumeRegion;
class                               TVolumeRegions;
class                               TPoints;


//----------------------------------------------------------------------------
                                        // Grey Segmentation
enum            GreyMatterFlags
                {                       
                GreyMatterNothing           = 0x0000,
                                        // Preprocessing
                GreyMatterGaussian          = 0x0001,
                GreyMatterBiasField         = 0x0002,
                                        // Processing
                GreyMatterSlim              = 0x0010,
                GreyMatterRegular           = 0x0020,
                GreyMatterFat               = 0x0040,
                GreyMatterWhole             = 0x0080,
                                        // Postprocessing
                GreyMatterPostprocessing    = 0x0100,
                GreyMatterAsymmetric        = 0x0200,
                GreyMatterSymmetric         = 0x0400,
                                        // Solution Points Check
                GreyMatterLauraCheck        = 0x1000,
                GreyMatterLoretaCheck       = 0x2000,
                GreyMatterSinglePointCheck  = 0x4000,
                GreyMatterAllSPCheck        = GreyMatterLauraCheck | GreyMatterLoretaCheck | GreyMatterSinglePointCheck,
                };


inline  char*   GreyMatterProcessingToString  ( GreyMatterFlags gf )  {   return  ( gf & GreyMatterSlim         ) ? "Thin" 
                                                                                : ( gf & GreyMatterRegular      ) ? "Regular" 
                                                                                : ( gf & GreyMatterFat          ) ? "Fat" 
                                                                                : ( gf & GreyMatterWhole        ) ? "Whole brain" 
                                                                                :                                   "Unknown" ; }
inline  char*   GreyMatterSymmetryToString    ( GreyMatterFlags gf )  {   return  ( gf & GreyMatterAsymmetric   ) ? "Asymmetrical" 
                                                                                : ( gf & GreyMatterSymmetric    ) ? "Symmetrical" 
                                                                                :                                   "Unknown" ; }

//----------------------------------------------------------------------------
                                        // Neighborhood: used for volumes and inverse solution matrices creation
enum            NeighborhoodType
                {
                Neighbors6,             // 3D cross
                Neighbors18,            // cube without its 8 corners
                Neighbors26,            // cube

                NumNeighborhood
                };

                                        // Class to hold all parameters associated to a given neighborhood
class   NeighborhoodClass
{
public:

    NeighborhoodType        Type;               // duplicated type
    char                    Name[ 32 ];         // textual version
    int                     NumNeighbors;       //
    double                  Radius;             // maximum radius where to find neighbors
    double                  MidDistanceCut;     // mid-distance cut to separate between different neighborhood
};

extern  NeighborhoodClass   Neighborhood[ NumNeighborhood ];


//----------------------------------------------------------------------------

enum            FilterResultsType
                {
                FilterResultSigned,
                FilterResultPositive,
                FilterResultNegative,
                FilterResultAbsolute,
                };


enum            ArrayOperandType
                {
                OperandMask                 = 1,
                OperandData,
                };

enum            ArrayOperationType
                {
                OperationSet,
                OperationThreshold,
                OperationThresholdAbove,
                OperationThresholdBelow,
                OperationBinarize,
                OperationRevert,
                OperationAnd,
                OperationOr,
                OperationXor,
                OperationAdd,
                OperationAddIfNotNull,
                OperationSub,
                OperationMultiply,
                OperationDivide,
                OperationMax,
                OperationRemap,
                OperationInvert,                // 1/x
                OperationAbsolute,              // |x|
                OperationKeep,
                };


//----------------------------------------------------------------------------

enum            InterpolationType
                {
                InterpolateUnknown              = 0x0000,
                
                                        // Type
                InterpolateTruncate             = 0x0001,   // truncating coordinates
                InterpolateNearestNeighbor      = 0x0002,   // rounding coordinates
                InterpolateLinear               = 0x0004,   // fast, solid and well behaved, will not exceed existing limits
                InterpolateUniformCubicBSpline  = 0x0008,   // smooth results
                InterpolateCubicHermiteSpline   = 0x0010,   // good compromise quality/speed
                InterpolateLanczos2             = 0x0020,   // quite good
                InterpolateLanczos3             = 0x0040,   // sharp results

                                        // Some handy enums
                InterpolateTypeNoFractionMask   = InterpolateTruncate | InterpolateNearestNeighbor,
                InterpolateTypeNonOvershootMask = InterpolateTruncate | InterpolateNearestNeighbor | InterpolateLinear,

                                        // Options
                InterpolateForceNegative        = 0x0100,   // option to force results <= 0
                InterpolateForcePositive        = 0x0200,   // option to force results >= 0 - !these 2 options being mutually exclusive!
                InterpolateOptionMask           = InterpolateForceNegative | InterpolateForcePositive,
                };

                                        
inline  bool    IsNotFractionning       ( InterpolationType it )    { return  ( it & InterpolateTypeNoFractionMask  ) != 0;	}	// Doesn't create new values, i.e. masks / ROIs remain masks / ROIs
inline  bool    IsFractionning          ( InterpolationType it )    { return ! IsNotFractionning ( it );					}   // Does create new values that didn't exist in the input data
inline  bool    IsNotOvershooting       ( InterpolationType it )    { return  ( it & InterpolateTypeNonOvershootMask ) != 0;}   // Doesn't create new values above or beyond the input range
inline  bool    IsOvershooting          ( InterpolationType it )    { return ! IsNotOvershooting ( it );					}   // Does create overshooting values, outside the input range - Basically used to flag Splines

                                        // Utility that will update the force positive / negative flags
                                        // It works with any type of input...
template <class TypeD>
void            SetOvershootingOption   ( InterpolationType& it, const TypeD* data, int numdata, bool fast = false );

                                        // Caller can mitigate the effects of spline overshooting with these flags
                                        // It will prevent positive data to return negative artifacts, and the other way round
                                        // Another option would be to clip the final contents to the original data boundary
//template <class TypeD>
//inline void CheckOvershooting         ( InterpolationType& it, TypeD&  v )
inline  void    CheckOvershooting       ( InterpolationType& it, double& v )
{
if ( ( it & InterpolateForcePositive ) && v < 0
  || ( it & InterpolateForceNegative ) && v > 0 )     
    
    v   = 0;
}


//----------------------------------------------------------------------------
                                        // Region Growing
                                        // Flags
enum            RegionGrowingFlags
                {
                GrowRegion                  = 0x00001,  // not used, just to not be empty
                MoveRegion                  = 0x00002,  // region can optimally move by allowing each voxel from the mask to be revoked

                GlobalStats                 = 0x00010,  // stats on whole data
                LocalStats                  = 0x00020,  // stats only locally, around current voxel
                StatsMask                   = 0x00040,  // use all data for stats
                StatsRing                   = 0x00080,  // use only the previous iteration, like a "growth ring"

                TestNeighbor                = 0x00100,  // the simplest test: a neighbor voxel is tested for its distances to Region and Non-Region
                TestCenterToNeighbor        = 0x00200,  // more subtle:       a neighbor voxel is tested against a central voxel distance
                RegionDistanceTestAbs       = 0x01000,  // absolute Z-score distance for central voxel
                RegionDistanceTestSigned    = 0x02000,  // signed   Z-score distance for central voxel
                NonRegionDistanceTestAbs    = 0x04000,  // absolute Z-score distance for neighbor voxel
                NonRegionDistanceTestSigned = 0x08000,  // signed   Z-score distance for neighbor voxel
                                                        // wrap together the previous flags for easier use, by increasing thickness of results
                Thickness0                  = TestNeighbor         | RegionDistanceTestAbs    | NonRegionDistanceTestAbs,       // basic test
                Thickness1                  = TestCenterToNeighbor | RegionDistanceTestAbs    | NonRegionDistanceTestSigned,    // close to Thickness0, well positionned results
                Thickness2                  = TestCenterToNeighbor | RegionDistanceTestSigned | NonRegionDistanceTestSigned,    // fuller, still well defined
                Thickness3                  = TestCenterToNeighbor | RegionDistanceTestSigned | NonRegionDistanceTestAbs,       // even fuller, a bit blocky though - not sure if usefull

                RemoveLessNeighbors         = 0x10000,  // at each iteration, filter out if less neighbors
                RemoveMedian                = 0x20000,  // at each iteration, filter out if less than half neighbors
                };

                                        // Max iterations - we could settle for less, like 1000 f.ex.
constexpr int   RegionGrowingMaxIterationsDefault   = INT_MAX;

                                        // Rule of thumb, then tuning:  RegionGrowingTolerance * RegionGrowingLessNeighborsThan = 0.1 ?

                                        // alternate between Neighbors26 and Neighbors6, so the results look closer to a growing sphere
constexpr auto  SmartNeighbors                      = Neighbors26 + 1;



enum            GreyLevelsCategories
                {
                BlackMin,
                BlackMax,
                BlackMode,
                BlackW,

                GreyMin,
                GreyMax,
                GreyMode,
                GreyW,

                WhiteMin,
                WhiteMax,
                WhiteMode,
                WhiteW,

                NumGreyValues
                };


enum            CubeSidesType
                {
                SideXMin,
                SideXMax,
                SideYMin,
                SideYMax,
                SideZMin,
                SideZMax,

                NumCubeSides
                };


enum            PartialDifferencesIndex
                {
                DiffDx,
                DiffDy,
                DiffDz,

                DiffDxy,
                DiffDxz,
                DiffDyz,

                DiffDxx,
                DiffDyy,
                DiffDzz,

                NumDiffs
                };


enum            SurfacePointsFlags
                {
                SurfaceOutside              = 0x0001,
                SurfaceInside               = 0x0002,

                SurfaceSafer                = 0x0010,

                SurfaceIncludeBack          = 0x0100,
                SurfaceIntermediatePoints   = 0x0200,

                SurfaceSort                 = 0x1000,

                SurfaceShowProgress         = 0x2000,
                };


enum            BoundingSizeType
                {
                BoundingSizeGivenNoShift,   // size is provided by caller, origin shift not allowed
                BoundingSizeGiven,          // size is provided by caller, origin shift allowed
                BoundingSizeOptimal,        // size will be estimated from transformed content + margin
                BoundingSizeBiggest,        // size will be the biggest transformed box (no margin) - intended for scaling and 90 degrees rotations to fall back to original size
                };


enum            TransformAndSaveFlags 
                {
                TransformToTarget           = 0x0001,
                TransformToSource           = 0x0002,

                TransformSourceRelative     = 0x0010,
                TransformSourceAbsolute     = 0x0020,

                TransformTargetRelative     = 0x0100,
                TransformTargetAbsolute     = 0x0200,
                TransformTargetReset        = 0x0400,
                };


//----------------------------------------------------------------------------

enum            SkullStrippingType
                {
                SkullStrippingNone,

                SkullStripping1A,           // Seed -> White -> Multiple RG         - Good results, but very slow
                SkullStripping1B,           // Improvement of SkullStripping1A      - Results are as good as V1A, but faster
                SkullStripping2,            // Seed -> White -> Single RG           - Results are OK-good, speed is OK
                SkullStripping3,            // Multiple fast masks -> intersection  - Results are unpredictables, fast speed

                NumSkullStrippingTypes,
                };


extern  char    SkullStrippingNames[ NumSkullStrippingTypes ][ 16 ];

                                        // Textual option, not an enum value
constexpr char* SkullStrippingDialogDefault     = "1";


//----------------------------------------------------------------------------

constexpr char*     TVolumeWriteFileExt         = FILEEXT_MRINII " " FILEEXT_MRIAVW_HDR;


template <class TypeD>
class   TVolume :   public  TArray3<TypeD> 
{
public:

    using           TArray3::TArray3;   // TArray3 constructors
                                        // Downsampling, with non-integer steps & an origin offset
                    TVolume ( const TVolume<TypeD> &array3, double downsampling, int numsubsampling, FilterTypes filtertype, InterpolationType interpolate, const double *origin );
                                        // Downsampling, with filter FilterTypeMean / FilterTypeMax / FilterTypeMedian
                                        // give the downsampling factor (2, 3..) and the # of subsampling in each block: downsampling is the max, or 1 / 2 / 3 to limit the time it takes
                    TVolume ( const TVolume<TypeD> &array3, int downsampling, int numsubsampling, FilterTypes filtertype );
                                        // used to set a Kernel filter
                    TVolume ( double diameter, ArraySizeType sizeconstraint );


    bool            IsIncluding    ( const TVolume<TypeD> &incl, bool binary, double thresholdf, double thresholdi )  const;


    bool            IsInteger   ( int numtests = 27127 )const;  // test if data is all integers - one out of 10 for 300x300x300 volume
    bool            IsNull      ()                      const;  // test if data is all == 0
    bool            IsPositive  ()                      const;  // test if data is all >= 0
    bool            IsNegative  ()                      const;  // test if data is all <= 0
    void            HasPositiveNegative ( bool& somepos, bool& someneg )    const;


    double          GetBackgroundValue  ()                                                                  const;
    bool            GetGreyWhiteValues  ( TArray1<double> &values, bool verbose = false )                   const;
    void            GetSurfacePoints    ( TPoints* surfp[ NumCubeSides ], TypeD threshold, TPointInt step, TPointFloat center, double depth, SurfacePointsFlags flags )    const;
    TPointFloat     GetBarycenter       ( TypeD backvalue )                                                 const;
    MriContentType  GuessTypeFromContent( double backgroundvalue = 0, TBoundingBox<double>* bounding = 0 )          const;


    using  TArray3::GetValueChecked;    // seems to forward all TArray3 methods at once
    TypeD           GetValueChecked ( double x, double y, double z, InterpolationType interpolate   )   const;


    void            Filter              ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterDim1          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterDim2          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterDim3          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            UnaryOp             ( ArrayOperandType typeoperand, ArrayOperationType typeoperation, double *p1 = 0, double *p2 = 0 );
    void            BinaryOp            ( TVolume<TypeD> &operand2, ArrayOperandType typeoperand1, ArrayOperandType typeoperand2, ArrayOperationType typeoperation );
    void            ApplyMaskToData     ( TVolume<TypeD> &mask );
    void            ApplyDataToMask     ( TVolume<TypeD> &data );
    void            ClearMaskToData     ( TVolume<TypeD> &mask );
    int             GetNumNeighbors     ( int x, int y, int z, NeighborhoodType neighborhood )      const;
    int             GetNumSet           ()                                                          const;
    double          GetPercentilePosition ( double percentile, TVolume<TypeD>* mask = 0 )           const;  // wraps around THistogram - useful for single call only
//  double          SegmentCerebellum       ( Volume& brainmask, double probregion2, bool showprogress );
    bool            SegmentCSF              ( FctParams& params, bool showprogress = false ); // from a full brain
    bool            SegmentGreyMatter       ( FctParams& params, bool showprogress = false ); // from a full brain
    bool            SegmentWhiteMatter      ( FctParams& params, bool showprogress = false ); // from a full brain, detailed: 0..1
    bool            SegmentTissues          ( FctParams& params, bool showprogress = false ); // from a full brain
    bool            FilterBiasField         ( FctParams& params, TVolume<float>* corr, bool showprogress = false );

    void            LevelsToRegions         ( TVolumeRegions& gor );
    void            LevelsClustersToRegions ( TVolumeRegions& gor, int minvoxels, int maxvoxels, NeighborhoodType neighborhood, bool showprogress = false );
    void            ClustersToRegions       ( TVolumeRegions& gor, int minvoxels, int maxvoxels, NeighborhoodType neighborhood, bool showprogress = false );   // same as above, without levels
    bool            ClustersToRegions       ( FctParams& params, NeighborhoodType neighborhood, bool showprogress );
    bool            KeepRegion              ( VolumeRegionsSort sortcriterion, int minvoxels, int maxvoxels, NeighborhoodType neighborhood, double probregion2, bool showprogress = false );
    void            RegionGrowing           ( TVolumeRegion& region, RegionGrowingFlags how, FctParams& params, const Volume* mask = 0, bool showprogress = false );
    void            RegionGrowing           ( Volume& regionvol, RegionGrowingFlags how, FctParams& params, const Volume* mask = 0, bool showprogress = false );

                                        // merging subsamples: FilterTypeMax, FilterTypeMedian, FilterTypeMean, FilterTypeGaussian
    TypeD           GetAntialiasedValue ( TMatrix44& transformi, TPointDouble& pcenter, double size, InterpolationType interpolate )    const;

    void            TransformAndSave    (   TMatrix44&    		transform,
                                            TransformAndSaveFlags transformflags,
                                            FilterTypes         filtertype,         InterpolationType   interpolate,        int             numsubsampling,
                                            const TPointDouble* sourceorigin,       const TPointDouble* targetorigin,
                                            const TPointDouble& sourcevoxelsize,    const TPointDouble* targetvoxelsize,
                                            BoundingSizeType    targetsize,         const TVector3Int*  giventargetsize,
                                            const char*         targetorientation,
                                            int                 niftitransform,     int                 niftiintentcode,    const char*     niftiintentname,
                                            const char*         fileout,            TVolume<TypeD>*     dataout,            TPointDouble*   origin,
                                            const char*         title       = 0
                                         )  const;



    void            DrawLine            ( TPointFloat fromp, TPointFloat top, TypeD v );
    void            DrawCylinder        ( TPointFloat fromp, TPointFloat top, double radius, double feather, TypeD v, ArrayOperationType typeoperation );
    void            DrawSphere          ( TPointFloat center, double radius, double feather, TypeD v, ArrayOperationType typeoperation );
    void            DrawCube            ( TPointFloat center, double radius, TypeD v, FilterTypes filtertype = FilterTypeMean );
    void            ErasePlane          ( TPointFloat center, TPointFloat normal, double feather );
    void            BlurrSphere         ( TPointFloat center, double radius );


    void            GetGradient         ( int x, int y, int z, TPointFloat& g, int d = 1 )          const;
    void            GetGradientSmoothed ( int x, int y, int z, TPointFloat& g, int d = 1 )          const;
    void            GetDifferences      ( int x, int y, int z, double *diff, int d = 1 )            const;  // Gradient + Hessian
    void            GetPrincipalGradient( int x, int y, int z, TPointFloat &g, int d, double &C )   const;
    TVector3Float   GetGradientToPoint  ( TPointFloat& p, const TVector3Float& dir, double threshold )  const;


    void            ReadFile            ( char* file, TStrings* tracknames = 0  );   // bin dlf freq seg
    void            WriteFile           (   const char*             file,               // bin nii hdr
                                            const TPointDouble*     origin          = 0,
                                            const TVector3Double*   voxelsize       = 0,
                                            const TVector3Double*   realsize        = 0,
                                            const char*             orientation     = NiftiOrientation,
                                            int                     niftitransform  = NiftiTransformDefault,
                                            int                     niftiintentcode = NiftiIntentCodeDefault,   
                                            const char*             niftiintentname = NiftiIntentNameDefault,
                                            AtomFormatType          atomformattype  = UnknownAtomFormat
                                        )   const;


    TVolume<TypeD>& operator    =       ( const TVolumeRegion&  op2 );
    TVolume<TypeD>& operator    =       ( const TVolumeRegions& op2 );


    using  TArray3::operator    =;
    using  TArray3::operator    [];
    using  TArray3::operator    ();


    TVolume<TypeD>& operator    |=      ( const TVolumeRegion&  op2 );
    TVolume<TypeD>& operator    |=      ( const TVolume<TypeD> &op2 );          // bool in and out
    TVolume<TypeD>& operator    &=      ( const TVolume<TypeD> &op2 );          // bool in and out
    TVolume<TypeD>& operator    +=      ( const TVolume<TypeD>& op2 );
    TVolume<TypeD>& operator    +=      ( double op2 );
    TVolume<TypeD>& operator    /=      ( double op2 );
    TVolume<TypeD>& operator    /=      ( const TVolume<TypeD>& op2 );
    TVolume<TypeD>& operator    *=      ( double op2 );
    TVolume<TypeD>  operator    |       ( const TVolume<TypeD> &op2 )   const;  // bool in and out
    TVolume<TypeD>  operator    &       ( const TVolume<TypeD> &op2 )   const;  // bool in and out
    TVolume<TypeD>  operator    +       ( const TVolume<TypeD> &op2 )   const;


protected:

    void            FilterLinear        ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterStat          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterFastGaussian  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterAnisoGaussian ( FctParams& params, bool showprogress = false );
    void            FilterRank          ( FilterTypes filtertype, FctParams& params );

    void            FilterThreshold         ( FilterTypes filtertype, FctParams& params );
    void            FilterBinarize          ( FctParams& params );
    void            FilterRevert            ( FctParams& params );
    void            FilterIntensityScale    ( FilterTypes filtertype, FctParams& params );
    void            FilterIntensityInvert   ( FctParams& params );
    void            FilterIntensityAdd      ( FilterTypes filtertype, FctParams& params );
    void            FilterIntensityOpposite ( FctParams& params );
    void            FilterIntensityAbsolute ( FctParams& params );
    void            FilterIntensityRemap    ( FctParams& params );
    void            FilterIntensityNorm     ( FctParams& params );
    void            FilterKeepValue         ( FctParams& params );
    void            FilterToMask            ( FctParams& params, bool showprogress = false );
    void            FilterSymmetrize        ( FctParams& params );

    void            FilterHistoEqual    ( FctParams& params, bool showprogress = false );
    void            FilterHistoEquBrain ( FctParams& params, bool showprogress = false );
    void            FilterHistoCompact  ( FctParams& params, bool showprogress = false );

    void            FilterDilateErode   ( FctParams& params, bool dilate, bool showprogress );
    void            FilterErode         ( FctParams& params, bool showprogress = false );
    void            FilterDilate        ( FctParams& params, bool showprogress = false );
    void            FilterClose         ( FctParams& params, bool showprogress = false );
    void            FilterOpen          ( FctParams& params, bool showprogress = false );
    void            FilterMorphGradient ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterThinning      ( FctParams& params, bool showprogress );
    void            FilterWaterfall     ( FilterTypes filtertype, FctParams& params, bool showprogress );

    void            FilterNeighbors     ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterRelax         ( FctParams& params, bool showprogress = false );
//  void            StatNeighbors       ( int x, int y, int z, NeighborhoodType neighborhood, TEasyStats &stat );
//  void            SetNeighbors        ( int x, int y, int z, TypeD v, NeighborhoodType neighborhood );
    void            FilterShapeFeature  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );

    void            FilterPartDeriv     ( FilterTypes filtertype, FctParams& params, TArray3< TVector3Float > *vectres = 0, bool showprogress = false );

    void            FilterPeeling       ( FctParams& params, bool showprogress = false );
    void            FilterPeelingRadius ( TPointFloat &p, TPointFloat &center, double &backthreshold, TVolume<TypeD> &temp );

    double          EstimateVoxelSize   ( double filevoxelsize );
    bool            SkullStripping      ( FctParams& params, bool showprogress = false ); // from a full head
    bool            SkullStripping_1A   ( FctParams& params, bool showprogress = false );
    bool            SkullStripping_1B   ( FctParams& params, bool showprogress = false );
    bool            SkullStripping_2    ( FctParams& params, bool showprogress = false );
    bool            SkullStripping_3    ( FctParams& params, bool showprogress = false );
    bool            BrainstemRemoval    ( FctParams& params, bool showprogress = false );

//  void            FilterResize        ( FctParams& params, bool showprogress = false );
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // Redefinitions - until we sort out all includes:
#define         UpdateApplication       { if ( IsMainThread () ) CartoolObjects.CartoolApplication->PumpWaitingMessages (); }


//----------------------------------------------------------------------------
template <class TypeD>
inline  void    SetOvershootingOption   ( InterpolationType& it, const TypeD* data, int numdata, bool fast )
{
                                        // Force resetting these flags first
ResetFlags ( it, InterpolateOptionMask );

                                        // We might not be concerned with the overshooting...
if ( ! IsOvershooting ( it ) || data == 0 || numdata <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scanning whole / part of data
TDownsampling       down ( numdata, fast ? 100000 : numdata );

bool                hasnegative     = false;
bool                haspositive     = false;


for ( int i = down.From; i <= down.To; i += down.Step ) {
                                        // !using a strict test, so to avoid both flags on with 0's!
    hasnegative    |= data[ i ] < 0;
    haspositive    |= data[ i ] > 0;
                                        // early exit?
    if ( hasnegative && haspositive )
        break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Deciding according to content
if      (   hasnegative &&   haspositive )  ;                                           // signed data, overshooting can not really be controlled
else if (   hasnegative && ! haspositive )  SetFlags ( it, InterpolateForceNegative );  // negative data, will prevent returning positive results
else if ( ! hasnegative &&   haspositive )  SetFlags ( it, InterpolateForcePositive );  // positive data, will prevent returning negative results
//elseif( ! hasnegative && ! haspositive )  ;                                           // all null data, we don't care anyway, splines will not create non-null values out of 0's
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Downsampling, with non-integer ratio & an origin offset
template <class TypeD>
        TVolume<TypeD>::TVolume     (   const TVolume<TypeD>&       array3, 
                                        double                      downsampling,   int                 numsubsampling, 
                                        FilterTypes                 filtertype,     InterpolationType   interpolate, 
                                        const double*               origin 
                                    )
{
Dim1            = RoundAbove ( ( array3.GetDim1 () - origin[ 0 ] ) / downsampling );
Dim2            = RoundAbove ( ( array3.GetDim2 () - origin[ 1 ] ) / downsampling );
Dim3            = RoundAbove ( ( array3.GetDim3 () - origin[ 2 ] ) / downsampling );
LinearDim       = Dim1 * Dim2 * Dim3;
                                        // always use Virtual Memory - no, for debugging
Array           = (TypeD *) AllocateMemory ( MemorySize () );

                                        // do a subsampling
double              substep         = downsampling / numsubsampling;
int                 mediansize      = numsubsampling;
TEasyStats          stat ( filtertype == FilterTypeMedian ? Cube ( mediansize ) : 0 );
int                 count;
double              getvalue;
double              v;

SetOvershootingOption ( interpolate, Array, LinearDim );

//DBGV3 ( downsampling, numsubsampling, mediansize, "downsampling  numsubsampling  mediansize" );


for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ ) {

    stat.Reset ();
    count   = 0;
    v       = 0;

                                        // scan kernel
    for ( double xdi = 0, xd = ( x - 0.25 ) * downsampling + origin[ 0 ]; xdi < numsubsampling; xdi++, xd += substep )
    for ( double ydi = 0, yd = ( y - 0.25 ) * downsampling + origin[ 1 ]; ydi < numsubsampling; ydi++, yd += substep )
    for ( double zdi = 0, zd = ( z - 0.25 ) * downsampling + origin[ 2 ]; zdi < numsubsampling; zdi++, zd += substep ) {

        getvalue    = array3.GetValueChecked ( xd, yd, zd, interpolate );

        if      ( filtertype == FilterTypeMedian )  stat.Add ( getvalue );
        else if ( filtertype == FilterTypeMax   )   Maxed ( v, getvalue );
        else if ( filtertype == FilterTypeMean  )   { v    += getvalue; count++; }
        } // subsampled block


    if      ( filtertype == FilterTypeMedian )  Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) stat.Median ();
    else if ( filtertype == FilterTypeMax   )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) v;
    else if ( filtertype == FilterTypeMean  )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) ( count ? v / count : 0 );
    } // for z
}


//----------------------------------------------------------------------------
                                        // Advanced downsampling, with filter FilterTypeMean / FilterTypeMax / FilterTypeMedian
template <class TypeD>
        TVolume<TypeD>::TVolume     (   const TVolume<TypeD>&       array3, 
                                        int                         downsampling,   int             numsubsampling, 
                                        FilterTypes                 filtertype
                                    )
{
Dim1            = ( array3.GetDim1 () + downsampling - 1 ) / downsampling;  // round above size, missing values will be 0
Dim2            = ( array3.GetDim2 () + downsampling - 1 ) / downsampling;
Dim3            = ( array3.GetDim3 () + downsampling - 1 ) / downsampling;
LinearDim       = Dim1 * Dim2 * Dim3;
                                        // always use Virtual Memory - no, for debugging
Array           = (TypeD *) AllocateMemory ( MemorySize () );

if ( downsampling <= 1 ) {
    CopyMemoryFrom ( array3.Array );
    return;
    }
                                        // either use the provided subsampling step, or set to a max of 27 (3*3*3) subsampling values
                                        // truncate the ratio, to have steps that repeat equally across each blocks (not like 101 101 101)
int                 substep         = AtLeast ( 1, downsampling / ( numsubsampling > 0 ? numsubsampling : 3 ) );
int                 mediansize      = downsampling / substep;
TEasyStats          stat ( filtertype == FilterTypeMedian ? Cube ( mediansize ) : 0 );
int                 count;
double              getvalue;
double              v;

//SetOvershootingOption ( interpolate, Array, LinearDim );

//DBGV3 ( downsampling, substep, mediansize, "downsampling  substep  mediansize" );


for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ ) {

    stat.Reset ();
    count   = 0;
    v       = 0;

                                        // scan kernel
    for ( int xdi = 0, xd = x * downsampling; xdi < downsampling; xdi += substep, xd += substep )
    for ( int ydi = 0, yd = y * downsampling; ydi < downsampling; ydi += substep, yd += substep )
    for ( int zdi = 0, zd = z * downsampling; zdi < downsampling; zdi += substep, zd += substep ) {

        getvalue    = array3.GetValueChecked ( xd, yd, zd );

        if      ( filtertype == FilterTypeMedian )  stat.Add ( getvalue );
        else if ( filtertype == FilterTypeMax   )   Maxed ( v, getvalue );
        else if ( filtertype == FilterTypeMean  )   { v    += getvalue; count++; }
        } // subsampled block


    if      ( filtertype == FilterTypeMedian )  Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) stat.Median ();
    else if ( filtertype == FilterTypeMax   )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) v;
    else if ( filtertype == FilterTypeMean  )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) ( count ? v / count : 0 );
    } // for z
}


//----------------------------------------------------------------------------
                                        // Relationship of diameter to # of neighbors:
                                        //      2.00 ->  6 neighbors
                                        //      2.83 -> 18 neighbors
                                        //      3.47 -> 26 neighbors
template <class TypeD>
        TVolume<TypeD>::TVolume ( double diameter, ArraySizeType sizeconstraint )
{
int                 size            = DiameterToKernelSize ( diameter, sizeconstraint );

Dim1            = size;
Dim2            = size;
Dim3            = size;


LinearDim       = Dim1 * Dim2 * Dim3;
Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
bool    TVolume<TypeD>::IsIncluding ( const TVolume<TypeD> &incl, bool binary, double thresholdf, double thresholdi ) const
{
TPointInt           MriSize ( min ( GetDim ( 0 ), incl.GetDim ( 0 ) ),
                              min ( GetDim ( 1 ), incl.GetDim ( 1 ) ),
                              min ( GetDim ( 2 ), incl.GetDim ( 2 ) ) );

TEasyStats          bfratio;
bool                hasfull;
bool                hasincl;
double              d1;
double              d2;


for ( int x = 1; x < MriSize.X; x++ )
for ( int y = 1; y < MriSize.Y; y++ )
for ( int z = 1; z < MriSize.Z; z++ ) {

                                                                 // must also test neighbors, otherwise deltas can be super big
    hasincl     =     incl ( x, y, z ) >= thresholdi && ( binary ||     incl ( x - 1, y, z ) >= thresholdi &&     incl ( x, y - 1, z ) >= thresholdi &&     incl ( x, y, z - 1 ) >= thresholdi );
    hasfull     = GetValue ( x, y, z ) >= thresholdf && ( binary || GetValue ( x - 1, y, z ) >= thresholdf && GetValue ( x, y - 1, z ) >= thresholdf && GetValue ( x, y, z - 1 ) >= thresholdf );

    if ( binary ) {

        if ( hasincl && ! hasfull )
            bfratio.Add ( 1 );
        } // binary

    else if ( hasincl && hasfull ) {
                                        // look at the ratio of voxel (signed) differences
        d1  = incl     ( x, y, z ) - incl     ( x - 1, y, z );
        d2  = GetValue ( x, y, z ) - GetValue ( x - 1, y, z );
        if ( d1 && d2 )     bfratio.Add ( d1 / d2 );

        d1  = incl     ( x, y, z ) - incl     ( x, y - 1, z );
        d2  = GetValue ( x, y, z ) - GetValue ( x, y - 1, z );
        if ( d1 && d2 )     bfratio.Add ( d1 / d2 );

        d1  = incl     ( x, y, z ) - incl     ( x, y, z - 1 );
        d2  = GetValue ( x, y, z ) - GetValue ( x, y, z - 1 );
        if ( d1 && d2 )     bfratio.Add ( d1 / d2 );
        } // if thresholds

    } // for z


//DBGV2 ( thresholdf, thresholdi, "thresholdf thresholdi" );
//if ( binary )
//    DBGV2 ( bfratio.GetNumItems (), (double) bfratio.GetNumItems () / ( incl.GetNumSet () + 1 ) * 100, "#binary diff: abs %?" )
//else
//    DBGV4 ( bfratio.Average (), bfratio.SD (), bfratio.CoefficientOfVariation (), fabs ( bfratio.CoefficientOfVariation () ) < 5.00, "ratio: Avg SD Coeff -> Included?" );

                                            // less than 5% outside (good: 1%, bad: 33%)
bool                included        = binary ? (double) bfratio.GetNumItems () / ( incl.GetNumSet () + 1 ) < 0.05
                                            // Aligned, though potentially rescaled: 0.77..1.73
                                            // Misaligned: 11..500
                                             : fabs ( bfratio.CoefficientOfVariation () ) < 5.00;

return  included;
}


//----------------------------------------------------------------------------
                                        // result is bool in all cases
template <class TypeD>
TVolume<TypeD>  TVolume<TypeD>::operator| ( const TVolume<TypeD>& op2 )     const
{
int     mindim1 = min ( Dim1, op2.Dim1 );
int     mindim2 = min ( Dim2, op2.Dim2 );
int     mindim3 = min ( Dim3, op2.Dim3 );

TVolume<TypeD>  temp ( mindim1, mindim2, mindim3 );


if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        temp.Array[ i ] = (TypeD) ( (bool) Array[ i ] || (bool) op2.Array[ i ] );
else {

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        temp.Array[ i ] = (TypeD) ( (bool) Array[ i ] || (bool) op2 ( d1, d2, d3 ) );
    }


return  temp;
}


template <class TypeD>
TVolume<TypeD>  TVolume<TypeD>::operator& ( const TVolume<TypeD>& op2 )     const
{
int     mindim1 = min ( Dim1, op2.Dim1 );
int     mindim2 = min ( Dim2, op2.Dim2 );
int     mindim3 = min ( Dim3, op2.Dim3 );

TVolume<TypeD>  temp ( mindim1, mindim2, mindim3 );


if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        temp.Array[ i ] = (TypeD) ( (bool) Array[ i ] && (bool) op2.Array[ i ] );
else {

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        temp.Array[ i ] = (TypeD) ( (bool) Array[ i ] && (bool) op2 ( d1, d2, d3 ) );
    }


return  temp;
}


template <class TypeD>
TVolume<TypeD>  TVolume<TypeD>::operator+ ( const TVolume<TypeD>& op2 )     const
{
int     mindim1 = min ( Dim1, op2.Dim1 );
int     mindim2 = min ( Dim2, op2.Dim2 );
int     mindim3 = min ( Dim3, op2.Dim3 );

TVolume<TypeD>  temp ( mindim1, mindim2, mindim3 );


if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        temp.Array[ i ] = (TypeD) ( Array[ i ] + op2.Array[ i ] );
else {

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        temp.Array[ i ] = (TypeD) ( Array[ i ] + op2 ( d1, d2, d3 ) );
    }


return  temp;
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator+= ( const TVolume<TypeD>& op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        Array[ i ] += op2.Array[ i ];
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );
    int     mindim3 = min ( Dim3, op2.Dim3 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        Array[ i ] += op2 ( d1, d2, d3 );
    }

return  *this;
}

                                        // result is bool in all cases
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator|= ( const TVolume<TypeD>& op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        Array[ i ]  = (TypeD) ( (bool) Array[ i ] || (bool) op2.Array[ i ] );
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );
    int     mindim3 = min ( Dim3, op2.Dim3 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        Array[ i ]  = (TypeD) ( (bool) Array[ i ] || (bool) op2 ( d1, d2, d3 ) );
    }

return  *this;
}

                                        // result is bool in all cases
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator&= ( const TVolume<TypeD>& op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        Array[ i ]  = (TypeD) ( (bool) Array[ i ] && (bool) op2.Array[ i ] );
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );
    int     mindim3 = min ( Dim3, op2.Dim3 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        Array[ i ]  = (TypeD) ( (bool) Array[ i ] && (bool) op2 ( d1, d2, d3 ) );
    }

return  *this;
}


template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator/= ( const TVolume<TypeD>& op2 )
{
if ( LinearDim == op2.LinearDim )       // same size -> simplified loop

    for ( int i = 0; i < LinearDim; i++ )

        Array[ i ] /= NonNull ( op2.Array[ i ] );
else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );
    int     mindim3 = min ( Dim3, op2.Dim3 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )

        Array[ i ] /= NonNull ( op2 ( d1, d2, d3 ) );
    }

return  *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator+= ( double op2 )
{
if ( op2 == 0 )
    return  *this;

UnaryOp ( OperandData, OperationAdd, &op2 );

return  *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator/= ( double op2 )
{
if ( op2 == 0 || op2 == 1 )
    return  *this;

UnaryOp ( OperandData, OperationDivide, &op2 );

return  *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator*= ( double op2 )
{
if ( op2 == 1 )
    return  *this;

UnaryOp ( OperandData, OperationMultiply, &op2 );

return  *this;
}


//----------------------------------------------------------------------------
/*
                                        // Filter is totally working, just updating the current views is the problem
void    Volume::FilterResize ( FctParams& params, bool /*showprogress* / )
{
if ( IsNotAllocated () )
    return;

int                 newdim1         = params ( FilterParamResizeDim1 );
int                 newdim2         = params ( FilterParamResizeDim2 );
int                 newdim3         = params ( FilterParamResizeDim3 );

Volume              copy ( *this );

Resize ( newdim1, newdim2, newdim3 );

Insert ( copy );
}
*/

//----------------------------------------------------------------------------
                                        // using just positions, not value of voxels
template <class TypeD>
TPointFloat     TVolume<TypeD>::GetBarycenter ( TypeD backvalue )   const
{
TPointDouble        c;
int                 n               = 0;


for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ )

    if ( GetValue ( x, y, z ) > backvalue ) {
        c      += TPointDouble ( x, y, z );
        n++;
        }

c      /= NonNull ( n );

return  c;
}


//----------------------------------------------------------------------------
                                        // Returns a double value, as we might need a cut between 0 and 1 f.ex.
template <class TypeD>
double  TVolume<TypeD>::GetBackgroundValue ()   const
{
THistogram          H;

return  H.ComputeBackground ( *this );
}


//----------------------------------------------------------------------------
                                        // Fitting a set of Gaussians in the histogram, then work with them as a model
                                        // Always search for 4 Gaussians:
                                        // - 1:    CSF
                                        // - 2, 3: Grey low + grey high, used to model the left skewness of the Grey distribution (instead of directly modelling the skewness)
                                        // - 4:    White (skewness not explicitly modelled here)
                                        // So the low cut for grey matter is between Gaussian 1 and 2, high cut between 2/3 and 4
template <class TypeD>
bool    TVolume<TypeD>::GetGreyWhiteValues      (   TArray1<double>&    values, 
                                                    bool                verbose 
                                                )   const
{
                                        // check for allocation
if ( values.GetDim1 () < NumGreyValues )
    values.Resize ( NumGreyValues );

                                        // set default, probable values
double              backvalue       = GetBackgroundValue ();
double              maxvalue        = GetMaxValue () * 0.80;    // cut out the tail - see below for better one

                                        // default values, used in case of analysis error
values[ BlackMin ]  = backvalue;
values[ BlackMax ]  = backvalue + ( maxvalue - backvalue ) * 0.25;
values[ GreyMin  ]  = values[ BlackMax ];
values[ GreyMax  ]  = backvalue + ( maxvalue - backvalue ) * 0.625;
values[ WhiteMin ]  = values[ GreyMax ];
values[ WhiteMax ]  = maxvalue;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute downsampled stats + using itself as a mask to save some space
#define             MaxGreyWhiteHistoSize   30000

TEasyStats          stats ( *this, this, true, true, MaxGreyWhiteHistoSize );

                                        // Removing before background and after long tail
stats.KeepWithin    ( backvalue, stats.Quantile ( 0.99 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Force fitting either with 3 or 4 Gaussians, only
enum                {
                    criterionloglikelihood  = 0,
                    criterionloglikelihoodW,
                    numcriterion
                    };

enum                {
                    gausscenter     = 0,
                    gaussheight,
                    gausswidthl,
                    gausswidthr,
                    gaussnum,
                    numgaussvars,
                    };


int                 mingauss        = 4; // 3;  // Forcing 4, using 2 Gaussians for the Grey matter
int                 maxgauss        = 4; // 3;
int                 rangegauss      = maxgauss - mingauss + 1;
TArray2<double>     criterion ( maxgauss - mingauss + 1, numcriterion );
TVolume<double>     gaussians ( maxgauss - mingauss + 1, maxgauss, numgaussvars );  // to store the gaussians parameters, with left and right SD
double              loglikelyhood;
TTracks<double>     mixg;
TTracks<double>     gaussmix;



for ( int numgaussi = 0, numgauss = mingauss; numgaussi < rangegauss; numgauss++, numgaussi++ ) {

                                        // EM Mixture of Gaussian
    loglikelyhood   = stats.GetGaussianMixture  (   numgauss,
                                                    SingleFloatEpsilon,       GaussianMixtureFaster, 
                                                    mixg,       
                                                    &gaussmix 
                                                );


//    if ( badresults ) {
//
//        if ( verbose )
//            ShowMessage ( "Failed to detect any grey or white grey levels.\nMaybe check this is not a binary mask of some sort...?", "Grey / White Matter", ShowMessageWarning );
//
//        return  false;
//        }

                                        // saving results
    gaussians ( numgaussi, 0, gaussnum )    = numgauss;

    for ( int i = 0; i < numgauss; i++ ) {

        gaussians ( numgaussi, i, gausscenter ) = mixg ( i, FunctionCenter );
        gaussians ( numgaussi, i, gaussheight ) = mixg ( i, FunctionHeight );
        gaussians ( numgaussi, i, gausswidthl ) = mixg ( i, FunctionWidth  );
        gaussians ( numgaussi, i, gausswidthr ) = mixg ( i, FunctionWidth  );
        }

                                        // saving to file
    //TExportTracks       expfile;
    //StringCopy  ( expfile.Filename, "E:\\Data\\BGW Histogram.EMGaussianMixture.", IntegerToString ( numgauss, 2 ), "." FILEEXT_EEGSEF );
    //CheckNoOverwrite ( expfile.Filename );
    //expfile.SetAtomType ( AtomTypeScalar );
    //expfile.NumTracks           = gaussmix.GetDim1 ();
    //expfile.NumTime             = gaussmix.GetDim2 ();
    //expfile.SamplingFrequency   = gaussmix.Index2.IndexRatio;                                           // will "rescale" to actual variable values
    //expfile.DateTime            = TDateTime ( 0, 0, 0, 0, 0, 0, gaussmix.Index2.IndexMin * 1000, 0 );   // will set the offset correctly
    //expfile.Write ( gaussmix, Transposed );
    //expfile.Filename.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // give a penalty for a higher # of gaussians
    criterion ( numgaussi, criterionloglikelihood   )   = loglikelyhood;
    criterion ( numgaussi, criterionloglikelihoodW  )   = numgauss * loglikelyhood;

//    DBGV3 ( numgauss, criterion ( numgaussi, criterionloglikelihood ), criterion ( numgaussi, criterionloglikelihoodW ), "#gauss  criterion: logl logl*#" );
    } // for numgauss


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Looking for the optimal number of Gaussians
                                        // Note that Gaussian Mixture looks how each Gaussian is close to the original curve, rather than the sum of Gaussian
int                 bestnumgauss;
int                 bestnumgaussi;


bestnumgaussi   = 0;

for ( int numgaussi = 1; numgaussi < rangegauss; numgaussi++ ) {

    if ( criterion ( numgaussi, criterionloglikelihood  ) > criterion ( bestnumgaussi, criterionloglikelihood  ) )

        bestnumgaussi   = numgaussi;
    }


bestnumgauss    = mingauss + bestnumgaussi;

//DBGV ( bestnumgauss, "Best number of Gaussians" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort the gaussians according to a criterion
TArray2<double>     bestgauss ( bestnumgauss, 2 );


for ( int i = 0; i < bestnumgauss; i++ ) {
                                        // height has already been set to 0 if gaussian is not relevant
    if ( gaussians ( bestnumgaussi, i, gaussheight ) > 0 ) {

        bestgauss ( i, 0 )  = i;
                                        // criterion is simply grey level center
        bestgauss ( i, 1 )  = gaussians ( bestnumgaussi, i, gausscenter );
        }
    else
        bestgauss ( i, 0 )  = bestgauss ( i, 1 )  = -1; // reset values

//    DBGV2 ( i, bestgauss ( i, 1 ), "Gauss#  criterion" );
    }

                                        // take the last 2 good gaussians, the bad ones will also land at the end
bestgauss.SortRows ( 1, Descending );

                                        // pick the gaussians with the highest criterion (center here)
int                 gausswhitei     = bestgauss ( 0, 0 );

                                        // specialize into grey low & grey high
int                 gaussgreylowi;
int                 gaussgreyhighi;

if ( bestgauss ( 1, 0 ) >= 0 
  && bestgauss ( 2, 0 ) >= 0
//&& bestnumgauss       >  3
//&& gaussians ( bestnumgaussi, bestgauss ( 2, 0 ), gaussheight ) > gaussians ( bestnumgaussi, bestgauss ( 1, 0 ), gaussheight ) * 0.50 
   ) {
    gaussgreylowi   = bestgauss ( 2, 0 );
    gaussgreyhighi  = bestgauss ( 1, 0 );
    }
else                                    // low and high are the same
    gaussgreylowi   =
    gaussgreyhighi  = bestgauss ( 1, 0 );


int                 gaussblacki     = bestnumgauss == 3 ? bestgauss ( 2, 0 )
                                                        : bestgauss ( 3, 0 ) != -1 ? bestgauss ( 3, 0 ) : bestgauss ( 2, 0 );   // first or second gaussians


                                        // check we have at least 2 valid gaussians!
if ( gaussgreylowi  == -1 
  || gaussgreyhighi == -1 
  || gausswhitei    == -1 ) {

    if ( verbose )
        ShowMessage ( "Failed to detect the grey and white grey levels,\nthis volume has some super weird grey level distribution.\nMaybe you can preprocess it beforehand?", "Grey / White Matter", ShowMessageWarning );

    return  false;
    }

                                        // finally, check the order of the gaussians
//if ( gaussians ( bestnumgaussi, gausswhitei, gausscenter ) < gaussians ( bestnumgaussi, gaussgreyi, gausscenter ) )
//    Permutate ( gausswhitei, gaussgreyi );


//DBGV4 ( gaussblacki, gaussgreylowi, gaussgreyhighi, gausswhitei, "black / grey low / grey high / white  indexes" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, the 2 or 3 gaussians are center-ordered
                                        // retrieve gaussians parameters

double              blackcenter     = gaussblacki == -1 ? 0 : gaussians ( bestnumgaussi, gaussblacki, gausscenter );
//double            blackheight     = gaussblacki == -1 ? 0 : gaussians ( bestnumgaussi, gaussblacki, gaussheight );
//double            blacksdl        = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussblacki, gausswidthl ) );
double              blacksdr        = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussblacki, gausswidthr ) );


double              greylowcenter   = gaussians ( bestnumgaussi, gaussgreylowi, gausscenter );
double              greylowheight   = gaussians ( bestnumgaussi, gaussgreylowi, gaussheight );
double              greylowsdl      = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussgreylowi, gausswidthl ) );
double              greylowsdr      = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussgreylowi, gausswidthr ) );

double              greyhighcenter  = gaussians ( bestnumgaussi, gaussgreyhighi, gausscenter );
double              greyhighheight  = gaussians ( bestnumgaussi, gaussgreyhighi, gaussheight );
double              greyhighsdl     = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussgreyhighi, gausswidthl ) );
double              greyhighsdr     = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gaussgreyhighi, gausswidthr ) );
                                        // average grey center
double              greycenter          = ( greylowcenter  * greylowsdr  * greylowheight
                                          + greyhighcenter * greyhighsdl * greyhighheight )
                                        / (                  greylowsdr  * greylowheight 
                                          +                  greyhighsdl * greyhighheight );


double              whitecenter     = gaussians ( bestnumgaussi, gausswhitei, gausscenter );
double              whiteheight     = gaussians ( bestnumgaussi, gausswhitei, gaussheight );
double              whitesdl        = GaussianWidthToSigma ( gaussians ( bestnumgaussi, gausswhitei, gausswidthl ) );


//DBGV3 ( blackcenter, blackheight, blacksdr, "black: center height SDr" );
//DBGV4 ( greylowcenter, greylowheight, greylowsdl, greylowsdr, "greylow: center height SDl SDr" );
//DBGV4 ( greyhighcenter, greyhighheight, greyhighsdl, greyhighsdr, "greyhigh: center height SDl SDr" );
//DBGV3 ( whitecenter, whiteheight, whitesdl, "white: center height SDr" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Intersections between Gaussians
double              xblackgrey1;
double              xblackgrey2;

if ( gaussblacki != -1 )
                                        // Looking at normalized Gaussians
    GaussiansIntersections  (   blackcenter,
                                gaussians ( bestnumgaussi, gaussblacki, gausswidthr ),     // right of black
                                1, // blackheight,
                                greylowcenter,
                                gaussians ( bestnumgaussi, gaussgreylowi, gausswidthl ),   // left of grey
                                1, // greylowheight,
                                xblackgrey1, xblackgrey2 
                            );


double              xgreywhite1;
double              xgreywhite2;
                                        // Looking at normalized Gaussians
GaussiansIntersections  (   greylowcenter,
                            gaussians ( bestnumgaussi, gaussgreylowi, gausswidthr ),        // !right of LOW grey!
                            1, // greylowheight,
                            whitecenter,
                            gaussians ( bestnumgaussi, gausswhitei, gausswidthl ),          // left of white
                            1, // whiteheight,
                            xgreywhite1, xgreywhite2
                        );


//DBGV3 ( numsolblackgrey, xblackgrey1, xblackgrey2, "Black-Grey intersection" );
//DBGV3 ( numsolgreywhite, xgreywhite1, xgreywhite2, "Grey-White intersection" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Estimates for the black-grey intersection

                                        // Left of Grey
double              greyminus           = greylowcenter - 0.95 * greylowsdl;

                                        // Right of Black
double              blackplus           = blackcenter + 1.25 * blacksdr;

                                        // Intersection between normalized Gaussians
double              gaussianintersectbg = IsInsideLimits ( xblackgrey1, blackcenter, greycenter )   ?   xblackgrey1 * 1.05
                                        : IsInsideLimits ( xblackgrey2, blackcenter, greycenter )   ?   xblackgrey2 * 1.05
                                        :                                                               0;
                                        // Weighted means of black and grey peaks
double              blackgreysummits    = ( blackcenter   * 1.0 * gaussians ( bestnumgaussi, gaussgreylowi, gaussheight )
                                          + greylowcenter * 5.0 * gaussians ( bestnumgaussi, gaussblacki,   gaussheight ) )
                                        / (                 1.0 * gaussians ( bestnumgaussi, gaussgreylowi, gaussheight ) 
                                                          + 5.0 * gaussians ( bestnumgaussi, gaussblacki,   gaussheight ) );


TEasyStats          statsbg;

statsbg.Add ( greyminus );
statsbg.Add ( blackplus );
if ( gaussianintersectbg != 0 )    
    statsbg.Add ( gaussianintersectbg );
statsbg.Add ( blackgreysummits );

                                        // Arithmetic mean is doing a fine job
values[ GreyMin ]   = statsbg.Mean ();


//DBGV5 ( greyminus, blackplus, gaussianintersectbg, blackgreysummits, values[ GreyMin ], "greyminus, blackplus, gaussianintersect, blackgreysummits -> grey min" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Estimates for the grey-white intersection

                                        // Left of White
double              whiteminus          = whitecenter - 1.60 * whitesdl;

                                        // Right of Grey
double              greyplus            = greyhighcenter + 1.0 * greyhighsdr;

                                        // Intersection between normalized Gaussians
double              gaussianintersectgw = IsInsideLimits ( xgreywhite1, greycenter, whitecenter )   ?   xgreywhite1 * 1.04
                                        : IsInsideLimits ( xgreywhite2, greycenter, whitecenter )   ?   xgreywhite2 * 1.04
                                        :                                                               0;
                                        // Weighted means of grey and white peaks
double              greysummitw         = 1.25 * whiteheight;
double              whitesummitw        = 1.00 * greyhighheight;
double              greywhitesummits    = ( greyhighcenter * greysummitw + whitecenter * whitesummitw ) / ( greysummitw + whitesummitw );


TEasyStats          statsgw;

statsgw.Add ( whiteminus );
statsgw.Add ( greyplus );
if ( gaussianintersectgw )    
    statsgw.Add ( gaussianintersectgw );
statsgw.Add ( greywhitesummits );

                                        // Arithmetic mean is doing the  job
values[ GreyMax ]   = statsgw.Mean ();


//DBGV5 ( whiteminus, greyplus, gaussianintersectgw, greywhitesummits, values[ GreyMax ], "whiteminus, greyplus, gaussianintersectgw, greywhitesummits -> grey max" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Copying spreading
values[ BlackW   ]  = gaussians ( bestnumgaussi, gaussblacki,   gausswidthr );
values[ GreyW    ]  = gaussians ( bestnumgaussi, gaussgreylowi, gausswidthr );  // ?
values[ WhiteW   ]  = gaussians ( bestnumgaussi, gausswhitei,   gausswidthl );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling in the missing values
values[ BlackMin ]  = backvalue;
values[ BlackMax ]  = values[ GreyMin ];

values[ WhiteMin ]  = values[ GreyMax ];
values[ WhiteMax ]  = GetMaxValue ();

values[ BlackMode ] = blackcenter;
values[ GreyMode  ] = greycenter;
values[ WhiteMode ] = whitecenter;

//DBGV4  ( values[ BlackMin ], values[ GreyMin ], values[ GreyMax ], values[ WhiteMax ], "GetGreyWhiteValues" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Getting regular histogram, already without null values
                                        // Used only to recover the modes of each part
THistogram          H   (   stats,
                            0,
                            0,
                            0,
                            3,  3,
                            (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );

                                        // Some more smoothing before looking for modes
FctParams           p;

p ( FilterParamDiameter )     = 2 * H.GetKernelSubsampling () + 1;

H.Filter ( FilterTypeGaussian, p );

//H.WriteFile ( "E:\\Data\\BGW Histogram.2.sef", "Modes" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // localizing the mode betwewen grey min and grey max
int                 blackmodei      = H.ToBin ( RealUnit, ( values[ BlackMin ] + values[ BlackMax ] ) / 2.0 );
int                 blackmini       = H.ToBin ( RealUnit, values[ BlackMin ] );
int                 blackmaxi       = H.ToBin ( RealUnit, values[ BlackMax ] );
int                 blackmodevalue  = 0;

for ( int i = blackmini; i <= blackmaxi; i++ )

    if ( H[ i ] > blackmodevalue ) {

        blackmodei      = i;
        blackmodevalue  = H[ i ];
        }


values[ BlackMode ] = Clip ( H.ToReal ( RealUnit, blackmodei ), values[ BlackMin ], values[ BlackMax ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // localizing the mode betwewen grey min and grey max
int                 greymodei       = H.ToBin ( RealUnit, ( values[ GreyMin ] + values[ GreyMax ] ) / 2.0 );
int                 greymini        = H.ToBin ( RealUnit, values[ GreyMin ] );
int                 greymaxi        = H.ToBin ( RealUnit, values[ GreyMax ] );
int                 greymodevalue   = 0;

for ( int i = greymini; i <= greymaxi; i++ )

    if ( H[ i ] > greymodevalue ) {

        greymodei       = i;
        greymodevalue   = H[ i ];
        }


values[ GreyMode ]  = Clip ( H.ToReal ( RealUnit, greymodei ), values[ GreyMin ], values[ GreyMax ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // localizing the mode between white min and white max
int                 whitemodei      = H.ToBin ( RealUnit, ( values[ WhiteMin ] + values[ WhiteMax ] ) / 2.0 );
int                 whitemini       = H.ToBin ( RealUnit, values[ WhiteMin ] );
int                 whitemaxi       = H.ToBin ( RealUnit, values[ WhiteMax ] );
int                 whitemodevalue  = 0;

for ( int i = whitemini; i <= whitemaxi; i++ )

    if ( H[ i ] > whitemodevalue ) {

        whitemodei      = i;
        whitemodevalue  = H[ i ];
        }


values[ WhiteMode ] = Clip ( H.ToReal ( RealUnit, whitemodei ), values[ WhiteMin ], values[ WhiteMax ] );

*/
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Only by content - File name guess has to be done elsewhere
                                        // Taken from TVolumeDoc::SetContentType
                                        // Background and Bounding box can be optionally given to speed up things, or they will be locally computed
template <class TypeD>
MriContentType  TVolume<TypeD>::GuessTypeFromContent    (   double  backgroundvalue,    TBoundingBox<double>*   bounding )  const
{

MriContentType      extracontenttype    = UnknownMriContentType;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case of binarized / discretized mask
bool                isdiscrete          = IsInteger ();
double              minvalue;
double              maxvalue;

GetMinMaxValues ( minvalue, maxvalue );

bool                somenegative        = minvalue < 0;


if ( ! somenegative && maxvalue <= 32 && isdiscrete ) {

    SetFlags    (   extracontenttype,   MriContentTypeMask,                                 MriContentTypeSegmented );

    SetFlags    (   extracontenttype,   MriContentTypeSegmentedMask,    maxvalue == 1   ?   MriContentTypeBinaryMask 
                                                                                        :   MriContentTypeRoiMask    );

    return  extracontenttype;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need 2 background values: one lower and one higher than backgroundvalue
if ( backgroundvalue == 0 ) 
                    backgroundvalue = GetBackgroundValue ();


THistogram          Hlow    (   *this,
                                0,
                                BackgroundMaxSamples,
                                0,
                                0,  3,
                                                                                                                                                // just in case we have negative values
                                (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls | HistogramAbsolute )
                            );
    

                                        // First, work with the histogram bins
double              lastpos         = Hlow.GetLastPosition ( BinUnit );
int                 backhighbin     = Hlow.ToBin ( RealUnit, backgroundvalue );
int                 backlowbin      = Hlow.ToBin ( RealUnit, backgroundvalue );

                                        // smooth it a lot
Hlow.Smooth ( 2 * Hlow.GetKernelSubsampling () + 1 );

//Hlow.WriteFile ( "E:\\Data\\Histo.Mri.sef" );

double              HlowMaxValue    = Hlow.GetMaxValue ();

                                        // go backward, until we reach the interesting level
while ( backhighbin > 0 && Hlow[ backhighbin ] < 0.10 * HlowMaxValue     )  backhighbin--; 
                                        // continue backward, until we pass the max
while ( backhighbin > 1 && Hlow[ backhighbin ] < Hlow[ backhighbin - 1 ] )  backhighbin--; 
                                        // move forward, to get passed the max
backhighbin    += backhighbin == 0 ? 1 : 2;

                                        // stretch forward the regular background
while ( backlowbin < lastpos / 2 && Hlow[ backlowbin ] > Hlow[ backlowbin + 1 ] )    backlowbin++;    


CheckOrder ( backlowbin, backhighbin );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now work with the real values
double              Hmaxvalue       = Hlow.GetMaxStat (); // maxvalue; // close enough
double              backhighreal    = Hlow.ToReal ( RealUnit, backhighbin );
double              backlowreal     = Hlow.ToReal ( RealUnit, backlowbin  );


if ( somenegative ) { // or maxvalue <= 0?
    backlowreal     = - backlowreal;
    backhighreal    = - backhighreal;
    Hmaxvalue       = - Hmaxvalue;
    maxvalue        = minvalue;     // !overwriting!
    }

//DBGV4 ( backlowreal, backhighreal, Hmaxvalue, maxvalue, "backlow backhigh,  backhigh high hmax maxvalue" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Study the thickness from the background to the outside until we reach the 0.
                                        // Non-segmented data have a long tail, segmented have shorter one.
                                        // However, we need 2 background values, in case of segmented fuzzy / blobby data
                                        // which will look like a full head (long trail), but not if we cut them more seriously

int                 steplow         = Step ( 16, true );

TPointInt           boundmin ( 0,        0,        0        );
TPointInt           boundmax ( Dim1 - 1, Dim2 - 1, Dim3 - 1 );
TEasyStats          statthickness;
TEasyStats          statthicknesslow;


TArray1<TypeD>      vs ( MaxSize () );
TypeD               v;
TPointInt           p;
                                        // scan from all sides, storing all values up to the border,
                                        // then analyse this trail backward
for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += steplow )
for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += steplow ) {

    vs.ResetMemory ();
    int     vsi     = 0;

    for ( p.X = boundmin.X; p.X <= boundmax.X; p.X ++ ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    int     count       = 0;
    int     countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 0 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 0 ) );


    vs.ResetMemory ();
    vsi     = 0;

    for ( p.X = boundmax.X; p.X >= boundmin.X; p.X -- ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    count       = 0;
    countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 0 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 0 ) );
    } // for z


for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += steplow )
for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += steplow ) {

    vs.ResetMemory ();
    int     vsi     = 0;

    for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y ++ ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    int     count       = 0;
    int     countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 1 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 1 ) );


    vs.ResetMemory ();
    vsi     = 0;

    for ( p.Y = boundmax.Y; p.Y >= boundmin.Y; p.Y -- ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    count       = 0;
    countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 1 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 1 ) );

    } // for z


for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += steplow )
for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += steplow ) {

    vs.ResetMemory ();
    int     vsi     = 0;

    for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z ++ ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    int     count       = 0;
    int     countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 2 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 2 ) );


    vs.ResetMemory ();
    vsi     = 0;

    for ( p.Z = boundmax.Z; p.Z >= boundmin.Z; p.Z -- ) {
        vs[ vsi++ ]     = v     = GetValue ( p );
        if ( v >= backhighreal )
            break;
        }

    count       = 0;
    countlow    = 0;

    for ( int i = vsi - 1; i >= 0; i-- ) {
        if ( vs[ i ] > 0 ) {
            count++;

            if ( vs[ i ] < backlowreal )
                countlow++;
            }
        else
            break;
        }

    statthickness.   Add ( (double) count    / GetDim ( 2 ) );
    statthicknesslow.Add ( (double) countlow / GetDim ( 2 ) );

    } // for y


//statthickness.Show ( "statthickness" );
//statthicknesslow.Show ( "statthicknesslow" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              lowthick        = statthicknesslow.Average () * 100;
double              ratiothick1     = statthickness   .Average ()       / ( statthicknesslow.Average () ? statthicknesslow.Average () : 1e-10 );
double              ratiothick2     = ratiothick1 * statthickness.SD () / ( statthicknesslow.SD ()      ? statthicknesslow.SD ()      : 1e-10 );
double              backlowratio    = backlowreal                       / NonNull ( backhighreal ) * 100;

//DBGV5 ( lowthick, ratiothick1, ratiothick2, backlowreal, backlowratio, "lowAvg  ratiothick1  ratiothick2  backlow  backlowratio" );

                                        // !NOTE! some package can segment the full head, ie, remove the background noise, but the content is still a full head

                                        // 1) Look at the average thickness, from the lowest background
                                        // Full average 20%,    lowest  0.114%
                                        // Seg  average 0.1%,   highest 0.89%
if      ( lowthick >= 2.00 ) {
                                        // full head
    SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeFullHead );

    return  extracontenttype;
    }
                                        // min head = 0.40   max seg = 0.05
else if ( lowthick <= 0.25 ) {
                                        // segmented
    SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeSegmented );
    }

else {
                                        // 2) Otherwise, look at the ratio between the average trails of the two backgrounds

                                        // Full average  4%,    highest 31%
                                        // Seg  average 35%,    lowest   7.5%
    if      ( ratiothick2 < 4.5 /*8.00*/ ) {
                                        // full head
        SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeFullHead );

        return  extracontenttype;
        }
    else if ( ratiothick2 > 40.00 ) {
                                        // segmented
        SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeSegmented );
        }

    else {
                                        // 3) Otherwise, look at the ratio of the background values

                                        // Full average 50%,    lowest  10.7%
                                        // Seg  average  5%,    highest  6%
        if      ( backlowratio >= 50 ) {
                                            // full head
            SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeFullHead );

            return  extracontenttype;
            }
        else if ( backlowratio <= 8 ) {
                                        // segmented
            SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeSegmented );
            }
        else
            ;                           // 4) Can't decide -> use the old way
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Old way: analysing the histogram

                                        // guess if this is a segmented stuff
                                        // by checking the border of the cube to be 0
int                 i11             = 0;
int                 i12             = Dim1 - 1;
int                 i21             = 0;
int                 i22             = Dim2 - 1;
int                 i31             = 0;
int                 i32             = Dim3 - 1;

/*
double              d1      = i12 - i11;
double              d2      = i22 - i21;
double              d3      = i32 - i31;

double      inward  = 0.0666;
                                        // move a bit inward, in case of a full head rotated into a bigger MRI
i11        += inward * d1;
i12        -= inward * d1;
i21        += inward * d2;
i22        -= inward * d2;
i31        += inward * d3;
i32        -= inward * d3;
*/

//DBGV3 ( i11, i21, i31, "min box to scan" );
//DBGV3 ( i12, i22, i32, "max box to scan" );
//DBGV ( backgroundvalue, "backgroundvalue" );


double              bv              = Clip ( fabs ( Hmaxvalue ) - 1, 0.0, 2 * backgroundvalue );
THistogram          H ( bv / 2 + 2 );
int                 x, y, z;
int                 step            = Step ( 128, true ) * 2;
//TEasyStats          stats;
double              hi;


for ( x = i11; x < i12; x += step )
for ( y = i21; y < i22; y += step )
for ( z = i31; z < i32; z += step ) {

//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x + 1, y, z ) <= bv )
//        H[ fabs ( GetValue ( x, y, z ) - GetValue ( x + 1, y, z ) ) / 2.0 + 0.5 ]++;
//
//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x, y + 1, z ) <= bv )
//        H[ fabs ( GetValue ( x, y, z ) - GetValue ( x, y + 1, z ) ) / 2.0 + 0.5 ]++;
//
//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x, y, z + 1 ) <= bv )
//        H[ fabs ( GetValue ( x, y, z ) - GetValue ( x, y, z + 1 ) ) / 2.0 + 0.5 ]++;


    hi  = fabs ( GetValue ( x, y, z ) - GetValue ( x + 1, y, z ) );
    if ( hi <= bv )
        H[ hi / 2 + 0.5 ]++;

    hi  = fabs ( GetValue ( x, y, z ) - GetValue ( x, y + 1, z ) );
    if ( hi <= bv )
        H[ hi / 2 + 0.5 ]++;

    hi  = fabs ( GetValue ( x, y, z ) - GetValue ( x, y, z + 1 ) );
    if ( hi <= bv )
        H[ hi / 2 + 0.5 ]++;


//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x + 1, y, z ) <= bv )
//        stats.Add ( fabs ( GetValue ( x, y, z ) - GetValue ( x + 1, y, z ) ) / 2 + 0.5 );
//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x, y + 1, z ) <= bv )
//        stats.Add ( fabs ( GetValue ( x, y, z ) - GetValue ( x, y + 1, z ) ) / 2 + 0.5 );
//    if ( GetValue ( x, y, z ) <= bv && GetValue ( x, y, z + 1 ) <= bv )
//        stats.Add ( fabs ( GetValue ( x, y, z ) - GetValue ( x, y, z + 1 ) ) / 2 + 0.5 );
    }
//DBGV2 ( stats.Average (), stats.SD (), "Avg SD" );

                                        // erode harder
H.Erode ( 0.01 );

//TFileName           buff;
//sprintf ( buff, "E:\\Data\\ContentType.Histo Background.sef" );
//H.WriteFile ( buff );


bool                segment;

segment     = H.GetExtent ( BinUnit ) == 1 || ( H.GetExtent ( BinUnit ) == 2 && H[ 1 ] / NonNull ( H[ 0 ] ) < 0.01 );

//DBGV2 ( H.GetExtent ( BinUnit ), segment, "GetExtent  Segmented" );


if ( IsUnknownMriType ( extracontenttype ) )    // no previous detection?

    if ( segment ) {
        SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeSegmented );
        }
    else {
        SetFlags    (   extracontenttype,   MriContentTypeMask,     MriContentTypeFullHead );

        return  extracontenttype;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Not a mask if floating point vallues - no need to go any further
if ( ! isdiscrete )
    return  extracontenttype;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan for a mask
                                        // check spatial neighbors for differencees
#define     numvoxelscanned     6133

double              v1;
double              v2;
double              numchanges;
double              numneighbors;
double              ratio;
int                 vi;
int                 modx, mody, modz;
int                 vol;
                                        // for binary / roi mask differentiation
int                 nnc1            = 0;
int                 nnc2            = 0;
int                 nn1             = 0;
int                 nn2             = 0;
bool                binarymask;

                                        // did the caller provide some bounding box?
TBoundingBox<double>Bounding;
if ( bounding )     Bounding        = *bounding;
else                Bounding.Set    ( *this, false, backgroundvalue );    // boundary of non-null data


if ( Bounding.MinSize() >= 4 ) {

    modx        = (int) ( Bounding.GetXExtent () - 3 );
    mody        = (int) ( Bounding.GetYExtent () - 3 );
    modz        = (int) ( Bounding.GetZExtent () - 3 );
    vol         = modx * mody * modz;
    numchanges  = numneighbors = 0;


    for ( int i = 0; i < numvoxelscanned; i++ ) {

        vi  = Round ( ( i / (double) ( numvoxelscanned - 1 ) ) * ( vol - 1 ) );

        x   = Bounding.XMin () +   vi % modx;
        y   = Bounding.YMin () + ( vi / modx ) % mody;
        z   = Bounding.ZMin () + ( vi / modx   / mody ) % modz;


        v   = GetValue ( x, y, z );
        if ( ! v )      continue;
                                        // roi mask test
                                        // do an average, a mask will still show blocky borders
                                        // while low rez data (as in inverse) will not be detected as mask
        v1 = max ( GetValue ( x,     y, z ), GetValue ( x + 1, y, z ) );
        v2 = max ( GetValue ( x + 2, y, z ), GetValue ( x + 3, y, z ) );

        if ( v2 ) {
            numneighbors++;
            if ( v1 != v2 )
                numchanges++;
            }

        v1 = max ( GetValue ( x, y,     z ), GetValue ( x, y + 1, z ) );
        v2 = max ( GetValue ( x, y + 2, z ), GetValue ( x, y + 2, z ) );

        if ( v2 ) {
            numneighbors++;
            if ( v1 != v2 )
                numchanges++;
            }

        v1 = max ( GetValue ( x, y, z     ), GetValue ( x, y, z + 1 ) );
        v2 = max ( GetValue ( x, y, z + 2 ), GetValue ( x, y, z + 3 ) );

        if ( v2 ) {
            numneighbors++;
            if ( v1 != v2 )
                numchanges++;
            }

                                        // binary mask test
        if      ( ! nn1 )               nn1 = v;
        else if ( ! nn2 && v != nn1 )   nn2 = v;

        if      ( v == nn1 )            nnc1++;
        else if ( v == nn2 )            nnc2++;
        } // for voxel scanned

                                        // if counting is not enough, use stats on the difference f.ex.
    ratio       = numchanges / ( numneighbors ? numneighbors : 1 );
//    DBGV3 ( numchanges, numneighbors, ratio, "#changes  #tests  mask ratio" );

    binarymask  = ( ! nnc1 && ! nnc2 ) || nnc1 && ! nnc2;
//    DBGV5 ( nn1, nnc1, nn2, nnc2, binarymask, "nn1, nnc1, nn2, nnc2, binarymask" );

                                            // lowest seg:                          0.868
                                            // lowest full: (import-iso-stdsag.hdr) 0.4313, (amim.hdr) 0.22947
                                            // highest mask:                        0.2275
    if      ( binarymask    )   SetFlags    (   extracontenttype,   MriContentTypeMaskMask,     MriContentTypeBinaryMask );
    else if ( ratio < 0.228 )   SetFlags    (   extracontenttype,   MriContentTypeMaskMask,     MriContentTypeRoiMask    );
    else                        ResetFlags  (   extracontenttype,   MriContentTypeMaskMask );
    }

else                                    // small bounding
//                              ResetFlags  (   extracontenttype,   MriContentTypeMaskMask );
                                SetFlags    (   extracontenttype,   MriContentTypeMaskMask,     MriContentTypeBinaryMask );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*          // Off for the moment: not really usefull, plus it interferes badly when falsely set by changing the isosurface curtting value

                                        // scan for a blob (== computed, continuous-like results of some sort)

                                        // get a very coarse histogram
THistogram          HV ( this, 4 );
                                        // smooth hard
HV.Smooth ( 3 );
                                        // erode hard
HV.Erode ( 0.001 );

//HV.WriteFile ( "E:\\Data\\MriIsABlob.txt", true );

int                 numdec          = 0;
int                 numinc          = 0;
                                        // count # of increasing & # decreasing values
for ( int i = HV.GetFirstPosition (); i <= HV.GetLastPosition () - 1; i++ ) {
    if ( HV[ i ] >= HV[ i + 1 ] )   numdec++;
    if ( HV[ i ] <= HV[ i + 1 ] )   numinc++;
    }
                                        // get the difference of count:
                                        // Blob has a long and continuous decreasing tail
                                        // Non-blob have more or less an equal amount of inc/dec, or have more inc then dec
double              did             = (double) ( numdec - numinc ) / HV.GetExtent ();

//DBGV2 ( HV.NumModes (), HV.MaximumPosition (), "#modes  MaxPos" );
//DBGV4 ( numdec, numinc, HV.GetExtent (), did * 100, "numdec, numinc, HV.GetExtent -> did" );

                                        // Min blob: 84, max non-blob: 21
                                        // Min IS blob: 58
                                        // max non-blob for Mouse: 0.53
if ( did < 0.555 )  extracontenttype &= ~MriContentTypeBlob;
else                extracontenttype |=  MriContentTypeBlob;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some consistency checks
if ( IsMask ( extracontenttype ) )      ResetFlags  (   extracontenttype,   MriContentTypeBlob );   // mask -> not a blob


return  extracontenttype;
}


//----------------------------------------------------------------------------
                                        // Data types are assumed to be exclusive: all integer or none
template <class TypeD>
bool    TVolume<TypeD>::IsInteger ( int numtests )    const
{
TDownsampling       downsp ( LinearDim, numtests );

for ( int i = downsp.From; i <= downsp.To; i += downsp.Step )
    if ( ! crtl::IsInteger ( (double) Array[ i ] ) )
        return  false;

return  true;
}


template <class TypeD>
bool    TVolume<TypeD>::IsPositive ()  const
{
for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] < 0 )
        return  false;

return  true;
}


template <class TypeD>
bool    TVolume<TypeD>::IsNegative ()  const
{
for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] > 0 )
        return  false;

return  true;
}


template <class TypeD>
bool    TVolume<TypeD>::IsNull ()   const
{
for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] != (TypeD) 0 )
        return  false;

return  true;
}


template <class TypeD>
void    TVolume<TypeD>::HasPositiveNegative ( bool& somepos, bool& someneg )  const
{
somepos     = false;
someneg     = false;

for ( int i = 0; i < LinearDim; i++ ) {
    
    if ( Array[ i ] > 0 )   somepos = true;
    if ( Array[ i ] < 0 )   someneg = true;

    if ( somepos && someneg )   
        return;
    }
}


//----------------------------------------------------------------------------
                                        // returned value is double, because of interpolate factor (think cutting mask between 0 and 1)
template <class TypeD>
TypeD   TVolume<TypeD>::GetValueChecked ( double x, double y, double z, InterpolationType interpolate )   const
{

if      ( IsFlag ( interpolate, InterpolateTruncate ) )

    return  GetValueChecked ( x, y, z );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( IsFlag ( interpolate, InterpolateNearestNeighbor ) )

    return  GetValueChecked ( x + 0.5, y + 0.5, z + 0.5 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( IsFlag ( interpolate, InterpolateLinear ) ) {

    int                 xi              = Truncate ( x );
    int                 yi              = Truncate ( y );
    int                 zi              = Truncate ( z );
    double              fx              = x - xi;
    double              fy              = y - yi;
    double              fz              = z - zi;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }
    if ( fy < 0 )   {   yi--;   fy += 1; }
    if ( fz < 0 )   {   zi--;   fz += 1; }


    double              l00             = ( 1 - fx ) * GetValueChecked ( xi, yi,     zi     ) + fx * GetValueChecked ( xi + 1, yi,     zi     );
    double              l01             = ( 1 - fx ) * GetValueChecked ( xi, yi + 1, zi     ) + fx * GetValueChecked ( xi + 1, yi + 1, zi     );
    double              l02             = ( 1 - fx ) * GetValueChecked ( xi, yi,     zi + 1 ) + fx * GetValueChecked ( xi + 1, yi,     zi + 1 );
    double              l03             = ( 1 - fx ) * GetValueChecked ( xi, yi + 1, zi + 1 ) + fx * GetValueChecked ( xi + 1, yi + 1, zi + 1 );

    double              l0              = ( 1 - fy ) * l00 + fy * l01;
    double              l1              = ( 1 - fy ) * l02 + fy * l03;

    double              l               = ( 1 - fz ) * l0 + fz * l1;

                                        // linear interpolation never under- nor over-shoot
    return  (TypeD) l;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a 4^3 = 64 voxels interpolation
else if ( IsFlag ( interpolate, InterpolateUniformCubicBSpline )
       || IsFlag ( interpolate, InterpolateCubicHermiteSpline  ) ) {

                                        // these interpolations have behave the same
    double              (*spline)   ( double, double, double, double, double )  = IsFlag ( interpolate, InterpolateUniformCubicBSpline ) ? UniformCubicBSpline
                                                                                                                                         : CubicHermiteSpline;

    int                 xi              = Truncate ( x );
    int                 yi              = Truncate ( y );
    int                 zi              = Truncate ( z );
    double              fx              = x - xi;
    double              fy              = y - yi;
    double              fz              = z - zi;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }
    if ( fy < 0 )   {   yi--;   fy += 1; }
    if ( fz < 0 )   {   zi--;   fz += 1; }

/*                                      // compute once the clipped indexes for each dimension, to avoid multiple testing if one would call GetValueChecked
                                        // !this notation implies boundary values extension!
    int                 xip             = Clip ( xi - 1, 0, Dim1 - 1 );
    int                 xi0             = Clip ( xi    , 0, Dim1 - 1 );
    int                 xi1             = Clip ( xi + 1, 0, Dim1 - 1 );
    int                 xi2             = Clip ( xi + 2, 0, Dim1 - 1 );
    int                 yip             = Clip ( yi - 1, 0, Dim2 - 1 );
    int                 yi0             = Clip ( yi    , 0, Dim2 - 1 );
    int                 yi1             = Clip ( yi + 1, 0, Dim2 - 1 );
    int                 yi2             = Clip ( yi + 2, 0, Dim2 - 1 );
    int                 zip             = Clip ( zi - 1, 0, Dim3 - 1 );
    int                 zi0             = Clip ( zi    , 0, Dim3 - 1 );
    int                 zi1             = Clip ( zi + 1, 0, Dim3 - 1 );
    int                 zi2             = Clip ( zi + 2, 0, Dim3 - 1 );

                                        // We need 4*4*4 = 64 voxels
    double              s00             = spline (   GetValue ( xip, yip, zip ),    GetValue ( xi0, yip, zip ),    GetValue ( xi1, yip, zip ),    GetValue ( xi2, yip, zip ), fx );
    double              s01             = spline (   GetValue ( xip, yip, zi0 ),    GetValue ( xi0, yip, zi0 ),    GetValue ( xi1, yip, zi0 ),    GetValue ( xi2, yip, zi0 ), fx );
    double              s02             = spline (   GetValue ( xip, yip, zi1 ),    GetValue ( xi0, yip, zi1 ),    GetValue ( xi1, yip, zi1 ),    GetValue ( xi2, yip, zi1 ), fx );
    double              s03             = spline (   GetValue ( xip, yip, zi2 ),    GetValue ( xi0, yip, zi2 ),    GetValue ( xi1, yip, zi2 ),    GetValue ( xi2, yip, zi2 ), fx );
    double              s10             = spline (   GetValue ( xip, yi0, zip ),    GetValue ( xi0, yi0, zip ),    GetValue ( xi1, yi0, zip ),    GetValue ( xi2, yi0, zip ), fx );
    double              s11             = spline (   GetValue ( xip, yi0, zi0 ),    GetValue ( xi0, yi0, zi0 ),    GetValue ( xi1, yi0, zi0 ),    GetValue ( xi2, yi0, zi0 ), fx );
    double              s12             = spline (   GetValue ( xip, yi0, zi1 ),    GetValue ( xi0, yi0, zi1 ),    GetValue ( xi1, yi0, zi1 ),    GetValue ( xi2, yi0, zi1 ), fx );
    double              s13             = spline (   GetValue ( xip, yi0, zi2 ),    GetValue ( xi0, yi0, zi2 ),    GetValue ( xi1, yi0, zi2 ),    GetValue ( xi2, yi0, zi2 ), fx );
    double              s20             = spline (   GetValue ( xip, yi1, zip ),    GetValue ( xi0, yi1, zip ),    GetValue ( xi1, yi1, zip ),    GetValue ( xi2, yi1, zip ), fx );
    double              s21             = spline (   GetValue ( xip, yi1, zi0 ),    GetValue ( xi0, yi1, zi0 ),    GetValue ( xi1, yi1, zi0 ),    GetValue ( xi2, yi1, zi0 ), fx );
    double              s22             = spline (   GetValue ( xip, yi1, zi1 ),    GetValue ( xi0, yi1, zi1 ),    GetValue ( xi1, yi1, zi1 ),    GetValue ( xi2, yi1, zi1 ), fx );
    double              s23             = spline (   GetValue ( xip, yi1, zi2 ),    GetValue ( xi0, yi1, zi2 ),    GetValue ( xi1, yi1, zi2 ),    GetValue ( xi2, yi1, zi2 ), fx );
    double              s30             = spline (   GetValue ( xip, yi2, zip ),    GetValue ( xi0, yi2, zip ),    GetValue ( xi1, yi2, zip ),    GetValue ( xi2, yi2, zip ), fx );
    double              s31             = spline (   GetValue ( xip, yi2, zi0 ),    GetValue ( xi0, yi2, zi0 ),    GetValue ( xi1, yi2, zi0 ),    GetValue ( xi2, yi2, zi0 ), fx );
    double              s32             = spline (   GetValue ( xip, yi2, zi1 ),    GetValue ( xi0, yi2, zi1 ),    GetValue ( xi1, yi2, zi1 ),    GetValue ( xi2, yi2, zi1 ), fx );
    double              s33             = spline (   GetValue ( xip, yi2, zi2 ),    GetValue ( xi0, yi2, zi2 ),    GetValue ( xi1, yi2, zi2 ),    GetValue ( xi2, yi2, zi2 ), fx );
*/

    double              s00             = spline (   GetValueChecked ( xi - 1, yi - 1, zi - 1 ),    GetValueChecked ( xi    , yi - 1, zi - 1 ),    GetValueChecked ( xi + 1, yi - 1, zi - 1 ),    GetValueChecked ( xi + 2, yi - 1, zi - 1 ), fx );
    double              s01             = spline (   GetValueChecked ( xi - 1, yi - 1, zi     ),    GetValueChecked ( xi    , yi - 1, zi     ),    GetValueChecked ( xi + 1, yi - 1, zi     ),    GetValueChecked ( xi + 2, yi - 1, zi     ), fx );
    double              s02             = spline (   GetValueChecked ( xi - 1, yi - 1, zi + 1 ),    GetValueChecked ( xi    , yi - 1, zi + 1 ),    GetValueChecked ( xi + 1, yi - 1, zi + 1 ),    GetValueChecked ( xi + 2, yi - 1, zi + 1 ), fx );
    double              s03             = spline (   GetValueChecked ( xi - 1, yi - 1, zi + 2 ),    GetValueChecked ( xi    , yi - 1, zi + 2 ),    GetValueChecked ( xi + 1, yi - 1, zi + 2 ),    GetValueChecked ( xi + 2, yi - 1, zi + 2 ), fx );

    double              s10             = spline (   GetValueChecked ( xi - 1, yi    , zi - 1 ),    GetValueChecked ( xi    , yi    , zi - 1 ),    GetValueChecked ( xi + 1, yi    , zi - 1 ),    GetValueChecked ( xi + 2, yi    , zi - 1 ), fx );
    double              s11             = spline (   GetValueChecked ( xi - 1, yi    , zi     ),    GetValueChecked ( xi    , yi    , zi     ),    GetValueChecked ( xi + 1, yi    , zi     ),    GetValueChecked ( xi + 2, yi    , zi     ), fx );
    double              s12             = spline (   GetValueChecked ( xi - 1, yi    , zi + 1 ),    GetValueChecked ( xi    , yi    , zi + 1 ),    GetValueChecked ( xi + 1, yi    , zi + 1 ),    GetValueChecked ( xi + 2, yi    , zi + 1 ), fx );
    double              s13             = spline (   GetValueChecked ( xi - 1, yi    , zi + 2 ),    GetValueChecked ( xi    , yi    , zi + 2 ),    GetValueChecked ( xi + 1, yi    , zi + 2 ),    GetValueChecked ( xi + 2, yi    , zi + 2 ), fx );

    double              s20             = spline (   GetValueChecked ( xi - 1, yi + 1, zi - 1 ),    GetValueChecked ( xi    , yi + 1, zi - 1 ),    GetValueChecked ( xi + 1, yi + 1, zi - 1 ),    GetValueChecked ( xi + 2, yi + 1, zi - 1 ), fx );
    double              s21             = spline (   GetValueChecked ( xi - 1, yi + 1, zi     ),    GetValueChecked ( xi    , yi + 1, zi     ),    GetValueChecked ( xi + 1, yi + 1, zi     ),    GetValueChecked ( xi + 2, yi + 1, zi     ), fx );
    double              s22             = spline (   GetValueChecked ( xi - 1, yi + 1, zi + 1 ),    GetValueChecked ( xi    , yi + 1, zi + 1 ),    GetValueChecked ( xi + 1, yi + 1, zi + 1 ),    GetValueChecked ( xi + 2, yi + 1, zi + 1 ), fx );
    double              s23             = spline (   GetValueChecked ( xi - 1, yi + 1, zi + 2 ),    GetValueChecked ( xi    , yi + 1, zi + 2 ),    GetValueChecked ( xi + 1, yi + 1, zi + 2 ),    GetValueChecked ( xi + 2, yi + 1, zi + 2 ), fx );

    double              s30             = spline (   GetValueChecked ( xi - 1, yi + 2, zi - 1 ),    GetValueChecked ( xi    , yi + 2, zi - 1 ),    GetValueChecked ( xi + 1, yi + 2, zi - 1 ),    GetValueChecked ( xi + 2, yi + 2, zi - 1 ), fx );
    double              s31             = spline (   GetValueChecked ( xi - 1, yi + 2, zi     ),    GetValueChecked ( xi    , yi + 2, zi     ),    GetValueChecked ( xi + 1, yi + 2, zi     ),    GetValueChecked ( xi + 2, yi + 2, zi     ), fx );
    double              s32             = spline (   GetValueChecked ( xi - 1, yi + 2, zi + 1 ),    GetValueChecked ( xi    , yi + 2, zi + 1 ),    GetValueChecked ( xi + 1, yi + 2, zi + 1 ),    GetValueChecked ( xi + 2, yi + 2, zi + 1 ), fx );
    double              s33             = spline (   GetValueChecked ( xi - 1, yi + 2, zi + 2 ),    GetValueChecked ( xi    , yi + 2, zi + 2 ),    GetValueChecked ( xi + 1, yi + 2, zi + 2 ),    GetValueChecked ( xi + 2, yi + 2, zi + 2 ), fx );


    double              s0              = spline ( s00, s01, s02, s03, fz );
    double              s1              = spline ( s10, s11, s12, s13, fz );
    double              s2              = spline ( s20, s21, s22, s23, fz );
    double              s3              = spline ( s30, s31, s32, s33, fz );


    double              s               = spline ( s0, s1, s2, s3, fy );


    CheckOvershooting ( interpolate, s );


    return  (TypeD) s;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (2*2)^3 = 64 voxels interpolation
else if ( IsFlag ( interpolate, InterpolateLanczos2 ) ) {

    int                 xi              = Truncate ( x );
    int                 yi              = Truncate ( y );
    int                 zi              = Truncate ( z );
    double              fx              = x - xi;
    double              fy              = y - yi;
    double              fz              = z - zi;
    double              lx[ Lanczos2Size2 ];     // Lanczos can be separated for each axis
    double              ly[ Lanczos2Size2 ];
    double              lz[ Lanczos2Size2 ];
//  double              t;
    double              v               = 0;
//  double              w               = 0;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }
    if ( fy < 0 )   {   yi--;   fy += 1; }
    if ( fz < 0 )   {   zi--;   fz += 1; }

                                        // compute Lanczos factors @ current point
    for ( int i = 0; i < Lanczos2Size2; i++ ) {
//      t           = M_PI * ( fx - i + Lanczos2Size - 1 );
//      lx[ i ]     = t ? Lanczos2Size * sin ( t ) * sin ( t / Lanczos2Size ) / ( t * t ) : 1;
        lx[ i ]     = FastLanczos2 ( fx - i + Lanczos2Size - 1 );

//      t           = M_PI * ( fy - i + Lanczos2Size - 1 );
//      ly[ i ]     = t ? Lanczos2Size * sin ( t ) * sin ( t / Lanczos2Size ) / ( t * t ) : 1;
        ly[ i ]     = FastLanczos2 ( fy - i + Lanczos2Size - 1 );

//      t           = M_PI * ( fz - i + Lanczos2Size - 1 );
//      lz[ i ]     = t ? Lanczos2Size * sin ( t ) * sin ( t / Lanczos2Size ) / ( t * t ) : 1;
        lz[ i ]     = FastLanczos2 ( fz - i + Lanczos2Size - 1 );
        }

                                        // scan kernel
    for ( int xd0 = 0, xd = xi + xd0 - Lanczos2Size2 + 1; xd0 < Lanczos2Size2; xd0++, xd++ )
    for ( int yd0 = 0, yd = yi + yd0 - Lanczos2Size2 + 1; yd0 < Lanczos2Size2; yd0++, yd++ )
    for ( int zd0 = 0, zd = zi + zd0 - Lanczos2Size2 + 1; zd0 < Lanczos2Size2; zd0++, zd++ ) {

        v  += GetValueChecked ( xd, yd, zd )
            * lx[ xd0 ] * ly[ yd0 ] * lz[ zd0 ];

//      w  += lx[ xd0 ] * ly[ yd0 ] * lz[ zd0 ];
        }


    CheckOvershooting ( interpolate, v );


    return  (TypeD) v;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (2*3)^3 = 216 voxels interpolation
else if ( IsFlag ( interpolate, InterpolateLanczos3 ) ) {

    int                 xi              = Truncate ( x );
    int                 yi              = Truncate ( y );
    int                 zi              = Truncate ( z );
    double              fx              = x - xi;
    double              fy              = y - yi;
    double              fz              = z - zi;
    double              lx[ Lanczos3Size2 ];     // Lanczos can be separated for each axis
    double              ly[ Lanczos3Size2 ];
    double              lz[ Lanczos3Size2 ];
//  double              t;
    double              v               = 0;
//  double              w               = 0;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }
    if ( fy < 0 )   {   yi--;   fy += 1; }
    if ( fz < 0 )   {   zi--;   fz += 1; }

                                        // compute Lanczos factors @ current point
    for ( int i = 0; i < Lanczos3Size2; i++ ) {
//      t           = M_PI * ( fx - i + Lanczos3Size - 1 );
//      lx[ i ]     = t ? Lanczos3Size * sin ( t ) * sin ( t / Lanczos3Size ) / ( t * t ) : 1;
        lx[ i ]     = FastLanczos3 ( fx - i + Lanczos3Size - 1 );

//      t           = M_PI * ( fy - i + Lanczos3Size - 1 );
//      ly[ i ]     = t ? Lanczos3Size * sin ( t ) * sin ( t / Lanczos3Size ) / ( t * t ) : 1;
        ly[ i ]     = FastLanczos3 ( fy - i + Lanczos3Size - 1 );

//      t           = M_PI * ( fz - i + Lanczos3Size - 1 );
//      lz[ i ]     = t ? Lanczos3Size * sin ( t ) * sin ( t / Lanczos3Size ) / ( t * t ) : 1;
        lz[ i ]     = FastLanczos3 ( fz - i + Lanczos3Size - 1 );
        }

                                        // scan kernel
    for ( int xd0 = 0, xd = xi + xd0 - Lanczos3Size + 1; xd0 < Lanczos3Size2; xd0++, xd++ )
    for ( int yd0 = 0, yd = yi + yd0 - Lanczos3Size + 1; yd0 < Lanczos3Size2; yd0++, yd++ )
    for ( int zd0 = 0, zd = zi + zd0 - Lanczos3Size + 1; zd0 < Lanczos3Size2; zd0++, zd++ ) {

        v  += GetValueChecked ( xd, yd, zd )
            * lx[ xd0 ] * ly[ yd0 ] * lz[ zd0 ];

//      w  += lx[ xd0 ] * ly[ yd0 ] * lz[ zd0 ];
        }


    CheckOvershooting ( interpolate, v );


    return  (TypeD) v;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if everything else fails
return  (TypeD) 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
                                        // Split implementation into sub-files for convenience
#include    "TVolume.Filters.h"
#include    "TVolume.Morphology.h"
#include    "TVolume.Masks.h"
#include    "TVolume.Regions.h"
#include    "TVolume.Gradients.h"
#include    "TVolume.Drawing.h"
