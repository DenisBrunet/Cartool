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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Initializing the Topographical Atomize and Agglomerate Hierarchical Clustering (T-AAHC)
                                        // It begins with all data points being assigned to a single cluster, with itself as the centroid.
                                        // It then computes the correlations between all pairs of maps, sort this big table, and 
                                        // proceed to merge these highest pairs together.
                                        // It stops when all data has been paired, resulting in (NumTimeFrames/2) clusters of 2 maps,
                                        // each with a corresponding (forced) mean centroid.
                                        // A single cluster of 1 map can remain if NumTimeFrames is odd, which will be taken care of
                                        // in the main loop.
                                        // This type of initialization is the "best" one, as it works through all the data.
                                        // However this can be quite expensive, and not suitable for Resting States analysis.
                                        // In the latter case, one could assume some data continuity and simply assign pairs
                                        // of successive maps as a fair starting point. Results, however, would change if data would be reshuffled.
int     TMicroStates::SegmentTAAHC_Init (   TMaps&          maps,           TLabeling&          labels,      
                                            PolarityType    polarity,       
                                            CentroidType  /*centroid*/,
                                            bool            ranking
                                        ) 
{
int                 nclusters       = NumTimeFrames;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // There is a lot to do on the first call!

                                        // 0.0) Allocate a big array, but only on first call (promised)
                                        // init prototypes and copy maps

                                        // allocate and copy ALL data: each map is its own template at first
maps.CopyFrom ( Data, NumTimeFrames );
        

//maps.Normalize ();                // not needed here, input data has done it already

                                        // set initial labeling: each map is its own template
OmpParallelFor

for ( long tf = 0; tf < NumTimeFrames; tf++ )

    labels.SetLabel ( tf, tf, PolarityDirect );          


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0.1) Compute all possible correlations between all maps
                                        // This is the historical version. It is optimal in the sense it doesn't car for the data ordering
                                        // Another option would be to simply assign pairs of successive data points. But then it would depend on the initial data ordering, which would still be OK for regular cases
int                 numtriangular       = ( NumTimeFrames * ( NumTimeFrames - 1 ) ) / 2;
TArray2<float>      allcorr ( numtriangular, 3 );

OmpParallelFor

for ( int nc1 = 0; nc1 < NumTimeFrames; nc1++ ) {

//  Gauge.Next ();

    for ( int nc2 = nc1 + 1; nc2 < NumTimeFrames; nc2++ ) {
                                        // exact index for insertion - mandatory for parallelization
        int     inserti     = numtriangular - ( ( NumTimeFrames - nc1 ) * ( NumTimeFrames - nc1 - 1 ) ) / 2 + nc2 - nc1 - 1;

        allcorr ( inserti, 0 )  = nc1;
        allcorr ( inserti, 1 )  = nc2;
        allcorr ( inserti, 2 )  = Project              ( maps[ nc1 ], maps[ nc2 ], polarity );
//      allcorr ( inserti, 2 )  = NormalizedDifference ( maps[ nc1 ], maps[ nc2 ], polarity );
        } // for nc2
    } // for nc1

                                        // sort by increasing correlation / decreasing distance
allcorr.SortRows ( 2, Ascending );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0.2) Scan all pairs, introduce them by order of importance, ie correlation
int                 lastcorri       = numtriangular - 1;

                                        // Consume all pairs from the triangular matrix
do {
                                        // move the end of the correlation list (best)
    for ( ; lastcorri >= 0 && allcorr ( lastcorri, 0 ) == UndefinedLabel; ) {

        lastcorri--;

        Gauge.Next ();
        }

    if ( lastcorri < 0 )                // no more pairs
        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use the array to get the most correlated pair of the remaining solo maps
    LabelType       index1      = allcorr ( lastcorri, 0 );
    LabelType       index2      = allcorr ( lastcorri, 1 );

                                        // a single remaining map will be processed by the Atomize step
    if (   index1 == UndefinedLabel 
        || index2 == UndefinedLabel )
        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update correlation table with new indexes
    OmpParallelBegin

    OmpFor
    for ( int aci = lastcorri; aci >= 0; aci-- ) {

        if ( allcorr ( aci, 0 ) == index1 || allcorr ( aci, 1 ) == index1
          || allcorr ( aci, 0 ) == index2 || allcorr ( aci, 1 ) == index2 )

            allcorr ( aci, 0 )  =
            allcorr ( aci, 1 )  = UndefinedLabel;

        if ( allcorr ( aci, 0 ) > index2 )   allcorr ( aci, 0 )--;
        if ( allcorr ( aci, 1 ) > index2 )   allcorr ( aci, 1 )--;
        }

                                        // update labeling
    OmpFor
    for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

        if      ( labels[ tf ] == index2 )  labels[ tf ] = index1;   // merge index2 into index1 (the smallest index value)
        else if ( labels[ tf ] >  index2 )  labels[ tf ]--;          // shift labels after index2
        }

    OmpParallelEnd

                                        // update maps, shift rows up
    if ( nclusters - 1 - index2 > 0 )

