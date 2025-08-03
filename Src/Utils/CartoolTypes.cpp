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

#include    "Dialogs.Input.h"
#include    "Math.Utils.h"
#include    "TVolume.h"     // used to compile extern variables
#include    "CartoolTypes.h"

#include    "TTracksDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Every filter long and short descriptions strings
const FilterPresetSpec  FilterPresets[ NumFilterTypes ] =
            {
            {   FilterTypeNone,                 "No Filter",                            "NoFilter"                  },

            {   FilterTypeMean,                 "Mean Filter",                          "Mean"                      },
            {   FilterTypeGaussian,             "Gaussian Filter",                      "Gaussian"                  },
            {   FilterTypeFastGaussian,         "Fast Gaussian Filter",                 "FastGaussian"              },
            {   FilterTypeHighPassLight,        "High Pass Filter",                     "HighPass"                  },
            {   FilterTypeLoG,                  "LoG Filter",                           "LoG"                       },
            {   FilterTypeAnisoGaussian,        "Anisotrop. Gaussian Filter",           "AnisoGaussian"             },

            {   FilterTypeLowPass,              "Butterworth Low-Pass",                 "LowPass"                   },
            {   FilterTypeHighPass,             "Butterworth High-Pass",                "HighPass"                  },
            {   FilterTypeBandPass,             "Butterworth Band-Pass",                "BandPass"                  },
            {   FilterTypeBandStop,             "Butterworth Band-Stop",                "BandStop"                  },

            {   FilterTypeIntensityAdd,         "Intensity Adding",                     "Add"                       },
            {   FilterTypeIntensitySub,         "Intensity Subtracting",                "Sub"                       },
            {   FilterTypeIntensityMult,        "Intensity Multiplying",                "Mult"                      },
            {   FilterTypeIntensityDiv,         "Intensity Dividing",                   "Div"                       },
            {   FilterTypeIntensityOpposite,    "Intensity Opposite",                   "Opposite"                  },
            {   FilterTypeIntensityInvert,      "Intensity Invert",                     "Invert"                    },
            {   FilterTypeIntensityAbs,         "Intensity Absolute",                   "Abs"                       },
            {   FilterTypeIntensityNorm,        "Intensity Normalization",              "Norm"                      },

            {   FilterTypeMin,                  "Min Filter",                           "Min"                       },
            {   FilterTypeMax,                  "Max Filter",                           "Max"                       },
            {   FilterTypeMinMax,               "Min or Max Filter",                    "MinOrMax"                  },
            {   FilterTypeRange,                "Range Filter",                         "Range"                     },
            {   FilterTypeMedian,               "Median Filter",                        "Median"                    },
            {   FilterTypeInterquartileMean,    "Interquartile Mean",                   "IQM"                       },
            {   FilterTypeMaxMode,              "Max Mode Filter",                      "MaxMode"                   },
            {   FilterTypeMeanSub,              "Mean Sub Filter",                      "MeanSub"                   },
            {   FilterTypeLogMeanSub,           "Log Mean Sub Filter",                  "LogMeanSub"                },
            {   FilterTypeMeanDiv,              "Mean Div Filter",                      "MeanDiv"                   },
            {   FilterTypeLogMeanDiv,           "Log Mean Div Filter",                  "LogMeanDiv"                },
            {   FilterTypeMAD,                  "Median Abs. Dev. (MAD)",               "MAD"                       },
            {   FilterTypeSD,                   "SD Filter",                            "SD"                        },
            {   FilterTypeSDInv,                "Inverted SD Filter",                   "InvSD"                     },
            {   FilterTypeCoV,                  "Coefficient of Variation",             "CoV"                       },
            {   FilterTypeLogCoV,               "Log Coeff. of Variation",              "LogCoV"                    },
            {   FilterTypeLogSNR,               "Log Signal to Noise Ratio",            "LogSNR"                    },
            {   FilterTypeMCoV,                 "Med. Coeff. of Variation",             "MedCoV"                    },
            {   FilterTypeLogMCoV,              "Log Med. Coeff. Variation",            "LogMedCoV"                 },
            {   FilterTypeMADSDInv,             "Inverted MAD + SD Filter",             "InvMadSD"                  },
            {   FilterTypeModeQuant,            "Mode Quantization",                    "ModeQuant"                 },
            {   FilterTypeEntropy,              "Entropy Filter",                       "Entropy"                   },
            {   FilterTypeRank,                 "Ranking Filter",                       "Rank"                      },
            {   FilterTypeRankRamp,             "Ranking Ramp Filter",                  "RankRamp"                  },

            {   FilterTypeThreshold,            "Threshold Filter",                     "Thresh"                    },
            {   FilterTypeThresholdAbove,       "Threshold Above Filter",               "ThreshAbove"               },
            {   FilterTypeThresholdBelow,       "Threshold Below Filter",               "ThreshBelow"               },
            {   FilterTypeThresholdBinarize,    "Threshold & Binarize",                 "ThreshBin"                 },
            {   FilterTypeIntensityRemap,       "Intensity Remapping",                  "Remap"                     },
            {   FilterTypeKeepValue,            "Keeping Value",                        "Keep"                      },
            {   FilterTypeBinarize,             "Binarize Filter",                      "Binarize"                  },
            {   FilterTypeRevert,               "Revert Filter",                        "Revert"                    },
            {   FilterTypeToMask,               "Converting to Mask",                   "ToMask"                    },
            {   FilterTypeSymmetrize,           "Symmetrize volume",                    "Sym"                       },

            {   FilterTypeHistoEqual,           "Histogram Equalization",               "HistoEqu"                  },
            {   FilterTypeHistoEqualBrain,      "Histogram Brain Equal.",               "HistoEquBrain"             },
            {   FilterTypeHistoCompact,         "Histogram Compaction",                 "HistoCompact"              },

            {   FilterTypeErode,                "Erode Filter",                         "MorphErode"                },
            {   FilterTypeDilate,               "Dilate Filter",                        "MorphDilate"               },
            {   FilterTypeOpen,                 "Open Filter",                          "MorphOpen"                 },
            {   FilterTypeClose,                "Close Filter",                         "MorphClose"                },
            {   FilterTypeMorphGradient,        "Morphology Gradient Filter",           "MorphGradient"             },
            {   FilterTypeMorphGradientInt,     "Morphology Internal Gradient Filter",  "MorphIntGradient"          },
            {   FilterTypeMorphGradientExt,     "Morphology External Gradient Filter",  "MorphExtGradient"          },

            {   FilterTypeThinning,             "Thinning Filter",                      "MorphThin"                 },
            {   FilterTypeWaterfallRidges,      "Waterfall Ridges",                     "MorphRidges"               },
            {   FilterTypeWaterfallValleys,     "Waterfall Valleys",                    "MorphValleys"              },
            {   FilterTypeLessNeighbors,        "Less Neighbors Filter",                "LessNeighbors"             },
            {   FilterTypeMoreNeighbors,        "More Neighbors Filter",                "MoreNeighbors"             },
            {   FilterTypeRelax,                "Relaxation Filter",                    "Relaxation"                },
            {   FilterTypeKeepBiggestRegion,    "Keep Biggest Region",                  "BiggestRegion"             },
            {   FilterTypeKeepCompactRegion,    "Keep Most Compact Region",             "MostCompactRegion"         },
            {   FilterTypeKeepBiggest2Regions,  "Keep Biggest 2 Regions",               "Biggest2Regions"           },
            {   FilterTypeKeepCompact2Regions,  "Keep Most Compact 2 Regions",          "MostCompact2Regions"       },

            {   FilterTypePercentFullness,      "Percentage of Fullness",               "PercentFullness"           },
            {   FilterTypeCutBelowFullness,     "Cut Below Fullness",                   "CutBelowFullness"          },
            {   FilterTypeSAV,                  "Volume Fragmentation",                 "VolFragment"               },
            {   FilterTypeCompactness,          "Volume Compactness",                   "VolCompact"                },

            {   FilterTypeGradient,             "Gradient Filter",                      "Gradient"                  },
            {   FilterTypeCanny,                "Canny Filter",                         "Canny"                     },
            {   FilterTypeLaplacian,            "Laplacian Filter",                     "Laplacian"                 },
            {   FilterTypeHessianEigenMax,      "Hessian Max Eigenvalue",               "HessianMaxEigen"           },
            {   FilterTypeHessianEigenMin,      "Hessian Min Eigenvalue",               "HessianMinEigen"           },
            {   FilterTypeKCurvature,           "k Curvature Filter",                   "kCurvature"                },
            {   FilterTypeKCCurvature,          "kC Curvature Filter",                  "kCCurvature"               },
            {   FilterTypePeeling,              "Peeling Filter",                       "Peeling"                   },

            {   FilterTypeClustersToRegions,    "Clusters To Regions",                  "ClustersToRegions"         },
            {   FilterTypeRegionGrowing,        "Region Growing",                       "RegionGrowing"             },

            {   FilterTypeHeadCleanup,          "Cleaning-up Full Head",                "Clean"                     },
            {   FilterTypeBiasField,            "Correcting Bias Field",                InfixBiasFieldCorrection    },
            {   FilterTypeSegmentCSF,           "Extracting CSF",                       InfixCsf                    },
            {   FilterTypeSegmentGrey,          "Computing Grey Mask",                  InfixGrey                   },
            {   FilterTypeSegmentWhite,         "Extracting White Matter",              InfixWhite                  },
            {   FilterTypeSegmentTissues,       "Segmenting Tissues",                   InfixTissues                },
            {   FilterTypeSkullStripping,       "Skull-Stripping",                      InfixBrain                  },
            {   FilterTypeBrainstemRemoval,     "Removing Brainstem",                   "NoBrainstem"               },
            {   FilterTypeMaskToSolutionPoints, "Brain to Solution Points",             "SolPoints"                 },

            {   FilterTypeAntialiasing,         "Antialiasing Filter",                  "Antialias"                 },

            {   FilterTypeEnvelopeSlidingWindow,"Sliding Window Envelope",              "EnvWindow"                 },
            {   FilterTypeEnvelopePeak,         "Peak Envelope",                        "EnvPeak"                   },
            {   FilterTypeEnvelopeAnalytic,     "Analytic Envelope",                    "EnvAnalytic"               },

            {   FilterTypeResize,               "Resizing",                             "Resizing"                  },

            };


