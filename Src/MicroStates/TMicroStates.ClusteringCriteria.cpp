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

#include    "Math.Utils.h"
#include    "Math.Resampling.h"
#include    "Math.Histo.h"

#include    "TMicroStates.h"            // segnumvar...

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Using the first or second derivative of a criterion can help improving an otherwise weak criterion
                                        // datai and datao can be the same
void    CriterionToDerivative   (   const double*   datai,          double*         datao,          int         datasize,
                                    int             minclusters,    int             maxclusters,
                                    CriterionDerivativeEnum     flag,
                                    double          scale
                                )
{
                                        // have a safe copy of the original data
TVector<double>     orgvar ( datasize );

for ( int i = 0; i < datasize; i++ ) {

    orgvar ( i )    = datai[ i ];
    datao[ i ]      = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // to get rid of the strong deceleration from many criterion, it could be a good idea to switch the data to ranks before applying the derivative
if ( IsFlag ( flag, CriterionDerivativeOnRanks ) ) {

    int                 minclustersr    = AtLeast ( 0,              minclusters - CriterionMargin );
    int                 maxclustersr    = NoMore  ( datasize - 1,   maxclusters + CriterionMargin );

    RankCriterion       ( orgvar, orgvar, minclustersr, maxclustersr, RankCriterionLinear );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              vm1;
double              v0;
double              vp1;


for ( int ncl = minclusters; ncl <= maxclusters; ncl++ ) {
                                        // get values around current center
    vm1                 = orgvar ( AtLeast ( 1,             ncl - 1 ) );
    v0                  = orgvar (                          ncl       );
    vp1                 = orgvar ( NoMore  ( datasize - 1,  ncl + 1 ) );    // !We know we have at least 1 more cluster past the max, precisely for these kind of side effects computation!


    if      ( IsFlag ( flag, CriterionDerivativePrime  ) )  datao[ ncl ]    = ( vp1 - vm1 ) / 2;
    else if ( IsFlag ( flag, CriterionDerivativeSecond ) )  datao[ ncl ]    = vm1 + vp1 - 2 * v0;
    else                                                    datao[ ncl ]    = 0;

    datao[ ncl ]   *= scale;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // also force clear these edge values
datao[ 0 ]  = 0;
datao[ 1 ]  = 0;

if ( IsFlag ( flag, CriterionDerivativeSecond ) )
    datao[ 2 ]  = 0;
}


//----------------------------------------------------------------------------
void    SmoothCurve         (   double*         data,       int             ncl1,   int         ncl2, 
                                FilterTypes     filtertype, FctParams&      params  
                            )
{
int                 Dim1            = ncl2 - ncl1 + 1;

if ( Dim1 <= 1 )
    return;

TVector<double>     line ( Dim1 );


for ( int nc = ncl1; nc <= ncl2; nc++ )
    line ( nc - ncl1 ) = data[ nc ];

                                        // put filter type in parameter
line.Filter ( filtertype, params, false );


for ( int nc = ncl1; nc <= ncl2; nc++ )
    data[ nc ]  = line ( nc - ncl1 );
}


//----------------------------------------------------------------------------
void    GetMinMax           (   const double*   data,       int             ncl1,   int         ncl2, 
                                double&         minv,       double&         maxv, 
                                bool            ignorenull 
                            )
{
minv    = Highest ( minv );
maxv    = Lowest  ( maxv );

for ( int nc = ncl1; nc <= ncl2; nc++ )

    if ( ! ( ignorenull && data[ nc ] == 0 ) ) {

        Mined ( minv, data[ nc ] );
        Maxed ( maxv, data[ nc ] );
        }
}


//----------------------------------------------------------------------------
                                        // Will force reset all data before ncl1 to 0
void    NormalizeCriterion  (   double*             data,
                                int                 ncl1,       int                 ncl2, 
                                NormalizeCurveFlag  minflag,    NormalizeCurveFlag  maxflag,    bool ignorenull 
                            )
{
                                        // index 0 is always null
                                        // also reset all data before ncl1, if needed
for ( int nc = 0; nc < AtLeast ( 1, ncl1 ); nc++ )

    data[ nc ]  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              minv;
double              maxv;

                                        // optionally doesn't account for null values to compute the scaling
GetMinMax ( data, ncl1, ncl2, minv, maxv, ignorenull );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              offset;
double              ratio;

                                        // Erroneous min / max
if ( minv == Highest ( minv ) || maxv == Lowest ( maxv ) ) {
    offset  = 0;
    ratio   = 1;
    }

else {
                                        // !Not all combinations make sense!
    if      ( maxflag == NormalizeTo1 ) {

        if      ( minflag == NormalizeNone ) {  // [0   ..maxv] -> [0..1]
            offset  = 0;
            ratio   = 1 / NonNull ( maxv );
            }
        else if ( minflag == NormalizeTo0       // [minv..maxv] -> [0..1]
               || minflag == NormalizeToE ) {   // [minv..maxv] -> [E..1]
            offset  = minv;
            ratio   = 1 / NonNull ( maxv - minv );
            }
                                                
        if ( minflag == NormalizeToE ) {
                                        // adjust offset and ratio to make some room for Epsilon
            offset -= ( NormalizedEpsilon / ( 1 - NormalizedEpsilon ) ) / ratio;
            ratio  *= 1 - NormalizedEpsilon;
            }
        }

    else if ( maxflag == NormalizeTo0 
           || maxflag == NormalizeToE ) {

        if      ( minflag == NormalizeNone ) {  // [0   ..maxv] -> [1..0]
            offset  = maxv;
            ratio   = 1 / NonNull ( - maxv );
            }
        else if ( minflag == NormalizeTo1 ) {   // [minv..maxv] -> [1..0]
            offset  = maxv;
            ratio   = 1 / NonNull ( minv - maxv );
            }
                                                // [X   ..maxv] -> [1..E]
        if ( maxflag == NormalizeToE ) {
                                        // adjust offset and ratio to make some room for Epsilon
            offset -= ( NormalizedEpsilon / ( 1 - NormalizedEpsilon ) ) / ratio;
            ratio  *= 1 - NormalizedEpsilon;
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int nc = ncl1; nc <= ncl2; nc++ )
                                        // if optionally doesn't account for null values, also let them unscaled(?)
    if ( ! ( ignorenull && data[ nc ] == 0 ) )

        data[ nc ]  = Clip ( ( data[ nc ] - offset ) * ratio, 0.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // Will force reset all data before ncl1 to 0
void    StandardizeCriterion    (   double*     data, 
                                    int         ncl1,   int     ncl2,
                                    bool        ignorenull 
                                )
{
                                        // index 0 is always null
                                        // also reset all data before ncl1, if needed
for ( int nc = 0; nc < AtLeast ( 1, ncl1 ); nc++ )

    data[ nc ]  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEasyStats          stat;


for ( int nc = ncl1; nc <= ncl2; nc++ )

    if ( ! ( ignorenull && data[ nc ] == 0 ) )

        stat.Add ( data[ nc ], ThreadSafetyIgnore );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int nc = ncl1; nc <= ncl2; nc++ )

    data[ nc ]  = stat.ZScore ( data[ nc ] );
}


//----------------------------------------------------------------------------
                                        // data can be any value, positive and negative, the whole curve will be ranked from min to max
                                        // however 0 values are explicitly skipped
                                        // ranks output variable could be the same as the input variable
                                        // Will force reset all data before ncl1 to 0
void    RankCriterion   (   const double*       datai,  double*             datao,
                            int                 ncl1,   int                 ncl2, 
                            RankCriterionFlag   flag
                        )

{
                                        // this used to be called before, as to have everything meaningful to be positive
//NormalizeCriterion  ( orgvar, minclustersr, maxclustersr, NormalizeToE, NormalizeTo1, true );


enum                {
                    SortDataIndex   = 0,
                    SortDataValue,
                    SortDataRank,
                    SortDataNum
                    };

TArray2<double>     sortdata ( ncl2 + 1, SortDataNum );

sortdata    = 0;

for ( int nc = 0; nc <= ncl2; nc++ ) {

    sortdata( nc, SortDataIndex )   = nc;
    sortdata( nc, SortDataValue )   = nc < ncl1 /*|| datai[ nc ] == 0*/ ? Lowest<double> () : datai[ nc ];  // null data is sent to Lowest value - not anymore, let 0 from criterion be a legal value, results are slightly better
    sortdata( nc, SortDataRank  )   = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by values
sortdata.SortRows ( SortDataValue, Descending );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numcl           = ncl2 - ncl1 + 1;
int                 numdata         = 0;
int                 numranks        = 0;

                                        // count only the original non-null values
                                        // also account for strictly equal values - in that case assign also the same rank
for ( int nc = 0; nc < numcl; nc++ ) {
                                        // values to ignore
    if ( sortdata ( nc, SortDataValue ) != Lowest<double> () )
        numdata++;
    else 
        break;


    if ( nc == 0 
      || nc != 0 && sortdata ( nc, SortDataValue ) != sortdata ( nc - 1, SortDataValue ) )
                                        // count only the number of different values
        numranks++;

                                        // index of strictly different values
    sortdata ( nc, SortDataRank )   = numranks;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now we can reset ranks, in case ranks is also data...
for ( int nc = 0; nc <= ncl2; nc++ )

    datao[ nc ] = 0;


for ( int ri = 0; ri < numdata; ri++ )

    if      ( flag == RankCriterionLinear  )    datao[ (int) sortdata ( ri, SortDataIndex ) ]   =          ( numranks + 1 - sortdata ( ri, SortDataRank ) ) / (double) numranks;    // weight by Reciprocal Rank (with non-null values)  3/3   2/3   1/3
    else if ( flag == RankCriterionSquare  )    datao[ (int) sortdata ( ri, SortDataIndex ) ]   = Square ( ( numranks + 1 - sortdata ( ri, SortDataRank ) ) / (double) numranks );  // weight by relative rank (non-null values)  3/3   2/3   1/3
    else if ( flag == RankCriterionInverse )    datao[ (int) sortdata ( ri, SortDataIndex ) ]   = 1 / (double) ( sortdata ( ri, SortDataRank ) );                                   // absolute weight for winners

}


//----------------------------------------------------------------------------
                                        // General function to compute the Within / Across Clusters Distances
                                        // !TEasyStats objects are not reset here, because caller can either want a per-cluster stat, or a total within clusters stat!
// Other possible flags:
// - Real data / Normalized data

void    TMicroStates::ComputeW  (   int                 nc,
                                    const TMaps&        maps,       const TLabeling&    labels,
                                    PolarityType        polarity,
                                    WFlag               flags,
                                    TEasyStats*         statcluster,
                                    TEasyStats*         statnoncluster,
                                    TEasyStats*         statall,
                                    TEasyStats*         indexall
                                )
{
long                maxmaps         = 4000;     // ?not sure if up-to-date with new Cartool64?
TDownsampling       downmaps ( NumTimeFrames, maxmaps );


                                        // save this if some criterion needs to convert from real indexes to downsampled indexes
CriterionDownSampling   = downmaps.Step;

// /*if ( VkQuery () )*/ DBGV5 ( nc, NumTimeFrames, maxmaps, downmaps.Step, numtf, "ComputeW: nc, NumTimeFrames, maxmaps, step, numtf" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ASymmetricMatrix    MatrixW;
int                 nummaps;


if ( IsFlag ( flags, WPooledDeterminant ) ) {

    nummaps     = labels.GetSizeOfClusters ( nc, nc, downmaps.Step );


    if ( nummaps == 0 )
        return;
                                        // create matrix
    MatrixW.AResizeZero ( nummaps,nummaps );
    } // WPooledDeterminant


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do we need to scan data outside the current cluster? try to avoid that if possible, this is costly...
bool                scannoncluster  = statnoncluster != 0 
                                   || statall        != 0 
                                   || indexall       != 0;

                                        // Within cluster sum of distance to centroid
for ( long tf = downmaps.From, Wj = -1; tf <= downmaps.To; tf += downmaps.Step ) {

    if ( labels.IsUndefined ( tf ) )
        continue;

                                        // update current column index (of lower triangular distance matrix)
    if ( labels[ tf ] == nc )
        Wj++;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if      ( IsFlag ( flags, WFromCentroid ) ) { // compute all distance from centroid (template)

                                        // avoid computing distance if caller didn't request some stats about them
        if ( ! ( labels[ tf ] == nc || scannoncluster ) )
            continue;

                                        // from template we can use negmap
        PolarityType    pol     = labels.GetPolarity ( tf );

                                        // topographical distances
        double          d       = CorrelationToSquareDifference ( Project ( maps[ nc ], Data[ tf ], pol ) );

                                        // either distance or square distance
        if ( IsFlag ( flags, WDistance ) )
            d   = sqrt ( d );

                                        // up to the caller, which stats are of interest
        if ( statcluster    && labels[ tf ] == nc )     statcluster     ->Add ( d,  ThreadSafetyIgnore  );
        if ( statnoncluster && labels[ tf ] != nc )     statnoncluster  ->Add ( d,  ThreadSafetyIgnore  );
        if ( statall                              )     statall         ->Add ( d,  ThreadSafetyIgnore  );
        if ( indexall                             )     indexall        ->Add ( tf, ThreadSafetyIgnore );
        } // WFromCentroid


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( IsFlag (  flags, WFlag ( WPooled | WPooledDeterminant ) ) ) {
                                        // scan all non-duplicate pairs of maps
        for ( long tf2 = tf + downmaps.Step, Wi = Wj; tf2 <= downmaps.To; tf2 += downmaps.Step ) {

            if ( labels.IsUndefined ( tf2 ) )
                continue;

                                        // update current line index (of lower triangular distance matrix)
            if ( labels[ tf2 ] == nc )
                Wi++;

                                        // avoid computing distance if caller didn't request some stats about them
            if ( labels[ tf ] != nc || ! ( labels[ tf2 ] == nc || scannoncluster ) )
                continue;

                                        // get the right sign between the pair (negmap is of no use here, as it only relates the maps against the template)
            PolarityType    pol     = polarity == PolarityEvaluate && Data[ tf ].IsOppositeDirection ( Data[ tf2 ] ) ? PolarityInvert : PolarityDirect;

                                        // this is for normalized data, i.e. for the template aspect
            double          d       = CorrelationToSquareDifference ( Project ( Data[ tf ], Data[ tf2 ], pol ) );

                                        // either distance or square distance
            if ( IsFlag ( flags, WDistance ) )
                d   = sqrt ( d );

                                        // up to the caller, which stats are of interest
            if      ( IsFlag ( flags, WPooled ) ) {

                if ( statcluster    && labels[ tf ] == nc && labels[ tf2 ] == nc )      statcluster     ->Add ( d,  ThreadSafetyIgnore  );
                if ( statnoncluster && labels[ tf ] != labels[ tf2 ]             )      statnoncluster  ->Add ( d,  ThreadSafetyIgnore  );
                if ( statall                                                     )      statall         ->Add ( d,  ThreadSafetyIgnore  );
                if ( indexall                                                    )    { indexall        ->Add ( tf, ThreadSafetyIgnore ); indexall        ->Add ( tf2, ThreadSafetyIgnore ); }
                }

            else if ( IsFlag ( flags, WPooledDeterminant ) ) {

//              MatrixW ( Wi, Wi )          = 0;    // not needed, whole matrix has been initialized to 0, but just in case another measure would be use

                                        // just to be sure and avoid crashing, though it shouldn't happen
                if ( Wi < MatrixW.n_rows && Wj < MatrixW.n_cols )
                                        // matrix is symmetrical
                    MatrixW ( Wi, Wj )  = 
                    MatrixW ( Wj, Wi )  = d;
                }
            } // for tf2

        } // WPooled || WPooledDeterminant

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if ( IsFlag ( flags, WPooledDeterminant ) ) {
                                        // Naive approach - will certainly crash
//  double              det             = MatrixW.determinant ();
                                        // Using Log Det instead
    double              logdet          = log_det ( MatrixW ).real ();

                                        // after an exp, the sum will hold the product of all determinants
                                        // the only option right now is the within cluster determinant
    if ( statcluster )  statcluster     ->Add ( logdet /*det*/, ThreadSafetyIgnore );
    } // WPooledDeterminant

}


//----------------------------------------------------------------------------
                                        // Compute once for all the Within and Across Clusters distances
                                        // !nclusters is the index, while nummaps the actual number of maps!
void    TMicroStates::ComputeAllWBA (   int                 nclusters,  int                 nummaps,
                                        const TMaps&        maps,       const TLabeling&    labels,
                                        PolarityType        polarity,
                                        TArray2<double>&    var
                                    )
{
                                        // Parallelized by types of statistics instead of per map - this will avoid conflicts while adding values to the TEasyStats objects
                                        // !Each parallel section uses different variables, so are thread-safe!
OmpParallelSectionsBegin

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OmpSectionBegin

StatWCentroidDistance       .Resize ( 1024 ); // Reset ();

for ( int nc = 0; nc < nummaps; nc++ )
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WFromCentroid        | WDistance         ), &StatWCentroidDistance,          0,                              0,                      0                           );

OmpSectionEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OmpSectionBegin

StatWCentroidSquareDistance .Resize ( 1024 ); // Reset ();
StatBCentroidSquareDistance .Resize ( 1024 ); // Reset ();

for ( int nc = 0; nc < nummaps; nc++ )
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WFromCentroid        | WSquareDistance   ), &StatWCentroidSquareDistance,    &StatBCentroidSquareDistance,   0,                      0                           );

OmpSectionEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OmpSectionBegin

StatWPooledDistance         .Resize ( 1024 );   // !Use copies if Median or any re-ordering is needed, as to keep the data ordered as they were input!
StatBPooledDistance         .Resize ( 1024 );
StatAPooledDistance         .Resize ( 1024 );
StatAPooledDistanceIndex    .Resize ( 1024 );

for ( int nc = 0; nc < nummaps; nc++ )
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WPooled              | WDistance         ), &StatWPooledDistance,            &StatBPooledDistance,           &StatAPooledDistance,   &StatAPooledDistanceIndex   );

OmpSectionEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OmpSectionBegin

StatWPooledSquareDistance   .Resize ( 1024 ); // Reset ();

for ( int nc = 0; nc < nummaps; nc++ )
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WPooled              | WSquareDistance   ), &StatWPooledSquareDistance,      0,                              0,                      0                           );

OmpSectionEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//OmpSectionBegin
//StatWPooledDetSquareDistance.Resize ( 1024 ); // Reset ();
//
//for ( int nc = 0; nc < nummaps; nc++ )
//  ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WPooledDeterminant   | WSquareDistance   ), &StatWPooledDetSquareDistance,   0,                              0,                      0                           );
//
//OmpSectionEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelSectionsEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Store all these means for later
//var ( segWCD,   nclusters ) = StatWCentroidDistance         .Mean ();
//var ( segWCD2,  nclusters ) = StatWCentroidSquareDistance   .Mean ();
//var ( segBCD2,  nclusters ) = StatBCentroidSquareDistance   .Mean ();
//var ( segWPD,   nclusters ) = StatWPooledDistance           .Mean ();
//var ( segBPD,   nclusters ) = StatBPooledDistance           .Mean ();
//var ( segAPD,   nclusters ) = StatAPooledDistance           .Mean ();
//var ( segWPD2,  nclusters ) = StatWPooledSquareDistance     .Mean ();
//var ( segWPdD2, nclusters ) = StatWPooledDetSquareDistance  .Mean ();

                                        // Robust versions
OmpParallelSectionsBegin

OmpSectionBegin     var ( segWCD,   nclusters ) =              StatWCentroidDistance         .Median ();    OmpSectionEnd
OmpSectionBegin     var ( segWCD2,  nclusters ) =              StatWCentroidSquareDistance   .Median ();    OmpSectionEnd
OmpSectionBegin     var ( segBCD2,  nclusters ) =              StatBCentroidSquareDistance   .Median ();    OmpSectionEnd
OmpSectionBegin     var ( segWPD,   nclusters ) = TEasyStats ( StatWPooledDistance          ).Median ();    OmpSectionEnd   // !work with a copy to keep the original ordering!
OmpSectionBegin     var ( segBPD,   nclusters ) = TEasyStats ( StatBPooledDistance          ).Median ();    OmpSectionEnd   // !work with a copy to keep the original ordering!
OmpSectionBegin     var ( segAPD,   nclusters ) = TEasyStats ( StatAPooledDistance          ).Median ();    OmpSectionEnd   // !work with a copy to keep the original ordering!
OmpSectionBegin     var ( segWPD2,  nclusters ) =              StatWPooledSquareDistance     .Median ();    OmpSectionEnd
OmpSectionBegin     var ( segWPdD2, nclusters ) = 0;         /*StatWPooledDetSquareDistance  .Median ();*/  OmpSectionEnd

OmpParallelSectionsEnd
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Compute cross-validation from templates & labeling
                                        // Actually returns 1 - CV
double  TMicroStates::ComputeCrossValidation    (   int             nclusters,  int             numelectrodes,
                                                    TMaps&          maps,       TLabeling&      labels,
                                                    PolarityType    polarity,
                                                    long            tfmin,      long            tfmax 
                                                )
{
int                 denom           = numelectrodes - 1 - nclusters;

                                        // not enough electrodes for the number of clusters
if ( denom <= 0 )
    return  1 - 0;

                                        // Original formula
                                        // With Difference, we need the polarity flag
double              sigma2          = ComputeSigmaMu2 ( numelectrodes, maps, labels, polarity, tfmin, tfmax );

double              cv              = sigma2 * Square ( ( numelectrodes - 1 ) / (double) denom );

                                        // Another formula (AIC)?
//double              cv              =   NumTimeFrames * log ( sigma2 ) + 2 * nclusters;


return  1 - cv;
}


//----------------------------------------------------------------------------
                                        // Compute dispersion W on normalized data
                                        // Pooled within-cluster sum of squares around cluster mean (Sum of within cluster variance, computed with robust statistics - no centroid used)
double  TMicroStates::ComputeKrzanowskiLaiW (   int                 nclusters,
                                                TMaps&              maps,       TLabeling&      labels,
                                                PolarityType        polarity 
                                            )
{
int                 nummaps;
double              w               = 0;
TEasyStats          statw; // ( 1000 );


                                        // scan each cluster
for ( int nc = 0; nc < nclusters; nc++ ) {

    statw.Reset ();

                                        // compute squared centroid distances - reset stats for each cluster
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WPooled | WSquareDistance ), &statw, 0, 0, 0 );

                                        // algebraic formula  (number of pairs) -> (number of maps)
    nummaps     = statw.PairsToNumItems ();
                                        // cumulate all dispersions across all clusters
                                        // official version, with Mean
    w          += statw.Sum () / NonNull ( 2 * nummaps );

                                        // robust version, through rescaled median to approximate the Sum
//  w          += ( statw.GetNumItems () * statw.Median () ) / NonNull ( 2 * nummaps );


//    DBGV5 ( nclusters, nc, nummaps, statw.Mean (), statw.GetNumItems (), "nclusters, nc, nummaps, statw.Mean (), statw.GetNumItems ()" );
    } // for nc


return  w;
}

                                        // Deceleration / convexities of the error curve W above
                                        // Can compute either the original formula, or some more robust adaptation, w or w/o robust statistics for both cases
