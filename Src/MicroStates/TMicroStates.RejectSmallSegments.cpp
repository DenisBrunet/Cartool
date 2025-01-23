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
                                        // Distribute a given chunk of time to its neighbors
void    TMicroStates::DistributeChunk   (   long               tfmin,      long            tfmax,
                                            long               segfrom,    long            segto,
                                            TMaps&             maps,       TLabeling&      labels,
                                            PolarityType       polarity,   double          limitcorr 
                                        )
{
LabelType           l;

                                        // distribute this short segment to neighbors
                                        // nobody on the left side?
if ( segfrom == tfmin ) {

    double              cor;
    LabelType           segr            = labels [ NoMore ( tfmax, segto + 1 ) ];    // could be UndefinedLabel


    for ( long tf = segfrom; tf <= segto; tf++ ) {
                                        // do we need to actually compute the correlation?
        cor     = segr == UndefinedLabel || limitcorr <= MinCorrelationThreshold    ? 1.0            : Project ( maps[ segr ], Data[ tf ], polarity );
        l       = segr == UndefinedLabel || cor       <  limitcorr                  ? UndefinedLabel : segr;

        labels.SetLabel ( tf, l );
        }
    } // no left side


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // nobody on the right side?
else if ( segto == tfmax ) {

    double              col;
    LabelType           segl            = labels [ AtLeast ( tfmin, segfrom - 1 ) ]; // could be UndefinedLabel


    for ( long tf = segfrom; tf <= segto; tf++ ) {
                                        // do we need to actually compute the correlation?
        col     = segl == UndefinedLabel || limitcorr <= MinCorrelationThreshold    ? 1.0            : Project ( maps[ segl ], Data[ tf ], polarity );
        l       = segl == UndefinedLabel || col       <  limitcorr                  ? UndefinedLabel : segl;

        labels.SetLabel ( tf, l );
        }

    } // no right side


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // split between left and right segs
else {
    double              cor;
    double              col;
    LabelType           segl            = labels [ segfrom - 1 ];   // not tfmin
    LabelType           segr            = labels [ segto   + 1 ];   // not tfmax
    long                tf;
    long                tfl;
    long                tfr;
    long                tfm;


                                        // from the left, assign to left seg
    for ( tf = segfrom; tf <= segto; tf++ ) {
                                        // do we need to actually compute the correlation?
        col     = segl == UndefinedLabel ? 1.0 : Project ( maps[ segl ], Data[ tf ], polarity );
        cor     = segr == UndefinedLabel ? 1.0 : Project ( maps[ segr ], Data[ tf ], polarity );

                                        // reached limit where right starts to be better than left?
        if ( segl != UndefinedLabel && segr != UndefinedLabel   // don't break if any side is UndefinedLabel, loop through the whole epoch
          && col < cor )                                        // left part is lower than right part, stop
            break;

        l       = segl == UndefinedLabel && segr == UndefinedLabel ? UndefinedLabel             // surrounded by UndefinedLabel -> UndefinedLabel
                : segl == UndefinedLabel ? ( cor <  limitcorr      ? UndefinedLabel : segr )    // left  is UndefinedLabel -> use right
                : segr == UndefinedLabel ? ( col <  limitcorr      ? UndefinedLabel : segl )    // right is UndefinedLabel -> use left
                :                            col <  limitcorr      ? UndefinedLabel : segl;     // here col >= cor, just check for limitcorr

        labels.SetLabel ( tf, l );
        }

                                        // are we done: any UndefinedLabel cases or left correlation wins it all?
    if ( tf > segto )
        return;

                                        // here we need to address the right part - tfl has NOT been assigned
                                        // here there are no UndefinedLabel cases
    tfl     = tf;

                                        // from the right, assign to right seg
    for ( tf = segto; tf >= tfl; tf-- ) {
        
        col     = Project ( maps[ segl ], Data[ tf ], polarity );
        cor     = Project ( maps[ segr ], Data[ tf ], polarity );

                                        // reached limit where left starts to be better than right?
        if ( cor < col )
            break;

        l       =                            cor <  limitcorr      ? UndefinedLabel : segr;     // here cor >= col, just check for limitcorr
        
        labels.SetLabel ( tf, l );
        }

                                        // tfr is NOT assigned
    tfr     = tf;

                                        // has left growth met the right growth?
    if ( tfr < tfl )
        return;
                                        // there is finally a central part remaining that is simply split in two
                                        // we have to cut somewhere, between the 2 limits
    tfm     = ( tfr + tfl + 1 ) / 2;

    for ( tf = tfl; tf <= tfr; tf++ ) {
        
        l       = tf < tfm ? segl : segr;  // attribute to the left or right segments

        col     = Project ( maps[ l ], Data[ tf ], polarity );

        if ( col < limitcorr )
            l   = UndefinedLabel;
        
        labels.SetLabel ( tf, l );
        }

    } // left & right side


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update all polarity flags
labels.UpdatePolarities ( Data, segfrom, segto, maps, polarity );


//for ( tf = segfrom; tf <= segto; tf++ )
//    ooo << "\t" << labels[ tf ];
//ooo << "\n";
}


