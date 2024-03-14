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

#include    "Math.Random.h"
#include    "Files.Utils.h"
#include    "Dialogs.Input.h"

#include    "TMaps.h"

#include    "GenerateRandomData.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     GenerateRandomDataTitle     = "Generating Random Data";


void    TCartoolMdiClient::GenerateRandomDataUI ()
{
int                 numel;
double              fileduration;
double              samplingfrequency;
TFileName           basefilename;
TFileName           filename;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GetValueFromUser ( "Number of tracks:", GenerateRandomDataTitle, numel, "", this );

if ( numel <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GetValueFromUser ( "File duration:", GenerateRandomDataTitle, fileduration, "1000", this );

if ( fileduration <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // currently not asking
samplingfrequency   = 1000;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GetFileFromUser     getbasefile ( "Output directory:", AllFilesFilter, 1, GetFileDirectory );

if ( ! getbasefile.Execute () || (int) getbasefile < 1 )
    return;
else 
    StringCopy ( basefilename, getbasefile[ 0 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GenerateRandomData  (   numel, 
                        fileduration,   samplingfrequency, 
                        basefilename,   filename
                    );

filename.Open ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
