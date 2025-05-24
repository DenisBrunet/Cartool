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

#pragma once

#include    "CartoolTypes.h"
#include    "Files.TFileName.h"

#include    "Math.Armadillo.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define             AllWriteLeadFieldFilesExt   FILEEXT_LF " " FILEEXT_RIS " " FILEEXT_EEGEP " " FILEEXT_EEGSEF " " FILEEXT_BIN " " FILEEXT_TXT


enum                WriteLeadFieldOptions
                    {
                    ComponentsAutomatic,
                    ComponentsSplit,
                    ComponentsNorm
                    };

template <class>    class   TArray2;

                                        // Right now, this class acts more like a wrapper for reading
                                        // and writing Lead Field files. It doesn't really perform
                                        // any mathematical functions, nor it is used for display.
class   TLeadField  :   public  TDataFormat
{
public:
                    TLeadField ();
                    TLeadField ( char *file );
                   ~TLeadField ();


    bool            IsOpen              ()      const   { return  Dim1 != 0; }

    void            Reset               ();
    bool            Set                 ( char* file );

    int             GetDim1             ()      const   { return  Dim1; }
    int             GetDim2             ()      const   { return  Dim2; }
    size_t          AtomSize            ()      const   { return  OriginalAtomType == AtomTypeVector ? 3 : 1; }

    int             GetNumElectrodes    ()      const   { return  Dim1; }
    int             GetNumSolutionPoints()      const   { return  Dim2; }

    bool            ReadFile            ( AMatrix&        K );  // Armadillo format
    bool            ReadFile            ( TArray2<float>& K );  // Cartool format
    static bool     WriteFile           ( const AMatrix& K, const char* file, WriteLeadFieldOptions option = ComponentsAutomatic );

                                        // !copy constructor & assignation not implemented!

protected:
    TFileName       FilePath;
    int             Dim1;
    int             Dim2;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
