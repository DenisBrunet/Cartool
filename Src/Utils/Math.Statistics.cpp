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

#include    "Math.Statistics.h"
#include    "TStatisticsDialog.h"       // enums  OutputPType, CorrectionType, StatTimeType, PairedType

#include    "TArray2.h"
#include    "TArray3.h"
#include    "TMaps.h"
#include    "Math.Random.h"
#include    "Dialogs.TSuperGauge.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // 2-tails Student t-value to p-value conversion
                                        // Average error is about 1e-15, Max error 1e-13
long double     Student_TwoTails_t_to_p ( long double t, int dof )
{
if ( dof <= 0 )                         // shouldn't happen, should return an error code instead!
    return  1;

long double         theta           = atan2 ( abs ( t ), sqrt ( (long double) dof ) );

if ( dof == 1 )
    return  1.0 - theta / HalfPi;


auto    Sum     = [] ( long double q, int i, int j ) -> long double
{
if ( j < i )    return 1;

long double         prodn           = 1;
long double         prodd           = 1;
long double         sum             = 1;

for ( int k = i; k <= j; k += 2 ) {

    prodn  *= q * k;
    prodd  *= k + 1;
    sum    += prodn / prodd;
    }

return  sum;
};

                                        // here dof > 1
long double         sinetheta       = sin ( theta );
long double         cosinetheta     = cos ( theta );
long double         p;


if ( IsOdd ( dof ) )    p   = 1.0 - ( theta + sinetheta * cosinetheta * Sum ( Square ( cosinetheta ), 2, dof - 3 ) ) / HalfPi;
else                    p   = 1.0 -           sinetheta               * Sum ( Square ( cosinetheta ), 1, dof - 3 );


return  Clip ( p, (long double) 0, (long double) 1 );
}


//----------------------------------------------------------------------------
                                        // Converts a p value to an appropriate output format
double      Format_p_value      (   double          p,
                                    OutputPType     how,                          
                                    bool            clippingpvalue,     double      pvaluemin 
                                )
{
switch ( how ) {

    case    OutputP:            return    clippingpvalue && p > pvaluemin           ?   1.0 :           p;      // !output 1 to signify not significative!
    case    Output1MinusP:      return    clippingpvalue && p > pvaluemin           ?   0.0 :     1.0 - p;
    case    OutputMinusLogP:    return    clippingpvalue && p > pvaluemin || p <= 0 ?   0.0 : - Log10 ( p );
    default:                    return                                                  0.0;
    }

}


//----------------------------------------------------------------------------
                                        // Multiple tests p-values correction
