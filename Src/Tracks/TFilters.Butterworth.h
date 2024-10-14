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

#pragma once

namespace crtl {

//----------------------------------------------------------------------------
                                        // Temporal filters
//----------------------------------------------------------------------------
                                        // Butterworth defines
enum            FilterCausality
                {
                FilterCausal,
                FilterNonCausal,
                };

                                        // Order limits for a single filter - must also be a multiple of 2
constexpr int   TFilterMinOrder                 =  2;
constexpr int   TFilterMaxOrder                 = 64;
constexpr char* TFilterMinOrderString           = "2";
constexpr char* TFilterMaxOrderString           = "64";
                                        // Order limits for a bandpass/bandstop filter - must also be a multiple of 4
constexpr int   TFilterMinTwiceOrder            = 2 * TFilterMinOrder;
constexpr int   TFilterMaxTwiceOrder            = 2 * TFilterMaxOrder;

constexpr int   ButterworthLowPassMinOrder      = TFilterMinOrder;
constexpr int   ButterworthLowPassMaxOrder      = TFilterMaxOrder;
constexpr int   ButterworthHighPassMinOrder     = TFilterMinOrder;
constexpr int   ButterworthHighPassMaxOrder     = TFilterMaxOrder;
constexpr int   ButterworthBandPassMinOrder     = TFilterMinTwiceOrder;
constexpr int   ButterworthBandPassMaxOrder     = TFilterMaxTwiceOrder;
constexpr int   ButterworthBandStopMinOrder     = TFilterMinTwiceOrder;
constexpr int   ButterworthBandStopMaxOrder     = TFilterMaxTwiceOrder;

//constexpr int TFilterDefaultOrder             = 2;   // historical value - not steep enough, though
constexpr int   TFilterDefaultOrder             = 8;
constexpr char* TFilterDefaultOrderString       = "8";
                                        // Using a Butteworth Bandstop as a Notch filter with these parameters (order multiple of 4, used as is in filter):
                                        // First version order was 12, but 8 looks actually very similar
constexpr int   TFilterNotchOrder               = 8;
constexpr double TFilterNotchWidth              = 1.0;
constexpr int   TFilterNotchMaxHarmonics        = 100;


//----------------------------------------------------------------------------
template <class TypeD>
class   TFilterButterworthHighPass  : public TFilter<TypeD>
{
public:
                    TFilterButterworthHighPass ();
                    TFilterButterworthHighPass ( int order, FilterCausality causal, double fsamp, double freq );


    void            Reset   ();
    void            Set     ( int order, FilterCausality causal, double fsamp, double freq );

    double          GetFrequencyCut         ()  const   { return  FrequencyCut;  }

    void            Apply                   ( TypeD* data, int numpts );


                                TFilterButterworthHighPass      ( const TFilterButterworthHighPass& op  );
    TFilterButterworthHighPass&     operator    =               ( const TFilterButterworthHighPass& op2 );


protected:
    double          SamplingFrequency;
    double          FrequencyCut;
    int             Order;
    FilterCausality Causal;

                                        // using fixed width arrays: faster and less memory footprint than a TArray1
    double          A  [ TFilterMaxOrder / 2 ];    // gains
    double          d1 [ TFilterMaxOrder / 2 ];    // Low & High pass
    double          d2 [ TFilterMaxOrder / 2 ];
};


//----------------------------------------------------------------------------
template <class TypeD>
class   TFilterButterworthLowPass   : public TFilter<TypeD>
{
public:
                    TFilterButterworthLowPass ();
                    TFilterButterworthLowPass ( int order, FilterCausality causal, double fsamp, double freq );


    void            Reset   ();
    void            Set     ( int order, FilterCausality causal, double fsamp, double freq );

    double          GetFrequencyCut         ()  const   { return  FrequencyCut;  }

    void            Apply                   ( TypeD* data, int numpts );


                                TFilterButterworthLowPass       ( const TFilterButterworthLowPass& op  );
    TFilterButterworthLowPass&      operator    =               ( const TFilterButterworthLowPass& op2 );


protected:
    double          SamplingFrequency;
    double          FrequencyCut;
    int             Order;
    FilterCausality Causal;

