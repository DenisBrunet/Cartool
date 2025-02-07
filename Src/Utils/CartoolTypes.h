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

#include    "Files.Extensions.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr size_t    KiloByte        = (size_t) 1 << 10;
constexpr size_t    MegaByte        = (size_t) 1 << 20;
constexpr size_t    GigaByte        = (size_t) 1 << 30;
constexpr size_t    TeraByte        = (size_t) 1 << 40;
constexpr size_t    PetaByte        = (size_t) 1 << 50;     // soon?
constexpr size_t    ExaByte         = (size_t) 1 << 60;     // getting all things ready for our friends at CERN

                                        
//----------------------------------------------------------------------------
                                        // Some fundamental types of Cartool:

                                        // Labels starts from 0 (i.e. they are indexes to arrays), therefor UndefinedLabel is set to -1
using       LabelType               = int;
constexpr LabelType UndefinedLabel  = (LabelType) -1;

                                        // A map is basically a (mathematical) vector of floats
template <class TypeD> class TVector;
using       TMapAtomType            = float;
using       TMap                    = TVector<TMapAtomType>;

                                        // Cartool internal MRI representation is single float, which covers all possible uses
using       MriType                 = float;
template <class TypeD> class    TVolume;
using       Volume                  = TVolume<MriType>; // and the corresponding volume type

                                        // ..and we need this little guy
using       uchar                   = unsigned char;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // All known filters across all classes and functions are listed here:
enum            FilterTypes
                {
                FilterTypeNone,

                FilterTypeMean,
                FilterTypeGaussian,
                FilterTypeFastGaussian,
                FilterTypeHighPassLight,
                FilterTypeLoG,
                FilterTypeAnisoGaussian,

                FilterTypeLowPass,
                FilterTypeHighPass,
                FilterTypeBandPass,
                FilterTypeBandStop,

                FilterTypeIntensityAdd,
                FilterTypeIntensitySub,
                FilterTypeIntensityMult,
                FilterTypeIntensityDiv,
                FilterTypeIntensityOpposite,
                FilterTypeIntensityInvert,
                FilterTypeIntensityAbs,
                FilterTypeIntensityNorm,

                FilterTypeMin,
                FilterTypeMax,
                FilterTypeMinMax,
                FilterTypeRange,
                FilterTypeMedian,
                FilterTypeInterquartileMean,
                FilterTypeMaxMode,
                FilterTypeMeanSub,
                FilterTypeLogMeanSub,
                FilterTypeMeanDiv,
                FilterTypeLogMeanDiv,
                FilterTypeMAD,
                FilterTypeSD,
                FilterTypeSDInv,
                FilterTypeCoV,
                FilterTypeLogCoV,
                FilterTypeLogSNR,
                FilterTypeMCoV,
                FilterTypeLogMCoV,
                FilterTypeMADSDInv,
                FilterTypeModeQuant,
                FilterTypeEntropy,
                FilterTypeRank,
                FilterTypeRankRamp,

                FilterTypeThreshold,
                FilterTypeThresholdAbove,
                FilterTypeThresholdBelow,
                FilterTypeThresholdBinarize,
                FilterTypeIntensityRemap,
                FilterTypeKeepValue,
                FilterTypeBinarize,
                FilterTypeRevert,
                FilterTypeToMask,
                FilterTypeSymmetrize,

                FilterTypeHistoEqual,
                FilterTypeHistoEqualBrain,
                FilterTypeHistoCompact,

                FilterTypeErode,
                FilterTypeDilate,
                FilterTypeOpen,
                FilterTypeClose,
                FilterTypeMorphGradient,
                FilterTypeMorphGradientInt,
                FilterTypeMorphGradientExt,

                FilterTypeThinning,
                FilterTypeWaterfallRidges,
                FilterTypeWaterfallValleys,
                FilterTypeLessNeighbors,
                FilterTypeMoreNeighbors,
                FilterTypeRelax,
                FilterTypeKeepBiggestRegion,
                FilterTypeKeepCompactRegion,
                FilterTypeKeepBiggest2Regions,
                FilterTypeKeepCompact2Regions,

                FilterTypePercentFullness,
                FilterTypeCutBelowFullness,
                FilterTypeSAV,
                FilterTypeCompactness,

                FilterTypeGradient,
                FilterTypeCanny,
                FilterTypeLaplacian,
                FilterTypeHessianEigenMax,
                FilterTypeHessianEigenMin,
                FilterTypeKCurvature,
                FilterTypeKCCurvature,
                FilterTypePeeling,

                FilterTypeClustersToRegions,
                FilterTypeRegionGrowing,

                FilterTypeBiasField,
                FilterTypeSegmentCSF,
                FilterTypeSegmentGrey,
                FilterTypeSegmentWhite,
                FilterTypeSegmentTissues,
                FilterTypeSkullStripping,
                FilterTypeBrainstemRemoval,
                FilterTypeMaskToSolutionPoints,

                FilterTypeAntialiasing,

                FilterTypeEnvelopeSlidingWindow,
                FilterTypeEnvelopeGapBridging,
                FilterTypeEnvelopeAnalytic,

                FilterTypeResize,

                NumFilterTypes
                };

                                        // Each filter needs some parameters - Here are there indexes in FctParams
