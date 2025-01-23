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

#include    "Dialogs.TSuperGauge.h"

namespace crtl {

//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::GetGradient ( int x, int y, int z, TPointFloat& g, int d )   const
{
g.X     = ( GetValueChecked ( x + d, y,     z     ) - GetValueChecked ( x - d, y,     z     ) ) / (double) ( 2 * d );
g.Y     = ( GetValueChecked ( x,     y + d, z     ) - GetValueChecked ( x,     y - d, z     ) ) / (double) ( 2 * d );
g.Z     = ( GetValueChecked ( x,     y,     z + d ) - GetValueChecked ( x,     y,     z - d ) ) / (double) ( 2 * d );
}

                                        // New version, takes much more points into account - not tested for speed
template <class TypeD>
void    TVolume<TypeD>::GetGradientSmoothed ( int x, int y, int z, TPointFloat& g, int d )   const
{
g.Reset ();


int                 d2          = AtLeast ( 1, d - 1 ); // half size of orthogonal search space, and at least 1 for some smoothing
int                 w           = 0;    // this could be non-integer

                                        // cumulate all gradients from 1 to d
for ( int di = 1; di <= d; di++ ) {
                                        // current step is 2*di - just use di here, adjust factor at the end
    w++;
                                        // straight lines in x, y, and z
    g.X    += ( GetValueChecked ( x + di, y,      z      ) - GetValueChecked ( x - di, y,      z      ) ) / di;
    g.Y    += ( GetValueChecked ( x,      y + di, z      ) - GetValueChecked ( x,      y - di, z      ) ) / di;
    g.Z    += ( GetValueChecked ( x,      y,      z + di ) - GetValueChecked ( x,      y,      z - di ) ) / di;

                                        // up and down above each axis
    for ( int dj = 1; dj <= d2; dj++ ) {

        w      += 8;
                                        // 4 crosses + 4 corners (still not the whole half cube, but already good)
        g.X    += ( GetValueChecked ( x + di, y + dj, z      ) - GetValueChecked ( x - di, y + dj, z      ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y - dj, z      ) - GetValueChecked ( x - di, y - dj, z      ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y,      z + dj ) - GetValueChecked ( x - di, y,      z + dj ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y,      z - dj ) - GetValueChecked ( x - di, y,      z - dj ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y + dj, z + dj ) - GetValueChecked ( x - di, y + dj, z + dj ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y + dj, z - dj ) - GetValueChecked ( x - di, y + dj, z - dj ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y - dj, z + dj ) - GetValueChecked ( x - di, y - dj, z + dj ) ) / di;
        g.X    += ( GetValueChecked ( x + di, y - dj, z - dj ) - GetValueChecked ( x - di, y - dj, z - dj ) ) / di;

        g.Y    += ( GetValueChecked ( x,      y + di, z + dj ) - GetValueChecked ( x,      y - di, z + dj ) ) / di;
        g.Y    += ( GetValueChecked ( x,      y + di, z - dj ) - GetValueChecked ( x,      y - di, z - dj ) ) / di;
        g.Y    += ( GetValueChecked ( x + dj, y + di, z      ) - GetValueChecked ( x + dj, y - di, z      ) ) / di;
        g.Y    += ( GetValueChecked ( x - dj, y + di, z      ) - GetValueChecked ( x - dj, y - di, z      ) ) / di;
        g.Y    += ( GetValueChecked ( x + dj, y + di, z + dj ) - GetValueChecked ( x + dj, y - di, z + dj ) ) / di;
        g.Y    += ( GetValueChecked ( x + dj, y + di, z - dj ) - GetValueChecked ( x + dj, y - di, z - dj ) ) / di;
        g.Y    += ( GetValueChecked ( x - dj, y + di, z + dj ) - GetValueChecked ( x - dj, y - di, z + dj ) ) / di;
        g.Y    += ( GetValueChecked ( x - dj, y + di, z - dj ) - GetValueChecked ( x - dj, y - di, z - dj ) ) / di;

        g.Z    += ( GetValueChecked ( x + dj, y,      z + di ) - GetValueChecked ( x + dj, y,      z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x - dj, y,      z + di ) - GetValueChecked ( x - dj, y,      z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x,      y + dj, z + di ) - GetValueChecked ( x,      y + dj, z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x,      y - dj, z + di ) - GetValueChecked ( x,      y - dj, z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x + dj, y + dj, z + di ) - GetValueChecked ( x + dj, y + dj, z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x + dj, y - dj, z + di ) - GetValueChecked ( x + dj, y - dj, z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x - dj, y + dj, z + di ) - GetValueChecked ( x - dj, y + dj, z - di ) ) / di;
        g.Z    += ( GetValueChecked ( x - dj, y - dj, z + di ) - GetValueChecked ( x - dj, y - dj, z - di ) ) / di;
        }
    }

                                        // adjusting for step 2*di above; average across all summed values
g  /= 2 * w;
}


//----------------------------------------------------------------------------
                                        // returns Dx Dy Dz (Gradient) & Dxy Dxz Dyz Dxx Dyy Dzz (Hessian)
template <class TypeD>
void    TVolume<TypeD>::GetDifferences ( int x, int y, int z, double *diff, int d )     const
{
                                        // quite strict test, but we really do need all the borders!
//if ( x < d || y < d || z < d || x >= Dim1 - d || y >= Dim2 - d || z >= Dim3 - d ) {
//    ClearVirtualMemory ( diff, sizeof ( *diff ) * NumDiffs );
//    return;
//    }

double              d2              = d << 1;
double              d4              = d << 2;

                                        // First order differences, a la Gradient
diff[ DiffDx  ] = ( GetValueChecked ( x + d, y, z ) - GetValueChecked ( x - d, y, z ) ) / d2;
diff[ DiffDy  ] = ( GetValueChecked ( x, y + d, z ) - GetValueChecked ( x, y - d, z ) ) / d2;
diff[ DiffDz  ] = ( GetValueChecked ( x, y, z + d ) - GetValueChecked ( x, y, z - d ) ) / d2;

                                        // Second order differences
diff[ DiffDxy ] = ( GetValueChecked ( x + d, y + d, z ) + GetValueChecked ( x - d, y - d, z ) - GetValueChecked ( x + d, y - d, z ) - GetValueChecked ( x - d, y + d, z ) ) / d4;
diff[ DiffDxz ] = ( GetValueChecked ( x + d, y, z + d ) + GetValueChecked ( x - d, y, z - d ) - GetValueChecked ( x + d, y, z - d ) - GetValueChecked ( x - d, y, z + d ) ) / d4;
diff[ DiffDyz ] = ( GetValueChecked ( x, y + d, z + d ) + GetValueChecked ( x, y - d, z - d ) - GetValueChecked ( x, y + d, z - d ) - GetValueChecked ( x, y - d, z + d ) ) / d4;


diff[ DiffDxx ] = ( GetValueChecked ( x + d, y, z ) + GetValueChecked ( x - d, y, z ) - 2 * GetValueChecked ( x, y, z ) ) / d4;
diff[ DiffDyy ] = ( GetValueChecked ( x, y + d, z ) + GetValueChecked ( x, y - d, z ) - 2 * GetValueChecked ( x, y, z ) ) / d4;
diff[ DiffDzz ] = ( GetValueChecked ( x, y, z + d ) + GetValueChecked ( x, y, z - d ) - 2 * GetValueChecked ( x, y, z ) ) / d4;
}


template <class TypeD>
void    TVolume<TypeD>::GetPrincipalGradient ( int x, int y, int z, TPointFloat &g, int d, double &C )     const
{
TPointFloat         grad;

GetGradient ( x, y, z, grad, d );


AMatrix33           m33;
                                        // build structure tensor from gradient
m33 ( 0, 0 )    =                   grad[ 0 ] * grad[ 0 ];
m33 ( 1, 1 )    =                   grad[ 1 ] * grad[ 1 ];
m33 ( 2, 2 )    =                   grad[ 2 ] * grad[ 2 ];
m33 ( 0, 1 )    = m33 ( 1, 0 )  =   grad[ 0 ] * grad[ 1 ];
m33 ( 0, 2 )    = m33 ( 2, 0 )  =   grad[ 0 ] * grad[ 2 ];
m33 ( 1, 2 )    = m33 ( 2, 1 )  =   grad[ 1 ] * grad[ 2 ];

                                        // eigenvector decomposition
AVector3            D;
AMatrix33           V;
                                        // signed ascending values
//AEigenvaluesEigenvectors33Arma ( m33, D, V );
AEigenvaluesEigenvectors33DDTT ( m33, D, V );   // much faster for our needs

                                        // convert to absolute eigenvalues
D[ 0 ]  = fabs ( D[ 0 ] );
D[ 1 ]  = fabs ( D[ 1 ] );
D[ 2 ]  = fabs ( D[ 2 ] );

                                        // resetting all (relatively) near-to-zero eigenvalues(?)
if ( D[ 0 ] / D[ 2 ] < SingleFloatEpsilon )     D[ 0 ]  = 0;
if ( D[ 1 ] / D[ 2 ] < SingleFloatEpsilon )     D[ 1 ]  = 0;

                                        // always last eigenvalue
                                                g.Set ( V ( 0, 2 ), V ( 1, 2 ), V ( 2, 2 ) );
//if      ( D[ 0 ] > D[ 1 ] && D[ 0 ] > D[ 2 ] )  g.Set ( V ( 0, 0 ), V ( 1, 0 ), V ( 2, 0 ) );
//else if ( D[ 1 ] > D[ 0 ] && D[ 1 ] > D[ 2 ] )  g.Set ( V ( 0, 1 ), V ( 1, 1 ), V ( 2, 1 ) );
//else                                            g.Set ( V ( 0, 2 ), V ( 1, 2 ), V ( 2, 2 ) );

                                        // reorient eigenvector to match gradient?
if ( g.IsOppositeDirection ( grad ) )
    g.Invert ();


C       = Square ( D[ 0 ] - D[ 1 ] )
        + Square ( D[ 0 ] - D[ 2 ] )
        + Square ( D[ 1 ] - D[ 2 ] );

                                        // let the calling function finish the formula withe constant scaling
//C       = 1 - expl ( - C * C / 2 / 10 );

//if ( VkQuery () )  DBGV4 ( val[ 0 ], val[ 1 ], val[ 2 ], C, "Eigenvalues  C" );
//if ( VkQuery () )  g.Show ( "Eigenvector1" );
}


//----------------------------------------------------------------------------
                                        // Input (relative) point p will be modified, although not guaranteed to be on the surface
template <class TypeD>
TVector3Float   TVolume<TypeD>::GetGradientToPoint ( TPointFloat& p, const TVector3Float& dir, double threshold )   const
{
TPointFloat         origp           = p;
TVector3Float       origdir         = dir;    // save direction from center to voxel

//SetOvershootingOption ( interpolate, Array, LinearDim, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                isinside        = GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) >= threshold;

                                        // follow radial direction to get just below the surface
if ( isinside ) {
                                        // go outward
    while ( GetValueChecked ( p.X + dir.X, p.Y + dir.Y, p.Z + dir.Z, InterpolateLinear ) >= threshold )
        p  += dir;
    }
else {                                  // go inward
    while ( GetValueChecked ( p.X,         p.Y,         p.Z,         InterpolateLinear ) <  threshold )
        p  -= dir;
    }

                                        // for safety!
if ( ! WithinBoundary ( p ) )
    return  dir;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the gradient near the surface
TVector3Float       gradient;

//GetGradient ( p.X, p.Y, p.Z, g );
GetGradientSmoothed ( p.X, p.Y, p.Z, gradient, 30 );    // yes, that's a lot

gradient.Normalize ();
                                        // make sure the gradient points outward
if ( gradient.ScalarProduct ( origdir ) < 0 )
    gradient.Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // vector from current position to original point
TVector3Float       deltap      = origp - p;
                                        // project on gradient direction & subtract -> orthogonal offset
deltap -= gradient * ( deltap.ScalarProduct ( gradient ) );
                                        // new p by shifting perpendicular to gradient - point might not be exactly on surface, though...
p      += deltap;

return  gradient;
}


//----------------------------------------------------------------------------
                                        // Get the surface points of the volume, for each side independently
                                        // The 'safe' parameter allows to test neighbors, to prevent entering holes in the surface
                                        // Points that didn't reach any border can optionally be set to the extremity
                                        // Points can be optionally set on the outside or the inside part of the surface
                                        // Returned points are in relative coordinates (0 on the corner)
template <class TypeD>
void    TVolume<TypeD>::GetSurfacePoints    (   TPoints*            surfp[ NumCubeSides ],
                                                TypeD               threshold,
                                                TPointInt           step,
                                                TPointFloat         center,
                                                double              depth,
                                                SurfacePointsFlags  flags 
                                            )   const
{
bool                outside         = flags & SurfaceOutside;
bool                safe            = flags & SurfaceSafer;
bool                addback         = flags & SurfaceIncludeBack;
bool                addintermediate = flags & SurfaceIntermediatePoints;
bool                sort            = flags & SurfaceSort;
bool                showprogress    = flags & SurfaceShowProgress;


TPointInt           boundmin;
TPointInt           boundmax;
TPointInt           limitmin;
TPointInt           limitmax;
int                 shift           = outside ? 1 : 0;


for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfp[ s ] )
        surfp[ s ]->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( safe ) {
    boundmin.Set ( 1, 1, 1 );
    boundmax.Set ( Dim1 - 2, Dim2 - 2, Dim3 - 2 );
    }
else {
    boundmin.Set ( 0, 0, 0 );
    boundmax.Set ( Dim1 - 1, Dim2 - 1, Dim3 - 1 );
    }

if ( step == 0 )
    step.Set ( 1, 1, 1 );

Clipped ( depth, (double) 0, (double) 100 );
                                        // set the limits for each dimensions
                                        // we go from each border to the center, until a relative depth is attained:
                                        // 0% border, 100% center, >100% is also possible
limitmin        = ( TPointInt ( center.X, center.Y, center.Z ) - boundmin ) * depth + boundmin;
limitmax        = ( TPointInt ( center.X, center.Y, center.Z ) - boundmax ) * depth + boundmax;

//limitmin.Show ( "limit min" );
//limitmax.Show ( "limit max" );

Clipped ( limitmin.X, boundmin.X, boundmax.X );
Clipped ( limitmin.Y, boundmin.Y, boundmax.Y );
Clipped ( limitmin.Z, boundmin.Z, boundmax.Z );
Clipped ( limitmax.X, boundmin.X, boundmax.X );
Clipped ( limitmax.Y, boundmin.Y, boundmax.Y );
Clipped ( limitmax.Z, boundmin.Z, boundmax.Z );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "Volume to Surface", showprogress ? 7 : 0 );


