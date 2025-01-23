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

#include    <stdio.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "Math.Random.h"
#include    "Math.Resampling.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TFilters.h"
#include    "GlobalOptimize.Tracks.h"
#include    "TList.h"
#include    "Geometry.TPoints.h"
#include    "TArray3.h"
#include    "Files.Stream.h"

#include    "TVolumeDoc.h"

#include    "Files.Extensions.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    WritePoints  ( const char* file, const std::vector<ClusterPoints>& clusters )
{
if ( StringIsEmpty ( file ) || clusters.empty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                xyzformat       = IsExtension ( file, FILEEXT_XYZ   );
bool                elsformat       = IsExtension ( file, FILEEXT_ELS   );
bool                sxyzformat      = IsExtension ( file, FILEEXT_SXYZ  );
bool                spiformat       = IsExtension ( file, FILEEXT_SPIRR );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Retrieve the total number of points to write

int                     totalnumpoints  = 0;
bool                    hasnames        = false;
TBoundingBox<double>    bounding ( clusters[ 0 ].Points );


for ( const ClusterPoints& cluster : clusters ) {

    totalnumpoints += cluster.Points.GetNumPoints ();

    hasnames       |= cluster.Names.IsNotEmpty ();
                                        // expand bounding box
    bounding       |= TBoundingBox<double> ( cluster.Points );
    }


if ( totalnumpoints == 0 )
    return;


int                 numclusters     = clusters.size ();
                                        // globally bypassing names output?
bool                outputnames     = hasnames && ( xyzformat || elsformat || sxyzformat || spiformat );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Write header

ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ) );


if      ( xyzformat ) {
    ofs << StreamFormatGeneral;
    ofs << StreamFormatInt32   << totalnumpoints << Tab;
    ofs << StreamFormatFloat32 << bounding.Radius () / SqrtThree;   // radius is estimated from a box of points, while we rather have a spheroidal cloud
    ofs << NewLine;
    }

else if ( elsformat ) {
    ofs << StreamFormatGeneral;
    ofs << StreamFormatText    << ELSTXT_MAGICNUMBER1 << NewLine;
    ofs << StreamFormatInt32   << totalnumpoints      << NewLine;
    ofs << StreamFormatInt32   << numclusters         << NewLine;
    }

else if ( sxyzformat ) {
    ofs << StreamFormatGeneral;
    ofs << StreamFormatInt32   << totalnumpoints;
    ofs << NewLine;
    }

