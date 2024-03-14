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

#include    "Geometry.TPoints.h"
#include    "Geometry.TDipole.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     GenerateDataTitle       = "Generating Data";


enum                GenerateTypeFlag
                    {
                    GenerateUnknown,
                    GenerateEeg,
                    GenerateRis,
                    };

enum                GenerateSegmentFlag
                    {
                    SeedMaps,
                    ConstantSegments,
                    HanningSegments,
                    LeakySegments,
                    };

class                       TLeadField;
template <class> class      TTracks;

                                        // Generate n maps which should correlate altogether within a specified range
void        GenerateData (  GenerateTypeFlag    what,
                            int                 nummapsmin,         int             nummapsmax,
                            double              correlationmin,     double          correlationmax,     double          correlationstep,
                            double              mapnoisemin,        double          mapnoisemax,        double          mapnoisestep,
                            int                 segdurationmin,     int             segdurationmax,
                            GenerateSegmentFlag flag,
                            bool                cyclicsegs,
                            bool                ignorepolarity,
                            bool                normalize,
                            TLeadField&         leadfield,          TTracks<float>& K,                  int             numsources,
                            TPoints             solp,
                            int                 numfiles,           int             fileduration,
                            const char*         basefilename,
                            bool                savemaps,           bool            savetemplatemaps,   bool            savetemplateris,
                            bool                runsegmentation,    int             numrandtrials
                            );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
