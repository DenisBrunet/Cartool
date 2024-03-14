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
                                        // Utility to perform the fastest read to files, to extract useful infos like number of electrodes etc..
                                        // It avoids running a full Open by the Doc Manager, which can take awhile
                                        // Each derived document has to implement its own static ReadFromHeader method
enum        ReadFromHeaderType
            {
            ReadNumElectrodes               = 1,
            ReadNumAuxElectrodes,
            ReadNumTimeFrames,
            ReadNumFrequencies,
            ReadSamplingFrequency,
            ReadNumSolPoints,
            ReadInverseScalar,
            ReadNumRois,
            ReadMagicNumber,
            ReadElectrodesNames,
            ReadNumClusters,
            ReadOriginalSamplingFrequency,  // frequency files
            ReadFrequencyType,              // frequency files
            };


bool        ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
