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

#include    "TMicroStates.h"

#include    "TArray2.h"
#include    "Math.Histo.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int     TMicroStates::ComputeMetaCriterion  (   TSelection&         critsel,            TSelection&             critselrank,        TSelection&         critselmax,
                                                int                 reqminclusters,     int                     reqmaxclusters,
                                                TArray2<double>&    var
                                            )
{
                                        // Clip the lowest number of clusters to some reasonable value
#if defined(UseAllCriteria)
                                                // 3: a posteriori knowledge that 2 is very spurrious, plus we don't care for 2 clusters
int                 minrankcluster      = 3;    // assuming well-behaved criteria in both EEG and ESI

#else
                                                // 4: we actually don't care for 2 & 3 clusters, as there are often peaks at 2 and 3, we improve the criteria by skipping this first hill
int                 minrankcluster      = 4;    // ESI cases can have a lot of criteria popping out at 3, which is weird;

#endif
                                        // Where to start considering criterion which includes some derivatives in their formula
                                        // This is some additional heuristic - Turn this off (= 2) when testing criteria
int                 minderivativecluster= 3;    // assuming well-behaved criteria in both EEG and ESI


int                 minclustercrit      = AtLeast ( minrankcluster, reqminclusters );
int                 maxclustercrit      =                           reqmaxclusters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save original criteria selection
TSelection          origcritsel ( critsel );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some criteria need the whole distribution, and can therefore be only computed here
FctParams           p;

                                        // !IMPORTANT!
                                        // !This is the results of (so) many trials as well as simulations, and has been found to be the most efficient on a lot of datasets!
                                        // !Smoothing the criteria LOOKS better, but results are NOT, having a tendency to produce some mid-range best all the time!

                                        // Computed from the derivatives of W and B
                                        // It makes use of a additional border value on each side, so call it with restricted range of clusters
if ( critsel[ segFreyVanGroenewoud            ]
  || critsel[ segFreyVanGroenewoudDeriv       ]
  || critsel[ segFreyVanGroenewoudDerivRobust ] )

    ComputeFreyVanGroenewoud ( reqminclusters, reqmaxclusters, var );

                                        // Computed from the ratios of successive W
                                        // It makes use of a additional border value on each side, so call it with restricted range of clusters
if ( critsel[ segHartigan           ]
  || critsel[ segHartiganDeriv      ]
  || critsel[ segHartiganDerivRobust] )

    ComputeHartigan          ( reqminclusters, reqmaxclusters, var );

                                        // Compute KL from the whole W curve
                                        // It makes use of a additional border value on each side, so call it with restricted range of clusters
if ( critsel[ segKrzanowskiLai      ] 
  || critsel[ segKrzanowskiLaiC     ] )

    ComputeKrzanowskiLai     ( reqminclusters, reqmaxclusters, KLFilterSize, CriterionDerivativeOnData, var );


if ( critsel[ segKrzanowskiLaiRobust    ] 
  || critsel[ segKrzanowskiLaiCRobust   ] )

    ComputeKrzanowskiLai     ( reqminclusters, reqmaxclusters, KLFilterSize, CriterionDerivativeOnRanks, var );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some criterion which behave badly can be somehow salvaged by using their derivative,
                                        // first or second ones, before being normalized.
                                        // Note that's why there is 1 extra data point / cluster on each side of var, BTW.
if ( critsel[ segCalinskiHarabaszDeriv     ] )
    CriterionToDerivative ( var[ segCalinskiHarabasz ],        var[ segCalinskiHarabaszDeriv          ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segCalinskiHarabaszDerivRobust   ] )
    CriterionToDerivative ( var[ segCalinskiHarabasz ],        var[ segCalinskiHarabaszDerivRobust    ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segCIndexDeriv     ] )
    CriterionToDerivative ( var[ segCIndex ],        var[ segCIndexDeriv          ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segCIndexDerivRobust   ] )
    CriterionToDerivative ( var[ segCIndex ],        var[ segCIndexDerivRobust    ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segCrossValidationDeriv    ] )
    CriterionToDerivative ( var[ segCrossValidation ],  var[ segCrossValidationDeriv        ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segCrossValidationDerivRobust ] )
    CriterionToDerivative ( var[ segCrossValidation ],  var[ segCrossValidationDerivRobust  ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segDaviesBouldinDeriv         ] )
    CriterionToDerivative ( var[ segDaviesBouldin ],    var[ segDaviesBouldinDeriv      ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segDaviesBouldinDerivRobust   ] )
    CriterionToDerivative ( var[ segDaviesBouldin ],    var[ segDaviesBouldinDerivRobust  ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segDunnDeriv  ] )
    CriterionToDerivative ( var[ segDunn ],             var[ segDunnDeriv ],        var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segDunnDerivRobust  ] )
    CriterionToDerivative ( var[ segDunn ],             var[ segDunnDerivRobust ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segDunnRobustDeriv    ] )
    CriterionToDerivative ( var[ segDunnRobust ],       var[ segDunnRobustDeriv ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segDunnRobustDerivRobust  ] )
    CriterionToDerivative ( var[ segDunnRobust ],       var[ segDunnRobustDerivRobust   ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segFreyVanGroenewoudDeriv ] )
    CriterionToDerivative ( var[ segFreyVanGroenewoud ],    var[ segFreyVanGroenewoudDeriv ],        var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segFreyVanGroenewoudDerivRobust ] )
    CriterionToDerivative ( var[ segFreyVanGroenewoud ],    var[ segFreyVanGroenewoudDerivRobust ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segGammaDeriv         ] )
    CriterionToDerivative ( var[ segGamma ],            var[ segGammaDeriv          ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segGammaDerivRobust   ] )
    CriterionToDerivative ( var[ segGamma ],            var[ segGammaDerivRobust    ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segHartiganDeriv  ] )
    CriterionToDerivative ( var[ segHartigan ],             var[ segHartiganDeriv ],        var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segHartiganDerivRobust  ] )
    CriterionToDerivative ( var[ segHartigan ],             var[ segHartiganDerivRobust ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segMcClainDeriv     ] )
    CriterionToDerivative ( var[ segMcClain ],        var[ segMcClainDeriv          ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segMcClainDerivRobust   ] )
    CriterionToDerivative ( var[ segMcClain ],        var[ segMcClainDerivRobust    ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segPointBiserialDeriv         ] )
    CriterionToDerivative ( var[ segPointBiserial ],    var[ segPointBiserialDeriv      ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segPointBiserialDerivRobust   ] )
    CriterionToDerivative ( var[ segPointBiserial ],    var[ segPointBiserialDerivRobust  ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segRatkowskiDeriv     ] )
    CriterionToDerivative ( var[ segRatkowski ],        var[ segRatkowskiDeriv          ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segRatkowskiDerivRobust   ] )
    CriterionToDerivative ( var[ segRatkowski ],        var[ segRatkowskiDerivRobust    ],       var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segSilhouettesDeriv       ] )
    CriterionToDerivative ( var[ segSilhouettes ],      var[ segSilhouettesDeriv        ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnData ), -1 );
