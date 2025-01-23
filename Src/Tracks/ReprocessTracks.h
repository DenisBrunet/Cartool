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

enum                TracksOptions
                    {
                    ProcessUnknown,
                    ProcessTracks,
                    ProcessRois,
                    };

enum                TimeOptions
                    {
                    UnknownTimeProcessing,
                    ExportTimeInterval,
                    ExportTimeTriggers,
                    ExcludeTimeTriggers
                    };

enum                FiltersOptions
                    {
                    NotUsingFilters,
                    UsingCurrentFilters,
                    UsingOtherFilters,
                    };

enum                RescalingOptions
                    {
                    NotRescaled,
                    ConstantRescaling,
                    GfpRescaling,
                    };

enum                SequenceOptions
                    {
                    UnknownSequenceProcessing,
                    SequenceProcessing,
                    AverageProcessing,
                    };

enum                ConcatenateOptions
                    {
                    NoConcatenateTime,
                    ConcatenateTime,
                    };


enum                ReferenceType;
enum                SavingEegFileTypes;
class               TTracksDoc;
class               TElectrodesDoc;
class               TRoisDoc;
template <class TypeD>  class   TTracksFilters;
class               TExportTracks;
class               TGoF;

                                        // This is a swiss-army knive function to reprocess EEG / tracks files:
                                        //  - Exporting all or some sub-set of tracks, including computed tracks like GFP / DIS / AVG
                                        //  - Computing ROIs
                                        //  - Re-incorporating tracks names from a coordinates files into the EEG file
                                        //  - Exporting a time interval only, or time points from epochs, or all time points but excluded epochs
                                        //  - Revoking filters, or forcing other filters
                                        //  - Adding as many null channels as supplemental tracks
                                        //  - Re-referencing
                                        //  - Applying some baseline correction
                                        //  - Rescaling by constant factor or by mean GFP
                                        //  - Computing the time-average of each file
                                        //  - Downsampling by integer ratios
                                        //  - Saving to other file types
                                        //  - Concatenating all results from any processing above into a single file
                                        //  - Running on a single file or in batch

                                        // Note 1: this function has some kind of overlap with the  PreProcessFiles  one, but is not designed for the same use
                                        // Note 2: the sequence of all these processing matters a lot, and is hard-wired into the code so to make sense
bool    ReprocessTracks (
                        TTracksDoc*         EEGDoc,
                        TracksOptions       tracksoptions,          const char*         tracks,
                        const TElectrodesDoc*   xyzdoc,             // either 0 if not used, or already opened - function does not handle the opening or closing
                        const TRoisDoc*         roisdoc,
                        TimeOptions         timeoptions,
                        long                timemin,                long                timemax,            // could be set to max for EOF
                        const char*         triggerstokeeplist,     const char*         triggerstoexcludelist,
                        const char*         addingnullchannels,
                        FiltersOptions      filtersoptions,         const TTracksFilters<float>*    altfilters,     double&     defaultsamplingfrequency,
                        ReferenceType       ref,                    const char*         reflist,
                        bool                baselinecorr,
                        long                baselinecorrpre,        long                baselinecorrpost,   // could be set to max for EOF
                        RescalingOptions    rescalingoptions,       double              rescalingfactor,
                        SequenceOptions     sequenceoptions,
                        int                 downsampleratio,
                        SavingEegFileTypes  filetype,
                        const char*         infixfilename,
                        const char*         freqinfix,              // for frequency processing purpose
                        bool                outputmarkers,
                        ConcatenateOptions  concatenateoptions,     long*               concatinputtime,        long*               concatoutputtime,
                        TExportTracks&      expfile,                // Needed for concatenating across multiple calls/files
                        const TGoF*         batchfilenames,         // For the moment, transmit this group of files
                        int                 batchfileindex,         // index of current file in batch processing
                        bool                silent
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