OmpParallelSectionsBegin


OmpSectionBegin
TPointInt           p;
                                        // scan from each side of the cube
Gauge.Next ();

if ( surfp[ SideXMin ] ) {

    for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += step.Y )
    for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += step.Z ) {

        for ( p.X = boundmin.X; p.X <= limitmin.X; p.X ++ ) // note this loop does NOT use step, we don't want to miss the real surface!
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X, p.Y + 1, p.Z ) >= threshold || GetValue ( p.X, p.Y - 1, p.Z ) >= threshold || GetValue ( p.X, p.Y, p.Z + 1 ) >= threshold || GetValue ( p.X, p.Y, p.Z - 1 ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideXMin ]->AddNoDuplicates ( p.X - shift, p.Y, p.Z );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.X % step.X ) )
                surfp[ SideXMin ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.X > limitmin.X )
            surfp[ SideXMin ]->AddNoDuplicates ( Dim1 - 1, p.Y, p.Z );
        } // for y, z
    }
OmpSectionEnd


OmpSectionBegin
TPointInt           p;
                                        // scan from each side of the cube
Gauge.Next ();

if ( surfp[ SideXMax ] ) {

    for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += step.Y )
    for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += step.Z ) {

        for ( p.X = boundmax.X; p.X >= limitmax.X; p.X -- )
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X, p.Y + 1, p.Z ) >= threshold || GetValue ( p.X, p.Y - 1, p.Z ) >= threshold || GetValue ( p.X, p.Y, p.Z + 1 ) >= threshold || GetValue ( p.X, p.Y, p.Z - 1 ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideXMax ]->AddNoDuplicates ( p.X + shift, p.Y, p.Z );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.X % step.X ) )
                surfp[ SideXMax ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.X < limitmax.X )
            surfp[ SideXMax ]->AddNoDuplicates ( 0, p.Y, p.Z );
        } // for y, z
    }
