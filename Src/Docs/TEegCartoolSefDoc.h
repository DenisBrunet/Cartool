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

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//                          Simple Eeg Format
//                          =================
//
// Cartool Project - Denis Brunet - Functional Brain Mapping Lab - HUG Geneva
//
// This format contains the minimum data needed to describe an EEG,
// would it be an ERP or a recording file.
// Triggers are not stored, they are given as an external .mrk file.

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


// 1) Header, fixed part:

struct  TSefHeader
{
    UINT32          Version;                // 'SE01'
    int             NumElectrodes;          // total number of electrodes
    int             NumAuxElectrodes;       //  out of which the last NumAux are auxiliaries
    int             NumTimeFrames;          // time length
    float           SamplingFrequency;      // frequency in Hertz
    short           Year;                   // Date of the recording
    short           Month;                  // (can be 00-00-0000 if unknown)
    short           Day;
    short           Hour;                   // Time of the recording
    short           Minute;                 // (can be 00:00:00:0000 if unknown)
    short           Second;
    short           Millisecond;
};

// 2) Header, variable part:
// the channels'names, a matrix of  NumElectrodes x 8 chars
// To allow an easy calculation of the data origin,
// be aware that each name ALWAYS take 8 bytes in the header, even if the string length is smaller than that.
// The remaining part is padded with bytes set to 0, f.ex.:
// binary values          70 112 122 00 00 00 00 00 65 70 55 00 00 00 00 00 ...
// string equivalence      F   P   z \0              A  F  7 \0

typedef char        TSefChannelName[8];     // 1 electrode name

// 3) Data part:
// Starting at file position:  sizeof ( TSefHeader ) + 8 * NumElectrodes
// Data is stored as a matrix  NumTimeFrames x NumElectrodes  of float (Little Endian - PC)
// Data is written already calibrated, assuming micro-volts.


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class   TEegCartoolSefDoc   :   public  TTracksDoc
{
public:
                    TEegCartoolSefDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  final           { return FileStream.IsOpen (); }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


protected:

    TArray1<float>  Tracks;
    long            BuffSize;


    bool            SetArrays       ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