                                        // using fixed width arrays: faster and less memory footprint than a TArray1
    double          A  [ TFilterMaxOrder / 2 ];    // gains
    double          d1 [ TFilterMaxOrder / 2 ];    // Low & High pass
    double          d2 [ TFilterMaxOrder / 2 ];
};


//----------------------------------------------------------------------------
template <class TypeD>
class   TFilterButterworthBandPass  : public TFilter<TypeD>
{
public:
                    TFilterButterworthBandPass ();
                    TFilterButterworthBandPass ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 );


    void            Reset   ();
    void            Set     ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 );

    double          GetFrequencyCutLow      ()  const   { return  FrequencyCutLow;  }   // lowest  frequency, NOT Low  Pass cutting frequency
    double          GetFrequencyCutHigh     ()  const   { return  FrequencyCutHigh; }   // highest frequency, NOT High Pass cutting frequency
    double          GetFrequencyCutCenter   ()  const   { return  ( FrequencyCutLow + FrequencyCutHigh ) / 2; }
    double          GetFrequencyCutWidth    ()  const   { return  fabs ( FrequencyCutHigh - FrequencyCutLow ); }

    void            Apply                   ( TypeD* data, int numpts );


                                    TFilterButterworthBandPass  ( const TFilterButterworthBandPass& op  );
    TFilterButterworthBandPass&     operator    =               ( const TFilterButterworthBandPass& op2 );


protected:
    double          SamplingFrequency;
    double          FrequencyCutLow;
    double          FrequencyCutHigh;
    int             Order;
    FilterCausality Causal;

                                        // using fixed width arrays: faster and less memory footprint than a TArray1
    double          A  [ TFilterMaxTwiceOrder / 4 ];    // gains
    double          d1 [ TFilterMaxTwiceOrder / 4 ];  
    double          d2 [ TFilterMaxTwiceOrder / 4 ];
    double          d3 [ TFilterMaxTwiceOrder / 4 ];  
    double          d4 [ TFilterMaxTwiceOrder / 4 ];
};


//----------------------------------------------------------------------------
template <class TypeD>
class   TFilterButterworthBandStop  : public TFilter<TypeD>
{
public:
                    TFilterButterworthBandStop ();
                    TFilterButterworthBandStop ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 );


    void            Reset   ();
    void            Set     ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 );

    double          GetFrequencyCutLow      ()  const   { return  FrequencyCutLow;  }   // lowest  frequency, NOT Low  Pass cutting frequency
    double          GetFrequencyCutHigh     ()  const   { return  FrequencyCutHigh; }   // highest frequency, NOT High Pass cutting frequency
    double          GetFrequencyCutCenter   ()  const   { return  ( FrequencyCutLow + FrequencyCutHigh ) / 2; }
    double          GetFrequencyCutWidth    ()  const   { return  fabs ( FrequencyCutHigh - FrequencyCutLow ); }

    void            Apply                   ( TypeD* data, int numpts );


                                    TFilterButterworthBandStop  ( const TFilterButterworthBandStop& op  );
    TFilterButterworthBandStop&     operator    =               ( const TFilterButterworthBandStop& op2 );


protected:
    double          SamplingFrequency;
    double          FrequencyCutLow;
    double          FrequencyCutHigh;
    int             Order;
    FilterCausality Causal;

                                        // using fixed width arrays: faster and less memory footprint than a TArray1
    double          A  [ TFilterMaxTwiceOrder / 4 ];    // gains
    double          d1 [ TFilterMaxTwiceOrder / 4 ];    
    double          d2 [ TFilterMaxTwiceOrder / 4 ];
    double          d3 [ TFilterMaxTwiceOrder / 4 ];    
    double          d4 [ TFilterMaxTwiceOrder / 4 ];
    double          ww13;                               
    double          ww2;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

//  Reference: "Recursive Digital Filters - A concise guide", Hollos S., Hollos J.R., Abrazol Publishing, 2014
//  And also : http://www.exstrom.com/journal/sigproc/index.html

