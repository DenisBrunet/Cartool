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
//----------------------------------------------------------------------------

constexpr char*     BadEpochsTitle                  = "Scanning Bad Epochs";
#define             InfixROC                        "ROC"
#define             InfixAbs                        "Abs"
#define             InfixNorm                       "Norm"

                                            // Main processing parameters:

                                            // 10..20
constexpr double    BadEpochsToleranceLiberal       = 0.20;
                                            // 20..30
constexpr double    BadEpochsToleranceRegular       = 0.30;
                                            // 40..50
constexpr double    BadEpochsToleranceStrict        = 0.45;

constexpr double    BadEpochsToleranceDefault       = BadEpochsToleranceStrict;
                                            // 60..70 would be the next level of strictness

                                        // We don't need much smapling frequency than that - plus it speeds up things a lot...
constexpr double    BadEpochsTargetSampling         = 125.0;

                                        // filtering duration / scaling for specific processing - between 500..1000
constexpr double    BadEpochsBadDuration            = 600.0;
                                        // merging criteria together - between 500..1000
//constexpr double    BadEpochsMergeDuration          = 1000.0;
constexpr double    BadEpochsMergeDuration          = 1500.0;
                                        // special merge at the borders, these parts are always suspicious so just kill a big hole around them
constexpr double    BadEpochsBorderDuration         = 2000.0;
                                        // merging meta-criteria together
constexpr double    BadEpochsFinalDuration          = 1000.0;

                                        // Force High-pass above this in all cases
constexpr double    BadEpochsMinFrequency           = 1.0;
                                        // Define the alpha range to be removed
constexpr double    BadEpochsAlphaMinFrequency      =  7.0;
constexpr double    BadEpochsAlphaMaxFrequency      = 13.0;
                                        // Current 2 bands for the analysis
constexpr double    BadEpochsBand1LowPass           =  9.0;
constexpr double    BadEpochsBand2HighPass          = 30.0;

                                        // faking our local filer type, so that only the maps will be Low-pass filtered for the Level criteria
constexpr FilterTypes FilterTypeAuto                = (FilterTypes) -1;


//----------------------------------------------------------------------------
                                        // Maps to criteria
                                        // Input can be either an existing TMaps* or a file name
                                        // Outputs are an array of TVector (criteria) + the actual final sampling frequency
                                        // Note that each input EEG can have a different sampling frequeny
constexpr int           ComputeCriteriaGaugeCount   = 5;


enum        BadEpochsCriteriaEnum
            {
                                        // Levels / Variances criteria
            BadEpochsMode1,
            BadEpochsMode2,
            BadEpochsMode3,
            BadEpochsMode4,
            BadEpochsMode5,
            BadEpochsMode6,
            BadEpochsVariance1,
            BadEpochsVariance2,
            BadEpochsVariance3,
            BadEpochsVariance4,
            BadEpochsVariance5,
            BadEpochsVariance6,
                                        // Noise components criteria
            BadEpochsAutoStepCorrelComp,
            BadEpochsAutoAvgCorrelComp,
            BadEpochsAutoAvgConvolComp,
            BadEpochsAutoStepCorrelNoise,
            BadEpochsAutoAvgCorrelNoise,
            BadEpochsAutoAvgConvolNoise,
            BadEpochsAutoCorrel1,
            BadEpochsAutoCorrel2,
            BadEpochsAutoCorrel3,
            BadEpochsAutoConvol1,
            BadEpochsAutoConvol2,
            BadEpochsAutoConvol3,
                                        // Meta-Criteria
            BadEpochsMetaCrit1,
            BadEpochsMetaCrit2,
            BadEpochsMetaCrit3,
            BadEpochsMetaCrit4,
            BadEpochsMetaCrit5,
            BadEpochsMetaCrit6,
            BadEpochsMetaCrit7,
            BadEpochsMetaCritAll,       // Final merge

            NumBadEpochsCriteria,

                                        // range of single criteria
            MinCriteriaTracks           = BadEpochsMode1,
            MaxCriteriaTracks           = BadEpochsAutoConvol3,
            NumCriteriaTracks           = MaxCriteriaTracks - MinCriteriaTracks + 1,
                                        // sub-ranges of criteria used for auto-correlation / convolution
            MinCorrelTrack              = BadEpochsAutoCorrel1,
            MaxCorrelTrack              = BadEpochsAutoCorrel3,
            MinConvolTrack              = BadEpochsAutoConvol1,
            MaxConvolTrack              = BadEpochsAutoConvol3,
                                        // ranges of criteria used for meta-criteria
            MinTrack1                   = BadEpochsMode1,
            MaxTrack1                   = BadEpochsVariance6,
            MinTrack2                   = BadEpochsAutoStepCorrelComp,
            MaxTrack2                   = BadEpochsAutoConvol3,
                                        // range of meta-criteria themselves
            MinMetaTrack                = BadEpochsMetaCrit1,
            MaxMetaTrack                = BadEpochsMetaCritAll,
            NumMetaTracks               = MaxMetaTrack - MinMetaTrack + 1,
            };


