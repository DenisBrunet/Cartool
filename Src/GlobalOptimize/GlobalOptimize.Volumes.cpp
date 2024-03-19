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

#include    "MemUtil.h"
#include    "Math.Histo.h"
#include    "Math.Resampling.h"
#include    "GlobalOptimize.Tracks.h"
#include    "TCacheVolumes.h"
#include    "TFilters.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TExportVolume.h"

#include    "GlobalOptimize.Volumes.h"
#include    "GlobalOptimize.Points.h"       // GlobalOptimizeMaxPoints, geometrical transform enums

#include    "TVolumeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

char                RemapIntensityNames[ NumRemapIntensityTypes ][ 32 ] =
                    { 
                    "No intensity remapping",
                    "Histogram equalization",
                    "Histogram brain equalization",
                    "Ranking data",
                    "Ramp-Ranking data",
                    "Binarize data",
                    "Conversion to mask",
                    "Data inverted",
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeProperties::TVolumeProperties ()
{
Reset ();
}


        TVolumeProperties::TVolumeProperties    (   const TVolumeDoc*   mridoc,     
                                                    const TMatrix44&    datatoras,
                                                    double              threshold,
                                                    const TMatrix44*    normtomri,  // input
                                                    const TPointDouble* center      // input
                                                )
{
SetVolume   (   mridoc,   
                datatoras,
                threshold,
                normtomri,
                center    
            );
}


void    TVolumeProperties::SetVolume    (   const TVolumeDoc*   mridoc,     
                                            const TMatrix44&    datatoras,
                                            double              threshold,
                                            const TMatrix44*    normtomri,  // input
                                            const TPointDouble* center      // input
                                        )
{
TGlobalOptimize::Reset ();

                                        // Copy all we need from MRI parameter
Vol                 = *mridoc->GetData ();
Bound               = *mridoc->GetBounding ();
Threshold           = threshold > 0 ? threshold : mridoc->GetBackgroundValue ();   // overridden threshold
MaxValue            = NonNull ( mridoc->GetMaxValue () );
ExtraContentType    = mridoc->GetExtraContentType ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

LeftRightIndex      = mridoc->GetAxisIndex ( LeftRight );
FrontBackIndex      = mridoc->GetAxisIndex ( FrontBack );
UpDownIndex         = mridoc->GetAxisIndex ( UpDown    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

InvStandardOrient   = datatoras;
                                        // RAS -> Data - Similar to  GetNormalizationTransform  function
InvStandardOrient.Invert();

                                        // standard way to go back to our MRI
NormToMRI           = normtomri ? *normtomri : InvStandardOrient;
                                        // we really need a non-null center
if ( center && center->IsNotNull () )

    Center  = *center;
else {
    Center  = mridoc->GetDefaultCenter ();
                                        // use integer center
    Center.Truncate ();
    }
}


void    TVolumeProperties::Reset ()
{
TGlobalOptimize::Reset ();

Vol.ResetMemory ();
Bound .Reset ();
Threshold           = 0;
MaxValue            = 0;
ExtraContentType    = UnknownMriContentType;
                                        // default to RAS
LeftRightIndex      = 0;
FrontBackIndex      = 1;
UpDownIndex         = 2;

InvStandardOrient.SetIdentity ();
NormToMRI        .SetIdentity ();
Center.Reset ();
}


//----------------------------------------------------------------------------
void    TVolumeProperties::GetSolution  (   GOMethod    method,             int         how, 
                                            double      requestedprecision, double      outliersprecision, 
                                            const char* title,
                                            TEasyStats* stat 
                                        )
{
                                        // Preprocessing volume
if      ( IsTransversePlaneMNI ( how ) ) {

    TFileName       MniSliceFile;

    StringCopy       ( MniSliceFile,    CartoolObjects.CartoolApplication->ApplicationDir, "\\",
                                        how == TransversePlaneMNIT1 ? MniT1BrainCentralSliceFileName 
                                                                    : MniT2BrainCentralSliceFileName );

    MniSlicedoc.Open ( MniSliceFile, OpenDocHidden );

                                        // slice file not found -> downgrade the search to a stable alternative
    if ( MniSlicedoc.IsNotOpen () )

        how     = TransversePlaneBiggestSurface;

    else {
                                        // Volumes are going through a simplified coregistration, so they need some preprocessing:
        FctParams           p;
                                        // about 5% smoothing - no min limit, if too small don't filter
                                        // now off, some volumes seem to get lost with it, and it doens't seem to hinder the convergence without it
//      p ( FilterParamDiameter )     = Bound.Radius () / 100.0 * 4.5;
//      Vol                  .Filter ( FilterTypeFastGaussian, p, false );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Histogram Equalization:

                                        // Extract volume data corresponding to the MNI central slices, using provided transform
        const Volume&               mnidata         = *MniSlicedoc->GetData ();
        TEasyStats                  stat ( mnidata.GetLinearDim () );


        OmpParallelFor
                                        // scan MNI slices, in relative space
        for ( int i = 0; i < mnidata.GetLinearDim (); i++ ) {

            TPointDouble    p;
                                        // index to x,y,z, in MNI slice coordinates
            mnidata.LinearIndexToXYZ ( i, p );
                                        // ..then in MNI Absolute (centered)
            MniSlicedoc->ToAbs ( p );
                                        // ..then to absolute input volume
            NormToMRI.Apply ( p );
                                        // safety: make sure we are back to relative input volume
            if ( NormToMRI.HasNoTranslation () )
                p  += Center;

                                        // Source volume, including background (0)
            double          vvol    = Vol.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );

            if ( vvol > 0 )
                stat.Add ( vvol, ThreadSafetyCare );
            }

                                        // do we have enough data to equalize?
        if ( stat.GetNumItems () > 100 ) {
                                        // Compute CDF only on the central slices
            THistogram          cdf     (   stat, 
                                            0,      0,      0, 
                                            0.0,    1.0, 
                                            HistogramCDFOptions
                                        );

            TVector<double>     toequ;
                                        // Convert it to some equalization LUT
            cdf.EqualizeCDF ( toequ );

                                        // Finally transform the whole volume from the slice LUT
            for ( int i = 0; i < Vol.GetLinearDim (); i++ )
                Vol[ i ] = toequ[ cdf.ToBin ( RealUnit, Vol[ i ] ) ];


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ranking the MNI slices
            p ( FilterParamThreshold )     = 0;
            MniSlicedoc->GetData ()->Filter ( FilterTypeRank, p, false );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ranking on the whole available data - simpler but not so correct:
//      p ( FilterParamThreshold )     = 0;
//      Vol.Filter ( FilterTypeRank, p, false );
//
//      p ( FilterParamThreshold )     = 0;
//      MniSlicedoc->GetData ()->Filter ( FilterTypeRank, p, false );
        }

    }

else if ( how != TransversePlaneGuillotine ) {
                                        // Otherwise ranking data (but not for mask!)
    if ( ! IsMask ( ExtraContentType ) ) {

        FctParams           p;
        p ( FilterParamThreshold )     = Threshold;

        Vol.Filter ( FilterTypeRank, p, false );
                                        
        Threshold           = 1.0 / Vol.GetLinearDim (); // new threshold - should be the smallest non-null ranked data, or epsilon
        MaxValue            = 1;                            // new max
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGlobalOptimize::GetSolution ( method, how, requestedprecision, outliersprecision, title, stat );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finish by updating the NormToMRI matrix with final results
if     ( How == SagittalPlaneSymmetric
      || How == SagittalPlaneSymmetricT1
      || How == SagittalPlaneSymmetricT1Gad )

    EvaluateSagittalPlaneMatrix ();

else if ( How == TransversePlaneGuillotine )
                                        // !original NormToMRI will be lost!
    EvaluateTransversePlaneGuillotineMatrix ( NormToMRI );

else if ( How == TransversePlaneLongest
       || How == TransversePlaneBiggestBox
       || How == TransversePlaneBiggestSurface ) {
                                        // !original NormToMRI will be lost!
    EvaluateTransversePlaneMatrix ( NormToMRI );

    CenterSlice ();
    }

else if ( IsTransversePlaneMNI ( How ) ) {
                                        // doing this will return the slice adjusted to the MNI space - another option, but not the one we want here
//  EvaluateTransversePlaneMatrix ( NormToMRI );

                                        // instead, we just update the translation and the rotation, skipping the scaling
                                                NormToMRI.SetTranslation ( Center[ 0 ], Center[ 1 ], Center[ 2 ] );

    if ( HasValue( RotationX ) )                NormToMRI.RotateX ( GetValue ( RotationX ), MultiplyRight );
    }
    
}


//----------------------------------------------------------------------------
double  TVolumeProperties::Evaluate ( TEasyStats* stat )
{
                                        // Route to the right function
                                        // the cost for systematic cost is much lower than the heavy evaluation functions!
                                        // and derived class pointer to functions is an absolute pain in the a..
if      ( How == TransversePlaneLongest         )   return  EvaluateTransversePlaneLongest          ( stat );
else if ( How == TransversePlaneBiggestBox      )   return  EvaluateTransversePlaneBiggestBox       ( stat );
else if ( How == TransversePlaneBiggestSurface  )   return  EvaluateTransversePlaneBiggestSurface   ( stat );
else if ( How == TransversePlaneMNIT1
       || How == TransversePlaneMNIT2           )   return  EvaluateTransversePlaneMNI              ( stat );
else if ( How == TransversePlaneGuillotine      )   return  EvaluateTransversePlaneGuillotine       ( stat );
else if ( How == SagittalPlaneSymmetric
       || How == SagittalPlaneSymmetricT1
       || How == SagittalPlaneSymmetricT1Gad    )   return  EvaluateSagittalPlaneSymmetric          ( stat );
else                                                return  0;
}


//----------------------------------------------------------------------------
inline void TVolumeProperties::EvaluateSagittalPlaneMatrix ()
{
                                        // compute the transformation matrix
NormToMRI       = InvStandardOrient;


if ( HasValue( TranslationX ) ) Center[ LeftRightIndex ]    = GetValue ( TranslationX );
if ( HasValue( TranslationY ) ) Center[ FrontBackIndex ]    = GetValue ( TranslationY );
if ( HasValue( TranslationZ ) ) Center[ UpDownIndex    ]    = GetValue ( TranslationZ );


                                NormToMRI.Translate ( Center[ 0 ], Center[ 1 ], Center[ 2 ], MultiplyLeft );


if ( HasValue( ShearYtoX ) || HasValue( ShearZtoX ) )   NormToMRI.ShearX  ( GetValue ( ShearYtoX ), GetValue ( ShearZtoX ), MultiplyRight );
if ( HasValue( ShearXtoY ) || HasValue( ShearZtoY ) )   NormToMRI.ShearY  ( GetValue ( ShearXtoY ), GetValue ( ShearZtoY ), MultiplyRight );
if ( HasValue( ShearXtoZ ) || HasValue( ShearYtoZ ) )   NormToMRI.ShearZ  ( GetValue ( ShearXtoZ ), GetValue ( ShearYtoZ ), MultiplyRight );

if ( HasValue( RotationX ) )    NormToMRI.RotateX ( GetValue ( RotationX ), MultiplyRight );
if ( HasValue( RotationY ) )    NormToMRI.RotateY ( GetValue ( RotationY ), MultiplyRight );
if ( HasValue( RotationZ ) )    NormToMRI.RotateZ ( GetValue ( RotationZ ), MultiplyRight );
}


//----------------------------------------------------------------------------
                                        // Find the sagittal slice with the lowest difference between its left and right
double  TVolumeProperties::EvaluateSagittalPlaneSymmetric ( TEasyStats *stat )
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
TPointInt           Step;
double              sumslice        = 0;
double              sumw            = 0;

                                        // Adaptive width search: starting bigger, then refining to smaller gaps
//double              precisionw      = Clip ( ( 3.0 + Log10 ( CurrentPrecision ) ) / 3.0, 0.0, 1.0 );  // 1 -> 0
double              precisionw      = Clip ( ( Log10 ( RequestedPrecision ) - Log10 ( CurrentPrecision ) ) / Log10 ( RequestedPrecision ), 0.0, 1.0 );  // 1 -> 0
double              dxrel           = precisionw * 0.10 + 0.05;
double              dx              = Bound.GetRadius ( LeftRightIndex ) * dxrel;
//double            dx              = Bound.GetRadius ( LeftRightIndex ) * 0.075;     // Fixed width

double              normz           = NonNull ( Bound.GetRadius ( UpDownIndex    ) );
double              normy           = NonNull ( Bound.GetRadius ( FrontBackIndex ) );


EvaluateSagittalPlaneMatrix ();

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );


MaxCorner       =   Bound.Radius();

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

Step.Y          =   AtLeast ( 1.0, Bound.YStep ( 50 ) );
Step.Z          =   AtLeast ( 1.0, Bound.ZStep ( 50 ) );


if ( stat )
    stat->Reset ();

                                        // Retrieving value from normalized coordinates
auto    getvalue = [ & ] ( double nx, double ny, double nz ) {

    TPointDouble        point ( nx, ny, nz );

    NormToMRI.Apply ( point );

    return  Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear );
};


OmpParallelForSum ( sumslice, sumw )
                                        // scan a sagittal sub-sampled slice
for ( int z = MinCorner.Z; z <= MaxCorner.Z; z += Step.Z )
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y += Step.Y ) {

                                        // get values from the 2 points across each side of the current sagittal plane
    double      v1      = getvalue ( -dx, y, z );

    double      v2      = getvalue (  dx, y, z );

    double      v1v2    = abs ( v1 - v2 );

                                        
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting some handy weights
    double      ztopw       = Clip ( z / normz + 1, 0.0, 1.0 );             // 1 for upper half, decreasing to 0 for lower half
    double      yfrontw     = Clip ( y / normy + 1, 0.0, 1.0 );             // 1 for front half, decreasing to 0 for back half
    double      ycenterw    = Clip ( 1 - abs ( y / normy ),   0.0, 1.0 );   // 0 at front & back sides
                                        // set a default weight of 1, by safety
    double      w           = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if      ( How == SagittalPlaneSymmetricT1 ) {

        w           = ztopw * ycenterw; // focus on top / center parts
                     // penalize asym   penalize cutting into white matter & credit for dark central vein
        sumslice   += ( v1v2          + max ( v1, v2 ) ) * w;
        }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( How == SagittalPlaneSymmetricT1Gad ) {

        w           = ztopw * yfrontw;  // focus on top / front parts
                                        // retrieve central value
        double      v0      = getvalue ( 0, y, z );
                     // penalize asym   credit central white (vein) (adding 1 to force results to be above 0)
        sumslice   += ( v1v2          - v0 + 1 ) * w;
        }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else { // if ( How == SagittalPlaneSymmetric ) {

        w           = ztopw * ycenterw; // focus on top / center parts
                   // penalize asymmetry
        sumslice   += v1v2 * w;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    sumw        += w;

    if ( stat && v1 && v2 )
        stat->Add ( v1v2 );
    }


return  sumslice / sumw;
}


//----------------------------------------------------------------------------
inline void TVolumeProperties::EvaluateTransversePlaneGuillotineMatrix ( TMatrix44& normtomri )
{
                                        // compute the transformation matrix
normtomri       = NormToMRI;

                                        // and set updated center
if ( HasValue( TranslationZ ) )             Center[ UpDownIndex    ]    = GetValue ( TranslationZ );


                                            normtomri.Translate ( Center[ 0 ], Center[ 1 ], Center[ 2 ], MultiplyLeft );


if ( HasValue( RotationX ) )                normtomri.RotateX ( GetValue ( RotationX ), MultiplyRight );
if ( HasValue( RotationY ) )                normtomri.RotateY ( GetValue ( RotationY ), MultiplyRight );
if ( HasValue( RotationZ ) )                normtomri.RotateZ ( GetValue ( RotationZ ), MultiplyRight );
}


//----------------------------------------------------------------------------
                                        // Find the bottom slice where the head/neck is cut
                                        // This function does not work with Nelder-Mead, as it does not evaluate correctly from afar, or you need some Annealing
double  TVolumeProperties::EvaluateTransversePlaneGuillotine ( TEasyStats *stat )
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
TPointInt           Step;
double              sumslice        = 0;
int                 numsum          = 0;
TMatrix44           normtomri;

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

                                        // get a delta proportional to data span
double              precisionw      = Clip ( ( Log10 ( RequestedPrecision ) - Log10 ( CurrentPrecision ) ) / Log10 ( RequestedPrecision ), 0.0, 1.0 );  // 1 -> 0

double              dz              = Bound.GetRadius ( UpDownIndex )   // corresponds to the typical tested range
                                      / 2                               // we scan half above and half below at the same time
                                      / 5                               // 5 = ( 2 * 2 + 1 ) the typical number of division of deltaz for search
                                      * precisionw                      // make the delta bigger at first, then shrinking for bettwe precision
                                      + 1;                              // keep at least 1 voxel above and below


if ( stat )
    stat->Reset ();


EvaluateTransversePlaneGuillotineMatrix ( normtomri );


MaxCorner.X     =   Bound.Radius() / 1.73;
MaxCorner.Y     =   Bound.Radius() / 1.73;
MaxCorner.Z     =   Bound.Radius() / 1.73;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

Step.X          =   AtLeast ( 1.0, Bound.XStep ( 50 ) );
Step.Y          =   AtLeast ( 1.0, Bound.YStep ( 50 ) );


OmpParallelForSum ( sumslice, numsum )
                                        // scan a transverse sub-sampled slice
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y += Step.Y )
for ( int x = MinCorner.X; x <= MaxCorner.X; x += Step.X ) {

                                        // scan above and below within a long enough range
    for ( double z = 1.0; z <= dz; z++ ) {

        TPointDouble        point ( x, y,  z );

        normtomri.Apply ( point );

        bool        ba      = Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold;


        point.Set ( x, y, -z );

        normtomri.Apply ( point );

        bool        bb      = Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cook a penalty according to each and every cases across current cutting plane:
        double      penalty     =   ba && ! bb ? 0      // good case, no penalty
                                : ! ba &&   bb ? 2      // oh no! slice above looks smaller than slice below - we definitely don't want to go that way!
                                :                1;     // ! ba && ! bb  or  ba && bb - just regular penalty

        sumslice   += penalty;
        numsum++;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( stat )
            stat->Add ( penalty );
        }

    }


