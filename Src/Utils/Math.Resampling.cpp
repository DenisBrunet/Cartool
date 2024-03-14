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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "TBaseDialog.h"
#include    "Math.Resampling.h"
#include    "Dialogs.Input.h"

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace crtl {

//----------------------------------------------------------------------------
        TDownsampling::TDownsampling ()
{
Reset ();
}


        TDownsampling::TDownsampling ( int numdata, int numwishedata )
{
Set ( numdata, numwishedata ); 
}


        TDownsampling::TDownsampling ( int from, int to, int numwishedata )
{
Set ( from, to, numwishedata ); 
}


void    TDownsampling::Reset ()
{
From            = 0;
To              = 0;
NumData         = 0;
NumDownData     = 0;
Step            = 1;
}


//----------------------------------------------------------------------------
                                        // Caller has an actual number of data points, and wish for another (approximate) amount
                                        // resulting in a step value and an exact final number
void    TDownsampling::Set ( int numdata, int numwishedata )
{
Reset ();

                                        // no data?
if ( numdata <= 0 )
    return;

NumData     = numdata;

                                        // just in case this parameter is 0, then use all data
if ( numwishedata <= 0 )
    numwishedata    = NumData;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // less data than wished data?
if ( NumData <= numwishedata ) {

    Step            = 1;

    NumDownData     = NumData;
    }

else {                                  // here we have more data than wished for, so we can downsample

                                        // truncated ratio -> lowest step -> highest number of sub-samples
//  Step      = AtLeast ( 1, NumData / numwishedata );
                                        // rounded   ratio -> closest number of sub-samples
    Step        = AtLeast ( 1, Round ( (double) NumData / numwishedata ) );
                                        // compute the exact number of data according to this step
    NumDownData = ( NumData + Step - 1 ) / Step;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting the final interval, starting from 0 as caller didn't specify an absolute beginning
SetInterval ( 0, NumData - 1 );
}


//----------------------------------------------------------------------------
                                        // !Final From and To can be recentered and could differ from the input 'from' and 'to'!
void    TDownsampling::Set ( int from, int to, int numwishedata )
{
CheckOrder ( from, to );

                                        // most of the job will be done here
Set         ( to - from + 1, numwishedata );

                                        // setting a nicely re-centered interval
SetInterval ( from, to );
}


//----------------------------------------------------------------------------
                                        // Correctly setting the final interval
                                        // !Uses NumData, NumDownData and Step!
void    TDownsampling::SetInterval ( int from, int /*to*/ )
{
                                        // formula is biased toward From
//From    = from;                       
//To      = to;

                                        // re-center the final interval
int                 lastrelpos      = ( NumDownData - 1 ) * Step;       // last relative TF reached when starting from 0
int                 delta           = ( NumData     - 1 ) - lastrelpos; // amount of trailing points that will be skipped due to the last incomplete step

From    = from + delta / 2;             // spread the delta as equally as possible on each side of the interval
To      = From + lastrelpos;            // set the last position on the exact absolute last step

//DBGV5 ( From, To, NumData, Step, NumDownData, "From, To, NumData, Step, NumDownData" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TResampling::TResampling ()
{
Reset ();
}


void    TResampling::Reset ()
{
NumData         = 0;
SampleSize      = 0;
NumResampling   = 0;
Coverage        = 0;
}


//----------------------------------------------------------------------------
void    TResampling::Show (char *title )
{
char                buff[ 1024 ];


sprintf ( buff,               "Number of Data: \t\t%0d\n",  NumData    );
sprintf ( StringEnd ( buff ), "Number of Resampling: \t%0d\n",  NumResampling );
sprintf ( StringEnd ( buff ), "Sample Size (abs): \t\t%0d\n",  SampleSize );
sprintf ( StringEnd ( buff ), "Sample Size (rel): \t\t%0g%%\n",  RoundTo ( GetRelativeSampleSize () * 100, 0.1 ) );
sprintf ( StringEnd ( buff ), "Coverage: \t\t%0g%%", RoundTo ( Coverage * 100, 0.1 ) );


ShowMessage ( buff, StringIsEmpty ( title ) ? "Statistics" : title );
}


//----------------------------------------------------------------------------
                                        // Compute the number of resampling to reach that coverage
double  TResampling::SampleSizeToNumResampling ( double coverage )
{
if ( NumData <= 0 || SampleSize <= 0 )
    return  0;

                                   // probability to NOT pick a given item from one sample of SampleSize size, without replacement
double              probnottf       = 1 - Clip ( SampleSize / (double) NumData, 0.0, 1.0 );

if ( probnottf == 0 )
    return  1;

                                        // number of resampling to reach that coverage
Clipped ( coverage, ResamplingMinCoverage, ResamplingMaxCoverage );

int                 numresampling   = AtLeast ( 1, Round ( LogBase ( AtLeast ( MinLogValue, 1 - coverage ), probnottf ) ) );

return  numresampling;
}


                                        // Compute the number of resampling to reach that coverage
double  TResampling::NumResamplingToSampleSize ( double coverage )
{
if ( NumData <= 0 || NumResampling <= 0 )
    return  0;


Clipped ( coverage, ResamplingMinCoverage, ResamplingMaxCoverage );


double              covinv          = coverage == 1 ? 0 : Power ( 1 - coverage, 1.0 / NumResampling );


int                 samplesize      = Clip ( RoundAbove ( NumData * ( 1 - covinv ) ), 1, NumData );


return  samplesize;
}


//----------------------------------------------------------------------------
                                        // Compute the probability that any given sample has to be picked
                                        // It therefor gives an estimate of the how much data has been covered
double  TResampling::GetCoverage ()
{
Coverage    = 0;

if ( NumData == 0 || SampleSize == 0 || NumResampling == 0 )
    return  Coverage;

                                   // probability to NOT pick a given item from one sample of SampleSize size, without replacement
double              probnottf       = 1 - Clip ( SampleSize / (double) NumData, 0.0, 1.0 );

if ( probnottf == 0 ) {
    Coverage    = 1;
    return  Coverage;
    }

               // probability that all successive samples don't have that given item
Coverage    = 1 - Power ( probnottf, NumResampling );

//                                        // For 10'000 data and 2'000 sample size
//                                        //     5 resampling -> 67%
//                                        //    10 resampling -> 89%
//                                        //    15 resampling -> 96%


//                                        // number of picks with replacement, i.e. putting back an item / allowing to resample the same item multiple times
//int                 numpicks        = NumResampling * SampleSize;
//                                        // probability that a given sample has been picked, the more picks, the higher
//                                        //      63% for 10'000 TF and   10'000 picks
//                                        //      86%                   2*10'000 picks
//                                        //      95%                   3*10'000 picks
//Coverage    = 1 - Power ( 1 - 1.0 / NumData, numpicks );


return  Coverage;
}


//----------------------------------------------------------------------------
                                        // Compute the relative sampling size, compared to the amount of data
                                        // !Both sampling size and num data should be set beforehand!
double  TResampling::GetRelativeSampleSize ()
{
return  SampleSize / NonNull ( NumData );
}


//----------------------------------------------------------------------------
void    TResampling::SetNumData ( int numdata )
{
NumData         = AtLeast ( 0, numdata );
}


void    TResampling::SetSampleSize ( int samplesize )
{
SampleSize      = Clip ( samplesize, 0, NumData );
}


void    TResampling::SetSampleSize ( const char* samplesize )
{
SetSampleSize ( StringToInteger ( samplesize ) );
}


void    TResampling::SetSampleSize ( owl::TEdit* samplesize )
{
char                buff[ 16 ];

samplesize->GetText ( buff, EditSizeText );

SetSampleSize ( buff );
}


void    TResampling::SetNumResampling ( int numresampling )
{
NumResampling   = AtLeast ( 0, numresampling );
}


void    TResampling::SetNumResampling ( const char* numresampling )
{
SetNumResampling ( StringToInteger ( numresampling ) );
}


void    TResampling::SetNumResampling ( owl::TEdit* numresampling )
{
char                buff[ 16 ];

numresampling->GetText ( buff, EditSizeText );

SetNumResampling ( buff );
}


//----------------------------------------------------------------------------
                                        // sample size -> # samples
int     TResampling::GetNumResampling   (   double  coverage,
                                            int     minnumsamples,      int maxnumsamples
                                        )
{
NumResampling   = 0;
                                        // no data or null sample size?
if ( NumData <= 0 || SampleSize <= 0 || ! IsInsideLimits ( coverage, 0.0, 1.0 ) )
    return  NumResampling;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // > 0
//NumResampling      = AtLeast ( 1, Round ( ( ResamplingPickRatio * NumData ) / SampleSize ) );
                                        // caller is asking for that given level of certainty
NumResampling   = SampleSizeToNumResampling ( coverage );

                                        // optionally forcing a minimum number of sample sets - no upper bound check here
if ( minnumsamples > 0 )
    Maxed ( NumResampling, minnumsamples );

                                        // optionally forcing a maximum number of sample sets - at least 1
if ( maxnumsamples > 0 )
    Mined ( NumResampling, maxnumsamples );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from SampleSize and NumResampling, deduce the probability to reach any given sample
GetCoverage ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  NumResampling;
}


//----------------------------------------------------------------------------
                                        // # samples -> sample size
int     TResampling::GetSampleSize  (   double  coverage,
                                        int     minsamplesize,      int maxsamplesize
                                    )
{
SampleSize      = 0;
                                        // no data or no samples?
if ( NumData <= 0 || NumResampling <= 0 || ! IsInsideLimits ( coverage, 0.0, 1.0 ) )
    return  SampleSize;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // > 0 and <= NumData
//SampleSize      = Clip ( Round ( ( ResamplingPickRatio * NumData ) / NumResampling ), 1, NumData );
                                        // caller is asking for that given level of certainty
SampleSize      = NumResamplingToSampleSize ( coverage );

                                        // optionally forcing a minimum sample size - without exceeding the number of data
if ( minsamplesize > 0 )
    Maxed ( SampleSize, NoMore ( NumData, minsamplesize ) );

                                        // optionally forcing a maximum sample size - at least 1
if ( maxsamplesize > 0 )
    Mined ( SampleSize, maxsamplesize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from SampleSize and NumResampling, deduce the probability to reach any given sample
GetCoverage ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  SampleSize;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







