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

enum        RegularizationType;
enum        AtomType;
class       TInverseMatrixDoc;

                                        // A few processings can work faster, without any loss, by using some downsampling technique:
constexpr int   DownsamplingTargetSizeGfp       = 10000;
constexpr int   DownsamplingTargetSizeReg       =  1000;
                                        // Results start looking good past 5'000, and improve up to 15'000 - let's settle to 10'000, which is already quite long to compute
constexpr int   DownsamplingTargetSizeZScore    = 10000;

                                        // Centralizing the inverse solution-to-infix string generation, so as to be consistent across code
char*       GetInverseInfix (   const TInverseMatrixDoc*    ISDoc,  RegularizationType  regularization, AtomType            datatype,
                                char*                       infix 
                            );


//----------------------------------------------------------------------------
                                        // Converts a group of files to another one (on disk)
                                        // according to a bunch of parameters relevant for segmentation/fitting
enum        DualDataType;
enum        SpatialFilterType;
enum        BackgroundNormalization;
enum        ZScoreType;
enum        FilterTypes;
enum        EpochsType;
enum        GfpPeaksDetectType;
enum        SkippingEpochsType;
class       TGoF;
class       TGoGoF;
class       TRois;
class       TStrings;
class       TSuperGauge;


void        PreProcessFiles (   const TGoF&             gofin,              AtomType                datatypeout,
                                DualDataType            dualdata,           const TGoF*             dualgofin,
                                SpatialFilterType       spatialfilter,      const char*             xyzfile,
                                bool                    computeesi,         TInverseMatrixDoc*      ISDoc,          RegularizationType  regularization,     RegularizationType* usedregularization,
                                bool                    mergecomplex,
                                bool                    gfpnormalize,
                                BackgroundNormalization backnorm,           ZScoreType              zscoremethod,   const char*         zscorefile,
                                bool                    ranking,
                                bool                    thresholding,       double                  threshold,
                                FilterTypes             envelope,           double                  envelopeduration,
                                const TRois*            rois,               FilterTypes             roimethod,
                                EpochsType              epochs,             const TStrings*         epochfrom,      const TStrings*     epochto,
                                GfpPeaksDetectType      gfppeaks,           const char*             listgfppeaks,
                                SkippingEpochsType      badepochs,          const char*             listbadepochs,  double              badepochstolerance,
                                const char*             outputdir,
                                const char*             fileprefix,
                                int                     clipfromname,       int                     cliptoname,
                                bool                    createtempdir,      char*                   temppath,
                                bool                    savemainfiles,      TGoGoF&                 gogofout,       TGoGoF*             dualgogofout,       TGoF&               gofoutdir,      bool&       newfiles,
                                bool                    savezscore,         TGoF*                   zscoregof,
                                TSuperGauge*            gauge = 0 
                            );


                                        // Centralizing corresponding progress bar's number of steps
int         PreProcessFilesGaugeSize    ( int numconditions );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
