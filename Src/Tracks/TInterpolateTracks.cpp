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

#include    "TInterpolateTracks.h"

#include    "Math.Utils.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.Extensions.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TExportTracks.h"

#include    "TTracksDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  InterpolationTypeInfix[ NumInterpolationTypes ][ 16 ] = 
            {
            "Unknown",
            "iSurfS",
            "iSphS",
            "CD.iSphS",
            "i3DS",
            };


//----------------------------------------------------------------------------
                                        // Computing the 4x4 transformation matrix to geometrically transform a set of electrodes to "fiducial space":
                                        //  - re-centering
                                        //  - re-orienting to ALS
                                        //  - rescaling each axis (independently) to unit 1
                                        // Result is an 12 parameters affine transform
                                        // It is subtly important to compute the transform solely on the fiducials points given,
                                        // and not by any sort of best fitting sphere / ellipsoid. This enforces that 2 electrodes models
                                        // with matching fiducial will indeed coregister together once transformed to the fiducial space!
TMatrix44       ComputeFiducialTransform    (   const char*             cfront, 
                                                const char*             cleft, 
                                                const char*             ctop,
                                                const char*             cright, 
                                                const char*             crear, 
                                                const TPoints&          points,
                                                const TStrings&         pointnames
                                            )
{
                                        // compute the 5 extrema
TPointFloat         front           = points.GetMeanPoint ( cfront, pointnames );
TPointFloat         left            = points.GetMeanPoint ( cleft,  pointnames );
TPointFloat         top             = points.GetMeanPoint ( ctop,   pointnames );
TPointFloat         right           = points.GetMeanPoint ( cright, pointnames );
TPointFloat         rear            = points.GetMeanPoint ( crear,  pointnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // New center is the middle of front-rear axis
//TPointFloat       center          = ( front + rear ) / 2;
                                        // New center is the middle of front-rear and left-right axis - in case we have a quite flat model
TPointFloat         center          = ( front + rear + left + right ) / 4;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing the 3 main axis
TPointFloat         centerfront     =   front - center;
TPointFloat         centertop       =   top   - center;
TPointFloat         centerleft      = ( left  - right  ) / 2;

                                        // check if Z is 0
if ( /*points.IsGrid () ||*/ centertop.IsNull () ) {
                                        // create an artificial orthogonal vector
    centertop   = centerfront.VectorialProduct ( centerleft );

    centertop.Normalize ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Concatenating the 3 vectors
TMatrix44           fid2centered;

fid2centered.SetColumn ( 0, centerfront );
fid2centered.SetColumn ( 1, centerleft  );
fid2centered.SetColumn ( 2, centertop   );

                                        // center to fiducial
TMatrix44           M               = TMatrix44 ( fid2centered ).Invert ();

                                        // center: shifting to z = 1 by translation
M.Translate ( 0, 0, /*points.IsGrid () ? 1 :*/ 0, MultiplyLeft );

                                        // centering to O by translation
M.Translate ( - center.X, - center.Y, - center.Z, MultiplyRight );

                                        // normalize the final homogeneous transform
M.Normalize ();


return  M;
}


//----------------------------------------------------------------------------
                                        // Projecting 3D electrodes to a X-Y plane
                                        // Input:  ALS orientation
                                        // Output: AL  orientation
void            ProjectToPlane              ( TPoints& points )
{
for ( int i = 0; i < points.GetNumPoints (); i++ ) {

                                        // radius for X and Y only
    double      rxy     = sqrt ( Square ( points[ i ].X ) + Square ( points[ i ].Y ) );

                                        // arc-tangent to the Z part gives the length of unfolded arc from to Z
    double      arc     = NonNull ( ArcTangent ( rxy, points[ i ].Z ) );


    points[ i ].X      *= arc / NonNull ( rxy );
    points[ i ].Y      *= arc / NonNull ( rxy );
    points[ i ].Z       = 0;
    }
}


//----------------------------------------------------------------------------
                                        // Running the full transform to fiducial
TPoints         TransformToFiducial         (   const TPoints&          points, 
                                                const TMatrix44&        transform, 
                                                TracksInterpolationType interpolationtype 
                                            )
{
TPoints             transfpoints    = points;

                                        // to Fiducial space
transform.Apply ( transfpoints );


switch ( interpolationtype ) {

    case Interpolation2DSurfaceSpline:
                                        // project if not already on a grid
                                        // coordinates bounded to [-Pi..Pi]
//      if ( ! transfpoints.IsGrid () )
            ProjectToPlane ( transfpoints );

        break;


    case InterpolationSphericalSpline:
    case InterpolationSphericalCurrentDensitySpline:
                                        // coordinates bounded to [-1..1]
        transfpoints.Normalize ();

        break;


    case Interpolation3DSpline:
                                        // coordinates APPROXIMATELY bounded around [-1..1] - results are NOT spherical
        break;
    }


return  transfpoints;
}


//----------------------------------------------------------------------------
double          SplineInterpolationKernel   (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPointFloat&      v1,                 const TPointFloat&      v2 
                                            )
{
if      ( interpolationtype == Interpolation2DSurfaceSpline ) {
                                        // squared distance between the (normalized) 2D projected positions (Z being set to 0)
    double              r2          = ( v1 - v2 ).Norm2 ();
                                        // using the squared radius of first param as the normative factor
//  double              w2          = NonNull ( v1.Norm2 () );

    if ( r2 == 0 )
        return  0;

                                        // kernel value
    return  pow ( r2, mdegree - 1 ) * log ( r2 );                       // Cartool implentation - Points have been already normalized
//          pow ( r2, mdegree - 1 );                                    // historical formula(?)
//          pow ( r2 / w2, mdegree - 1 ) * log ( ( r2 + w2 ) / w2 );    // If/when points have not been normalized
    } // Interpolation2DSurfaceSpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( interpolationtype == InterpolationSphericalSpline ) {

    long double         cos12       = v1.Cosine ( v2 );
    long double         Pnm2,  Pnm1,  Pn;
    long double         k           = 0;


    for ( int n = 1; n <= NumLegendreTermsInterpol; n++ ) {

        NextLegendre ( Pnm2, Pnm1, Pn, cos12, n );

        k  += ( 2 * n + 1 ) / powl ( n * ( n + 1 ), mdegree ) * Pn;
        }

    return  k / FourPi;
    } // InterpolationSphericalSpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Correct formula, see: Electroencephalography and clinical Neurophysiology, 1990
else if ( interpolationtype == InterpolationSphericalCurrentDensitySpline ) {

    long double         cos12       = v1.Cosine ( v2 );
    long double         Pnm2,  Pnm1,  Pn;
    long double         k           = 0;


    for ( int n = 1; n <= NumLegendreTermsInterpol; n++ ) {

        NextLegendre ( Pnm2, Pnm1, Pn, cos12, n );

        k  += ( 2 * n + 1 ) / powl ( n * ( n + 1 ), mdegree - 1 ) * Pn;
        }

    return  k / FourPi;
    } // InterpolationSphericalCurrentDensitySpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( interpolationtype == Interpolation3DSpline ) {
                                        // squared distance between the (normalized) 3D positions
    double              r2          = ( v1 - v2 ).Norm2 ();
                                        // using the squared radius of first param as the normative factor
//  double              w2          = NonNull ( v1.Norm2 () );

    if ( r2 == 0 )
        return  0;

                                        // kernel value
    return  pow ( r2, mdegree - 1 ) * log ( r2 );                       // Cartool implentation - Points have been already normalized
//          pow ( r2 / w2, mdegree - 1 ) * log ( ( r2 + w2 ) / w2 );    // If/when points have not been normalized
    } // Interpolation3DSpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // should not happen
else
    return  0;
}


