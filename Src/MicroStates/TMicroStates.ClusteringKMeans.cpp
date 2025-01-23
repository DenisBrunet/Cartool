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
                                        // Not thread safe for the moment due to use of global  RandomUniform
void    TMicroStates::GetRandomMaps (   int     nclusters,  TMaps&      maps    )
{
                                        // Not really optimal, but has a small memory footprint and actually runs fast due to the small chances of conflicts
TArray1<int>        picked ( nclusters );
bool                goodpick;
long                randtf;


for ( int nc = 0; nc < nclusters; nc++ ) {
                                        // Probability is uniform across all existing maps
                                        // !If one wants reproducible results, then use a fixed see when calling Reload!
    randtf      = RandomUniform ( (UINT) NumTimeFrames );

                                        // all picks should differ from each others!
    goodpick    = true;

    for ( int nc2 = 0; nc2 < nc; nc2++ )
                                        // with enough data, this should be quite rare
        if ( picked[ nc2 ] == randtf ) {
            goodpick  = false;
            break;
            }


    if ( goodpick ) {
        picked[ nc ]    = randtf;
        maps  [ nc ]    = Data[ randtf ];
        }
    else
        nc--;                           // do it again...

    } // for nclusters

}


//----------------------------------------------------------------------------
                                        // This part is the parallelized one
bool    TMicroStates::SegmentKMeans_Once    (
                                            int             nclusters,
                                            TMaps&          maps,       TLabeling&          labels,
                                            PolarityType    polarity,
                                            double          &gev,
                                            CentroidType    centroid,   bool                ranking
                                            )
{
                                        // 0.1) Ramdomly picking maps from data as inital templates - also doing the initial labeling
GetRandomMaps   ( nclusters, maps );

                                        // 0.2) Maps -> Labels
maps.CentroidsToLabeling    (  
                            Data,   
                            0,          NumTimeFrames - 1, 
                            nclusters, 
                            0,          labels, 
                            polarity,   IgnoreCorrelationThreshold  // don't limit correlation here, get the full labeling, the templates computation will already sort out the bad TFs
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the loop Maps <-> Labels, until convergence of GEV
constexpr int       KMeansMaxIter       = 100;      // Max number of K-Means iterations
constexpr double    LabelingConvergence = 1e-6;     // Relative error threshold to stop iterative process
double              gevbefore;

gev     = 0;

                                        // don't loop forever by using a liberal max number of iterations
for ( int kmi = 0; kmi < KMeansMaxIter; kmi++ ) {

//  UpdateApplication;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Labels -> Maps
    maps.LabelingToCentroids    ( 
                                Data,       &ToData, 
                                nclusters, 
                                labels, 
                                polarity,   MeanCentroid /*centroid*/,  ranking     // for the inner loop, use a plain Mean Centroid to improve speed
                                );

                                        // lost some maps for some weird reasons? this is an unrecoverable problem, quit now!
    if ( maps.SomeNullMaps ( nclusters ) )

        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Maps -> Labels
                                        // This is the right sequence: the labels correspond to the maps,
                                        // hence the error computation is correct
    maps.CentroidsToLabeling    (  
                                Data, 
                                0,          NumTimeFrames - 1, 
                                nclusters, 
                                0,          labels, 
                                polarity,   IgnoreCorrelationThreshold  // don't limit correlation here
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Compute new error from new templates & labeling
    gevbefore   = gev;

    gev         = ComputeGEV ( maps, labels, 0, NumTimeFrames - 1 );


    if ( RelativeDifference ( gev, gevbefore ) < LabelingConvergence    // reached convergence?
      || gev < gevbefore                                                // not good, GEV is suddenly degrading?
        )
        break;

    } // for kmi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // If caller requested a better centroid than the Mean Centroid
if ( centroid != MeanCentroid ) {
                                        // Use final labeling for this more costly operation
    maps.LabelingToCentroids    ( 
                                Data,       &ToData, 
                                nclusters, 
                                labels, 
                                polarity,   centroid,   ranking 
                                );

                                        // Launch a final labeling, for consistency
    maps.CentroidsToLabeling    (  
                                Data, 
                                0,          NumTimeFrames - 1, 
                                nclusters, 
                                0,          labels, 
                                polarity,   IgnoreCorrelationThreshold  // don't limit correlation here
                                );

                                        // Final error
    gev         = ComputeGEV ( maps, labels, 0, NumTimeFrames - 1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return true;
}


//----------------------------------------------------------------------------
int     TMicroStates::SegmentKMeans (   int             nclusters,
                                        TMaps&          maps,           TLabeling&          labels,
                                        PolarityType    polarity,
                                        int             numrandomruns,
                                        CentroidType    centroid,
                                        bool            ranking
                                      )
{
double              tempgev;
double              bestgev;
TMaps               tempmaps   ( nclusters, NumRows );
TLabeling           templabels ( NumTimeFrames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bestgev             = Lowest ( bestgev );
int                 numeffrandomruns    = 0;    // used to count the real number of trials

                                        // runs many time the segmentation
for ( int runagain = 0; runagain < numrandomruns; runagain++, numeffrandomruns++ ) {

                                        // exhaust the Gauge until we are done
    if ( nclusters == 1 && runagain >= 1            // 1 cluster
      || numeffrandomruns > 10 * numrandomruns ) {  // we are looping way too much with erroneous empty maps

        Gauge.Next ();
        continue;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // early stop or error?
                                        // Parallelized part
    if ( ! SegmentKMeans_Once   (
                                nclusters, 
                                tempmaps,   templabels, // store results to temp variables
                                polarity, 
                                tempgev, 
                                centroid,   ranking
                                ) )
        {
        runagain--;                     // segmentation failed, restart it

        continue;
        }

                                        // only increment when successful
    Gauge.Next ();

    if ( ! GroupGauge.IsAlive () && CartoolObjects.CartoolApplication->IsInteractive () )
        CartoolObjects.CartoolApplication->SetMainTitle    ( Gauge );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // is the last trial better than the current best?
    if ( tempgev > bestgev ) {

        bestgev     = tempgev;          // our current best global explained variance
        labels      = templabels;       // our current best maps and labeling
        maps        = tempmaps;
        }

    } // for runagain


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // In some rare occurences, some maps might have disappeared, usually after a relabeling
                                        // Pack the labels so we return some tidy labeling & the actual final number of maps
return  labels.PackLabels ( maps );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
