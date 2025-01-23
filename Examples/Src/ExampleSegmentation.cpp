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

using namespace std;
using namespace crtl;

void    ExampleSegmentation ()
{
cout << fastendl;
cout << "--------------------------------------------------------------------------------" << fastendl;
cout << "ExampleSegmentation:" << fastendl;
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
TFileName           basedir;
TFileName           basefilename;
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


basedir         = "E:\\Data\\Test Files\\ERPs\\ExampleSegmentation.KMeans";

cout << "Microstates segmentation   : " << "Running k-means clustering.." << fastendl;

bool    segok   =

ms.Segmentation (   gofdata,    
                    AnalysisERP,            ModalityEEG,
                    atomtype,               polarity,           dataref,
                    ClusteringKMeans,
                    1,                      maxseg,
                    numkmeanstrials,                        // number of random trials
                    false,                  -1,             // limit on correlation
                    true,                                   // sequentialize
                    false,                  -1,             // merge correlation
                    true,                   3,     20,      // smoothing
                    true,                   3,              // reject small segments
                    MapOrderingTemporally,  0,              // templates ordering
                    basedir
                );

cout << "Success                    : " << BoolToString ( segok ) << fastendl;


basefilename    = basedir;
AppendFilenameAsSubdirectory ( basefilename );


markersfile     = basefilename + ".error.data.mrk";
markers.ReadFile ( markersfile );
int                 metacritkm      = markers[ 0 ]->From + 1;
cout << "Meta-Criterion             = " << metacritkm << fastendl;


templatefile    = basefilename + "." + IntegerToString ( metacritkm, 2 ) + ".*." + FILEEXT_EEGEP;
GetFirstFile ( templatefile );
cout << "Template file              : " << templatefile << fastendl;


segfile         = basefilename + "." + IntegerToString ( metacritkm, 2 ) + ".*." + FILEEXT_SEG;
GetFirstFile ( segfile );
cout << "Segmentation file          : " << segfile << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

basedir         = "E:\\Data\\Test Files\\ERPs\\ExampleSegmentation.TAAHC";

cout << fastendl;
cout << "Microstates segmentation   : " << "Running T-AAHC clustering.." << fastendl;

segok   =

ms.Segmentation (   gofdata,    
                    AnalysisERP,            ModalityEEG,
                    atomtype,               polarity,           dataref,
                    ClusteringTAAHC,
                    1,                      maxseg,
                    0,                                      // number of random trials - not relevant here
                    false,                  -1,             // limit on correlation
                    true,                                   // sequentialize
                    false,                  -1,             // merge correlation
                    true,                   3,     20,      // smoothing
                    true,                   3,              // reject small segments
                    MapOrderingTemporally,  0,              // templates ordering
                    basedir
                );

cout << "Success                    : " << BoolToString ( segok ) << fastendl;


basefilename    = basedir;
AppendFilenameAsSubdirectory ( basefilename );


markersfile     = basefilename + ".error.data.mrk";
markers.ReadFile ( markersfile );
int                 metacritta      = markers[ 0 ]->From + 1;
cout << "Meta-Criterion             = " << metacritta << fastendl;


templatefile    = basefilename + "." + IntegerToString ( metacritta, 2 ) + ".*." + FILEEXT_EEGEP;
GetFirstFile ( templatefile );
cout << "Template file              : " << templatefile << fastendl;


segfile         = basefilename + "." + IntegerToString ( metacritta, 2 ) + ".*." + FILEEXT_SEG;
GetFirstFile ( segfile );
cout << "Segmentation file          : " << segfile << fastendl;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // A bit more "manual": running the loop ourselves and calling each processing step
                                        // This is for the sake of demo, don't do that unless you really know what you are doing!
basedir         = "E:\\Data\\Test Files\\ERPs\\ExampleSegmentation.KMeans.Manually";
basefilename    = basedir;
AppendFilenameAsSubdirectory ( basefilename );
CreatePath      ( basefilename,     true );


cout << fastendl;
cout << "Microstates segmentation   : " << "Manually running k-means clustering.." << fastendl;


ms.ReadData         ( gofdata, atomtype, ReferenceAsInFile, false );

ms.PreprocessMaps   (   ms.Data,
                        false,
                        atomtype,           polarity,           dataref,
                        false,
                        GetProcessingRef ( ProcessingReferenceEEG ),
                        true,           // always normalized
                        true            // compute Norm Gfp Dis arrays
                    );


for ( int numclusters = 2; numclusters <= maxseg; numclusters++ ) {

    cout << "#Clusters  = " << numclusters;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TMaps               templates;
    TLabeling           labels;
    int                 numsegments;

    numsegments     = ms.SegmentKMeans  (   numclusters,       templates,       labels,
                                            polarity, 
                                            numkmeanstrials,
                                            MeanCentroid,
                                            false
                                        );

    cout  << Tab << "->" << Tab << "#Segments  = " << numsegments << fastendl;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Re-ordering clusters for the sake of comparison
    TArray2<int>        ordering;

    ms.GetTemporalOrdering  ( numclusters, labels, ordering );

    labels.ReorderLabels ( templates, ordering );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Write segmentation & templates
    TFileName           segtemplatesfile    = basefilename + "." + IntegerToString ( numclusters, 2 ) + ".Segmentation.Templates.sef";
    TFileName           segfile             = basefilename + "." + IntegerToString ( numclusters, 2 ) + ".Segmentation.seg";


    ms.WriteSegFile (   numsegments,
                        templates,
                        labels,
                        segfile
                    );

    templates.WriteFile ( segtemplatesfile );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for numclusters


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cout << fastendl;
}