//----------------------------------------------------------------------------
ASymmetricMatrix SplineFillA                (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPoints&          frompoints 
                                            )
{
                                        // compute matrix size
                                        // K-part size depends only on number of electrodes
int                 maxelfrom       = frompoints.GetNumPoints ();

                                        // E-part size depends on interpolation type and degree
int                 esize;

if      ( interpolationtype == Interpolation2DSurfaceSpline                 )   esize = ( mdegree * ( mdegree + 1 ) ) / 2;
else if ( interpolationtype == InterpolationSphericalSpline                 )   esize = 1;
else if ( interpolationtype == InterpolationSphericalCurrentDensitySpline   )   esize = 1;
else if ( interpolationtype == Interpolation3DSpline                        )   esize = ( mdegree * ( mdegree + 1 ) * ( mdegree + 2 ) ) / ( 3 * 2 );
else                                                                            esize = 0;

                                        // matrix size is electrodes + E part
int                 matsize         = maxelfrom + esize;

                                        // matrix is symetric and loosely sparse
                                        //      ( K  E )
                                        //      ( E' 0 )
ASymmetricMatrix    A ( matsize, matsize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              e;

                                        // filling
if      ( interpolationtype == Interpolation2DSurfaceSpline ) {

                                        // K kernel
    for ( int i = 0; i < maxelfrom; i++ )
    for ( int j = 0; j <= i;        j++ )

        A ( i, j )  = A ( j, i )  = SplineInterpolationKernel ( interpolationtype, mdegree, frompoints[ i ], frompoints[ j ] );

                                        // E
    for ( int i = 0; i < maxelfrom; i++ ) {

        for ( int d = 0, row = maxelfrom; d < mdegree; d++ )
        for ( int k = 0;                  k <= d;      k++ ) {

            if ( d - k == 0 )   e   = 1;    // avoiding 0^0

            else                e   = pow ( frompoints[ i ].X, d - k );

            if ( k != 0 )       e  *= pow ( frompoints[ i ].Y, k     );

            A ( row, i )  = A ( i, row )  = e;

            row++;
            }
        }
    } // Interpolation2DSurfaceSpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( interpolationtype == InterpolationSphericalSpline 
       || interpolationtype == InterpolationSphericalCurrentDensitySpline ) {

                                        // K kernel
    for ( int i = 0; i < maxelfrom; i++ )
    for ( int j = 0; j <= i;        j++ )
                                        // always spherical spline
        A ( i, j )  = A ( j, i )  = SplineInterpolationKernel ( InterpolationSphericalSpline, mdegree, frompoints[ i ], frompoints[ j ] );


                                        // E
    for ( int i = 0; i < maxelfrom; i++ )

        A ( maxelfrom, i )  = A ( i, maxelfrom )  = 1;
    } // InterpolationSphericalSpline, InterpolationSphericalCurrentDensitySpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( interpolationtype == Interpolation3DSpline ) {

                                        // K kernel
    for ( int i = 0; i < maxelfrom; i++ )
    for ( int j = 0; j <= i;        j++ )

        A ( i, j )  = A ( j, i )  = SplineInterpolationKernel ( interpolationtype, mdegree, frompoints[ i ], frompoints[ j ] );


                                        // E
    for ( int i = 0; i < maxelfrom; i++ ) {

        for ( int d = 0, row = maxelfrom; d <  mdegree; d++ )
        for ( int k = 0;                  k <= d;       k++ )
        for ( int g = 0;                  g <= k;       g++ ) {

            if ( d - k == 0 )   e   = 1;    // avoiding 0^0

            else                e   = pow ( frompoints[ i ].X, d - k );

            if ( k - g != 0 )   e  *= pow ( frompoints[ i ].Y, k - g );

            if ( g     != 0 )   e  *= pow ( frompoints[ i ].Z, g     );

            A ( row, i )  = A ( i, row )  = e;

            row++;
            }
        }
    } // Interpolation3DSpline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  A;
}


