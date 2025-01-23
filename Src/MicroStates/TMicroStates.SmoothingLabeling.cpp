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

#include    "TMicroStates.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Re-labeling with smoothing

                                        // With files parameters & relative time interval
void    TMicroStates::SmoothingLabeling     (  
                                            int             filemin,    int             filemax,    
                                            long            fromtf,     long            totf,
                                            int             nclusters,  
                                            TMaps&          maps,       TSelection*     mapsel,
                                            TLabeling&      labels,
                                            int             winsize,    double          lambda,
                                            PolarityType    polarity,   double          limitcorr 
                                            )
{
long                tfmin;
long                tfmax;


for ( int f = filemin; f <= filemax; f++ ) {

    TimeRangeToDataRange ( f, fromtf, totf, tfmin, tfmax );

    SmoothingLabeling   (
                        tfmin,      tfmax, 
                        nclusters, 
                        maps,       mapsel, 
                        labels, 
                        winsize,    lambda, 
                        polarity,   limitcorr 
                        );
    }

}


//----------------------------------------------------------------------------
                                        // With absolute time interval
void    TMicroStates::SmoothingLabeling     (  
                                            long            tfmin,      long            tfmax,
                                            int             nclusters,  
                                            TMaps&          maps,       TSelection*     mapsel,
                                            TLabeling&      labels,
                                            int             winsize,    double          lambda,
                                            PolarityType    polarity,   double          limitcorr 
                                            )
{
                                        // !labeling must have been done at that point!

double              origsigma2      = ComputeSigmaMu2 ( NumElectrodes, maps, labels, tfmin, tfmax );     // explicitly used in the formula

                                        // empty labeling (degenerate cases) or perfect labeling (very likely synthetic data)?
if ( origsigma2 == 0 )
    return;                             // either way, smoothing can not improve things...


double              gev             = ComputeGEV ( maps, labels, tfmin, tfmax );   // for convergence

                                        // same as above: nothing explained -> return
//if ( gev == 0 )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

constexpr int       SmoothingMaxIter    = 20;       // Max number of smoothing iterations
constexpr double    LabelingConvergence = 1e-6;     // Relative error threshold to stop iterative process
TLabeling           templabels ( labels );          // local copy of given labeling
//bool              samelabel;
double              gevbefore;
LabelType           undefi              = nclusters;// index of the undefined labels


                                        // limit the number of repetitions by safety - only a few iterations are needed in practice
for ( int smoothi = 0; smoothi < SmoothingMaxIter; smoothi++ ) {

//  samelabel   = true;

                                        // !reset only these TFs!
    templabels.Reset ( tfmin, tfmax );


    OmpParallelBegin

    TArray1<int>        histo ( nclusters + 1 );        // 1 more for UndefinedLabel case

    OmpFor
                                        // maps -> labels
    for ( long tf = tfmin; tf <= tfmax; tf++ ) {

                                        // compute histogram of neighbors' clusters
        histo   = 0;

        for ( long tf2 = AtLeast ( tfmin, tf - winsize ); tf2 <= NoMore ( tfmax, tf + winsize ); tf2++ )

            if ( tf2 != tf )

                if ( labels.IsUndefined ( tf2 ) )   histo[ undefi        ]++;   // dummy cluster, just for UndefinedLabel - !ït has NO associated maps!
                else                                histo[ labels[ tf2 ] ]++;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case for UndefinedLabel: there is no single template map associated to undefined state, so we have to account for this case separately
                                        // If there is a majority of undefined data points AND current data point is undefined THEN keep it undefined
                                        // !current data point HAS to be undefined as to avoid flipping labeled to unlabeled and unlabeled to labeled alternatively!
                                        // !not applied to a currently labeled data point, which could make it undefined therefor moving the edge!
        if ( labels.IsUndefined ( tf ) ) {
                                        // if we need a real count
//          int                 countundef      = 0;
//
//          for ( int nc = 0; nc < nclusters; nc++ )
//                                      // count how many real clusters it outpass
//              countundef     +=  histo[ nc ] <= histo[ undefi ];
//
//
//          if ( countundef == nclusters )
//                                      // if undefined is the most probable..
//              continue;               // ..then stay UndefinedLabel

                                        // simplified code to stop at first cluster with more than undefined count
            int                 nc              = 0;

            for ( nc = 0; nc < nclusters; nc++ )
                                        // stop at first cluster with STRICTLY more labels than undefined
                if ( histo[ nc ] > histo[ undefi ] )

                    break;


            if ( nc == nclusters )      // no cluster count went above undefined count?

                continue;               // keep current undefined state
            }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From here we ignore any UndefinedLabel labels from histogram
                                        // We can have either: 1) a valid labels or 2) a somehow isolated (minority) undefined labels, which could then be flipped to labeled depending on limitcorr value
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute new error by using the histogram to alter the scalar product results
        PolarityType        pol;
        double              corr;
        double              diff;
        double              diffmin     = Highest ( diffmin );
        

        for ( int nc = 0; nc < nclusters; nc++ ) {

            if ( mapsel && ! (*mapsel)[ nc ] )
                continue;

                                        // test and store polarity
            pol         = polarity == PolarityEvaluate && maps[ nc ].IsOppositeDirection ( Data[ tf ] ) ? PolarityInvert : PolarityDirect;

                                        // compute correlation with previous polarity
            corr        = Project ( maps[ nc ], Data[ tf ], pol );
                
                                        // upper part of the error ratio SigmaMu/Sigma2: lower the error for most probable maps
//          diff        = ( Square ( Norm[ tf ] ) * ( 1 - Square ( corr ) ) )       // from original article
            diff        = ( Square ( Norm[ tf ] ) * ( 1 - SignedSquare ( corr ) ) ) // !accounting for ERP case where correlation -0.9 should be worse than +0.5, and not better! Still a highly negative correlation is very unlikely to occur anyway
                        / ( 2 * origsigma2 * ( NumElectrodes - 1 ) ) 
                        - lambda * histo[ nc ];

                                        // updating the labeling, so we have to test for the correlation limit
                                        // !if limitcorr == IgnoreCorrelationThreshold, this will force any unlabeled data point into labeled!
                                        // !if limitcorr != IgnoreCorrelationThreshold, unlabeled data point due to low correlation will remain unlabeled!
            if ( corr >= limitcorr && diff < diffmin ) {

                diffmin     = diff;

                templabels.SetLabel ( tf, nc, pol );
                }

            } // for nc

        } // for tf

    OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check that the labeling has indeed changed
//  for ( long tf = tfmin; tf <= tfmax; tf++ ) {
//
//      if ( labels.IsDifferent ( templabels, tf ) ) {
//
//          samelabel   = false;
//          break;
//          } // if different
//      } // for tf


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // udpate with new labeling
    labels      = templabels;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Compute new error from new templates & labeling
    gevbefore   = gev;

    gev         = ComputeGEV ( maps, labels, tfmin, tfmax );

    if ( gev > gevbefore                                                // new GEV is above the previous one? it actually shouldn't (smoothing degrades the labeling) so that means we are in a oscillating state
      || gev == 0                                                       // nothing explained?
//    || samelabel                                                      // no change in labeling?
      || RelativeDifference ( gev, gevbefore ) <= LabelingConvergence   // real loop convergence
        )

        break;

    } // for smoothi

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
