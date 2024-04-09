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
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking

                                        // .bin files
struct  TEegEgi_Mff_Bin_HeaderFixed
{
    UINT            Version;
    UINT            HeaderSize;
    UINT            DataSize;
    UINT            NumTracks;
};                                      // 4 * 4 = 16 bytes


struct  TEegEgi_Mff_Bin_HeaderVariable1
{
    UINT            Offset;             // of each track, after the header
};                                      // 4 bytes


struct  TEegEgi_Mff_Bin_HeaderVariable2
{
    UCHAR           BitsPerTracks;
    UCHAR           TrackFrequency[ 3 ];   // yes, integer on 3 bytes

                                        // insert the 3 bytes unsigned integer into a 4 bytes unsigned integer
    UINT            GetTrackFrequency ()    { UINT f = 0; *((char *) &f) = TrackFrequency[ 0 ]; *((char *) &f + 1) = TrackFrequency[ 1 ]; *((char *) &f + 2) = TrackFrequency[ 2 ]; return f; }
};                                      // 4 bytes


struct  TEegEgi_Mff_Bin_HeaderOptional
{
    UINT            Type;
    ULONGLONG       NumberOfBlocks;
    ULONGLONG       TotalNumberOfSamples;
    UINT            NumTracks;
};                                      // 2 * 4 + 2 * 8 = 24 bytes


class   TEegEgi_Mff_Bin_Channel
{
public:
                    TEegEgi_Mff_Bin_Channel ()   { SamplesPerBlock = ChannelSize = BytesPerSample = 0; }


    ULONG           SamplesPerBlock;    // # of samples for this channel
    UINT            ChannelSize;        // size in bytes
    UINT            BytesPerSample;     // 4...
};
                                        // Each block can have a different size (!),
                                        // and each track of the block, too, can have different sizes
class   TEegEgi_Mff_Bin_Block
{
public:
    ULONG               FileOrigin;         // where data starts in file (absolute position)
    ULONG               TimeFrameOrigin;    // what is the absolute first time frame of this block
    ULONG               NumTimeFrames;      // maximum length of this block
    UINT                SamplingFrequency;  // maximum sampling frequency of this block
    std::vector<TEegEgi_Mff_Bin_Channel>    ChannelsSpec;   // every channel info for this block
};

                                        // Not linked to a specific file, just an internal structure to browse the sequences
class   TEegEgi_Mff_Session
{
public:
    ULONG           FirstBlock;
    ULONG           LastBlock;
    ULONG           NumTimeFrames;
    UINT            SamplingFrequency;
    TDateTime       DateTime;
};


EndBytePacking

//----------------------------------------------------------------------------
                                        // The latest iteration of file formats from EGI
                                        // This is finally an open format, based on XML
                                        // It has strong similarities with the .nsr format,
                                        // as it it resembles more of a database than a continuous recording
class   TEegEgiMffDoc   :   public  TTracksDoc
{
public:
                    TEegEgiMffDoc   ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const           { return NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );


    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;
    bool            UpdateSession   ( int newsession )                                              final;
    void            UpdateTitle     ()                                                              final;

    bool            IsNanoSecondsPrecision ()       { return Version == 0; }    // else micro-seconds (!)


protected:

    owl::TInStream* InputStream;

    std::vector<UCHAR>                  Tracks;
    std::vector<double>                 Gains;
    std::vector<double>                 Zeros;

    std::vector<TEegEgi_Mff_Session>    Sequences;
    std::vector<TEegEgi_Mff_Bin_Block>  Blocks;
    ULONG           NumBlocks;
    ULONG           MaxSamplesPerBlock;
    bool            EqualTracks;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;

    void            DateTimeStringToFields          ( char *datetime, int &year, int &month, int &day, int &hour, int &minute, double &second );
    double          DurationStringToMicroseconds    ( char *duration );
    void            DurationStringToFields          ( char *duration, int &sec, int &millisec, int &microsec, int &nanosec );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
