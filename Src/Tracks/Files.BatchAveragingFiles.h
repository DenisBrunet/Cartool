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

namespace crtl {

//----------------------------------------------------------------------------

constexpr char*     BatchAveragingTitle         = "Batch Averaging";

enum        ExecFlags;
enum        FrequencyAnalysisType;
enum        PolarityType;
class       TGoF;

                                        // Each  file*  acts as a flag as whether or not computing a given output
void    BatchAveragingScalar        (   const TGoF& gof,
                                        char*       meanfile,       char*       sdfile,         char*       snrfile,
                                        char*       medianfile,     char*       madfile,
                                        ExecFlags   execflags
                                    );
                                        // can also save the results as norms
void    BatchAveragingVectorial     (   const TGoF& gof, 
                                        char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                        char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                        ExecFlags   execflags
                                    );

void    BatchPoolAveragingVectorial (   const TGoF& gof, 
                                        char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                        char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                        int         numlocalavg,    int         numrepeatavg,
                                        ExecFlags   execflags
                                    );

void    BatchAveragingFreq          (   const TGoF&             gof,
                                        FrequencyAnalysisType   freqtype,       PolarityType    fftapproxpolarity,
                                        char*                   meanfile,       char*           sdfile, 
                                        ExecFlags               execflags
                                    );

void    BatchAveragingErrorData     (   const TGoF& gof, 
                                        char*       meanfile,
                                        ExecFlags   execflags
                                    );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
