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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "TFilters.h"
#include    "Geometry.TPoint.h"
#include    "Math.Histo.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Strings.Utils.h"
#include    "TArray1.h"
#include    "TArray3.h"

#include    "GlobalOptimize.Tracks.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool    Volume::HeadCleanup ( FctParams& /*params*/, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeHeadCleanup ].Text, showprogress ? 6 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();

TVolume             headmask ( *this );
FctParams           p;

p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Keep biggest region, i.e. the head
Gauge.Next ();
                                        // erode handsomely by safety
p ( FilterParamDiameter )       = 3;
headmask.Filter ( FilterTypeErode, p );


Gauge.Next ();

headmask.Filter ( FilterTypeKeepBiggestRegion, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Post-process mask:
Gauge.Next ();
                                        // expand back + some more room
p ( FilterParamDiameter )       = 3 + 2;
headmask.Filter ( FilterTypeDilate, p );


//TFileName           _file;
//StringCopy ( _file, "E:\\Data\\HeadMask.nii" );
//headmask.WriteFile ( _file );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // soften the edge of mask - set in voxel size is OK
Gauge.Next ();

p ( FilterParamDiameter )     = 3.47;
headmask.Filter ( FilterTypeGaussian, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();
                                        // apply mask to original data
*this      *= headmask;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();

return  true;
}


//----------------------------------------------------------------------------
                                        // extract from the brain
bool    Volume::SegmentWhiteMatter ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


double              detailed        = Clip ( params ( FilterParamWhiteDetails ), (double) 0, (double) 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // Algorithm main parameters
double              MADSDsize       = 3.47;

//double              MADSDcut        = 0.75;     // 0.66: precise; 0.75: OK; 0.85: conservative
double              MADSDcut        = detailed ? ( 1 - detailed ) * ( 1.00 - 0.50 ) + 0.50 : 0.80;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeSegmentWhite ].Text, showprogress ? 6 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();

Volume              brainmask ( *this );
FctParams           p;


p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
brainmask.Filter ( FilterTypeToMask, p );


//*this       = brainmask;
//return  true;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Process White Matter
Gauge.Next ();
                                        // use MAD * SD
Volume              madsd ( *this );

p ( FilterParamDiameter   ) = MADSDsize;
p ( FilterParamResultType ) = FilterResultSigned;
madsd.Filter ( FilterTypeMADSDInv, p );

                                        // clip to mask
madsd.ApplyMaskToData ( brainmask );


//*this       = madsd;
//return  true;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Cut into MADSD
Gauge.Next ();
                                        // get the CDF
THistogram          H   (   madsd,
                            &brainmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF /*| HistogramSmooth*/ | HistogramNormNone | HistogramLinear )
                        );


double              hcut            = H.GetPercentilePosition ( RealUnit, MADSDcut );

//H.ToCDF ();                             // for display!
//H.WriteFile ( "E:\\Data\\Histo.MADSD.txt", false );
//DBGV ( hcut, "highcut" );


                                        // threshold to high MADSD values
p ( FilterParamThresholdMin )   = hcut;
p ( FilterParamThresholdMax )   = FLT_MAX;
p ( FilterParamThresholdBin )   = 1;
madsd.Filter ( FilterTypeThresholdBinarize, p );


//*this           = madsd;
//return  true;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) cut to threshold
Gauge.Next ();

TArray1<double>     values;


bool                greywhiteok     = GetGreyWhiteValues ( values );

double              whitecut        = values[ WhiteMin ];
//double              whitecut        = 0.50 * values[ GreyMin ] + 0.50 * values[ GreyMax ];  // use this due to inhomogeneities

                                        // Simple thresholding
if ( greywhiteok )
    for ( int i = 0; i < LinearDim; i++ )
        if ( GetValue ( i ) < whitecut )
            madsd ( i )  = 0;

