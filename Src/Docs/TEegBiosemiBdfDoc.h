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
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TEegBdfChannel
{
public:
                    TEegBdfChannel ()   { SamplesPerBlock = ChannelSize = 0; }

    int             SamplesPerBlock;    // # of samples for this channel
    int             ChannelSize;        // size in bytes
};


EndBytePacking

//----------------------------------------------------------------------------
                                        // Class will handle both BDF and EDF files, as they are nearly identical
enum    EdfType
        {
        UnknownBdfEdf   = 0x0000,

        BiosemiBdf      = 0x0001,

        BiosemiEdf      = 0x0010,
        RegularEdf      = 0x0020,
        EdfPlusCont     = 0x0040,
        EdfPlusDisc     = 0x0080,

        BdfMask         = BiosemiBdf,
        EdfMask         = BiosemiEdf | RegularEdf | EdfPlusCont | EdfPlusDisc,
        };

inline bool     IsBdf       ( EdfType t )   { return IsFlag ( t, BdfMask ); }
inline bool     IsEdf       ( EdfType t )   { return IsFlag ( t, EdfMask ); }
inline int      CellSize    ( EdfType t )   { return IsEdf  ( t ) ? 2 : IsBdf ( t ) ? 3 : 0;  } // BDF is EDF with 3 bytes per sample

                                        // EDF recommended max block size
constexpr int   EdfMaxBlockSize     = 0xF000; // 61440;


inline INT32    Bdf24To32   ( const UCHAR* triplet );


//----------------------------------------------------------------------------

class   TEegBiosemiBdfDoc   :   public  TTracksDoc
{
public:
                    TEegBiosemiBdfDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ()                                  final;
    bool            Close           ()                                  final;
    bool            IsOpen          ()                                  final       { return FileStream.IsOpen (); }
    bool            Open            ( int mode, const char *path = 0 )  final;


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


protected:

    EdfType         FileType;
    int             MaxSamplesPerBlock;
    int             BlockSize;
    TArray1<double> Gains;
    TArray1<double> Offsets;
    TArray1<TEegBdfChannel> ChannelsSampling;

    int             NumElectrodesInFile;
    TArray1<UCHAR>  Block;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;
    int             GetTrailingSize     ();
    static int      GetTrailingSize     ( const char* file, int BlockSize, int MaxSamplesPerBlock, EdfType FileType, const TArray1<TEegBdfChannel>& ChannelsSampling );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
