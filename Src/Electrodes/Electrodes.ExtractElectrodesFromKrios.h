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
//----------------------------------------------------------------------------
                                        // Krios scanner
constexpr char* KriosTitle              = "Krios Conversion";

                                        // KriosType:
                                        //  electrode
constexpr char* KriosTypeScanned        = "SCANNED";
                                        //  landmark
constexpr char* KriosTypeProbed         = "PROBED";
                                        //  repeated measure in case of error(?)
constexpr char* KriosTypeEstimated      = "ESTIMATED";

                                        // KriosSpecialSensor
constexpr char* KriosSpecialSensorEeg   = "EEG";
constexpr char* KriosSpecialSensorLeft  = "LEFT_PP_MARKER";
constexpr char* KriosSpecialSensorFront = "NASION_MARKER";
constexpr char* KriosSpecialSensorRight = "RIGHT_PP_MARKER";


enum        KriosIndexes
            {
            KriosId,                    // index
            KriosName,                  // electrode name - !can be empty!
            KriosType,                  // type from scanner - see above
            KriosX,                     // coordinate
            KriosY,                     // coordinate
            KriosZ,                     // coordinate
            KriosSpecialSensor,         // type of sensor - see above
            KriosDate,

            NumKriosIndexes,
            };


bool        ExtractElectrodesFromKrios  (   const char*     filein,
                                            const char*     namestoskip,
                                            const char*     xyzreffile,
                                            char*           fileout 
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
