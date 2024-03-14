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

#include    "ESI.HeadSphericalModel.h"

#include    "GlobalOptimize.Points.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define     OutputSpherizationModelFiles

                                        // !New version gets optimal re-centered points, and must skip any translation!
bool    ComputeSpherizationModel    (   TFitModelOnPoints&  gosurf  )
{
const TBoundingBox<double>& bounding    = gosurf.Bounding;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First compute an approximate model with only 4 parameters
TFitModelOnPoints   gosurflo ( gosurf.Points );

gosurflo.Set ( GOStepsDefault );
                                        // Best Fitting Ellipsoid - Better, an ellipsoid is a good approximation of the brain
                                        // scalings
gosurflo.AddGroup ();
gosurflo.AddDim   ( ScaleX, bounding.GetRadius ( 0 ) * 0.75, bounding.GetRadius ( 0 ) * 1.25 );
gosurflo.AddDim   ( ScaleY, bounding.GetRadius ( 1 ) * 0.75, bounding.GetRadius ( 1 ) * 1.25 );
gosurflo.AddDim   ( ScaleZ, bounding.GetRadius ( 2 ) * 0.75, bounding.GetRadius ( 2 ) * 1.25 );

                                        // rotation
gosurflo.AddGroup ();
gosurflo.AddDim   ( RotationX, -15, 15 );

                                        // translations - not needed if points have been already re-centered
//gosurflo.AddGroup ();
//gosurflo.AddDim   ( TranslationX, -bounding.GetRadius ( 0 ) * 0.05, bounding.GetRadius ( 0 ) * 0.05 );
//gosurflo.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.05, bounding.GetRadius ( 1 ) * 0.05 );
//gosurflo.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.05, bounding.GetRadius ( 2 ) * 0.05 );


gosurflo.GetSolution    (   FitModelPointsMethod,   FitModelPointsHow, 
                            GODefaultConvergence,   0, 
                            "Low-Rez Head Model" 
                        );


//DBGV3 ( gosurflo.GetValue ( TranslationX ), gosurflo.GetValue ( TranslationY ), gosurflo.GetValue ( TranslationZ ), "HeadModel 1: MriCenterToInverseCenter" );
//DBGV3 ( gosurflo.GetValue ( ScaleX ), gosurflo.GetValue ( ScaleY ), gosurflo.GetValue ( ScaleZ ), "HeadModel 1: Scaling" );
//DBGV  ( gosurflo.GetValue ( RotationX ), "HeadModel 1: RotationX" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Then compute a finer model with 15+ parameters
                                        // Feed the previous approximate model as a starting point
gosurf.Set ( GOStepsDefault );

                                        // scalings
gosurf.AddGroup ();
gosurf.AddDim   ( ScaleX, gosurflo.GetValue ( ScaleX ) * 0.90, gosurflo.GetValue ( ScaleX ) * 1.10 );
gosurf.AddDim   ( ScaleY, gosurflo.GetValue ( ScaleY ) * 0.90, gosurflo.GetValue ( ScaleY ) * 1.10 );
gosurf.AddDim   ( ScaleZ, gosurflo.GetValue ( ScaleZ ) * 0.90, gosurflo.GetValue ( ScaleZ ) * 1.10 );

                                        // rotation & shearing can be put together
gosurf.AddGroup ();
gosurf.AddDim   ( RotationX, gosurflo.GetValue ( RotationX ) - 5, gosurflo.GetValue ( RotationX ) + 5 );
gosurf.AddDim   ( ShearXtoY, -0.02, 0.02 ); // shearing, for subjects or even asymmetrical templates - this makes for a tighter fit

                                        // translations - not needed if points have been already re-centered
//gosurf.AddGroup ();
//gosurf.AddDim   ( TranslationX, gosurflo.GetValue ( TranslationX ) - bounding.GetRadius ( 0 ) * 0.01, gosurflo.GetValue ( TranslationX ) + bounding.GetRadius ( 0 ) * 0.01 );
//gosurf.AddDim   ( TranslationY, gosurflo.GetValue ( TranslationY ) - bounding.GetRadius ( 1 ) * 0.01, gosurflo.GetValue ( TranslationY ) + bounding.GetRadius ( 1 ) * 0.01 );
//gosurf.AddDim   ( TranslationZ, gosurflo.GetValue ( TranslationZ ) - bounding.GetRadius ( 2 ) * 0.01, gosurflo.GetValue ( TranslationZ ) + bounding.GetRadius ( 2 ) * 0.01 );

                                        // pinch along the X axis on the Y and Z axis
gosurf.AddGroup ();
gosurf.AddDim   ( SinusPinchYtoX, 0.00, -0.25 );
gosurf.AddDim   ( SinusPinchYtoZ, 0.00, -0.25 );

                                        // flattening front, back, left-right, up-down
gosurf.AddGroup ();
gosurf.AddDim   ( FlattenYPos, 0.00, 0.50 );
gosurf.AddDim   ( FlattenYNeg, 0.00, 0.50 );

gosurf.AddGroup ();
gosurf.AddDim   ( FlattenZPos, 0.00, 0.50 );
gosurf.AddDim   ( FlattenX,    0.00, 0.50 );

                                        // bumps from the parietal plates
gosurf.AddGroup ();
gosurf.AddDim   ( TopBumpY,        0.00, 0.50 );
gosurf.AddDim   ( TopLateralBumpY, 0.00, 0.25 );


TEasyStats          gosurfq;

#ifdef _DEBUG
gosurf.GetSolution  (   HeadModelPointsMethod,  HeadModelPointsHow, 
                        1e-2,   0, 
                        "Hi-Rez Head Model", 
                        &gosurfq 
                    );
#else
gosurf.GetSolution  (   HeadModelPointsMethod,  HeadModelPointsHow, 
                        GODefaultConvergence,   0, 
                        "Hi-Rez Head Model", 
                        &gosurfq 
                    );
#endif

//gosurfq.Show ( "HiRez Head Model quality" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined ( OutputSpherizationModelFiles )
                                        // force model for checking
//gosurf.SetValue ( TopLateralBumpX, 0.0 );
//AddExtension ( file, "A.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );
//gosurf.SetValue ( TopLateralBumpX, 0.10 );
//AddExtension ( file, "B.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );
//gosurf.SetValue ( TopLateralBumpX, 0.20 );
//AddExtension ( file, "C.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );
//gosurf.SetValue ( TopLateralBumpX, 0.30 );
//AddExtension ( file, "D.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );

/*
AddExtension ( file, "6 Latteral Bump.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );

gosurf.SetValue ( TopLateralBumpX, 0.0 );
AddExtension ( file, "5 Top Bump.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );
gosurf.SetValue ( TopBumpX, 0.0 );
AddExtension ( file, "4 Flattening.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );

gosurf.SetValue ( FlattenXPos, 0.0 );
gosurf.SetValue ( FlattenXNeg, 0.0 );
gosurf.SetValue ( FlattenYNeg, 0.0 );
gosurf.SetValue ( FlattenZ, 0.0 );
AddExtension ( file, "3 Pinching.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );

gosurf.SetValue ( SinusPinchXtoY, 0.0 );
gosurf.SetValue ( SinusPinchXtoZ, 0.0 );
AddExtension ( file, "2 Ellipsoid.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );

double              meanradius      = gosurf.GetMeanRadius ();
gosurf.SetValue ( ScaleX, meanradius );
gosurf.SetValue ( ScaleY, meanradius );
gosurf.SetValue ( ScaleZ, meanradius );
AddExtension ( file, "1 Sphere.xyz" ); gosurf.WriteFile ( file ); RemoveExtension ( file, 2 );
*/

TFileName           pointsfile;
StringCopy              ( pointsfile, "E:\\Data\\Model.FromPoints" );
AddExtension            ( pointsfile, FILEEXT_XYZ );
CheckNoOverwrite        ( pointsfile );
gosurf.Points.WriteFile ( pointsfile );


TFileName           modelfile;
StringAppend        ( modelfile, "E:\\Data\\Model.LoRez.", GetGOMethodExtension ( HeadModelPointsMethod ) );
AddExtension        ( modelfile, FILEEXT_XYZ );
CheckNoOverwrite    ( modelfile );
gosurflo.WriteFile  ( modelfile );

StringReplace       ( modelfile, ".LoRez.", ".HiRez." );
CheckNoOverwrite    ( modelfile );
gosurf.WriteFile    ( modelfile );

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // returned model seems OK?
double              modelsd         = gosurfq.SD () * ( bounding.MeanSize () / 192.0 );
bool                modelok         = modelsd < 1.8;


//DBGV3 ( gosurfq.Average (), gosurfq.Max (), gosurfq.SD (), "Model Quality:  AvgDiff  DiffMax  SDDiff" );
//DBGV3 ( gosurf[ 2 ][ 0 ].Value, gosurf[ 2 ][ 2 ].Value, gosurf[ 2 ][ 3 ].Value, "Translate" )
//DBGV  ( gosurf[ 1 ][ 0 ].Value, "Rotate" )
//DBGV3 ( gosurf[ 0 ][ 0 ].Value, gosurf[ 0 ][ 1 ].Value, gosurf[ 0 ][ 2 ].Value, "Radius" )
//DBGV2 ( gosurf[ 3 ][ 0 ].Value, gosurf[ 3 ][ 1 ].Value, "Pinch, Hex" )
//DBGV4 ( gosurfq.SD (), bounding.MeanSize (), modelsd, modelok, "Model Quality:  SDDiff MaxSize modelsd -> OK" );


return  modelok;
}