void    TMicroStates::ComputeKrzanowskiLai  (   int                 minclusters,  int             maxclusters,    int             /*numwfilters*/,
                                                CriterionDerivativeEnum     flag,
                                                TArray2<double>&    var 
                                            )
{
                                        // smooth out the W curve to get rid of some noise
//FctParams           p;
//p ( FilterParamDiameter )     = 1 + 2 * numwfilters;
//SmoothCurve ( var [ segSumWPD2 ], minclusters, maxclusters, FilterTypeGaussian, p );


TVector<double>         sumw ( maxclusters + 1 + CriterionMargin );

for ( int ncl = 0; ncl <= maxclusters + CriterionMargin; ncl++ )

    sumw ( ncl )    = var ( segSumWPD2, ncl );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // to get rid of the strong deceleration from many criterion, it could be a good idea to switch the data to ranks before applying the derivative
if ( IsFlag ( flag, CriterionDerivativeOnRanks ) ) {

    int                 minclustersr    = AtLeast ( 0,                  minclusters - CriterionMargin );
    int                 maxclustersr    = NoMore  ( var.GetDim2 () - 1, maxclusters + CriterionMargin );    // data has been allocated with that extra bucket already

    RankCriterion       ( sumw, sumw, minclustersr, maxclustersr, RankCriterionLinear );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double*             KL          = IsFlag ( flag, CriterionDerivativeOnRanks ) ? var[ segKrzanowskiLaiRobust  ] : var[ segKrzanowskiLai  ];
double*             KLC         = IsFlag ( flag, CriterionDerivativeOnRanks ) ? var[ segKrzanowskiLaiCRobust ] : var[ segKrzanowskiLaiC ];

for ( int ncl = 0; ncl <= maxclusters; ncl++ ) {

    KL [ ncl ]  = 0;
    KLC[ ncl ]  = 0;
    }


double              w1;
double              w2;
double              w3;
double              d1;
double              d2;
double              norm;
                                        // KL does not exist for 1 and max cluster (is always giving 2?)
                                        // also remove border as a side effect of filtering (alters the shape of the curve)
//for ( int ncl = AtLeast ( numwfilters + 1, minclusters ); ncl < maxclusters - numwfilters; ncl++ ) {
for ( int ncl = minclusters; ncl <= maxclusters; ncl++ ) {

                                        // Official Krzanowski - Lai
    w1          = sumw[ ncl - 1 ] * Power ( var ( segmaps, ncl - 1 ), 2.0 / (double) NumElectrodes );
    w2          = sumw[ ncl     ] * Power ( var ( segmaps, ncl     ), 2.0 / (double) NumElectrodes );
    w3          = sumw[ ncl + 1 ] * Power ( var ( segmaps, ncl + 1 ), 2.0 / (double) NumElectrodes );
                                                // DIFF
    d1          = w1 - w2;
    d2          = w2 - w3;

                                        // KL = DIFF(k) / DIFF(k+1) - from the original article
    KL [ ncl ]  = d1 / NonNull ( d2 );

                                        // Adapted formula: using a normalized Laplacian, less sensitive to division by "0"
//  norm        = w1;
//  norm        = ( w1 + w2 + w3 ) / 3;
    norm        = max ( w1, w2, w3 );   // best normalization

    KLC[ ncl ]  = ( d1 - d2 ) / NonNull ( norm );
    }

                                        // void 2 first KL: 1 because of missing data, 2 because of slope artifact
KL [ 1 ]    = 0;
KLC[ 1 ]    = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rather than smoothing W, smooth KL instead <- this
//FctParams           p;
//p ( FilterParamDiameter )     = 1 + 2 * numwfilters;
//SmoothCurve ( var [ segKrzanowskiLai ], minclusters, maxclusters, FilterTypeGaussian, p );
//p ( FilterParamDiameter )     = 1 + 2 * 2 * numwfilters;
//SmoothCurve ( var [ segKrzanowskiLai ], minclusters, maxclusters, FilterTypeMinMax, p );
}


//----------------------------------------------------------------------------
                                        // Calinski-Harabasz index
                                        // Ratio of [between clusters error] to [within clusters errors], using the centroids
double  TMicroStates::ComputeCalinskiHarabasz   (   int             nclusters )
{
double              trW;
double              trB;
int                 numtf;
//TMap              avgmap ( NumRows );
double              CHi;


                                        // in case we need the average map
//for ( long tf = 0; tf < NumTimeFrames; tf++ ) 
//    if ( labels.IsDefined ( tf ) )
//        avgmap += Data[ tf ];           // not straightforward in case of ignore polarity...


numtf   = StatWCentroidSquareDistance.GetNumItems ();


trW     = StatWCentroidSquareDistance.Sum ();


/*                                      // B, between cluster scatter matrix
                                        // upgrade ComputeW to compute against average / arbitrary map
                                        // average of all normalized data
avgmap     /= NonNull ( numtf );

trB         = 0;

for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;

                                        // official B, on real data
//  trB    += Square ( Norm[ tf ] ) * NormalizedDifference ( maps[ labels[ tf ] ], avgmap, polarity );
                                        // simplified B, on real data
//  trB    += Square ( Norm[ tf ] );
                                        // simplified B, on normalized data
//  trB    += 1;
    }
*/
                                        // simplified B, on normalized data (equivalent to above, but faster)

trB     = numtf;

                                        // final formula
if ( numtf - nclusters > 0 && nclusters - 1 > 0 )
    CHi     = ( trB * ( numtf - nclusters ) ) / ( trW * ( nclusters - 1 ) );
else
    CHi     = 0;


return  CHi;
}


//----------------------------------------------------------------------------
                                        // Ratio of [between clusters min distance] to [within clusters max distance]
double  TMicroStates::ComputeDunn   (   int               /*nclusters*/ )
{
                                        // Official Dunn index
double              dmin            = TEasyStats ( StatBPooledDistance ).Min ();    // between clusters
double              dmax            = TEasyStats ( StatWPooledDistance ).Max ();    // within clusters

                                        // Official formula: Highest between cluster and lowest within cluster
//double            Dunn            = dmin / NonNull ( dmax );
                                        // Using difference to avoid near-zero divisions
double              Dunn            = dmin - dmax;

return  Dunn;
}

                                        // Ratio of [between clusters min distance] to [within clusters max distance]
                                        // Adapted to use robust statistics
double  TMicroStates::ComputeDunnRobust (   int             /*nclusters*/ )
{
                                        // More robust - but not the same curve
//double            dmin            = TEasyStats ( StatBPooledDistance ).Quantile ( 0.10 ); // between clusters
//double            dmax            = TEasyStats ( StatWPooledDistance ).Quantile ( 0.90 ); // within clusters
//double            dmin            = TEasyStats ( StatBPooledDistance ).TruncatedMean ( 0.00, 0.25 );  // between clusters
//double            dmax            = TEasyStats ( StatWPooledDistance ).TruncatedMean ( 0.75, 1.00 );  // within clusters
double              dmin            = TEasyStats ( StatBPooledDistance ).TruncatedMean ( 0.00, 0.05 );  // between clusters
double              dmax            = TEasyStats ( StatWPooledDistance ).TruncatedMean ( 0.95, 1.00 );  // within clusters

                                        // Official formula
//double              DunnRobust      = dmin / NonNull ( dmax );
                                        // Using difference to avoid near-zero divisions
double              DunnRobust      = dmin - dmax;

return  DunnRobust;
}


//----------------------------------------------------------------------------
                                        // McClain index
double  TMicroStates::ComputeMcClain    (   int               /*nclusters*/ )
{
//double            bmean           = StatWPooledDistance.Mean ();     // between clusters
//double            wmean           = StatBPooledDistance.Mean ();     // within clusters
double              bmean           = TEasyStats ( StatWPooledDistance ).Median ();     // between clusters
double              wmean           = TEasyStats ( StatBPooledDistance ).Median ();     // within clusters

                                        // Official formula: looking for a max
//double            McClain         = wmean / NonNull ( bmean );
                                        // Using difference to avoid near-zero divisions
double              McClain         = wmean - bmean;

return  McClain;
}


//----------------------------------------------------------------------------
                                        // C-Index from Hubert & Levin
                                        // [Sum of within clusters pair distances] normalized by [Sum of all pair distances]
                                        // !Can be costly for large dataset!
                                        // Lower is best, so invert results
double  TMicroStates::ComputeCIndex     (   int               /*nclusters*/ )
{
                                        // !Use a copy to preserve the orignal's ordering!
TEasyStats          statwpooleddistance ( StatWPooledDistance );
TEasyStats          statapooleddistance ( StatAPooledDistance );


int                 numpairs        = statwpooleddistance.GetNumItems ();
                                        // ratio of within clusters pairs to total number of pairs
double              percentilecut   = numpairs / (double) statapooleddistance.GetNumItems ();
                                        // average the lowest numpairs of all pairs
double              dmin            = statapooleddistance.TruncatedMean ( 0,                 percentilecut );
                                        // average the biggest numpairs of all pairs
double              dmax            = statapooleddistance.TruncatedMean ( 1 - percentilecut, 1             );
                                        // average statwpooleddistance before, as we have averages in dmin and dmax
                                        // then normalize the sum of within clusters pairwise distances
//double            cindex          = ( statwpooleddistance.Mean () - dmin ) / NonNull ( dmax - dmin );
double              cindex          = ( statwpooleddistance.Median () - dmin ) / NonNull ( dmax - dmin );


//DBGV4 ( nclusters, numpairs, stata.GetNumItems (), percentilecut, "nclusters: numpairs, totalpairs, percentilecut" );
//DBGV5 ( nclusters, dmin, dmax, sumd, cindex, "nclusters: dmin, dmax, sumd -> cindex" );

                                        // invert to have a max
return  -cindex;
}


//----------------------------------------------------------------------------
                                        // Davies & Bouldin
                                        // [Sum of max distances between groups] normalized by [corresponding template distance]
                                        // Lower is best, so invert results
double  TMicroStates::ComputeDaviesBouldin  (   int                 nclusters,
                                                TMaps&              maps,       TLabeling&      labels,
                                                PolarityType        polarity 
                                            )
{
TArray1<double>     sumd ( nclusters );
TEasyStats          statw ( 1024 );

                                        // compute all within cluster average distances
for ( int nc = 0; nc < nclusters; nc++ ) {

    statw.Reset ();

    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WFromCentroid | WDistance ), &statw, 0, 0, 0 );

//  sumd ( nc )     = statw.Mean ();
    sumd ( nc )     = statw.Median ();  // more robust
    }


