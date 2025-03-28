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

#include    "ESI.RisToVolume.h"
#include    "TRisToVolumeDialog.h"      // RisToVolumeInterpolationType, RisToVolumeFileType

#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "Files.TVerboseFile.h"
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
                    TSolutionPointsDoc*     spdoc,          RisToVolumeInterpolationType    interpol,
                    const TVolumeDoc*       mrigrey,
                    int                     fromtf,         int             totf,           int             steptf,
                    FilterTypes             merging,
                    AtomFormatType          atomformat,     
                    RisToVolumeFileType     filetype,       const char*     fileprefix,
                    TGoF&                   volgof,
                    bool                    silent
                    )

{
volgof.Reset ();

                                        // These are deal-breakers
if ( StringIsEmpty ( risfile ) || spdoc == 0 || mrigrey == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // These can be fixed, though
if ( steptf < 1 )
    steptf  = 1;


if ( ! (    merging == FilterTypeMedian 
         || merging == FilterTypeMean   ) )

    merging     = FilterTypeNone;


if ( ! (    atomformat == AtomFormatByte 
         || atomformat == AtomFormatFloat ) )

    atomformat  = RisToVolumeDefaultAtomFormat;


char                fileext[ 32 ];

if      ( IsFileTypeNifti   ( filetype ) )      StringCopy  ( fileext, FILEEXT_MRINII       );
else if ( IsFileTypeAnalyze ( filetype ) )      StringCopy  ( fileext, FILEEXT_MRIAVW_HDR   );
else                                            StringCopy  ( fileext, DefaultMriExt        );

bool                outputn3d       = IsFileTypeN3D ( filetype );
bool                outputn4d       = IsFileType4D  ( filetype );

if ( ! ( outputn3d || outputn4d ) )
    return;


                                        // force silent if not in interactive mode
if ( ! silent && CartoolObjects.CartoolApplication->IsNotInteractive () )
    silent  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( ! silent ) {

    Gauge.Set           ( RisToVolumeTitle, SuperGaugeLevelInter );
                                            // we don't know how many TF to actually save - but we give it 100% of progress bar
    Gauge.AddPart       ( 0, 100 );

    CartoolObjects.CartoolApplication->SetMainTitle ( RisToVolumeTitle, risfile, Gauge );
    }


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
double              radiusf         = 0;    // radius between SPs
double              kernelradiusf   = 0;    // real Kernel radius
int                 kerneldiameteri = 0;    // corresponding voxel (int) size of Kernel & radius
int                 kernelradiusi   = 0;


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
                                        // reading ris file
Gauge.Next ( 0, SuperGaugeUpdateTitle );

                                        // It could also be a TRisDoc*, and reading only the blocks needed...
TMaps               ris;

ris.ReadFile ( risfile, 0, AtomTypePositive, ReferenceNone );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating the same checks as in TRisToVolumeDialog
int                 numtf           = ris.GetNumMaps ();
int                 lasttimeframes  = numtf - 1;

Clipped ( fromtf, totf, 0, lasttimeframes );

                                        // at least writing 1 data point; also rounding the number of saved data points up
int                 numsavedblocks  = AtLeast ( 1, RoundAbove ( ( totf - fromtf + 1 ) / (double) steptf ) );
                                        // update the upper bound, using multiples of steptf
                    totf            = fromtf + numsavedblocks * steptf - 1;


if ( totf > lasttimeframes ) {
                                        // new upper bound does not fit in data range
    if ( numsavedblocks > 1 ) {
                                        // just decrease the number of blocks by 1
        numsavedblocks--;
        totf       -= steptf;
        }
    else { // numsavedblocks == 1
                                        // nope, can not go any lower, so we adjust the limits instead
        totf        = lasttimeframes;   // >= fromtf
        numtf       = totf - fromtf + 1;// >= 1
        steptf      = numtf;            // >= 1
        }
    }
                                        // Here: fromtf and totf are in [0..lasttimeframes]; totf adjusted for steptf

                                        // we can reset any filtering in that case
if ( steptf == 1 )
    merging     = FilterTypeNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! silent )
    Gauge.SetRange ( 0, numsavedblocks + 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TBoundingBox<int>*    volsize         = mrigrey->GetSize ();
TExportVolume               expvol;


expvol.VolumeFormat         = atomformat;

expvol.NumDimensions        = outputn4d ? 4              : 3;
expvol.Dimension.X          = volsize->GetXExtent ();
expvol.Dimension.Y          = volsize->GetYExtent ();
expvol.Dimension.Z          = volsize->GetZExtent ();
expvol.NumTimeFrames        = outputn4d ? numsavedblocks : 1;
expvol.SamplingFrequency    = ris.GetSamplingFrequency () / NonNull ( steptf ); // adjust the resulting sampling frequency

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

TFileName           BaseFileName;
TFileName           VerboseFile;
char                buff[ 256 ];


StringCopy      ( BaseFileName, risfile );
RemoveExtension ( BaseFileName );
if ( StringIsNotEmpty ( fileprefix ) )
    PrefixFilename  ( BaseFileName, StringCopy ( buff, fileprefix, "." ) );


StringCopy      ( VerboseFile,              BaseFileName,           "." "RIS To Volume",    "." FILEEXT_VRB );
CheckNoOverwrite( VerboseFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Results of Inverse Solution To Volumes" );



verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Solution Points file:",  spdoc->GetDocPath () );
verbose.Put ( "MRI Grey Mask file:",    mrigrey->GetDocPath () );
verbose.Put ( "RIS file:",              risfile );
}


verbose.NextTopic ( "Volume Parameters:" );
{
verbose.Put ( "Using interpolation:", VolumeInterpolationPresetString[ interpol ] );
//verbose.Put ( "Using interpolation:", SPInterpolationTypeNames[ spinterpol ] );
}


verbose.NextTopic ( "Time Parameters:" );
{
verbose.Put ( "From     [TF]:", fromtf );
verbose.Put ( "To       [TF]:", totf );
verbose.Put ( "By steps [TF]:", steptf );
verbose.Put ( "Number of blocks written:", numsavedblocks );
verbose.Put ( "Averaging each block:", merging == FilterTypeNone ? "None" : FilterPresets[ merging ].Ext /*Text*/ );
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Output data type:", AtomFormatTypePresets[ atomformat ].Text );
verbose.Put ( "Output format:", VolumeFileTypeString[ filetype ] );
verbose.Put ( "Output dimensions:", IsFileTypeN3D ( filetype ) ? 3 : 4 );

verbose.NextLine ();
verbose.Put ( "Verbose file (this):", VerboseFile );
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OK, can start the real job now
TMap                meanrismap  ( ris.GetDimension () );


for ( int blocki = 0; blocki < numsavedblocks; blocki++ ) {

                                        // current block TF range
    int     blockfromtf     = fromtf      + blocki * steptf;
    int     blocktotf       = blockfromtf +          steptf - 1;
    bool    firstblock      = blocki == 0;


    Gauge.Next ( 0, SuperGaugeUpdateTitle );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // generate file name
    if ( outputn3d 
      || outputn4d && firstblock ) {
                                        // set current volume's file name
        StringCopy          ( expvol.Filename,      BaseFileName );

        if ( outputn3d ) {
            StringAppend        ( expvol.Filename, ".TF", IntegerToString ( blockfromtf,    NumIntegerDigits ( lasttimeframes ) ) );
            if ( steptf > 1 )
                StringAppend    ( expvol.Filename, "-",   IntegerToString ( blocktotf,      NumIntegerDigits ( lasttimeframes ) ) );
            }
        else {
            StringAppend        ( expvol.Filename, ".TF", IntegerToString ( fromtf,         NumIntegerDigits ( lasttimeframes ) ) );
            if ( numsavedblocks > 1 )                                                        
                StringAppend    ( expvol.Filename, "-",   IntegerToString ( totf,           NumIntegerDigits ( lasttimeframes ) ) );
            }

        AddExtension        ( expvol.Filename, FILEEXT_RIS );   // output files XXX.ris.hdr
        AddExtension        ( expvol.Filename, fileext     );

        CheckNoOverwrite    ( expvol.Filename );

                                        // store current volume file
        volgof.Add          ( expvol.Filename );

                                        // for each written volume, either 3D or 4D
        expvol.TimeOrigin   = TimeFrameToSeconds ( blockfromtf, ris.GetSamplingFrequency () );  // current TF relative time
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // average a block of TFs, or simply copying a single data point
    if      ( merging == FilterTypeMedian   )   ris.Median ( blockfromtf, blocktotf, meanrismap );
    else if ( merging == FilterTypeMean     )   ris.Mean   ( blockfromtf, blocktotf, meanrismap );
    else                                        meanrismap  = ris[ blockfromtf ];


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute output volume according to the type of interpolation
    vol.ResetMemory ();

                                        // Solution points scan is longer and for linear and kernel interpolation cases
    if      ( IsSolutionPointsScan ( interpol ) ) {

        OmpParallelFor

        for ( int spi = 0; spi < points.GetNumPoints (); spi++ ) {

            TPointFloat         sp          = points    [ spi ];
            double              spv         = meanrismap[ spi ];

            mrigrey->ToRel ( sp );

            sp     += 0.5;

                                        // voxel is kernel shifted + truncated to voxel
            int                 xki, yki, zki;
            TPointInt           vox;

            for ( xki = 0, vox.X = sp.X - kernelradiusi; xki < kerneldiameteri; xki++, vox.X++ )
            for ( yki = 0, vox.Y = sp.Y - kernelradiusi; yki < kerneldiameteri; yki++, vox.Y++ )
            for ( zki = 0, vox.Z = sp.Z - kernelradiusi; zki < kerneldiameteri; zki++, vox.Z++ ) {

                   
                if ( grey.GetValueChecked ( vox ) <= mrithreshold ) continue;   // for exact grey mask
//              if ( ! grey.WithinBoundary ( vox ) )                continue;   // for debugging

                                        // floating point, exact position used for the weight
                TPointFloat         sp0         = vox - sp + 0.5;

                double              w           = 0;

                if      ( interpol == VolumeInterpolationLinearRect )
                                        // squared kernel, linear weight
                    w       = CubicRoot (   ( 1 - Clip ( abs ( sp0.X ) / kernelradiusf, 0.0, 1.0 ) )
                                          * ( 1 - Clip ( abs ( sp0.Y ) / kernelradiusf, 0.0, 1.0 ) )
                                          * ( 1 - Clip ( abs ( sp0.Z ) / kernelradiusf, 0.0, 1.0 ) ) );

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


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // atomic because different solution points could write simultaneously on vol
                OmpAtomic
                vol     ( vox )     += w * spv; // spread current solution point value to neighbors

                if ( firstblock ) {             // storing weights is needed only once
                    OmpAtomic
                    weights ( vox ) += w;       // keep count of all weights used at each voxel
                    }

                } // for kernel xki, yki, zki

            } // for solution point


        vol    /= weights;              // rescale by each cumulated weights at each voxel

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

            vol.LinearIndexToXYZ   ( li, pvol );

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

        } // IsVoxelScan


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set optimal MaxValue - note that no method will overshoot, ever
    if      ( outputn3d )               expvol.MaxValue     = vol.GetAbsMaxValue ();
    else if ( outputn4d && firstblock ) expvol.MaxValue     = ris.GetAbsMaxValue ( fromtf, totf );  // not optimal when writing integers + IsSolutionPointsScan


    expvol.Write ( vol, ExportArrayOrderZYX );

    } // for blocki


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // final verbose output files
{
verbose.NextLine ();


verbose.Put ( "Number of volumes files:", volgof.NumFiles () );

for ( int fi = 0; fi < volgof.NumFiles (); fi++ )
    verbose.Put ( "", volgof[ fi ] );


verbose.NextLine ();
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! silent ) {

    Gauge.FinishParts ();

    //CartoolObjects.CartoolApplication->SetMainTitle ( RisToVolumeTitle, volgof[ 0 ], Gauge );

    UpdateApplication;
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