//----------------------------------------------------------------------------
/*                                      // Not used anymore
void    SaveSpherizedMri ( char *file, TVolumeDoc* mridoc, Volume *volume, TFitModelOnPoints &gosurf, bool creepy )
{
if ( mridoc == 0 || volume == 0 )
    return;


//bool               ismask           = mridoc->IsMask ();
//int                 subsampling     = ismask ? 1 : 2;   // don't subsample masks, it makes them much fatter (no good when going to the beach)
int                 subsampling     = 2;
int                 subsampling3    = subsampling * subsampling * subsampling;
TPointFloat         p;
long                v;
TEasyStats          median ( subsampling3 );


TExportVolume     expvol;


StringCopy ( expvol.Filename, file );

expvol.Dim      [ 0 ]   =
expvol.Dim      [ 1 ]   =
expvol.Dim      [ 2 ]   = Power2Above ( 2 * gosurf.GetMaxRadius () + 1 + 4 );

expvol.VoxelSize        = mridoc->GetVoxelSize();   // just to relay the real voxel size up to the unspherization

expvol.Origin.X         = expvol.Dim[ 0 ] / 2.0;
expvol.Origin.Y         = expvol.Dim[ 1 ] / 2.0;
expvol.Origin.Z         = expvol.Dim[ 2 ] / 2.0;

expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, NiftiTransform );

expvol.NiftiIntentCode  = NiftiIntentCode;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentName , NiftiIntentNameSize - 1 );

StringCopy  ( expvol.Orientation, NiftiOrientation, 3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "MRI Spherization", expvol.Dim[ 2 ] );


SetOvershootingOption ( interpolate, volume->Array, LinearDim );


for ( int z = 0; z < expvol.Dim[ 2 ]; z++ ) {

    Gauge.Next ();

    for ( int y = 0; y < expvol.Dim[ 1 ]; y++ )
    for ( int x = 0; x < expvol.Dim[ 0 ]; x++ ) {

        median.Reset ();

                                        // scan all voxels of the downsample cluster
        for ( int zd = 0; zd < subsampling; zd++ )
        for ( int yd = 0; yd < subsampling; yd++ )
        for ( int xd = 0; xd < subsampling; xd++ ) {

                                        // If upgrade needed: look at TransformAndSave + Lanczos

            p    = TPointFloat ( x - expvol.Origin.X + 0.5 / subsampling - 0.5 + (double) xd / subsampling,
                                 y - expvol.Origin.Y + 0.5 / subsampling - 0.5 + (double) yd / subsampling,
                                 z - expvol.Origin.Z + 0.5 / subsampling - 0.5 + (double) zd / subsampling );

                                        // outside sphere?
            if ( p.Norm () > 1 && creepy )
//              median.Add ( 0 );
                continue;

            p   = gosurf.Unspherize ( p );

            mridoc->ToRel ( p );

                                        // add to buffer, & no linear interpolation!
            median.Add ( volume->GetValueChecked ( p.X, p.Y, p.Z, InterpolateLanczos3 ) );
            }

        median.Sort ( true );
                                        // median value both filters the data, without creating new values
        v   = median.Median ();


        expvol.Write ( v );
        }
    }
}
*/