if ( critsel[ segSilhouettesDerivRobust ] )
    CriterionToDerivative ( var[ segSilhouettes ],      var[ segSilhouettesDerivRobust  ],  var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ), -1 );


if ( critsel[ segTraceW             ] )
    CriterionToDerivative ( var[ segTraceW ],           var[ segTraceW ],           var.GetDim2 (), 
                            reqminclusters, reqmaxclusters, 
                            (CriterionDerivativeEnum) ( CriterionDerivativeSecond | CriterionDerivativeOnRanks ),  1 );



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Force resetting unreliable positions due to some derivation, either from the formula itself, or as postprocessing
for ( int ncl = 0; ncl < minderivativecluster; ncl++ ) {
                                        // mainly all derivatives formulas, plus a few badly behaving criteria
    var ( segCalinskiHarabaszDeriv,         ncl )   = 0;
    var ( segCalinskiHarabaszDerivRobust,   ncl )   = 0;
    var ( segCIndexDeriv,                   ncl )   = 0;
    var ( segCIndexDerivRobust,             ncl )   = 0;
    var ( segCrossValidationDeriv,          ncl )   = 0;
    var ( segCrossValidationDerivRobust,    ncl )   = 0;
    var ( segDaviesBouldinDeriv,            ncl )   = 0;
    var ( segDaviesBouldinDerivRobust,      ncl )   = 0;
    var ( segDunnDeriv,                     ncl )   = 0;
    var ( segDunnDerivRobust,               ncl )   = 0;
    var ( segDunnRobustDeriv,               ncl )   = 0;
    var ( segDunnRobustDerivRobust,         ncl )   = 0;
    var ( segFreyVanGroenewoud,             ncl )   = 0;
    var ( segFreyVanGroenewoudDeriv,        ncl )   = 0;
    var ( segFreyVanGroenewoudDerivRobust,  ncl )   = 0;
    var ( segGammaDeriv,                    ncl )   = 0;
    var ( segGammaDerivRobust,              ncl )   = 0;
    var ( segHartigan,                      ncl )   = 0;
    var ( segHartiganDeriv,                 ncl )   = 0;
    var ( segHartiganDerivRobust,           ncl )   = 0;
    var ( segKrzanowskiLai,                 ncl )   = 0;
    var ( segKrzanowskiLaiC,                ncl )   = 0;
    var ( segKrzanowskiLaiRobust,           ncl )   = 0;
    var ( segKrzanowskiLaiCRobust,          ncl )   = 0;
    var ( segMcClainDeriv,                  ncl )   = 0;
    var ( segMcClainDerivRobust,            ncl )   = 0;
    var ( segPointBiserialDeriv,            ncl )   = 0;
    var ( segPointBiserialDerivRobust,      ncl )   = 0;
    var ( segRatkowskiDeriv,                ncl )   = 0;
    var ( segRatkowskiDerivRobust,          ncl )   = 0;
    var ( segSilhouettesDeriv,              ncl )   = 0;
    var ( segSilhouettesDerivRobust,        ncl )   = 0;
    var ( segTraceW,                        ncl )   = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check and remove useless criterion (null/constant or with only 1 non-null value)
for ( int ci = segCritMin; ci <= segCritMax; ci++ ) {

    if ( ! critsel[ ci ] )
        continue;


    int         numnotnull      = 0;

    for ( int ncl = reqminclusters; ncl <= reqmaxclusters; ncl++ )

        numnotnull     += var [ ci ][ ncl ] != 0;

                                        // criterion with less than 2 non-null values is useless
    if ( numnotnull <= 1 )
        critsel.Reset ( ci );
    }

                                        // also spread that to specialized selection
critselrank    &= critsel;
critselmax     &= critsel;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Criteria have to be somehow normalized / ranked:
                                        // - They don't have the same scale at all, so at least we need to Normalize to [0..1] (or Standardize)
                                        // - They don't have the same distribution, so we normalize their distribution by taking their ranks instead
                                        // - Only then can we merge the results together

                                        // Best smoothing strategies, which are quite close:
                                        // - Criterion: Normalize, Rank(Linear) then Smooth with Gaussian(3), then Rescale max to 1
                                        //   Stats    : Consistent Median(1), being Median-IQR, Rank(Inverse)
                                        // - Criterion: Normalize, Rank(Linear)
                                        //   Stats    : Use 4 times center + 1 time edges, Consistent Median(1), being Median-IQR, Rank(Inverse)
                                        // Current chosen best is the second way, we keep the criterion and their ranking intact; we use a quasi-Gaussian filter, but with non-linear behavior (ConsistentMedian)

                                        // !Many trials have been run, this is the most efficient that has been found!
                                        // !No smoothing whatsoever!

RankCriterionFlag   rankstylecrit   = RankCriterionLinear;      // we prefer criterion to be ranked linearly, to avoid the SD to artificially prefer low values which will appear closer to each others than the higher values
//RankCriterionFlag rankstylemeta   = RankCriterionInverse;     // just to rescale the final criterion more nicely with the best answers being 1, 0.5 0.333 etc..

//TArray2<double>     ranks ( var );

p ( FilterParamDiameter )     = 3;


for ( int ci = segCritMin; ci <= segCritMax; ci++ )

    RankCriterion       ( var [ ci ], var [ ci ] /*ranks [ ci ]*/, minclustercrit, maxclustercrit, rankstylecrit );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Oopsie, all criteria failed? as a fallback, restore all of them...
                                        // Set min criteria to 3, so we can compute some stats
if ( (int) critsel < 3 /*1*/ )

//  critsel.Set ( segCritMin, segCritMax );
    critselrank = 
    critselmax  = 
    critsel     = origcritsel;          // back original selection


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now compute a compound criterion from the normalized criterion above
int                 numcrit         = (int) critselrank;
TEasyStats          critstat ( numcrit );


for ( int ncl = reqminclusters; ncl <= reqmaxclusters; ncl++ ) {

                                        // below this value, everything is 0 - skip any computation which could divide by 0
    if ( ncl < minrankcluster ) {
        var ( segMeanRanks, ncl )   = 0;
        continue;
        }

                                        // merge all ranks
    critstat.Reset ();

    for ( TIteratorSelectedForward ci ( critselrank ); (bool) ci; ++ci )
                                        
      //critstat.Add ( var ( ci(), ncl ),         ThreadSafetyIgnore ); // artihmetic mean
        critstat.Add ( log ( var ( ci(), ncl ) ), ThreadSafetyIgnore ); // geometrical mean

//  var ( segMeanRanks, ncl )   = critstat.Mean  ();
    var ( segMeanRanks, ncl )   = exp ( critstat.Mean () );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ranking median curve
RankCriterion       ( var [ segMeanRanks ], var [ segMeanRanks ], minclustercrit, maxclustercrit, rankstylecrit );

                                        // Retrieve position of max mean
//int                 argmaxmedian        = ArgMax ( &var ( segMeanRanks, 0 ), minclustercrit, maxclustercrit );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merge all ArgMax'es of the max criteria
numcrit         = (int) critselmax;
critstat.Resize ( numcrit /*+ 2*/ );

                                        // do 1 big stat on all ArgMax'es
critstat.Reset ();

for ( TIteratorSelectedForward ci ( critselmax ); (bool) ci; ++ci ) {

    int             argmax      = ArgMax ( &var ( ci(), 0 ), reqminclusters, reqmaxclusters );

    critstat.Add ( argmax, ThreadSafetyIgnore );
    }

                                        // This single value seems to be the most reliable one
                                        // don't allow for interpolated value, between 2 maxes 3 and 5 f.ex. return 3 or 5, but not 4
int                 medianargmax    = Round ( critstat.Median ( true ) );

                                        // cumulate each criterion max, to give a feeling of the distribution of the maxes
for ( int i = 0; i < critstat.GetNumItems (); i++ )

    var ( segMetaCrit, critstat[ i ] )++;

NormalizeCriterion  ( var [ segMetaCrit ], minclustercrit, maxclustercrit, NormalizeTo0, NormalizeTo1, false );
                                        // add one more step on criterion to make it stand out in case of equality
var ( segMetaCrit, medianargmax )++;

NormalizeCriterion  ( var [ segMetaCrit ], minclustercrit, maxclustercrit, NormalizeTo0, NormalizeTo1, false );

                                        // extract histogram from data
                                        // it looks highly correlated to the mean rank, but not exactly
{
THistogram              histoargmax;
double                  margin              = 5;
double                  subsampling         = 10;
bool                    smoothing           = true;
double                  histosize           = 0;
//double                  margin              = 0;  // old parameters - not sure if correct as upper border can sometimes appear artifically high
//double                  subsampling         = 1;
//bool                    smoothing           = true;
//double                  histosize           = reqmaxclusters + 1;

                                        // histogram, not normalized
histoargmax.ComputeHistogram    (   critstat, 
                                    0,          0,              reqmaxclusters, 
                                    margin,     subsampling,
                                    (HistogramOptions) ( HistogramPDF | ( smoothing ? HistogramSmooth : HistogramRaw ) | HistogramCount | HistogramLinear ),
                                    &histosize 
                                );

                                        // transfer histogram back to variable
//for ( int ncl = minrankcluster; ncl <= reqmaxclusters; ncl++ )
//    var ( segHistoArgLocalMax, ncl )    = histoargmax[ ncl ];

for ( int ncl = minrankcluster; ncl <= reqmaxclusters; ncl++ )
    var ( segHistoArgLocalMax, ncl )    = histoargmax.GetValue ( (double) ncl );

                                        // Normalization is visually more pleasant
NormalizeCriterion  ( var [ segHistoArgLocalMax ], minclustercrit, maxclustercrit, NormalizeToE, NormalizeTo1, true );
//RankCriterion       ( var [ segHistoArgLocalMax ], var [ segHistoArgLocalMax ], minclustercrit, maxclustercrit, rankstylecrit );
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Merge the 3 curves: Mean of Criteria, Histo of Local ArgMaxes, Pseudo-Gaussian of ArgMax
                                        // Result is actually quite good, but in the end, no better (even slightly less good) than the MedianArgMax

for ( int ncl = reqminclusters; ncl <= reqmaxclusters; ncl++ ) {
                                        // using 2 or 3 criteria, adding the single value ArgMax at the right position
    critstat.Reset ();

                                        // do a geometrical mean, only on existing data
    if ( var ( segMeanRanks,            ncl ) > 1e-3 )   critstat.Add ( logl ( var ( segMeanRanks,         ncl ) ), ThreadSafetyIgnore );

    if ( var ( segHistoArgLocalMax,     ncl ) > 1e-3 )   critstat.Add ( logl ( var ( segHistoArgLocalMax,  ncl ) ), ThreadSafetyIgnore );
                                        // this is normally a single value somewhere, but we converted it to a curve
    if ( var ( segMedianArgMaxCurve,    ncl ) > 1e-3 )   critstat.Add ( logl ( var ( segMedianArgMaxCurve, ncl ) ), ThreadSafetyIgnore );


    var ( segMerge3Curves, ncl )    = exp ( critstat.Mean () );
    }

                                        // rank final stuff
RankCriterion       ( var [ segMerge3Curves  ], var [ segMerge3Curves  ], minclustercrit, maxclustercrit, rankstylecrit );

                                        // Retrieve max of meta-criterion
int                 argmerge3curves     = ArgMax ( &var ( segMerge3Curves, 0 ), minclustercrit, maxclustercrit );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Merge the 3 Arg estimates - yes, that makes a lot of steps
critstat.Reset ();
                                        // let's see what we get with these 3 guys
critstat.Add ( argmaxmedian,    ThreadSafetyIgnore );
critstat.Add ( medianargmax,    ThreadSafetyIgnore );
critstat.Add ( argmerge3curves, ThreadSafetyIgnore );


int                 median3args     = critstat.Median ();
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now choose the best of the best
int                 argmetacrit     = medianargmax; // argmerge3curves;

                                        // just poke value of 1 at most probable cluster
//var ( segMetaCrit, argmaxmedian   )   = 0.25;
//var ( segMetaCrit, medianargmax   )   = 0.50;
var ( segMetaCrit, argmetacrit      )   = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  argmetacrit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
