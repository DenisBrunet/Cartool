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

#include    <vector>

#include    "Geometry.TPoint.h"
#include    "Geometry.TPoints.h"
#include    "Geometry.TBoundingBox.h"
#include    "TVolume.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A specific display space, like 2D or 3D
class   TDisplaySpace
{
public:
                        TDisplaySpace ();


    TPoints             Points;         // discrete set of points
    double              MedianDistance; // between points
    double              PointRadius;    // for rendering
    TBoundingBox<double>Bounding;       // of the 3D rendering

                                        // !TODO! moving from TBaseDoc base class to here
//  TTriangleSurface    Rendering;      // of the object
//  TOrientation        Orientation;
//  TGeometryTransform* GeometryTransform;

                        TDisplaySpace       ( const TDisplaySpace &op );
    TDisplaySpace&      operator        =   ( const TDisplaySpace &op2 );
};


//----------------------------------------------------------------------------
                                        // The many display spaces relevant for a given object

enum {
    DisplaySpaceNone        = -1,       // no space in use
    DisplaySpace3D          =  0,       // default
    DisplaySpaceProjected,
    DisplaySpaceHeight,
    NumDisplaySpaces
    };

extern const char   DisplaySpaceName[ NumDisplaySpaces ][ 32 ];


class   TDisplaySpaces
{
public:
                        TDisplaySpaces ();
                        TDisplaySpaces ( int numspaces );
                       ~TDisplaySpaces ();


    void                Reset           ();
    void                Set             ( int numspaces );


    int                 GetNumSpaces ()                     const   { return NumSpaces; };
//  TDisplaySpace*      GetDisplaySpace ( int index = 0 )           { return index >= 0 && index < NumSpaces ? Spaces + index : 0; };


                        TDisplaySpaces      ( const TDisplaySpaces &op  );
    TDisplaySpaces&     operator        =   ( const TDisplaySpaces &op2 );


                        operator    int     ()              const   { return NumSpaces; }
                        operator    bool    ()              const   { return NumSpaces; }

    TDisplaySpace      &operator    []      ( int index )           { return Spaces[ index ]; };
    const TDisplaySpace&operator    []      ( int index )   const   { return Spaces[ index ]; };


protected:
    int                         NumSpaces;
    std::vector<TDisplaySpace>  Spaces;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
