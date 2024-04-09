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
#include    "TInverseResults.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking

                                        // 'RI01' type
struct  TRisHeader {
    char            Magic[ 4 ];
    int             NumSolutionPoints;
    int             NumTimeFrames;
    float           SamplingFrequency;  // in Herz
    char            IsInverseScalar;    // 1 if scalar, 0 if vectorial  (value, and not text)
};                                      // size = 17


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TRisDoc :   public  TTracksDoc,     // tracks content
                    public  TInverseResults // can produce Inverse Solution results
{
public:
                    TRisDoc         ( owl::TDocument *parent = 0 );


    bool            Open            ( int mode, const char* path = 0 );
    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const               { return  NumElectrodes > 0; }
    bool            CommitRis       ( bool force = false );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


    const char*     GetInverseTitle ()  const final         { return owl::TFileDocument::GetTitle (); } 

    void            GetInvSol       ( int reg, long tf1, long tf2, TArray1< float         >& inv, TTracksView *eegview, TRois *rois = 0 )     const final;
    void            GetInvSol       ( int reg, long tf1, long tf2, TArray1< TVector3Float >& inv, TTracksView *eegview, TRois *rois = 0 )     const final;


protected:

    TTracks<float>  Tracks;             // NumElectrodes == NumSolPoints here

    int             NumTracks;          // 1 or 3 * NumElectrodes, in the case of vectorial data
    int             TotalTracks;


    bool            SetArrays       ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
