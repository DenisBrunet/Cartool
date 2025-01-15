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

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TArray2;
template <class TypeD> class        TSetArray2;


//----------------------------------------------------------------------------
                                        // base class for frequency files

constexpr int       MaxCharFrequencyName        = 16;
constexpr int       MaxCharFreqType             = 32;

                                        // max number of points to be displayed
constexpr int       FreqMaxPointsDisplay        = 1 * 60 * 1000;

                                        // known frequency analysis types, and their names
enum                FrequencyAnalysisType 
                    {
                    FreqUnknown,

                    FreqFFTNorm,
                    FreqFFTNorm2,
                    FreqFFTComplex,
                    FreqFFTPhase,

                    FreqFFTApproximation,

                    FreqSTransform,         // old one, used only to open old files - now use one of the following 3
                    FreqSTransformNorm,
                    FreqSTransformNorm2,
                    FreqSTransformComplex,
                    FreqSTransformPhase,

                    FreqButterworthBank,

                    FreqESI,

                    NumFrequencyAnalysisTypes,

                                        // a few useful ranges:
                    FreqFFTMin              = FreqFFTNorm,
                    FreqFFTMax              = FreqFFTPhase,
                    FreqSTransformMin       = FreqSTransform,
                    FreqSTransformMax       = FreqSTransformPhase,
                    };


extern  const char  FrequencyAnalysisNames[ NumFrequencyAnalysisTypes ][ MaxCharFreqType ];

                                        // At one point, replace all these with a struct with fields, used in a predefined array of formats
inline  bool    IsFreqTypeFFT               ( FrequencyAnalysisType ft )    { return  IsInsideLimits ( ft, FreqFFTMin,          FreqFFTMax        ); }
inline  bool    IsFreqTypeFFTApproximation  ( FrequencyAnalysisType ft )    { return  ft == FreqFFTApproximation; }
inline  bool    IsFreqTypeSTransform        ( FrequencyAnalysisType ft )    { return  IsInsideLimits ( ft, FreqSTransformMin,   FreqSTransformMax ); }
inline  bool    IsFreqTypeButterworthBank   ( FrequencyAnalysisType ft )    { return  ft == FreqButterworthBank; }
inline  bool    IsFreqTypeESI               ( FrequencyAnalysisType ft )    { return  ft == FreqESI; }

inline  bool    IsFreqTypeReal              ( FrequencyAnalysisType ft )    { return  ft == FreqFFTApproximation; }
inline  bool    IsFreqTypeComplex           ( FrequencyAnalysisType ft )    { return  ft == FreqFFTComplex || ft == FreqSTransformComplex; }
inline  bool    IsFreqTypePhase             ( FrequencyAnalysisType ft )    { return  ft == FreqFFTPhase   || ft == FreqSTransformPhase;   }
inline  bool    IsFreqTypePositive          ( FrequencyAnalysisType ft )    { return  ! ( IsFreqTypeReal ( ft ) || IsFreqTypeComplex ( ft ) || IsFreqTypePhase ( ft ) ); }

                                        // reverse from a string to a FrequencyAnalysisType, as .freq files store only a string, not the enum itself
FrequencyAnalysisType   StringToFreqType    ( const char* freqtypestring ); 


//----------------------------------------------------------------------------
                                        // Class is derived from TTracksDoc as it shares many common methods,
                                        // it will allow Cartool to use the tracks view on these data, too.
                                        // Then class has its own methods dedicated to frequcencies
class   TFreqDoc    :   public  TTracksDoc
{
public:
                    TFreqDoc                    ( owl::TDocument *parent = 0 );

                                        // owl::TDocument
    bool            InitDoc                     ();
    bool            IsOpen	                    ()              const       { return  NumFrequencies > 0; }

                                        // TLimits
    void            InitLimits                  ( InitType how )        final;

