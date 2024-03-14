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

#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Files.Conversions.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::FilesConversionVrbToTvaUI ()
{
static GetFileFromUser  getfiles ( "", AllVerboseFilesFilter, 1, GetFileMulti );

if ( ! getfiles.Execute () )
    return;


TSuperGauge         Gauge ( "VRB to TVA Conversion", (int) getfiles + 1 );

for ( int i = 0; i < (int) getfiles; i++ ) {

    Gauge.Next ();

    FileConversionVrbToTva ( getfiles[ i ] );
    }

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
