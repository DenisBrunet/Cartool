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
                                        // Wrapping all the troubles of reading the native NetStation files into these classes
enum    NsrVersion  {
                    NsrVersion1     = 1,
                    NsrVersion2     = 2,
                    NsrVersion3     = 3,
                    NsrVersion4     = 4
                    };

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking

                                        // defines, reads and swap the data
class   TEegEgiNsrTopHeader
{
public:
                    TEegEgiNsrTopHeader::TEegEgiNsrTopHeader ( std::ifstream* ifs );


    long            Version;            // format on file
    long            ToSessionTime;      // follow this to get the TWLD TEPC time per session info (except the 1st one, see the record 22 instead)
    long            ToMainHeader;
    long            NumMainRecords;
    long            Num;
    long            OneK;

//  bool            IsCalibrated ()     { return NumInfos >= 6; } // was: bytes 5 & 6
};


//----------------------------------------------------------------------------

class   TEegEgiNsrMainHeaderFirst
{
public:
                    TEegEgiNsrMainHeaderFirst::TEegEgiNsrMainHeaderFirst () {}
                    TEegEgiNsrMainHeaderFirst::TEegEgiNsrMainHeaderFirst ( std::ifstream* ifs, long address );


    USHORT          RecordType;
    long            Long1;
    short           NumRecords;
    short           Num;
    short           Num256;
    long            ToRecords;
};


//----------------------------------------------------------------------------
/*
                    Record codes deciphered
                    =======================

Code            Meaning                     Usual value

00              ?                           Slightly Different structure
20              empty?
21              String table of content     Textual values
22              Date and time
23              List of system records?
24              List of session records?
26              Sampling frequency          in Hertz
27              Name of the net
28              Filters used
29              Board gain                  1
30              Scale                       # microVolts per # bits
31              Gain calibration            table
32              Zero calibration            table
33              Montage?
34              New (?) Trigger list
36              Trigger list                always in milliseconds
37              Number of sessions          1..n
37              Session offsets
                Data in blocks
39              Length in blocks and TFs for each session?
*/

                                        // Reading these struct will tell which type of data (RecordCode) it points to
class   TEegEgiNsrRecord
{
public:
    long            RecordCode; // Address1 in a Record0
    long            L1;
    long            L2;
    long            L3;
    long            L4;
    long            Address1;
    long            Code1;
    long            C1;
    long            L5;
    long            Address2;
    long            Code2;
    long            C2;
    long            L6;
    long            Address3;
    long            Code3;
    long            C3;
    long            L7;
    long            Address4;
    long            Code4;
    long            C4;
    long            L8;
    long            Address5;
    long            Code5;
    long            C5;
    long            L9;
    long            Address6;
    long            Code6;
    long            C6;

    void            Read ( std::ifstream* ifs, int type );
};


class   TEegEgiNsrTableCell
{
public:
    long            Id;

    union {
    long            Address;
    long            Data;
    };
};


EndBytePacking

//----------------------------------------------------------------------------
                                        // Wrapping the main header
class   TEegEgiNsrMainHeader
{
public:
                    TEegEgiNsrMainHeader::TEegEgiNsrMainHeader ( std::ifstream* ifs );


    NsrVersion      GetVersion              ()  const           { return  Version; }
    void            GetNetName              ( char *nname );

    bool            IsAtomInteger           ();
    bool            IsAtomFloat             ();
    size_t          GetAtomSize             ()  const           { return AtomSize; }


    int             GetNumFilters           ();
    bool            HasHighpassFilter       ();

    int             GetNumElectrodesInFile  ()  const           { return NumElectrodes; }
    int             GetNumElectrodes        ()  const           { return Version == NsrVersion1 ? NumElectrodes + 1 : NumElectrodes; }
    int             GetNumAuxElectrodes     ();
    void            SetAuxElectrodes        ( TSelection &aux );

    int             GetSamplingFrequency    ();
    bool            GetDateTime             ( TDateTime &dt, int session );

    long            GetSessionOffset        ( int session );
    int             GetNumSessions          ()  const           { return NumSessions; }

    bool            GetDataBlocks           ( std::vector<std::vector<long>>& db, long *tf, unsigned long &blocksize );
    bool            GetGains                ( double *gains, int session );
    bool            GetZeros                ( double *zeros, int session );
    bool            GetBoardGain            ( double &gain );
    bool            GetScale                ( double &muvolts, double &numbits );
    void            GetTriggers             ( std::vector<TMarker>& tl );


protected:

    std::ifstream*  ToFile;
    NsrVersion      Version;        // coded format
    long            Address;
    int             TotalRecords;
    bool            Splitted;
    int             NumSessions;
    int             NumElectrodes;
    size_t          AtomSize;


    TEegEgiNsrMainHeaderFirst   First1;
    TEegEgiNsrMainHeaderFirst   First2;
    TEegEgiNsrRecord            Record;
    int             CurrentRecordIndex;
    long            CurrentRecordCode;

    bool            GetNextRecord       ( int &type, bool reset = false );
    bool            GetRecord           ( int type, long recordcode );
    int             GetNumData          ( long startaddress );
    int             GetData             ( long startaddress, TEegEgiNsrTableCell *list, bool swap = false, int level = 0 );
    int             GetStrings          ( long startaddress, TEegEgiNsrTableCell *list, int level = 0 );
    int             FindNumSessions     ();
    int             FindNumElectrodes   ();
    bool            IsFloatTable        ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A lot of hacking around to be able to open files from old EGI recording machines
                                        // but still better than duplicating files into .raw
                                        // Format is a sort of database, with blocks of data spread randomly on the disk, making it
                                        // very difficult to read. It also has different versions following the development of the
                                        // recording machines.
                                        // Note: recording machines were Apple machines, hence files had no extensions per se
                                        // We gave it the "NSR" (Net Station Recording) extension when copying files from Mac to PC

constexpr int   NSRV2_AUX1              =  49;
constexpr int   NSRV2_AUX2              = 114;
                                        // trigger codes (of max 4 letters) that indicate an event, not a trigger
                                        // built from .nsr files, and seemed to fit exactly to the MFF ones, too
constexpr char* EgiEventCodes           = "SESS CELL bgin TRSP DIN resp fbck";


class   TEegEgiNsrDoc   :   public  TTracksDoc
{
public:
                    TEegEgiNsrDoc   ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const           { return NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );


    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;
    bool            UpdateSession   ( int newsession )                                              final;


protected:

    owl::TInStream*         InputStream;

    std::vector<float>      Tracks;
    std::vector<UCHAR>      FileBuff;
    std::vector<double>     Gains;
    std::vector<double>     Zeros;

    size_t                  AtomSize;
    int                     NumElectrodesInFile;

    std::vector<std::vector<long>>  DataBlocks;
    std::vector<long>               TimeFrames;
    unsigned long           TFPerBlock;
    unsigned long           Block;
    unsigned long           Offset;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;
    bool            SetGainsZeros       ( TEegEgiNsrMainHeader &mainheader, int session );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