//  Pamphlet:
//  =========
// 2nd or higher order Butterworth (maximally flat) filter, implemented digitally with bilinear transform.
// Data is passed through twice (forward and backward) by default giving zero phase shift, though single forward pass (causal filter) is possible, too.
// Rolloff at low frequencies of -6*order dB/octave (or -20*order dB/decade), and becoming infinitely steep at the nyquist frequency.
// Filter is implemented with (order/2) cascaded 2nd-order sections (note: hence the need for even orders).
// In case of two passes, frequency cut is adjusted such as keeping the -3dB in the final results. Otherwise the frequency cut will appear to be shifted due to double filtering.

// Note: For Band-pass and Band-stop, rolloff is twice the order from the user value, so -6*2*order dB/octave (or -20*2*order dB/decade). The verbose shows the exact order being used, though.
// Note: The two passes will result in the final rolloff to be slightly higher then advertized, due to the double filtering. The exact slope was not estimated, though...
// Note: -20dB/decade = -6dB/octave; rolloff = order * -6dB/octave  or  order * -20dB/decade
// Note: Cutoff frequency at -3dB corresponds to half power (10 Log ( 0.5 ) = -3), or 0.707 amplitude (20 Log ( 0.707 ) = -3)


template <class TypeD>
        TFilterButterworthHighPass<TypeD>::TFilterButterworthHighPass ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterButterworthHighPass<TypeD>::TFilterButterworthHighPass ( int order, FilterCausality causal, double fsamp, double freq )
{
Set ( order, causal, fsamp, freq );
}


template <class TypeD>
void    TFilterButterworthHighPass<TypeD>::Reset ()
{
SamplingFrequency   = 0;
FrequencyCut        = 0;
Order               = 0;
Causal              = FilterNonCausal;

                                        // !sequence & number of members!
ClearVirtualMemory  ( A,  3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );
}


template <class TypeD>
            TFilterButterworthHighPass<TypeD>::TFilterButterworthHighPass ( const TFilterButterworthHighPass& op )
{
SamplingFrequency   = op.SamplingFrequency;
FrequencyCut        = op.FrequencyCut;
Order               = op.Order;
Causal              = op.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op.A,   3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );
}


template <class TypeD>
TFilterButterworthHighPass<TypeD>& TFilterButterworthHighPass<TypeD>::operator= ( const TFilterButterworthHighPass& op2 )
{
if ( &op2 == this )
    return  *this;


SamplingFrequency   = op2.SamplingFrequency;
FrequencyCut        = op2.FrequencyCut;
Order               = op2.Order;
Causal              = op2.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op2.A,  3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );


return  *this;
}


template <class TypeD>
void    TFilterButterworthHighPass<TypeD>::Set ( int order, FilterCausality causal, double fsamp, double freq )
{
Reset ();


SamplingFrequency   = fabs ( fsamp );

FrequencyCut        = Clip ( fabs ( freq ), 0.0, SamplingFrequency / 2 );
                                        // !order must be even!
Order               = Clip ( RoundToEven ( order ), ButterworthHighPassMinOrder, ButterworthHighPassMaxOrder );

Causal              = causal;

//DBGV2 ( FrequencyCut, Order, "TFilterButterworthHighPass::Set: FrequencyCut, Order" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cascading 2nd order filter elements
int                 order2          = Order / 2;
                                        // first calculate analog frequency corresponding to the desired digital filter cutoff ("prewarp").
double              omega           = tan ( Pi * FrequencyCut / SamplingFrequency );
                                        // Because applying twice  the filter, forward and backward, will shift the frequency response "leftward", we can compensate this by re-shifting it
                                        // adjust omega so fcut corresponds to -1.5dB, shifting response "left", otherwise frequencies past cut will be too attenuated
if ( Causal == FilterNonCausal )
                    omega          /= PowerRoot ( 3, 2.5 * Order );

double              omega2          = Square ( omega );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing coefficients - identical for Low and High Pass
double              rk;
double              Dn;

for ( int k = 0; k < order2; k++ ) {
    rk          = sin ( Pi * ( 2 * k + 1 ) / ( 2 * Order ) );
    Dn          =     1 + 2 * rk * omega +     omega2;
    A [ k ]     =     1                                 / Dn;
    d1[ k ]     =   ( 2                  - 2 * omega2 ) / Dn;
    d2[ k ]     = - ( 1 - 2 * rk * omega +     omega2 ) / Dn;
    }
}