constexpr int   MaxNumParams        = 8;

enum            FilterParams
                {
                FilterParamDiameter         = 0,    // Mean, Median, Gaussian, Morphology etc...

                FilterParamThreshold        = 0,

                FilterParamThresholdMin     = 0,
                FilterParamThresholdMax     = 1,
                FilterParamThresholdBin     = 2,

                FilterParamRemapFrom        = 0,
                FilterParamRemapTo          = 1,

                FilterParamKeepValue        = 0,

                FilterParamBinarized        = 0,

                FilterParamToMaskThreshold  = 0,
                FilterParamToMaskNewValue   = 1,
                FilterParamToMaskCarveBack  = 2,

                FilterParamSymmetrizeAxis   = 0,
                FilterParamSymmetrizeOrigin = 1,

                FilterParamNumModeQuant     = 1,

                FilterParamResultType       = 1,    // Mean Sub and the like
                FilterParamLogOffset        = 2,

                FilterParamOrder            = 0,    // Butterworth filters and the like
                FilterParamFreqCut          = 1,
                FilterParamFreqCutMin       = 1,
                FilterParamFreqCutMax       = 2,

                FilterParamOperand          = 0,    // Add Subtract etc.. by constant

                FilterParamCRatio           = 2,    // FilterTypeKCCurvature
                
                FilterParamResizeDim1       = 0,
                FilterParamResizeDim2       = 1,
                FilterParamResizeDim3       = 2,

                FilterParamThinningRepeat   = 0,

                FilterParamNumNeighbors     = 0,    // Less / More Neighbors
                FilterParamNeighborhood     = 1,

                FilterParamNumRelax         = 1,    // Relaxation

                FilterParamFullnessCut      = 1,

                FilterParamRegionMinSize    = 0,

                RegionGrowingNeighborhood   = 0,    // 6, 18 or 26
                RegionGrowingTolerance,             // Z-score distance of central voxels compared to stats, smaller value -> more compact regions
                RegionGrowingLocalStatsWidth,       // width for local stats
                RegionGrowingLessNeighborsThan,     // value in [0..1], smaller value -> more growth
                RegionGrowingMaxIterations,         // max number of iterations

                FilterParamBiasFieldRepeat  = 0,

                FilterParamGreyType         = 0,
                FilterParamGreyAxis         = 1,
                FilterParamGreyOrigin       = 2,

                FilterParamWhiteDetails     = 0,

                FilterParamTissuesParams    = 0,

                FilterParamSkullStrippingMethod     = 0,
                FilterParamSkullStrippingVoxelSize  = 1,
                FilterParamSkullStrippingIsTemplate = 2,

                FilterParamBrainstemMethod          = 0,
                FilterParamBrainstemVoxelSize       = 1,

                FilterParamEnvelopeWidth    = 0,
                };


class       FilterPresetSpec
{
public:
    FilterTypes         Code;
    char                Text [ 64 ];
    char                Ext  [ 32 ];
};