/*                                      // Similarly to Thin Grey Matter, we can used some distance to Modes
if ( greywhiteok )

    for ( int i = 0; i < LinearDim; i++ ) {

        double      v       = GetValue ( i );

        if ( fabs ( v - values[ WhiteMode ] ) >= fabs ( v - values[ GreyMode ] ) + ( values[ GreyMax ] - values[ GreyMode ] ) )
            madsd ( i )  = 0;
        }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) remove remaining isolated voxels
Gauge.Next ();

p ( FilterParamNumNeighbors )     = 3;
p ( FilterParamNeighborhood )     = Neighbors26;
madsd.Filter ( FilterTypeLessNeighbors, p );

p ( FilterParamNumNeighbors )     = 2;
p ( FilterParamNeighborhood )     = Neighbors26;
madsd.Filter ( FilterTypeLessNeighbors, p );


                                        // remove white matter
//ClearMaskToData ( madsd );
*this           = madsd;

return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();

//return  true;
}


//----------------------------------------------------------------------------
                                        // extract grey from the brain, simple threshold - White
                                        // it works pretty well on real brains, but the MNI has a weird SD behavior, which spoils the White matter
/*
bool    Volume::SegmentGreyMatter ( FctParams&, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeSegmentGrey ].Text, showprogress ? 4 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Subtract the white
Gauge.Next ();
                                        // get white matter
Volume              whitematter ( *this );
FctParams           p;

                                        // less for MNI (to do)
p ( FilterParamWhiteDetails )   = 0.50;
//p ( FilterParamWhiteDetails )   = 1.00;
whitematter.SegmentWhiteMatter ( p );

ClearMaskToData ( whitematter );

//ShowMessage ( "Removed White", "" );
//return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Cut below grey & above white
Gauge.Next ();

TArray1<double>     values;


bool                greywhiteok     = GetGreyWhiteValues ( values, false );

double              greycut         = values[ GreyMin ];                        // should be this
//double            greycut         = 0.33 * values[ BlackMin ]   + 0.67 * values[ BlackMax ];  // use this due to inhomogeneities
double              whitecut        = 0.20 * values[ WhiteMin ] + 0.80 * values[ WhiteMax ];


if ( greywhiteok )
    for ( int i = 0; i < LinearDim; i++ )
        if ( GetValue ( i ) < greycut || GetValue ( i ) > whitecut )
            GetValue ( i )  = 0;


                                        // binarize
Gauge.Next ();

p ( FilterParamBinarized )     = 1;
Filter ( FilterTypeBinarize, p );


Gauge.Finished ();

return  true;
}
*/

//----------------------------------------------------------------------------

bool    Volume::SegmentGreyMatter ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