return  numsum ? sumslice / numsum : GOMaxEvaluation;;
}


//----------------------------------------------------------------------------
/*
void    TVolumeProperties::ShowProgress ()
{
TFileName           file;


                                        // Saving in relative coordinates...
//double             *greyorigin      = mridoc->GetOrigin ();
                                        // compute the transformation matrix
TMatrix44           guillotinetomriabs ( NormToMRI );

//guillotinetomriabs.SetTranslation ( guillotinetomriabs.GetTranslationX () - greyorigin[ 0 ],
//                                    guillotinetomriabs.GetTranslationY () - greyorigin[ 1 ],
//                                    guillotinetomriabs.GetTranslationZ () - greyorigin[ 2 ] );

                                        // and the inverse transform
TMatrix44           mriabstoguillotine ( guillotinetomriabs );
mriabstoguillotine.Invert ();

                                        // write the guillotine plane
sprintf ( file, "%s\\Guillotine Plane.Progress %03d.els", CrisTransfer.BaseFileName, Iteration );
ofstream    ofp ( file );
ofp << "ES01" << "\n" << 4 << "\n" << 1 << "\n" << "MRI Guillotine Plane" << "\n" << 4 << "\n" << 2 << "\n";

TPointDouble        point;
double  ext = Bound.Radius();

point.Set ( -ext, -ext, 0 );
guillotinetomriabs.Apply ( point );
ofp << point.X << " " << point.Y << " " << point.Z << " " << "A" << "\n";
point.Set ( -ext,  ext, 0 );
guillotinetomriabs.Apply ( point );
ofp << point.X << " " << point.Y << " " << point.Z << " " << "B" << "\n";
point.Set (  ext, -ext, 0 );
guillotinetomriabs.Apply ( point );
ofp << point.X << " " << point.Y << " " << point.Z << " " << "D" << "\n";
point.Set (  ext,  ext, 0 );
guillotinetomriabs.Apply ( point );
ofp << point.X << " " << point.Y << " " << point.Z << " " << "C" << "\n";
}
*/

//----------------------------------------------------------------------------
void    TVolumeProperties::CenterSlice ()
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
TPointDouble        point;
TPointDouble        first;
TPointDouble        last;

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

                                      // scan a long strip, to be sure to catch any translation/rotation
#define             extfactor       2.00
                                        // Scan Front-Back
MaxCorner.X     =   Bound.Radius();
MaxCorner.Y     =   Bound.Radius() * extfactor;
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;


                                        // first side
for ( int y = MinCorner.Y; y <= MaxCorner.Y && first.IsNull () ; y++ )
    for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
        point.Set ( x, y, 0 );

        NormToMRI.Apply ( point );

        if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
            first.Set ( 0, y, 0 );
            break;
            }
        }

                                        // second side
for ( int y = MaxCorner.Y; y >= MinCorner.Y && last.IsNull (); y-- )
    for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
        point.Set ( x, y, 0 );

        NormToMRI.Apply ( point );

        if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
            last.Set ( 0, y, 0 );
            break;
            }
        }

                                        // get the middle of the central line
point       = ( first + last ) / 2;
                                        // transform to real space
NormToMRI.Apply ( point );
                                        // save new center: this will keep the LR, fix the FB, & update the UD accordingly
Center      = point;

//Center.Truncate ();
                                        // plus update the matrix
NormToMRI.SetTranslation ( Center[ 0 ], Center[ 1 ], Center[ 2 ] );
}


//----------------------------------------------------------------------------
inline void TVolumeProperties::EvaluateTransversePlaneMatrix ( TMatrix44& normtomri )
{
                                        // Either:
                                        //   Translate + InvStandardOrient or
                                        //   Translate + InvStandardOrient + some corrective rotations
normtomri       = NormToMRI;

                                        // and set updated center
if ( HasValue( TranslationZ ) )             Center[ UpDownIndex    ]    = GetValue ( TranslationZ );
if ( HasValue( TranslationY ) )             Center[ FrontBackIndex ]    = GetValue ( TranslationY );

                                        // force set pre-translation
                                            normtomri.SetTranslation ( Center[ 0 ], Center[ 1 ], Center[ 2 ] );


if ( HasValue( RotationX ) )                normtomri.RotateX ( GetValue ( RotationX ), MultiplyRight );


if ( HasValue( Scale  ) )                   normtomri.Scale  ( GetValue ( Scale ), GetValue ( Scale ), GetValue ( Scale ), MultiplyRight );
if ( HasValue( ScaleY ) )                   normtomri.ScaleY ( GetValue ( ScaleY ), MultiplyRight );
if ( HasValue( ScaleZ ) )                   normtomri.ScaleZ ( GetValue ( ScaleZ ), MultiplyRight );
}


//----------------------------------------------------------------------------
double  TVolumeProperties::EvaluateTransversePlaneLongest ( TEasyStats* stat )
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
int                 fb1             = INT_MAX;
int                 fb2             = INT_MAX;
TMatrix44           normtomri;
TPointDouble        point;

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

if ( stat )
    stat->Reset ();

EvaluateTransversePlaneMatrix ( normtomri );

                                        // take the radius of bounding box:
                                        // - bounding box: this is where the data are, so restrict the scan
                                        // - radius: by shifts & translations we can potentially cut anyhow in the blob

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Scan Front-Back
MaxCorner.X     =   Bound.Radius() * 0.25; // a central band is enough in our case
MaxCorner.Y     =   Bound.Radius() * extfactor;
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

                                        // first side
for ( int y = MinCorner.Y; y <= MaxCorner.Y && fb1 == INT_MAX; y++ )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb1     = y;
        break;
        }
    }

                                        // no need to search for the other end?
if ( fb1 == INT_MAX )
    return  GOMaxEvaluation;

                                        // second side
for ( int y = MaxCorner.Y; y >= fb1 && fb2 == INT_MAX; y-- )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb2     = y;
        break;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // return length of slice
double              length          = fb2 - fb1 + 1;

if ( stat )
    stat->Add ( length );

return  1.0 / length;
}


//----------------------------------------------------------------------------
double  TVolumeProperties::EvaluateTransversePlaneBiggestBox ( TEasyStats* stat )
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
int                 fb1             = INT_MAX;
int                 fb2             = INT_MAX;
int                 lr1             = INT_MAX;
int                 lr2             = INT_MAX;
TMatrix44           normtomri;
TPointDouble        point;

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

if ( stat )
    stat->Reset ();

EvaluateTransversePlaneMatrix ( normtomri );

                                        // take the radius of bounding box:
                                        // - bounding box: this is where the data are, so restrict the scan
                                        // - radius: by shifts & translations we can potentially cut anyhow in the blob

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Scan Front-Back
MaxCorner.X     =   Bound.Radius() * 0.25; // a central band is enough in our case
MaxCorner.Y     =   Bound.Radius() * extfactor;
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

                                        // first side
for ( int y = MinCorner.Y; y <= MaxCorner.Y && fb1 == INT_MAX; y++ )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb1     = y;
        break;
        }
    }

                                        // no need to search for the other end?
if ( fb1 == INT_MAX )
    return  GOMaxEvaluation;

                                        // second side
for ( int y = MaxCorner.Y; y >= fb1 && fb2 == INT_MAX; y-- )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb2     = y;
        break;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Scan Left-Right
MaxCorner.X     =   Bound.Radius() * extfactor;
MaxCorner.Y     =   Bound.Radius() * 0.25; // a central band is enough in our case
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

                                        // first side
for ( int x = MinCorner.X; x <= MaxCorner.X && lr1 == INT_MAX; x++ )
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        lr1     = x;
        break;
        }
    }

                                        // no need to search for the other end?
if ( lr1 == INT_MAX )
    return  GOMaxEvaluation;

                                        // second side
for ( int x = MaxCorner.X; x >= lr1 && lr2 == INT_MAX; x-- )
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y++ ) {
    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        lr2     = x;
        break;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Return the surface estimate of the slice
                                        // length * width is good enough, as the brain is roughly round
                                        // that would no work so well with a full head, due to some squariness/bumps of the skull

double              surface         = ( fb2 - fb1 + 1 ) * ( lr2 - lr1 + 1 );

if ( stat )
    stat->Add ( surface );

return  1.0 / surface;
}


