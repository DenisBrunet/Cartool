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

#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       PreProcessMrisNumGauge  = 13;

enum        MRISequenceType;
enum        ResizingType;
enum        BoundingSizeType;
enum        ReorientingType;
enum        OriginFlags;
enum        SkullStrippingType;
enum        GreyMatterFlags;
enum        SPPresetsEnum;
class       TGoF;
class       TSuperGauge;


void        PreprocessMris  (   const TGoF&         gofin,
                                MRISequenceType     mrisequence,
                                bool                cleanup,
                                bool                isotropic,
                                ResizingType        resizing,           double              resizingvalue,
                                bool                anyresizing,        BoundingSizeType    targetsize,         const TPointInt&    sizeuser,
                                ReorientingType     reorienting,        const char*         reorientingstring,
                                bool                sagittalplane,      bool                transverseplane,
                                OriginFlags         origin,             const TPointDouble& arbitraryorigin,
                                SkullStrippingType  skullstripping,     bool                bfc,
                                bool                computegrey,        GreyMatterFlags     greyflags,
                                SPPresetsEnum       sppreset,           GreyMatterFlags     spflags,
                                int                 numsps,             double              ressps,
                                const char*         spfrombrainfile,    const char*         spfromspfile,
                                const char*         infixfilename,
                                TGoF&               gofout,
                                TSuperGauge*        gauge,              bool                showsubprocess
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