GreyMatterFlags     flags           = (GreyMatterFlags) (int) params ( FilterParamGreyType   );
int                 lraxis          =                         params ( FilterParamGreyAxis   );
int                 lrorigin        =                 Round ( params ( FilterParamGreyOrigin ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeSegmentGrey ].Text, showprogress ? 10 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full brain
Gauge.Next ();

Volume              brainmask ( *this );
FctParams           p;


double              backvalue       = GetBackgroundValue ();

p ( FilterParamToMaskThreshold )     = backvalue;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
brainmask.Filter ( FilterTypeToMask, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1.1) Smooth out?
Gauge.Next ();

if ( IsFlag ( flags, GreyMatterGaussian ) ) {
    p ( FilterParamDiameter )     = 3;
    Filter ( FilterTypeGaussian, p, false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1.2) Correct Bias Field?
Gauge.Next ();

if ( IsFlag ( flags, GreyMatterBiasField ) ) {
    p ( FilterParamBiasFieldRepeat )     = 1;
    Filter ( FilterTypeBiasField, p, true );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Get grey levels
Gauge.Next ();

TArray1<double>     values;
bool                greywhiteok     = GetGreyWhiteValues ( values );


if ( ! greywhiteok )
    return  false;

                                        // force black min to 0
values[ BlackMin ]  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2.1) Outer edge of the brain
/*
Gauge.Next ();

Volume              edgebrain;


if ( ! IsFlag ( flags, GreyMatterWhole ) ) {

    edgebrain   = *this;

                                        // clip to grey and white, with inner part filled (kind of pre-segmentation)
    p ( FilterParamToMaskThreshold )     = values[ GreyMin  ];
    p ( FilterParamToMaskNewValue  )     = values[ WhiteMax ];
    p ( FilterParamToMaskCarveBack )     = true;
    edgebrain.Filter ( FilterTypeToMask, p );

                                        // take internal part of the (external) edge
    p ( FilterParamDiameter )     = IsFlag ( flags, GreyMatterSlim ) ? 1 : 2;
    edgebrain.Filter ( FilterTypeMorphGradientInt, p );
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Volume              Grey ( Dim1, Dim2, Dim3 );

                                        // variables for Gaussian distribution
int                 numSD;              // in Gaussian
int                 neighsize;          // neighborhood size
int                 neighsize2;


if      ( IsFlag ( flags, GreyMatterSlim    ) ) {   numSD   = 1;    neighsize   = 3;    }
else if ( IsFlag ( flags, GreyMatterRegular ) ) {   numSD   = 2;    neighsize   = 5;    }
else if ( IsFlag ( flags, GreyMatterRegular ) ) {   numSD   = 2;    neighsize   = 5;    }
else if ( IsFlag ( flags, GreyMatterFat     ) ) {   numSD   = 2;    neighsize   = 5;    }
else                                           	{   numSD   = 1;    neighsize   = 5;    }

neighsize2      = neighsize / 2.0;

//numSD       = ::GetValue ( "Num SD:", "Seg Brain" );
//neighsize   = ::GetValue ( "neighsize:", "Seg Brain" );

                                        // Approximate Gaussian, using the actual grey level ranges
TGaussian           gaussb ( ( values[ BlackMin ] + values[ BlackMax ] ) / 2.0, ( values[ BlackMax ] - values[ BlackMin ] ) / 2.0, numSD );
TGaussian           gaussg ( ( values[ GreyMin  ] + values[ GreyMax  ] ) / 2.0, ( values[ GreyMax  ] - values[ GreyMin  ] ) / 2.0, numSD );
TGaussian           gaussw ( ( values[ WhiteMin ] + values[ WhiteMax ] ) / 2.0, ( values[ WhiteMax ] - values[ WhiteMin ] ) / 2.0, numSD );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Scan and evaluate greys
Gauge.Next ();

TBoundingBox<int>   bounding ( *this, false, 0.1 );

//SetOvershootingOption ( interpolate, Array, LinearDim, true );

OmpParallelFor

for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ ) {

    for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
    for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

                                        // don't count outside the mask!
        if ( ! brainmask ( x, y, z ) )
            continue;

        double      v       = GetValue ( x, y, z );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Gaussian probabilities of voxel value
        double      pblackgauss     = gaussb ( v );
        double      pgreygauss      = gaussg ( v );
        double      pwhitegauss     = gaussw ( v );
        double      pblacknum;
        double      pgreynum;
        double      pwhitenum;

//      { double totpn    = pblackgauss + pgreygauss + pwhitegauss;  if ( totpn == 0 )   totpn   = 1;    pblackgauss    /= totpn;    pgreygauss    /= totpn;    pwhitegauss    /= totpn; }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) neighborhood histogram probabilities
        if ( IsFlag ( flags, GreyMatterRegular ) 
          || IsFlag ( flags, GreyMatterFat     ) ) {

            pblacknum   = pgreynum  = pwhitenum = 0;

                                        // scan all voxels of the downsample cluster
            for ( int zd = 0; zd < neighsize; zd++ )
            for ( int yd = 0; yd < neighsize; yd++ )
            for ( int xd = 0; xd < neighsize; xd++ ) {

                double      vn      = GetValueChecked ( x + xd - neighsize2,
                                                        y + yd - neighsize2,
                                                        z + zd - neighsize2, InterpolateTruncate );

                                        // counting voxels in each category
  //            if ( IsInsideLimits ( vn, values[ BlackMin ], values[ BlackMax ] ) )    pblacknum++;
  //            if ( IsInsideLimits ( vn, values[ GreyMin  ], values[ GreyMax  ] ) )    pgreynum ++;
  //            if ( IsInsideLimits ( vn, values[ WhiteMin ], values[ WhiteMax ] ) )    pwhitenum++;

                                        // sum of square to most probable value - best results
                pblacknum += Square ( vn - values[ BlackMode ] );
                pgreynum  += Square ( vn - values[ GreyMode  ] );
                pwhitenum += Square ( vn - values[ WhiteMode ] );
                }

                                        // the smaller the error, the better
            pblacknum   = 1 / NonNull ( pblacknum );
            pgreynum    = 1 / NonNull ( pgreynum  );
            pwhitenum   = 1 / NonNull ( pwhitenum );
            } // scanneigh


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) merge probabilities
        if      ( IsFlag ( flags, GreyMatterSlim ) ) {
                                        // plain thresholds - good
//          Grey ( x, y, z )    = v >= values[ GreyMin  ] && v <= values[ GreyMax  ] ? 1 : 0;

                                        // distance to mode + penalty from estimated SD  - better
            Grey ( x, y, z )    = fabs ( v - values[ GreyMode ] ) < fabs ( v - values[ WhiteMode ] ) + ( values[ WhiteMode ] - values[ WhiteMin  ] )
                               && fabs ( v - values[ GreyMode ] ) < fabs ( v - values[ BlackMode ] ) + ( values[ BlackMax  ] - values[ BlackMode ] ) ? 1 : 0;

                                        // plain thresholds + forcing grey on white near the edge, where detection is usually bad and important!
//          Grey ( x, y, z )    = v >= values[ GreyMin  ] && ( v <= values[ GreyMax  ] || edgebrain ( x, y, z ) ) ? 1 : 0;
            }

        else if ( IsFlag ( flags, GreyMatterRegular ) ) {
                                        // the most logical formula
//          Grey ( x, y, z )    = pgreygauss  * pgreynum >  pblackgauss * pblacknum 
//                             && pgreygauss  * pgreynum >= pwhitegauss * pwhitenum ? 1 : 0;

                                        // gaussian only
//          Grey ( x, y, z )    = pgreygauss > pblackgauss && pgreygauss >= pwhitegauss ? 1 : 0;

                                        // count only
//          Grey ( x, y, z )    = pgreynum > pblacknum && pgreynum >= pwhitenum ? 1 : 0;

                                        // testing each probabilities independently
            Grey ( x, y, z )    = ( pgreynum >  pblacknum || pgreygauss >  pblackgauss )
                               && ( pgreynum >= pwhitenum || pgreygauss >= pwhitegauss ) ? 1 : 0;

                                        // testing each probabilities independently + forcing grey on white near the edge, where detection is usually bad and important!
//          Grey ( x, y, z )    =    ( pgreynum >  pblacknum                            || pgreygauss >  pblackgauss )
//                              && ( ( pgreynum >= pwhitenum || edgebrain ( x, y, z ) ) || pgreygauss >= pwhitegauss ) ? 1 : 0;
            }

        else if ( IsFlag ( flags, GreyMatterFat ) ) {
                                        // artificially boosting the grey probability
//          Grey ( x, y, z )    = pgreygauss  * pgreynum >  pblackgauss * pblacknum * 0.50 
//                             && pgreygauss  * pgreynum >= pwhitegauss * pwhitenum * 0.50 ? 1 : 0;

                                        // testing each probabilities independently
            Grey ( x, y, z )    =    ( pgreynum >  pblacknum        || pgreygauss >  pblackgauss * 0.50 )
                                  && ( pgreynum >= pwhitenum * 0.50 || pgreygauss >= pwhitegauss        ) ? 1 : 0;

                                        // testing each probabilities independently + forcing grey on white near the edge, where detection is usually bad and important!
//          Grey ( x, y, z )    =      ( pgreynum >  pblacknum                            || pgreygauss >  pblackgauss * 0.50 )
//                                && ( ( pgreynum >= pwhitenum || edgebrain ( x, y, z ) ) || pgreygauss >= pwhitegauss        ) ? 1 : 0;
            }

        else if ( IsFlag ( flags, GreyMatterWhole ) ) {
                                        // always "grey"
            Grey ( x, y, z )    = 1;
            }

        } // for y, z

    } // for x


//StringCopy ( expvolgrey.Filename, file );
//PostfixFilename ( expvolgrey.Filename, " Raw" );
//expvolgrey.Write ( Grey );
//StringCopy ( expvolgrey.Filename, file );

//TFileName           _file;
//StringCopy ( _file, "E:\\Data\\BrainToGrey.1.nii" );
//Grey.WriteFile ( _file );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Postprocessings

                                        // Could also use the ratio of # Grey / # White or # Brain to adjust the results..
if      ( IsFlag ( flags, GreyMatterPostprocessing ) ) {

    Gauge.Next ();
                                        // for all processing: clean-up floating / border voxels
    p ( FilterParamNumNeighbors )     = 9;
    p ( FilterParamNeighborhood )     = Neighbors26;
    Grey.Filter ( FilterTypeLessNeighbors, p );


    for ( int i = 0; i < 3; i++ ) {
        p ( FilterParamNumNeighbors )     = 4;
        p ( FilterParamNeighborhood )     = Neighbors26;
        Grey.Filter ( FilterTypeLessNeighbors, p );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ();

    if      ( IsFlag ( flags, GreyMatterSlim ) ) {
                                        // well, nothing more for now
        }

    else if ( /*IsFlag ( flags, GreyMatterRegular ) ||*/ IsFlag ( flags, GreyMatterWhole ) ) {
                                        // minimal close + filter in-between
        p ( FilterParamDiameter   ) = 2;
        p ( FilterParamResultType ) = FilterResultSigned;
        Grey.Filter ( FilterTypeMax, p );

        p ( FilterParamDiameter   ) = 2.83;
        p ( FilterParamResultType ) = FilterResultSigned;
        Grey.Filter ( FilterTypeMedian, p );

        p ( FilterParamDiameter   ) = 2;
        p ( FilterParamResultType ) = FilterResultSigned;
        Grey.Filter ( FilterTypeMin, p );

        } // GreyMatterRegular || GreyMatterWhole

    else if ( IsFlag ( flags, GreyMatterFat ) ) {
                                        // slightly stronger close
        p ( FilterParamDiameter )   = 1;
        Grey.Filter ( FilterTypeClose, p );

//        p ( FilterParamDiameter   ) = 2;
//        p ( FilterParamResultType ) = FilterResultSigned;
//        Grey.Filter ( FilterTypeMax, p );

        p ( FilterParamDiameter   ) = 2.83;
        p ( FilterParamResultType ) = FilterResultSigned;
        Grey.Filter ( FilterTypeMedian, p );

        } // GreyMatterFat


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ();
                                        // clear mask again to get rid of leaky results
    Grey.ApplyMaskToData ( brainmask );

                                        // clear Black/LCR again after the filters
    if ( ! IsFlag ( flags, GreyMatterSlim ) )
        for ( int i = 0; i < LinearDim; i++ )
            if ( GetValue ( i ) < values[ GreyMin ] ) // ( IsFlag ( flags, GreyMatterWhole ) ? values[ GreyMin ] : backvalue ) )
                Grey ( i )  = 0;

                                        // just in case, remove small floating blobs
    Grey.KeepRegion ( SortRegionsCount, 50, INT_MAX, Neighbors26, 0 );

                                        // binarize
    p ( FilterParamBinarized )     = 1;
    Grey.Filter ( FilterTypeBinarize, p );
    } // postprocessing