OmpSectionEnd


OmpSectionBegin
TPointInt           p;

Gauge.Next ();

if ( surfp[ SideYMin ] ) {

    for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += step.X )
    for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += step.Z ) {

        for ( p.Y = boundmin.Y; p.Y <= limitmin.Y; p.Y ++ )
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X + 1, p.Y, p.Z ) >= threshold || GetValue ( p.X - 1, p.Y, p.Z ) >= threshold || GetValue ( p.X, p.Y, p.Z + 1 ) >= threshold || GetValue ( p.X, p.Y, p.Z - 1 ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideYMin ]->AddNoDuplicates ( p.X, p.Y - shift, p.Z );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.Y % step.Y ) )
                surfp[ SideYMin ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.Y > limitmin.Y )
            surfp[ SideYMin ]->AddNoDuplicates ( p.X, Dim2 - 1, p.Z );
        } // for x, z
    }
OmpSectionEnd


OmpSectionBegin
TPointInt           p;

Gauge.Next ();

if ( surfp[ SideYMax ] ) {

    for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += step.X )
    for ( p.Z = boundmin.Z; p.Z <= boundmax.Z; p.Z += step.Z ) {

        for ( p.Y = boundmax.Y; p.Y >= limitmax.Y; p.Y -- )
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X + 1, p.Y, p.Z ) >= threshold || GetValue ( p.X - 1, p.Y, p.Z ) >= threshold || GetValue ( p.X, p.Y, p.Z + 1 ) >= threshold || GetValue ( p.X, p.Y, p.Z - 1 ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideYMax ]->AddNoDuplicates ( p.X, p.Y + shift, p.Z );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.Y % step.Y ) )
                surfp[ SideYMax ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.Y < limitmax.Y )
            surfp[ SideYMax ]->AddNoDuplicates ( p.X, 0, p.Z );
        } // for x, z
    }
