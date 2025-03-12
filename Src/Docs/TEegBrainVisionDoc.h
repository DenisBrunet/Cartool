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
                                        // recognized data types
enum        BrainVisionDataType
            {      
            BVTypeNone      = 0x00,

            BVTypeFloat32   = 0x01,
            BVTypeInt16     = 0x02,
            BVTypeUint16    = 0x04,
            BVTypeBinMask   = BVTypeFloat32 | BVTypeInt16 | BVTypeUint16,

            BVTypeAscii     = 0x10,
            BVTypeAscMask   = BVTypeAscii,
            };

                                        // known header parts
enum        BrainVisionHeaderPart
            {      
            NoInfos,
            CommonInfos,
            BinaryInfos,
            AsciiInfos,
            ChannelInfos,
            Coordinates,
            MarkerInfos,
            Comment,
            };


#define     BrainVisionBinaryString         "BINARY"
#define     BrainVisionAsciiString          "ASCII"
#define     BrainVisionFloat32String        "IEEE_FLOAT_32"
#define     BrainVisionInt16String          "INT_16"
#define     BrainVisionUint16String         "UINT_16"
#define     BrainVisionMultiplexedString    "MULTIPLEXED"
#define     BrainVisionVectorizedString     "VECTORIZED"

                                        // some files have a bug and no space between Brain and Vision...
                                        // and the .vhdr has no ",", while the .vmrk has...
constexpr char* BrainVisionGrepHeaderVhdr   = "Brain ?Vision Data Exchange Header File,? Version [0-9]\\.[0-9]";
constexpr char* BrainVisionGrepHeaderVmrk   = "Brain ?Vision Data Exchange Marker File,? Version [0-9]\\.[0-9]";


//----------------------------------------------------------------------------

class   TEegBrainVisionDoc  :   public  TTracksDoc
{
public:
                    TEegBrainVisionDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  final           { return InputStream.is_open (); }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


protected:

    ifstream        InputStream;

    bool            Multiplexed;        // yep, data can be either multiplexed or vectorized
    int             AscSkipLines;
    int             AscSkipColumns;

    BrainVisionDataType DataType;       // recognized data types
    int                 BinTypeSize;    // binary case: size of an atomic data
    LONGLONG            BuffSize;       // binary case: size of one "line" of data (channels or TFs), ascii case: all data

    TArray1<char>   Tracks;
    TArray1<double> Gain;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
