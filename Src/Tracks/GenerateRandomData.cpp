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

#include    "GenerateRandomData.h"

#include    "Files.Utils.h"
#include    "TMaps.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Basically a wrapper around TMaps.Random
void    GenerateRandomData      (   int                 numel,
                                    int                 fileduration,       double          samplingfrequency,
                                    const char*         basefilename,       char*           outputfile
                                )
{
                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseFileName;
TFileName           fileoutprefix;
TFileName           buff;
TFileName           filename;


StringCopy      ( BaseDir,          basefilename                            );

StringCopy      ( fileoutprefix,    ToFileName ( basefilename ) );

StringCopy      ( BaseFileName,     BaseDir,     "\\",   fileoutprefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMaps               eegmaps    ( fileduration, numel    );

eegmaps.Random ( -1.0, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

StringCopy          ( filename,     BaseFileName );
StringAppend        ( filename,     "." "White Noise" );
AddExtension        ( filename,     FILEEXT_EEGSEF );

//CheckNoOverwrite    ( filename );

eegmaps.WriteFile   ( filename, false, samplingfrequency );

if ( outputfile )
    StringCopy ( outputfile, filename );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
