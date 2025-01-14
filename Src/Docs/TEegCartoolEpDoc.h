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

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TEegCartoolEpDoc    :   public  TTracksDoc
{
public:
                    TEegCartoolEpDoc    ( owl::TDocument *parent = 0 );


    bool            CanClose            ();
    bool            IsOpen              ()  const           { return NumElectrodes > 0; }
    bool            Open 	            ( int mode, const char* path = 0 );


    static bool     ReadFromHeader      ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks       ( long tf1, long tf2, TArray2<float>& buff, int tfoffset = 0 )                                  final;
    void            SetReferenceType    ( ReferenceType ref, const char* tracks = 0, const TStrings* elnames = 0, bool verbose = true ) final;


protected:

    TTracks<float>  Tracks;
    bool            CreatingEP;


    bool            SetArrays           ()  override;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
