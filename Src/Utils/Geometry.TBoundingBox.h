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

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TVolume;
class                               TPoints;


//----------------------------------------------------------------------------

template <class TypeD>
class   TBoundingBox
{
public:
                    TBoundingBox ();
                    TBoundingBox ( TypeD x1, TypeD x2, TypeD y1, TypeD y2, TypeD z1, TypeD z2 );
                    TBoundingBox ( const TPoints& points );
                    TBoundingBox ( const Volume& volume, bool automatic, double backvalue = 0 );
                    TBoundingBox ( const TVolume<UCHAR>&   volume, bool automatic, double backvalue = 0 );


                                            // set boundaries
    void            Reset   () ;
    void            Set     ( TypeD x1, TypeD x2, TypeD y1, TypeD y2, TypeD z1, TypeD z2 );
    void            Set     ( int nump, const TPointFloat* listp );
    void            Set     ( const TPoints& points );
    void            Set     ( const Volume& volume, bool automatic, double backvalue = 0 );
    void            Set     ( const TVolume<UCHAR>&   volume, bool automatic, double backvalue = 0 );


    bool            IsEmpty ()              const           { return xmin == 0 && xmax == 0 && ymin == 0 && ymax == 0 && zmin == 0 && zmax == 0; }


    TypeD           XMin    ()              const       { return xmin; }
    TypeD           XMax    ()              const       { return xmax; }
    TypeD           YMin    ()              const       { return ymin; }
    TypeD           YMax    ()              const       { return ymax; }
    TypeD           ZMin    ()              const       { return zmin; }
    TypeD           ZMax    ()              const       { return zmax; }
    TypeD           Min     ( int axis )    const       { return axis == 0 ? XMin () : axis == 1 ? YMin () : ZMin (); }
    TypeD           Max     ( int axis )    const       { return axis == 0 ? XMax () : axis == 1 ? YMax () : ZMax (); }
    void            GetMin  ( float *min )  const       { min[ 0 ] = xmin; min[ 1 ] = ymin; min[ 2 ] = zmin; }
    void            GetMax  ( float *max )  const       { max[ 0 ] = xmax; max[ 1 ] = ymax; max[ 2 ] = zmax; }
    TypeD           XMean   ()              const       { return ( xmin + xmax ) / 2; }
    TypeD           YMean   ()              const       { return ( ymin + ymax ) / 2; }
    TypeD           ZMean   ()              const       { return ( zmin + zmax ) / 2; }

    TypeD           GetXExtent    ()        const;      // data type-dependent, inlined to avoid duplicates
    TypeD           GetYExtent    ()        const;
    TypeD           GetZExtent    ()        const;
    TypeD           GetExtent     ( int axis )  const   { return axis == 0 ? GetXExtent () : axis == 1 ? GetYExtent () : GetZExtent (); }
    TypeD           GetMeanExtent ()        const       { return (TypeD) ( ( GetExtent ( 0 ) + GetExtent ( 1 ) + GetExtent ( 2 ) ) / 3 ); }
    TypeD           GetMaxExtent  ()        const       { return max ( GetExtent ( 0 ), GetExtent ( 1 ), GetExtent ( 2 ) ); }
    TypeD           GetMinExtent  ()        const       { return min ( GetExtent ( 0 ), GetExtent ( 1 ), GetExtent ( 2 ) ); }

    TypeD           XStep ( int numsteps )  const;          // data type-dependent, inlined to avoid duplicates
    TypeD           YStep ( int numsteps )  const;
    TypeD           ZStep ( int numsteps )  const;
    TypeD           Step  ( int numsteps, bool minimum ) const  { return  minimum ? min ( XStep ( numsteps ), YStep ( numsteps ), ZStep ( numsteps ) ) : max ( XStep ( numsteps ), YStep ( numsteps ), ZStep ( numsteps ) ); }