//----------------------------------------------------------------------------
double  TVolumeProperties::EvaluateTransversePlaneBiggestSurface ( TEasyStats* stat )
{
TPointInt           MinCorner;
TPointInt           MaxCorner;
int                 fb1             = INT_MAX;
int                 fb2             = INT_MAX;
int                 lr1             = INT_MAX;
int                 lr2             = INT_MAX;
TMatrix44           normtomri;
TPointDouble        point;
double              normx           = Bound.GetRadius ( LeftRightIndex );

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

if ( stat )
    stat->Reset ();

EvaluateTransversePlaneMatrix ( normtomri );

                                        // take the radius of bounding box:
                                        // - bounding box: this is where the data are, so restrict the scan
                                        // - radius: by shifts & translations we can potentially cut anyhow in the blob

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Scan Front-Back
MaxCorner.X     =   Bound.Radius() * 0.25; // a central band is enough in our case
MaxCorner.Y     =   Bound.Radius() * extfactor;
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

                                        // first side
for ( int y = MinCorner.Y; y <= MaxCorner.Y && fb1 == INT_MAX; y++ )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {

    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb1     = y;
        break;
        }
    }

                                        // no need to search for the other end?
if ( fb1 == INT_MAX )
    return  GOMaxEvaluation;

                                        // second side
for ( int y = MaxCorner.Y; y >= fb1 && fb2 == INT_MAX; y-- )
for ( int x = MinCorner.X; x <= MaxCorner.X; x++ ) {

    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        fb2     = y;
        break;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Scan Left-Right
MaxCorner.X     =   Bound.Radius() * extfactor;
MaxCorner.Y     =   Bound.Radius() * 0.25; // a central band is enough in our case
MaxCorner.Z     =   Bound.Radius() * extfactor;

MinCorner.X     = - MaxCorner.X;
MinCorner.Y     = - MaxCorner.Y;
MinCorner.Z     = - MaxCorner.Z;

                                        // first side
for ( int x = MinCorner.X; x <= MaxCorner.X && lr1 == INT_MAX; x++ )
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y++ ) {

    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        lr1     = x;
        break;
        }
    }

                                        // no need to search for the other end?
if ( lr1 == INT_MAX )
    return  GOMaxEvaluation;

                                        // second side
for ( int x = MaxCorner.X; x >= lr1 && lr2 == INT_MAX; x-- )
for ( int y = MinCorner.Y; y <= MaxCorner.Y; y++ ) {

    point.Set ( x, y, 0 );

    normtomri.Apply ( point );

    if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
        lr2     = x;
        break;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Return the surface of the slice
double              surface         = 0;
double              wsurface        = 0;

                                        // scan slice, accounting for all voxels (including below threshold) within convex hull
OmpParallelForSum ( surface, wsurface )

for ( int x = lr1; x <= lr2; x++ ) {

    int             y1          = INT_MAX;
    int             y2          = INT_MAX;
    TPointDouble    point;

    for ( int y = fb1; y <= fb2; y++ ) {

        point.Set ( x, y, 0 );

        normtomri.Apply ( point );

        if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
            y1      = y;
            break;
            }
        }

    if ( y1 == INT_MAX )
        continue;


    for ( int y = fb2; y >= fb1; y-- ) {

        point.Set ( x, y, 0 );

        normtomri.Apply ( point );

        if ( Vol.GetValueChecked ( point.X, point.Y, point.Z, InterpolateLinear ) >= Threshold ) {
            y2      = y;
            break;
            }
        }

//  surface    += y2 - y1 + 1;
                                        // "weighted surface": more weight toward sagittal line, because cutting through the temporal lobe can introduce some penalty (holes)
    double          xw          = AtLeast ( 0.0, 1 - fabs ( (double) x ) / normx );

    surface    +=   y2 - y1 + 1;
    wsurface   += ( y2 - y1 + 1 ) * xw;
    }


if ( stat )
    stat->Add ( surface );

return  1.0 / wsurface;
}


//----------------------------------------------------------------------------
double  TVolumeProperties::EvaluateTransversePlaneMNI ( TEasyStats* stat )
{
const Volume&       mnidata         = *MniSlicedoc->GetData ();
TMatrix44           normtomri;
double              normz           = MniSlicedoc->GetBounding ()->GetZExtent () / 2.0;
double              error           = 0;

//SetOvershootingOption ( interpolate, Vol.Array, LinearDim, true );

if ( stat )
    stat->Reset ();

EvaluateTransversePlaneMatrix ( normtomri );


OmpParallelForSum ( error )
                                        // scan MNI slice, in relative space
                                        // cumulate all penalties: outside of MNI slice, and outside of source volume (i.e. not restricting to a given mask)
for ( int i = 0; i < mnidata.GetLinearDim (); i++ ) {

    TPointDouble    p;
                                        // index to x,y,z, in MNI slice coordinates
    mnidata.LinearIndexToXYZ ( i, p );

                                        // MNI value, including background (0)
    double      vslice  = mnidata.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ..to Absolute (centered)
    MniSlicedoc->ToAbs ( p );

                                        // ..then to input volume
    normtomri.Apply ( p );

                                        // Source volume, including background (0)
    double      vvol    = Vol.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // difference between volume and MNI slice
    double      vdiff   = abs ( vvol - vslice );

//  error      += Square ( vdiff );
    error      += vdiff;                // regular difference seems to be doing fine


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( stat /*&& vslice && vvol*/ )
        stat->Add ( vdiff );
    }

                                        // the lowest the differences, the better
return  error;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFitPointsOnVolume::TFitPointsOnVolume ()
{
Reset ();
}


        TFitPointsOnVolume::TFitPointsOnVolume ( const TPoints& frompoints, const Volume& volume, double threshold, TMatrix44 normtomri, const TMatrix44* mriabstoguillotine )
{
Reset ();


FromPoints          = frompoints;

Vol                 = volume;
Threshold           = threshold;

if ( mriabstoguillotine )
    MriAbsToGuillotine  = *mriabstoguillotine;

Org.Set ( 0, 0, 0 );
normtomri.Apply ( Org );
                                        // set a default radius to the maximum possible
RadiusMax           = sqrt ( Square ( (double) Vol.GetDim1 () ) + Square ( (double) Vol.GetDim2 () ) + Square ( (double) Vol.GetDim3 () ) );
                                        // then refine this radius
//SetMaximumRadius ();
                                        // standard way to go back to our MRI
NormToMRI           = normtomri;
}


void    TFitPointsOnVolume::Reset ()
{
TGlobalOptimize::Reset ();

FromPoints.Reset ();

Vol                 = 0;
Threshold           = 0;

NormToMRI.SetIdentity ();
MriAbsToGuillotine.Reset ();

Org.Set ( 0, 0, 0 );
RadiusMax           = 0;
}


//----------------------------------------------------------------------------
double  TFitPointsOnVolume::Evaluate ( TEasyStats *stat )
{
TEasyStats          statdr ( (int) FromPoints );

if ( stat )
    stat->Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // minimize the least squares between of the shortest distances between points
OmpParallelFor

for ( int i = 0; i < (int) FromPoints; i++ ) {

    TPointFloat         pt ( FromPoints[ i ] );
                                        // we might have null points, in which case we skip them
    if ( pt == 0 )
        continue;

    Transform ( pt );

                                        // clip electrodes below the neck cut
    if ( MriAbsToGuillotine.IsNotNull () ) {

        TPointFloat     cutit ( pt );

        MriAbsToGuillotine.Apply ( cutit );

        if ( cutit.Z < 0 )
            continue;
        }

                                        // norm of the vectorial difference
    double              mind        = GetMinimumDistance ( pt );

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

    sumsqr      += Square ( dr );

    numsumsqr++;

    if ( stat )
        stat->Add ( dr );
    }


return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;
}


//----------------------------------------------------------------------------
                                        // find the minimum distance from a given point, radially to the surface of the volume
double  TFitPointsOnVolume::GetMinimumDistance ( TPointFloat& pt )
{
                                        // we allow ourselves to directly modify parameter pt
NormToMRI.Apply ( pt );

                                        // get direction from center to voxel
TPointFloat         v           = pt - Org;
                                        // get intersection of vector with any border:

                                        // first have an outer estimate
double              r           = Vol.WithinBoundary ( pt.X, pt.Y, pt.Z ) ? RadiusMax : v.Norm ();
                                        // (now we can do this)
v.Normalize ();
                                        // this is the point, in relative MRI coordinates, accounting with further rounding errors
TPointFloat         psurf       = Org + v * r + TPointFloat ( 0.5, 0.5, 0.5 );

                                        // then dichotomically converge to the border, finishing either inside or outside (don't care)
do {
    r          /= 2;
    psurf      += v * ( Vol.WithinBoundary ( psurf.X, psurf.Y, psurf.Z ) ? r : -r );
    } while ( r >= 0.25 );              // until close enough

                                        // check we are inside limits, for security
if ( ! Vol.WithinBoundary ( psurf.X, psurf.Y, psurf.Z ) )
    psurf      -= v;

                                        // finally converge toward center, stopping at the shape's inner border
for ( ; Vol.GetValueChecked ( psurf.X, psurf.Y, psurf.Z ) <= Threshold; psurf -= v );

                                        // returns the distance between the original point and the surface,
                                        // once corrected for the added rounding errors
                                        // it also approximates the real distance (orthogonal) to the surface by using a scalar product with the normal
TPointFloat         grad;

Vol.GetGradient ( psurf.X, psurf.Y, psurf.Z, grad );

return  ( pt - psurf + ( 0.25 + 0.5 ) ).Norm () * fabs ( v.ScalarProduct ( grad ) );
}


//----------------------------------------------------------------------------
/*
                                        // find the minimum distance from a given point, radially to the surface of the volume
double  TFitPointsOnVolume::GetMinimumDistance ( TPointFloat& pt )
{
TPointFloat         v;
TPointFloat         psurf;

                                        // we allow ourselves to directly modify parameter pt
NormToMRI.Apply ( pt );

                                        // get direction from center to voxel
v       = pt - Org;

v.Normalize ();

psurf   = Org + v * RadiusMax + TPointDouble ( 0.5, 0.5, 0.5 );


                                        // check for inside limits
for ( ; ! Vol.WithinBoundary ( psurf.X, psurf.Y, psurf.Z ); psurf -= v );

                                        // finally converge toward center, stopping at the shape's inner border
for ( ; Vol.GetValueChecked ( psurf.X, psurf.Y, psurf.Z ) <= Threshold; psurf -= v );


psurf  -= TPointDouble ( 0.5, 0.5, 0.5 );

                                        // returns the distance between the original point and the surface,
                                        // once corrected for the added rounding errors
                                        // it also approximates the real distance (orthogonal) to the surface by using a scalar product with the normal
double              mind            = ( pt - psurf + TPointDouble ( 0.25, 0.25, 0.25 ) ).Norm () * fabs ( v * Vol.GetGradientVector ( psurf ) );
                                        // returns the surface point
pt      = psurf;

return  mind;
}


void    TFitPointsOnVolume::SetMaximumRadius ()
{
                                        // !! we assume NormToMRI has not been set yet !!

int                 MinCorner[ 3 ];
int                 MaxCorner[ 3 ];
int                 Step     [ 3 ];
TPointFloat         pt;
double              maxrad          = 0;


TSuperGauge         Gauge ( "Radius Cubitus", 4 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MinCorner[ 0 ]  = 0;
MinCorner[ 1 ]  = 0;
MinCorner[ 2 ]  = 0;

MaxCorner[ 0 ]  = Vol.GetDim1 () - 1;
MaxCorner[ 1 ]  = Vol.GetDim2 () - 1;
MaxCorner[ 2 ]  = Vol.GetDim3 () - 1;

Step     [ 0 ]  =
Step     [ 1 ]  =
Step     [ 2 ]  = Vol.Step ( 60 );



                                        // scan from each side of the cube
Gauge.Next ();

for ( int x = MinCorner[ 0 ]; x <= MaxCorner[ 0 ]; x += Step[ 0 ] )
    for ( int y = MinCorner[ 1 ]; y <= MaxCorner[ 1 ]; y += Step[ 1 ] ) {

        pt.Set ( x, y, MinCorner[ 2 ] );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );

        pt.Set ( x, y, MaxCorner[ 2 ] );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );
        }


Gauge.Next ();

for ( int x = MinCorner[ 0 ]; x <= MaxCorner[ 0 ]; x += Step[ 0 ] )
    for ( int z = MinCorner[ 2 ]; z <= MaxCorner[ 2 ]; z += Step[ 2 ] ) {

        pt.Set ( x, MinCorner[ 1 ], z );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );

        pt.Set ( x, MaxCorner[ 1 ], z );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );
        }


Gauge.Next ();

for ( int y = MinCorner[ 1 ]; y <= MaxCorner[ 1 ]; y += Step[ 1 ] )
    for ( int z = MinCorner[ 2 ]; z <= MaxCorner[ 2 ]; z += Step[ 2 ] ) {

        pt.Set ( MinCorner[ 0 ], y, z );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );

        pt.Set ( MaxCorner[ 0 ], y, z );
        GetMinimumDistance ( pt );
        maxrad  = max ( (double) ( pt - Org ).Norm (), maxrad );
        }

Gauge.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // adjust radius to its minimum
DBGV2 ( RadiusMax, maxrad, "Bigradius  Smallradius" );

RadiusMax           = maxrad * 1.01 + 2;

}
*/

//----------------------------------------------------------------------------
inline void     TFitPointsOnVolume::Transform ( TPointFloat& p )
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

if ( HasValue( Scale  ) )                   p      *= GetValue ( Scale  );
if ( HasValue( ScaleX ) )                   p.X    *= GetValue ( ScaleX );
if ( HasValue( ScaleY ) )                   p.Y    *= GetValue ( ScaleY );
if ( HasValue( ScaleZ ) )                   p.Z    *= GetValue ( ScaleZ );
}


