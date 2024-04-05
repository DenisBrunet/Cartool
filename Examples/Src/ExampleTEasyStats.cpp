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

#include    <iostream>

#include	"Math.Stats.h"
#include    "Math.Random.h"

using namespace std;
using namespace crtl;

void    ExampleTEasyStats ()
{
                                        // To make use any robust stats, one needs to pre-allocate the object,
                                        // or use the Resize method. Each data point passed to the Add method will
                                        // be stored internally.
                                        // It is best to allocate the exact number of data points that will be
                                        // needed, although it will gracefully resize itself if caller requests more
int                 numdata     = 1000000;
TEasyStats          stat ( numdata );   

                                        // Create a Normal random generator object
double              center          = 0;
double              spread          = 1;
TRandNormal         randnormal ( center, spread );

for ( int i = 0; i < numdata; i++ )
    stat.Add ( randnormal (), ThreadSafetyIgnore );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
cout << "ExampleTEasyStats:" << fastendl;
cout << "Picking " << numdata << " random Gaussian data N(" << center << "," << spread << ")" << fastendl;
                                        // Parametric stats - these do not need any allocation
cout << "Mean           = " << stat.Mean    ()                      << fastendl;
cout << "SD             = " << stat.SD      ()                      << fastendl;
                                        // Robust stats - at that point, data will be sorted
cout << "Min            = " << stat.Min     ()                      << fastendl;
cout << "Max            = " << stat.Max     ()                      << fastendl;
cout << "Median         = " << stat.Median  ()                      << fastendl;
cout << "MAD            = " << stat.MAD     ()                      << fastendl;
cout << "Quantile 25%   = " << stat.Quantile            ( 0.25 )    << fastendl;
cout << "Quantile 75%   = " << stat.Quantile            ( 0.75 )    << fastendl;
cout << "IQM            = " << stat.InterQuartileMean   ()          << fastendl;
cout << "IQR            = " << stat.InterQuartileRange  ()          << fastendl;

cout << fastendl << flush;
}
