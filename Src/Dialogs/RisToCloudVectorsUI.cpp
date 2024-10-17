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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Files.TGoF.h"
#include    "Files.Conversions.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TCartoolMdiClient::RisToCloudVectorsUI ()
{
GetFileFromUser     files ( "RIS files to convert to clouds of vectors:", AllRisFilesFilter, 1, GetFileMulti );

if ( ! files.Execute () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check first the last group's own coherence, as tracks files
TracksCompatibleClass       CompatRis;

( (const TGoF&) files).AllTracksAreCompatible ( CompatRis );

                                        // We allow files with different dimensions, as long they contain the given SP names
if      ( CompatRis.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any tracks at all!" NewLine 
                    "Check again your input files...", 
                    RisToPointsTitle, ShowMessageWarning );
    return;
    }


if      ( CompatRis.NumTF == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any samples or time at all!" NewLine 
                    "Check again your input files...", 
                    RisToPointsTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Group is coherent regarding file types? (doing this first, maybe?)
TracksGroupClass        tg;

if ( ( (const TGoF&) files).AnyTracksGroup ( tg ) ) {

    if      (   tg.noneofthese  ) { 
        ShowMessage (   "Oopsie, files don't really look like tracks,"  NewLine 
                        "are you trying to fool me?!"                   NewLine 
                        "Check again your input files...", 
                        RisToPointsTitle, ShowMessageWarning );   
        return; 
        }
    else if ( ! tg.allrisv      ) { 
        ShowMessage (   "Files don't seem to be consistently of the vectorial RIS type!" NewLine 
                        "Check again your input files...", 
                        RisToPointsTitle, ShowMessageWarning );
        return; 
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All time points
int                 tfmin           = -1;
int                 tfmax           = -1;

char                savingsp[ KiloByte ];
GetInputFromUser ( "Which Solution Points to save (maybe not all of them...)?", RisToPointsTitle, savingsp, "*", this );

bool                spontaneous     = GetAnswerFromUser ( "Spontaneous data?", RisToPointsTitle, this );
bool                normalize       = GetAnswerFromUser ( "Normalizing points (spherical vs. real cloud)?", RisToPointsTitle, this );

TGoF                outgof;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set           ( RisToPointsTitle, SuperGaugeLevelInter );
Gauge.AddPart       ( 0, (int) files );

bool                openingfiles    = (int) outgof * (int) files <= 10;

//if ( (int) files > 5 )
//
//    WindowMinimize ( CartoolMainWindow );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int eegi = 0; eegi < (int) files; eegi++ ) {

    CartoolApplication->SetMainTitle ( "Converting", files[ eegi ], Gauge );

    Gauge.Next ( 0, SuperGaugeUpdateTitle );


    RisToCloudVectors   (
                        files[ eegi ],
                        tfmin,                  tfmax,
                        0,                      savingsp,
                        spontaneous,            normalize,
                        outgof,
                        true
                        );


    if ( openingfiles )
        outgof.OpenFiles ();
    } // for eegi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

WindowMaximize ( CartoolMainWindow );

Gauge.FinishParts ();

CartoolApplication->SetMainTitle ( RisToPointsTitle, "", Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
