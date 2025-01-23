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

#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "Electrodes.ExtractElectrodesFromKrios.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::ExtractElectrodesFromKriosUI ()
{
GetFileFromUser     getcsvfile ( "Provide some Krios scanner CSV files:", "CSV file" "|" FILEFILTER_CSV, 0, GetFileMulti );

if ( ! getcsvfile.Execute () )
    return;


char                namestoskip[ KiloByte ];

GetInputFromUser   ( "Scanner names to ignore:", KriosTitle, namestoskip, "CMS DRL COM GND" );


GetFileFromUser     getxyzfile ( "Provide an optional target xyz file (recommended):", AllCoordinatesFilesFilter, 0, GetFileRead );

getxyzfile.Execute ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set       ( KriosTitle, SuperGaugeLevelInter );
Gauge.AddPart   ( 0,    (int) getcsvfile + 1,  100 );


bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    = (int) getcsvfile == 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fileout;
TGoF                filesout;
bool                openauto        = (int) getcsvfile <= 10;


for ( int fi = 0; fi < (int) getcsvfile; fi++ ) {

    Gauge.Next ();

    if ( ! ExtractElectrodesFromKrios  ( getcsvfile[ fi ], namestoskip, getxyzfile, fileout ) )
        continue;

    if ( openauto )
        fileout.Open ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CartoolApplication->AnimateViews    = oldav;

Gauge.FinishParts   ();
Gauge.HappyEnd      ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
