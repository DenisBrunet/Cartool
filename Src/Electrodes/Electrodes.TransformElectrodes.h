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

#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    CoregistrationType;
class   TCoregistrationTransform;
class   TMatrix44;


bool    TransformElectrodes (   const char*             xyzsourcefile,
                                const char*             mritargetfile,      // not actually needed for computation

                                const TCoregistrationTransform& transform,
                                CoregistrationType      processing,

                                const Volume&           mask,
                                const Volume&           gradient,
                                const TPointDouble&     origin, 
                                const TMatrix44&        mriabstoguillotine,

                                const char*             basefilename,
                                char*                   xyztransfile,
                                char*                   altxyztransfile,        const char*             altelectrodes,
                                TMatrix44*              xyzcoregtonorm
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
