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

#pragma once

#include    "TEegCartoolEpDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // TEegNeuroscanAvgDoc is derived from TEegCartoolEpDoc, as it has all we need already
                                        // IF it happens more files would be EP files, then we could create an TTracksErpDoc class f.ex.
                                        // and derive all from it
class   TEegNeuroscanAvgDoc :   public  TEegCartoolEpDoc
{
public:
                    TEegNeuroscanAvgDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );


protected:

    TArray1<double> Gains;
    TArray1<double> Zeros;


    bool            SetArrays       ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