void        Correct_p_values    (   TArray3<float>&     Results,            int                 numvars,        int                 maxnumtf,
                                    CorrectionType      how,
                                    int                 bonferronicorrectionvalue,
                                    double              fdrcorrectionvalue
                                )
{
if ( how == CorrectionNone )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( how == CorrectionBonferroni ) {

    double              p;


    for ( int v   = 0; v   < numvars;  v++   )
    for ( int tf0 = 0; tf0 < maxnumtf; tf0++ ) {

        p       = Results ( tf0, v, Test_p_value );
                                        // Bonferroni correction is eaysy, though a well-known over-kill
        Results ( tf0, v, Test_p_value )    = Clip ( p * bonferronicorrectionvalue, 0.0, 1.0 );
        }

    } // CorrectionBonferroni


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // FDR can be done with 3 variants
else if ( IsCorrectionFDR ( how ) ) {

    enum                SortedPEnum
                        {
                        index,
                        pvalue,
                        linearvalue,
                        qvalue,
                        numsortedpenum,
                        };

    TArray2<float>      fdr ( numvars, numsortedpenum );


    for ( int tf0 = 0; tf0 < maxnumtf; tf0++ ) {
                                        // transfer all p-values
        for ( int v   = 0; v   < numvars;  v++   ) {

            fdr ( v, index  ) = v;
            fdr ( v, pvalue ) = Results ( tf0, v, Test_p_value );
            } // for numvars

                                        // sort by increasing p's
        fdr.SortRows ( pvalue, Ascending );

                                        // update list post sort
        for ( int v   = 0; v   < numvars;  v++   ) {
                                        // normalized linear index
            fdr ( v, linearvalue  )   = (double) ( v + 1 ) / numvars;
                                        // corrected value (q): p-value rescaled by normalized slope value
            fdr ( v, qvalue       )   = fdr ( v, pvalue ) / fdr ( v, linearvalue );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Option 1: Threshold p-values above cut
        if ( how == CorrectionFDRThresholdedP ) {

            int                 cuttingi;
                          // starting from 1 can allow a few more guys in, as first value can be above the first epsilon
            for ( cuttingi = 0; cuttingi < numvars; cuttingi++ )

                if ( fdr ( cuttingi, qvalue ) > fdrcorrectionvalue )
  
                    break;
  
              for ( ; cuttingi < numvars; cuttingi++ )
                                        // clipping all p-values past this index - results are still p-values, though
                  Results ( tf0, fdr ( cuttingi, index ), Test_p_value ) = 1.0;

            } // CorrectionFDRThresholdedP


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Option 2: Corrected values - but resulting q-values are not monotonic
        else if ( how == CorrectionFDRCorrectedP ) {

            for ( int v = 0, v2 = fdr ( v, index ); v < numvars;  v++, v2 = fdr ( NoMore ( numvars - 1, v ), index ) )

                Results ( tf0, v2, Test_p_value )   = Clip ( fdr ( v, qvalue ), (float) 0.0, (float) 1.0 );

            } // CorrectionFDRCorrectedP


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Option 3: Adjusted values from Corrected values - same as above but enforcing monotonicity of the q-values
        else if ( how == CorrectionFDRAdjustedP ) {
                                        // enforce monotonicity
            for ( int v  = 0;     v  < numvars;  v++  )
            for ( int v2 = v + 1; v2 < numvars;  v2++ )

                Mined ( fdr ( v, qvalue ), fdr ( v2, qvalue ) );


            for ( int v = 0, v2 = fdr ( v, index ); v < numvars;  v++, v2 = fdr ( NoMore ( numvars - 1, v ), index ) )

                Results ( tf0, v2, Test_p_value )   = Clip ( fdr ( v, qvalue ), (float) 0.0, (float) 1.0 );

            } // CorrectionFDRAdjustedP

        } // for tf0

    } // IsCorrectionFDR

}


//----------------------------------------------------------------------------
                                        // Check the minimum duration of segments of valid p-values
void        CheckDuration_p_values  (   TArray3<float>&     Results,            int                 numvars,        int                 maxnumtf,
                                        bool                clippingpvalue,     double              pvaluemin,      int                 pvalueduration
                                    )
{
if ( maxnumtf < pvalueduration )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 duration;
long                tfbegin;
double              p;


for ( int v = 0; v < numvars; v++ ) {

    duration    = 0;


    for ( int tf0 = 0; tf0 < maxnumtf; tf0++ ) {

        p       = Format_p_value ( Results ( tf0, v, Test_p_value ), Output1MinusP, clippingpvalue, pvaluemin );


        if ( p != 0 ) {             // valid TF

            if ( duration == 0 )    // starting a new segment
                tfbegin = tf0;

            duration++;
            }
        else {                      // non-valid TF

                                    // not enough duration?
            if ( duration != 0 && duration < pvalueduration )

                for ( ; tfbegin < tf0; tfbegin++ )  // clear last and too short segment
                    Results ( tfbegin, v, Test_p_value )    = 1;

            Results ( tf0, v, Test_p_value )     = 1;   // clear current tf
            duration                    = 0;
            }
        } // for tf

                                    // not enough duration, up to end of track?
    if ( duration != 0 && duration < pvalueduration )

        for ( ; tfbegin < maxnumtf; tfbegin++ ) // clear last and too short segment
            Results ( tfbegin, v, Test_p_value )    = 1;

    } // for numvars

}


//----------------------------------------------------------------------------
                                        // Where the optional time average value is being stored
int     GetTimeAverageIndex ( const TArray3<float>& data )  { return data.GetDim1 () - 1; }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy functions to scan data for missing values and return the actual number of available samples

                                        // 1 group case
int     GetRealNumberOfSamples  (   const TArray3<float>&   data,
                                    int                     tf,             int                 v,          int                 numsamples,
                                    double                  missingvalue, 
                                    TArray1<int>&           sampleindex 
                                )
{
int         numrealsamples      = 0;

for ( int s = 0; s < numsamples; s++ )

    if ( data ( tf, v, s ) != missingvalue ) 

        sampleindex[ numrealsamples++ ] = s;    // index of existing sample

return  numrealsamples;
}

                                        // 2 groups case
int     GetRealNumberOfSamples  (   const TArray3<float>&   data1,          const TArray3<float>&   data2,
                                    int                     tf1,            int                     tf2,        int                 v,          int                 numsamples,
                                    double                  missingvalue, 
                                    TArray1<int>&           sampleindex 
                                )
{
int         numrealsamples      = 0;

for ( int s = 0; s < numsamples; s++ )

    if ( data1 ( tf1, v, s ) != missingvalue
      && data2 ( tf2, v, s ) != missingvalue )

        sampleindex[ numrealsamples++ ] = s;    // index of pair of existing samples

return  numrealsamples;
}


//----------------------------------------------------------------------------
                                        // Computing mean, SD, #samples for a specific group
                                        // Used for both t-test and randomization
                                        // Updates the sample indexes and results
void    ComputeSingleGroupMeasures  (   const TArray3<float>&   data,               StatTimeType    stattime,
                                        int                     numtf,              int             numvars,        int             numsamples,
                                        bool                    checkmissingvalues, double          missingvalue,
                                        TArray3<float>&         results
                                    )
{
                                        // index to average value (actually an additional data point)
int                 avgtfi              = GetTimeAverageIndex ( data );


OmpParallelBegin

TArray1<int>        sampleindex ( checkmissingvalues ? numsamples : 0 );

OmpFor

for ( int tf0 = 0; tf0 < numtf; tf0++ ) {
                                        // which index to use according to average
    int                 tf              = IsTimeSequential ( stattime ) ? tf0 : avgtfi;
    int                 numrealsamples;

    for ( int v = 0; v < numvars; v++ ) {


        if ( checkmissingvalues ) {

            numrealsamples = GetRealNumberOfSamples (   data,
                                                        tf,             v,          numsamples,
                                                        missingvalue, 
                                                        sampleindex 
                                                    );
                                                                
            if ( numrealsamples == 0 ) {
                                        // no data available - skip actual computation and continue loop
                results ( tf0, v, TestNumSamples ) = 0;

                continue; // for v
                }
            } // if checkmissingvalues
        else
            numrealsamples  = numsamples;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        long double         sum         = 0;
        long double         sum2        = 0;

        for ( int s = 0; s < numrealsamples; s++ ) {

            long double     temp    = data ( tf, v, checkmissingvalues ? sampleindex[ s ] : s );

            sum    +=          temp;
            sum2   += Square ( temp );
            } // for file

                                        // compute these variables
        long double         avg         = sum  / numrealsamples;
        long double         avg2        = sum2 / numrealsamples;
        long double         var         = abs ( ( sum2 - Square ( sum ) / numrealsamples ) / NonNull ( numrealsamples - 1 ) );

                                        // and store
        results ( tf0, v, TestMean                 ) = avg;
        results ( tf0, v, TestMeanSquares          ) = avg2;
        results ( tf0, v, TestVariance             ) = var;
        results ( tf0, v, TestStandardDeviation    ) = sqrt ( var );
        results ( tf0, v, TestStandardError        ) = sqrt ( var / numrealsamples );
        results ( tf0, v, TestNumSamples           ) = numrealsamples;
        } // for variable

    } // for tf

OmpParallelEnd
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    Run_t_test  (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                        StatTimeType            stattime1,          StatTimeType            stattime2,
                        int                     numtf1,             int                     numtf2,             int                     jointnumtf,
                        int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                        bool                    checkmissingvalues, double                  missingvalue,
                        int                     numvars,            
                        PairedType              paired,
                        TArray3<float>&         results1,           TArray3<float>&         results2,           TArray3<float>&         jointresults
                    )
{
char                title[ 256 ];

StringCopy  ( title, PairedTypeString[ paired ], " " "t-test" );

TSuperGauge         Gauge ( title, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute individual stats for each group
ComputeSingleGroupMeasures  (   data1,              stattime1,
                                numtf1,             numvars,        paired == TestPaired ? jointnumsamples : numsamples1,
                                checkmissingvalues, missingvalue,
                                results1
                            );


ComputeSingleGroupMeasures  (   data2,              stattime2,
                                numtf2,             numvars,        paired == TestPaired ? jointnumsamples : numsamples2,
                                checkmissingvalues, missingvalue,
                                results2
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // index to average value (actually an additional data point)
int                 avgtfi              = GetTimeAverageIndex ( data1 );


if ( paired == TestPaired ) {

    int                 numtf           = jointnumtf;
    int                 numsamples      = jointnumsamples;

                                        // compute the average of difference
    OmpParallelBegin

    TArray1<int>        sampleindex ( checkmissingvalues ? numsamples : 0 );
                                        
    OmpFor

    for ( int tf0 = 0; tf0 < numtf; tf0++ ) {
                                        // which index to use according to average
        int             tf1             = IsTimeSequential ( stattime1 ) ? tf0 : avgtfi;
        int             tf2             = IsTimeSequential ( stattime2 ) ? tf0 : avgtfi;
        int             numrealsamples;


        for ( int v = 0; v < numvars; v++ ) {

            if ( Gauge.IsAlive () )
                Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( tf0 * numvars * StepThread () + v, numtf * numvars ) );


            if ( checkmissingvalues ) {

                numrealsamples  = GetRealNumberOfSamples    (   data1,          data2,
                                                                tf1,            tf2,            v,          numsamples,
                                                                missingvalue, 
                                                                sampleindex 
                                                            );

                if ( numrealsamples == 0 ) {
                                        // no data available - skip actual computation and continue loop
                    results1    ( tf0, v, TestNumSamples ) = 0;
                    results2    ( tf0, v, TestNumSamples ) = 0;
                    jointresults( tf0, v, Test_p_value )   = 1.0;

                    continue; // for v
                    }
                } // if checkmissingvalues
            else
                numrealsamples  = numsamples;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            long double         sum         = 0;
            long double         sum2        = 0;

            for ( int s = 0; s < numrealsamples; s++ ) {

                long double         temp    = data1 ( tf1, v, checkmissingvalues ? sampleindex[ s ] : s ) 
                                            - data2 ( tf2, v, checkmissingvalues ? sampleindex[ s ] : s );

                sum    +=          temp;
                sum2   += Square ( temp );
                } // for file

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute these variables
            int                 dof         = numrealsamples - 1;

            long double         avg         = sum  / numrealsamples;

            long double         avg2        = sum2 / numrealsamples;

            long double         var         = abs ( ( sum2 - Square ( sum ) / numrealsamples ) / NonNull ( numrealsamples - 1 ) );
                                        // paired ttest
            long double         sterr       = sqrt ( var / numrealsamples );

            long double         t           = avg / NonNull ( sterr );

            long double         p           = Student_TwoTails_t_to_p ( t, dof );

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store results
            jointresults ( tf0, v, TestNumSamples           )   = 2 * numrealsamples;
            jointresults ( tf0, v, TestDoF                  )   = dof;
            jointresults ( tf0, v, TestMean                 )   = avg;
            jointresults ( tf0, v, TestMeanSquares          )   = avg2;
            jointresults ( tf0, v, TestVariance             )   = var;
            jointresults ( tf0, v, TestStandardDeviation    )   = sqrt ( var );
            jointresults ( tf0, v, TestStandardError        )   = sterr;
            jointresults ( tf0, v, Test_t_value             )   = t;
            jointresults ( tf0, v, Test_p_value             )   = p;
            } // for variable

        } // for tf

    OmpParallelEnd
    } // if paired

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // unpaired ttest

    int                 numtf           = jointnumtf;


    OmpParallelFor

    for ( int tf0 = 0; tf0 < numtf; tf0++ ) {

        int             tf1             = IsTimeSequential ( stattime1 ) ? tf0 : 0;
        int             tf2             = IsTimeSequential ( stattime2 ) ? tf0 : 0;
        int             numrealsamples1;
        int             numrealsamples2;


        for ( int v = 0; v < numvars; v++ ) {

            if ( Gauge.IsAlive () )
                Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( tf0 * numvars * StepThread () + v, jointnumtf * numvars ) );


            if ( IsTimeSequential ( stattime1 ) || tf0 == 0 )   numrealsamples1 = results1 ( tf1, v, TestNumSamples );
            if ( IsTimeSequential ( stattime2 ) || tf0 == 0 )   numrealsamples2 = results2 ( tf2, v, TestNumSamples );


            if ( numrealsamples1 == 0 || numrealsamples2 == 0 ) {
                jointresults ( tf0, v, Test_p_value ) = 1.0;
                continue;
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute these variables
            int                 dof         = numrealsamples1 + numrealsamples2 - 2;

            long double         avg         = results1 ( tf1, v, TestMean )
                                            - results2 ( tf2, v, TestMean );
                                        // joint var and sd
            long double         var         = abs ( ( ( numrealsamples1 - 1 ) * abs ( results1 ( tf1, v, TestVariance ) )
                                                    + ( numrealsamples2 - 1 ) * abs ( results2 ( tf2, v, TestVariance ) ) ) / NonNull ( dof ) );
                                        // unpaired ttest
            long double         sterr       = sqrt ( abs ( results1 ( tf1, v, TestVariance ) / numrealsamples1
                                                         + results2 ( tf2, v, TestVariance ) / numrealsamples2 ) );
//          long double         sterr       = sqrt ( jointresults ( tf0, v, TestVariance ) / ( 1.0 / ( 1.0 / numrealsamples1 + 1.0 / numrealsamples2 ) ) ); // another formula(?)

            long double         t           = avg / NonNull ( sterr );

            long double         p           = Student_TwoTails_t_to_p ( t, dof );

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store results
            jointresults ( tf0, v, TestNumSamples           )   = numrealsamples1 + numrealsamples2;
            jointresults ( tf0, v, TestDoF                  )   = dof;
            jointresults ( tf0, v, TestMean                 )   = avg;
            jointresults ( tf0, v, TestVariance             )   = var;
            jointresults ( tf0, v, TestStandardDeviation    )   = sqrt ( var );
            jointresults ( tf0, v, TestStandardError        )   = sterr;
            jointresults ( tf0, v, Test_t_value             )   = t;
            jointresults ( tf0, v, Test_p_value             )   = p;
            } // for variable

        } // for tf

    } // else unpaired

} // Run_t_test


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    Run_Randomization_test  (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                                    StatTimeType            stattime1,          StatTimeType            stattime2,
                                    int                     numtf1,             int                     numtf2,             int                     jointnumtf,
                                    int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                                    bool                    checkmissingvalues, double                  missingvalue,
                                    int                     numvars,            
                                    PairedType              paired,
                                    int                     numrand,
                                    TArray3<float>&         results1,           TArray3<float>&         results2,           TArray3<float>&         jointresults
                                )
{
char                title[ 256 ];

StringCopy  ( title, PairedTypeString[ paired ], " " "Randomization" );

TSuperGauge         Gauge ( title, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute individual stats for each group
ComputeSingleGroupMeasures  (   data1,              stattime1,
                                numtf1,             numvars,        paired == TestPaired ? jointnumsamples : numsamples1,
                                checkmissingvalues, missingvalue,
                                results1
                            );


ComputeSingleGroupMeasures  (   data2,              stattime2,
                                numtf2,             numvars,        paired == TestPaired ? jointnumsamples : numsamples2,
                                checkmissingvalues, missingvalue,
                                results2
                            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // index to average value (actually an additional data point)
int                 avgtfi              = GetTimeAverageIndex ( data1 );


if ( paired == TestPaired ) {

    int                 numtf           = jointnumtf;
    int                 numsamples      = jointnumsamples;

                                        // compute the average of difference
    OmpParallelBegin
                                        // by safety, allocate a random generator for each thread
    TRandCoin           randcoin;
    TArray1<int>        sampleindex ( checkmissingvalues ? numsamples : 0 );

    OmpFor

    for ( int tf0 = 0; tf0 < numtf; tf0++ ) {
                                        // which index to use according to average
        int                 tf1             = IsTimeSequential ( stattime1 ) ? tf0 : avgtfi;
        int                 tf2             = IsTimeSequential ( stattime2 ) ? tf0 : avgtfi;
        int                 numrealsamples;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        for ( int v = 0; v < numvars; v++ ) {

            if ( Gauge.IsAlive () )
                Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( tf0 * numvars * StepThread () + v, numtf * numvars ) );


            if ( checkmissingvalues ) {

                numrealsamples  = GetRealNumberOfSamples    (   data1,          data2,
                                                                tf1,            tf2,            v,          numsamples,
                                                                missingvalue, 
                                                                sampleindex
                                                            );

                if ( numrealsamples == 0 ) {
                                        // no data available - skip actual computation and continue loop
                    results1    ( tf0, v, TestNumSamples ) = 0;
                    results2    ( tf0, v, TestNumSamples ) = 0;
                    jointresults( tf0, v, Test_p_value )   = 1.0;

                    continue; // for v
                    }
                } // if checkmissingvalues
            else
                numrealsamples  = numsamples;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the data sum of differences
            long double         sumdata         = 0;

            for ( int s = 0; s < numrealsamples; s++ )

                sumdata    += data1 ( tf1, v, checkmissingvalues ? sampleindex[ s ] : s ) 
                            - data2 ( tf2, v, checkmissingvalues ? sampleindex[ s ] : s );

                                        // !this average retains the sign!
            long double         avg             = sumdata / numrealsamples;

            sumdata     = abs ( sumdata );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the permutations
            int                 countabove      = 0;

            for ( int e = 0; e < numrand; e++ ) {

                long double         sum             = 0;

                for ( int s = 0; s < numrealsamples; s++ ) {

                    long double         temp    = data1 ( tf1, v, checkmissingvalues ? sampleindex[ s ] : s ) 
                                                - data2 ( tf2, v, checkmissingvalues ? sampleindex[ s ] : s );

                    if ( randcoin () )      sum -= temp;
                    else                    sum += temp;
                    }

                                        // 2-tailed, does not care for positive or negative
                if ( abs ( sum ) >= sumdata )
                    countabove++;
                } // for e


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute these variables
            long double         p           = Clip ( (long double) countabove / numrand, (long double) 0.0, (long double) 1.0 );

                                        // store results
            jointresults ( tf0, v, TestNumSamples   )   = 2 * numrealsamples;
            jointresults ( tf0, v, TestMean         )   = avg;
            jointresults ( tf0, v, Test_p_value     )   = p;
            } // for variable

        } // for tf

    OmpParallelEnd

    } // if paired

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // unpaired

    int                 numtf           = jointnumtf;

                                        // compute the difference of average
    OmpParallelBegin
                                        // by safety, allocate a random generator for each thread
    TRandUniform        randunif;
    TSelection          group1pick ( numsamples1 + numsamples2, OrderSorted );

    TArray1<int>        sampleindex1 ( checkmissingvalues ? numsamples1 : 0 );
    TArray1<int>        sampleindex2 ( checkmissingvalues ? numsamples2 : 0 );

    OmpFor

    for ( int tf0 = 0; tf0 < numtf; tf0++ ) {
                                        // which index to use according to average
        int                 tf1             = IsTimeSequential ( stattime1 ) ? tf0 : avgtfi;
        int                 tf2             = IsTimeSequential ( stattime2 ) ? tf0 : avgtfi;
        int                 numrealsamples1;
        int                 numrealsamples2;
        int                 numrealsamples;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        for ( int v = 0; v < numvars; v++ ) {

            if ( Gauge.IsAlive () )
                Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( tf0 * numvars * StepThread () + v, numtf * numvars ) );


            if ( checkmissingvalues ) {
            
                numrealsamples1 = GetRealNumberOfSamples    (   data1,
                                                                tf1,            v,          numsamples1,
                                                                missingvalue, 
                                                                sampleindex1
                                                            );

                if ( numrealsamples1 == 0 )
                    results1 ( tf0, v, TestNumSamples ) = 0;    // no data available


                numrealsamples2 = GetRealNumberOfSamples    (   data2,
                                                                tf2,            v,          numsamples2,
                                                                missingvalue, 
                                                                sampleindex2
                                                            );
            
                if ( numrealsamples2 == 0 )
                    results2 ( tf0, v, TestNumSamples ) = 0;    // no data available


                numrealsamples  = numrealsamples1 + numrealsamples2;

                                        // skip actual computation and continue loop
                if ( numrealsamples1 == 0 || numrealsamples2 == 0 ) {
                    jointresults ( tf0, v, Test_p_value   ) = 1.0;
                    continue;
                    }

                } // if checkmissingvalues
            else {
                numrealsamples1 = numsamples1;
                numrealsamples2 = numsamples2;
                numrealsamples  = numsamples1 + numsamples2;
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the data difference of averages
            long double         sum1        = 0;

            for ( int s = 0; s < numrealsamples1; s++ )     sum1 += data1 ( tf1, v, checkmissingvalues ? sampleindex1[ s ] : s );


            long double         sum2        = 0;

            for ( int s = 0; s < numrealsamples2; s++ )     sum2 += data2 ( tf2, v, checkmissingvalues ? sampleindex2[ s ] : s );

                                        // !this average retains the sign!
            long double         avg         = sum1 / numrealsamples1 
                                            - sum2 / numrealsamples2;

            long double         sumdata     = abs ( avg );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the permutations
            int                 countabove  = 0;

            for ( int e = 0; e < numrand; e++ ) {

                sum1    = 0;
                sum2    = 0;
                group1pick.Reset ();
                                        // pick for group1
                                        // use a selection, to avoid picking the same sample multiple times
                                        // ?As per the TAnova, there could be a slight bias toward the biggest group, making the p slightly lower?
                do     group1pick.Set ( randunif ( (uint) numrealsamples ) );
                while ( group1pick.NumSet () < numrealsamples1 );

                                        // assign to sums from group1
                for ( int s = 0; s < numrealsamples1; s++ )

                    if ( group1pick[ s ] )                      sum1 += data1 ( tf1, v, checkmissingvalues ? sampleindex1[ s ] : s );
                    else                                        sum2 += data1 ( tf1, v, checkmissingvalues ? sampleindex1[ s ] : s );

                                        // assign to sums from group2
                for ( int s = 0; s < numrealsamples2; s++ )

                    if ( group1pick[ s + numrealsamples1 ] )    sum1 += data2 ( tf2, v, checkmissingvalues ? sampleindex2[ s ] : s );
                    else                                        sum2 += data2 ( tf2, v, checkmissingvalues ? sampleindex2[ s ] : s );

                                        // 2-tailed, does not care for positive or negative
                if ( abs (   sum1 / numrealsamples1 
                           - sum2 / numrealsamples2 ) >= sumdata )
                    countabove++;

                } // for e


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute these variables
            long double         p           = Clip ( (long double) countabove / numrand, (long double) 0.0, (long double) 1.0 );

                                        // compute and store these variables
            jointresults ( tf0, v, TestNumSamples   )   = numrealsamples;
            jointresults ( tf0, v, TestMean         )   = avg;
            jointresults ( tf0, v, Test_p_value     )   = p;
            } // for variable

        } // for tf

    OmpParallelEnd

    } // if unpaired

} // Run_Randomization_test


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Compute 2 centroids with exactly numpercentroid maps in them, taken randomly from 2 groups
void        RandomizedMaps  (   const TMaps&        group1,         bool            group1tocentroid1,      bool            group1tocentroid2,
                                const TMaps&        group2,         bool            group2tocentroid1,      bool            group2tocentroid2,
                                int                 numpercentroid,
                                PolarityType        polarity,       PairedType      paired,
                                TMap&               centroid1,      TMap&           centroid2,
                                TVector<int>&       randindex1,     TVector<int>&   randindex2,
                                TRandCoin&          randcoin
                            )
{
                                        // randomized indexes for each group
//TVector<int>        randindex1;       // local versions, allocated each time...
//TVector<int>        randindex2;

int                 nummaps1            = group1.GetNumMaps ();
int                 nummaps2            = group2.GetNumMaps ();

                                        // number of picks per group can be either n (paired case), or at most 2n (unpaired case)

if ( paired == TestPaired ) {

    randindex1.RandomSeries (
                            numpercentroid, 
                            min ( nummaps1, nummaps2 ),
                            (TRandUniform*) &randcoin   // all TRand* classes just derive from TRand, they only specialize  their methods, not the TRand's content
                            );
                                        // !identical indexes!
    randindex2  = randindex1;
    }

else { // TestUnpaired

    randindex1.RandomSeries (
                            2 * numpercentroid, 
                            nummaps1,
                            (TRandUniform*) &randcoin   // all TRand* classes just derive from TRand, they only specialize  their methods, not the TRand's content
                            );
                                        // here a different series
    randindex2.RandomSeries (
                            2 * numpercentroid, 
                            nummaps2,
                            (TRandUniform*) &randcoin
                            );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // build an array of TMap* that randomly points either to group1 or group2 maps
TArray1<TMap*>      shuffgroup1 ( numpercentroid );
TArray1<TMap*>      shuffgroup2 ( numpercentroid );
int                 sgi1            = 0;
int                 sgi2            = 0;

bool                g1toc1;
bool                g1toc2;

                                        // Centroids do their marker: asking for n maps in each
for ( int ci = 0, ri1 = 0, ri2 = 0; ci < numpercentroid; ci++ ) {


    g1toc1  = ri1 <  nummaps1                               // check that we are not trying to pick more data than it exists
           &&      group1tocentroid1 
           && (  ! group2tocentroid1 
                || group2tocentroid1 && randcoin () )
           || ri2 == nummaps2;                              // also check that group2 is not totally done


    shuffgroup1[ sgi1++ ]   = g1toc1 ? &group1[ randindex1[ ri1++ ] ] 
                                     : &group2[ randindex2[ ri2++ ] ];

                                        // paired case has to be handled here, because we know which one has gone to centroid1, we can do centroid2 symmetrically
    if ( paired == TestPaired ) {

        shuffgroup2[ sgi2++ ]   = g1toc1 ? &group2[ randindex2[ ri2++ ] ] 
                                         : &group1[ randindex1[ ri1++ ] ];
        }

    else {                              // centroid2: unpaired case

        g1toc2  = ri1 <  nummaps1
               &&      group1tocentroid2 
               && (  ! group2tocentroid2 
                    || group2tocentroid2 && randcoin () )
               || ri2 == nummaps2;


        shuffgroup2[ sgi2++ ]   = g1toc2 ? &group1[ randindex1[ ri1++ ] ] 
                                         : &group2[ randindex2[ ri2++ ] ];
        }

    } // for centroid


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pick either mean or median of clusters
centroid1   = ComputeCentroid ( shuffgroup1, MeanCentroid, AtomTypeScalar, polarity );
centroid2   = ComputeCentroid ( shuffgroup2, MeanCentroid, AtomTypeScalar, polarity );
}


//----------------------------------------------------------------------------
void        Run_TAnova  (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                            StatTimeType            stattime1,          StatTimeType            stattime2,
                            int                     numtf,
                            int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                            int                     numvars,            
                            PairedType              paired,
                            int                     numrand,            
                            ReferenceType           processingref,      PolarityType            polarity, 
                            TArray3<float>&         Results,            TArray2<float>*         sampleddiss
                        )
{
char                title[ 256 ];

StringCopy      ( title, PairedTypeString[ paired ], " ", InfixTAnova );

TSuperGauge         Gauge ( title, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // index to average value (actually an additional data point)
int                 avgtfi              = GetTimeAverageIndex ( data1 );

int                 numsamples          = jointnumsamples;

bool                centeraverage       = processingref == ReferenceAverage;
double              precision           = 1e-6;
    
                                        // optionally returning the sampled dissimilarities - used for tracing
if ( sampleddiss )
    sampleddiss->Resize ( numtf, numrand + 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelBegin

TMaps               mapsg1      ( numsamples1, numvars );   // data, group1
TMaps               mapsg2      ( numsamples2, numvars );   // data, group2

TMap                centrdata1  ( numvars );
TMap                centrdata2  ( numvars );

TMap                centrrand1  ( numvars );
TMap                centrrand2  ( numvars );
                                    // by safety, allocate a random generator for each thread
TRandCoin           randcoin;
TVector<int>        randindex1;
TVector<int>        randindex2;

OmpFor

for ( int tf0 = 0; tf0 < numtf; tf0++ ) {
                                        // which index to use according to average
    int                 tf1             = IsTimeSequential ( stattime1 ) ? tf0 : avgtfi;
    int                 tf2             = IsTimeSequential ( stattime2 ) ? tf0 : avgtfi;

                                        // transfer & normalize data
    for ( int s = 0; s < numsamples1; s++ )
    for ( int v = 0; v < numvars;     v++ )

        mapsg1 ( s, v ) = data1 ( tf1, v, s );


    for ( int s = 0; s < numsamples2; s++ )
    for ( int v = 0; v < numvars;     v++ )

        mapsg2 ( s, v ) = data2 ( tf2, v, s );


    mapsg1.Normalize ( AtomTypeScalar, -1, centeraverage );
    mapsg2.Normalize ( AtomTypeScalar, -1, centeraverage );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute a first template aligned on the middle map of each group
                                        // pick either mean or median of clusters
    centrdata1  = mapsg1.ComputeCentroid ( MeanCentroid, AtomTypeScalar, polarity );
    centrdata2  = mapsg2.ComputeCentroid ( MeanCentroid, AtomTypeScalar, polarity );

                                        // same amount of data per centroid
//  RandomizedMaps  (   mapsg1,             true,   false,
//                      mapsg2,             false,  true,
//                      numminsamples,
//                      polarity,           paired, 
//                      centrdata1,         centrdata2
//                  );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the original data Correlation
    long double         corrdata        = RoundTo ( centrdata1.Correlation ( centrdata2, polarity, false ), precision );


    if ( sampleddiss )                  // store 1 value as the real data dissimilarity
        (*sampleddiss) ( tf0, 0 )   = RoundTo ( CorrelationToDifference ( corrdata ), precision );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the permutations
    int                 countabove      = 0;


    for ( int e = 0; e < numrand; e++ ) {

        if ( Gauge.IsAlive () )
            Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( tf0 * numrand * StepThread () + e, numtf * numrand ) );

                                        // randomly assign the same amount of maps to each random centroid
        RandomizedMaps  (   mapsg1,         true,       true,
                            mapsg2,         true,       true,
                            numsamples,
                            polarity,       paired,
                            centrrand1,     centrrand2,
                            randindex1,     randindex2,
                            randcoin
                        );

                                        // there we have the correlation of the randomized groups
        long double         corrrand        = RoundTo ( centrrand1.Correlation ( centrrand2, polarity, false ), precision );


        if ( sampleddiss )              // following values are the randomized ones (should be inferior if significatively different)
            (*sampleddiss) ( tf0, e + 1 )   = RoundTo ( CorrelationToDifference ( corrrand ), precision );

                                        // 1-tailed distribution
        if ( corrrand <= corrdata )     // if random map is further (more dissimilar / less correlated), then this is against the odds - random average should look closer
            countabove++;

        } // for e

                                        // count how many randomized pairs were above, i.e. distance increased for randomization instead of decreasing for the Null Hypothesis
    long double         p           = (long double) countabove / numrand;

                                        // compute and store these variables
    Results ( tf0, 0, TestNumSamples        )   = numsamples1 + numsamples2;
    Results ( tf0, 0, Test_Dissimilarity    )   = RoundTo ( CorrelationToDifference ( corrdata ), precision );
    Results ( tf0, 0, Test_p_value          )   = Clip ( (double) p, 0.0, 1.0 );
    } // for tf

OmpParallelEnd
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
