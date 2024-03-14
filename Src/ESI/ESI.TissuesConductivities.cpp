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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "ESI.TissuesConductivities.h"
#include    "ESI.TissuesThicknesses.h"
#include    "ESI.LeadFields.h"                  // OneLeadFieldPreset

#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "Math.Utils.h"
#include    "Math.Resampling.h"
#include    "Math.Histo.h"
#include    "GlobalOptimize.Tracks.h"
#include    "TSelection.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const TissuesSpec       TissuesSpecs[ NumTissuesIndex ] =
            {
            {   NoTissueIndex       , "No tissue",      0           },

            {   ScalpIndex          , "Scalp",          0.4137      },  // McCann 2021 Weighted mean value
            {   FatIndex            , "Fat",            0.0400      },
            {   MuscleIndex         , "Muscle",         0.3243      },  // McCann 2021 Weighted mean value
            {   CsfIndex            , "CSF",            1.7358      },  // McCann 2021 Weighted mean value
            {   BloodIndex          , "Blood",          0.5737      },  // McCann 2021 Weighted mean value
            {   EyeIndex            , "Eye",            1.5100      },
            {   AirIndex            , "Air",            3e-15       },

            {   SkullIndex          , "Skull",          0.0160      },  // McCann 2021 Weighted mean value
            {   SkullCompactIndex   , "Compact skull",  0.0046      },  // McCann 2021 Weighted mean value  ?doesn't match the 3.6 ratio between spongy and compact?
            {   SkullSpongyIndex    , "Spongy skull",   0.0480      },  // McCann 2021 Weighted mean value
            {   SkullSutureIndex    , "Skull suture",   0.0266      },  // McCann 2021 Weighted mean value

            {   BrainIndex          , "Brain",          0.3841      },  // McCann 2021 Weighted mean value
            {   GreyIndex           , "Grey Matter",    0.3787      },  // McCann 2021 Weighted mean value
            {   WhiteIndex          , "White Matter",   0.1462      },  // McCann 2021 Weighted mean value
            };

                                        // Just for tests
const TissuesIndex      TissuesRemapped1[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            BrainIndex,         // ScalpIndex,
            BrainIndex,         // FatIndex,
            BrainIndex,         // MuscleIndex,
            BrainIndex,         // CsfIndex,
            BrainIndex,         // BloodIndex,
            BrainIndex,         // EyeIndex,
            BrainIndex,         // AirIndex,
            BrainIndex,         // SkullIndex,
            BrainIndex,         // SkullCompactIndex,
            BrainIndex,         // SkullSpongyIndex,
            BrainIndex,         // SkullSutureIndex,
            BrainIndex,         // BrainIndex,
            BrainIndex,         // GreyIndex,
            BrainIndex,         // WhiteIndex,
            };
                                        // Just for tests
const TissuesIndex      TissuesRemapped2[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            BrainIndex,         // ScalpIndex,
            BrainIndex,         // FatIndex,
            BrainIndex,         // MuscleIndex,
            BrainIndex,         // CsfIndex,
            BrainIndex,         // BloodIndex,
            BrainIndex,         // EyeIndex,
            SkullIndex,         // AirIndex,
            SkullIndex,         // SkullIndex,
            SkullIndex,         // SkullCompactIndex,
            SkullIndex,         // SkullSpongyIndex,
            SkullIndex,         // SkullSutureIndex,
            BrainIndex,         // BrainIndex,
            BrainIndex,         // GreyIndex,
            BrainIndex,         // WhiteIndex,
            };

const TissuesIndex      TissuesRemapped3[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            ScalpIndex,         // ScalpIndex,
            ScalpIndex,         // FatIndex,
            ScalpIndex,         // MuscleIndex,
            BrainIndex,         // CsfIndex,
            BrainIndex,         // BloodIndex,
            ScalpIndex,         // EyeIndex,
            SkullIndex,         // AirIndex,
            SkullIndex,         // SkullIndex,
            SkullIndex,         // SkullCompactIndex,
            SkullIndex,         // SkullSpongyIndex,
            SkullIndex,         // SkullSutureIndex,
            BrainIndex,         // BrainIndex,
            BrainIndex,         // GreyIndex,
            BrainIndex,         // WhiteIndex,
            };

const TissuesIndex      TissuesRemapped4[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            ScalpIndex,         // ScalpIndex,
            ScalpIndex,         // FatIndex,
            ScalpIndex,         // MuscleIndex,
            CsfIndex,           // CsfIndex,
            CsfIndex,           // BloodIndex,
            ScalpIndex,         // EyeIndex,
            SkullIndex,         // AirIndex,
            SkullIndex,         // SkullIndex,
            SkullIndex,         // SkullCompactIndex,
            SkullIndex,         // SkullSpongyIndex,
            SkullIndex,         // SkullSutureIndex,
            BrainIndex,         // BrainIndex,
            BrainIndex,         // GreyIndex,
            BrainIndex,         // WhiteIndex,
            };

