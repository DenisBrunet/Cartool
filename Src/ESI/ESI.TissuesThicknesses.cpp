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

#include    "ESI.TissuesThicknesses.h"

#include    "Math.Random.h"
#include    "Math.Resampling.h"
#include    "Math.Histo.h"
#include    "TArray3.h"
#include    "TFilters.h"
#include    "Files.TVerboseFile.h"
#include    "GlobalOptimize.Tracks.h"

#include    "ESI.TissuesConductivities.h"
#include    "TCreateInverseMatricesDialog.h"     // TCreateInverseMatricesStruct

#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

char        TissuesLimitsString[ NumTissuesLimits ][ 8 ] =
            {
            "Inner",
            "Outer",
            "Thick",

            "Inner%",
            "Outer%",
            "Thick%",
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // From Roche, we have 3 interesting mean measures (males+females), which can interpolate to 0 (newborn), and up to 20yo
void        AgeToSkullThicknesses_Roche ( double age, double& vertex, double& lambda, double& nasionbregma )
{
                                        // formula valid from 0 to 20 yo, after that it is considered stable with this study
                                        // it seems we can push it back downto 6 month pre-term, i.e. -0.5 year
Clipped ( age, SkullRocheMinAge, SkullRocheMaxAge );

                                        // Polynomial regression, done in Excel
                                        // max at 5.50
vertex          = 0.000009446242636601540000000000 * powl ( age, 5 ) 
                - 0.000620319286298091000000000000 * powl ( age, 4 ) 
                + 0.015623870257432400000000000000 * powl ( age, 3 ) 
                - 0.193411281780735000000000000000 * powl ( age, 2 ) 
                + 1.304797848964830000000000000000 *        age 
                + 0.801749159837984000000000000000;

                                        // max at 6.4
lambda          =-0.000000874303233450169000000000 * powl ( age, 6 ) 
                + 0.000066806737937810700000000000 * powl ( age, 5 ) 
                - 0.002046648993681260000000000000 * powl ( age, 4 ) 
                + 0.032258829477856900000000000000 * powl ( age, 3 ) 
                - 0.287182885923529000000000000000 * powl ( age, 2 ) 
                + 1.599201227389930000000000000000 *        age 
                + 0.841345460863540000000000000000;

                                        // max at 5.05
nasionbregma    =-0.000001436028494006260000000000 * powl ( age, 6 ) 
                + 0.000102422066281027000000000000 * powl ( age, 5 ) 
                - 0.002885660844764400000000000000 * powl ( age, 4 ) 
                + 0.040738161856106000000000000000 * powl ( age, 3 ) 
                - 0.306182846374295000000000000000 * powl ( age, 2 ) 
                + 1.311049399323510000000000000000 *        age 
                + 1.248428210823240000000000000000;
}


                                        // Returns an estimated mean skull thickness from age (in year)
                                        // "Increase in cranial thickness during growth" - Roche - Human Biology - 1953
double      AgeToSkullThickness_Roche ( double age )
{
                                        // Roche for 0 to 20
double              vertex;
double              lambda;
double              nasionbregma;

                                        // Use the Roche estimates
AgeToSkullThicknesses_Roche ( age, vertex, lambda, nasionbregma );

                                        // do an average of all 3 estimates
double              thickness       = ( vertex + lambda + nasionbregma ) / 3;

                                        // or at least an epsilon?
return  AtLeast ( 0.0, thickness );
}

                                        // Global scaling from 20 to 100 years
                                        // "Evaluation of Skull Cortical Thickness Changes With Age and Sex From Computed Tomography Scans" Lillie, Stitzel, JBMR 2015

double      AgeToSkullThickness_Lillie ( double age )
{
Clipped ( age, SkullLillieMinAge, SkullLillieMaxAge );

                                        // it globally increases by 10% over the 80 years range
double              thickness       = 5.00 + ( age - SkullLillieMinAge ) / ( SkullLillieMaxAge - SkullLillieMinAge ) * ( 5.50 - 5.00 );


return  thickness;
}

                                        // returns a global scaling in the range (0..1], from the input age
                                        // this can be used to scale a whole skull
double      AgeToSkullThickness ( double age )
{
if ( age <= SkullRocheMaxAge )

    return  AgeToSkullThickness_Roche ( age );

else {
                                        // Lillie after 20 - this is fortunate the joint is at 20 with Roche data...
                                        // however the 2 curves do not connect, we have to bridge the gap.
                                        // More or less, Roche ends up at 5.6, while Lillie begins at 5.00
                                        // the gap is filled with a global scaling
    double              toroche         = AgeToSkullThickness_Roche  ( SkullRocheMaxAge );
    double              fromlillie      = AgeToSkullThickness_Lillie ( SkullRocheMaxAge );
    double              ratio           = toroche / fromlillie;

    double              thickness       = AgeToSkullThickness_Lillie ( age ) * ratio;

    return  thickness;
    }
}


//{                                       // output age to skull thickness
//TVector<double>         skullthickness ( 10 * ( 100 + 1 ) );
//
//skullthickness.Index1.IndexRatio    = 10;
//skullthickness.Index1.IndexMin      = -1;
//
//for ( int i = 0; i < skullthickness.GetDim (); i++ )
//    skullthickness[ i ]    = AgeToSkullThickness ( skullthickness.ToReal ( i ) );
//
//skullthickness.WriteFile ( "E:\\Data\\AgeToSkullThickness.sef", "Thickness" );
//}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define             DebugRadius

#if defined (DebugRadius)
#include    "TCreateInverseMatricesDialog.h"
extern  TCreateInverseMatricesStruct    CrisTransfer;
bool                outputradius     = false;
TTracks<double>     outputradsavg;
TVector<int>        outputcat;
int                 currel;
#endif


bool    EstimateSkullRadii  (   
                        const TPointFloat&      p,
                        const Volume&           fullvolume,         const Volume&   skulllimit,         const Volume&     brainlimit, 
                        double                  fullbackground,     double          brainbackground,
                        const TPointFloat&      center,
                        float&                  innercsf,
                        float&                  innerskull,
                        float&                  outerskull
                        )
{
                                        // Using some uniform radius value, so we get rid of the actual MRI size
#define             NormalizedRadius        100.0

TPointFloat         psurf;
TVector3Float       dir;
double              radiusmax;
int                 maxnormrad      = NormalizedRadius + 1;
//TVector<double>   radavg ( maxnormrad );
double              radavg[ (int) NormalizedRadius + 1 ];   // should be a bit faster?


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init surface point
psurf       = p;

                                        // re-project point to head surface, to have a clean start, as original point could be indeed far from the surface
                                        // now done before calling
//psurf.ResurfacePoint ( fullvolume, center, fullbackground );

                                        // normalize clique
radiusmax   = psurf.Norm ();

                                        // scan direction, going inward
dir         = psurf / -NormalizedRadius;

                                        // finally, set relative coordinates
psurf      += center;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Converge point toward center, sample volume and average radius line
InterpolationType   interpolate     = InterpolateCubicHermiteSpline;

SetOvershootingOption    ( interpolate, fullvolume.GetArray (), fullvolume.GetLinearDim (), true );


for ( int r = 0; r < maxnormrad; r++ ) {

    radavg[ r ]     = fullvolume.GetValueChecked ( psurf.X, psurf.Y, psurf.Z, interpolate );
                                        // converge to center
    psurf          += dir;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // smooth it out
//radavg.Filter ( FilterTypeGaussian, 4 );
//radavg.Filter ( FilterTypeGaussian, 8 );
//radavg.Filter ( FilterTypeGaussian, 10 );

//FctParams           params;
//params ( FilterParamDiameter )     = 5;
//radavg.Filter ( FilterTypeMedian,       params );
//radavg.Filter ( FilterTypeFastGaussian, params );

                                        // bandpass the curves: getting rid of bias field in the full head, and enhancing the first bright-dark-grey transitions
TVector<double>          radavgf ( 3 * maxnormrad );

                                        // copy with mirrored margins
for ( int i = 0; i < maxnormrad; i++ )

    radavgf[ maxnormrad         + i ]   =
    radavgf[ maxnormrad     - 1 - i ]   =
    radavgf[ 3 * maxnormrad - 1 - i ]   = radavg[ i ];


FctParams           params;

//params ( FilterParamDiameter )     = 3;
//radavgf.Filter ( FilterTypeMedian, params );

params ( FilterParamDiameter )     = 5;
radavgf.Filter ( FilterTypeFastGaussian, params );

                                        // copy back
for ( int i = 0; i < maxnormrad; i++ )
    radavg[ i ]     = radavgf[ maxnormrad + i ];


#if defined (DebugRadius)
if ( (bool) outputradsavg )
    for ( int r = 0; r < maxnormrad; r++ )
        outputradsavg ( currel, r )   += radavg[ r ];
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convert sampled line into a list of extrema (peaks & valleys)
//TArray1<int>      extrema ( maxnormrad + 1 );             // contains the normalized positions, positive for peak, negative for valleys
int                 extrema[ (int) NormalizedRadius + 1 ];  // should be a bit faster?
int                 currpos         = 0;
int                 numextrema      = 0;
#define             extremavalue(I)     radavg[ abs ( extrema[ I ] ) ]


do {
                                        // next summit (first maximum doesn't count)
    for ( currpos++; currpos < maxnormrad - 1; currpos++ )

        if ( radavg[ currpos ] > radavg[ currpos + 1 ]
          && radavg[ currpos ] > radavg[ currpos - 1 ] ) {

            extrema[ numextrema++ ] = currpos;
            break;
            }
                                        // break if center has been reached
    if ( currpos >= maxnormrad - 1 )
        break;

                                        // next valley (last minimum doesn't count)
    for ( currpos++; currpos < maxnormrad - 1; currpos++ )

        if ( radavg[ currpos ] < radavg[ currpos + 1 ]
          && radavg[ currpos ] < radavg[ currpos - 1 ] ) {

            extrema[ numextrema++ ] = -currpos; // !code a valley with a negative value!
            break;
            }
                                        // break if center has been reached
    if ( currpos >= maxnormrad - 1 )
        break;

                                        // no need to scan more than 7 peaks/valleys
    } while ( numextrema < 7 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get deepest limit for the skull
psurf           = p;

psurf.ResurfacePoint ( skulllimit, center, brainbackground );

double              radiusbrain     = psurf.Norm ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert brain radius to normalized coordinates - invert radius, as low index corresponds to outer parts
                                        // force set a limit to the brain expected position, to avoid wrong tests
#define             MinBrainRelativeRadius          0.33

#define             MaxInnerSkullRelativeRadius     0.98

#define             MinDeltaSkullRelativeRadius     0.01
#define             MaxOuterSkullRelativeRadius     ( MaxInnerSkullRelativeRadius + MinDeltaSkullRelativeRadius )

#define             DefaultInnerSkullRelativeRadius 0.866
#define             DefaultOuterSkullRelativeRadius 0.940


//double            brainposn       = NormalizedRadius * ( 1 - Clip ( radiusbrain / radiusmax, MinBrainRelativeRadius, 1.0 ) );
double              brainposn       = NormalizedRadius * ( 1 - NoMore ( 1.0, radiusbrain / radiusmax ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get valley above brain surface
int                 brainvalley     = -1;
int                 valleyi         = -1;
int                 normpos;
double              v;
double              minv            = DBL_MAX;

                                        // search for the last valley before the brain surface
for ( int extri = 0; extri < numextrema; extri++ ) {
                                        // skip peaks (usually the first extrema)
    if ( extrema[ extri ] >= 0 )
        continue;


    normpos = abs ( extrema[ extri ] );
                                        // difference with previous peak
    v       = radavg[ normpos ];

                                        // break if too deep to the brain surface, but continue if no valley has been set yet
    if ( normpos >= brainposn && valleyi != -1 )
        break;


    if ( v < minv ) {
        minv            = v;
        valleyi         = extri;
        brainvalley     = normpos;      // normalized position
        }
    }


#if defined (DebugRadius)
if ( (bool) outputradsavg )
    outputcat[ currel ]  += valleyi;
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // estimate the skull thickness from the valley
//int               outeri;
//int               inneri;
double              outer;
double              inner;
//double            w;

                                        // Lowest valley

                                        // normalize / use theta-phi instead?
bool                topside         = p.Z >= -10;

                                        // get min valley

                                        // simply the absolute min is a very stable guess
//int                 peak            = 0;
//int                 valley;
//int                 minvalley       = 0;
int                 minvalleyvalue  = INT_MAX;
//int                 valleyi         = 0;

                                        // in case detection from brain surface has failed
if ( valleyi == -1 )

    for ( int i = 0; i < numextrema; i++ ) {

        if ( extrema[ i ] >= 0 ) {
                                        // peak
    //      peak    = extrema[ i ];
            } // if peak

        else {
                                        // valley
            v   = abs ( extrema[ i ] );


            if ( v > NormalizedRadius * ( topside ? 0.33 : 0.60 ) ) // depends on which side?
                break;

                                        // get minimum valley
            if ( radavg[ (int) v ] < minvalleyvalue ) {
    //          minvalley       = v;
                minvalleyvalue  = radavg[ (int) v ];
                valleyi         = i;
                }
            } // if valley
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // still not defined?
if ( valleyi == -1 ) {
                                        // Settle for some default values and return

                                        // Point closest as possible to the brain
    psurf           = p;
    psurf.ResurfacePoint ( brainlimit, center, brainbackground );
    radiusbrain     = psurf.Norm ();

                                        // set some standard values
    innercsf        = radiusbrain + 0.5;    // give it a little extra space from the brain
    innerskull      = DefaultInnerSkullRelativeRadius * radiusmax;
    outerskull      = DefaultOuterSkullRelativeRadius * radiusmax;

                                            // more checks
    Maxed ( innerskull, (float) ( innercsf   + MinCsfThickness   ) );
    Maxed ( outerskull, (float) ( innerskull + MinSkullThickness ) );


    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inner           = -1;
double              d;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) First, simple valley, nothing close
if ( valleyi == 1 && numextrema >= 2

    && abs ( extrema[ valleyi + 1 ] ) - abs ( extrema[ valleyi ] ) > 7 * ( NormalizedRadius / 100.0 ) // far enough

    && RelativeDifference ( extremavalue ( valleyi + 1 ), extremavalue ( valleyi ) ) > 0.30 // high enough
    ) {

    brainvalley     = abs ( extrema[ valleyi ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Small bump very close
                                        // - on the left
else if ( valleyi > 2

        && abs ( extrema[ valleyi ] ) - abs ( extrema[ valleyi - 2 ] ) < 15 * ( NormalizedRadius / 100.0 ) // close enough

        && fabs ( extremavalue ( valleyi - 2 ) - extremavalue ( valleyi ) ) < 40
        ) {

    brainvalley     = abs ( extrema[ valleyi - 1 ] );

    inner           = abs ( extrema[ valleyi     ] );
//      outer           = abs ( extrema[ valleyi - 2 ] );

                                        // inflates case 2 to be comparable to case 1, but it makes some cases too much thicker - maybe set a limit?
    d               = abs ( extrema[ valleyi - 1 ] ) - abs ( extrema[ valleyi - 2 ] );
    outer           = abs ( extrema[ valleyi - 2 ] ) - d * 0.50;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Small bump very close
                                        // - on the right
else if ( valleyi < numextrema - 2

        && abs ( extrema[ valleyi + 2 ] ) - abs ( extrema[ valleyi ] ) < 15 * ( NormalizedRadius / 100.0 ) // close enough

        && fabs ( extremavalue ( valleyi + 2 ) - extremavalue ( valleyi ) ) < 40
            ) {

    brainvalley     = abs ( extrema[ valleyi + 1 ] );

    inner           = abs ( extrema[ valleyi + 2 ] );

    d               = abs ( extrema[ valleyi + 1 ] ) - abs ( extrema[ valleyi     ] );
    outer           = abs ( extrema[ valleyi     ] ) - d * 0.50;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Big valley further away
else if ( valleyi < numextrema - 1

        && RelativeDifference ( extremavalue ( valleyi - 1 ), extremavalue ( valleyi ) ) > 0.15 // .20 OK
        && RelativeDifference ( extremavalue ( valleyi + 1 ), extremavalue ( valleyi ) ) > 0.15 ) {

    brainvalley     = abs ( extrema[ valleyi ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 9) WTF cases
else {

    if ( numextrema >= 1 )
        brainvalley = abs ( extrema[ valleyi ] );
    else
        brainvalley = NormalizedRadius;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( valleyi - 2 >= 0 && brainvalley > brainposn ) {
                                        // switch to 1 outer valley
    valleyi    -= 2;

    brainvalley = abs ( extrema[ valleyi ] );

    inner       = -1;
    }

                                        // scan on the left and the right independently
                                        // stopping at the inflexion on the curve
if ( inner == -1 ) {
    double              outernext;
    int                 thick;

    thick   = 2;
    outer    = radavg[ brainvalley - thick ] - radavg[ brainvalley - thick + 1 ];

    do {
                                        // check limits
        if ( brainvalley - thick <= 0 )
            break;

                                        // get the next delta
        outernext    = radavg[ brainvalley - thick - 1 ] - radavg[ brainvalley - thick ];

                                        // stop if one side starts decreasing
        if ( outernext <= outer * 1.20 )
            break;

        outer        = outernext;
        thick++;
        } while ( true );


    outer    = brainvalley - thick;


    double              innernext;
    thick   = 2;
    inner   = radavg[ brainvalley + thick ] - radavg[ brainvalley + thick - 1 ];

    do {
                                        // check limits
        if ( brainvalley + thick >= maxnormrad - 1 )
            break;

                                        // get the next delta
        innernext   = radavg[ brainvalley + thick + 1 ] - radavg[ brainvalley + thick ];

                                        // stop if one side starts decreasing
        if ( innernext <= inner * 1.20 )
            break;

        inner       = innernext;
        thick++;
        } while ( true );


    inner   = brainvalley + thick;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Point closest as possible to the brain
psurf           = p;

psurf.ResurfacePoint ( brainlimit, center, brainbackground );

radiusbrain     = psurf.Norm ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // final clipping, with a bit of safety?

                                        // !this will totally fail if cerebellum is missing!
//Clipped ( inner, 0.1, brainposn   );
//Clipped ( outer, 0.0, inner - 0.1 );

                                        // invert & normalize, back to radius
inner           = ( NormalizedRadius - inner ) / NormalizedRadius;
outer           = ( NormalizedRadius - outer ) / NormalizedRadius;

                                        // force clipping to reasonable limits
innercsf        = radiusbrain + 0.5;    // give it a little extra space from the brain
innerskull      = Clip ( inner, MinBrainRelativeRadius,              MaxInnerSkullRelativeRadius ) * radiusmax;
outerskull      = Clip ( outer, inner + MinDeltaSkullRelativeRadius, MaxOuterSkullRelativeRadius ) * radiusmax;

                                        // more checks
Maxed ( innerskull, (float) ( innercsf   + MinCsfThickness   ) );
Maxed ( outerskull, (float) ( innerskull + MinSkullThickness ) );


return  true;
}


//----------------------------------------------------------------------------
void    EstimateSkullRadii  (   
                        const TPoints&          points,
                        const Volume&           fullvolume,         const Volume&       skulllimit,         const Volume&     brainlimit, 
                        double                  fullbackground,     double              brainbackground,
                        const TPointFloat&      center,
                        TMap&                   innercsf,
                        TMap&                   innerskull,
                        TMap&                   outerskull
                        )
{
for ( int ei = 0; ei < (int) points; ei++ ) {

    #if defined (DebugRadius)
    currel  = ei;
    #endif

    EstimateSkullRadii  (   points[ ei ], 
                            fullvolume,         skulllimit,         brainlimit,
                            fullbackground,     brainbackground, 
                            center,
                            innercsf   ( ei ),
                            innerskull ( ei ),
                            outerskull ( ei )
                        );
    }
}


//----------------------------------------------------------------------------
                                        // Filtering radius estimates
                                        // Computing a single rescaling factor
double      FilterEstimates     (   TMaps&              radius,
                                    SpatialFilterType   spatialfilter,      const char*         filexyz
                                )
{
if ( spatialfilter == SpatialFilterNone )
    return  1;

int                 numestimates    = radius.GetNumMaps ();
int                 numel           = radius.GetDimension ();
TMaps               oldradius ( radius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filter radius values directly
radius.FilterSpatial ( spatialfilter, filexyz );

                                            
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute a global scaling old radius / new radius, as filtering has a tendency to move the points inward
TEasyStats          stats ( radius.GetLinearDim () );
double              filterratio;


for ( int ri = 0; ri < numestimates; ri++ )
for ( int ei = 0; ei < numel; ei++ )

    if ( radius ( ri, ei ) )    stats.Add ( oldradius ( ri, ei ) / radius ( ri, ei ) );

filterratio     = stats.Median ( false );

return  filterratio;
}

                                        // Computing 3 rescaling factors, one for each X, Y and Z axis
TPointDouble    FilterEstimates     (   TMaps&              radius,
                                        SpatialFilterType   spatialfilter,      const char*         filexyz,    const TPoints&          sphel
                                    )
{
if ( spatialfilter == SpatialFilterNone )
    return  TPointDouble ( 1 );

int                 numestimates    = radius.GetNumMaps ();
int                 numel           = radius.GetDimension ();
TMaps               oldradius ( radius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Focusing only on the max positions to estimate the best rescaling
TPoints             oldmaxes ( numestimates );

for ( int ri = 0; ri < numestimates; ri++ )
for ( int ei = 0; ei < numel; ei++ ) {

    Maxed ( oldmaxes[ ri ].X, sphel[ ei ].X * radius ( ri, ei ) );
    Maxed ( oldmaxes[ ri ].Y, sphel[ ei ].Y * radius ( ri, ei ) );
    Maxed ( oldmaxes[ ri ].Z, sphel[ ei ].Z * radius ( ri, ei ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filter radius values directly
radius.FilterSpatial ( spatialfilter, filexyz );

                                            
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeat the scan, but the weights have been filtered out
TPoints             newmaxes ( numestimates );

for ( int ri = 0; ri < numestimates; ri++ )
for ( int ei = 0; ei < numel; ei++ ) {

    Maxed ( newmaxes[ ri ].X, sphel[ ei ].X * radius ( ri, ei ) );
    Maxed ( newmaxes[ ri ].Y, sphel[ ei ].Y * radius ( ri, ei ) );
    Maxed ( newmaxes[ ri ].Z, sphel[ ei ].Z * radius ( ri, ei ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute a global scaling old radius / new radius, per X/Y/Z component
TGoEasyStats        stats ( 3, numestimates );

for ( int ri = 0; ri < numestimates; ri++ ) {

    stats[ 0 ].Add ( oldmaxes[ ri ].X / newmaxes[ ri ].X );
    stats[ 1 ].Add ( oldmaxes[ ri ].Y / newmaxes[ ri ].Y );
    stats[ 2 ].Add ( oldmaxes[ ri ].Z / newmaxes[ ri ].Z );
    }


TPointDouble    filterratio;

filterratio.X   = stats[ 0 ].Median ( false );
filterratio.Y   = stats[ 1 ].Median ( false );
filterratio.Z   = stats[ 2 ].Median ( false );


return  filterratio;
}


//----------------------------------------------------------------------------
                                        // Filtering radius estimates
double      FilterEstimates     (   TArray3<float>&     radius,             int                 ti,         int                 li,
                                    SpatialFilterType   spatialfilter,      const char*         filexyz
                                )
{
if ( spatialfilter == SpatialFilterNone )
    return  1;

int                 numel           = radius.GetDim1 ();
TMaps               mradius ( 1, numel );

                                        // transfer to TMaps
for ( int ei = 0; ei < numel; ei++ )
    mradius ( 0, ei )   = radius ( ei, ti, li );

                                        // call the other function
double              filterratio     = FilterEstimates ( mradius, spatialfilter, filexyz );

                                        // transfer back
for ( int ei = 0; ei < numel; ei++ )
    radius ( ei, ti, li )   = mradius ( 0, ei );


return  filterratio;
}


TPointDouble    FilterEstimates     (   TArray3<float>&     radius,             int                 ti,         int                     li,
                                        SpatialFilterType   spatialfilter,      const char*         filexyz,    const TPoints&          sphel
                                    )
{
if ( spatialfilter == SpatialFilterNone )
    return  TPointDouble ( 1 );

int                 numel           = radius.GetDim1 ();
TMaps               mradius ( 1, numel );

                                        // transfer to TMaps
for ( int ei = 0; ei < numel; ei++ )
    mradius ( 0, ei )   = radius ( ei, ti, li );

                                        // call the other function
TPointDouble        filterratio     = FilterEstimates ( mradius, spatialfilter, filexyz, sphel );

                                        // transfer back
for ( int ei = 0; ei < numel; ei++ )
    radius ( ei, ti, li )   = mradius ( 0, ei );


return  filterratio;
}


//----------------------------------------------------------------------------
                                        // Refining the thicknesses with the expected mean thickness
                                        // Tested on Human skull, but with the right targeted thickness, should also work on monkeys f.ex.
                                        // Note that there is a source of approximation here:
                                        // Skull thicknesses from litterature is measured orthogonal to the surface
                                        // while Cartool thicknesses are computed toward the center of skull, hence not exactly
                                        // perpendicular. This has a very limited impact, though, but worth mentionning.

                                        // !Skull, Spongy Skull and CSF thicknesses should all be non-null!
void        AdjustSkullThickness(   const TPoints&              points,
                                    const TPointDouble&         voxelsize,
                                    TArray3<float>&             tissuesradii,   double      targetskullthickness    
                                )
{
if ( targetskullthickness <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = tissuesradii.GetDim1 ();
TEasyStats          stats ( numel );


for ( int ei = 0; ei < numel; ei++ )
                                        // limit the stats to the upper part of the skull:
                                        // this is the part from which the thickness has been evaluated,
                                        // and to have a more consistent measure of scaling for the XYZ (electrodes) and the Skull points (all around)
    if ( points[ ei ].Z > 0
      && tissuesradii ( ei, SkullIndex,ThickAbs ) > 0 )

        stats.Add ( tissuesradii ( ei, SkullIndex,ThickAbs ) );

                                        // targetskullthickness is in [mm], while our data are in voxels, so don't forget to scale accordingly!
double              dataskullthickness      = stats.Median ( false ) * voxelsize.Mean ();   // voxels must be isotropic here, still taking the mean voxel size...
                                       
//double            rescale                 = targetskullthickness / dataskullthickness * ( 1 - 0.035 );    // note: it seems the final stats show an overshoot of about 3.5% of the radius, so we can tune it down here
double              rescale                 = targetskullthickness / dataskullthickness;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              deltaskull;
double              deltaspongy;

for ( int ei = 0; ei < numel; ei++ ) {
                                        // shouldn't happen, but must be avoided anyway
    if ( tissuesradii ( ei, SkullIndex,ThickAbs ) == 0 )
        continue;

                                        // formula works for both shrinking (usual case) and expanding
    deltaskull                                        = tissuesradii ( ei, SkullIndex,      ThickAbs ) * ( 1 - rescale );   // delta positive for shrinkage, negative for expansion
    deltaspongy                                       = tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) * ( 1 - rescale );   // delta positive for shrinkage, negative for expansion


/*                                      // symmetrical updates on each side of the skull - new way
    tissuesradii ( ei, SkullIndex,InnerAbs )         += deltaskull / 2;
    tissuesradii ( ei, SkullIndex,OuterAbs )         -= deltaskull / 2;
    tissuesradii ( ei, SkullIndex,ThickAbs )          = tissuesradii ( ei, SkullIndex,OuterAbs ) - tissuesradii ( ei, SkullIndex,InnerAbs );

                                        // likewise for spongy skull
    tissuesradii ( ei, SkullSpongyIndex,InnerAbs )   += deltaspongy / 2;
    tissuesradii ( ei, SkullSpongyIndex,OuterAbs )   -= deltaspongy / 2;
    tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = tissuesradii ( ei, SkullSpongyIndex,OuterAbs ) - tissuesradii ( ei, SkullSpongyIndex,InnerAbs );

                                        // update outer CSF with inner Skull
    tissuesradii ( ei, CsfIndex,OuterAbs )            = tissuesradii ( ei, SkullIndex,InnerAbs );
    tissuesradii ( ei, CsfIndex,ThickAbs )            = AtLeast ( (float) MinCsfThickness, tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,InnerAbs ) );
    tissuesradii ( ei, CsfIndex,InnerAbs )            = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );
*/

                                        // asymmetrical updates on only one side of the skull - older way
    if ( rescale <= 1 ) {
                                        // shrinking: adjust only the inner skull part, i.e. only pushing outward, as the outer skull
                                        // seems the most reliable, and we prefer to give some room to the brain / spi
        tissuesradii ( ei, SkullIndex,InnerAbs )         += deltaskull;
        tissuesradii ( ei, SkullSpongyIndex,InnerAbs )   += deltaspongy;
                                        // then update CSF
        tissuesradii ( ei, CsfIndex,OuterAbs )            = tissuesradii ( ei, SkullIndex,InnerAbs );
        tissuesradii ( ei, CsfIndex,ThickAbs )            = AtLeast ( (float) MinCsfThickness, tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,InnerAbs ) );
        tissuesradii ( ei, CsfIndex,InnerAbs )            = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );
        }
    else {
                                        // expanding only the outer part - inner part is already quite close to the brain
        tissuesradii ( ei, SkullIndex,OuterAbs )         -= deltaskull;
        tissuesradii ( ei, SkullSpongyIndex,OuterAbs )   -= deltaspongy;
        }

    tissuesradii ( ei, SkullIndex,ThickAbs )          = tissuesradii ( ei, SkullIndex,OuterAbs )       - tissuesradii ( ei, SkullIndex,InnerAbs );
    tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = tissuesradii ( ei, SkullSpongyIndex,OuterAbs ) - tissuesradii ( ei, SkullSpongyIndex,InnerAbs );

    }

}


//----------------------------------------------------------------------------
                                        // Updating the Scalp fields + converting all absolute values to relative (to outer scalp)
bool        RadiusAbsToRadiusRel(   const TPoints&              points,
                                    TArray3<float>&             tissuesradii
                                )
{
int                 numel           = tissuesradii.GetDim1 ();
bool                thicknessok     = true;
double              maxradius;


for ( int ei = 0; ei < numel; ei++ ) {
                                        // update these fields
    tissuesradii ( ei, ScalpIndex,OuterAbs )  = points[ ei ].Norm ();
    tissuesradii ( ei, ScalpIndex,InnerAbs )  = tissuesradii ( ei, SkullIndex,OuterAbs );
    tissuesradii ( ei, ScalpIndex,ThickAbs )  = tissuesradii ( ei, ScalpIndex,OuterAbs ) - tissuesradii ( ei, ScalpIndex,InnerAbs );

                                        // !force min scalp thickness!
    if ( tissuesradii ( ei, ScalpIndex,ThickAbs ) <= MinScalpThickness ) {
        tissuesradii ( ei, ScalpIndex,ThickAbs )  = MinScalpThickness;
        tissuesradii ( ei, ScalpIndex,InnerAbs ) -= tissuesradii ( ei, ScalpIndex,ThickAbs );

        tissuesradii ( ei, SkullIndex,OuterAbs )  = tissuesradii ( ei, ScalpIndex,InnerAbs );
        tissuesradii ( ei, SkullIndex,ThickAbs )  = tissuesradii ( ei, SkullIndex,OuterAbs ) - tissuesradii ( ei, SkullIndex,InnerAbs );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute corresponding relative values
    maxradius   = tissuesradii ( ei, ScalpIndex,OuterAbs );

    for ( int ti = NoTissueIndex + 1; ti < NumTissuesIndex; ti++ ) {
        tissuesradii ( ei, ti, InnerRel )     = NoMore ( 1.0, tissuesradii ( ei, ti, InnerAbs ) / maxradius );
        tissuesradii ( ei, ti, OuterRel )     = NoMore ( 1.0, tissuesradii ( ei, ti, OuterAbs ) / maxradius );
        tissuesradii ( ei, ti, ThickRel )     = NoMore ( 1.0, tissuesradii ( ei, ti, ThickAbs ) / maxradius );
        }

    tissuesradii ( ei, ScalpIndex, OuterRel ) = 1.0;

                                        // fool-proofing the results
    if ( tissuesradii ( ei, CsfIndex,ThickAbs         ) < 0
      || tissuesradii ( ei, SkullIndex,ThickAbs       ) < 0
      || tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) < 0
      || tissuesradii ( ei, ScalpIndex,ThickAbs       ) < 0 )

        thicknessok = false;

    } // for point


return  thicknessok;
}


//----------------------------------------------------------------------------
                                        // If a single estimation is not certain, do some resampling
bool    EstimateTissuesRadii_T1 ( 
                                const TPoints&          points,
                                SpatialFilterType       smoothing,          const TElectrodesDoc*   xyzdoc,
                                const Volume&           fullvolume,         const Volume&           skulllimit,     const Volume&     brainlimit, 
                                const TPointFloat       inversecenter,
                                const TPointDouble&     voxelsize,
                                bool                    adjustradius,       double                  targetskullthickness,
                                TArray3<float>&         tissuesradii
                                )
{
int                 numel           = (int) points;
double              fullbackground  = fullvolume.GetBackgroundValue ();     // call these once only!
double              brainbackground = skulllimit.GetBackgroundValue ();     // both skulllimit and brainlimit should be mask

int                 numcliques      = 101; // 50; // 25;    // more cliques means better precision
double              scale           = fullvolume.MeanSize () / 200;
double              cliqueside      = /*2.5*/ 5 * scale;    // smaller radius means closer to anatomy, but more prone to artifacts and unwanted "resolution"
TPoints             clique;

int                 numestimates    = numcliques;

TMaps               innercsf        ( numestimates, numel );
TMaps               innerskull      ( numestimates, numel );
TMaps               outerskull      ( numestimates, numel );

TRandSpherical      randsph;


                                        // allocate results
tissuesradii.Resize ( numel, NumTissuesIndex, NumTissuesLimits );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined (DebugRadius)
outputradius    = true;

if ( outputradius ) {
    outputradsavg.Resize ( (int) points, NormalizedRadius + 1 );
    outputcat    .Resize ( (int) points );
    }
else {
    outputradsavg.DeallocateMemory ();
    outputcat    .DeallocateMemory ();
    }
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Generate cliques, and loop through all methods
                                        // Cumulate all results into a TMaps
for ( int cliquei = 0, esti = 0; cliquei < numcliques; cliquei++ ) {


    clique.Set ( points );
                                        // first clique is original set of points
                                        // next cliques have a random radius added
                                        // ?To reduce errors, why not rescale each clique' radii by the relative brain ratio clique/real?
    if ( cliquei )

        for ( int ei = 0; ei < numel; ei++ ) {
                                        // although randdir is 3D spherical, it will be projected onto the head surface anyway, giving a distribution on a disc
            clique[ ei ]   += randsph () * cliqueside;
                                        // resurface now
            clique[ ei ].ResurfacePoint ( fullvolume, inversecenter, fullbackground );
            }


//  TFileName   _file;
//  StringCopy  ( _file, "E:\\Data\\Radius.Clique", IntegerToString ( cliquei + 1, 2 ), ".xyz" );
//  CheckNoOverwrite ( _file );
//  clique.WriteFile ( _file );


    EstimateSkullRadii  (   clique, 
                            fullvolume,         skulllimit,         brainlimit, 
                            fullbackground,     brainbackground, 
                            inversecenter, 
                            innercsf[ esti ],   innerskull[ esti ],     outerskull[ esti ] 
                        );

    } // for clique


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional filtering
if ( smoothing && xyzdoc ) {
                                        // Smarter rescaling, one per X, Y and Z component
    TFileName           filepoints ( xyzdoc->GetDocPath () );
    TPoints             sphel ( filepoints );

    sphel.Normalize ();


    TPointDouble        csffilterratio      = FilterEstimates ( innercsf,   smoothing, xyzdoc->GetDocPath (), sphel );
    TPointDouble        innerfilterratio    = FilterEstimates ( innerskull, smoothing, xyzdoc->GetDocPath (), sphel );
    TPointDouble        outerfilterratio    = FilterEstimates ( outerskull, smoothing, xyzdoc->GetDocPath (), sphel );
                                        // applying the same ratio for the 3 layers
    TPointDouble        filterratio         = ( csffilterratio + innerfilterratio + outerfilterratio ) / 3.0;


    for ( int ri = 0; ri < numestimates; ri++ )
    for ( int ei = 0; ei < numel; ei++ ) {
                                        // go into 3D to modulate the value of interest
                                                //  scalar                    !vector!
        innercsf   ( ri, ei )   = ( ( sphel[ ei ] * innercsf   ( ri, ei ) ) * filterratio ).Norm ();
        innerskull ( ri, ei )   = ( ( sphel[ ei ] * innerskull ( ri, ei ) ) * filterratio ).Norm ();
        outerskull ( ri, ei )   = ( ( sphel[ ei ] * outerskull ( ri, ei ) ) * filterratio ).Norm ();
        }
    
//  csffilterratio  .Show ( "csffilterratio"   ); innerfilterratio.Show ( "innerfilterratio" ); outerfilterratio.Show ( "outerfilterratio" );
//  filterratio     .Show ( "filterratio" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merging all estimates together
                                        // Median of all estimates
TMap                statinnercsf    = innercsf  .ComputeCentroid ( MeanCentroid /*MedianCentroid*/, AtomTypePositive, PolarityDirect );
TMap                statinnerskull  = innerskull.ComputeCentroid ( MeanCentroid /*MedianCentroid*/, AtomTypePositive, PolarityDirect );
TMap                statouterskull  = outerskull.ComputeCentroid ( MeanCentroid /*MedianCentroid*/, AtomTypePositive, PolarityDirect );

                                        // Transfer stats
for ( int ei = 0; ei < numel; ei++ ) {
                                        // Setting CSF first
    tissuesradii ( ei, CsfIndex,InnerAbs )    = statinnercsf   ( ei );
    tissuesradii ( ei, CsfIndex,OuterAbs )    = statinnerskull ( ei );
    tissuesradii ( ei, CsfIndex,ThickAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,InnerAbs );

    if ( tissuesradii ( ei, CsfIndex,ThickAbs ) < MinCsfThickness ) {
                                        // provide for default minimal CSF thickness
        tissuesradii ( ei, CsfIndex,ThickAbs )    = MinCsfThickness;
        tissuesradii ( ei, CsfIndex,InnerAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting skull, above the CSF
    tissuesradii ( ei, SkullIndex,InnerAbs )  = tissuesradii ( ei, CsfIndex,InnerAbs ) + tissuesradii ( ei, CsfIndex,ThickAbs );
    tissuesradii ( ei, SkullIndex,OuterAbs )  = statouterskull ( ei );
    tissuesradii ( ei, SkullIndex,ThickAbs )  = tissuesradii ( ei, SkullIndex,OuterAbs ) - tissuesradii ( ei, SkullIndex,InnerAbs );    // not clipping here - check is done later


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting the spongy part
    double          midskull    = ( tissuesradii ( ei, SkullIndex,InnerAbs ) + tissuesradii ( ei, SkullIndex,OuterAbs ) ) / 2;

    tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = AtLeast ( MinSpongySkullThickness, SkullThicknessToSpongy ( tissuesradii ( ei, SkullIndex,ThickAbs ), SkullSpongyPercentage, SkullCompactMinThickness, SkullCompactMaxThickness ) );
    tissuesradii ( ei, SkullSpongyIndex,InnerAbs )    = midskull - tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;
    tissuesradii ( ei, SkullSpongyIndex,OuterAbs )    = midskull + tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;
    }


innercsf  .DeallocateMemory ();
innerskull.DeallocateMemory ();
outerskull.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Refining the thicknesses with the expected mean thickness
if ( adjustradius && targetskullthickness > 0 )

    AdjustSkullThickness    (   points, voxelsize,  tissuesradii, targetskullthickness    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finishing: setting missing values, computing relative ratios and final consistency check
bool                thicknessok     = RadiusAbsToRadiusRel  (   points,     tissuesradii  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined (DebugRadius)
                                        // complimentary output
TExportTracks     exprad;
sprintf ( exprad.Filename, "%s\\More\\Scalp Skull Radii.sef", CrisTransfer.BaseFileName );
exprad.SetAtomType ( AtomTypeScalar );
exprad.NumTracks        = numel;
exprad.NumTime          = NumTissuesIndex;

for ( int t = 0; t < exprad.NumTime; t++ )
    for ( int i = 0; i < exprad.NumTracks; i++ )
        exprad.Write ( tissuesradii ( i, t, ThickAbs ) );


if ( outputradius && (bool) outputcat ) {

    TFileName           _file;

    outputradsavg  /= numestimates;
    outputcat      /= numestimates;     // average of categories

    TExportTracks     exprad;
    sprintf ( exprad.Filename, "%s\\More\\Radii.Avg.sef", CrisTransfer.BaseFileName );
    exprad.SetAtomType ( AtomTypeScalar );
    exprad.NumTracks        = outputradsavg.GetDim1 ();
    exprad.NumTime          = outputradsavg.GetDim2 ();
//    if ( xyzdoc ) {
//        exprad.ElectrodesNames  = xyzdoc->GetElectrodesNames ();
//
//        for ( int i = 0; i < exprad.NumTracks; i++ )
//            sprintf ( StringEnd ( exprad.ElectrodesNames[ i ] ), "-%0d", outputcat[ i ] );
//        }

    for ( int t = 0; t < exprad.NumTime; t++ )
       for ( int i = 0; i < exprad.NumTracks; i++ )
           exprad.Write ( outputradsavg[ i ][ t ] );


/*                                        // split output by category
    TSelection          selcat ( (int) points, OrderSorted );
//    TPoints             surfacec;
//    TPoints             innercsfc;
//    TPoints             innerskullc;
//    TPoints             outerskullc;
    TStrings            names;
//    TExportTracks       exprad;


    for ( int c = 0; c <= 20; c++ ) {

        selcat      .Reset ();
//        surfacec    .Reset ();
//        innercsfc   .Reset ();
//        innerskullc .Reset ();
//        outerskullc .Reset ();
        names       .Reset ();


        for ( int i = 0; i < (int) points; i++ )
            selcat.Set ( i, outputcat[ i ] == c );

        if ( ! (bool) selcat )
            continue;

                                        // distribute points
        for ( TIteratorSelectedForward seli ( selcat ); (bool) seli; ++seli ) {
                                        // points already with MRI center
//            surfacec   .Add ( pointsouterscalp[ seli() ] );
//            innercsfc  .Add ( pointsinnercsf  [ seli() ] );
//            innerskullc.Add ( pointsinnerskull[ seli() ] );
//            outerskullc.Add ( pointsouterskull[ seli() ] );

//            if ( xyznames ) names.Add ( (*xyznames)[ seli() ] );
            //else            names.Add ( IntegerToString ( seli() + 1 ) );
            names.Add ( IntegerToString ( seli() + 1 ) );
            }

//        sprintf ( _file, "%s\\More\\Cat %0d.Surface.spi",     CrisTransfer.BaseFileName, c );
//        surfacec.WriteFile ( _file, &names );
//        sprintf ( _file, "%s\\More\\Cat %0d.Csf.Inner.spi", CrisTransfer.BaseFileName, c );
//        innercsfc.WriteFile ( _file, &names );
//        sprintf ( _file, "%s\\More\\Cat %0d.Skull.Inner.spi", CrisTransfer.BaseFileName, c );
//        innerskullc.WriteFile ( _file, &names );
//        sprintf ( _file, "%s\\More\\Cat %0d.Skull.Outer.spi", CrisTransfer.BaseFileName, c );
//        outerskullc.WriteFile ( _file, &names );

                                        // distribute tracks
        sprintf ( exprad.Filename, "%s\\More\\Cat %0d.Radius.Avg.sef", CrisTransfer.BaseFileName, c );
        exprad.SetAtomType ( AtomTypeScalar );
        exprad.NumTracks        = (int) selcat;
        exprad.NumTime          = outputradsavg.GetDim2 ();
        exprad.ElectrodesNames  = names;

        for ( int t = 0; t < exprad.NumTime; t++ )
        for ( TIteratorSelectedForward seli ( selcat ); (bool) seli; ++seli )
            exprad.Write ( outputradsavg[ seli() ][ t ] );

        } // category
*/
    }

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  thicknessok;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Get the inner and outer limits of a given tissues set of indexes
                                        // Scans a line from any two points, not necessarily from center
bool    ScanTissuesMaxInterval  (   const Volume&       tissues,    TPointFloat         center,     // tissues relative center (voxel offset)
                                    TPointFloat         pinside,    TPointFloat         psurface,   // absolute (centered on 0)
                                    double              radiusmin,  double              radiusmax,
                                    TissuesIndex        minindex,   TissuesIndex        maxindex,
                                    float&              rmin,       float&              rmax
                                )
{
TPointFloat         top             = psurface - pinside;
top.Normalize ();

TPointFloat         p;
double              subvoxel        = 0.1;

                                        // constant shift
center     += pinside + 0.5;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // center toward surface
for ( rmin = radiusmin; rmin <= radiusmax; rmin++ ) {
            
    p       = center + top * rmin;
                                        // reached an edge? !interpolating between index values is wrong!
    if ( ! IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) )
        continue;

                                        // we entered the searched range - backtracking a little for sub-voxel accuracy
    do {
        rmin   -= subvoxel;

        p       = center + top * rmin;

        } while ( IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) );
                                    // interface is in-between
    rmin   += 0.5 * subvoxel;

    break;
    }

                                        // nothing? reset radius limits
if ( rmin > radiusmax ) {
    rmin    = rmax  = 0;
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // surface toward center
for ( rmax = radiusmax; rmax >= radiusmin; rmax-- ) {
            
    p       = center + top * rmax;
                                        // reached an edge? !interpolating between index values is wrong!
    if ( ! IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) )
        continue;

                                        // we entered the searched range - backtracking a little for sub-voxel accuracy
    do {
        rmax   += subvoxel;

        p       = center + top * rmax;

        } while ( IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) );
                                    // interface is in-between
    rmax   -= 0.5 * subvoxel;

    break;
    }

                                        // nothing? reset radius limits
if ( rmax < radiusmin ) {
    rmin     = rmax  = 0;
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Get the next inward tissue
bool    ScanTissuesNextBelow    (   const Volume&       tissues,    TPointFloat         center,     // relative (voxel offset)
                                    TPointFloat         pinside,    TPointFloat         psurface,   // absolute (centered on 0)
                                    TissuesIndex        minindex,   TissuesIndex        maxindex,
                                    float&              rmin,       float               rmax
                                )
{
TPointFloat         top             = psurface - pinside;
top.Normalize ();

TPointFloat         p;
double              subvoxel        = 0.1;

                                        // constant shift
center     += pinside + 0.5;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        
double              radiusmin       = 1;
double              radiusmax       = rmax - 0.5;   // even a slight delta should leave previous tissue


for ( rmin = radiusmax; rmin >= radiusmin; rmin-- ) {
            
    p       = center + top * rmin;

    if ( ! IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) )
        break;
    }

                                        // no tissue from beginning or reached center (likely undesirable)?
if ( rmin == radiusmax || rmin < radiusmin ) {
    rmin     = 0;
    return  false;
    }

                                        // backtracking for sub-voxel accuracy and to get back into tissue
do {
    rmin   += subvoxel;

    p       = center + top * rmin;

    } while ( ! IsInsideLimits ( (TissuesIndex) (int) tissues.GetValueChecked ( p.X, p.Y, p.Z ), minindex, maxindex ) );

//rmin   -= 0.5 * subvoxel;

return true;
}


//----------------------------------------------------------------------------
/*                                        // Get the next inward sphere intersection
bool    ScanTissuesNextBelow    (   TPointFloat             pinside,    TPointFloat             psurface,   // absolute (centered on 0)
                                    double                  tissueradius,                                   // tissue radius, on the sphere
                                    float&                  rmin,       float                   rmax        // radius on pinside-psurface axis
                                )
{
TPointFloat         top             = psurface - pinside;
TPointFloat         p;
double              step            = 0.001;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        
double              radiusmin       = 0;
double              radiusmax       = rmax;


for ( rmin = radiusmax; rmin >= radiusmin; rmin -=step ) {
            
    p       = pinside + top * rmin;

    if ( p.Norm () < tissueradius )
        break;
    }

                                        // no intersection reached?
if ( rmin == radiusmax || rmin < radiusmin ) {
    rmin     = 0;
    return  false;
    }


step   /= 100;
                                        // backtracking for sub-voxel accuracy and to get back into tissue
do {
    rmin   += step;

    p       = pinside + top * rmin;

    } while ( p.Norm () < tissueradius );

rmin   -= 0.5 * step;

return true;
}
*/

//----------------------------------------------------------------------------
                                        // Estimate the spongy layer as a ratio of the full skull, plus clipping the compact bone
                                        // Can return 0
double  SkullThicknessToSpongy  (   double      skullthickness,
                                    double      spongypercentage,
                                    double      compactminthickness,    double      compactmaxthickness
                                )
{
                                        // Removing a FIXED compact thickness to the whole skull
//double              compact         = SkullCompactThickness;
//double              spongy          = skullthickness - 2 * compact; // there are 2 compact layers thicknesses to be removed

                                        // Removing a RELATIVE compact thickness to the whole skull - seems more accurate as indeed compact layer thickness vary
double              spongy          = spongypercentage * skullthickness;

                                        // compute remaining compact bone, clipped to safety boundaries
double              compact         = Clip ( ( skullthickness - spongy ) / 2, compactminthickness, compactmaxthickness );

                                        // in case of clipping, propagate back to the spongy bone
                    spongy          = AtLeast ( 0.0, skullthickness - 2 * compact );


return  spongy;
}


//----------------------------------------------------------------------------
                                        // Using a precomputed tissues segmented template
                                        // Currently used for MNI - it will need some tuning to be run confidently on subjects' data
bool    EstimateTissuesRadii_Segmentation ( 
                                const TPoints&          points,
                                SpatialFilterType       smoothing,          const TElectrodesDoc*   xyzdoc,
                                const Volume&           tissues,            TPointDouble            mritissuesorigin,
                                TPointDouble            mricenter,          const TPointDouble&     voxelsize,
                                TPointFloat             inversecenter,
                                bool                    adjustradius,       double                  targetskullthickness,
                                TArray3<float>&         tissuesradii,
                                TVerboseFile*           verbose
                                )
{
int                 numel           = (int) points;

                                        // allocate results
tissuesradii.Resize ( numel, NumTissuesIndex, NumTissuesLimits );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !Children MNI Templates has a different MRI size, hence origin, as the Adults MNI Template!
if ( mritissuesorigin != mricenter ) {

    TPointFloat             deltaorigin         = mritissuesorigin - mricenter;

    inversecenter  += deltaorigin;

                                        // save this to verbose!
    if ( verbose ) {

        verbose->NextLine ( 2 );

        (ofstream&) *verbose << "Template MRI and Template Tissues appear to have different origins!" << fastendl;
        (ofstream&) *verbose << "Cartool will adjust the tissues center, but better check for the resulting tissues thicknesses!" << fastendl;

    //  verbose->Put ( "MRI Origin:",        mricenter           );  // already outputted
        verbose->Put ( "Tissues Center:",    mritissuesorigin    );
        verbose->Put ( "Shift applied:",     deltaorigin         );
        }


//  if ( CheckToBool ( CrisTransfer.Interactive ) )
//      ShowMessage ( "Template MRI and Template Tissues appear to have different origins!" NewLine "Fixing this by adjusting tissues center.", CreatingInverseTitle, ShowMessageWarning, this );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Extract a smoothed brain surface
FctParams           p;


Volume              brainlimit  ( tissues );
                                        // brain minus the CSF
p ( FilterParamThresholdMin )     = BrainMinIndex;
p ( FilterParamThresholdMax )     = BrainMaxIndex;
p ( FilterParamThresholdBin )     = 1;
brainlimit.Filter ( FilterTypeThresholdBinarize, p );
                                        // fill the gaps
p ( FilterParamDiameter )     = 6;
brainlimit.Filter ( FilterTypeDilate, p );

p ( FilterParamDiameter )     = 6;
p ( FilterParamNumRelax )     = 1;
brainlimit.Filter ( FilterTypeRelax,  p );

p ( FilterParamDiameter )     = 6;
brainlimit.Filter ( FilterTypeErode,  p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scanning tissues underneath each electrode
TPointFloat         center0;

OmpParallelFor

for ( int ei = 0; ei < numel; ei++ ) {

                                        // Scanning for skull - should always return non-null values
    bool        skullok     =   ScanTissuesMaxInterval  (   tissues,            inversecenter,
                                                            center0,            points[ ei ],               // line from origin to ei
                                                            1,                  points[ ei ].Norm (),
                                                            SkullMinIndex,      SkullMaxIndex,
                                                            tissuesradii ( ei, SkullIndex,InnerAbs ), tissuesradii ( ei, SkullIndex,OuterAbs ) 
                                                        );

    if ( skullok )

        tissuesradii ( ei, SkullIndex,ThickAbs )  = tissuesradii ( ei, SkullIndex,OuterAbs ) - tissuesradii ( ei, SkullIndex,InnerAbs );
    else
        continue; // this should be caught later on and generate some error messages


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Scanning for spongy skull, within the skull limits above
    bool        spongyok    =   ScanTissuesMaxInterval  (   tissues,            inversecenter,
                                                            center0,            points[ ei ],
                                                            tissuesradii ( ei, SkullIndex,InnerAbs ), tissuesradii ( ei, SkullIndex,OuterAbs ),
                                                            SkullSpongyIndex,   SkullSpongyIndex,
                                                            tissuesradii ( ei, SkullSpongyIndex,InnerAbs ), tissuesradii ( ei, SkullSpongyIndex,OuterAbs ) 
                                                        );

    if ( spongyok )

        tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = tissuesradii ( ei, SkullSpongyIndex,OuterAbs ) - tissuesradii ( ei, SkullSpongyIndex,InnerAbs );

    else {
                                        // provide for default minimal spongy skull thickness
        double          midskull    = ( tissuesradii ( ei, SkullIndex,InnerAbs ) + tissuesradii ( ei, SkullIndex,OuterAbs ) ) / 2;

        tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = MinSpongySkullThickness;
        tissuesradii ( ei, SkullSpongyIndex,InnerAbs )    = midskull - tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;
        tissuesradii ( ei, SkullSpongyIndex,OuterAbs )    = midskull + tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;
        }
*/

    double          midskull    = ( tissuesradii ( ei, SkullIndex,InnerAbs ) + tissuesradii ( ei, SkullIndex,OuterAbs ) ) / 2;
                                        // Skip scanning, and estimate the spongy part from the whole skull thickness
                                        // Note that currently the spongy tissues was done exactly this way at construction, so we are not losing anything - on the contrary
    tissuesradii ( ei, SkullSpongyIndex,ThickAbs )    = AtLeast ( MinSpongySkullThickness, SkullThicknessToSpongy ( tissuesradii ( ei, SkullIndex,ThickAbs ), SkullSpongyPercentage, SkullCompactMinThickness, SkullCompactMaxThickness ) );
    tissuesradii ( ei, SkullSpongyIndex,InnerAbs )    = midskull - tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;
    tissuesradii ( ei, SkullSpongyIndex,OuterAbs )    = midskull + tissuesradii ( ei, SkullSpongyIndex,ThickAbs ) / 2;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scanning for CSF & blood, but only next below the skull
                                        // Problem is going deep into "CSF holes" below the brain surface, which is not desirable for a spherical model
/*
    tissuesradii ( ei, CsfIndex,OuterAbs )  = tissuesradii ( ei, SkullIndex,InnerAbs );

    bool        csfok       =   ScanTissuesNextBelow    (   tissues,            inversecenter,
                                                            center0,            points[ ei ],
                                                            CsfIndex,           BloodIndex, 
                                                            tissuesradii ( ei, CsfIndex,InnerAbs ), tissuesradii ( ei, CsfIndex,OuterAbs ) 
                                                        );

                                        // if CSF exists
    if ( csfok )
                                        // compute CSF thickness - distance are quantified in voxels, so add 1 to include border
        tissuesradii ( ei, CsfIndex,ThickAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,InnerAbs );

    else {
                                        // provide for default minimal CSF thickness
        tissuesradii ( ei, CsfIndex,ThickAbs )    = MinCsfThickness;
        tissuesradii ( ei, CsfIndex,OuterAbs )    = tissuesradii ( ei, SkullIndex,InnerAbs );
        tissuesradii ( ei, CsfIndex,InnerAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );
        }
*/

                                        // Just landing to the smoothed-out brain surface - Closer to the EstimateTissuesRadii_T1 method
                                        // This ensures to keep the CSF layer above the brain, and NOT deep into it!
    TPointFloat     psurf ( points[ ei ] );
                                        // !Also shift origin here, as brainlimit has been extracted from Tissues!
    psurf.ResurfacePoint ( brainlimit, inversecenter, 1 );

    tissuesradii ( ei, CsfIndex,InnerAbs )    = psurf.Norm ();
    tissuesradii ( ei, CsfIndex,OuterAbs )    = tissuesradii ( ei, SkullIndex,InnerAbs );
    tissuesradii ( ei, CsfIndex,ThickAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,InnerAbs );

    if ( tissuesradii ( ei, CsfIndex,ThickAbs ) < MinCsfThickness ) {
                                        // provide for default minimal CSF thickness
        tissuesradii ( ei, CsfIndex,ThickAbs )    = MinCsfThickness;
        tissuesradii ( ei, CsfIndex,OuterAbs )    = tissuesradii ( ei, SkullIndex,InnerAbs );
        tissuesradii ( ei, CsfIndex,InnerAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );
        }
    } // for electrode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have filled in: ( Skull / SkullSpongy / CSF ) x ( In / Out / Thick )
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional filtering
if ( smoothing && xyzdoc ) {
                                        // filtering & recovering global rescaling factors
//  double              skullfilterratio        =   FilterEstimates ( tissuesradii,   SkullIndex,       smoothing,  xyzdoc->GetDocPath () );
//  double              skullspongyfilterratio  =   FilterEstimates ( tissuesradii,   SkullSpongyIndex, smoothing,  xyzdoc->GetDocPath () );
                                        // !only on the CSF which is problematic!
                                        // Filtering the thickness only, then updating the inner part
  /*double              csffilterratio          =*/ FilterEstimates ( tissuesradii,   CsfIndex,ThickAbs,   smoothing,  xyzdoc->GetDocPath () );

                                        // update inner part
    for ( int ei = 0; ei < numel; ei++ )
        tissuesradii ( ei, CsfIndex,InnerAbs )    = tissuesradii ( ei, CsfIndex,OuterAbs ) - tissuesradii ( ei, CsfIndex,ThickAbs );

//    DBGV3 ( skullfilterratio, skullspongyfilterratio, csffilterratio, "skullfilterratio, skullspongyfilterratio, csffilterratio" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Refining the thicknesses with the expected mean thickness
if ( adjustradius && targetskullthickness > 0 )

    AdjustSkullThickness    (   points, voxelsize,  tissuesradii, targetskullthickness    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finishing: setting missing values, computing relative ratios and final consistency check
bool                thicknessok     = RadiusAbsToRadiusRel  (   points,     tissuesradii  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  thicknessok;
}


//----------------------------------------------------------------------------
                                        // Output as a set of 3D surfaces, for display
void    WriteTissuesRadiiToFile     (   const TArray3<float>&   tissuesradii,       const TSelection&       seltissues,
                                        const TPoints&          xyzpoints,          const TStrings&         xyznames,
                                        const TPointFloat&      mricenter,
                                        const TPointFloat&      inversecenter,
                                        const char*             filesurfaceall
                                    )
{
if ( tissuesradii.IsNotAllocated ()
  || seltissues  .IsNotAllocated ()
  || xyzpoints   .IsEmpty ()
  || StringIsEmpty ( filesurfaceall ) )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             pointsouterscalp        ( seltissues[ ScalpIndex         ] ? (int) xyzpoints : 0 );
TPoints             pointsouterskull        ( seltissues[ SkullIndex         ] ? (int) xyzpoints : 0 );
TPoints             pointsouterskullspongy  ( seltissues[ SkullSpongyIndex   ] ? (int) xyzpoints : 0 );
TPoints             pointsinnerskullspongy  ( seltissues[ SkullSpongyIndex   ] ? (int) xyzpoints : 0 );
TPoints             pointsinnerskull        ( seltissues[ SkullIndex         ] ? (int) xyzpoints : 0 );
TPoints             pointsinnercsf          ( seltissues[ CsfIndex           ] ? (int) xyzpoints : 0 );
TPointFloat         deltacenter     = inversecenter - mricenter;

                                        // compute each layer's absolute positions
for ( int ei = 0; ei < (int) xyzpoints; ei++ ) {

    const TPointFloat&      p           = xyzpoints[ ei ];
                                        // absolute position, translated back to MRI center
    if ( seltissues[ ScalpIndex         ] )     pointsouterscalp        [ ei ]  =   p                                                         + deltacenter;
    if ( seltissues[ SkullIndex         ] )     pointsouterskull        [ ei ]  = ( p * tissuesradii ( ei, SkullIndex,       OuterRel ) ) + deltacenter;
    if ( seltissues[ SkullSpongyIndex   ] )     pointsouterskullspongy  [ ei ]  = ( p * tissuesradii ( ei, SkullSpongyIndex, OuterRel ) ) + deltacenter;
    if ( seltissues[ SkullSpongyIndex   ] )     pointsinnerskullspongy  [ ei ]  = ( p * tissuesradii ( ei, SkullSpongyIndex, InnerRel ) ) + deltacenter;
    if ( seltissues[ SkullIndex         ] )     pointsinnerskull        [ ei ]  = ( p * tissuesradii ( ei, SkullIndex,       InnerRel ) ) + deltacenter;
    if ( seltissues[ CsfIndex           ] )     pointsinnercsf          [ ei ]  = ( p * tissuesradii ( ei, CsfIndex,         InnerRel ) ) + deltacenter;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting the data structure for saving a complex electrode setup .els
vector<ClusterPoints>   clusters;
ClusterPoints           cluster;
                                        // for all clusters:
cluster.Names       = xyznames;
cluster.ClusterType = Cluster3D;

if ( seltissues[ ScalpIndex         ] ) {   cluster.Points = pointsouterscalp;          StringCopy ( cluster.ClusterName, "Scalp"          );   clusters.push_back ( cluster );   }
if ( seltissues[ SkullIndex         ] ) {   cluster.Points = pointsouterskull;          StringCopy ( cluster.ClusterName, "SkullOut"       );   clusters.push_back ( cluster );   }
if ( seltissues[ SkullSpongyIndex   ] ) {   cluster.Points = pointsouterskullspongy;    StringCopy ( cluster.ClusterName, "SkullSpongyOut" );   clusters.push_back ( cluster );   }
if ( seltissues[ SkullSpongyIndex   ] ) {   cluster.Points = pointsinnerskullspongy;    StringCopy ( cluster.ClusterName, "SkullSpongyIn"  );   clusters.push_back ( cluster );   }
if ( seltissues[ SkullIndex         ] ) {   cluster.Points = pointsinnerskull;          StringCopy ( cluster.ClusterName, "SkullIn"        );   clusters.push_back ( cluster );   }
if ( seltissues[ CsfIndex           ] ) {   cluster.Points = pointsinnercsf;            StringCopy ( cluster.ClusterName, "CSF"            );   clusters.push_back ( cluster );   }

WritePoints  ( filesurfaceall, clusters );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