extern const FilterPresetSpec   FilterPresets[ NumFilterTypes ];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Coding the memory format of a variable (byte, integer, float...)
enum                AtomFormatType
                    {
                    UnknownAtomFormat,
                                        // coding by number of bits, signed & unsigned
                    AtomFormatUnsignedInt8,
                    AtomFormatSignedInt8,
                    AtomFormatUnsignedInt16,
                    AtomFormatSignedInt16,
                    AtomFormatUnsignedInt32,
                    AtomFormatSignedInt32,
                    AtomFormatUnsignedInt64,
                    AtomFormatSignedInt64,

                    AtomFormatFloat32,
                    AtomFormatFloat64,
                    AtomFormatFloat80,

                    AtomFormatComplex64,

                    NumAtomFormatType,

                                        // classic Windows types
                    AtomFormatByte              = AtomFormatUnsignedInt8,
                    AtomFormatInt               = AtomFormatSignedInt32,
                    AtomFormatLong              = AtomFormatSignedInt32,
                    AtomFormatFloat             = AtomFormatFloat32,
                    AtomFormatDouble            = AtomFormatFloat64,
//                  AtomFormatLongDouble        = AtomFormatFloat80,        // not sure about this one, this is old x87 format
                    AtomFormatComplex           = AtomFormatComplex64,      // 2 floats
                                        // handy indexes
                    AtomFormatIntegerMin        = AtomFormatUnsignedInt8,
                    AtomFormatIntegerMax        = AtomFormatSignedInt64,
                    AtomFormatFloatMin          = AtomFormatFloat32,
                    AtomFormatFloatMax          = AtomFormatFloat80,
                    AtomFormatComplexMin        = AtomFormatComplex64,
                    AtomFormatComplexMax        = AtomFormatComplex64,
                    };


class               AtomFormatClass
{
public:
    AtomFormatType          Type;                   // redundant but included for clarity
    char                    Text [ 32 ];            // 
    unsigned int            NumBytes;               // 
    unsigned int            NumBits ()      const   { return    NumBytes * 8; }
};

extern  const AtomFormatClass   AtomFormatTypePresets[ NumAtomFormatType ];


bool    IsFormatInteger ( AtomFormatType af );
bool    IsFormatFloat   ( AtomFormatType af );
bool    IsFormatComplex ( AtomFormatType af );
bool    IsSigned        ( AtomFormatType af );


//----------------------------------------------------------------------------
                                        // Coding the type (meaning) of a variable

constexpr int       ContentTypeMaxChars         = 64;
constexpr int       MaxExtraContentType         = 10;


enum                AtomType
                    {
                    UnknownAtomType,

                    AtomTypePositive,           // scalar data >= 0
                    AtomTypeAngular,            // scalar angular data iether in [0..pi] or in [-pi..pi] !it doesn't assume positive only data!
                    AtomTypeScalar,             // scalar data of any sort
                    AtomTypeComplex,            // complex value
                    AtomTypeVector,             // 3D vector (dipole)
                    AtomTypeCoordinates,        // 3D coordinates (point in 3D space)
                    AtomTypeString,             // text
                                                // Special values for retrieval purpose:
                    AtomTypeUseOriginal,        //  - will query what the OPENING type is, f.ex. 'scalar'
                    AtomTypeUseCurrent,         //  - will query what the CURRENT type is, f.ex. 'positive' after some filtering applied

                    NumAtomTypes
                    };

extern  const char  AtomNames[ NumAtomTypes ][ ContentTypeMaxChars ];


inline  bool        IsUnknownType       ( AtomType at )     { return at == UnknownAtomType;     }
inline  bool        IsUnresolvedType    ( AtomType at )     { return at == UnknownAtomType
                                                                  || at == AtomTypeUseOriginal
                                                                  || at == AtomTypeUseCurrent;  }
inline  bool        IsPositive          ( AtomType at )     { return at == AtomTypePositive;    }
inline  bool        IsAngular           ( AtomType at )     { return at == AtomTypeAngular;     }
inline  bool        IsScalar            ( AtomType at )     { return at == AtomTypeScalar;      }
inline  bool        IsComplex           ( AtomType at )     { return at == AtomTypeComplex;     }
inline  bool        IsVector            ( AtomType at )     { return at == AtomTypeVector;      }
inline  bool        IsAbsolute          ( AtomType at )     { return at == AtomTypePositive
                                                                  || at == AtomTypeVector;      }
                                        // Asking user