OmpSectionEnd


OmpSectionBegin
TPointInt           p;

Gauge.Next ();

if ( surfp[ SideZMin ] ) {

    for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += step.X )
    for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += step.Y ) {

        for ( p.Z = boundmin.Z; p.Z <= limitmin.Z; p.Z ++ )
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X + 1, p.Y, p.Z ) >= threshold || GetValue ( p.X - 1, p.Y, p.Z ) >= threshold || GetValue ( p.X, p.Y + 1, p.Z ) >= threshold || GetValue ( p.X, p.Y - 1, p.Z ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideZMin ]->AddNoDuplicates ( p.X, p.Y, p.Z - shift );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.Z % step.Z ) )
                surfp[ SideZMin ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.Z > limitmin.Z )
            surfp[ SideZMin ]->AddNoDuplicates ( p.X, p.Y, Dim3 - 1 );
        } // for x, y
    }
OmpSectionEnd


OmpSectionBegin
TPointInt           p;

Gauge.Next ();

if ( surfp[ SideZMax ] ) {

    for ( p.X = boundmin.X; p.X <= boundmax.X; p.X += step.X )
    for ( p.Y = boundmin.Y; p.Y <= boundmax.Y; p.Y += step.Y ) {

        for ( p.Z = boundmax.Z; p.Z >= limitmax.Z; p.Z -- )
            if ( safe && ( GetValue ( p ) >= threshold || GetValue ( p.X + 1, p.Y, p.Z ) >= threshold || GetValue ( p.X - 1, p.Y, p.Z ) >= threshold || GetValue ( p.X, p.Y + 1, p.Z ) >= threshold || GetValue ( p.X, p.Y - 1, p.Z ) >= threshold )
            || ! safe &&   GetValue ( p ) >= threshold ) {
                surfp[ SideZMax ]->AddNoDuplicates ( p.X, p.Y, p.Z + shift );
                break;
                }
//              else if ( addintermediate )
            else if ( addintermediate && ! ( p.Z % step.Z ) )
                surfp[ SideZMax ]->Add ( p.X, p.Y, p.Z );

        if ( addback && p.Z < limitmax.Z )
            surfp[ SideZMax ]->AddNoDuplicates ( p.X, p.Y, 0 );
        } // for x, y
    }
OmpSectionEnd


OmpParallelSectionsEnd

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

if ( sort )
    for ( int s = 0; s < NumCubeSides; s++ )
        if ( surfp[ s ] )
            surfp[ s ]->Sort ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}

