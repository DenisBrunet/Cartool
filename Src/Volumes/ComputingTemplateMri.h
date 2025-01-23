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

#include    "Geometry.TPoint.h"                 // TVector3Int
#include    "CoregistrationMrisUI.h"            // CoregistrationSpecsType

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     TemplateMriTitle        = "Template MRI";


enum    BuildTemplateType
        {
        BuildTemplateSelfRef,       // 2 stages: brains are first all merged into a single average brain; the latter is then coregistered to the MNI brain
        BuildTemplateMNI,           // 1 stage:  brains are coregistered one by one to the MNI brain

        NumBuildTemplateType
        };

enum    CoregistrationTemplateTypes
        {
        CoregistrationBoot,         // parameters for booting coregistration and producing a first average brain
        CoregistrationLoop,         // parameters for already existing average brain

        NumCoregistrationTemplateTypes,
        };


extern const CoregistrationSpecsType    CoregTemplate[ NumBuildTemplateType ][ NumCoregistrationTemplateTypes ];


enum    TemplateSPType
        {
        TemplateNoSP,
        TemplateExtractSPFromTemplate,
        TemplateLoadSPFromFile,
        };


//----------------------------------------------------------------------------

enum        GreyMatterFlags;
class       TGoF;
class       NormalizeBrainResults;
class       TEasyStats;
class       TSuperGauge;


void    ComputingTemplateMri    (   const TGoF&         mrifiles,
                                    BuildTemplateType   howtemplate,
                                    TemplateSPType      howsp,
                                    const char*         mnifile,
                                    const char*         mnispfile,
                                    bool                templatesymmetrical,
                                    GreyMatterFlags     greyflags,
                                    int                 numsolpoints,           double          ressolpoints,
                                    GreyMatterFlags     spflags,
                                    double              precision,              int             numiterations,
                                    const char*         fileprefix,
                                    bool                savingcoregmris
                                );


void    NormalizeBrains         (   const TGoF&             mrifiles,
                                    NormalizeBrainResults*& allnorms,
                                    TVector3Int&            mrimaxdim,
                                    TSuperGauge&            Gauge
                                );


void    MergeMris               (   const TGoF&             mrifiles,
                                    BuildTemplateType       howtemplate,
                                    const NormalizeBrainResults*    allnorms,
                                    const char*             mnifile,
                                    const TVector3Int&      mrimaxdim,
                                    double                  precision,              int             numiterations,
                                    bool                    symmetrical,
                                    const char*             basefilename,   
                                    char*                   templatefile,           TGoF&           gofmatrices,
                                    bool                    savingcoregmris,
                                    TEasyStats&             quality,
                                    TSuperGauge&            Gauge
                                );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
