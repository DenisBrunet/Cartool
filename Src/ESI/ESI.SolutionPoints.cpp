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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "ESI.SolutionPoints.h"

#include    "Strings.Utils.h"
#include    "Math.Resampling.h"
#include    "TVolume.h"
#include    "Geometry.TPoints.h"
#include    "Strings.Utils.h"
#include    "TVector.h"
#include    "TSelection.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TVolumeDoc.h"
#include    "TExportVolume.h"

#include    "GlobalOptimize.Tracks.h"
#include    "GlobalOptimize.Points.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Brain document to Grey Mask file
bool    SegmentGreyMatter ( const TVolumeDoc* mridoc, GreyMatterFlags greyflags, const char* greyfile )
{
FctParams           p;

//DBGM ( mridoc->GetDocPath (), "Brain to extract the Grey from" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // local copy
Volume              Grey ( *mridoc->GetData () );

p ( FilterParamGreyType   ) = greyflags;
p ( FilterParamGreyAxis   ) = mridoc->GetAxisIndex ( LeftRight );   // for symmetric case
p ( FilterParamGreyOrigin ) = mridoc->GetOrigin ()[ mridoc->GetAxisIndex ( LeftRight ) ] ? mridoc->GetOrigin        ()[ mridoc->GetAxisIndex ( LeftRight ) ]
                                                                                         : mridoc->GetDefaultCenter ()[ mridoc->GetAxisIndex ( LeftRight ) ];   // !a bit risky if center is not the symmetric pivot!

                                        // IF input volume is already a mask, just use it without any other processing
bool                greyok          = mridoc->IsMask () ? true 
                                                        : Grey.SegmentGreyMatter ( p, true );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write to file
const TBoundingBox<int>*    Size    = mridoc->GetSize ();
TPointDouble                point;


TExportVolume       expvol;

StringCopy ( expvol.Filename, greyfile );

expvol.VolumeFormat     = GetVolumeAtomType ( &Grey, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) );

expvol.MaxValue         = Grey.GetAbsMaxValue ();   // we know that

expvol.Dimension.X      = Size->GetXExtent ();
expvol.Dimension.Y      = Size->GetYExtent ();
expvol.Dimension.Z      = Size->GetZExtent ();

expvol.VoxelSize        = mridoc->GetVoxelSize ();

expvol.Origin           = mridoc->GetOrigin ();

expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () );

expvol.NiftiIntentCode  = NiftiIntentCodeGreyMask;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentNameGreyMask , NiftiIntentNameSize - 1 );

StringCopy  ( expvol.Orientation, NiftiOrientation, 3 );


expvol.Write ( Grey, ExportArrayOrderZYX );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


return  greyok;
}


//----------------------------------------------------------------------------
                                        // Number of neighbors for grey points only
void    GetNumGreyNeighbors (   const TPoints&      points,
                                double              neighborradius,
                                TArray1<int>&       NumNeighbors,
                                const TSelection*   spsrejected
                            )
{
                                        // allocate array count
NumNeighbors.Resize ( (int) points );

                                        // scan all pairs of points
for ( int i = 0; i < (int) points; i++ ) {
                                        // optionally skipping rejected points?
    if ( spsrejected && spsrejected->IsSelected ( i ) )
        continue;

    for ( int j = i + 1; j < (int) points; j++ ) {
                                        // optionally skipping rejected points?
        if ( spsrejected && spsrejected->IsSelected ( j ) )
            continue;


        if ( ( points[ j ] - points[ i ] ).Norm () <= neighborradius ) {

            NumNeighbors[ i ]++;
            NumNeighbors[ j ]++;
            } // if neighborradius
        } // for j
    } // for i
}


//----------------------------------------------------------------------------
                                        // Either the desired number of points or resolution should be set