void    TFitPointsOnVolume::GetMatrix ( TMatrix44& m )
{
m.SetIdentity ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations
if ( HasValue( TranslationX ) )             m.TranslateX ( GetValue ( TranslationX ), MultiplyLeft );
if ( HasValue( TranslationY ) )             m.TranslateY ( GetValue ( TranslationY ), MultiplyLeft );
if ( HasValue( TranslationZ ) )             m.TranslateZ ( GetValue ( TranslationZ ), MultiplyLeft );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    if ( HasValue( RotationX ) )            m.RotateX ( GetValue ( RotationX ), MultiplyLeft );
    if ( HasValue( RotationY ) )            m.RotateY ( GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationZ ) )            m.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasValue( Scale  ) )                   m.Scale  ( GetValue ( Scale ), GetValue ( Scale ), GetValue ( Scale ), MultiplyLeft );
if ( HasValue( ScaleX ) )                   m.ScaleX ( GetValue ( ScaleX ), MultiplyLeft );
if ( HasValue( ScaleY ) )                   m.ScaleY ( GetValue ( ScaleY ), MultiplyLeft );
if ( HasValue( ScaleZ ) )                   m.ScaleZ ( GetValue ( ScaleZ ), MultiplyLeft );


}


void    TFitPointsOnVolume::Transform ( TPoints& points )
{
TPointFloat         p;

for ( int i = 0; i < points.GetNumPoints (); i++ ) {

    p.Set ( points[ i ] );

    Transform ( p );

    points[ i ] = p;

//  Transform ( points[ i ] );
    }
}


//----------------------------------------------------------------------------
/*
void    TFitPointsOnVolume::ShowProgress ()
{
TFileName           file;

TPoints             points          = *FromPoints;

Transform ( points );


if ( Iteration < 100 ) {

    sprintf ( file, "%s\\Points On Volume.Progress %03d."FILEEXT_XYZ, CrisTransfer.BaseFileName, Iteration );

    points.WriteFile ( file );
    }
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFitVolumeOnVolume::TFitVolumeOnVolume ()
{
FromVolumesSmoothed = 0;

Reset ();
}


        TFitVolumeOnVolume::TFitVolumeOnVolume ( const TVolumeDoc*  fromvolume, RemapIntensityType      fromremap,  TMatrix44*  fromrel_fromabs,
                                                 const TVolumeDoc*  tovolume,   RemapIntensityType      toremap,    TMatrix44*  torel_toabs,
                                                 FitVolumeType      flags )
{
FromVolumesSmoothed = 0;

SetFitVolumeOnVolume    (   fromvolume,     fromremap,  fromrel_fromabs,
                            tovolume,       toremap,    torel_toabs,
                            flags 
                        );
}


//----------------------------------------------------------------------------
void    TFitVolumeOnVolume::SetFitVolumeOnVolume    (   const TVolumeDoc*   fromvolume, RemapIntensityType      fromremap,   TMatrix44*  fromrel_fromabs,
                                                        const TVolumeDoc*   tovolume,   RemapIntensityType      toremap,     TMatrix44*  torel_toabs,
                                                        FitVolumeType       flags )
{
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save flags
Flags       = flags;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get normalization matrices:

                                        // absolute voxel space to relative voxel space
GetNormalizationTransform ( fromvolume,         0, 
                            false,              FromCenter,   
                            &FromAbs_FromRel,   0            
                            );

                                        // overriding normalization, which can now contain more than going to RAS
if ( fromrel_fromabs ) {

    FromAbs_FromRel = TMatrix44 ( *fromrel_fromabs ).Invert ();

    FromCenter.Set  ( FromAbs_FromRel.GetTranslationX (), FromAbs_FromRel.GetTranslationY (), FromAbs_FromRel.GetTranslationZ () );
    }

//FromRel_FromAbs   = TMatrix44 ( FromAbs_FromRel ).Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // relative voxel space to absolute voxel space
GetNormalizationTransform ( tovolume,           0, 
                            false,              ToCenter,     
                            0,                  &ToRel_ToAbs 
                            );

                                        // overriding normalization, which can now contain more than going to RAS
if ( torel_toabs ) {

    ToRel_ToAbs     = *torel_toabs;

    TMatrix44           ToAbs_ToRel     = TMatrix44 ( ToRel_ToAbs ).Invert ();

    ToCenter.Set  ( ToAbs_ToRel.GetTranslationX (), ToAbs_ToRel.GetTranslationY (), ToAbs_ToRel.GetTranslationZ () );
    }

//ToAbs_ToRel     = TMatrix44 ( ToRel_ToAbs ).Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // result will have the same size, units & orientation as the target
FromVoxelSize   = fromvolume->GetVoxelSize ();
ToVoxelSize     = tovolume  ->GetVoxelSize ();

fromvolume->OrientationToString ( FromOrientation );
tovolume  ->OrientationToString ( ToOrientation   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate & copy data
FromVolume          = *fromvolume->GetData ();
ToVolume            = *tovolume  ->GetData ();

//FromVolumeF.Resize ( fromvolume->GetData ()->GetDim ( 0 ), fromvolume->GetData ()->GetDim ( 1 ), fromvolume->GetData ()->GetDim ( 2 ) );
//ToVolumeF  .Resize ( tovolume  ->GetData ()->GetDim ( 0 ), tovolume  ->GetData ()->GetDim ( 1 ), tovolume  ->GetData ()->GetDim ( 2 ) );

                                        // using the gradient also works, but not really better
//FctParams           p;
//p ( FilterParamDiameter   )     = 0;
//FromVolume.Filter ( FilterTypeGradient,   p, true );
//ToVolume  .Filter ( FilterTypeGradient,   p, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set masks from volumes
SetMask ( FromVolume, FromMask );
SetMask ( ToVolume,   ToMask   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate cache for the From volume (+- 2 step values around current smoothing)
#define             MaxVolumeCache      5

FromVolumesSmoothed = new TCacheVolumes<MriType> ( MaxVolumeCache );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert values, like histogram equalization or binarization
FromRemap           = fromremap;
ToRemap             = toremap;

                                        // not needed if smoothing?
RemapVolume ( FromVolume, FromRemap );
RemapVolume ( ToVolume,   ToRemap   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // original boundaries
//ToBound       = *tovolume  ->GetBounding ();
//FromBound     = *fromvolume->GetBounding ();

                                        // !boundaries from actual masks, which could be bigger than original data!
ToBound         = TBoundingBox<double> ( ToMask,   false, 1 );
FromBound       = TBoundingBox<double> ( FromMask, false, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save these data for the inner loop
FromNormExt     = TPointDouble ( FromBound.GetXExtent () / 2, FromBound.GetYExtent () / 2, FromBound.GetZExtent () / 2 );
}


void    TFitVolumeOnVolume::Reset ()
{
TGlobalOptimize::Reset ();


FromVolume      .ResetMemory ();
FromMask        .ResetMemory ();
FromRemap       = RemapIntensityNone;
FromBound       .Reset ();
FromCenter      .Reset ();
FromVoxelSize   .Reset ();
ClearString ( FromOrientation, 4 );

ToVolume        .ResetMemory ();
ToMask          .ResetMemory ();
ToRemap         = RemapIntensityNone;
ToBound         .Reset ();
ToCenter        .Reset ();
ToVoxelSize     .Reset ();
ClearString ( ToOrientation, 4 );

Flags           = FitVolumeNone;

FromAbs_FromRel .SetIdentity ();
//FromRel_FromAbs .SetIdentity ();
//ToAbs_ToRel     .SetIdentity ();
ToRel_ToAbs     .SetIdentity ();

ToAbs_FromAbs   .SetIdentity ();
FromAbs_ToAbs   .SetIdentity ();
ToRel_FromRel   .SetIdentity ();
FromRel_ToRel   .SetIdentity ();

if ( FromVolumesSmoothed != 0 )
    delete  FromVolumesSmoothed;
FromVolumesSmoothed = 0;

ToVolumeSmoothed.DeallocateMemory ();
ToVolumeSmoothedStep= INT_MAX;

FromNormExt   .Reset ();
}


//----------------------------------------------------------------------------
void    TFitVolumeOnVolume::SetMask ( Volume& volume, Volume& mask )
{
FctParams           p;

                                        // 1) threshold and binarize
mask        = volume;
p ( FilterParamToMaskThreshold )     = volume.GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = volume.MinSize () >= 3;   // no carving for slices

mask.Filter ( FilterTypeToMask, p );

                                        // Not used for the moment, it seems we have good convergence without this
//if ( IsEqualSizes ( Flags ) ) {
//                                        // 2) with the mask bigger than the data, it will add this extra layer as a border constrain
//    p ( FilterParamDiameter )     = 4;
//    mask.Filter ( FilterTypeDilate, p );
//    }
}


//----------------------------------------------------------------------------
void    TFitVolumeOnVolume::RemapVolume ( Volume& volume, RemapIntensityType remap )
{
FctParams           p;

switch ( remap ) {

    case    RemapIntensityNone:
        ;

    case    RemapIntensityEqualize:

        volume.Filter ( FilterTypeHistoEqual,   p, false );
        break;

    case    RemapIntensityEqualizeBrain:

        volume.Filter ( FilterTypeHistoEqualBrain,   p, false );
        break;

    case    RemapIntensityRank:

        p ( FilterParamThreshold )     = volume.GetBackgroundValue ();
        volume.Filter ( FilterTypeRank, p, false );
        break;

    case    RemapIntensityRankRamp:

        p ( FilterParamThreshold )     = volume.GetBackgroundValue ();
        volume.Filter ( FilterTypeRankRamp, p, false );
        break;

    case    RemapIntensityBinarize:

        p ( FilterParamThresholdMin )     = volume.GetBackgroundValue ();
        p ( FilterParamThresholdMax )     = FLT_MAX;
        p ( FilterParamThresholdBin )     = 1;
        volume.Filter ( FilterTypeThresholdBinarize, p );
        break;

    case    RemapIntensityMask:

//      p ( FilterParamToMaskThreshold )     = volume.GetBackgroundValue ();
//      p ( FilterParamToMaskNewValue  )     = 1;
//      p ( FilterParamToMaskCarveBack )     = true;                 // carving
//      volume.Filter ( FilterTypeToMask, p );

//      volume      = FromMask;

        p ( FilterParamThresholdMin )     = volume.GetBackgroundValue ();
        p ( FilterParamThresholdMax )     = FLT_MAX;
        p ( FilterParamThresholdBin )     = 1;
        volume.Filter ( FilterTypeThresholdBinarize, p, false );
        break;

    case    RemapIntensityInvert:

        volume.Filter ( FilterTypeRevert, p, false );
        break;

/*                                        // normalize volume
double              maxv;

maxv        = fromvolume->GetMaxValue ();
FromVolume /= NonNull ( maxv );

//for ( int x = 0; x < FromVolume.GetDim1 (); x++ )
//for ( int y = 0; y < FromVolume.GetDim2 (); y++ )
//for ( int z = 0; z < FromVolume.GetDim3 (); z++ )
//    FromVolume ( x, y, z ) /= NonNull ( maxv );*/

    } // switch remap
}


//----------------------------------------------------------------------------
                                        // estimate the equivalent target distance in the source space
double  TFitVolumeOnVolume::DistanceTargetToSource ( double dt )
{
                                        // compute 3 vectors, one in each dimensions, from the ToCenter
TPointDouble        f0 ( ToCenter                                          );
TPointDouble        f1 ( ToCenter.X + dt, ToCenter.Y,      ToCenter.Z      );
TPointDouble        f2 ( ToCenter.X,      ToCenter.Y + dt, ToCenter.Z      );
TPointDouble        f3 ( ToCenter.X,      ToCenter.Y,      ToCenter.Z + dt );
                                        // going to From space
//TransformTargetToSource ( f0 );
//TransformTargetToSource ( f1 );
//TransformTargetToSource ( f2 );
//TransformTargetToSource ( f3 );

ToRel_FromRel.Apply ( f0 );
ToRel_FromRel.Apply ( f1 );
ToRel_FromRel.Apply ( f2 );
ToRel_FromRel.Apply ( f3 );

                                        // get the distance for each transformed axis
f1     -= f0;
f2     -= f0;
f3     -= f0;
                                        // merge the three directions (arbitrary formula)
return  ( f1.Norm () + f2.Norm () + f3.Norm () ) / 3.0;
}


