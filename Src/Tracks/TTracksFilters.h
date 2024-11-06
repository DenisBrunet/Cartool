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


#include    <stdio.h>

#include    "TArray2.h"
#include    "TArray3.h"
#include    "TFilters.h"
#include    "Strings.TSplitStrings.h"

#include    "TTracksFiltersDialog.h"    // TTracksFiltersStruct

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                SpatialFilterType;


//----------------------------------------------------------------------------
                                        // absolute min frequency allowed in Cartool (doubt it makes any sense)
constexpr double    EegFilterMinFrequency           = 0.001;
                                        // default high pass value to suggest in dialogs
constexpr double    EegFilterDefaultMinFrequency    = 0.1;

                                        
constexpr int       EegMaxFilterSide                = ( 10 * 5000 ) / 2;    // margin needed for the lowest freq of 0.1Hz, with max sampling rate of 5000Hz
constexpr int       EegMinLowFilterSide             = 25;


constexpr int       MaxNotches                      = 32;

                                        // It could also be possible to define something like FiltersTimely / FiltersUntimely
enum                FilterPrecedence
                    {
                    UnknownPrecedence   = 0x00,
                                        // Breaking-up filters into Temporal and Non-Temporal ones: convenient for buffer allocation or extra temporal margin, and fine-tuning calls
                    FilterTemporal      = 0x01,
                    FilterNonTemporal   = 0x02,

                    AllFilters          = FilterTemporal | FilterNonTemporal,
                    };


constexpr FilterTypes   TracksFiltersEnvelope   = FilterTypeEnvelopeGapBridging;


//----------------------------------------------------------------------------
                                        // Encapsulates the set of filters available to user, and how to call them in sequence
template <class TypeD>
class   TTracksFilters
{
public:
                    TTracksFilters ();


    TTracksFiltersStruct    FiltersParam;       // Glue to TTracksFiltersDialog - Stores the requested filters


    bool            Baseline;

    double          ButterworthHigh;
    double          ButterworthHighRequested;
    int             OrderHigh;
    double          ButterworthLow;             // applied values, after clipping
    double          ButterworthLowRequested;    // asked by the user
    int             OrderLow;
    FilterCausality Causal;

    
    int             NumNotches;
    double          Notches[ MaxNotches ];      // requested by user
    bool            NotchesAutoHarmonics;

    bool                SpatialFiltering;       // It would be nice to have XyzFile here, too, while also specifying the filter type in FiltersParam
    SpatialFilterType   SpatialFilter;

    bool            Ranking;

    ReferenceType   Reference;                  // Default reference, but can be overridden by ApplyFilter to the caller own responsibility
    TSelection      ReferenceTracks;
    
    int             Rectification;
    double          EnvelopeWidth;              // in [ms]

    bool            ThresholdKeepAbove;
    double          ThresholdKeepAboveValue;
    bool            ThresholdKeepBelow;
    double          ThresholdKeepBelowValue;

                                        // Local
    double          SamplingFrequency;
    double          FreqMin;                    // current min and max frequencies
    double          FreqMax;                    // allowed for filters



    void            SetFromStruct       ( double fsamp = 0, bool silent = true );   // calls SetFilters     - converts a struct to variables
    void            SetFilters          ( double fsamp,     bool silent = true );   // init each relevant TFilterXXX objects; calls SetSamplingFrequency & check frequency ranges correctness
    void            SetSamplingFrequency( double fsamp );                           // tries to update its SamplingFrequency
    int             GetSafeMargin       ()  const;                                  // returns the necessary margin for filtering
    int             GetOrder            ()  const;
    int             GetSpatialFilterDim ()  const       { return FilterSpatial.IsAllocated () ? FilterSpatial.GetNumPoints () : 0; }


    char*           ParametersToText    (       char* text )                const;
    void            TextToParameters    ( const char* text, const char* xyzfile, double fsamp );

                                                // Tests if all conditions are met for any given filter - f.ex. Butterworth can not run if no sampling frequency are provided
    bool            HasSamplingFrequency    ()  const   { return SamplingFrequency > 0;                             }
    bool            HasBaseline             ()  const   { return Baseline;                                          }
    bool            HasButterworthHigh      ()  const   { return ButterworthHigh > 0 && HasSamplingFrequency ();    }
    bool            HasButterworthLow       ()  const   { return ButterworthLow  > 0 && HasSamplingFrequency ();    }
    bool            HasButterworthBand      ()  const   { return HasButterworthHigh () && HasButterworthLow ();     }   // combining high and low pass into a band-pass
    bool            HasButterworthFilter    ()  const   { return HasButterworthHigh () || HasButterworthLow ();     }
    bool            HasNotches              ()  const   { return NumNotches      > 0 && HasSamplingFrequency ();    }
    bool            HasEnvelope             ()  const   { return EnvelopeWidth   > 0 && HasSamplingFrequency ();    }
    bool            HasTemporalFilter       ()  const   { return HasBaseline () || HasButterworthHigh () || HasButterworthLow () || HasNotches () || HasEnvelope (); }

