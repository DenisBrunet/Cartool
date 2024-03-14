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

enum        CorrelateFilesGaugeEnum
            {
            gaugecorrglobal,
            gaugecorrfile1,
            gaugecorrfile2,
            };


enum        CorrelateType;
class       TGoF;
class       TSuperGauge;


void    CorrelateFiles (    TGoF&           filenames1,     TGoF&           filenames2, 
                            CorrelateType   correlate, 
                            bool            ignorepolarity,
                            bool            spatialfilter1, bool            spatialfilter2,     char*           xyzfile,
                            int             cliptf,
                            int             numrand,
                            const char*     corrtitle,      TSuperGauge*    gauge 
                         );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
