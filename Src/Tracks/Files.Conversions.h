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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void                FileConversionVrbToTva  ( const char* file );
//void              SplitMatFiles           ( const char* file );


//----------------------------------------------------------------------------

enum                FrequencyAnalysisType;
class               TGoF;

void                MergeTracksToFreqFiles  ( const TGoF& gof,           FrequencyAnalysisType freqtype = (FrequencyAnalysisType) 0, char *returnfreqfile = 0, bool showgauge = true );
void                MergeTracksToFreqFiles  ( const char* filestemplate, FrequencyAnalysisType freqtype = (FrequencyAnalysisType) 0, char *returnfreqfile = 0, bool showgauge = true );
void                SortFFTApproximation    ( const char* file );


//----------------------------------------------------------------------------

constexpr char*     RisToPointsTitle        = "RIS to Vectors";

class               TSelection;

void                RisToCloudVectors       (
                                            const char*         risfile,
                                            int                 tfmin,          int             tfmax,
                                            const TSelection*   spselin,        const char*     splist,
                                            bool                spontaneous,    bool            normalize,
                                            TGoF&               outgof,
                                            bool                showprogress
                                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