void    ComputeBadEpochsCriteria        (   const TMaps*        mapsin,                     const char*         filename,
                                            double              targetsamplingfrequency,
                                            FilterTypes         filtertype,                 const double*       freqcut,
                                            double              badduration,
                                            TVector<float>*     criteria,
                                            double&             samplingfrequency,
                                            TSuperGauge*        Gauge,
                                            TVector<float>*     rawcriteria     = 0
                                        );


//----------------------------------------------------------------------------
                                        // Compute criteria within 2 different bands
constexpr int       NumCriteriaBands                = 2;
constexpr int       ComputeCriteriaBandsGaugeCount  = NumCriteriaBands * ComputeCriteriaGaugeCount + 1;


void    ComputeBadEpochsCriteriaBands   (   const TMaps*        mapsin,                     const char*         filename,
                                            const TSelection*   ignoretracks,
                                            double              targetsamplingfrequency,
                                            double              badduration,
                                            TVector<float>*     criteria,
                                            double&             samplingfrequency,
                                            TSuperGauge*        Gauge
                                        );


//----------------------------------------------------------------------------

void    BinarizeCriterion   (   const TVector<float>&   trackin,
                                TVector<float>&         trackout,              
//                              double                  duration,           // [ms] duration we want to keep
//                              double                  samplingfrequency,
                                double                  alpha               // cutting value
                            );


void    CloseBorders        (   TVector<float>&         track,              
                                double                  duration,           // [ms] duration we want to keep
                                double                  samplingfrequency
                            );


void    MaxPerSegment       (   TVector<float>&         track   );


//----------------------------------------------------------------------------

void    ProcessBadEpochsMetaCriterion   (   const TVector<float>*   criteria,
                                            const TSelection&       critsel,
                                            double                  alpha,
                                            double                  mergeduration,  double          borderduration,
                                            double                  samplingfrequency,
                                            TVector<float>&         metacriterion
                                        );


//----------------------------------------------------------------------------

constexpr int       NumCriteriaBandsTracks          = NumCriteriaBands * NumCriteriaTracks;
constexpr int       TotalCriteriaBandsTracks        = NumCriteriaBandsTracks + NumMetaTracks;

                                        // indexes of meta-criteria, after all bands of criteria
enum                MetaCriteriaIndex
                    {
                    MetaCrit1i                      = NumCriteriaBandsTracks,
                    MetaCrit2i,
                    MetaCrit3i,
                    MetaCrit4i,
                    MetaCrit5i,
                    MetaCrit6i,
                    MetaCrit7i,
                    MetaCritAlli
                    };

                                        // which number of criteria are retained
extern const int    CriteriaBetaCuts[ NumMetaTracks - 1 ];


#define             BadEpochsMetaCritAllName        "MC"
extern const char   BadEpochsCriterionName[ NumBadEpochsCriteria ][ 16 ];


enum        CenterSdlSdrEnum
            {
            Center,
            SpreadLeft,
            SpreadRight,

            NumCenterSdlSdrEnum
            };
                                                    // Real calibration for adaptive Merge Duration
extern const double BadEpochsCriterionCalib[ NumCriteriaBandsTracks ][ NumCenterSdlSdrEnum ];

                                        // numcrit starting from 1 to 48
inline  double  GetAlphaCut ( int numcrit, double alpha )   {   return                          BadEpochsCriterionCalib[ numcrit - 1 ][ Center      ] 
                                                                        + alpha * ( alpha < 0 ? BadEpochsCriterionCalib[ numcrit - 1 ][ SpreadLeft  ] 
                                                                                              : BadEpochsCriterionCalib[ numcrit - 1 ][ SpreadRight ] ); }


void    ProcessBadEpochsMetaCriteria    (   TVector<float>*         criteria,
                                            double                  alpha,
                                            double                  mergeduration,  double          borderduration,
                                            double                  samplingfrequency
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
