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

#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.BatchAveragingFiles.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TFilters.h"
#include    "TVector.h"
#include    "Math.Utils.h" 
#include    "Math.Stats.h"
#include    "Math.Histo.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "GlobalOptimize.Tracks.h" 

#include    "TMarkers.h"
#include    "BadEpochs.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const int   CriteriaBetaCuts[ NumMetaTracks - 1 ]  = { 1, 6, 12, 18, 24, 30, 36 };


const char  BadEpochsCriterionName[ NumBadEpochsCriteria ][ 16 ] =
            {
            "Mode1"                       ,
            "Mode2"                       ,
            "Mode3"                       ,
            "Mode4"                       ,
            "Mode5"                       ,
            "Mode6"                       ,
            "Var1"                        ,
            "Var2"                        ,
            "Var3"                        ,
            "Var4"                        ,
            "Var5"                        ,
            "Var6"                        ,
            "Aut1CorC"                    ,
            "AutACorC"                    ,
            "AutACnvC"                    ,
            "Aut1CorN"                    ,
            "AutACorN"                    ,
            "AutACnvN"                    ,
            "AutCor1"                     ,
            "AutCor2"                     ,
            "AutCor3"                     ,
            "AutCnv1"                     ,
            "AutCnv2"                     ,
            "AutCnv3"                     ,

            BadEpochsMetaCritAllName "01" ,
            BadEpochsMetaCritAllName "06" ,
            BadEpochsMetaCritAllName "12" ,
            BadEpochsMetaCritAllName "18" ,
            BadEpochsMetaCritAllName "24" ,
            BadEpochsMetaCritAllName "30" ,
            BadEpochsMetaCritAllName "36" ,

            BadEpochsMetaCritAllName "All",
            };


