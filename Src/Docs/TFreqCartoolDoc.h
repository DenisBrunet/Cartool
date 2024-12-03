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

#include    "TFreqDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//                          Frequency File Format
//                          =====================
//
// Cartool Project - Denis Brunet - Functional Brain Mapping Lab, Geneva, Switzerland
//

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


// 1) Header, fixed part:

struct  TFreqHeader
{
    int             Version;                // 'FR01' or 'FR02'
    char            Type[MaxCharFreqType];  // of analysis and data
    int             NumChannels;            // total number of electrodes
    int             NumFrequencies;         // written in file
    int             NumBlocks;              // of repeated analysis, ~ time frames
    double          SamplingFrequency;      // of data in the original file
    double          BlockFrequency;         // frequency of repeated blocks
    short           Year;                   // Date of the recording
    short           Month;                  // (can be 00-00-0000 if unknown)
    short           Day;
    short           Hour;                   // Time of the recording
    short           Minute;                 // (can be 00:00:00:0000 if unknown)
    short           Second;
    short           Millisecond;
};

// 2) Header, variable part:

// Channels'names, a matrix of  NumElectrodes * sizeof ( TFreqChannel )  chars
// To allow an easy calculation of the data origin,
// be aware that each name ALWAYS take 8 bytes in the header, even if the string length is smaller than that.
// The remaining part is padded with bytes set to 0, f.ex.:
// binary values          70 112 122 00 00 00 00 00 65 70 55 00 00 00 00 00 ...
// string equivalence      F   P   z \0              A  F  7 \0

struct  TFreqChannel
{
    char            ChannelName[8];
};


// if Version == 'FR01'
// Frequencies multiple of SamplingFrequency
// NumFrequencies * sizeof ( TFreqFrequency )

struct  TFreqFrequency
{
    double          Frequency;
};


// or, if Version == 'FR02', simply a list of strings

struct  TFreqFrequencyName
{
    char            FrequencyName[16];
};


// 3) Data part:

// Starting at file position:  sizeof ( TFreqHeader ) + NumElectrodes * sizeof ( TFreqChannel ) + NumFrequencies * sizeof ( TFreqFrequency )
// Data are stored as a matrix  NumTimeFrames x NumElectrodes x NumFrequencies
// of either float (Little Endian - PC) for DataType == FFTNorm, FFTNorm2, FFTApproximation
// or complex for DataType == FFTComplex

/*                                        // These could be useful to have direct access in file on disk
                                        // Also would be better as functions...
LONGLONG            DataOrg     = sizeof ( TFreqHeader )
                                + numel  * sizeof ( TFreqChannel       )
                                + numf   * sizeof ( TFreqFrequencyName );

LONGLONG            ElNamesOrg  = sizeof ( TFreqHeader );

LONGLONG            FreqNamesOrg= sizeof ( TFreqHeader )
                                + numel  * sizeof ( TFreqChannel );
#define GetFreqValue(f,el,tf)   ( ifd.seekg ( DataOrg       + ( ( numel * tf + el  ) * numf + f ) * sizeof ( data ), ios::beg ), ifd.read ( (char *) &data, sizeof ( data ) ), data )
#define GetFreqName(f)          ( ifd.seekg ( FreqNamesOrg  + f  * sizeof ( freqname ), ios::beg ),                              ifd.read ( (char *) &freqname, sizeof ( freqname ) ), freqname.FrequencyName[ MaxCharFrequencyName    - 1 ] = 0, freqname.FrequencyName )
#define GetElName(el)           ( ifd.seekg ( ElNamesOrg    + el * sizeof ( elname ),   ios::beg ),                              ifd.read ( (char *) &elname,   sizeof ( elname   ) ), elname.ChannelName    [ sizeof ( TFreqChannel ) - 1 ] = 0, elname.ChannelName )
*/


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Files can legally be either scalar (norm, norm2 or real), or complex (2 x single float)
class   TFreqCartoolDoc :   public  TFreqDoc
{
public:
                    TFreqCartoolDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            Commit          ( bool force = false );
    bool            IsOpen          ()      const           { return  NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float>    &buff,                                  int tfoffset = 0 )  final;
    void            ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &buff,                                  int tfoffset = 0 )  final;
    void            ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &realpart, TSetArray2<float> &imagpart, int tfoffset = 0 )  final;
    double          GetFreqValue    ( long el, long tf, long f )                                                                        final;


protected:

    owl::TInStream* InputStream;

    int             Version;

    TArray1<float>  Spectrum;
    size_t          SpectrumSize;
    size_t          AtomSize;


    bool            SetArrays       ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
