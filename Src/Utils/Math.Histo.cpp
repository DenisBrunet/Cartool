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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Resampling.h"
#include    "Math.Stats.h"
#include    "TVector.h"
#include    "Dialogs.Input.h"
#include    "TVolume.h"
#include    "Math.Utils.h"
#include    "Math.Histo.h"
#include    "Files.Stream.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        THistogram::THistogram ()
      : TVector<double> ()
{
Reset ();
}


        THistogram::THistogram ( int numbins )
      : TVector<double> ()  // NOT calling constructor with numbins, as we want to be sure to have a min size histogram
{
Reset ();

Set ( crtl::AtLeast ( 1, numbins ) );
}


void    THistogram::Set ( int numbins )
{
                                                     // Making sure memory is properly reset
Resize ( numbins, (MemoryAllocationType) ( MemoryAuto | ResizeClearMemory ) );
}


void    THistogram::Reset ()
{
ResetMemory ();

MarginFactor        = 0;
KernelSubsampling   = 1;
}


        THistogram::THistogram ( const THistogram &op )
      : TVector<double> ( op )
{
MarginFactor        = op.MarginFactor;
KernelSubsampling   = op.KernelSubsampling;
}


THistogram& THistogram::operator= ( const THistogram &op2 )
{
TVector<double>::operator= ( op2 );

MarginFactor        = op2.MarginFactor;
KernelSubsampling   = op2.KernelSubsampling;

return  *this;
}

                                        // THistogram is derived from TVector<double>, so we can not assign directly...
THistogram& THistogram::operator= ( const TVector<float>& op2 )
{
if ( Dim1 != op2.GetDim1 () )
                                        // no need to reset memory, as we will copy from op2 right after this
    Resize ( op2.GetDim1 (), (MemoryAllocationType) ( MemoryAuto | ResizeNoReset ) );


for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  = op2[ i ];

return  *this;
}


THistogram& THistogram::operator= ( const TVector<double>& op2 )
{
TVector<double>::operator= ( op2 );

return  *this;
}


THistogram& THistogram::operator= ( double op2 )
{
TVector<double>::operator= ( op2 );

return  *this;
}


//----------------------------------------------------------------------------
                                        // Kernel Density, min and max data fully specified + some margin options
                                        // compute the optimal histogram size needed to represent the data + the linear transform from data to bins
                                        // Note: function could be moved outside of THistogram
