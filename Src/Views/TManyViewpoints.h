/*
==========================================================================
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
==========================================================================
*/

#pragma once

#include    "OpenGL.Geometry.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    {
        ViewpointsNone,
        ViewpointsTopBackLeftSummary,
        ViewpointsFrontBack,
        ViewpointsLeftBackRight,

        NumViewpoints
        };


class   TOneViewpoint
{
public:
                    TOneViewpoint ();

    TGLCoordinates<GLfloat>     Center;
    TGLMatrix                   Orientation;
    bool            CanMove;
    bool            Summary;
    int             SliceToShow;

    void            Reset ();
};


//----------------------------------------------------------------------------
                                            // Class used to handle simultaneous multiple viewpoints of the same graphical object
                                            // Currently NOT in use, but prototype code was working at some point in time..

                                            // Class is lightweight, using an array seems good enough
constexpr int       NumMaxViewPoints        = 4;

class   TManyViewpoints
{
public:
                        TManyViewpoints ();

    int                 Mode;
    int                 NumViewpoints;
    double              ResizeX;
    double              ResizeY;
    TOneViewpoint       Viewpoints[ NumMaxViewPoints ];

    void                Reset               ();
    void                Set                 ( int mode, double maxsize, const TGLMatrix* standardorient );

                        operator    bool    ()          const   { return NumViewpoints && Mode; }
    TOneViewpoint&      operator    []      ( int i )           { return Viewpoints[ i ]; }
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TOneViewpoint::TOneViewpoint ()
{
Reset ();
}


void    TOneViewpoint::Reset ()
{
Center     .Reset ();
Orientation.SetIdentity ();
CanMove     = false;
Summary     = false;
SliceToShow = -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TManyViewpoints::TManyViewpoints ()
{
Reset ();
}


void    TManyViewpoints::Reset ()
{
Mode            = ViewpointsNone;
NumViewpoints   = 0;
ResizeX         = ResizeY       = 1.0;

for ( int g = 0; g < MAXVIEWPOINTS; g++ )
    Viewpoints[ g ].Reset ();
}


//----------------------------------------------------------------------------
void    TManyViewpoints::Set ( int mode, double maxsize, const TGLMatrix* standardorient )
{
Reset ();


if ( mode == ViewpointsTopBackLeftSummary ) {

    Mode             = ViewpointsTopBackLeftSummary;
    NumViewpoints    = 4;
    ResizeX          = 2;
    ResizeY          = 2;

    Viewpoints[ 0 ].Center.Set ( - 0.5 * maxsize, - 0.5 * maxsize, 0 );
    Viewpoints[ 1 ].Center.Set ( - 0.5 * maxsize,   0.5 * maxsize, 0 );
    Viewpoints[ 2 ].Center.Set (   0.5 * maxsize, - 0.5 * maxsize, 0 );
    Viewpoints[ 3 ].Center.Set (   0.5 * maxsize,   0.5 * maxsize, 0 );

    Viewpoints[ 0 ].Orientation.SetOrientation ( OrientCoronalBack   );
    Viewpoints[ 1 ].Orientation.SetOrientation ( OrientTransverseTop );
    Viewpoints[ 2 ].Orientation.SetOrientation ( OrientSagittalLeft  );

    Viewpoints[ 0 ].Orientation *= *standardorient;
    Viewpoints[ 1 ].Orientation *= *standardorient;
    Viewpoints[ 2 ].Orientation *= *standardorient;

    Viewpoints[ 3 ].Orientation.SetIdentity ();

    Viewpoints[ 0 ].SliceToShow = 0;
    Viewpoints[ 1 ].SliceToShow = 1;
    Viewpoints[ 2 ].SliceToShow = 2;
    Viewpoints[ 3 ].SliceToShow = -1;

    Viewpoints[ 0 ].CanMove     = Viewpoints[ 1 ].CanMove   = Viewpoints[ 2 ].CanMove   = false;
    Viewpoints[ 3 ].CanMove     = true;

    Viewpoints[ 0 ].Summary     = Viewpoints[ 1 ].Summary   = Viewpoints[ 2 ].Summary   = false;
    Viewpoints[ 3 ].Summary     = true;
    }

else if ( mode == ViewpointsLeftBackRight ) {
    Mode             = ViewpointsLeftBackRight;
    NumViewpoints    = 3;
    ResizeX          = 3;
    ResizeY          = 1;

    Viewpoints[ 0 ].Center.Set ( - 1.0 * maxsize,   0.0 * maxsize, 0 );
    Viewpoints[ 1 ].Center.Set (   0.0 * maxsize,   0.0 * maxsize, 0 );
    Viewpoints[ 2 ].Center.Set (   1.0 * maxsize,   0.0 * maxsize, 0 );

    Viewpoints[ 0 ].Orientation.SetOrientation ( OrientSagittalLeft  );
    Viewpoints[ 1 ].Orientation.SetOrientation ( OrientCoronalBack   );
    Viewpoints[ 2 ].Orientation.SetOrientation ( OrientSagittalRight );

    Viewpoints[ 0 ].Orientation *= *standardorient;
    Viewpoints[ 1 ].Orientation *= *standardorient;
    Viewpoints[ 2 ].Orientation *= *standardorient;

    Viewpoints[ 0 ].SliceToShow = 2;
    Viewpoints[ 1 ].SliceToShow = 0;
    Viewpoints[ 2 ].SliceToShow = 2;

    Viewpoints[ 0 ].CanMove     = Viewpoints[ 1 ].CanMove   = Viewpoints[ 2 ].CanMove   = false;

    Viewpoints[ 0 ].Summary     = Viewpoints[ 1 ].Summary   = Viewpoints[ 2 ].Summary   = false;
    }

else if ( mode == ViewpointsFrontBack ) {
    Mode             = ViewpointsFrontBack;
    NumViewpoints    = 2;
    ResizeX          = 2;
    ResizeY          = 1;

    Viewpoints[ 0 ].Center.Set ( - 0.5 * maxsize,   0.0 * maxsize, 0 );
    Viewpoints[ 1 ].Center.Set (   0.5 * maxsize,   0.0 * maxsize, 0 );

    Viewpoints[ 0 ].Orientation.SetOrientation ( OrientCoronalFront );
    Viewpoints[ 1 ].Orientation.SetOrientation ( OrientCoronalBack  );

    Viewpoints[ 0 ].Orientation *= *standardorient;
    Viewpoints[ 1 ].Orientation *= *standardorient;

    Viewpoints[ 0 ].SliceToShow = 0;
    Viewpoints[ 1 ].SliceToShow = 0;

    Viewpoints[ 0 ].CanMove     = Viewpoints[ 1 ].CanMove   = false;

    Viewpoints[ 0 ].Summary     = Viewpoints[ 1 ].Summary   = false;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
