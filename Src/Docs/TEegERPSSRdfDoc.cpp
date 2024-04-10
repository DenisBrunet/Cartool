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

#include    <owl/pch.h>

#include    "MemUtil.h"

#include    "TArray1.h"

#include    "TEegERPSSRdfDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegERPSSRdfDoc::TEegERPSSRdfDoc (TDocument *parent)
      : TTracksDoc ( parent )
{
InputStream         = 0;

DataOrg             = 0;
Compressed          = false;
NeedSwap            = false;
NumRecords          = 0;
TFRecord            = 0;

Reference           = ReferenceAsInFile;
}


bool	TEegERPSSRdfDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegERPSSRdfDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose();
}


//----------------------------------------------------------------------------
void    TEegERPSSRdfDoc::ReadNativeMarkers ()
{
                                        // scan the whole file for the triggers
long                tfi;
char                triggername[8];
RAWRHDR             recordheader;
ushort              s;


ifstream    ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // goto first block
ifs.seekg ( DataOrg );


for ( int r=0; r < NumRecords; r++ ) {

    ifs.read ( (char *) &recordheader, sizeof ( recordheader ) );

    SwappedBytes ( recordheader.rrh_nevents, NeedSwap );
                                        // scan events of this block
    for ( int e=0; e < recordheader.rrh_nevents; e++ ) {
                                        // this is not 100% correct to spec, as there can be lost blocks (!!) in between
        tfi     = r * TFRecord + recordheader.rrh_events[ e ].rrhe_sampoff;
        s       = recordheader.rrh_events[ e ].rrhe_ecode;

        SwappedBytes ( s, NeedSwap );

        if      ( s == PAUSE_MARK )     StringCopy  ( triggername, "PAUSE" );
        else if ( s == DELETE_MARK )    StringCopy  ( triggername, "DELETE" );
        else if ( s & 0x8000 )          StringCopy  ( triggername, "META-EVENT" );
        else                            sprintf ( triggername, "%0hu", s );

        AppendMarker ( TMarker ( tfi, tfi, s, triggername, MarkerTypeTrigger ) );
        } // for events

    if ( r < NumRecords - 1 )
        ifs.seekg ( RecordSize[ r ] - sizeof ( RAWRHDR ), ios::cur );
    } // for records
}


//----------------------------------------------------------------------------
bool	TEegERPSSRdfDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


RAWHDR          rawheader;
                                    // read a standard marker
ifs.read ( (char *) &rawheader, sizeof ( rawheader ) );

bool    needswap    = rawheader.rh_byteswab == BOSWABBED;

SwappedBytes ( rawheader.rh_magic,   needswap );
SwappedBytes ( rawheader.rh_nchans,  needswap );
SwappedBytes ( rawheader.rh_speriod, needswap );

if ( rawheader.rh_magic != EIDDRMRAW )
    return  false;


switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)       = rawheader.rh_nchans;
        return  true;

    case ReadNumAuxElectrodes :
        *((int *) answer)       = 0; // unknown from header
        return  true;

    case ReadSamplingFrequency :
        *((double *) answer)    = (double) 1000000 / rawheader.rh_speriod;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegERPSSRdfDoc::Open	(int /*mode*/, const char* path)
{

if ( path )
    SetDocPath ( path );

SetDirty ( false );

if ( GetDocPath() ) {

    RAWHDR          rawheader;

    ifstream    ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // read a standard marker
    ifs.read ( (char *) &rawheader, sizeof ( rawheader ) );

    NeedSwap    = rawheader.rh_byteswab == BOSWABBED;

    SwappedBytes ( rawheader.rh_magic,   NeedSwap );
    SwappedBytes ( rawheader.rh_flags,   NeedSwap );
    SwappedBytes ( rawheader.rh_nchans,  NeedSwap );
    SwappedBytes ( rawheader.rh_calsize, NeedSwap );
    SwappedBytes ( rawheader.rh_res,     NeedSwap );
    SwappedBytes ( rawheader.rh_dvers,   NeedSwap );
    SwappedBytes ( rawheader.rh_recsize, NeedSwap );
    SwappedBytes ( rawheader.rh_time,    NeedSwap );
    SwappedBytes ( rawheader.rh_speriod, NeedSwap );

    if ( rawheader.rh_magic != EIDDRMRAW ) {
        ShowMessage ( "Wrong Magic Number.", "Opening error", ShowMessageWarning );
        return  false;
        }

    Compressed          = rawheader.rh_flags & RH_F_RCOMP;
    NumElectrodes       = rawheader.rh_nchans;
    NumMinElectrodes    = NumElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = (double) 1000000 / rawheader.rh_speriod;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gains.Resize ( NumElectrodes );
    for ( int e=0; e < NumElectrodes; e++ )
        Gains[ e ]  =  rawheader.rh_calsize && rawheader.rh_res ? (double) rawheader.rh_res / rawheader.rh_calsize
                                                                : 100.0 / 2048; // heuristic calibration

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    DataOrg             = ifs.tellg();
    TFRecord            = rawheader.rh_recsize;

                                        // either the real (not compressed) or max buffer size (compressed)
    int BuffSize        = sizeof ( RAWRHDR ) + rawheader.rh_recsize * rawheader.rh_nchans * sizeof ( short );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    RAWRHDR             recordheader;
    NumRecords          = 0;
                                        // count actual number of records
    if ( Compressed ) {

        while ( ! ifs.fail () ) {

            NumRecords++;

            ifs.read ( (char *) &recordheader, sizeof ( recordheader ) );

            SwappedBytes ( recordheader.rrh_nencshrts, NeedSwap );
                                        // skip variable part
            ifs.seekg ( recordheader.rrh_nencshrts * sizeof ( short ), ios::cur );
            }

        NumRecords--;
        }
    else {
        ifs.seekg ( 0, ios::end );

        NumRecords  = ( (ulong) ifs.tellg () - DataOrg + 1 ) / BuffSize;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can now compute the total duration
    NumTimeFrames       = NumRecords * rawheader.rh_recsize;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate the records size array
    RecordSize.Resize ( NumRecords );

                                        // rescan and store
    if ( Compressed ) {

        ifs.close ();
        ifs.open  ( GetDocPath (), ios::binary );
        ifs.seekg ( DataOrg, ios::beg );

        for ( int r = 0; r < NumRecords; r++ ) {

            ifs.read ( (char *) &recordheader, sizeof ( recordheader ) );

            SwappedBytes ( recordheader.rrh_nencshrts, NeedSwap );
                                        // skip the variable part
            ifs.seekg ( recordheader.rrh_nencshrts * sizeof ( short ), ios::cur );
                                        // and store it!
            RecordSize[ r ] = sizeof ( RAWRHDR ) + recordheader.rrh_nencshrts * sizeof ( short );
            }
        }
    else {

        for ( int r=0; r < NumRecords; r++ )
                                            // constant
            RecordSize[ r ] = BuffSize;
        }

    ifs.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                            // time in seconds since 1-1-1970
    TDate   pcstart ( 1, 1970 );

    unsigned long s         = rawheader.rh_time;

    DateTime = TDateTime ( pcstart.Year(), pcstart.Month(), pcstart.DayOfMonth(),
                           0, s / 60, s % 60, 0, 0 );
//                         0, 0, rawheader.rh_time, 0, 0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // channels can be re-mapped!
    if ( rawheader.rh_flags & RH_F_DCMAP ) {

        RemappingIndex.Resize ( NumElectrodes );

        for ( int i=0; i < NumElectrodes; i++ )

            RemappingIndex[ rawheader.rh_chmap[ i ] ] = i; // store the REVERSE mapping
//          RemappingIndex[ i ] = i;
        };


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "UCSD ERP Lab" );
    StringCopy ( ProductName, "ERPSS" );
    StringCopy ( VersionName, FILEEXT_EEGRDF );
    Version             = rawheader.rh_dvers;
    Subversion          = NumElectrodes;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy electrodes names, null terminated
    for ( int i=0; i < NumElectrodes; i++ )
                                        // be careful to also remap the names!
        StringCopy ( ElectrodesNames[ RemappingIndex ? RemappingIndex[ i ] : i ], rawheader.rh_chdesc[ i ] );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open EEG file
    InputStream     = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;

                                        // check for auxilliary
    InitAuxiliaries ();
    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegERPSSRdfDoc::SetArrays ()
{
OffGfp          = NumElectrodes + PseudoTrackOffsetGfp;
OffDis          = NumElectrodes + PseudoTrackOffsetDis;
OffAvg          = NumElectrodes + PseudoTrackOffsetAvg;


Tracks.Resize ( TotalElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegERPSSRdfDoc::ProcessTracks ( long reltf, bool firsttrack )
{
static uchar        nib[ 8 ];           // a queue for read nibbles, one nibble per byte, in the lsb
static int          numnib;             // how many currently in the queue, due to short (4 nibbles) reads
long                delta;
ushort              s;
int                 elm;


if ( Compressed ) {

    if ( firsttrack )                   // start from first tf of block, and do incremental computation - we don't know where is the first absolute value...
        numnib      = 0;                // reset the queued nibbles
    else                                // do a simple increment, within current position in current block, on current values in Tracks
        reltf = 0;


    for ( ; reltf >= 0; reltf-- ) {

        for ( int el = 0; el < NumElectrodes; el++ ) {

            elm = RemappingIndex ? RemappingIndex[ el ] : el;

                                        // need to fetch some more nibbles?
            if ( numnib == 0
              || numnib <= 1 && ( nib[ 0 ] & 0x0c ) == 0x08
              || numnib <= 2 && ( nib[ 0 ] & 0x0e ) == 0x0c
              || numnib <= 3 && ( nib[ 0 ] & 0x0f ) == 0x0e )
                {
                                        // read next short
                InputStream->read ( (char *) &s, sizeof ( s ) );

//              SwappedBytes ( s, NeedSwap );
                                        // split and distribute its nibbles in the queue
                nib[ numnib++ ] = (uchar) ( ( ( s & 0xf000 ) >> 12 ) &0xf );
                nib[ numnib++ ] = (uchar) ( ( ( s & 0x0f00 ) >>  8 ) &0xf );
                nib[ numnib++ ] = (uchar) ( ( ( s & 0x00f0 ) >>  4 ) &0xf );
                nib[ numnib++ ] = (uchar) ( ( ( s & 0x000f )       ) &0xf );
                }

                                        // now check first nibble coding bits 0DDD / 10DD / 110D / 1110 ?
                                        // compute either a delta or an absolute value
            if      ( ( nib[ 0 ] & 0x08 ) == 0 ) {

                delta = ( ( nib[ 0 ] & 0x04 ) ? 0xfffffffcL : 0L ) | ( nib[ 0 ] & 0x03 );
                Tracks[ elm ] += delta;
                numnib--;
                if ( numnib > 0 )
                    MoveVirtualMemory ( nib, nib + 1, numnib );
                }
            else if ( ( nib[ 0 ] & 0x04 ) == 0 ) {

                delta = ( ( nib[ 0 ] & 0x02 ) ? 0xffffffe0L : 0L ) | ( ( nib[ 0 ] & 0x01 ) << 4 ) | nib[ 1 ];
                Tracks[ elm ] += delta;
                numnib -= 2;
                if ( numnib > 0 )
                    MoveVirtualMemory ( nib, nib + 2, numnib );
                }
            else if ( ( nib[ 0 ] & 0x02 ) == 0 ) {

                delta = ( ( nib[ 0 ] & 0x01 ) ? 0xffffff00L : 0L ) | ( nib[ 1 ] << 4 ) | nib[ 2 ];
                Tracks[ elm ] += delta;
                numnib -= 3;
                if ( numnib > 0 )
                    MoveVirtualMemory ( nib, nib + 3, numnib );
                }
            else {

                delta  = ( ( nib[ 1 ] & 0x08 ) ? 0xfffff000L : 0L ) | ( nib[ 1 ] << 8 ) | ( nib[ 2 ] << 4 ) | nib[ 3 ];
                Tracks[ elm ] = delta;
                numnib -= 4;
                if ( numnib > 0 )
                    MoveVirtualMemory ( nib, nib + 4, numnib );
                }
            } // electrodes
        } // TFs
    } // Compressed

else {  // not Compressed
                                        // this case is not te most efficient, as the transfer in Tracks is useless
                                        // still it uniformizes the treatment with the compressed case...

                                        // first access in block -> set position
    if ( firsttrack && reltf )
        InputStream->seekg ( reltf * NumElectrodes * sizeof ( short ), ios::cur );

                                        // read and copy
    for ( int el = 0; el < NumElectrodes; el++ ) {

        elm = RemappingIndex ? RemappingIndex[ el ] : el;

        InputStream->read ( (char *) &s, sizeof ( s ) );
        Tracks[ elm ] = s;
        } // electrodes
    } // not Compressed
}


//----------------------------------------------------------------------------

void    TEegERPSSRdfDoc::TfToBlockOffset ( const int& tf, int& block, long& offset )     const
{
block   = DataOrg + sizeof ( RAWRHDR );
offset  = tf;

for ( int r = 0; r < NumRecords && offset >= TFRecord; r++, offset -= TFRecord )

    block  += RecordSize[ r ];
}


void    TEegERPSSRdfDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 el;
int                 tf;
long                tfi;
long                tfoff;
long*               toT;
int                 Block;
double*             toG;
int                 reci;
bool                firsttrack  = true;


TfToBlockOffset     ( tf1, Block, tfoff );

InputStream->seekg  ( Block, ios::beg );

reci    = tf1 / TFRecord;


for ( tfi = tf1, tf = 0 ; tfi <= tf2; ) {

    for ( ; tfoff < TFRecord && tfi <= tf2; tfoff++, tfi++, tf++ ) {

        ProcessTracks ( tfoff, firsttrack );  // specify if it is a first call, even in a random access in a block

        for ( el=0, toT=Tracks, toG=Gains; el < NumElectrodes; el++, toT++, toG++ )
            buff ( el, tfoffset + tf )  = *toT * *toG;

        firsttrack = false;
        }

    tfoff = 0;
    firsttrack = true;                  // we change block, so it is a "first call"

    if ( reci < NumRecords - 1 ) {      // set position to next data block, if not last block
        Block += RecordSize[ reci ];    // (note that we already have an offset of  sizeof ( RAWRHDR ) )
        reci++;
        InputStream->seekg ( Block, ios::beg );
        }
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
