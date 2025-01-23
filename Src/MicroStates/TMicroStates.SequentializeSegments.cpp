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
//      New Sequentialization:
// Keep together the bits that share some time overlap,
// otherwise, split them. All in 1 pass, no parameters needed.
// Ex:
//              1   11  111
//              11       11
// becomes
//              1   22  333
//              11       33

int     TMicroStates::SequentializeSegments (   int             nclusters,  
                                                TMaps&          maps,       TLabeling&          labels, 
                                                PolarityType    polarity,   
                                                CentroidType    centroid,
                                                bool            ranking
                                            )
{
int                 newnclusters    = nclusters;    // next cluster starting from this
TArray1<int>        allfiles ( MaxNumTF );
long                begintf;
long                endtf;
long                tfmin;
long                tfmax;


                                        // scan all existig clusters
for ( int nc = 0; nc < nclusters; nc++ ) {

                                        // cumulate all files for current cluster into 1 bool array
    allfiles    = 0;

    for ( int  f  = 0; f  < NumFiles;   f++  ) {

        TimeRangeToDataRange ( f, 0, MaxNumTF - 1, tfmin, tfmax );

        for ( long tf = tfmin, tf0 = 0; tf <= tfmax; tf++, tf0++ )
                                        // current labels?
            if ( labels [ tf ] == nc )
                                        // overwrite if not allocated yet, or if a beginning as the current file already began
                if ( allfiles[ tf0 ] <= 0 )
                                        // beginning of segment? -> coded with a negative index for later check, to be able to separate 2 consecutive but non-overlapping segments
                    allfiles[ tf0 ]  = tf0 == 0 || labels [ tf - 1 ] != nc ? - ( f + 1 ) 
                                                                           :   ( f + 1 );
        } // for file


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan the cumulated track
    begintf     = 0;

    do {
                                        // scan for separate segments, search for the next beginning
        for ( ; begintf < MaxNumTF; begintf++ )
                                        // any non-null will code for a beginning
            if ( allfiles[ begintf ] != 0 )
                break;

                                        // no more segment for this current cluster?
        if ( begintf == MaxNumTF )
            break;

                                        // here a segment begins
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // search for segment's end
        for ( endtf = begintf; endtf < MaxNumTF; endtf++ )
              // stop if either last TF, or the beginning of a non-overlapping segment (<0), or no more segment (==0)
            if ( endtf == MaxNumTF - 1 || allfiles[ endtf + 1 ] <= 0 )
                break;

                                        // here a segment ends
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert all overlapping segments to next cluster index
        for ( int  f  = 0; f  < NumFiles;   f++  ) {

            TimeRangeToDataRange ( f, begintf, endtf, tfmin, tfmax );

            for ( long tf = tfmin; tf <= tfmax; tf++ )
                if ( labels [ tf ] == nc )
                    labels [ tf ]    = newnclusters;
            }

                                        // ready for next cluster - or it will be the maximum number of new clusters either
        newnclusters++;
                                        // next segment after endtf
        begintf     = endtf + 1;

        } while ( true );

    } // for nc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // recompute the template maps

maps.Resize ( newnclusters, NumRows );   


maps.LabelingToCentroids    ( 
                            Data,       &ToData, 
                            newnclusters, 
                            labels, 
                            polarity,   centroid,   ranking 
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pack labels and return the final number of maps
return  labels.PackLabels ( maps );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
