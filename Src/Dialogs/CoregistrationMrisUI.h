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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     MriCoregistrationTitle      = "MRI Coregistration";


enum    CoregistrationTypes
        {
                                        // Regular, general prupose affine coregistration
        CoregistrationTrans,
        CoregistrationRotTrans,
        CoregistrationRotTransScale1,
        CoregistrationRotTransScale3,
        CoregistrationRotTransScale3Shear2,
        CoregistrationRotTransScale3Shear3,
        CoregistrationRotTransScale3Shear6,
                                        // Brain-specific affine coregistration
        CoregistrationBrainTrans,
        CoregistrationBrainRotTrans,
        CoregistrationBrainRotTransScale1,
        CoregistrationBrainRotTransScale3,
        CoregistrationBrainRotTransScale3Shear2,
        CoregistrationBrainRotTransScale3Shear3,
        CoregistrationBrainRotTransScale3Shear6,

        NumCoregistrationTypes,

        CoregistrationRegularMin= CoregistrationTrans,
        CoregistrationRegularMax= CoregistrationRotTransScale3Shear6,
        CoregistrationBrainMin  = CoregistrationBrainTrans,
        CoregistrationBrainMax  = CoregistrationBrainRotTransScale3Shear6,
        };


class       TVerboseFile;


class   CoregistrationSpecsType
{
public:
        CoregistrationTypes     Code;
        char                    Text    [ 128 ];
        char                    Choice  [   2 ];
        bool                    IsLinearTransform;
        int                     NumTranslations;
        int                     NumRotations;
        int                     NumScalings;
        int                     NumShearings;

        void                    ToVerbose ( TVerboseFile& verbose ) const;
};


extern const CoregistrationSpecsType    CoregistrationSpecs[ NumCoregistrationTypes ];


//----------------------------------------------------------------------------

class       TVolumeDoc;

void    CoregistrationMrisUI(   const TVolumeDoc*   SourceMri   = 0,
                                const TVolumeDoc*   TargetMri   = 0    
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
