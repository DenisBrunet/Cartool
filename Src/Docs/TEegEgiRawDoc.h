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

struct  TEegEgiRawHeader
{
    long            Version;
    short           Year;
    short           Month;
    short           Day;
    short           Hour;
    short           Minute;
    short           Second;
    long            Millisecond;
    short           SamplingRate;
    short           Channels;
    short           BoardGain;
    short           Bits;
    short           Range;
    long            Samples;
    short           Events;
};                                      // 3 * 4 + 12 * 2 = 36 bytes

EndBytePacking

//----------------------------------------------------------------------------
                                        // EGI intermediate files, converted from the .nsr recording files
                                        // It is recommended to use the latter files, format reading might not be up-to-date
class   TEegEgiRawDoc   :   public  TTracksDoc
{
public:
                    TEegEgiRawDoc   ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const           { return NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


protected:
    owl::TInStream*     InputStream;

    long                DataOrg;

    int                 NumElectrodesInFile;
    std::vector<float>  Tracks;
    std::vector<char>   FileBuff;
    int                 BuffSize;
    int                 NumEvents;
    double              ScaleData;
    double              ScaleDataCalibrated;
    std::vector<double> Gains;
    std::vector<double> Zeros;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
