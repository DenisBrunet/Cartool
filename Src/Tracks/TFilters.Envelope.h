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

#pragma once

#include    "Dialogs.Input.h"

#include    "Math.Utils.h"
#include    "Math.FFT.MKL.h"
#include    "FrequencyAnalysis.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // Temporal filter
//----------------------------------------------------------------------------

inline  void            CheckFilterTypeEnvelope ( FilterTypes& how );
inline  FilterTypes     GetEnvelopeTypeFromUser ();

                                        // !Analytic envelope expect signed data - for absolute data use another method!
template <class TypeD>
class   TFilterEnvelope : public TFilter<TypeD>
{
public:
                    TFilterEnvelope ();
                    TFilterEnvelope ( FilterTypes how, double fsamp, double width );


    void            Reset   ();
    void            Set     ( FilterTypes how, double fsamp, double width );

    void            Apply                   ( TypeD* data, int numpts );
    void            ApplySlidingWindow      ( TypeD* data, int numpts );    // will rectify data
    void            ApplyPeak               ( TypeD* data, int numpts );    // will rectify data
    void            ApplyAnalytic           ( TypeD* data, int numpts );    // input must be non-rectified, signed signal

    double          GetEnvelopeWidthTF      ( ArraySizeType sizeconstraint = AnySize )  const;  // Width[ms] to Width[TF]


                        TFilterEnvelope     ( const TFilterEnvelope& op  );
    TFilterEnvelope&    operator    =       ( const TFilterEnvelope& op2 );


protected:

    FilterTypes     How;
    double          SamplingFrequency;
    double          EnvelopeWidth;      // in [ms]

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // Checking the legal envelope filter types
void    CheckFilterTypeEnvelope ( FilterTypes& how ) 
{
if ( ! (    how == FilterTypeEnvelopeSlidingWindow 
         || how == FilterTypeEnvelopePeak
         || how == FilterTypeEnvelopeAnalytic      ) )

    how     = FilterTypeNone;
}