//----------------------------------------------------------------------------
                                        // "Hosting" these definitions for TVolume.h
NeighborhoodClass   Neighborhood[ NumNeighborhood ] =
                {
                {   Neighbors6,     "6 Neighbors",      6,                  1,       (                 1   + sqrt ( (double) 2 ) ) / 2.0   },
                {   Neighbors18,    "18 Neighbors",    18,  sqrt ( (double) 2 ),     ( sqrt ( (double) 2 ) + sqrt ( (double) 3 ) ) / 2.0   },
                {   Neighbors26,    "26 Neighbors",    26,  sqrt ( (double) 3 ),     ( sqrt ( (double) 3 ) +                 2   ) / 2.0   },
                };


char            SkullStrippingNames[ NumSkullStrippingTypes ][ 16 ] =
                {
                { "None"      },
                { "Method 1A" },
                { "Method 1B" },
                { "Method 2"  },
                { "Method 3"  },
                };


//----------------------------------------------------------------------------

ExecFlags   SetSilent       (       ExecFlags& ef )     { return SetFlags ( ef, SilentMask, Silent      ); }
ExecFlags   SetInteractive  (       ExecFlags& ef )     { return SetFlags ( ef, SilentMask, Interactive ); }
bool        IsSilent        ( const ExecFlags  ef )     { return IsFlag   ( ef,             Silent      ); }
bool        IsInteractive   ( const ExecFlags  ef )     { return IsFlag   ( ef,             Interactive ); }