AtomType            GetDataType         ();


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                ReferenceType
                    {
                    UnknownReference,

                    ReferenceAsInFile,                          // used for loading EEG files "as is", without re-referencing
                    ReferenceNone           = ReferenceAsInFile,// used for computation of templates, also don't re-reference

                    ReferenceArbitraryTracks,                   // Any arbitrary selection of reference tracks

                    ReferenceAverage,                           // Average Reference, which uses all valid EEG channels, excluding Bad, Auxiliary and Pseudo-Tracks channels

                    ReferenceUsingCurrent,                      // Special value that needs to be contextually resolved, f.ex. during batch processing

                    NumReferenceTypes,
                    };

extern  const char  ReferenceNames[ NumReferenceTypes ][ 32 ];

                                        // References which actually do something
inline bool         IsEffectiveReference    ( ReferenceType ref )   { return ref == ReferenceArbitraryTracks || ref == ReferenceAverage; }


//----------------------------------------------------------------------------
                                        // Define a centralized function so that all processing that somehow
                                        // need a reference for processing will actually use the same one.
enum                ProcessingReferenceType
                    {
                    ProcessingReferenceNone,
                    ProcessingReferenceEEG,
                    ProcessingReferenceESI,

                    ProcessingReferenceAsking                   // Special value for interactive specification
                    };


ReferenceType       GetProcessingRef          ( ProcessingReferenceType how /*, AtomType datatype*/ );


//----------------------------------------------------------------------------
                                        // Having a central function to check & potentially correct the reference is quite handy, so we can see all calling code at once
                                        // If strict is enabled, it enforces no reference for Positive and Vectorial data
                                        // Otherwise, nothing is changed, the caller can modify the reference at will, f.ex. Average Reference for Correlation of Positive data
inline  void    CheckPositiveReference  ( ReferenceType& ref, AtomType at ) 
{
if ( IsPositive ( at ) 
  && ref != ReferenceAsInFile )   ref     = ReferenceAsInFile; 
}


inline  void    CheckAngularReference   ( ReferenceType& ref, AtomType at ) 
{
if ( IsAngular( at ) 
  && ref != ReferenceAsInFile )   ref     = ReferenceAsInFile; 
}


inline  void    CheckVectorReference    ( ReferenceType& ref, AtomType at ) 
{ 
if ( IsVector   ( at ) 
  && ref != ReferenceAsInFile )   ref     = ReferenceAsInFile; 
}

                                        // Runs all checks, currently only downgrades any reference to ReferenceAsInFile / ReferenceNone
inline  void    CheckReference          ( ReferenceType& ref, AtomType at ) 
{
CheckPositiveReference  ( ref, at ); 
CheckAngularReference   ( ref, at ); 
CheckVectorReference    ( ref, at ); 
}


//----------------------------------------------------------------------------
                                        // Tracks have currently 3 additional tracks
enum                PseudoTracksIndex
                    {
                    PseudoTrackOffsetGfp,
                    PseudoTrackOffsetDis,
                    PseudoTrackOffsetAvg,

                    NumPseudoTracks,
                    };


//----------------------------------------------------------------------------
                                        // How to handle maps possible polarity inversion
enum                PolarityType
                    {
                    PolarityDirect      = 0,    // !make it equivalent to false, so we can use it as a (bool) when calling TVector functions!
                    PolarityInvert      = 1,    // !  "         "         true           "                "                   "             !
                    PolarityEvaluate    = 2,    // used as INPUT parameter: polarity will be evaluated at each call, comparing a data point to a given template (output will only be the first 2 values above)

                    NumPolarityTypes
                    };

extern  const char  PolarityNames[ NumPolarityTypes ][ 64 ];

                                        // conversion functions
inline  bool        PolarityInvertToBool    ( PolarityType polarity )   { return polarity == PolarityInvert; }             // returns true if invert (PolarityType)
inline  int         PolarityToSign          ( PolarityType polarity )   { return polarity == PolarityInvert ? -1 : 1; }    // returns +-1 for Cumulate
bool                GetPolarityFromUser     ( const char* title, PolarityType& polarity );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        EpochsType
            {
            EpochNone,                  // unspecified
            EpochWholeTime,             // whole time range
            EpochsPeriodic,             // cyclic & equal epochs
            EpochsFromList,             // user specified epochs

            NumEpochsTypes,
            };

extern  const char  EpochsNames[ NumEpochsTypes ][ 32 ];