const double BadEpochsCriterionCalib[ NumCriteriaBandsTracks ][ NumCenterSdlSdrEnum ] = {
        {   2.47,   0.66,   4.06    },
        {   2.48,   0.67,   3.99    },
        {   2.48,   0.66,   3.97    },
        {   2.45,   0.66,   3.9     },
        {   2.4 ,   0.63,   3.84    },
        {   2.37,   0.62,   3.67    },
        {   2.31,   0.58,   3.54    },
        {   2.28,   0.56,   3.36    },
        {   2.2 ,   0.53,   3.18    },
        {   2.12,   0.51,   3.03    },
        {   2.09,   0.52,   2.82    },
        {   2.06,   0.51,   2.62    },
        {   2.01,   0.48,   2.44    },
        {   1.95,   0.47,   2.32    },
        {   1.89,   0.44,   2.17    },
        {   1.87,   0.43,   2.04    },
        {   1.8 ,   0.41,   1.92    },
        {   1.78,   0.41,   1.84    },
        {   1.74,   0.4 ,   1.69    },
        {   1.65,   0.35,   1.54    },
        {   1.6 ,   0.35,   1.42    },
        {   1.57,   0.33,   1.38    },
        {   1.52,   0.3 ,   1.28    },
        {   1.46,   0.3 ,   1.23    },
        {   1.43,   0.3 ,   1.14    },
        {   1.36,   0.29,   1.11    },
        {   1.31,   0.25,   1.1     },
        {   1.28,   0.23,   1.07    },
        {   1.23,   0.24,   1.02    },
        {   1.19,   0.23,   0.96    },
        {   1.16,   0.23,   0.92    },
        {   1.1 ,   0.22,   0.89    },
        {   1.05,   0.22,   0.82    },
        {   1.01,   0.22,   0.79    },
        {   0.98,   0.21,   0.76    },
        {   0.92,   0.19,   0.71    },
        {   0.89,   0.21,   0.68    },
        {   0.86,   0.2 ,   0.63    },
        {   0.8 ,   0.18,   0.63    },
        {   0.77,   0.17,   0.6     },
        {   0.71,   0.18,   0.61    },
        {   0.68,   0.15,   0.55    },
        {   0.61,   0.16,   0.51    },
        {   0.57,   0.15,   0.51    },
        {   0.51,   0.14,   0.46    },
        {   0.44,   0.13,   0.44    },
        {   0.37,   0.12,   0.41    },
        {   0.32,   0.11,   0.3     }
        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // debugging flags
//#define             BadEpochsSaveSteps

#if defined(BadEpochsSaveSteps)
TFileName           _basefile;
#endif


//----------------------------------------------------------------------------
                                        // Maps to criteria
void    ComputeBadEpochsCriteria    (   const TMaps*        mapsin,
                                        const char*         filename,                   int                 session,
                                        double              targetsamplingfrequency,
                                        FilterTypes         filtertype,                 const double*       freqcut,
                                        double              badduration,
                                        TVector<float>*     criteria,
                                        double&             samplingfrequency,
                                        TSuperGauge*        Gauge,
                                        TVector<float>*     rawcriteria
                                    )
{
if ( ( mapsin == 0 && StringIsEmpty ( filename ) )
   || criteria == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // either use the provided maps, or load from file
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );


const TMaps*        tomapsin        = mapsin    ? mapsin
                                                : new TMaps ( filename, session, AtomTypeScalar, ReferenceNone );

double              samplingfrequencyin= tomapsin->GetSamplingFrequency ();

if ( samplingfrequencyin <= 0 )

    return;

                                        // we can heavily downsample, targetting 125[Hz] but not lower
int                 downsampling        = AtLeast ( 1, Truncate ( samplingfrequencyin / targetsamplingfrequency ) );

                    samplingfrequency   = samplingfrequencyin / downsampling;

//DBGV4 ( samplingfrequencyin, BadEpochsTargetSampling, downsampling, samplingfrequency, "samplingfrequencyin, BadEpochsTargetSampling, downsampling, samplingfrequency" );


TMaps               maps ( *tomapsin, downsampling );


if ( mapsin == 0 )
                                        // locally allocated - we can delete them now
    delete  tomapsin;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 timemin             = 0;
int                 timemax             = maps.GetNumMaps () - 1;
int                 numtf               = timemax - timemin + 1;
int                 numel               = maps.GetDimension ();


int                 requiredtf          = MillisecondsToTimeFrame ( badduration, samplingfrequency ) + 3;

if ( numtf < requiredtf )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time filtering
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );


FctParams           params;

                                        // Remove any DC, by safety, before any high-pass filtering
maps.TimeCentering ();


if      ( filtertype == FilterTypeLowPass ) {
                                        // Low pass becomes Band pass 1..lowfreq
    params ( FilterParamOrder )         = 2;
    params ( FilterParamFreqCutMin )    =       BadEpochsMinFrequency;
    params ( FilterParamFreqCutMax )    = freqcut[ 0 ];

    maps.FilterTime ( FilterTypeBandPass, params );
    }

else if ( filtertype == FilterTypeHighPass ) {
                                        // High pass with hard low limit of 1[Hz]
    params ( FilterParamOrder )         = 4;
    params ( FilterParamFreqCut )       = max ( BadEpochsMinFrequency, freqcut[ 0 ] );

    maps.FilterTime ( FilterTypeHighPass, params );
    }

else if ( filtertype == FilterTypeBandPass ) {
                                        // Band pass with hard low limits
    params ( FilterParamOrder )         = 2;
    params ( FilterParamFreqCutMin )    = max ( BadEpochsMinFrequency, freqcut[ 0 ] );
    params ( FilterParamFreqCutMax )    = max ( BadEpochsMinFrequency, freqcut[ 1 ] );

    maps.FilterTime ( FilterTypeBandPass, params );
    }

else if ( filtertype == FilterTypeBandStop ) {
                                        // first High pass with low limit of 1[Hz]
    params ( FilterParamOrder )         = 4;
    params ( FilterParamFreqCut )       =       BadEpochsMinFrequency;

    maps.FilterTime ( FilterTypeHighPass, params );

                                        // then Band stop
    params ( FilterParamOrder )         = 2;
    params ( FilterParamFreqCutMin )    = max ( BadEpochsMinFrequency, freqcut[ 0 ] );
    params ( FilterParamFreqCutMax )    = max ( BadEpochsMinFrequency, freqcut[ 1 ] );

    maps.FilterTime ( FilterTypeBandStop, params );
    }
/*
                                        // Used to be a High-Pass 1 then Band-Stop 7-13
else if ( filtertype == FilterTypeAuto ) {
                                        // High pass with hard low limit of 1[Hz]
    params ( FilterParamOrder )         = 4;
    params ( FilterParamFreqCut )       =       BadEpochsMinFrequency;

    maps.FilterTime ( FilterTypeHighPass, params );

                                        // then Band stop
    params ( FilterParamOrder )         = 2;
    params ( FilterParamFreqCutMin )    = BadEpochsAlphaMinFrequency;
    params ( FilterParamFreqCutMax )    = BadEpochsAlphaMaxFrequency;

    maps.FilterTime ( FilterTypeBandStop, params );
    }
*/
else if ( filtertype == FilterTypeAuto ) {
                                        // High pass
    params ( FilterParamOrder )         = 4;
    params ( FilterParamFreqCut )       = BadEpochsBand2HighPass;

    maps.FilterTime ( FilterTypeHighPass, params );
    }

else if ( filtertype == FilterTypeNone ) {
    }

else {
    ShowMessage ( "Filter not implemented", FilterPresets[ filtertype ].Text, ShowMessageWarning );
    exit ( 1 );
    }

                                        // Here, we are sure to have a High-Pass above 1[Hz] (if any filter was required)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Pre-processing
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // Force Average Reference before Z-Scoring - to avoid big slow waves in the data
maps.AverageReference   ( AtomTypeScalar );

                                        // Force each track to have the same SD / power
maps.ZScore             ( ZScoreSigned_CenterScale );

                                        // to be 100% correct - but not a lot of differences
maps.AverageReference   ( AtomTypeScalar );


#if defined(BadEpochsSaveSteps)

bool                savepreproc     = false && StringIsNotEmpty ( filename );

if ( savepreproc ) {

    TFileName           fileproc ( filename, TFilenameNoPreprocessing );

    RemoveExtension ( fileproc );
    StringAppend    ( fileproc, ".PreProc1" );
    AddExtension    ( fileproc, FILEEXT_EEGSEF );

    maps.WriteFile   ( fileproc );
    }
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocate all tracks for all remaining processing
for ( int beci = 0; beci < NumBadEpochsCriteria; beci++ )

    criteria [ beci ].Resize ( numtf );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting some statistics on each TF
TEasyStats          mapstat   ( numel );

double              mode2;
double              mode3;
double              mad;
double              iqr;
double              madleft;
double              madright;


for ( int tfi = 0; tfi < numtf; tfi++ ) {

    if ( Gauge )    Gauge->Actualize ();

                                        // stats on temporal standardized tracks
    mapstat.Set ( maps[ tfi ], true );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    mode2       = mapstat.MaxModeHRM ();
    mode3       = mapstat.MaxModeHSM ();

    mad         = mapstat.MAD ( mode2 );
    iqr         = mapstat.InterQuartileRange ();

    mapstat.MADAsym ( mode2, madleft, madright );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Modes
    criteria[ BadEpochsMode1     ] ( tfi )  = mode2;

    criteria[ BadEpochsMode2     ] ( tfi )  = mode3;
                                        // ?we had average referenced - is that residual some noise from the average, which happen to still be discriminative?
    criteria[ BadEpochsMode3     ] ( tfi )  = mapstat.Mean ();

    criteria[ BadEpochsMode4     ] ( tfi )  = mapstat.Median ( false );

    criteria[ BadEpochsMode5     ] ( tfi )  = mapstat.MaxModeHistogram ();

    criteria[ BadEpochsMode6     ] ( tfi )  = mapstat.InterQuartileMean ();
                                        // looks more related to Skewness / Kurtosis family
//  criteria[ BadEpochsMode7     ] ( tfi )  = RelativeDifference ( mapstat.Quantile ( 0.25 ), mapstat.Quantile ( 0.75 ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Variances
    criteria[ BadEpochsVariance1 ] ( tfi )  = mad;

    criteria[ BadEpochsVariance2 ] ( tfi )  = iqr;

    criteria[ BadEpochsVariance3 ] ( tfi )  = fabs ( mapstat.Max () ) - fabs ( mapstat.Min () );

    criteria[ BadEpochsVariance4 ] ( tfi )  = mapstat.Sn ( 500 );

    criteria[ BadEpochsVariance5 ] ( tfi )  = madleft - madright;

    criteria[ BadEpochsVariance6 ] ( tfi )  = iqr - mad;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for tf


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // bank of correlation/convolution scales
#define             AutoCorrWindowStep         4.0
#define             AutoCorrWindowShort        8.0
#define             AutoCorrWindowSize1       15.0
#define             AutoCorrWindowSize2       31.0
#define             AutoCorrWindowSize3       62.0
//#define           AutoCorrWindowSize4      125.0
//#define           AutoCorrWindowSize5      250.0
//#define           AutoCorrWindowSize6      500.0
//#define           AutoCorrWindowSize7     1000.0

int                 autocorrwindshort   = AtLeast ( 1, Truncate ( MillisecondsToTimeFrame ( AutoCorrWindowShort, samplingfrequency ) ) );
int                 autocorrwind1       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize1 / 2.0, samplingfrequency ) ) );    // > short interval above
int                 autocorrwind2       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize2 / 2.0, samplingfrequency ) ) );
int                 autocorrwind3       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize3 / 2.0, samplingfrequency ) ) );
//int               autocorrwind4       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize4 / 2.0, samplingfrequency ) ) );
//int               autocorrwind5       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize5 / 2.0, samplingfrequency ) ) );
//int               autocorrwind6       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize6 / 2.0, samplingfrequency ) ) );
//int               autocorrwind7       = AtLeast ( 1, Round    ( MillisecondsToTimeFrame ( AutoCorrWindowSize7 / 2.0, samplingfrequency ) ) );
int                 autocorrwind        = autocorrwind3;                                                                                        // max half-window size
int                 autocorrstep        = AtLeast ( 1, Truncate ( MillisecondsToTimeFrame ( AutoCorrWindowStep, samplingfrequency ) ) );        // speeding up things for higher sampling frequencies
double              corr1;
double              corr2;
double              corrl;
double              convl;

