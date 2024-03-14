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

enum                        GOMethod;
class                       TPoints;
class                       TStrings;
class                       TMatrix44;
template <class>    class   TArray2;


//----------------------------------------------------------------------------
double  CoregisterXyzToXyz      (   TPoints&            frompoints,     const TPoints&      topoints, 
                                    GOMethod            method,         int                 how,            double              precision,
                                    int                 numscaling,     bool                finalscaling,
                                    const TStrings*     xyznames,       const char*         filename,
                                    const char*         title       = 0
                                );

void    AveragePoints           (   TPoints*            listpoints, 
                                    int                 numlists, 
                                    TArray2<bool>&      oktoaverage, 
                                    TPoints&            averagepoints, 
                                    TStrings*           xyznames,
                                    char*               filename
                                );

void    CenterAndReorient       (   TPoints&            xyzpoints, 
                                    TMatrix44&          matrix, 
                                    char*               filexyz 
                                );

void    SymmetrizeXyz           (   TPoints&            xyzpoints );


//----------------------------------------------------------------------------
                                        // Realign Sagittal + "Transverse" planes
                                        // Can also coregister to another model
bool    NormalizeXyz            (   TPoints&            xyz,
                                    TStrings&           xyznames,
                                    TPoints&            buddyxyz,
                                    const char*         xyzreffile
                                );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