enum        SkippingEpochsType
            {
            NoSkippingBadEpochs,        // not skipping bad epochs, i.e. using whole dataset
            SkippingBadEpochsAuto,      // skip from list automatically
            SkippingBadEpochsList,      // skip from list from a given list

            NumSkippingEpochsTypes,
            };

extern  const char  SkippingEpochsNames[ NumSkippingEpochsTypes ][ 32 ];


//enum        BadEpochsAccuracy
//            {
//            BadEpochsLiberal,
//            BadEpochsRegular,
//            BadEpochsStrict,
//            };


//----------------------------------------------------------------------------

enum        GfpPeaksDetectType
            {
            NoGfpPeaksDetection,        // no GFP peaks extraction - leaving the input interval unchanged
            GfpPeaksDetectionAuto,      // GFP peaks extracted automatically
            GfpPeaksDetectionList,      // GFP peaks extracted from a given list

            NumGfpPeaksDetectionTypes,
            };

extern  const char  GfpPeaksDetectNames[ NumGfpPeaksDetectionTypes ][ 32 ];


//----------------------------------------------------------------------------

enum        ResamplingType
            {
            NoTimeResampling,
            TimeResampling,

            NumResamplingTypes,
            };

extern  const char  ResamplingNames[ NumResamplingTypes ][ 32 ];


//----------------------------------------------------------------------------

enum        PseudoTracksType
            {
            NoPseudoTracks,
            ComputePseudoTracks,
            };


//----------------------------------------------------------------------------

enum        BackgroundNormalization
            {
            BackgroundNormalizationNone,
            BackgroundNormalizationComputingZScore,
            BackgroundNormalizationLoadingZScoreFile,

            NumBackgroundNormalization,
            };

extern  const char  BackgroundNormalizationNames[ NumBackgroundNormalization ][ 64 ];


//----------------------------------------------------------------------------

enum        CentroidType
            {
            MeanCentroid,               // Average of cluster
            WeightedMeanCentroid,       // Weighted average of cluster
            MedianCentroid,             // Median of cluster
            MedoidCentroid,             // Central element of cluster
            EigenVectorCentroid,        // Eigenvector of cluster
            MaxCentroid,                // Max of cluster

            NumCentroidTypes        
            };

extern  const char  CentroidNames[ NumCentroidTypes ][ 32 ];


//----------------------------------------------------------------------------

enum        CorrelateType 
            {
            CorrelateTypeNone,
            CorrelateTypeSpatialCorrelation,
            CorrelateTypeTimeCorrelation,
            CorrelateTypeCoherence,
            CorrelateTypePhaseIntCoupling,

            CorrelateTypeAuto,
            CorrelateTypeLinearLinear,
            CorrelateTypeLinearLinearRobust,
            CorrelateTypeLinearCircular,
            CorrelateTypeLinearCircularRobust,
            CorrelateTypeCircularLinear,
            CorrelateTypeCircularLinearRobust,
            CorrelateTypeCircularCircular,

            NumCorrelateTypes,
            };

extern  const char  CorrelateTypeNames[ NumCorrelateTypes ][ 64 ];

                                        // Each type of computation has its own constraints on tracks / time / frequencies
inline  bool    CorrelateTypeSameTracks         ( CorrelateType c )     { return c == CorrelateTypeSpatialCorrelation; }

inline  bool    CorrelateTypeSameDuration       ( CorrelateType c )     { return c == CorrelateTypeTimeCorrelation 
                                                                              || c == CorrelateTypeCoherence
                                                                              || c == CorrelateTypePhaseIntCoupling; }

inline  bool    CorrelateTypeSameFrequencies    ( CorrelateType c )     { return c == CorrelateTypeTimeCorrelation 
                                                                              || c == CorrelateTypeCoherence; }


inline  bool    CorrelateTypeFrequenciesAllowed ( CorrelateType c )     { return c == CorrelateTypeTimeCorrelation 
                                                                              || c == CorrelateTypeCoherence
                                                                              || c == CorrelateTypePhaseIntCoupling; }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Enumerate all allowed regularizations, including special ones used for automatic evaluation