//      MoveVirtualMemory ( maptarray[ index2 ], maptarray[ index2 + 1 ],
//                              ( nclusters - 1 - index2 ) * maptarray.GetDim2 () * maptarray.AtomSize () );
        for ( int i = index2; i < nclusters - 1; i++ )
            maps[ i ]   = maps[ i + 1 ];


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we now have one less cluster
    nclusters--;

    } while ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0.3) Recompute all maps, as we have pairs now
                                        // We don't really need a weighted sum for 2 maps!
maps.LabelingToCentroids    ( 
                            Data,       &ToData, 
                            nclusters, 
                            labels, 
                            polarity,   MeanCentroid /*centroid*/,  ranking 
                            );

return  nclusters;
}


//----------------------------------------------------------------------------
                                        // Topographical Atomize & Agglomerate Hierarchical clustering (T-AAHC), original method by Denis Brunet
                                        // !tempmaps should be deallocated on first call - or another way should be found to detect first call!
int     TMicroStates::SegmentTAAHC  (   int             nclusters,
                                        TMaps&          maps,           TLabeling&          labels,      
                                        PolarityType    polarity,       
                                        CentroidType    centroid,
                                        bool            ranking,
                                        TMaps&          savedmaps,      TLabeling&          savedlabels,
                                        TMaps&          tempmaps,       // to avoid re-allocating this guy locally on each call - could be made optional, though
                                        int             maxclusters 
                                    ) 
{
bool                init            = tempmaps.IsNotAllocated ();
int                 startclust;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( init )
                                        // Initializing to (NumTimeFrames / 2) clusters
    startclust  = SegmentTAAHC_Init     (   tempmaps,   labels,      
                                            polarity,       
                                            centroid,
                                            ranking
                                        );
else {
                                        // restore from 'maxclusters' saved point
    tempmaps    = savedmaps;            
    labels      = savedlabels;
    startclust  = maxclusters;
    } 


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Loop down to the requested number of clusters
TSelection          clustertf ( NumTimeFrames, OrderSorted );
TSelection          index2tf  ( NumTimeFrames, OrderSorted );
                                        // pointer to specialized function, according to polarity
bool    (*keepbestlabeling) ( const TMap&, const TMap&, double&, double )   = polarity == PolarityEvaluate ? &KeepBestLabelingEvaluate : &KeepBestLabelingDirect;


                                        // decrement number of clusters until down to the requested one
for ( int nc = startclust; nc > nclusters; ) {

    Gauge.Next ();

    if ( ! GroupGauge.IsAlive () && CartoolObjects.CartoolApplication->IsInteractive () )
        CartoolObjects.CartoolApplication->SetMainTitle    ( Gauge );

                                        // 1) Destroy "least valuable" cluster
    double              mincorr         = Highest ( mincorr );
    LabelType           index           = UndefinedLabel;

                                        // 1.1) scan each cluster
    for ( int nc1 = 0; nc1 < nc; nc1++ ) {

        int                 nummaps     = 0;
        double              sumcorr     = 0;
        clustertf.Reset ();

                                        // scan this cluster
        OmpParallelForSum ( nummaps, sumcorr )

        for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

            if ( labels[ tf ] != nc1 )
                continue;
                                        // store current cluster tf's
            nummaps++;

            clustertf.Set ( tf );
//                                      // AAHC with Topographic behavior
//                                      // Compute the sum of distance from all maps to the template
            sumcorr    += Project ( tempmaps[ nc1 ], Data[ tf ], polarity );
//          sumcorr    += Square ( Project ( tempmaps[ nc1 ], Data[ tf ], polarity ) );
            } // for tf && cluster nc1


        if ( nummaps != 0 ) {
                                        // average of correlation does not seem to give the best error measure
//          sumcorr    /= nummaps;
                                        // select the min sum of correlations cluster
                                        // for a single map, this is 0 (more than a single map shouldn't occur here, this step is solved at init time)
            if ( sumcorr < mincorr ) {
                mincorr     = sumcorr;
                index       = nc1;
                index2tf.Copy ( 0, NumTimeFrames - 1, &clustertf );
                }

            } // if nummaps

        } // for nc1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1.2) distribute maps from cluster index to all others
    if ( index != UndefinedLabel ) {

        for ( TIteratorSelectedForward tfi ( index2tf ); (bool) tfi; ++tfi ) {

            labels.Reset ( tfi() );
            
                                        // search to which cluster it should be distributed
            double          maxcorr     = Lowest ( maxcorr );

                                        // scan again each cluster
            for ( int nc1 = 0; nc1 < nc; nc1++ )

                if ( nc1 != index )     // but not index!
                                        // !Don't allocate at all if negative correlation - final Labeling will fill the holes later on!
                                        // In fact, using correlation threshold of 0.5, instead 0, is the actual limit used since forever, and seems to give better results
                    if ( (*keepbestlabeling) (   tempmaps[ nc1 ],    Data[ tfi() ], maxcorr, 0.5 /*0*/  ) )

                        labels.SetLabel ( tfi(), nc1 );


            labels.UpdatePolarities ( Data, tfi(), tfi(), tempmaps, polarity );
            } // for index tf's

    
        OmpParallelFor
                                            // update remaining labeling by shifting down anything greater than to index
        for ( long tf = 0; tf < NumTimeFrames; tf++ )

            if ( labels.IsDefined ( tf ) && labels[ tf ] > index )

                labels[ tf ]--;              

                                            // same for maps
        if ( nc - 1 - index > 0 )

            for ( int i = index; i < nc - 1; i++ )

                tempmaps[ i ]   = tempmaps[ i + 1 ];

        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we have one less cluster
    nc--;                               


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Labels -> Maps

                                        // recompute all templates
    tempmaps.LabelingToCentroids    ( 
                                    Data,       &ToData, 
                                    nc, 
                                    labels, 
                                    polarity,   centroid,   ranking 
                                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time to save these precious intermediate results, for later calls
                                        // done here before any relabeling, i.e. straight from the aggregation part
    if ( init && nc <= maxclusters ) {
                                        // !force cropping the range of maps, we are NOT copying the whole tempmaps full size structure!            
        savedmaps.CopyFrom ( tempmaps, maxclusters );

        savedlabels = labels;
        
        init        = false;            // prevent from re-saving the maps again, just in (some unlikely, but still possible) case we might fall back to nc == maxclusters due to some relabeling
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Maps -> Labels
                                        // as we have new maps, the labeling has to be updated too,
                                        // when going back to the loop, we need this labeling to be correct
    Gauge.Next ();

    if ( ! GroupGauge.IsAlive () && CartoolObjects.CartoolApplication->IsInteractive () )
        CartoolObjects.CartoolApplication->SetMainTitle    ( Gauge );


//  if ( nc <= maxclusters ) { // refit only when needed
    if ( nc <= nclusters ) {   // refit only at the end

        tempmaps.CentroidsToLabeling    (   
                                        Data, 
                                        0,          NumTimeFrames - 1, 
                                        nc, 
                                        0,          labels, 
                                        polarity,   IgnoreCorrelationThreshold 
                                        );

        tempmaps.LabelingToCentroids    ( 
                                        Data,       &ToData,
                                        nc,    
                                        labels, 
                                        polarity,   centroid,   ranking 
                                        );

                                        // compact maps & labels, update effective number of maps
        nc      = labels.PackLabels ( tempmaps );
        } // if ! simpler

    } // for nc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer final maps
maps.CopyFrom ( tempmaps, nclusters );
                                        // In some rare occurences, some maps might have disappeared, usually after a relabeling
                                        // Pack the labels so we return some tidy labeling & the actual final number of maps
return  labels.PackLabels ( maps );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
