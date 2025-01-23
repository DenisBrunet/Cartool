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

#include    "Math.TMatrix44.h"
#include    "Geometry.TPoint.h"
#include    "Geometry.TBoundingBox.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TVolumeDoc;
                                        // Get the transform matrix and / or center to transform from normalized space to relative space
                                        // Stand alone function, it doesn't really belong to a class
void    GetNormalizationTransform ( const TVolumeDoc*   MriFull,            const TVolumeDoc*   MriBrain,
                                    bool                estimatecenter,     TPointDouble&       MriCenter, 
                                    TMatrix44*          AbsNorm_to_RelVoxel,TMatrix44*          RelVoxel_to_AbsNorm 
                                  );


//----------------------------------------------------------------------------

enum    TGeometryTransformType {
                        GeometryUnknown,

                        TalairachAvg152T1Fld,               // very old stuff
                        TalairachAvg152T1_smac,             // old stuff
                        TalairachAve152_sym,                // old stuff
                        TalairachMNI152_2001,               // official MNI152 from 2001
                        TalairachMNI152_2009,               // official MNI152 from 2009 - for now, no differences with 2001, though
                        TalairachNIHPD_Infant,              // official NIHPD Infant 33-44 months
                        Talairach,                          // plain Talairach

                        NumGeometryTransformTypes
                        };

extern const char   GeometryTransformName[ NumGeometryTransformTypes ][ 32 ];

                                        // A basis to transform to other referentials
class   TGeometryTransform
{
public:
                        TGeometryTransform ( TGeometryTransformType type );


    TGeometryTransformType  Type;
    char                    Name[ 64 ];

    void                ConvertTo   ( TPointFloat&  p,      const TBoundingBox<double>* bounding = 0 )  const;  // a single point
    void                ConvertTo   ( char*         filein, const TBoundingBox<double>* bounding = 0 )  const;  // an entire file

protected:
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