//----------------------------------------------------------------------------

const char  EpochsNames[ NumEpochsTypes ][ 32 ] =
            {
            "Unspecified epoch",
            "Whole time line",
            "Periodic epochs",
            "Epochs from list",
            };


const char  SkippingEpochsNames[ NumSkippingEpochsTypes ][ 32 ] =
            {
            "No bad epochs",
            "Skipping bad epochs, automatic",
            "Skipping bad epochs, from list",
            };


//----------------------------------------------------------------------------

const char  GfpPeaksDetectNames[ NumGfpPeaksDetectionTypes ][ 32 ] =
            {
            "No GFP Peaks extraction",
            "GFP Peaks extraction, automatic",
            "GFP Peaks extraction, from list",
            };


//----------------------------------------------------------------------------

const char  ResamplingNames[ NumResamplingTypes ][ 32 ] =
            {
            "No resampling",
            "Resampling",
            };


//----------------------------------------------------------------------------

const char  BackgroundNormalizationNames[ NumBackgroundNormalization ][ 64 ] =
            {
            "No Standardization",
            "Computing Standardization Factors",
            "Loading Standardization Factors files",
            };


//----------------------------------------------------------------------------

const char  CentroidNames[ NumCentroidTypes ][ 32 ] = {
            "Mean Centroid",
            "Weighted Mean Centroid",
            "Median Centroid",
            "Medoid Centroid",
            "Eigenvector Centroid",
            "Max Centroid",
            };