template <class TypeD>
void    TFilterButterworthHighPass<TypeD>::Apply ( TypeD* data, int numpts )
{
                                        // cascading 2nd order filter elements
int                 order2          = Order / 2;
double              d;
                                        // on the stack for OpenMP
double              w0 [ TFilterMaxOrder / 2 ];
double              w1 [ TFilterMaxOrder / 2 ];
double              w2 [ TFilterMaxOrder / 2 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init W's to 0
auto        ResetWs     = [ & ] ( int order2 )      { for ( int k = 0; k < order2; k++ )  w2[ k ] = w1[ k ] = 0; };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply filter backward - optional in case one doesn't want data "from the future"
if ( Causal == FilterNonCausal ) {

    ResetWs ( order2 );

    for ( int ti = numpts - 1; ti >= 0; ti-- ) {
        d   = data[ ti ];

        for ( int k = 0; k < order2; k++ ) {

            w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d;
            d           = A[ k ] * ( w0[ k ] - 2 * w1[ k ] + w2[ k ] );
            w2  [ k ]   = w1[ k ];
            w1  [ k ]   = w0[ k ];
            }

        data[ ti ]  = d;
        }
    } // ! Causal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ResetWs ( order2 );

for ( int ti = 0; ti < numpts; ti++ ) {
    d   = data[ ti ];

    for ( int k = 0; k < order2; k++ ) {

        w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d;
        d           = A[ k ] * ( w0[ k ] - 2 * w1[ k ] + w2[ k ] );
        w2  [ k ]   = w1[ k ];
        w1  [ k ]   = w0[ k ];
        }

    data[ ti ]  = d;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
        TFilterButterworthLowPass<TypeD>::TFilterButterworthLowPass ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterButterworthLowPass<TypeD>::TFilterButterworthLowPass ( int order, FilterCausality causal, double fsamp, double freq )
{
Set ( order, causal, fsamp, freq );
}


template <class TypeD>
void    TFilterButterworthLowPass<TypeD>::Reset ()
{
SamplingFrequency   = 0;
FrequencyCut        = 0;
Order               = 0;
Causal              = FilterNonCausal;

                                        // !sequence & number of members!
ClearVirtualMemory  ( A,  3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );
}


template <class TypeD>
            TFilterButterworthLowPass<TypeD>::TFilterButterworthLowPass ( const TFilterButterworthLowPass& op )
{
SamplingFrequency   = op.SamplingFrequency;
FrequencyCut        = op.FrequencyCut;
Order               = op.Order;
Causal              = op.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op.A,   3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );
}


template <class TypeD>
TFilterButterworthLowPass<TypeD>& TFilterButterworthLowPass<TypeD>::operator= ( const TFilterButterworthLowPass& op2 )
{
if ( &op2 == this )
    return  *this;


SamplingFrequency   = op2.SamplingFrequency;
FrequencyCut        = op2.FrequencyCut;
Order               = op2.Order;
Causal              = op2.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op2.A,  3 * ( TFilterMaxOrder      / 2 ) * sizeof ( *A  ) );


return  *this;
}


template <class TypeD>
void    TFilterButterworthLowPass<TypeD>::Set ( int order, FilterCausality causal, double fsamp, double freq )
{
Reset ();


SamplingFrequency   = fabs ( fsamp );

FrequencyCut        = Clip ( fabs ( freq ), 0.0, SamplingFrequency / 2 );
                                        // !order must be even!
Order               = Clip ( RoundToEven ( order ), ButterworthLowPassMinOrder, ButterworthLowPassMaxOrder );

Causal              = causal;

//DBGV2 ( FrequencyCut, Order, "TFilterButterworthLowPass::Set: FrequencyCut, Order" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cascading 2nd order filter elements
int                 order2          = Order / 2;
                                        // first calculate analog frequency corresponding to the desired digital filter cutoff ("prewarp").
double              omega           = tan ( Pi * FrequencyCut / SamplingFrequency );
                                        // Because applying twice  the filter, forward and backward, will shift the frequency response "leftward", we can compensate this by re-shifting it
                                        // adjust omega so fcut corresponds to -1.5dB, shifting response "right", otherwise frequencies before cut will be too attenuated
if ( Causal == FilterNonCausal )
                    omega          *= PowerRoot ( 3, 2.5 * Order );

double              omega2          = Square ( omega );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing coefficients - identical for Low and High Pass
double              rk;
double              Dn;

for ( int k = 0; k < order2; k++ ) {
    rk          = sin ( Pi * ( 2 * k + 1 ) / ( 2 * Order ) );
    Dn          =     1 + 2 * rk * omega +     omega2;
    A [ k ]     =                              omega2   / Dn;
    d1[ k ]     =   ( 2                  - 2 * omega2 ) / Dn;
    d2[ k ]     = - ( 1 - 2 * rk * omega +     omega2 ) / Dn;
    }
}


template <class TypeD>
void    TFilterButterworthLowPass<TypeD>::Apply ( TypeD* data, int numpts )
{
                                        // cascading 2nd order filter elements
int                 order2          = Order / 2;
double              d;
                                        // on the stack for OpenMP
double              w0 [ TFilterMaxOrder / 2 ];
double              w1 [ TFilterMaxOrder / 2 ];
double              w2 [ TFilterMaxOrder / 2 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init W's to 0
auto        ResetWs     = [ & ] ( int order2 )      { for ( int k = 0; k < order2; k++ )  w2[ k ] = w1[ k ] = 0; };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply filter backward - optional in case one doesn't want data "from the future"
if ( Causal == FilterNonCausal ) {

    ResetWs ( order2 );

    for ( int ti = numpts - 1; ti >= 0; ti-- ) {
        d   = data[ ti ];

        for ( int k = 0; k < order2; k++ ) {

            w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d;
            d           = A[ k ] * ( w0[ k ] + 2 * w1[ k ] + w2[ k ] );
            w2  [ k ]   = w1[ k ];
            w1  [ k ]   = w0[ k ];
            }

        data[ ti ]  = d;
        }
    } // ! Causal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ResetWs ( order2 );

for ( int ti = 0; ti < numpts; ti++ ) {
    d   = data[ ti ];

    for ( int k = 0; k < order2; k++ ) {

        w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d;
        d           = A[ k ] * ( w0[ k ] + 2 * w1[ k ] + w2[ k ] );
        w2  [ k ]   = w1[ k ];
        w1  [ k ]   = w0[ k ];
        }

    data[ ti ]  = d;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
        TFilterButterworthBandPass<TypeD>::TFilterButterworthBandPass ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterButterworthBandPass<TypeD>::TFilterButterworthBandPass ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 )
{
Set ( order, causal, fsamp, freq1, freq2 );
}


template <class TypeD>
void    TFilterButterworthBandPass<TypeD>::Reset ()
{
SamplingFrequency   = 0;
FrequencyCutLow     = 0;
FrequencyCutHigh    = 0;
Order               = 0;
Causal              = FilterNonCausal;

                                        // !sequence & number of members!
ClearVirtualMemory  ( A, 5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );
}


template <class TypeD>
            TFilterButterworthBandPass<TypeD>::TFilterButterworthBandPass ( const TFilterButterworthBandPass& op )
{
SamplingFrequency   = op.SamplingFrequency;
FrequencyCutLow     = op.FrequencyCutLow;
FrequencyCutHigh    = op.FrequencyCutHigh;
Order               = op.Order;
Causal              = op.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op.A,  5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );
}


template <class TypeD>
TFilterButterworthBandPass<TypeD>& TFilterButterworthBandPass<TypeD>::operator= ( const TFilterButterworthBandPass& op2 )
{
if ( &op2 == this )
    return  *this;


SamplingFrequency   = op2.SamplingFrequency;
FrequencyCutLow     = op2.FrequencyCutLow;
FrequencyCutHigh    = op2.FrequencyCutHigh;
Order               = op2.Order;
Causal              = op2.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op2.A, 5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );


return  *this;
}


template <class TypeD>
void    TFilterButterworthBandPass<TypeD>::Set ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 )
{
Reset ();


SamplingFrequency   = fabs ( fsamp );


FrequencyCutLow     = fabs ( freq1 );
FrequencyCutHigh    = fabs ( freq2 );

Clipped     ( FrequencyCutLow,  0.0, SamplingFrequency / 2 );
Clipped     ( FrequencyCutHigh, 0.0, SamplingFrequency / 2 );
CheckOrder  ( FrequencyCutLow, FrequencyCutHigh );
                                        // !order must be multiples of 4 - cumulating orders of low and high passes together!
                                        // For consistency, we allow 2 times TFilterMaxOrder, although it starts to have some ringing
Order               = Clip ( (int) RoundTo ( order, 4 ), ButterworthBandPassMinOrder, ButterworthBandPassMaxOrder );

Causal              = causal;

//DBGV3 ( FrequencyCutLow, FrequencyCutHigh, Order, "TFilterButterworthBandPass::Set: FrequencyCutLow, FrequencyCutHigh, Order" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cascading 2nd order filter elements, twice
int                 order4          = Order / 4;
                                        // center of pass band
double              center          = cos ( Pi * ( FrequencyCutHigh + FrequencyCutLow ) / SamplingFrequency ) 
                                    / cos ( Pi * ( FrequencyCutHigh - FrequencyCutLow ) / SamplingFrequency );

double              center2         = Square ( center );
                                        // width of pass band
double              width           = tan ( Pi * ( FrequencyCutHigh - FrequencyCutLow ) / SamplingFrequency );
                                        // Because applying twice  the filter, forward and backward, will make the band-pass narrower, we can compensate this by expanding it
if ( Causal == FilterNonCausal )
                    width          *= PowerRoot ( 3, 1.25 * Order );

double              width2          = Square ( width );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing coefficients
double              rk;
double              Dn;

for ( int k = 0; k < order4; k++ ) {
    rk          = sin ( Pi * ( 2 * k + 1 ) / (double) Order );
    Dn          =                  1 + 2 * rk * width  + width2;
    A [ k ]     =                                        width2   / Dn;
    d1[ k ]     =  4 * center * (  1 +     rk * width           ) / Dn;
    d2[ k ]     =  2 *          ( -1 - 2 * center2     + width2 ) / Dn;
    d3[ k ]     =  4 * center * (  1 -     rk * width           ) / Dn;
    d4[ k ]     =             - (  1 - 2 * rk * width  + width2 ) / Dn;
    }
}