//----------------------------------------------------------------------------
                                        // Reject small segments, given a labeling
                                        // filemin, filemax: files to be considered, though sequentially
                                        // fromtf, totf: inside each file, a range of TF to be considered

                                        // There could be an optional flag to change the behavior: Delete / ForceMerge / MergeAboveCorrelation
void    TMicroStates::RejectSmallSegments   (
                                            int             filemin,    int             filemax,    
                                            long            fromtf,     long            totf,
                                            TMaps&          maps,       TLabeling&      labels,
                                            PolarityType    polarity,   double          limitcorr,
                                            int             rejectsize 
                                            )
{
long                tfmin;
long                tfmax;


for ( int f = filemin; f <= filemax; f++ ) {

    TimeRangeToDataRange ( f, fromtf, totf, tfmin, tfmax );

    RejectSmallSegments (   
                        tfmin,      tfmax, 
                        maps,       labels, 
                        polarity,   limitcorr, 
                        rejectsize 
                        );
    }

}


//----------------------------------------------------------------------------
void    TMicroStates::RejectSmallSegments   (   
                                            long            tfmin,      long            tfmax,
                                            TMaps&          maps,       TLabeling&      labels,
                                            PolarityType    polarity,   double          limitcorr,
                                            int             rejectsize
                                            )
{
LabelType           currlabel;
long                currfrom;
long                currto;
TArray1<uchar>      deleting ( MaxNumTF );


Gauge.Blink ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Cumulate all segments to be deleted into a single time line
                                        //    because all small segments at calling time have to disappear!

deleting.ResetMemory ();

                                        // beginning of first segment
currlabel   = labels [ tfmin ];
currfrom    = tfmin;


for ( long tf = tfmin; tf <= tfmax; tf++ ) {

    if ( tf < tfmax && labels[ tf + 1 ] == currlabel ) 

        continue;
                                        // reached end of segment, or end of data which is also the end of current segment

                                        // is segment smaller than given limit?
                                        // don't change undefined segments, they are not REAL segment after all
    if (    currlabel != UndefinedLabel 
         && tf - currfrom + 1 <= rejectsize )
                                    // set this time interval for deletion
        for ( long tf2 = currfrom, tf0 = tf2 - tfmin; tf2 <= tf; tf2++, tf0++ )

            deleting[ tf0 ] = true;

                                        // new segment is starting from here
    currlabel   = tf < tfmax ? labels[ tf + 1 ] : UndefinedLabel;
    currfrom    = tf + 1;
    } // for tf


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Scan the cumulated times for time chunks to be deleted & distributed to neighbors

for ( currfrom = tfmin; currfrom <= tfmax; currfrom++ ) {

    if ( ! deleting[ currfrom - tfmin ] )
        continue;

    for ( currto = currfrom; currto <= tfmax; currto++ )
        if ( currto == tfmax || ! deleting[ currto - tfmin + 1 ] )
            break;

                                        // here [currfrom..currto] is to be deleted

    DistributeChunk ( tfmin, tfmax, currfrom, currto,
                      maps, labels, polarity, limitcorr );


    currfrom    = currto;
    }

                                        // !Do NOT alter the maps inside the deletion loop, either as topograhy, or as indexes!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


Gauge.EndBlink ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
