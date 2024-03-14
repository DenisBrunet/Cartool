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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Strings.Utils.h"
#include    "TList.h"

#include    "GlobalOptimize.Points.h"

#include    "ESI.TissuesConductivities.h"   // TissuesSpec

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

char                StringPointsFitting[ NumPointsFitting ][ 16 ]=
                    {
                    "Norm1",
                    "WNorm1",
                    "Norm2",
                    "WNorm2",

                    "Contain1",
                    "Contain2",
                    };


//----------------------------------------------------------------------------
        TFitModelOnPoints::TFitModelOnPoints ()
{
Reset ();
}


        TFitModelOnPoints::TFitModelOnPoints ( const TPoints& points )
{
Reset ();

Points      = points;
Center      = points.GetCenter ();
Bounding.Set ( Points );
}


void    TFitModelOnPoints::Reset ()
{
TGlobalOptimize::Reset ();

Points  .Reset ();
Center  .Reset ();
Bounding.Reset ();
}


//----------------------------------------------------------------------------
                                        // Version without testing for outliers
double  TFitModelOnPoints::Evaluate ( TEasyStats *stat )
{
//double            normz           = Bounding.ZMax ();
double              sum             = 0;
double              sumw            = 0;
TVector3Float       tooptimalcenter = GetTranslation ();

if ( stat )
    stat->Reset ();


TPointFloat         pm[ 12 ];

if ( How == ContainModelNorm1 
  || How == ContainModelNorm2 ) {
                                        // icosahedron / 12 vertices
//  double              phi             = ( 1 + sqrt ( 5.0 ) ) / 2;
    #define             phi             1.6180339887498948482045868343656

    pm[  0 ]    = TPointFloat (    0,   1,   phi ) + tooptimalcenter;   Transform ( pm[  0 ] );
    pm[  1 ]    = TPointFloat (    0,  -1,  -phi ) + tooptimalcenter;   Transform ( pm[  1 ] );
    pm[  2 ]    = TPointFloat (    0,   1,  -phi ) + tooptimalcenter;   Transform ( pm[  2 ] );
    pm[  3 ]    = TPointFloat (    0,  -1,   phi ) + tooptimalcenter;   Transform ( pm[  3 ] );
    pm[  4 ]    = TPointFloat (    1,  phi,    0 ) + tooptimalcenter;   Transform ( pm[  4 ] );
    pm[  5 ]    = TPointFloat (   -1, -phi,    0 ) + tooptimalcenter;   Transform ( pm[  5 ] );
    pm[  6 ]    = TPointFloat (    1, -phi,    0 ) + tooptimalcenter;   Transform ( pm[  6 ] );
    pm[  7 ]    = TPointFloat (   -1,  phi,    0 ) + tooptimalcenter;   Transform ( pm[  7 ] );
    pm[  8 ]    = TPointFloat (  phi,    0,    1 ) + tooptimalcenter;   Transform ( pm[  8 ] );
    pm[  9 ]    = TPointFloat ( -phi,    0,   -1 ) + tooptimalcenter;   Transform ( pm[  9 ] );
    pm[ 10 ]    = TPointFloat ( -phi,    0,    1 ) + tooptimalcenter;   Transform ( pm[ 10 ] );
    pm[ 11 ]    = TPointFloat (  phi,    0,   -1 ) + tooptimalcenter;   Transform ( pm[ 11 ] );

    #undef              phi
    }

                                        // minimize the least squares / distance between the model and the points
OmpParallelForSum ( sum, sumw )

for ( int i = 0; i < (int) Points; i++ ) {
                                        // !re-centered to current optimization center!
    TPointFloat     pto     = Points[ i ] + tooptimalcenter;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !input:  centered to points optimal center!
    TPointFloat     pmodel  = pto;
                                        // !output: centered to model!
    Transform ( pmodel );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // DIFFERENCE OF VECTORS'NORM, insensitive to any angular shift - we are interested in a model used to transform norms of vectors
                                        // !note that both points are optimally recentered as to avoid any NORM BIAS!
    double      tonorm      = pto   .Norm ();

    double      modelnorm   = pmodel.Norm ();

    double      dr          = modelnorm - tonorm;
                                        // setting a default weight of 1
    double      w           = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if      ( How == FitModelNorm1 ) {
                                        // distance - better for precision fitting
        sum    += fabs   ( dr );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( How == FitModelNorm2 ) {
                                        // variance - better with very noisy data
        sum    += Square ( dr );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( How == FitModelWeightedNorm1 ) {
                                        // top part weight always 1, bottom part fading down to 0
//      w       = Clip ( pto.Z / normz + 1, 0.0, 1.0 );
                                    // upper and back points have more weight - points with the less weight are the face one
        w       = max (     ( pto.Z - Bounding.ZMin () ) / Bounding.GetZExtent (),      // superior points (skull) == more weight / lower points (extrapolated) == less weight
                        1 - ( pto.Y - Bounding.YMin () ) / Bounding.GetYExtent ()  );   // anterior points (face)  == less weight
                                    // weighted distance
        sum    += w * fabs   ( dr );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( How == FitModelWeightedNorm2 ) {
                                        // top part weight always 1, bottom part fading down to 0
//      w       = Clip ( pto.Z / normz + 1, 0.0, 1.0 );
                                    // upper and back points have more weight - points with the less weight are the face one
        w       = max (     ( pto.Z - Bounding.ZMin () ) / Bounding.GetZExtent (),      // superior points (skull) == more weight / lower points (extrapolated) == less weight
                        1 - ( pto.Y - Bounding.YMin () ) / Bounding.GetYExtent ()  );   // anterior points (face)  == less weight
                                    // weighted variance
        sum    += w * Square ( dr );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( How == ContainModelNorm1 
           || How == ContainModelNorm2 ) {

        if ( dr >= 0 ) {
                                        // inside model
            double      d1      = ( pto - pm[  0 ] ).Norm2 ();
            double      d2      = ( pto - pm[  1 ] ).Norm2 ();
            double      d3      = ( pto - pm[  2 ] ).Norm2 ();
            double      d4      = ( pto - pm[  3 ] ).Norm2 ();
            double      d5      = ( pto - pm[  4 ] ).Norm2 ();
            double      d6      = ( pto - pm[  5 ] ).Norm2 ();
            double      d7      = ( pto - pm[  6 ] ).Norm2 ();
            double      d8      = ( pto - pm[  7 ] ).Norm2 ();
            double      d9      = ( pto - pm[  8 ] ).Norm2 ();
            double      d10     = ( pto - pm[  9 ] ).Norm2 ();
            double      d11     = ( pto - pm[ 10 ] ).Norm2 ();
            double      d12     = ( pto - pm[ 11 ] ).Norm2 ();

            double      dmin    = sqrt ( min ( min ( d1, d2, d3 ), min ( d4, d5, d6 ), min ( d7, d8, d9 ), min ( d10, d11, d12 ) ) );
            double      dmax    = sqrt ( max ( max ( d1, d2, d3 ), max ( d4, d5, d6 ), max ( d7, d8, d9 ), max ( d10, d11, d12 ) ) );

            if ( How == ContainModelNorm1 )

                sum        += sqrt (  modelnorm         // smaller model is better
                                    * tonorm )          // centered is better
                            * ( dmax - dmin );          // smaller differences between each model's side is better
            else // ContainModelNorm2

                sum        += modelnorm                 // smaller model is better
                            * tonorm                    // centered is better
                            * Square ( dmax - dmin );   // smaller differences between each model's side is better

/*                                      // another formula:
            double      dr1     = ( pto - pm[  0 ] ).Norm () - ( pto - pm[  1 ] ).Norm ();
            double      dr2     = ( pto - pm[  2 ] ).Norm () - ( pto - pm[  3 ] ).Norm ();
            double      dr3     = ( pto - pm[  4 ] ).Norm () - ( pto - pm[  5 ] ).Norm ();
            double      dr4     = ( pto - pm[  6 ] ).Norm () - ( pto - pm[  7 ] ).Norm ();
            double      dr5     = ( pto - pm[  8 ] ).Norm () - ( pto - pm[  9 ] ).Norm ();
            double      dr6     = ( pto - pm[ 10 ] ).Norm () - ( pto - pm[ 11 ] ).Norm ();

            if ( How == ContainModelNorm1 ) {
                dr1     = abs ( dr1 );
                dr2     = abs ( dr2 );
                dr3     = abs ( dr3 );
                dr4     = abs ( dr4 );
                dr5     = abs ( dr5 );
                dr6     = abs ( dr6 );

                sum    += max ( max ( dr1, dr2, dr3 ), max ( dr4, dr5, dr6 ) );
                }
            else {
                dr1     = Square ( dr1 );
                dr2     = Square ( dr2 );
                dr3     = Square ( dr3 );
                dr4     = Square ( dr4 );
                dr5     = Square ( dr5 );
                dr6     = Square ( dr6 );

                sum    += max ( max ( dr1, dr2, dr3 ), max ( dr4, dr5, dr6 ) );
                }
*/
            }
        else
            sum        += GOMaxEvaluation;  // not tolerating any point outside
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    sumw   += w;

    if ( stat )
        stat->Add ( fabs ( dr ) );
    }


return  sum / sumw;
}


//----------------------------------------------------------------------------
double      TFitModelOnPoints::GetMaxRadius ()  const
{
if      ( HasValue( Scale  ) )  return  GetValue ( Scale );
else if ( HasValue( ScaleX ) 
       && HasValue( ScaleY )
       && HasValue( ScaleZ ) )  return  max ( GetValue ( ScaleX ), GetValue ( ScaleY ), GetValue ( ScaleZ ) );
else                            return  1;
}


double      TFitModelOnPoints::GetMeanRadius () const
{
if      ( HasValue( Scale  ) )  return  GetValue ( Scale );
else if ( HasValue( ScaleX ) 
       && HasValue( ScaleY )
       && HasValue( ScaleZ ) )  return  ( GetValue ( ScaleX ) + GetValue ( ScaleY ) + GetValue ( ScaleZ ) ) / 3.0;
else                            return  1;
}


//----------------------------------------------------------------------------
                                        // Transform a point, not necessarily on the surface, to its corresponding point projected on the model surface
                                        // Returned point is model-centered
TPointFloat TFitModelOnPoints::ToModel ( TPointFloat p )    const
{
                                        // manually doing the first translation
p      += GetTranslation ();
                                        // project point onto model surface, full scale
Transform ( p );

return p;
}


TPoints     TFitModelOnPoints::ToModel ( const TPoints& listp, bool translate )     const
{
TPoints             listmodel;

for ( int i = 0; i < listp.GetNumPoints (); i++ )

    listmodel.Add ( ToModel ( listp[ i ] ) );

                                        // translate back to original space
if ( translate )
    listmodel  -= GetTranslation ();


return  listmodel;
}

                                        // Returns normalized (radius 1) values
TPointFloat TFitModelOnPoints::Spherize ( TPointFloat p )   const
{
                                        // manually doing the first translation
p      += GetTranslation ();
                                        // keeping a copy of original point, correctly centered
TPointFloat         pm ( p );
                                        // project point onto model surface, full scale
Transform ( pm );
                                        // rescale/normalize original point according to model boundary
p      /= pm.Norm ();
                                        // ?as a safety, don't allow anything past radius 1?
//if ( p.Norm2 () > 1 )
//    p.Normalize ();

return  p;
}


TDipole     TFitModelOnPoints::Spherize ( const TDipole& d )    const
{
TDipole             ds ( d );


ds.Position     = Spherize ( d.Position );

                                        // transform vector
ds.Direction    =   Spherize ( d.Position + d.Direction )
                  - ds.Position;
                                        // restore original norm
ds.Direction   *= d.Direction.Norm () / ds.Direction.Norm ();


return  ds;
}


TPoints     TFitModelOnPoints::Spherize ( const TPoints& listp )    const
{
TPoints             listsphsp;

for ( int i = 0; i < listp.GetNumPoints (); i++ )

    listsphsp.Add ( Spherize ( listp[ i ] ) );

return  listsphsp;
}


//----------------------------------------------------------------------------
                                        // Input should be normalized (radius <= 1) point
                                        // Returns point in original scale
TPointFloat TFitModelOnPoints::Unspherize ( TPointFloat p, bool translate ) const
{
                                        // keeping a copy of original point, correctly centered
TPointFloat         pm ( p );
                                        // project point onto model surface, full scale
Transform ( pm );
                                        // rescale spherized point according to model boundary
p      *= pm.Norm ();
                                        // translate back to original space
if ( translate )
    p      -= GetTranslation ();

return  p;
}


TPoints     TFitModelOnPoints::Unspherize ( const TPoints& listp )  const
{
TPoints             listunsphsp;

for ( int i = 0; i < listp.GetNumPoints (); i++ )

    listunsphsp.Add ( Unspherize ( listp[ i ], true ) );

return  listunsphsp;
}


//----------------------------------------------------------------------------
// To select which operations to perform, just allocate any of the needed parameters, f.ex.:
//
//   Sphere:                                TranslationX, Scale
//   Ellipsoid:                             TranslationX, ScaleX, ScaleY, ScaleZ
//   Ellipsoid + Pinch:                     TranslationX, ScaleX, ScaleY, ScaleZ, PinchXtoYZ
//   Ellipsoid + 2 Pinches:                 TranslationX, ScaleX, ScaleY, ScaleZ, PinchY, PinchZ
//   Ellipsoid + SinusPinch:                TranslationX, ScaleX, ScaleY, ScaleZ, SinusPinchXtoYZ
//   Ellipsoid + 2 SinusPinches:            TranslationX, ScaleX, ScaleY, ScaleZ, SinusPinchXtoY, SinusPinchXtoZ
//   Ellipsoid + SinusPinch + Hexagonal:    TranslationX, ScaleX, ScaleY, ScaleZ, SinusPinchXtoYZ, TopBumpX, TopLateralBumpX, RotateZ

                                        // Model unfortunately will induce some "drift" from the original point's direction
                                        // It needs some basic GO / search to correct and get the actual answer aligned to the original direction
void        TFitModelOnPoints::Transform ( TPointFloat& p ) const
{
TPointFloat         pcurr;
TPointFloat         o1;
TPointFloat         o2;

                                        // feeding normalized vectors to model estimation is totally fine
p.Normalize ();
                                        // current position is starting point
pcurr   = p;
                                        // get an orthogonal basis around starting point
pcurr.GetOrthogonalVectors ( o1, o2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              step        = tan ( DegreesToRadians ( 10   ) ) * SqrtTwo;  // absolute step of 10 degrees wide - also accounting for diagonal
double              precision   = tan ( DegreesToRadians (  0.1 ) );            // absolute precision, in degree
                                        // rescale vectorial steps right now
o1     *= step;
o2     *= step;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convergence by Cross-Hair search
TPointFloat         p0;     // central point
TPointFloat         pr;     // right 1 step
TPointFloat         pl;     // left
TPointFloat         pu;     // up
TPointFloat         pd;     // down

do {
    step   /= 2;
    o1     /= 2;    // divide the vectorial steps as well
    o2     /= 2;

    p0      = pcurr;
    pr      = pcurr + o1;
    pl      = pcurr - o1;
    pu      = pcurr + o2;
    pd      = pcurr - o2;

    _Transform ( p0 );
    _Transform ( pr );
    _Transform ( pl );
    _Transform ( pu );
    _Transform ( pd );

//  double              cos0    = p.Cosine ( p0 );
//  double              cosr    = p.Cosine ( pr );
//  double              cosl    = p.Cosine ( pl );
//  double              cosu    = p.Cosine ( pu );
//  double              cosd    = p.Cosine ( pd );
                                        // manually doing the cosine, saving computation on p.Norm2()
    p0.Normalize ();
    pr.Normalize ();
    pl.Normalize ();
    pu.Normalize ();
    pd.Normalize ();

    double              cos0    = p.ScalarProduct ( p0 );
    double              cosr    = p.ScalarProduct ( pr );
    double              cosl    = p.ScalarProduct ( pl );
    double              cosu    = p.ScalarProduct ( pu );
    double              cosd    = p.ScalarProduct ( pd );

                                        // Simple Cross-Hair search
                                        // cos0 will contain the cosine of last angular shift
    if      ( cos0 > cosr && cos0 > cosl ) {

        if      ( cos0 > cosu && cos0 > cosd );//pcurr += 0,            cos0    = cos0;     // remaining centered
        else if ( cosu > cosd )                 pcurr  += o2,           cos0    = cosu;
        else /*if ( cosd > cosu )*/             pcurr  -= o2,           cos0    = cosd;
        }

    else if ( cosr > cosl ) {

        if      ( cos0 > cosu && cos0 > cosd )  pcurr  +=   o1,         cos0    = cosr;
        else if ( cosu > cosd )                 pcurr  += ( o1 + o2 ),  cos0    = sqrt ( 2 - ( cosr * cosr + cosu * cosu ) );
        else /*if ( cosd > cosu )*/             pcurr  += ( o1 - o2 ),  cos0    = sqrt ( 2 - ( cosr * cosr + cosd * cosd ) );
        }

    else { // if ( cosl > cosr ) {

        if      ( cos0 > cosu && cos0 > cosd )  pcurr  -=   o1,         cos0    = cosl;
        else if ( cosu > cosd )                 pcurr  -= ( o1 - o2 ),  cos0    = sqrt ( 2 - ( cosl * cosl + cosu * cosu ) );
        else /*if ( cosd > cosu )*/             pcurr  -= ( o1 + o2 ),  cos0    = sqrt ( 2 - ( cosl * cosl + cosd * cosd ) );
        }

                                        // early break if current vector not moving much
//  if ( pcurr.IsAligned ( p, SingleFloatEpsilon ) )
    if ( cos0 > 1 - SingleFloatEpsilon )    // save sqrt's and stuff, we already know the cosine of best angle
        break;

    } while ( step > precision );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally transform the optimally corrected position
_Transform ( pcurr );

p   = pcurr;                                        // returning the actual model point
//p = porig * ( bestcurr.Norm () / porig.Norm () ); // returning the original point rescaled by model norm
}


void        TFitModelOnPoints::_Transform ( TPointFloat& p ) const
{
//if ( p.IsNull () )
//    return;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !Any translation re-centering has to be done before call, like:  p += GetTranslation ();!
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Orient to canonical model
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TMatrix44       mat;
                                        // Rotate from points space to model space (similarly to the MNI space)
    if ( HasValue( RotationZ ) )            mat.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );
    if ( HasValue( RotationY ) )            mat.RotateY ( GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationX ) )            mat.RotateX ( GetValue ( RotationX ), MultiplyLeft );

    mat.Apply ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // model starts on the unit sphere, then gets reshaped by various transforms
p.Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // it is possible to enforce Z symmetry - maybe the underlying spherical model could benefit from that? not tested, though
//bool                flipz           = p.Z < 0;
//
//if ( flipz )
//    p.Z     = -p.Z;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we need a normalized vector

                                        // Various flattening
if ( HasValue( FlattenX    ) )              p.X    += GetValue ( FlattenX    ) * sin ( p.X * HalfPi );
if ( HasValue( FlattenXPos ) && p.X > 0 )   p.X    += GetValue ( FlattenXPos ) * sin ( p.X * HalfPi );
if ( HasValue( FlattenXNeg ) && p.X < 0 )   p.X    += GetValue ( FlattenXNeg ) * sin ( p.X * HalfPi );


if ( HasValue( FlattenY    ) )              p.Y    += GetValue ( FlattenY    ) * sin ( p.Y * HalfPi );
if ( HasValue( FlattenYPos ) && p.Y > 0 )   p.Y    += GetValue ( FlattenYPos ) * sin ( p.Y * HalfPi );
if ( HasValue( FlattenYNeg ) && p.Y < 0 )   p.Y    += GetValue ( FlattenYNeg ) * sin ( p.Y * HalfPi );


if ( HasValue( FlattenZ    ) )              p.Z    += GetValue ( FlattenZ    ) * sin ( p.Z * HalfPi );
if ( HasValue( FlattenZPos ) && p.Z > 0 )   p.Z    += GetValue ( FlattenZPos ) * sin ( p.Z * HalfPi );
if ( HasValue( FlattenZNeg ) && p.Z < 0 )   p.Z    += GetValue ( FlattenZNeg ) * sin ( p.Z * HalfPi );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simulate top and top-lateral bumps (only)
                                        // compute needed variable
double              alpha;

//if ( ! flipz ) { // skipping fancy features for below the head

                                        // get angle in the coronal slice
if ( HasValue( TopBumpX ) || HasValue( TopLateralBumpX ) )      alpha   = RadiansToDegrees ( ArcTangent (  p.Y,  p.Z ) );
if ( HasValue( TopBumpY ) || HasValue( TopLateralBumpY ) )      alpha   = RadiansToDegrees ( ArcTangent ( -p.Z,  p.X ) );


if ( HasValue( TopBumpX ) || HasValue( TopLateralBumpX ) || HasValue( TopBumpY ) || HasValue( TopLateralBumpY ) ) {
                                        // then recenter to vertex, and allow positive / negative values
    alpha      -= 270.0;
    if ( alpha < -180 )     alpha  += 360;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // top bump is done with a powered cosine
#define             topbumpextent   45.0


if ( HasValue( TopBumpX ) && p.Y < 0
  && alpha > -topbumpextent && alpha < topbumpextent ) {

    double          alphanorm      = ( alpha + topbumpextent ) / topbumpextent / 2;
    double          ratio           = 1 + GetValue ( TopBumpX ) * Power ( sin ( alphanorm * Pi ), 2.00 + GetValue ( TopBumpX ) );

                                        // non-linear decrease contribution, to have a longer influence
    ratio   = Square ( p.X ) + ( 1 - Square ( p.X ) ) * ratio;

    p.Y    *= ratio;
    p.Z    *= ratio;
    }


if ( HasValue( TopBumpY ) && p.Z > 0
  && alpha > -topbumpextent && alpha < topbumpextent ) {

    double          alphanorm      = ( alpha + topbumpextent ) / topbumpextent / 2;
    double          ratio           = 1 + GetValue ( TopBumpY ) * Power ( sin ( alphanorm * Pi ), 2.00 + GetValue ( TopBumpY ) );

                                        // non-linear decrease contribution, to have a longer influence
    ratio   = Square ( p.Y ) + ( 1 - Square ( p.Y ) ) * ratio;

    p.X    *= ratio;
    p.Z    *= ratio;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // top-lateral bumps
#define             latbumpmiddle1  -55.0
#define             latbumpmiddle2   55.0
#define             latbumpextent    50.0
#define             latbumpxcenter    0.33


if ( HasValue( TopLateralBumpX )
  && (    alpha >= latbumpmiddle1 - latbumpextent && alpha <= latbumpmiddle1 + latbumpextent
       || alpha >= latbumpmiddle2 - latbumpextent && alpha <= latbumpmiddle2 + latbumpextent ) ) {

    double          alphanorm      = ( alpha - ( ( alpha < 0 ? latbumpmiddle1 : latbumpmiddle2 ) - latbumpextent ) ) / latbumpextent / 2;
    double          ratio           = 1 + GetValue ( TopLateralBumpX ) * Power ( sin ( alphanorm * Pi ), 1.25 );

                                        // linear decrease, shifted to the back
    double          xshifted        = fabs ( p.X - latbumpxcenter ) / ( 1 - latbumpxcenter );
    Clipped ( xshifted, (double) 0, (double) 1 );

    ratio   = xshifted + ( 1 - xshifted ) * ratio;

    p.Y    *= ratio;
    p.Z    *= ratio;
    }


if ( HasValue( TopLateralBumpY )
  && (    alpha >= latbumpmiddle1 - latbumpextent && alpha <= latbumpmiddle1 + latbumpextent
       || alpha >= latbumpmiddle2 - latbumpextent && alpha <= latbumpmiddle2 + latbumpextent ) ) {

    double          alphanorm      = ( alpha - ( ( alpha < 0 ? latbumpmiddle1 : latbumpmiddle2 ) - latbumpextent ) ) / latbumpextent / 2;
    double          ratio           = 1 + GetValue ( TopLateralBumpY ) * Power ( sin ( alphanorm * Pi ), 1.25 );

                                        // linear decrease, shifted to the back
    double          xshifted        = fabs ( -p.Y - latbumpxcenter ) / ( 1 - latbumpxcenter );
    Clipped ( xshifted, (double) 0, (double) 1 );

    ratio   = xshifted + ( 1 - xshifted ) * ratio;

    p.X    *= ratio;
    p.Z    *= ratio;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // "jaws" inflates on the sides, simpler case
if ( p.Z < 0 && HasValue( InflateLowZtoX ) ) {

                                        // up to which "vertical" angle do we spread the cosine (90° being the vertical / max )
    #define             lowinflatemaxx      80.0
    #define             lowinflatemaxy      80.0

    double      y           = p.Z;
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );

                                        // Cosine way:
                                        // inflating from 0 to a max, following a cos shape
    double      angle       = RadiansToDegrees ( ArcTangent ( fabs ( y ), x ) );
    double      ratio       = angle < lowinflatemaxx ? ( 1 - ( cos ( angle / lowinflatemaxx * Pi ) + 1 ) / 2 ) * GetValue ( InflateLowZtoX ) + 1
                                                     : GetValue ( InflateLowZtoX ) + 1;

    p.X    *= ratio;
    p.Z    *= ratio;
    }

/*                                      // inflate both in front and back
if ( p.Z < 0 && HasValue( InflateLowZtoY ) ) {

    double      y           = p.Z;
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );

                                        // a shift in cosine will generate a hollow part
    double      shift       = HasValue( InflateLowZtoYHollow ) ? GetValue ( InflateLowZtoYHollow ) : 0;
                                        // normalize angle [0..1]
    double      angle       = ArcTangent ( fabs ( y ), x ) / HalfPi;
                                        // radius from single parameter
    double      radius      = GetValue ( InflateLowZtoY );
                                        // make the cosine longer to get a negative part
    double      ratio       = ( 1 - ( cos ( ( angle * ( 1 + shift ) - shift ) * Pi ) + 1 ) / 2 ) * radius + 1;


    p.Y    *= ratio;
    p.Z    *= ratio;
    }
* /

                                        // inflate in front
if ( p.Z < 0 && p.Y >= 0 && HasValue( InflateLowZtoYPos ) ) {

    double      y           = p.Z;
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );

                                        // a shift in cosine will generate a hollow part
    double      shift       = HasValue( InflateLowZtoYPosHollow ) ? GetValue ( InflateLowZtoYPosHollow ) : 0;
                                        // normalize angle [0..1]
    double      angle       = ArcTangent ( fabs ( y ), x ) / HalfPi;
                                        // radius tends to average of front and back, to make the junction
    double      radius      =       angle   * GetValue ( InflateLowZtoYPos )
                            + ( 1 - angle ) * GetValue ( InflateLowZtoYPos );
                                        // make the cosine longer to get a negative part
    double      ratio       = ( 1 - ( cos ( ( angle * ( 1 + shift ) - shift ) * Pi ) + 1 ) / 2 ) * radius + 1;

                                        // limit expansion at the bottom
    ratio   = angle + ( 1 - angle ) * ratio;


    p.Y    *= ratio;
    p.Z    *= ratio;
    }

                                        // inflate in back - also needs the front parameters
if ( p.Z < 0 && p.Y < 0 && HasValue( InflateLowZtoYNeg ) ) {

    double      y           = p.Z;
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );

                                        // a shift in cosine will generate a hollow part
    double      shift       = HasValue( InflateLowZtoYNegHollow ) ? GetValue ( InflateLowZtoYNegHollow ) : 0;
                                        // normalize angle [0..1]
    double      angle       = ArcTangent ( fabs ( y ), x ) / HalfPi;
                                        // radius tends to average of front and back, to make the junction
    double      radius      =       angle   * GetValue ( InflateLowZtoYNeg )
                            + ( 1 - angle ) * GetValue ( InflateLowZtoYNeg );
                                        // make the cosine longer to get a negative part
    double      ratio       = ( 1 - ( cos ( ( angle * ( 1 + shift ) - shift ) * Pi ) + 1 ) / 2 ) * radius + 1;

                                        // limit expansion at the bottom
    ratio   = angle + ( 1 - angle ) * ratio;


    p.Y    *= ratio;
    p.Z    *= ratio;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Neck curvature, deflate below occipital bump
if ( p.Z < 0 && p.Y < 0 && HasValue( DeflateLowZtoXYNeg ) ) {

    double      y           = fabs ( p.Z );
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );
                                        // top-down angle is in [0..Pi/2]
    double      angle       = ArcTangent ( y, x ) / HalfPi - 0.15;
                                        // finally in [-1..1]
    angle       = Clip ( angle / ( angle < 0 ? 0.15 : 0.85 ), -1.0, 1.0 );

//  double      wside       = ( 1 - cos ( fabs ( angle ) * Pi ) ) / 2;
    double      wside       = ( ( 1 - cos ( fabs ( angle ) * Pi ) ) / 2                 // bump part itself
                               + ArcTangent ( fabs ( p.X ), fabs ( p.Y ) ) / HalfPi     // back part only
                              ) / 2;

    double      radius      = GetValue ( DeflateLowZtoXYNeg ) * ( 1 - wside ) + wside;


    p.X    *= radius;
    p.Y    *= radius;
    }

                                        // Supraorbital ridge, below equator, in front
if ( p.Z < 0 && p.Y > 0 && HasValue( DeflateLowZtoXYPos ) ) {

    double      y           = fabs ( p.Z );
    double      x           = sqrt ( p.X * p.X + p.Y * p.Y );
                                        // top-down angle is in [0..Pi/2]
    double      angle       = ArcTangent ( y, x ) / HalfPi - 0.10;
                                        // finally in [-1..1]
    angle       = Clip ( angle / ( angle < 0 ? 0.10 : 0.90 ), -1.0, 1.0 );

//  double      wside       = ( 1 - cos ( fabs ( angle ) * Pi ) ) / 2;
    double      wside       = ( ( 1 - cos ( fabs ( angle ) * Pi ) ) / 2                 // bump part itself
                               + ArcTangent ( fabs ( p.X ), fabs ( p.Y ) ) / HalfPi     // front part only
                              ) / 2;

    double      radius      = GetValue ( DeflateLowZtoXYPos ) * ( 1 - wside ) + wside;


    p.X    *= radius;
    p.Y    *= radius;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Pinching: conical / slope in X axis
if ( HasValue( PinchXtoYZ ) ) {
    double          ratio          = 1 + GetValue ( PinchXtoYZ ) * p.X;

                                            p.Y    *= ratio;
                                            p.Z    *= ratio;
    }

if ( HasValue( PinchYtoXZ ) ) {
    double          ratio          = 1 + GetValue ( PinchYtoXZ ) * p.Y;

                                            p.X    *= ratio;
                                            p.Z    *= ratio;
    }

if ( HasValue( PinchZtoXY ) ) {
    double          ratio          = 1 + GetValue ( PinchZtoXY ) * p.Z;

                                            p.X    *= ratio;
                                            p.Y    *= ratio;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
if ( HasValue( SinusPinchXtoYZ ) ) {
                                        // boost the linear pinch (from X) with the non-linear sinus
    double          ratio          = 1 + GetValue ( SinusPinchXtoYZ ) * sin ( p.X * HalfPi );

    p.Y    *= ratio;
    p.Z    *= ratio;
    }
*/

//    } // not flipz

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasValue( SinusPinchXtoY ) )           p.Y    *= 1 + GetValue ( SinusPinchXtoY ) * sin ( p.X * HalfPi );
if ( HasValue( SinusPinchXtoZ ) )           p.Z    *= 1 + GetValue ( SinusPinchXtoZ ) * sin ( p.X * HalfPi );

if ( HasValue( SinusPinchYtoX ) )           p.X    *= 1 + GetValue ( SinusPinchYtoX ) * sin ( p.Y * HalfPi );
if ( HasValue( SinusPinchYtoZ ) )           p.Z    *= 1 + GetValue ( SinusPinchYtoZ ) * sin ( p.Y * HalfPi );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we can modify the norm of vector
                                        // Scaling
if ( HasValue( Scale  ) )                   p      *= GetValue ( Scale  );
if ( HasValue( ScaleX ) )                   p.X    *= GetValue ( ScaleX );
if ( HasValue( ScaleY ) )                   p.Y    *= GetValue ( ScaleY );
if ( HasValue( ScaleZ ) )                   p.Z    *= GetValue ( ScaleZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasValue( ShearXtoY ) )                p.Y    += GetValue ( ShearXtoY ) * p.X;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time to flip back to correct orientation
//if ( flipz )
//    p.Z     = -p.Z;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TMatrix44       mat;
                                        // Revert to original points orientation - !revert order or rotations, too!
    if ( HasValue( RotationX ) )            mat.RotateX ( -GetValue ( RotationX ), MultiplyLeft );
    if ( HasValue( RotationY ) )            mat.RotateY ( -GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationZ ) )            mat.RotateZ ( -GetValue ( RotationZ ), MultiplyLeft );

    mat.Apply ( p );
    }
}


//----------------------------------------------------------------------------
void    TFitModelOnPoints::WriteFile ( const char* file, TVector3Float* translate ) const
{
                                        // plot the BF as points, for visualization
TPoints             points;
TPointFloat         p;
double              dtheta;
#define             deltaangle      0.1

                                        // sample surface sphere
points.SetSphericalSurfaceEqui ( 1500 );


for ( int i = 0; i < points.GetNumPoints (); i++ )
                                        // skipping centering part, as it is already centered by construct
    Transform ( points[ i ] );

                                        // translate back to original space
if ( translate )    points     -= *translate;       // specified by caller
else                points     -= GetTranslation ();// center of original points


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for visualization, show the origin
points.Add ( 0, 0, 0 );

//points.Sort ();

points.WriteFile ( file );
}


//----------------------------------------------------------------------------
/*
void    TFitModelOnPoints::ShowProgress ()
{
TFileName           file;


if ( Iteration < 100 ) {

    sprintf ( file, "%s\\Model On Points.Progress %03d."FILEEXT_XYZ, CrisTransfer.BaseFileName, Iteration );

    WriteFile ( file );
    }
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TPointsProperties::TPointsProperties ()
{
Reset ();
}


        TPointsProperties::TPointsProperties ( const TPoints& points )
{
Reset ();

Points              = points;
}


void    TPointsProperties::Reset ()
{
TGlobalOptimize::Reset ();

Points.Reset ();
}


//----------------------------------------------------------------------------
                                        // routing to the right function
double  TPointsProperties::Evaluate ( TEasyStats *stat )
{
if      ( How == PointsSagittal     )   return  EvaluateSagittal    ( stat );
else if ( How == PointsTransverse   )   return  EvaluateTransverse  ( stat );
else if ( How == PointsReorientTop  )   return  EvaluateToTop       ( stat );
else                                    return  0;
}


//----------------------------------------------------------------------------
                                        // There is no guarantee that it will end up with Y being the greatest axis - it could be any other axis
double  TPointsProperties::EvaluateSagittal ( TEasyStats *stat )
{
TEasyStats          statdr  ( Points.GetNumPoints() );
TPoints             transformedpoints ( Points );


if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the whole new set of transformed, non-null points
Transform ( transformedpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between of the shortest distances between points
OmpParallelFor

for ( int i = 0; i < transformedpoints.GetNumPoints(); i++ ) {
                                        // symmetrize according to plane YZ
    TPointFloat         pt (   -transformedpoints[ i ].X, 
                                transformedpoints[ i ].Y, 
                                transformedpoints[ i ].Z );

                                        // norm of the vectorial difference
    double              mind        = transformedpoints.GetMinimumDistance ( pt );

    statdr.Add ( mind, ThreadSafetyCare );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from 4 to 1 SD, when below convergence threshold OutliersPrecision
                                        // # of SD, % of data: 1 68%, 2 95%, 3 99%
double              limitdr         = CurrentPrecision < OutliersPrecision ? statdr.Average () + ( CurrentPrecision / OutliersPrecision * 3 + 1 ) * statdr.SD () : DBL_MAX;
double              sumsqr          = 0;
double              sumyaxis        = 0;
//int               numsumsqr       = 0;

                                        // some points might have been rejected
OmpParallelForSum ( sumsqr, sumyaxis )

for ( int i = 0; i < (int) statdr; i++ ) {

    double          dr          = statdr[ i ];

    if ( dr > limitdr )
        continue;

    sumsqr     += Square ( dr );                        // lateral distance

    sumyaxis   += Square ( transformedpoints[ i ].Y );  // front-back elongation

//  numsumsqr++;

    if ( stat ) 
        stat->Add ( dr );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimizing lateral error - however it can also converge to Y () symmetry
//return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;
                                        // minimizing lateral error while also favoring front-back max distance
return  sumyaxis ? sumsqr / sumyaxis : GOMaxEvaluation;
}


//----------------------------------------------------------------------------
                                        // Orienting the convex part to Z, looking like a default transverse plane
double  TPointsProperties::EvaluateTransverse ( TEasyStats *stat )
{
TPoints             transformedpoints ( Points );


if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the whole new set of transformed, non-null points
Transform ( transformedpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between of the shortest distances between points
double              sumsqr          = 0;
int                 numsumsqr       = 0;

OmpParallelForSum ( sumsqr, numsumsqr )

for ( int i = 0; i < transformedpoints.GetNumPoints(); i++ ) {

    TPointFloat         pt          = transformedpoints[ i ];

    pt.Normalize ();
                                        // being as closest to the Z axis -> minimize the other direction
    double              mind        = 1 - pt.Z;

    sumsqr      += Square ( mind );

    numsumsqr++;

    if ( stat )
        stat->Add ( mind );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;
}


//----------------------------------------------------------------------------
                                        // Not entirely satisfactory - EGI systems are a bit disbalanced
double  TPointsProperties::EvaluateToTop ( TEasyStats *stat )
{
TPoints             transformedpoints ( Points );


if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the whole new set of transformed, non-null points
Transform ( transformedpoints );

double              limitzmax   = transformedpoints.GetLimitMax ().Z;

//TBoundingBox<double>b ( transformedpoints );
//double              limitzmax   = b.ZMax ();
//double              closeto0    = b.GetZExtent () * 0.20;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between of the shortest distances between points
double              sumsqr          = 0;
int                 numsumsqr       = 0;


OmpParallelForSum ( sumsqr, numsumsqr )

for ( int i = 0; i < transformedpoints.GetNumPoints(); i++ ) {

    const TPointFloat&  pt          = transformedpoints[ i ];

                                        // minimize the Z distance to top
    double          mind        = limitzmax - pt.Z;
                                        // minimize the Z distance to top AND transverse plane spreading
    //double          mind        = ( limitzmax - pt.Z )
    //                            * sqrt ( Square ( pt.X ) + Square ( pt.Y ) );

    sumsqr      += Square ( mind );

    numsumsqr++;

    if ( stat )
        stat->Add ( mind );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;
}


//----------------------------------------------------------------------------

void    TPointsProperties::Transform ( TPointFloat& p )     const
{
                                        // global scaling first
if ( HasValue( Scale  ) )                   p      *= GetValue ( Scale  );
if ( HasValue( ScaleX ) )                   p.X    *= GetValue ( ScaleX );
if ( HasValue( ScaleY ) )                   p.Y    *= GetValue ( ScaleY );
if ( HasValue( ScaleZ ) )                   p.Z    *= GetValue ( ScaleZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations
if ( HasValue( TranslationX ) )             p.X    += GetValue ( TranslationX );
if ( HasValue( TranslationY ) )             p.Y    += GetValue ( TranslationY );
if ( HasValue( TranslationZ ) )             p.Z    += GetValue ( TranslationZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TMatrix44       mat;

    if ( HasValue( RotationX ) )            mat.RotateX ( GetValue ( RotationX ), MultiplyLeft );
    if ( HasValue( RotationY ) )            mat.RotateY ( GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationZ ) )            mat.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );

    mat.Apply ( p );
    }
}


void    TPointsProperties::Transform ( TPoints& points, bool reorient, TPoints* auxpoints )   const
{
OmpParallelFor

for ( int i = 0; i < points.GetNumPoints (); i++ )
    if ( points[ i ].IsNotNull () )
        Transform ( points[ i ] );


if ( auxpoints )
    for ( int i = 0; i < auxpoints->GetNumPoints (); i++ )
        Transform ( (*auxpoints)[ i ] );


if ( reorient && How == PointsTransverse )
    ResolveFrontBackOrientation  (  points, auxpoints  );
}


//----------------------------------------------------------------------------
                                        // Disambiguate front-back orientation (only) - assuming Z is to top
void    TPointsProperties::ResolveFrontBackOrientation  (   TPoints&   points,     TPoints*   auxpoints  )
{
TBoundingBox<double>    bounding ( points );
double                  fbdepth         =  0.30 * bounding.GetRadius ( 1 ); // magic numbers - don't touch
double                  udheight        = -0.20 * bounding.GetRadius ( 2 );
double                  balance         = 0;
TPointFloat             center;         bounding.GetMiddle ( center );


for ( int i = 0; i < points.GetNumPoints(); i++ ) {

    TPointFloat         p       = points[ i ];

    if ( p.IsNull () )     continue;

    p  -= center;
                                        // count lower, forward / backward electrodes
    if ( p.Z < udheight ) {
        if ( p.Y < -fbdepth )     balance++;
        if ( p.Y >  fbdepth )     balance--;
        }
    }


bool                istoback        = balance < 0;

if ( istoback ) {
                                        // rotate 180 degrees around Z
//  TMatrix44       mat;
//  mat.RotateZ ( 180, MultiplyLeft );
//  mat.Apply ( points );
                                        // simpler, faster
    for ( int i = 0; i < points.GetNumPoints(); i++ ) {
        points[ i ].X   = -points[ i ].X;
        points[ i ].Y   = -points[ i ].Y;
        }

    if ( auxpoints )
        for ( int i = 0; i < auxpoints->GetNumPoints(); i++ ) {
            (*auxpoints)[ i ].X     = -(*auxpoints)[ i ].X;
            (*auxpoints)[ i ].Y     = -(*auxpoints)[ i ].Y;
            }
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFitPointsOnPoints::TFitPointsOnPoints ()
{
Reset ();
}


        TFitPointsOnPoints::TFitPointsOnPoints  (   const TPoints&      frompoints, 
                                                    const TPoints&      topoints, 
                                                    const TMatrix44*    mriabstoguillotine
                                                )
{
Reset ();


FromPoints          = frompoints;
ToPoints            = topoints;

if ( mriabstoguillotine )
    MriAbsToGuillotine  = *mriabstoguillotine;
}


void    TFitPointsOnPoints::Reset ()
{
TGlobalOptimize::Reset ();

FromPoints        .Reset ();
ToPoints          .Reset ();

MriAbsToGuillotine.Reset ();
}


//----------------------------------------------------------------------------
                                        // routing to the right function
double  TFitPointsOnPoints::Evaluate ( TEasyStats *stat )
{
if      ( How == ClosestPoints )    return  EvaluatePoints ( stat );
else if ( How == MatchingPairs )    return  EvaluatePairs  ( stat );
else                                return  0;
}


//----------------------------------------------------------------------------
double  TFitPointsOnPoints::EvaluatePoints ( TEasyStats *stat )
{
TEasyStats              statdr ( FromPoints.GetNumPoints () );

if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between of the shortest distances between points
OmpParallelFor

for ( int i = 0; i < FromPoints.GetNumPoints (); i++ ) {

    TPointFloat     pt ( FromPoints[ i ] );

                                        // we might have null points, in which case we skip them
    if ( pt.IsNull () )
        continue;


    Transform ( pt );

                                        // clip electrodes below the neck cut
    if ( MriAbsToGuillotine.IsNotNull () ) {

        TPointFloat         cutit ( pt );

        MriAbsToGuillotine.Apply ( cutit );

        if ( cutit.Z < 0 )
            continue;
        }

                                        // norm of the vectorial difference
    double          mind        = GetMinimumDistance ( pt );

    statdr.Add ( mind, ThreadSafetyCare );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from 4 to 1 SD, when below convergence threshold OutliersPrecision
                                        // # of SD, % of data: 1 68%, 2 95%, 3 99%
double              limitdr         = CurrentPrecision < OutliersPrecision ? statdr.Average () + ( CurrentPrecision / OutliersPrecision * 3 + 1 ) * statdr.SD () : DBL_MAX;
double              sumsqr          = 0;
int                 numsumsqr       = 0;

                                        // some points might have been rejected
OmpParallelForSum ( sumsqr, numsumsqr )

for ( int i = 0; i < (int) statdr; i++ ) {

    double          dr          = statdr[ i ];

    if ( dr > limitdr )
        continue;

    sumsqr      += Square ( dr );   // better to avoid local convergence to a part of the target
//  sumsqr      += dr;

    numsumsqr++;

    if ( stat ) 
        stat->Add ( dr );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimizing the sum of errors
double          errordist       = numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;

                                        // penalize smaller transforms
double          errorsize       = HasValue ( Scale  ) ? 1 / ( GetValue ( Scale  ) + SingleFloatEpsilon )
                                : HasValue ( ScaleX ) ? 1 / ( CubicRoot ( GetValue ( ScaleX ) * GetValue ( ScaleY ) * GetValue ( ScaleZ ) ) + SingleFloatEpsilon )
                                :                       1;
                                        // combining both error metrics
return   errordist * errorsize;
//return   errordist;
}


//----------------------------------------------------------------------------
double  TFitPointsOnPoints::EvaluatePairs ( TEasyStats *stat )
{
TEasyStats              statdr ( FromPoints.GetNumPoints () );

if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares of distances between pairs of points
                                        // we run the two lists in parallel
OmpParallelFor

for ( int i = 0; i < FromPoints.GetNumPoints (); i++ ) {
                                        // !points should be "in sync", i.e. should have the exact same ordering!
    TPointFloat         pt          ( FromPoints[ i ] );
    const TPointFloat&  pto         = ToPoints  [ i ];

                                        // we might have null points, in which case we skip them
    if ( pt.IsNull () || pto.IsNull () )
        continue;


    Transform ( pt );

                                        // norm of the vectorial difference
    double          mind        = ( pto - pt ).Norm ();

    statdr.Add ( mind, ThreadSafetyCare );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from 4 to 1 SD, when below convergence threshold OutliersPrecision
                                        // # of SD, % of data: 1 68%, 2 95%, 3 99%
double              limitdr         = CurrentPrecision < OutliersPrecision ? statdr.Average () + ( CurrentPrecision / OutliersPrecision * 3 + 1 ) * statdr.SD () : DBL_MAX;
double              sum             = 0;
int                 numsum          = 0;

                                        // some points might have been rejected
OmpParallelForSum ( sum, numsum )

for ( int i = 0; i < (int) statdr; i++ ) {

    double          dr          = statdr[ i ];

    if ( dr > limitdr )
        continue;

//  sum    += Square ( dr );
    sum    += dr;

    numsum++;

    if ( stat )
        stat->Add ( dr );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  numsum ? sum / numsum : GOMaxEvaluation;
}


//----------------------------------------------------------------------------
/*
void    TFitPointsOnPoints::ShowProgress ()
{
TFileName           file;

TPoints             points          = FromPoints;

Transform ( points );


if ( Iteration < 100 ) {

    sprintf ( file, "%s\\Points To Points.Progress %03d."FILEEXT_XYZ, CrisTransfer.BaseFileName, Iteration );

    points.WriteFile ( file );
    }
}
*/

//----------------------------------------------------------------------------
                                        // find the minimum distance from a given point to the target model
double  TFitPointsOnPoints::GetMinimumDistance ( const TPointFloat &p )
{
const TPoints&      topoints        = ToPoints;

double              d;
double              mind            = DBL_MAX;


for ( int i = 0; i < (int) topoints; i++ ) {
                                        // norm of the vectorial difference
    d   = ( topoints[ i ] - p ).Norm2 ();

    if ( d < mind )
        mind    = d;
    }


return  sqrt ( mind );
}

                                        // Assume ToNormals has been set at that point
double  TFitPointsOnPoints::GetClosestPoint ( TPointFloat& p )
{
                                        // either use the octree (if it exists), or the whole list otherwise
const TPoints&      topoints        = ToPoints;

double              d;
double              mind            = DBL_MAX;
TPointFloat         minp;


for ( int i = 0; i < (int) topoints; i++ ) {
                                        // norm of the vectorial difference
    d   = ( topoints[ i ] - p ).Norm2 ();

    if ( d < mind ) {
        mind    = d;
        minp    = topoints [ i ];
        }
    }

                                        // copy the target point & normal
p       = minp;

return  sqrt ( mind );
}


//----------------------------------------------------------------------------
                                        // The transform goes from a "subject" points to an aligned target
                                        // To handle all cases, it would need
                                        // TranslateFrom -> RotateFrom -> Rescale -> RotateTo -> TranslateTo
void    TFitPointsOnPoints::Transform ( TPointFloat &p, bool scaling )
{
                                        // translations
if ( HasValue( TranslationX ) )             p.X    += GetValue ( TranslationX );
if ( HasValue( TranslationY ) )             p.Y    += GetValue ( TranslationY );
if ( HasValue( TranslationZ ) )             p.Z    += GetValue ( TranslationZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TMatrix44       mat;

    if ( HasValue( RotationX ) )            mat.RotateX ( GetValue ( RotationX ), MultiplyLeft );
    if ( HasValue( RotationY ) )            mat.RotateY ( GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationZ ) )            mat.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );

    mat.Apply ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( scaling ) {
    if ( HasValue( Scale  ) )                   p      *= GetValue ( Scale  );
    if ( HasValue( ScaleX ) )                   p.X    *= GetValue ( ScaleX );
    if ( HasValue( ScaleY ) )                   p.Y    *= GetValue ( ScaleY );
    if ( HasValue( ScaleZ ) )                   p.Z    *= GetValue ( ScaleZ );
    }
}


void    TFitPointsOnPoints::Transform ( TPoints& points, bool scaling )
{
OmpParallelFor

for ( int i = 0; i < points.GetNumPoints(); i++ )

    Transform ( points[ i ], scaling );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