    void            Boundaries ( TypeD &xi, TypeD &xa, TypeD &yi, TypeD &ya, TypeD &zi, TypeD &za )     const
                                                            { xi = xmin; xa = xmax; yi = ymin; ya = ymax; zi = zmin; za = zmax; }
    TypeD           VolumicSize ()          const           { return GetXExtent() * GetYExtent() * GetZExtent(); }

    TypeD           MinSize     ()          const           { return min ( GetXExtent (), GetYExtent (), GetZExtent () ); }
    TypeD           MaxSize     ()          const           { return max ( GetXExtent (), GetYExtent (), GetZExtent () ); }
    TypeD           MeanSize    ()          const           { return ( GetXExtent () + GetYExtent () + GetZExtent () ) / 3; }
    double          BoxDiameter ()          const           { return sqrt ( (double) Square ( (double) GetXExtent () ) + Square ( (double) GetYExtent () ) + Square ( (double) GetZExtent () ) ); }


    double         *GetMiddle ( double *m )                     const   { m[ 0 ] = (double) ( xmin + xmax ) / 2; m[ 1 ] = (double) ( ymin + ymax ) / 2; m[ 2 ] = (double) ( zmin + zmax ) / 2; return m; }
    void            GetMiddle ( TPointFloat &v )                const   { v[ 0 ] = (double) ( xmin + xmax ) / 2; v[ 1 ] = (double) ( ymin + ymax ) / 2; v[ 2 ] = (double) ( zmin + zmax ) / 2; }
    void            GetMiddle ( TypeD &x, TypeD &y, TypeD &z )  const   { x = (double) ( xmin + xmax ) / 2; y = (double) ( ymin + ymax ) / 2; z = (double) ( zmin + zmax ) / 2; }
    void            GetMiddle ( TypeD &x, TypeD &y )            const   { x = (double) ( xmin + xmax ) / 2; y = (double) ( ymin + ymax ) / 2; }
                                        // center is not always the middle
    const TPointDouble& GetCenter ()                        const   { return center; }
    void            SetCenter ( double *v ); // set from outside the center, then compute new radius

                                        // radius is max distance from the center
    TypeD           Radius ()                               const   { return radius; }
    TypeD           GetXRadius    ()                        const   { return GetXExtent () / 2; }   // should be fine for both double and int
    TypeD           GetYRadius    ()                        const   { return GetYExtent () / 2; }
    TypeD           GetZRadius    ()                        const   { return GetZExtent () / 2; }
    TypeD           GetRadius     ( int axis )              const   { return axis == 0 ? GetXRadius () : axis == 1 ? GetYRadius () : GetZRadius (); }
                                        // absolute radius is distance from 0 to center (used for clipping)
    double          AbsRadius ()                                const   { double r0 = sqrt ( center.X * center.X + center.Y * center.Y + center.Z * center.Z ); double r = radius; return max ( r0, r ); }

    double          NormalizedRadius    ( const TPointDouble& p )   const;


    bool            Contains ( TypeD x, TypeD y, TypeD z )      const   { return x >= xmin && x <= xmax && y >= ymin && y <= ymax && z >= zmin && z <= zmax; }
    bool            Contains ( const float  *p )                const   { return p[ 0 ] >= xmin && p[ 0 ] <= xmax && p[ 1 ] >= ymin && p[ 1 ] <= ymax && p[ 2 ] >= zmin && p[ 2 ] <= zmax; }
    bool            Contains ( const double *p )                const   { return p[ 0 ] >= xmin && p[ 0 ] <= xmax && p[ 1 ] >= ymin && p[ 1 ] <= ymax && p[ 2 ] >= zmin && p[ 2 ] <= zmax; }
    bool            Contains ( const TBoundingBox<int> &b )     const   { return Contains ( b.XMin(), b.YMin(), b.ZMin() ) && Contains ( b.XMax(), b.YMax(), b.ZMax() ); }
    bool            Contains ( const TBoundingBox<double> &b )  // give a bit of margin when comparing doubles and ints
                                                                const   { return Contains ( b.XMin() + 0.5, b.YMin() + 0.5, b.ZMin() + 0.5 )
                                                                  && Contains ( b.XMax() - 0.5, b.YMax() - 0.5, b.ZMax() - 0.5 ); }
    bool            Contains ( const TPointDouble& p, double margin )   const;


