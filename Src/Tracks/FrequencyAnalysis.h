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

#include    <complex>

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


enum    FreqAnalysisType
        {
        UnknownFreqAnalysis,
        FreqAnalysisFFT,
        FreqAnalysisPowerMaps,          // FFT for surface EEG: Average Reference, Norm^2
        FreqAnalysisFFTApproximation,
        FreqAnalysisSTransform,         // Stockwell transform, a sort of Wavelet very close to Morlet's
        };


enum    FreqOutputBands
        {
        UnknownFreqOutput,
        OutputLinearInterval,
        OutputLogInterval,
        OutputBands,
        };


enum    FreqOutputAtomType
        {
        UnknownFreqOutputAtom,
        OutputAtomReal,
        OutputAtomNorm,
        OutputAtomNorm2,
        OutputAtomComplex,
        OutputAtomPhase,
        };


enum    FreqWindowingType
        {
        FreqWindowingNone,
        FreqWindowingHanning,
        FreqWindowingHanningBorder,
        };


//----------------------------------------------------------------------------
                                        //  a few utilities used by FrequencyAnalysis
void    InvertFreqMap                   (   std::complex<float>*    v,  
                                            int                     numel   
                                        );

bool    IsNegativeCorrelation           (   std::complex<float>*    vc1, 
                                            std::complex<float>*    vc2, 
                                            int                     numel   
                                        );

void    ApproximateFrequency            (   std::complex<float>     cstl[], 
                                            int                     numel   
                                        );

void    ApproximateFrequencyOptimized   (   std::complex<float>     cstl[], 
                                            int                     numel   
                                        );


//----------------------------------------------------------------------------
                                        // Frequency analysis, with all options
enum    ReferenceType;
enum    MarkerType;
class   TTracksDoc;


bool    FrequencyAnalysis   (   TTracksDoc*         eegdoc,             // not const, because we activate/deactivate filters
                                const char*         xyzfile,
                                FreqAnalysisType    analysis,
                                const char*         channels,           // could be empty or "*" to select all regular tracks
                                ReferenceType       ref,                const char*         reflist,
                                long                timemin,            long                timemax,            bool                endoffile,
                                double              samplingfrequency,
                                int                 numblocks,          int                 blocksize,          int                 blockstep,          double              blocksoverlap,
                                FreqOutputBands     outputbands,        FreqOutputAtomType  outputatomtype,
                                bool                outputmarkers,      MarkerType          outputmarkerstype,
                                const char*         outputbandslist,
                                double              outputfreqmin,      double              outputfreqmax,      double              outputfreqstep,     
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
                                bool                silent
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