    bool            HasSpatialFilter        ()  const   { return SpatialFiltering && SpatialFilter != SpatialFilterNone; }
    bool            HasRanking              ()  const   { return Ranking;                                           }
    bool            HasReference            ( ReferenceType ref = ReferenceUsingCurrent )   const   { return IsEffectiveReference ( ref == ReferenceUsingCurrent ? Reference : ref );   }   // default is to test local Reference, but it can be overridden with any parameter
    bool            HasRectification        ()  const   { return Rectification != 0;                                }
    bool            HasThresholdKeepAbove   ()  const   { return ThresholdKeepAbove;                                }
    bool            HasThresholdKeepBelow   ()  const   { return ThresholdKeepBelow;                                }
                                                                                                        // HasReference not included for the moment
    bool            HasNonTemporalFilter    ()  const   { return HasSpatialFilter () || HasRanking () /*|| HasReference ()*/ || HasRectification () || HasThresholdKeepAbove () || HasThresholdKeepBelow (); }

    bool            HasAnyFilter            ()  const   { return HasTemporalFilter () || HasNonTemporalFilter ();   }
    bool            HasNoFilters            ()  const   { return ! HasAnyFilter ();                                 }

                                        // To apply the reference, FilterNonTemporal flag should be set
    void            ApplyFilters            ( TArray2<TypeD>&   data, int numel, int numpts, int tfoffset, ReferenceType reference, const TSelection* referencetracks, const TSelection* validtracks, const TSelection* auxtracks, FilterPrecedence filterprecedence = AllFilters );


                    TTracksFilters      ( const TTracksFilters &op  );
    TTracksFilters& operator    =       ( const TTracksFilters &op2 );


private:
                                        // All these private members need not to be copied - they are all internally allocated and handled

                                        // Temporal filters
    TFilterBaseline             <TypeD>     FilterBaseline;
    TFilterButterworthLowPass   <TypeD>     FilterButterworthLowPass;
    TFilterButterworthHighPass  <TypeD>     FilterButterworthHighPass;
    TFilterButterworthBandPass  <TypeD>     FilterButterworthBandPass;
    TFilters                    <TypeD>     FilterNotches;  // implemented as Butterworth Band-stop filters - !could be more than NumNotches if NotchesAutoHarmonics is being used!
    TFilterEnvelope             <TypeD>     FilterEnvelope;

                                        // Non-temporal (i.e. instantaneous or topographical) filters
    TFilterSpatial              <TypeD>     FilterSpatial;
    TFilterRanking              <TypeD>     FilterRanking;
    TFilterReference            <TypeD>     FilterReference;
    TFilterRectification        <TypeD>     FilterRectification;
    TFilterThreshold            <TypeD>     FilterThreshold;


