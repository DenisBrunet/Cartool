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

#include    "Geometry.TPoints.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"

#include    "Electrodes.BuildTemplateElectrodes.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define             AveragingXyzTitle           "Averaging Electrodes Coordinates"


//----------------------------------------------------------------------------
                                        // Important:
                                        // All input xyz's should be from the same source (variations should come from the subjects)!
                                        // They also should have the same type of orientation, although some 180 degrees around Z could be compensated for in case of RAS orientation.
void    TCartoolMdiClient::BuildTemplateElectrodesUI ()
{
                                        // show some intro
//ShowMessage (   "You have to give a list of .xyz or .els files to be averaged," NewLine 
//                "and an output .xyz file for the averaged result." NewLine 
//                "You also need to specify which electrodes are Vertex, Front and Back.", 
//                AveragingXyzTitle );

                                        // get files
GetFileFromUser     getfilesin ( "Coordinates files to be averaged:", AllCoordinatesFilesFilter,   1, GetFileMulti );
GetFileFromUser     getfileout ( "Output average file:", "Head Coordinates files|" FILEFILTER_XYZ, 1, GetFileWrite );


if ( ! getfilesin.Execute () )
    return;


if ( (int) getfilesin <= 0 ) {
//  ShowMessage ( "Yeah man, give me at least 2 files!" NewLine "sigh...", AveragingXyzTitle, ShowMessageWarning );
    return;
    }


if ( ! getfileout.Execute () ) {
//  ShowMessage ( "Problem with the output file...", AveragingXyzTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compatibility check

if ( ! ((TGoF*) getfilesin)->AllExtensionsAre ( AllCoordinatesFilesExt ) ) {
    ShowMessage ( "Please provide some legal Electrodes Coordinates data!", AveragingXyzTitle, ShowMessageWarning );
    return;
    }


ElectrodesCompatibleClass   CompatElectrodes;

((TGoF*) getfilesin)->AllElectrodesAreCompatible ( CompatElectrodes );


if      ( CompatElectrodes.NumElectrodes == CompatibilityNotConsistent ) {
    ShowMessage ( "Electrodes Coordinates don't seem to have the same number of electrodes!", AveragingXyzTitle, ShowMessageWarning );
    return;
    }
else if ( CompatElectrodes.NumElectrodes == CompatibilityIrrelevant ) {
    ShowMessage ( "Electrodes Coordinates don't seem to have any electrodes at all!", AveragingXyzTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStrings            xyznames;

                                        // !We currently do not allow a single file with a null electrode!
for ( int i = 0; i < (int) getfilesin; i++ ) {

    TPoints         xyzpoints ( getfilesin[ i ], i ? 0 : &xyznames );

    for ( int p = 0; p < (int) xyzpoints; p++ ) {

        if ( xyzpoints[ p ].IsNull () ) {

            ShowMessage (   "This file seems to have undefined / null positions electrodes!" NewLine 
                            "Sorry about that, but check your files and come back...", 
                            getfilesin[ i ], ShowMessageWarning );

            return;
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get reference points
char                inputcz [ 256 ];
char                inputfpz[ 256 ];
char                inputoz [ 256 ];

ClearString ( inputcz  );
ClearString ( inputfpz );
ClearString ( inputoz  );

                                        // EGI 257: Ref; 26; 125 138
if ( ! GetInputFromUser ( "Which electrodes are Cz / Vertex (provide multiple names to get an average):", "Cz / Vertex", inputcz , "" ) )    return;
if ( ! GetInputFromUser ( "Which electrodes are Fpz / Front (provide multiple names to get an average):", "Fpz / Front", inputfpz, "" ) )    return;
if ( ! GetInputFromUser ( "Which electrodes are Oz / Back (provide multiple names to get an average):",   "Oz / Back",   inputoz,  "" ) )    return;

                                        // split into tokens & analyse
TSplitStrings       czsplit  ( inputcz , UniqueStrings );
TSplitStrings       fpzsplit ( inputfpz, UniqueStrings );
TSplitStrings       ozsplit  ( inputoz , UniqueStrings );
int                 numcz           = (int) czsplit;
int                 numfpz          = (int) fpzsplit;
int                 numoz           = (int) ozsplit;
TArray1<int>        czindex     ( numcz  );
TArray1<int>        fpzindex    ( numfpz );
TArray1<int>        ozindex     ( numoz  );
bool                czok;
bool                fpzok;
bool                ozok;

                                        // check # of ref
if ( numcz <= 0 || numfpz <= 0 || numoz <= 0 ) {

    ShowMessage ( "Some of your reference electrodes were empty!", AveragingXyzTitle, ShowMessageWarning );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // analyse & check each reference point
czok                = numcz;

for ( int i = 0; i < numcz; i++ ) {
    czindex [ i ]   = xyznames.GetIndex ( czsplit [ i ] );
    czok           &= czindex [ i ] >= 0;
    }


fpzok               = numfpz;

for ( int i = 0; i < numfpz; i++ ) {
    fpzindex [ i ]  = xyznames.GetIndex ( fpzsplit[ i ] );
    fpzok          &= fpzindex[ i ] >= 0;
    }


ozok                = numoz;

for ( int i = 0; i < numoz; i++ ) {
    ozindex [ i ]   = xyznames.GetIndex ( ozsplit [ i ] );
    ozok           &= ozindex [ i ] >= 0;
    }

                                        // are the given reference OK?
if ( ! ( czok && fpzok && ozok ) ) {
    ShowMessage ( "The provided electrode names do not all match!" NewLine "Check the exact names from electrodes file and come back..." , AveragingXyzTitle, ShowMessageWarning );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this extra paramter is not (yet) offered to the user
bool                centercz        = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

BuildTemplateElectrodes (   getfilesin,
                            czindex,
                            fpzindex,
                            ozindex,
                            centercz,
                            getfileout
                        );

                                        // complimentary opening - assumes user gave a genuine <something>.xyz file name...
CartoolObjects.CartoolDocManager->OpenDoc ( getfileout, dtOpenOptions );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}





