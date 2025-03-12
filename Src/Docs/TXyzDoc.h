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

#include    "TElectrodesDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TXyzDoc :   public TElectrodesDoc
{
public:
                    TXyzDoc ( owl::TDocument *parent = 0 );


    bool            Close	        ();
    bool            Commit	        ( bool force = false );
    bool            Revert	        ( bool force = false );
    bool            IsOpen	        ()  final                   { return  NumElectrodes > 0; }
    bool            Open 	        ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ExtractToFile   ( const char* xyzfile, TSelection selection, bool removeselection )   const final;


protected:

    void            SetArrays       ( int numclusters, int numelectrodes );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
