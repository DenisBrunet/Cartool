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

#include    "Math.TMatrix44.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A global coregistration transform split into easily updatable components from a user perspective
                                        // Rotations, scaling and translations are assigned respectively to separate matrices, which are then 
                                        // combined to provide the final transform.
                                        // This allows the user to update a single geometrical operation which is actually inside a series of nested operations,
                                        // which is impossible to do with a single transform matrix, updated from the left or from the right only.
                                        // It also conveniently wraps the Gluing transform, which can not be modeled as a homogeneous transform matrix.
class   TCoregistrationTransform
{
public:
    inline          TCoregistrationTransform ();


    TMatrix44       SourceToTargetOrientation;
    TMatrix44       ScaleOnly;
    TMatrix44       RotateOnly;
    TMatrix44       TranslateMriOnly;
    bool            Gluing;


    inline void     Reset           ();
                                        
    inline TMatrix44 ComposeTransform ()    const       { return TranslateMriOnly * RotateOnly * ScaleOnly * SourceToTargetOrientation; }   // !Transforms ordering matters!

    inline void     Apply           ( TPoints &points, const Volume& mask, const Volume& gradient, const TPointDouble& origin, const TMatrix44& mriabstoguillotine, double inflating = 0 )  const;

                                        // !Transforms are being applied on separate matrices!
    inline void     RotateX         ( double a )        { RotateOnly        .RotateX    ( a, MultiplyLeft ); }
    inline void     RotateY         ( double a )        { RotateOnly        .RotateY    ( a, MultiplyLeft ); }
    inline void     RotateZ         ( double a )        { RotateOnly        .RotateZ    ( a, MultiplyLeft ); }

    inline void     TranslateX      ( double t )        { TranslateMriOnly  .TranslateX ( t, MultiplyLeft ); }
    inline void     TranslateY      ( double t )        { TranslateMriOnly  .TranslateY ( t, MultiplyLeft ); }
    inline void     TranslateZ      ( double t )        { TranslateMriOnly  .TranslateZ ( t, MultiplyLeft ); }

    inline void     Scale           ( double s )        { ScaleOnly         .Scale      ( s, MultiplyLeft ); }
    inline void     ScaleX          ( double s )        { ScaleOnly         .ScaleX     ( s, MultiplyLeft ); }
    inline void     ScaleY          ( double s )        { ScaleOnly         .ScaleY     ( s, MultiplyLeft ); }
    inline void     ScaleZ          ( double s )        { ScaleOnly         .ScaleZ     ( s, MultiplyLeft ); }

    inline void     ToggleGlueing   ()                  { Gluing    = ! Gluing; }
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

    TCoregistrationTransform::TCoregistrationTransform ()
{
Reset ();
}


void    TCoregistrationTransform::Reset ()
{
SourceToTargetOrientation   .SetIdentity ();
ScaleOnly                   .SetIdentity ();
RotateOnly                  .SetIdentity ();
TranslateMriOnly            .SetIdentity ();

Gluing          = false;
}

                                        // Input volumes are split into 2:
                                        //  - a mask for the geometrical boundaries
                                        //  - a smoothed gradient to compute some projection direction
void    TCoregistrationTransform::Apply (   TPoints&                points, 
                                            const Volume&           mask,               const Volume&           gradient,
                                            const TPointDouble&     origin, 
                                            const TMatrix44&        mriabstoguillotine,
                                            double                  inflating
                                        )   const
{
if ( points.IsEmpty () )

    return;

                                        // First the whole 4x4 homogeneous transform
ComposeTransform ().Apply ( points );   // Results in Absolute MRI space

                                        // Then the optional non-linear gluing operation
if ( Gluing )

    points.ResurfacePoints  (   mask,       gradient,
                                origin,
                                &mriabstoguillotine,
                                inflating
                            );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
