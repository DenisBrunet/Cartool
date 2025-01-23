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
                                        // ERPSS raw data files

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


//   byte order indicators
#define BOCANON        ((unsigned short) 0xAA55)        // canonical form
#define BOSWABBED      ((unsigned short) 0x55AA)        // needs byte swabbing


//   multiplexed raw reference header magic number
#define   EIDDRMRAW ((unsigned short) 0x00B0)


//   raw header flag bits
#define RH_F_CONV   0x0001                              // converted from other format
#define RH_F_RCOMP  0x0002                              // raw file is compressed
#define RH_F_DCMAP  0x4000                              // channel mapping used during dig.


//   limits and constants
#define RH_CHANS       256                              // maximum # channels
#define RH_DESC         64                              // max # chars in description
#define RH_CHDESC        8                              // max # chars in channel descriptor
#define RH_SFILL         4                              // short filler
#define RH_LFILL         6                              // long filler
#define RH_ALOG        828                              // max # chars in ASCII log


typedef struct rawhdr {
    unsigned short  rh_byteswab;                        // ERPSS byte swab indicator
    unsigned short  rh_magic;                           // file magic number
    unsigned short  rh_flags;                           // flags
    unsigned short  rh_nchans;                          // # channels
    unsigned short  rh_calsize;                         // (if norm, |cal|)
    unsigned short  rh_res;                             // (if norm, in pts/unit)
    unsigned short  rh_pmod;                            // # times processed
    unsigned short  rh_dvers;                           // dig. program version
    unsigned short  rh_l2recsize;                       // log 2 record size
    unsigned short  rh_recsize;                         // record size in pts
    unsigned short  rh_errdetect;                       // error detection in effect
    unsigned short  rh_chksum;                          // error detection chksum
    unsigned short  rh_tcomp;                           // log 2  time comp/exp
    unsigned short  rh_narbins;                         // (# art. rej. count slots)
    unsigned short  rh_sfill[RH_SFILL];                 // short filler (to 16 slots)
    unsigned short  rh_nrmcs[RH_CHANS];                 // (cal sizes used for norm.)
    long            rh_time;                            // creation time, secs since 1970
    long            rh_speriod;                         // digitization sampling period
    long            rh_lfill[RH_LFILL];                 // long filler (to 8 slots)
    char            rh_chdesc[RH_CHANS][RH_CHDESC];     // chan descriptions
    char            rh_chmap[RH_CHANS];                 // input chan mapping array
    char            rh_subdesc[RH_DESC];                // subject description
    char            rh_expdesc[RH_DESC];                // experimenter description
    char            rh_ename[RH_DESC];                  // experiment name
    char            rh_hname[RH_DESC];                  // host machine name
    char            rh_filedesc[RH_DESC];               // file description
    char            rh_arbdescs[RH_DESC];               // (art. rej. descriptions)
    char            rh_alog[RH_ALOG];                   // ASCII log
}   RAWHDR;


//    Pre-defined event codes.
#define PAUSE_MARK    ((unsigned short) 0xFFFF)
#define DELETE_MARK   ((unsigned short) 0xFFFE)


#define RRH_SIZE        512                             // raw record header size
#define RRH_FILLER       24                             // raw r ecord header filler
#define RRH_EVENTS      110                             // max # events in hdr


//    Raw record header event structure.
//    Size must be 4 bytes.
struct rrhevent {
    unsigned char   rrhe_sampoff;                       // sample offset
    unsigned char   rrhe_ccode;                         // condition code
    unsigned short  rrhe_ecode;                         // event code
};


//    Raw record header flag bits
#define RRH_F_COMPRESSED  01                            // compressed data

//    Raw record magic number.
#define EIDDSMRAW  ( (unsigned short) 0x00F0 )


//    Raw record header for multiplexed short raw data
typedef struct rawrhdr {
    unsigned short  rrh_byteswab;                       // byteswab indicator
    unsigned short  rrh_magic;                          // magic number
    unsigned short  rrh_flags;                          // compressed, ...
    unsigned short  rrh_nchans;                         // # channels
    unsigned short  rrh_l2recsize;                      // log 2 of samples/record
    unsigned short  rrh_recsize;                        // samples/record per chan
    unsigned short  rrh_aderrs;                         // # A/D errs
    unsigned short  rrh_runnum;                         // run #
    unsigned short  rrh_errdetect;                      // error detection flag
    unsigned short  rrh_nevelost;                       // # events lost
    unsigned short  rrh_nevents;                        // # events in array
    unsigned short  rrh_nencshrts;                      // if comp., # encoded shorts
    char            rrh_dummy[RRH_FILLER];              // to 512 bytes
    long            rrh_speriod;                        // sampling period in usec
    long            rrh_recnum;                         // record #
    long            rrh_starttime;                      // time, start of dig.
    long            rrh_runtime;                        // time, last start from pause
    long            rrh_eventn;                         // cumulative # of 1st event in array
    long            rrh_chksum;                         // checksum to 0L for record
    struct rrhevent rrh_events[RRH_EVENTS];             // fixed array of events
} RAWRHDR;

#define RRH_NULLP  ((RAWRHDR *) 0)


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // ERPSS format
class   TEegERPSSRdfDoc :   public  TTracksDoc
{
public:
                    TEegERPSSRdfDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const           { return NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;


protected:

    owl::TInStream* InputStream;

    TArray1<long>   Tracks;
    TArray1<double> Gains;
    bool            Compressed;
    bool            NeedSwap;
    int             TFRecord;
    int             NumRecords;
    TArray1<int>    RecordSize;
    TArray1<int>    RemappingIndex;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;
    void            ProcessTracks       ( long reltf, bool firsttrack );
    inline void     TfToBlockOffset     ( const int& tf, LONGLONG& block, long& offset )     const;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
