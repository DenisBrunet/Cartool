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

constexpr char*     EEG128DOC_FILEVERSION       = "2.0";
constexpr int       EEG128DOC_TFPERRECORD       = 128;
constexpr int       EEG128DOC_NUMCHANNELS       = 128;
constexpr double    EEG128DOC_SAMPLFREQUENCY    = 200;
constexpr double    EEG128DOC_ZERO              = 128;
constexpr double    EEG128DOC_SCALE             =  15;
constexpr char*     EEG128DOC_TAGNAME           = "event";
constexpr int       EEG128DOC_BPCHANNEL         = 127;

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


struct  TModeSet                        /* for tagging type of disk-save */
{
    UINT16          keybd_sav   : 1;
    UINT16          pbtn_sav    : 1;
    UINT16          autosz_sav  : 1;
    UINT16          dummy0      : 1;
    UINT16          dummy1      : 1;
    UINT16          dummy2      : 1;
    UINT16          dummy3      : 1;
    UINT16          dummy4      : 1;
    UINT16          dummy5      : 1;
};


struct  TDate128
{
    short           da_year;
    char            da_day;
    char            da_mon;
};


struct  TTimeVal
{
    UCHAR           hundredths;
    UCHAR           seconds;
    UCHAR           minutes;
    UCHAR           hours;
};


struct  T128FileRec                     /* records to be written or read from file */
{
	TTimeVal        time;               /* 4 bytes */
	TDate128        date;               /* 4 bytes */
	TModeSet        mode;               /* 2 bytes */
	short           gain;               /* 2 bytes    X1, X2, X4 eeg signal gain factors */
	short           id;                 /* 2 bytes */
	short           pbutt_marker;       /* 2 bytes, boolean */
	char            d1, d2, d3, d4;     /* 4 bytes    ='s  20 bytes  */
	UCHAR           data[ EEG128DOC_TFPERRECORD ][ EEG128DOC_NUMCHANNELS ]; /* eeg data */
};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Beth Israel Deaconess Medical Center (BIDMC) file format
class   TEegBIDMC128Doc :   public  TTracksDoc
{
public:
                    TEegBIDMC128Doc (owl::TDocument *parent = 0);

                   
    bool            CanClose    ();
    bool            Close       ();
    bool            IsOpen      ()  const       { return NumElectrodes > 0; }
    bool            Open        ( int mode, const char *path = 0 );
    static bool     ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer );

                                            // overriding virtual functions
    bool            SetArrays ();
    void            ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 );

protected:

    owl::TInStream* InputStream;

    long            DataOrg;

    T128FileRec     FileBuff;
    int             Block;
    int             Offset;
    TSelection      ValidElectrodes;
    int             MarkerIndex;
                                        // virtual TMarkers
    void            ReadNativeMarkers   ();
    inline void     TfToBlockOffset     ( const int& tf, int& block, int& offset )  const;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
