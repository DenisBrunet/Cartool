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

#include    <owl/pch.h>

#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"
#include    "TFilters.h"
#include    "CartoolTypes.h"            // PolarityType CentroidType

#include    "ComputeCentroidFiles.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::ComputeCentroidFilesUI ( owlwparam w )
{
using namespace crtl;

GetFileFromUser     files ( "Set of files to compute the centroids from:", AllErpEegRisFilesFilter,
                            w == CM_COMPUTECENTROIDEEG      ?   1
                          : w == CM_COMPUTECENTROIDRIS      ?   4
                          :                                     1, GetFileMulti );

if ( ! files.Execute () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now run a lot of consistency checks
const TGoF&         gof             = (const TGoF&) files;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Too restrictive, we could bear to average different extensions together
//if ( ! gof.CheckExtension ( "Group of files doesn't share the same extension!" ) )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check whole group file types
TracksGroupClass        tg;

if ( ! gof.AnyTracksGroup ( tg ) ) {
    if ( tg.noneofthese )   ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    else                    ShowMessage ( "Files don't seem to be consistently of the same type!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check whole group dimensions
TracksCompatibleClass   tc;

                                        // Checks for the most important dimensions (note that numsolpoints is NumTracks for .ris)
gof.AllTracksAreCompatible ( tc );

//DBGV6 ( NumTracks, numauxtracks, numsolpoints, numtf, NumFreqs, samplingfrequency, "NumTracks, numauxtracks, numsolpoints, numtf, NumFreqs, samplingfrequency" );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of electrodes/tracks!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    return;
    }
else if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes/tracks at all!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    return;
    }


//if      ( tc.NumAuxTracks > 0 ) {
//    ShowMessage ( "Files shouldn't have auxiliary channels!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
//    return;
//    }
                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( tc.NumAuxTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of auxiliary tracks!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    return;
    }


if      ( tc.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    return;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
//  ShowMessage ( "Files don't seem to have the same sampling frequencies!\nCheck again your input files...", CentroidTitle, ShowMessageWarning );
    if ( ! GetAnswerFromUser ( "Files don't seem to have the same sampling frequencies!\nDo you want to proceed anyway (not recommended)?", CentroidTitle ) )
        return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From here, files are assumed to be compatibles
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ask first for the type of centroid                                        
char                answer          = GetOptionFromUser (   "Type of Centroid:"     NewLine 
                                                            Tab "(M)ean"            NewLine 
                                                            Tab "(W)eighted Mean"   NewLine 
                                                            Tab "Me(D)ian"          NewLine 
                                                            Tab "Med(O)id"          NewLine 
                                                            Tab "(E)igenvector"     NewLine 
                                                            Tab "Ma(X)", 
                                                          CentroidTitle, "M W D O E X", "M", this );

if ( answer == EOS )   return;


CentroidType        centroidflag    = answer == 'M' ? MeanCentroid
                                    : answer == 'W' ? WeightedMeanCentroid
                                    : answer == 'D' ? MedianCentroid
                                    : answer == 'O' ? MedoidCentroid
                                    : answer == 'E' ? EigenVectorCentroid
                                    : answer == 'X' ? MaxCentroid
                                    :                 MeanCentroid;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data type

                                        // extra care for vectorial ris
bool                centroidv       = false;

if ( tg.allrisv ) {
                                        // in case of vectorial RIS, offer to compute either on the norm or on the vectors
    answer      = GetOptionFromUser ( "RIS data appear to be vectorial,\ncomputing the (N)orm or the (V)ectorial centroids?", 
                                      CentroidTitle, "N V", "N", this );

    centroidv   = answer == 'V';
    }


AtomType            datatype        = tg.alleeg                 ?   AtomTypeScalar
                                    : tg.allrisv && centroidv   ?   AtomTypeVector 
                                    : tg.allris                 ?   AtomTypePositive
                                    :                               AtomTypeScalar;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Current way the segmentation produces RIS templates

                                        // ranking norm RIS
bool                ranking         = tg.allris 
//                                 && IsPositive ( datatype )           // "vectorial" ranking is allowed
                                   && centroidflag != MedianCentroid    // Median can produce a lot of 0's, which will behave badly with our Ranking parameters
                                   && GetAnswerFromUser ( "Ranking input RIS data (recommended for more robust results)?", CentroidTitle, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ranking is useless here if ranking later
//bool              toabszscore     = allris && ! ranking;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // thresholding norm RIS
bool                thresholding    = tg.allris 
//                                 && IsPositive ( datatype )           // "vectorial" ranking is allowed
                                   && centroidflag != MedianCentroid    // Median can produce a lot of 0's, which will behave badly with our Ranking parameters
                                   && ranking
                                   && GetAnswerFromUser ( "Thresholding input RIS data?", CentroidTitle, this );
double              threshold       = 0;

if ( thresholding )
    GetValueFromUser ( "Threshold value:", CentroidTitle, threshold, "0.90", this );

if ( thresholding && threshold == 0 )
    thresholding    = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // default is to normalize, but this is also exclusive with ranking
bool                normalize       = ! ranking;
//                                 && ! IsVector ( datatype );  // allow to normalize per 3D vector


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // then ask for some petty details
                                   
PolarityType        polarity        = ! IsPositive ( datatype )     // EEG or vectorial RIS are OK
                                   && GetAnswerFromUser ( "Ignore polarity?", CentroidTitle, this ) ? PolarityEvaluate : PolarityDirect;


ReferenceType       processingref   = tg.alleeg ? ReferenceAverage : ReferenceNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SpatialFilterType   spatialfilter   = ( tg.alleeg                     // ESI has no Spatial filtering
                                     && GetAnswerFromUser ( "Applying Spatial filtering?", CentroidTitle, this ) ) ? SpatialFilterDefault : SpatialFilterNone;

TFileName           xyzfile;

if ( spatialfilter != SpatialFilterNone ) {

    GetFileFromUser     getxyzfile ( "", AllCoordinatesFilesFilter, 1, GetFileRead );

    if ( ! getxyzfile.Execute () || (int) getxyzfile < 1 )
        spatialfilter   = SpatialFilterNone;
    else 
        StringCopy ( xyzfile, getxyzfile[ 0 ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ( BatchProcessingPriority );

ComputeCentroidFiles    (   gof,                OneFileOneCentroid,
                            centroidflag,
                            datatype,
                            polarity,
                            processingref,
//                          toabszscore,
                            ranking,
                            thresholding,       threshold,
                            normalize,
                            spatialfilter,      xyzfile,
                            0,
                            true
                        );

SetProcessPriority ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
