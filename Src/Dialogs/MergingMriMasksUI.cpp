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

#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"

#include    "ESI.TissuesThicknesses.h"

#include    "MergingMriMasks.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TCartoolMdiClient::MergingMriMasksUI ()
{
                                        // Getting all files
static GetFileFromUser  getfilehead     ( "Full head Mask (mandatory):",                    AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfileskull    ( "Full skull Mask (mandatory):",                   AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfileskullsp  ( "Spongy skull Sub-Mask (optional):",              AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfilecsf      ( "CSF Probabilities / Mask (mandatory):",          AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfilegrey     ( "Grey Matter Probabilities / Mask (mandatory):",  AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfilewhite    ( "White Matter Probabilities / Mask (mandatory):", AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfileblood    ( "Venous sinus Mask (optional):",                  AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfileair      ( "Air sinus Mask (optional):",                     AllMriFilesFilter, 1, GetFileRead );
static GetFileFromUser  getfileeyes     ( "Eyes Mask (optional):",                          AllMriFilesFilter, 1, GetFileRead );

                                        // !head should be a global mask that wraps all other tissues!
ShowMessage (   "Warning:"                                              NewLine 
                NewLine 
                "Some input volumes should be MASKS,"                   NewLine
                "while others should be PROBABILITIES in range [0..1]," NewLine
                "although masks could be accepted and taken as 0/1 probabilities.",
                "Merging Masks", ShowMessageWarning );

                                        // Mandatory volumes
if ( ! getfilehead      .Execute () )   return;
if ( ! getfileskull     .Execute () )   return;
if ( ! getfilecsf       .Execute () )   return;
if ( ! getfilegrey      .Execute () )   return;
if ( ! getfilewhite     .Execute () )   return;
                                        // Optional volumes
       getfileskullsp   .Execute ();
       getfileblood     .Execute ();
       getfileair       .Execute ();
       getfileeyes      .Execute ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // If user didn't give the spongy, maybe we can create it?
//bool              createspongy        = (bool) fileskullsp;
bool                createspongy        = ! (bool) getfileskullsp ? GetAnswerFromUser ( "Automatically creating the skull spongy part?", MergingMriMasksTitle, this ) : false;
double              compactthickness    = 0;

if ( createspongy ) {

    GetValueFromUser ( "Keeping a compact table thickness of [mm]:", MergingMriMasksTitle, compactthickness, FloatToString ( SkullCompactThickness, 1 ), this );

    if ( compactthickness <= 0 ) {
        createspongy        = false;
        compactthickness    = 0;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MergingMriMasks (
                getfilehead,
                getfileskull,
                getfileskullsp,
                getfilecsf,
                getfilegrey,
                getfilewhite,
                getfileblood,
                getfileair,
                getfileeyes,
                createspongy,       compactthickness
                );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