//----------------------------------------------------------------------------
void            ComputeKernels              (   TracksInterpolationType interpolationtype,  int                     mdegree,
                                                const TPoints&          frompoints,
                                                const TPoints&          topoints,
                                                TArray2<double>&        kernel
                                            )
{
int                 maxelfrom       = frompoints.GetNumPoints ();
int                 maxelto         = topoints  .GetNumPoints ();


for ( int toei = 0; toei < maxelto;   toei++ )
for ( int frei = 0; frei < maxelfrom; frei++ )

    kernel ( toei, frei )   =   SplineInterpolationKernel   (   interpolationtype,  mdegree, 
                                                                frompoints[ frei ], 
                                                                topoints  [ toei ] 
                                                            );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//OptimizeOff

        TInterpolateTracks::TInterpolateTracks ()
{
Reset ();
}


void    TInterpolateTracks::Reset ()
{
InterpolationType       = UnknownInterpolation;
SplineDegree            = DefaultInterpolationDegree;   // !we force the polynomial degree to 1 (spline degree 2) to absolutely avoid creating new extrema!
TargetSpace             = BackToOriginalElectrodes;

FromOrigXyzFile         .Reset ();
FromXyzFile             .Reset ();
FromXyzExclFile         .Reset ();
FromXyz_To_FidFile      .Reset ();
FromXyz_To_DestXyzFile  .Reset ();

DestXyzFile             .Reset ();
DestXyz_To_FidFile      .Reset ();
DestXyz_To_FromXyzFile  .Reset ();

FromOrigPoints          .Reset ();
FromOrigPointsNames     .Reset ();
FromBadElectrodes.resize ( 0 );
FromPoints              .Reset ();
FromPointsNames         .Reset ();
From_To_FidPoints       .Reset ();

FromNormalized          = AlreadyNormalized;
FromFiducials           .Reset ();

DestPoints              .Reset ();
DestPointsNames         .Reset ();
Dest_To_FidPoints       .Reset ();

DestNormalized          = AlreadyNormalized;
DestFiducials           .Reset ();

From_To_Fid             .SetIdentity ();
Fid_To_From             .SetIdentity ();
Dest_To_Fid             .SetIdentity ();
Dest_To_From            .SetIdentity ();
From_To_Dest            .SetIdentity ();

A                       .ARelease ();
LU                      .Reset ();
Kernel                  .DeallocateMemory ();
ToHasSamePositionAsFrom .DeallocateMemory ();

for ( int d = 0; d < MaxInterpolationDegree; d++ )
    Dest_To_FidPoints_Power[ d ].Reset ();
}


void    TInterpolateTracks::FilesCleanUp ()    const
{
                                        // removing ONLY the temp files
DeleteFileExtended  ( FromXyzExclFile        );
DeleteFileExtended  ( FromXyz_To_FidFile     );
DeleteFileExtended  ( FromXyz_To_DestXyzFile );

DeleteFileExtended  ( DestXyz_To_FidFile     );
DeleteFileExtended  ( DestXyz_To_FromXyzFile );
}


//----------------------------------------------------------------------------

bool    TInterpolateTracks::Set (   TracksInterpolationType         interpolationtype,  int                             splinedegree,
                                    InterpolationTargetOptions      targetspace,

                                    const char*                     fromxyzfile,
                                    ElectrodesNormalizationOptions  fromnormalized,
                                    const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,   
                                    const char*                     frombadelectrodes,
                                                
                                    const char*                     destxyzfile,
                                    ElectrodesNormalizationOptions  destnormalized,
                                    const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,   

                                    const char*                     temppath,
                                    bool                            silent
                                )
{
Reset ();

                                        // force silent if not in interactive mode
if ( ! silent && CartoolObjects.CartoolApplication->IsNotInteractive () )
    silent  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Making local copies as these will be needed all along our interpolation journey
InterpolationType   = interpolationtype;
SplineDegree        = Clip ( splinedegree, 1, MaxInterpolationDegree );
TargetSpace         = targetspace;

FromBadElectrodes   = frombadelectrodes ? frombadelectrodes : "";   // we tolerate a null pointer as an empty string, but not the STL...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FromNormalized      = fromnormalized;

FromFiducials.Set   ( NumLandmarks, max ( StringSize ( fromfront ), StringSize ( fromleft ), StringSize ( fromtop ), StringSize ( fromright ), StringSize ( fromrear ) ) );

StringCopy ( FromFiducials[ LandmarkFront ], fromfront );
StringCopy ( FromFiducials[ LandmarkLeft  ], fromleft  );
StringCopy ( FromFiducials[ LandmarkTop   ], fromtop   );
StringCopy ( FromFiducials[ LandmarkRight ], fromright );
StringCopy ( FromFiducials[ LandmarkRear  ], fromrear  );

                                        // checking for empty fields
if ( FromNormalized == FiducialNormalization
  && FromFiducials.SomeStringsGrep ( GrepEmptyInput, GrepOptionDefault ) ) {

    if ( ! silent )
        ShowMessage (   "Not all 'From' landmarks have been provided!"      NewLine 
                        "Can not compute a proper fiducial normalization.." NewLine 
                        "Check your input, or select 'Already Normlaized'", 
                        InterpolationTitle, ShowMessageWarning );
    Reset ();
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DestNormalized      = destnormalized;

DestFiducials.Set   ( NumLandmarks, max ( StringSize ( destfront ), StringSize ( destleft ), StringSize ( desttop ), StringSize ( destright ), StringSize ( destrear ) ) );

StringCopy ( DestFiducials[ LandmarkFront ], destfront );
StringCopy ( DestFiducials[ LandmarkLeft  ], destleft  );
StringCopy ( DestFiducials[ LandmarkTop   ], desttop   );
StringCopy ( DestFiducials[ LandmarkRight ], destright );
StringCopy ( DestFiducials[ LandmarkRear  ], destrear  );

                                        // checking for empty fields
if ( DestNormalized == FiducialNormalization
  && DestFiducials.SomeStringsGrep ( GrepEmptyInput, GrepOptionDefault ) ) {

    if ( ! silent )
        ShowMessage (   "Not all 'To' landmarks have been provided!"        NewLine 
                        "Can not compute a proper fiducial normalization.." NewLine 
                        "Check your input, or select 'Already Normlaized'", 
                        InterpolationTitle, ShowMessageWarning );
    Reset ();
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set temp path for all all temp files, preferably using the EEG path
TFileName           TempPath;

if      ( StringIsEmpty ( temppath ) ) {

    GetTempDir      ( TempPath );
    }

else if ( ! IsDirectory ( temppath ) ) {

    StringCopy      ( TempPath, temppath );
    RemoveFilename  ( TempPath );
                                        // up one dir?
    if ( IsGeodesicsMFFPath ( temppath ) )
        RemoveFilename  ( TempPath );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From original points
StringCopy  ( FromOrigXyzFile, fromxyzfile );


if ( ! FromOrigPoints.ReadFile ( FromOrigXyzFile, &FromOrigPointsNames ) ) {

    if ( ! silent )
        ShowMessage ( "Can not open Electrodes Coordinates file!", FromOrigXyzFile, ShowMessageWarning );
    Reset ();
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From with excluded electrodes
if ( ! StringIsSpace ( FromBadElectrodes.c_str () ) ) {

    TFileName           filename;

    StringCopy      ( filename, FromOrigXyzFile );
    GetFilename     ( filename );
    StringCopy      ( FromXyzExclFile, TempPath, "\\", filename );
    StringAppend    ( FromXyzExclFile, ".", /*InfixXyzFrom,*/ InfixExcl );
    AddExtension    ( FromXyzExclFile, ToExtension ( FromOrigXyzFile ) );
    CheckNoOverwrite( FromXyzExclFile );


    FromOrigPoints.ExtractToFile ( FromXyzExclFile, FromBadElectrodes.c_str (), &FromOrigPointsNames );


    StringCopy      ( FromXyzFile, FromXyzExclFile );
    }
else {                                  // regular case
    FromXyzExclFile.Reset ();
    StringCopy      ( FromXyzFile, FromOrigXyzFile );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open From xyz, which can be either with Excluded electrodes, or the original From
if ( ! FromPoints.ReadFile ( FromXyzFile, &FromPointsNames ) ) {

    if ( ! silent )
        ShowMessage ( "Can not open Electrodes Coordinates file!", FromXyzFile, ShowMessageWarning );
    Reset ();
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // To points
if ( TargetSpace == BackToOriginalElectrodes )

    StringCopy ( DestXyzFile, FromOrigXyzFile );

else // ToOtherElectrodes
    StringCopy ( DestXyzFile, destxyzfile );

                                        // open To xyz
if ( ! DestPoints.ReadFile ( DestXyzFile, &DestPointsNames ) ) {

    if ( ! silent )
        ShowMessage ( "Can not open Electrodes Coordinates file!", DestXyzFile, ShowMessageWarning );
    Reset ();
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! SetTransformMatrices () ) {
    Reset ();
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           filename;


if ( TargetSpace == ToOtherElectrodes ) {
                                        // write the To xyz coregistered
    StringCopy      ( filename, DestXyzFile );
    GetFilename     ( filename );
    StringCopy      ( DestXyz_To_FromXyzFile, TempPath, "\\", filename );
//  StringAppend    ( DestXyz_To_FromXyzFile, ".", InterpolationTypeInfix[ interpolationtype ] );

    StringCopy      ( filename, FromXyzFile );
    GetFilename     ( filename );
    StringAppend    ( DestXyz_To_FromXyzFile, InfixToCoregistered, filename );

    AddExtension    ( DestXyz_To_FromXyzFile, FILEEXT_XYZ );
    CheckNoOverwrite( DestXyz_To_FromXyzFile );


    TPoints             dest_to_frompoints ( DestPoints );
                                        // Dest to From space
    Dest_To_From.Apply ( dest_to_frompoints );

    dest_to_frompoints.WriteFile ( DestXyz_To_FromXyzFile, &DestPointsNames );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write the From xyz coregistered
    StringCopy      ( filename, FromXyzFile );
    GetFilename     ( filename );
    StringCopy      ( FromXyz_To_DestXyzFile, TempPath, "\\", filename );
//  StringAppend    ( FromXyz_To_DestXyzFile, ".", InterpolationTypeInfix[ interpolationtype ] );

    StringCopy      ( filename, DestXyzFile );
    GetFilename     ( filename );
    StringAppend    ( FromXyz_To_DestXyzFile, InfixToCoregistered, filename );

    AddExtension    ( FromXyz_To_DestXyzFile, FILEEXT_XYZ );
    CheckNoOverwrite( FromXyz_To_DestXyzFile );


    TPoints             from_to_destpoints ( FromPoints );
                                        // FromExcl to To space
    From_To_Dest.Apply ( from_to_destpoints );

    from_to_destpoints.WriteFile ( FromXyz_To_DestXyzFile, &FromPointsNames );

    }

else { // BackToOriginalElectrodes

    DestXyz_To_FromXyzFile.Reset ();
    FromXyz_To_DestXyzFile.Reset ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From Fiducial xyz
StringCopy      ( filename, FromXyzFile );
GetFilename     ( filename );
StringCopy      ( FromXyz_To_FidFile, TempPath, "\\", filename );
//StringAppend    ( FromXyz_To_FidFile, ".", InterpolationTypeInfix[ interpolationtype ] );
StringAppend    ( FromXyz_To_FidFile, ".", InfixXyzFrom, InfixFiducial );
AddExtension    ( FromXyz_To_FidFile, FILEEXT_XYZ );
CheckNoOverwrite( FromXyz_To_FidFile );


From_To_FidPoints   = TransformToFiducial ( FromPoints, From_To_Fid, interpolationtype );


From_To_FidPoints.WriteFile ( FromXyz_To_FidFile, &FromPointsNames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // To Fiducial xyz
                                        // !User can have asked no bad electrodes, and back to original, which would cause From == To, that's why we force names with "From" and "To" here!
StringCopy      ( filename, DestXyzFile );
GetFilename     ( filename );
StringCopy      ( DestXyz_To_FidFile, TempPath, "\\", filename );
//StringAppend    ( DestXyz_To_FidFile, ".", InterpolationTypeInfix[ interpolationtype ] );
StringAppend    ( DestXyz_To_FidFile, ".", InfixXyzTo, InfixFiducial );
AddExtension    ( DestXyz_To_FidFile, FILEEXT_XYZ );
CheckNoOverwrite( DestXyz_To_FidFile );


Dest_To_FidPoints   = TransformToFiducial ( DestPoints, Dest_To_Fid, interpolationtype );


Dest_To_FidPoints.WriteFile ( DestXyz_To_FidFile, &DestPointsNames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here From_To_FidPoints and Dest_To_FidPoints are in Fiducial space
                                        // From_To_FidPoints also had the bad channels removed
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetSpline ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Simpler initialization for only bad electrodes interpolation
bool    TInterpolateTracks::Set (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                    const char*                     fromxyzfile,
                                    ElectrodesNormalizationOptions  fromnormalized,
                                    const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                    const char*                     frombadelectrodes,

                                    const char*                     temppath,
                                    bool                            silent
                                )
{
return (    Set (   interpolationtype,      splinedegree,
                    BackToOriginalElectrodes,

                    fromxyzfile,
                    fromnormalized,
                    fromfront,  fromleft,   fromtop,    fromright,  fromrear,
                    frombadelectrodes,
                    
                    0,
                    AlreadyNormalized,
                    0,          0,          0,          0,          0,

                    temppath,
                    silent
                ) );
}


//----------------------------------------------------------------------------
                                        // Simpler initialization to go from one electrodes model to another one
    bool    TInterpolateTracks::Set (   TracksInterpolationType         interpolationtype,  int                             splinedegree,

                                        const char*                     fromxyzfile,
                                        ElectrodesNormalizationOptions  fromnormalized,
                                        const char* fromfront,  const char* fromleft,   const char* fromtop,    const char* fromright,  const char* fromrear,
                                        
                                        const char*                     destxyzfile,
                                        ElectrodesNormalizationOptions  destnormalized,
                                        const char* destfront,  const char* destleft,   const char* desttop,    const char* destright,  const char* destrear,

                                        const char*                     temppath,
                                        bool                            silent
                                    )
{
return (    Set (   interpolationtype,      splinedegree,
                    ToOtherElectrodes,

                    fromxyzfile,
                    fromnormalized,
                    fromfront,  fromleft,   fromtop,    fromright,  fromrear,
                    0,
                    
                    destxyzfile,
                    destnormalized,
                    destfront,  destleft,   desttop,    destright,  destrear,

                    temppath,
                    silent
                ) );
}


//----------------------------------------------------------------------------
                                        // Setup for affine transformation To->From space

                                        // Note that From_To_Fid is used in two different contexts:
                                        //  - to compute From -> Fid -> To
                                        //  - to enhance SplineFillA / SplineInterpolationKernel / Interpolate by centering/orienting/normalizing
bool    TInterpolateTracks::SetTransformMatrices ()
{
                                        // not done yet?
if ( ! IsOpen () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the transformation?
if ( FromNormalized == FiducialNormalization ) {
                                        // !use original From xyz to get all landmarks/electrodes!
    From_To_Fid= ComputeFiducialTransform   (   FromFiducials[ LandmarkFront ], 
                                                FromFiducials[ LandmarkLeft  ],
                                                FromFiducials[ LandmarkTop   ],
                                                FromFiducials[ LandmarkRight ], 
                                                FromFiducials[ LandmarkRear  ],
                                                FromOrigPoints,
                                                FromOrigPointsNames );

    Fid_To_From = TMatrix44 ( From_To_Fid ).Invert ();
    }
else { // AlreadyNormalized - trust From electrodes are in a normalized, fiducial-like space

    From_To_Fid.SetIdentity ();
    Fid_To_From.SetIdentity ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( TargetSpace == BackToOriginalElectrodes ) {
                                        // using the From transform, even if identity (trusting input xyz)
    Dest_To_Fid = From_To_Fid;
    }

else { // TargetSpace == ToOtherElectrodes

    if ( DestNormalized == FiducialNormalization ) {

        Dest_To_Fid = ComputeFiducialTransform  (   DestFiducials[ LandmarkFront ], 
                                                    DestFiducials[ LandmarkLeft  ], 
                                                    DestFiducials[ LandmarkTop   ],
                                                    DestFiducials[ LandmarkRight ], 
                                                    DestFiducials[ LandmarkRear  ], 
                                                    DestPoints,
                                                    DestPointsNames );
        }
    else { // AlreadyNormalized - trust From electrodes are in a normalized, fiducial-like space

        Dest_To_Fid.SetIdentity ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compose the To to From transformation - used only to show how the 2 electrodes models superimpose, but not in any interpolation computation
if (    TargetSpace == BackToOriginalElectrodes
     || ( Dest_To_Fid.IsIdentity () && Fid_To_From.IsIdentity () ) ) {

    Dest_To_From.SetIdentity ();
    From_To_Dest.SetIdentity ();
    }
else {
                            // only compose matrices if they are not both identities
    Dest_To_From    = Fid_To_From * Dest_To_Fid;
    Dest_To_From.Normalize ();

    From_To_Dest    = TMatrix44 ( Dest_To_From ).Invert ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // All Spline parameters
constexpr int       InvalidIndex    = -1;

bool    TInterpolateTracks::SetSpline ()
{
                                        // not done yet?
if ( ! IsOpen () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setup the matrix from parameters
A       = SplineFillA ( InterpolationType, SplineDegree, From_To_FidPoints );

                                        // init the LU decomposition solver
LU.Set ( A );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need points coordinates raised to some power
int                 maxelfrom       = From_To_FidPoints.GetNumPoints ();
int                 maxelto         = Dest_To_FidPoints.GetNumPoints ();

                                        // initializing the electrodes to power
for ( int d = 0; d < SplineDegree; d++ ) {
                                        // copy whole list of points
    Dest_To_FidPoints_Power[ d ]    = Dest_To_FidPoints;
                                        // then elevate their components to some power
    for ( int toei = 0; toei < maxelto; toei++ )

        if ( d == 0 )
            Dest_To_FidPoints_Power[ d ][ toei ] = 1;
        else
            Dest_To_FidPoints_Power[ d ][ toei ].Power ( d );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Kernels computation has to be done only once
Kernel.Resize ( maxelto, maxelfrom );


ComputeKernels  (   InterpolationType,      SplineDegree, 
                    From_To_FidPoints, 
                    Dest_To_FidPoints, 
                    Kernel
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scanning To and From electrodes for exact positions
                                        // - Back-to-From is the most obvious case
                                        // - Downsampling then number of electrodes, which can be quite common, too
                                        // - Going to another space case can also happen, for very specific cases like Cz, Fpz etc... This will run as the other cases.

                                        // will hold any index in the (full) from space
ToHasSamePositionAsFrom.Resize ( maxelto );
ToHasSamePositionAsFrom     = InvalidIndex;


if ( TargetSpace == BackToOriginalElectrodes ) {
                                        // this one is easy, we have an exact 1-to-1 correspondance
    for ( int toei = 0, frei = 0; toei < maxelto;   toei++, frei++ )
                                        // identity
        ToHasSamePositionAsFrom[ toei ]     = frei;
    }

else { // ToOtherElectrodes
                                        // !we need ALL original points!
    TPoints             AllFrom2FidPoints   = TransformToFiducial ( FromOrigPoints, From_To_Fid, InterpolationType );

                                        // search for some very close correspondance between "to" and "from" electrodes
    int                 allmaxelfrom    =   AllFrom2FidPoints.GetNumPoints ();
    double              radius          = ( AllFrom2FidPoints.GetRadius () + Dest_To_FidPoints.GetRadius () ) / 2;

    for ( int toei = 0; toei < maxelto;   toei++ ) {

        for ( int frei = 0; frei < allmaxelfrom; frei++ )
                                        // using some relative difference
            if ( ( AllFrom2FidPoints[ frei ] - Dest_To_FidPoints[ toei ] ).Norm () / radius < SingleFloatEpsilon ) {

                ToHasSamePositionAsFrom[ toei ]     = frei; // !index of original xyz!
                break;
                } // matching electrodes


        if ( ToHasSamePositionAsFrom[ toei ] != InvalidIndex )
            continue;
        } // for to electrode
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Simpler wrapper to full InterpolateTracks method
bool    TInterpolateTracks::InterpolateTracks   (   const char*         fileeeg,
                                                    const char*         infixfilename,  const char*         fileoutext, char*               fileout,
                                                    bool                silent
                                                )
{
                                        // will nicely close document upon exit
TOpenDoc<TTracksDoc>    eegdoc ( fileeeg, OpenDocHidden );

return  InterpolateTracks   (   eegdoc,
                                infixfilename,  fileoutext,     fileout,
                                silent
                            );
}


//----------------------------------------------------------------------------

bool    TInterpolateTracks::InterpolateTracks   (   const TTracksDoc*   eegdoc,
                                                    const char*         infixfilename,  const char*         fileoutext, char*               fileout,
                                                    bool                silent
                                                )
{
                                        // this shouldn't happen
if ( eegdoc == 0 )
    return  false;

                                        // check we have all the info we need
if ( ! IsOpen () )
    return  false;

                                        // force silent if not in interactive mode
if ( ! silent && CartoolObjects.CartoolApplication->IsNotInteractive () )
    silent  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                hasfrombadelectrodes= ! StringIsSpace ( FromBadElectrodes.c_str () );
TSelection          fromgoodsel ( eegdoc->GetTotalElectrodes(), OrderSorted );

                                        // select all
fromgoodsel.Set ();
                                        // but without pseudo-electrodes
eegdoc->ClearPseudo ( fromgoodsel );

if ( hasfrombadelectrodes )
                                        // remove specified electrodes
    fromgoodsel.Reset ( FromBadElectrodes.c_str (), &FromOrigPointsNames, ! silent );

                                        // not the same number of FINAL electrodes?
if ( (int) fromgoodsel != FromPoints.GetNumPoints () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // auxiliary channels not allowed
if ( eegdoc->GetNumAuxElectrodes() != 0 )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // generate a new filename
TFileName           neweegfile;
unique_ptr<char[]>  buff ( new char[ EditSizeTextLong ] );


eegdoc->GetBaseFileName ( neweegfile );


if ( StringIsNotEmpty ( infixfilename ) )

    StringAppend    ( neweegfile,   ".",    infixfilename );

else {
    StringAppend    ( neweegfile,   ".",                    InterpolationTypeInfix[ InterpolationType ], IntegerToString ( buff.get () , SplineDegree ) );
    StringAppend    ( neweegfile,   InfixToCoregistered,    IntegerToString ( buff.get (), DestPoints.GetNumPoints () ) );
    }

/*                                      // insert a new directory in the base name
CreatePath ( neweegfile, false );       // too much directories, indeed... maybe create a big one for the whole batch
AppendFilenameAsSubdirectory ( neweegfile );
*/

                                        // extension
AddExtension        ( neweegfile, fileoutext );

CheckNoOverwrite    ( neweegfile );

if ( fileout != 0 )
    StringCopy ( fileout, neweegfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file filling, one per new file
TFileName           verbosefile;

StringCopy      ( verbosefile, neweegfile  );
AddExtension    ( verbosefile, FILEEXT_VRB );

TVerboseFile    verbose ( verbosefile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Tracks Interpolation" );


verbose.NextTopic ( "Files:" );
{
verbose.Put ( "Input   EEG File   :", eegdoc->GetDocPath () );
verbose.Put ( "Output  EEG File   :", neweegfile );
verbose.Put ( "Verbose File (this):", verbosefile );
}


verbose.NextTopic ( "'From' Space:" );
{
verbose.Put ( "Electrodes coordinates file:", FromOrigXyzFile );
verbose.NextLine ();

//verbose.Put ( "Bad electrodes:", FromBadElectrodes.empty () ? "None" : FromBadElectrodes.c_str () );

{                                       // all this business to output an optimal string, while not altering the user string
TSelection          removesel ( FromOrigPoints.GetNumPoints (), OrderSorted );

removesel.Reset ();

removesel.Set ( FromBadElectrodes.empty () ? "" : FromBadElectrodes.c_str (), &FromOrigPointsNames, ! silent );

if ( (bool) removesel ) {
    verbose.Put ( "Number of bad electrodes:", (int) removesel );
    verbose.Put ( "Bad electrodes:", (bool) removesel ? removesel.ToText ( buff.get (), &FromOrigPointsNames, "*" ) : "None" );
    verbose.NextLine ();
    }
}


verbose.Put ( "Re-orient, center & normalize:", FromNormalized == AlreadyNormalized       ? "Using electrodes coordinates from file 'as is'" 
                                                                /*FiducialNormalization*/ : "Transforming to a Fiducial coordinate system" );

if ( FromNormalized == FiducialNormalization ) {
    verbose.Put ( "Fiducial Front:", FromFiducials[ LandmarkFront ] );
    verbose.Put ( "Fiducial Left :", FromFiducials[ LandmarkLeft  ] );
    verbose.Put ( "Fiducial Top  :", FromFiducials[ LandmarkTop   ] );
    verbose.Put ( "Fiducial Right:", FromFiducials[ LandmarkRight ] );
    verbose.Put ( "Fiducial Rear :", FromFiducials[ LandmarkRear  ] );
    }
}


verbose.NextTopic ( "'To' Space:" );
{
verbose.Put ( "Destination space is:", TargetSpace == BackToOriginalElectrodes ? "Identical to 'From' space" : "Another space" );

if ( TargetSpace == ToOtherElectrodes ) {

    verbose.Put ( "Electrodes coordinates file:", DestXyzFile );
    verbose.NextLine ();

    verbose.Put ( "Re-orient, center & normalize:", DestNormalized == AlreadyNormalized       ? "Using electrodes coordinates from file 'as is'" 
                                                                    /*FiducialNormalization*/ : "Transforming to a Fiducial coordinate system" );

    if ( DestNormalized == FiducialNormalization ) {
        verbose.Put ( "Fiducial Front:", DestFiducials[ LandmarkFront ] );
        verbose.Put ( "Fiducial Left :", DestFiducials[ LandmarkLeft  ] );
        verbose.Put ( "Fiducial Top  :", DestFiducials[ LandmarkTop   ] );
        verbose.Put ( "Fiducial Right:", DestFiducials[ LandmarkRight ] );
        verbose.Put ( "Fiducial Rear :", DestFiducials[ LandmarkRear  ] );
        }
    }
}


if ( TargetSpace == ToOtherElectrodes ) {

    verbose.NextTopic ( "Coregistration between 'From' and 'To' Spaces:" );

    verbose.Put ( "Coregistration method:", TargetSpace == BackToOriginalElectrodes || From_To_Dest.IsIdentity () ? "None" : "Affine transformation (12 parameters)" );
    }


verbose.NextTopic ( "Interpolation:" );
{
verbose.Put ( "Interpolation method:" );

if      ( InterpolationType == Interpolation2DSurfaceSpline                 )   (ofstream&) verbose << "Surface Spline"                         << NewLine;
else if ( InterpolationType == InterpolationSphericalSpline                 )   (ofstream&) verbose << "Spherical Spline"                       << NewLine;
else if ( InterpolationType == InterpolationSphericalCurrentDensitySpline   )   (ofstream&) verbose << "Current Density with Spherical Spline"  << NewLine;
else if ( InterpolationType == Interpolation3DSpline                        )   (ofstream&) verbose << "3D Spline"                              << NewLine;

verbose.Put ( "Degree of the Spline:", SplineDegree );
}


verbose.NextLine ();
verbose.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                {
                    gaugeinterpolglobal  = 0,
                    gaugeinterpol,
                    };

TSuperGauge         Gauge;

if ( ! silent ) {

    Gauge.Set           ( InterpolationTitle, SuperGaugeLevelInter );

    Gauge.AddPart       (  gaugeinterpolglobal, 1,   05 );
    Gauge.AddPart       (  gaugeinterpol,       0,   95 );

    CartoolObjects.CartoolApplication->SetMainTitle ( InterpolationTitle " of", eegdoc->GetDocPath (), Gauge );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxtf           = eegdoc->GetNumTimeFrames();
int                 maxelfrom       = From_To_FidPoints.GetNumPoints ();
int                 maxelto         = Dest_To_FidPoints.GetNumPoints ();
                                        
int                 Am              = A.n_rows;     // retrieve matrix dimensions, which can vary according to parameters (actually, Am=An, but anyway...)
int                 An              = A.n_cols;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do we really need to mathematically solve anything? Sometimes user just want to downsample data
bool                needtosolve     = false;

for ( int toei = 0; toei < maxelto; toei++ )

    if (   ToHasSamePositionAsFrom[ toei ] == InvalidIndex 
        || fromgoodsel.IsNotSelected ( ToHasSamePositionAsFrom[ toei ] )
        || InterpolationType == InterpolationSphericalCurrentDensitySpline ) {

        needtosolve = true;
        break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ( gaugeinterpolglobal, SuperGaugeUpdateTitle );

TMaps               datain  ( eegdoc->GetDocPath (), AtomTypeScalar, ReferenceAsInFile );

TMaps               dataout ( maxtf, TargetSpace == BackToOriginalElectrodes ? eegdoc->GetNumElectrodes () 
                                                                             : maxelto );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                     // accessing From_To_FidPoints, Kernel, SplineDegree, Dest_To_FidPoints_Power
auto    Interpolate2DSurfaceSpline  = [ this ] ( int    toei,   const AVector&  pq )
{
int                 maxelfrom   = From_To_FidPoints.GetNumPoints ();
double              v           = 0;

                                        // K part
for ( int i = 0; i < maxelfrom; i++ )

    v      += pq[ i ] * Kernel[ toei ][ i ];

                                        // Q part
for ( int d = 0, row = maxelfrom; d < SplineDegree; d++ )
for ( int k = 0;                  k <= d;           k++ )

    v      += pq[ row++ ] * Dest_To_FidPoints_Power[ d - k ][ toei ].X
                          * Dest_To_FidPoints_Power[ k     ][ toei ].Y;

return  v;
};


auto    InterpolateSphericalSpline  = [ this ] ( int    toei,   const AVector&  pq )
{
int                 maxelfrom   = From_To_FidPoints.GetNumPoints ();

                                        // constant part
double              v           = pq[ maxelfrom ];

                                        // K part
for ( int i = 0; i < maxelfrom; i++ )

    v      += pq[ i ] * Kernel[ toei ][ i ];

return  v;
};


auto    InterpolateSphericalCurrentDensitySpline    = [ this ] ( int    toei,   const AVector&  pq )
{
int                 maxelfrom   = From_To_FidPoints.GetNumPoints ();

                                        // constant part
double              v           = 0;

                                        // K part
for ( int i = 0; i < maxelfrom; i++ )

    v      += pq[ i ] * Kernel[ toei ][ i ];

return  v;
};


auto    Interpolate3DSpline2        = [ this ] ( int    toei,   const AVector&  pq )
{
int                 maxelfrom   = From_To_FidPoints.GetNumPoints ();
double              v           = 0;

                                        // K part
for ( int i = 0; i < maxelfrom; i++ )

    v      += pq[ i ] * Kernel[ toei ][ i ];

                                        // Q part
                                        // loop expanded for SplineDegree == 2
int        &row         = maxelfrom;

v      += pq[ row     ] * Dest_To_FidPoints_Power[ 0 ][ toei ].X
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Y
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Z

        + pq[ row + 1 ] * Dest_To_FidPoints_Power[ 1 ][ toei ].X
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Y
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Z
         
        + pq[ row + 2 ] * Dest_To_FidPoints_Power[ 0 ][ toei ].X
                        * Dest_To_FidPoints_Power[ 1 ][ toei ].Y
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Z
         
        + pq[ row + 3 ] * Dest_To_FidPoints_Power[ 0 ][ toei ].X
                        * Dest_To_FidPoints_Power[ 0 ][ toei ].Y
                        * Dest_To_FidPoints_Power[ 1 ][ toei ].Z;

return  v;
};


auto    Interpolate3DSpline         = [ this ] ( int    toei,   const AVector&  pq )
{
int                 maxelfrom   = From_To_FidPoints.GetNumPoints ();
double              v           = 0;

                                        // K part
for ( int i = 0; i < maxelfrom; i++ )

    v      += pq[ i ] * Kernel[ toei ][ i ];

                                        // Q part
for ( int d = 0, row = maxelfrom; d <  SplineDegree; d++ )
for ( int k = 0;                  k <= d;            k++ )
for ( int g = 0;                  g <= k;            g++ )

    v      += pq[ row++ ] * Dest_To_FidPoints_Power[ d - k ][ toei ].X
                          * Dest_To_FidPoints_Power[ k - g ][ toei ].Y
                          * Dest_To_FidPoints_Power[ g     ][ toei ].Z;

return  v;
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // picking the right lambda
function<double ( int, const AVector& )>    interpolate;


if      ( InterpolationType == Interpolation2DSurfaceSpline                 )   interpolate     = Interpolate2DSurfaceSpline;
else if ( InterpolationType == InterpolationSphericalSpline                 )   interpolate     = InterpolateSphericalSpline;
else if ( InterpolationType == InterpolationSphericalCurrentDensitySpline   )   interpolate     = InterpolateSphericalCurrentDensitySpline;
else if ( InterpolationType == Interpolation3DSpline && SplineDegree == 2   )   interpolate     = Interpolate3DSpline2;     // optimized version
else if ( InterpolationType == Interpolation3DSpline && SplineDegree >  2   )   interpolate     = Interpolate3DSpline;      // works for all degrees, including 2


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.SetRange ( gaugeinterpol, maxtf );


OmpParallelBegin
                                        // allocate B and X
AVector                     B ( Am );
AVector                     X ( An );
TIteratorSelectedForward    eti;

OmpFor

for ( int tf = 0; tf < maxtf; tf++ ) {

    Gauge.Next ( gaugeinterpol, SuperGaugeUpdateTitle );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting B

                                        // clearing whole vector
    B.AReset ();

                                        // then transferring the (optionally reduced) "from" data
    if ( hasfrombadelectrodes ) {
                                        // TIteratorSelectedForward is thread-safe
        for ( eti.FirstSelected ( fromgoodsel ); (bool) eti && eti.GetIndex () < maxelfrom; ++eti )
                // new index                       original from index
            B[ eti.GetIndex () ]    = datain ( tf, eti() );
        }

    else {                              // faster by bypassing fromgoodsel
                                        // even faster if data types are compatible - note that Armadillo advanced constructors can directly point to some memory, like 'mat(ptr_aux_mem, n_rows, n_cols, copy_aux_mem = true, strict = false)'
        if ( typeid ( AReal ) == typeid ( TMapAtomType ) )
            CopyVirtualMemory ( B.memptr (), &datain ( tf, 0 ), datain ( 0 ).GetMemorySize () );

        else                            // ol' loop
            for ( int ei = 0; ei < maxelfrom; ei++ )
                B[ ei ] = datain ( tf, ei );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // solving A * X = B, with fast option, thread-safe
    if ( needtosolve )

        LU.Solve ( X, B );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // using spline & transferring results back

                                        // for each "to" electrode
    for ( int toei = 0; toei < maxelto; toei++ ) {

                                        // current "Dest" electrode possible geometrical match to a "From" electrode
        int             desttofromindex     = ToHasSamePositionAsFrom[ toei ];

                                        // decide if we need to interpolate current electrode
        bool            dosolve             =  desttofromindex == InvalidIndex                                  // is current "Dest" electrode NOT an exact geometrical match to a "From" electrode?
                                            || fromgoodsel.IsNotSelected ( desttofromindex )                    // or, it might be a match, but is it a bad electrodes?
                                            || InterpolationType == InterpolationSphericalCurrentDensitySpline; // or are we computing Current Density?

        if ( dosolve )  dataout ( tf, toei )    = interpolate ( toei, X );          // do the actual interpolation now
        else            dataout ( tf, toei )    = datain ( tf, desttofromindex );   // plain copy of original data: faster and will not introduce rounding errors
        } // for electrode
    } // for tf

OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting the exported file
TExportTracks     expfile;

StringCopy ( expfile.Filename, neweegfile );

//StringCopy ( expfile.Type, fileoutext );

expfile.SetAtomType ( AtomTypeScalar );
expfile.NumTracks           = DestPoints.GetNumPoints ();
expfile.NumAuxTracks        = 0;
expfile.NumTime             = eegdoc->GetNumTimeFrames ();
expfile.SamplingFrequency   = eegdoc->GetSamplingFrequency ();

expfile.DateTime            = eegdoc->DateTime;

expfile.ElectrodesNames     = DestPointsNames;
                                        // here we have the exact value
expfile.MaxValue            = dataout.GetAbsMaxValue (); // eegdoc->GetAbsMaxValue ();

expfile.OutputTriggers      = (ExportTriggers) ( TriggersAsTriggers | MarkersAsMarkers );
expfile.Markers             = *eegdoc;
expfile.NumTags             = eegdoc->GetNumMarkers ();
expfile.TimeMin             = 0;
expfile.TimeMax             = expfile.NumTime - 1;

                                        // this should use some fast disk write
expfile.Write ( dataout );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! silent ) {

    Gauge.FinishParts ();

    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//OptimizeOn

}
