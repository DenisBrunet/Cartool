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

#include    "Files.Stream.h"
#include    "TTracks.h"
#include    "TTracksDoc.h"
#include    "TExportTracks.h"

using namespace std;
using namespace crtl;

void    ExampleTTracks ()
{
cout << fastendl;
cout << "--------------------------------------------------------------------------------" << fastendl;
cout << "ExampleTTracks:" << fastendl;
cout << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // TTracks class is quite minimalistic, and is used as a 2D array with a few more properties
                                        // Useful when time series are needed:
                                        //  - Data is stored as row-major, contiguous in the second dimension ("time")
                                        //  - Data[ 0 ] hence points to the time series of first track, Data[ 1 ] of second track etc..
TTracks<float>      tracks;
TFileName           fileinput   ( "E:\\Data\\Test Files\\Spontaneous\\cenbas.sef" );
TFileName           fileoutput  ( "E:\\Data\\Test Files\\Spontaneous\\cenbas.Gaussian.sef" );

tracks.ReadFile ( fileinput );

cout << "Num tracks                 = " << tracks.GetDim1 () << fastendl;
cout << "Num time points            = " << tracks.GetDim2 () << fastendl;
cout << "Min Value                  = " << tracks.GetMinValue () << fastendl;
cout << "Max Value                  = " << tracks.GetMaxValue () << fastendl;


TVector<double>     gfp;
tracks.ComputeGFP ( gfp );
cout << "Average GFP                = " << gfp.Average () << fastendl;


FctParams       p;
p ( FilterParamDiameter )   = 7;
                                        // Filtering dimension 2, i.e. "tracks"
tracks.FilterDim2 ( FilterTypeFastGaussian, p );

cout << "Writing new file           = " << fileoutput << fastendl;
tracks.WriteFile ( fileoutput );

tracks.ComputeGFP ( gfp );
cout << "Average GFP after Gaussian = " << gfp.Average () << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
}