//----------------------------------------------------------------------------
double  TFitVolumeOnVolume::Evaluate ( TEasyStats *stat )
{
TPointInt           ToFirst;
TPointInt           ToLast;
int                 ToStep;
int                 ToSmooth;
TPointInt           FromFirst;
TPointInt           FromLast;
int                 FromStep;
int                 FromSmooth;


EvaluateMatrices ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // constant stepping
ToStep        = 1;
//ToStep          = AtLeast ( 1, Truncate ( ToBound.Step ( 128, false ) ) );

                                        // shift the first position so that by consecutive ToStep jumps, we will land on the center
ToFirst.X       = ToBound.XMin () + ( Truncate ( ToCenter.X - ToBound.XMin () ) % ToStep );
ToFirst.Y       = ToBound.YMin () + ( Truncate ( ToCenter.Y - ToBound.YMin () ) % ToStep );
ToFirst.Z       = ToBound.ZMin () + ( Truncate ( ToCenter.Z - ToBound.ZMin () ) % ToStep );

                                        // max # of jumps from modified ToFirst, while remaining inside boundary limits
ToLast .X       = ToFirst.X + TruncateTo ( ToBound.XMax () - ToFirst.X, ToStep );
ToLast .Y       = ToFirst.Y + TruncateTo ( ToBound.YMax () - ToFirst.Y, ToStep );
ToLast .Z       = ToFirst.Z + TruncateTo ( ToBound.ZMax () - ToFirst.Z, ToStep );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // constant stepping, using the same sampling as in target space
FromStep      = 1;
//FromStep        = AtLeast ( 1, Truncate ( FromBound.Step ( 128, false ) ) );
//FromStep      = AtLeast ( 1, Truncate ( DistanceTargetToSource ( ToStep ) ) );

                                        // shift the first position so that by consecutive FromStep jumps, we will land on the center
FromFirst.X     = FromBound.XMin () + ( Truncate ( FromCenter.X - FromBound.XMin () ) % FromStep );
FromFirst.Y     = FromBound.YMin () + ( Truncate ( FromCenter.Y - FromBound.YMin () ) % FromStep );
FromFirst.Z     = FromBound.ZMin () + ( Truncate ( FromCenter.Z - FromBound.ZMin () ) % FromStep );

                                        // max # of jumps from modified FromFirst, while remaining inside boundary limits
FromLast .X     = FromFirst.X + TruncateTo ( FromBound.XMax () - FromFirst.X, FromStep );
FromLast .Y     = FromFirst.Y + TruncateTo ( FromBound.YMax () - FromFirst.Y, FromStep );
FromLast .Z     = FromFirst.Z + TruncateTo ( FromBound.ZMax () - FromFirst.Z, FromStep );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some filtering
ToSmooth        = ToStep;
//ToSmooth        = 2 * ToStep;           // best results
//ToSmooth        = ToRemap == RemapIntensityBinarize || ToRemap == RemapIntensityMask ? 0 : 2 * ToStep;           // best results

                                        // estimate the From step equivalent to the To step
FromSmooth      = ToSmooth;
//FromSmooth      = 2 * FromStep;
//FromSmooth      = FromRemap == RemapIntensityBinarize || FromRemap == RemapIntensityMask ? 0 : max ( 1, Round ( DistanceTargetToSource ( ToSmooth ) ) );
//FromSmooth      = AtLeast ( 1, Round ( DistanceTargetToSource ( ToSmooth ) ) );

//if ( VkQuery () ) DBGV4 ( FromStep, FromSmooth, ToStep, ToSmooth, "FromStep, FromSmooth, ToStep, ToSmooth" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                tosmoothing     = false; // true;
bool                fromsmoothing   = false; // true;
bool                reloadfromvolume= fromsmoothing;
                                        // With smoothing:
                                        // - work with local filtered copies
                                        // - both volumes can be downsampled differently
Volume*             tovolume        = ! tosmoothing     ? &ToVolume     
                                                        : &ToVolumeSmoothed;

Volume*             fromvolume      = ! fromsmoothing   ? &FromVolume
                                                          // get a cache slot for this smoothing factor
                                                        : FromVolumesSmoothed->GetCache ( FromVolume.GetDim1 (), FromVolume.GetDim2 (), FromVolume.GetDim3 (), 
                                                                                          FromSmooth, reloadfromvolume );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Any To smoothing?

#define             StepToGaussianSmooth(S)     (2 * (S))
                                        // odd kernel size
//#define             StepToGaussianSmooth(S)     (2 * (S) + 1)


if ( tosmoothing && ToSmooth != ToVolumeSmoothedStep ) {
                                        // use copies, because filtering ahead
    ToVolumeSmoothed    = ToVolume;

    ToVolumeSmoothedStep= ToSmooth;


    FctParams           p;

    p ( FilterParamDiameter )     = StepToGaussianSmooth ( ToVolumeSmoothedStep );

    ToVolumeSmoothed.Filter ( FilterTypeFastGaussian,   p, false );

    if ( ToRemap != RemapIntensityInvert )  // don't re-do, if done at initialization

        RemapVolume ( ToVolumeSmoothed, ToRemap   );

                                        // updating mask - maybe a tiny bit better
//  SetMask ( ToVolumeSmoothed, ToMask );
    } // new filtered volume


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // should reload the data, according to caches?
if ( fromsmoothing && reloadfromvolume ) {
                                        // set a copy
    fromvolume->Insert ( FromVolume );


    FctParams           p;

    p ( FilterParamDiameter )     = StepToGaussianSmooth ( FromSmooth );

    fromvolume->Filter ( FilterTypeFastGaussian,    p, false );


    if ( FromRemap != RemapIntensityInvert )  // don't re-do, if done at initialization

        RemapVolume ( *fromvolume, FromRemap );

                                    // updating mask - maybe a tiny bit better
//  SetMask ( *fromvolume, FromMask );
    } // new filtered volume


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//#define             maxhv       512
//TArray2<double>     ProbToFrom ( maxhv, maxhv );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( stat )
    stat->Reset ();


InterpolationType   volumeinterpolation     = InterpolateLinear;

double              sumsqr          = 0;
int                 numsumsqr       = 0;

                                        // Scan the Target volume
if ( IsEqualSizes ( Flags ) || IsSourceBigger ( Flags ) ) {

    SetOvershootingOption    ( volumeinterpolation, fromvolume->GetArray (), fromvolume->GetLinearDim (), true );


    OmpParallelForSum ( sumsqr, numsumsqr )
                                        // scan from the target space, going backward to the source space
                                        // test all points within target mask
    for ( int x = ToFirst.X; x <= ToLast.X; x += ToStep ) {

        UpdateApplication;

        for ( int y = ToFirst.Y; y <= ToLast.Y; y += ToStep )
        for ( int z = ToFirst.Z; z <= ToLast.Z; z += ToStep ) {
                                        // should be part of target
            if ( ! ToMask ( x, y, z ) )
                continue;

                                        // target to source
            TPointDouble        pf  ( x, y, z );
    //      TransformTargetToSource ( pf );
            ToRel_FromRel.Apply ( pf );


            double              fromv       = fromvolume->GetValueChecked ( pf.X, pf.Y, pf.Z, volumeinterpolation );
            double              tov         = tovolume  ->GetValue        (    x,    y,    z );

                                        // 1 patch interactive intensity remapping
            if ( HasValue( FitVolumeFromIntensityRescale ) )
                fromv  *= exp ( GetValue ( FitVolumeFromIntensityRescale ) );
    //          fromv   = ( fromv - GetValue ( FitVolumeFromIntensityOffset ) ) * exp ( GetValue ( FitVolumeFromIntensityRescale ) );

                                        // !which formula is used will affect the quality intervals!
//          double              dv          = tov - fromv;
            double              dv          = RelativeDifference ( tov, fromv );


    //      ProbToFrom ( tov, fromv )++;
    //      ProbToFrom ( Clip ( tov, 0.0, maxhv - 1.0 ), Clip ( fromv, 0.0, maxhv - 1.0 ) )++;


            sumsqr     += Square ( dv );
            numsumsqr++;


            if ( stat )
                stat->Add ( fabs ( dv ) );
            } // for y, z
        } // for x
    } // if IsEqualSizes || IsSourceBigger


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scan the Source volume

if ( IsEqualSizes ( Flags ) || IsTargetBigger ( Flags ) ) {

    SetOvershootingOption    ( volumeinterpolation, tovolume  ->GetArray (), tovolume  ->GetLinearDim (), true );


    OmpParallelForSum ( sumsqr, numsumsqr )
                                        // scan from the source space, going forward to the target space
                                        // scan remaining points not inside target mask but still inside source mask
    for ( int x = FromFirst.X; x <= FromLast.X; x += FromStep ) {

        UpdateApplication;

        for ( int y = FromFirst.Y; y <= FromLast.Y; y += FromStep )
        for ( int z = FromFirst.Z; z <= FromLast.Z; z += FromStep ) {
                                        // should be part of source
            if ( ! FromMask ( x, y, z ) )
                continue;

                                        // source to target
            TPointDouble        pf ( x, y, z );
    //      TransformSourceToTarget ( pf );
            FromRel_ToRel.Apply ( pf );

                                        // should not be already included in To
            if ( IsEqualSizes ( Flags ) && ToMask.GetValueChecked ( pf ) )
                continue;


            double              fromv       = fromvolume->GetValue        (    x,    y,    z );
            double              tov         = tovolume  ->GetValueChecked ( pf.X, pf.Y, pf.Z, volumeinterpolation );

                                        // 1 patch interactive intensity remapping
            if ( HasValue( FitVolumeFromIntensityRescale ) )
                fromv  *= exp ( GetValue ( FitVolumeFromIntensityRescale ) );
    //          fromv   = ( fromv - GetValue ( FitVolumeFromIntensityOffset ) ) * exp ( GetValue ( FitVolumeFromIntensityRescale ) );

                                        // !which formula is used will affect the quality intervals!
//          double              dv          = tov - fromv;
            double              dv          = RelativeDifference ( tov, fromv );


    //      ProbToFrom ( tov, fromv )++;
    //      ProbToFrom ( Clip ( tov, 0.0, maxhv - 1.0 ), Clip ( fromv, 0.0, maxhv - 1.0 ) )++;


            sumsqr     += Square ( dv );
            numsumsqr++;


            if ( stat )
                stat->Add ( fabs ( dv ) );
            } // for y, z
        } // for x
    } // if IsEqualSizes || IsTargetBigger


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Scan the Source volume, but from the Target space

if ( IsEqualSizes ( Flags ) || IsTargetBigger ( Flags ) ) {

                                        // guess the Source transformed limits in the Target space
    FromFirst.Set ( INT_MAX, INT_MAX, INT_MAX );
    FromLast .Set ( INT_MIN, INT_MIN, INT_MIN );

                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMin (), FromBound.YMin (), FromBound.ZMin () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMin (), FromBound.YMin (), FromBound.ZMax () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMin (), FromBound.YMax (), FromBound.ZMin () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMin (), FromBound.YMax (), FromBound.ZMax () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMax (), FromBound.YMin (), FromBound.ZMin () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMax (), FromBound.YMin (), FromBound.ZMax () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMax (), FromBound.YMax (), FromBound.ZMin () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );
                                        // convert each corner to get a wide bounding box
    pf.Set ( FromBound.XMax (), FromBound.YMax (), FromBound.ZMax () );
//    TransformSourceToTarget ( pf );
    FromRel_ToRel.Apply ( pf );
    FromFirst.X     = min ( pf.X, (double) FromFirst.X );
    FromFirst.Y     = min ( pf.Y, (double) FromFirst.Y );
    FromFirst.Z     = min ( pf.Z, (double) FromFirst.Z );
    FromLast.X      = max ( pf.X, (double) FromLast .X );
    FromLast.Y      = max ( pf.Y, (double) FromLast .Y );
    FromLast.Z      = max ( pf.Z, (double) FromLast .Z );

                                        // we work in the Target space, so use the same stepping
    FromStep    = ToStep;


                                        // !scan from the target space!
                                        // scan remaining points not inside target mask but still inside source mask
    for ( int x = FromFirst.X; x <= FromLast.X; x += FromStep )
    for ( int y = FromFirst.Y; y <= FromLast.Y; y += FromStep )
    for ( int z = FromFirst.Z; z <= FromLast.Z; z += FromStep ) {

        UpdateApplication;

                                        // already done?
        if ( IsEqualSizes ( Flags ) && ToMask.GetValueChecked ( x, y, z ) )
            continue;

                                        // target to source
        pf.Set ( x, y, z );
//      TransformTargetToSource ( pf );
        ToRel_FromRel.Apply ( pf );

                                        // should be included in From
        if ( ! FromMask.GetValueChecked ( pf ) )
            continue;


        fromv       = fromvolume->GetValueChecked ( pf.X, pf.Y, pf.Z, volumeinterpolation );
        tov         = tovolume  ->GetValueChecked (    x,    y,    z );

                                        // 1 patch interactive intensity remapping
        if ( HasValue( FitVolumeFromIntensityRescale ) )
            fromv  *= GetValue ( FitVolumeFromIntensityRescale );


        dv          = tov - fromv;

        sumsqr     += dv * dv;
        numsumsqr++;


        if ( stat )
            stat->Add ( fabs ( dv ) );
        }
    } // if IsEqualSizes || IsTargetBigger
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  numsumsqr ? sumsqr / numsumsqr : GOMaxEvaluation;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // cross probabilities scanning
TEasyStats          pabstat;
//TEasyStats          pabstat ( maxhv );
double              errhisto        = 0;

