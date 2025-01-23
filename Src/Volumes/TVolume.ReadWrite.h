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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Utils.h"
#include    "Geometry.TPoint.h"
#include    "Math.Stats.h"
#include    "Files.Utils.h"
#include    "TVolume.h"

#include    "TExportVolume.h"

#include    "TFreqCartoolDoc.h"         // TFreqFrequencyName

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Transform and save to file or TArray3
                                        // To parallelize, split in 2: one to transform in memory (which we need anyway for some calls), another one to save to file.
template <class TypeD>
void    TVolume<TypeD>::TransformAndSave (  TMatrix44&          transform,
                                            TransformAndSaveFlags transformflags,
                                            FilterTypes         filtertype,         InterpolationType   interpolate,        int             numsubsampling,
                                            const TPointDouble* sourceorigin,       const TPointDouble* targetorigin,
                                            const TPointDouble& sourcevoxelsize,    const TPointDouble* targetvoxelsize,
                                            BoundingSizeType    targetsize,         const TVector3Int*  giventargetsize,
                                            const char*         targetorientation,
                                            int                 niftitransform,     int                 niftiintentcode,    const char*     niftiintentname,
                                            const char*         fileout,            TVolume<TypeD>*     dataout,            TPointDouble*   origin,
                                            const char*         title 
                                         )  const
{
if ( IsNotAllocated () )
    return;

                                        // more worrisome would be a wrong filter type...
if ( ! ( filtertype == FilterTypeAntialiasing  
      || filtertype == FilterTypeMedian  
      || filtertype == FilterTypeMax     
      || filtertype == FilterTypeMean    
      || filtertype == FilterTypeGaussian 
      || numsubsampling == 1            // filter is to be ignored when there is no sub-sampling
        ) ) {

    ShowMessage ( "Unknown filter type...", "TransformAndSave", ShowMessageWarning );
    exit ( 1 );
    }


bool                savetofile      = fileout;
bool                savetomemory    = dataout;
                                        // Note that file and memory options are not exclusive
if ( ! ( savetofile || savetomemory ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // process the transform matrices
TMatrix44           transformd;         // direct:  source to target
TMatrix44           transformi;         // inverse: target to source

                                        // get first the direct transform
if ( transformflags & TransformToSource ) {
    transformd      = transform;
    transformd.Invert ();
    }
else
    transformd      = transform;

                                        // then possibly update the transform with the provided source/target origins, if any:
                                        //   - we need to start from Relative source
if ( ( transformflags & TransformSourceAbsolute ) && sourceorigin )

    transformd.Translate ( - *sourceorigin, MultiplyRight );

                                        //   - we need to finish into Relative target
if ( ( transformflags & TransformTargetAbsolute ) && targetorigin )

    transformd.Translate (   *targetorigin, MultiplyLeft  );

                                        // here transformd is: Source Rel -> Transform -> Target Rel

                                        // We can get the inverse transform now
transformi      = transformd;
transformi.Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting output type
TExportVolume       expvol;

if ( savetofile )
    StringCopy ( expvol.Filename, fileout );

expvol.VolumeFormat     = GetVolumeAtomType ( this, filtertype, interpolate, ToExtension ( expvol.Filename ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target dimension
TPointDouble        point;
TPointInt           boundmin;
TPointInt           boundmax;
TPointInt           bounddelta;

#define             boundingmarginfactor    0.05
#define             boundingmarginmin       5.0


if      ( ( targetsize == BoundingSizeGivenNoShift || targetsize == BoundingSizeGiven )
       && giventargetsize               // size should of course be valid - otherwise case will devolve into BoundingSizeOptimal
       && giventargetsize->IsNotNull () ) {


    expvol.Dim      [ 0 ] = abs ( giventargetsize->X );
    expvol.Dim      [ 1 ] = abs ( giventargetsize->Y );
    expvol.Dim      [ 2 ] = abs ( giventargetsize->Z );


    if ( targetsize == BoundingSizeGiven ) {
                                        // also setting the min boundary, "biggest" way, for correct output
        TPoints             corners;

        corners.SetCorners ( 0, 0, 0, Dim1 - 1, Dim2 - 1, Dim3 - 1 );

        transformd.Apply ( corners );

                                            // result is in absolute target space
        boundmin        = corners.GetLimitMin () - 0.5;
        boundmax        = corners.GetLimitMax () + 0.5;
                                            // shift boundmin so that results remain centered, in case the transformed data does not fit exactly in target size
                                            // this works both ways, either for cropping or padding
        bounddelta      = ( boundmax - boundmin ) + 1.0;

        boundmin       += ( bounddelta - *giventargetsize ) / 2;
        }
    else // if ( targetsize == BoundingSizeGivenNoShift )
                                        // don't change anything to the given output boundary
        boundmin        = 0;

    } // BoundingSizeGiven

else if ( targetsize == BoundingSizeBiggest ) {

    TPoints             corners;

    corners.SetCorners ( 0, 0, 0, Dim1 - 1, Dim2 - 1, Dim3 - 1 );

    transformd.Apply ( corners );

                                        // results are in absolute target space
    boundmin        = corners.GetLimitMin () - 0.5;
    boundmax        = corners.GetLimitMax () + 0.5;
    bounddelta      = ( boundmax - boundmin ) + 1.0;

                                        // get relative size from absolute boundaries
    expvol.Dim      [ 0 ] = bounddelta.X;
    expvol.Dim      [ 1 ] = bounddelta.Y;
    expvol.Dim      [ 2 ] = bounddelta.Z;
    } // BoundingSizeBiggest

else { // BoundingSizeOptimal and any other fallback cases

    double              backvalue       = GetBackgroundValue ();

    boundmin    = INT_MAX;
    boundmax    = INT_MIN;

    for ( int x = 0; x < Dim1; x++ )
    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {

        if ( GetValue ( x, y, z ) < backvalue )
            continue;


        point.Set ( x, y, z );

        transformd.Apply ( point );

                                        // results are in absolute target space
        Maxed ( boundmax.X, (int) ( point.X + 0.5 ) );
        Maxed ( boundmax.Y, (int) ( point.Y + 0.5 ) );
        Maxed ( boundmax.Z, (int) ( point.Z + 0.5 ) );

        Mined ( boundmin.X, (int) ( point.X - 0.5 ) );
        Mined ( boundmin.Y, (int) ( point.Y - 0.5 ) );
        Mined ( boundmin.Z, (int) ( point.Z - 0.5 ) );
        }

                                        // give some more room at the borders - the same for all 3 axis
    bounddelta      = ( boundmax - boundmin ) + 1.0;
    double              boundingmargin  = AtLeast ( boundingmarginmin, boundingmarginfactor * bounddelta.Mean () );
                                        // it could also be given 1 axis to not augment, the neck, but that means it must apply to full head AND to a correct orientation detection, which might fail...
    boundmin       -= boundingmargin;
    boundmax       += boundingmargin;

                                        // get relative size from absolute margins
    expvol.Dim      [ 0 ] = boundmax.X - boundmin.X + 1;
    expvol.Dim      [ 1 ] = boundmax.Y - boundmin.Y + 1;
    expvol.Dim      [ 2 ] = boundmax.Z - boundmin.Z + 1;

                                        // avoid odd numbers, for a clean middle
//  expvol.Dim      [ 0 ]&= ~ ((int) 1);
//  expvol.Dim      [ 1 ]&= ~ ((int) 1);
//  expvol.Dim      [ 2 ]&= ~ ((int) 1);
    } // BoundingSizeOptimal


//    boundmin.Show ( "boundmin" );
//    boundmax.Show ( "boundmax" );
//DBGV3 ( Dim1, Dim2, Dim3, "Original Volume: Dimension" );
//DBGV3 ( expvol.Dim[ 0 ], expvol.Dim[ 1 ], expvol.Dim[ 2 ], "Transform Volume: Dimension" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target origin
if ( transformflags & TransformTargetReset )
    expvol.Origin.Reset ();
else {
    expvol.Origin.X     = ( targetorigin ? targetorigin->X : 0 ) - boundmin.X;
    expvol.Origin.Y     = ( targetorigin ? targetorigin->Y : 0 ) - boundmin.Y;
    expvol.Origin.Z     = ( targetorigin ? targetorigin->Z : 0 ) - boundmin.Z;
    }

if ( origin )
    *origin = expvol.Origin;

//DBGV6 ( boundmin.X, boundmin.Y, boundmin.Z, expvol.Origin.X, expvol.Origin.Y, expvol.Origin.Z, "Transform Volume: boundmin -> Origin" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // caller knows the resulting orientation, trust him
if ( StringLength ( targetorientation ) == 3 )

    StringCopy  ( expvol.Orientation, targetorientation, 3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target voxel size

                                        // target voxel size overriding, usually for coregistration case
if      ( targetvoxelsize != 0 && targetvoxelsize->IsNotNull () )

    expvol.VoxelSize    = *targetvoxelsize;

else if ( sourcevoxelsize.IsNull () )
                                        // we have a problem - set to a fallback value of 1
    expvol.VoxelSize    = 1.0;

else {

    TPointDouble        isotropic       = transformd.IsIsotropic  ( sourcevoxelsize, 0.02  );
    TPointDouble        orthogonal      = transformd.IsOrthogonal ( sourcevoxelsize, 0.001 );


    if      ( isotropic.IsNotNull () )  // result is quasi-isotropic
                                        // this is the best case, even if vectors are not orthogonals, if voxels are the same in every direction, we are safe
                                        // set to the average of the 3 axis
        expvol.VoxelSize    = isotropic;

    else if ( orthogonal.IsNotNull () )
                                        // result is orthogonal (norm is close to the biggest component), anisotropy is not a problem here
        expvol.VoxelSize    = orthogonal;

    else                                // non-isotropic & non-orthogonal: this is going to be dirty!
                                        // do an approximation by averaging all 3 axis, there is not clean way to do that
        expvol.VoxelSize    = transformd.GetMeanVoxel ( sourcevoxelsize );

                                        // if voxel size is not equal in all directions, but still very close
                                        // round voxel sizes to nearest values
    if ( expvol.VoxelSize.X != expvol.VoxelSize.Y && RelativeDifference ( expvol.VoxelSize.X, expvol.VoxelSize.Y ) < 0.02
      && expvol.VoxelSize.X != expvol.VoxelSize.Z && RelativeDifference ( expvol.VoxelSize.X, expvol.VoxelSize.Z ) < 0.02
      && expvol.VoxelSize.Y != expvol.VoxelSize.Z && RelativeDifference ( expvol.VoxelSize.Y, expvol.VoxelSize.Z ) < 0.02 )
                                        // assign all voxels to the rounded precision average
        expvol.VoxelSize.X  =
        expvol.VoxelSize.Y  =
        expvol.VoxelSize.Z  = RoundTo ( expvol.VoxelSize.Mean (), 0.01 );
    }


//transformd.Show ( "Transform Forward" );
//sourcevoxelsize .Show ( "Transform Volume: Input VoxelSize" );
//expvol.VoxelSize.Show ( "Transform Volume: Output VoxelSize" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // caller might have given these complementary fields
expvol.NiftiTransform       = niftitransform;

expvol.NiftiIntentCode      = niftiintentcode;

StringCopy  ( expvol.NiftiIntentName, niftiintentname ? niftiintentname : NiftiIntentNameDefault, NiftiIntentNameSize - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( StringIsNotEmpty ( title ) )
    Gauge.Set       ( title, expvol.Dim[ 2 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if needed, create a Kernel mask of whatever size
TVolume<double>     K;

if ( filtertype == FilterTypeGaussian ) {

    K               =   TVolume<double> ( numsubsampling, AnySize );
    TPointDouble        Ko ( ( K.GetDim1 () - 1 ) / 2.0, ( K.GetDim2 () - 1 ) / 2.0, ( K.GetDim3 () - 1 ) / 2.0 );
    TPointDouble        kp;

    int                 gaussnumSD      = 2;
    TGaussian           gauss ( 0, ( numsubsampling - 1 ) / 2, gaussnumSD );

                                        // compute the Kernel
    for ( int xk = 0; xk < K.GetDim1 (); xk++ )
    for ( int yk = 0; yk < K.GetDim2 (); yk++ )
    for ( int zk = 0; zk < K.GetDim3 (); zk++ ) {

        kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );

        if      ( filtertype == FilterTypeGaussian      )       K ( xk, yk, zk )    = gauss ( kp.Norm () );
        else /*if ( filtertype == FilterTypeMean        )*/     K ( xk, yk, zk )    = 1;

                                        // Gaussian with own curvature (second derivative)?
//        kp      /= numsubsampling / 2;
//        double  n2 = kp.Norm2 ();
//        if      ( filtertype == FilterTypeGaussian      )       K ( xk, yk, zk )    = ( 16 + 64 * n2 ) / 24 * expl ( - 4 * n2 );
        }

                                        // normalize Kernel
    double              sumk            = 0;

    for ( int i = 0; i < K.GetLinearDim (); i++ )
        sumk       += K[ i ];

    K      /= sumk;


//    DBGV3 ( numsubsampling, K.GetDim1 (), Ko.X, "diameter  Kernel: Dim1 Origin" );
//    for ( int xk = 0; xk < K.GetDim1 (); xk++ )
//    for ( int yk = 0; yk < K.GetDim2 (); yk++ )
//    for ( int zk = 0; zk < K.GetDim3 (); zk++ )
//        DBGV4 ( xk, yk, zk, K ( xk, yk, zk ) * 1000, "x,y,z  K" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // In case of writing to memory
if ( savetomemory )

    dataout->Resize ( expvol.Dim[ 0 ], expvol.Dim[ 1 ], expvol.Dim[ 2 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 subsampling3    = Cube ( numsubsampling );
double              offset          = - (double) ( numsubsampling - 1 ) / numsubsampling / 2.0;   // shift to the "left", size of cube is half the jump (1/numsubsampling)

                                        // Safety check for results range
SetOvershootingOption ( interpolate, Array, LinearDim );


                                        // scan from target space
//OmpParallelBegin

TEasyStats          stat ( filtertype == FilterTypeMedian ? subsampling3 : 0 );
//TPointDouble        point;
double /*TypeD*/    getvalue;
double /*TypeD*/    v;

//OmpFor

for ( int z = 0; z < expvol.Dim[ 2 ]; z++ ) {

    Gauge.Next ();


    for ( int y = 0; y < expvol.Dim[ 1 ]; y++ )
    for ( int x = 0; x < expvol.Dim[ 0 ]; x++ ) {


        if      ( filtertype == FilterTypeAntialiasing ) {
                                        // set to center voxel
            point.Set ( x + boundmin.X, y + boundmin.Y, z + boundmin.Z );
                                        // starts with half voxel size
            v       = GetAntialiasedValue ( transformi, point, 0.5, interpolate );
            } // antialiasing

                                        // speed up things if not sub sampling
        else if ( numsubsampling == 1 ) {
                                        // set to center voxel
            point.Set ( x + boundmin.X, y + boundmin.Y, z + boundmin.Z );

            transformi.Apply ( point );

            v       = GetValueChecked ( point.X, point.Y, point.Z, interpolate );
            } // numsubsampling == 1

        else {

            stat.Reset ();
            v       = 0;

                                        // scan all voxels of the downsample cluster
            for ( int zd = 0; zd < numsubsampling; zd++ )
            for ( int yd = 0; yd < numsubsampling; yd++ )
            for ( int xd = 0; xd < numsubsampling; xd++ ) {

                point.Set ( x + boundmin.X + (double) xd / numsubsampling + offset,
                            y + boundmin.Y + (double) yd / numsubsampling + offset,
                            z + boundmin.Z + (double) zd / numsubsampling + offset );

                transformi.Apply ( point );

                getvalue    = GetValueChecked ( point.X, point.Y, point.Z, interpolate );


                if      ( filtertype == FilterTypeMedian    )   stat.Add ( getvalue, ThreadSafetyIgnore );
                else if ( filtertype == FilterTypeMax       )   stat.Add ( getvalue, ThreadSafetyIgnore );
                else if ( filtertype == FilterTypeMean      )   stat.Add ( getvalue, ThreadSafetyIgnore );
                else if ( filtertype == FilterTypeGaussian  )   v  += K ( xd, yd, zd ) * getvalue;  // K holds the exact weights, we can sum up directly
                } // subsampled block


            if      ( filtertype == FilterTypeMedian    )       v   = stat.Median ();
            else if ( filtertype == FilterTypeMax       )       v   = stat.Max    ();
            else if ( filtertype == FilterTypeMean      )       v   = stat.Mean   ();
//          else if ( filtertype == FilterTypeGaussian  )       v;
            } // no antialiasing && numsubsampling != 1


        if ( savetofile     )   expvol.Write ( v );     // give it a double, write will convert according to its internal type
        if ( savetomemory   )   dataout->GetValue ( x, y, z ) = (TypeD) v;
        } // for y, x
    } // for z

//OmpParallelEnd
}


//----------------------------------------------------------------------------
                                        // Conditional recursive splitting into 8 sub-cubes
template <class TypeD>
TypeD   TVolume<TypeD>::GetAntialiasedValue     (   TMatrix44&          transformi, 
                                                    TPointDouble&       pcenter,
                                                    double              size, 
                                                    InterpolationType   interpolate 
                                                )   const
{
                                        // distance from center to next centers
double              d               = size / 2;
                                        // volume of a single sub-cube
double              w               = Cube ( size );

//if ( VkQuery () && Log2 ( d ) < -2 ) DBGV2 ( pcenter.Z, 2 + Log2 ( d ), "Z  depth" );

                                        // compute sub-cubes values
double              v[ 8 ];
TPointDouble        p;


p.Set ( pcenter.X - d, pcenter.Y - d, pcenter.Z - d );      transformi.Apply ( p );     v[ 0 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X - d, pcenter.Y - d, pcenter.Z + d );      transformi.Apply ( p );     v[ 1 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X - d, pcenter.Y + d, pcenter.Z - d );      transformi.Apply ( p );     v[ 2 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X - d, pcenter.Y + d, pcenter.Z + d );      transformi.Apply ( p );     v[ 3 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X + d, pcenter.Y - d, pcenter.Z - d );      transformi.Apply ( p );     v[ 4 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X + d, pcenter.Y - d, pcenter.Z + d );      transformi.Apply ( p );     v[ 5 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X + d, pcenter.Y + d, pcenter.Z - d );      transformi.Apply ( p );     v[ 6 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;
p.Set ( pcenter.X + d, pcenter.Y + d, pcenter.Z + d );      transformi.Apply ( p );     v[ 7 ]  = GetValueChecked ( p.X, p.Y, p.Z, interpolate ) * w;

                                        // limit sub-levels
if ( d >= 1.0 / ( 64 * 4 ) ) {
                                        // limit if there is no chance of contribution, when back to a single char
//if ( w >= 1.0 / UCHAR_MAX / 8 ) {
                                        // 30 goes to -3, 40 to -2
    #define             neighbordisp        ( 35 * w )
    bool                resample[ 8 ];
                                            // pick exactly which sub-cube(s) to resample
    resample[ 0 ]   = max ( fabs ( v[ 0 ] - v[ 1 ] ), fabs ( v[ 0 ] - v[ 2 ] ), fabs ( v[ 0 ] - v[ 4 ] ) ) > neighbordisp;
    resample[ 1 ]   = max ( fabs ( v[ 0 ] - v[ 1 ] ), fabs ( v[ 1 ] - v[ 3 ] ), fabs ( v[ 1 ] - v[ 5 ] ) ) > neighbordisp;
    resample[ 2 ]   = max ( fabs ( v[ 2 ] - v[ 3 ] ), fabs ( v[ 0 ] - v[ 2 ] ), fabs ( v[ 2 ] - v[ 6 ] ) ) > neighbordisp;
    resample[ 3 ]   = max ( fabs ( v[ 2 ] - v[ 3 ] ), fabs ( v[ 1 ] - v[ 3 ] ), fabs ( v[ 3 ] - v[ 7 ] ) ) > neighbordisp;
    resample[ 4 ]   = max ( fabs ( v[ 4 ] - v[ 5 ] ), fabs ( v[ 4 ] - v[ 6 ] ), fabs ( v[ 0 ] - v[ 4 ] ) ) > neighbordisp;
    resample[ 5 ]   = max ( fabs ( v[ 4 ] - v[ 5 ] ), fabs ( v[ 5 ] - v[ 7 ] ), fabs ( v[ 1 ] - v[ 5 ] ) ) > neighbordisp;
    resample[ 6 ]   = max ( fabs ( v[ 6 ] - v[ 7 ] ), fabs ( v[ 4 ] - v[ 6 ] ), fabs ( v[ 2 ] - v[ 6 ] ) ) > neighbordisp;
    resample[ 7 ]   = max ( fabs ( v[ 6 ] - v[ 7 ] ), fabs ( v[ 5 ] - v[ 7 ] ), fabs ( v[ 3 ] - v[ 7 ] ) ) > neighbordisp;


    if ( resample[ 0 ] )    {   p.Set ( pcenter.X - d, pcenter.Y - d, pcenter.Z - d );      v[ 0 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 1 ] )    {   p.Set ( pcenter.X - d, pcenter.Y - d, pcenter.Z + d );      v[ 1 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 2 ] )    {   p.Set ( pcenter.X - d, pcenter.Y + d, pcenter.Z - d );      v[ 2 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 3 ] )    {   p.Set ( pcenter.X - d, pcenter.Y + d, pcenter.Z + d );      v[ 3 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 4 ] )    {   p.Set ( pcenter.X + d, pcenter.Y - d, pcenter.Z - d );      v[ 4 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 5 ] )    {   p.Set ( pcenter.X + d, pcenter.Y - d, pcenter.Z + d );      v[ 5 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 6 ] )    {   p.Set ( pcenter.X + d, pcenter.Y + d, pcenter.Z - d );      v[ 6 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    if ( resample[ 7 ] )    {   p.Set ( pcenter.X + d, pcenter.Y + d, pcenter.Z + d );      v[ 7 ]  = GetAntialiasedValue ( transformi, p, d, interpolate ); }
    }

                                        // v's are already weighted by their local volume size, which total sum is 1
return  (TypeD) ( v[ 0 ] + v[ 1 ] + v[ 2 ] + v[ 3 ] + v[ 4 ] + v[ 5 ] + v[ 6 ] + v[ 7 ] );
}


//----------------------------------------------------------------------------
                                        // freq file: Dim1=Time, Dim2=Electrodes, Dim3=Frequencies
template <class TypeD>
void    TVolume<TypeD>::ReadFile ( char* file, TStrings* tracknames )
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get file ready
bool                binformat       = IsExtensionAmong ( file, "bin dlf" );
bool                freqformat      = IsExtensionAmong ( file, FILEEXT_FREQ );
bool                segformat       = IsExtensionAmong ( file, FILEEXT_SEG );
bool                FileBin         = binformat || freqformat;

                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = binformat || freqformat || segformat;
assert ( formatok );
#endif

                                        // binary file is a dump, caller must allocate the object so we know what to load
#if defined(CHECKASSERT)
assert ( ! binformat || IsAllocated () );
#endif
//if ( binformat && IsNotAllocated () )
//    return;


if ( ! CanOpenFile ( file, CanOpenFileRead ) )
    return;


ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ), FileBin ? ios::binary | ios::in : ios::in );

if ( ifs.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get dimensions
int                 dim1;
int                 dim2;
int                 dim3;


if      ( binformat ) {
                                        // already allocated
    dim1            = Dim1;
    dim2            = Dim2;
    dim3            = Dim3;
    }

else if ( freqformat ) {

    TFreqHeader         freqheader;


    ifs.read ( (char *) (&freqheader), sizeof ( freqheader ) );

    int                 Version         = freqheader.Version;

    if ( ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 )
      && ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER2 ) )
        return ;

                                        // try to detect frequency analysis type
    FrequencyAnalysisType   FreqType    = StringToFreqType ( freqheader.Type );

    if ( IsFreqTypeComplex ( FreqType ) )
        return; // or OK?


    dim1        = freqheader.NumBlocks;     // aka NumTimeFrames
    dim2        = freqheader.NumChannels;
    dim3        = freqheader.NumFrequencies;


    if ( tracknames ) {

        tracknames->Reset ();
                                        // read electrode names
        char            buff[ 256 ];

        for ( int i = 0; i < freqheader.NumChannels; i++ ) {
            ifs.read ( buff, sizeof ( TFreqChannel ) );

            buff[ sizeof ( TFreqChannel ) ] = 0; // force EOS

            tracknames->Add ( buff );
            }
        } // if tracknames


//    BuffSize            = NumFrequencies * sizeof ( float ); // * NumElectrodes;

    LONGLONG    DataOrg     = sizeof ( TFreqHeader )
                            + freqheader.NumChannels    * sizeof ( TFreqChannel )
                            + freqheader.NumFrequencies * ( IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 ) ? sizeof ( TFreqFrequency ) : sizeof ( TFreqFrequencyName ) );

                                        // jump to data
    ifs.seekg ( DataOrg, ios::beg );
    }

else if ( segformat ) {

    char                buff[ 256 ];
    buff[255]   = 0;


    ifs.getline ( buff, 256 );

    int                 NumClusters;
                            // NumClusters, NumFiles, NumTimeFrames
    sscanf ( buff, "%d %d %d", &NumClusters, &dim1, &dim2 );

                                        // get number of variables
    ifs >> dim3;
                                        // skip names
    ifs.getline ( buff, 256 );
    }

                                        // allocate
Resize ( dim1, dim2, dim3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Read data
ResetMemory ();


if ( binformat ) {
                                        // memory dump
//    for ( int i = 0; i < LinearDim; i++ )
//        ifs.read ( (char *) &Array[ i ], AtomSize () );

    ifs.read ( (char *) Array, MemorySize () );

    } // binformat


else if ( freqformat ) {
                                        // allocation is the same order as the file order
//    for ( int i = 0; i < LinearDim; i++ )
//        ifs.read ( (char *) &Array[ i ], AtomSize () );

    ifs.read ( (char *) Array, MemorySize () );

    } // binformat


else if ( segformat ) {

    char                buff[ 256 ];

                                        // read in the values
    for ( int tf = 0; tf < Dim2; tf++ ) 
    for ( int fi = 0; fi < Dim1; fi++ ) 
    for ( int vi = 0; vi < Dim3; vi++ )

        GetValue ( fi, tf, vi )     = StringToDouble ( GetToken ( &ifs, buff ) );

    } // segformat

}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::WriteFile   (   const char*             file, 
                                        const TPointDouble*     origin, 
                                        const TVector3Double*   voxelsize,          const TVector3Double*   realsize, 
                                        const char*             orientation,
                                        int                     niftitransform,     int                     niftiintentcode,    const char* niftiintentname,
                                        AtomFormatType          atomformattype
                                    )   const
{
if ( IsNotAllocated () || StringIsEmpty ( file ) )
    return;

                                        // output like an MRI?
if ( IsExtensionAmong ( file, TVolumeWriteFileExt ) ) {

    TExportVolume     expvol;


    StringCopy ( expvol.Filename, file );

    expvol.VolumeFormat = atomformattype == UnknownAtomFormat   ? GetVolumeAtomType ( this, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) ) 
                                                                : atomformattype;

    expvol.MaxValue     = GetMaxValue ();           // we know that

    expvol.Dim      [ 0 ] = Dim1;
    expvol.Dim      [ 1 ] = Dim2;
    expvol.Dim      [ 2 ] = Dim3;

    if ( origin )
        expvol.Origin               = *origin;

    if ( voxelsize )
        expvol.VoxelSize            = *voxelsize;

    if ( realsize )
        expvol.RealSize             = *realsize;

    if ( orientation )
        StringCopy  ( expvol.Orientation, orientation, 3 );
                                        // caller might have given these complementary fields
    expvol.NiftiTransform           = niftitransform;

    expvol.NiftiIntentCode          = niftiintentcode;

    StringCopy  ( expvol.NiftiIntentName, niftiintentname ? niftiintentname : NiftiIntentNameDefault, NiftiIntentNameSize - 1 );

                                        // just do it!
    expvol.Write ( *this, ExportArrayOrderZYX );

    }
else {                                  // simple binary dump
    ofstream            os ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

    if ( ! os.good () )
        return;

    os.write ( (char *) Array, MemorySize () );
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