//----------------------------------------------------------------------------
/*                                      // Not used anymore
void    SaveUnspherizedMri ( char *file, TVolumeDoc* mrigreydoc, TVolumeDoc *mribrain, TFitModelOnPoints &gosurf )
{
                                        // have we ran the Spherization beforehand?
if ( ! (bool) gosurf || mrigreydoc == 0 || mribrain == 0 )
    return;


Volume*             volume          = mrigreydoc->GetData ();
//bool               ismask           = mrigreydoc->IsMask ();
int                 subsampling     = 2;
int                 subsampling3    = subsampling * subsampling * subsampling;
TPointFloat         p;
long                v;
TEasyStats          median ( subsampling3 );
TPointFloat         sphericalcenter ( mrigreydoc->GetOrigin()[ 0 ], mrigreydoc->GetOrigin()[ 1 ], mrigreydoc->GetOrigin()[ 2 ] );
TPointFloat         braincenter     ( mribrain->GetOrigin()[ 0 ],   mribrain->GetOrigin()[ 1 ],   mribrain->GetOrigin()[ 2 ] );



TExportVolume     expvol;


StringCopy ( expvol.Filename, file );

expvol.Dim      [ 0 ]   = mribrain->GetSize()->GetXExtent ();
expvol.Dim      [ 1 ]   = mribrain->GetSize()->GetYExtent ();
expvol.Dim      [ 2 ]   = mribrain->GetSize()->GetZExtent ();

expvol.VoxelSize        = mribrain->GetVoxelSize();

expvol.Origin           = braincenter;

expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, NiftiTransform );

expvol.NiftiIntentCode  = NiftiIntentCode;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentName , NiftiIntentNameSize - 1 );

StringCopy  ( expvol.Orientation, NiftiOrientation, 3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "MRI Un-Spherization", expvol.Dim[ 2 ] );


SetOvershootingOption ( interpolate, volume->Array, LinearDim );


for ( int z = 0; z < expvol.Dim[ 2 ]; z++ ) {

    Gauge.Next ();

    for ( int y = 0; y < expvol.Dim[ 1 ]; y++ )
    for ( int x = 0; x < expvol.Dim[ 0 ]; x++ ) {

        median.Reset ();

                                        // scan all voxels of the downsample cluster
        for ( int zd = 0; zd < subsampling; zd++ )
        for ( int yd = 0; yd < subsampling; yd++ )
        for ( int xd = 0; xd < subsampling; xd++ ) {

                                        // If upgrade needed: look at TransformAndSave + Lanczos

            p   = TPointFloat ( x + 0.5 / subsampling - 0.5 + (double) xd / subsampling,
                                y + 0.5 / subsampling - 0.5 + (double) yd / subsampling,
                                z + 0.5 / subsampling - 0.5 + (double) zd / subsampling );

            p   = gosurf.Spherize ( p - braincenter );
                                        // maybe rescale before translation?                        
            p  *= gosurf.GetMaxRadius ();

            p  += sphericalcenter;
                                        // add to buffer, & no linear interpolation!
            median.Add ( volume->GetValueChecked ( p.X, p.Y, p.Z, InterpolateTruncate ) );
            }

        median.Sort ( true );


//      if ( ismask ) {
//          median.Sort ( Descending );
//          v   = median[ 0 ];      // max -> fatter mask
//          }
//      else                        // median value both filters the data, without creating new values
            v   = median.Median ();


        expvol.Write ( v );
        }
    }
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
