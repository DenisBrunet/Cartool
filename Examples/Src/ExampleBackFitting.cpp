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
#include    "TMicroStatesFitDialog.h"

using namespace std;
using namespace crtl;

void    ExampleBackFitting ()
{
cout << fastendl;
cout << "--------------------------------------------------------------------------------" << fastendl;
cout << "ExampleBackFitting:" << fastendl;
cout << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data
TGoF                gofdata;
                                        // 4 conditions ERPs
gofdata.Add ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE1.V1.Recut.sef"     );
gofdata.Add ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE2pv.V2pv.Recut.sef" );
gofdata.Add ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE2vu.V2vu.Recut.sef" );
gofdata.Add ( "E:\\Data\\Test Files\\ERPs\\Face.Avg RISE3.V3.Recut.sef"     );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need a few file names..
TFileName           segbasedir;
TFileName           segbasefilename;
TFileName           fitbasedir;
TFileName           markersfile;
TFileName           templatefile;
TFileName           segfile;
TMarkers            markers;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sets and runs micro-states analysis
TMicroStates        ms;
int                 maxseg          = 15;
AtomType            atomtype        = AtomTypeScalar;
PolarityType        polarity        = PolarityDirect;
ReferenceType       dataref         = ReferenceAverage;
int                 numkmeanstrials = 300;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Running a clustering so we have some templates
segbasedir      = "E:\\Data\\Test Files\\ERPs\\ExampleFitting\\QuickSegmentation";
segbasefilename = segbasedir;
AppendFilenameAsSubdirectory ( segbasefilename );


cout << fastendl;
cout << "Microstates back-fitting   : " << "Running 1 k-means clustering.." << fastendl;


int                 numclusters     = 4;

ms.Segmentation (   gofdata,    
                    AnalysisERP,            ModalityEEG,
                    atomtype,               polarity,           dataref,
                    ClusteringKMeans,
                    numclusters,            numclusters,
                    numkmeanstrials,                        // number of random trials
                    false,                  -1,             // limit on correlation
                    true,                                   // sequentialize
                    false,                  -1,             // merge correlation
                    true,                   3,     20,      // smoothing
                    true,                   3,              // reject small segments
                    MapOrderingTemporally,  0,              // templates ordering
                    segbasedir
                );


templatefile    = segbasefilename + "." + IntegerToString ( numclusters, 2 ) + ".*." + FILEEXT_EEGEP;
GetFirstFile ( templatefile );
cout << "Template file              : " << templatefile << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Back-Fitting
fitbasedir      = "E:\\Data\\Test Files\\ERPs\\ExampleFitting";

                                        // Specifying the variables to extract:
TSelection          varout   ( fitnumvar,       OrderSorted );

varout.Set ( fitfonset       );
varout.Set ( fitloffset      );
varout.Set ( fitnumtf        );
varout.Set ( fittfcentroid   );
varout.Set ( fitmeancorr     );
varout.Set ( fitgev          );
varout.Set ( fitbcorr        );
varout.Set ( fittfbcorr      );
varout.Set ( fitgfptfbcorr   );
varout.Set ( fitmaxgfp       );
varout.Set ( fittfmaxgfp     );
varout.Set ( fitmeangfp      );
varout.Set ( fitmeanduration );
varout.Set ( fittimecoverage );
varout.Set ( fitsegdensity   );

                                        // Specifying the output files:
MicroStatesOutFlags outputflags         = NoMicroStatesOutFlags;

SetFlags ( outputflags, VariablesLongNames          );

SetFlags ( outputflags, SheetLinesAsSamples         );
SetFlags ( outputflags, SheetColumnsAsFactors       );
SetFlags ( outputflags, SaveOneFilePerGroup         );

SetFlags ( outputflags, WriteClustersFiles          );
SetFlags ( outputflags, WriteEmptyClusters          );
SetFlags ( outputflags, WriteCorrelationFiles       );

SetFlags ( outputflags, DeleteTempFiles             );


cout << "Microstates back-fitting   : " << "Running.." << fastendl;

bool    fitok   =

ms.BackFitting  (   templatefile,
                    TGoGoF ( gofdata ),     1,      // fitting on the grand means here - it should give the same results as the segmentation!
                    AnalysisERP,            ModalityEEG,
                    atomtype,   polarity,   dataref,
                    false,      -1,                 // limit on correlation
                    true,       3,          20,     // smoothing
                    true,       3,                  // reject small segments
                    varout,
                    outputflags,
                    fitbasedir
                );

cout << "Success                    : " << BoolToString ( fitok ) << fastendl;

                                        // Get rid of the temp segmentation - template file has been locally copied anyway...
NukeDirectory ( segbasedir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
}