//DBGV6 ( autocorrwind1, autocorrwind2, autocorrwind3, autocorrwind4, autocorrwind5, autocorrstep, "autocorrwind 1 to 5, autocorrstep" );


for ( int tfi = 0; tfi < numtf; tfi++ ) {

    if ( Gauge )    Gauge->Actualize ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Min Correlation on a narrow interval: a highly decorrelated close neighbor (+-1 TF) is fishy
    corr1   = maps ( tfi ).Correlation ( maps ( LeftMirroring  ( tfi, autocorrwindshort, numtf - 1 ) ) );
    corr2   = maps ( tfi ).Correlation ( maps ( RightMirroring ( tfi, autocorrwindshort, numtf - 1 ) ) );

                                        // if ranking, we can skip the correction
    criteria[ BadEpochsAutoStepCorrelComp  ] ( tfi )  = PearsonToFisher ( max ( fabs ( corr1 ), fabs ( corr2 ) ) );
    criteria[ BadEpochsAutoStepCorrelNoise ] ( tfi )  = PearsonToFisher ( min ( fabs ( corr1 ), fabs ( corr2 ) ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sum up all correlations within varying window sizes
    for ( int aci = 1; aci <= autocorrwind; aci += autocorrstep ) {

        corr1   = maps ( tfi ).Correlation ( maps ( LeftMirroring  ( tfi, aci, numtf - 1 ) ) );
        corr2   = maps ( tfi ).Correlation ( maps ( RightMirroring ( tfi, aci, numtf - 1 ) ) );

                                        // deskew data
        corrl   = PearsonToFisher ( fabs ( corr1 ) ) 
                + PearsonToFisher ( fabs ( corr2 ) );

                                        // Note: in term of time scales, BadEpochsAutoConvol1 ~= BadEpochsAutoCorrel2, Convolution is 2 times "broader"
                                        // Use both a different summation and a different Correlation formula
        convl   = PearsonToFisher ( fabs (                       maps ( LeftMirroring  ( tfi, aci, numtf - 1 ) ).
                                           CorrelationSpearman ( maps ( RightMirroring ( tfi, aci, numtf - 1 ) ), true ) ) );


                                        // this sequence of tests allows to skip the other tests (shorter windows)
        criteria[ BadEpochsAutoCorrel3 ] ( tfi )  += corrl;
        criteria[ BadEpochsAutoConvol3 ] ( tfi )  += convl;

        if ( aci > autocorrwind2 )      continue;

        criteria[ BadEpochsAutoCorrel2 ] ( tfi )  += corrl;
        criteria[ BadEpochsAutoConvol2 ] ( tfi )  += convl;

        if ( aci > autocorrwind1 )      continue;

        criteria[ BadEpochsAutoCorrel1 ] ( tfi )  += corrl;
        criteria[ BadEpochsAutoConvol1 ] ( tfi )  += convl;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for tf

                                        // averaging more makes more smoothing, dividing by sqrt(interval) instead of interval looks more comparable in absolute
criteria[ BadEpochsAutoCorrel1 ]   /= sqrt ( (double) autocorrwind1 );
criteria[ BadEpochsAutoCorrel2 ]   /= sqrt ( (double) autocorrwind2 );
criteria[ BadEpochsAutoCorrel3 ]   /= sqrt ( (double) autocorrwind3 );
criteria[ BadEpochsAutoConvol1 ]   /= sqrt ( (double) autocorrwind1 );
criteria[ BadEpochsAutoConvol2 ]   /= sqrt ( (double) autocorrwind2 );
criteria[ BadEpochsAutoConvol3 ]   /= sqrt ( (double) autocorrwind3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

maps.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Main tracks are computed, post-process them
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( rawcriteria ) {

    for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ )

        rawcriteria [ beci ]    = criteria [ beci ];

                                        // not raw anymore, but otherwise, merging doesn't make any sense...
    rawcriteria[ BadEpochsAutoAvgCorrelComp     ]   = -FLT_MAX;
    rawcriteria[ BadEpochsAutoAvgCorrelNoise    ]   =  FLT_MAX;

    for ( int beci = MinCorrelTrack; beci <= MaxCorrelTrack; beci++ ) {

//        rawcriteria[ beci ].ToRank ();

        rawcriteria[ BadEpochsAutoAvgCorrelComp  ].Maxed ( rawcriteria[ beci ] );
        rawcriteria[ BadEpochsAutoAvgCorrelNoise ].Mined ( rawcriteria[ beci ] );
        }


    rawcriteria[ BadEpochsAutoAvgConvolComp     ]   = -FLT_MAX;
    rawcriteria[ BadEpochsAutoAvgConvolNoise    ]   =  FLT_MAX;

    for ( int beci = MinConvolTrack; beci <= MaxConvolTrack; beci++ ) {

//        rawcriteria[ beci ].ToRank ();

        rawcriteria[ BadEpochsAutoAvgConvolComp  ].Maxed ( rawcriteria[ beci ] );
        rawcriteria[ BadEpochsAutoAvgConvolNoise ].Mined ( rawcriteria[ beci ] );
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Post-processing some criterion by ranking
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );


for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ ) {
                                        // done later
    if      ( beci == BadEpochsAutoAvgCorrelComp
           || beci == BadEpochsAutoAvgCorrelNoise
           || beci == BadEpochsAutoAvgConvolComp
           || beci == BadEpochsAutoAvgConvolNoise
            )
        continue;

                                        // Z-Score then abs
    criteria[ beci ].ZScoreAbsolute ();

//                                      // merging negative and positive parts together / already positive only
//  criteria[ beci ].Absolute ();
//
//  criteria[ beci ].ToRank ( false );  // ignoring nulls
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All single criteria have been ranked - merging them now + ranking results
criteria[ BadEpochsAutoAvgCorrelComp    ]   = -FLT_MAX;
criteria[ BadEpochsAutoAvgCorrelNoise   ]   =  FLT_MAX;

for ( int beci = MinCorrelTrack; beci <= MaxCorrelTrack; beci++ ) {

    criteria[ BadEpochsAutoAvgCorrelComp  ].Maxed ( criteria[ beci ] );
    criteria[ BadEpochsAutoAvgCorrelNoise ].Mined ( criteria[ beci ] );
    }

//criteria[ BadEpochsAutoAvgCorrelComp  ].ToRank ( false );   // looking at the high values, i.e. very highly correlated can also be fishy (blinks)
//criteria[ BadEpochsAutoAvgCorrelNoise ].ToRank ( false );
criteria[ BadEpochsAutoAvgCorrelComp  ].ZScoreAbsolute ();
criteria[ BadEpochsAutoAvgCorrelNoise ].ZScoreAbsolute ();


criteria[ BadEpochsAutoAvgConvolComp    ]   = -FLT_MAX;
criteria[ BadEpochsAutoAvgConvolNoise   ]   =  FLT_MAX;

for ( int beci = MinConvolTrack; beci <= MaxConvolTrack; beci++ ) {

    criteria[ BadEpochsAutoAvgConvolComp  ].Maxed ( criteria[ beci ] );
    criteria[ BadEpochsAutoAvgConvolNoise ].Mined ( criteria[ beci ] );
    }

//criteria[ BadEpochsAutoAvgConvolComp  ].ToRank ( false );
//criteria[ BadEpochsAutoAvgConvolNoise ].ToRank ( false );
criteria[ BadEpochsAutoAvgConvolComp  ].ZScoreAbsolute ();
criteria[ BadEpochsAutoAvgConvolNoise ].ZScoreAbsolute ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // testing different levels of smoothing
TStrings            criterianames;

for ( int beci = 0; beci < NumBadEpochsCriteria; beci++ )
    criterianames.Add ( BadEpochsCriterionName[ beci ] );


TFileName           _basefile;
TFileName           _file;

StringCopy      ( _basefile, "E:\\Data\\BadEpochs." );
StringRandom    ( StringEnd ( _basefile ), 3 );


//int                 numtf               = criteria[ 0 ].GetDim ();
TArray2<float>      criteriatracks  ( NumCriteriaTracks, numtf );
int                 critmemsize     = criteria[ 0 ].MemorySize ();
FctParams           p;

criteriatracks.Index2.IndexRatio   = samplingfrequency;


for ( int smooth = 0; smooth <= 1000; smooth += 100 ) {
                                        // copy criteria tracks
    for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ )
        CopyVirtualMemory   ( criteriatracks[ beci ], criteria[ beci ], critmemsize );


    p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( smooth, samplingfrequency ) );

    criteriatracks.FilterDim2 ( FilterTypeGaussian, p );


    StringCopy          ( _file, _basefile, "." "CriteriaTracks", ".Smooth", IntegerToString ( smooth, 4 ), "." FILEEXT_EEGSEF );
    criteriatracks.WriteFile    ( _file, &criterianames );

    Cartool.CartoolDocManager->OpenDoc ( _file, dtOpenOptions );
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smoothing all these guys - after standardization / merging
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );


if ( badduration > 0 ) {

    FctParams           p;

    p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( badduration, samplingfrequency ) );

                                        // Spread each criterion power, so that we can look for either short & strong peaks, or long & weaker bursts
    for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ ) {

        criteria[ beci ].Filter ( FilterTypeGaussian, p );
                                        // final update Z-Scoring
        criteria[ beci ].ZScoreAsym ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have all raw criteria tracks
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}


//----------------------------------------------------------------------------
                                        // Compute criteria in 2 different bands
void    ComputeBadEpochsCriteriaBands(  const TMaps*        mapsin,
                                        const char*         filename,                   int                 session,
                                        const TSelection*   ignoretracks,
                                        double              targetsamplingfrequency,
                                        double              badduration,
                                        TVector<float>*     criteria,
                                        double&             samplingfrequency,
                                        TSuperGauge*        Gauge
                                    )
{
if ( ( mapsin == 0 && StringIsEmpty ( filename ) )
   || criteria == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // either use the provided maps, or load from file
if ( Gauge )    Gauge->Next ( -1, SuperGaugeUpdateTitle );


bool                isesi               = IsExtensionAmong ( filename, AllRisFilesExt );

AtomType            datatype            = isesi ? AtomTypePositive : AtomTypeScalar;
                                                                                       // no ref for ESI
ReferenceType       dataref             = ReferenceNone; // GetProcessingRef ( isesi ? ProcessingReferenceESI : ProcessingReferenceNone );

const TMaps*        tomapsin            = mapsin    ? mapsin
                                                    : new TMaps ( filename, session, datatype, dataref );

double              samplingfrequencyin= tomapsin->GetSamplingFrequency ();

if ( samplingfrequencyin <= 0 )

    return;

                                        // we can heavily downsample, targetting 125[Hz] but not lower
int                 downsampling        = AtLeast ( 1, Truncate ( samplingfrequencyin / targetsamplingfrequency ) );

                    samplingfrequency   = samplingfrequencyin / downsampling;

//DBGV4 ( samplingfrequencyin, BadEpochsTargetSampling, downsampling, samplingfrequency, "samplingfrequencyin, BadEpochsTargetSampling, downsampling, samplingfrequency" );


TMaps               maps ( *tomapsin, downsampling, ignoretracks );


if ( mapsin == 0 )
                                        // locally allocated - we can delete them now
    delete  tomapsin;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // currently 2 bands
FilterTypes         filtertype1         = FilterTypeLowPass;
double              filtercut1          = BadEpochsBand1LowPass;

FilterTypes         filtertype2         = FilterTypeHighPass;
double              filtercut2          = BadEpochsBand2HighPass;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<float>      criteriab [ NumBadEpochsCriteria ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First frequency band
ComputeBadEpochsCriteria    (   &maps,
                                0,                          0,
                                targetsamplingfrequency,                    // downsampling done once for all
                                filtertype1,                &filtercut1,    // first filter
                                badduration,
                                criteriab,
                                samplingfrequency,
                                Gauge
                            );


for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ )

    criteria[                     beci ]    = criteriab[ beci ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Second frequency band
ComputeBadEpochsCriteria    (   &maps,
                                0,                          0,
                                targetsamplingfrequency,                    // downsampling done once for all
                                filtertype2,                &filtercut2,    // second filter
                                badduration,
                                criteriab,
                                samplingfrequency,
                                Gauge
                            );


for ( int beci = MinCriteriaTracks; beci <= MaxCriteriaTracks; beci++ )

    criteria[ NumCriteriaTracks + beci ]    = criteriab[ beci ];

}


//----------------------------------------------------------------------------
                                        // Output results are binarized 0/1
void    BinarizeCriterion   (   const TVector<float>&   trackin,
                                TVector<float>&         trackout,              
//                              double                  duration,           // [ms] duration we want to keep
//                              double                  samplingfrequency,
                                double                  alpha               // cutting value
                            )
{
if ( trackin.IsNotAllocated () )
    return;


//TVector<float>      fulltrack       ( trackin );
                    trackout        = trackin;  // OK even if they are the same
int                 numtf           = trackin.GetDim ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // threshold and binarize at once
for ( int i = 0; i < numtf; i++ )

    trackout[ i ]   = trackout[ i ] >= alpha;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Used when there were much fewer criteria
                                        // Problem is that it cumulates with the smoothing already done at the criteria computation time, kind of doubling it. It also makes the criteria behave less linearly, making the ROC curves estimation a bit tricky
                                        // Let's maybe postpone any morphology closing up until to the very end, at the Grand Merging time.
/*                                        // fill gaps and expand borders for safety
p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( duration, samplingfrequency ) );

trackout.Filter ( FilterTypeMax, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smart finish: expand borders while the original tracks is still decreasing (cutting at the hollows)
p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( duration, samplingfrequency ) );

fulltrack.Filter ( FilterTypeGaussian, p );

                                        // extended track starts as the original track
TVector<float>      trackext ( trackout );

                                        // loop with margin of 1 TF
for ( int tfi = 1; tfi < numtf; tfi++ ) {
                                        // found onset?
    if ( ! ( trackout ( tfi ) && ! trackout ( tfi - 1 ) ) )

        continue;

                                        // expand left by going backward
    for ( int tfi2 = tfi - 1; tfi2 >= 0; tfi2-- )
                                        // still decreasing?
        if ( fulltrack ( tfi2 ) < fulltrack ( tfi2 + 1 ) )
                                        // expand left
            trackext ( tfi2 )   = trackout ( tfi );
        else
            break;
    }


for ( int tfi = numtf - 2; tfi >= 0; tfi-- ) {
                                        // found offset?
    if ( ! ( trackout ( tfi ) && ! trackout ( tfi + 1 ) ) )

        continue;

                                        // expand right by going forward
    for ( int tfi2 = tfi + 1; tfi2 < numtf; tfi2++ )
                                        // still decreasing?
        if ( fulltrack ( tfi2 ) < fulltrack ( tfi2 - 1 ) )
                                        // expand right
            trackext ( tfi2 )   = trackout ( tfi );
        else
            break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // there it is, the extended track
trackout    = trackext;
*/
}


//----------------------------------------------------------------------------
void    CloseBorders    (   TVector<float>&         track,              
                            double                  duration,           // [ms] duration we want to keep
                            double                  samplingfrequency
                        )
{
if ( track.IsNotAllocated () )
    return;


int                 borderttf       = MillisecondsToTimeFrame ( duration, samplingfrequency );
int                 numtf           = track.GetDim ();
float               closeleft       = 0;
float               closeright      = 0;

                                        // search for any value with the border
for ( int tfi = 0, tfli = 0, tfri = numtf - 1; tfi < borderttf; tfi++, tfli++, tfri-- ) {

    Maxed ( closeleft,  track ( tfli ) );
    Maxed ( closeright, track ( tfri ) );
    }

                                        // clip the whole border if anything lies within it
for ( int tfi = 0, tfli = 0, tfri = numtf - 1; tfi < borderttf; tfi++, tfli++, tfri-- ) {

    if ( closeleft  )   track ( tfli )  = closeleft;
    if ( closeright )   track ( tfri )  = closeright;
    }
}


//----------------------------------------------------------------------------
void    MaxPerSegment   (   TVector<float>&         track
                        )
{
if ( track.IsNotAllocated () )
    return;


int                 numtf           = track.GetDim ();

                                        // set each epoch to its local max - could also be the Median / Mean?
                                        // maybe put this in a proper Filter?
for ( int tfi1 = 0; tfi1 < numtf; tfi1++ ) {
                                        // found onset?
    if ( ! ( track ( tfi1 ) != 0 && ( tfi1 == 0 || track ( tfi1 - 1 ) == 0 ) ) )

        continue;

                                        // search for offset
    for ( int tfi2 = tfi1; tfi2 < numtf; tfi2++ ) 
                                        // found offset?
        if ( track ( tfi2 ) != 0 && ( tfi2 == numtf - 1 || track ( tfi2 + 1 ) == 0 ) ) {

                                        // get max value for that segment
            float       maxseg      = 0;

            for ( int tfi = tfi1; tfi <= tfi2; tfi++ )

                Maxed ( maxseg, track ( tfi ) );

                                        // spread max value to the whole segment
            for ( int tfi = tfi1; tfi <= tfi2; tfi++ )

                track ( tfi )   = maxseg;

                                        // jump past current end
            tfi1    = tfi2;

            break;
            }

    }
}


//----------------------------------------------------------------------------
                                        // Compares how (this) fits the other list, which supposedly contains a training set
                                        // it needs the real number of total TF to estimate correctly all variables
void    TMarkers::TestingAgainst    (   const TMarkers&     trainingset, 
                                        long                numtf,
                                        double&             truepositiverate,
                                        double&             truenegativerate,
                                        double&             falsepositiverate,
                                        double&             falsenegativerate
                                    )   const
{
truepositiverate    = 0;
truenegativerate    = 1;
falsepositiverate   = 0;
falsenegativerate   = 1;

if ( IsEmpty () || trainingset.IsEmpty () )
    return;


TVector<bool>       projecttest;
TVector<bool>       projecttraining;

                                        // project into a linear time-line
            ProjectToVector ( projecttest,     numtf );

trainingset.ProjectToVector ( projecttraining, numtf );



//int                 numtest         = 0;
//int                 numtraining     = 0;
int                 numtruepos      = 0;
int                 numtrueneg      = 0;
int                 numfalsepos     = 0;
int                 numfalseneg     = 0;


for ( long tf = 0; tf < numtf; tf++ ) {

//  if (   projecttest     [ tf ] )                         numtest++;
//  if (   projecttraining [ tf ] )                         numtraining++;

    if (   projecttest[ tf ] &&   projecttraining[ tf ] )   numtruepos ++;
    if (   projecttest[ tf ] && ! projecttraining[ tf ] )   numfalsepos++;
    if ( ! projecttest[ tf ] && ! projecttraining[ tf ] )   numtrueneg ++;
    if ( ! projecttest[ tf ] &&   projecttraining[ tf ] )   numfalseneg++;
    }

                                        // sensitivity - ratio of actual positive correct guesses / avoiding false negative
truepositiverate    = numtruepos + numfalseneg > 0  ? numtruepos  / (double) ( numtruepos + numfalseneg ) 
                                                    : 0;
                                        // specificity - ratio of actual negative correct guesses / avoiding false positive
truenegativerate    = numtrueneg + numfalsepos > 0  ? numtrueneg  / (double) ( numtrueneg + numfalsepos ) 
                                                    : 0;
                                        // error type I  - prob. of false alarm - how many positive guesses were wrong
falsepositiverate   = 1 - truenegativerate;

                                        // error type II - miss rate - how many negative guesses were wrong
falsenegativerate   = 1 - truepositiverate;

                                        // differentiating both the true positive and true negative
//double      accuracy    = ( numtruepos + numtrueneg ) / ( numtruepos + numtrueneg + numfalsepos + numfalseneg );
}

/*
double  TMarkers::Overlap   (   const TMarkers&     trainingset )
{
double              globaloverlap   = 0;
double              markeroverlap;

                                        // looking from each this marker..
for ( int fromi = 0; fromi < GetNumMarkers (); fromi++ ) {

    markeroverlap  = 0;
                                        // ..to how well it matches the target list
    for ( int toi = 0; toi < markers.GetNumMarkers (); toi++ )

        Maxed ( markeroverlap, markers[ toi ]->Overlap ( *(Markers[ fromi ]), how ) );

                                        // cumulate the best overlap for current from marker
    globaloverlap  += markeroverlap;
    } // for templist


globaloverlap  /= NonNull ( GetNumMarkers () );

return  globaloverlap;
}
*/

//----------------------------------------------------------------------------
                                        // Process a bunch of criteria together to produce a meta-criterion
                                        // There is a safety measure to skip criteria with nearly all responses on
                                        // The output parameter  metacriterion  can safely be part of the input selection
void    ProcessBadEpochsMetaCriterion   (   const TVector<float>*   criteria,
                                            const TSelection&       critsel,
                                            double                  alpha,
                                            double                  mergeduration,  double          borderduration,
                                            double                  samplingfrequency,
                                            TVector<float>&         metacriterion
                                        )
{
if ( criteria == 0 || (int) critsel == 0 ) {

    metacriterion.ResetMemory ();
    return;
    }


int                 numtf               = criteria[ 0 ].GetDim ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Process criteria with specified cutting value alpha
//Gauge.Next ( -1, SuperGaugeUpdateTitle );


TVector<float>      tempcriterion ( numtf );
TVector<float>      result        ( numtf );
FctParams           p;

p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( mergeduration, samplingfrequency ) );

                                        // scanning only required criteria
for ( TIteratorSelectedForward beci ( critsel ); (bool) beci; ++beci ) {
    
                                        // criterion to boolean markers - also don't modify the original tracks, we might need them later again
    BinarizeCriterion   (   criteria[ beci() ],   tempcriterion,
                            alpha
                        );


    if ( mergeduration > 0 ) {
                                        // Closing = Max then Min
        tempcriterion.Filter ( FilterTypeMax, p );
                                        // dilating before merging
//      tempcriterion.Filter ( FilterTypeMin, p );
        }

                                        // cumulate results
    result     += tempcriterion;
    }

                                        // normalize results in [0..1] - actually, no, return an actual count
//result     /= (double) critselcopy;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Closing the results, merging contributions from all criteria
                                        // It also helps securing the boundaries between all criteria
//Gauge.Next ( -1, SuperGaugeUpdateTitle );


if ( mergeduration > 0 ) {
                                        // Closing = Max then Min
//  result.Filter ( FilterTypeMax, p );
                                        // eroding after the mix
    result.Filter ( FilterTypeMin, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case for closing borders, with some extra duration as it is very often quite bad
//Gauge.Next ( -1, SuperGaugeUpdateTitle );


if ( borderduration > 0 )

    CloseBorders    ( result,   borderduration,     samplingfrequency );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set each epoch to its local max - could also be the Median / Mean?
//Gauge.Next ( -1, SuperGaugeUpdateTitle );


MaxPerSegment   ( result );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer results
metacriterion   = result;
}


//----------------------------------------------------------------------------
                                        // Computing the Meta-Criteria
                                        // - Binarized Tracks to Meta-Criteria
                                        // - Various merging scheme for the 2 ways above, for all 5 meta-groups
void    ProcessBadEpochsMetaCriteria    (   TVector<float>*         criteria,
                                            double                  alpha,
                                            double                  mergeduration,  double          borderduration,
                                            double                  samplingfrequency
                                        )
{
if ( criteria == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSelection          selall ( NumCriteriaBandsTracks, OrderSorted );

selall.Set ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numcrit;
double              mergedurationfactor;

FctParams           p;
p ( FilterParamBinarized )     = true;

criteria[ MetaCritAlli ].Resize ( criteria[ 0 ].GetDim () );

                                        // for different number of criteria
for ( int metai0 = 0, metai = NumCriteriaBandsTracks; metai0 < NumMetaTracks - 1; metai0++, metai++ ) {

    numcrit             = CriteriaBetaCuts[ metai0 ];

    mergedurationfactor = 1 - ( numcrit - 1 ) / (double) NumCriteriaBandsTracks;    // [1..epsilon]


    ProcessBadEpochsMetaCriterion   (   criteria,
                                        selall,
                                        GetAlphaCut ( numcrit, alpha ),
                                        mergeduration * mergedurationfactor,  borderduration,
                                        samplingfrequency,
                                        criteria[ metai ]
                                    );

                                        // beta cut per meta-criterion
    criteria[ metai ].ThresholdAbove ( numcrit );

                                        // binarize
    criteria[ metai ].Filter ( FilterTypeBinarize, p );

                                        // cumulate
    criteria[ MetaCritAlli ]   += criteria[ metai ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // binarize
criteria[ MetaCritAlli ].Filter ( FilterTypeBinarize, p );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Currently expects the maps to be averaged reference, so to minimize big drifts in data
                                        // Basically, it looks at the Non-Gaussianity among electrodes at a given time position
                                        // It can somehow be related to Nelson rules for a special case of EEG (f.ex. more points above mean ~= skewness)
                                        // and is definitely a Statistical Process Control (SPC) method
void    TMarkers::BadEpochsToMarkers    (   const TMaps*        mapsin,             
                                            const char*         filename,       int             session,
                                            double              tolerance,
                                            const char*         newmarkername,
                                            const TSelection*   ignoretracks
                                        )
{
ResetMarkers ();

if ( mapsin == 0 && StringIsEmpty ( filename ) )
    return;

if ( mapsin && mapsin->IsNotAllocated () )
    return;


double              samplingfrequencyin;

if ( mapsin )
    samplingfrequencyin = mapsin->GetSamplingFrequency ();
else
    ReadFromHeader ( filename, ReadSamplingFrequency, &samplingfrequencyin );


if ( samplingfrequencyin <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Deciphering parameters
char                markername[ 256 ];

StringCopy ( markername, StringIsEmpty ( newmarkername ) ? "Bad" : newmarkername );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Control parameters - maybe as parameters?
double              badduration     = BadEpochsBadDuration   ;  // [ms] duration we want to keep
double              mergeduration   = BadEpochsMergeDuration ;  // [ms] duration resolution for the intermediate merged criteria
double              borderduration  = BadEpochsBorderDuration;  // [ms] extra margin for borders
double              finalduration   = BadEpochsFinalDuration ;  // [ms] duration resolution for final merge


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set           ( BadEpochsTitle, SuperGaugeLevelSub );

Gauge.AddPart       ( 0, ComputeCriteriaBandsGaugeCount + 3 );

Gauge.CurrentPart   = 0;

Cartool.CartoolApplication->SetMainTitle   ( BadEpochsTitle, "", Gauge );
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<float>      criteria [ TotalCriteriaBandsTracks ];
double              samplingfrequency;


ComputeBadEpochsCriteriaBands   (   mapsin,
                                    filename,               session,
                                    ignoretracks,
                                    BadEpochsTargetSampling,    // we can heavily downsample, targetting 125[Hz]
                                    badduration,
                                    criteria,
                                    samplingfrequency,
                                    &Gauge
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have all raw criteria tracks - we can post-process to convert them as bad time chunks
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FctParams           p;


#if defined(BadEpochsSaveSteps)

StringCopy      ( _basefile, "E:\\Data\\BadEpochs." );
StringRandom    ( StringEnd ( _basefile ), 3 );

TFileName           _file;
StringCopy ( _file, _basefile, "." FILEEXT_EEGSEF );


                                        // concatenate criteria to a single file
int                 numtf               = criteria[ 0 ].GetDim ();
TArray2<float>      criteriatracks  ( NumCriteriaBandsTracks, numtf );
int                 critmemsize     = criteria[ 0 ].MemorySize ();

                                        // copy criteria tracks
for ( int beci = 0; beci < NumCriteriaBandsTracks; beci++ )

    CopyVirtualMemory   ( criteriatracks[ beci ], criteria[ beci ], critmemsize );

                                        // for a better visual display, all criteria have been made with the interesting part >= 0
p ( FilterParamThreshold )     = 0;
criteriatracks.FilterDim2 ( FilterTypeThresholdAbove, p );


TStrings            criterianames;

for ( int beci = 0; beci < NumCriteriaTracks; beci++ )
    criterianames.Add ( BadEpochsCriterionName[ beci ] );
for ( int beci = 0; beci < NumCriteriaTracks; beci++ )
    criterianames.Add ( BadEpochsCriterionName[ beci ] );
for ( int beci = 0; beci < NumMetaTracks; beci++ )
    criterianames.Add ( BadEpochsCriterionName[ MinMetaTrack + beci ] );


criteriatracks.Index2.IndexRatio   = samplingfrequency;
StringCopy          ( _file, _basefile, "." "CriteriaTracks", "." FILEEXT_EEGSEF );
criteriatracks.WriteFile    ( _file, &criterianames );

//Cartool.CartoolDocManager->OpenDoc ( _file, dtOpenOptions );

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Process each meta-criterion individually, with their own optimal tolerance range
Gauge.Next ( -1, SuperGaugeUpdateTitle );


ProcessBadEpochsMetaCriteria    (   criteria,
                                    tolerance,
                                    mergeduration,  borderduration,
                                    samplingfrequency
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final Closing, after the merge of the intermediate criteria
Gauge.Next ( -1, SuperGaugeUpdateTitle );


if ( finalduration > 0 ) {
                                        // Expand on each side + Closing
    p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( 2 * finalduration, samplingfrequency ) );
                                        // Closing = Max then Min
    criteria[ MetaCritAlli ].Filter ( FilterTypeMax, p );

                                        // final closing
    p ( FilterParamDiameter )     = Round ( MillisecondsToTimeFrame ( finalduration, samplingfrequency ) );

    criteria[ MetaCritAlli ].Filter ( FilterTypeMin, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(BadEpochsSaveSteps)

//TArray2<float>      criteriacrit ( TotalCriteriaBandsTracks, numtf );
TArray2<float>      criteriacrit ( NumMetaTracks, numtf );
TVector<float>      tempcriterion ( numtf );

/*
for ( int beci = 0; beci < NumCriteriaBandsTracks; beci++  ) {

    for ( int i = 0; i < numtf; i++ )

        for ( int metai0 = 0; metai0 < NumMetaTracks - 1; metai0++ )

            if ( criteria[ beci ][ i ] >= GetAlphaCut ( CriteriaBetaCuts[ metai0 ], tolerance ) ) {
                                        // tells in how much meta-criteria it will remain
                criteriacrit[ beci ][ i ]   = NumMetaTracks - 1 - metai0;
                break;
                }

    }
*/
                                        // Copy Meta-criteria
for ( int metai = NumCriteriaBandsTracks; metai < TotalCriteriaBandsTracks; metai++ )

//    CopyVirtualMemory   ( criteriacrit[ metai ], criteria[ metai ], critmemsize );
    CopyVirtualMemory   ( criteriacrit[ metai - NumCriteriaBandsTracks ], criteria[ metai ], critmemsize );

                                        // Copy final Meta-criteria
//CopyVirtualMemory   ( criteriacrit[ MetaCritAlli ], criteria[ MetaCritAlli ], critmemsize );


//StringCopy ( _file, _basefile, "." "Criteria", "." FILEEXT_EEGSEF );
//criteriacrit.Index2.IndexRatio   = samplingfrequency;
//criteriacrit.WriteFile ( _file, &criterianames );
//Cartool.CartoolDocManager->OpenDoc ( _file, dtOpenOptions );

//return;

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined(BadEpochsSaveSteps)

StringCopy ( _file, _basefile, "." "Summary", "." FILEEXT_EEGSEF );

criteriacrit.Index2.IndexRatio   = samplingfrequency;

//criteriacrit.WriteFile ( _file, &criterianames );
criteriacrit.WriteFile ( _file );

Cartool.CartoolDocManager->OpenDoc ( _file, dtOpenOptions );

return;

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convert to markers
Gauge.Next ( -1, SuperGaugeUpdateTitle );


TrackToMarkers  ( criteria[ MetaCritAlli ], markername, false );


//long                totalflagged    = GetMarkersTotalLength ();
//double              ratioflagged    = totalflagged / (double) numtf;
//
//DBGV ( ratioflagged * 100, "% flagged bad" );

                                        // time to upsample the markers back to the original time scale
                                        // !currently rescales the ending of each marker to the right-most position - this way the first and last markers can reach the first and last TF of the original file!
int             downsampling    = samplingfrequencyin / samplingfrequency;

UpsampleMarkers ( downsampling );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

Gauge.FinishParts ();

Cartool.CartoolApplication->SetMainTitle   ( BadEpochsTitle, "", Gauge );

Gauge.Finished ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
