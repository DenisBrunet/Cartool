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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Strings.TFixedString.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TElectrodesDoc.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     ElecDownTitle           = "Electrodes Downsampling";


void    TCartoolMdiClient::DownsamplingElectrodesUI ()
{
                                        // try to use all opened XYZ docs, which can come from various directories
TGoF                xyzfiles;

CartoolObjects.CartoolDocManager->GetDocs ( xyzfiles, AllCoordinatesFilesExt );


if ( xyzfiles.IsNotEmpty () ) {
                                        // just give some warning, though    
    ShowMessage (   "Downsampling will be applied to all currently opened electrodes files!" NewLine 
                    "If this is not what you intend to do, close all files or run a separate Cartool...", 
                    ElecDownTitle, ShowMessageNormal, this );
    }

else {
                                        // if no XYZ files were already opened, ask user
    GetFileFromUser     getxyzfiles ( "Electrodes Coordinates files to downsample:", AllCoordinatesFilesFilter, 0, GetFileMulti );

    if ( ! getxyzfiles.Execute () )
        return;

    xyzfiles    = (TGoF&) getxyzfiles;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                answer          = GetOptionFromUser (   "Do you intend to:"         NewLine
                                                            NewLine
                                                            Tab "(K)eep electrodes, or" NewLine
                                                            Tab "(R)emove electrodes?",
                                                            ElecDownTitle, "K R", "" );
if ( answer == EOS )   return;

bool                removeselection = answer == 'R';


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                ellist[ 4 * KiloByte ];

if ( ! GetInputFromUser ( removeselection ? "Electrodes to remove:" : "Electrodes to keep:", ElecDownTitle, ellist, "" ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // UI done
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set       ( ElecDownTitle, SuperGaugeLevelInter );
Gauge.AddPart   ( 0,    (int) xyzfiles + 1,  100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fileout;
//TGoF                filesout;
bool                openauto        = (int) xyzfiles <= 10;


for ( int fi = 0; fi < (int) xyzfiles; fi++ ) {

    Gauge.Next ();


    TOpenDoc<TElectrodesDoc>    XYZDoc ( xyzfiles[ fi ], OpenDocHidden );

    if ( XYZDoc.IsNotOpen () )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can have different number of electrodes across files...
    TSelection              elsel ( XYZDoc->GetNumElectrodes (), OrderArbitrary );

    elsel.Set ( ellist, XYZDoc->GetElectrodesNames (), true );


    StringCopy          ( fileout,  XYZDoc->GetDocPath () );
    RemoveExtension     ( fileout );
    StringAppend        ( fileout,  "." "Down", IntegerToString ( removeselection ? elsel.NumNotSet () : elsel.NumSet () ) );
    AddExtension        ( fileout,  FILEEXT_XYZ );

    CheckNoOverwrite    ( fileout );


    XYZDoc->ExtractToFile ( fileout, elsel, removeselection );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( openauto )
        fileout.Open ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.FinishParts   ();
Gauge.HappyEnd      ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
