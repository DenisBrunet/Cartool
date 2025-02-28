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
cout << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2 input files
TFileName           filemaps        ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.sef" );
TFileName           filetemplates   ( "E:\\Data\\Test Files\\ERPs\\Faces.Templates.08.ep" );

                                        // output files for norm and gfp lines
TFileName           filemapsnorm    = filemaps;
TFileName           filemapsgfp     = filemaps;

PostfixFilename ( filemapsnorm, ".Norm" );
PostfixFilename ( filemapsgfp,  ".Gfp"  );

                                        // output files for correlations
TFileName           filetemplatescorr   = filetemplates;
TFileName           filemapscorr        = filemaps;
TFileName           filemapstemplcorr   = filemaps;

PostfixFilename ( filetemplatescorr, ".Correlations" );
PostfixFilename ( filemapscorr,      ".Correlations" );
PostfixFilename ( filemapstemplcorr, ".Correlations.Templates" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // TMaps class is relevant when working with the topographic approach, as it contains a lot of dedicated methods
                                        // Maps[ t ] points to a vector of e contiguous values, corresponding to the e electrodes at time point t

AtomType            datatype        = AtomTypeScalar;   // EEG is signed, scalar data
ReferenceType       ref             = ReferenceAverage; // Data will be read and averaged reference'd straight away

                                        // Read the maps with proper data type and reference set
TMaps               maps        ( filemaps,      0, datatype, ref );
TMaps               templates   ( filetemplates, 0, datatype, ref );


cout << "Num tracks                 = " << maps.GetDimension () << fastendl;
cout << "Num time points            = " << maps.GetNumMaps ()   << fastendl;
cout << "Sampling Frequency         = " << maps.GetSamplingFrequency ()   << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute a single track that holds the norm of each map for each time point
TVector<double>     norm;
maps.ComputeNorm ( norm, ref );

                                        // Compute a single track that holds the GFP of each map for each time point
TVector<double>     gfp;
maps.ComputeGFP ( gfp, ref, datatype );

                                        // Show the average norm and GFP
cout << "Average Norm               = " << norm.Average () << fastendl;
cout << "Average GFP                = " << gfp .Average () << fastendl;

                                        // Write them to files
cout << "Writing new file           = " << filemapsnorm << fastendl;
norm.WriteFile ( filemapsnorm );

cout << "Writing new file           = " << filemapsgfp << fastendl;
gfp .WriteFile ( filemapsgfp  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing some spatial correlations
PolarityType        polarity        = PolarityDirect;   // used for ERP cases, i.e. the sign of Correlation is relevant
ReferenceType       refcorr         = ReferenceNone;    // we don't need to recompute the average reference here, as data has been re-referenced when read
TMaps               corr;
                                        // Operating on objects in memory:

                                        // Correlations across templates
corr.Correlate  (   templates,  templates,  CorrelateTypeLinearLinear,  polarity, refcorr );
cout << "Writing new file           = " << filetemplatescorr << fastendl;
corr.WriteFile  ( filetemplatescorr );

                                        // Correlations across data
corr.Correlate  (   maps,       maps,       CorrelateTypeLinearLinear,  polarity, refcorr );
cout << "Writing new file           = " << filemapscorr << fastendl;
corr.WriteFile  ( filemapscorr      );

                                        // Correlations between data and templates
corr.Correlate  (   maps,       templates,  CorrelateTypeLinearLinear,  polarity, refcorr );
cout << "Writing new file           = " << filemapstemplcorr << fastendl;
corr.WriteFile  ( filemapstemplcorr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Operating on files - same results as above:
cout << "Writing new file           = " << "Correlation between 2 files" << fastendl;
CorrelateFiles (    TGoF ( filemaps ),  TGoF ( filetemplates ), // single file is converted to a group of files (TGoF) on the fly
                    CorrelateTypeSpatialCorrelation, 
                    polarity,
                    false, false, 0,
                    0,
                    0,
                    "",         0
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
}