else {
    Gauge.Next ();
    Gauge.Next ();
    Gauge.Next ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Symmetrize?
Gauge.Next ();

if ( IsFlag ( flags, GreyMatterSymmetric ) && lrorigin != 0 ) {
//    DBGV2 ( lraxis, lrorigin, "lraxis, lrorigin" );

    if ( lraxis == 0 ) {

        for ( int y = 0; y < Dim2; y++ )
        for ( int z = 0; z < Dim3; z++ )

              for ( int x1 = lrorigin - 1, x2 = lrorigin + 1; x1 >= 0 && x2 < Dim1; x1--, x2++ ) // more centered
//            for ( int x1 = lrorigin - 1, x2 = lrorigin; x1 >= 0 && x2 < Dim1; x1--, x2++ )     // more left
                if ( Grey ( x1, y, z ) || Grey ( x2, y, z ) )
                    Grey ( x1, y, z )   = Grey ( x2, y, z ) = 1;
        } // lraxis == 0

    else if ( lraxis == 1 ) {

        for ( int x = 0; x < Dim1; x++ )
        for ( int z = 0; z < Dim3; z++ )

              for ( int y1 = lrorigin - 1, y2 = lrorigin + 1; y1 >= 0 && y2 < Dim2; y1--, y2++ ) // more centered
//            for ( int y1 = lrorigin - 1, y2 = lrorigin; y1 >= 0 && y2 < Dim2; y1--, y2++ )     // more left
                if ( Grey ( x, y1, z ) || Grey ( x, y2, z ) )
                    Grey ( x, y1, z )   = Grey ( x, y2, z ) = 1;
        } // lraxis == 1

    else if ( lraxis == 2 ) {

        for ( int x = 0; x < Dim1; x++ )
        for ( int y = 0; y < Dim2; y++ )

              for ( int z1 = lrorigin - 1, z2 = lrorigin + 1; z1 >= 0 && z2 < Dim3; z1--, z2++ ) // more centered
//            for ( int z1 = lrorigin - 1, z2 = lrorigin; z1 >= 0 && z2 < Dim3; z1--, z2++ )     // more left
                if ( Grey ( x, y, z1 ) || Grey ( x, y, z2 ) )
                    Grey ( x, y, z1 )   = Grey ( x, y, z2 ) = 1;
        } // lraxis == 2

                                        // Median: don't for Slim, otherwise?
                                        // result can  have some weird duplications, so smooth it out a bit
//    p ( FilterParamDiameters  )   = 2.83;
//    p ( FilterParamResultType )   = FilterResultSigned;
//    Grey.Filter ( FilterTypeMedian, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();

                                        // do some checking...
//bool                includingok     = IsIncluding ( Grey, true, GetBackgroundValue (), Grey.GetBackgroundValue () );
bool                emptygrey       = Grey.GetNumSet () == 0;

                                        // copy result
*this       = Grey;

//DBGV3 ( greywhiteok, emptygrey, includingok, "greywhiteok, emptygrey, includingok" );
                                        // fool-proof test
return  greywhiteok && ! emptygrey /*&& includingok*/;
}


//----------------------------------------------------------------------------

bool    Volume::SegmentCSF ( FctParams& /*params*/, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeSegmentCSF ].Text, showprogress ? 4 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();

Volume              brainmask ( *this );
FctParams           p;


p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
brainmask.Filter ( FilterTypeToMask, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Correct Bias Field?
//Gauge.Next ();
//
//if ( flags & GreyMatterBiasField ) {
//    p ( FilterParamBiasFieldRepeat )     = 1;
//    Filter ( FilterTypeBiasField, p, showprogress );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get grey levels
Gauge.Next ();

TArray1<double>     values;

GetGreyWhiteValues ( values );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

TBoundingBox<int>   bounding ( *this, false, 0.1 );
Volume              Black ( Dim1, Dim2, Dim3 );
TPointDouble        point;
int                 blackmax        = values[ BlackMax ];

                                        // basic thresholding
OmpParallelFor

for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ ) {

    for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
    for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

        if ( ! brainmask ( x, y, z ) )
            continue;

        double      v       = GetValue ( x, y, z );

        Black ( x, y, z )    = v < blackmax ? 1 : 0;
        } // for y, z
    } // for x


