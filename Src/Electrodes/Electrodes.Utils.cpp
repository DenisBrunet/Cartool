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

#include    "Electrodes.Utils.h"

#include    "Math.Stats.h"
#include    "Math.TMatrix44.h"
#include    "Geometry.TPoints.h"
#include    "Geometry.TBoundingBox.h"

#include    "GlobalOptimize.Points.h"

#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
double  CoregisterXyzToXyz  (   TPoints&            frompoints,     const TPoints&      topoints, 
                                GOMethod            method,         int                 how,            double              precision,
                                int                 numscaling,     bool                finalscaling,
                                const TStrings*     xyznames,       const char*         filename,
                                const char*         title
                            )
{
TFitPointsOnPoints  goxyz   ( frompoints, topoints );
TEasyStats          goxyzq;

TBoundingBox<double>    boxfrom ( frompoints );
TBoundingBox<double>    boxto   ( topoints   );


goxyz.Set ( GOStepsDefault );


if ( numscaling == 1 ) {
                                        // global scale
    goxyz.AddGroup ();
    goxyz.AddDim ( Scale, boxto.Radius () / boxfrom.Radius () * 0.75, boxto.Radius () / boxfrom.Radius () * 1.25 );
    }
else {                                  // non-rigid scaling
    goxyz.AddGroup ();
    goxyz.AddDim ( ScaleX, boxto.GetRadius ( 0 ) / boxfrom.GetRadius ( 0 ) * 0.75, boxto.GetRadius ( 0 ) / boxfrom.GetRadius ( 0 ) * 1.25 );
    goxyz.AddDim ( ScaleY, boxto.GetRadius ( 1 ) / boxfrom.GetRadius ( 1 ) * 0.75, boxto.GetRadius ( 1 ) / boxfrom.GetRadius ( 1 ) * 1.25 );
    goxyz.AddDim ( ScaleZ, boxto.GetRadius ( 2 ) / boxfrom.GetRadius ( 2 ) * 0.75, boxto.GetRadius ( 2 ) / boxfrom.GetRadius ( 2 ) * 1.25 );
    }

                                        // rotations
double              rotz            = how == MatchingPairs ? 45 : 20;   // give enough Z rotation to account for (unfortunate) wrong front-back detection...

goxyz.AddGroup ();
goxyz.AddDim   ( RotationX, -20,   20   );
goxyz.AddDim   ( RotationY, -20,   20   );
goxyz.AddDim   ( RotationZ, -rotz, rotz );  

                                        // translations
goxyz.AddGroup ();
goxyz.AddDim   ( TranslationX, -boxto.GetRadius ( 0 ) * 0.20, boxto.GetRadius ( 0 ) * 0.20 );
goxyz.AddDim   ( TranslationY, -boxto.GetRadius ( 1 ) * 0.20, boxto.GetRadius ( 1 ) * 0.20 );
goxyz.AddDim   ( TranslationZ, -boxto.GetRadius ( 2 ) * 0.20, boxto.GetRadius ( 2 ) * 0.20 );


goxyz.GetSolution   (   method,         how, 
                        precision,      0, 
                        title, 
                        &goxyzq
                    );


double              quality         = 1e4 / ( NonNull ( goxyzq.Average () ) * NonNull ( goxyzq.Max () ) * NonNull ( goxyzq.SD () ) );

//DBGV5 ( goxyzq.Average (), goxyzq.Max (), goxyzq.SD (), goxyzq.CoV () * 100, quality, "Coregistration Quality:  AvgDiff DiffMax SDDiff CoV -> quality" );
//DBGV  ( goxyz[ 0 ][ 0 ].Value, "Scale" );
//DBGV3 ( goxyz[ 1 ][ 0 ].Value, goxyz[ 1 ][ 1 ].Value, goxyz[ 1 ][ 2 ].Value, "Rotations ZYX" );
//DBGV3 ( goxyz[ 2 ][ 0 ].Value, goxyz[ 2 ][ 1 ].Value, goxyz[ 2 ][ 2 ].Value, "Translations" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

goxyz.Transform ( frompoints, finalscaling );


if ( StringIsNotEmpty ( filename ) )
    frompoints.WriteFile ( filename, xyznames );


return  quality;
}


