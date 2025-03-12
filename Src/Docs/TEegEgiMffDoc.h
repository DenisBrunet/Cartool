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

                                        // .bin file structure:
//      TEegEgi_Mff_Bin_HeaderFixed
// ns * TEegEgi_Mff_Bin_HeaderVariable1
// ns * TEegEgi_Mff_Bin_HeaderVariable2
//      TEegEgi_Mff_Bin_HeaderOptionalLength
//      TEegEgi_Mff_Bin_HeaderOptional - optional
//      1 Block of data
// or, repeating last block infos:
//      (UINT) 0
//      1 Block of data

                                        // Note: documentation does not specify if these are signed or unsigned integers, but just their sizes in byte
struct  TEegEgi_Mff_Bin_HeaderFixed
{
    INT32           Version;
    INT32           HeaderSize;
    INT32           DataBlockSize;
    INT32           NumberSignals;
};                                      // 4 * 4 = 16 bytes

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // offset of each track within each block
using   TEegEgi_Mff_Bin_HeaderVariable1         = INT32;
                                        // 4 bytes

                                        // info for each track
struct  TEegEgi_Mff_Bin_HeaderVariable2
{
    UINT8           BitsPerTracks;
    UINT8           SignalFrequency[ 3 ];// yes, here is a 3 bytes integer for you sir

                                        // converting the 3 bytes unsigned integer into a 4 bytes unsigned integer
    INT32           GetTrackFrequency ()    { INT32 f = 0; *((UINT8 *) &f) = SignalFrequency[ 0 ]; *((UINT8 *) &f + 1) = SignalFrequency[ 1 ]; *((UINT8 *) &f + 2) = SignalFrequency[ 2 ]; return f; }
};                                      // 4 bytes

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using   TEegEgi_Mff_Bin_HeaderOptionalLength    = INT32;
                                        // 4 bytes


struct  TEegEgi_Mff_Bin_HeaderOptional
{
    INT32           EGIType;
    INT64           NumberOfBlocks;
    INT64           NumberOfSamples;
    INT32           NumberDistinctSignals;
};                                      // 2 * 4 + 2 * 8 = 24 bytes


EndBytePacking

//----------------------------------------------------------------------------
                                        // Cartool structures
                                        // Number of blocks, block size and number of time frames all fit in INT32
class   TEegEgi_Mff_Bin_Channel
{
public:
                    TEegEgi_Mff_Bin_Channel ()   { SamplesPerBlock = ChannelSize = BytesPerSample = 0; }


    INT32           SamplesPerBlock;    // # of samples for this channel
    INT32           ChannelSize;        // size in bytes
    INT32           BytesPerSample;     // currently only 4
};
                                        // Each block can have a different size (!),
                                        // and each track of the block, too, can have different sizes
class   TEegEgi_Mff_Bin_Block
{
public:
    INT64           FileOrigin;         // where data starts in file (absolute position)
    INT32           BlockDuration;      // maximum length of this block
    INT32           SamplingFrequency;  // maximum sampling frequency of this block
    TArray1<TEegEgi_Mff_Bin_Channel>    ChannelsSpec;   // every channel info for this block
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Not linked to a specific file, just an internal structure to browse the sequences
class   TEegEgi_Mff_Session
{
public:
    INT32           FirstBlock;
    INT32           LastBlock;
    INT32           NumTimeFrames;
    INT32           SamplingFrequency;
    TDateTime       DateTime;
};


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
    bool            IsOpen          () final            { return FileStream.IsOpen (); }
    bool            Open            ( int mode, const char *path = 0 );


    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;
    bool            UpdateSession   ( int newsession )                                              final;
    void            UpdateTitle     ()                                                              final;

    bool            IsNanoSecondsPrecision ()   const   { return Version == 0; }    // else micro-seconds (!)


protected:

    TVector<float>                      Tracks;
    TVector<double>                     Gains;
    TVector<double>                     Zeros;

    std::vector<TEegEgi_Mff_Session>    Sequences;
    std::vector<TEegEgi_Mff_Bin_Block>  Blocks;
    INT32           NumBlocks;
    INT32           MaxSamplesPerBlock;
    bool            EqualTracks;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;

    void            DateTimeStringToFields          ( const char* datetime, int &year, int &month, int &day, int &hour, int &minute, double &second )   const;
    double          DurationStringToMicroseconds    ( const char* duration )    const;
    void            DurationStringToFields          ( const char* duration, int &sec, int &millisec, int &microsec, int &nanosec )  const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
