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

class   TGoGoF;
class   TGoF;

                                        // Functions to be converted as methods at some points, hence the systematic parameters
bool            HasSubjects             ( const TGoGoF& subjects );
int             NumSubjects             ( const TGoGoF& subjects );
int             NumConditions           ( const TGoF&   subject  );
int             NumConditions           ( const TGoGoF& subjects );
int             NumClusters             ( const TGoGoF& subjects );
int             NumEpochs               ( const TGoF&   subject, const char* fileprefix );
int             NumEpochs               ( const TGoGoF& subjects, int subji, const char* fileprefix );

void            AddSubject              ( TGoGoF& subjects, const TGoF& subject );
void            AddSubjects             ( TGoGoF& subjects, const TGoGoF& moresubjects );
void            RemoveSubject           ( TGoGoF& subjects );
void            AddCondition            ( TGoGoF& subjects, const TGoF& onecondition );
void            AddConditions           ( TGoGoF& subjects, const TGoGoF& moresubjects );
void            RemoveCondition         ( TGoGoF& subjects );

bool            HasInverses             ( const TGoF& inverses );
int             NumInverses             ( const TGoF& inverses );
bool            IsSingleInverse         ( const TGoF& inverses );                           // single inverse matrix
bool            AreMatchingInverses     ( const TGoF& inverses, const TGoGoF& subjects );   // multiple inverse matrices AND same number as subjects

void            AddInverse              ( TGoF& inverses, const char* inverse );
void            AddInverses             ( TGoF& inverses, const TGoF& moreinverses );
void            RemoveInverse           ( TGoF& inverses );

bool            ReadCsvRis              ( const char* filename, TGoGoF& subjects, TGoF& inverses, VerboseType verbosey = Interactive );


//----------------------------------------------------------------------------

enum    ComputingRisPresetsEnum;
enum    SpatialFilterType;


//constexpr CentroidType  RisCentroidMethod = MedianCentroid;                   // Median gives sharper shapes/contours, counterpart is it basically forces to store all the data, and is more time-consuming to compute
constexpr CentroidType  RisCentroidMethod   = MeanCentroid;                     // maps can be quite empty after optional thresholding, a Median might be too radical while a Mean will still output something
//constexpr CentroidType  RisCentroidMethod = WeightedMeanCentroid;

constexpr FilterTypes   RisRoiMethod        = FilterTypeMean;                   // for the same reason as for centroids: merging clipped data with a Median can produce too much null results

constexpr FilterTypes   RisEnvelopeMethod   = FilterTypeEnvelopeGapBridging;    // results close to analytic, but can work with positive-only data


bool    ComputingRis    (   ComputingRisPresetsEnum esicase,
                            const TGoGoF&       subjects,                  
                            int                 numsubjects,            int                 numconditions,
                            
                            const TGoF&         inversefiles,           RegularizationType  regularization,     BackgroundNormalization     backnorm,
                            AtomType            datatypeepochs,         AtomType            datatypefinal,
                            CentroidType        centroidsmethod,

                            SpatialFilterType   spatialfilter,          const char*         xyzfile,
                            bool                ranking,
                            bool                thresholding,           double              keepingtopdata,
                            bool                envelope,               FilterTypes         envelopetype,       double          envelopeduration,
                            bool                roiing,                 const char*         roifile,            FilterTypes     roimethod,

                            bool                savingindividualfiles,  bool                savingepochfiles,   bool            savingzscorefactors,
                            bool                computegroupsaverages,  bool                computegroupscentroids,
                            const char*         basedir,                const char*         fileprefix,
                            VerboseType         verbose
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
