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

#include    "CartoolTypes.h"            // RegularizationType BackgroundNormalization AtomType ZScoreType CentroidType FilterTypes

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    ComputingRisPresetsEnum;
enum    GroupsLayoutEnum;
enum    SpatialFilterType;
class   TGoGoF;
class   TGoF;


//constexpr CentroidType  RisCentroidMethod = MedianCentroid;                   // Median gives sharper shapes/contours, counterpart is it basically forces to store all the data, and is more time-consuming to compute
constexpr CentroidType  RisCentroidMethod   = MeanCentroid;                     // maps can be quite empty after optional thresholding, a Median might be too radical while a Mean will still output something
//constexpr CentroidType  RisCentroidMethod = WeightedMeanCentroid;

constexpr FilterTypes   RisRoiMethod        = FilterTypeMean;                   // for the same reason as for centroids: merging clipped data with a Median can produce too much null results

constexpr FilterTypes   RisEnvelopeMethod   = FilterTypeEnvelopeGapBridging;    // results close to analytic, but can work with positive-only data


bool    ComputingRis    (   ComputingRisPresetsEnum esicase,
                            const TGoGoF&       gogof,                  
                            GroupsLayoutEnum    grouplayout,            int                 numsubjects,        int             numconditions,
                            
                            const TGoF&         inversefiles,           RegularizationType  regularization,     BackgroundNormalization     backnorm,
                            AtomType            datatypeepochs,         AtomType            datatypefinal,
                            CentroidType        centroidsmethod,

                            SpatialFilterType   spatialfilter,          const char*         xyzfile,
                            bool                ranking,
                            bool                thresholding,           double              keepingtopdata,
                            bool                envelope,               FilterTypes         envelopetype,       double          envelopelowfreq,    double          envelopeduration,
                            bool                roiing,                 const char*         roifile,            FilterTypes     roimethod,

                            bool                savingindividualfiles,  bool                savingepochfiles,   bool            savingzscorefactors,
                            bool                computegroupsaverages,  bool                computegroupscentroids,
                            const char*         basedir,                const char*         fileprefix,
                            VerboseType         verbose
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