void    THistogram::ComputeHistogramSize (  double      kerneldensity,  double      mindata,            double      maxdata,
                                            double      marginfactor,   double      kernelsubsampling,
                                            double&     curvesize,      double&     curvemin,           double&     curveratio 
                                         )
{
                                        // sanity checks
crtl::Maxed ( marginfactor,         (double) 0 );
crtl::Maxed ( kernelsubsampling,    (double) 1 );

                                        // Storing these can be useful later on
MarginFactor        = marginfactor;
KernelSubsampling   = kernelsubsampling;


if ( kerneldensity <= 0 ) {
    curvesize   = HistoMinSize;
    curvemin    = 0;
    curveratio  = 1;
    return;
    }

                                        // the actual final bin size, which accounts of any optional sub-sampling
double              highkerneldensity   = kerneldensity / kernelsubsampling;
                                        // estimate if the input data is quantized vs real floats (very unlikely to have an integer kernel from real data!)
bool                quantizeddata       = IsInteger ( kerneldensity /*highkerneldensity*/ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // updating the min and max needed for the histogram
                    curvemin        = mindata                       // first existing data
                                    - marginfactor * kerneldensity; // caller can request some additional room before and after, which can be handy in case of smoothing

double              curvemax        = maxdata 
                                    + marginfactor * kerneldensity;

                                        // accounting for the bucket mean position - !this means access to bin array needs Round the real values (not Truncate)!
if ( quantizeddata ) {
                                        // shift half the integer bin size
    curvemin   += ( highkerneldensity - 1 ) / 2.0;
    curvemax   -= ( highkerneldensity - 1 ) / 2.0;
                                        // truncating left - !also change ToBin function!
//  curvemax   -= ( highkerneldensity - 1 );
    }
else {
                                        // shift half the real bin size
    curvemin   += highkerneldensity / 2;
    curvemax   -= highkerneldensity / 2;
                                        // truncating left - !also change ToBin function!
//  curvemax   -= highkerneldensity;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if not already provided (most cases), estimate curve size from data
                                        // otherwise, trust the value by the caller, which should come from some earlier estimates (f.ex. across more data)
if ( curvesize <= 0 ) {

    if ( quantizeddata ) {   
                                        // integer data - we have to take care of last bin, as kerneldensity divisions might miss it
        curvesize   = RoundAbove ( ( curvemax - curvemin ) / highkerneldensity ) + 1;
                                        // re-compute new max from actual curve size
        curvemax    = curvemin + ( curvesize - 1 ) * highkerneldensity;
        }

    else                                // floating points data
                                // # intermediate bins     incl subsampling      final single bin around max
        curvesize   = Truncate ( ( curvemax - curvemin ) / highkerneldensity ) + 1;
    }

                                        // something failed, set a minimum size just to avoid errors (?old code, might not be needed anymore?)
crtl::Maxed ( curvesize, HistoMinSize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rescaling factors to transform into curve space
curveratio      = ( curvesize - 1 ) / NonNull ( curvemax - curvemin );
                                        // if ignoring the Truncate part, formula simplifies to:
//curveratio    = kernelsubsampling / kerneldensity;


//DBGV6 ( kerneldensity, marginfactor, kernelsubsampling, curvesize, curveratio, kernelsubsampling / kerneldensity, "kerneldensity, marginfactor, kernelsubsampling, curvesize, curveratio" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        THistogram::THistogram      (   TEasyStats&         stats, 
                                        double              kerneldensity,  double      mindata,            double      maxdata,
                                        double              marginfactor,   double      kernelsubsampling,
                                        HistogramOptions    options,
                                        double*             usercurvesize
                                    )
{
ComputeHistogram    (   stats,
                        kerneldensity,  mindata,            maxdata,
                        marginfactor,   kernelsubsampling,
                        options,
                        usercurvesize
                    );
}


//----------------------------------------------------------------------------
                                        // Main workhorse to compute an histogram from stats - All other functions will call this one at one point or another
                                        // Kernel density: 0 to compute an optimal Kernel; > 0 if provided and trusted by caller; < 0 also provided but will skip the final Gaussian convolution
                                        // mindata & maxdata: 0 to compute from the data; != 0 if provided by caller
                                        // !stats could be modified if  HistogramAbsolute or HistogramIgnoreNulls  options are being used!
void    THistogram::ComputeHistogram (  TEasyStats&         stats, 
                                        double              kerneldensity,  double      mindata,            double      maxdata,    // kerneldensity, mindata and maxdata can be 0 for automatic scaling
                                        double              marginfactor,   double      kernelsubsampling,  
                                        HistogramOptions    options,
                                        double*             usercurvesize
                                     )
{
Reset ();


if ( ! ( stats.IsAllocated () && stats.IsNotEmpty () ) ) {
                                        // if size was given, at least return the resized curve (also, resetted)
    Set ( usercurvesize ? (int) *usercurvesize : 0 );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ignore nulls early on
                                        // !Will modify the given stats!
if ( IsFlag ( options, HistogramIgnoreNulls ) )

    stats.RemoveNulls ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Collapse negative and positive data together
                                        // !Will modify the given stats!
if ( IsFlag ( options, HistogramAbsolute ) )

    stats.ToAbsolute ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // either we have the density set by the caller, or we compute gaussian Kernel size ourselves, which gives the "resolution" of the data
if ( kerneldensity <= 0 )

    kerneldensity   = stats.GaussianKernelDensity ( IsFlag ( options, HistogramDiscrete ) ? KernelDensityDiscrete : KernelDensityDefault );

                                        // both values set to 0 -> use the range of data, otherwise trust the caller
bool                givenlimits     = ! ( mindata == 0 && maxdata == 0 );

if ( ! givenlimits ) {

    mindata     = stats.Min ();
    maxdata     = stats.Max ();
    }

                                        // force the range to start / finish on 0(?)
//if ( IsFlag ( options, HistogramBoundToZero ) )
//    if      ( mindata > 0 && maxdata > 0 )  mindata = 0;
//    else if ( mindata < 0 && maxdata < 0 )  maxdata = 0;


//                                        // or simply checking boundaries?
//Mined ( mindata, stats.Min () );
//Maxed ( maxdata, stats.Max () );


crtl::Maxed ( marginfactor,         (double) 0 );
crtl::Maxed ( kernelsubsampling,    (double) 1 );

                                        // resetting CDF and Log?
//if ( IsFlag ( options, HistogramCDF ) && IsFlag ( options, HistogramLog ) )
//    SetFlags  ( options, HistogramLog, HistogramLinear );

                                        // CDF will always normalize to Max by itself
if ( IsFlag ( options, HistogramCDF ) )
    ResetFlags  ( options, HistogramNormMask );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              curvesize       = usercurvesize ? *usercurvesize : 0;
double              curvemin;
double              curveratio;


ComputeHistogramSize (  kerneldensity,  mindata,            maxdata, 
                        marginfactor,   kernelsubsampling,
                        curvesize,      curvemin,           curveratio 
                     );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate curve + set rescaling factors to transform into curve space
Set ( (int) curvesize );

if ( usercurvesize )    
    *usercurvesize  = curvesize;        // updating user parameter - in some rare instance ComputeHistogramSize is allowed to adjust the user given value

                                        // storing the transform to go from the real data to a bucket and vice-versa
Index1.IndexMin     = curvemin;
Index1.IndexRatio   = curveratio;

                                        // one more check
if ( curvesize == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // populating histogram

                                        // cumulate all data in histogram
for ( int i = 0; i < (int) stats; i++ )
                                        // Will handle the proper conversion real to bin
    GetValue ( stats[ i ] )++;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional smoothing
if ( IsFlag ( options, HistogramSmooth ) && kerneldensity > 0 ) {

    FctParams           p;
                                        // KS Bins already correspond to 1 KD
                                        // We just have to use the KS as the smoothing factor, with an optimal scale between 3 and 4
    p ( FilterParamDiameter )     = 4 * kernelsubsampling + 1;

                                        // then Gaussian it with the Kernel
    Filter ( FilterTypeGaussian, p, false );
    }

//else if ( ( options & HistogramRaw ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional Log
if ( IsFlag ( options, HistogramLog ) )

    for ( int b = 0; b < Dim1; b++ )

        Array[ b ]  = Log ( Array[ b ] + 0.1 ) - Log ( 0.1 );
    
//else if ( ( options & HistogramLinear ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // CDF
if ( IsFlag ( options, HistogramCDF ) )

    ToCDF ( HistogramNormMax );

//else if ( IsFlag ( options, HistogramPDF ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional normalization
if      ( IsFlag ( options, HistogramNormMax  ) )   NormalizeMax  ();   // setting the max to 1 is easier for visual exploration
else if ( IsFlag ( options, HistogramNormArea ) )   NormalizeArea ();   // the real mathematical formula, total area / probability == 1
//elseif( IsFlag ( options, HistogramCount    ) )   ;                   // nothing to do, we already have a count


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TFileName           _file ( "E:\\Data\\Histogram.sef" );
//CheckNoOverwrite ( _file );
//WriteFile   ( _file, "Histo" );
}


//----------------------------------------------------------------------------
                                        // Zero value bin can be anywhere
void    THistogram::ClearZero ()
{
                                        // index of 0 bucket (can be anywhere)
int                 index0          = ToBin ( RealUnit, 0 );
                                        // resetting only if a legal null bin, i.e. within histogram range
if ( WithinBoundary ( index0 ) )

    Array[ index0 ]     = 0;
}


//----------------------------------------------------------------------------
                                        // !This is often done on a copy object, as we are going to overwrite it!
                                        // Nulls should be taken care of at histogram creation
void    THistogram::ToCDF   (   HistogramOptions    options     )
{

for ( int i = 1; i < Dim1; i++ )

    Array[ i ]     += Array[ i - 1 ];


if      ( IsFlag ( options, HistogramNormMax ) )
                                        // to range [0..1]
    NormalizeMax ();

//else if ( IsFlag ( options, HistogramCount ) ) ;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Histogram from volume
        THistogram::THistogram          (   const Volume&       data,
                                            const Volume*       mask,
                                            int                 numdownsamples,
                                            double              kerneldensity,
                                            double              marginfactor,   double      kernelsubsampling,
                                            HistogramOptions    options
                                        )
{
ComputeHistogram    (   data,
                        mask,
                        numdownsamples,
                        kerneldensity,
                        marginfactor,   kernelsubsampling,
                        options
                    );
}


//----------------------------------------------------------------------------
                                        // Histogram from full volume, with optional mask
                                        // !Code is very similar to TEasyStats::Set, maybe at one point merge the two!
void    THistogram::ComputeHistogram    (   const Volume&       data,
                                            const Volume*       mask,
                                            int                 numdownsamples,
                                            double              kerneldensity,
                                            double              marginfactor,   double      kernelsubsampling,  
                                            HistogramOptions    options
                                        )
{
Reset ();


bool                ignorenulls     = IsFlag ( options, HistogramIgnoreNulls    );
bool                ignorepositive  = IsFlag ( options, HistogramIgnorePositive );
bool                ignorenegative  = IsFlag ( options, HistogramIgnoreNegative );
bool                toabsolute      = IsFlag ( options, HistogramAbsolute       );

                                        // We are making use of these options right here, so let's reset them so that ComputeHistogram will not make any second use of them
ResetFlags  ( options, HistogramIgnoreNulls     );
ResetFlags  ( options, HistogramIgnorePositive  );
ResetFlags  ( options, HistogramIgnoreNegative  );
ResetFlags  ( options, HistogramAbsolute        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case of mask, we can restrict to the exact number of set voxels
int                 numitems        = mask ? mask->GetNumSet () : data.GetLinearDim ();

                                        // this will handle all cases, even if numdownsamples is <= 0
TDownsampling       down    ( numitems, numdownsamples );

                                        // optional downsampling?
bool                isdown          = down.Step > 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this oviously duplicates a lot of data, but once in a TEasyStats, we can call the other function straight away
TEasyStats          stats ( down.NumDownData );
double              v;

                                        // fill stats with selected data
for ( int i = 0, j = 0; i < (int) data; i++ )
                                        // No existing mask, or mask OK?
    if ( ! mask || (*mask) ( i ) ) {

        v   = data[ i ];

                                        // don't feed unwanted data
        if ( ignorenulls    && v == 0.0  
          || ignorepositive && v  > 0.0  
          || ignorenegative && v  < 0.0  )  continue;


        if ( toabsolute )
            v   = fabs ( v );

                                        // No downsampling, or downsampled item? test must be on a separate line, downsampling count is after masking
        if ( ! isdown || ! ( j++ % down.Step ) )

            stats.Add ( v, ThreadSafetyIgnore );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              mindata             = stats.Min ();
double              maxdata             = stats.Max ();
                                        // For volume histograms, force the range to start / finish on 0 - make it a flag to be be used elsewhere
if      ( mindata > 0 && maxdata > 0 )  mindata = 0;
else if ( mindata < 0 && maxdata < 0 )  maxdata = 0;


//if ( kerneldensity <= 0 )
//                                        // in case we want to upscale results - see below
//    kerneldensity   = stats.GaussianKernelDensity ();


ComputeHistogram    (   stats, 
                        kerneldensity,  mindata,    maxdata,
                        marginfactor,   kernelsubsampling,  
//                      marginfactor,   kernelsubsampling * kerneldensity,  // we can force upscaling - results then look like the old version
                        options
                    );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*                                        // Histogram from resampled volume
void    THistogram::ComputeHistogram    (   const Volume&       data,
                                            double              percentage,     TVector<int>&   randindex,
                                            double              kerneldensity,
                                            double              marginfactor,   double          kernelsubsampling,  
                                            HistogramOptions    options
                                        )
{
bool                ignorenulls     = IsFlag ( options, HistogramIgnoreNulls    );
bool                ignorepositive  = IsFlag ( options, HistogramIgnorePositive );
bool                ignorenegative  = IsFlag ( options, HistogramIgnoreNegative );
bool                toabsolute      = IsFlag ( options, HistogramAbsolute       );

                                        // We are making use of these options right here, so let's reset them so that ComputeHistogram will not make any second use of them
ResetFlags  ( options, HistogramIgnoreNulls     );
ResetFlags  ( options, HistogramIgnorePositive  );
ResetFlags  ( options, HistogramIgnoreNegative  );
ResetFlags  ( options, HistogramAbsolute        );


int                 numbsamples         = data.GetLinearDim ();
int                 numsubsamples       = Clip ( percentage, 0.0, 1.0 ) * numbsamples;
                                        // get a random series of indexes
//TVector<int>        randindex;
randindex.RandomSeries ( numsubsamples, numbsamples );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEasyStats          stats ( numsubsamples );
double              v;

                                        // fill stats with selected data
for ( int i = 0; i < numsubsamples; i++ ) {

    v   = data ( randindex[ i ] );

                                        // don't feed unwanted data
    if ( ignorenulls    && v == 0.0  
      || ignorepositive && v  > 0.0  
      || ignorenegative && v  < 0.0  )  continue;


    if ( toabsolute )
        v   = fabs ( v );


    stats.Add ( v, ThreadSafetyIgnore );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputeHistogram    (   stats, 
                        kerneldensity,  0,  0,
                        marginfactor,   kernelsubsampling,  
                        options
                    );
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        THistogram::THistogram          (   const Volume&       data,
                                            const Volume*       mask,
                                            int                 li1,            int             ls1, 
                                            int                 li2,            int             ls2, 
                                            int                 li3,            int             ls3, 
                                            double              kerneldensity,
                                            double              marginfactor,   double      kernelsubsampling,  
                                            HistogramOptions    options
                                        )
{
ComputeHistogram    (   data,
                        mask,
                        li1,    ls1, 
                        li2,    ls2, 
                        li3,    ls3, 
                        kerneldensity,
                        marginfactor,   kernelsubsampling,
                        options
                    );
}


//----------------------------------------------------------------------------
                                        // Histogram from clipped volume
void    THistogram::ComputeHistogram    (   const Volume&       data,
                                            const Volume*       mask,
                                            int                 li1,            int             ls1, 
                                            int                 li2,            int             ls2, 
                                            int                 li3,            int             ls3, 
                                            double              kerneldensity,
                                            double              marginfactor,   double          kernelsubsampling,  
                                            HistogramOptions    options
                                        )
{
Reset ();


bool                ignorenulls     = IsFlag ( options, HistogramIgnoreNulls    );
bool                ignorepositive  = IsFlag ( options, HistogramIgnorePositive );
bool                ignorenegative  = IsFlag ( options, HistogramIgnoreNegative );
bool                toabsolute      = IsFlag ( options, HistogramAbsolute       );

                                        // We are making use of these options right here, so let's reset them so that ComputeHistogram will not make any second use of them
ResetFlags  ( options, HistogramIgnoreNulls     );
ResetFlags  ( options, HistogramIgnorePositive  );
ResetFlags  ( options, HistogramIgnoreNegative  );
ResetFlags  ( options, HistogramAbsolute        );


int                 dim1            = data.GetDim1 ();
int                 dim2            = data.GetDim2 ();
int                 dim3            = data.GetDim3 ();

crtl::Clipped ( li1, ls1, 0, dim1 - 1 );
crtl::Clipped ( li2, ls2, 0, dim2 - 1 );
crtl::Clipped ( li3, ls3, 0, dim3 - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reserving maximum amount, not accounting for mask
TEasyStats          stats ( ( ls1 - li1 + 1 ) * ( ls2 - li2 + 1 ) * ( ls3 - li3 + 1 ) );
double              v;

                                        // fill stats with selected data
for ( int i1 = li1; i1 <= ls1; i1++ )
for ( int i2 = li2; i2 <= ls2; i2++ )
for ( int i3 = li3; i3 <= ls3; i3++ )

    if ( ! mask || (*mask) ( i1, i2, i3 ) ) {

        v   = data ( i1, i2, i3 );

                                        // don't feed unwanted data
        if ( ignorenulls    && v == 0.0  
          || ignorepositive && v  > 0.0  
          || ignorenegative && v  < 0.0  )  continue;


        if ( toabsolute )
            v   = fabs ( v );


        stats.Add ( v, ThreadSafetyIgnore );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputeHistogram    (   stats, 
                        kerneldensity,  0,  0,
                        marginfactor,   kernelsubsampling,  
                        options
                    );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Simple Gaussian filter - done in-site
void    THistogram::Smooth ( int num )
{
for ( ; num > 0; num-- ) {

    double              h1              = Array[ 0 ];
    double              h2              = h1;

                                        // border: using existing data only
    Array[ 0 ]  = ( 2 * h1 + Array[ 1 ] ) / 3;

                                        // border-safe part
    for ( int b = 1; b < Dim1 - 1; b++ ) {

        h1          = Array[ b ];

        Array[ b ]  = ( 2 * h1 + h2 + Array[ b + 1 ] ) / 4;

        h2          = h1;
        }

                                        // border: using existing data only
    Array[ Dim1 - 1 ] = ( 2 * Array[ Dim1 - 1 ] + h2 ) / 3;
    }
}


//----------------------------------------------------------------------------
                                        // remove same #bins everywhere
void    THistogram::Erode ( int nbins )
{
for ( int b = 0; b < Dim1; b++ )

    Array[ b ]  = crtl::AtLeast ( 0.0, Array[ b ] - nbins );
}

                                        // remove % of max bins everywhere
void    THistogram::Erode ( double percentmax )
{
Erode ( (int) ( GetMaxValue () * percentmax ) );
}


//----------------------------------------------------------------------------
                                        // Compact the empty, or nearly empty bins
                                        // results will be totally set with safe values, even outside data range
void    THistogram::Pack ( TArray1<int> &topack, TArray1<int> &tounpack, double minreject )
{
                                        // allocate lut?
topack  .Resize ( Dim1 );
tounpack.Resize ( Dim1 );

                                        // set default values
topack              = 0;
tounpack            = 0;

                                        // copy original data
THistogram          h ( *this );

                                        // clear our histogram
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 iorigin         = 0;    // index of original data
int                 ipack           = -1;   // index of compacted data

                                        // find first non-null bin
for ( ; iorigin < Dim1; iorigin++ )
    if ( h[ iorigin ] > minreject )
        break;

                                        // no bins found?
if ( iorigin == Dim1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // filling topack before iorigin not needed, as we have set topack to 0, and this is the value of the first ipack
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // compact non-empty bins
for ( ; iorigin < Dim1; iorigin++ ) {

    if ( h[ iorigin ] > minreject ) {
        ipack++;                        // set current packed index
        Array    [ ipack ]  = h[ iorigin ];
        tounpack [ ipack ]  =    iorigin;
        }

    topack[ iorigin ]       =    ipack; // safe filling, even for non-existant data, all empty bins will project to the previous non-empty compact bin
    }

                                        // fill remaining part of tounpack to project to last original valid bin
for ( int i = ipack + 1; i < Dim1; i++ )
    tounpack[ i ]   = tounpack [ ipack ];

                                        // fill remaining part with linear extrapolation?
//for ( iorigin = tounpack[ ipack ] + 1, ipack++; iorigin < Dim1 && ipack < Dim1; iorigin++, ipack++ ) {
//    tounpack[ ipack   ] = iorigin;
//    topack  [ iorigin ] = ipack;
//    }


/*                                        // scan original
for ( iorigin = 0, ipack = 0; iorigin < Dim1; iorigin++ ) {

    topack   [ iorigin ]    =    ipack;     // safe filling, even for non-existant data: 0 bins and next non-0 bin will project to ipack

    if ( h[ iorigin ] > minreject ) {
        Array    [ ipack   ]    = h[ iorigin ];
        tounpack [ ipack   ]    =    iorigin;
        ipack++;
        }
    }

                                        // fill remaining lut part with linear extrapolation
for ( iorigin = tounpack[ ipack - 1 ] + 1; iorigin < Dim1 && ipack < Dim1; iorigin++, ipack++ ) {
    topack  [ iorigin ] = ipack;     // !!!!!!!!
    tounpack[ ipack   ] = iorigin;
    }
*/
                                        // Note that a linear extrapolation could be done for the values before 0
                                        // but we have to not pack the first 0's beforehand, then pack after the first real data
}


//----------------------------------------------------------------------------
                                        // Undo the Pack function, using one of its output
void    THistogram::Unpack ( TArray1<int> &tounpack )
{
                                        // not allocated
if ( tounpack.GetDim () != Dim1 /*|| tounpack[ 0 ] == Dim1 - 1*/ )
    return;

                                        // copy original data
THistogram          h ( *this );

                                        // clear our histogram
Reset ();

                                        // applies LUT back to original values
int                 iorigin;
int                 ipack;

                                        // unpack all packed bins
for ( ipack = 0, iorigin = tounpack[ ipack ]; ipack < Dim1 && iorigin >= 0; ipack++ ) {
    iorigin             = tounpack[ ipack ];
    Array[ iorigin ]   += h       [ ipack ];    // sum, for the end of histogram projects into the last bin
    }

                                        // now, some extra bins might have appeared, without the proper lut, just put them at the end
//for ( iorigin = tounpack[ ipack - 1 ] + 1; ipack < Dim1 && iorigin < Dim1; ipack++, iorigin++ )
//    Array[ iorigin ]    = h[ ipack ];

}


//----------------------------------------------------------------------------
                                        // Returns a LUT to equalize with, with a range in [0..1]
                                        // Caller should take care to ignore nulls, if they are unwanted
void    THistogram::EqualizeCDF ( TVector<double>& toequ )
{
                                        // allocate lut?
if ( toequ.GetDim () != Dim1 )
    toequ.Resize  ( Dim1 );


int                 from            = GetFirstPosition ( BinUnit );
int                 to              = GetLastPosition  ( BinUnit );

if ( ! ( from >= 0 && to >= 0 ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              minhisto        = 0;
double              maxhisto        = 1;
double              cdfmin          = Array[ from ];
double              cdfmax          = Array[   to ];
double              cdfdelta        = ( maxhisto - minhisto ) / NonNull ( cdfmax - cdfmin );
int                 b;

                                        // set safe values before any data
for ( b = 0; b <= from; b++ )
    toequ[ b ]  = minhisto;

                                        // equalize data
for ( ; b <= to; b++ )
    toequ[ b ]  = ( Array[ b ] - cdfmin ) * cdfdelta + minhisto;

                                        // set safe values after any data
for ( ; b < Dim1; b++ )
    toequ[ b ]  = maxhisto;
}


//----------------------------------------------------------------------------
double  THistogram::GetFirstPosition ( HistoUnit unit )     const
{
for ( int b = 0; b < Dim1; b++ )
    if ( Array[ b ] )
        return  ToReal ( unit, b );

return -1;
}


double  THistogram::GetLastPosition ( HistoUnit unit )      const
{
for ( int b = Dim1 - 1; b >= 0; b-- )
    if ( Array[ b ] )
        return  ToReal ( unit, b );

return -1;
}


double  THistogram::GetMaxPosition ( HistoUnit unit )       const
{
if ( IsNotAllocated () )
    return  -1;

return  ToReal ( unit, TVector<double>::GetMaxPosition () );
}


double  THistogram::GetMinPosition ( HistoUnit unit )       const
{
if ( IsNotAllocated () )
    return  -1;

return  ToReal ( unit, TVector<double>::GetMinPosition () );
}


double  THistogram::GetMiddlePosition ( HistoUnit unit )      const
{
return  ( GetLastPosition ( unit ) + GetFirstPosition ( unit ) ) / 2;
}

                                        // Normalized position, from first to last bin to [0..1]
double  THistogram::GetRelativePosition ( HistoUnit unit, double from )   const
{
return ( from - GetFirstPosition ( unit ) ) / NonNull ( GetLastPosition ( unit ) - GetFirstPosition ( unit ) );
}


double  THistogram::GetExtent ( HistoUnit unit )      const
{
return  GetLastPosition ( unit ) - GetFirstPosition ( unit ) 
      + ( unit == BinUnit || IsStepInteger () ? 1 : 0 );    // integer cases include bin at the border
}


double  THistogram::GetRelativeExtent ()    const
{
return  GetExtent ( BinUnit ) / NonNull ( Dim1 );
}


int     THistogram::GetNumNonEmptyBins ( HistoUnit unit )   const
{
int                 n               = 0;

for ( int b = 0; b < Dim1; b++ )
    if ( Array[ b ] )
        n++;

return  ToRealWidth ( unit, n );
}

                                        // Using the last bin to estimate the original data max value
double  THistogram::GetMaxStat ()   const   
{
return  ToReal ( RealUnit, Dim1 - 1 + 0.5 );
}


//----------------------------------------------------------------------------
/*                                      // version 2
int     THistogram::GetNextInflexionPosition ( int from, bool forward )
{

int                 inc             = forward ? 1 : -1;

                                        // go backward for deceleration, "top of mountain"
int                 pos             = from + inc;
double              d1;
double              d2;

                                        // first, search for acceleration
for ( ; pos > 2 && pos < Dim1 - 3; pos += inc ) {

//    d1  = Array[ pos ] - Array[ pos - inc ];
//    d2  = Array[ pos + inc ] - Array[ pos ];
    d1  = 2 * Array[ pos ] - Array[ pos - 2 * inc ] - Array[ pos - inc ];
    d2  = Array[ pos + 2 * inc ] + Array[ pos + inc ] - 2 * Array[ pos ];

    if ( ! ( d1 < 0 && d2 < 0 && d2 > d1 ) )
        break;
    }

                                        // second, stop when deceleration is found
for ( ; pos > 2 && pos < Dim1 - 3; pos += inc ) {

//    d1  = Array[ pos ] - Array[ pos - inc ];
//    d2  = Array[ pos + inc ] - Array[ pos ];
    d1  = 2 * Array[ pos ] - Array[ pos - 2 * inc ] - Array[ pos - inc ];
    d2  = Array[ pos + 2 * inc ] + Array[ pos + inc ] - 2 * Array[ pos ];

    if ( d1 < 0 && d2 < 0 && d2 > d1 )
        break;
    }


//DBGV ( pos, "new inflexion" );

return  pos;
}
*/
/*                                      // version 3
                                        // Inflexion is the middle of the slope where derivative starts decreasing
int     THistogram::GetNextInflexionPosition ( int from, bool forward )
{
int                 inc             = forward ? 1 : -1;
int                 pos             = from + inc;
double              d1;
double              d2;
double              mindelta        = 0;

                                        // first, search for acceleration
for ( ; pos >= 4 * inc && pos < Dim1 - 4 * inc; pos += inc ) {
                                        // somehow account for difference of scaling of histograms, and solve out small/identical differences

                                        // look at smallest negative pre-step
    d1  = max ( (double) ( Array[ pos ] - Array[ pos -     inc ] ),
                         ( Array[ pos ] - Array[ pos - 2 * inc ] ) / 2.0,
                         ( Array[ pos ] - Array[ pos - 4 * inc ] ) / 4.0 );
                                        // look at biggest negative post-step
    d2  = min ( (double) ( Array[ pos +     inc ] - Array[ pos ] ),
                         ( Array[ pos + 2 * inc ] - Array[ pos ] ) / 2.0,
                         ( Array[ pos + 4 * inc ] - Array[ pos ] ) / 4.0 );

                                        // reached enough acceleration to get out?
    if ( d1 < mindelta && d2 < mindelta && d2 < d1 - 1 )
        break;
    }

                                        // move a bit
pos    += inc;

                                        // second, stop when deceleration is found
for ( ; pos >= 4 * inc && pos < Dim1 - 4 * inc; pos += inc ) {
                                        // look at smallest negative pre-step
    d1  = max ( (double) ( Array[ pos ] - Array[ pos -     inc ] ),
                         ( Array[ pos ] - Array[ pos - 2 * inc ] ) / 2.0,
                         ( Array[ pos ] - Array[ pos - 4 * inc ] ) / 4.0 );
                                        // look at biggest negative post-step
    d2  = min ( (double) ( Array[ pos +     inc ] - Array[ pos ] ),
                         ( Array[ pos + 2 * inc ] - Array[ pos ] ) / 2.0,
                         ( Array[ pos + 4 * inc ] - Array[ pos ] ) / 4.0 );

                                        // enough deceleration? (remember d1 and d2 are negative)
    if ( d1 < mindelta && d2 < mindelta && d2 > d1 * 0.95 )
        break;
    }


//DBGV ( pos, "new inflexion" );

return  pos;
}
*/
                                        // version 4
                                        // Inflexion is the middle of the slope where derivative starts decreasing
                                        // Internal use only, works on bin indexes
int     THistogram::GetNextInflexionPosition ( int from, bool forward )     const
{
                                        // we need some extent to do a proper search!
if ( Dim1 < 7 )
    return  from;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 inc             = forward ? 1 : -1;
int                 pos             = from + inc;
int                 posmin          = forward ? 1        : 4;
int                 posmax          = forward ? Dim1 - 5 : Dim1 - 2;
double              d1;
double              d2;
double              d12;
double              mindelta        = 0;
int                 step2;
int                 step4;

                                        // first, search for acceleration
for ( ; pos >= posmin && pos <= posmax; pos += inc ) {
                                        // somehow account for difference of scaling of histograms, and solve out small/identical differences

                                        // look at smallest negative pre-step
                                        // don't go "before" from, which is also a decreasing slope!
    step2   = min ( 2, abs ( pos - from ) );
    step4   = min ( 4, abs ( pos - from ) );
    d1      = max ( (double) ( Array[ pos           ] - Array[ pos -         inc ] ),
                             ( Array[ pos           ] - Array[ pos - step2 * inc ] ) / (double) step2,
                             ( Array[ pos           ] - Array[ pos - step4 * inc ] ) / (double) step4 );

                                        // look at biggest negative post-step
    d2      = min ( (double) ( Array[ pos +     inc ] - Array[ pos               ] ),
                             ( Array[ pos + 2 * inc ] - Array[ pos               ] ) / 2.0,
                             ( Array[ pos + 4 * inc ] - Array[ pos               ] ) / 4.0 );

//  DBGV3 ( pos, d1, d2, "Inflexion Acc: position d1 d2" );

                                        // reached enough acceleration to get out?
    if ( d1 < mindelta 
      && d2 < mindelta 
      && d2 < d1 - 1 
        )
        break;
    }

                                        // move a bit
pos    += inc;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // second, stop when deceleration is found
for ( ; pos >= posmin && pos <= posmax; pos += inc ) {

                                        // look at smallest negative pre-step
                                        // don't go "before" from, which is also a decreasing slope!
    step2   = min ( 2, abs ( pos - from ) );
    step4   = min ( 4, abs ( pos - from ) );
    d1      = max ( (double) ( Array[ pos           ] - Array[ pos -         inc ] ),
                             ( Array[ pos           ] - Array[ pos - step2 * inc ] ) / (double) step2,
                             ( Array[ pos           ] - Array[ pos - step4 * inc ] ) / (double) step4 );

                                        // look at biggest negative post-step
    d2      = min ( (double) ( Array[ pos +     inc ] - Array[ pos               ] ),
                             ( Array[ pos + 2 * inc ] - Array[ pos               ] ) / 2.0,
                             ( Array[ pos + 4 * inc ] - Array[ pos               ] ) / 4.0 );

                                        // max curvature
    d12     = max ( (double) ( - 2 * Array[ pos ] + Array[ pos +     inc ] + Array[ pos -         inc ] ),
                             ( - 2 * Array[ pos ] + Array[ pos + 2 * inc ] + Array[ pos - step2 * inc ] ) / 2.0,
                             ( - 2 * Array[ pos ] + Array[ pos + 4 * inc ] + Array[ pos - step4 * inc ] ) / 4.0 ) / 2;

                                        // normalized deceleration
    d12    /= NonNull ( ( 2 * Array[ pos ] + Array[ pos + inc ] + Array[ pos - inc ] ) / 4 );


//  DBGV7 ( pos, d1, d2, d12, Array[ pos - inc ], Array[ pos ], Array[ pos + inc ], "Inflexion Dec: position d1 d2 d12  v-1v0v+1" );

                                        // enough deceleration?
    if ( d1 < mindelta 
      && d2 < mindelta 
//    && d12 > 0                        // when not normalized
      && d12 > 0.001                    // up to 0.004?
      || d2 == 0 || d12 == 0            // also test for degenerate cases
       )    
        break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//DBGV ( pos, "new inflexion" );

return  pos;
}


//----------------------------------------------------------------------------
                                        // Internal use only, works on bin indexes
int     THistogram::GetNextLCornerPosition ( int from )     const
{
double              d1;
double              d2;
double              d12;
//double              d12old          = -DBL_MAX;
int                 inc             = 1; // forward ? 1 : -1;
int                 pos             = from + inc;
int                 step2;
int                 step4;
double              maxd            = 0;
int                 posmin          = 1;            // forward ? 1        : 4;
int                 posmax          = Dim1 - 5;     // forward ? Dim1 - 5 : Dim1 - 2;


                                        // second, stop when deceleration is found
for ( ; pos >= posmin && pos <= posmax; pos += inc ) {

                                        // look at smallest negative pre-step
                                        // don't go "before" from, which is also a decreasing slope!
    step2   = min ( 2, abs ( pos - from ) );
    step4   = min ( 4, abs ( pos - from ) );
//    d1      = max ( (double) ( Array[ pos ] - Array[ pos -         inc ] ),
//                             ( Array[ pos ] - Array[ pos - step2 * inc ] ) / (double) step2,
//                             ( Array[ pos ] - Array[ pos - step4 * inc ] ) / (double) step4 );
    d1      = ( (double) ( Array[ pos ] - Array[ pos -         inc ] )
                       + ( Array[ pos ] - Array[ pos - step2 * inc ] ) / (double) step2
                       + ( Array[ pos ] - Array[ pos - step4 * inc ] ) / (double) step4 ) / 3.0;

                                        // look at biggest negative post-step
////    d2      = min ( (double) ( Array[ pos +     inc ] - Array[ pos ] ),
//    d2      = max ( (double) ( Array[ pos +     inc ] - Array[ pos ] ),
//                             ( Array[ pos + 2 * inc ] - Array[ pos ] ) / 2.0,
//                             ( Array[ pos + 4 * inc ] - Array[ pos ] ) / 4.0 );
    d2      = ( (double) ( Array[ pos +     inc ] - Array[ pos ] )
                       + ( Array[ pos + 2 * inc ] - Array[ pos ] ) / 2.0
                       + ( Array[ pos + 4 * inc ] - Array[ pos ] ) / 4.0 ) / 3;

                                        // max curvature
//    d12     = max ( (double) ( - 2 * Array[ pos ] + Array[ pos +     inc ] + Array[ pos -         inc ] ),
//                             ( - 2 * Array[ pos ] + Array[ pos + 2 * inc ] + Array[ pos - step2 * inc ] ) / 2.0,
//                             ( - 2 * Array[ pos ] + Array[ pos + 4 * inc ] + Array[ pos - step4 * inc ] ) / 4.0 ) / 2;
    d12     = ( (double) ( - 2 * Array[ pos ] + Array[ pos +     inc ] + Array[ pos -         inc ] )
                       + ( - 2 * Array[ pos ] + Array[ pos + 2 * inc ] + Array[ pos - step2 * inc ] ) / 2.0
                       + ( - 2 * Array[ pos ] + Array[ pos + 4 * inc ] + Array[ pos - step4 * inc ] ) / 4.0 ) / 2 / 3;

                                        // normalize
    d12    /= ( 2 * Array[ pos ] + Array[ pos + inc ] + Array[ pos - inc ] + 1e-6 ) / 4;

    maxd    = max ( maxd, d12 );

//    DBGV5 ( pos, d1, d2, d12old * 1000, d12 * 1000, "@pos  d1 d2  d12old d12" );
//    DBGV4 ( pos, maxd * 1000, d12 * 1000, RelativeDifference ( d12, maxd ), "@pos  maxd d12 Reldiff" );


//    if ( d1 >= 0 || d2 >= 0 || d12 > 0.005 && d12 < d12old * 0.90 ) // works ~ for max d2

                                        // for averages of d1, d2, d12
    if ( d1 >= 0 || d2 >= 0
      ||         d12 > 0.005
         && (    d12 < maxd * 0.75
              || RelativeDifference ( d12, maxd ) > 0.20 ) )
        break;

//    d12old  = d12;
    }


//DBGV2 ( from, pos, "from -> LCorner" );

return  pos;
}


//----------------------------------------------------------------------------
double  THistogram::GetRelativeValueMax  ( int b )  const
{
return  (double) Array[ Clip ( b, 0, Dim1 - 1 ) ] / GetMaxValue ();
}


double  THistogram::GetRelativeValueArea  ( int b ) const
{
return  (double) Array[ Clip ( b, 0, Dim1 - 1 ) ] / NonNull ( GetTotalArea ( BinUnit ) );
}


double  THistogram::GetTotalArea ( HistoUnit unit, double from, double to )     const
{
if ( unit == RealUnit ) {
    from    = ToIndex ( from );
    to      = ToIndex ( to   );
    }
else {
    // let from be 0
    if ( to == 0 )  to  = Dim1 - 1;
    }

crtl::Clipped ( from, to, (double) 0, (double) Dim1 - 1 );


double              sum             = 0;

for ( int b = from; b <= to; b++ )
    sum    += Array[ b ];

return  sum;
}


//----------------------------------------------------------------------------
                                        // Wrapper for TArray3
double  THistogram::ComputeBackground ( const Volume& data )
{
bool                somepos;
bool                someneg;

data.HasPositiveNegative ( somepos, someneg );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Whole volume estimate
double              wholebackgroundpos  = 0;
double              wholebackgroundneg  = 0;

                                        // separate estimate for the positive part only
if ( somepos ) {

    ComputeHistogram    (   data,
                            0,
                            BackgroundMaxSamples,
                            0,
                            0,      3,
                                            // !NO SMOOTHING at creation time, so that Ranking detection is not compromised!
                                            // Also having a smaller Kernel Density does not impair the ranks, histogram is always flat anyway
                                            // !Current method accounts / needs for the null values!
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramCount | HistogramLinear | HistogramIgnoreNegative /*| HistogramIgnoreNulls*/ )
                        );

    wholebackgroundpos  = ComputeBackground ( RealUnit );
    }

                                        // separate estimate for the negative part only
if ( someneg ) {

    ComputeHistogram    (   data,
                            0,
                            BackgroundMaxSamples,
                            0,
                            0,      3,
                                            // !NO SMOOTHING at creation time, so that Ranking detection is not compromised!
                                            // Also having a smaller Kernel Density does not impair the ranks, histogram is always flat anyway
                                            // !Current method accounts / needs for the null values!
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramCount | HistogramLinear | HistogramAbsolute | HistogramIgnorePositive /*| HistogramIgnoreNulls*/ )
                        );
                                        // !positive value!
    wholebackgroundneg  = ComputeBackground ( RealUnit );
    }

                                        // merge the 2 estimates
double              wholebackground     = somepos && someneg ?  ( wholebackgroundpos + wholebackgroundneg ) / 2 // max ( wholebackgroundpos, wholebackgroundneg )
                                        : somepos            ?  wholebackgroundpos
                                        : someneg            ?  wholebackgroundneg
                                        :                       0;

//DBGV3 ( wholebackgroundneg, wholebackgroundpos, wholebackground, "Background: neg pos final" );

return  wholebackground;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Random resampling - It technically works, but it gives each time the same background value!
double              percentageresamp    = Clip ( 10000.0 / data.GetLinearDim (), 0.10, 0.25 );
TVector<int>        randindex;
int                 numresampling       = 3;
THistogram          Hresamp;
TEasyStats          statres ( numresampling );

                                        // resampling & using for different stats at the same time
for ( int i = 0; i < numresampling; i++ ) {

    Hresamp.ComputeHistogram    (   data,  
                                    percentageresamp,   randindex,
                                    0,
//                                  0,      1,
//                                  (HistogramOptions) ( HistogramPDF | HistogramRaw  | HistogramCount | HistogramLinear | HistogramAbsolute )
                                    0,      3,
                                    (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute )
                                );

    statres.Add ( Hresamp.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );


//    DBGV2 ( i, statres[ i ], "Background resampled" );
//    TFileName           _file;
//    sprintf ( _file, "E:\\Data\\Volume.Resampled.Histogram.%02d.sef", i + 1 );
//    Hresamp.WriteFile ( _file );
    }

//statres.Show ( "Stats Resampled Background" );

double              resampbackground    = statres.Median ();

//DBGV ( resampbackground, "Background resampled" );

return  resampbackground;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Used to be the default - It seems now that the whole volume background is as reliable as the octant one
                                        // Splitting volume into 8 octants, computing histogram for each of them
int                 Dim1            = data.GetDim1 ();
int                 Dim2            = data.GetDim2 ();
int                 Dim3            = data.GetDim3 ();

THistogram          H1  ( data, 0, 0,        Dim1 / 2, 0,        Dim2 / 2, 0,        Dim3 / 2, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H2  ( data, 0, 0,        Dim1 / 2, 0,        Dim2 / 2, Dim3 / 2, Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H3  ( data, 0, 0,        Dim1 / 2, Dim2 / 2, Dim2 - 1, 0,        Dim3 / 2, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H4  ( data, 0, 0,        Dim1 / 2, Dim2 / 2, Dim2 - 1, Dim3 / 2, Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H5  ( data, 0, Dim1 / 2, Dim1 - 1, 0,        Dim2 / 2, 0,        Dim3 / 2, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H6  ( data, 0, Dim1 / 2, Dim1 - 1, 0,        Dim2 / 2, Dim3 / 2, Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H7  ( data, 0, Dim1 / 2, Dim1 - 1, Dim2 / 2, Dim2 - 1, 0,        Dim3 / 2, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
THistogram          H8  ( data, 0, Dim1 / 2, Dim1 - 1, Dim2 / 2, Dim2 - 1, Dim3 / 2, Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );


                                        // 6 halves, in each 3 directions
//THistogram          H1  ( *this, 0, 0,        Dim1 - 1, 0,        Dim2 - 1, 0,        Dim3 / 2, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
//THistogram          H2  ( *this, 0, 0,        Dim1 - 1, 0,        Dim2 - 1, Dim3 / 2, Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
//THistogram          H3  ( *this, 0, 0,        Dim1 - 1, 0,        Dim2 / 2, 0,        Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
//THistogram          H4  ( *this, 0, 0,        Dim1 - 1, Dim2 / 2, Dim2 - 1, 0,        Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
//THistogram          H5  ( *this, 0, 0,        Dim1 / 2, 0,        Dim2 - 1, 0,        Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
//THistogram          H6  ( *this, 0, Dim1 / 2, Dim1 - 1, 0,        Dim2 - 1, 0,        Dim3 - 1, 0, 0, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramCount | HistogramLinear | HistogramAbsolute ) );
                               
                               

TEasyStats          stat ( 8 );

stat.Add ( H1.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H2.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H3.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H4.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H5.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H6.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H7.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );
stat.Add ( H8.ComputeBackground ( RealUnit ), ThreadSafetyIgnore );

                                        // for a more robust estimation, use the average of all histograms
double              octantbackground    = stat.Median ( false );

//stat.Show ( "Stats Background" );
//DBGV ( octantbackground, "Background: median octants" );

return  octantbackground;
*/
}


//----------------------------------------------------------------------------
double  THistogram::ComputeBackground ( HistoUnit unit )    const
{
                                        // work with a copy, as we are going to heavily massage this histogram
THistogram          h ( *this );

double              maxstat         = h.GetMaxStat ();


//TFileName           _file;
//StringCopy      ( _file, "E:\\Data\\ComputeBackground." );
//GetTempFileName ( StringEnd ( _file ) );
//RemoveExtension ( _file );
//AddExtension    ( _file, FILEEXT_EEGSEF );
//h.WriteFile     ( _file );
//_file.Open      ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case 1: binarized / discretized mask
int                 numneb          = GetNumNonEmptyBins ( RealUnit ) - 1;  // estimate of non-null, non-background different bins
bool                fewintbins      = numneb < 50 && IsStepInteger ();      // integer ratio means integer data


//DBGV5 ( numneb, Dim1, Step (), KernelSubsampling, fewintbins, "numneb, Dim1, Step, KernelSubsampling, fewintbins" );
                                        // few discrete bins? this can miss ROIs masks, like the AAL with 116 values
if ( fewintbins )

    return  MaskBackgroundValue;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                        // TODO: ROIs case? actually would be easier from the volume itself than from the histogram
//TEasyStats          statsrd;
//for ( int i = 1; i < Dim1; i++ )
//    if ( h[ i ] ) statsrd.Add ( RelativeDifference ( h[ i ], h[ i - 1 ] ), ThreadSafetyIgnore );
//statsrd.Show ( "RelDiff" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case 2: Ranked data -> histogram is all flat, except for first bin
                                        // !For a proper detection, histogram should NOT be filtered!

                                        // Last bucket is nearly 1 - high chance this is equalized / ranked
                                        // ?could also test for value 100, in case data has been rescaled?
bool                maxis1          = RelativeDifference ( maxstat, 1 ) < 0.001;


if ( Dim1 > 7                           // there needs to be a few buckets for proper testing
  && maxis1   ) {
                                        // ignoring null bucket
    double              area            = NonNull ( h.GetTotalArea ( BinUnit, 1 ) );

                                        // Every first buckets have a near equal share of data, which happens only after ranking
    bool                binequ1         = RelativeDifference ( h[ 1 ] / area, 1.0 / ( Dim1 - 1 ) ) < 0.10;
    bool                binequ2         = RelativeDifference ( h[ 2 ] / area, 1.0 / ( Dim1 - 1 ) ) < 0.10;
    bool                binequ3         = RelativeDifference ( h[ 3 ] / area, 1.0 / ( Dim1 - 1 ) ) < 0.10;
    bool                binequ4         = RelativeDifference ( h[ 4 ] / area, 1.0 / ( Dim1 - 1 ) ) < 0.10;
    bool                binequ5         = RelativeDifference ( h[ 5 ] / area, 1.0 / ( Dim1 - 1 ) ) < 0.10;
                                            // each bucket is equal to proportional probability
    bool                isranked        = binequ1 && binequ2 && binequ3 && binequ4 && binequ5;

                                        // we can also have a special ranking, where histogram is a ramp
                                        // so we can look at the relative laplacian in a few successive buckets
//  bool                lapl2           = fabs ( 2 * h[ 2 ] - h[ 1 ] - h[ 3 ] ) / NonNull ( h[ 2 ] ) < 0.1; // 0.01;
//  bool                lapl3           = fabs ( 2 * h[ 3 ] - h[ 2 ] - h[ 4 ] ) / NonNull ( h[ 3 ] ) < 0.1; // 0.01;
//  bool                lapl4           = fabs ( 2 * h[ 4 ] - h[ 3 ] - h[ 5 ] ) / NonNull ( h[ 4 ] ) < 0.1; // 0.01;
//  bool                isramp          = lapl2 && lapl3 && lapl4;

                                        // Ramp Ranking tested by line fitting
                                        // Simpler formula
//  double              maxh            = h[ Dim1 - 3 ] + h[ Dim1 - 2 ] + h[ Dim1 - 1 ];
//  double              h25             = h[ Round ( Dim1 * 0.25 ) - 1 ] + h[ Round ( Dim1 * 0.25 ) ] + h[ Round ( Dim1 * 0.25 ) + 1 ];
//  double              h50             = h[ Round ( Dim1 * 0.50 ) - 1 ] + h[ Round ( Dim1 * 0.50 ) ] + h[ Round ( Dim1 * 0.50 ) + 1 ];
//  double              h75             = h[ Round ( Dim1 * 0.75 ) - 1 ] + h[ Round ( Dim1 * 0.75 ) ] + h[ Round ( Dim1 * 0.75 ) + 1 ];
//  double              delta1          = RelativeDifference ( h25 / maxh, 0.25 );
//  double              delta2          = RelativeDifference ( h50 / maxh, 0.50 );
//  double              delta3          = RelativeDifference ( h75 / maxh, 0.75 );
                                        // Exact formula
    int                 minx            = 2;            // skipping bin 0, then centered on first cluster of 3
    int                 maxx            = Dim1 - 2;     // centered on last cluster of 3
    int                 deltax          = NonNull ( maxx - minx );
    int                 x25             = Round ( deltax * 0.25 + minx );
    int                 x50             = Round ( deltax * 0.50 + minx );
    int                 x75             = Round ( deltax * 0.75 + minx );
    double              minh            = h[ minx - 1 ] + h[ minx ] + h[ minx + 1 ];
    double              maxh            = h[ maxx - 1 ] + h[ maxx ] + h[ maxx + 1 ];
    double              deltah          = NonNull ( maxh - minh );
    double              h25             = h[ x25  - 1 ] + h[ x25  ] + h[ x25  + 1 ];
    double              h50             = h[ x50  - 1 ] + h[ x50  ] + h[ x50  + 1 ];
    double              h75             = h[ x75  - 1 ] + h[ x75  ] + h[ x75  + 1 ];
    double              delta1          = RelativeDifference ( ( h25 - minh ) / deltah, ( x25 - minx ) / (double) deltax );
    double              delta2          = RelativeDifference ( ( h50 - minh ) / deltah, ( x50 - minx ) / (double) deltax );
    double              delta3          = RelativeDifference ( ( h75 - minh ) / deltah, ( x75 - minx ) / (double) deltax );

//    DBGV3 ( delta1, delta2, delta3, "delta1, delta2, delta3" );
    
    bool                isramp          = delta1 < 0.12 && delta2 < 0.10 && delta3 < 0.10;

//                                      // somehow equivalent, but testing relative differences between successive buckets
//  bool                bucket12        = RelativeDifference ( h[ 1 ], h[ 2 ] ) < 1e-2;
//  bool                bucket23        = RelativeDifference ( h[ 2 ], h[ 3 ] ) < 1e-2;
//  bool                bucket34        = RelativeDifference ( h[ 3 ], h[ 4 ] ) < 1e-2;
//  bool                bucket45        = RelativeDifference ( h[ 4 ], h[ 5 ] ) < 1e-2;
//
//  bool                isranked        = maxis1
//                                     && h[ 1 ]                                        // shouldn't be null
//                                     && bucket12 && bucket23 && bucket34 && bucket45;

                                        // Equalization doesn't make nice and nearly equal buckets, but we can take a shoot at 5 buckets together
    double              avgfirstbuckets = ( ( h[ 1 ] + h[ 2 ] + h[ 3 ] + h[ 4 ] + h[ 5 ] ) / 5 ) / area;
                                        // using a quite loose threshold here
    bool                isequalized     = RelativeDifference ( avgfirstbuckets, 1.0 / ( Dim1 - 1 ) ) < 0.10;


//    DBGV4 ( maxis1, isranked, isramp, isequalized, "maxis1, isranked, isramp, isequalized" );


    if ( isranked || isramp || isequalized )

        return  RankBackgroundValue;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // After ranking detection, we can now filter out the histogram
                                        // Worth mentionning is that filtering will displace to the right the valley after initial max
FctParams           p;

                                        // manually filter the histogram with appropriate Gaussian, as to fill gaps due to subsampling
p ( FilterParamDiameter )     = 2 * GaussianSigmaToWidth ( KernelSubsampling ) + 1;
//p ( FilterParamDiameter )     = 2 * GaussianSigmaToWidth ( 2 * KernelSubsampling ) + 1;   // better for brains, kind of harming for full head

h.Filter ( FilterTypeGaussian, p, false );


//PostfixFilename ( _file, ".Filtered" );
//h.WriteFile     ( _file );
//_file.Open      ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we supposed to have real data, not mask / discrete values
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Crop histogram

/*                                        // clear useless 0 background ?
int                 minpos          = h[ 0 ] != 0 && h[ 1 ] == 0 ? 0 : -1;
//int                 minpos        = 0;
                                        // crop beginning
for ( int i = 0; i <= minpos; i++ )
    h[ i ]  = 0;
*/

int                 maxpos          = h.GetPercentilePosition ( BinUnit, 0.99 ); // for segmented brain?

//DBGV2 ( minpos, maxpos, "cropping histogram" );

                                        // crop tail
for ( int i = maxpos; i < Dim1; i++ )

    h[ i ]  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We want to locate the biggest mode on the lower half of the histogram

double              secondhalf      = h.GetMiddlePosition ( BinUnit );
int                 nummodes;
int                 nummodesfirsthalf;
#define             NumHistoDegrade     100
                                        // slowly degrade the histogram to reach 2 / 3 modes
                                        // set a limit to the # of iterations, just in case
for ( int loop = 0; loop < NumHistoDegrade; loop++ ) {
                                        // heavy filtering to degrade the histogram
    p ( FilterParamDiameter )     = 2 * 5 + 1;

    h.Filter ( FilterTypeGaussian, p, false );

                                        // see what is left
//  nummodesfirsthalf   = h.GetNumModes ();


    nummodes            = h.GetNumModes ();

                                        // we are interested in the first half of the histogram
    nummodesfirsthalf   = 0;

    for ( int i = 1; i <= nummodes; i++ )
        if ( h.GetModePosition ( BinUnit, i ) <= secondhalf )
            nummodesfirsthalf++;


//  TFileName           _file;
//  sprintf ( _file, "E:\\Data\\Background Histogram.%02d.sef", loop );
//  h.WriteFile ( _file );
//  DBGV ( loop, "loop" );

                                        // we aim for 2 or 3 modes - usually 1 or 2
    if ( nummodesfirsthalf <= 2 /*3*/ )
        break;
    }


//TFileName           _file;
//StringCopy  ( _file, "E:\\Data\\Background Histogram.99.sef" );
//h.WriteFile ( _file );
//DBGV2 ( nummodesfirsthalf, secondhalf, "nummodesfirsthalf  secondhalf" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // No modes were found in the first half of histogram
                                        // Maybe caller removed the null values?
if ( nummodesfirsthalf == 0 )
                                        // anyway, we bail out and return a sort of fool-proof value
    return  maxstat / ErrorBackgroundValue;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose the mode of the background
                                        // here, 1, 2 or 3 modes in second part of histogram
//                  nummodes        = h.GetNumModes (); // still up-to-date from loop above
int                 whichmode;


if      ( nummodesfirsthalf == 1 ) {

    whichmode   = 1;

//    DBGV2 ( h.GetModePosition ( BinUnit, 1 ), h.GetModeValue ( 1 ), "1 Mode position, value" );
    }

else if ( nummodesfirsthalf == 2 ) {
                                        // in case of pseudo-background, pick the second one

    double              mode1           = h.GetModeValue     ( 1 );
    double              mode2           = h.GetModeValue     ( 2 );
//  double              modepos1        = h.GetModePosition  ( BinUnit, 1 );
    double              modepos2        = h.GetModePosition  ( BinUnit, 2 );

//    DBGV4 ( modepos1, modepos2, mode1, mode2, "2 Modes positions, values" );

/*
                                        // relative height of the mode 2 is high (2 backgrounds) -> mode 2
    TVector< double >   relheight ( 2 );
                                        // uncomment to select second mode if below mode 3
    if ( mode2 > mode1 )                // mode 2 above mode 1?
        relheight[ 0 ]  = 0.33;
    else                                // mode 2 is high -> mode 2
        relheight[ 0 ]  = mode2 / (double) ( mode1 + 1e-6 ) > 0.10 ? 0.33 : 0.66;
    relheight[ 1 ]      = 1 - relheight[ 0 ];

                                        // mode 2 position is closer to mode 1 -> mode 2
    TVector< double >   relpos ( 2 );
    relpos[ 0 ]         = ( modepos2 - modepos1 ) / (double) ( h.GetExtent () + 1e-6 ) < 0.25 ? 0.33 : 0.66;
    relpos[ 1 ]         = 1 - relpos[ 0 ];

                                        // compose criterion
    TVector< double >   mode12 ( relheight * relpos );
    mode12.Normalize1 ();

//    DBGV2 ( relheight[ 0 ], relheight[ 1 ], "relheight" );
//    DBGV2 ( relpos[ 0 ], relpos[ 1 ], "relpos" );
//    DBGV2 ( mode12[ 0 ], mode12[ 1 ], "mode 1 or 2 / 3" );


    whichmode   = mode12[ 0 ] > mode12[ 1 ] ? 1 : 2;
*/
                                        // higher than wider -> mode 1
    whichmode   = mode2 > mode1 ? 1
               // about 33% / 4%                      about 4% / 35%
                : mode2 / ( mode1 + 1e-6 ) < 1.5 * modepos2 / ( h.GetExtent ( BinUnit ) + 1e-6 ) ? 1 : 2;
    }

else if ( nummodesfirsthalf >= 3 ) {    // could be more than 3, if the loop above failed to converge

    double              mode1           = h.GetModeValue     ( 1 );
    double              mode2           = h.GetModeValue     ( 2 );
    double              mode3           = h.GetModeValue     ( 3 );
    double              modepos1        = h.GetModePosition  ( BinUnit, 1 );
    double              modepos2        = h.GetModePosition  ( BinUnit, 2 );
    double              modepos3        = h.GetModePosition  ( BinUnit, 3 );
    double              diff12          = RelativeDifference ( mode1, mode2 );
    double              diff13          = RelativeDifference ( mode1, mode3 );
    double              diff23          = RelativeDifference ( mode2, mode3 );

//    DBGV6 ( modepos1, modepos2, modepos3, mode1, mode2, mode3, "3 Modes positions, values" );
//    DBGV3 ( diff12, diff13, diff23, "RelDiff 12 13 23" );

                                        // diff23 is min (small bumps 2 & 3) -> mode 1
    TVector<double>     mindiff23 ( 2 );
    mindiff23[ 0 ]      = diff23 < diff12 && diff23 * 0.50 < diff13 ? 0.66 : 0.33;
    mindiff23[ 1 ]      = 1 - mindiff23[ 0 ];

                                        // relative height of the mode 2 is high (2 backgrounds) -> mode 2
    TVector<double>     relheight ( 2 );
                                        // uncomment to select second mode if below mode 3
//  if ( mode2 < mode3 * 0.80 )        // mode 2 below mode 3, ignore mode 2
//      relheight[ 0 ]  = 0.33;
//  else                                // mode 2 above mode 3, but how much?
        relheight[ 0 ]  = ( mode2 - mode3 ) / ( mode1 - mode3 + 1e-6 ) < 0.10 ? 0.66 : 0.33;
    relheight[ 1 ]      = 1 - relheight[ 0 ];

                                        // mode 2 position is closer to mode 1 -> mode 1
    TVector<double>     relpos ( 2 );
    relpos[ 0 ]         = ( modepos2 - modepos1 ) / ( modepos3 - modepos1 + 1e-6 );
    relpos[ 1 ]         = 1 - relpos[ 0 ];

                                        // compose criterion
    TVector<double>     mode12 ( mindiff23 * relheight * relpos );
    mode12.Normalize1 ();

//    DBGV2 ( mindiff23[ 0 ], mindiff23[ 1 ], "mindiff23" );
//    DBGV2 ( relheight[ 0 ], relheight[ 1 ], "relheight" );
//    DBGV2 ( relpos[ 0 ], relpos[ 1 ], "relpos" );
//    DBGV2 ( mode12[ 0 ], mode12[ 1 ], "mode 1 or 2 / 3" );


    whichmode   = mode12[ 0 ] > mode12[ 1 ] ? 1 : 2;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from the background mode, compute background cut estimators

                                        // get some interesting variables
int                 whichmodepos    = h.GetModePosition ( BinUnit, whichmode );
int                 nextinfl        = h.GetNextInflexionPosition ( whichmodepos, true );
//int                 nextlcorner     = h.GetNextLCornerPosition ( nextinfl );
int                 minmodes        = whichmode < nummodes ? h.GetModesMiddlePosition ( BinUnit, whichmode, whichmode + 1 )
                                                           : h.GetNextLCornerPosition ( nextinfl );
//int               nextmodepos     = h.GetModePosition ( BinUnit, whichmode < nummodes ? whichmode + 1 : whichmode );
//int               previnfl        = H.GetNextInflexionPosition ( 0.75 * minmodes + 0.25 * nextmodepos, false );

h.Erode ( 0.001 );
int                 lastpos         = h.GetLastPosition ( BinUnit );


//DBGV3 ( nummodesfirsthalf, whichmode, whichmodepos, "#ofmodes  back mode # & position" );
//DBGV4 ( whichmode, whichmodepos, nextinfl, nextlcorner, "mode#, searching from -> next inflexion, next LCorner" );
//DBGV2 ( whichmode, minmodes, "which mode -> next min" );
//DBGV2 ( nextmodepos, previnfl, "next mode -> previous inflexion" );
//DBGV2 ( whichmodepos, lastpos, "searchfrom & lastpos" );

                                        // compute background estimators from our variables
//double              est1a           = ToReal ( unit, 6.00 * nextinfl    - 5.00 * whichmodepos ); // further from inflexion, from max position
//double              est1b           = ToReal ( unit, 1.75 * nextlcorner - 0.33 * whichmodepos ); // further from L corner, from max position
//double              est1c           = ToReal ( unit, 2.30 * nextlcorner - 0.90 * nextinfl );     // further from L corner, from inflexion

//double            est2            = ToReal ( unit, minmodes );                                 // min between modes
double              est2            = ToReal ( unit, 0.05 * whichmodepos + 0.95 * minmodes );    // min between modes, - compensation for filter drifts

double              est3            = ToReal ( unit, 0.85 * whichmodepos + 0.15 * lastpos );     // reasonable position, as a percentage of spreading


//DBGV5 ( est1a, est1b, est1c, est2, est3, "estimators" );
//DBGV2 ( est2, est3, "estimators" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // combine estimators into the final cut
double              back;

                                        // do some stats on estimators, to see their coherence
TEasyStats          stat;

//stat.Add ( est1a, ThreadSafetyIgnore );   // a bit weak - doesn't rescale nicely with subsampling
//stat.Add ( est1b, ThreadSafetyIgnore );   // idem
//stat.Add ( est1c, ThreadSafetyIgnore );   // idem
stat.Add ( est2, ThreadSafetyIgnore  );     // one of the best
stat.Add ( est3, ThreadSafetyIgnore  );     // one of the best

                                        // simple average with enough estimators seems good
                                        // add a bit of overshooting to compensate from downsampling
back    = Clip ( stat.Average () * 1.10, 0.0, maxstat );

                                        // give more weight to the min, if the quality seems bad (high CoV)
//double              wmin            = Clip ( stat.CoV (), (double) 0, (double) 1 );
//double              wavg            = 1 - wmin;
//back    = crtl::AtLeast ( 0.0, wavg * stat.Average () + wmin * stat.Min () );


//stat.Show ( "Estimators" );
//DBGV4 ( stat.Average (), stat.Min (), wavg, back, "Avg  Min  CoV -> wAvg -> back" );
//DBGV2 ( nummodesfirsthalf, ToReal ( RealUnit, back ), "#modes -> background" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // A little bit of rounding, the amount of which is estimated from the max data
double              precision       = Power10 ( RoundAbove ( Log10 ( maxstat ) ) ) / RoundBackgroundValue;

back    = RoundTo ( back, precision );

//DBGV2 ( back, precision, "background, precision" );

return  back;
}


//----------------------------------------------------------------------------
                                        // Internal use only, works on bin indexes
int     THistogram::Descend ( int b )   const
{
if ( b >= Dim1 )
    return b;

for ( ; b < Dim1 - 1; b++ )
    if ( Array[ b ] < Array[ b + 1 ] )
        break;

return b;
}


int     THistogram::Ascend ( int b )    const
{
if ( b >= Dim1 )
    return b;

for ( ; b < Dim1 - 1; b++ )
    if ( Array[ b ] > Array[ b + 1 ] ) // was >= ?
        break;

return b;
}


//----------------------------------------------------------------------------
/*                                      // This code could be useful to search hills and valleys, as it has been tested
void        NormalizeBumpPit ( float *data, int numpts, bool bump, float *results )
{

int                 tf1;
int                 tf2;
//int                 tftop;
TEasyStats          stat;

                                        // currently it will skip first clipped hill - it needs a low to begin
//if ( data[ 1 ] > data[ 0 ] )
//    tf1     = 0;

                                        // downhill
for ( tf1 = 0; tf1 < numpts - 1; tf1++ )
    if (   bump && data[ tf1 + 1 ] > data[ tf1 ]
      || ! bump && data[ tf1 + 1 ] < data[ tf1 ]  )
        break;

if ( tf1 >= numpts - 1 ) {
    tf2     = 0;
    goto    bumpofinito;
    }


do {
                                        // escalate
    for ( tf2 = tf1 + 1; tf2 < numpts - 1; tf2++ )
        if (   bump && data[ tf2 + 1 ] < data[ tf2 ]
          || ! bump && data[ tf2 + 1 ] > data[ tf2 ]  )
            break;

//    if ( tf2 >= numpts - 1 )
//        return;

//    tftop   = tf2;

                                        // downhill again
    for ( tf2; tf2 < numpts - 1; tf2++ )
        if (   bump && data[ tf2 + 1 ] > data[ tf2 ]
          || ! bump && data[ tf2 + 1 ] < data[ tf2 ]  )
            break;

//    if ( tf2 >= numpts - 1 )
//        return;
                                        // [tf1..tf2] contains the next local bump/pit

                                        // analyze that bump
    stat.Reset ();

    for ( int tfi = tf1; tfi <= tf2; tfi++ )
        stat.Add ( data[ tfi ], ThreadSafetyIgnore );

//    double      energy      = stat.SD ();
//    double      energy      = stat.SD () / ( tf2 - tf1 + 1 ) * 10;
    double      energy      = stat.Average ();
    double      maxv        = stat.Max ();
    double      minv        = stat.Min ();

                                        // just poke a constant energy value
//    for ( int tfi = tf1; tfi <= tf2; tfi++ )
//        results[ tfi ]  = energy;

                                        // poke the SD with the normalized enveloppe of the data
//    double      rescale = maxv - data[ tf1 ];
//
//    for ( int tfi = tf1; tfi <= tftop; tfi++ )
////        results[ tfi ]  = (double) ( tfi - tf1 ) / ( tftop - tf1 ) * energy;  // linear
//        results[ tfi ]  = ( data[ tfi ] - data[ tf1 ] ) * rescale * energy;
//
//
//    rescale     = maxv - data[ tf2 ];
//
//    for ( int tfi = tftop + 1; tfi <= tf2; tfi++ )
////        results[ tfi ]  = (double) ( tfi - tf2 ) / ( tftop - tf2 ) * energy;  // linear
//        results[ tfi ]  = ( data[ tfi ] - data[ tf2 ] ) * rescale * energy;


                                        // reshape energy with normalized min..max
    for ( int tfi = tf1; tfi <= tf2; tfi++ )
//        results[ tfi ]  = bump ? maxv == minv ? 0 : ( data[ tfi ] - minv ) / ( maxv - minv ) * energy
        results[ tfi ]  = bump ? maxv ? data[ tfi ] / maxv * energy : 0
//                               : ( maxv - data[ tfi ] ) / ( maxv - minv ) * energy;
//                               : ( maxv - data[ tfi ] ) / ( maxv - minv ) / minv * energy;  // penalize high pits
//                               : ( maxv - data[ tfi ] ) / ( maxv - minv ) / minv;   // interesting
//                               : 10 / ( 1 + maxv ) / ( 1 + minv ) / ( 1 + data[ tfi ] );
                               : 10 / ( 1 + minv ) / ( 1 + data[ tfi ] );   // good! low min and low data -> high criterion


                                        // next shape
    tf1     = tf2;

    } while ( tf2 < numpts - 1 );


bumpofinito:
                                        // there is always a trail to set
for ( tf2++; tf2 < numpts; tf2++ )
    results[ tf2 ]  = 0;

}
*/


//----------------------------------------------------------------------------
int     THistogram::GetNumModes ()  const
{
int                 n;
bool                descend;
int                 b               = GetFirstPosition ( BinUnit );

                                        // trivial mode?
if      ( b == Dim1 - 1 )   return 1;
else if ( b == -1       )   return 0;


if ( Array[ b ] > Array[ b + 1 ] )
    n = 1, descend = true;
else
    n = 0, descend = false;


do {
    if ( descend ) {
        b = Descend ( b );
        descend = false;
        }
    else {
        b = Ascend ( b );
        descend = true;
        n++;
        }

//  for ( ; b < Dim1 - 1; b++ )
//      if ( Array[ b ] != 0 )
//          break;

    } while ( b < Dim1 - 2 );
//n--;

return n;
}


//----------------------------------------------------------------------------
double  THistogram::GetModePosition ( HistoUnit unit, int m )   const
{
                                        // Search is done in bin space
int                 n;
bool                descend;
int                 b               = GetFirstPosition ( BinUnit );
int                 nmodes          = GetNumModes ();

                                        // wrong parameter?
if ( m < 1 || m > nmodes )
    return -1;

                                        // trivial mode?
if      ( b == Dim1 - 1 )   return b == 1 ? ToReal ( unit, b ) : -1;
else if ( b == -1       )   return -1;


if ( Array[ b ] > Array[ b + 1 ] )
    n = 1, descend = true;
else
    n = 0, descend = false;
                                        // first mode?
if ( descend && m == 1 )
    return  ToReal ( unit, b );


do {
                                        // reached the right mode?
    if ( m == n )
        return  ToReal ( unit, b );

    if ( descend ) {
        b = Descend ( b );
        descend = false;
        }
    else {
        b = Ascend ( b );
        descend = true;
        n++;
        }
    } while ( b < Dim1 - 2 );

                                        // the last bin
return  ToReal ( unit, b );
}


double  THistogram::GetMaximumModePosition ( HistoUnit unit )   const
{
int                 nmodes          = GetNumModes ();


if ( nmodes == 0 )
    return  0;


int                 modepos;
int                 maxmodepos;
double              maxmode         = -1;


for ( int i = 1; i <= nmodes; i++ ) {

    modepos         = GetModePosition ( BinUnit, i );

    if ( Array[ modepos ] > maxmode ) {
        maxmodepos  = modepos;
        maxmode     = Array[ modepos ];
        }
    }

return  ToReal ( unit, maxmodepos );
}


int     THistogram::GetMaximumModeIndex ()  const
{
int                 nmodes          = GetNumModes ();


if ( nmodes == 0 )
    return  0;


int                 modepos;
int                 maxmodeindex;
double              maxmode         = -1;


for ( int i = 1; i <= nmodes; i++ ) {

    modepos         = GetModePosition ( BinUnit, i );

    if ( Array[ modepos ] > maxmode ) {
        maxmodeindex= i;
        maxmode     = Array[ modepos ];
        }
    }

return  maxmodeindex;
}


double  THistogram::GetModeValue ( int m )  const
{
int                 modepos         = GetModePosition ( BinUnit, m );

if ( modepos == -1 )
    return  -1;

return  Array[ modepos ];
}


//----------------------------------------------------------------------------
double  THistogram::GetModesMiddlePosition ( HistoUnit unit, int m1, int m2 )   const
{
int                 nmodes          = GetNumModes ();


if ( ! ( IsInsideLimits ( m1, 1, nmodes ) && IsInsideLimits ( m2, 1, nmodes ) && m1 != m2 ) )
    return -1;


CheckOrder ( m1, m2 );

int                 b1              = GetModePosition ( BinUnit, m1 );
int                 b2              = GetModePosition ( BinUnit, m2 );
double              minvalue        = Array[ b1 ];
int                 minpos          = b1;

for ( int b = b1; b <= b2; b++ )
    if ( Array[ b ] < minvalue ) {
        minvalue    = Array[ b ];
        minpos      = b;
        }


return  ToReal ( unit, minpos );
}


//----------------------------------------------------------------------------
double  THistogram::GetPercentilePosition ( HistoUnit unit, double p )  const
{
double              area            = GetTotalArea ( BinUnit );

if ( area == 0 )
    return  0;

crtl::Clipped ( p, 0.0, 1.0 );


double              sum             = Array[ 0 ] / area;
int                 b               = 0;

for ( ; b < Dim1; b++, sum += Array[ b ] / area )

    if ( sum >= p )
                                        // which side is the closest?
        return b > 0 && p - ( sum - Array[ b ] / area ) < sum - p ? ToReal ( unit, b - 1 )
                                                                  : ToReal ( unit, b     );


return  ToReal ( unit, Dim1 - 1 );
}


//----------------------------------------------------------------------------
int     THistogram::GetNumZeroPlateaux ( HistoUnit unit, double from, double to )   const
{
if ( unit == RealUnit ) {
    from    = ToIndex ( from );
    to      = ToIndex ( to   );
    }

if ( from < 0 || to < 0 )
    return 0;


int                 n               = 0;
bool                inplateau       = false;


for ( int b = from; b <= to; b++ )

    if ( Array[ b ] == 0 )
        if ( inplateau )
            continue;
        else { // entering a new plateau
            inplateau = true;
            n++;
            }
    else
        inplateau = false;

return  n;
}


//----------------------------------------------------------------------------
void    THistogram::GetValleys ( int minscale, int maxscale, int scalestep )
{
FctParams           p;

THistogram          LoG        ( Dim1 );
THistogram          bestscales ( Dim1 );
//THistogram        allscales  ( ( maxscale - minscale ) / scalestep + 1, Dim1 );


//bestscales      = -DBL_MAX;   // for max
bestscales      = 0;            // for average


for ( int si = minscale; si <= maxscale; si += scalestep ) {
                                        // copy data
    LoG         = *this;

                                        // compute LoG
    p ( FilterParamDiameter )     = si / SqrtTwo;         // theory says Gaussian radius should be R/sqrt(2) when looking for blob of radius R

                                        // ?this shouldn't be there?
    LoG.Filter ( FilterTypeGaussian, p, false );

    LoG.Filter ( FilterTypeLoG,      p, false );


/*                                        // KL way - remove the FilterTypeLoG beforehand, remove the bestscales summation afterhand

    double              w1, w2, w3;
    double              d1, d2;
    double              dd;
    double              norm;


    for ( int i = 0; i < Dim1; i++ ) {

        w1  = LoG[ crtl::AtLeast ( 0, i - minscale / 2 )        ];
        w2  = LoG[ i                                      ];
        w3  = LoG[ Crtl::NoMore  ( Dim1 - 1, i + minscale / 2 ) ];

        d1  = w1 - w2;
        d2  = w2 - w3;

                                            // use normalized Laplacian, less sensitive to division by "0"
    //  norm    = var ( segw, nclusters - 1 );
    //  norm    = w1;
    //  norm    = ( w1 + w2 + w3 ) / 3;
        norm    = max ( w1, w2, w3 );
    //  norm    = max ( fabs ( sqrt ( d1 * d1 + 1 ) ), fabs ( sqrt ( d2 * d2 + 1 ) ) );
    //  norm    = 1;

                                            // keeping all values:
    //  var ( segkl, nclusters ) = ( d1 - d2 ) / NonNull ( norm );

                                            // clipping irrelevant values:

                                // no raising, no inflexion,  no concave
    //  var ( segkl, nclusters ) = d1 < 0 ||   d1 * d2 < 0 || d1 <= d2 ? 0

           // no raising, no concave / inflexion, no concave / same slope
        dd  = d1 < 0   || d1 < 0 && d2 > 0     || d1 <= d2 ? 0
            : ( d1 - d2 ) / NonNull ( norm );

                                        // average all scales
        bestscales[ i ]    += dd;
        }
*/

                                        // store highest values in result buffer
                                        // Laplacian is negative for bumps, positive for valleys
    for ( int i = 0; i < Dim1; i++ )
                                        // keep highest scale
//        if ( LoG[ i ] > bestscales[ i ] )
//            bestscales[ i ] = LoG[ i ];
                                        // average all scales
        bestscales[ i ]    += LoG[ i ];


//    for ( int i = 0; i < Dim1; i++ )
//        allscales ( ( si - minscale ) / scalestep, i ) = LoG[ i ];
    }

bestscales.NormalizeMax ();


//allscales.WriteFile ( "E:\\Data\\Scale Space Laplacian.sef" );


*this   = bestscales;
}


//----------------------------------------------------------------------------
/*                                        // Looking at the data as a track, finding the max position and left/right spreading
template <class TypeD>
void    TVector<TypeD>::ComputeZScoreFactors    ( double& center, double& spreadleft, double& spreadright )
{
center      = 0;
spreadleft  = 0;
spreadright = 0;


if ( IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this one was easy
center      = GetMaxPosition ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Browsing the curves directly
                                        // Center is estimated by the arg max
                                        // Spreading is estimated by the % of decay of an unknown Gaussian
enum                GaussianCutEnum
                    {
                    GaussianCutSD,              // requested number of SD of a Gaussian..
                    GaussianCutHeight,          // ..gives this height
                    GaussianCutThreshold,       // actual threshold for a given curve
                    GaussianCutDone,            // flag to stop the search
                    NumGaussianCutEnum,
                    };

int                 numgaussianestim    = 5;    // number of SD estimators
TArray2<double>     gaussiancut ( numgaussianestim, NumGaussianCutEnum );

                                        // here we set the SD distances of a Gaussian, remaining above 50% as some curves fall very slowly and would have no values
gaussiancut ( 0, GaussianCutSD )    = 0.3;  // 96% decay 
gaussiancut ( 1, GaussianCutSD )    = 0.6;  // 84%
gaussiancut ( 2, GaussianCutSD )    = 0.8;  // 73%
gaussiancut ( 3, GaussianCutSD )    = 1.0;  // 61%
gaussiancut ( 4, GaussianCutSD )    = 1.2;  // 49%


for ( int esti = 0; esti < numgaussianestim; esti++ ) {
                                        // convert SDs to Gaussian heights
    gaussiancut ( esti, GaussianCutHeight )     = Gaussian ( gaussiancut ( esti, GaussianCutSD ), 0, GaussianSigmaToWidth ( 1 ), 1 );
                                        // set curve thresholds
    gaussiancut ( esti, GaussianCutThreshold )  = Array[ (int) center ] * gaussiancut ( esti, GaussianCutHeight );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEasyStats          sdleft  ( numgaussianestim );
TEasyStats          sdright ( numgaussianestim );

                                        // search for SD left
for ( int esti = 0; esti < numgaussianestim; esti++ )

    gaussiancut ( esti, GaussianCutDone      )  = false;


for ( int tf = center - 1; tf >= 0; tf-- )
                                        // run through all estimators
for ( int esti = 0; esti < numgaussianestim; esti++ )

    if ( ! gaussiancut ( esti, GaussianCutDone ) 
           && Array[ tf ] <= gaussiancut ( esti, GaussianCutThreshold ) ) {
            
        sdleft .Add ( ( center - ( tf + ( gaussiancut ( esti, GaussianCutThreshold ) - Array[ tf ] ) 
                                        / NonNull ( Array[ tf + 1 ] - Array[ tf ] )                   ) )
                        / gaussiancut ( esti, GaussianCutSD ), ThreadSafetyIgnore );

        gaussiancut ( esti, GaussianCutDone )   = true;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for SD right
for ( int esti = 0; esti < numgaussianestim; esti++ )

    gaussiancut ( esti, GaussianCutDone      )  = false;


for ( int tf = center + 1; tf < Dim1; tf++ )
                                        // run through all estimators
for ( int esti = 0; esti < numgaussianestim; esti++ )

    if ( ! gaussiancut ( esti, GaussianCutDone ) 
           && Array[ tf ] <= gaussiancut ( esti, GaussianCutThreshold ) ) {
           
        sdright.Add ( ( ( tf - ( gaussiancut ( 0, GaussianCutThreshold ) - Array[ tf ] ) 
                                / NonNull ( Array[ tf + 1 ] - Array[ tf ] )                ) - center ) 
                        / gaussiancut ( esti, GaussianCutSD ), ThreadSafetyIgnore );

        gaussiancut ( esti, GaussianCutDone )   = true;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Getting some robust estimates
spreadleft  = sdleft .Median ();
spreadright = sdright.Median ();

//DBGV6 ( center, spreadleft, spreadright, Dim1, sdleft.GetNumItems (), sdright.GetNumItems (), "center, spreadleft, spreadright, Dim1, sdleft.GetNumItems (), sdright.GetNumItems ()" );
}
*/


                                        // maxbinspertf  gives the precision of the histogrammed curve, usually give 1000
void    THistogram::ComputeZScoreFactors    (   int             maxbinspertf,
                                                double&         center,         double&     spreadleft, double&     spreadright 
                                            )   const
{
center      = 0;
spreadleft  = 0;
spreadright = 0;


if ( IsNotAllocated () || GetAbsMaxValue () == 0 || maxbinspertf <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this one was easy
//center      = GetMaxPosition ( RealUnit );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Better: stats to estimate center and spread
double              tracksscaling   = maxbinspertf / NonNull ( GetAbsMaxValue () );
TEasyStats          stats ( Dim1 * maxbinspertf * 0.50 );   // as a initial guess, we estimate that about half curve is filled in average


                                        // a bit tricky to fill the stats
for ( int tf = 0; tf < Dim1; tf++ )
                                        // fill as many items as multiple of this [0..1] value
    for ( int i = 0; i < Array[ tf ] * tracksscaling; i++ ) 

        stats.Add ( tf, ThreadSafetyIgnore );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Robust estimate for center
TEasyStats          statcenter ( NumMaxModeRobustEstimates + 1 );

stats.MaxModeRobust ( statcenter );

int                 c0      = GetMaxPosition ( RealUnit );  // an additional good contender

statcenter.Add ( c0, ThreadSafetyIgnore );
                                        // if max is 0, keep it this way?
center  = c0 == 0 ? 0 : statcenter.Median ( false );

//if ( VkQuery () ) statcenter.Show ( "THistogram::ComputeZScoreFactors  Center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Bunch of estimates for (asymmetrical) spreading
double              sdleft;
double              sdright;
double              madleft;
double              madright;

                                        // these estimates are actually quite robusts
stats.SDAsym                 ( center, sdleft, sdright );

stats.MADAsym                ( center, madleft, madright );


spreadleft      = ( madleft  + sdleft  ) / 2;
spreadright     = ( madright + sdright ) / 2;

//DBGV4 ( sdright, madright, iqrright, spreadright, "sdright, madright, iqrright -> spreadright" );
//DBGV3 ( center, spreadleft, spreadright, "center, sdleft, sdright" );
}


//----------------------------------------------------------------------------
double  THistogram::HowSymetric ( int &T )  const
{
/*
TVector<double>     hs ( Dim1 );
                                        // copy as symetric
for ( int b=0; b < Dim1; b++ )
    hs[ b ] = Array[ Dim1 - 1 - b ];

double  maxcc = Array->MaxCrossCorrelation ( hs, T );

return  maxcc / NonNull ( GetTotalArea() );
*/

TVector<double>     h1 ( *this );
TVector<double>     h2 ( Dim1  );

                                        // copy as symetric
for ( int b = 0; b < Dim1; b++ )
    h2[ Dim1 - 1 - b ] = Array[ b ];


h1.Smooth           ();                 // reduce noise
h2.Smooth           ();

h1.AverageReference ();                 // increase contrast
h2.AverageReference ();

h1.Normalize        ();                 // to have normalized values
h2.Normalize        ();


double              maxcc           = h1.MaxCrossCorrelation ( h2, T );

return  maxcc / Dim1;
}


//----------------------------------------------------------------------------
void    THistogram::WriteFileVerbose ( char *file, bool logplot )   const
{
if ( StringIsEmpty ( file ) )
    return;


TFileName           file2 ( file, TFilenameExtendedPath );

if ( ! file2.IsExtension ( FILEEXT_TXT ) )
    ReplaceExtension ( file2, FILEEXT_TXT );

CheckNoOverwrite    ( file2 );

StringCopy          ( file, file2 );

ofstream            o ( file2 );


if ( o.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              area            = GetTotalArea ( BinUnit );
double              maxvalue        = GetMaxValue ();
double              resize          = (double) 80 / NonNull ( maxvalue );
int                 maxpos          = GetMaxPosition ( RealUnit );


o << StreamFormatFixed;
o << StreamFormatFloat32 << "Index";
o << StreamFormatFloat32 << "Prob. [0/00]";
o << NewLine;


for ( int b = 0; b < Dim1; b++ ) {
                                        // index, in real values
    o << StreamFormatFloat32 << ToReal ( RealUnit, b );

                                        // exact value
    o << StreamFormatFloat32 << (float) ( Array[ b ] * 1000 );

//                                      // relative value
//  o << StreamFormatInt8    << Round ( (double) Array[ b ] / NonNull ( area ) * 1000 );

                                        // a proportional plot
    o << " ";

    if ( logplot )
        o.width ( (int) ( log ( (double) Array[ b ] / NonNull ( maxvalue ) * 10000 + 1 ) / log ( (double) 10001 ) * 80 ) );
    else
        o.width ( (int) ( Array[ b ] * resize + 0.5 ) );

                                        // output a character symbolizing the current bin:
                                        // M for max, | for flat, > for max, < for min, / and \ for slopes
    if      ( b == maxpos                                             ) o << "M";
    else if ( b > 0        && Array[ b - 1 ] == Array[ b ]
           || b < Dim1 - 1 && Array[ b + 1 ] == Array[ b ] ) o << "|";
    else if ( b > 0        && Array[ b - 1 ] <  Array[ b ]
           && b < Dim1 - 1 && Array[ b + 1 ] <  Array[ b ] ) o << ">";
    else if ( b > 0        && Array[ b - 1 ] >  Array[ b ]
           && b < Dim1 - 1 && Array[ b + 1 ] >  Array[ b ] ) o << "<";
    else if ( b > 0        && Array[ b - 1 ] >  Array[ b ]
           || b < Dim1 - 1 && Array[ b + 1 ] <  Array[ b ] ) o << "/";
    else if ( b > 0        && Array[ b - 1 ] <  Array[ b ]
           || b < Dim1 - 1 && Array[ b + 1 ] >  Array[ b ] ) o << "\\";
    else                                                                o << "*";


    o << NewLine;
    }

o << NewLine NewLine;


o << "Maximum value:      " << GetMaxValue          ()           << NewLine;
o << "Maximum position:   " << GetMaxPosition       ( RealUnit ) << NewLine;
o << "Area:               " << GetTotalArea         ( RealUnit ) << NewLine;
o << "First bin:          " << GetFirstPosition     ( RealUnit ) << NewLine;
o << "Last bin:           " << GetLastPosition      ( RealUnit ) << NewLine;
o << "Extent:             " << GetExtent            ( RealUnit ) << NewLine;
o << "Middle:             " << GetMiddlePosition    ( RealUnit ) << NewLine;


int                 nmodes          = GetNumModes ();

o << "Number of modes:    " << nmodes << Tab "(";
for ( int m = 1; m <= nmodes; m++ )
    o << " " << GetModePosition ( RealUnit, m );
o << " )" NewLine;


//int     T;
//double  sym = HowSymetric ( T );
//o << "Symetry factor:     " << sym << "  with shift: " << T;
//o << NewLine NewLine;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
