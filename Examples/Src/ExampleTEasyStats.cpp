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

#include	"Math.Stats.h"
#include    "Math.Random.h"

using namespace std;
using namespace crtl;

void    ExampleTEasyStats ()
{
cout << fastendl;
cout << "--------------------------------------------------------------------------------" << fastendl;
cout << "ExampleTEasyStats:" << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create a Normal random generator object
double              center          = 0;
double              spread          = 1;
TRandNormal         randnormal ( center, spread );

int                 numdata     = 1000000;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Fast stats, without internal storage
TEasyStats          statna;

for ( int i = 0; i < numdata; i++ )
    statna.Add ( randnormal (), ThreadSafetyIgnore );


cout << fastendl;
cout << "TEasyStats, non-allocated:" << fastendl;

cout << "Picking " << numdata << " random Gaussian data N(" << center << "," << spread << ")" << fastendl;
                                        // Parametric stats - these do not need any allocation
cout << "Min            = " << statna.Min     ()                    << fastendl;
cout << "Max            = " << statna.Max     ()                    << fastendl;
cout << "Range          = " << statna.Range   ()                    << fastendl;
cout << "AbsoluteMax    = " << statna.AbsoluteMax   ()              << fastendl;
cout << "Mean           = " << statna.Mean    ()                    << fastendl;
cout << "Variance       = " << statna.Variance()                    << fastendl;
cout << "SD             = " << statna.SD      ()                    << fastendl;
cout << "CoV            = " << statna.CoV     ()                    << fastendl;
cout << "SNR            = " << statna.SNR     ()                    << fastendl;
cout << "Sum            = " << statna.Sum     ()                    << fastendl;
cout << "Sum2           = " << statna.Sum2    ()                    << fastendl;

cout << fastendl << flush;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // To make use any robust stats, one needs to pre-allocate the object,
                                        // or use the Resize method. Each data point passed to the Add method will
                                        // be stored internally.
                                        // It is best to allocate the exact number of data points that will be
                                        // needed, although it will gracefully resize itself if caller requests more
TEasyStats          stata ( numdata );   

for ( int i = 0; i < numdata; i++ )
    stata.Add ( randnormal (), ThreadSafetyIgnore );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
cout << "TEasyStats, allocated:" << fastendl;

cout << "Picking " << numdata << " random Gaussian data N(" << center << "," << spread << ")" << fastendl;

                                        // Parametric stats - these do not need any allocation
cout << "Min                    = " << stata.Min    ()                      << fastendl;
cout << "Max                    = " << stata.Max    ()                      << fastendl;
cout << "Range                  = " << stata.Range  ()                      << fastendl;
cout << "AbsoluteMax            = " << stata.AbsoluteMax()                  << fastendl;
cout << "Mean                   = " << stata.Mean   ()                      << fastendl;
cout << "Variance               = " << stata.Variance   ()                  << fastendl;
cout << "SD                     = " << stata.SD     ()                      << fastendl;

                                        // Robust stats - at that point, data will be sorted
cout << "Median                 = " << stata.Median ()                      << fastendl;
cout << "IQM                    = " << stata.InterQuartileMean ()           << fastendl;
cout << "TruncatedMean          = " << statna.TruncatedMean ( 0.1, 0.9 )    << fastendl;
cout << "FirstMode              = " << stata.FirstMode ()                   << fastendl;
cout << "LastMode               = " << stata.LastMode ()                    << fastendl;
cout << "MaxModeHistogram       = " << stata.MaxModeHistogram ()            << fastendl;
cout << "MaxModeHRM             = " << stata.MaxModeHRM ()                  << fastendl;
cout << "MaxModeHSM             = " << stata.MaxModeHSM ()                  << fastendl;

cout << "Quantile 25%           = " << stata.Quantile ( 0.25 )              << fastendl;
cout << "Quantile 75%           = " << stata.Quantile ( 0.75 )              << fastendl;
cout << "MAD                    = " << stata.MAD ()                         << fastendl;
cout << "IQR                    = " << stata.InterQuartileRange ()          << fastendl;
cout << "Qn                     = " << stata.Qn ( 1000 )                    << fastendl;
cout << "Sn                     = " << stata.Sn ( 1000 )                    << fastendl;

cout << "CoV                    = " << statna.CoV ()                        << fastendl;
cout << "RobustCoV              = " << statna.RobustCoV ()                  << fastendl;
cout << "SNR                    = " << statna.SNR ()                        << fastendl;
cout << "RobustSNR              = " << statna.RobustSNR ()                  << fastendl;
cout << "Kurtosis               = " << stata.Kurtosis  ()                   << fastendl;
cout << "RobustKurtosis         = " << stata.RobustKurtosis ()              << fastendl;
cout << "SkewnessPearson        = " << statna.SkewnessPearson ()            << fastendl;
cout << "RobustSkewnessPearson  = " << statna.RobustSkewnessPearson ()      << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl << flush;
}