const TissuesIndex      TissuesRemapped5[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            ScalpIndex,         // ScalpIndex,
            ScalpIndex,         // FatIndex,
            ScalpIndex,         // MuscleIndex,
            CsfIndex,           // CsfIndex,
            CsfIndex,           // BloodIndex,
            ScalpIndex,         // EyeIndex,
            SkullIndex,         // AirIndex,
            SkullIndex,         // SkullIndex,
            SkullIndex,         // SkullCompactIndex,
            SkullIndex,         // SkullSpongyIndex,
            SkullIndex,         // SkullSutureIndex,
            GreyIndex,          // BrainIndex,
            GreyIndex,          // GreyIndex,
            WhiteIndex,         // WhiteIndex,
            };
                                        // Identity - provide all tissues without any remapping
const TissuesIndex      TissuesRemappedAll[ NumTissuesIndex ] = 
            {
            NoTissueIndex,      // NoTissueIndex,
            ScalpIndex,         // ScalpIndex,
            FatIndex,           // FatIndex,
            MuscleIndex,        // MuscleIndex,
            CsfIndex,           // CsfIndex,
            BloodIndex,         // BloodIndex,
            EyeIndex,           // EyeIndex,
            AirIndex,           // AirIndex,
            SkullCompactIndex,  // SkullIndex,
            SkullCompactIndex,  // SkullCompactIndex,
            SkullSpongyIndex,   // SkullSpongyIndex,
            SkullSutureIndex,   // SkullSutureIndex,
            GreyIndex,          // BrainIndex,
            GreyIndex,          // GreyIndex,
            WhiteIndex,         // WhiteIndex,
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Skull absolute conductivity as a function of age
double      AgeToSkullConductivity ( double age )
{
Clipped ( age, SkullCondMinAge, SkullCondMaxAge );

                                        // Older formula, taken from first version of Hoekema 2003 (there are 2 articles, this was from the first one)
                                        // For 27 yo, this gives 0.067 - This fits nearly perfectly the suggested 1/15 ratio from Malmivuo (0.0666)
//double            relcond         = 108.394296172821 / 1000 * expl ( -0.0177803660498663 * age );

                                        // Newer formula, with new exponential fitting and absolute mean conductivity - 2021
                                        // This gives much lower absolute values than the old formula (about 3.3 times less)
                                        // Ratio to Brain is 1/16 at 30 yo, 1/19 for 40 yo
double              relcond         = 0.033 * expl ( -0.018460 * age );


return  relcond;
}


//{                                       // output age to skull conductivity
//TVector<double>         skullcond ( 1 * 101 );
//
//skullcond.Index1.IndexRatio     = 1;
//skullcond.Index1.IndexMin       = 0;
//
//for ( int i = 0; i < skullcond.GetDim (); i++ )
//    skullcond[ i ]    = AgeToSkullConductivity ( skullcond.ToReal ( i ) );
//
//skullcond.WriteFile ( "E:\\Data\\AgeToSkullConductivity.sef", "Cond" );
//}


//----------------------------------------------------------------------------
                                        // Compute the conductivities of each layer, starting from the deepest one
void        OneLeadFieldPreset::GetLayersConductivities     (   double              skullcond,          double&             skullcompactcond,       double&             skullspongycond,
                                                                TSelection&         seltissues,         TArray1<double>&    sigma 
                                                            )   const
{
GetTissuesSelection ( seltissues );

                                        // Splitting an average whole-skull conductivity into compact and spongy conductivities
SplitSkullConductivity      (   skullcond,  
                                SkullCompactToSpongyRatio,  SkullSpongyPercentage,
                                skullcompactcond,           skullspongycond
                            );

sigma.Resize ( NumLayers );

if      ( IsHuman () ) {
                                        // Conductivities are set differently for each model
    if      ( IsIsotropic3ShellSphericalAry () ) {
                                        // used to test backward compatibility - value set for a 30 years old
//      sigma[ 0 ]          = BackwardAryBrainScalpCond;
//      sigma[ 1 ]          = skullcond;
//      sigma[ 2 ]          = BackwardAryBrainScalpCond;
                                        // new average conductivity
        sigma[ 0 ]          = WeightedBrainScalpCond;
        sigma[ 1 ]          = skullcond;
        sigma[ 2 ]          = WeightedBrainScalpCond;
        }

    else if ( IsIsotropic3ShellSpherical () ) {

        sigma[ 0 ]          = WeightedBrainCond;    // merging all tissues' conductivities
        sigma[ 1 ]          = skullcond;
        sigma[ 2 ]          = TissuesSpecs[ ScalpIndex  ].Conductivity;
        }

    else if ( IsIsotropic4ShellSpherical () ) {

        sigma[ 0 ]          = WeightedBrainCond;    // same as 3-Shell, as internally it also has some CSF
        sigma[ 1 ]          = TissuesSpecs[ CsfIndex    ].Conductivity;
        sigma[ 2 ]          = skullcond;
        sigma[ 3 ]          = TissuesSpecs[ ScalpIndex  ].Conductivity;
        }

    else if ( IsIsotropic6ShellSpherical () ) {

        sigma[ 0 ]          = WeightedBrainCond;    // same as 3-Shell, as internally it also has some CSF
        sigma[ 1 ]          = TissuesSpecs[ CsfIndex    ].Conductivity;
        sigma[ 2 ]          = skullcompactcond;
        sigma[ 3 ]          = skullspongycond;
        sigma[ 4 ]          = skullcompactcond;
        sigma[ 5 ]          = TissuesSpecs[ ScalpIndex  ].Conductivity;
        }

    } // IsHuman

/*
else if ( IsMacaque () ) {

    numlayers   = 3;
    sigma.Resize ( numlayers );

    sigma[ BrainLayerIndex3Shell    ]   = TissuesSpecs[ BrainIndex    ].Conductivity;
    sigma[ SkullLayerIndex3Shell    ]   = sigma[ BrainLayerIndex3Shell ] / 15.0;    // low ratio - previously 20
    sigma[ ScalpIndex3Shell         ]   = TissuesSpecs[ BrainIndex    ].Conductivity;
    }


else if ( IsMouse () ) {

    numlayers   = 3;
    sigma.Resize ( numlayers );

    sigma[ BrainLayerIndex3Shell    ]   = TissuesSpecs[ BrainIndex    ].Conductivity;
    sigma[ SkullLayerIndex3Shell    ]   = sigma[ BrainLayerIndex3Shell ] / 10.0;    // very low ratio
    sigma[ ScalpIndex3Shell         ]   = TissuesSpecs[ BrainIndex    ].Conductivity;
    }
*/
}


//----------------------------------------------------------------------------
                                        // Ad-hoc conversion of a whole skull average conductivity into compact and spongy conductivities
                                        // It is based on average sandwhich with 55% of internal spongy bone
void        SplitSkullConductivity  (   double      skullcond,          
                                        double      skullcompacttospongyratio,  double      skullspongypercentage,
                                        double&     skullcompactcond,           double&     skullspongycond
                                    )
{
                                        // working with resistivities, as they simply add-up in a serial montage (mathematically equivalent to a geometrical weighted average, but way less readable)
double              skullresistivity        = 1 / skullcond;

                                        // formula obtained from a serial montage and replacing compact with spongy
double              skullspongyresistivity  = skullresistivity
                                            / (                                     skullspongypercentage
                                                + skullcompacttospongyratio * ( 1 - skullspongypercentage ) );
                                        // converting to the other value
double              skullcompactresistivity = skullcompacttospongyratio * skullspongyresistivity;

                                        // finally reverting to conductivities
skullcompactcond    = 1 / skullcompactresistivity;
skullspongycond     = 1 / skullspongyresistivity;
}


//----------------------------------------------------------------------------

void    SplitTissuesSegmentation    (
                                    const Volume&       tissues,
                                    const TSelection&   tissuessel,
                                    vector<Volume>&     splittissues
                                    )
{

                                        // allocate as many volumes as tissues to prepare for gradients
FctParams           p;

splittissues.resize ( NumTissuesIndex );


//for ( TIteratorSelectedForward ti ( tissuessel ); (bool) ti; ++ti ) {
for ( int ti = 0; ti < NumTissuesIndex; ti++ ) {

    if ( tissuessel.IsNotSelected ( ti ) ) {
                                        // just making sure the object is empty
        splittissues[ ti ].DeallocateMemory ();

        continue;
        }

                                        // copy original tissues    
    splittissues[ ti ]   = tissues;

                                        // keep the current one only
//  p ( FilterParamKeepValue )      = ti;
//  splittissues[ ti ].Filter ( FilterTypeKeepValue, p );
                                        // same, but assigning the same values for all masks
    p ( FilterParamThresholdMin )   = ti;
    p ( FilterParamThresholdMax )   = ti;
    p ( FilterParamThresholdBin )   = 1;                    // used as as probability after smoothing
    splittissues[ ti ].Filter ( FilterTypeThresholdBinarize, p );

                                        // doing some heavy smoothing to get rid of the sharp edges from mask
                                        // it will also allow to peek from a somewhat wide - or forgiving - depth and still returns a proper gradient
//    p ( FilterParamDiameter )       = 5; // 7;
//    splittissues[ ti ].Filter ( FilterTypeFastGaussian, p );
                                        // maybe a little more restrain in the filtering, also using anisotropy(?)
    p ( FilterParamDiameter )       = 5;
    splittissues[ ti ].Filter ( FilterTypeAnisoGaussian, p );


//    TFileName       _file;
//    StringCopy      ( _file, "E:\\Data\\Tissues.Remapped.", IntegerToString ( ti, 2 ), " ", tissuesspecs[ ti ].Text, ".Smoothed.nii" );
//    splittissues[ ti ].WriteFile ( _file );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







