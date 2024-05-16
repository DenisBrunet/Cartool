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

#include    <vector>

#include    "CartoolTypes.h"            // Volume

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Conductivities

                                        // Indexes of all known tissues
enum        TissuesIndex
            {
            NoTissueIndex,
                                        // Other tissues
            ScalpIndex,
            FatIndex,
            MuscleIndex,
            CsfIndex,                   // !CSF and Blood currently need to be consecutive for EstimateTissuesRadii_Segmentation!
            BloodIndex,
            EyeIndex,
            AirIndex,
                                        // Skull tissues
            SkullIndex,
            SkullCompactIndex,
            SkullSpongyIndex,
            SkullSutureIndex,
                                        // Brain tissues
            BrainIndex,
            GreyIndex,
            WhiteIndex,

            NumTissuesIndex,
                                        // A few handy ranges
            OtherMinIndex       = ScalpIndex,
            OtherMaxIndex       = AirIndex,
            SkullMinIndex       = SkullIndex,
            SkullMaxIndex       = SkullSutureIndex,
            BrainMinIndex       = BrainIndex,
            BrainMaxIndex       = WhiteIndex,
            };


class               TissuesSpec
{
public:
    TissuesIndex    Code;
    char            Text [ 16 ];        // Textual version, can include space
    char            Label[ 16 ];        // Label used for ROIs, no spaces allowed
    double          Conductivity;
};

                                        // All latest tissues conductivities
extern const TissuesSpec    TissuesSpecs        [ NumTissuesIndex ];

                                        // Not all tissues get used for all models, these predefined arrays make clear what tissues are in used
                                        // by remapping the unused ones to one that makes most sense for inverse models
extern const TissuesIndex   TissuesRemapped1    [ NumTissuesIndex ];
extern const TissuesIndex   TissuesRemapped2    [ NumTissuesIndex ];
extern const TissuesIndex   TissuesRemapped3    [ NumTissuesIndex ];
extern const TissuesIndex   TissuesRemapped4    [ NumTissuesIndex ];
extern const TissuesIndex   TissuesRemapped5    [ NumTissuesIndex ];
extern const TissuesIndex   TissuesRemappedAll  [ NumTissuesIndex ];


//----------------------------------------------------------------------------
                                        // Weighting each conductivity proportionally to each tissue -> 0.3625
                                        // Result is pretty close to the McCann average brain conductivity
#define             WeightedBrainCond           ( 0.57 * TissuesSpecs[ GreyIndex    ].Conductivity  \
                                                + 0.37 * TissuesSpecs[ WhiteIndex   ].Conductivity  \
                                                + 0.05 * TissuesSpecs[ CsfIndex     ].Conductivity  \
                                                + 0.01 * TissuesSpecs[ BloodIndex   ].Conductivity )
                                                // That should be the conductivity for Ary, which groups scalp+brain vs skull -> 0.3881
#define             WeightedBrainScalpCond      ( 0.5 * WeightedBrainCond                           \
                                                + 0.5 * TissuesSpecs[ ScalpIndex    ].Conductivity )
                                        // Used for backward compatibility with the old ratio of 15.72 : 1 @ 30 yo
constexpr double    BackwardAryBrainScalpCond   = 0.2982;


//----------------------------------------------------------------------------

constexpr double    SkullCondMinAge             =   0.0;
constexpr double    SkullCondMaxAge             = 100.0;

                                        // Age in [year] to Absolute conductivitiy in [Siemens/m]
double      AgeToSkullConductivity  ( double age );


void        SplitSkullConductivity  (
                                    double      skullcond,          
                                    double      skullcompacttospongyratio,  double      skullspongypercentage,
                                    double&     skullcompactcond,           double&     skullspongycond
                                    );


//----------------------------------------------------------------------------

class                           TSelection;

void        SplitTissuesSegmentation(
                                    const Volume&           tissues,
                                    const TSelection&       tissuessel,
                                    std::vector<Volume>&    splittissues
                                    );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







