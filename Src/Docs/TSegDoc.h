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
                                        // Used to open .seg or .data documents
class   TSegDoc : public  TTracksDoc
{
public:
                    TSegDoc ( owl::TDocument *parent = 0 );


    bool            Open            ( int mode, const char *path=0 );
    bool            Close           ();
    bool            IsOpen          ()      const   { return  NumElectrodes > 0; }
    bool            CanClose        ();
    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
                                        // overriding virtual functions
    bool            SetArrays       ();
    void            SetReferenceType( ReferenceType, char* = 0, const TStrings* = 0 )   { Reference     = ReferenceAsInFile; }    // always
    bool            CanFilter       ()      const           { return    ExtensionIs ( FILEEXT_DATA ); }
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 );
    void            GetTracks       ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0, AtomType atomtype = AtomTypeUseCurrent, PseudoTracksType pseudotracks = NoPseudoTracks, ReferenceType reference = ReferenceAsInFile, TSelection* referencesel = 0, TRois *rois = 0 );

    int             GetNumFiles     ()      const   { return NumFiles; }
    int             GetNumVars      ()      const   { return NumVars; }
    int             GetNumClusters  ()      const   { return NumClusters; }


protected:

    TTracks<float>  Tracks;
    int             NumClusters;        // of the segmentation
    int             NumFiles;           // number of times repeating infos
    int             NumVars;            // number of data stores for each file

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
