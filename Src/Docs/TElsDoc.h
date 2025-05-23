/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "TElectrodesDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TElsDoc :   public TElectrodesDoc
{
public:
                    TElsDoc ( owl::TDocument *parent = 0 );


    bool            Close	        ()                                  final;
    bool            Commit	        ( bool force = false )              final;
    bool            Revert	        ( bool force = false )              final;
    bool            IsOpen	        ()                                  final   { return  NumElectrodes > 0; }
    bool            Open 	        ( int mode, const char *path = 0 )  final;


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );

    void            ExtractToFile   ( const char* xyzfile, TSelection selection, bool removeselection )   const;

protected:
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