                                        // Asking user for envelope type
FilterTypes     GetEnvelopeTypeFromUser ()
{
char                option          = GetOptionFromUser (   "Envelope filter (NOT Analytic for positive data):" NewLine 
                                                            NewLine
                                                            Tab "- (S)liding Window - Signed or positive data"  NewLine
                                                            Tab "- (P)eak Envelope   - Signed or positive data"  NewLine 
                                                            Tab "- (A)nalytic           - Signed data only", 
                                                            "Envelope Filter", "S P A", "P" );

if      ( option == 'S' )   return FilterTypeEnvelopeSlidingWindow;
else if ( option == 'P' )   return FilterTypeEnvelopePeak;
else if ( option == 'A' )   return FilterTypeEnvelopeAnalytic;
else                        return FilterTypeNone;
}


//----------------------------------------------------------------------------
template <class TypeD>
        TFilterEnvelope<TypeD>::TFilterEnvelope ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterEnvelope<TypeD>::TFilterEnvelope ( FilterTypes how, double fsamp, double width )
{
Set ( how, fsamp, width );
}


template <class TypeD>
void    TFilterEnvelope<TypeD>::Reset ()
{
How                 = FilterTypeNone;
SamplingFrequency   = 0;
EnvelopeWidth       = 0;
}


template <class TypeD>
            TFilterEnvelope<TypeD>::TFilterEnvelope ( const TFilterEnvelope& op )
{
How                 = op.How;
SamplingFrequency   = op.SamplingFrequency;
EnvelopeWidth       = op.EnvelopeWidth;
}


template <class TypeD>
TFilterEnvelope<TypeD>& TFilterEnvelope<TypeD>::operator= ( const TFilterEnvelope& op2 )
{
if ( &op2 == this )
    return  *this;


How                 = op2.How;
SamplingFrequency   = op2.SamplingFrequency;
EnvelopeWidth       = op2.EnvelopeWidth;


return  *this;
}


template <class TypeD>
void    TFilterEnvelope<TypeD>::Set ( FilterTypes how, double fsamp, double width )
{
Reset ();

How                 = how;
SamplingFrequency   = fabs ( fsamp );
EnvelopeWidth       = fabs ( width );
}


template <class TypeD>
double  TFilterEnvelope<TypeD>::GetEnvelopeWidthTF ( ArraySizeType sizeconstraint ) const
{
                                        // Still in floating points, so converting from frequency to width and back to frequency will not loas precision
double              wtf             = MillisecondsToTimeFrame ( EnvelopeWidth, SamplingFrequency );

                                        // rounding to odd/even will forcibly convert to integer
if      ( sizeconstraint == OddSize  )      wtf = RoundToOdd  ( Round ( wtf ) );
else if ( sizeconstraint == EvenSize )      wtf = RoundToEven ( Round ( wtf ) );
//elseif( sizeconstraint == AnySize  )      wtf =               Round ( wtf );  // keeping the floating point precision

return  wtf;
}


//----------------------------------------------------------------------------
                                        // Global dispatcher to the right function - although the specialized functions are directly callable, too
template <class TypeD>
void    TFilterEnvelope<TypeD>::Apply ( TypeD* data, int numpts )
{
if ( data == 0 || numpts <= 0 )
    return;

if ( How == FilterTypeNone )
    return;


if      ( How == FilterTypeEnvelopeSlidingWindow    )   ApplySlidingWindow  ( data, numpts );
else if ( How == FilterTypeEnvelopePeak             )   ApplyPeak           ( data, numpts );
else if ( How == FilterTypeEnvelopeAnalytic         )   ApplyAnalytic       ( data, numpts );
}


//----------------------------------------------------------------------------
                                        // Averaging sliding window - fast
                                        // Envelope is rescaled to look like the Analytic "on-top" envelope
                                        // Works well on any type of narrow-band / broad-band filtered / non-filtered data
                                        // Data will be rectified internally
                                        // Uses mirroring at boundaries
template <class TypeD>
void    TFilterEnvelope<TypeD>::ApplySlidingWindow ( TypeD* data, int numpts )
{
                                        // convert width to a correct buffer size - must be odd
int                 EnvelopeWidthTF     = (int) GetEnvelopeWidthTF ( OddSize );
TVector<TypeD>      EnvelopeBuff ( EnvelopeWidthTF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rectify
for ( int ii = 0; ii < numpts; ii++ )
    data[ ii ]   = fabs ( data[ ii ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 halfwidth       = EnvelopeWidthTF / 2;  // will generate a shifting error of 0.5 TF to the left, if EnvelopeWidthTF is not odd - which it should usually be
double              sum;
int                 io              = 0;                    // EnvelopeBuff will act as a circular buffer, io is current insertion point
int                 i;

                                        // Examples shown for buffer for size 5 and 10 data points
                                        // first border
for ( ; io < halfwidth; io++ )
                                        // filling with mirror part:    #2 #1  0  0  0 
    EnvelopeBuff[ io ]  = data[ LeftMirroring ( 0, halfwidth - io, numpts - 1 ) ];

for ( i = 0; i < halfwidth; i++, io++ )
                                        // filling with data part:      #2 #1 #0 #1  0
    EnvelopeBuff[ io ]  = data[ i ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( ; i < numpts; i++ ) {
                                        // adding next data point:      #2 #1 #0 #1 #2, then #1 #0 #1 #2 #3, then #0 #1 #2 #3 #4
    EnvelopeBuff[ io ]  = data[ i ];


    sum     = 0;                        // redo the sum each time - better precision

    for ( int j = 0; j < EnvelopeWidthTF; j++ )
        sum += EnvelopeBuff[ j ];

    data[ i - halfwidth ] = sum / EnvelopeWidthTF /* * SqrtTwo */;

    io      = ++io % EnvelopeWidthTF;   // circular buffer
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last border
                                        // current buffer state:        #5 #6 #7 #8 #9
for ( ; i < numpts + halfwidth; i++ ) {
                                        // filling with mirror part:    #6 #7 #8 #9 #8, then #7 #8 #9 #8 #7
    EnvelopeBuff[ io ] = data[ RightMirroring ( numpts, i - numpts, numpts - 1 ) ];

    sum     = 0;                        // redo the sum each time - better precision

    for ( int j = 0; j < EnvelopeWidthTF; j++ )
        sum += EnvelopeBuff[ j ];

    data[ i - halfwidth ] = sum / EnvelopeWidthTF /* * SqrtTwo */;

    io      = ++io % EnvelopeWidthTF;   // circular buffer
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smoothing results make results quite nice
EnvelopeBuff.Resize ( max ( numpts, EnvelopeBuff.GetDim1 () ), (MemoryAllocationType) ( MemoryAuto | ResizeClearMemory ) );   // resize without shrinking
                                        // fast copy to buffer
CopyVirtualMemory ( EnvelopeBuff.GetArray (), data, numpts * sizeof ( TypeD ) );

                                        // then smoothing results
FctParams           p;

p ( FilterParamDiameter )     = Round ( GaussianWidthToSigma ( GetEnvelopeWidthTF () ) );

EnvelopeBuff.Filter ( FilterTypeGaussian, p, false );

                                        // ad-hoc rescaling to look more like the Analytic Envelope
EnvelopeBuff   *= 1.5;

                                        // fast copy back
CopyVirtualMemory ( data, EnvelopeBuff.GetArray (), numpts * sizeof ( TypeD ) );
}


//----------------------------------------------------------------------------
                                        // PeakToPeak Envelope - fast
                                        // Envelope is naturally looking like the Analytic "on-top" envelope, by construction
                                        // Works well on any type of narrow-band / broad-band filtered / non-filtered data
                                        // Data will be rectified internally
                                        // Does not need nor use any mirroring at boundaries - borders results are actually quite OK
template <class TypeD>
void    TFilterEnvelope<TypeD>::ApplyPeak ( TypeD* data, int numpts )
{
                                        // convert width to a correct buffer size
double              EnvelopeWidthTF     = GetEnvelopeWidthTF ();


if ( EnvelopeWidthTF <= 1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<TypeD>      EnvelopeBuff ( numpts );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rectify
for ( int i = 0; i < numpts; i++ )
    data[ i ]   = fabs ( data[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Closing: filling gaps by closing with linear interpolations
int                 radius          = AtLeast ( 1, Round ( EnvelopeWidthTF / 2 ) );
double              v1;
double              v2;
int                 tfi1m;
int                 tfi2m;

                                        // fast copy to buffer
CopyVirtualMemory ( EnvelopeBuff.GetArray (), data, numpts * sizeof ( TypeD ) );

                                        // range will cover left and right margins
for ( int tfi1 = -radius + 1; tfi1 < numpts; tfi1++ ) {

                                        // optional mirroring; going half a cycle further in case clipping just before a peak
    tfi1m   = tfi1 >= 0 ? tfi1 : LeftMirroring ( tfi1, /*0*/ radius / 2, numpts - 1 );

    v1      = data[ tfi1m ];

                                        // from tfi1, explore all radii, starting from 2 as it only modifies in-between extremas
    for ( int ri = 2, tfi2 = tfi1 + ri; ri <= radius; ri++, tfi2++ ) {

                                        // interval fully within left margin -> nothing to be done for that radius
        if ( tfi2 <= 0 ) continue;

                                        // optional mirroring;             going half a cycle further in case clipping just before a peak
        tfi2m   = tfi2 < numpts ? tfi2 : RightMirroring ( tfi1, ri + /*0*/ radius / 2, numpts - 1 );

        v2      = data[ tfi2m ];

                                        // interpolate between the extremas (both being excluded)
        for ( int filltfi = tfi1 + 1; filltfi < tfi2; filltfi++ )
                                        // check we are within limits - left and right margins can cause out-of-bounds
            if ( filltfi >= 0 && filltfi < numpts )
                                        // keeping the max across all top lines
                Maxed ( EnvelopeBuff[ filltfi ], (TypeD) LinearRescale ( filltfi, tfi1, tfi2, v1, v2, false ) );

        } // for tfi2
    } // for tfi1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // then smoothing results - as little as possible to retain the shape
FctParams           p;

p ( FilterParamDiameter )     = Round ( GaussianWidthToSigma ( EnvelopeWidthTF ) );

EnvelopeBuff.Filter ( FilterTypeGaussian, p, false );

                                        // fast copy back
CopyVirtualMemory ( data, EnvelopeBuff.GetArray (), numpts * sizeof ( TypeD ) );
}


/*                                      // Closing: filling gaps between consecutive local maxes, with a linear interpolation
                                        // It actually doesn't need a duration/distance, it will pick the maxes as they come

                                        // Scan each local max to wherever position they are
                                        // then linear fill between each pair of local maxes
                                        // The advantage is that it always take the next max without knowing the exact size of the jump
int                 prevmaxtfi      = 0;
double              prevmaxdata     = Array[ prevmaxtfi ];
int                 currmaxtfi;
double              currmaxdata;
double              deltadata;
int                 radius          = AtLeast ( 1, Round ( ( envelopewidth - 1 ) / 2.0 ) );


for ( int tfi = prevmaxtfi + 1; tfi < numpts - 1; tfi++ ) {

    currmaxtfi  = -1;
    currmaxdata =  0;

                                        // search within a local radius only
    for ( int tfri = tfi; tfri < numpts - 1; tfri++ ) {
                                        // local max + better max?
        if ( Array[ tfri ] >  Array[ tfri - 1 ]
          && Array[ tfri ] >  Array[ tfri + 1 ]
          && Array[ tfri ] >= currmaxdata ) {

            currmaxtfi  =        tfri;
            currmaxdata = Array[ tfri ];
            }

                                        // break at radius, but only if some value has been found, otherwise continue until we have a local max
      if ( currmaxtfi != -1 && tfri - tfi >= radius )
            break;
        } // local radius

                                        // at the end of data, this could happen
    if      ( currmaxtfi == -1 )
        break;

                                        // Here we have 2 maxes: prevmaxtfi and currmaxtfi
    deltadata   = currmaxdata - prevmaxdata;
                                        // compute the interpolation between the 2 TFs - the extrema don't need to be re-computed
    for ( int filltfi = prevmaxtfi + 1; filltfi < currmaxtfi; filltfi++ )
                                        // linear interpolation
        Array[ filltfi ]    = prevmaxdata + ( (double) ( filltfi - prevmaxtfi ) / ( currmaxtfi - prevmaxtfi ) ) * deltadata;

                                        // done, save current max
    prevmaxtfi  = currmaxtfi;
    prevmaxdata = currmaxdata;
    tfi         = currmaxtfi + 1;       // skip at least 2 TFs (when including the tfi++ of the loop), and some local radius
//    tfi++;
    }

                                        // manually force from last max to last TF, if at least 2 TFs of difference
if ( prevmaxtfi < numpts - 2 ) {

    currmaxtfi  =        numpts - 1;
    currmaxdata = Array[ numpts - 1 ];

    deltadata   = currmaxdata - prevmaxdata;
                                        // compute the interpolation between the 2 TFs - the extrema don't need to be re-computed
    for ( int filltfi = prevmaxtfi + 1; filltfi < currmaxtfi; filltfi++ )
                                        // linear interpolation
        Array[ filltfi ]    = prevmaxdata + ( (double) ( filltfi - prevmaxtfi ) / ( currmaxtfi - prevmaxtfi ) ) * deltadata;
    }
*/

//----------------------------------------------------------------------------
                                        // Using the well-known Analytic signal (= signal + i * Hilbert) Envelope - Nice-looking but could be slow
                                        // Works well ONLY on narrow-band filtered data - Broad-band are less convincing
                                        // Data need to be NON-rectified (because FFT)
                                        // Uses implicit cyclical mirroring at boundaries (because FFT)
                                        // numpts should be even for FFT(?)
template <class TypeD>
void    TFilterEnvelope<TypeD>::ApplyAnalytic ( TypeD* data, int numpts )
{
                                        // lowest and highest frequency indexes from FFT
int                 firstfreqi          = 0;
int                 lastfreqi           = numpts / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need the Hilbert transform, so we go through some Fourier to do that

                                        // ?can we put this as fields?
mkl::TMklFft        fft  ( mkl::FromReal,      FFTRescalingForward,  numpts );
mkl::TMklFft        ffti ( mkl::BackToComplex, FFTRescalingBackward, numpts );


TVector<AReal>      X ( numpts );
TVector<AComplex>   F ( numpts );       // allocate all frequency slots, though a real FFT will generate only numpts/2+1 frequencies
TVector<AComplex>   A ( numpts );


for ( int i = 0; i < numpts; i++ )   
    X ( i ) = data[ i ];

                                        // real FFT only - will not touch anything past freqsize in the F vector, which will remain 0
//fft ( data, F );  // for float case, we could skip the transfer...
fft ( X, F );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Signal + Hilbert in frequency domain is actually simple:
                                        // 1) double the frequencies - !but NOT for f=0 AND f=dim/2 (by symmetry)!
for ( int i = firstfreqi + 1; i < lastfreqi; i++ )
    F ( i )    *= 2;

                                        // 2) then clear-up the negative frequencies
//for ( int i = lastfreqi + 1; i < numpts; i++ )    // not needed as fft has not written anything past lastfreqi
//    F ( i )     = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // invert FFT
ffti ( F, A );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Analytic signal computation done, which already contains  signal + i * Hilbert
                                        // which norm is the instantaneous amplitude (i.e. envelope)  sm = norm ( sa )
for ( int i = 0; i < numpts; i++ )
                                        // Note: imaginary part contains Hilbert transform
    data[ i ]   = abs ( A ( i ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
template <class TypeD>
void    TFilter<TypeD>::SetHilbert ()
{
                                        // will act as a FIR, as feedback coefficients don't exist
BCoeff.Resize ( 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ACoeff.Resize ( Order );

ACoeff[ 0 ]         = 0;

for ( int i = 1; i < Order; i++ )

    ACoeff[ i ] = 2 / ( i * Pi ) * Square ( sin ( Pi * i / 2.0 ) );


//ACoeff.Resize ( 2 * Order + 1 );
//
//ACoeff[ Order ]         = 0;
//
//for ( int i = 1; i < Order; i++ )
//
//    ACoeff[ 2 * Order - i ] = ACoeff[ Order + i ] = 2 / ( i * Pi ) * Square ( sin ( Pi * i / 2.0 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//ACoeff[ 0 ]         = 0;
//
//for ( int i = 1; i < Order; i++ )
//
//    ACoeff[ i ] = ( -1 + cos ( Pi * i ) ) / ( Pi * i );
//
//for ( int i = 1; i < Order; i++ )
//
//    ACoeff[ i ] = ACoeff[ i ] * cos ( Pi / 2.0 * i / (double) ( Order - 1 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//for ( int i = 0; i < (int) ACoeff; i++ )
//    DBGV5 ( freq1 * fsamp, freq2 * fsamp, i, ACoeff[ i ] * sfr, BCoeff[ i ], "freq1 freq2   i -> a & b coeff" );
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


}
