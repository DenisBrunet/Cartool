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

#include    "ESI.LeadFields.h"

#include    "CartoolTypes.h"
#include    "Strings.Utils.h"
#include    "Geometry.TPoints.h"
#include    "Geometry.TDipole.h"
#include    "TArray1.h"
#include    "TArray3.h"
#include    "Math.Resampling.h"
#include    "Dialogs.TSuperGauge.h"
#include    "GlobalOptimize.Tracks.h"
#include    "GlobalOptimize.Points.h"
#include    "Volumes.SagittalTransversePlanes.h"
#include    "TFilters.h"
#include    "TVolume.h"

#include    "ESI.TissuesConductivities.h"
#include    "ESI.TissuesThicknesses.h"
#include    "ESI.HeadSphericalModel.h"
#include    "ESI.InverseModels.h"

#include    "TElectrodesDoc.h"
#include    "TVolumeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void        OneLeadFieldPreset::GetTissuesSelection ( TSelection& seltissues  )   const
{
seltissues  = TSelection ( NumTissuesIndex, OrderArbitrary );


if      ( IsHuman () ) {
                                        // Conductivities are set differently for each model
    if      ( IsIsotropic3ShellSphericalAry () ) {

        seltissues.Set ( BrainIndex         );
        seltissues.Set ( SkullIndex         );
        seltissues.Set ( ScalpIndex         );
        }

    else if ( IsIsotropic3ShellSpherical () ) {

        seltissues.Set ( BrainIndex         );
        seltissues.Set ( SkullIndex         );
        seltissues.Set ( ScalpIndex         );
        }

    else if ( IsIsotropic4ShellSpherical () ) {

        seltissues.Set ( BrainIndex         );
        seltissues.Set ( CsfIndex           );
        seltissues.Set ( SkullIndex         );
        seltissues.Set ( ScalpIndex         );
        }

    else if ( IsIsotropic6ShellSpherical () ) {

        seltissues.Set ( BrainIndex         );
        seltissues.Set ( CsfIndex           );
        seltissues.Set ( SkullIndex         );  // using the whole skull..
        seltissues.Set ( SkullSpongyIndex   );  // ..plus its inner spongy part, which will split the skull into 3 parts, hence a total of 6 layers
        seltissues.Set ( ScalpIndex         );
        }

    } // IsHuman

/*
else if ( IsMacaque () ) {
                                        // Currently Ary 3-Shell
    seltissues.Set ( BrainIndex         );
    seltissues.Set ( SkullIndex         );
    seltissues.Set ( ScalpIndex         );
    }


else if ( IsMouse () ) {
                                        // Currently Ary 3-Shell - !Could use a 2 layers model instead, or skull / CSF / brain!
    seltissues.Set ( BrainIndex         );
    seltissues.Set ( SkullIndex         );
    seltissues.Set ( ScalpIndex         );
    }
*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Nearest neighbors interpolation
/*void    InterpolateLeadFieldNN ( Matrix& K, TPoints& inputsolpoint, int nnsize, int nnpower, TPoints& outputsolpoint )
{
TSuperGauge         gauge;

gauge.Set ( "Lead Field Interpolation" );

gauge.AddPart ( 0, (int) outputsolpoint, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get all dimensions
int                 numel           = K.Nrows ();
//int                 numsolpsrc3     = K.Ncols ();
//int                 numsolpsrc      = numsolpsrc3 / 3;
int                 numsolptrg      = (int) outputsolpoint;
int                 numsolptrg3     = numsolptrg  * 3;

                                        // allocate new downsampled matrix
Matrix              Ktrg ( numel, numsolptrg3 );


TArray2< double >   dist ( (int) inputsolpoint, 2 );
double              step            = inputsolpoint.GetMedianDistance ();
int                 ini;
double              sumx;
double              sumy;
double              sumz;
double              sumw;
//TEasyStats          statd ( (int) outputsolpoint );
//TFileName           buff;


                                        // scan each target point
for ( int outi = 0; outi < (int) outputsolpoint; outi++ ) {

    gauge.Next ( 0 );

                                        // compute all relative distances to target point
    for ( int ini = 0; ini < (int) inputsolpoint; ini++ ) {
        dist ( ini, 0 )     = ( *inputsolpoint[ ini ] - *outputsolpoint[ outi ] ).Norm2 ();
        dist ( ini, 1 )     = ini;
        }

                                        // sort by distances
    dist.Sort ( Ascending, 0 );

//    statd.Add ( dist ( 0, 0 ) );

                                        // special case: landing right onto a point?
    if ( dist ( 0, 0 ) == 0 ) {
                                        // get index of source point
        ini      = dist ( 0, 1 );
                                        // then simply copy that single point values
        for ( int el = 0; el < numel; el++ ) {
            Ktrg[ el ][ 3 * outi     ]  = K[ el ][ 3 * ini     ];
            Ktrg[ el ][ 3 * outi + 1 ]  = K[ el ][ 3 * ini + 1 ];
            Ktrg[ el ][ 3 * outi + 2 ]  = K[ el ][ 3 * ini + 2 ];
            }

        } // dist == 0

    else { // dist != 0

        sumw        = 0;
                                        // weights are inversely proportional to distance
        for ( int nni = 0; nni < nnsize; nni++ ) {
            dist ( nni, 0 )     = 1 / Power ( sqrt ( dist ( nni, 0 ) ) / step, nnpower );
            sumw               += dist ( nni, 0 );
            }

//#if defined(CHECKASSERT)
//      assert ( sumw != 0 );
//#endif

                                        // compute interpolated values
        for ( int el = 0; el < numel; el++ ) {

            sumx    = sumy  = sumz  = 0;

            for ( int nni = 0; nni < nnsize; nni++ ) {
                ini         = dist ( nni, 1 );

                sumx       += K[ el ][ 3 * ini     ] * dist ( nni, 0 );
                sumy       += K[ el ][ 3 * ini + 1 ] * dist ( nni, 0 );
                sumz       += K[ el ][ 3 * ini + 2 ] * dist ( nni, 0 );
                } // for nni

            Ktrg[ el ][ 3 * outi     ]  = sumx / sumw;
            Ktrg[ el ][ 3 * outi + 1 ]  = sumy / sumw;
            Ktrg[ el ][ 3 * outi + 2 ]  = sumz / sumw;
            } // for el

        } // dist != 0

    } // for outi

/*
statd.Sort ( true );
statd.Show ( "Min Distances" );

TVector<double>     curve;
statd.ComputeHistogram ( 0, 0, 0, 3, 3, true, curve );

TFileName           _file;
StringCopy ( _file, "E:\\Data\\NN Shortest Distance Histo.ep" );
curve.WriteFile ( _file );
* /

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // return downsampled version
K   = Ktrg;

}
*/

//----------------------------------------------------------------------------
                                        // Linear interpolation between 2 vectors (could be any dimensions BTW)
                                        // from v1 (t=0) to v2 (t=1)
TVector3Float   Interpolate2Vectors (   TVector3Float   v1, TVector3Float   v2, 
                                        double          t
                                    )
{
                                        // each dimension is interpolated independently
                                        // it gives the same results as the more complex way below, with less side-effects...
return  ( v1 * ( 1 - t ) ) + ( v2 * t );

/*
                                        // each vector weight
double              t1              = 1 - t;
double              t2              = t;
                                        // each vector norm
double              n1              = v1.Norm ();
double              n2              = v2.Norm ();


if      ( n1 == 0 && n2 == 0 ) {        // both null vectors, returns null vector
    r.Reset ();
    return;
    }
else if ( n1 == 0 ) {                   // 1 null vector, simply interpolate the remaining one linearly
    r       = v2 * t2;
    return;
    }
else if ( n2 == 0 ) {                   // 1 null vector, simply interpolate the remaining one linearly
    r       = v1 * t1;
    return;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here no null vectors
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // normalize vectors
v1     /= n1;
v2     /= n2;

                                        // interpolate angular position
double              cosomega        = Clip ( v1.ScalarProduct ( v2 ), -1.0, 1.0 );

if      ( cosomega == 1.0 ) {           // aligned, same direction, boils down to each component interpolation
    r       = v1 * t1 + v2 * t2;
    return;
    }
else if ( cosomega == -1.0 ) {          // aligned, opposite direction
#if defined(CHECKASSERT)
    assert ( cosomega == -1 );
#endif
    r       = v1 * t1 + v2 * t2;        // !wrong formula - actually, no correct formula exists!
    return;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here: not in an alignment-degenerated case
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // interpolate norm
double              n               = n1 * t1 + n2 * t2;

double              omega           = acos ( cosomega );
double              sinomega        = n / sin ( omega );    // sin != 0 - also include norm rescaling here
                                        // interpolate angles
r       = v1 * ( sin ( t1 * omega ) * sinomega ) + v2 * ( sin ( t2 * omega ) * sinomega );
*/
}

                                        // interpolate between 8 corners
TVector3Float   Interpolate8Vectors (   TVector3Float   v000,   TVector3Float   v001,   TVector3Float   v010,   TVector3Float   v011,
                                        TVector3Float   v100,   TVector3Float   v101,   TVector3Float   v110,   TVector3Float   v111,
                                        double          t,      double          u,      double          v
                                    )
{
                                        // do a succession of linear interpolations
TPointFloat         v00             = Interpolate2Vectors ( v000, v001, t );
TPointFloat         v01             = Interpolate2Vectors ( v010, v011, t );
TPointFloat         v10             = Interpolate2Vectors ( v100, v101, t );
TPointFloat         v11             = Interpolate2Vectors ( v110, v111, t );

TPointFloat         v0              = Interpolate2Vectors ( v00,  v01,  u );
TPointFloat         v1              = Interpolate2Vectors ( v10,  v11,  u );

return                                Interpolate2Vectors ( v0,   v1,   v );
}


//----------------------------------------------------------------------------
                                        // can update spsrejected
void    InterpolateLeadFieldLinear  (   AMatrix&        K,                  const TPoints&          inputsolpoint,  
                                        TPoints&        outputsolpoint,     TSelection&             spsrejected     )
{
TSuperGauge         gauge;

gauge.Set ( "Lead Field Interpolation" );

gauge.AddPart ( 0, (int) outputsolpoint, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get all dimensions
int                 numel           = K.n_rows;
//int               numsolpsrc3     = K.n_cols;
//int               numsolpsrc      = numsolpsrc3 / 3;
int                 numsolptrg      = (int) outputsolpoint;
int                 numsolptrg3     = numsolptrg  * 3;

                                        // allocate new downsampled matrix
AMatrix             Ktrg            = AMatrixZero ( numel, numsolptrg3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert the grid-aligned points to a volume
TBoundingBox<double>    spbounding ( inputsolpoint );
double                  step            = inputsolpoint.GetMedianDistance ();

TMatrix44           sptovol;
sptovol.Translate ( - ( spbounding.XMin () - step ),    - ( spbounding.YMin () - step ),    - ( spbounding.ZMin () - step ),    MultiplyLeft ); // add 1 step on each side, for the truncation to work correctly
sptovol.Scale     ( 1 / step,                           1 / step,                           1 / step,                           MultiplyLeft );

TVector3Int         size      ( spbounding.GetXExtent () / step + 3, spbounding.GetYExtent () / step + 3, spbounding.GetZExtent () / step + 3 );
TVolume<int>        spvol     ( size.X, size.Y, size.Z );
//Volume            spvolout  ( size.X, size.Y, size.Z );


OmpParallelFor

for ( int ini = 0; ini < (int) inputsolpoint; ini++ ) {

    TPointDouble        p ( inputsolpoint[ ini ] );

    sptovol.Apply ( p );

    p      += 1e-3;                     // points are on a grid, get rid of transformation rounding errors to land nicely on a grid

    p.Truncate ();

    spvol ( p ) = ini + 1;
//    spvolout ( p ) = ( (double) ini / (int) inputsolpoint * 254 ) + 1;
    }


//spvol.WriteFile ( "E:\\Data\\LF spvol.hdr" );
//spvolout.WriteFile ( "E:\\Data\\LF spvol.hdr" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 index [ 8 ];
TPointFloat         v000;
TPointFloat         v001;
TPointFloat         v010;
TPointFloat         v011;
TPointFloat         v100;
TPointFloat         v101;
TPointFloat         v110;
TPointFloat         v111;


//TEasyStats          statd ( (int) outputsolpoint );
//TFileName           buff;

                                        // scan each target point
for ( int outi = 0; outi < (int) outputsolpoint; outi++ ) {

    gauge.Next ( 0 );

                                        // caller wants to ignore this point, let the LF to 0
    if ( spsrejected[ outi ] )
        continue;

                                        // get point
    TPointDouble        p ( outputsolpoint[ outi ] );
                                        // transform to volume
    sptovol.Apply ( p );

    p      += 1e-3;                     // points are on a grid, get rid of transformation rounding errors to land nicely on a grid

//  p.Truncate ();                      // don't: we need the fraction for the interpolation

                                        // get fractions from point to corners
    double          fx          = Fraction ( p.X );
    double          fy          = Fraction ( p.Y );
    double          fz          = Fraction ( p.Z );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case: spot on a point, doesn't need 8 neighbors / interpolation
    if ( ( fx + fy + fz ) <= 5e-3 ) {

        if ( spvol.GetValueChecked ( p.X,     p.Y,     p.Z     ) == 0 ) {

            spsrejected.Set ( outi );   // shouldn't happen, but let's update the rejected points
            continue;                   // let the LF to 0
            } // neighbor not OK

                                        // get single index
        index[ 0 ]  = spvol.GetValueChecked ( p.X    , p.Y    , p.Z     ) - 1;

        for ( int el = 0; el < numel; el++ ) {
            Ktrg ( el, 3 * outi     )   = K ( el, 3 * index[ 0 ]     );
            Ktrg ( el, 3 * outi + 1 )   = K ( el, 3 * index[ 0 ] + 1 );
            Ktrg ( el, 3 * outi + 2 )   = K ( el, 3 * index[ 0 ] + 2 );
            } // for el

        continue;
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, not exactly on a point

                                        // check all corners have neighbors, for a later trilinear interpolation
    if ( ! ( spvol.GetValueChecked ( p.X,     p.Y,     p.Z     ) != 0
          && spvol.GetValueChecked ( p.X + 1, p.Y,     p.Z     ) != 0
          && spvol.GetValueChecked ( p.X,     p.Y + 1, p.Z     ) != 0
          && spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z     ) != 0
          && spvol.GetValueChecked ( p.X,     p.Y,     p.Z + 1 ) != 0
          && spvol.GetValueChecked ( p.X + 1, p.Y,     p.Z + 1 ) != 0
          && spvol.GetValueChecked ( p.X,     p.Y + 1, p.Z + 1 ) != 0
          && spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z + 1 ) != 0 ) ) {

        spsrejected.Set ( outi );       // in case SP without enough neighbors, default is to just clear up the LF at this position - update the rejected points, too
        continue;                       // let the LF to 0
        } // neighbors not OK

                                        // get the 8 corners indexes
    index[ 0 ]  = spvol.GetValueChecked ( p.X    , p.Y    , p.Z     ) - 1;
    index[ 1 ]  = spvol.GetValueChecked ( p.X + 1, p.Y    , p.Z     ) - 1;
    index[ 2 ]  = spvol.GetValueChecked ( p.X    , p.Y + 1, p.Z     ) - 1;
    index[ 3 ]  = spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z     ) - 1;
    index[ 4 ]  = spvol.GetValueChecked ( p.X    , p.Y    , p.Z + 1 ) - 1;
    index[ 5 ]  = spvol.GetValueChecked ( p.X + 1, p.Y    , p.Z + 1 ) - 1;
    index[ 6 ]  = spvol.GetValueChecked ( p.X    , p.Y + 1, p.Z + 1 ) - 1;
    index[ 7 ]  = spvol.GetValueChecked ( p.X + 1, p.Y + 1, p.Z + 1 ) - 1;

                                        // interpolate
    for ( int el = 0; el < numel; el++ ) {
                                        // get the 8 vectors at corners, for current electrode
        v000.Set ( K ( el, 3 * index[ 0 ]     ), K ( el, 3 * index[ 0 ] + 1 ), K ( el, 3 * index[ 0 ] + 2 ) );
        v001.Set ( K ( el, 3 * index[ 1 ]     ), K ( el, 3 * index[ 1 ] + 1 ), K ( el, 3 * index[ 1 ] + 2 ) );
        v010.Set ( K ( el, 3 * index[ 2 ]     ), K ( el, 3 * index[ 2 ] + 1 ), K ( el, 3 * index[ 2 ] + 2 ) );
        v011.Set ( K ( el, 3 * index[ 3 ]     ), K ( el, 3 * index[ 3 ] + 1 ), K ( el, 3 * index[ 3 ] + 2 ) );
        v100.Set ( K ( el, 3 * index[ 4 ]     ), K ( el, 3 * index[ 4 ] + 1 ), K ( el, 3 * index[ 4 ] + 2 ) );
        v101.Set ( K ( el, 3 * index[ 5 ]     ), K ( el, 3 * index[ 5 ] + 1 ), K ( el, 3 * index[ 5 ] + 2 ) );
        v110.Set ( K ( el, 3 * index[ 6 ]     ), K ( el, 3 * index[ 6 ] + 1 ), K ( el, 3 * index[ 6 ] + 2 ) );
        v111.Set ( K ( el, 3 * index[ 7 ]     ), K ( el, 3 * index[ 7 ] + 1 ), K ( el, 3 * index[ 7 ] + 2 ) );

                                        // tri-linear interpolation
        TPointFloat         r   = Interpolate8Vectors ( v000,   v001,   v010,   v011, 
                                                        v100,   v101,   v110,   v111, 
                                                        fx,     fy,     fz
                                                      );
        Ktrg ( el, 3 * outi     )   = r.X;
        Ktrg ( el, 3 * outi + 1 )   = r.Y;
        Ktrg ( el, 3 * outi + 2 )   = r.Z;
        } // for el

    } // for outi

/*
statd.Show ( "Min Distances" );

TVector<double>     curve;
statd.ComputeHistogram ( 0, 0, 0, 3, 3, true, curve );

TFileName           _file;
StringCopy ( _file, "E:\\Data\\NN Shortest Distance Histo.ep" );
curve.WriteFile ( _file );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // return downsampled version
K   = Ktrg;
}


//----------------------------------------------------------------------------
void    RejectPointsFromLeadField   (   AMatrix&        K,  const TSelection&   spsrejected     )
{
                                        // nothing to do?
if ( ! (bool) spsrejected )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get all dimensions
int                 numel           = K.n_rows;
int                 numsolpsrc3     = K.n_cols;
int                 numsolpsrc      = numsolpsrc3 / 3;
int                 numsolptrg      = numsolpsrc - (int) spsrejected;
int                 numsolptrg3     = numsolptrg  * 3;

//DBGV3 ( numsolpsrc, spsrejected.Size (), (int) spsrejected, "RejectPointsFromLeadField: numsolpsrc, spsrejected.Size (), (int) spsrejected" );

                                        // allocate new downsampled matrix
AMatrix             Kdown           = AMatrixZero ( numel, numsolptrg3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TSuperGauge         gauge;

gauge.Set ( "Lead Field Downsampling" );

gauge.AddPart ( 0, numsolpsrc, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 sp2             = 0;


for ( int i = 0; i < numsolpsrc /*spsrejected.Size ()*/; i++ ) {

    gauge.Next ( 0 );

    if ( spsrejected[ i ] )
        continue;

    for ( int el = 0; el < numel; el++ ) {
        Kdown ( el, 3 * sp2     )  = K ( el, 3 * i     );
        Kdown ( el, 3 * sp2 + 1 )  = K ( el, 3 * i + 1 );
        Kdown ( el, 3 * sp2 + 2 )  = K ( el, 3 * i + 2 );
        }

    sp2++;
    }

                                        // overwrite with downsampled data
K           = Kdown;
}