TEasyStats          statpairs;
TEasyStats          statdb;

for ( int nc = 0; nc < nclusters; nc++ ) {

    double      sumd1   = sumd ( nc );

                                        // we need the max of pairs between clusters
    statpairs.Reset ();

    for ( int nc2 = 0; nc2 < nclusters; nc2++ ) {

        if ( nc2 == nc )    continue;

        double      sumd2   = sumd ( nc2 );

        double      d       = CorrelationToDifference ( Project ( maps[ nc ], maps[ nc2 ], polarity ) );

        statpairs.Add ( ( sumd1 + sumd2 ) / NonNull ( d ), ThreadSafetyIgnore );
        } // for nc2

                                        // cumulate maxes
    if ( statpairs.IsNotEmpty () )
        statdb.Add ( statpairs.Max (), ThreadSafetyIgnore );
    } // for nc

                                        // invert sign to have a "max criterion"
double              db              = - statdb.Mean ();
                                        
return  db;
}


//----------------------------------------------------------------------------
                                        // Cubic Clustering Criterion (CCC)
                                        // Looking at the difference between the variance and the estimate of the variance, assuming data are clustered in hyperboxes
                                        // Data are supposed to have 0 mean, and be independent across tracks (like after PCA).
                                        // Better with more data.
                                        //
                                        // Interpretation:
                                        //   - Pick the max, preferably > 2 (good confidence) or > 0 (a bit dubtious)
                                        //   - Negative an decreasing above 2 clusters, unimodal or long-tailed distribution
                                        //   - Very negative (-30) mean there are outliers
                                        //   - Ever growing, distribution is grainy or rounded data
                                        //   - Different peaks mean hierarchical structure
                                        // Caveats:
                                        //    - the average of samples per cluster should be at least 10, otherwise the CCC does not behave correctly
                                        //    - not suited for elongated (correlated) or irregularly shaped clusters
