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

#include    <owl/pch.h>

#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "PCA.h"
#include    "ICA.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Common UI to compute either PCA or ICA on a set of files
void    TCartoolMdiClient::PCA_ICA_UI ( owlwparam w )
{
                                        // give a generic title at first
PcaIcaType          processing      = w == CM_PCA ? PcaProcessing
                                    : w == CM_ICA ? IcaProcessing
                                    :               UnknownPcaProcessing;

if ( processing == UnknownPcaProcessing )
    return;


const char*         pcatitle        = PcaIcaTypeString[ processing ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                filenames;


GetFileFromUser     getfiles  ( processing == PcaProcessing ? "Data file(s) to be PCA'ed:" : "Data file(s) to be ICA'ed:",
                                AllErpEegRisFilesFilter,
                                1, GetFileMulti );

if ( ! getfiles.Execute () )
    return;


filenames   = (TGoF&) getfiles;
                                        // no files? get out
if ( filenames.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TracksCompatibleClass   tc;
TracksGroupClass        tg;

                                        // Check first for the new group's own coherence, as tracks files
filenames.AllTracksAreCompatible ( tc );


if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes or tracks at all!", pcatitle, ShowMessageWarning );
    return;
    }


if ( tc.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!", pcatitle, ShowMessageWarning );
    return;
    }

                                        // Group is coherent regarding file types? (doing this first, maybe?)
if ( ! filenames.AnyTracksGroup ( tg ) ) {

    if      (   tg.noneofthese )    ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!", pcatitle, ShowMessageWarning );
    else if ( ! tg.alleeg      )    ShowMessage ( "Files don't seem to be consistently of the EEG type!", pcatitle, ShowMessageWarning );
    else if ( ! tg.allris      )    ShowMessage ( "Files don't seem to be consistently of the RIS type!", pcatitle, ShowMessageWarning );
    else                            ShowMessage ( "Files don't seem to be consistently of the same type!", pcatitle, ShowMessageWarning );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store some of the extracted variables
bool                alleeg          = tg.alleeg;
bool                allris          = tg.allris;
bool                allrisv         = tg.allrisv;

                                        // Here we have a TGoF which has some kind of coherence

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, do some checking on the 2 groups of files

                                        // what about vectorial data?
                                        // correlation somehow works, but there some details to take care (dimensions and stuff), so it's not usable for the moment
if ( allrisv ) {
    ShowMessage ( "Can not run a PCA/ICA on vectorial data!", pcatitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                answer          = GetOptionFromUser ( "Type of PCA/ICA:" NewLine Tab "- (R)egular tracks" NewLine Tab "- (N)ormalized Topographies", 
                                                          pcatitle, "R N", "R", this );

if ( answer == EOS )   return;


bool                datanormalized  = answer == 'N';


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                covrobust       = false; // GetAnswerFromUser ( "Using Robust Covariance (less components)?", pcatitle, this );

bool                saveastracks    = GetAnswerFromUser ( "Do you want to save the Whitened data?", pcatitle, this );

bool                saveaspoints    = GetAnswerFromUser ( "Do you want to see the first components as 3D points?", pcatitle, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AtomType           	datatype        = allris ? AtomTypePositive : AtomTypeScalar;

ReferenceType       dataref         = datanormalized && alleeg ? ReferenceAverage : ReferenceAsInFile;

//bool                ignorepolarity  = false;
                                        // this option is not really proven, and generated a lot more eigenvectors, which is not what we want
//bool                ignorepolarity  = alleeg
//                                   && ! IsAbsolute ( datatype )
//                                   && GetAnswerFromUser ( "Ignoring Polarities (Spontaneous data)?", pcatitle, this );

                                        // Note about centering the data:
                                        // - PCA "requires" to have the data centered in time, per electrode
                                        //   It does indeed give a better 1 & 2 components in case of templates
                                        // - In case of templates, data are centered across electrodes, not time
                                        //   If we do the PCA without the centering, the 1 & 2 components "look" nice and preserved the norm = 1 and center
                                        //   but it looks like the discrimination is less good than with the center across time
//bool                timecentering   = true; // ! datanormalized;
//bool                timecentering   = GetAnswerFromUser ( "Centering data in time (usually yes)?", "PCA Transform" );
//bool                timecentering   = datanormalized ? GetAnswerFromUser ( "Centering template data in time (usually yes)?", "PCA Transform" ) 
//                                                     : true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally clipping beginning and ending time frames?
int                 cliptf          = 0;

//char                buff[ 256 ];
//
//GetInputFromUser ( "Temporal margin removed on each side, specified in [ms], [s] or [TF]:", pcatitle, buff, "5s", this );
//
//cliptf  = Round ( StringToTimeFrame ( buff, tc.SamplingFrequency ) );
//
//Maxed ( cliptf, 0 );

//  DBGV2 ( tc.samplingfrequency, cliptf, buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                prefix[ 256 ];

ClearString ( prefix );

GetInputFromUser ( "Optional output files' prefix:", pcatitle, prefix, "", this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here done asking user
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge     Gauge;
Gauge.Set       ( pcatitle );


Gauge.AddPart   ( gaugepcamain, PCAFileNumGauge * (int) filenames, 50 );  // gaugepcamain
Gauge.AddPart   ( gaugepca,     PCANumGauge     * (int) filenames, 50 );  // gaugepca


if ( (int) filenames >= 5 )
                                        // batch can be long, hide Cartool until we are done
    WindowMinimize ( CartoolMainWindow );
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) filenames; i++ ) {

    PcaIcaFile (    filenames[ i ],
                    datatype,
                    dataref,
                    datanormalized,
                    covrobust,
                    cliptf,
                    processing,
                    true, true, true /*false*/, 
                    saveastracks,
                    saveaspoints,
                    prefix,
                    &Gauge 
                );

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

WindowMaximize ( CartoolMainWindow );

//CartoolApplication->ResetMainTitle ();
Gauge.FinishParts ();
CartoolApplication->SetMainTitle ( pcatitle, "", Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