//----------------------------------------------------------------------------
                                        // A fully null column or a single NaN in a column will flag said column as rejected
void    CheckNullLeadField  (   const AMatrix&      K,  TSelection&     spsrejected     )
{
                                        // scan for null LF's SP -> update spsrejected
int                 numel           = K.n_rows;
int                 numsolp3        = K.n_cols;
int                 numsolp         = numsolp3 / 3;
int                 el;


for ( int sp = 0; sp < numsolp; sp++ ) {

//  if ( spsrejected[ sp ] )            // only test for null columns, don't omit already skipped SPs
//      continue;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( el = 0; el < numel; el++ )

        if ( K ( el, 3 * sp     ) != 0
          || K ( el, 3 * sp + 1 ) != 0
          || K ( el, 3 * sp + 2 ) != 0 )

            break;

                                        // full column is 0?
    if ( el == numel ) {
        spsrejected.Set ( sp );

        continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // NaN test should be done separately
    for ( el = 0; el < numel; el++ )
                                        // at least one element is NaN?
        if ( IsNotAProperNumber ( K ( el, 3 * sp     ) ) 
          || IsNotAProperNumber ( K ( el, 3 * sp + 1 ) ) 
          || IsNotAProperNumber ( K ( el, 3 * sp + 2 ) ) ) {

            spsrejected.Set ( sp );

            break;
            }
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // General formula, for any skull radius and relative conductivity
                                        // Adjust the radius of the position of the dipole
double      AryR3ToR1   (   double  radius3, 
                            double  Xi,         double  innerskullradius,   double  outerskullradius 
                        )
{
//Mined ( radius3, innerskullradius );


double              radialradius1       = RadialR3ToR1     ( radius3, Xi, innerskullradius, outerskullradius );
double              tangentialradius1   = TangentialR3ToR1 ( radius3, Xi, innerskullradius, outerskullradius );

                                        // approximation: computing the average of the radial and the tangential R3 to R1
double              radius3toradius1    = radius3 > 0 ? ( radialradius1 + tangentialradius1 ) / 2
                                                        / radius3
                                                      : 1;

return  radius3toradius1;
}

                                        // Adjust the moment ("intensity") of the dipole itself
double      AryM3ToM1   (   double  radius3, 
                            double  Xi,         double  innerskullradius,   double  outerskullradius 
                        )
{
//Mined ( radius3, innerskullradius );


double              radialradius1       = RadialR3ToR1     (                    radius3, Xi, innerskullradius, outerskullradius );
double              tangentialradius1   = TangentialR3ToR1 (                    radius3, Xi, innerskullradius, outerskullradius );


double              radialmoment1       = RadialM3ToM1     ( radialradius1,     radius3, Xi, innerskullradius, outerskullradius );
double              tangentialmoment1   = TangentialM3ToM1 ( tangentialradius1, radius3, Xi, innerskullradius, outerskullradius );

                                        // approximation: computing the average of the radial and the tangential M3 to M1
double              moment3tomoment1    = ( radialmoment1 + tangentialmoment1 ) / 2;


return  moment3tomoment1;
}


//----------------------------------------------------------------------------
                                        // More specific formula, which weights its results by radial and tangential contributions
                                        // Input dipole & electrode positions are already normalized, potential is therefor for a normalized sphere
                                        // Returns the updated dipole
double      PotentialIsotropic3ShellApproxSphericalAry (
                                            TDipole&                dipole,     PotentialFlags          flags,
                                            const TPointFloat&      electrodepos,
                                            const TArray1<double>&  R,          const TArray1<double>&  sigma
                                        )
{
                                        // conductivity ratio between skull and {brain/scalp} (identical in this model)
double              Xi                  = sigma[ 1 ] / sigma[ 0 ];
double              innerskullradius    = R    [ 0 ];
double              outerskullradius    = R    [ 1 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( flags == ComputeLeadField )
                                        // direction not provided, compute it toward electrode
    dipole.SetDirection ( electrodepos );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // original radius of dipole's position
double              radius3             = dipole.Position.Norm ();

                                        // Early test for degenerate case of dipole position exactly set at the center
if ( radius3 < SingleFloatEpsilon ) {
                                        // shift an epsilon toward electrode
    dipole.Position     = electrodepos * ( SingleFloatEpsilon / electrodepos.Norm () );

    radius3             = dipole.Position.Norm ();
    }
                                        // In case the spherization radius was the electrode itself, there could be some solution
                                        // points that are further (in radius) that this. We can clip the radius to some safe
                                        // limit in that case.
                                        // If the spherization has been done with the solution point itself, there is no problem.
//Mined ( radius3, innerskullradius );


double              radialradius1       = RadialR3ToR1     (                    radius3, Xi, innerskullradius, outerskullradius );
double              tangentialradius1   = TangentialR3ToR1 (                    radius3, Xi, innerskullradius, outerskullradius );


double              radialmoment1       = RadialM3ToM1     ( radialradius1,     radius3, Xi, innerskullradius, outerskullradius );
double              tangentialmoment1   = TangentialM3ToM1 ( tangentialradius1, radius3, Xi, innerskullradius, outerskullradius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute radial and tangential contribution of current dipole

                                        // how radial is the dipole itself, compared to the projected surface point (NOT to the electrode)
double              radialw             = Square ( dipole.Position.Cosine ( dipole.Direction ) );

                                        // Handling the case where electrode is perfectly aligned to the dipole's position, which then causes errors
                                        // Just removing an epsilon seems to put the train back on tracks
if ( RelativeDifference ( fabs ( radialw ), 1 ) < SingleFloatEpsilon )

    radialw     = 1 - DoubleFloatEpsilon;


double              tangentialw         = 1 - radialw;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mixing the radial and tangential corrections, according to dipole proportions
                                        // Note that they are very close to each other, so the mixing is not really a big problem
double              radius3toradius1;
double              moment3tomoment1;

                                        // weighted sum of the radial and the tangential contributions
radius3toradius1    = radius3 > 0 ? ( radialw     * radialradius1
                                    + tangentialw * tangentialradius1 )
                                    / radius3
                                  : 0;

moment3tomoment1    =   radialw     * radialmoment1
                      + tangentialw * tangentialmoment1;


                                      // simple average - the old way
//radius3toradius1  = radius3 > 0 ? ( radialradius1 
//                                  + tangentialradius1 ) / 2
//                                  / radius3
//                                : 0;
//
//moment3tomoment1  = ( radialmoment1
//                    + tangentialmoment1 ) / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Updating the equivalent 1 shell dipole:

                                        // - Deep shifted position of the 1 shell equivalent dipole
dipole.Position    *= radius3toradius1;


double              UI;


if ( flags == ComputeLeadField ) {
                                        // - New position -> new direction, we want to evaluate the potential exactly toward the electrode
    dipole.SetDirection ( electrodepos );

                                        // - Compute (positive) voltage of dipole POINTING TOWARD ELECTRODE, now using only {brain/scalp} conductivity
    UI      = PotentialIsotropic1ShellExactSphericalVector ( dipole, electrodepos, sigma[ 0 ] )
                                        // - Adjust the moment of the dipole - not sure about that, though, but results look somehow better with the correction
            * moment3tomoment1;

                                        // - Rescale dipole with obtained potential
    dipole.Direction   *= UI;
    }

else
                                        // - Compute voltage from any dipole
    UI      = PotentialIsotropic1ShellExactSphericalVector ( dipole, electrodepos, sigma[ 0 ] )
                                        // - Adjust the moment of the dipole - not sure about that, though, but results look somehow better with the correction
            * moment3tomoment1;
    

return  UI;
}


//----------------------------------------------------------------------------
                                        // Simplified Legendre terms, as most of them cancel out
                                        // Ary - Equation (3a)
long double     AryFn   (   double  n,  double  Xi,     double  innerskullradius,   double  outerskullradius    )
{
long double         dn;
long double         fn;


dn          =   ( ( n + 1 ) * Xi + n 
                )
              * (   ( n * Xi / ( n + 1 ) + 1 )
                  + ( 1 - Xi ) * ( powl ( innerskullradius, 2 * n + 1 ) - powl ( outerskullradius, 2 * n + 1 ) )
                )
            - n * Square ( 1 - Xi ) * powl ( innerskullradius / outerskullradius, 2 * n + 1 );


fn          = Xi * Square ( 2 * n + 1 ) / ( dn * ( n + 1 ) );

return  fn;
}


//----------------------------------------------------------------------------
                                        // Error between the radial components of a dipole from 3-Shell to equivalent 1 shell
                                        // Equation (9), Xi being the ratio of conductivities between layer 2 and layers 1 & 3
double      RadialRho   (   double  radius1,    double  radius3,
                            double  Xi,         double  innerskullradius,   double  outerskullradius 
                        )
{
                                        // formula is not friendly with 0 radii
if ( radius1 == 0 || radius3 == 0 )
    return  0;

                                        // lowest radius3 is pretty instable, we can interpolate from a safer low limit instead
if ( radius3 < Shell3to1LowestRadius3 )

    return  RadialRho ( radius1 * Shell3to1LowestRadius3 / radius3, Shell3to1LowestRadius3,
                        Xi, innerskullradius, outerskullradius );


long double         mubb            = 0;
long double         mubF            = 0;
//long double       muFF            = 0;
long double         factor;

                                        // higher radii need more polynomials
int                 numlegendre     = NumLegendreTermsAryMin + Clip ( radius3 / innerskullradius, 0.0, 1.0 ) * ( NumLegendreTermsAryMax - NumLegendreTermsAryMin );

for ( int i = 1; i <= numlegendre; i++ ) {

    factor  = ( 2 * i + 1.0 );

    mubb   += factor * powl ( radius1, 2 * i - 2.0 );

    mubF   += factor * powl  ( radius1,     i - 1.0 ) 
                     * powl  ( radius3,     i - 1.0 ) 
                     * AryFn ( i, Xi, innerskullradius, outerskullradius );

//  muFF   += factor * powl ( radius3, 2 * i - 2.0 )
//                   * Square ( AryFn ( i, Xi, innerskullradius, outerskullradius ) );
    }

                                        // muFF is constant for a given radius3, no need to compute it for minimization
                                        // also moment can be taken out of the equation
double              rho             = /*muFF*/ - Square ( mubF ) / mubb;

return  rho;
}
                                        // Error between the tangential components of a dipole from 3-Shell to equivalent 1 shell
                                        // Equation (16)
double      TangentialRho   (   double  radius1,    double  radius3,
                                double  Xi,         double  innerskullradius,   double  outerskullradius 
                            )
{
                                        // formula is not friendly with 0 radii
if ( radius1 == 0 || radius3 == 0 )
    return  0;

                                        // lowest radius3 is pretty instable, we can interpolate from a safer low limit instead
if ( radius3 < Shell3to1LowestRadius3 )

    return  TangentialRho ( radius1 * Shell3to1LowestRadius3 / radius3, Shell3to1LowestRadius3,
                            Xi, innerskullradius, outerskullradius );


long double         mubb            = 0;
long double         mubF            = 0;
//long double       muFF            = 0;
long double         factor;

                                        // higher radii need more polynomials
int                 numlegendre     = NumLegendreTermsAryMin + Clip ( radius3 / innerskullradius, 0.0, 1.0 ) * ( NumLegendreTermsAryMax - NumLegendreTermsAryMin );

for ( int i = 1; i <= numlegendre; i++ ) {

    factor  = ( 2 * i + 1.0 ) * ( i + 1 ) / i;

    mubb   += factor * powl ( radius1, 2 * i - 2.0 );

    mubF   += factor * powl  ( radius1,     i - 1.0 ) 
                     * powl  ( radius3,     i - 1.0 ) 
                     * AryFn ( i, Xi, innerskullradius, outerskullradius );

//  muFF   += factor * powl ( radius3, 2 * i - 2.0 )
//                   * Square ( AryFn ( i, Xi, innerskullradius, outerskullradius ) );
    }

                                        // muFF is constant for a given radius3, no need to compute it for minimization
                                        // also moment can be taken out of the equation
double              rho             = /*muFF*/ - Square ( mubF ) / mubb;

return  rho;
}


//----------------------------------------------------------------------------
                                        // There is no analytic solution, it needs some iterative search to minimimize the error Rho
double      RadialR3ToR1    (   double  radius3,
                                double  Xi,         double  innerskullradius,   double  outerskullradius 
                            )
{
if ( radius3 == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for robust starting point, linearly downward from radius3
double              radius1         = radius3;
double              step            = radius3 * Shell3to1StepInit;
double              radialrhoold;
double              radialrho;


radialrho       = RadialRho ( radius1, radius3, Xi, innerskullradius, outerskullradius );

do {
    radialrhoold    = radialrho;

    radius1        -= step;
                                        // evaluate on the left and on the right
    radialrho       = RadialRho ( radius1, radius3, Xi, innerskullradius, outerskullradius );

    } while ( radialrho < radialrhoold && radius1 >= 0 );

                                        // center back to the closest absolute min
radius1     = Clip ( radius1 + step, 0.0, radius3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now do a dichotomic search, starting from the best estimate
double              radialrhol;
double              radialrhor;

step   /= 2;
                                        
do {
                                        // evaluate on the left and on the right
    radialrhol      = RadialRho ( radius1 - step, radius3, Xi, innerskullradius, outerskullradius );
    radialrhor      = RadialRho ( radius1 + step, radius3, Xi, innerskullradius, outerskullradius );

                                        // choose which direction is minimum
    if ( radialrhol < radialrhor )  radius1    -= step;
    else                            radius1    += step;

    step   /= 2;

    } while ( step > Shell3to1Convergence );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  radius1;
}

                                        // Equation (8)
double      RadialM3ToM1    (   double      radius1,    double  radius3,
                                double      Xi,         double  innerskullradius,   double  outerskullradius 
                            )
{
if ( radius3 == 0 )
    radius1 = radius3 = 1e-10;


long double         mubb            = 0;
long double         mubF            = 0;
long double         factor;

                                        // higher radii need more polynomials
int                 numlegendre     = NumLegendreTermsAryMin + Clip ( radius3 / innerskullradius, 0.0, 1.0 ) * ( NumLegendreTermsAryMax - NumLegendreTermsAryMin );

for ( int i = 1; i <= numlegendre; i++ ) {

    factor  = ( 2 * i + 1.0 );

    mubb   += factor * powl ( radius1, 2 * i - 2.0 );

    mubF   += factor * powl  ( radius1,     i - 1.0 ) 
                     * powl  ( radius3,     i - 1.0 ) 
                     * AryFn ( i, Xi, innerskullradius, outerskullradius );
    }


double              m3              = mubF / mubb;

return  m3;
}


//----------------------------------------------------------------------------
                                        // There is no analytic solution, it needs some iterative search to minimimize the error Rho
double      TangentialR3ToR1    (   double  radius3,
                                    double  Xi,         double  innerskullradius,   double  outerskullradius 
                                )
{
if ( radius3 == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for robust starting point, linearly downward from radius3
double              radius1         = radius3;
double              step            = radius3 * Shell3to1StepInit;
double              tangentialrhoold;
double              tangentialrho;


tangentialrho   = TangentialRho ( radius1, radius3, Xi, innerskullradius, outerskullradius );

do {
    tangentialrhoold= tangentialrho;

    radius1        -= step;
                                        // evaluate on the left and on the right
    tangentialrho   = TangentialRho ( radius1, radius3, Xi, innerskullradius, outerskullradius );

    } while ( tangentialrho < tangentialrhoold && radius1 >= 0 );

                                        // center back to the closest absolute min
radius1     = Clip ( radius1 + step, 0.0, radius3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now do a dichotomic search, starting from the best estimate
double              tangentialrhol;
double              tangentialrhor;

step   /= 2;


do {
                                        // evaluate on the left and on the right
    tangentialrhol  = TangentialRho ( radius1 - step, radius3, Xi, innerskullradius, outerskullradius );
    tangentialrhor  = TangentialRho ( radius1 + step, radius3, Xi, innerskullradius, outerskullradius );
                                        // choose which direction is minimum
    if ( tangentialrhol < tangentialrhor )  radius1    -= step;
    else                                    radius1    += step;

    step   /= 2;

    } while ( step > Shell3to1Convergence );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  radius1;
}

                                        // Equation (15)
double      TangentialM3ToM1    (   double  radius1,    double  radius3,
                                    double  Xi,         double  innerskullradius,   double  outerskullradius 
                                )
{
if ( radius3 == 0 )
    radius1 = radius3 = 1e-10;


long double         mubb            = 0;
long double         mubF            = 0;
long double         factor;

                                        // higher radii need more polynomials
int                 numlegendre     = NumLegendreTermsAryMin + Clip ( radius3 / innerskullradius, 0.0, 1.0 ) * ( NumLegendreTermsAryMax - NumLegendreTermsAryMin );

for ( int i = 1; i <= numlegendre; i++ ) {

    factor  = ( 2 * i + 1.0 ) * ( i + 1 ) / i;

    mubb   += factor * powl ( radius1, 2 * i - 2.0 );

    mubF   += factor * powl  ( radius1,     i - 1.0 ) 
                     * powl  ( radius3,     i - 1.0 ) 
                     * AryFn ( i, Xi, innerskullradius, outerskullradius );
    }


double              m3              = mubF / mubb;

return  m3;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Field vector - all parameters are centered on dipole origin, real coordinates are allowed (not only spherical/normalized)
                                        // Full formula with proper scaling
TVector3Float   DipoleElectricField         (
                                            const TVector3Float&    dipoledir, 
                                            const TVector3Float&    relpos, 
                                            double                  sigma
                                            )
{
const TVector3Float&r               = relpos;
double              rnorm           = r.Norm ();
TVector3Float       rhat            = r / rnorm;

return  ( rhat * ( 3 * dipoledir.ScalarProduct ( rhat ) ) - dipoledir )
        / ( FourPi * sigma * relpos.Norm3 () );
}


//----------------------------------------------------------------------------
                                        // Vectorial direct computation - Fender formula, gives identical results as the N-Shell for 1 layer
                                        // Still an approximation for Kev big enough, otherwise it needs more terms
                                        // Spherical case only, any radius (normalized or arbitrary)
double          PotentialIsotropic1ShellExactSphericalVector  (
                                            const TDipole&          dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            )
{
const TVector3Float&r               = electrodepos;
double              rnorm           = r.Norm ();
TVector3Float       rhat            = r / rnorm;

TVector3Float       d               = electrodepos - dipole.Position;
double              dnorm           = d.Norm ();
TVector3Float       dhat            = d / dnorm;

TVector3Float       rd              = ( ( rhat + dhat ) / ( rnorm * dnorm * ( 1 + rhat.ScalarProduct ( dhat ) ) )       // additional contribution - note: angle is in [0..pi/2)
                                               + dhat   * ( 2 / Square ( dnorm ) )                                );    // basically the contribution from the simpler formula

double              v               = dipole.Direction.ScalarProduct ( rd )
                                     / ( FourPi * sigma )
                                     / 2.46; // not in actual formula, used for comparison with current N-Shell implementation

return  v;
}


//----------------------------------------------------------------------------
                                        // Approximate & simpler formula for any real / spherical cases
                                        // Any geometrical case, any radius (normalized or arbitrary)
double          PotentialIsotropic1ShellApproxRealVector (
                                            const TDipole&          dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            )
{
TVector3Float       kev             = electrodepos - dipole.Position;

double              v               = dipole.Direction.ScalarProduct ( kev )
                                    / ( FourPi * sigma * kev.Norm3 () ); // + cst

                                        // Equivalent to
//double              kevnorm         = kev.Norm ();
//                    kev            /= kevnorm;
//double              v               = kev.ScalarProduct ( dipole.Direction )
//                                    / ( FourPi * sigma * Square ( kevnorm ) ); // + cst


return  v;
}


//----------------------------------------------------------------------------
/*                                        // Using Lead Field formula - not tested
double          PotentialIsotropic1ShellExactVectorialLeadField  (
                                            const TDipole&          dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            )
{
TVector3Float       r               = electrodepos - dipole.Position;

return  r.ScalarProduct ( DipoleElectricField ( dipole.Direction, r, sigma ) )
        / 1.5;  // approximate rescaling
}
*/

//----------------------------------------------------------------------------
                                        // Legendre computation - Zhang formula, simplified case for 1-Shell
                                        // Spherical case only, radius normalized
double          PotentialIsotropic1ShellExactSphericalLegendre  (
                                            TDipole&                dipole,
                                            const TPointFloat&      electrodepos,
                                            double                  sigma
                                            )
{
if ( electrodepos.IsNull () )
                                        // electrode in center(!)
    return  INFINITY;


if ( dipole.Direction.IsNull () )
                                        // dipole direction is null(!)
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              R                   = 1.0;

                                        // Dipole position is already normalized in a sphere, spradius = Ro / Re
                                        // Also not allowing solution point to be above the most inner sphere
double              spradius            = NoMore ( R, (double) dipole.Position.Norm () );

                                        // Early test for degenerate case of dipole position exactly set at the center
if ( spradius < SingleFloatEpsilon ) {
                                        // shift an epsilon toward electrode
    dipole.Position     =  electrodepos * ( 100 * SingleFloatEpsilon / electrodepos.Norm () );

    spradius            = dipole.Position.Norm ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting alpha, beta and gamma angles
                                        // Added some checks for degenerate cases which still can happen
double              cosalpha;
double              sinalpha;
double              cosbeta;
double              cosgamma;
double              singamma;

                                        // Beta is how perpendicular the dipole is relative to the electrode-center-sp plane - result in [0..Pi]
if ( dipole.Position .IsNull ()     // dipole spot on center
                                    // degenerate angles: some one of these vectors are aligned, we are therefore in the same "plane" anyway
    || dipole.Direction.IsAligned ( dipole.Position, SingleFloatEpsilon )
    || electrodepos    .IsAligned ( dipole.Position, SingleFloatEpsilon ) )

    cosbeta         = 1;            // dipole is on same plane, so beta = 0 -> cosbeta = 1

else {
                                    // !sequence matters to have the correct angle!
    TVector3Float       P1          = dipole.Direction.VectorialProduct ( dipole.Position );
    TVector3Float       P2          = electrodepos    .VectorialProduct ( dipole.Position );

    cosbeta         = P1.Cosine ( P2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Alpha is the angle between the dipole position and its direction - aka "how much radial" is dipole vector - result in [0..Pi]
if ( dipole.Position.IsNull () ) {

    cosalpha    = 1;
    sinalpha    = 0;
    }
else {
    cosalpha    = dipole.Position.Cosine ( dipole.Direction );
    sinalpha    = sqrt ( 1 - Square ( cosalpha ) );             // alpha is always in [0..Pi] so sin ( alpha ) is always the positive root >= 0
//  sinalpha    = dipole.Position.Sine   ( dipole.Direction );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Gamma is the angle between dipole position and the electrode - result in [0..Pi]
if ( dipole.Position.IsNull () ) {

    cosgamma    = 0;                    // if dipole is spot on center, we arbitrarily choose cosgamma = 0 as it simplifies the Legendre terms
    singamma    = 1;
    }
else {
    cosgamma    = dipole.Position.Cosine ( electrodepos );
    singamma    = sqrt ( 1 - Square ( cosgamma ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Handling the case where electrode is perfectly aligned to the dipole's position, which then causes the Legendre to return some NaN
                                        // Just removing an epsilon seems to put the train back on tracks
if ( RelativeDifference ( fabs ( cosgamma ), 1 ) < SingleFloatEpsilon ) {

    cosgamma    = Sign ( cosgamma ) * ( 1 - DoubleFloatEpsilon );
    singamma    = sqrt ( 1 - Square ( cosgamma ) );
    }

                                        // Same with dipole orientation and position
if ( RelativeDifference ( fabs ( cosalpha ), 1 ) < SingleFloatEpsilon ) {

    cosalpha    = Sign ( cosalpha ) * ( 1 - DoubleFloatEpsilon );
    sinalpha    = sqrt ( 1 - Square ( cosalpha ) );
    }


//if ( RelativeDifference ( fabs ( cosbeta ), 1 ) < SingleFloatEpsilon )
//
//    cosbeta     = Sign ( cosbeta ) * ( 1 - DoubleFloatEpsilon );


//if ( spradius < SingleFloatEpsilon )
//
//    spradius    = DoubleFloatEpsilon;
                                        // dipole on the exact edge seems "OK", but of course it needs a lot more polynomials to converge
//if ( spradius > 1 - SingleFloatEpsilon )
//
//    spradius    = 1 - SingleFloatEpsilon;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Equ. (1H') from Zhang

                                        // Splitting dipole into Radial and Tangential components
double              Dr              = dipole.Direction.Norm () * cosalpha;
double              Dt              = dipole.Direction.Norm () * sinalpha;

                                        // Distance between electrode and dipole
double              L               = ( electrodepos - dipole.Position ).Norm ();
//double            L               = sqrt ( Square ( R ) + Square ( spradius ) - 2 * R * spradius * cosgamma );    // also
double              L3              = Cube ( L );

                                        // Radial voltage contribution
double              Ur              = Dr 
                                    * ( 2 * ( R * cosgamma - spradius ) / L3 + ( 1 / ( spradius * L ) - 1 / ( spradius * R ) ) );

                                        // Tangential voltage contribution
double              Ut              = Dt * cosbeta * singamma 
                                    * ( 2 * R / L3 + ( L + R ) / ( L * R * ( R - spradius * cosgamma + L ) ) );

                                        // Total voltage
double              UI              = ( Ur + Ut ) / ( FourPi * sigma );

return  UI;
}


//----------------------------------------------------------------------------
                                        // N Layers Isotropic Sphere potential exact equations (Zhang, equation 1I & 2I p.340; also Nunez Srinivasan book)
                                        // Input dipole & electrode positions are already normalized, potential is therefor for a normalized sphere
                                        // !sigma and R size is numlayers, so indexes range is [0..numlayers-1], NOT [1..numlayers] as per the article!
                                        // !formula for cos beta is wrong in the article!
                                        // Radii R are relative to the last sphere, R[ numlayers - 1 ] = 1
                                        // Position is not modified, only the direction & power is computed

//#define         DebugPotentials

#if defined (DebugPotentials)
TArray2<float>  potiso;
int             potisosp;
#endif
                                        // was 150 for 4-Shell, raised to 300 for 6-Shell by safety
#define     PotentialIsotropicNLayersMaxTerms       300
#define     PotentialIsotropicNLayersError          1e-6


double          PotentialIsotropicNShellExactSphericalLegendre  (   
                                                TDipole&                dipole,     PotentialFlags          flags,
                                                const TPointFloat&      electrodepos,
                                                const TArray1<double>&  R,          const TArray1<double>&  sigma,
                                                int                     maxterms,   double                  convergence
                                                )
{
if ( electrodepos.IsNull () )
                                        // electrode in center(!)
    return  INFINITY;


if ( dipole.Direction.IsNull () && flags != ComputeLeadField )
                                        // dipole direction is null(!)
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numlayers           = R.GetDim ();

                                        // Dipole position is already normalized in a sphere, spradius = Ro / Re
                                        // Also not allowing solution point to be above the most inner sphere
double              spradius            = NoMore ( R[ 0 ], (double) dipole.Position.Norm () );

                                        // Early test for degenerate case of dipole position exactly set at the center
if ( spradius < SingleFloatEpsilon ) {
                                        // shift an epsilon toward electrode
    dipole.Position     =  electrodepos * ( /*100 * */ SingleFloatEpsilon / electrodepos.Norm () );

    spradius            = dipole.Position.Norm ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting alpha, beta and gamma angles
                                        // Added some checks for degenerate cases which still can happen
double              cosalpha;
double              sinalpha;
double              cosbeta;
double              cosgamma;

                                        // Beta is how perpendicular the dipole is relative to the electrode-center-sp plane - result in [0..Pi]
if ( flags == ComputeLeadField ) {

    dipole.SetDirection ( electrodepos );
                                        // dipole vector is within the dipole position + electrode plane
    cosbeta             = 1;
    }
else { // ComputePotentials             // dipole direction provided

    if ( dipole.Position .IsNull ()     // dipole spot on center
                                        // degenerate angles: some one of these vectors are aligned, we are therefore in the same "plane" anyway
      || dipole.Direction.IsAligned ( dipole.Position, SingleFloatEpsilon )
      || electrodepos    .IsAligned ( dipole.Position, SingleFloatEpsilon ) )

        cosbeta         = 1;            // dipole is on same plane, so beta = 0 -> cosbeta = 1

    else {
                                        // !sequence matters to have the correct angle!
        TVector3Float       P1          = dipole.Direction.VectorialProduct ( dipole.Position );
        TVector3Float       P2          = electrodepos    .VectorialProduct ( dipole.Position );

        cosbeta         = P1.Cosine ( P2 );
        }
    } // else ComputePotentials


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Alpha is the angle between the dipole position and its direction - aka "how much radial" is dipole vector - result in [0..Pi]
if ( dipole.Position.IsNull () ) {

    cosalpha    = 1;
    sinalpha    = 0;
    }
else {
    cosalpha    = dipole.Position.Cosine ( dipole.Direction );
    sinalpha    = sqrt ( 1 - Square ( cosalpha ) );             // alpha is always in [0..Pi] so sin ( alpha ) is always the positive root >= 0
//  sinalpha    = dipole.Position.Sine   ( dipole.Direction );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Gamma is the angle between dipole position and the electrode - result in [0..Pi]
if ( dipole.Position.IsNull () )

    cosgamma    = 0;                    // if dipole is spot on center, we arbitrarily choose cosgamma = 0 as it simplifies the Legendre terms
else
    cosgamma    = dipole.Position.Cosine ( electrodepos );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Handling the case where electrode is perfectly aligned to the dipole's position, which then causes the Legendre to return some NaN
                                        // Just removing an epsilon seems to put the train back on tracks
if ( RelativeDifference ( fabs ( cosgamma ), 1 ) < SingleFloatEpsilon )

    cosgamma    = Sign ( cosgamma ) * ( 1 - DoubleFloatEpsilon );

                                        // Same with dipole orientation and position
if ( RelativeDifference ( fabs ( cosalpha ), 1 ) < SingleFloatEpsilon ) {

    cosalpha    = Sign ( cosalpha ) * ( 1 - DoubleFloatEpsilon );
    sinalpha    = sqrt ( 1 - Square ( cosalpha ) );
    }


//if ( RelativeDifference ( fabs ( cosbeta ), 1 ) < SingleFloatEpsilon )
//
//    cosbeta     = Sign ( cosbeta ) * ( 1 - DoubleFloatEpsilon );


//if ( spradius < SingleFloatEpsilon )
//
//    spradius    = DoubleFloatEpsilon;
                                        // dipole on the exact edge seems "OK", but of course it needs a lot more polynomials to converge
//if ( spradius > 1 - SingleFloatEpsilon )
//
//    spradius    = 1 - SingleFloatEpsilon;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long double         RoRe;
long double         m11, m12, m21, m22;
long double         p11, p12, p21, p22;
long double         t11, t12, t21, t22;
long double         fn;
//long double       Pn;
//long double       Pln;
long double         Pnm2,  Pnm1,  Pn;
long double         Plnm2, Plnm1, Pln;
long double         deltaUI;
long double         UI              = 0;
long double         error           = 0;


for ( int n = 1; n <= maxterms; n++ ) {

                                        // First part of equ. 1I p.340
    RoRe    = ( 2 * n + 1 ) / (long double) n
            * powl ( spradius, n - 1 ); // Ro / Re = spradius, as both electrode and SP are already in a sphere, therefor electrode radius is 1, outer SP radius is also 1, so ratio is actual SP radius


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mixing up radii and (isotropic) conductivities here (equ. 4I p.340)

                                        // init matrix to identity - also fine if numlayers == 1
    m11 = 1;    m12 = 0;
    m21 = 0;    m22 = 1;

                                        // !index shift of -1 compared to article!
    for ( int k = 0; k < numlayers - 1; k++ ) {

                                        // here we can see that only the ratio of the conductivities between 2 successive layers matters
        long double     sksk1       = sigma[ k ] / sigma[ k + 1 ];
                                        // here we can see that only the relative radii ratio to the electrode / outer shell matters
                                        // radii are already within a normalized sphere, Re = R[numlayers-1] = 1
        p11     = n        + ( n + 1 ) *   sksk1;                                           p12     =            ( n + 1 ) * ( sksk1 - 1 ) * powl ( 1 / R[ k ], 2 * n + 1 );
        p21     =              n       * ( sksk1 - 1 ) * powl (     R[ k ], 2 * n + 1 );    p22     = ( n + 1 ) +  n       *   sksk1;

                                        // copying to temp
        t11     = m11; t12     = m12; 
        t21     = m21; t22     = m22;

                                        // right multiplication   M1 x M2 x ... x Mnumlayers-1
        m11     = t11 * p11 + t12 * p21;    m12     = t11 * p12 + t12 * p22;
        m21     = t21 * p11 + t22 * p21;    m22     = t21 * p12 + t22 * p22;
        }

                                        // also fine if numlayers == 1
    long double mden        = powl ( 2 * n + 1, numlayers - 1 );

//  m11    /= mden;    m12    /= mden;  // not needed
    m21    /= mden;    m22    /= mden;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Isotropic conductivities in all layers -> fn = gn (equ. 2I 3I p.340)
    fn      = n / ( n * m22 + ( 1 + n ) * m21 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Legendre polynomials
//  Pn      = Legendre                 ( cosgamma, n );
//  Pln     = AssociatedLegendreOrder1 ( cosgamma, n );

                                        // Legendre polynomials, using iterative formula - !init values 0 or 1 and 1 or 2 respectively, so 1 matches both!
    NextLegendre                 ( Pnm2,  Pnm1,  Pn,  cosgamma, n );
    NextAssociatedLegendreOrder1 ( Plnm2, Plnm1, Pln, cosgamma, n );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Updating potential (equ. 1I p.340)
                                        // !Article says + cosbeta, BUT IT SHOULD BE - cosbeta! (see f.ex. Nunez & Srinivasan "Electric Fields of the Brain" 2006, p.573 equ. H.2.2 for tangential dipole)
                                        // !It might also stand from a bad textual description of what is angle beta p.337, and the lack of a proper figure with the angles depicted!
    deltaUI = RoRe * fn * ( n * cosalpha * Pn - cosbeta * sinalpha * Pln );

                                        // Safety measure, it can happen if cosgamma == 0 (which checked before, but better be safe)
    if ( IsNotAProperNumber ( deltaUI ) )
        break;


    UI     += deltaUI;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Error convergence is super-tricky:
                                        // deltas oscillate positive and negative, so we need to abs + smooth them
                                        // also reset error to current delta if it is above current smoothed value
    deltaUI = fabs ( deltaUI / NonNull ( UI ) );
    error   = max ( deltaUI, ( error + deltaUI ) / 2 );


#if defined (DebugPotentials)
    potiso ( potisosp, n - 1 )  = Log10 ( error );
//    potiso ( potisosp, n - 1 )  = fabs ( UI );
//    potiso ( potisosp, n - 1 )  = UI;
//    potiso ( potisosp, n - 1 )  = deltaUI;
#endif

    if ( error < convergence ) break;

//  if ( VkQuery () ) DBGV7 ( n, RoRe, fn, Pn, Pln, deltaUI, UI, "n, RoRe, fn, Pn, Pln, deltaUI, UI" );
    } // big sum


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // formula for a sphere of radius 1
UI     /= FourPi * sigma[ numlayers - 1 ];


if ( flags == ComputeLeadField )

    dipole.Direction   *= UI;


return  UI;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // sigma contains the absolute conductivity factors for each layer
                                        // !It is assumed all points have been correctly centered to the proper inverse center!
                                        // ?Also minimizing the Rho error could be optimized to be only on the electrode positions at hand - NOT the whole sphere?
void    ComputeLeadFieldLSMAC       (
                                    const OneLeadFieldPreset&   lfpreset,
                                    const TPoints&              xyzpoints,
                                    const TPoints&              solpoints,
                                    const TFitModelOnPoints&    surfacemodel,
                                    const TArray1<double>&      sigma,
                                    const TArray3<float>&       radius,
                                    AMatrix&                    K
                                    )
{

int                 numel               = (int) xyzpoints;
int                 numsolp             = (int) solpoints;

                                        // resize if it was not already set
K.resize ( numel, 3 * numsolp );


#if defined (DebugPotentials)
potiso.Resize ( numel * numsolp, PotentialIsotropicNLayersMaxTerms );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         gauge;

gauge.Set ( lfpreset.Title );

gauge.AddPart ( 0,  numel,   100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Store all SP to "border" distances, using the head model
TArray1<double>     spsurfradius ( numsolp );

                                        // Spherize the solution points
for ( int spi = 0; spi < numsolp; spi++ )
                                        // transform to real size model, still centered to model
    spsurfradius[ spi ] = surfacemodel.ToModel ( solpoints[ spi ] ).Norm ();


/*
#if defined(InvMatrixMoreOutputs)
TExportTracks       exprad;
sprintf ( exprad.Filename, "%s\\More\\SPs Radii.sef", CrisTransfer.BaseFileName );
exprad.SetAtomType ( AtomTypeScalar );
exprad.NumTracks        = 4;
exprad.NumTime          = numsolp;

exprad.ElectrodesNames.Resize ( exprad.NumTracks, 256 );
StringCopy   ( exprad.ElectrodesNames[ 0 ], "Depth" );
StringCopy   ( exprad.ElectrodesNames[ 1 ], "Surface" );
StringCopy   ( exprad.ElectrodesNames[ 2 ], "RelDepth" );
StringCopy   ( exprad.ElectrodesNames[ 3 ], "SphRelDepth" );

for ( int spi = 0; spi < numsolp; spi++ ) {
    exprad.Write ( solpoints[ spi ].Norm () );                          // SP depth
    exprad.Write ( spsurfradius [ spi ] );                              // corresponding point on the surface
    exprad.Write ( solpoints[ spi ].Norm () / spsurfradius [ spi ] );   // relative depth according to model
    exprad.Write ( normsolpoints[ spi ].Norm () );                      // same results as above
    }
#endif
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // If we wish to work directly on the voltages and/or save to temp file
//TArray2<float>      UI ( numel, numsolp );

                                        // 1) Fill matrix with vectors sp -> electrode
OmpParallelFor

for ( int ei = 0; ei < numel; ei++ ) {

    gauge.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TPointFloat         electrodepos        = xyzpoints[ ei ];

                                        // using the real electrode position to get the electrode radius - real electrode radius, but can be compromised from coregistration, and slightly different than the head model
    double              elradius            = electrodepos.Norm ();

                                        // using the model to get the electrode radius - more compatible with the solution points spherization
//  double              elradius            = surfacemodel.ToModel ( electrodepos ).Norm ();

                                        // whatever formula, we want a spherical model
    electrodepos.Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // consider the skull radius
    double              innerskullradius    = 0;
    double              outerskullradius    = 0;


    if      ( lfpreset.IsSkullRadiusFixedRatio     () ) {
                                        // skull gets thicker in absolute with bigger scalp
        innerskullradius    = lfpreset.InnerSkullRadius;
        outerskullradius    = lfpreset.OuterSkullRadius;
        }
    else if ( lfpreset.IsSkullRadiusModulatedRatio () ) {
                                        // Use the model to estimate the skull+scalp distance
                                        // what is beyond model scalp is considered more scalp, thus proportionally minimizing the skull radii
                                        // don't do the final translate to keep the result centered
        TPointFloat     tomodel     = surfacemodel.Unspherize ( xyzpoints[ ei ], false );

                                        // modulate external radius
        double          elradiuscorrection  = elradius / tomodel.Norm ();

                                        // skull remains cst in absolute size
        innerskullradius    = lfpreset.InnerSkullRadius / elradiuscorrection;
        outerskullradius    = lfpreset.OuterSkullRadius / elradiuscorrection;
        }
                                        // General case
    else if ( lfpreset.IsSkullRadiusGiven () ) {
                                        // use estimated thicknesses
        innerskullradius    = radius ( ei, SkullIndex, InnerRel );
        outerskullradius    = radius ( ei, SkullIndex, OuterRel );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfering the radii to the proper positions

                                        // With this model, radii will vary per electrode
    TArray1<double>     R ( sigma.GetDim () );


    if      ( lfpreset.NumLayers == 3 ) {

        R[ 0 ]  = innerskullradius;
        R[ 1 ]  = outerskullradius;
        R[ 2 ]  = 1;
        }
    else if ( lfpreset.NumLayers == 4 ) {

        R[ 0 ]  = radius ( ei, CsfIndex,   InnerRel );
        R[ 1 ]  = innerskullradius;
        R[ 2 ]  = outerskullradius;
        R[ 3 ]  = 1;
        }
    else if ( lfpreset.NumLayers == 6 ) {

        R[ 0 ]  = radius ( ei, CsfIndex,          InnerRel );
        R[ 1 ]  = innerskullradius;
        R[ 2 ]  = radius ( ei, SkullSpongyIndex,  InnerRel );
        R[ 3 ]  = radius ( ei, SkullSpongyIndex,  OuterRel );
        R[ 4 ]  = outerskullradius;
        R[ 5 ]  = 1;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we use the n layers corrected solution points
    for ( int spi = 0; spi < numsolp; spi++ ) {

        UpdateApplication;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Getting the spherization radius

                                        // Normalization by current SP radius (equivalent to a spherization):
                                        // - This is more equivalent to the SMAC model
                                        // - Pushes the SP "inward" according to its own radius, actually spherizing the whole SP distribution
                                        // + This will be much more unlikely that a SP will be above the skull radius
        double              spradius            = spsurfradius[ spi ];

                                        // spherized solution point
        TDipole             dipolesph;
        dipolesph.Position  = solpoints[ spi ] / spradius;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ellipsoid equivalent radius, which uses both electrode and SP radii - not sure if the subtle difference is worth it(?)
//      double              normradius          = sqrt ( Square ( elradius ) + Square ( spradius ) );


        if      ( lfpreset.IsIsotropicNShellSpherical () ) {

#if defined (DebugPotentials)
                                        // !comment out parallel omp for correct tracing!
            potisosp                = spi * numel + ei;
            dipolesph.SolPointIndex = spi;
#endif
                                        // !value returned for normalized sphere of radius 1!
//          UI ( ei, spi ) =
            PotentialIsotropicNShellExactSphericalLegendre  (   dipolesph,  ComputeLeadField,
                                                                electrodepos,
                                                                R,          sigma, 
                                                                PotentialIsotropicNLayersMaxTerms, PotentialIsotropicNLayersError
                                                            );
                                        // rescaling for sphere of arbitrary radius
                               // not sure why this 1000     radius converted to [m]
            dipolesph.Direction    /= 1000 * Square ( spradius / 1000 );
            }

        else if ( lfpreset.IsIsotropic3ShellSphericalAry () ) {

                                        // !value returned for normalized sphere of radius 1!
//          UI ( ei, spi ) =
            PotentialIsotropic3ShellApproxSphericalAry  (   dipolesph,  ComputeLeadField,
                                                            electrodepos,
                                                            R,          sigma
                                                        );
                                        // same rescaling formula as above
                                        // !Historically, this step was NOT applied in the LSMAC / Ary model, which was remaining in the normalized sphere!
            dipolesph.Direction    /= 1000 * Square ( spradius / 1000 );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here dipole has been updated
        K ( ei, 3 * spi     )   = dipolesph.Direction.X;
        K ( ei, 3 * spi + 1 )   = dipolesph.Direction.Y;
        K ( ei, 3 * spi + 2 )   = dipolesph.Direction.Z;
        }
    }

#if defined (DebugPotentials)
potiso.WriteFile ( "E:\\Data\\PotIso.Convergence.sef" );
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Center each column, so that its average is 0 (average reference)
K   = ACenteringMatrix ( numel ) * K;
}


//----------------------------------------------------------------------------
                                        // High-level wrapper, computing from individual T1 volumes
bool    ComputeLeadFieldLSMAC_T1    (   const OneLeadFieldPreset&   lfpreset,
                                        double                  age,
                                        const char*             headfile,
                                        const char*             brainfile,
                                        const char*             xyzfile,
                                        const char*             spfile,
                                        AMatrix&                K,
                                        TPoints&                xyzpoints,  TStrings&       xyznames,
                                        TPoints&                solpoints,  TStrings&       solpointsnames,
                                        TPointFloat&            mricentertoinversecenter,
                                        TArray3<float>&         tissuesradii
                                    )
{
if (   lfpreset.IsUndefined ()
  || ! lfpreset.IsGeometryLSMAC ()
  || StringIsEmpty ( headfile  )
  || StringIsEmpty ( brainfile )
  || StringIsEmpty ( xyzfile   )
  || StringIsEmpty ( spfile    ) )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need: Head, Brain, Electrodes and Solution Points, age and type of Lead Field model
TOpenDoc<TVolumeDoc>        mrihead     ( headfile,  OpenDocHidden );

TOpenDoc<TVolumeDoc>        mribrain    ( brainfile, OpenDocHidden );

TOpenDoc<TElectrodesDoc>    xyzdoc      ( xyzfile, OpenDocHidden );


xyzpoints   =  xyzdoc->GetPoints ( DisplaySpace3D );
xyznames    = *xyzdoc->GetElectrodesNames ();
                                        // there might be too much points / names
xyzpoints.RemoveLast ( xyzpoints.GetNumPoints () - xyzdoc->GetNumElectrodes () );
xyznames .RemoveLast ( xyznames.NumStrings    () - xyzdoc->GetNumElectrodes () );

solpointsnames;
solpoints.ReadFile   ( spfile, &solpointsnames );


if ( mrihead .IsNotOpen ()
  || mribrain.IsNotOpen ()
  || xyzdoc  .IsNotOpen ()
  || xyzpoints.IsEmpty  ()
  || solpoints.IsEmpty  () )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need an optimal L-SMAC Lead Field center
TPointFloat             mricenter       = mrihead->GetOrigin ();

TPoints                 headsurfacepoints;

headsurfacepoints.GetSurfacePoints ( *mrihead->GetData (), mricenter, false /*true*/ );
                                        // We can now cook a relative shift from these inputs
                                        // Be careful to the correct use of this translation, either add or subtract if applied to points or to a center
mricentertoinversecenter    = ComputeOptimalInverseTranslation ( &headsurfacepoints, &solpoints, &xyzpoints, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // It also helps if we know where the head stops
TMatrix44               mriabstoguillotine;

bool                    guillotineok    = SetGuillotinePlane    (   mrihead, 
                                                                    mriabstoguillotine
                                                                );
if ( ! guillotineok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Keep only the top points of the head for the spherization model
TPoints                 topsurfacepoints ( headsurfacepoints );

topsurfacepoints.KeepTopHeadPoints  ( *mrihead, mricentertoinversecenter, false, &mriabstoguillotine );

topsurfacepoints.DownsamplePoints   ( HeadModelNumPoints );

if ( topsurfacepoints.IsEmpty () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !TRANSLATE ALL POINTS TO USE THE INVERSE CENTER!
                    xyzpoints          += mricentertoinversecenter;
                    solpoints          += mricentertoinversecenter;
                  //headsurfacepoints  += mricentertoinversecenter;
                    topsurfacepoints   += mricentertoinversecenter;
TPointFloat         inversecenter       = mricenter - mricentertoinversecenter;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need a spherization model, to transform from the real head to a sphere, and vice-versa

                                        // Init model with only the top points
TFitModelOnPoints       surfacemodel    = TFitModelOnPoints ( topsurfacepoints );

bool                    surfacemodelok  = ComputeSpherizationModel ( surfacemodel );

if ( ! surfacemodelok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute layers conductivities
double              skullcond           = AgeToSkullConductivity ( age );  // absolute conductivity in [S/m]
double              skullcompactcond;
double              skullspongycond;
TSelection          seltissues;
TArray1<double>     sigma;

                                        // Compute the conductivities of each layer, starting from the deepest one
lfpreset.GetLayersConductivities    (   skullcond,  skullcompactcond,   skullspongycond,
                                        seltissues, sigma   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FctParams           p;

Volume              head ( *mrihead->GetData () );

                                        // use the same flag as global filtering
//if ( smoothmri ) {
//    p ( FilterParamDiameter )     = 3.47; // 5;
//    head.Filter ( FilterTypeGaussian, p, true );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Safe limit for brain surface
Volume              brainlimit  ( *mribrain->GetData () );

                                        // Force converting to mask
if ( mribrain->IsMask () ) {

    p ( FilterParamBinarized )     = 1;

    brainlimit.Filter ( FilterTypeBinarize, p );
    }
else {
    p ( FilterParamThresholdMin )     = mribrain->GetBackgroundValue ();
    p ( FilterParamThresholdMax )     = FLT_MAX;
    p ( FilterParamThresholdBin )     = 1;

    brainlimit.Filter ( FilterTypeThresholdBinarize, p );
    }


p ( FilterParamDiameter )   = 1;
brainlimit.Filter ( FilterTypeClose, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some users still provide brains without cerebellum, which we need to assess brain limits
                                        // We are going to run some local skull stripping and use it to upgrade our current brain
Volume              localbrain ( head );

p ( FilterParamSkullStrippingMethod     ) = SkullStripping1B;   // current best
p ( FilterParamSkullStrippingVoxelSize  ) = 1.5 * mrihead->GetVoxelSize ().Mean ();         // it seems to work with templates files, too...
p ( FilterParamSkullStrippingIsTemplate ) = IsTemplateFileName ( mrihead->GetDocPath () );

localbrain.Filter   ( FilterTypeSkullStripping, p, true );

                                        // upgrade using the cerebellum part only - we want to avoid any possible artifacts on the top, cortex part
for ( int x = 0; x < brainlimit.GetDim1 ();     x++ )
for ( int y = 0; y < brainlimit.GetDim2 () / 2; y++ )   // rear
for ( int z = 0; z < brainlimit.GetDim3 () / 2; z++ )   // bottom
                                        // inject into brainlimit directly
    brainlimit ( x, y, z )  = (bool) brainlimit ( x, y, z )  || (bool) localbrain ( x, y, z );
 
                                        // upgrade using the whole new brain?
//brainlimit |= localbrain;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Safe limit for skull radius search
Volume              skulllimit ( brainlimit );

                                        // add a bit of post-processing
p ( FilterParamDiameter )   = 8.0;
skulllimit.Filter ( FilterTypeDilate, p );


p ( FilterParamDiameter )   = 9.0;      // could erode even a tiny bit more, by using a Min filter
skulllimit.Filter ( FilterTypeErode,  p );


p ( FilterParamDiameter )   = 6.0;
p ( FilterParamNumRelax )   = 1;
skulllimit.Filter ( FilterTypeRelax,  p );

                                        // Assessing those intermediate volumes?
//TFileName       _file;
//StringCopy      ( _file,    "E:\\Data\\LeadField.localbrain", "." DefaultMriExt       );
//localbrain.WriteFile    ( _file );
//StringCopy      ( _file,    "E:\\Data\\LeadField.skulllimit", "." DefaultMriExt       );
//skulllimit.WriteFile    ( _file );
//StringCopy      ( _file,    "E:\\Data\\LeadField.brainlimit", "." DefaultMriExt       );
//brainlimit.WriteFile    ( _file );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute layers thicknesses
double              skullthickness  = AgeToSkullThickness ( age );  // in [mm]

bool                radiusok        = 

    EstimateTissuesRadii_T1     (
                                xyzpoints,
//                              SpatialFilterNone,                      0,          // could lead to bad radii, but results are more anatomically precise - but do we really need that for our case?
                                SpatialFilterInterseptileWeightedMean,  xyzdoc,     // safer, results are smoother but a bit less precise
                                head,               skulllimit,         brainlimit,
                                inversecenter,
                                mrihead->GetVoxelSize (),
                                true,               skullthickness,
                                tissuesradii
                                );
if ( ! radiusok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, we can compute the Lead Field
ComputeLeadFieldLSMAC       (   lfpreset,
                                xyzpoints,
                                solpoints, 
                                surfacemodel, 
                                sigma,
                                tissuesradii,
                                K
                            );

return  true;
}


//----------------------------------------------------------------------------
                                        // High-level wrapper, computing from segmented tissues volume
bool    ComputeLeadFieldLSMAC_Segmentation  (   const OneLeadFieldPreset&   lfpreset,
                                                double                  age,
                                                const char*             headfile,
                                                const char*             tissuesfile,
                                                const char*             xyzfile,
                                                const char*             spfile,
                                                AMatrix&                K,
                                                TPoints&                xyzpoints,  TStrings&       xyznames,
                                                TPoints&                solpoints,  TStrings&       solpointsnames,
                                                TPointFloat&            mricentertoinversecenter,
                                                TArray3<float>&         tissuesradii
                                            )
{
if (   lfpreset.IsUndefined ()
  || ! lfpreset.IsGeometryLSMAC ()
  || StringIsEmpty ( headfile    )
  || StringIsEmpty ( tissuesfile )
  || StringIsEmpty ( xyzfile     )
  || StringIsEmpty ( spfile      ) )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TVolumeDoc>        mrihead     ( headfile,  OpenDocHidden );

TOpenDoc<TVolumeDoc>        mritissues ( tissuesfile, OpenDocHidden );

TOpenDoc<TElectrodesDoc>    xyzdoc      ( xyzfile, OpenDocHidden );


xyzpoints   =  xyzdoc->GetPoints ( DisplaySpace3D );
xyznames    = *xyzdoc->GetElectrodesNames ();
                                        // there might be too much points / names
xyzpoints.RemoveLast ( xyzpoints.GetNumPoints () - xyzdoc->GetNumElectrodes () );
xyznames .RemoveLast ( xyznames.NumStrings    () - xyzdoc->GetNumElectrodes () );

solpointsnames;
solpoints.ReadFile   ( spfile, &solpointsnames );


if ( mrihead   .IsNotOpen ()
  || mritissues.IsNotOpen ()
  || xyzdoc    .IsNotOpen ()
  || xyzpoints .IsEmpty   ()
  || solpoints .IsEmpty   () )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need an optimal L-SMAC Lead Field center
TPointFloat             mricenter       = mrihead->GetOrigin ();

TPoints                 headsurfacepoints;

headsurfacepoints.GetSurfacePoints ( *mrihead->GetData (),    mricenter,     false /*true*/ );
                                        // We can now cook a relative shift from these inputs
                                        // Be careful to the correct use of this translation, either add or subtract if applied to points or to a center
mricentertoinversecenter    = ComputeOptimalInverseTranslation ( &headsurfacepoints, &solpoints, &xyzpoints, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // It also helps if we know where the head stops
TMatrix44               mriabstoguillotine;

bool                    guillotineok    = SetGuillotinePlane    (   mrihead, 
                                                                    mriabstoguillotine
                                                                );
if ( ! guillotineok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Keep only the top points of the head for the spherization model
TPoints                 topsurfacepoints ( headsurfacepoints );

topsurfacepoints.KeepTopHeadPoints  ( *mrihead, mricentertoinversecenter, false, &mriabstoguillotine );

topsurfacepoints.DownsamplePoints   ( HeadModelNumPoints );

if ( topsurfacepoints.IsEmpty () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !TRANSLATE ALL POINTS TO USE THE INVERSE CENTER!
                    xyzpoints          += mricentertoinversecenter;
                    solpoints          += mricentertoinversecenter;
                  //headsurfacepoints  += mricentertoinversecenter;
                    topsurfacepoints   += mricentertoinversecenter;
TPointFloat         inversecenter       = mricenter - mricentertoinversecenter;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need a spherization model, to transform from the real head to a sphere, and vice-versa

                                        // Init model with only the top points
TFitModelOnPoints       surfacemodel    = TFitModelOnPoints ( topsurfacepoints );

bool                    surfacemodelok  = ComputeSpherizationModel ( surfacemodel );

if ( ! surfacemodelok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute layers conductivities
double              skullcond           = AgeToSkullConductivity ( age );  // absolute conductivity in [S/m]
double              skullcompactcond;
double              skullspongycond;
TSelection          seltissues;
TArray1<double>     sigma;

                                        // Compute the conductivities of each layer, starting from the deepest one
lfpreset.GetLayersConductivities    (   skullcond,  skullcompactcond,   skullspongycond,
                                        seltissues, sigma   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute layers thicknesses
double              skullthickness  = AgeToSkullThickness ( age );  // in [mm]

bool                radiusok    =

    EstimateTissuesRadii_Segmentation (
                                xyzpoints,
                                SpatialFilterNone,                      0,                  // smoothing off
//                              SpatialFilterInterseptileWeightedMean,  Xyz,                // heavy filter
//                              SpatialFilterOutlier,                   Xyz,                // light filter to simply remove outliers - no big change in shape
                                *mritissues->GetData (),                mritissues->GetOrigin (),
                                mricenter,                              mrihead->GetVoxelSize (),
                                inversecenter,
                                true,                                   skullthickness,     // could also work for the species than human, but not tested
                                tissuesradii
                            );
if ( ! radiusok )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, we can compute the Lead Field
ComputeLeadFieldLSMAC       (   lfpreset,
                                xyzpoints,
                                solpoints, 
                                surfacemodel, 
                                sigma,
                                tissuesradii,
                                K
                            );

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
