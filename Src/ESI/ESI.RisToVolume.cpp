/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "ESI.RisToVolume.h"
#include    "TRisToVolumeDialog.h"      // VolumeInterpolationPreset

#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TVolume.h"
#include    "TWeightedPoints.h"

#include    "TExportVolume.h"
#include    "TVolumeDoc.h"
#include    "TSolutionPointsDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    RisToVolume (
                    const char*             risfile,
                    TSolutionPointsDoc*     spdoc,          VolumeInterpolationPreset   interpol,
                    const TVolumeDoc*       mrigrey,
                    int                     fromtf,         int             totf,           int             steptf,
                    FilterTypes             merging,
                    AtomFormatType          atomformat,     
                    const char*             fileprefix,     char*           fileext,
                    TGoF&                   volgof,
                    TSuperGauge*            gauge
                    )

{
volgof.Reset ();

                                        // Parameters checking
if ( StringIsEmpty ( risfile ) || spdoc == 0 || mrigrey == 0 )
    return;


if ( ! (    atomformat == AtomFormatByte 
         || atomformat == AtomFormatFloat ) )
    return;


if ( StringIsEmpty ( fileext ) )
//  return;
    StringCopy  ( fileext, DefaultMriExt );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Initialization for interpolation

const Volume&       grey            = *mrigrey->GetData ();
TVolume<double>     vol ( grey.GetDim1 (), grey.GetDim2 (), grey.GetDim3 () );
int                 mrithreshold    = mrigrey->GetCsfCut ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Variables for Voxels scan - Nearest Neighbors cases

const TArray3<ushort>*            toip1nn         = 0;
const TArray3<TWeightedPoints4>*  toip4nn         = 0;

                                        // Some volume interpolation needs the SP interpolation
SPInterpolationType         spinterpol  = interpol == VolumeInterpolation1NN        ?   SPInterpolation1NN
                                        : interpol == VolumeInterpolation4NN        ?   SPInterpolation4NN
                                        :                                               SPInterpolationNone;    // all other cases

                                        // Initialize the requested SP interpolation
if ( ! spdoc->BuildInterpolation ( spinterpol, mrigrey ) )
    return;
    

if      ( spinterpol == SPInterpolation1NN  )   toip1nn = spdoc->GetInterpol1NN ();
else if ( spinterpol == SPInterpolation4NN  )   toip4nn = spdoc->GetInterpol4NN ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Variables for Solution Points scan

TPoints             points;
TVolume<double>     weights;
double              radiusf;            // radius between SPs
double              kernelradiusf;      // real Kernel radius
int                 kerneldiameteri;    // corresponding voxel (int) size of Kernel & radius
int                 kernelradiusi;


if ( IsSolutionPointsScan ( interpol ) ) {

    weights.Resize ( grey.GetDim1 (), grey.GetDim2 (), grey.GetDim3 () );
    points  = spdoc->GetPoints ( DisplaySpace3D );


    radiusf     = spdoc->GetMedianDistance ();

    if      ( interpol == VolumeInterpolationLinearRect                     )   kernelradiusf   =                      radiusf;     // square    kernel
//  else if ( interpol == VolumeInterpolationLinearSpherical                )   kernelradiusf   =         sqrt ( 3 ) * radiusf;     // spherical kernel - boosting the Kernel size so as to reach the diagonal vertices
//  else if ( interpol == VolumeInterpolationQuadraticFastSplineSpherical   )   kernelradiusf   = /*1.5*/ sqrt ( 3 ) * radiusf;     // spherical kernel - boosting just a little bit to reach the diagonal vertices
    else if ( interpol == VolumeInterpolationCubicFastSplineSpherical       )   kernelradiusf   = 2.0                * radiusf;     // spherical kernel - no need to boost, kernel is big enough already


    kerneldiameteri = DiameterToKernelSize ( 2 * kernelradiusf, OddSize );
    kernelradiusi   = kerneldiameteri / 2;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TBoundingBox<int>*    volsize         = mrigrey->GetSize ();
TExportVolume               expvol;


expvol.VolumeFormat         = atomformat;

//expvol.MaxValue             = MAXBYTE;    // after normalization

expvol.Dim          [ 0 ]   = volsize->GetXExtent ();
expvol.Dim          [ 1 ]   = volsize->GetYExtent ();
expvol.Dim          [ 2 ]   = volsize->GetZExtent ();

expvol.VoxelSize            = mrigrey->GetVoxelSize ();

expvol.RealSize             = mrigrey->GetRealSize ();

expvol.Origin               = mrigrey->GetOrigin ();

//if ( MRIDoc->HasKnownOrientation () ) // always saving orientation?
    mrigrey->OrientationToString ( expvol.Orientation );

                                        // output volumes have the same space meaning as input volume
expvol.NiftiTransform       = mrigrey->GetNiftiTransform ();
                                        // guess from input file
expvol.NiftiIntentCode      = GuessNiftiIntentFromFilename ( risfile );
                                        // put some short info here
StringCopy  ( expvol.NiftiIntentName, NiftiIntentNameRis );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reading ris file
if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // It could also be a TRisDoc*, and reading only the blocks needed...
TMaps               ris;

ris.ReadFile ( risfile, 0, AtomTypePositive, ReferenceNone );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtf           = ris.GetNumMaps ();
int                 maxtf           = numtf - 1;


Clipped ( fromtf, totf, 0, maxtf );

Clipped ( steptf,       1, numtf );


if ( gauge )    gauge->SetRange ( -1, ( totf - fromtf + steptf ) / steptf + 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              risabsmax       = ris.GetAbsMaxValue ();
double              rescalefactor   = 1;


if      ( IsFormatFloat   ( atomformat ) )
                                        // floating points can get all the precision they need, no need for rescaling
    rescalefactor       = 1;

else if ( atomformat == AtomFormatByte ) {
                                        // getting an simple power of 10 rescaling factor - drawback is the resulting lack of precision...
                                        // also note that resulting final max might be lower than that due to possible time merging
//  rescalefactor   = RescalingFactor ( risabsmax, MAXBYTE );
                                        // rescaling to 255
    rescalefactor   = MAXBYTE / (double) risabsmax;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OK, can start the real job now
TMap                meanrismap ( ris.GetDimension () );

                      // be more strict by allowing only full blocks of steptf length
for ( int tf = fromtf; ( tf + steptf - 1 ) <= totf; tf += steptf ) {

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // set current volume's file name
    StringCopy          ( expvol.Filename, risfile    );
    PrefixFilename      ( expvol.Filename, fileprefix );
    RemoveExtension     ( expvol.Filename );

    StringAppend        ( expvol.Filename, ".TF", IntegerToString ( tf, NumIntegerDigits ( maxtf ) ) );
    if ( steptf > 1 )
        StringAppend    ( expvol.Filename, "-", IntegerToString ( NoMore ( totf /*maxtf*/, tf + steptf - 1 ), NumIntegerDigits ( maxtf ) ) );

    AddExtension        ( expvol.Filename, FILEEXT_RIS );   // output files XXX.ris.hdr
    AddExtension        ( expvol.Filename, fileext     );


    CheckNoOverwrite    ( expvol.Filename );

                                        // store current volume file
    volgof.Add          ( expvol.Filename );


    expvol.Begin ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // average a block of TFs, or simply copy if 1 TF
    if      ( merging == FilterTypeMedian   )   ris.Median ( tf, tf + steptf - 1, meanrismap );
    else if ( merging == FilterTypeMean     )   ris.Mean   ( tf, tf + steptf - 1, meanrismap );
    else                                        ris.Mean   ( tf, tf + steptf - 1, meanrismap );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    vol.ResetMemory ();

                                        // Solution points scan is longer and for linear and kernel interpolation cases
    if ( IsSolutionPointsScan ( interpol ) ) {

        weights.ResetMemory ();


        OmpParallelFor

        for ( int spi = 0; spi < points.GetNumPoints (); spi++ ) {

            TPointFloat         sp          = points[ spi ];

            mrigrey->ToRel ( sp );

            sp += 0.5;

                                        // voxel is kernel shifted + truncated to voxel
            int                 xki, yki, zki;
            TPointInt           vox;

            for ( xki = 0, vox.X = sp.X - kernelradiusi; xki < kerneldiameteri; xki++, vox.X++ )
            for ( yki = 0, vox.Y = sp.Y - kernelradiusi; yki < kerneldiameteri; yki++, vox.Y++ )
            for ( zki = 0, vox.Z = sp.Z - kernelradiusi; zki < kerneldiameteri; zki++, vox.Z++ ) {

                   
                if ( grey.GetValueChecked ( vox ) <= mrithreshold ) continue;   // for exact grey mask
//              if ( ! grey.WithinBoundary ( vox ) )                continue;   // for debugging

                                        // floating point, exact position used for the weight
                TPointFloat         sp0;
                sp0.X   = vox.X - sp.X + 0.5;
                sp0.Y   = vox.Y - sp.Y + 0.5;
                sp0.Z   = vox.Z - sp.Z + 0.5;

                double              w;

                if      ( interpol == VolumeInterpolationLinearRect )
                                        // squared kernel, linear weight
                    w       = CubicRoot (   ( 1 - Clip ( fabs ( sp0.X ) / kernelradiusf, 0.0, 1.0 ) )
                                          * ( 1 - Clip ( fabs ( sp0.Y ) / kernelradiusf, 0.0, 1.0 ) )
                                          * ( 1 - Clip ( fabs ( sp0.Z ) / kernelradiusf, 0.0, 1.0 ) ) );

//              else if ( interpol == VolumeInterpolationLinearSpherical )
//                                  // radial kernel, linear weight, with exact position
//                  w       = 1 - NoMore ( 1.0, sp0.Norm () / kernelradiusf );
//
//              else if ( interpol == VolumeInterpolationQuadraticFastSplineSpherical ) {
//                                                                           // kernel spans on 3 intervals / 4 points -> center & span = 1.5
//                  double  dk  = NoMore ( 1.0, sp0.Norm () / kernelradiusf ) * 1.5 + 1.5;
//                                  // piecewise definition
//                  if ( dk < 2.0 )     w   = -2 * Square ( dk ) + 6 * dk -3;
//                  else                w   = Square ( 3 - dk );
//                  }

                else if ( interpol == VolumeInterpolationCubicFastSplineSpherical ) {
                                                                                // kernel spans on 4 intervals / 5 points -> center & span = 2
                    double      dk      = NoMore ( 1.0, sp0.Norm () / kernelradiusf ) * 2.0 + 2.0;
                                    // piecewise definition
                    if ( dk < 2.0 )  {  dk -= 2;    w   = 3 * Cube ( dk ) - 6 * Square ( dk ) + 6 * dk + 4; }
                    else                            w   = Cube ( 4 - dk );
                    }


                OmpAtomic
                vol     ( vox ) += w * meanrismap[ spi ];
                OmpAtomic
                weights ( vox ) += w;
                } // for kernel xki, yki, zki

            } // for solution point


        vol    /= weights;              // cumulated weights applied for each voxel
        vol    *= rescalefactor;
        } // IsSolutionPointsScan


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Voxel scan is simpler and for 1NN and 4NN interpolation cases
    else if ( IsVoxelScan ( interpol ) ) {

                                        // do current volume
        OmpParallelFor

        for ( int li = 0; li < vol.GetLinearDim (); li++ ) {
                                    // clip to grey, even if there are some interpolation available (used for inverse display mostly)
            if ( grey[ li ] <= mrithreshold )
                continue;


            TPointFloat     pvol;

            vol.LinearIndexToXYZ ( li, pvol );

            mrigrey ->ToAbs            ( pvol );

            spdoc   ->AbsoluteToVolume ( pvol );

            pvol.Round ();


            double          v       = 0;

            if      ( interpol == VolumeInterpolation1NN ) {

                if ( ! toip1nn->WithinBoundary ( pvol ) )                               continue;

                if (   toip1nn->GetValue       ( pvol ) == UndefinedInterpolation1NN )  continue;

                v   = meanrismap ( toip1nn->GetValue ( pvol ) );

                } // VolumeInterpolation1NN

            else if ( interpol == VolumeInterpolation4NN ) {

                if ( ! toip4nn->WithinBoundary ( pvol ) )       continue;

                const TWeightedPoints4* toi4    = &toip4nn->GetValue ( pvol );

                if ( toi4->IsNotAllocated () )                  continue;

                v   = (   toi4->w1 * meanrismap ( toi4->i1 )
                        + toi4->w2 * meanrismap ( toi4->i2 )
                        + toi4->w3 * meanrismap ( toi4->i3 )
                        + toi4->w4 * meanrismap ( toi4->i4 ) ) / TWeightedPoints4SumWeights;

                } // VolumeInterpolation4NN


            vol[ li ]   = v;
            } // for z, y, x


        vol    *= rescalefactor;
        } // IsVoxelScan


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    expvol.Write ( vol, ExportArrayOrderZYX );

    } // for tf


if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