*this       = Black;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();

return  true;
}


//----------------------------------------------------------------------------
                                        // Currently splitting into CSF / Grey Matter / White Matter
bool    Volume::SegmentTissues ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


GreyMatterFlags     flags       	= (GreyMatterFlags) (int) params ( FilterParamTissuesParams );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeSegmentTissues ].Text, showprogress ? 5 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();

Volume              brainmask ( *this );
FctParams           p;


p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
brainmask.Filter ( FilterTypeToMask, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Correct Bias Field?
Gauge.Next ();

if ( IsFlag ( flags, GreyMatterBiasField ) ) {
    p ( FilterParamBiasFieldRepeat )     = 1;
    Filter ( FilterTypeBiasField, p, true );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get grey levels
Gauge.Next ();

TArray1<double>     values;

GetGreyWhiteValues ( values );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

Volume              Segment ( Dim1, Dim2, Dim3 );
TPointDouble        point;
TBoundingBox<int>   bounding ( *this, false, 0.1 );

                                        // basic thresholding
OmpParallelFor

for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ ) {

    for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
    for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

        if ( ! brainmask ( x, y, z ) )
            continue;

        double      v       = GetValue ( x, y, z );

        Segment ( x, y, z )  =   v <  values[ BlackMin ] ? 0
                               : v <  values[ GreyMin  ] ? 1
                               : v <  values[ GreyMax  ] ? 2
                               : v <= values[ WhiteMax ] ? 3
                               :                           4;
        } // for y, z
    } // for x


