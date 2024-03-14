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

constexpr char*     BatchAveragingTitle         = "Batch Averaging";

enum        FrequencyAnalysisType;
enum        PolarityType;
class       TGoF;

                                        // Each  file*  acts as a flag as whether or not computing a given output
void    BatchAveragingScalar    (   const TGoF& gof,
                                    char*       meanfile,       char*       sdfile,         char*       snrfile,
                                    char*       medianfile,     char*       madfile,
                                    bool        openresults,    bool        showgauge       = true
                                );
                                        // can also save the results as norms
void    BatchAveragingVectorial (   const TGoF& gof, 
                                    char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                    char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                    bool        openresults,    bool        showgauge       = true
                                );

void    BatchPoolAveragingVectorial (   const TGoF& gof, 
                                        char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                        char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                        int         numlocalavg,    int         numrepeatavg,
                                        bool        openresults,    bool        showgauge 
                                    );

void    BatchAveragingFreq      (   const TGoF&             gof,
                                    FrequencyAnalysisType   freqtype,       PolarityType    fftapproxpolarity,
                                    char*                   meanfile,       char*           sdfile, 
                                    bool                    openresults,    bool            showgauge       = true
                                );

void    BatchAveragingErrorData (   const TGoF& gof, 
                                    char*       meanfile,
                                    bool        openresults,    bool        showgauge       = true
                                );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