//----------------------------------------------------------------------------
void    AveragePoints   (   TPoints*        listpoints, 
                            int             numlists, 
                            TArray2<bool>&  oktoaverage, 
                            TPoints&        averagepoints, 
                            TStrings*       xyznames,
                            char*           filename
                        )
{
int                 numpoints       = listpoints[ 0 ].GetNumPoints ();
TGoEasyStats        statsx ( numpoints /*, numlists*/ );
TGoEasyStats        statsy ( numpoints /*, numlists*/ );
TGoEasyStats        statsz ( numpoints /*, numlists*/ );


for ( int li = 0; li < numlists;  li++ )
for ( int pi = 0; pi < numpoints; pi++ ) {
                                        // testing for dummy points (maybe too far, too ??)
    if ( ! oktoaverage ( li, pi ) )
        continue;

    statsx[ pi ].Add ( listpoints[ li ][ pi ].X );
    statsy[ pi ].Add ( listpoints[ li ][ pi ].Y );
    statsz[ pi ].Add ( listpoints[ li ][ pi ].Z );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use median to get rid of extremes / outliers
TPoints             avgarray ( numpoints );

for ( int pi = 0; pi < numpoints; pi++ ) {
    avgarray[ pi ].X    = statsx[ pi ].Mean (); // Median ( false );
    avgarray[ pi ].Y    = statsy[ pi ].Mean (); // Median ( false );
    avgarray[ pi ].Z    = statsz[ pi ].Mean (); // Median ( false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert to list, and proceed
averagepoints.Set ( avgarray );


if ( StringIsNotEmpty ( filename ) )
    averagepoints.WriteFile ( filename, xyznames );
}


//----------------------------------------------------------------------------
void    CenterAndReorient   (   TPoints&        xyzpoints, 
                                TMatrix44&      matrix, 
                                char*           filexyz 
                            )
{
TElectrodesDoc*     Xyz;
bool                closexyz;


closexyz            = ! Cartool.CartoolDocManager->IsOpen ( filexyz );
Xyz                 = dynamic_cast< TElectrodesDoc* > ( Cartool.CartoolDocManager->OpenDoc ( filexyz, dtOpenOptionsNoView /*dtOpenOptions*/ ) );
Xyz->PreventClosing ();

                                        // Local -> RAS
matrix              = Xyz->GetStandardOrientation ( LocalToRAS );

                                        // center Local -> RAS
TPointDouble        center          = Xyz->GetBounding ( DisplaySpace3D )->GetCenter ();

matrix.Translate ( -center.X, -center.Y, -center.Z, MultiplyRight );


matrix.Apply ( xyzpoints );


if ( Xyz ) {
    Xyz->AllowClosing ();

    if ( closexyz && Xyz->CanClose ( true ) )
        Cartool.CartoolDocManager->CloseDoc ( Xyz );
    }
}


//----------------------------------------------------------------------------
void    SymmetrizeXyz   (   TPoints&    xyzpoints   )
{
                                        // 1) Find the best Sagittal plane

TBoundingBox<double>    bounding ( xyzpoints );
TPointsProperties       gosag    ( xyzpoints );
//TEasyStats            gosagq;


gosag.Set ( GOStepsDefault );

                                        // rotations
gosag.AddGroup ();
gosag.AddDim    ( RotationY, -15, 15 );
gosag.AddDim    ( RotationZ, -15, 15 );

                                        // translation
gosag.AddGroup ();
gosag.AddDim    ( TranslationX, -bounding.GetRadius ( 2 ) * 0.10,  bounding.GetRadius ( 2 ) * 0.10 );


gosag.GetSolution   (   PointsPropertiesMethod,     PointsSagittal, 
                        PointsPropertiesPrecision,  1e-2, 
                        0
                        //&gosagq
                    );


//DBGV3 ( gosagq.Average (), gosagq.Max, gosagq.SD (), "Sagittal Quality:  AvgDiff DiffMax SDDiff" );
//DBGV2 ( gosag[ 0 ][ 0 ].Value, gosag[ 0 ][ 1 ].Value, "Sagittal rotates Y Z" );
//DBGV  ( gosag[ 0 ][ 2 ].Value, "Sagittal translate X" );


gosag.Transform ( xyzpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Average points with their symmetric twins
TPoints             sympoints;

TVector3Float       p;
TVector3Float       ps;
TVector3Float       pc;


for ( int i = 0; i < (int) xyzpoints; i++ ) {

    p       = xyzpoints[ i ];
                                        // symmetric point
    ps      = p;
    ps.X    = - ps.X;
                                        // closest point to symmetric point
    pc      = xyzpoints.GetClosestPoint ( ps );


    if ( pc == p )                      // closest is original point itself?
        p.X = 0;                        // force to exact sagittal plane
    else {
        p   = ( ps + pc ) / 2;          // set to average position
        p.X = - p.X;                    // and reverting the symmetry
        }


    sympoints.Add ( p );
    }

                                        // transfer the new symmetric points
xyzpoints   = sympoints;

}


//----------------------------------------------------------------------------
                                        // Realign Sagittal + "Transverse" planes
bool    NormalizeXyz    (   TPoints&        xyz,
                            TStrings&       xyznames,
                            TPoints&        buddyxyz,
                            const char*     xyzreffile
                        )
{
TSuperGauge         Gauge;
Gauge.Set       ( "Electrodes Normalizing", SuperGaugeLevelDefault );
Gauge.AddPart   ( 0, 4 + ( StringIsNotEmpty ( xyzreffile ) ? 4 : 0 ),  100 );


//#define     DebugNormalizeXyz
#undef      DebugNormalizeXyz

#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.0.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Centering
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Rough geometrical center
//xyz      -= xyz.GetCenter ();
//buddyxyz -= xyz.GetCenter ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Better center: BFS center
Gauge.Next ();
UpdateApplication;

TFitModelOnPoints   gosurf1 ( xyz );

TBoundingBox<double>    bounding    = gosurf1.Bounding;
TPointDouble        center          = gosurf1.Center;
double              spread          = bounding.Radius ();


gosurf1.Set ( GOStepsDefault );

                                        // scaling
gosurf1.AddGroup ();
gosurf1.AddDim   ( Scale,  spread * 0.75, spread * 1.25 );

                                        // translations
gosurf1.AddGroup ();
gosurf1.AddDim   ( TranslationX, -center.X - spread * 0.20, -center.X + spread * 0.20 );
gosurf1.AddDim   ( TranslationY, -center.Y - spread * 0.20, -center.Y + spread * 0.20 );
gosurf1.AddDim   ( TranslationZ, -center.Z - spread * 0.20, -center.Z + spread * 0.20 );


gosurf1.GetSolution     (   FitModelPointsMethod,   FitModelPointsHow, 
                            GODefaultConvergence,   0, 
                            0 
                        );

                                        // adjusting center
xyz        += gosurf1.GetTranslation ();
buddyxyz   += gosurf1.GetTranslation ();

#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.1.Center.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Orientation
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Aligning Top to +Z (approximately..)
Gauge.Next ();
UpdateApplication;

TPointsProperties   gotop ( xyz );

                                        // much more intermediate steps
gotop.Set ( GOStepsPrecise );

                                        // rotations
gotop.AddGroup ();
gotop.AddDim    ( RotationX, -90, 90 );
gotop.AddDim    ( RotationY, -90, 90 );
gotop.AddDim    ( RotationZ, -90, 90 );


gotop.GetSolution   (   PointsPropertiesMethod,     PointsReorientTop, 
                        PointsPropertiesPrecision,  0, 
                        0 // "Top Orientation Search"
                    );   


gotop.Transform ( xyz      );
gotop.Transform ( buddyxyz );

#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.2.ToTop.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Finding Sagittal plane YZ
Gauge.Next ();
UpdateApplication;

TPointsProperties   gosag ( xyz );

                                        // much more intermediate steps
gosag.Set ( GOStepsPrecise );

                                        // rotations
gosag.AddGroup ();
gosag.AddDim    ( RotationY, -25, 25 );
gosag.AddDim    ( RotationZ, -90, 90 );


gosag.GetSolution   (   GlobalBoxScan /*PointsPropertiesMethod*/,     PointsSagittal, 
                        PointsPropertiesPrecision,  0, 
                        0 // "Sagittal Plane Search"
                    );   


gosag.Transform ( xyz      );
gosag.Transform ( buddyxyz );

#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.3.Sagittal.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Finding upward orientation / approximate Transverse plane
Gauge.Next ();
UpdateApplication;

/*
TPointsProperties   gotra ( xyz );


gotra.Set ( GOStepsPrecise );

                                        // rotation
gotra.AddGroup ();
gotra.AddDim    ( RotationX, -90, 90 );


gotra.GetSolution   (   PointsPropertiesMethod,     PointsTransverse, 
                        PointsPropertiesPrecision,  0, 
                        0 // "Transverse Plane Search"
                    );


gotra.Transform ( xyz, true, &buddyxyz );
*/
                                        // "Top" has been already resolved, it should be enough
TPointsProperties::ResolveFrontBackOrientation  ( xyz, &buddyxyz );

#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.4.FrontBack.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can not continue without a reference xyz
if ( StringIsEmpty ( xyzreffile ) )
    return  true;                       // still a valid exit

                                        // open the reference xyz
TStrings            xyzrefnames;
TPoints             xyzref ( xyzreffile, &xyzrefnames );


if      ( (int) xyz > (int) xyzref )    // reference does not have enough points
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here xyzref COULD have more electrodes than the input xyz
                                        // Output will complement the missing points as best as it could...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Coregistering to a given model, in current orientation then turned 180°
                                        // then picking the one with the best fit to solve the front-back ambiguity
TPoints             xyz000 ( xyz );
TPoints             xyz180 ( xyz );
TMatrix44           mat;

mat.RotateZ ( 180, MultiplyLeft );
                                        // turning whole thing 180°
mat.Apply   ( xyz180 );

                                        // run twice the coregistration, with 2 versions of the xyz
Gauge.Next ();
UpdateApplication;

double              quality000      = CoregisterXyzToXyz    (   xyz000,         xyzref, 
                                                                FitPointsOnPointClosestMethod,  ClosestPoints,  1e-3,       // quick & good enough
                                                                3,              true,
                                                                0,              0
                                                            );

Gauge.Next ();
UpdateApplication;

double              quality180      = CoregisterXyzToXyz    (   xyz180,         xyzref, 
                                                                FitPointsOnPointClosestMethod,  ClosestPoints,  1e-3,       // quick & good enough
                                                                3,              true,
                                                                0,              0
                                                            );

//DBGV2 ( quality000, quality180, "quality000, quality180" );

                                        // keeping the best coregistered xyz aside
TPoints             xyzcoreg        = quality000 > quality180 ? xyz000 : xyz180;

#ifdef DebugNormalizeXyz
xyzcoreg.WriteFile ( "E:\\Data\\NormalizePoints.5.Coreg.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotate our xyz, and coregister again for best results
Gauge.Next ();
UpdateApplication;

if ( quality180 > quality000 ) {

    mat.Apply ( xyz      );
    mat.Apply ( buddyxyz );
                                        // re-apply coregistration, but without scaling
                                        // !buddyxyz not transformed!
    CoregisterXyzToXyz  (   xyz,            xyzref, 
                            FitPointsOnPointClosestMethod,  ClosestPoints,  1e-3,       // quick & good enough
                            3,              false,
                            0,              0
                        );
    }
else {
                                        // re-apply coregistration, but without scaling
                                        // !buddyxyz not transformed!
    CoregisterXyzToXyz  (   xyz,            xyzref, 
                            FitPointsOnPointClosestMethod,  ClosestPoints,  1e-3,       // quick & good enough
                            3,              false,
                            0,              0
                        );
    }


#ifdef DebugNormalizeXyz
xyz.WriteFile ( "E:\\Data\\NormalizePoints.5.FrontBackCoreg.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Try remapping by electrode names

int                 numelfrom       = (int) xyz;
int                 numelto         = (int) xyzref;
TSelection          fromsel ( numelfrom, OrderSorted );
TSelection          tosel   ( numelto,   OrderSorted );

                                        // !allocate the reordered list of points = same number of points as the reference!
TPoints             xyzremap        = xyzref;
                                        // in case some target poinst would be missing, apply some raw rescaling
xyzremap    *= xyz.GetRadius () / xyzref.GetRadius ();


int                 numin           = 0;

                                        // if enough names -> remapping by names
if ( (int) xyznames >= (int) xyzrefnames ) {

    for ( int fromi = 0; fromi < numelfrom; fromi++ )
    for ( int toi   = 0; toi   < numelto;   toi  ++ ) {

        if ( fromsel[ fromi ] || tosel[ toi ] )
            continue;
    
        if ( StringIs ( xyznames[ fromi ], xyzrefnames[ toi ] ) ) {
        
            fromsel.Set ( fromi );
            tosel  .Set (   toi );
                                        // reassign position according to reference
            xyzremap[ toi ]     = xyz[ fromi ];
                                        // count how many are in - index overlap is impossible here
            if ( ++numin == numelto )
                break;
            }
        }

#ifdef DebugNormalizeXyz
xyzremap.WriteFile ( "E:\\Data\\NormalizePoints.6.Remapped.Names.xyz" );
#endif

    } // remapping by names


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) If either not enough names, or name remapping failed -> remapping by location
Gauge.Next ();
UpdateApplication;

                                        // !this is not an 'else' from previous test, so it can account for previous method's failure!
if ( (int) xyznames < (int) xyzrefnames 
  || numin != numelto ) {

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TArray2<float>      dist    ( numelfrom * numelto, 3 );

    for ( int fromi = 0, i = 0; fromi < numelfrom; fromi++      )
    for ( int toi   = 0;        toi   < numelto;   toi  ++, i++ ) {

        dist ( i, 0 )   = fromi;
        dist ( i, 1 )   = toi;
        dist ( i, 2 )   = ( xyzcoreg[ fromi ] - xyzref[ toi ] ).Norm2 ();   // Euclidean distance
    //  dist ( i, 2 )   = - xyzcoreg[ fromi ].Cosine ( xyzref[ toi ] );     // Cosine distance
        }

    dist.SortRows ( 2, Ascending );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // look now at all pairs of electrodes, starting by the closest ones
    fromsel.Reset ();
    tosel  .Reset ();
    numin   = 0;

                                        // !It is possible that some xyzremap be not overwritten - in that case the xyzref will be used!
    for ( int i = 0; i < dist.GetDim1 () && numin < numelto; i++ ) 
                                            // no electrode should have been assigned yet
        if ( ! ( fromsel[ dist ( i, 0 ) ] || tosel[ dist ( i, 1 ) ] ) ) {
                                            // they're done, these two
            fromsel.Set ( dist ( i, 0 ) );
            tosel  .Set ( dist ( i, 1 ) );
                                            // reassign position according to reference
            xyzremap[ dist ( i, 1 ) ]   = xyz[ dist ( i, 0 ) ];
                                            // count how many are in - index overlap is impossible here
            numin++;
            }

    // here, we could still have numin < numelto...

#ifdef DebugNormalizeXyz
xyzremap.WriteFile ( "E:\\Data\\NormalizePoints.7.Remapped.Positions.xyz" );
#endif

    } // remapping by location


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we have the remapped points - !could be less than from the file!
xyz         = xyzremap;
xyznames    = xyzrefnames;
                                        // this is the only thing we can do for our buddies (sorted X, then Y, then Z)
//buddyxyz.Sort ();                     // but we would need to sort names accordingly...

#ifdef DebugNormalizeXyz
xyzremap.WriteFile ( "E:\\Data\\NormalizePoints.8.Final.xyz" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
