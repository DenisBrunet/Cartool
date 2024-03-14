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

constexpr char*     MergingMriMasksTitle        = "Merging Head Tissues";


void    MergingMriMasks (
                        const char*     filehead,
                        const char*     fileskull,
                        const char*     fileskullsp,
                        const char*     filecsf,
                        const char*     filegrey,
                        const char*     filewhite,
                        const char*     fileblood,
                        const char*     fileair,
                        const char*     fileeyes,
                        bool            createspongy,       double          compactthickness
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