/*                                      // estimating intensity remapping
double              wsum;
double              w;
double              maxp;
TArray1<double>     TovToFromv ( maxhv );
TArray1<double>     FromvToTov ( maxhv );


for ( int i = 0; i < maxhv; i++ ) {

//    wsum    = w     = 0;
    maxp    = -1;

//    for ( int j = 0; j < maxhv; j++ )
    for ( int j = 10; j < maxhv; j++ )
        if ( ProbToFrom ( i, j ) )
            {
//            wsum   += j * ProbToFrom ( i, j );
//            w      += ProbToFrom ( i, j );

            if ( ProbToFrom ( i, j ) > maxp ) {
                maxp            = ProbToFrom ( i, j );
                TovToFromv[ i ] = j;
                }
            }

//    TovToFromv[ i ]   = wsum / NonNull ( w );
    }


for ( int j = 0; j < maxhv; j++ ) {

//    wsum    = w     = 0;
    maxp    = -1;

//    for ( int i = 0; i < maxhv; i++ )
    for ( int i = 10; i < maxhv; i++ )
        if ( ProbToFrom ( i, j ) )
            {
//            wsum   += i * ProbToFrom ( i, j );
//            w      += ProbToFrom ( i, j );

            if ( ProbToFrom ( i, j ) > maxp ) {
                maxp            = ProbToFrom ( i, j );
                FromvToTov[ i ] = i;
                }
            }

//    FromvToTov[ j ]   = wsum / NonNull ( w );
    }


p ( FilterParamDiameter )     = 5;
TovToFromv.Filter ( FilterTypeMedian,   p );
FromvToTov.Filter ( FilterTypeMedian,   p );
p ( FilterParamDiameter )     = 3;
TovToFromv.Filter ( FilterTypeGaussian, p );
FromvToTov.Filter ( FilterTypeGaussian, p );
* /


for ( int i = 0; i < maxhv; i++ ) {

    pabstat.Reset ();

    for ( int j = 0; j < maxhv; j++ )
        if ( ProbToFrom ( i, j ) )
            pabstat.Add ( ProbToFrom ( i, j ) );

//    errhisto  += pabstat.SD ();
    errhisto  += pabstat.Variance ();    // interesting
//    errhisto  += Square ( pabstat.SD () * pabstat.Mean () );   // more interesting
//    errhisto  += Square ( pabstat.CoV () );
    }


for ( int j = 0; j < maxhv; j++ ) {

    pabstat.Reset ();

    for ( int i = 0; i < maxhv; i++ )
        if ( ProbToFrom ( i, j ) )
            pabstat.Add ( ProbToFrom ( i, j ) );

//    errhisto  += pabstat.SD ();
    errhisto  += pabstat.Variance ();
//    errhisto  += Square ( pabstat.SD () * pabstat.Mean () );
//    errhisto  += Square ( pabstat.CoV () );
    }


static              count           = -1;
count++;
TFileName           _file;

if ( ! ( Iteration % 5 ) ) {
                                        // for display, log it
    for ( int i= 0; i < (int) ProbToFrom; i++ )
        ProbToFrom.GetValue ( i )  = log ( 1 + ProbToFrom.GetValue ( i ) );

    sprintf ( _file, "e:\\Data\\PAB.CrossProb.Progress %03d.%s", count, "ep" );
    ProbToFrom.WriteFile ( _file );

//    sprintf ( _file, "e:\\Data\\PAB.TovToFromv.Progress %03d.%s", count, "ep" );
//    TovToFromv.WriteFile ( _file );
//
//    sprintf ( _file, "e:\\Data\\PAB.FromvToTov.Progress %03d.%s", count, "ep" );
//    FromvToTov.WriteFile ( _file );
    }


return  numsumsqr ? errhisto * sumsqr / numsumsqr : GOMaxEvaluation;
//return  errhisto;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
TVector<double>     pa ( 256 );
TVector<double>     pb ( 256 );

for ( int i = 0; i < (int) pa; i++ )
    for ( int j = 0; j < (int) pb; j++ )
        pa[ i ]    += ProbToFrom ( i, j );

pa.Normalize ();

for ( int j = 0; j < (int) pb; j++ )
    for ( int i = 0; i < (int) pa; i++ )
        pb[ j ]    += ProbToFrom ( i, j );

pb.Normalize ();


double              normpab         = 0;

for ( int i= 0; i < (int) ProbToFrom; i++ )
    normpab    += ProbToFrom.GetValue ( i );

ProbToFrom    /= normpab;


double              hab             = 0;

for ( int i = 0; i < (int) pa; i++ )
    for ( int j = 0; j < (int) pb; j++ )
//        hab    += ProbToFrom ( i, j ) ? ProbToFrom ( i, j ) * log ( ProbToFrom ( i, j ) / NonNull ( pa[ i ] * pb[ j ] ) ) : 0;
        hab    += pa[ i ] + pb[ j ] - ProbToFrom ( i, j );


return  1 / hab;
*/

/*
TEasyStats          pabstat;

//pabstat.Add ( ProbToFrom );

for ( int i= 0; i < (int) ProbToFrom; i++ )
    if ( ProbToFrom.GetValue ( i ) )
        pabstat.Add ( ProbToFrom.GetValue ( i ) );


//if ( VkQuery () )  pabstat.Show ( "pabstat" );
if ( Iteration < 50 ) {
    sprintf ( _file, "e:\\Data\\PAB stat.Progress %03d.%03d.%s", count, Iteration, "txt" );
    pabstat.WriteFileVerbose ( _file );
    }


return  pabstat.SD ();
//return  1 / pabstat.SD ();
//return  pabstat.SignalToNoiseRatio ();
*/
}


//----------------------------------------------------------------------------

void    TFitVolumeOnVolume::EvaluateMatrices ()
{
                                        // compute the transformation matrix
ToAbs_FromAbs.SetIdentity ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // global scaling first
if ( HasValue( Scale  ) )                   ToAbs_FromAbs.Scale  ( GetValue ( Scale ), GetValue ( Scale ), GetValue ( Scale ), MultiplyLeft );
if ( HasValue( ScaleX ) )                   ToAbs_FromAbs.ScaleX ( GetValue ( ScaleX ), MultiplyLeft );
if ( HasValue( ScaleY ) )                   ToAbs_FromAbs.ScaleY ( GetValue ( ScaleY ), MultiplyLeft );
if ( HasValue( ScaleZ ) )                   ToAbs_FromAbs.ScaleZ ( GetValue ( ScaleZ ), MultiplyLeft );

//if ( HasValue( Scale  ) )                   DBGV ( GetValue ( Scale  ),  HasValue( Scale  )" );
//if ( HasValue( ScaleX ) )                   DBGV3 ( GetValue ( ScaleX ), GetValue ( ScaleY ), GetValue ( ScaleZ ),  HasValue( ScaleX ), GetValue ( ScaleY ), GetValue ( ScaleZ )" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) )                ToAbs_FromAbs.RotateX ( GetValue ( RotationX ), MultiplyLeft );
if ( HasValue( RotationY ) )                ToAbs_FromAbs.RotateY ( GetValue ( RotationY ), MultiplyLeft );
if ( HasValue( RotationZ ) )                ToAbs_FromAbs.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );

//if ( HasValue( RotationX ) )                   DBGV3 ( GetValue ( RotationX ), GetValue ( RotationY ), GetValue ( RotationZ ), "GetValue ( RotationX ), GetValue ( RotationY ), GetValue ( RotationZ )" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations
if ( HasValue( TranslationX ) )             ToAbs_FromAbs.TranslateX ( GetValue ( TranslationX ), MultiplyLeft );
if ( HasValue( TranslationY ) )             ToAbs_FromAbs.TranslateY ( GetValue ( TranslationY ), MultiplyLeft );
if ( HasValue( TranslationZ ) )             ToAbs_FromAbs.TranslateZ ( GetValue ( TranslationZ ), MultiplyLeft );

//if ( HasValue( TranslationX ) )                   DBGV3 ( GetValue ( TranslationX ), GetValue ( TranslationY ), GetValue ( TranslationZ ), "GetValue ( TranslationX ), GetValue ( TranslationY ), GetValue ( TranslationZ )" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temporarily adjust origin
                                            ToAbs_FromAbs.Translate ( HasValue( FitVolumeShearShiftX ) ? GetValue ( FitVolumeShearShiftX ) : 0,
                                                                      HasValue( FitVolumeShearShiftY ) ? GetValue ( FitVolumeShearShiftY ) : 0,
                                                                      HasValue( FitVolumeShearShiftZ ) ? GetValue ( FitVolumeShearShiftZ ) : 0 , MultiplyLeft );

if ( HasValue( FitVolumeNormCenterRotateX ) )   ToAbs_FromAbs.RotateX ( GetValue ( FitVolumeNormCenterRotateX ), MultiplyLeft );
if ( HasValue( FitVolumeNormCenterRotateZ ) )   ToAbs_FromAbs.RotateZ ( GetValue ( FitVolumeNormCenterRotateZ ), MultiplyLeft );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // shearing