enum                RegularizationType
                    {
                                        // special values, which are coded negatively, so that..
                    RegularizationNone          = -3,
                    RegularizationAutoGlobal    = -2,
                    RegularizationAutoLocal     = -1,
                                        // ..these next 13 regularizations levels are equivalent to their numeric values (f.ex. Regularization7 value is 7)
                    Regularization0             = 0,    // no regularization at all
                    Regularization1,
                    Regularization2,
                    Regularization3,
                    Regularization4,
                    Regularization5,
                    Regularization6,
                    Regularization7,
                    Regularization8,
                    Regularization9,
                    Regularization10,
                    Regularization11,
                    Regularization12,
                    NumSavedRegularizations,

                    FirstRegularization         = Regularization0,
                    LastRegularization          = Regularization12,
                    };

bool                IsRegularRegularization ( RegularizationType reg );
bool                IsRegularRegularization ( const char*        reg );
bool                IsSpecialRegularization ( RegularizationType reg );

                                        // Useful strings for dialogs and files
constexpr char*     RegularizationNoneStringS           = "N";
constexpr char*     RegularizationNoneStringL           = "None";
constexpr char*     RegularizationAutoGlobalStringS     = "G";
constexpr char*     RegularizationAutoGlobalStringL     = "Global";
constexpr char*     RegularizationAutoLocalStringS      = "L";
constexpr char*     RegularizationAutoLocalStringL      = "Local";

                                        // ..and conversion utilities between strings and enums
inline  RegularizationType  StringToRegularization ( const char* regstr );
inline  char*               RegularizationToString ( RegularizationType reg, char* regstr, bool shortversion );

                                        // Used to be 6 (the middle value in [0..12]), but simulations give this:
                                        // Best regularization is 1 (for 1 dipole) to 4 (1000 dipoles), with SNRs of 10 in sources and 10 in EEG
constexpr char*     DefaultRegularization       = "4";
                                        // Frequency data is already pretty smooth
constexpr char*     DefaultRegularizationFreq   = "3";


//----------------------------------------------------------------------------
                                        // Various ways to Z-Score, according to data type and expected results
                                        // Currently: EEG should use ZScoreSigned_CenterScale; ESI default is ZScorePositive_CenterScaleOffset
enum                ZScoreType
                    {
                                        // main choices of standardization:
                    ZScoreNone                                  = 0x000000,   // no Z-Score

                    ZScorePositive_CenterScale                  = 0x000001,   // Regular Z-Score, signed results
                    ZScorePositive_CenterScaleOffset            = 0x000002,   // Regular Z-Score, then offset to be > 0 - better for actual segmentation
                    ZScorePositive_CenterScaleAbs               = 0x000004,   // Regular Z-Score, then Absolute (-Z -> +Z)
                    ZScorePositive_CenterScalePlus              = 0x000008,   // Regular Z-Score, then keeping only Z > 0 
                    ZScorePositive_NocenterScale                = 0x000010,   // Z-Score without centering (i.e. from 0), using all data variance
                    ZScorePositive_CenterScaleInvertOffset      = 0x000020,   // invert + offset, to emphasize the min's and ignore the max'es
                    ZScorePositive_Mask                         = ZScorePositive_CenterScale | ZScorePositive_CenterScaleOffset | ZScorePositive_CenterScaleAbs | ZScorePositive_CenterScalePlus | ZScorePositive_NocenterScale | ZScorePositive_CenterScaleInvertOffset, 

                    ZScoreVectorial_CenterVectors_CenterScale   = 0x000100,   // second best choice
                    ZScoreVectorial_CenterVectors_Scale         = 0x000200,   // last choice
                    ZScoreVectorial_CenterScaleByComponent      = 0x000400,   // best one for both segmentation and display
                    ZScoreVectorial_Mask                        = ZScoreVectorial_CenterVectors_CenterScale | ZScoreVectorial_CenterVectors_Scale | ZScoreVectorial_CenterScaleByComponent,

                    ZScoreSigned_CenterScale                    = 0x001000,   // regular Z-Score
                    ZScoreSigned_Mask                           = ZScoreSigned_CenterScale,

                                        // handy enums:
                    ZScoreProcessingMask                        = ZScorePositive_Mask | ZScoreVectorial_Mask | ZScoreSigned_Mask,

                    ZScorePositive_Default                      = ZScorePositive_CenterScaleOffset,
                    ZScoreVectorial_Default                     = ZScoreVectorial_CenterVectors_CenterScale,
                    ZScoreSigned_Default                        = ZScoreSigned_CenterScale,

                                        // options enums:
                    ZScoreMaxData                               = 0x010000,   // Option to scan only the local maxes/mins
                    ZScoreAllData                               = 0x020000,   // Option to scan all data (default)
                    ZScoreDimension3                            = 0x040000,   // Dimension of positive data is a norm of 3 components (3D vectorial dipole)
                    ZScoreDimension6                            = 0x080000,   // Dimension of positive data is a norm of 6 components (2 x 3D vectorial dipole, for complex input in ESI)
                    ZScoreOptionsMask                           = 0x0F0000,   // Mask for options
                    };


