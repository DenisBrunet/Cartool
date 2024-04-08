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

#include    "TMaps.h"
#include    "TExportTracks.h"
#include    "CorrelateFiles.h"

using namespace std;
using namespace crtl;

void    ExampleTMaps ()
{
cout << fastendl;
cout << "--------------------------------------------------------------------------------" << fastendl;
cout << "ExampleTMaps:" << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // TMaps class is relevant to work with the topographic approach, as it contains a lot of dedicated methods
                                        // Maps[ t ] points to a vector of e contiguous values, corresponding to the e electrodes at time point t
TFileName           filemaps        ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.Spatial.sef" );
TFileName           filetemplates   ( "E:\\Data\\Test Files\\ERPs\\Faces.Templates.08.ep" );

TMaps               maps        ( filemaps,      AtomTypeScalar, ReferenceAverage );
TMaps               templates   ( filetemplates, AtomTypeScalar, ReferenceAverage );


cout << "Num tracks                 = " << maps.GetDimension () << fastendl;
cout << "Num time points            = " << maps.GetNumMaps ()   << fastendl;
cout << "Sampling Frequency         = " << maps.GetSamplingFrequency ()   << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<double>     norm;
maps.ComputeNorm ( norm, ReferenceAverage );

TVector<double>     gfp;
maps.ComputeGFP ( gfp, ReferenceAverage, AtomTypeScalar );

cout << "Average Norm               = " << norm.Average () << fastendl;
cout << "Average GFP                = " << gfp.Average () << fastendl;

norm.WriteFile ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.Spatial.Norm.sef" );
gfp .WriteFile ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.Spatial.Gfp.sef" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing some spatial correlations
TMaps           corr;

                                        // Operating on objects in memory

                                        // Correlations across templates
corr.Correlate  (   templates,  templates,  CorrelateTypeLinearLinear,  PolarityDirect, ReferenceNone );
corr.WriteFile ( "E:\\Data\\Test Files\\ERPs\\Faces.Templates.08.Correlations.sef" );

                                        // Correlations across data
corr.Correlate  (   maps,       maps,       CorrelateTypeLinearLinear,  PolarityDirect, ReferenceNone );
corr.WriteFile ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.Spatial.Correlations.sef" );

                                        // Correlations between data and templates
corr.Correlate  (   maps,       templates,  CorrelateTypeLinearLinear,  PolarityDirect, ReferenceNone );
corr.WriteFile ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.Spatial.Corr.Templates.sef" );

                                        // Operating on files - same results as above
CorrelateFiles (    TGoF ( filemaps ),           TGoF ( filetemplates ), 
                    CorrelateTypeSpatialCorrelation, 
                    PolarityDirect,
                    false, false, 0,
                    0,
                    0,
                    "",         0
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl << flush;
}