                                        // TTracksDoc
    void            SetReferenceType            ( ReferenceType ref, const char* tracks = 0, const TStrings* elnames = 0, bool verbose = true ) final   { Reference = ReferenceAsInFile; }  // re-referencing not allowed
    bool            CanFilter                   ()              const               final   { return false; }                   // filtering not allowed
    bool            SetFiltersActivated         ( bool state, bool silent = false ) final   { FiltersActivated = false; return FiltersActivated; }
    void            GetTracks                   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0, AtomType atomtype = AtomTypeUseCurrent, PseudoTracksType pseudotracks = NoPseudoTracks, ReferenceType reference = ReferenceAsInFile, const TSelection* referencetracks = 0, const TRois *rois = 0 )  override;
    bool            HasStandardDeviation        ()              const               final   { return false; }

                                        // TFreqDoc
    virtual int     GetNumFrequencies           ()              const   { return NumFrequencies; }
    virtual double  GetOriginalSamplingFrequency()              const   { return OriginalSamplingFrequency; }

    virtual const char*     GetFrequencyName    ( int f = 0 )   const   { return  FrequenciesNames[ f ]; }
    virtual const TStrings* GetFrequenciesNames ()              const   { return &FrequenciesNames; }

    void            SetCurrentFrequency         ( int f, bool notifyviews = false );   // if called by a view, prevent a call back
    int             GetCurrentFrequency         ()              const   { return CurrentFrequency; }
    int             GetFrequencyOffset          ()              const   { return CurrentFrequency * TotalElectrodes; }

    FrequencyAnalysisType   GetFreqType         ()              const   { return  IsInsideLimits ( FreqType, FreqUnknown, (FrequencyAnalysisType) ( NumFrequencyAnalysisTypes - 1 ) ) ? FreqType : FreqUnknown; }
    const char*     GetFreqTypeName             ()              const   { return  FrequencyAnalysisNames[ GetFreqType () ]; }
    bool            IsFreqTypeFFT               ()              const   { return  crtl::IsFreqTypeFFT               ( GetFreqType () ); }
    bool            IsFreqTypeFFTApproximation  ()              const   { return  crtl::IsFreqTypeFFTApproximation  ( GetFreqType () ); }
    bool            IsFreqTypeSTransform        ()              const   { return  crtl::IsFreqTypeSTransform        ( GetFreqType () ); }
    bool            IsFreqTypeButterworthBank   ()              const   { return  crtl::IsFreqTypeButterworthBank   ( GetFreqType () ); }
    bool            IsFreqTypeESI               ()              const   { return  crtl::IsFreqTypeESI               ( GetFreqType () ); }
    bool            IsFreqTypeReal              ()              const   { return  crtl::IsFreqTypeReal              ( GetFreqType () ); }
    bool            IsFreqTypeComplex           ()              const   { return  crtl::IsFreqTypeComplex           ( GetFreqType () ); }
    bool            IsFreqTypePhase             ()              const   { return  crtl::IsFreqTypePhase             ( GetFreqType () ); }
    bool            IsFreqTypePositive          ()              const   { return  crtl::IsFreqTypePositive          ( GetFreqType () ); }

                                        // get all tracks (but no pseudos), all freqs, as they read from the file
    virtual void    ReadFrequencies             ( long tf1, long tf2, TSetArray2<float> &buff, int tfoffset = 0 ) = 0;
                                        // same as above, but can return real and imaginary parts
    virtual void    ReadFrequencies             ( long tf1, long tf2, TSetArray2<float> &realpart, TSetArray2<float> &imagpart, int tfoffset = 0 ) = 0;
                                        // get all tracks (pseudos optionals), all freqs
    virtual void    GetFrequencies              ( long tf1, long tf2, TSetArray2<float> &buff, int tfoffset = 0, AtomType atomtype = AtomTypeUseCurrent, PseudoTracksType pseudotracks = NoPseudoTracks, ReferenceType reference = ReferenceAsInFile, TSelection* referencetracks = 0, TRois *rois = 0 );
                                        // handy function to just retrieve data on-demands
    virtual double  GetFreqValue                ( long el, long tf, long f ) = 0;

                                        // process the file to have nicely ordered polarities
    void            SortFFTApproximation ();


protected:

    int                     NumFrequencies;
    double                  OriginalSamplingFrequency;  // the SamplingFrequency in the original file
    TStrings                FrequenciesNames;           // frequency names
    FrequencyAnalysisType   FreqType;                   // type of analysis that produced this frequency data
    int                     CurrentFrequency;
};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