*this       = Segment;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();

return  true;
}


//----------------------------------------------------------------------------
/*                                        // this is a copy from the full head
double  Volume::SegmentCerebellum ( Volume &brainmask, double probregion2, bool showprogress )
{
if ( IsNotAllocated () )
    return  0;

                                        // test size
int                 maxsize         = Power2Rounded ( MaxSize () );

if ( maxsize != 128 ) {
//  ShowMessage ( "This segmentation is currently tuned for volumes of about 128 voxels side size only.\nIt appears the current volume does not fit to that size, unfortunately...\n\nThe operation is aborted, sorry!", "MRI Segmentation", ShowMessageWarning );
    return  0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "Cerebellum Segmentation", showprogress ? 10 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy full head now
Gauge.Next ();

Volume              kneg ( *this );
//Volume            original ( *this );


//*this       = brainmask;
//ShowMessage ( "brainmask", "" );
//*this       = original;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Compute inside, using K curvature+
Gauge.Next ();

FctParams           p;
p ( FilterParamDiameter   )     = 5;
p ( FilterParamResultType )     = FilterResultPositive;
Filter ( FilterTypeKCurvature, p );

                                        // cut within brain only
ApplyMaskToData ( brainmask );


p ( FilterParamBinarized )     = 255;
Filter ( FilterTypeBinarize, p );
//ShowMessage ( "K+", "" );
                                        // temp save
//Volume            kpos ( *this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Cut geometrical boundaries from K curvature-
Gauge.Next ();

p ( FilterParamDiameter   )     = 7;
p ( FilterParamResultType )     = FilterResultNegative;
kneg.Filter ( FilterTypeKCurvature, p );
//*this       = kneg;
//ShowMessage ( "K cut", "" );


p ( FilterParamBinarized )     = 255;
kneg.Filter ( FilterTypeBinarize, p );

                                        // lose fat, but keep connections
Gauge.Next ();

p ( FilterParamThinningRepeat )     = 2;
kneg.Filter ( FilterTypeThinning, p );
//*this       = kneg;
//ShowMessage ( "K thinning", "" );

                                        // connect as much as possible the missing bridges, without filling too much
p ( FilterParamDiameter )     = 2;
//kneg.Filter ( FilterTypeClose, p );
kneg.Filter ( FilterTypeDilate, p );
//*this       = kneg;
//ShowMessage ( "K close", "" );

                                        // connect missing bridges, make it thicker, too
p ( FilterParamDiameter )     = 1;
//kneg.Filter ( FilterTypeDilate, p );
kneg.Filter ( FilterTypeErode, p );
//*this       = kneg;
//ShowMessage ( "K dilate", "" );

                                        // temp restore
//*this       = kpos;
                                        // cut cereb with boundaries
ClearMaskToData ( kneg );

//ShowMessage ( "cereb after K cut", "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Detach the Cerebellum from the remaining Brain Stem connection
                                        //    the other parts of the brain are already well detached

Gauge.Next ();
                                        // shrink by looking at the % of Fullness, and cut below threshold
                                        // use a  big filter + histogram to get the cut value
Volume              perful ( *this );
p ( FilterParamDiameter )     = 9.00;
perful.Filter ( FilterTypePercentFullness, p );

                                                                        // ignore background!
double              pfcut           = perful.GetPercentilePosition ( 0.90, true, &brainmask );

//DBGV ( pfcut, "pfcut" );

//if ( pfcut < 41 )
//    pfcut   = 41;                       // min limit

if ( pfcut > 55 )
    pfcut   = 55;                       // max limit, usefull for MNI template - could be less

                                        // cut lowest connectivities
for ( int i = 0; i < LinearDim; i++ )
    if ( perful[ i ] < pfcut )
        GetValue ( i ) = 0;

//ShowMessage ( "cutting %F", "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) The cerebellum is now supposedly detached from the brain, just pick it up
Gauge.Next ();

//ShowMessage ( "before region", "" );

//DBGV2 ( brainmask.GetNumSet (), brainmask.GetNumSet () * 0.09, "#Mask  Limit" );
                                        // set a max limit to the size of cerebellum (6% of brain), for the MNI case has big remaining cortices (13% of brain) that have to be ignored
KeepRegion ( SortRegionsCompactCount, brainmask.GetNumSet () * 0.005, brainmask.GetNumSet () * 0.09, Neighbors26, probregion2 );

//ShowMessage ( "keep region", "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Postprocessing the cerebellum skeleton we have (make it bigger and smoother)
Gauge.Next ();

p ( FilterParamDiameter )     = 5;                        // dilate is quite fat, for all cases
p ( FilterParamDiameter )    += 3;                        // + first part of Close
Filter ( FilterTypeDilate, p );
//ShowMessage ( "dilate", "" );

                                        // second part of Close
Gauge.Next ();

p ( FilterParamDiameter )     = 3;
Filter ( FilterTypeErode, p );
//ShowMessage ( "close", "" );

                                        // wide smoothing
Gauge.Next ();

p ( FilterParamDiameter )     = 9;
p ( FilterParamNumRelax )     = 2;
Filter ( FilterTypeRelax, p );

                                        // punch brain mask, to clip any thick part outside the brain
ApplyMaskToData ( brainmask );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Evaluate quality of final cerebellum
Gauge.Next ();

TBoundingBox<int>   boxbrain ( brainmask, 0.1 );
TBoundingBox<int>   boxcereb ( *this,    0.1 );

double              bbr             = boxbrain.Radius ();
double              bcr             = boxcereb.Radius ();

TPointFloat         bbc;
TPointFloat         bcc;
boxbrain.GetMiddle ( bbc );
boxcereb.GetMiddle ( bcc );

                                        // ratio of the distance from each centers to the mean max distance
//double              eccentricity    = Clip ( ( bbc - bcc ).Norm () / ( bbr - bcr ? bbr - bcr : 1 ), 0, 1 );
//DBGV ( eccentricity * 100, "eccentricity" );
                                        // a per axis ratio of distance center / border
double              eccentricityx   = Clip ( fabs ( bbc.X - bcc.X ) / ( boxbrain.GetXExtent () - boxcereb.GetXExtent () ) * 2, 0, 1 );
double              eccentricityy   = Clip ( fabs ( bbc.Y - bcc.Y ) / ( boxbrain.GetYExtent () - boxcereb.GetYExtent () ) * 2, 0, 1 );
double              eccentricityz   = Clip ( fabs ( bbc.Z - bcc.Z ) / ( boxbrain.GetZExtent () - boxcereb.GetZExtent () ) * 2, 0, 1 );
//DBGV3 ( eccentricityx * 100, eccentricityy * 100, eccentricityz * 100, "eccentricity x y z" );

                                        // good cerebs are centered -> low minecc
double              minecc          = min ( eccentricityx, eccentricityy, eccentricityz );
                                        // good cerebs are at the border -> high maxecc
double              maxecc          = max ( eccentricityx, eccentricityy, eccentricityz );
                                        // good cerebs have a radius ratio of ~50..60%, look how we differ from that number
double              voldiff         = Clip ( fabs ( bcr / ( bbr ? bbr : 1 ) - 0.50 ) * 2, 0, 1 );
                                        // build a confidence factor on these 3 measures
double              cerebellity     = ( 1 - minecc ) * maxecc * ( 1 - voldiff );

//DBGV3 ( bcr, bbr, bcr / ( bbr ? bbr : 1 ) * 100, "cerebr brainr -> volratio" );
//DBGV4 ( ( 1 - minecc ) * 100, maxecc * 100, ( 1 - voldiff ) * 100, cerebellity * 100, "minecc maxecc voldiff -> cerebellity" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//ShowMessage ( "Done Cereb", "" );

Gauge.Finished ();

return  cerebellity;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