template <class TypeD>
void    TFilterButterworthBandPass<TypeD>::Apply ( TypeD* data, int numpts )
{
                                        // cascading 2nd order filter elements, twice
int                 order4          = Order / 4;
double              d;
                                        // on the stack for OpenMP
double              w0 [ TFilterMaxTwiceOrder / 4 ];
double              w1 [ TFilterMaxTwiceOrder / 4 ];
double              w2 [ TFilterMaxTwiceOrder / 4 ];
double              w3 [ TFilterMaxTwiceOrder / 4 ];
double              w4 [ TFilterMaxTwiceOrder / 4 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init W's to 0
auto        ResetWs     = [ & ] ( int order4 )      { for ( int k = 0; k < order4; k++ )  w4[ k ] = w3[ k ] = w2[ k ] = w1[ k ] = 0; };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply filter backward - optional in case one doesn't want data "from the future"
if ( Causal == FilterNonCausal ) {

    ResetWs ( order4 );

    for ( int ti = numpts - 1; ti >= 0; ti-- ) {
        d   = data[ ti ];

        for ( int k = 0; k < order4; k++ ) {

            w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d3[ k ] * w3[ k ]+ d4[ k ] * w4[ k ] + d;
            d           = A[ k ] * ( w0[ k ] - 2 * w2[ k ] + w4[ k ] );
            w4  [ k ]   = w3[ k ];
            w3  [ k ]   = w2[ k ];
            w2  [ k ]   = w1[ k ];
            w1  [ k ]   = w0[ k ];
            }

        data[ ti ]  = d;
        }
    } // ! Causal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ResetWs ( order4 );

for ( int ti = 0; ti < numpts; ti++ ) {
    d   = data[ ti ];

    for ( int k = 0; k < order4; k++ ) {

        w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d3[ k ] * w3[ k ]+ d4[ k ] * w4[ k ] + d;
        d           = A[ k ] * ( w0[ k ] - 2 * w2[ k ] + w4[ k ] );
        w4  [ k ]   = w3[ k ];
        w3  [ k ]   = w2[ k ];
        w2  [ k ]   = w1[ k ];
        w1  [ k ]   = w0[ k ];
        }

    data[ ti ]  = d;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
        TFilterButterworthBandStop<TypeD>::TFilterButterworthBandStop ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterButterworthBandStop<TypeD>::TFilterButterworthBandStop ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 )
{
Set ( order, causal, fsamp, freq1, freq2 );
}


template <class TypeD>
void    TFilterButterworthBandStop<TypeD>::Reset ()
{
SamplingFrequency   = 0;
FrequencyCutLow     = 0;
FrequencyCutHigh    = 0;
Order               = 0;
Causal              = FilterNonCausal;

                                        // !sequence & number of members!
ClearVirtualMemory  ( A, 5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );
ww13                = 0;
ww2                 = 0;
}


template <class TypeD>
            TFilterButterworthBandStop<TypeD>::TFilterButterworthBandStop ( const TFilterButterworthBandStop& op )
{
SamplingFrequency   = op.SamplingFrequency;
FrequencyCutLow     = op.FrequencyCutLow;
FrequencyCutHigh    = op.FrequencyCutHigh;
Order               = op.Order;
Causal              = op.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op.A,  5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );
ww13                = op.ww13;
ww2                 = op.ww2;
}


template <class TypeD>
TFilterButterworthBandStop<TypeD>& TFilterButterworthBandStop<TypeD>::operator= ( const TFilterButterworthBandStop& op2 )
{
if ( &op2 == this )
    return  *this;


SamplingFrequency   = op2.SamplingFrequency;
FrequencyCutLow     = op2.FrequencyCutLow;
FrequencyCutHigh    = op2.FrequencyCutHigh;
Order               = op2.Order;
Causal              = op2.Causal;

                                        // !sequence & number of members!
CopyVirtualMemory   ( A,    op2.A, 5 * ( TFilterMaxTwiceOrder / 4 ) * sizeof ( *A  ) );
ww13                = op2.ww13;
ww2                 = op2.ww2;


return  *this;
}


template <class TypeD>
void    TFilterButterworthBandStop<TypeD>::Set ( int order, FilterCausality causal, double fsamp, double freq1, double freq2 )
{
Reset ();


SamplingFrequency   = fabs ( fsamp );


FrequencyCutLow     = fabs ( freq1 );
FrequencyCutHigh    = fabs ( freq2 );

Clipped     ( FrequencyCutLow,  0.0, SamplingFrequency / 2 );
Clipped     ( FrequencyCutHigh, 0.0, SamplingFrequency / 2 );
CheckOrder  ( FrequencyCutLow, FrequencyCutHigh );
                                        // !order must be multiples of 4 - cumulating orders of low and high passes together!
                                        // For consistency, we allow 2 times TFilterMaxOrder, although it starts to have some ringing
Order               = Clip ( (int) RoundTo ( order, 4 ), ButterworthBandStopMinOrder, ButterworthBandStopMaxOrder );

Causal              = causal;

//DBGV3 ( FrequencyCutLow, FrequencyCutHigh, Order, "TFilterButterworthBandStop::Set: FrequencyCutLow, FrequencyCutHigh, Order" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cascading 2nd order filter elements, twice
int                 order4          = Order / 4;
                                        // center of stop band
double              center          = cos ( Pi * ( FrequencyCutHigh + FrequencyCutLow ) / SamplingFrequency ) 
                                    / cos ( Pi * ( FrequencyCutHigh - FrequencyCutLow ) / SamplingFrequency );

double              center2         = Square ( center );
                                        // width of pass band
double              width           = tan ( Pi * ( FrequencyCutHigh - FrequencyCutLow ) / SamplingFrequency );
                                        // Because applying twice  the filter, forward and backward, will make the band-pass narrower, we can compensate this by expanding it
if ( Causal == FilterNonCausal )
                    width          /= PowerRoot ( 3, 1.25 * Order );

double              width2          = Square ( width );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing coefficients
double              rk;
double              Dn;

for ( int k = 0; k < order4; k++ ) {
    rk          = sin ( Pi * ( 2 * k + 1 ) / (double) Order );
    Dn          =                  1 + 2 * rk * width  + width2;
    A [ k ]     =                                        1        / Dn;
    d1[ k ]     =  4 * center * (  1 +     rk * width           ) / Dn;
    d2[ k ]     =  2 *          ( -1 - 2 * center2     + width2 ) / Dn;
    d3[ k ]     =  4 * center * (  1 -     rk * width           ) / Dn;
    d4[ k ]     =             - (  1 - 2 * rk * width  + width2 ) / Dn;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ww13    =     4 * center;
ww2     = 2 + 4 * center2;
}


template <class TypeD>
void    TFilterButterworthBandStop<TypeD>::Apply ( TypeD* data, int numpts )
{
                                        // cascading 2nd order filter elements, twice
int                 order4          = Order / 4;
double              d;
                                        // on the stack for OpenMP
double              w0 [ TFilterMaxTwiceOrder / 4 ];
double              w1 [ TFilterMaxTwiceOrder / 4 ];
double              w2 [ TFilterMaxTwiceOrder / 4 ];
double              w3 [ TFilterMaxTwiceOrder / 4 ];
double              w4 [ TFilterMaxTwiceOrder / 4 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init W's to 0
auto        ResetWs     = [ & ] ( int order4 )      { for ( int k = 0; k < order4; k++ )  w4[ k ] = w3[ k ] = w2[ k ] = w1[ k ] = 0; };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply filter backward - optional in case one doesn't want data "from the future"
if ( Causal == FilterNonCausal ) {

    ResetWs ( order4 );

    for ( int ti = numpts - 1; ti >= 0; ti-- ) {
        d   = data[ ti ];

        for ( int k = 0; k < order4; k++ ) {

            w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d3[ k ] * w3[ k ]+ d4[ k ] * w4[ k ] + d;
            d           = A[ k ] * ( w0[ k ] - ww13 * w1[ k ] + ww2 * w2[ k ] - ww13 * w3[ k ] + w4[ k ] );
            w4  [ k ]   = w3[ k ];
            w3  [ k ]   = w2[ k ];
            w2  [ k ]   = w1[ k ];
            w1  [ k ]   = w0[ k ];
            }

        data[ ti ]  = d;
        }
    } // ! Causal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ResetWs ( order4 );

for ( int ti = 0; ti < numpts; ti++ ) {
    d   = data[ ti ];

    for ( int k = 0; k < order4; k++ ) {

        w0  [ k ]   = d1[ k ] * w1[ k ] + d2[ k ] * w2[ k ] + d3[ k ] * w3[ k ]+ d4[ k ] * w4[ k ] + d;
        d           = A[ k ] * ( w0[ k ] - ww13 * w1[ k ] + ww2 * w2[ k ] - ww13 * w3[ k ] + w4[ k ] );
        w4  [ k ]   = w3[ k ];
        w3  [ k ]   = w2[ k ];
        w2  [ k ]   = w1[ k ];
        w1  [ k ]   = w0[ k ];
        }

    data[ ti ]  = d;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
