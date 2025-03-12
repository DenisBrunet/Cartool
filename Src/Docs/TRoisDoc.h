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

#include    "TBaseDoc.h"
#include    "TRois.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Basically encapsulates a TRois
class	TRoisDoc    :   public  TBaseDoc
{
public:
                    TRoisDoc ( owl::TDocument *parent = 0 );
                   ~TRoisDoc ();

                                        // owl::TDocument
    bool            InitDoc ();
    static bool     ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer );
    bool            Open 	( int mode, const char* path = 0 );
    bool            Close	();
    bool            IsOpen	()  final               { return  ROIs; }
    bool            Commit	( bool force = false );
    bool            Revert	( bool force = false );


    TRois*          ROIs;


protected:
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