    void            Show ( char *title = 0 )                    const;


    void            Expand  ( double delta );


                    TBoundingBox    ( const TBoundingBox &op );
    TBoundingBox&   operator    =   ( const TBoundingBox &op2 );

                                        // testing between 2 TBoundingBox
    bool                    operator    ==  ( const TBoundingBox<TypeD> &op2 )  const   { return     xmin == op2.xmin && xmax == op2.xmax && ymin == op2.ymin && ymax == op2.ymax && zmin == op2.zmin && zmax == op2.zmax;   }
    bool                    operator    !=  ( const TBoundingBox<TypeD> &op2 )  const   { return ! ( xmin == op2.xmin && xmax == op2.xmax && ymin == op2.ymin && ymax == op2.ymax && zmin == op2.zmin && zmax == op2.zmax ); }

    TBoundingBox<TypeD>&    operator    |=  ( const TBoundingBox<TypeD> &op2 )          { Mined ( xmin, op2.xmin ); Mined ( ymin, op2.ymin ); Mined ( zmin, op2.zmin ); Maxed ( ymax, op2.ymax ); Maxed ( zmax, op2.zmax ); Maxed ( xmax, op2.xmax ); center = ( center + op2.center ) / 2; radius = BoxDiameter() / 2; return  *this; }


protected:
    TypeD           xmin;
    TypeD           xmax;
    TypeD           ymin;
    TypeD           ymax;
    TypeD           zmin;
    TypeD           zmax;
    TPointDouble    center;
    TypeD           radius;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
            TBoundingBox<TypeD>::TBoundingBox ()
{
Reset ();
}


template <class TypeD>
void    TBoundingBox<TypeD>::Reset ()
{
xmin    = xmax  =
ymin    = ymax  =
zmin    = zmax  = 0;

center.Reset ();
radius  = 0;
}


//----------------------------------------------------------------------------
template <class TypeD>
            TBoundingBox<TypeD>::TBoundingBox ( TypeD x1, TypeD x2, TypeD y1, TypeD y2, TypeD z1, TypeD z2 )
{
Reset ();

Set ( x1, x2, y1, y2, z1, z2 );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TBoundingBox<TypeD>::TBoundingBox ( const TPoints& points )
{
Set ( points );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TBoundingBox<TypeD>::Set ( const TPoints& points )
{
Reset ();

if ( points.IsEmpty () )
    return;


float               minx            =  FLT_MAX;
float               miny            =  FLT_MAX;
float               minz            =  FLT_MAX;
float               maxx            = -FLT_MAX;
float               maxy            = -FLT_MAX;
float               maxz            = -FLT_MAX;


for ( int i = 0; i < (int) points; i++ ) {
    Mined ( minx, points[ i ].X );
    Mined ( miny, points[ i ].Y );
    Mined ( minz, points[ i ].Z );
    Maxed ( maxx, points[ i ].X );
    Maxed ( maxy, points[ i ].Y );
    Maxed ( maxz, points[ i ].Z );
    }


Set ( minx, maxx, miny, maxy, minz, maxz );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TBoundingBox<TypeD>::TBoundingBox ( const Volume& volume, bool automatic, double backvalue )
{
Set ( volume, automatic, backvalue );
}


template <class TypeD>
        TBoundingBox<TypeD>::TBoundingBox ( const TVolume<uchar>& volume, bool automatic, double backvalue )
{
Set ( volume, automatic, backvalue );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TBoundingBox<TypeD>::Set ( const Volume& volume, bool automatic, double backvalue )
{
Reset ();

if ( volume.IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              d;
double              t0              = volume.GetMaxValue () / 1000; // 0.1; // 1;
double              t1              = automatic ? volume.GetBackgroundValue () : backvalue;
int                 left    [ 2 ]   = { -1, -1 };
int                 top     [ 2 ]   = { -1, -1 };
int                 front   [ 2 ]   = { -1, -1 };
int                 right   [ 2 ]   = { -1, -1 };
int                 bottom  [ 2 ]   = { -1, -1 };
int                 back    [ 2 ]   = { -1, -1 };


                                        // scan with 2 thresholds
for ( int x = 0; x < volume.GetDim1 () && ( left[0] < 0 || left[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( left[0] < 0 || left[1] < 0 ); y++ )
for ( int z = 0; z < volume.GetDim3 () && ( left[0] < 0 || left[1] < 0 ); z++ ) {

//  d   = volume ( x, y, z );
    d   = fabs ( volume ( x, y, z ) );  // work in absolute values above threshold

    if ( d >= t0 && left[0] < 0 )   left[0] = x;
    if ( d >= t1 && left[1] < 0 )   left[1] = x;
    }


for ( int x = volume.GetDim1 () - 1; x >= 0 && ( right[0] < 0 || right[1] < 0 ); x-- )
for ( int y = 0; y < volume.GetDim2 () && ( right[0] < 0 || right[1] < 0 ); y++ )
for ( int z = 0; z < volume.GetDim3 () && ( right[0] < 0 || right[1] < 0 ); z++ ) {

    d   = fabs ( volume ( x, y, z ) );

    if ( d >= t0 && right[0] < 0 )  right[0] = x;
    if ( d >= t1 && right[1] < 0 )  right[1] = x;
    }


for ( int y = 0; y < volume.GetDim2 () && ( top[0] < 0 || top[1] < 0 ); y++ )
for ( int x = 0; x < volume.GetDim1 () && ( top[0] < 0 || top[1] < 0 ); x++ )
for ( int z = 0; z < volume.GetDim3 () && ( top[0] < 0 || top[1] < 0 ); z++ ) {

    d   = fabs ( volume ( x, y, z ) );

    if ( d >= t0 && top[0] < 0 )    top[0] = y;
    if ( d >= t1 && top[1] < 0 )    top[1] = y;
    }


for ( int y = volume.GetDim2 () - 1; y >= 0 && ( bottom[0] < 0 || bottom[1] < 0 ); y-- )
for ( int x = 0; x < volume.GetDim1 () && ( bottom[0] < 0 || bottom[1] < 0 ); x++ )
for ( int z = 0; z < volume.GetDim3 () && ( bottom[0] < 0 || bottom[1] < 0 ); z++ ) {

    d   = fabs ( volume ( x, y, z ) );

    if ( d >= t0 && bottom[0] < 0 ) bottom[0] = y;
    if ( d >= t1 && bottom[1] < 0 ) bottom[1] = y;
    }


for ( int z = 0; z < volume.GetDim3 () && ( front[0] < 0 || front[1] < 0 ); z++ )
for ( int x = 0; x < volume.GetDim1 () && ( front[0] < 0 || front[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( front[0] < 0 || front[1] < 0 ); y++ ) {

    d   = fabs ( volume ( x, y, z ) );

    if ( d >= t0 && front[0] < 0 )  front[0] = z;
    if ( d >= t1 && front[1] < 0 )  front[1] = z;
    }


for ( int z = volume.GetDim3 () - 1; z >= 0 && ( back[0] < 0 || back[1] < 0 ); z-- )
for ( int x = 0; x < volume.GetDim1 () && ( back[0] < 0 || back[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( back[0] < 0 || back[1] < 0 ); y++ ) {

    d   = fabs ( volume ( x, y, z ) );

    if ( d >= t0 && back[0] < 0 )   back[0] = z;
    if ( d >= t1 && back[1] < 0 )   back[1] = z;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // one more point for boundary
    if ( left[i] >= 0 ) {
        right [i]++;
        bottom[i]++;
        back  [i]++;
    /*
        if ( left > 0 )     left--;
        if ( top > 0 )      top--;
        if ( front > 0 )    front--;
        if ( right < Size.GetXExtent() - 2 )     right++;
        if ( bottom < Size.GetYExtent() - 2 )   bottom++;
        if ( back < Size.GetZExtent() - 2 )      back++;
    * /
        }
*/

                                        // take the one with smallest threshold
                                        // if it is reasonably close to the other (5%)
if ( fabs ( (double) ( left  [ 0 ] - left  [ 1 ] ) ) < 0.05 * volume.GetDim1 ()
  && fabs ( (double) ( right [ 0 ] - right [ 1 ] ) ) < 0.05 * volume.GetDim1 ()
  && fabs ( (double) ( top   [ 0 ] - top   [ 1 ] ) ) < 0.05 * volume.GetDim2 ()
  && fabs ( (double) ( bottom[ 0 ] - bottom[ 1 ] ) ) < 0.05 * volume.GetDim2 ()
  && fabs ( (double) ( front [ 0 ] - front [ 1 ] ) ) < 0.05 * volume.GetDim3 ()
  && fabs ( (double) ( back  [ 0 ] - back  [ 1 ] ) ) < 0.05 * volume.GetDim3 () )

    Set ( left[ 0 ], right[ 0 ], top[ 0 ], bottom[ 0 ], front[ 0 ], back[ 0 ] );
else                                                                        
    Set ( left[ 1 ], right[ 1 ], top[ 1 ], bottom[ 1 ], front[ 1 ], back[ 1 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if empty, set to max size
if ( XMin () == -1 && XMax () == -1 )

    Set ( 0, volume.GetDim1 () - 1,
          0, volume.GetDim2 () - 1,
          0, volume.GetDim3 () - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // estimate of barycenter
double              c[ 3 ];

                                        // set middle as the center
SetCenter ( GetMiddle ( c ) );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TBoundingBox<TypeD>::Set ( const TVolume<uchar>& volume, bool automatic, double backvalue )
{
Reset ();

if ( volume.IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uchar               d;
uchar               t0              = 1;
uchar               t1              = automatic ? AtLeast ( t0 + 1, volume.GetMaxValue () / 10 ) /*volume.GetBackgroundValue ()*/ : backvalue;
int                 left    [ 2 ]   = { -1, -1 };
int                 top     [ 2 ]   = { -1, -1 };
int                 front   [ 2 ]   = { -1, -1 };
int                 right   [ 2 ]   = { -1, -1 };
int                 bottom  [ 2 ]   = { -1, -1 };
int                 back    [ 2 ]   = { -1, -1 };


                                        // scan with 2 thresholds
for ( int x = 0; x < volume.GetDim1 () && ( left[0] < 0 || left[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( left[0] < 0 || left[1] < 0 ); y++ )
for ( int z = 0; z < volume.GetDim3 () && ( left[0] < 0 || left[1] < 0 ); z++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && left[0] < 0 )   left[0] = x;
    if ( d >= t1 && left[1] < 0 )   left[1] = x;
    }


for ( int x = volume.GetDim1 () - 1; x >= 0 && ( right[0] < 0 || right[1] < 0 ); x-- )
for ( int y = 0; y < volume.GetDim2 () && ( right[0] < 0 || right[1] < 0 ); y++ )
for ( int z = 0; z < volume.GetDim3 () && ( right[0] < 0 || right[1] < 0 ); z++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && right[0] < 0 )  right[0] = x;
    if ( d >= t1 && right[1] < 0 )  right[1] = x;
    }


for ( int y = 0; y < volume.GetDim2 () && ( top[0] < 0 || top[1] < 0 ); y++ )
for ( int x = 0; x < volume.GetDim1 () && ( top[0] < 0 || top[1] < 0 ); x++ )
for ( int z = 0; z < volume.GetDim3 () && ( top[0] < 0 || top[1] < 0 ); z++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && top[0] < 0 )    top[0] = y;
    if ( d >= t1 && top[1] < 0 )    top[1] = y;
    }


for ( int y = volume.GetDim2 () - 1; y >= 0 && ( bottom[0] < 0 || bottom[1] < 0 ); y-- )
for ( int x = 0; x < volume.GetDim1 () && ( bottom[0] < 0 || bottom[1] < 0 ); x++ )
for ( int z = 0; z < volume.GetDim3 () && ( bottom[0] < 0 || bottom[1] < 0 ); z++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && bottom[0] < 0 ) bottom[0] = y;
    if ( d >= t1 && bottom[1] < 0 ) bottom[1] = y;
    }


for ( int z = 0; z < volume.GetDim3 () && ( front[0] < 0 || front[1] < 0 ); z++ )
for ( int x = 0; x < volume.GetDim1 () && ( front[0] < 0 || front[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( front[0] < 0 || front[1] < 0 ); y++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && front[0] < 0 )  front[0] = z;
    if ( d >= t1 && front[1] < 0 )  front[1] = z;
    }


for ( int z = volume.GetDim3 () - 1; z >= 0 && ( back[0] < 0 || back[1] < 0 ); z-- )
for ( int x = 0; x < volume.GetDim1 () && ( back[0] < 0 || back[1] < 0 ); x++ )
for ( int y = 0; y < volume.GetDim2 () && ( back[0] < 0 || back[1] < 0 ); y++ ) {

    d   = volume ( x, y, z );

    if ( d >= t0 && back[0] < 0 )   back[0] = z;
    if ( d >= t1 && back[1] < 0 )   back[1] = z;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // one more point for boundary
    if ( left[i] >= 0 ) {
        right [i]++;
        bottom[i]++;
        back  [i]++;
    /*
        if ( left > 0 )     left--;
        if ( top > 0 )      top--;
        if ( front > 0 )    front--;
        if ( right < Size.GetXExtent() - 2 )     right++;
        if ( bottom < Size.GetYExtent() - 2 )   bottom++;
        if ( back < Size.GetZExtent() - 2 )      back++;
    * /
        }
*/

                                        // take the one with smallest threshold
                                        // if it is reasonably close to the other (5%)
if ( fabs ( (double) ( left  [ 0 ] - left  [ 1 ] ) ) < 0.05 * volume.GetDim1 ()
  && fabs ( (double) ( right [ 0 ] - right [ 1 ] ) ) < 0.05 * volume.GetDim1 ()
  && fabs ( (double) ( top   [ 0 ] - top   [ 1 ] ) ) < 0.05 * volume.GetDim2 ()
  && fabs ( (double) ( bottom[ 0 ] - bottom[ 1 ] ) ) < 0.05 * volume.GetDim2 ()
  && fabs ( (double) ( front [ 0 ] - front [ 1 ] ) ) < 0.05 * volume.GetDim3 ()
  && fabs ( (double) ( back  [ 0 ] - back  [ 1 ] ) ) < 0.05 * volume.GetDim3 () )

    Set ( left[ 0 ], right[ 0 ], top[ 0 ], bottom[ 0 ], front[ 0 ], back[ 0 ] );
else                                                                        
    Set ( left[ 1 ], right[ 1 ], top[ 1 ], bottom[ 1 ], front[ 1 ], back[ 1 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if empty, set to max size
if ( XMin () == -1 && XMax () == -1 )

    Set ( 0, volume.GetDim1 () - 1,
          0, volume.GetDim2 () - 1,
          0, volume.GetDim3 () - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // estimate of barycenter
double              c[ 3 ];

                                        // set middle as the center
SetCenter ( GetMiddle ( c ) );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TBoundingBox<TypeD>::Show ( char *title )   const
{
ShowValues ( StringIsEmpty ( title ) ? "Box Min"    : title, "fff", (double) XMin(),       (double) YMin(),       (double) ZMin() );
ShowValues ( StringIsEmpty ( title ) ? "Box Max"    : title, "fff", (double) XMax(),       (double) YMax(),       (double) ZMax() );
ShowValues ( StringIsEmpty ( title ) ? "Box Extent" : title, "fff", (double) GetXExtent(), (double) GetYExtent(), (double) GetZExtent() );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TBoundingBox<TypeD>::Expand ( double delta )
{
Set ( xmin - delta, xmax + delta, ymin - delta, ymax + delta, zmin - delta, zmax + delta );
}


//----------------------------------------------------------------------------
                                        // Test a point with a given tolerance
template <class TypeD>
bool    TBoundingBox<TypeD>::Contains ( const TPointDouble& p, double margin )  const
{
Maxed ( margin, 0.0 );

return  IsInsideLimits ( (double) p.X, xmin - margin * GetXExtent (), xmax + margin * GetXExtent () )
     && IsInsideLimits ( (double) p.Y, ymin - margin * GetYExtent (), ymax + margin * GetYExtent () )
     && IsInsideLimits ( (double) p.Z, zmin - margin * GetZExtent (), zmax + margin * GetZExtent () );
}


//----------------------------------------------------------------------------
                                        // Return the normalized distance to the ellipsoid centered in the box
                                        // 0 : center; < 1: within elliposoid; 1 : ellipsoid edge; > 1 beyond edge
template <class TypeD>
double  TBoundingBox<TypeD>::NormalizedRadius    ( const TPointDouble& p )  const
{
TPointDouble        pn ( p );
TPointDouble        middle;


GetMiddle ( middle );

pn     -= middle;

pn     /= TPointDouble ( GetXExtent () / 2, GetYExtent () / 2, GetZExtent () / 2 );

return  pn.Norm ();
}


//----------------------------------------------------------------------------
template <class TypeD>
        TBoundingBox<TypeD>::TBoundingBox ( const TBoundingBox &op )
{
xmin                = op.xmin;
xmax                = op.xmax;
ymin                = op.ymin;
ymax                = op.ymax;
zmin                = op.zmin;
zmax                = op.zmax;
center              = op.center;
radius              = op.radius;
}


template <class TypeD>
TBoundingBox<TypeD>&    TBoundingBox<TypeD>::operator= ( const TBoundingBox &op2 )
{
if ( &op2 == this )
    return  *this;


xmin                = op2.xmin;
xmax                = op2.xmax;
ymin                = op2.ymin;
ymax                = op2.ymax;
zmin                = op2.zmin;
zmax                = op2.zmax;
center              = op2.center;
radius              = op2.radius;


return  *this;
}


//----------------------------------------------------------------------------
                                        // Specialized versions of functions
template <class TypeD>
void    TBoundingBox<TypeD>::Set ( TypeD x1, TypeD x2, TypeD y1, TypeD y2, TypeD z1, TypeD z2 )
{
CheckOrder ( x1, x2 );
xmin    = x1;
xmax    = x2;

CheckOrder ( y1, y2 );
ymin    = y1;
ymax    = y2;

CheckOrder ( z1, z2 );
zmin    = z1;
zmax    = z2;

                                        // default center is middle
double              c[ 3 ];

SetCenter ( GetMiddle ( c ) );


radius      = BoxDiameter () / 2;
}


template <class TypeD>
void    TBoundingBox<TypeD>::Set ( int nump, const TPointFloat* listp )
{
if ( nump <= 0 || listp == 0 )  return;

xmin    =
xmax    = (TypeD) listp->X;
ymin    =
ymax    = (TypeD) listp->Y;
zmin    =
zmax    = (TypeD) listp->Z;


OmpParallelSectionsBegin

OmpSectionBegin
for ( int i = 1; i < nump; i++ ) {
    if ( listp[i].X < xmin )  xmin    = (TypeD) listp[i].X;
    if ( listp[i].X > xmax )  xmax    = (TypeD) listp[i].X;
    }
OmpSectionEnd

OmpSectionBegin
for ( int i = 1; i < nump; i++ ) {
    if ( listp[i].Y < ymin )  ymin    = (TypeD) listp[i].Y;
    if ( listp[i].Y > ymax )  ymax    = (TypeD) listp[i].Y;
    }
OmpSectionEnd

OmpSectionBegin
for ( int i = 1; i < nump; i++ ) {
    if ( listp[i].Z < zmin )  zmin    = (TypeD) listp[i].Z;
    if ( listp[i].Z > zmax )  zmax    = (TypeD) listp[i].Z;
    }
OmpSectionEnd

OmpParallelSectionsEnd

                                        // compute the barycenter rather than middle
double              x               = 0;
double              y               = 0;
double              z               = 0;

OmpParallelForSum ( x, y, z )

for ( int i = 0; i < nump; i++ ) {
    x += listp[i].X;
    y += listp[i].Y;
    z += listp[i].Z;
    }

x /= nump;
y /= nump;
z /= nump;

center = TPointDouble ( x, y, z );

                                        // compute max distance from the center
double  dx, dy, dz;
double  n2, r2;
r2 = 0;

for ( int i=0; i < nump; i++ ) {
    dx = listp[i].X - x;
    dy = listp[i].Y - y;
    dz = listp[i].Z - z;

    n2 = dx * dx + dy * dy + dz * dz;

    if ( n2 > r2 )
        r2 = n2;
    }

radius = sqrt ( r2 );
}


template <class TypeD>
void    TBoundingBox<TypeD>::SetCenter ( double *v )
{
                                        // check boundary
if ( v[0] < xmin || v[0] > xmax
  || v[1] < ymin || v[1] > ymax
  || v[2] < zmin || v[2] > zmax )
    return;

center.X = v[0];
center.Y = v[1];
center.Z = v[2];


double  cx, cy, cz;
double  r;
radius = 0;
                                        // scan the 8 borders to have the furthest point
for ( int x=0; x <= 1; x++ )
for ( int y=0; y <= 1; y++ )
for ( int z=0; z <= 1; z++ ) {

    cx = x * xmin + ( 1 - x ) * xmax - v[0];
    cy = y * ymin + ( 1 - y ) * ymax - v[1];
    cz = z * zmin + ( 1 - z ) * zmax - v[2];

    r = sqrt ( cx * cx + cy * cy + cz * cz );

    if ( r > radius )
        radius = Round ( r );
    }
}

                                        // double version, using real delta
template <>     inline  double  TBoundingBox<double>::GetXExtent ()             const   { return xmax - xmin; }
template <>     inline  double  TBoundingBox<double>::GetYExtent ()             const   { return ymax - ymin; }
template <>     inline  double  TBoundingBox<double>::GetZExtent ()             const   { return zmax - zmin; }

                                        // int version, using integer delta + 1 (f.ex. from 0 to 9 -> 10 bins)
template <>     inline  int     TBoundingBox<int>::GetXExtent ()                const   { return xmax - xmin + 1; }
template <>     inline  int     TBoundingBox<int>::GetYExtent ()                const   { return ymax - ymin + 1; }
template <>     inline  int     TBoundingBox<int>::GetZExtent ()                const   { return zmax - zmin + 1; }


                                        // double version, stepping is precise
template <>     inline  double  TBoundingBox<double>::XStep ( int numsteps )    const   { return  GetXExtent () / ( numsteps - 1 ); }
template <>     inline  double  TBoundingBox<double>::YStep ( int numsteps )    const   { return  GetYExtent () / ( numsteps - 1 ); }
template <>     inline  double  TBoundingBox<double>::ZStep ( int numsteps )    const   { return  GetZExtent () / ( numsteps - 1 ); }

                                        // int version, a truncated version of the integer ratio
template <>     inline  int     TBoundingBox<int>::XStep ( int numsteps )       const   { return  numsteps >= GetXExtent () ? 1 : Round ( (double) GetXExtent () / numsteps ); }
template <>     inline  int     TBoundingBox<int>::YStep ( int numsteps )       const   { return  numsteps >= GetYExtent () ? 1 : Round ( (double) GetYExtent () / numsteps ); }
template <>     inline  int     TBoundingBox<int>::ZStep ( int numsteps )       const   { return  numsteps >= GetZExtent () ? 1 : Round ( (double) GetZExtent () / numsteps ); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
