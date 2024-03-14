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

//#define     CHECKASSERT
#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Geometry.TTriangleSurface.h"

#include    "Math.Utils.h"
#include    "TSelection.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void    TTriangleSurface::SurfaceThroughPoints ( const TPoints& listp, const TSelection& sel, int type, double *params )
{
Reset ();

NumPoints           = (int) sel;
int                 nump            = NumPoints;

if ( nump <= 0 )    return;

MeanDistance        = listp.GetMedianDistance ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if     ( type == 3 /*Cluster3D*/ ) {
                                        // alloc space & copy the points
    ListPoints.Resize ( NumPoints );

    bool                nogood;
    int                 insert          = 0;
                                        // copy only unique points
    for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

        nogood = false;
                                        // scan all previous inserted points for duplicates
        for ( int p2 = 0; p2 < insert; p2++ )

            if ( ListPoints[ p2 ] == listp[ seli() ] ) {

                NumPoints--;
                nogood = true;
                break;
                }

        if ( nogood )   continue;

        ListPoints[ insert ].Index     = seli();    // this will be the original points index - needed later on
        ListPoints[ insert ].X         = listp[ seli() ].X;
        ListPoints[ insert ].Y         = listp[ seli() ].Y;
        ListPoints[ insert ].Z         = listp[ seli() ].Z;
        insert++;
        }


    ComputeDelaunay3D ();

    ComputeTrianglesNormals ();
                                        // no need to keep the sorted list
    ListPoints.DeallocateMemory ();
                                        // finally set this variable correctly
    NumPoints = 3 * NumTriangles;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( type == 1 /*ClusterLine*/ ) {

    NumPoints = 0;

    int                 MaxTriangles        = 4 * nump + 4 * ( nump - 1 );

    ListTriangles       .Resize ( 3 * MaxTriangles );
    ListTrianglesIndexes.Resize ( 3 * MaxTriangles );

    ComputeLine ( listp, sel, params[ 0 ] );

    ComputeTrianglesNormals ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( type == 2 /*ClusterGrid*/ ) {

    int                 Dim1            = params[ 1 ];
    int                 Dim2            = params[ 2 ];

    NumPoints = 0;

    int                 MaxTriangles        = 2 * ( Dim1 - 1 ) * ( Dim2 - 1 );

    ListTriangles       .Resize ( 3 * MaxTriangles );
    ListTrianglesIndexes.Resize ( 3 * MaxTriangles );

                                        // scan the grid
    NumPoints       = 0;

    for ( int d2 = 0; d2 < Dim2 - 1; d2++ )
    for ( int d1 = 0; d1 < Dim1 - 1; d1++ ) {

        int     i11     = sel.GetValue ( d2         * Dim1 + d1     );
        int     i12     = sel.GetValue ( d2         * Dim1 + d1 + 1 );
        int     i21     = sel.GetValue ( ( d2 + 1 ) * Dim1 + d1     );
        int     i22     = sel.GetValue ( ( d2 + 1 ) * Dim1 + d1 + 1 );

        ListTriangles [ NumPoints ] = listp[ i11 ];  ListTrianglesIndexes[ NumPoints++ ] = i11;
        ListTriangles [ NumPoints ] = listp[ i12 ];  ListTrianglesIndexes[ NumPoints++ ] = i12;
        ListTriangles [ NumPoints ] = listp[ i22 ];  ListTrianglesIndexes[ NumPoints++ ] = i22;

        ListTriangles [ NumPoints ] = listp[ i11 ];  ListTrianglesIndexes[ NumPoints++ ] = i11;
        ListTriangles [ NumPoints ] = listp[ i22 ];  ListTrianglesIndexes[ NumPoints++ ] = i22;
        ListTriangles [ NumPoints ] = listp[ i21 ];  ListTrianglesIndexes[ NumPoints++ ] = i21;

        } // for d1, d2

    NumTriangles  = NumPoints / 3;

    ComputeTrianglesNormals ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( type == 0 /*ClusterPoint*/ ) {

    NumPoints = 0;

    int                 MaxTriangles        = 6 * nump;

    ListTriangles       .Resize ( 3 * MaxTriangles );
    ListTrianglesIndexes.Resize ( 3 * MaxTriangles );

    ComputePoint ( listp, sel, params[ 0 ] );

    ComputeTrianglesNormals ();
    }
}


//------------------------------------------------------------------------------
                                        // Original method by Denis Brunet
                                        // It builds a 2D Delaunay tesselation from 3D points approximated locally on a 2D plane.
void    TTriangleSurface::ComputeDelaunay3D ()
{
                                        // get barycenter & extrema of data
D3DPoint            limitmin        = Highest ( limitmin.X );
D3DPoint            limitmax        = Lowest  ( limitmax.X );
D3DPoint            datacenter;

for ( int i = 0; i < NumPoints; i++ ) {

    datacenter += ListPoints[ i ];

    Mined ( limitmin.X, ListPoints[ i ].X );
    Mined ( limitmin.Y, ListPoints[ i ].Y );
    Mined ( limitmin.Z, ListPoints[ i ].Z );
    Maxed ( limitmax.X, ListPoints[ i ].X );
    Maxed ( limitmax.Y, ListPoints[ i ].Y );
    Maxed ( limitmax.Z, ListPoints[ i ].Z );
    }

                                        // for robustness, center is a mix of barycenter and extrema
datacenter  = (     datacenter / NumPoints 
                + ( limitmin + limitmax ) / 2 
              ) / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute mean data radius
D3Dfloat            dataradius      = 0;

for ( int i = 0; i < NumPoints; i++ )

    dataradius += ( ListPoints[ i ] - datacenter ).Norm ();

dataradius /= NumPoints;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for some "best" first edge: looking for the 2 closest points
D3Dfloat            mindist         = Highest ( mindist );
int                 insai           = TIndexNull;
int                 insbi           = TIndexNull;
                                        // scan all pairs of points, in case electrodes are not well spatially distributed
for ( int ai = 0;      ai < NumPoints - 1; ai++ )
for ( int bi = ai + 1; bi < NumPoints;     bi++ ) {

    D3Dfloat        norm2           = ( ListPoints[ bi ] - ListPoints[ ai ] ).Norm2 ();

    if ( norm2 > 0 && norm2 < mindist ) {

        mindist = norm2;
        insai   = ai;
        insbi   = bi;
        }
    }

                                        // this looks quite bad already...
if ( insai == TIndexNull || insbi == TIndexNull ) 
    return; 


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // structure used to build the triangulation
TTriangleNetwork    triangulation ( ListPoints );

                                        // insert first edge from which everything will expand
triangulation.InitEdge ( ListPoints[ insai ], ListPoints[ insbi ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Triangles candidates could be rejected according to various criteria:
D3Dfloat            maxdist         = 2.50 * MeanDistance;                              // 2.00 .. 2.50

D3Dfloat            maxradius       = 1.75 * dataradius / PowerRoot ( NumPoints, 3 );   // 1.65 .. 1.75 - old formula
//D3Dfloat          maxradius       = 1.20 * MeanDistance;                              // 1.00 .. 1.70 - new formula

//D3Dfloat          maxarea         = 3.75 / PowerRoot ( NumPoints, 3 );                // 3.5 .. 5     - old formula
D3Dfloat            maxarea         = 8.00 * Square ( MeanDistance );                   // 5.00 .. 8.00 - new formula

//D3Dfloat          minangle        = 30;                                               // 20 .. 35
//D3Dfloat          maxangle        = 100;                                              // 90 .. 105

                                        // edge nearly 0, or too long for our needs
auto                IsBadEdge           = [ maxdist ] ( const D3Dfloat& vn )    { return vn / maxdist < 1e-6 || vn > maxdist; };

auto                IsCircleRadiusTooBig= [ maxradius ] ( const D3Dfloat& R )   { return R > maxradius; };

auto                IsTriangleAreaTooBig= [ dataradius, maxarea ]   ( D3Dfloat normab, D3Dfloat normbc, D3Dfloat normac, const D3Dfloat& R )
{
double              area            = ( normab * normbc * normac ) / ( R * Square ( dataradius ) );
return  area > maxarea;
};

/*auto              AreBadAngles        = [ &vab, &vac, &vbc, minangle, maxangle ] ()
{
D3Dfloat            bac             = RadiansToDegrees ( acos ( Clip (   vab.ScalarProduct ( vac ), (D3Dfloat) -1.0, (D3Dfloat)1.0 ) ) );
D3Dfloat            cba             = RadiansToDegrees ( acos ( Clip ( - vab.ScalarProduct ( vbc ), (D3Dfloat) -1.0, (D3Dfloat)1.0 ) ) );
D3Dfloat            acb             = 180 - bac - cba; // RadiansToDegrees ( acos ( Clip (   vac.ScalarProduct ( vbc ), -1.0, 1.0 ) ) );
                                        // strict
//return  ! IsInsideLimits ( bac, minangle, maxangle )
//     || ! IsInsideLimits ( cba, minangle, maxangle )
//     || ! IsInsideLimits ( acb, minangle, maxangle ); // or sum >= 1
                                        // a bit more permissive
//return  ! IsInsideLimits ( bac, minangle, maxangle )
//      + ! IsInsideLimits ( cba, minangle, maxangle )
//      + ! IsInsideLimits ( acb, minangle, maxangle ) >= 2;
                                        // even more permissive
return  ! IsInsideLimits ( bac, minangle, maxangle )
      + ! IsInsideLimits ( cba, minangle, maxangle )
      + ! IsInsideLimits ( acb, minangle, maxangle ) >= 3;
};
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We search for a triangle <a, b, c>
int                 ai,         bi,         ci,         di,         circumdi;
D3DPoint            mab,        mac;
D3DPoint            center;
D3DVector           vab,        vac,        vbc,        vad;
D3DVector           voabac,     vcenterd;
D3DVector           voab,       voac;
D3Dfloat            norm2,      radius2;
D3Dfloat            cosangle;
D3Dfloat            sinangle;
D3Dfloat            normab;
D3Dfloat            normac;
D3Dfloat            normbc;
D3Dfloat            R;
D3Dfloat            trilab,     trilac;
D3Dfloat            temp;
bool                pointisin;
TEdge*              nextedge;

                                    // translates an original index to a local one (ListPoints)
auto                GetLocalIndex   = [ this ] ( int orgindex )
{
for ( int i = 0; i < NumPoints; i++ )
    if ( ListPoints[ i ].Index == orgindex )    return  i;
return  TIndexNull;
};


while ( ( nextedge = triangulation.GetNextBorder () ) != 0 ) {
                                        // convert original vertices indexes to local indexes
    ai  = GetLocalIndex ( nextedge->Vertex1->Index );
    bi  = GetLocalIndex ( nextedge->Vertex2->Index );

                                        // we have the first 2 points <a, b>
    const D3DPointI&        a       = ListPoints[ ai ];
    const D3DPointI&        b       = ListPoints[ bi ];


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // vector a-b
    vab     = b - a;
    normab  = vab.Norm ();

    if ( normab == 0 )      continue;   // this shouldn't happen

    vab    /= normab;

                                        // middle point of <a, b>
    mab     = ( a + b ) / (D3Dfloat) 2;

    pointisin   = true;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for c
    for ( ci = 0; ci < NumPoints; ci++ ) {

        if ( ci == ai || ci == bi )                 continue;

        const D3DPointI&        c       = ListPoints[ ci ];
                                        // rule out c if it is already sharing this edge in a triangle
        if ( triangulation.GetTriangle ( nextedge, c ) )    continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // vector a-c
        vac     = c - a;
        normac  = vac.Norm ();

        if ( normac == 0 )              continue;

        vac    /= normac;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a, b, c colinear (cosine == +- 1) ?
        cosangle    = vab.ScalarProduct ( vac );
        if ( abs ( cosangle ) >= 1 - 1e-6 ) continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // vector b-c
        vbc     = c - b;
        normbc  = vbc.Norm ();

        if ( normbc == 0 )              continue;

        vbc    /= normbc;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // orthogonal to a-b
        voab    = vac - vab * cosangle;
        voab.Normalize ();

                                        // middle point of a-c
        mac     = ( a + c ) / (D3Dfloat) 2;

                                        // orthogonal to a-c
        voac    = vab - vac * cosangle;
        voac.Normalize ();


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // circumradius
        temp    = ( normab + normbc + normac )
                * ( normbc + normac - normab )
                * ( normab + normbc - normac )
                * ( normab - normbc + normac );

        R       = ( normab * normbc * normac ) / sqrt ( temp > 0 ? temp : DBL_MIN );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Switching all these tests (including IsBadEdge) off should more or less generate a full convex hull
                                        // It might not do so all the time, though, due to the local planar approximation and the 3D circumradius test

                                        // Testing only the candidate new edges: ac and bc, but not ab, which already exists and is therefor trustworthy
        if ( IsBadEdge ( normac )                       // non non-sense edge
          || IsBadEdge ( normbc )                       // non non-sense edge
          || IsCircleRadiusTooBig ( R )                 // radius increases with flat triangles - big triangles also interfere somehow
          || IsTriangleAreaTooBig ( normab, normbc, normac, R ) )   continue;   // avoiding big triangles: underneath the cap, or around the ears
//        || AreBadAngles ()            )   continue;   // can reject some interesting triangles


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2 of the 3 trilinear coordinates
        trilab  =   R * ( vbc.ScalarProduct ( vac ) );  // cos C =   ca . cb
        trilac  = - R * ( vab.ScalarProduct ( vbc ) );  // cos B = - ab . bc

                                        // average of 2 estimations of center of sphere (to reduce numerical instability)
                                        // O is on orthogonal line, which crosses the middle of segment
                                        // and the length on the ortho line is R . cos Angle
        center  =  (   ( mab + voab * trilab ) 
                     + ( mac + voac * trilac ) ) 
                   / (D3Dfloat) 2;

        radius2 = Square ( R );

        voabac  = vab.VectorialProduct ( vac );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // any point inside the sphere a, b, c ?
        pointisin   = false;
        circumdi    = TIndexNull;

        for ( di = 0; di < NumPoints; di++ ) {

            if ( di == ai || di == bi || di == ci )     continue;

            const D3DPointI&        d       = ListPoints[ di ];

            vcenterd    = d - center;
            norm2       = vcenterd.Norm2 ();

            cosangle    = norm2 ? vcenterd.ScalarProduct ( voabac ) / sqrt ( norm2 )    // cosine of angle to triangle's normal
                                : 0;
                                        
            if ( abs ( norm2 - radius2 ) / radius2 < 1e-6               // 4 points on the same spheroid?
              && cosangle                          < 1e-6 )             // while being also coplanar?

                circumdi = di;

                                        // secret sauce: decreasing radius the closer d is to perpendicular to triangle ~= relatively increasing sphere height perpendicularly to triangle
            sinangle    = cosangle ? sqrt ( 1 - Square ( cosangle ) )   // sine: 0 if orthogonal, 1 if in triangle plane
                                   : 1;                                 // no change if d colinear to triangle plane

            norm2      *= ( 1 + sinangle ) / 2;                         // adjust length of vector d by  ( 1 + sine ) / 2: orthogonal d is shorten by 2; colinear d is kept unchanged

                                        // d is inside spheroid abc?
            pointisin   = norm2 < radius2;

            if ( pointisin )
                break;                  // triangle abc fails, no need to loop any further
            } // for di


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( ! pointisin )              // no points inside spheroid abc -> c is therefor a good match -> get out and insert new triangle abc
            break;
        } // for ci


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no point inside spheroid abc?
    if ( ! pointisin ) {
                                        // insert!
        const D3DPointI&        c       = ListPoints[ ci ];

        triangulation.AddTriangle ( a, b, c );


        if ( circumdi != TIndexNull ) {
                                        // insert another triangle straight away if on a circum-circle
                                        // test all cases for the correct triangle insertion
                                        // triangle could be ACD, ADB or BDC according to relative positions
            const D3DPointI&        circumd     = ListPoints[ circumdi ];

            vad     = circumd - a;

            if      ( vad.IsOppositeDirection ( voac ) )    triangulation.AddTriangle ( a, c,       circumd );
            else if ( vad.IsOppositeDirection ( voab ) )    triangulation.AddTriangle ( a, circumd, b       );
            else                                            triangulation.AddTriangle ( b, circumd, c       );
            }
        }

    } // while GetNextBorder


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling any remaining holes in the tesselation, from triangles that didn't fulfilled the criteria but which neighbors did
triangulation.FillHoles ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert the structure to a simple list of graphical triangles
NumTriangles            = triangulation.GetNumTriangles ();

ListTriangles       .Resize ( 3 * NumTriangles );
ListTrianglesIndexes.Resize ( 3 * NumTriangles );


const D3DPointI*    v[ 3 ];

for ( int ti = 0, t3 = 0; ti < NumTriangles; ti++ ) {

    triangulation.GetTriangleVertices ( ti, v );

                                        // vector from center to triangle center ("to outside")
    center      = ( *v[ 0 ] + *v[ 1 ] + *v[ 2 ] ) / (D3Dfloat) 3;

    vab         = *v[ 1 ] - *v[ 0 ];    // vector a-b

    vac         = *v[ 2 ] - *v[ 0 ];    // vector a-c
    
    voabac      = vab.VectorialProduct ( vac );


    ListTriangles       [ t3 ]  = *v[ 0 ];
    ListTrianglesIndexes[ t3 ]  =  v[ 0 ]->Index;
    t3++;

                                        // inserting counter-clockwise triangles
    if ( ( center - datacenter ).IsSameDirection ( voabac ) ) {

        ListTriangles       [ t3 ]  = *v[ 1 ];
        ListTrianglesIndexes[ t3 ]  =  v[ 1 ]->Index;
        t3++;

        ListTriangles       [ t3 ]  = *v[ 2 ];
        ListTrianglesIndexes[ t3 ]  =  v[ 2 ]->Index;
        t3++;
        }
    else {
        ListTriangles       [ t3 ]  = *v[ 2 ];
        ListTrianglesIndexes[ t3 ]  =  v[ 2 ]->Index;
        t3++;

        ListTriangles       [ t3 ]  = *v[ 1 ];
        ListTrianglesIndexes[ t3 ]  =  v[ 1 ]->Index;
        t3++;
        }
    }
}


//------------------------------------------------------------------------------
void    TTriangleSurface::ComputeLine ( const TPoints& listp, const TSelection& sel, double mindist )
{
                                        // compute 2 vectors, for doing a cross product
                                        // transfer points to vectors
TVector3Float       v1;
TVector3Float       vn;
TVector3Float       v1n;

v1  = listp[ sel.FirstSelected () ];
vn  = listp[ sel.LastSelected  () ];

                                        // main direction of line
v1n = vn - v1;
                                        // set width
//float   w = v1n.Norm() / ( nump > 1 ? nump - 1 : 1 ) * 0.75;
double              w               = mindist * 0.5;
v1n.Normalize();
                                        // set 3 orthogonals vectors
TVector3Float       vx;
TVector3Float       vy;
TVector3Float       vz;

vx.Reset ();
vy.Reset ();
vz.Reset ();

vx[ 0 ] = vy[ 1 ] = vz[ 2 ] = 1;

                                        // project them on line
double              nx              = vx.ScalarProduct ( v1n );
double              ny              = vy.ScalarProduct ( v1n );
double              nz              = vz.ScalarProduct ( v1n );


TVector3Float       vp1;
TVector3Float       vp2;

                                        // reject vx?
if ( fabs ( nx ) > fabs ( ny ) && fabs ( nx ) > fabs ( nz ) )
    vp1 = vy - ( v1n * ny ),
    vp2 = vz - ( v1n * nz );
                                        // reject vy?
else if ( fabs ( ny ) > fabs ( nx ) && fabs ( ny ) > fabs ( nz ) )
    vp1 = vx - ( v1n * nx ),
    vp2 = vz - ( v1n * nz );
                                        // reject vz?
else
    vp1 = vx - ( v1n * nx ),
    vp2 = vy - ( v1n * ny );

                                        // we have 2 orthogonals, projected vectors
vp1.Normalize();
vp2.Normalize();

                                        // set the width
vp1 *= w;
vp2 *= w;
v1n *= w / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector3Float       p1, p11, p12, p13, p14;
TVector3Float       p2, p21, p22, p23, p24;


NumPoints       = 0;
NumTriangles    = 0;
int             lastsel         = sel.LastSelected ();


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    int     i   = seli();
                                        // current line segment
    p1  = listp[ i ];

                                        // do an independant patch around each electrode
    p11 = p1 - vp1 - v1n;   p12 = p1 + vp1 - v1n;
    p13 = p1 - vp1 + v1n;   p14 = p1 + vp1 + v1n;
    p21 = p1 - vp2 - v1n;   p22 = p1 + vp2 - v1n;
    p23 = p1 - vp2 + v1n;   p24 = p1 + vp2 + v1n;

                                        // on first axis
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p13;   ListTrianglesIndexes[ NumPoints++ ] = i;
                                        // on second axis
    ListTriangles [ NumPoints ] = p21;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p22;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p24;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p21;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p24;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p23;   ListTrianglesIndexes[ NumPoints++ ] = i;

                                        // bridging between 2 electrodes?
    if ( i == lastsel )
        break;

    p2  = listp[ i + 1 ];
                                        // set triangles vertices
    p11 = p1 + vp1 + v1n;        p12 = p1 - vp1 + v1n;
    p13 = p1 + vp2 + v1n;        p14 = p1 - vp2 + v1n;
    p21 = p2 + vp1 - v1n;        p22 = p2 - vp1 - v1n;
    p23 = p2 + vp2 - v1n;        p24 = p2 - vp2 - v1n;

                                        // on first axis
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p21;   ListTrianglesIndexes[ NumPoints++ ] = i+1;
    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;

    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p21;   ListTrianglesIndexes[ NumPoints++ ] = i+1;
    ListTriangles [ NumPoints ] = p22;   ListTrianglesIndexes[ NumPoints++ ] = i+1;

                                        // on second axis
    ListTriangles [ NumPoints ] = p13;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p23;   ListTrianglesIndexes[ NumPoints++ ] = i+1;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;

    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p23;   ListTrianglesIndexes[ NumPoints++ ] = i+1;
    ListTriangles [ NumPoints ] = p24;   ListTrianglesIndexes[ NumPoints++ ] = i+1;
    }


NumTriangles  = NumPoints / 3;
}


//------------------------------------------------------------------------------
void    TTriangleSurface::ComputePoint ( const TPoints& listp, const TSelection& sel, double mindist )
{
                                        // set width
double              w               = mindist * 0.25;
                                        // set 3 orthogonals vectors
TVector3Float       vx;
TVector3Float       vy;
TVector3Float       vz;

vx.Reset ();
vy.Reset ();
vz.Reset ();

vx[ 0 ] = vy[ 1 ] = vz[ 2 ] = w;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector3Float       p1, p11, p12, p13, p14;


NumPoints       = 0;
NumTriangles    = 0;

for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    int     i   = seli();

                                        // current line segment
    p1  = listp[ i ];

                                        // choose a plane
    p11 = p1 - vy - vz;     p12 = p1 + vy - vz;
    p13 = p1 - vy + vz;     p14 = p1 + vy + vz;
                                        // add it
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p13;   ListTrianglesIndexes[ NumPoints++ ] = i;

                                        // choose a plane
    p11 = p1 - vx - vz;     p12 = p1 + vx - vz;
    p13 = p1 - vx + vz;     p14 = p1 + vx + vz;
                                        // add it
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p13;   ListTrianglesIndexes[ NumPoints++ ] = i;

                                        // choose a plane
    p11 = p1 - vx - vy;     p12 = p1 + vx - vy;
    p13 = p1 - vx + vy;     p14 = p1 + vx + vy;
                                        // add it
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p12;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p11;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p14;   ListTrianglesIndexes[ NumPoints++ ] = i;
    ListTriangles [ NumPoints ] = p13;   ListTrianglesIndexes[ NumPoints++ ] = i;
    }


NumTriangles  = NumPoints / 3;
}


//------------------------------------------------------------------------------
void    TTriangleSurface::ComputeTrianglesNormals ()
{
                                        // compute normal to triangles
ListTrianglesNormals.Resize ( NumTriangles );

TPointFloat         v1,  v2,  v3;

for ( int t = 0, t3 = 0; t < NumTriangles; t++ ) {

    v1      = ListTriangles[ t3++ ];
    v2      = ListTriangles[ t3++ ];
    v3      = ListTriangles[ t3++ ];

    v2     -= v1;
    v3     -= v1;

    v1      = v2.VectorialProduct ( v3 );
    v1.Normalize ();
    v1.Invert ();

    ListTrianglesNormals[ t ]   = v1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute normal to points
                                        // by averaging normals of neighbors triangles
ListPointsNormals.Resize ( 3 * NumTriangles );

int*                ti;
int                 t;
int                 numtri;
int                 tril[ 128 ];
int                 pidx;
//double            ps;

for ( int i = 0; i < 3 * NumTriangles; i++ ) {
    pidx = ListTrianglesIndexes[ i ];

    for ( numtri = 0, t = 0, ti = ListTrianglesIndexes; t < NumTriangles; t++, ti+=3 )
        if ( *ti == pidx || *(ti+1) == pidx || *(ti+2) == pidx )
            tril[ numtri++ ] = t;

    if ( numtri ) {
        v1      = 0.0;

        while ( numtri-- ) {
            v1     += ListTrianglesNormals[ tril[ numtri ] ];

/*                                      // sum up only coherent normals, ie close to triangle normal
            ps = ListTrianglesNormals[ tril[ numtri ] ].X * ListTrianglesNormals[ i / 3 ].X
               + ListTrianglesNormals[ tril[ numtri ] ].Y * ListTrianglesNormals[ i / 3 ].Y
               + ListTrianglesNormals[ tril[ numtri ] ].Z * ListTrianglesNormals[ i / 3 ].Z;

            if ( ps > 0.82 ) {
                n[0]    += ListTrianglesNormals[ tril[ numtri ] ].X;
                n[1]    += ListTrianglesNormals[ tril[ numtri ] ].Y;
                n[2]    += ListTrianglesNormals[ tril[ numtri ] ].Z;
                }
*/
            }
        }
    else
        v1      = 1.0;

    v1.Normalize ();
    ListPointsNormals[ i ] = v1;
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
