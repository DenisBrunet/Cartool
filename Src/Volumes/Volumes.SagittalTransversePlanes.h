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

enum    GOMethod;
class   TVolumeDoc;
class   TMatrix44;


void    SaveSagittalPlane       (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    normtomriabs,
                                    const char*         fileoriginal,
                                    const char*         filetransformed
                                );

void    SaveTransversePlane     (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    normtomriabs,
                                    const char*         fileoriginal,
                                    const char*         filetransformed
                                );

//----------------------------------------------------------------------------
                                        // Doing a single search with all parameters
double  SearchSagittalPlaneMri  (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    MriRel_to_MriAbs,
                                    TMatrix44&          SagAbs_to_MriRel,   // input & output 
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    int                 how,
                                    double              precision,
                                    const char*         title           = 0,
                                    bool                verbose         = false
                                );
                                        // Smart search which can run multiple SearchSagittalPlaneMri and make choices about the results
bool    SetSagittalPlaneMri     (   const TVolumeDoc*   mridoc,     
                                    const TMatrix44&    MriRel_to_MriAbs,
                                    TMatrix44&          SagAbs_to_MriRel,   // input & output
                                    TPointDouble&       center,             // input & output
                                    double*             Q               = 0,
                                    const char*         title           = 0
                                );

//----------------------------------------------------------------------------
                                        // Doing a single search with all parameters
double  SearchTransversePlaneMri(   const TVolumeDoc*   mribrain,
                                    const TMatrix44&    MriRel_to_MriAbs,   // not actually used
                                    TMatrix44&          TraAbs_to_MriRel,   // input & output 
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    int                 how,
                                    double              precision,
                                    const char*         title           = 0,
                                    bool                verbose         = false
                                );
                                        // Smart search which can run multiple SearchSagittalPlaneMri and make choices about the results
bool    SetTransversePlaneMri   (   const TVolumeDoc*   mribrain, 
                                    const TMatrix44&    MriRel_to_MriAbs,   // not actually used
                                    TMatrix44&          TraAbs_to_MriRel,   // input & output
                                    TPointDouble&       center,             // input & output
                                    double*             Q               = 0,
                                    const char*         title           = 0
                                );

//----------------------------------------------------------------------------

double  SearchGuillotinePlane   (   const TVolumeDoc*   mrihead,
                                    TMatrix44&          mriabstoguillotine, // input & output 
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    double              precision,
                                    const char*         title           = 0,
                                    bool                verbose         = false
                                );

bool    SetGuillotinePlane      (   const TVolumeDoc*   mrihead, 
                                    TMatrix44&          mriabstoguillotine, // output
                                    double*             Q               = 0,
                                    const char*         title           = 0
                                );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