if ( HasValue( FitVolumeShearXtoY ) )       ToAbs_FromAbs.ShearX ( GetValue ( FitVolumeShearXtoY ), 0, MultiplyLeft );
if ( HasValue( FitVolumeShearXtoZ ) )       ToAbs_FromAbs.ShearX ( 0, GetValue ( FitVolumeShearXtoZ ), MultiplyLeft );
if ( HasValue( FitVolumeShearYtoX ) )       ToAbs_FromAbs.ShearY ( GetValue ( FitVolumeShearYtoX ), 0, MultiplyLeft );
if ( HasValue( FitVolumeShearYtoZ ) )       ToAbs_FromAbs.ShearY ( 0, GetValue ( FitVolumeShearYtoZ ), MultiplyLeft );
if ( HasValue( FitVolumeShearZtoX ) )       ToAbs_FromAbs.ShearZ ( GetValue ( FitVolumeShearZtoX ), 0, MultiplyLeft );
if ( HasValue( FitVolumeShearZtoY ) )       ToAbs_FromAbs.ShearZ ( 0, GetValue ( FitVolumeShearZtoY ), MultiplyLeft );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // perspective
/*                                      // technically working, but the warping is too strong, and messes up the zoom factor too much
if ( HasValue( FitVolumePerspectiveZtoXYDelta ) && GetValue ( FitVolumePerspectiveZtoXYDelta ) != 0 ) {
                                        // we need an absolute size
    double              radiusz         = FromNormExt.Z;
    double              radiusxy        = ( FromNormExt.X + FromNormExt.Y ) / 2;
                                        // force shift to be negative, as the perspective applies to the negative part of the axis
    double              shift           = - radiusz * radiusxy / AbsNonNull ( GetValue ( FitVolumePerspectiveZtoXYDelta ) );
                                        // !near is the one closer to 0!
    double              perspnear       = shift + radiusz;
    double              perspfar        = shift - radiusz;

                                        // !!!!! test should be < 0 (better results), but test > 0 gives a real perspective impression?????
    if ( GetValue ( FitVolumePerspectiveZtoXYDelta ) < 0 )     // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion

    ToAbs_FromAbs.TranslateZ        (  shift, MultiplyLeft );

    ToAbs_FromAbs.PerspectiveZtoXY  ( perspnear, perspfar, MultiplyLeft );
                                        // de-shift + compensate for mid-planes perspective shift
    ToAbs_FromAbs.TranslateZ        ( -shift - Square ( radiusz ) / shift, MultiplyLeft );

    if ( GetValue ( FitVolumePerspectiveZtoXYDelta ) < 0 )     // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion
    }


if ( HasValue( FitVolumePerspectiveZtoXDelta ) && GetValue ( FitVolumePerspectiveZtoXDelta ) != 0 ) {
                                        // we need an absolute size
    double              radiusz         = FromNormExt.Z;
    double              radiusx         = FromNormExt.X;
                                        // force shift to be negative, as the perspective applies to the negative part of the axis
    double              shift           = - radiusz * radiusx / AbsNonNull ( GetValue ( FitVolumePerspectiveZtoXDelta ) );
                                        // !near is the one closer to 0!
    double              perspnear       = shift + radiusz;
    double              perspfar        = shift - radiusz;

                                        // !!!!! test should be < 0 (better results), but test > 0 gives a real perspective impression?????
    if ( GetValue ( FitVolumePerspectiveZtoXDelta ) < 0 )      // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion

    ToAbs_FromAbs.TranslateZ        (  shift, MultiplyLeft );

    ToAbs_FromAbs.PerspectiveZtoX   ( perspnear, perspfar, MultiplyLeft );
                                        // de-shift + compensate for mid-planes perspective shift
    ToAbs_FromAbs.TranslateZ        ( -shift - Square ( radiusz ) / shift, MultiplyLeft );

    if ( GetValue ( FitVolumePerspectiveZtoXDelta ) < 0 )      // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion
    }


if ( HasValue( FitVolumePerspectiveZtoYDelta ) && GetValue ( FitVolumePerspectiveZtoYDelta ) != 0 ) {
                                        // we need an absolute size
    double              radiusz         = FromNormExt.Z;
    double              radiusy         = FromNormExt.Y;
                                        // force shift to be negative, as the perspective applies to the negative part of the axis
    double              shift           = - radiusz * radiusy / AbsNonNull ( GetValue ( FitVolumePerspectiveZtoYDelta ) );
                                        // !near is the one closer to 0!
    double              perspnear       = shift + radiusz;
    double              perspfar        = shift - radiusz;

//    if ( VkQuery () )  DBGV6 ( GetValue ( FitVolumePerspectiveZtoYDelta ), radiusz, radiusy, shift, perspnear, perspfar, "delta -> radiusz, radiusy, shift, perspnear, perspfar" );

                                        // !!!!! test should be < 0 (better results), but test > 0 gives a real perspective impression?????
    if ( GetValue ( FitVolumePerspectiveZtoYDelta ) < 0 )      // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion

    ToAbs_FromAbs.TranslateZ        (  shift, MultiplyLeft );

    ToAbs_FromAbs.PerspectiveZtoY   ( perspnear, perspfar, MultiplyLeft );
                                        // de-shift + compensate for mid-planes perspective shift
    ToAbs_FromAbs.TranslateZ        ( -shift - Square ( radiusz ) / shift, MultiplyLeft );

    if ( GetValue ( FitVolumePerspectiveZtoYDelta ) < 0 )      // perspective in the other direction?
        ToAbs_FromAbs.ScaleZ        ( -1, MultiplyLeft );   // inversion
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore temporary origin

if ( HasValue( FitVolumeNormCenterRotateZ ) )   ToAbs_FromAbs.RotateZ ( - GetValue ( FitVolumeNormCenterRotateZ ), MultiplyLeft );
if ( HasValue( FitVolumeNormCenterRotateX ) )   ToAbs_FromAbs.RotateX ( - GetValue ( FitVolumeNormCenterRotateX ), MultiplyLeft );

                                            ToAbs_FromAbs.Translate ( HasValue( FitVolumeShearShiftX ) ? - GetValue ( FitVolumeShearShiftX ) : 0,
                                                                      HasValue( FitVolumeShearShiftY ) ? - GetValue ( FitVolumeShearShiftY ) : 0,
                                                                      HasValue( FitVolumeShearShiftZ ) ? - GetValue ( FitVolumeShearShiftZ ) : 0 , MultiplyLeft );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // make the transform relative, from voxel space to voxel space
ToRel_FromRel   = FromAbs_FromRel * ToAbs_FromAbs * ToRel_ToAbs;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set inverse transforms
FromAbs_ToAbs   = ToAbs_FromAbs;
FromAbs_ToAbs.Invert ();

FromRel_ToRel   = ToRel_FromRel;
FromRel_ToRel.Invert ();
}


//----------------------------------------------------------------------------
/*                                        // Backward transform
                                        // be careful when reading the order of transformations
void    TFitVolumeOnVolume::TransformTargetToSource ( TPointDouble &p )
{

ToRel_ToAbs.Apply ( p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // global scaling first
if ( HasValue( Scale  ) )                   p      *= GetValue ( Scale  );
if ( HasValue( ScaleX ) )                   p.X    *= GetValue ( ScaleX );
if ( HasValue( ScaleY ) )                   p.Y    *= GetValue ( ScaleY );
if ( HasValue( ScaleZ ) )                   p.Z    *= GetValue ( ScaleZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TempMat.SetIdentity ();

    if ( HasValue( RotationX ) )            TempMat.RotateX ( GetValue ( RotationX ), MultiplyLeft );
    if ( HasValue( RotationY ) )            TempMat.RotateY ( GetValue ( RotationY ), MultiplyLeft );
    if ( HasValue( RotationZ ) )            TempMat.RotateZ ( GetValue ( RotationZ ), MultiplyLeft );

    TempMat.Apply ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations
if ( HasValue( TranslationX ) )             p.X    += GetValue ( TranslationX );
if ( HasValue( TranslationY ) )             p.Y    += GetValue ( TranslationY );
if ( HasValue( TranslationZ ) )             p.Z    += GetValue ( TranslationZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temporarily adjust origin ?????
//if ( HasValue( FitVolumeShearShiftX ) )     p.X    += GetValue ( FitVolumeShearShiftX );
//if ( HasValue( FitVolumeShearShiftY ) )     p.Y    += GetValue ( FitVolumeShearShiftY );
//if ( HasValue( FitVolumeShearShiftZ ) )     p.Z    += GetValue ( FitVolumeShearShiftZ );

                                        // here we are supposed to be in the From space
                                        // compute a normalized position, centered in From space, with extent -1..+1
TPointDouble        centerdelta ( HasValue( FitVolumeNormCenterDeltaX ) ? GetValue ( FitVolumeNormCenterDeltaX ) : 0,
                                  HasValue( FitVolumeNormCenterDeltaY ) ? GetValue ( FitVolumeNormCenterDeltaY ) : 0,
                                  HasValue( FitVolumeNormCenterDeltaZ ) ? GetValue ( FitVolumeNormCenterDeltaZ ) : 0 );

TPointDouble        pnorm ( ( p + centerdelta ) / FromNormExt );

                                        // rotations
if ( HasValue( FitVolumeNormCenterRotateX ) ) {

    TempMat.SetIdentity ();

    TempMat.RotateX ( GetValue ( FitVolumeNormCenterRotateX ), MultiplyLeft );
    TempMat.RotateZ ( GetValue ( FitVolumeNormCenterRotateZ ), MultiplyLeft );

    TempMat.Apply ( pnorm );
    }


//pnorm.Clipped ( -1, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // shearing
if ( HasValue( FitVolumeShearXtoY ) )     { TempMat.SetIdentity (); TempMat.ShearX ( GetValue ( FitVolumeShearXtoY ), 0, MultiplyLeft ); TempMat.Apply ( p ); }
if ( HasValue( FitVolumeShearXtoZ ) )     { TempMat.SetIdentity (); TempMat.ShearX ( 0, GetValue ( FitVolumeShearXtoZ ), MultiplyLeft ); TempMat.Apply ( p ); }
if ( HasValue( FitVolumeShearYtoX ) )     { TempMat.SetIdentity (); TempMat.ShearY ( GetValue ( FitVolumeShearYtoX ), 0, MultiplyLeft ); TempMat.Apply ( p ); }
if ( HasValue( FitVolumeShearYtoZ ) )     { TempMat.SetIdentity (); TempMat.ShearY ( 0, GetValue ( FitVolumeShearYtoZ ), MultiplyLeft ); TempMat.Apply ( p ); }
if ( HasValue( FitVolumeShearZtoX ) )     { TempMat.SetIdentity (); TempMat.ShearZ ( GetValue ( FitVolumeShearZtoX ), 0, MultiplyLeft ); TempMat.Apply ( p ); }
if ( HasValue( FitVolumeShearZtoY ) )     { TempMat.SetIdentity (); TempMat.ShearZ ( 0, GetValue ( FitVolumeShearZtoY ), MultiplyLeft ); TempMat.Apply ( p ); }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Pinch / slope - needs other dimensions unmodified!
//if ( HasValue( PinchXtoYZ ) ) {
//    double          ratio          = 1 + GetValue ( PinchXtoYZ ) * pnorm.X;
//
//    p.Y    *= ratio;
//    p.Z    *= ratio;
//    }
//
//if ( HasValue( PinchYtoXZ ) ) {
//    double          ratio          = 1 + GetValue ( PinchYtoXZ ) * pnorm.Y;
//
//    p.X    *= ratio;
//    p.Z    *= ratio;
//    }
//
//if ( HasValue( PinchZtoXY ) ) {
//    double          ratio          = 1 + GetValue ( PinchZtoXY ) * pnorm.Z;
//
//    p.X    *= ratio;
//    p.Y    *= ratio;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasValue( PinchXtoY ) )                p.Y    *= 1 + GetValue ( PinchXtoY ) * pnorm.X;
if ( HasValue( PinchXtoZ ) )                p.Z    *= 1 + GetValue ( PinchXtoZ ) * pnorm.X;

if ( HasValue( PinchYtoX ) )                p.X    *= 1 + GetValue ( PinchYtoX ) * pnorm.Y;
if ( HasValue( PinchYtoZ ) )                p.Z    *= 1 + GetValue ( PinchYtoZ ) * pnorm.Y;

if ( HasValue( PinchZtoX ) )                p.X    *= 1 + GetValue ( PinchZtoX ) * pnorm.Z;
if ( HasValue( PinchZtoY ) )                p.Y    *= 1 + GetValue ( PinchZtoY ) * pnorm.Z;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Pinch / slope - with flatter response
//if ( HasValue( SinusPinchXtoYZ ) ) {
//    double          ratio          = 1 + GetValue ( SinusPinchXtoYZ ) * sin ( pnorm.X * HalfPi );
//
//    p.Y    *= ratio;
//    p.Z    *= ratio;
//    }
//
//if ( HasValue( SinusPinchYtoXZ ) ) {
//    double          ratio          = 1 + GetValue ( SinusPinchYtoXZ ) * sin ( pnorm.Y * HalfPi );
//
//    p.X    *= ratio;
//    p.Z    *= ratio;
//    }
//
//if ( HasValue( SinusPinchZtoXY ) ) {
//    double          ratio          = 1 + GetValue ( SinusPinchZtoXY ) * sin ( pnorm.Z * HalfPi );
//
//    p.X    *= ratio;
//    p.Y    *= ratio;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Various flattening
                                        // !this is a revert transform, subtract instead of add!

if ( HasValue( FlattenX    )                )   p.X    -= GetValue ( FlattenX    ) * sin ( pnorm.X * HalfPi );
if ( HasValue( FlattenXPos ) && pnorm.X > 0 )   p.X    -= GetValue ( FlattenXPos ) * sin ( pnorm.X * HalfPi );
if ( HasValue( FlattenXNeg ) && pnorm.X < 0 )   p.X    -= GetValue ( FlattenXNeg ) * sin ( pnorm.X * HalfPi );


if ( HasValue( FlattenY    )                )   p.Y    -= GetValue ( FlattenY    ) * sin ( pnorm.Y * HalfPi );
if ( HasValue( FlattenYPos ) && pnorm.Y > 0 )   p.Y    -= GetValue ( FlattenYPos ) * sin ( pnorm.Y * HalfPi );
if ( HasValue( FlattenYNeg ) && pnorm.Y < 0 )   p.Y    -= GetValue ( FlattenYNeg ) * sin ( pnorm.Y * HalfPi );


if ( HasValue( FlattenZ    )                )   p.Z    -= GetValue ( FlattenZ    ) * sin ( pnorm.Z * HalfPi );
if ( HasValue( FlattenZPos ) && pnorm.Z > 0 )   p.Z    -= GetValue ( FlattenZPos ) * sin ( pnorm.Z * HalfPi );
if ( HasValue( FlattenZNeg ) && pnorm.Z < 0 )   p.Z    -= GetValue ( FlattenZNeg ) * sin ( pnorm.Z * HalfPi );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore temporary origin
//if ( HasValue( FitVolumeNormCenterRotateX ) ) {
//
//    TempMat.SetIdentity ();
//
//    TempMat.RotateZ ( - GetValue ( FitVolumeNormCenterRotateZ ), MultiplyLeft );
//    TempMat.RotateX ( - GetValue ( FitVolumeNormCenterRotateX ), MultiplyLeft );
//
//    TempMat.Apply ( pnorm );
//    }


if ( HasValue( FitVolumeShearShiftX ) )     p.X    -= GetValue ( FitVolumeShearShiftX );
if ( HasValue( FitVolumeShearShiftY ) )     p.Y    -= GetValue ( FitVolumeShearShiftY );
if ( HasValue( FitVolumeShearShiftZ ) )     p.Z    -= GetValue ( FitVolumeShearShiftZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switch back to absolute space
//p   = ( ( pnorm + 1 ) * pnormext ) - pnormcenter;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // back to voxel
FromAbs_FromRel.Apply ( p );

}
*/

//----------------------------------------------------------------------------
/*                                        // Forward transform
                                        // be careful when reading the order of transformations
void    TFitVolumeOnVolume::TransformSourceToTarget ( TPointDouble &p )
{

FromRel_FromAbs.Apply ( p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute a normalized position, centered in From space, with extent -1..+1
TPointDouble        centerdelta ( HasValue( FitVolumeNormCenterDeltaX ) ? GetValue ( FitVolumeNormCenterDeltaX ) : 0,
                                  HasValue( FitVolumeNormCenterDeltaY ) ? GetValue ( FitVolumeNormCenterDeltaY ) : 0,
                                  HasValue( FitVolumeNormCenterDeltaZ ) ? GetValue ( FitVolumeNormCenterDeltaZ ) : 0 );

TPointDouble        pnorm ( ( p + centerdelta ) / FromNormExt );

                                        // rotations
if ( HasValue( FitVolumeNormCenterRotateX ) ) {

    TempMat.SetIdentity ();

    TempMat.RotateX ( - GetValue ( FitVolumeNormCenterRotateX ), MultiplyRight );
    TempMat.RotateZ ( - GetValue ( FitVolumeNormCenterRotateZ ), MultiplyRight );

    TempMat.Apply ( pnorm );
    }


//pnorm.Clipped ( -1, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Various flattening
if ( HasValue( FlattenZ    )                )   p.Z    += GetValue ( FlattenZ    ) * sin ( pnorm.Z * HalfPi );
if ( HasValue( FlattenZPos ) && pnorm.Z > 0 )   p.Z    += GetValue ( FlattenZPos ) * sin ( pnorm.Z * HalfPi );
if ( HasValue( FlattenZNeg ) && pnorm.Z < 0 )   p.Z    += GetValue ( FlattenZNeg ) * sin ( pnorm.Z * HalfPi );


if ( HasValue( FlattenY    )                )   p.Y    += GetValue ( FlattenY    ) * sin ( pnorm.Y * HalfPi );
if ( HasValue( FlattenYPos ) && pnorm.Y > 0 )   p.Y    += GetValue ( FlattenYPos ) * sin ( pnorm.Y * HalfPi );
if ( HasValue( FlattenYNeg ) && pnorm.Y < 0 )   p.Y    += GetValue ( FlattenYNeg ) * sin ( pnorm.Y * HalfPi );


if ( HasValue( FlattenX    )                )   p.X    += GetValue ( FlattenX    ) * sin ( pnorm.X * HalfPi );
if ( HasValue( FlattenXPos ) && pnorm.X > 0 )   p.X    += GetValue ( FlattenXPos ) * sin ( pnorm.X * HalfPi );
if ( HasValue( FlattenXNeg ) && pnorm.X < 0 )   p.X    += GetValue ( FlattenXNeg ) * sin ( pnorm.X * HalfPi );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasValue( PinchZtoY ) )                p.Y    /= 1 + GetValue ( PinchZtoY ) * pnorm.Z;
if ( HasValue( PinchZtoX ) )                p.X    /= 1 + GetValue ( PinchZtoX ) * pnorm.Z;

if ( HasValue( PinchYtoZ ) )                p.Z    /= 1 + GetValue ( PinchYtoZ ) * pnorm.Y;
if ( HasValue( PinchYtoX ) )                p.X    /= 1 + GetValue ( PinchYtoX ) * pnorm.Y;

if ( HasValue( PinchXtoZ ) )                p.Z    /= 1 + GetValue ( PinchXtoZ ) * pnorm.X;
if ( HasValue( PinchXtoY ) )                p.Y    /= 1 + GetValue ( PinchXtoY ) * pnorm.X;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations
if ( HasValue( TranslationZ ) )             p.Z    -= GetValue ( TranslationZ );
if ( HasValue( TranslationY ) )             p.Y    -= GetValue ( TranslationY );
if ( HasValue( TranslationX ) )             p.X    -= GetValue ( TranslationX );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( HasValue( RotationX ) || HasValue( RotationY ) || HasValue( RotationZ ) ) {

    TempMat.SetIdentity ();

    if ( HasValue( RotationX ) )            TempMat.RotateX ( - GetValue ( RotationX ), MultiplyRight );
    if ( HasValue( RotationY ) )            TempMat.RotateY ( - GetValue ( RotationY ), MultiplyRight );
    if ( HasValue( RotationZ ) )            TempMat.RotateZ ( - GetValue ( RotationZ ), MultiplyRight );

    TempMat.Apply ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // global scaling last
if ( HasValue( ScaleZ ) )                   p.Z    /= GetValue ( ScaleZ );
if ( HasValue( ScaleY ) )                   p.Y    /= GetValue ( ScaleY );
if ( HasValue( ScaleX ) )                   p.X    /= GetValue ( ScaleX );
if ( HasValue( Scale  ) )                   p      /= GetValue ( Scale  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // back to voxel
ToAbs_ToRel.Apply ( p );

}
*/