//----------------------------------------------------------------------------

const char  CorrelateTypeNames[ NumCorrelateTypes ][ 64 ] =
            {
            "No Correlation/Coherence",
            "Spatial Correlation",
            "Time Correlation",
            "Time Coherence",
            "Phase-Intensity Coupling",

            "Correlation Automatic",
            "LinLin",                // "Correlation Linear-Linear",
            "LinLinR",               // "Correlation Linear-Linear Robust",
            "LinCir",                // "Correlation Linear-Circular",
            "LinCirR",               // "Correlation Linear-Circular Robust",
            "CirLin",                // "Correlation Circular-Linear",
            "CirLinR",               // "Correlation Circular-Linear Robust",
            "CirCir",                // "Correlation Circular-Circular",
            };


//----------------------------------------------------------------------------
                                        // Definitions
const AtomFormatClass   AtomFormatTypePresets[ NumAtomFormatType ] =
                    {
                    {   UnknownAtomFormat,          "Unknown format",               0   },

                    {   AtomFormatUnsignedInt8,     "Unsigned byte",                1   },
                    {   AtomFormatSignedInt8,       "Signed byte",                  1   },
                    {   AtomFormatUnsignedInt16,    "Unsigned short integer",       2   },
                    {   AtomFormatSignedInt16,      "Signed short integer",         2   },
                    {   AtomFormatUnsignedInt32,    "Unsigned integer",             4   },
                    {   AtomFormatSignedInt32,      "Signed integer",               4   },
                    {   AtomFormatUnsignedInt64,    "Unsigned long integer",        8   },
                    {   AtomFormatSignedInt64,      "Signed long integer",          8   },

                    {   AtomFormatFloat32,          "Single floating point",        4   },
                    {   AtomFormatFloat64,          "Double floating point",        8   },
                    {   AtomFormatFloat80,          "Long double floating point",   10  },

                    {   AtomFormatComplex64,        "Complex",                      8   },
                    };


const char          AtomNames[ NumAtomTypes ][ ContentTypeMaxChars ] = 
                    {
                    "Data type is unknown",
                    "Data is positive",
                    "Data is angular values",
                    "Data is signed",
                    "Data is complex",
                    "Data is 3D vectors",
                    "Data is 3D points",
                    "Data is strings",
                    "Use original data type"
                    "Use current data type"
                    };


const char          ReferenceNames[ NumReferenceTypes ][ 32 ] = 
                    {
                    "Unknown Reference",
                    "No Reference",
                    "Arbitrary Track(s) Reference",
                    "Average Reference",
                    "Current Reference",
                    };


const char          ContentNames[ NumContentType ][ ContentTypeMaxChars ] = 
                    {
                    "Content Type not specified",
                    "EEG",
                    "Results of Inverse Solution",
                    "Segmentation",
                    "Data file",
                    "Error.data file",
                    "Histogram",
                    "Frequencies",
//                  "ICA",
                    "Solution Points",
                    "Electrodes",
                    "Volume",
                    "Lead Field",
                    "Matrix",
                    "ROIs",
                    };


const DualDataSpec  DualDataPresets[ NumDualDataTypes ] =
                    {
                    { NoDualData,       "No alternative dataset",                   UnknownAtomType,    ""              },
                    { DualEeg,          "EEG alternative dataset",                  AtomTypeScalar,     FILEEXT_EEGSEF  },
                    { DualMeg,          "MEG alternative dataset",                  AtomTypePositive,   FILEEXT_EEGSEF  },
                    { DualRis,          "Inverse Solution alternative dataset",     AtomTypePositive,   FILEEXT_RIS     },

                    };


