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

#include    "TVolumeDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     NiftiTitle          = "Opening Nifti MRI";
constexpr char*     NiftiMethod1Title   = "Reslicing Nifti Method 1";
constexpr char*     NiftiMethod2Title   = "Reslicing Nifti Method 2";
constexpr char*     NiftiMethod3Title   = "Reslicing Nifti Method 3";
                                        // Currently will force reslice all Nifit volumes to this target voxel size, in [mm]:
constexpr double    NiftiTargetRes      = 1.0;


class   TVolumeNiftiDoc     :  public  TVolumeDoc
{
public:
                    TVolumeNiftiDoc ( owl::TDocument *parent = 0 );

                                            // owl::TDocument
    bool            Commit  ( bool force = false );
    bool            Open    ( int mode, const char *path = 0 );

protected:
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
