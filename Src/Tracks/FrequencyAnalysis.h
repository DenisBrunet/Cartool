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

#include    <complex>

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     FrequencyAnalysisTitle      = "Frequency Analysis";
                                        // also limit the number of frequencies to average
constexpr int       STranformMaxAvg             = 25;
                                        // when high frequency resolution is available, limit the averaging to this step
constexpr double    AvgMinStep_hz               = 0.25;
                                        // smallest step between output frequencies
constexpr double    FrequencyResolution         = 0.01; // 0.1;

constexpr double    FreqMinLog10Value           = 1e-3;
                                        // Hanning windowing deletes half of the signal, we can rescale the results by 2 to recover the original amplitude
constexpr double    HanningRescaling            = 2.0;


//----------------------------------------------------------------------------
                                        // Analysis specifications
enum    FreqAnalysisCases
        {
        FreqCaseUndefined       = 0x0000,
                                            // General use cases (all mutually exclusive):
        FreqCaseEEGSurface      = 0x0100,
        FreqCaseEEGIntra        = 0x0200,
        FreqCaseGeneral         = 0x0400,   // general case, neither surface nor intra-cranial    

                                            // Computation method (all mutually exclusive):
        FreqMethodFFT           = 0x0010,   // FFT method
        FreqMethodFFTApprox     = 0x0020,   // FFT Approximation method
        FreqMethodST            = 0x0040,   // STransform method
        };


inline  bool    IsSurfaceCase       ( FreqAnalysisCases c )     { return IsFlag ( c, FreqCaseEEGSurface ); }
inline  bool    IsIntraCase         ( FreqAnalysisCases c )     { return IsFlag ( c, FreqCaseEEGIntra   ); }
inline  bool    IsGeneralCase       ( FreqAnalysisCases c )     { return IsFlag ( c, FreqCaseGeneral    ); }

inline  bool    IsFFTMethod         ( FreqAnalysisCases c )     { return IsFlag ( c, FreqMethodFFT      ); }
inline  bool    IsFFTApproxMethod   ( FreqAnalysisCases c )     { return IsFlag ( c, FreqMethodFFTApprox); }
inline  bool    IsSTMethod          ( FreqAnalysisCases c )     { return IsFlag ( c, FreqMethodST       ); }
                                        // Power Maps is just Surface EEG + FFT + Average Reference
inline  bool    IsPowerMaps         ( FreqAnalysisCases c )     { return IsSurfaceCase ( c ) && IsFFTMethod ( c ); }

const char*     GetCaseString       ( FreqAnalysisCases c );
const char*     GetMethodString     ( FreqAnalysisCases c );
const char*     GetInfix            ( FreqAnalysisCases c );

                                        // Analysis and legal output types have a not-straightforward relationship, so let's wrap it into a proper function
enum    FreqOutputAtomType;

const char*     AnalysisAtomtypeCompatibility ( FreqAnalysisCases   analysis,   
                                                bool                savingbands,  
                                                bool                averagingblocks, 
                                                FreqOutputAtomType  outputatomtype  );


//----------------------------------------------------------------------------

enum    FreqOutputBands
        {
        OutputLinearInterval,
        OutputLogInterval,
        OutputBands,
        };

                                        // Controlling how the FFT results should be rescaled / divided
enum    FFTRescalingType
        {
        FFTRescalingNone,
        FFTRescalingSqrtWindowSize,
        FFTRescalingWindowSize,
        NumFFTRescalingType,

        FFTRescalingForward         = FFTRescalingNone,
        FFTRescalingBackward        = FFTRescalingWindowSize,   // default rescaling used for forward AND backward (FFT then FFTI) to get back to original scaling
        FFTRescalingParseval        = FFTRescalingWindowSize,   // FFT forward only Parseval rescaling, which preserves the energy of the signal - still be careful of real signal "folded" output
        };

extern const char   FFTRescalingString[ NumFFTRescalingType ][ 64 ];

                                        // Controlling how the FFT complex values are written to file
enum    FreqOutputAtomType
        {
        OutputAtomReal,
        OutputAtomNorm,
        OutputAtomNorm2,
        OutputAtomComplex,
        OutputAtomPhase,
        NumFreqOutputAtomType
        };

extern const char   FreqOutputAtomString[ NumFreqOutputAtomType ][ 32 ];


enum    FreqWindowingType
        {
        FreqWindowingNone,
        FreqWindowingHanning,
        FreqWindowingHanningBorder,
        NumFreqWindowingType
        };

extern const char   FreqWindowingString[ NumFreqWindowingType ][ 64 ];


//----------------------------------------------------------------------------
                                        // Frequency analysis, with all options
enum    ReferenceType;
enum    SkippingEpochsType;
enum    MarkerType;
enum    VerboseType;
class   TTracksDoc;


bool    FrequencyAnalysis   (   TTracksDoc*         eegdoc,             // not const, because we activate/deactivate filters
                                const char*         xyzfile,
                                FreqAnalysisCases   analysis,
                                const char*         channels,           // could be empty or "*" to select all regular tracks
                                ReferenceType       ref,                const char*         reflist,
                                long                timemin,            long                timemax,
                                SkippingEpochsType  badepochs,          const char*         listbadepochs,
                                double              samplingfrequency,
                                int                 blocksize,          double              blocksoverlap,
                                FFTRescalingType    fftnorm,
                                FreqOutputBands     outputbands,
                                FreqOutputAtomType  outputatomtype,
                                bool                outputmarkers,      MarkerType          outputmarkerstype,
                                const char*         outputbandslist,
                                double              outputfreqmin,      double              outputfreqmax,
                                double              outputfreqstep,     
                                int                 outputdecadestep,
                                bool                outputsequential,   // averaged otherwise
                                FreqWindowingType   windowing,
                                bool                optimaldownsampling,
                                const char*         infixfilename,      bool                createsubdir,
                                char*               fileoutfreq,        // these parameters act as flags: if null, ignore specific output, otherwise, generate file
                                char*               fileoutsplitelec,
                                char*               fileoutsplitfreq,
                                char*               fileoutspectrum,
                                char*               fileoutapprfreqs,
                                VerboseType         verbosey
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
