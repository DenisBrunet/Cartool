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
int     TMicroStates::MergeCorrelatedSegments   (   int             nclusters,
                                                    TMaps&          maps,           TLabeling&          labels,
                                                    PolarityType    polarity,       
                                                    CentroidType    centroid,
                                                    bool            ranking,
                                                    double          corrthresh 
                                                )
{
                                        // nothing to do?
if ( nclusters <= 1 )
    return  nclusters;


double              highestcorr;
LabelType           index1;
LabelType           index2;
TMap                map12 ( NumRows );



do {
                                        // index1 < index2
    highestcorr     = maps.GetClosestPair ( nclusters, polarity, index1, index2 );

                                        // max correlation of any pair of maps below threshold?
    if ( highestcorr < corrthresh )
        break;                          // we're done


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // anticipate the averaging of the 2 labeling
                                        // by averaging their 2 prototype maps (an approximation)
    map12  =         maps[ index1 ];

    map12.Cumulate ( maps[ index2 ], polarity == PolarityEvaluate && map12.IsOppositeDirection ( maps[ index2 ] ) );

    map12.Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update labeling
    OmpParallelFor

    for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

        if ( labels[ tf ] == index1 || labels[ tf ] == index2 )
                                        // merge index2 into index1 (the smallest index value)
            labels.SetLabel ( tf, index1, polarity == PolarityEvaluate && map12.IsOppositeDirection ( Data[ tf ] ) ? PolarityInvert : PolarityDirect );


        if ( labels[ tf ] > index2 )
            labels[ tf ]--;             // shift labels after index2
        }

                                        // already done for index1 / index2
//  labels.UpdatePolarities ( Data, 0, NumTimeFrames - 1, maps, polarity );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // labels -> maps
    maps.Reset ( nclusters );

                                        // here, one less cluster here
    nclusters--;


    maps.LabelingToCentroids    ( 
                                Data,       &ToData, 
                                nclusters, 
                                labels, 
                                polarity,   centroid,   ranking 
                                );


    } while ( nclusters > 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pack and get exact number of final maps
return  labels.PackLabels ( maps );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