//----------------------------------------------------------------------------

bool    IsFormatInteger ( AtomFormatType af )   { return  IsInsideLimits ( af,  AtomFormatIntegerMin,   AtomFormatIntegerMax ); }
bool    IsFormatFloat   ( AtomFormatType af )   { return  IsInsideLimits ( af,  AtomFormatFloatMin,     AtomFormatFloatMax   ); }
bool    IsFormatComplex ( AtomFormatType af )   { return  IsInsideLimits ( af,  AtomFormatComplexMin,   AtomFormatComplexMax ); }
bool    IsSigned        ( AtomFormatType af )   { return  ! ( af == AtomFormatUnsignedInt8 || af == AtomFormatUnsignedInt16 || af == AtomFormatUnsignedInt32 || af == AtomFormatUnsignedInt64 ); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
AtomType        GetDataType ()
{
//char            answer      = GetOptionFromUser ( "Type of data:\n\t(P)ositive\n\t(A)ngular\n\t(S)calar\n\t(C)omplex\n\t(V)ectorial",
//                                                  "Data Type", "P A S C V", "S" );
char            answer      = GetOptionFromUser ( "Type of data:\n\t(P)ositive\n\t(V)ectorial",
                                                  "Data Type", "P V", "P" );

if ( answer == EOS )   return  UnknownAtomType;


AtomType            datatype;


datatype    = answer == 'P' ? AtomTypePositive
            : answer == 'A' ? AtomTypeAngular
            : answer == 'S' ? AtomTypeScalar
            : answer == 'C' ? AtomTypeComplex
            : answer == 'V' ? AtomTypeVector
            :                 UnknownAtomType;


return  datatype;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // It is necessary to have a centralized function to define which reference is used for
                                        // the template matching, no reference or average reference.
                                        // This can depend on the preprocessing, like ESI or Z-Score, and the type of data,
                                        // signed, positive, vectorial or norm of vectors.
ReferenceType   GetProcessingRef ( ProcessingReferenceType how /*, AtomType datatype*/ )
{
ReferenceType       processingref;


if ( how == ProcessingReferenceAsking ) {

    char                answer          = GetOptionFromUser ( "Apply some (A)verage Reference, or (N)o Reference?", 
                                                              "Centroids Computation", "A N", "A" );

    processingref   = answer == 'A' ?   ReferenceAverage
                    :                   ReferenceNone;
    }

else                                    
                                        // Basically AvgRef all the time
    processingref   = how == ProcessingReferenceNone  ? ReferenceNone
                    : how == ProcessingReferenceEEG   ? ReferenceAverage
                    : how == ProcessingReferenceESI   ? ReferenceAverage
                    :                                   ReferenceAverage;

                                        // Allow any sort of re-referencing
//CheckReference ( processingref, datatype );


return  processingref;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char          PolarityNames[ NumPolarityTypes ][ 64 ] = 
                    {
                    "Data directly summed up, no inversion test",
                    "Data inverted first, then summed up",
                    "Data inversion evaluated first, then summed up",
                    };


                                        // !Can return an unchanged timezscore - resetting should be done by the caller!
                                        // Recognized content types:
                                        // TracksContentERP, TracksContentEEGRecording, TracksContentFrequency, TracksContentUnknown to ask user
bool            GetPolarityFromUser (   const char*     title,  PolarityType&       polarity    )
{
polarity    = PolarityDirect;

char                answer          = GetOptionFromUser (   "Maps polarity:" NewLine NewLine Tab "- (E)RP case, accounting for polarities" NewLine Tab "- (S)pontaneous case, disregarding polarities", 
                                                            title, "E S", 0 );

if ( answer == EOS )   return  false;


polarity    = answer == 'E' ?   PolarityDirect
            :         /*'S'*/   PolarityEvaluate;


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool    IsRegularRegularization ( RegularizationType reg )  { return IsInsideLimits ( reg, FirstRegularization, LastRegularization ); }
bool    IsRegularRegularization ( const char*        reg )  { return IsRegularRegularization ( (RegularizationType) StringToInteger ( reg ) ); }
bool    IsSpecialRegularization ( RegularizationType reg )  { return ! IsRegularRegularization ( reg ); }


//----------------------------------------------------------------------------
                                        // convert ZScoreType to string - we can't have a string table here
const char*     ZScoreEnumToString ( ZScoreType z )
{
switch ( GetZScoreProcessing ( z ) ) {

    case    ZScoreNone:                                 return  "No standardization";

    case    ZScorePositive_CenterScale:                 return  "Z-Score ( positive data )";
    case    ZScorePositive_CenterScaleOffset:           return  "Z-Score ( positive data ), then offset of " MinSDToKeepS " sigma";
    case    ZScorePositive_CenterScaleAbs:              return  "Z-Score ( positive data ), then absolute value";
    case    ZScorePositive_CenterScalePlus:             return  "Z-Score ( positive data ), then keeping positive part";
    case    ZScorePositive_NocenterScale:               return  "Data rescaled by SD (no centering)";
    case    ZScorePositive_CenterScaleInvertOffset:     return  "Z-Score ( positive data ), then inverted and offset of " MinSDToKeepS " sigma";

    case    ZScoreVectorial_CenterVectors_CenterScale:  return  "Vector rescaled by ( Z-Score ( Norm ), then offset of " MinSDToKeepS " sigma )";
    case    ZScoreVectorial_CenterVectors_Scale:        return  "Vector rescaled by its Norm's SD (no centering)";
    case    ZScoreVectorial_CenterScaleByComponent:     return  "Vector with Z-Score done on each component";

    case    ZScoreSigned_CenterScale:                   return  "Z-Score";

    default:                                            return  "Unknwown standardization";
    }
}


//----------------------------------------------------------------------------
                                        // returns the string '.ZScore' + some optional letter
const char*     ZScoreEnumToInfix ( ZScoreType z )
{
switch ( GetZScoreProcessing ( z ) ) {

    case    ZScoreNone:                                 return  "";

    case    ZScorePositive_CenterScale:                 return  PostfixStandardizationZScore "PS";
    case    ZScorePositive_CenterScaleOffset:           return  PostfixStandardizationZScore "PO";
    case    ZScorePositive_CenterScaleAbs:              return  PostfixStandardizationZScore "PA";
    case    ZScorePositive_CenterScalePlus:             return  PostfixStandardizationZScore "P+";
    case    ZScorePositive_NocenterScale:               return  PostfixStandardizationZScore "PK";
    case    ZScorePositive_CenterScaleInvertOffset:     return  PostfixStandardizationZScore "PIO";

    case    ZScoreVectorial_CenterVectors_CenterScale:  return  PostfixStandardizationZScore "VN";
    case    ZScoreVectorial_CenterVectors_Scale:        return  PostfixStandardizationZScore "VK";
    case    ZScoreVectorial_CenterScaleByComponent:     return  PostfixStandardizationZScore "VC";

    case    ZScoreSigned_CenterScale:                   return  PostfixStandardizationZScore "SS";

    default:                                            return  PostfixStandardizationZScore;
    }
}


//----------------------------------------------------------------------------
                                        // returns the string correct factor file infix
const char*     ZScoreEnumToFactorFileInfix ( ZScoreType z )
{
switch ( GetZScoreProcessing ( z ) ) {

    case    ZScoreNone:                                 return  "";
    default:                                            return  InfixStandardizationZScoreFactors;
    }
}


void    ZScoreEnumToFactorNames (   ZScoreType      z,  TStrings&       zscorenames )
{
zscorenames.Reset ();

switch ( GetZScoreProcessing ( z ) ) {

    case    ZScoreNone:
        break;

    default:
                                        // just saving the Center (Mode) and the Left SD (MAD) of distribution
        zscorenames.Add ( InfixStandardizationZScoreFactorsCenter );
        zscorenames.Add ( InfixStandardizationZScoreFactorsSpread );
    }
}


//----------------------------------------------------------------------------
                                        // !Can return an unchanged timezscore - resetting should be done by the caller!
                                        // Recognized content types:
                                        // TracksContentERP, TracksContentEEGRecording, TracksContentFrequency, TracksContentUnknown to ask user
void        DataTypeToZScoreEnum ( AtomType         datatype,           TracksContentType       contenttype,
                                   bool*            askedsignedzscore,  ZScoreType&             timezscore )
{
bool                asking          = ! askedsignedzscore || ! *askedsignedzscore;

if ( IsUnknownType ( datatype ) && asking )

    datatype    = GetDataType ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // work on a local variable
ZScoreType          zsp;


if      ( IsVector   ( datatype ) ) {

    if ( contenttype == TracksContentUnknown && asking ) {
                                        // reset variable, so any further call will not be asking anything
        if ( askedsignedzscore )
            *askedsignedzscore  = true;

        char            answer      = GetOptionFromUser (   "Data is vectorial, select the type of standardization:" NewLine 
                                                            NewLine 
                                                            Tab "(N)orm of the vectors" NewLine 
                                                            Tab "(E)ach X,Y,Z component independently", 
                                                            "Tracks Standardization", "N E", "N" );

        if ( answer == EOS )   return;


        zsp     = answer == 'N' ? ZScoreVectorial_CenterVectors_CenterScale
                : answer == 'E' ? ZScoreVectorial_CenterScaleByComponent
                :                 ZScoreNone;
        }

    else

        zsp     = ZScoreVectorial_Default;

    } // IsVector


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( IsPositive ( datatype ) ) {
                                        // standardized positive data (ESI) are now basically all positive + offset
    if      ( contenttype == TracksContentERP
           || contenttype == TracksContentFrequency
           || contenttype == TracksContentEEGRecording )

            zsp     = ZScorePositive_Default;

    else { // if ( contenttype == TracksContentUnknown ) {
                                        // doesn't have an explicit type of EEG, we have to ask the user
                                        // asking all the time if control variable not provided, once if it is
        if ( asking ) {
                                        // reset variable, so any further call will not be asking anything
            if ( askedsignedzscore )
                *askedsignedzscore  = true;


            char            answer      = GetOptionFromUser (   "Data is positive, select the type of standardization:" NewLine 
                                                                NewLine 
                                                                Tab "(O)ffseted Z-Score   which remain positive" NewLine 
                                                                Tab "(S)igned Z-Score   OK but results are not positive anymore" NewLine 
                                                                Tab "(A)bsolute Z-Score" NewLine 
                                                                Tab "(R)escaling only without centering", 
                                                                "Tracks Standardization", "O S A R", "O" );

            if ( answer == EOS )   return;


            zsp     = answer == 'S' ? ZScorePositive_CenterScale
                    : answer == 'O' ? ZScorePositive_CenterScaleOffset
//                  : answer == 'P' ? ZScorePositive_CenterScalePlus
                    : answer == 'A' ? ZScorePositive_CenterScaleAbs
//                  : answer == 'I' ? ZScorePositive_CenterScaleInvertOffset
                    : answer == 'R' ? ZScorePositive_NocenterScale
                    :                 ZScoreNone;
            } // asking user
        else                            // !already asked the user, so retrieve the Z-Score from the parameter!
            zsp     = GetZScoreProcessing ( timezscore );

        } // TracksContentUnknown

    } // IsPositive


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else // if ( IsScalar ( datatype ) )    // Signed data, or fall-back for any other case
                                        // also, there is no alternative choices to ask user from
    zsp     = ZScoreSigned_Default;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // be careful to only update the type part
timezscore      = SetZScore ( zsp, timezscore );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TDataFormat::TDataFormat ()
{
Reset ();
}


void    TDataFormat::Reset ()
{
OriginalAtomType        = UnknownAtomType;
CurrentAtomType         = UnknownAtomType;

ContentType             = UnknownContentType;
ExtraContentType        = 0;
ExtraContentTypeNames[ 0 ][ 0 ] = 0;
}


const char*     TDataFormat::GetContentTypeName ( char* s )  const
{
if ( s == 0 )
    return  "";
                                         // add extra info if we have some                  then content
if ( ExtraContentType )     StringCopy ( s, ExtraContentTypeNames[ ExtraContentType ], " ", ContentNames[ ContentType ] );
else                        StringCopy ( s,                                                 ContentNames[ ContentType ] );

return  s;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