//----------------------------------------------------------------------------
                                        // Transform a whole volume, close to TransformAndSave except:
                                        // - no FilterTypeGaussian
                                        // - currently size is target size
                                        // - voxel and orientation are taken from target
void    TFitVolumeOnVolume::TransformToTarget   (   const Volume&           volume, 
                                                    FilterTypes             filtertype,
                                                    InterpolationType       interpolate, 
                                                    int                     numsubsampling,
                                                    int                     niftitransform,
                                                    int                     niftiintentcode,    const char*         niftiintentname,
                                                    const char*             file,               char*               title 
                                                )
{
if ( volume.IsNotAllocated () )
    return;


EvaluateMatrices ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting output type
TExportVolume     expvol;

StringCopy ( expvol.Filename, file );

expvol.VolumeFormat     = GetVolumeAtomType ( &volume, filtertype, interpolate, ToExtension ( expvol.Filename ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target dimension
expvol.Dim      [ 0 ] = ToVolume.GetDim ( 0 );  // volume.GetDim ( 0 );
expvol.Dim      [ 1 ] = ToVolume.GetDim ( 1 );  // volume.GetDim ( 1 );
expvol.Dim      [ 2 ] = ToVolume.GetDim ( 2 );  // volume.GetDim ( 2 );

//DBGV3 ( expvol.Dim[ 0 ], expvol.Dim[ 1 ], expvol.Dim[ 2 ], "Transform Volume: Dimension" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target origin
expvol.Origin       = ToCenter;

//DBGV3 ( expvol.Origin.X, expvol.Origin.Y, expvol.Origin.Z, "Transform Volume: Origin" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // caller knows the resulting orientation, trust him
if ( ToOrientation[ 0 ] && ToOrientation[ 1 ] && ToOrientation[ 2 ] ) {
    expvol.Orientation[ 0 ] = ToOrientation[ 0 ];
    expvol.Orientation[ 1 ] = ToOrientation[ 1 ];
    expvol.Orientation[ 2 ] = ToOrientation[ 2 ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target voxel size
expvol.VoxelSize    = ToVoxelSize;

//expvol.VoxelSize.Show ( "Transform Volume: VoxelSize" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

expvol.NiftiTransform       = AtLeast ( NiftiTransformDefault, niftitransform );

expvol.NiftiIntentCode      = niftiintentcode;

StringCopy  ( expvol.NiftiIntentName, niftiintentname, NiftiIntentNameSize - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                showprogress    = StringIsNotEmpty ( title );

TSuperGauge         Gauge ( StringIsEmpty ( title ) ? "Transforming" : title, showprogress ? expvol.Dim[ 2 ] : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Maxed ( numsubsampling, 1 );

int                 subsampling3    = Cube ( numsubsampling );
double              offset          = - (double) ( numsubsampling - 1 ) / numsubsampling / 2.0;   // shift to the "left", size of cube is half the jump (1/numsubsampling)
Volume              vol ( ToVolume.GetDim ( 0 ), ToVolume.GetDim ( 1 ), ToVolume.GetDim ( 2 ) );

                                        // Safety check for results range
SetOvershootingOption    ( interpolate, volume.GetArray (), volume.GetLinearDim () );


OmpParallelBegin

TEasyStats          median ( filtertype == FilterTypeMedian ? subsampling3 : 0 );
int                 count;
double              getvalue;
double              v;
TPointDouble        point;

OmpFor
                                        // scan from target space
for ( int z = 0; z < expvol.Dim[ 2 ]; z++ ) {

    Gauge.Next ();

    for ( int y = 0; y < expvol.Dim[ 1 ]; y++ )
    for ( int x = 0; x < expvol.Dim[ 0 ]; x++ ) {

                                        // speed up things if not sub sampling
        if ( numsubsampling == 1 ) {

            point.Set ( x, y, z );

//          TransformTargetToSource ( point );
            ToRel_FromRel.Apply ( point );

//          point  -= 0.5;

            v       = volume.GetValueChecked ( point.X, point.Y, point.Z, interpolate );
            } // numsubsampling == 1

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else { // numsubsampling != 1

            if ( filtertype == FilterTypeMedian )   median.Reset ();
            v       = 0;
            count   = 0;

                                    // scan all voxels of the downsample cluster
            for ( int zd = 0; zd < numsubsampling; zd++ )
            for ( int yd = 0; yd < numsubsampling; yd++ )
            for ( int xd = 0; xd < numsubsampling; xd++ ) {

                point.Set ( x + (double) xd / numsubsampling + offset,
                            y + (double) yd / numsubsampling + offset,
                            z + (double) zd / numsubsampling + offset );

//              TransformTargetToSource ( point );
                ToRel_FromRel.Apply ( point );

//              point  -= 0.5;

                getvalue    = volume.GetValueChecked ( point.X, point.Y, point.Z, interpolate );


                if      ( filtertype == FilterTypeMedian )              median.Add ( getvalue, ThreadSafetyIgnore );
                else if ( filtertype == FilterTypeMax   )               Maxed ( v, getvalue );
                else if ( filtertype == FilterTypeMean  )       { v  += getvalue; count++; }
                } // subsampled block


            if      ( filtertype == FilterTypeMedian )      v   = median.Median ();
//          else if ( filtertype == FilterTypeMax   )             v;
            else if ( filtertype == FilterTypeMean  )       v   = count ? v / count : 0;

            } // numsubsampling != 1

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // also remap intensity, if this a parameter
        if ( HasValue( FitVolumeFromIntensityRescale ) )
//          v   = Clip ( Round ( v * exp ( GetValue ( FitVolumeFromIntensityRescale ) ) ), 0, UCHAR_MAX );
//          v   = Clip ( Round ( ( v - GetValue ( FitVolumeFromIntensityOffset ) ) * exp ( GetValue ( FitVolumeFromIntensityRescale ) ) ), 0, UCHAR_MAX );
            v   = v * exp ( GetValue ( FitVolumeFromIntensityRescale ) );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//      expvol.Write ( v );
        vol ( x, y, z ) = v;

        } // for y, x
    } // for z

OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Just write at once
expvol.Write ( vol, ExportArrayOrderZYX );
}


//----------------------------------------------------------------------------
                                        // Transform a whole volume, close to TransformAndSave except:
                                        // - no FilterTypeGaussian
                                        // - currently size is target size
                                        // - voxel and orientation are taken from target
void    TFitVolumeOnVolume::TransformToSource   (   const Volume&           volume, 
                                                    FilterTypes             filtertype, 
                                                    InterpolationType       interpolate, 
                                                    int                     numsubsampling,
                                                    int                     niftitransform,
                                                    int                     niftiintentcode,    const char*         niftiintentname,
                                                    const char*             file,               char*               title 
                                                )
{
if ( volume.IsNotAllocated () )
    return;


EvaluateMatrices ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting output type
TExportVolume     expvol;

StringCopy ( expvol.Filename, file );

expvol.VolumeFormat     = GetVolumeAtomType ( &volume, filtertype, interpolate, ToExtension ( expvol.Filename ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target dimension
expvol.Dim      [ 0 ] = FromVolume.GetDim ( 0 );
expvol.Dim      [ 1 ] = FromVolume.GetDim ( 1 );
expvol.Dim      [ 2 ] = FromVolume.GetDim ( 2 );

//DBGV3 ( expvol.Dim[ 0 ], expvol.Dim[ 1 ], expvol.Dim[ 2 ], "Transform Volume: Dimension" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target origin
expvol.Origin       = FromCenter;

//DBGV3 ( expvol.Origin.X, expvol.Origin.Y, expvol.Origin.Z, "Transform Volume: Origin" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // caller knows the resulting orientation, trust him
if ( FromOrientation[ 0 ] && FromOrientation[ 1 ] && FromOrientation[ 2 ] ) {
    expvol.Orientation[ 0 ] = FromOrientation[ 0 ];
    expvol.Orientation[ 1 ] = FromOrientation[ 1 ];
    expvol.Orientation[ 2 ] = FromOrientation[ 2 ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set target voxel size
expvol.VoxelSize    = FromVoxelSize;

//expvol.VoxelSize.Show ( "Transform Volume: VoxelSize" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

expvol.NiftiTransform       = AtLeast ( NiftiTransformDefault, niftitransform );

expvol.NiftiIntentCode      = niftiintentcode;

StringCopy  ( expvol.NiftiIntentName, niftiintentname, NiftiIntentNameSize - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                showprogress    = StringIsNotEmpty ( title );

TSuperGauge         Gauge ( StringIsEmpty ( title ) ? "Transforming" : title, showprogress ? expvol.Dim[ 2 ] : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Maxed ( numsubsampling, 1 );

int                 subsampling3    = Cube ( numsubsampling );
double              offset          = - (double) ( numsubsampling - 1 ) / numsubsampling / 2.0;   // shift to the "left", size of cube is half the jump (1/numsubsampling)
Volume              vol ( FromVolume.GetDim ( 0 ), FromVolume.GetDim ( 1 ), FromVolume.GetDim ( 2 ) );

                                        // Safety check for results range
SetOvershootingOption    ( interpolate, volume.GetArray (), volume.GetLinearDim () );


OmpParallelBegin

TEasyStats          median ( filtertype == FilterTypeMedian ? subsampling3 : 0 );
int                 count;
double              getvalue;
double              v;
TPointDouble        point;

OmpFor
                                        // scan from target space
for ( int z = 0; z < expvol.Dim[ 2 ]; z++ ) {

    Gauge.Next ();


    for ( int y = 0; y < expvol.Dim[ 1 ]; y++ )
    for ( int x = 0; x < expvol.Dim[ 0 ]; x++ ) {

                                        // speed up things if not sub sampling
        if ( numsubsampling == 1 ) {

            point.Set ( x, y, z );

//          TransformSourceToTarget ( point );
            FromRel_ToRel.Apply ( point );

//          point  -= 0.5;

            v       = volume.GetValueChecked ( point.X, point.Y, point.Z, interpolate );
            } // numsubsampling == 1

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else {
            if ( filtertype == FilterTypeMedian )   median.Reset ();
            v       = 0;
            count   = 0;

                                    // scan all voxels of the downsample cluster
            for ( int zd = 0; zd < numsubsampling; zd++ )
            for ( int yd = 0; yd < numsubsampling; yd++ )
            for ( int xd = 0; xd < numsubsampling; xd++ ) {

                point.Set ( x + (double) xd / numsubsampling + offset,
                            y + (double) yd / numsubsampling + offset,
                            z + (double) zd / numsubsampling + offset );

//              TransformSourceToTarget ( point );
                FromRel_ToRel.Apply ( point );

//              point  -= 0.5;

                getvalue    = volume.GetValueChecked ( point.X, point.Y, point.Z, interpolate );


                if      ( filtertype == FilterTypeMedian )              median.Add ( getvalue, ThreadSafetyIgnore );
                else if ( filtertype == FilterTypeMax   )               Maxed ( v, getvalue );
                else if ( filtertype == FilterTypeMean  )       { v  += getvalue; count++; }
                } // subsampled block


            if      ( filtertype == FilterTypeMedian )      v   = median.Median ();
//          else if ( filtertype == FilterTypeMax   )             v;
            else if ( filtertype == FilterTypeMean  )       v   = count ? v / count : 0;

            } // numsubsampling != 1

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // also remap intensity, if this a parameter
        if ( HasValue( FitVolumeFromIntensityRescale ) )
//          v   = Clip ( Round ( v / exp ( GetValue ( FitVolumeFromIntensityRescale ) ) ), 0, UCHAR_MAX );
//          v   = Clip ( Round ( v / exp ( GetValue ( FitVolumeFromIntensityRescale ) ) + GetValue ( FitVolumeFromIntensityOffset ) ), 0, UCHAR_MAX );
            v   = v * exp ( GetValue ( FitVolumeFromIntensityRescale ) );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//        expvol.Write ( v );
        vol ( x, y, z ) = v;

        } // for y, x
    } // for z

OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Just write at once
expvol.Write ( vol, ExportArrayOrderZYX );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