else if ( spiformat ) {
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                name[ 256 ];
char                buff[ 256 ];


for ( const ClusterPoints& cluster : clusters ) {

    const TPoints&      points          = cluster.Points;
    const TStrings&     names           = cluster.Names;
    int                 numpoints       = points.GetNumPoints ();

                                        // current cluster info
    if ( elsformat ) {

        StringCopy  ( name, StringIsNotEmpty ( cluster.ClusterName ) ? cluster.ClusterName : "Cluster" );
        
        ofs << StreamFormatGeneral;
        ofs << StreamFormatText    << name                  << NewLine;
        ofs << StreamFormatInt32   << numpoints             << NewLine;
        ofs << StreamFormatInt32   << cluster.ClusterType   << NewLine;
        }


    ofs << StreamFormatFixed;

    for ( int pi = 0; pi < numpoints; pi++ ) {

        ofs << StreamFormatFloat32 << points[ pi ].X << Tab;
        ofs << StreamFormatFloat32 << points[ pi ].Y << Tab;
        ofs << StreamFormatFloat32 << points[ pi ].Z;


        if ( outputnames ) {
                                        // current name exists?
            if ( pi < names.NumStrings () )

                StringCopy ( name, names[ pi ] );

            else {
                                        // provide a default name, which depends on guessed target, f.ex. electrodes or solution points
                if      ( xyzformat
                       || elsformat  )                           IntegerToString ( name, pi + 1, 0 );
                else if ( sxyzformat )  StringCopy ( name, "e",  IntegerToString ( buff, pi + 1, 0 ) );
                else if ( spiformat  )  StringCopy ( name, "sp", IntegerToString ( buff, pi + 1, 0 ) );
                else                                             IntegerToString ( name, pi + 1, 0 );
                }

            ofs << Tab << StreamFormatText << name;
            } // if outputnames


        ofs << NewLine;
        } // for points
    } // for cluster
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Definitions
char    PointsGeometryNames[ NumPointsGeometry ][ 32 ] = {
                    "",                             // no additional info to be shown
                    "Irregular",
                    "Non-aligned Grid",
                    "Aligned Grid",
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TPoints::TPoints ()
{
Reset ();
}


        TPoints::TPoints ( int numpoints )
{
Resize ( numpoints );
}

                                        // !There is no way to enforce Points to give away its memory!
void    TPoints::Reset ()
{
OmpCriticalBegin (TPointsReset)

NumPoints   = 0;
Points.clear ();

OmpCriticalEnd
}

                                        // Shrinking & expanding will keep content (but might move the whole data elsewhere)
void    TPoints::Resize ( int newnumpoints )
{
if ( newnumpoints <= 0 ) {
    Reset ();                           // to be consistent and avoid calls to resize(0)
    return;
    }


OmpCriticalBegin (TPointsResize)

NumPoints   = AtLeast ( 0, newnumpoints );  // safety for negative values, also legal to be 0 for resetting
Points.resize ( NumPoints );

OmpCriticalEnd
}


//----------------------------------------------------------------------------
void    TPoints::Show ( const char* title )     const
{
if ( IsEmpty () ) {
    ShowMessage ( "- Empty -", StringIsEmpty ( title ) ? "List of Points" : title );
    return;
    }

for ( int i = 0; i < NumPoints; i++ )

    ShowValues ( StringIsEmpty ( title ) ? "List of Points" : title, "fff", Points[ i ].X, Points[ i ].Y, Points[ i ].Z );
}


//----------------------------------------------------------------------------
void    TPoints::Set ( const TPoints& points )
{
                                        // in case of self assignation
if ( &points == this )
    return;

OmpCriticalBegin (TPointsSet)

NumPoints   = points.NumPoints;
Points      = points.Points;

OmpCriticalEnd
}


void    TPoints::Set ( const TPoints& points, const int* indexes, int numindexes )
{
if ( indexes == 0 || numindexes <= 0 ) {
    Reset ();
    return;
    }

Resize ( numindexes );

OmpCriticalBegin (TPointsSet)

for ( int i = 0; i < numindexes; i++ )
                                        // !no checks on limits of indexes!
    Points[ i ] = points[ indexes [ i ] ];

OmpCriticalEnd
}


//----------------------------------------------------------------------------
                                        // Set the 8 corners of a cube
void    TPoints::SetCorners ( double xmin, double ymin, double zmin, double xmax, double ymax, double zmax )
{
Reset ();

Add ( xmin, ymin, zmin );
Add ( xmin, ymin, zmax );
Add ( xmin, ymax, zmin );
Add ( xmin, ymax, zmax );
Add ( xmax, ymin, zmin );
Add ( xmax, ymin, zmax );
Add ( xmax, ymax, zmin );
Add ( xmax, ymax, zmax );
}


//----------------------------------------------------------------------------
/*
void    TPoints::SetLineOfPoints ( TPointFloat fromp, TPointFloat top, bool includingextrema, int subsampling )
{
Reset ();


TPointFloat         step            = top - fromp;
double              length          = step.Norm () * subsampling;
int                 numsteps        = length + ( includingextrema ? 1 : -1 );

step       /= length;

                                        // scan intermediate points, excluding the 2 extrema
for ( TPointFloat p = includingextrema ? fromp : fromp + step; numsteps > 0; numsteps--, p += step )
    Add ( p );
}
*/

void    TPoints::SetLineOfPoints ( TPointFloat fromp, TPointFloat top, bool includingextrema, int subsampling )
{
Reset ();


TPointFloat         dir             = top - fromp;
double              length          = dir.Norm () * subsampling;
                                              // adjust to include / exclude extrema
int                 numsteps        = length + ( includingextrema ? 0 : -2 );

                                        // check for degenerate case
if ( numsteps <= 0 /*|| length == 0*/ ) {
    Add ( fromp );
    return;
    }

                                        // shift the 2 extrema
if ( ! includingextrema ) {
    dir    /= length;
    fromp  += dir;
    top    -= dir;
    }
                                        // use barycentric coordinates, to avoid cumulative errors
                                        // and also more symmetrical in case fromp and top are perumtated
for ( int i = 0; i <= numsteps; i++ )
    Add ( fromp * ( 1 - (double) i / numsteps )
        + top   * (     (double) i / numsteps ) );
}


//----------------------------------------------------------------------------
void    TPoints::SetSphericalSurfaceEqui ( double deltaangle )
{
Reset ();


deltaangle      = DegreesToRadians ( deltaangle );

TPointFloat         p;
double              dtheta;
                                        // Theta is in XY plane (0..2Pi, Longitude), Phi is angle from Z (0..Pi, Latitude)     
for ( double phi = 0; phi <= Pi; phi += deltaangle ) {
                                        // real step
    dtheta  = 1.0 / ( sin ( phi ) + 1e-3 ) * deltaangle;
                                        // converted into an integer step, to have a nice stitching
    dtheta  = Round ( TwoPi / dtheta );

    if  ( dtheta == 0 )
        continue;

    dtheta  = TwoPi / dtheta;

    for ( double theta = 0; theta < TwoPi - dtheta * 0.5; theta += dtheta ) {

        p.ToCartesian ( theta, phi, 1.0 );

        Add ( p );
        }
    }
}


void    TPoints::SetSphericalSurfaceEqui ( int numpoints )
{
                                        // this will not give the exact number of points because of some rounding in equatorial circles
double              deltaangle      = ( 4 * 180 ) / sqrt ( numpoints * FourPi );

SetSphericalSurfaceEqui ( deltaangle );
}


//----------------------------------------------------------------------------
void    TPoints::SetSphericalSurfaceRnd ( int numpoints )
{
Reset ();


TRandSpherical      randsph;

for ( int i = 0; i < numpoints; i++ )

    Add ( randsph () );
}


//----------------------------------------------------------------------------
void    TPoints::SetSphericalSpiral ( int numpoints )
{
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameter zerotoone is the curvilinear axis, in [0..1]
auto                SpiralPoint = [] ( double zerotoone, int numcircles )
{
if      ( zerotoone <= 0.0 )    return  TPointDouble ( 0, 0,  1 );
else if ( zerotoone >= 1.0 )    return  TPointDouble ( 0, 0, -1 );
else {
    double      phi         = acos ( 1 - 2 * zerotoone );

    double      theta       = zerotoone * TwoPi * numcircles;

    TPointDouble        p;
    p.ToCartesian ( theta, phi, 1.0 );

    return  p;
    }
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numcircles      = AtLeast ( 1, Round ( sqrt ( numpoints / 16.0 ) ) );   // how many revolutions depends on the requested number of points
int                 numcumulength   = 360 * numcircles; // seems fair enough
TVector<double>     cumulength ( numcumulength );
TPointDouble        p1;
TPointDouble        p2;

                                        // init point
p2          = SpiralPoint ( 0.0, numcircles );
cumulength[ 0 ]  = 0;

                                        // build a table of spiral cumulated length, from the curvilenar dimension
for ( int i = 1; i < numcumulength; i++ ) {

    double      zerotoone   = i / (double) ( numcumulength - 1 );

    p1      = p2;

    p2      = SpiralPoint ( zerotoone, numcircles );
                                        // cumulate distances
    cumulength[ i ] = cumulength[ i - 1 ] + ( p1 - p2 ).Norm ();
    }


cumulength.NormalizeMax ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 lastx           = 0;

for ( int i = 0; i < numpoints; i++ ) {
                                        // straightforward formula, but will give unequal spacing
//  double      zerotoonex  = i / (double) ( numpoints - 1 );
                                        // the one we want for equidistant points
    double      zerotooney  = i / (double) ( numpoints - 1 );
                                        // the one we need to compute the point
    double      zerotoonex  = 0;

                                        // from first to penultimate point
    for ( ; lastx < numcumulength - 1; lastx++ )
                                        // inverse curve search
        if ( zerotooney >= cumulength[ lastx ] && zerotooney <= cumulength[ lastx + 1 ] ) {
                                        // get some linear interpolation weight
            double      w       = ( zerotooney              - cumulength[ lastx ] ) 
                                / ( cumulength[ lastx + 1 ] - cumulength[ lastx ] );

            zerotoonex  = ( lastx + w ) / (double) ( numcumulength - 1 );

            // NOT incrementing lastx, in case next point is still inside current limit...
            break;
            }
            
    Add ( SpiralPoint ( zerotoonex, numcircles ) );
    }
}


//----------------------------------------------------------------------------
void    TPoints::Add ( const TPoints& points )
{
                                        // something to add from?
if ( points.IsEmpty () )
    return;

                                        // anything to add to?
if ( IsEmpty () ) {
    Set ( points );
    return;
    }


OmpCriticalBegin (TPointsAdd)

NumPoints  += points.NumPoints;

for ( auto p = points.Points.cbegin (); p != points.Points.cend (); p++ )

    Points.push_back ( *p );

OmpCriticalEnd
}


void    TPoints::Add ( const TPointFloat& point )
{
OmpCriticalBegin (TPointsAdd)

NumPoints++;
Points.push_back ( point );

OmpCriticalEnd
}


void    TPoints::Add ( const Volume& volume, double x0, double y0, double z0, double scale )
{
int                 x;
int                 y;
int                 z;

for ( int i = 0; i < volume.GetLinearDim () ; i++ )

    if ( volume[ i ] ) {

        volume.LinearIndexToXYZ ( i, x, y, z );

        Add ( TPointFloat ( x0 + x * scale, y0 + y * scale, z0 + z * scale ) );
        }
}


void    TPoints::AddNoDuplicates ( const TPointFloat& point )
{
if ( ! Contains ( point ) )

    Add ( point );
}


void    TPoints::AddNoDuplicates ( const TPoints& points )
{
for ( int i = 0; i < points.NumPoints; i++ )

    AddNoDuplicates ( points[ i ] );
}


void    TPoints::RemoveLast ( int howmany )
{
if ( howmany <= 0 )
    return;

if ( howmany >= NumPoints ) {
    Reset ();
    return;
    }
                                        // that will do it
Resize ( NumPoints - howmany );
}


//----------------------------------------------------------------------------
                                        // No other threads should modify Points while testing for existence
const TPointFloat*  TPoints::Contains ( const TPointFloat& point )  const
{
if ( IsEmpty () )
    return  0;


const TPointFloat*  topoint         = 0;

OmpCriticalBegin (TPointsContains)

for ( int i = 0; i < NumPoints; i++ )

    if ( Points[ i ] == point ) {                    // exact same point
//  if ( ( Points[ i ] - point ).Norm2 () < 1e-6 ) { // nearly same point

        topoint = &Points[ i ];
        break;
        }

OmpCriticalEnd

return  topoint;
}


//----------------------------------------------------------------------------
        TPoints::TPoints ( const TPoints &op )
{
Set ( op );
}


TPoints&    TPoints::operator= ( const TPoints &op2 )
{
if ( &op2 == this )
    return  *this;

Set ( op2 );

return  *this;
}


TPoints&    TPoints::operator= ( const Volume& op2 )
{
Reset ();

Add ( op2, 0, 0, 0, 1 );

return  *this;
}


//----------------------------------------------------------------------------
                                        // !Not thread-safe!
TPoints&    TPoints::operator+= ( double op2 )
{
if ( op2 == 0 )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]   += op2;

return  *this;
}


TPoints&   TPoints::operator+= ( const TPointFloat &op2 )
{
if ( op2.IsNull () )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    += op2;

return  *this;
}


TPoints&   TPoints::operator+= ( const TPointDouble &op2 )
{
if ( op2.IsNull () )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    += TPointFloat ( op2 );

return  *this;
}


TPoints&   TPoints::operator+= ( const TPoints& op2 )
{
Add ( op2 );

return  *this;
}

                                        // !Not thread-safe!
TPoints&    TPoints::operator-= ( double op2 )
{
if ( op2 == 0 )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]   -= op2;

return  *this;
}


TPoints&   TPoints::operator-= ( const TPointFloat &op2 )
{
if ( op2.IsNull () )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    -= op2;

return  *this;
}


TPoints&   TPoints::operator-= ( const TPointDouble &op2 )
{
if ( op2.IsNull () )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    -= TPointFloat ( op2 );

return  *this;
}


TPoints&   TPoints::operator/= ( double op2 )
{
if ( op2 == 0 || op2 == 1 )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    /= op2;

return  *this;
}


TPoints&   TPoints::operator/= ( const TPointFloat &op2 )
{
for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    /= op2;

return  *this;
}


TPoints&   TPoints::operator/= ( const TArray1<float> &op2 )
{
for ( int i = 0; i < NumPoints; i++ )
    if ( op2[ i ] )
        Points[ i ]    /= op2[ i ];

return  *this;
}

                                        // !Not thread-safe!
TPoints&    TPoints::operator*= ( double op2 )
{
if ( op2 == 1 )
    return  *this;

for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    *= op2;

return  *this;
}

                                        // !Not thread-safe!
TPoints&    TPoints::operator*= ( const TPointFloat& op2 )
{
for ( int i = 0; i < NumPoints; i++ )
    Points[ i ]    *= op2;

return  *this;
}


//----------------------------------------------------------------------------
                                        // Remove points too close to each others, aiming at the requested target size
void    TPoints::DownsamplePoints   (   int     targetsize  )
{
                                        // no need to downsample?
if ( NumPoints <= targetsize )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             points ( *this );   // copy to an array for faster access
TArray1<bool>       removed ( NumPoints );
TPointFloat         pi;
TPointFloat         pj;

                                        // set a good radius estimate, then use a loop to converge
double              radius2     = (double) NumPoints / targetsize;
double              radius      = sqrt ( radius2 );


double              gaugemax        = 0;

TSuperGauge         Gauge ( "Surface Downsampling", 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numguess        = 0;
int                 numremain;
int                 numremoved;


do {

    removed     = false;
    numremoved  = 0;

                                        // scan all points
    for ( int i = 0; i < (int) points; i++ ) {

        if ( removed[ i ] )     continue;

        pi  = points[ i ];
                                        // scan successive points
        for ( int j = i + 1; j < (int) points; j++ ) {

            if ( removed[ j ] )     continue;

            pj  = points[ j ];
                                        // early break, points are sorted in X
            if ( pj.X > pi.X + radius )
                break;
                                        // real distance test
            if ( ( pi - pj ).Norm2 () < radius2 ) {
                removed[ j ]    = true;
                numremoved++;
                }
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now see what we have...
    numremain   = (int) points - numremoved;

//    DBGV5 ( (int) points, targetsize, radius, numremain, 1.0 * ( 1 + 12 * Power ( numguess / 100.0, 3 ) ), "#points  #target  radius  #remain  threshold%" );

                                        // stop if we are close enough to the requested # of points
                                        // preferring to be above than below, and relaxing the criterion with the # of iterations
    if ( RelativeDifference ( numremain, targetsize ) < 0.01 * ( 1 + 12 * Cube ( numguess / 100.0 ) ) )
        break;

                                        // update step according to current guess
    radius2    *= numremain > targetsize ? 1.07 : 1 / 1.05;
    radius      = sqrt ( radius2 );
    numguess++;


    Maxed ( gaugemax, 1 - 10 * RelativeDifference ( numremain, targetsize ) );

    Gauge.SetValue ( SuperGaugeDefaultPart, 100 * gaugemax );

    } while ( numguess < 100 );


Gauge.Finished ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer the remaining points
Reset ();

for ( int i = 0; i < (int) points; i++ )
    if ( ! removed[ i ] )
        Add ( points[ i ] );
}


//----------------------------------------------------------------------------
                                        // Get all surface points of the MRI
                                        // Points are in relative voxel position (0 on the corner),
                                        // and on the outside part of the surface, for a better viewing
void    TPoints::GetSurfacePoints ( 
                                const Volume&       volume, 
                                const TPointFloat&  center,
                                bool                includebottom
                                )
{
Reset ();

if ( volume.IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Extract mask from volume
Volume              mask ( volume );
double              back            = volume.GetBackgroundValue ();
double              backmask        = 1;


FctParams           params;

params ( FilterParamToMaskThreshold )     = back;
params ( FilterParamToMaskNewValue  )     = backmask;
//params ( FilterParamToMaskCarveBack )   = false;               // not carving - make it a full mask, as we don't want any points inside any open hole of the surface, like sinus
params ( FilterParamToMaskCarveBack )     = true;                // carving     - make better points around the eyes / nose / neck in case of high curvature


mask.Filter ( FilterTypeToMask, params );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TPointInt           step;
TPointFloat         middle ( volume.GetDim ( 0 ) / 2, volume.GetDim ( 1 ) / 2, volume.GetDim ( 2 ) / 2 );
TPoints*            surfps[ NumCubeSides ];


//DBGV ( back, "GetSurfacePoints Threshold" );

for ( int s = 0; s < NumCubeSides; s++ )
    surfps[ s ] = new TPoints;


if ( ! includebottom && surfps[ SideZMin ] != 0 ) {
    delete  surfps[ SideZMin ];
    surfps[ SideZMin ]  = 0;
    }


step.X  = step.Y    = step.Z    = volume.Step ( 128, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get all surface points, from all sides of the cube
                                        // use the hole-less mask instead of the real head
//volume.GetSurfacePoints (   surfps, 
//                            back, 
mask  .GetSurfacePoints (   surfps,     // using mask instead of original volume
                            backmask,
                            step, 
                            middle, 
                            1.50, 
                            (SurfacePointsFlags) ( SurfaceOutside | SurfaceSort | SurfaceShowProgress ) 
                        );

                                        // cumulate all points
for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfps[ s ] )
        AddNoDuplicates ( *surfps[ s ] );


for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfps[ s ] )
        delete  surfps[ s ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sometimes, we can benefit from the sort
Sort ();

                                        // finally applying translation
*this  -= center;

//WriteFile ( "E:\\Data\\Surface.spi" );
}


//----------------------------------------------------------------------------
                                        // Keeping the useful points for head modeling, the one that are the most "patatoid-looking", avoiding the eyes, ears, jaws...
void    TPoints::KeepTopHeadPoints (
                                const TVolumeDoc&   mridoc, 
                                TVector3Float       centertranslate,
                                bool                rebalance,
                                const TMatrix44*    mriabstoguillotine
                                )
{
if ( ! mridoc.IsOpen () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use 2 inferior threshold: one for the front face, one for the back
const TBoundingBox<double>* Bound           = mridoc.GetBounding ();

                                        // temporarily shifting to optimally centered space
                            *this          += centertranslate;                          // points shift this way
TPointDouble                center          = mridoc.GetOrigin () - centertranslate;    // but center the other way


TPoints             origsurfp ( *this );
TPointFloat         p;

                                        // origin can be closer to one of the edge, getting a mean distance is safer
double              meanwidth       = Bound->GetXExtent () / 2;
double              meandepth       = Bound->GetYExtent () / 2;
//double            meanheight      = Bound->GetZExtent () / 2;
double              meanheight      = fabs ( Bound->ZMax () - center.Z );   // !more reliable to only use the top part of the head, as bottom part of the head could be cropped anywhere!


double              aboveears       = meanheight *  0.05;   // above ears, with some safety margin - we hate ears
double              frontears       = meandepth  *  0.15;   // front limit to the ears
double              backears        = meandepth  * -0.40;   // back limit to the ears

double              abovenose       = meanheight * -0.50;   // just above nose
double              nosewidth       = meanwidth  *  0.25;   // nose width

double              aboveeyes       = meanheight * -0.20;   // just above eyes (note that it will currently encompass the nose detection)
double              eyeswidth       = meanwidth  *  0.75;   // eyes width

double              bottomface      = meanheight * -0.50;   // bottom of the face
double              bottomskull     = meanheight * -0.50;   // bottom of the back head

double              alwaysabove     = max ( aboveears, abovenose, aboveeyes );


//TSuperGauge         Gauge ( "Filtering Top Points", (int) origsurfp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Reset ();


for ( int pi = 0; pi < (int) origsurfp; pi++ ) {

//    Gauge.Next ();

                                        // skipping points below lower MRI
    if ( mriabstoguillotine ) {
                                        // set point back to absolute space
        p       = origsurfp[ pi ] - centertranslate;

//      mridoc.ToAbs ( p );            // already in abs

        mriabstoguillotine->Apply ( p );

        if ( p.Z < 1 )
            continue;
        } // if mriabstoguillotine


    p       = origsurfp[ pi ];

                                        // whole top head always taken
    if ( p.Z >= alwaysabove ) {
        Add ( p );
        continue;
        }

                                        // skip hole around ears
    if ( p.Y > backears
      && p.Y < frontears )
        continue;

                                        // skip hole around eyes (currently bigger than nose part)
    if ( p.Y > 0
      &&        p.Z   < aboveeyes
      && fabs ( p.X ) < eyeswidth )
        continue;

                                        // skip hole around nose
    if ( p.Y > 0
      &&        p.Z   < abovenose
      && fabs ( p.X ) < nosewidth )
        continue;

                                        // skip low back head
    if ( p.Y <= backears
      && p.Z <  bottomskull )
        continue;

                                        // skip low front head
    if ( p.Y >= frontears
      && p.Z <  bottomface )
        continue;

                                        // here it survived the pruning, insert point!
    Add ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no re-balancing
if ( ! rebalance ) {
                                        // back to original space
    *this  -= centertranslate;
                                        // by safety
    Sort ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Balancing front and back inferior points
TPoints             top;
TPoints             belowfront;
TPoints             belowback;

                                        // scan all points and sort into different buckets
for ( int pi = 0; pi < NumPoints; pi++ )

    if      ( Points[ pi ].Z >= 0 )     top.       Add ( Points[ pi ] );   // comment this to apply a global balance instead of only on inferior points
    else if ( Points[ pi ].Y <  0 )     belowback. Add ( Points[ pi ] );
    else                                belowfront.Add ( Points[ pi ] );


int                 numbelowfront   = (int) belowfront;
int                 numbelowback    = (int) belowback;

//DBGV5 ( NumPoints, (int) top, numbelowfront, numbelowback, RelativeDifference ( numbelowfront, numbelowback ) * 100, "#points top front back %diff" );

                                        // does it need any balancing?
if (                  abs ( numbelowfront - numbelowback ) < 50 
    || RelativeDifference ( numbelowfront,  numbelowback ) < 0.01 ) {
                                        // back to original space
    *this  -= centertranslate;
                                        // by safety
    Sort ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Re-balance bottom part

                                        // reset and transfer top part, always
*this   = top;


TVector<int>        randindex;
TRandUniform        randunif;

if ( numbelowfront > numbelowback ) {
                                        // add below back points
    Add ( belowback );

                                        // pick some front points and insert them    
    randindex.RandomSeries ( numbelowback, numbelowfront, &randunif );

    for ( int i = 0; i < numbelowback; i++ )

        Add ( belowfront[ randindex[ i ] ] );
    }
else {
                                        // add below front points
    Add ( belowfront );

                                        // pick some back points and insert them    
    randindex.RandomSeries ( numbelowfront, numbelowback, &randunif );

    for ( int i = 0; i < numbelowfront; i++ )

        Add ( belowback[ randindex[ i ] ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // back to original space
*this  -= centertranslate;
                                        // return a sorted list
Sort ();
}


//----------------------------------------------------------------------------
void    TPoints::Normalize ()
{
for ( int i = 0; i < NumPoints; i++ )
    Points[ i ].Normalize ();
}


void    TPoints::Invert ()
{
for ( int i = 0; i < NumPoints; i++ )
    Points[ i ].Invert ();
}


//----------------------------------------------------------------------------
void    TPoints::Sort ()
{
OmpCriticalBegin (TPointsSort)

                                        // Results in decreasing order
sort    (   Points.begin(),     Points.end(),
            [] ( const TPointFloat& p1, const TPointFloat& p2 ) { return p1 > p2; } // '>' exists in TPointT
        );
                                        // Results in increasing order
//sort    (   Points.begin(),     Points.end(),
//            [] ( const TPointFloat& p1, const TPointFloat& p2 ) { return p1 < p2; } // '<' exists in TPointT
//        );

OmpCriticalEnd
}


//----------------------------------------------------------------------------
TPointFloat TPoints::GetCenter ()  const
{
TPointFloat         center;

if ( IsEmpty () )
    return  center;


for ( int i = 0; i < NumPoints; i++ )

    center     += Points[ i ];

                                        // then average
center     /= NonNull ( NumPoints );


return  center;

}


double      TPoints::GetRadius ()  const
{
if ( IsEmpty () )
    return  0;


TPointFloat         center          = GetCenter ();
double              r               = 0;


for ( int i = 0; i < NumPoints; i++ )

    r      += ( Points[ i ] - center ).Norm ();


r          /= NonNull ( NumPoints );


return  r;
}


//----------------------------------------------------------------------------
TPointFloat TPoints::GetMeanPoint ()   const
{
TPointFloat         mean;

if ( IsEmpty () )
    return  mean;


for ( int i = 0; i < NumPoints; i++ )
    
    mean   += Points[ i ];

mean   /= NumPoints;


return  mean;
}


TPointFloat TPoints::GetMeanPoint ( const char* selpoints, const TStrings&    pointnames )  const
{
                                        // create a selected set of electrodes
TSelection          elsel ( NumPoints, OrderSorted );

elsel.Reset ();
                                        // set specified electrodes
elsel.Set ( selpoints, &pointnames );



TPointFloat         meanpos;
                                        // sum all electrodes
for ( TIteratorSelectedForward seli ( elsel ); (bool) seli; ++seli )

    meanpos    += Points[ seli() ];

                                        // then average
meanpos    /= NonNull ( (int) elsel );


return  meanpos;
}


//----------------------------------------------------------------------------
                                        // Median of each coordinate separately
TPointFloat TPoints::GetMedianPoint ( bool strictvalue )  const
{
TPointFloat         median;

if      ( IsEmpty () )

    return  median;

else if ( NumPoints == 1 )

    return  Points[ 0 ];

else if ( NumPoints == 2 )
                                        // equivalent to mean for 2 points anyway
    return  GetMeanPoint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoEasyStats        stats ( 3, NumPoints );

for ( int i = 0; i < NumPoints; i++ ) {

    stats[ 0 ].Add ( Points[ i ].X );
    stats[ 1 ].Add ( Points[ i ].Y );
    stats[ 2 ].Add ( Points[ i ].Z );
    }


median.X    = stats[ 0 ].Median ( strictvalue );
median.Y    = stats[ 1 ].Median ( strictvalue );
median.Z    = stats[ 2 ].Median ( strictvalue );


return  median;
}


//----------------------------------------------------------------------------
                                        // This is NOT the median of x, y, z, but the point with minimal distance to all the others
TPointFloat TPoints::GetMedoidPoint ()   const
{
if      ( IsEmpty () ) {

    TPointFloat         empty;
    return  empty;
    }
else if ( NumPoints == 1 )

    return  Points[ 0 ];


double              mind            = DBL_MAX;
int                 mindi           = -1;
double              sumd;

                                        // hopefully there are not too much points in there, as we have a quadratic search... - otherwise use some subsampling(?)
for ( int i = 0; i < NumPoints; i++ ) {

    sumd    = 0;

    for ( int j = 0; j < NumPoints; j++ )
        
        if ( j != i )
                                        // sum of Euclidean distance
//          sumd   += ( Points[ j ] - Points[ i ] ).Norm ();
                                        // sum of squared Euclidean distance i.e. min variance
            sumd   += ( Points[ j ] - Points[ i ] ).Norm2 ();


    if ( sumd < mind ) {
        mind    = sumd;
        mindi   = i;
        }
    }


return  Points[ mindi ];
}


//----------------------------------------------------------------------------
                                        // The point with highest probability
                                        // Approximate version, done for each dimension independently
                                        // Exact solution would need some Mahalanobis / Eigenvectors type of Kernel
TPointFloat TPoints::GetModalPoint ()   const
{
TPointFloat         modal;

if      ( IsEmpty () )

    return  modal;

else if ( NumPoints == 1 )

    return  *Points[ 0 ];

else if ( NumPoints == 2 )
                                        // !2 points still not enough!
    return  GetMeanPoint ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need the SD for each dimension
TGoEasyStats        stats ( 3 );

for ( int i = 0; i < NumPoints; i++ ) {

    stats[ 0 ].Add ( Points[ i ].X );
    stats[ 1 ].Add ( Points[ i ].Y );
    stats[ 2 ].Add ( Points[ i ].Z );
    }

                                        // Estimate Gaussian Kernel Density from SD + a some extra smoothing
                                        // SD could also be approximated as Range / 4 (or other constant which can be estimated from simulation data)
double              sdx             = NonNull ( GaussianWidthToSigma ( 1.06 * stats[ 0 ].SD () / Power ( NumPoints, 1 / 5.0 ) ) * 4 );
double              sdy             = NonNull ( GaussianWidthToSigma ( 1.06 * stats[ 1 ].SD () / Power ( NumPoints, 1 / 5.0 ) ) * 4 );
double              sdz             = NonNull ( GaussianWidthToSigma ( 1.06 * stats[ 2 ].SD () / Power ( NumPoints, 1 / 5.0 ) ) * 4 );

                                        // Scanning each dimension for the point with highest probability (density)
                                        // We save computing the whole histogram if we restrict ourselves to an existing point
double              maxdx           = 0;
double              maxdy           = 0;
double              maxdz           = 0;
int                 maxdxi          = -1;
int                 maxdyi          = -1;
int                 maxdzi          = -1;
double              sumdx;
double              sumdy;
double              sumdz;


for ( int i = 0; i < NumPoints; i++ ) {

    sumdx   = 0;
    sumdy   = 0;
    sumdz   = 0;

    for ( int j = 0; j < NumPoints; j++ ) {
                                        // All Gaussians have the same height, as they all have the same sigma        
        sumdx  += Normal ( Points[ i ].X, Points[ j ].X, sdx );
        sumdy  += Normal ( Points[ i ].Y, Points[ j ].Y, sdy );
        sumdz  += Normal ( Points[ i ].Z, Points[ j ].Z, sdz );
        }

    if ( sumdx > maxdx ) {
        maxdx   = sumdx;
        maxdxi  = i;
        }
    if ( sumdy > maxdy ) {
        maxdy   = sumdy;
        maxdyi  = i;
        }
    if ( sumdz > maxdz ) {
        maxdz   = sumdz;
        maxdzi  = i;
        }
    }

                                        // we agregate different points together!
modal.X     = Points[ maxdxi ].X;
modal.Y     = Points[ maxdyi ].Y;
modal.Z     = Points[ maxdzi ].Z;


return  modal;
}


//----------------------------------------------------------------------------
TPointFloat TPoints::GetLimitMin () const
{
TPointFloat         limitmin ( Highest<float> () );


for ( int i = 0; i < NumPoints; i++ ) {

    Mined ( limitmin.X, Points[ i ].X );
    Mined ( limitmin.Y, Points[ i ].Y );
    Mined ( limitmin.Z, Points[ i ].Z );
    }


return  limitmin;
}


TPointFloat TPoints::GetLimitMax () const
{
TPointFloat         limitmax ( Lowest<float> () );


for ( int i = 0; i < NumPoints; i++ ) {

    Maxed ( limitmax.X, Points[ i ].X );
    Maxed ( limitmax.Y, Points[ i ].Y );
    Maxed ( limitmax.Z, Points[ i ].Z );
    }


return  limitmax;
}


//----------------------------------------------------------------------------
void    TPoints::WriteFile ( const char* file, const TStrings*    names, TSelection* exclsel, int numpoints, ClusterType type )   const
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                xyzformat       = IsExtension ( file, FILEEXT_XYZ   );
bool                elsformat       = IsExtension ( file, FILEEXT_ELS   );
bool                sxyzformat      = IsExtension ( file, FILEEXT_SXYZ  );
bool                spiformat       = IsExtension ( file, FILEEXT_SPIRR );

bool                outputnames     = names != 0 && ( xyzformat || elsformat || sxyzformat || spiformat );
char                name[ 256 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Retrieve the actual number of points to write

                                        // default set to the number of points, if no override
if ( ! IsInsideLimits ( numpoints, 0, NumPoints ) )
    numpoints   = NumPoints;

                                        // caller is responsible for exclsel being of the right size...
if ( exclsel )
    numpoints  -= exclsel->NumSet ();

                                        // hu?
if ( numpoints <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Write header

ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ) );


if      ( xyzformat ) {
    ofs << StreamFormatGeneral;
    ofs << StreamFormatInt32   << numpoints << Tab;
    ofs << StreamFormatFloat32 << GetRadius ();
    ofs << NewLine;
    }

else if ( elsformat ) {
                                        // converting a list of points to els: just do a general purpose set of points
    int                 numclusters     = 1;

    ofs << StreamFormatGeneral;
    ofs << StreamFormatText    << ELSTXT_MAGICNUMBER1 << NewLine;
    ofs << StreamFormatInt32   << numpoints           << NewLine;
    ofs << StreamFormatInt32   << numclusters         << NewLine;

    ofs << StreamFormatGeneral;
    ofs << StreamFormatText    << (   type == ClusterPoint  ? "Single Points"
                                    : type == ClusterLine   ? "Lines"
                                    : type == ClusterGrid   ? "Grid"
                                    : type == Cluster3D     ? "3D Points"
                                    :                         "Points"      )       << NewLine;
    ofs << StreamFormatInt32   << numpoints                                         << NewLine;
    ofs << StreamFormatInt32   << ( type == ClusterTypeUnknown ? Cluster3D : type ) << NewLine;
    }

else if ( sxyzformat ) {
    ofs << StreamFormatGeneral;
    ofs << StreamFormatInt32   << numpoints;
    ofs << NewLine;
    }

else if ( spiformat ) {
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Write points
ofs << StreamFormatFixed;

                                        // in case of exclusion, we need to loop through all electrodes
if ( exclsel )
    numpoints   = NumPoints;

                                        // Loop should handle both the TSelection and regular index cases
for ( int i = 0; i < numpoints; i++ ) {

    if ( exclsel && exclsel->IsSelected ( i ) )
        continue;


    ofs << StreamFormatFloat32 << Points[ i ].X << Tab;
    ofs << StreamFormatFloat32 << Points[ i ].Y << Tab;
    ofs << StreamFormatFloat32 << Points[ i ].Z;


    if ( outputnames ) {
                                        // current name exists?
        if ( i < names->NumStrings () )

            StringCopy ( name, (*names)[ i ] );

        else {
            if      ( xyzformat
                   || elsformat  )  sprintf ( name, "%0d",   i + 1 );
            else if ( sxyzformat )  sprintf ( name, "e%0d",  i + 1 );
            else if ( spiformat  )  sprintf ( name, "sp%0d", i + 1 );
            else                    sprintf ( name, "%0d",   i + 1 );
            }

        ofs << Tab << StreamFormatText << name;
        }


    ofs << NewLine;
    }
}


//----------------------------------------------------------------------------
void    TPoints::ExtractToFile ( const char* file, const char* exclpoints, TStrings*    names /*, bool removeselection*/ )  const
{
TSelection      selection ( NumPoints, OrderArbitrary );

selection.Reset ();

selection.Set ( exclpoints, names );

WriteFile ( file, names, &selection );
}


//----------------------------------------------------------------------------
bool    TPoints::ReadFile ( const char* file, TStrings*    names )
{
Reset ();

if ( names )
    names->Reset ();


if ( StringIsEmpty ( file ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get file ready
bool                xyzformat       = IsExtensionAmong ( file, FILEEXT_XYZ " " FILEEXT_SXYZ );
bool                elsformat       = IsExtensionAmong ( file, FILEEXT_ELS );
bool                spiformat       = IsExtensionAmong ( file, FILEEXT_SPIRR );
bool                locformat       = IsExtensionAmong ( file, FILEEXT_LOC );
bool                FileBin         = locformat;


if ( ! CanOpenFile ( file, CanOpenFileRead) )
    return  false;

ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ), FileBin ? ios::binary | ios::in : ios::in );

if ( ifs.fail () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get / count the number of points
char                buff[ 256 ];
int                 numpoints;
int                 numclusters     = 1;


if      ( xyzformat ) {
                                        // get first number of first line
    ifs.getline ( buff, 256 );
    sscanf ( buff, "%d", &numpoints );
    if ( numpoints <= 0 )
        return  false;
    }

else if ( elsformat ) {

    ifs.getline ( buff, 256 );
    if ( ! StringStartsWith ( buff, ELSTXT_MAGICNUMBER1 ) )
        return  false;


    ifs.getline ( buff, 256 );
    numpoints   = StringToInteger ( buff );
    if ( numpoints <= 0 )
        return  false;

                                        // get first number of first line
    ifs.getline ( buff, 256 );
    numclusters = StringToInteger ( buff );
    if ( numclusters <= 0 )
        return  false;
    }

else if ( spiformat ) {
                                        // max # of points: just scan all lines
    numpoints       = INT_MAX;
/*                                      // we don't need the # of points at that level, the line-by-line scan will do the job
    numpoints       = CountLines ( file );

    if ( numpoints <= 0 )
        return  false;
*/
    }

else if ( locformat ) {

    int                 magicnumber;

    ifs.read ( (char *) &magicnumber, sizeof ( magicnumber ) );

    if ( magicnumber != 1 )
        return  false;

    ifs.read ( (char *) &numpoints, sizeof ( numpoints ) );

    if ( numpoints <= 0 )
        return  false;
    }
else                                    // max # of points: just scan all lines
    numpoints       = INT_MAX;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Read the points
char                name[ 256 ];
TPointFloat         pf;
TPointDouble        pd;

                                        // clusters currently used only by els format
for ( int ci = 0; ci < numclusters; ci++ ) {

                                        // currently, all points are put read, whatever the cluster and the cluster type, and put into the same single structure
    if ( elsformat ) {
                                        // cluster name
        ifs.getline ( buff, 256 );

                                        // number of points within current cluster
        ifs.getline ( buff, 256 );
        numpoints   = StringToInteger ( buff );

                                        // cluster type - not used for the moment
        ifs.getline ( buff, 256 );
        } // if elsformat


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int i = 0, j = 1; i < numpoints; i++ ) {

                                            // EOF or some kind of bad surprise?
        if ( ifs.eof () || ! ifs.good () )
            break;


        if ( FileBin ) {

            ifs.read ( (char *) &pd, pd.MemorySize () );

            if ( names ) {
                StringCopy  ( name, "sp", IntegerToString ( buff, j , 0 ) );
                names->Add  ( name );
                }

                                            // convert points from [m] to [mm]
            if ( locformat )
                pd     *= 1000;


            Add ( pd );
            } // FileBin

        else { // text file
                                            // get the whole line
            ifs.getline ( buff, 256 );      // for LF only use '\r' separators
                                            // skip empty lines
            if ( StringIsEmpty ( buff ) )
                continue;

                                                        // els format has one more opetional field to qualify each electrode (like BAD)
            if ( names ) {

                if ( sscanf ( buff, "%f %f %f %s", &pf.X, &pf.Y, &pf.Z, name ) < 4 ) {
                                            // names are required -> provide a default name if it is missing
                    if      ( xyzformat
                           || elsformat )   StringCopy  ( name, /*"e",*/    IntegerToString ( buff, j , 0 ) );  // !removing the 'e' for Tracks Interpolation!
                    else if ( spiformat )   StringCopy  ( name, "sp",       IntegerToString ( buff, j , 0 ) );
                    }

                names->Add ( name );
                }
            else
                sscanf ( buff, "%f %f %f", &pf.X, &pf.Y, &pf.Z );


            Add ( pf );
            } // ! FileBin

        j++;
        } // for numpoints

    } // for clusters


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( locformat ) {
                                        // Besa files will be internally reoriented & recentered to be consistent with the internal behavior of this program
    bool                ForceBesa;

//  static TStringGrep  grepbesa ( "(?i)(_ACPC|_TAL|_TAL_MASKED)\\." );
//  ForceBesa       = grepbesa.Matched ( GetTitle () );
    ForceBesa       = true;


    if ( ForceBesa ) {
                                        // we don't know the global dimensions, but we can guess them
        TPointFloat                 center;
        TListIterator<TPointFloat>  iterator;

                                        // get max position
        for ( int i = 0; i < NumPoints; i++ ) {
            Maxed ( center.X, Points[ i ].X );
            Maxed ( center.Y, Points[ i ].Y );
            Maxed ( center.Z, Points[ i ].Z );
            }
                                        // take mid-position of next power of 2 above
        center.X    = Power2Above ( center.X ) / 2;
        center.Y    = Power2Above ( center.Y ) / 2;
        center.Z    = Power2Above ( center.Z ) / 2;

//        center.Show ( "LOC center:" );

        (*this)    -= center;
        }
    } // loc file


return  true;
}


//----------------------------------------------------------------------------
double  TPoints::GetMinimumDistance ( const TVector3Float& p )     const
{
double              mind            = DBL_MAX;


for ( int i = 0; i < NumPoints; i++ )
                                      // norm of the vectorial difference
    Mined ( mind, (double) ( Points[ i ] - p ).Norm2 () );


return  sqrt ( mind );
}


TVector3Float TPoints::GetClosestPoint ( const TVector3Float& p )   const
{
double              d;
double              mind            = DBL_MAX;
TVector3Float       minp;


for ( int i = 0; i < NumPoints; i++ ) {
                                        // norm of the vectorial difference
    d   = ( Points[ i ] - p ).Norm2 ();

    if ( d < mind ) {
        mind    = d;
        minp    = Points[ i ];
        }
    }


return  minp;
}


//----------------------------------------------------------------------------
                                        // find the typical (median here) distance between points
double  TPoints::GetMedianDistance ()  const
{
int                 maxscan         = min ( NumPoints - 1, 250 );
int                 step            = NumPoints < 250 ? 1 : 3;
double              minnorm2;
TEasyStats          statd ( maxscan );
double              mindistance;


                                        // first loop is a subsample of all points
for ( int s = 0, i = 0; s < maxscan; s++, i = ( s / (double) ( maxscan - 1 ) ) * ( NumPoints - 1 ) ) {

    minnorm2    = DBL_MAX;

                                        // second loop does not need to see all points either, but going until the end is the point
    for ( int j = i + 1; j < NumPoints; j += step )

        Mined ( minnorm2, (double) ( Points[ i ] - Points[ j ] ).Norm2 () );


    statd.Add ( sqrt ( minnorm2 ) );
    }

statd.Sort ( true );

//statd.Show ( "SP min distances" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // median looks very robust
mindistance     = statd.Median ( false );

                                        // for safety
if ( NumPoints <= 1 || mindistance <= 0 )
    mindistance = 1;


//DBGV4 ( statd.Median (), statd.MaxModeHistogram (), statd.Average (), mindistance, "Median Mode Average -> mindistance" );
//DBGV5 ( ExtraContentType == PointsGeometryGridAligned, mind.X, mind.Y, mind.Z, mindistance, "RegularGrid  mindx mindy mindz -> mindistance" );

return  mindistance;
}


//----------------------------------------------------------------------------
                                        // Returns an array containing the indexes of all neighbors
                                        // Caller has to specify the type of neighborhood
void    TPoints::GetNeighborsIndexes    (   TArray2<int>&   neighbi,    NeighborhoodType    neightype,     double      mediandist   )  const
{
int                 numsneigh       = Neighborhood[ neightype ].NumNeighbors;
    
                                        // for each solution point, allocate the max number of indexes, plus 1 to hold the count of neighbors
neighbi.Resize ( NumPoints, numsneigh + 1 );

if ( NumPoints == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              neighdist       = Square ( Neighborhood[ neightype ].MidDistanceCut * ( mediandist <= 0 ? GetMedianDistance () : mediandist ) );

                                        // are these 2 loops small enough? otherwise, creating a volume and scanning it
for ( int i = 0;     i < NumPoints; i++ )
for ( int j = i + 1; j < NumPoints; j++ ) {

    if ( ( Points[ j ] - Points[ i ] ).Norm2 () < neighdist ) {
                                // safety check                               // new neighbor
        if ( neighbi ( i, 0 ) < numsneigh )  neighbi ( i, ++neighbi ( i, 0 ) ) = j;
                                        // symmetry saves time
        if ( neighbi ( j, 0 ) < numsneigh )  neighbi ( j, ++neighbi ( j, 0 ) ) = i;
        }

    } // for i, j

}


//----------------------------------------------------------------------------
int     TPoints::GetGeometryType ()    const
{
TPointFloat         mind;
int                 maxscan         = min ( NumPoints - 1, 250 );
int                 step            = NumPoints < 250 ? 1 : 3;
double              norm2;
double              minnorm2;
TEasyStats          statd ( 3 * maxscan );


                                        // first loop is a subsample of all sp's
for ( int spi = 0, sp1 = 0; spi < maxscan; spi++, sp1 = (double) spi / ( maxscan - 1 ) * ( NumPoints - 2 ) ) {

    minnorm2    = DBL_MAX;

                                        // second loop does not need to see all points either, but going until the end is the point
    for ( int sp2 = sp1 + 1; sp2 < NumPoints; sp2 += step ) {

        norm2       = ( Points[ sp1 ] - Points[ sp2 ] ).Norm2 ();

        minnorm2    = min ( norm2, minnorm2 );
        }

    statd.Add ( sqrt ( minnorm2 ) );

    }

statd.Sort ( true );

//statd.Show ( "SP min distances" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute MedDistance constant, used as a scale for all sorts of display things
                                        // median looks very robust
double              MedDistance     = statd.Median ( false );

                                        // for safety
if ( NumPoints <= 1 || MedDistance <= 0 )
    MedDistance     = 1;

//DBGV ( MedDistance, "MedianDistance" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Guess content type - regular grid, aligned or tilted
int             ExtraContentType;

                                        // also: MCoV is 0 for regular & aligned, 1e-6 for regular & tilted, > 0.07 for irregular
ExtraContentType    = statd.RobustCoV () < 1e-4 ? PointsGeometryGridAligned : PointsGeometryIrregular;

//DBGV2 ( statd.RobustCoV (), ::IsGeometryGrid ( ExtraContentType ), "RobustCoV -> IsGrid" );
//DBGV ( statd.RobustCoV (), PointsGeometryNames[ ExtraContentType ] );

/*
statd.Show ( "SP min distances" );

TVector<double>          curve;
statd.ComputeHistogram ( 0, 0, 0, 3, 3, true, curve );

TFileName           _file;
StringCopy ( _file, "E:\\Data\\Points Distance Histo.ep" );
curve.WriteFile ( _file );
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Guess content type - regular grid, aligned only
if ( IsGeometryGrid ( ExtraContentType ) ) {
                                        // we restrict ourselves to strictly aligned grids for the moment
    statd.Reset  ();

                                        // first loop is a subsample of all sp's
    for ( int spi = 0, sp1 = 0; spi < maxscan; spi++, sp1 = (double) spi / ( maxscan - 1 ) * ( NumPoints - 2 ) ) {
                                        // compute vector between current point and the next one
        mind        = Points[ sp1 ] - Points[ sp1 + 1 ];
                                        // snap it to median distance
        mind       /= NonNull ( MedDistance );
                                        // then study how it fractions: regular should give a lot of 0's, irregular few
        statd.Add ( fabs ( Fraction ( mind.X ) ) );
        statd.Add ( fabs ( Fraction ( mind.Y ) ) );
        statd.Add ( fabs ( Fraction ( mind.Z ) ) );
        }

    statd.Sort ( true );

//  statd.Show ( "Fractions" );
    /*
    TVector<double>          curve;
    statd.ComputeHistogram ( 0, 0, 0, 3, 3, true, curve );

    TFileName           _file;
    StringCopy ( _file, "E:\\Data\\Points Fractions Histo.ep" );
    curve.WriteFile ( _file );
    */

                                        // also: Median, MCoV
    ExtraContentType    = statd.InterQuartileRange () < 1e-4 ? PointsGeometryGridAligned : PointsGeometryGridNotAligned;

//    DBGV3 ( statd.Median (), statd.InterQuartileRange (), ::IsGeometryGridAligned ( ExtraContentType ), "Median InterQuartileRange -> IsGridAligned" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//DBGV ( ExtraContentType, PointsGeometryNames[ ExtraContentType ] );

return  ExtraContentType;
}


//----------------------------------------------------------------------------
                                        // Resurface points, starting radially from given center, then optionally using gradient to the surface for better outcomes
                                        // It can also optionally skip points which appear to be to close to the neck
void    TPoints::ResurfacePoints        (
                                        const Volume&       surface,
                                        const Volume&       gradient,
                                        TPointFloat         center,
                                        const TMatrix44*    mriabstoguillotine,
                                        double              inflating
                                        )
{
if ( IsEmpty () || surface.IsNotAllocated () || center.IsNull () )
    return;


bool                gradientdirection   = gradient.IsAllocated ();
double              threshold           = surface.GetBackgroundValue ();


InterpolationType   interpolate     = InterpolateLinear; // InterpolateCubicHermiteSpline;

SetOvershootingOption    ( interpolate, surface.GetArray (), surface.GetLinearDim (), true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Extracting a few variables from the Guillotine plane
double              guillotinemin           = 0;
double              guillotinemax           = 0;
double              guillotinetransition    = 0;
TVector3Float       guillotinedir;
#define             minguillotinethreshold  1.5


if ( mriabstoguillotine ) {
                                    // where is the center projecting on the Guillotine space?
    TPointFloat         centercut ( 0, 0, 0 );

    mriabstoguillotine->Apply ( centercut );

                                    // where is the top projecting on the Guillotine space?
    TPointFloat         topcut ( 0, 0, surface.GetDim3 () - center.Z );

    topcut.ResurfacePoint ( surface, center, threshold );

    mriabstoguillotine->Apply ( topcut );

                                        // center to top of the head's range
    double              deltatopz   = topcut.Z - centercut.Z;
                                        // max limit is the center
    guillotinemax           = centercut.Z;
                                        // min limit is symmetrical to top of head
    guillotinemin           = centercut.Z - deltatopz;
                                        // now we can clip both of them    
    Clipped ( guillotinemin, guillotinemax, minguillotinethreshold, (double) FLT_MAX /*(double) topcut.Z*/ );
                                        // transition region - could be 0 after clipping
    guillotinetransition    = guillotinemax - guillotinemin;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can retrieve the perpendicular direction to Guillotine plane directly (checked):
    guillotinedir.Set ( (*mriabstoguillotine) ( 2, 0 ), (*mriabstoguillotine) ( 2, 1 ), (*mriabstoguillotine) ( 2, 2 ) );
                                        // there is no guarantee this vector is normalized
    guillotinedir.Normalize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpCriticalBegin (TPointsResurface)

OmpParallelFor

for ( int i = 0; i < NumPoints; i++ ) {
                                        // absolute coordinates
    const TPointFloat&  origp           = Points[ i ];

    TPointFloat         p               = origp;
                                        // direction for resurfacing
    TVector3Float       dir;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if guillotine plane is given, get a gradient instead of radius direction
    bool                resurfacepoint;
    double              guillotinez     = 0;

    if ( mriabstoguillotine ) {
                                        // input is absolute point
        TPointFloat         cutit       = origp;

        mriabstoguillotine->Apply ( cutit );
                                        // how far above Guillotine plane are we?
        guillotinez     = cutit.Z;
                                        // never allow to go below the Guillotine plane!
        resurfacepoint  = guillotinez > minguillotinethreshold;
        }
    else // no Guillotine plane provided -> always project

        resurfacepoint  = true;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( resurfacepoint  ) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting the resurfacing direction
        if ( gradientdirection ) {
                                        // we have a "gradient" volume provided - use it
            p      += center;

//          gradient.GetGradient ( p.X, p.Y, p.Z, dir, 3 );
            gradient.GetGradientSmoothed ( p.X, p.Y, p.Z, dir, 30 );    // gives more intuitive / robust results

            p      -= center;
                                        // gradient points inward, we want it to point outward
            dir.Invert    ();
                                        // there we are
            dir.Normalize ();


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Using the guillotine perpendicular axis to go from a SPHERICAL to a CYLINDRICAL projection.
                                        // It fixes issues of points projecting downward the neck, as points got closer to the Guillotine plane.
                                        // A second nice property is that it makes a more realistic projection, as the neck / cheeks are more cylindrical than spherical, as is the top of the head
            if ( mriabstoguillotine && guillotinez < guillotinemax ) {
                                        // 0 from top to center; ramp 0 to 1 between max and min; 1 below min to bottom
                double      w       = guillotinetransition > 0 ? Clip ( ( guillotinemax - guillotinez ) / guillotinetransition, 0.0, 1.0 )
                                                               : guillotinez <= guillotinemax;  // step function otherwise
                                        // linearly orthogonalize direction
                                        // bottom points get fully cylindrical; points higher & closer to the center gets more spherical
                dir     = dir.OrthogonalTo ( guillotinedir ) *       w 
                        + dir                                * ( 1 - w );

                dir.Normalize ();
                }
            }
        else { // ! gradientdirection - simply use direction from center to point
                                        // already in absolute coordinates
            dir     = origp;

            dir.Normalize ();
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // absolute coordinates to absolute coordinates
        p.ResurfacePoint ( surface, center, dir, threshold );

        } // resurfacepoint


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally push a little further from the surface, if requested
    if ( inflating != 0 )

        p  += dir * inflating;

                                        // there it is!
    Points[ i ]     = p;
    } // for surfp

OmpCriticalEnd
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