enum                TracksContentType;
class               TStrings;

                                        // some utilities to manage these options
inline  ZScoreType  ClearZScoreProcessing       ( ZScoreType Z )                {   return  (ZScoreType) ( Z & ~ZScoreProcessingMask ); }
inline  ZScoreType  ClearZScoreOptions          ( ZScoreType Z )                {   return  (ZScoreType) ( Z & ~ZScoreOptionsMask    ); }
inline  ZScoreType  GetZScoreProcessing         ( ZScoreType Z )                {   return  (ZScoreType) ( Z &  ZScoreProcessingMask ); }
inline  ZScoreType  GetZScoreOptions            ( ZScoreType Z )                {   return  (ZScoreType) ( Z &  ZScoreOptionsMask    ); }
                                        // safely composing from processing and options
inline  ZScoreType  SetZScore                   ( ZScoreType ZP, ZScoreType ZO ){   return  (ZScoreType) ( GetZScoreProcessing ( ZP ) | GetZScoreOptions ( ZO ) ); }

inline  bool        IsZScore                    ( ZScoreType Z )                {   return  GetZScoreProcessing ( Z ) != ZScoreNone;    }
inline  bool        IsZScorePositive            ( ZScoreType Z )                {   return  Z & ZScorePositive_Mask;                    }
inline  bool        IsZScoreVectorial           ( ZScoreType Z )                {   return  Z & ZScoreVectorial_Mask;                   }
inline  bool        IsZScoreSigned              ( ZScoreType Z )                {   return  Z & ZScoreSigned_Mask;                      }
inline  bool        IsZScoreScalar              ( ZScoreType Z )                {   return  ! IsZScoreVectorial ( Z );                  }   // norm and signed, which are both scalar values

const char*         ZScoreEnumToString          ( ZScoreType z );
const char*         ZScoreEnumToInfix           ( ZScoreType z );
const char*         ZScoreEnumToFactorFileInfix ( ZScoreType z );
void                ZScoreEnumToFactorNames     ( ZScoreType z, TStrings&    zscorenames );
void                DataTypeToZScoreEnum        ( AtomType         datatype,           TracksContentType    contenttype,
                                                  bool*            askedsignedzscore,  ZScoreType&          timezscore );

constexpr int       NumZValuesCenter        = 0;
constexpr int       NumZValuesSpread        = 1;
constexpr int       NumZValuesCalibration   = 2;
constexpr int       NumZMatrixCalibration   = 9;

                                        // how many SD to keep, for positive data, by shifting the Z-Score
                                        // 3 seems a good compromise for the vast majority of cases
constexpr double    MinSDToKeep             = 3;
#define             MinSDToKeepS            "3"

                                        // handy conversion functions to make a Zs value into a positive-only equivalent Zp, and vice-versa
template <class TypeD>
inline  TypeD       ZSignedToZPositive      ( TypeD& zsigned )                  { zsigned   = AtLeast ( 0.0, zsigned / MinSDToKeep + 1.0 ); return zsigned; }
template <class TypeD>
inline  TypeD       ZPositiveToZSigned      ( TypeD& zpos    )                  { zpos      = ( zpos - 1.0 )         * MinSDToKeep;         return zpos;    }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // All known Content Types
enum                ContentType
                    {
                    UnknownContentType,
                    ContentTypeEeg,                     // EEG data
                    ContentTypeRis,                     // Inverse Space data
                    ContentTypeSeg,                     // Segmentation
                    ContentTypeData,                    // Data (results)
                    ContentTypeErrorData,               // Results from clustering - used so often it has its own type
                    ContentTypeHistogram,               // Histogram
                    ContentTypeFreq,                    // Frequency space
//                  ContentTypeIca,                     // ICA
                    ContentTypeSp,                      //
                    ContentTypeElectrodes,              //
                    ContentTypeVolume,                  // MRI
                    ContentTypeLeadField,               // Matrix of Lead Field
                    ContentTypeMatrix,                  // Matrix of IS
                    ContentTypeRoi,                     //

                    NumContentType
                    };