    TArray3<double> NeighDist;                      // Neighborhood distances for Spatial Filtering
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TTracksFilters<TypeD>::TTracksFilters ()
{
Baseline                    = false;

ButterworthHigh             = 0;
ButterworthHighRequested    = 0;
OrderHigh                   = TFilterDefaultOrder;
ButterworthLow              = 0;
ButterworthLowRequested     = 0;
OrderLow                    = TFilterDefaultOrder;
Causal                      = FilterNonCausal;

NumNotches                  = 0;
for ( int n = 0; n < MaxNotches; n++ )
    Notches[ n ]            = 0;
NotchesAutoHarmonics        = false;

SpatialFiltering            = false;
SpatialFilter               = SpatialFilterDefault;

Ranking                     = false;

Reference                   = ReferenceAsInFile;

Rectification               = 0;
EnvelopeWidth               = 0;

ThresholdKeepAbove          = false;
ThresholdKeepAboveValue     = 0;
ThresholdKeepBelow          = false;
ThresholdKeepBelowValue     = 0;


SamplingFrequency           = 0;
FreqMin                     = EegFilterDefaultMinFrequency;
FreqMax                     = 0;
}


template <class TypeD>
        TTracksFilters<TypeD>::TTracksFilters ( const TTracksFilters &op )
{
Baseline                    = op.Baseline;

ButterworthHigh             = op.ButterworthHigh;
ButterworthHighRequested    = op.ButterworthHighRequested;
OrderHigh                   = op.OrderHigh;
ButterworthLow              = op.ButterworthLow          ;
ButterworthLowRequested     = op.ButterworthLowRequested;
OrderLow                    = op.OrderLow;
Causal                      = op.Causal;

NumNotches                  = op.NumNotches;
CopyVirtualMemory ( (void *) &Notches, (void *) &op.Notches, MaxNotches * sizeof ( *Notches ) );
NotchesAutoHarmonics        = op.NotchesAutoHarmonics;

SpatialFiltering            = op.SpatialFiltering;
SpatialFilter               = op.SpatialFilter;

Ranking                     = op.Ranking;

Reference                   = op.Reference;
ReferenceTracks             = op.ReferenceTracks;

Rectification               = op.Rectification;
EnvelopeWidth               = op.EnvelopeWidth;

ThresholdKeepAbove          = op.ThresholdKeepAbove;
ThresholdKeepAboveValue     = op.ThresholdKeepAboveValue;
ThresholdKeepBelow          = op.ThresholdKeepBelow;
ThresholdKeepBelowValue     = op.ThresholdKeepBelowValue;


SamplingFrequency           = op.SamplingFrequency;
FreqMin                     = op.FreqMin;
FreqMax                     = op.FreqMax;
                                        // these might not be needed, they will be overwritten at each SetFilters calls(?)
FilterBaseline              = op.FilterBaseline;
FilterButterworthLowPass    = op.FilterButterworthLowPass;
FilterButterworthHighPass   = op.FilterButterworthHighPass;
FilterButterworthBandPass   = op.FilterButterworthBandPass;
FilterNotches               = op.FilterNotches;
FilterEnvelope              = op.FilterEnvelope;
FilterSpatial               = op.FilterSpatial;
FilterRanking               = op.FilterRanking;
FilterRectification         = op.FilterRectification;
FilterThreshold             = op.FilterThreshold;

                                        // also copy the data from the dialog itself
CopyVirtualMemory ( (void *) &FiltersParam, (void *) &op.FiltersParam, sizeof ( FiltersParam ) );
}


template <class TypeD>
TTracksFilters<TypeD>&  TTracksFilters<TypeD>::operator= ( const TTracksFilters &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Baseline                    = op2.Baseline;

ButterworthHigh             = op2.ButterworthHigh;
ButterworthHighRequested    = op2.ButterworthHighRequested;
OrderHigh                   = op2.OrderHigh;
ButterworthLow              = op2.ButterworthLow          ;
ButterworthLowRequested     = op2.ButterworthLowRequested;
OrderLow                    = op2.OrderLow;
Causal                      = op2.Causal;

NumNotches                  = op2.NumNotches;
CopyVirtualMemory ( (void *) &Notches, (void *) &op2.Notches, MaxNotches * sizeof ( *Notches ) );
NotchesAutoHarmonics        = op2.NotchesAutoHarmonics;

SpatialFiltering            = op2.SpatialFiltering;
SpatialFilter               = op2.SpatialFilter;

Ranking                     = op2.Ranking;

Reference                   = op2.Reference;
ReferenceTracks             = op2.ReferenceTracks;

Rectification               = op2.Rectification;
EnvelopeWidth               = op2.EnvelopeWidth;

ThresholdKeepAbove          = op2.ThresholdKeepAbove;
ThresholdKeepAboveValue     = op2.ThresholdKeepAboveValue;
ThresholdKeepBelow          = op2.ThresholdKeepBelow;
ThresholdKeepBelowValue     = op2.ThresholdKeepBelowValue;


SamplingFrequency           = op2.SamplingFrequency;
FreqMin                     = op2.FreqMin;
FreqMax                     = op2.FreqMax;
                                        // these might not be needed, they will be overwritten at each SetFilters calls(?)
FilterBaseline              = op2.FilterBaseline;
FilterButterworthLowPass    = op2.FilterButterworthLowPass;
FilterButterworthHighPass   = op2.FilterButterworthHighPass;
FilterButterworthBandPass   = op2.FilterButterworthBandPass;
FilterNotches               = op2.FilterNotches;
FilterEnvelope              = op2.FilterEnvelope;
FilterSpatial               = op2.FilterSpatial;
FilterRanking               = op2.FilterRanking;
FilterRectification         = op2.FilterRectification;
FilterThreshold             = op2.FilterThreshold;

                                        // also copy the data from the dialog itself
CopyVirtualMemory ( (void *) &FiltersParam, (void *) &op2.FiltersParam, sizeof ( FiltersParam ) );

return  *this;
}


//----------------------------------------------------------------------------
                                        // Update SamplingFrequency as much as possible
template <class TypeD>
void    TTracksFilters<TypeD>::SetSamplingFrequency ( double fsamp )            
{
                                        // try to use the given frequency - allow updating for each call, though
if ( /*SamplingFrequency <= 0 &&*/ fsamp > 0 )
    SamplingFrequency = AtLeast ( (double) 0, fsamp );

                                        // one more chance: going into the dialog part
if ( SamplingFrequency <= 0 )
    SamplingFrequency = AtLeast ( (double) 0, StringToDouble ( FiltersParam.ValueSamplingFrequency ) );
}


//----------------------------------------------------------------------------
                                        // Returns the min margin necessary ON EACH SIDE of the buffer for the best results, according to the current filters in use
template <class TypeD>
int     TTracksFilters<TypeD>::GetSafeMargin ()   const
{
if ( ! ( HasTemporalFilter () && HasSamplingFrequency () ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !Note that we could take advantage of the Causal flag, and in this case return a right margin of 0!
int                 margin          = 0;
int                 marginhigh      = 0;
//int               marginbaseline  = 0;
int                 marginlow       = 0;
int                 marginnotches   = 0;
int                 marginenvelope  = 0;

                                        // Spread a full cycle on the lowest frequency on each side
if ( HasButterworthHigh () )

    marginhigh  = Round (   FrequencyToTimeFrame ( ButterworthHigh, SamplingFrequency ) 
                          * NoMore ( (const int) 10, OrderHigh )    // Higher orders need more margin, but we cap it up to 10 as there doesn't seem any real gain above, while computation is really getting worse
                          / 2                                       // Half cycle seems enough on each side - less starts to be noticeable
                        );

                                        // DC without High Pass is a tough choice: 
                                        // do the baseline on what is requested (no margin), or add the biggest possible margin?
                                        // currently, don't add any margin, the baseline is done on the requested data
//if ( HasBaseline () )
// 
//    marginbaseline  =   Round (   FrequencyToTimeFrame ( EegFilterMinFrequency, SamplingFrequency ) 
//                                / 2                                                               
//                              );


                                        // Spread a full cycle on the highest frequency on each side
if ( HasButterworthLow () )

    marginlow   = AtLeast ( EegMinLowFilterSide, Round (   FrequencyToTimeFrame ( ButterworthLow, SamplingFrequency ) 
                                                         * OrderLow     // Higher orders need more margin
                                                       ) 
                          );


                                        // Quite a generous margin for notch, as line artifacts can be strong and could mess with the current Butterworth Bandstop
if ( HasNotches () ) {

    int             mn;

    for ( int n = 0; n < NumNotches; n++ ) {  // using fundamental frequency is enough

        mn  = Round (   FrequencyToTimeFrame ( Notches[ n ] - TFilterNotchWidth / 2, SamplingFrequency ) 
                      * TFilterNotchOrder
                      * NoMore ( 6, FilterNotches.GetNumFilters () )    // we can save some computation when no harmonics are being requested
                    );

        Maxed ( marginnotches, mn );
        }
    }

                                        // Most Envelope filters will need some margin
if ( HasEnvelope () )

    marginenvelope  = (int) FilterEnvelope.GetEnvelopeWidthTF ( OddSize );
//  marginenvelope  = (int) FilterEnvelope.GetEnvelopeWidthTF ( TracksFiltersEnvelope == FilterTypeEnvelopeAnalytic ? EvenSize : OddSize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compose all margins together, taking the biggest margin of all filters
margin      = max ( marginhigh, marginlow, marginnotches, marginenvelope /*, marginbaseline*/ );

                                        //   There is a subtlety here:
                                        // Spatial Filtering / ThresholdAbove / ThresholdBelow don't need any margin for themselves,
                                        // but to allow GetTracks to compute a correct Dissimilarity on the first retrieved TF,
                                        // we need 1 more TF at the beginning, so that the difference between TF and TF-1 is correct
                                        // Update: this is not the purpose of this function, the caller has to care for 0 margin and upgrade to 1 if the Dissimilarity is to be computed
//Maxed ( margin, 1 );

                                        // By safety, also set an absolute max limit...
                                        // At that point, the frequencies should have been already checked, and this should be unnecessary
Mined  ( margin, EegMaxFilterSide );


return  margin;
}


//----------------------------------------------------------------------------
template <class TypeD>
int     TTracksFilters<TypeD>::GetOrder ()  const       
{
return  HasButterworthBand  () ? 2 * max ( OrderHigh, OrderLow )            // Order is multiple of 4, done by DOUBLING the single high/low filter order. Final filter has a steeper rolloff. This is similar to Matlab f.ex.
//      HasButterworthBand  () ? RoundTo ( max ( OrderHigh, OrderLow ), 4 ) // Order is multiple of 4, ROUNDED to closest value from single high/low filter order. This means final filter has a more comparable rolloff as the individual filter. It however adds some confusion, or requires all filters orders to be also multiple of 4.
      : HasButterworthHigh  () ? OrderHigh                                  // could handle different orders for high and low filters - though these are currently the same
      : HasButterworthLow   () ? OrderLow 
      :                          0; 
}    


//----------------------------------------------------------------------------
                                        // Checking frequencies coherence & setting up filter objects to be ready for operation
template <class TypeD>
void    TTracksFilters<TypeD>::SetFilters ( double fsamp, bool silent )
{
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Non temporal / frequency filters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasSpatialFilter () ) {
                                        // Will gracefully handle an empty XyzFile, as well as avoiding reloading/recomputing from the same file
    FilterSpatial.Set   (   SpatialFilter, 
                            FiltersParam.XyzFile, 
                            SpatialFilterMaxNeighbors ( DefaultSFDistance ), 
                            SpatialFilterMaxDistance  ( DefaultSFDistance ) 
                        );

                                        // If failed, revoke the spatial filter
    if ( FilterSpatial.IsNotAllocated () /*|| FilterSpatial.GetNumElectrodes () != numel*/ )
        SpatialFiltering    = false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// FilterRanking: nothing to Set


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasReference () )

    FilterReference.Set ( Reference, ReferenceTracks );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasRectification () )

    FilterRectification.Set ( Rectification == IDC_RECTIFICATIONABS     ?   FilterRectifyAbs
                          : /*Rectification == IDC_RECTIFICATIONPOWER   ?*/ FilterRectifySquare );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filter is cutting, dialog is remaining
if      ( HasThresholdKeepAbove () && HasThresholdKeepBelow () )
                                        // when both thresholding are being used, the actual method depends on the order of the thresolds
    if ( ThresholdKeepAboveValue > ThresholdKeepBelowValue )    FilterThreshold.Set ( FilterClipAboveAndBelow, ThresholdKeepAboveValue, ThresholdKeepBelowValue );

    else                                                        FilterThreshold.Set ( FilterClipAboveOrBelow,  ThresholdKeepAboveValue, ThresholdKeepBelowValue );

else if ( HasThresholdKeepAbove () )                            FilterThreshold.Set ( FilterClipBelow,         ThresholdKeepAboveValue );

else if ( HasThresholdKeepBelow () )                            FilterThreshold.Set ( FilterClipAbove,         ThresholdKeepBelowValue );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Temporal / frequency filters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Try to update SamplingFrequency
SetSamplingFrequency ( fsamp );

if ( ! HasSamplingFrequency () ) {
                                        // force reset these filters(?) so at least we don't claim to have done them...
    //FiltersParam.ButterworthLow     = BoolToCheck ( false );
    //FiltersParam.ButterworthHigh    = BoolToCheck ( false );
    //FiltersParam.Envelope           = BoolToCheck ( false );
    //FiltersParam.Notches            = BoolToCheck ( false );

    return;                             // problem here...
    }

                                        // setting current frequency limits
FreqMax     = SamplingFrequency / 2;
FreqMin     = EegFilterMinFrequency;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // By safety, we can adjust the requested frequencies to be within current safe limits
bool                freqadjusted    = false;

                                        // reload the frequencies asked by the user
ButterworthLow      = ButterworthLowRequested;
ButterworthHigh     = ButterworthHighRequested;

                                        // adjust values if needed
if ( HasButterworthHigh () )
    freqadjusted   |= ! Clipped ( ButterworthHigh, FreqMin, FreqMax );


if ( HasButterworthLow () )
    freqadjusted   |= ! Clipped ( ButterworthLow,  FreqMin, FreqMax );


if ( HasNotches () ) {

    for ( int n = 0; n < NumNotches; n++ )

        freqadjusted   |= ! Clipped ( Notches[ n ], FreqMin, FreqMax );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasEnvelope () ) {

//  double          minwidth        = FrequenciesToEnvelopeWidth ( ButterworthHigh, ButterworthLow );
//
//  if ( EnvelopeWidth < minwidth && ! silent ) {
//      char                buff[ 256 ];
//      StringCopy  ( buff, "Averaging time window should be at least  ", FloatToString ( minwidth, 2 ), " [ms]" );
//      ShowMessage ( buff, "Filtering", ShowMessageWarning );
//      }

                                        // !FilterTypeEnvelopeAnalytic expects the real, non-rectified data!
    FilterEnvelope.Set ( TracksFiltersEnvelope, SamplingFrequency, EnvelopeWidth );
                                        // asking user - !EnvelopeWidth rounding depends on method!
//  FilterEnvelope.Set ( GetEnvelopeTypeFromUser (), SamplingFrequency, EnvelopeWidth );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this seems the right place - TTracksDoc::SetFilters and TTracksFilters::SetFromStruct will end up calling SetFilters
                                        // High & Low pass filters will be automatically redirected to BandPass WITH TWICE THE ORDER
if      ( HasButterworthBand () )   FilterButterworthBandPass.Set ( GetOrder (), Causal, SamplingFrequency, ButterworthLow, ButterworthHigh );
else if ( HasButterworthHigh () )   FilterButterworthHighPass.Set ( GetOrder (), Causal, SamplingFrequency, ButterworthHigh );
else if ( HasButterworthLow  () )   FilterButterworthLowPass .Set ( GetOrder (), Causal, SamplingFrequency, ButterworthLow  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( HasNotches () ) {

    FilterNotches.Reset ();

                                        // looping through input notch frequencies
    for ( int n = 0; n < NumNotches; n++ ) {
                                        // looping through optional harmonics
        for ( int hi = 1; hi < TFilterNotchMaxHarmonics; hi++ ) {

                                        // current harmonic, including fundamental frequency
            double      fnh     = hi * Notches[ n ];

                                        // no more harmonics?
            if ( ! NotchesAutoHarmonics && hi == 2                                  // auto-harmonics were actually not requested
              || fnh > SamplingFrequency / 2                                        // strictly above Nyquist
              || HasButterworthLow () && fnh > ButterworthLow + 2 * Notches[ n ]    // skip auto-harmonics above the low-pass limit + 2 stops (OK for order 2)
               )
                break;

                                        // check last bandstop is legal, otherwise go for a low pass
            if ( fnh + TFilterNotchWidth / 2 >= SamplingFrequency / 2 )
                                        // width "shifted left"
                FilterNotches.Add ( new TFilterButterworthLowPass <TypeD> ( TFilterNotchOrder, Causal, SamplingFrequency, fnh - TFilterNotchWidth ) );
            else
                                                                         // !exact order, NOT doubled!
                FilterNotches.Add ( new TFilterButterworthBandStop<TypeD> ( TFilterNotchOrder, Causal, SamplingFrequency, fnh - TFilterNotchWidth / 2, fnh + TFilterNotchWidth / 2 ) );

//          DBGV5 ( Notches[ n ], hi, fnh, FilterNotches[ (int) FilterNotches - 1 ].GetFrequencyCutCenter (), (int) FilterNotches, "Notch -> ith harmonic, real freq, #notches" );
            } // for harmonics
        } // for notch
    } // if notches


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( freqadjusted && ! silent ) {
                                        // informing user that the requested frequencies have been modified
    char                buff[ 256 ];

    if      ( ButterworthLow  != ButterworthLowRequested 
           && ButterworthHigh != ButterworthHighRequested   )   StringCopy  ( buff, "Frequencies have been clipped to  ",           FloatToString ( FreqMin, 3 ), " .. ", FloatToString ( FreqMax, 3 ), " [Hz]" );
    else if ( ButterworthLow  != ButterworthLowRequested    )   StringCopy  ( buff, "Low pass frequency has been clipped to  ",     FloatToString ( FreqMax, 3 ), " [Hz]" );
    else if ( ButterworthHigh != ButterworthHighRequested   )   StringCopy  ( buff, "High pass frequency has been clipped to  ",    FloatToString ( FreqMin, 3 ), " [Hz]" );

    ShowMessage ( buff, "Filtering", ShowMessageWarning );
    }

}


//----------------------------------------------------------------------------
                                        // This will copy / convert / adjust / set all internal variables needed to pilot the filtering
template <class TypeD>
void    TTracksFilters<TypeD>::SetFromStruct ( double fsamp, bool silent )
{
Baseline                    = FiltersParam.GetBaseline ();          // checks we are Non Causal
                                        // convert and store in a convenient way the frequencies - note that a value of 0 means no filter
ButterworthLowRequested     = FiltersParam.GetButterworthLow  ();   // what user wanted
ButterworthHighRequested    = FiltersParam.GetButterworthHigh ();
ButterworthLow              = ButterworthLowRequested;              // what user will get, after safety checks
ButterworthHigh             = ButterworthHighRequested;
                                        // these 2 are assumed to be identical, though nothing prevents them to be different - in that case, one should use 2 separate filers instead of a single bandpass
OrderHigh                   = FiltersParam.GetOrderHigh ();
OrderLow                    = FiltersParam.GetOrderLow  ();

Causal                      = FiltersParam.GetCausal ();

SpatialFiltering            = FiltersParam.GetSpatialFiltering ();
//SpatialFilter             = SpatialFilterDefault;

Ranking                     = FiltersParam.GetRanking ();

Rectification               = FiltersParam.GetRectification ();

EnvelopeWidth               = FiltersParam.GetEnvelopeWidth ();

ThresholdKeepAbove          = FiltersParam.IsThresholdAbove  ();
ThresholdKeepAboveValue     = FiltersParam.GetThresholdAbove ();
ThresholdKeepBelow          = FiltersParam.IsThresholdBelow  ();
ThresholdKeepBelowValue     = FiltersParam.GetThresholdBelow ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan & store all notches - usually fundamentals only
NumNotches              = 0;
//Notches.Reset ();
NotchesAutoHarmonics    = FiltersParam.GetNotchesAutoHarmonics ();

if ( FiltersParam.GetNotches () ) {
                                                        // NOT allowing repeated notches of the same frequency, now that we have more powerful notch filter
    TSplitStrings       tokens ( FiltersParam.NotchesValues, UniqueStrings, " \t,;" );
    double              v;

    for ( int i = 0; i < (int) tokens && NumNotches < MaxNotches; i++ ) {

        v   = StringToDouble ( tokens[ i ] );

        if ( v > 0 )
            Notches[ NumNotches++ ] = v;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // which in turn will call SetSamplingFrequency, as we really need this value set
SetFilters ( fsamp, silent );
}


//----------------------------------------------------------------------------
                                        // The exact sequence of filters, with branches according to:
                                        // temporal / topographical  x  pre- / post-reference
                                        // The caller sets the flags according to his needs
template <class TypeD>
void    TTracksFilters<TypeD>::ApplyFilters (   TArray2<TypeD>&     data,       int                 numel,      
                                                int                 numpts,     int                 tfoffset, 
                                                ReferenceType       reference,  const TSelection*   referencetracks,    const TSelection*   validtracks,    const TSelection*   auxtracks, 
                                                FilterPrecedence    filterprecedence 
                                            )
{
if ( filterprecedence == UnknownPrecedence )
    return;

                                        // Put everything in a big block
OmpParallelBegin

                                        // Pre-Reference, Temporal filters
if ( IsFlag ( filterprecedence, FilterTemporal ) && ( HasBaseline () || HasButterworthBand () || HasButterworthHigh () || HasButterworthLow () || HasNotches () ) ) {

    OmpFor

    for ( int el = 0; el < numel; el++ ) {

        if ( auxtracks && auxtracks->IsSelected ( el ) )
            continue;

                                        // !Always before High Pass!
        if      ( HasBaseline ()        )   FilterBaseline.Apply            ( data[ el ] + tfoffset, numpts );

                                        // force Band Pass
        if      ( HasButterworthBand () )   FilterButterworthBandPass.Apply ( data[ el ] + tfoffset, numpts );

        else if ( HasButterworthHigh () )   FilterButterworthHighPass.Apply ( data[ el ] + tfoffset, numpts );

        else if ( HasButterworthLow  () )   FilterButterworthLowPass .Apply ( data[ el ] + tfoffset, numpts );


        if ( HasNotches () )                FilterNotches.Apply             ( data[ el ] + tfoffset, numpts );
        } // for el

    } // FilterTemporal


                                        // Pre-Reference Non-Temporal + Reference filters       !testing parameter reference!
if ( IsFlag ( filterprecedence, FilterNonTemporal ) && ( HasSpatialFilter () || HasRanking () || HasReference ( reference ) ) ) {
                                    // Allocate private objects
    TVector<TypeD>          temp ( numel );

    OmpFor

    for ( long tf0 = 0; tf0 < numpts; tf0++ ) {

                                    // copy data to temp map
        for ( int i = 0; i < numel; i++ )
            temp[ i ]    = data ( i, tfoffset + tf0 );


        if ( HasSpatialFilter () )          FilterSpatial.Apply ( temp );                       // could also make use auxtracks?


        if ( HasRanking       () )          FilterRanking.Apply ( temp, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );  // ignoring null values or not? or ignoring only for positive data?

                                    // !Calling  HasReference()  with overridden parameter!
        if ( HasReference ( reference ) )   FilterReference.Apply ( temp, reference, referencetracks, validtracks, auxtracks );

                                    // copy back map results
        for ( int i = 0; i < numel; i++ )
            data ( i, tfoffset + tf0 )  = temp[ i ];
        } // for tf
    } // FilterNonTemporal


                                        // Post-Reference, both Temporal and Non-Temporal filters
if ( HasRectification () || HasEnvelope () || HasThresholdKeepAbove () || HasThresholdKeepBelow () ) {

    OmpFor

    for ( int el = 0; el < numel; el++ ) {

        if ( auxtracks && auxtracks->IsSelected ( el ) )
            continue;

                                        // BEFORE Envelope
                                        // !We call with a vector of temporal data, but it does not matter for this operator!
        if ( IsFlag ( filterprecedence, FilterNonTemporal ) && HasRectification () )            FilterRectification .Apply  ( data[ el ] + tfoffset, numpts );

                                        // AFTER Rectification
        if ( IsFlag ( filterprecedence, FilterTemporal    ) && HasEnvelope () )                 FilterEnvelope      .Apply  ( data[ el ] + tfoffset, numpts );

                                        // !We call with a vector of temporal data, but it does not matter for this operator!
        if ( IsFlag ( filterprecedence, FilterNonTemporal ) && ( HasThresholdKeepAbove () 
                                                              || HasThresholdKeepBelow () ) )   FilterThreshold     .Apply  ( data[ el ] + tfoffset, numpts );
        } // for el

    } // After Reference


OmpParallelEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
char   *TTracksFilters<TypeD>::ParametersToText ( char *text )    const
{
constexpr char*     TextSeparator   = ", ";

ClearString ( text );


if ( HasBaseline () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "DC removed" );
    }

                                        // Bypass Low and High filters, converted to Band Pass
if      ( HasButterworthBand () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Band Pass ", FloatToString ( ButterworthHigh, 2 ), " to ", FloatToString ( ButterworthLow, 2 ) );
    }

else if ( HasButterworthHigh () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "High Pass ", FloatToString ( ButterworthHigh, 2 ) );
    }

else if ( HasButterworthLow () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Low Pass ", FloatToString ( ButterworthLow, 2 ) );
    }


if ( HasButterworthFilter () ) {
    AppendSeparator ( text, TextSeparator );
                                        // !Order for band-pass cumulates orders of high and low filters!
    StringAppend    ( text, "Order ", IntegerToString ( GetOrder () ) );
    }

                                        // by security, put this first, so decoding will read it first too, as it can condition baseline
if ( HasButterworthFilter () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, Causal == FilterCausal ? "Causal filtering" : "Non-Causal filtering" );
    }


if ( HasNotches () ) {
    AppendSeparator ( text, TextSeparator );

                                        // Current effective notches, including the one generated automatically
//  StringAppend    ( text, (int) FilterNotches == 1 ? "Butterworth Notch " : "Butterworth Notches " );
//  for ( int ni = 0; ni < (int) FilterNotches; ni++ )
//      StringAppend ( text, ni ? " " : "", FloatToString ( FilterNotches[ ni ].GetFrequencyCutCenter (), 2 ) );

                                        // Only the requested values from user
    StringAppend    ( text, NumNotches == 1 ? "Butterworth Notch " : "Butterworth Notches " );

    for ( int ni = 0; ni < NumNotches; ni++ )
        StringAppend ( text, ni ? " " : "", FloatToString ( Notches[ ni ], 2 ) );

    if ( NotchesAutoHarmonics )
        StringAppend ( text, " + All Harmonics" );
    }


if ( HasSpatialFilter () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, SpatialFilterLongName[ SpatialFilter ] );
    }


if ( HasRanking () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Ranked" );
    }


if ( HasRectification () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Rectification ", Rectification == IDC_RECTIFICATIONABS ? "Abs" : "Power" );
    }


if ( HasEnvelope () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Envelope of ", FloatToString ( EnvelopeWidth, 2 ), " ms" );
    }

                                        // bonus: write the sampling frequency, in case we reload with  TextToParameters
if ( HasSamplingFrequency () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Sampling Frequency ", FloatToString ( SamplingFrequency, 2 ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if ( HasThresholdKeepAbove () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Threshold showing Above ", FloatToString ( ThresholdKeepAboveValue, 2 ) );
    }


if ( HasThresholdKeepBelow () ) {
    AppendSeparator ( text, TextSeparator );

    StringAppend    ( text, "Threshold showing Below ", FloatToString ( ThresholdKeepBelowValue, 2 ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( StringIsEmpty ( text ) )
    StringCopy ( text, "No filters" );


return text;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TTracksFilters<TypeD>::TextToParameters ( const char* text, const char* xyzfile, double fsamp )
{
const char*         toc;
char                t [ 256 ];


FiltersParam.ResetAll ();

                                        // scan string and update the parameters structure itself
if ( ( toc = StringContains ( text, "Band Pass " ) ) != 0 ) {

    sscanf ( toc, "Band Pass %[-+.0-9]s", t );
    FiltersParam.SetButterworthHigh ( t );

    toc = StringContains ( toc, " to " );
    sscanf ( toc, " to %[-+.0-9]s", t );
    FiltersParam.SetButterworthLow  ( t );
    }
                                        // still allowing old input, both Low and High filters, which will later be automatically interpreted as Band Pass
if ( ( toc = StringContains ( text, "High Pass " ) ) != 0 ) {
    sscanf ( toc, "High Pass %[-+.0-9]s", t );
    FiltersParam.SetButterworthHigh ( t );
    }

if ( ( toc = StringContains ( text, "Low Pass " ) ) != 0 ) {
    sscanf ( toc, "Low Pass %[-+.0-9]s", t );
    FiltersParam.SetButterworthLow ( t );
    }

                                        // !High & Low pass must be done before!
if ( ( toc = StringContains ( text, "Order" ) ) != 0 ) {
    sscanf ( toc, "Order %[0-9]s", t );
                                        // Comment this in case GetOrder is not doubling the order for Bandpass
                                        // Bandpass has twice the order of each low/high individual filter
    if ( FiltersParam.IsBandPass () )
                                        // set half the band-pass order
        IntegerToString ( t, StringToInteger ( t ) / 2 );

    FiltersParam.SetOrderHigh ( t );
    FiltersParam.SetOrderLow  ( t );
    }

                                        // set this BEFORE baseline, as it conditions it
if      ( ( toc = StringContains ( text, "Non-Causal" ) ) != 0 )    // !tries longest string "Non-Causal" first!
    FiltersParam.SetCausal ( toc );
else if ( ( toc = StringContains ( text, "Causal"     ) ) != 0 ) 
    FiltersParam.SetCausal ( toc );
   

if ( ( toc = StringContains ( text, "DC removed" ) ) != 0 ) {
    FiltersParam.SetBaseline ( toc );
    }

                                 // new string is "Butterworth Notch", but "Notch" allows for older versions to load anyway
if ( ( toc = StringContains ( text, "Notch" ) ) != 0 ) {

    if ( StringContains ( text, "Notches" ) )   // skip this word
        toc += StringSize ( "Notches" );
    else
        toc += StringSize ( "Notch" );


    FiltersParam.SetNotchesAutoHarmonics ( StringContains ( toc, "+ All Harmonics" ) );


    StringCopy ( t, toc );
                                        // cut until a ',' or '+'
    for ( int i = 0; i < StringLength ( t ); i ++ )
        if ( t[ i ] == ',' || t[ i ] == '+' )
            t[ i ] = 0;
                                        // sending a string
    FiltersParam.SetNotches ( t );
    }


if ( ( toc = StringContains ( text, "Rank" ) ) != 0 ) {
    FiltersParam.SetRanking ( toc );
    }


if ( ( toc = StringContains ( text, "Rectification " ) ) != 0 ) {
    sscanf ( toc, "Rectification %[A-Za-z]s", t );
    FiltersParam.SetRectification ( t );
    }

                                        // Rectification not mandatory anymore
if ( ( toc = StringContains ( text, "Envelope of " ) ) != 0 ) {
    sscanf ( toc, "Envelope of %[-+.0-9]s", t );
    FiltersParam.SetEnvelopeWidth ( t );

                                        // for safety
    if ( ! FiltersParam.IsRectification () )
        FiltersParam.SetRectification ( "Power" );
    }


if ( ( toc = StringContains ( text, "Sampling Frequency " ) ) != 0 ) {
    double          samplingfrequency;
    sscanf ( toc, "Sampling Frequency %lf", &samplingfrequency );
    SetSamplingFrequency ( samplingfrequency ); 
                                        // could also set  FiltersParam.ValueSamplingFrequency  ?
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // testing all possible spatial filter strings
if ( ( SpatialFilter = TextToSpatialFilterType ( text ) ) != SpatialFilterNone )

    FiltersParam.SetSpatialFiltering ( xyzfile );   // FiltersParam  stores only the XYZ file, not the type of filter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ( toc = StringContains ( text, "showing Above " ) ) != 0 ) {
    sscanf ( toc, "showing Above %[-+.0-9]s", t );
    FiltersParam.SetThresholdAbove ( t );
    }


if ( ( toc = StringContains ( text, "showing Below " ) ) != 0 ) {
    sscanf ( toc, "showing Below %[-+.0-9]s", t );
    FiltersParam.SetThresholdBelow ( t );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convert & check parameters
SetFromStruct ( fsamp, false );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