double  TMicroStates::ComputeCubicClusteringCriterion   (   int             nclusters,
                                                            TMaps&        /*maps*/,     TLabeling&      labels,
                                                            PolarityType  /*polarity*/ 
                                                        )
{
TMap                mapt ( NumRows );
//TArray1<int>        clustercount ( nclusters );
TArray2<double>     s ( NumRows, 3 );
double              v               = 0;
int                 nummaps         = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // get the max norm of data
TEasyStats          statnorm;

for ( long tf = 0; tf < NumTimeFrames; tf++ )
    if ( labels.IsDefined ( tf ) )
        statnorm.Add ( Norm[ tf ], ThreadSafetyIgnore );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute size of hypercube
for ( int dimi = 0; dimi < NumRows; dimi++ ) {
    s ( dimi, 0 ) = Highest<double> ();
    s ( dimi, 1 ) = Lowest <double> ();
    }


for ( long tf = 0; tf < NumTimeFrames; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;


    mapt    = Data[ tf ] * (double) labels.GetSign ( tf );
                                        // real data version - !skip this for template version only!
//  mapt   *= Norm[ tf ] / statnorm.Max ();     // normalize the norm to avoid Power overflows - not sure it really gives better results

//  clustercount ( labels[ tf ] )++;

    nummaps++;

    for ( int dimi = 0; dimi < NumRows; dimi++ ) {
                                        // store min & max along each axis
        Mined ( s ( dimi, 0 ), (double) mapt[ dimi ] );
        Maxed ( s ( dimi, 1 ), (double) mapt[ dimi ] );
        }
    }

                                        // now we have each axis size / range
for ( int dimi = 0; dimi < NumRows; dimi++ )

    s ( dimi, 2 ) =  s ( dimi, 1 ) - s ( dimi, 0 );


                                        // Use descending sizes from now
s.SortRows ( 2, Descending );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Assess each cluster is populated enough
TEasyStats          statcl ( nclusters );


for ( int nc = 0; nc < nclusters; nc++ ) {

    statcl.Add ( clustercount ( nc ), ThreadSafetyIgnore );

//    if ( clustercount ( nc ) < 10 )
//        DBGV3 ( nclusters, nc + 1, clustercount ( nc ), "nclusters, nc + 1, clustercount ( nc )" );
    }

//DBGV3 ( nclusters, statcl.Mean (), statcl.Median (), "nclusters  Mean / MEdian clustercount" );

if ( statcl.Median () < 10 ) {
    DBGV2 ( nclusters, statcl.Median (), "Not enough population: nclusters  Avg clustercount" );
    return  0;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute volume of the hypercube around data
for ( int dimi = 0; dimi < NumRows; dimi++ )
                                        // instead of product, use sum of log
    if ( s ( dimi, 2 ) != 0 )
        v      += log ( AtLeast ( (double) MinLogValue, s ( dimi, 2 ) ) );

                                        // here we have our volume
v       = exp ( v );

                                        // trial with radius?
//double              r                = 0;
//for ( int dimi = 0; dimi < NumRows; dimi++ )
//    r      += Square ( s ( dimi, 2 ) );
//r   = sqrt ( r );
//r      /= nclusters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Average size of each cluster of data hypercubes [dim]
double              c               = Power ( v / nclusters, 1.0 / NumRows );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Average number of hypercubes in each axis
TArray1<double>     U ( NumRows );
double              sumU2           = 0;


for ( int dimi = 0; dimi < NumRows; dimi++ ) {
                                        // ratio of range to average hypercube size [ratio]: > 1 means this dimension bigger than average hypercube
    U ( dimi )  = s ( dimi, 2 ) / NonNull ( c );

    sumU2      += Square ( U ( dimi ) );

//    DBGV5 ( nclusters, dimi, c, s ( dimi, 2 ), U ( dimi ), "nclusters  dimi  c  size  U" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Real R2
double              R2;

R2      = 1 - NumRows / NonNull ( sumU2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Find the inter-class dimension
int                 nstar;

                                        // pick the highest index with U < 1 (U in decreasing order), but strictly below nclusters
for ( nstar = 0; nstar < nclusters - 1; nstar++ )
    if ( U ( nstar ) < 1 )
        break;

                                        // below nclusters & index starting from 0
nstar   = AtMost  ( nclusters - 2,  nstar );
nstar   = AtLeast ( 0,              nstar );

                                        // force a minimum of dimensions in? - results don't look very consistent
//if ( nstar == 0 && nclusters > 1 ) {
//    nstar   = nclusters / 2;
//    DBGV3 ( nclusters, nstar, U ( nstar ), "nclusters, nstar, U (nstar)" );
//    }

//DBGV3 ( nclusters, nstar, U ( nstar ), "nclusters, nstar, U (nstar)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute E(R2)
double              sumnu           = 0;
double              sumnu2          = 0;
double              sumu2           = 0;
double              ER2;
double              ER2R2;
double              scale;
double              CCC;


for ( int dimi = 0; dimi <= nstar; dimi++ )
    sumnu  += 1 / ( nummaps + U ( dimi ) );

for ( int dimi = nstar + 1; dimi < NumRows; dimi++ )
    sumnu2 += Square ( U ( dimi ) ) / ( nummaps + U ( dimi ) );

for ( int dimi = 0; dimi < NumRows; dimi++ )
    sumu2  += Square ( U ( dimi ) );

                                        // Estimate of R2
ER2     = AtLeast ( 0.0, 1 - ( ( ( sumnu + sumnu2 ) / NonNull ( sumu2 ) ) * ( Square ( (double) ( nummaps - nclusters ) ) / nummaps ) * ( 1 + 4.0 / nummaps ) ) );
                                        // before the log
ER2R2   = AtLeast ( 0.0, ( 1 - ER2 ) / NonNull ( 1 - R2 ) );
                                        // kind of heuristic normalization formula - also prevent scaling to be null
scale   = sqrt ( nummaps * NonNull ( nstar ) / 2 ) / Power ( 0.001 + ER2, 1.2 );
                                        // finally we get our shit
CCC     = ER2R2 > MinLogValue ? log ( ER2R2 ) * scale : -1;


//DBGV6 ( nclusters, nstar, R2, ER2, ER2R2, CCC, "nclusters, nstar, R2, ER2, ER2R2, CCC" );

return  CCC;
}


//----------------------------------------------------------------------------
                                        // Look at the maximum difference of W weighted by nclusters^2
                                        // Use the Det(W) instead of Tr(W) to give less weight to higly correlated variables
                                        // and works best with elliptical clusters (high correlation within groups)
/*
double  TMicroStates::ComputeMarriott   (   int     nclusters )
{
double              sumlogdet       = StatWPooledDetSquareDistance.Sum ();

double              Marriott        = sumlogdet + 2 * log ( (double) AtLeast ( 1, nclusters ) );    // multiply by (nclusters^2)

return  Marriott;
}
*/

//----------------------------------------------------------------------------
/*                                      // Point Biserial Correlation measures how the distances across all points match the clustering
                                        // Faster implementation, but sometimes gives exactly the same results as the squared version below, and sometimes quite different
double  TMicroStates::ComputePointBiserial    (   int    nclusters,   TLabeling&      labels )
{
TEasyStats          statwpooleddistance ( StatWPooledDistance );
TEasyStats          statbpooleddistance ( StatBPooledDistance );
TEasyStats          statapooleddistance ( StatAPooledDistance );


double              avgSw           = statwpooleddistance.Mean ();

double              avgSb           = statbpooleddistance.Mean ();

double              sda             = statapooleddistance.SD   ();

int                 numpairsw       = statwpooleddistance.GetNumItems ();
int                 numpairsb       = statbpooleddistance.GetNumItems ();
int                 numpairsa       = statapooleddistance.GetNumItems ();


double              PB              = ( avgSb - avgSw ) * sqrt ( numpairsw * numpairsb ) / NonNull ( numpairsa * sda );


return  PB;
}
*/

                                        // Do a correlation of the ditance matrix with the binarized version of it, with 1 if points are not in the same clusters
                                        // Here the computation is done on the lower triangular + diagonal parts only, nearly the same as the full squared matrix of distances
double  TMicroStates::ComputePointBiserial  (   int    nclusters,   TLabeling&      labels )
{
                                        // !Don't sort nor Median StatAPooledDistance to keep it coherent with StatAPooledDistanceIndex!

                                        // total number of valid data, which is also the size of the diagonal distance matrix
//int               nummaps         = NumTimeFrames;                                                        // all data, without downsampling nor Correlation Threshold
//int               nummaps         = RoundAbove ( NumTimeFrames / (double) CriterionDownSampling );        // downsampled number of points / indexes - !rounding above!
//int               nummaps         = GetSizeOfClusters ( 0, nclusters - 1, labels, CriterionDownSampling );// count of attributed maps - could be less in case of Correlation Thresholding f.ex.
int                 nummaps         = StatAPooledDistance.PairsToNumItems ();                               // same value / behavior as above

//DBGV5 ( nclusters, NumTimeFrames, RoundAbove ( NumTimeFrames / (double) CriterionDownSampling ), labels.GetSizeOfClusters ( 0, nclusters - 1, CriterionDownSampling ), StatAPooledDistance.PairsToNumItems (), "ComputePointBiserial:  nclusters, NumTimeFrames, RoundAbove ( NumTimeFrames / CriterionDownSampling )  GetSizeOfClusters, StatAPooledDistance.PairsToNumItems" );


int                 numlowtriang    = StatAPooledDistance.GetNumItems ();               // triangular matrix size without the diagonal (and without the undefined labels) - quite the same as the squared matrix

int                 vectorsize      = numlowtriang + nummaps;                           // triangular matrix size with diagonal

                                        // create 2 big vectors
TMap                W  ( vectorsize );  // real distances
TMap                Wb ( vectorsize );  // binarized version of the classification: 1 for between cluster (further distance), 0 for within cluster (close distance)

                                        // same labels -> false
auto                AreDifferentLabels  = [] ( const auto& L1, const auto& L2 )     { return  L1 == L2 ? 0 : 1; };


                                        // transfer lower diagonal part, where pairs are always of non-identical maps
for ( int vi = 0, ii = 0; vi < numlowtriang; vi++, ii += 2 ) {
                                        // !Note: insert this twice to have the real squared matrix W (the upper triangular part)!
    W [ vi ]    =                              StatAPooledDistance     [ vi     ];      // distance between pair
    Wb[ vi ]    = AreDifferentLabels ( labels[ StatAPooledDistanceIndex[ ii     ] ],
                                       labels[ StatAPooledDistanceIndex[ ii + 1 ] ] );  // TF indexes are stored sequentially
    }

                                        // transfer diagonal part, where pairs are always of identical maps
for ( int vi = numlowtriang, mapi = 0; vi < vectorsize; vi++, mapi++ ) {
    W [ vi ]    = 0;                                    // distance is always 0
    Wb[ vi ]    = AreDifferentLabels ( mapi, mapi );    // indexes are always the same
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do a Pearson correlation among the 2 types of data
double              PointBiserial   = W.Correlation ( Wb, true );

return  PointBiserial;
}


//----------------------------------------------------------------------------
double  TMicroStates::ComputeSilhouettes    (   int     nclusters,  TLabeling&  labels )
{
                                        // !Don't sort nor Median StatAPooledDistance to keep it coherent with StatAPooledDistanceIndex!

                                        // Get total number of valid data
//int               nummaps         = NumTimeFrames;                                                        // all data, without downsampling nor Correlation Threshold
int                 nummaps         = RoundAbove ( NumTimeFrames / (double) CriterionDownSampling );        // downsampled number of points / indexes - !rounding above!
//int               nummaps         = GetSizeOfClusters ( 0, nclusters - 1, labels, CriterionDownSampling ); // count of attributed maps - could be less in case of Correlation Thresholding f.ex.
//int               nummaps         = StatAPooledDistance.PairsToNumItems ();                               // same value / behavior as above

//DBGV5 ( nclusters, NumTimeFrames, RoundAbove ( NumTimeFrames / (double) CriterionDownSampling ), GetSizeOfClusters ( 0, nclusters - 1, labels, CriterionDownSampling ), StatAPooledDistance.PairsToNumItems (), "ComputeSilhouettes:  nclusters, NumTimeFrames, RoundAbove ( NumTimeFrames / CriterionDownSampling )  GetSizeOfClusters, StatAPooledDistance.PairsToNumItems" );


int                 numlowtriang    = StatAPooledDistance.GetNumItems ();                             // triangular matrix size without the diagonal (and without the undefined labels) - quite the same as the squared matrix

                                        // within and between clusters distances
TArray2<double>     Dw ( nummaps,                2 );   // !nummaps can be greater than the actual number of maps!
TArray3<double>     Db ( nummaps, nclusters + 1, 2 );   // 1 more for min distance
int                 mapi1;
int                 mapi2;
LabelType           label1;
LabelType           label2;
double              d;

                                        // transfer lower diagonal part, where pairs are always of non-identical maps
for ( int vi = 0, ii = 0; vi < numlowtriang; vi++, ii += 2 ) {

    mapi1       = StatAPooledDistanceIndex[ ii     ];
    mapi2       = StatAPooledDistanceIndex[ ii + 1 ];
    label1      = labels                  [ mapi1  ];
    label2      = labels                  [ mapi2  ];
    d           = StatAPooledDistance     [ vi     ];   // distance between pair

                                        // convert to downsampled indexes
    mapi1      /= CriterionDownSampling;
    mapi2      /= CriterionDownSampling;

//    if ( mapi1 >= nummaps || mapi2 >= nummaps ) { DBGV4 ( nclusters, nummaps, mapi1, mapi2, "Sil Error:  nclusters, nummaps, mapi1, mapi2" ); continue; }


    if ( label1 == label2 ) {
        Dw ( mapi1,            0 ) += d;    // pairs of maps from the same cluster
        Dw ( mapi1,            1 ) ++;
        Dw ( mapi2,            0 ) += d;
        Dw ( mapi2,            1 ) ++;
        }
    else {
        Db ( mapi1, label2,    0 ) += d;    // pairs of maps from different clusters, update map1 toward cluster2
        Db ( mapi1, label2,    1 ) ++;
        Db ( mapi1, nclusters, 1 ) ++;      // + global count
        Db ( mapi2, label1,    0 ) += d;    // pairs of maps from different clusters, update map2 toward cluster1
        Db ( mapi2, label1,    1 ) ++;
        Db ( mapi2, nclusters, 1 ) ++;      // + global count
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Average all distances

for ( int mapi = 0, mapio = 0; mapi < nummaps; mapi++, mapio += CriterionDownSampling ) {
                                        // actually all TFs
    if ( labels.IsUndefined ( mapio ) )
        continue;

                                        // empty slot
    if ( Dw ( mapi, 1 ) == 0 && Db ( mapi, nclusters, 1 ) == 0 )
        continue;

                                        // average distance within cluster for each map
    Dw ( mapi, 0 )     /= NonNull ( Dw ( mapi, 1 ) );

                                        // initialize min average distances
    Db ( mapi, nclusters, 0 )   = Highest<double> ();

                                        // average distance between clusters for each map
    for ( int nc = 0; nc < nclusters; nc++ ) {

        Db ( mapi, nc, 0 ) /= NonNull ( Db ( mapi, nc, 1 ) );
                                        // get min across clusters (!null distance for within cluster!)
        if ( Db ( mapi, nc, 0 ) != 0 )
            Mined ( Db ( mapi, nclusters, 0 ), Db ( mapi, nc, 0 ) );
        }

                                        // by safety
    if ( Db ( mapi, nclusters, 0 ) == Highest<double> () )
        Db ( mapi, nclusters, 0 )   = 1e300;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute silhouette for each map
TArray1<double>     S ( nummaps );
double              a;
double              b;
TEasyStats          stats;              // silhouettes stats
//TGoEasyStats      statsk ( nclusters + 1 );


for ( int mapi = 0, mapio = 0; mapi < nummaps; mapi++, mapio += CriterionDownSampling ) {
                                        // actually all TFs
    if ( labels.IsUndefined ( mapio ) )
        continue;

                                        // empty slot
    if ( Dw ( mapi, 1 ) == 0 && Db ( mapi, nclusters, 1 ) == 0 )
        continue;


    a           = Dw ( mapi,            0 );
    b           = Db ( mapi, nclusters, 0 );
    S ( mapi )  = ( b - a ) / NonNull ( max ( a, b ) );
                                        // add to global stats
    stats                     .Add ( S ( mapi ), ThreadSafetyIgnore );
                                        // add to per cluster stats
//  statsk ( labels[ mapio ] ).Add ( S ( mapi ), ThreadSafetyIgnore );
    }

//stats.Show ( "silhouettes" );

                                        // merge all per clusters stats into a final stat
//for ( int nc = 0; nc < nclusters; nc++ )
//
//    statsk ( nclusters ).Add ( statsk ( nc ).SD (), ThreadSafetyIgnore ); // 2)
////  statsk ( nclusters ).Add ( statsk ( nc ).Mean () * Clip ( 1 - statsk ( nc ).SD (), (double) 0, (double) 1 ), ThreadSafetyIgnore ); // 3)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // NOT using robust, we want the criterion to actually degrade with lower values!
double              avgsilh         = stats.Mean ();

                                        // using across clusters stats
//double            avgrangesd      = stats.Mean () * Clip ( 1 - statsk ( nclusters ).Range (), (double) 0, (double) 1 ); // of SD(k)  2)
//double            avgerrorsilh    = statsk ( nclusters ).Mean (); // 3) of (Mean*1-SD)(k)


return  avgsilh;
}


//----------------------------------------------------------------------------
                                        // Trace of W
                                        // Looks very similar to KL (after second derivative) and not that useful...
double  TMicroStates::ComputeTraceW     (   int               /*nclusters*/ )
{
                                        // official formula
//double            trW             = StatWCentroidSquareDistance.Sum ();
                                        // slightly better with distance?
double              trW             = StatWCentroidDistance.Sum ();


return  trW;

                                        // Ball index, then max of first derivative
//return  trW / nclusters;
}


//----------------------------------------------------------------------------
                                        // Baker and Hubert Gamma index
                                        // Compute a ratio of concordant quadruples of points (within distance less than between distance)
                                        // and discordant quadruples (within distance greater than between distance)
                                        // This implementation is much more efficient, and makes use of all available data
double  TMicroStates::ComputeGamma  (   int                 /*nclusters*/,
                                        double&             GPlusCriterion,     double&             Tau 
                                    )
{
                                        // Count set of pairs according to distances
int                 numpairsw       = StatWPooledDistance.GetNumItems ();
int                 numpairsb       = StatBPooledDistance.GetNumItems ();
int                 numpairsa       = numpairsw + numpairsb;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Use histograms to count the number of pairs below or above a given value                                        
double                  minwb               = 0;    // min distance between 2 normalized maps: 0
double                  maxwb               = 2.0;  // max distance between 2 normalized maps: 2 with polarities, sqrt(2) for ignore polarity
double                  histosize           = 200;  // 0 to get an optimal array size, but it could get really big sometimes, so override to something realistic and fast

                                        // histograms, not normalized
THistogram              histow  (   StatWPooledDistance, 
                                    0, minwb, maxwb, 
                                    0, 5, 
                                    (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramCount | HistogramLinear ), 
                                    &histosize
                                );
THistogram              histob  (   StatBPooledDistance, 
                                    0, minwb, maxwb, 
                                    0, 5, 
                                    (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramCount | HistogramLinear ), 
                                    &histosize
                                );


THistogram              cdfb   ( histob );
                                        // cdf, not normalized
cdfb.ToCDF ( HistogramCount );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              splus           = 0;    // count within clusters pairs closer  than between clusters pairs
double              sminus          = 0;    // count within clusters pairs further than between clusters pairs


for ( int wbi = 0; wbi < histosize; wbi++ ) {
                                        // S+ (number of Within pairs and Between pairs, with Within pairs at lower distances than Between pairs)
                                        // for each pair value w, the number of pairs in B higher than w is exactly the ( 1 - CDFB[w] ) * #B
                                        // "repeating" the sum for each w is simply done by multiplying by HistogramW[w]
    splus      += histow ( wbi ) * ( cdfb ( histosize - 1 ) - cdfb ( wbi ) );

                                        // S- (number of Within pairs and Between pairs, with Within pairs at greater distances than Between pairs)
                                        // same idea, the number of B pairs lower than w is CDFB[w] * #B
    sminus     += histow ( wbi ) *                            cdfb ( wbi );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Interesting to actually see the histograms...
//curve_w.NormalizeArea ();
//curve_b.NormalizeArea ();


char                    buff[ 256 ];
char                    base[ 256 ];

StringRandom ( base, 8 );


TExportTracks           expdur;
StringCopy      ( expdur.Filename,          "E:\\Data\\Gamma.", base, "." );
StringAppend    ( expdur.Filename,          IntegerToString ( buff, nclusters, 2 ) );
StringAppend    ( expdur.Filename,          ".W.Histo" );
AddExtension    ( expdur.Filename,          FILEEXT_EEGSEF );

expdur.SetAtomType ( AtomTypeScalar );
expdur.NumTime              = histow.GetDim1 ();
expdur.NumTracks            = 1;
expdur.SamplingFrequency    = histow.Index1.IndexRatio;

expdur.Write ( histow );
expdur.End ();


StringReplace   ( expdur.Filename, ".W.", ".B." );
expdur.NumTime              = histob.GetDim1 ();
expdur.SamplingFrequency    = histob.Index1.IndexRatio;
expdur.Write ( histob );
expdur.End ();
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // look for a minimum
double              nn1             = numpairsa * ( numpairsa - 1 );

//                  GPlusCriterion  = 2 * sminus / NonNull ( nn1 );
                                        // invert formula to get a max - also sminus could totally be 0
                    GPlusCriterion  = nn1 / NonNull ( 2 * sminus );

//                  GMinusCriterion = ( nn1 - 2 * splus ) / NonNull ( nn1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Kendall / Tau-a denominator
//double            tauden          = NumTimeFrames * ( NumTimeFrames - 1 ) / 2.0;
                                        // Tau-b
//double            tauden          = sqrt ( ( numpairsa - numpairsw )
//                                         * ( numpairsa - numpairsb ) );
                                        // Rohlf
double              tauden          = sqrt ( ( nn1 / 2.0 - numpairsa )
                                           * ( nn1 / 2.0             ) );

                    Tau             = ( splus - sminus ) / NonNull ( tauden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              gamma           = ( splus - sminus ) / (double) NonNull ( splus + sminus );


return  gamma;
}


//----------------------------------------------------------------------------
                                        // Frey Index, looks at the ratio between Between and Within cluster distances, ideally being 1
                                        // !It needs the Mean W and B through all clusters!
void    TMicroStates::ComputeFreyVanGroenewoud  (   int               minclusters,  int             maxclusters,
                                                    TArray2<double>&  var 
                                                )
{

for ( int ncl = 0; ncl <= maxclusters; ncl++ )
    var ( segFreyVanGroenewoud, ncl )   = 0;


double              dw;
double              db;
//double              angle;
double              frvgr;


for ( int ncl = minclusters /*AtLeast ( 2, minclusters )*/; ncl <= maxclusters; ncl++ ) {

                                        // could also try segWCD or segWCD2

                                        // other formula: delta from previous to next
//  dw      = var ( segWPD, ncl + 1 ) - var ( segWPD, ncl - 1 );
//  db      = var ( segBPD, ncl + 1 ) - var ( segBPD, ncl - 1 );
                                        // official formula: delta from current to next
    dw      = var ( segWPD, ncl + 1 ) - var ( segWPD, ncl );
    db      = var ( segBPD, ncl + 1 ) - var ( segBPD, ncl );

                                        // db / dw has a singularity if dw == 0, which can happen on our data
                                        // so instead of the slope being 1, look at the angle being Pi/4

                                        // Criterion tries to minimize db / dw, so invert it for maximization
    frvgr   = dw / NonNull ( db );
                                        // officially, if db / dw is below 1, there is no solution
    if ( frvgr > 1 ) frvgr = 0;


//                                      // abs version where we don't care for signs -> more peaks
//  angle   = ArcTangent ( fabs ( db ), fabs ( dw ) );
//
//                                      // compute how far we are from Pi/4
//  frvgr   = 1 - fabs ( angle / M_PI_4 - 1 );

                                        // ?
//  frvgr   = 1 - fabs ( db - dw );


    var ( segFreyVanGroenewoud, ncl )   = frvgr;
    }

                                        // void 2 first values
var ( segFreyVanGroenewoud, 1 ) = 0;
//var ( segFreyVanGroenewoud, 2 ) = 0;
}


//----------------------------------------------------------------------------
                                        // Hartigan Index
                                        // !It needs the Mean W through all clusters!
void    TMicroStates::ComputeHartigan   (   int               minclusters,  int             maxclusters,
                                            TArray2<double>&  var 
                                        )
{

for ( int ncl = 0; ncl <= maxclusters; ncl++ )
    var ( segHartigan, ncl )  = 0;


double              hart;

                                        // official formula segWCD2, but also OK with segWPD

for ( int ncl = minclusters /*AtLeast ( 2, minclusters )*/; ncl <= maxclusters; ncl++ ) {

                                        // Official formula
                                        // the weighting does not seem to matter, plus we prefer to give a little less emphasis on the first clusters
//  hart    = ( var ( segWCD2, ncl ) / NonNull ( var ( segWCD2, ncl + 1 ) ) - 1 ) * ( NumTimeFrames - ncl - 1 );
//  hart    = ( var ( segWCD2, ncl ) / NonNull ( var ( segWCD2, ncl + 1 ) ) - 1 );

                                        // with a real derivative approach the results are smoother, and the local max seems better localized
//  hart    = ( var ( segWCD2, ncl - 1 ) / NonNull ( var ( segWCD2, ncl + 1 ) ) - 1 );

                                        // another formula: relative improvement + correction
    hart    = ( var ( segWCD2, ncl ) - var ( segWCD2, ncl + 1 ) ) / NonNull ( var ( segWCD2, ncl + 1 ) ) * ( NumTimeFrames - ncl + 1 );


    var ( segHartigan, ncl )    = hart;
    }

                                        // void 2 first values
var ( segHartigan, 1 )  = 0;
//var ( segHartigan, 2 )  = 0;
//var ( segHartigan, 3 )  = 0;          // also this one seems lousy?
}


//----------------------------------------------------------------------------
double  TMicroStates::ComputeRatkowski  (   int             nclusters,
                                            TMaps&          maps,       TLabeling&  labels,
                                            PolarityType    polarity
                                        )
{
                                        // If robust statistics are used
long                maxel           = 1000;
int                 stepel          = AtLeast ( 1, Round ( NumRows       / ( (double) maxel / 1.5 ) ) );
int                 numel           = ( NumRows + stepel - 1 ) / stepel;

long                maxtf           = 3000;
int                 steptf          = AtLeast ( 1, Round ( NumTimeFrames / ( (double) maxtf / 1.5 ) ) );
int                 numtf           = ( NumTimeFrames + steptf - 1 ) / steptf;


TMap                mapt ( NumRows );
//TGoEasyStats      wss  ( NumRows, NumTimeFrames );
//TGoEasyStats      bss  ( NumRows, NumTimeFrames );
TGoEasyStats        wss  ( numel, numtf );
TGoEasyStats        bss  ( numel, numtf * AtLeast ( 1, nclusters - 1 ) );   // between has more data
PolarityType        pol;


                                        // Do a classical search among within and between groups, then store stats per dimension
for ( long tf = 0; tf < NumTimeFrames; tf+=steptf ) {

    if ( labels.IsUndefined ( tf ) )
        continue;

    mapt    = Data[ tf ];


    for ( int nc = 0; nc < nclusters; nc++ ) {
                                        // we need to evaluate for each template
        pol     = polarity == PolarityEvaluate ? maps[ nc ].IsOppositeDirection ( mapt ) ? PolarityInvert : PolarityDirect
                                               : polarity;

        if ( nc == labels[ tf ] )       // within
            for ( int dimi = 0; dimi < NumRows; dimi+=stepel )
                wss ( dimi / stepel ).Add ( Square ( maps[ nc ][ dimi ] - ( (bool) pol ? -mapt[ dimi ] : mapt[ dimi ] ) ) );

        else                            // between
            for ( int dimi = 0; dimi < NumRows; dimi+=stepel )
                bss ( dimi / stepel ).Add ( Square ( maps[ nc ][ dimi ] - ( (bool) pol ? -mapt[ dimi ] : mapt[ dimi ] ) ) );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merge all dimensions
TEasyStats          statdims ( numel /*NumRows*/ );


for ( int dimi = 0; dimi < numel /*NumRows*/; dimi++ )
                                        // try a ratio of Between to Within
//  statdims.Add ( sqrt ( bss ( dimi ).Mean   () / NonNull ( wss ( dimi ).Mean   () ) ), ThreadSafetyIgnore );    // ratio of means
    statdims.Add ( sqrt ( bss ( dimi ).Median () / NonNull ( wss ( dimi ).Median () ) ), ThreadSafetyIgnore );    // ratio of medians
//  statdims.Add ( sqrt ( bss ( dimi ).Mean () ) - sqrt ( wss ( dimi ).Mean () ),        ThreadSafetyIgnore );           // difference of means


double              Rat            = statdims.Median ();              // Good, better than Mean
//double            Rat            = statdims.MaxModeHistogram ();
//double            Rat            = statdims.ConsistentMedian ( 1 );   // Also good, more spiky


return  Rat;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
