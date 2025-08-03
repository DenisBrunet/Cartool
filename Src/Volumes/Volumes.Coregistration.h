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

#include    "Geometry.TPoint.h"
#include    "Math.TMatrix44.h"
#include    "GlobalOptimize.h"          // GOMethod

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        RemapIntensityType;
enum        FitVolumeType;
class       TVolumeDoc;
class       TGoF;
class       CoregistrationSpecsType;


void    CoregisterMris      (   const TVolumeDoc*   SourceMri,  RemapIntensityType  fromremap,
                                const TVolumeDoc*   TargetMri,  RemapIntensityType  toremap,
                                FitVolumeType       inclusionflags,
                                const CoregistrationSpecsType& coregtype,
                                GOMethod            method,
                                double              precision,
                                const TGoF&         buddymris,  const TGoF&         buddypoints,
                                const char*         fileprefix,
                                TGoF&               outputmats, TGoF&               outputmris,     TGoF&               outputpoints,
                                double&             quality,    char*               qualityopinion,
                                ExecFlags           execflags
                            );


void    CoregisterBrains    (   const TVolumeDoc*   SourceMri,  RemapIntensityType  fromremap,
                                const TVolumeDoc*   TargetMri,  RemapIntensityType  toremap,
                                FitVolumeType       inclusionflags,
                                const CoregistrationSpecsType& coregtype,
                                double              precision,
                                const TGoF&         buddymris,  const TGoF&         buddypoints,
                                const char*         fileprefix,
                                TGoF&               outputmats, TGoF&               outputmris,     TGoF&               outputpoints,
                                double&             quality,    char*               qualityopinion,
                                ExecFlags           execflags
                            );


//----------------------------------------------------------------------------

class   NormalizeBrainResults
{
public:

    TMatrix44           Rel_to_Abs;
    TPointDouble        Origin;
    TPointDouble        OriginToTarget;

    void                Reset ()        { Rel_to_Abs.SetIdentity (); Origin.Reset (); OriginToTarget.Reset (); }
};


bool    NormalizeBrain      (   const TVolumeDoc*       MriDoc,
                                NormalizeBrainResults&  norm
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