extern  const char  ContentNames[ NumContentType ][ ContentTypeMaxChars ];


//----------------------------------------------------------------------------
                                        // Used for electrodes / points dimensionality
enum                ClusterType
                    {
                    ClusterTypeUnknown      = -1,
                    ClusterPoint            = 0,    // value gives roughly the dimensionality of cluster: point=0 line=1 grid=2 3D=3
                    ClusterLine,
                    ClusterGrid,
                    Cluster3D,

                    NumClusterType
                    };


//----------------------------------------------------------------------------
                                        // Which type of alternative dataset
enum                DualDataType
                    {
                    NoDualData,
                    DualEeg,
                    DualMeg,
                    DualRis,

                    NumDualDataTypes
                    };


class               DualDataSpec
{
public:
    DualDataType    Code;
    char            Text            [ 64 ];
    AtomType        DataType;
    char            SavingExtension [  8 ];
};

extern  const DualDataSpec  DualDataPresets[ NumDualDataTypes ];


//----------------------------------------------------------------------------
                                        // Centralize basic informations about the data
class   TDataFormat
{
public:
                    TDataFormat ();


    void            Reset ();

                                        // !Set OriginalAtomType only once, but CurrentAtomType as many times as needed!
    virtual void    SetAtomType         ( AtomType at )             { if ( IsUnknownType ( OriginalAtomType ) )     OriginalAtomType  = at;
                                                                    /*if ( IsUnknownType ( CurrentAtomType  ) )*/   CurrentAtomType   = at; }

                                        // Resolve any AtomTypeUseXXX type, or just return the given type (which shouldn't be a type to be resolved either)
    virtual AtomType GetAtomType        ( AtomType at )     const   { return  at == AtomTypeUseOriginal ?   OriginalAtomType        
                                                                            : at == AtomTypeUseCurrent  ?   CurrentAtomType        
                                                                            :                               at;             }
    const char*     GetAtomTypeName     ( AtomType at )     const   { return  AtomNames[ GetAtomType ( at ) ];              }

                                        // Force the caller to specify either Original, Current, or any other type
    bool            IsUnknownType       ( AtomType at )     const   { return crtl::IsUnknownType ( GetAtomType ( at ) );    }
    bool            IsPositive          ( AtomType at )     const   { return crtl::IsPositive    ( GetAtomType ( at ) );    }
    bool            IsAngular           ( AtomType at )     const   { return crtl::IsAngular     ( GetAtomType ( at ) );    }
    bool            IsScalar            ( AtomType at )     const   { return crtl::IsScalar      ( GetAtomType ( at ) );    }
    bool            IsComplex           ( AtomType at )     const   { return crtl::IsComplex     ( GetAtomType ( at ) );    }
    bool            IsVector            ( AtomType at )     const   { return crtl::IsVector      ( GetAtomType ( at ) );    }
    bool            IsAbsolute          ( AtomType at )     const   { return crtl::IsAbsolute    ( GetAtomType ( at ) );    }


    virtual const char*     GetContentTypeName  ( char* s ) const;

    int             GetContentType      ()                  const   { return ContentType;              }
    int             GetExtraContentType ()                  const   { return ExtraContentType;         }
    bool            IsContentType       ( int type )        const   { return ContentType      == type; }
    bool            IsExtraContentType  ( int type )        const   { return ExtraContentType == type; }


protected:
                                        // These fields are quite sensitive and belong to object owners
    AtomType        OriginalAtomType;   // Set only once as the original data type
    AtomType        CurrentAtomType;    // Set as many times as data is processed / filtered

    int             ContentType;        // meaning of data, like EEG, Frequencies, Histogram... - note that it is sometimes hard to have full confidence of the content's type, so it is used more like a strong hint
    int             ExtraContentType;   // additional meaning, like ERP, Raw EEG, Templates...
    char            ExtraContentTypeNames[ MaxExtraContentType ][ ContentTypeMaxChars ];
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
