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
#include    "Files.TGoF.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::SplitFreqFilesUI ( owlwparam w )
{
static GetFileFromUser  getfiles ( "Frequency files to be split:", AllFreqFilesFilter, 1, GetFileMulti );

if ( ! getfiles.Execute () )
    return;


if      ( w == CM_SPLITFREQBYFREQUENCY )    ((TGoF*)getfiles)->SplitFreqFiles ( SplitFreqByFrequency );
else if ( w == CM_SPLITFREQBYELECTRODE )    ((TGoF*)getfiles)->SplitFreqFiles ( SplitFreqByElectrode );
else if ( w == CM_SPLITFREQBYTIME      )    ((TGoF*)getfiles)->SplitFreqFiles ( SplitFreqByTime      );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