bool    ComputeSolutionPoints (   
                            const TVolumeDoc*   mribraindoc,        const TVolumeDoc*   mrigreydoc, 
                            int                 numsolpointswished, double              resolutionwished,
                            GreyMatterFlags     spflags,
                            NeighborhoodType    loretaneighborhood, NeighborhoodType    lauraneighborhood, 
                            TPoints&            solpoints,          TStrings&           spnames,
                            const char*         filesp
                            )
{
enum                {
                    gaugedosplobal,
                    gaugedospconv,
                    };

TSuperGauge         gauge;
int                 maxguess        = 100; // 30;

gauge.Set ( "Solution Points" );

gauge.AddPart ( gaugedosplobal,        7,   25 );
gauge.AddPart ( gaugedospconv,  maxguess,   75 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Estimate the step from the targeted number of solution points and the size of Grey
Volume              volume         ( *mrigreydoc ->GetData () );
int                 numgrey         = volume.GetNumSet ();

if ( numgrey == 0 )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              step;
double              stephigh;
double              steplow;


if      ( numsolpointswished > 0 ) {

    step            = AtLeast ( 1.0, CubicRoot ( (double) numgrey / numsolpointswished * 1.25 ) );
    stephigh        = step * 1.50;
    steplow         = step * 0.50;
    }
else if ( resolutionwished > 0 ) {

    step            = resolutionwished;
    stephigh        = step;
    steplow         = step;
    }
else 
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Preprocess grey mask
                                        // a measure of global scale, used for filtering(?)
double              sizescale       = mribraindoc->GetBounding ()->MeanSize () / 100.0;
FctParams           params;


gauge.Next ( gaugedosplobal );
                                        // get rid of any downsampling aliasing by inflating (half, as grey is already Fat style) the estimated step
if ( sizescale < 1 ) {
                                        // for lower resolution, this is more precise
    params ( FilterParamDiameter   )    = AtLeast ( sizescale, step / 2 );
    params ( FilterParamResultType )    = FilterResultSigned;
    volume.Filter ( FilterTypeMax, params );
    }
else {
                                        // this way faster - OK at higher resolution
    params ( FilterParamDiameter )     = AtLeast ( 1.0 /*sizescale * 0.5*/, step * 0.20 );

    volume.Filter ( FilterTypeDilate, params );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugedosplobal );

Volume              BrainMask      ( *mribraindoc->GetData () );
bool                isbrainmask     = mribraindoc->IsMask ();

                                        // fill the mask, in case it's a hollow grey thing, which is not an appropriate mask for here
params ( FilterParamToMaskThreshold )     = isbrainmask ? 0.5 : BrainMask.GetBackgroundValue ();
params ( FilterParamToMaskNewValue  )     = 1;
params ( FilterParamToMaskCarveBack )     = true;                // carving

BrainMask.Filter ( FilterTypeToMask, params );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugedosplobal );
                                        // shrink outer part of the brain
if ( sizescale < 1 ) {
                                        // for lower resolution, this is more precise
    params ( FilterParamDiameter )     = isbrainmask ? step * 0.5 : step * 0.5;

    BrainMask.Filter ( FilterTypeMin, params );
    }
else {
                                        // this way faster - OK at higher resolution
    params ( FilterParamDiameter )     = AtLeast ( 1.0 /*sizescale * 0.5*/, step * 0.20 );

    BrainMask.Filter ( FilterTypeErode, params );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugedosplobal );
                                        // clip anything outside the brain
volume.ApplyMaskToData ( BrainMask );


//TFileName           _file;
//StringCopy ( _file, filesphere );
//ReplaceExtension ( _file, "Volume for SP.hdr" );
//volume.WriteFile ( _file );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setup for flags
//bool                spfilter        = spflags & GreyMatterCheckMask;
Volume              spvol;
TPointDouble        origin;
int                 numsp;
FctParams           plaura;
FctParams           ploreta;
FctParams           psingle;
NeighborhoodType    neighborhood    = min ( loretaneighborhood, lauraneighborhood );
                                        // 0 to 3 neighbors is a very, very low population (all the time < 0.1%)
                                        // 4 to 7 neighbors is very low population (nearly all the time < 0.3%)
plaura ( FilterParamNeighborhood )    = lauraneighborhood;
plaura ( FilterParamNumNeighbors )    = lauraneighborhood  == Neighbors26 ? 8 : lauraneighborhood  == Neighbors18 ? 6 : 2;

ploreta ( FilterParamNeighborhood )   = loretaneighborhood;
ploreta ( FilterParamNumNeighbors )   = loretaneighborhood == Neighbors26 ? 8 : loretaneighborhood == Neighbors18 ? 6 : 2;    // reject lower than 8/6/2

//psingle ( FilterParamNeighborhood )   = Neighbors6;         // using the highest neighborhood constraint
psingle ( FilterParamNeighborhood )   = neighborhood;         // using the requested neighborhood, for more precise results
psingle ( FilterParamNumNeighbors )   = 1;                    // points without neighbor


TPointDouble        greyorigin      = mrigreydoc->GetOrigin ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             points;
TPoints             bestsolpoints;
int                 bestnumsolpoints    = 0;



for ( int numguess = 0; numguess < maxguess; numguess++ ) {

    gauge.Next ( gaugedospconv );
//    gauge.SetValue ( gaugedospconv, Maxed ( gaugemax, 1 - RelativeDifference ( steplow, stephigh ) ) );
    
                                        // compute an origin shift, so that we are actually centered to the grey center,
                                        // laying the solution points nicely on each side
    origin.X        = ModuloTo ( greyorigin.X, step ) + 0.5 * step;
    origin.Y        = ModuloTo ( greyorigin.Y, step ) + 0.5 * step;
    origin.Z        = ModuloTo ( greyorigin.Z, step ) + 0.5 * step;

                                        // downsample volume
    spvol           = Volume ( volume, step, 3, FilterTypeMedian, InterpolateTruncate, origin );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( spflags & GreyMatterSymmetric ) {

        int                 xo              = greyorigin.X / step;

        for ( int y = 0; y < spvol.GetDim2 (); y++ )
        for ( int z = 0; z < spvol.GetDim3 (); z++ )
//      for ( int x1 = xo - 1, x2 = xo + 1; x1 >= 0 && x2 < spvol.GetDim1 (); x1--, x2++ ) // more centered
        for ( int x1 = xo - 1, x2 = xo; x1 >= 0 && x2 < spvol.GetDim1 (); x1--, x2++ )     // more left

            if ( spvol ( x1, y, z ) || spvol ( x2, y, z ) )

                spvol ( x1, y, z )  = spvol ( x2, y, z ) = 1;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( spflags & ( GreyMatterLauraCheck | GreyMatterLoretaCheck | GreyMatterSinglePointCheck ) ) {
                                        // we repeat the filtering a few times, as removing some points will affect its former neighbors too
        for ( int i = 0; i < 3; i++ ) {

            if ( spflags & GreyMatterLauraCheck       )   spvol.Filter ( FilterTypeLessNeighbors, plaura  );
            if ( spflags & GreyMatterLoretaCheck      )   spvol.Filter ( FilterTypeLessNeighbors, ploreta );
            if ( spflags & GreyMatterSinglePointCheck )   spvol.Filter ( FilterTypeLessNeighbors, psingle );
            }
        } // Laura / Loreta / singleton


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transform volume into points
    solpoints.Reset ();

    solpoints.Add ( spvol, origin.X, origin.Y, origin.Z, step );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove the points out of mask
                                        // skip that part if forced symmetry, as some symmetrical points could/will not be part of the original brain
    if ( ! ( spflags & GreyMatterSymmetric ) ) {

        points.Set ( solpoints );

        solpoints.Reset ();
                                        // keep points within original brain
        for ( int i = 0; i < (int) points; i++ )

            if ( BrainMask.GetValue ( points[ i ] ) )

                solpoints.Add ( points[ i ] );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // how many points remaining?
    numsp           = (int) solpoints;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a single run is enough when resolution has been specified
    if ( resolutionwished > 0 ) {

        bestsolpoints       = solpoints;
        bestnumsolpoints    = numsp;
        break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store the current closest number of solution points
    if ( fabs ( (double) ( numsp - numsolpointswished ) ) < fabs ( (double) ( bestnumsolpoints - numsolpointswished ) ) 
      && numsp >= numsolpointswished    // while preferring being above the requested number of points (5009 is clearer than 4993)
        ) {

        bestsolpoints       = solpoints;
        bestnumsolpoints    = numsp;
        }
                                        // stop if we are close enough to the requested # of points
                                        // relaxing the criterion with the # of iterations
//  if ( fabs ( numsp - numsolpointswished ) / numsolpointswished < 0.003 * ( 1 + 12 * Power ( (double) numguess / maxguess, 3 ) ) )

                                        // do an early exit if spot on!
    if ( numsp == numsolpointswished )
        break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not close enough, update step with dichomotic approach

                                        // update current range of steps
    if ( numsp >= numsolpointswished )  Maxed ( steplow,  step );
    else                                Mined ( stephigh, step );
                                        // new step is middle point
    step    = ( steplow + stephigh ) / 2;

                                        // both limits converged?
    if ( RelativeDifference ( steplow, stephigh ) < SingleFloatEpsilon )
        break;
    }


gauge.FinishPart ( gaugedospconv );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

gauge.Next ( gaugedosplobal );


solpoints   = bestsolpoints;
                                        // transform to absolute (centered) coordinates
mrigreydoc->ToAbs ( solpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sort the points by octants
gauge.Next ( gaugedosplobal );

                                        // Anterior, Posterior, Superior, Inferior, Left, Right
#define             anterior            "A"
#define             posterior           "P"
#define             superior            "S"
#define             inferior            "I"
#define             left                "L"
#define             right               "R"

char                boxname[][ 64 ]     = {
                                        left  "" anterior  "" superior,   
                                        left  "" anterior  "" inferior,   
                                        left  "" posterior "" superior,  
                                        left  "" posterior "" inferior,
                                        right "" anterior  "" superior,   
                                        right "" anterior  "" inferior,   
                                        right "" posterior "" superior,  
                                        right "" posterior "" inferior
                                        };

TPoints             box[ 8 ];
TPointFloat         p;
char                buff[ 256 ];

                                        // distribute the points into their appropriate boxes
for ( int i = 0; i < solpoints.GetNumPoints (); i++ ) {

    p.Set ( solpoints[ i ] );
                                        // build the proper index in [0..7] from position compared to origin
    box[  ( ( p.X > 0 ) << 2 )          // left-right
        | ( ( p.Y < 0 ) << 1 )          // anterior-posterior
        |   ( p.Z < 0 )                 // superior-inferior
        ].Add ( p );
    }


solpoints.Reset ();
spnames.  Reset ();


gauge.Next ( gaugedosplobal );

                                        // put back the boxed points into the list
for ( int i = 0; i < 8; i++ ) {
                                        // sort by X, then Y, then Z
    box[ i ].Sort ();
                                        // create new names, including the position
    for ( int spi = 0; spi < (int) box[ i ]; spi++ ) {
        sprintf ( buff, "%s%0d", boxname[ i ], spi + 1 );
        spnames.Add ( buff );
        }

    solpoints   += box[ i ];
    }

                                        // write coordinates & labels
solpoints.WriteFile ( filesp, &spnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // mribraindoc could either be the original grey mask, or the original brain
bool    DownsampleSolutionPoints (  const TVolumeDoc*   mribraindoc, const TPoints& solpoints, double neighborradius, int numsolpointswished,
                                    TSelection&         spsrejected )
{
int                 numsolp         = (int) solpoints;
TPointFloat         spcenter        = solpoints.GetClosestPoint ( solpoints.GetCenter () );
double              mind            = solpoints.GetMedianDistance ();

                                        // build a list of rejected points
spsrejected.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
enum                {
                    gaugedownspglobal     = 0,
                    gaugedownspgrey,
                    };

TSuperGauge         gauge;

gauge.Set ( "Downsampling SPs" );

gauge.AddPart ( gaugedownspglobal,  2,  25 );
gauge.AddPart ( gaugedownspgrey,    4,  75 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reject anything not in the grey, if brain exists
if ( mribraindoc ) {
                                        // we need a local grey mask
    gauge.Next ( gaugedownspgrey );

    const Volume*       Brain       = mribraindoc->GetData ();
                                        // need a downsampled local copy, to allow some modifications
    int                 downratio       = AtLeast ( 1, Round ( (double) mribraindoc->GetSize ()->GetMaxExtent () / 128 ) );     // 64..128..191 -> 1, 192..256..383 -> 2 etc...

    Volume              Grey ( *Brain, downratio, downratio, FilterTypeMean );
                                        // solution points can be absolute
    TPointDouble        braindowncenter = mribraindoc->GetOrigin () / downratio;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the grey matter
    gauge.Next ( gaugedownspgrey );

                                        // skip this in case the input is already a mask...
    if ( ! mribraindoc->IsMask () ) {
        FctParams           p;

        p ( FilterParamGreyType )   = GreyMatterAsymmetric | GreyMatterFat | GreyMatterPostprocessing;
                                        // only needed for symmetric case:
//      p ( FilterParamGreyAxis   ) = mribraindoc->GetAxisIndex ( LeftRight );   // for symmetric case
//      p ( FilterParamGreyOrigin ) = mribraindoc->GetOrigin ()[ mribraindoc->GetAxisIndex ( LeftRight ) ] ? mribraindoc->GetOrigin        ()[ mribraindoc->GetAxisIndex ( LeftRight ) ]
//                                                                                                         : mribraindoc->GetDefaultCenter ()[ mribraindoc->GetAxisIndex ( LeftRight ) ];   // !a bit risky if center is not the symmetric pivot!
        Grey.SegmentGreyMatter ( p, false );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Estimate the step from the targeted number of solution points and the size of Grey
    int                 numgrey         = Grey.GetNumSet ();
    double              step            = PowerRoot ( (double) numgrey / numsolpointswished * 0.85, 3.0 );

    if ( step <= 0 )
        step    = 1;

//    DBGV3 ( numgrey, numsolpointswished, step, "numgrey, numsolpointswished -> step" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // inflate according to the (full) estimated step, to get rid of any aliasing
    gauge.Next ( gaugedownspgrey );

    FctParams           params;
    params ( FilterParamDiameter   )    = step;
    params ( FilterParamResultType )    = FilterResultSigned;
    Grey.Filter ( FilterTypeMax, params );

                                        // clip any leak outside the brain - not needed as the input SPs are already only within the brain
//  gauge.Next ( gaugedownspgrey );
//
//  Grey.ApplyMaskToData ( *Brain );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reject if not inside grey
    gauge.Next ( gaugedownspgrey );

    for ( int spi = 0; spi < numsolp; spi++ ) {

        TPointInt       pi ( solpoints[ spi ].X / downratio + braindowncenter.X,
                             solpoints[ spi ].Y / downratio + braindowncenter.Y,
                             solpoints[ spi ].Z / downratio + braindowncenter.Z );

                                        // check boundaries, just in case
        if ( Grey.GetValueChecked ( pi ) == 0 )

            spsrejected.Set ( spi );
        }

                            // in case we had a problem somewhere, don't reject all the points!
    if ( numsolp == (int) spsrejected )
        spsrejected.Reset ();

    } // if cliptogrey
else
    gauge.FinishPart ( gaugedownspgrey );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove points not on a downsampled grid
gauge.Next ( gaugedownspglobal );
                                        // set the step according to what remains from the brain
int                 numspremaining  = numsolp - (int) spsrejected;

int                 step            = Round ( PowerRoot (   (double) numspremaining / numsolpointswished 
                                                          * ( (bool) spsrejected ? 0.85 : 1 ), 3.0       ) );

//DBGV4 ( numsolp, numspremaining, numsolpointswished, step, "numsolp, numspremaining, numsolpointswished -> step" );

                                        // add to the list of rejected points those that don't fit on the downsampling grid
for ( int spi = 0; spi < numsolp; spi++ ) {

    TPointFloat     downp   = ( solpoints[ spi ] - spcenter ) / mind;
                                        // falls on the sub-sampled grid?
    if ( Round ( downp.X ) % step != 0
      || Round ( downp.Y ) % step != 0
      || Round ( downp.Z ) % step != 0 )

        spsrejected.Set ( spi );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking for points without neighbors
gauge.Next ( gaugedownspglobal );


TVector<int>        NumNeighbors ( (int) solpoints );
int                 newnum          = (int) solpoints - (int) spsrejected;
int                 oldnum          = 0;


do {
                                        // simplified count for single neighbors
    NumNeighbors.ResetMemory ();


    for ( int i = 0; i < (int) solpoints; i++ ) {

        if ( spsrejected[ i ] )
            continue;

        for ( int j = i + 1; j < (int) solpoints; j++ ) {

            if ( spsrejected[ j ] )
                continue;

            if ( ( solpoints[ j ] - solpoints[ i ] ).Norm () <= step * neighborradius ) {
                NumNeighbors[ i ]++;
                NumNeighbors[ j ]++;
                }
            } // for j
        } // for i

                                        // reject no-neighbor points
    for ( int i = 0; i < (int) solpoints; i++ )
        if ( ! spsrejected[ i ] && NumNeighbors[ i ] <= 0 )
            spsrejected.Set ( i );

//    DBGV3 ( (int) solpoints, spsrejected.NumSet (), neighborradius, "#points #spsrejected, neighborradius" );


    oldnum      = newnum;
    newnum      = (int) solpoints - (int) spsrejected;

    } while ( oldnum != newnum );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Looking for points outside the grid defined by a set of reference points, as to avoid extrapolation
                                        // Tri-linear: look for all 8 cube neighbors
void    ScanForOutsidePoints ( const TPoints& referencepoints, const TPoints& checkedpoints, TSelection& spsrejected )
{
                                        // convert the grid-aligned points to a volume
TBoundingBox<double>    spbounding ( referencepoints );
double                  step            = referencepoints.GetMedianDistance ();

TMatrix44           sptovol;
sptovol.Translate ( - ( spbounding.XMin () - step ),    - ( spbounding.YMin () - step ),    - ( spbounding.ZMin () - step ),    MultiplyLeft ); // add 1 step on each side, for the truncation to work correctly
sptovol.Scale     ( 1 / step,                           1 / step,                           1 / step,                           MultiplyLeft );

TVector3Int         size      ( spbounding.GetXExtent () / step + 3, spbounding.GetYExtent () / step + 3, spbounding.GetZExtent () / step + 3 );
//TVolume<int>      spvol     ( size.X, size.Y, size.Z );
Volume              spvol     ( size.X, size.Y, size.Z ); // we just need a flag here
//Volume            spvolout  ( size.X, size.Y, size.Z );

TPointDouble        p;
double              fx;
double              fy;
double              fz;


                                        // build a list of rejected points
spsrejected.Reset ();


//#define             DebugSpVol
#if defined(DebugSpVol)
TPoints             volreferencepoints ( referencepoints );
TPoints             acceptedpoints;
TPoints             rejectedpoints;
sptovol.Apply ( volreferencepoints );
#endif


for ( int ini = 0; ini < (int) referencepoints; ini++ ) {

    p.Set ( referencepoints[ ini ] );

    sptovol.Apply ( p );

    p      += 1e-3;                     // points are on a grid, get rid of transformation rounding errors to land nicely on a grid

    p.Truncate ();

//  spvol ( p ) = ini + 1;
    spvol ( p ) = true;
//  spvolout ( p ) = ( (double) ini / (int) referencepoints * 254 ) + 1;
    }


                                        // scan each target point
for ( int outi = 0; outi < (int) checkedpoints; outi++ ) {
                                        // get point
    p.Set ( checkedpoints[ outi ] );
                                        // transform to volume
    sptovol.Apply ( p );

    p      += 1e-3;                     // points are on a grid, get rid of transformation rounding errors to land nicely on a grid

//  p.Truncate ();                      // don't: we need the fraction for the interpolation

                                        // get fractions from point to corners
    fx          = Fraction ( p.X );
    fy          = Fraction ( p.Y );
    fz          = Fraction ( p.Z );

                                        // special case: spot on a point, doesn't need 8 neighbors / interpolation
    if ( ( fx + fy + fz ) <= 5e-3 ) {

        if ( spvol.GetValueChecked ( p.X,     p.Y,     p.Z     ) != 0 ) {
#if defined(DebugSpVol)
//            acceptedpoints.Add ( *checkedpoints[ outi ] );
            acceptedpoints.Add ( p );
#endif
            }
        else {
                                        // no points below?
#if defined(DebugSpVol)
//            rejectedpoints.Add ( *checkedpoints[ outi ] );
            rejectedpoints.Add ( p );
#endif
            spsrejected.Set ( outi );
            }

        continue;
        } // exact point
                                        // here, not exactly on a point

                                        // check all corners have neighbors, for a later trilinear interpolation
    if (   spvol.GetValueChecked ( p.X,     p.Y,     p.Z     ) != 0
        && spvol.GetValueChecked ( p.X + 1, p.Y,     p.Z     ) != 0
        && spvol.GetValueChecked ( p.X,     p.Y + 1, p.Z     ) != 0
        && spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z     ) != 0
        && spvol.GetValueChecked ( p.X,     p.Y,     p.Z + 1 ) != 0
        && spvol.GetValueChecked ( p.X + 1, p.Y,     p.Z + 1 ) != 0
        && spvol.GetValueChecked ( p.X,     p.Y + 1, p.Z + 1 ) != 0
        && spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z + 1 ) != 0 ) {
#if defined(DebugSpVol)
//      acceptedpoints.Add ( *checkedpoints[ outi ] );
        acceptedpoints.Add ( p );
#endif
        }
    else {
                                        // no neighbor gets close enough? -> reject
#if defined(DebugSpVol)
//      rejectedpoints.Add ( *checkedpoints[ outi ] );
        rejectedpoints.Add ( p );
#endif
        spsrejected.Set ( outi );
        }

    } // for checkedpoints



#if defined(DebugSpVol)
TFileName           _file;
sprintf ( _file, "%s\\More\\SPVol.Reference Points.hdr", CrisTransfer.BaseFileName );
spvol.WriteFile ( _file );
//spvolout.WriteFile ( _file );

sprintf ( _file, "%s\\More\\SPVol.Reference Points.spi", CrisTransfer.BaseFileName );
//volreferencepoints -= TPointFloat ( 0.5, 0.5, 0.5 );    //for better visualization, as we truncated the positions
volreferencepoints.WriteFile ( _file );

sprintf ( _file, "%s\\More\\SPVol.Accepted Points.spi", CrisTransfer.BaseFileName );
//acceptedpoints -= TPointFloat ( 0.5, 0.5, 0.5 );
acceptedpoints.WriteFile ( _file );

sprintf ( _file, "%s\\More\\SPVol.Rejected Points.spi", CrisTransfer.BaseFileName );
//rejectedpoints -= TPointFloat ( 0.5, 0.5, 0.5 );
rejectedpoints.WriteFile ( _file );

//exit ( 1 );
#endif
}


//----------------------------------------------------------------------------
                                        // Mark points that have 0 neighbors
                                        // Retuns the # of points added to spsrejected
int     RejectSingleNeighbors   (   const TPoints&      pointsin,
                                    NeighborhoodType    neighborhood,
                                    TSelection&         spsrejected
                                )
{
TPoints             points ( pointsin );
double              neighborradius  = points.GetMedianDistance () * Neighborhood[ neighborhood ].MidDistanceCut;
TArray1<int>        NumNeighbors;
                                        // we doon't really need noneighbors right now, as LORETA and LAURA already track down these points
//TSelection          noneighbors     = TSelection ( (int) points, OrderSorted );     // do a local count, for display

                                                                                     // don't count rejected points!
GetNumGreyNeighbors ( points, neighborradius, NumNeighbors, &spsrejected );

                                        // Check # of neighbors
int                 numnn0          = 0;

for ( int i = 0; i < (int) points; i++ ) {
                                        // if no neighbors & not already rejected
    if ( NumNeighbors[ i ] == 0 && ! spsrejected[ i ] ) {
        spsrejected.Set ( i );
//      noneighbors.Set ( i );
        numnn0++;
        }
    }


/*if ( (bool) noneighbors ) {

    char                buff[ 1024 ];
                                        // always show to the user
    sprintf ( buff, "%0d Solution Points have 0 neighbors, and are therefore improper for LORETA / LAURA inversion.\n", (int) noneighbors );
    StringAppend ( buff, "These points will be kept in the output inverse space, but setting the Inverse Matrix to 0 at these positions.\n" );
    ShowMessage ( buff, CreatingInverseTitle, ShowMessageWarning, this );

                                        // save this to verbose!
    verbose.NextLine ( 1 );
    verbose.NextTopic ( "Matrix Inversion Warning:" );
    (ofstream&) verbose << buff;

                                        // write list of 0 neighbors points
    verbose.NextLine ();
    for ( int sp = 0; sp < (int) points; sp++ )
        if ( noneighbors [ sp ] ) {
            (ofstream&) verbose << StreamFormatFloat32 << points[ sp ].X << "\t";
            (ofstream&) verbose << StreamFormatFloat32 << points[ sp ].Y << "\t";
            (ofstream&) verbose << StreamFormatFloat32 << points[ sp ].Z << "\t";
            (ofstream&) verbose << StreamFormatText    << spnames  [ sp ];
            (ofstream&) verbose << "\n";
            }
    verbose.NextLine ();
    verbose.Flush ();
    } // if noneighbors*/


return  numnn0;
}


//----------------------------------------------------------------------------
                                        // will modify the given list of points
void    RejectPointsFromList    ( TPoints& points, TStrings&    names, const TSelection& spsrejected )
{
                                        // nothing to do?
if ( spsrejected.IsNotAllocated () || (int) spsrejected == 0 )
    return;

                                        // do a local copy
TPoints             pointsin ( points );
TStrings            namesin  ( names  );

points.Reset ();
names .Reset ();


for ( int i = 0; i < (int) pointsin; i++ ) {

    if ( spsrejected[ i ] )
        continue;

    points.Add ( pointsin[ i ] );

    if ( i < (int) namesin )
        names.Add ( namesin[ i ] );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
