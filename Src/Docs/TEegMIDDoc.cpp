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

#include    <time.h>

#include    "MemUtil.h"

#include    "TArray1.h"

#include    "TEegMIDDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegMIDDoc::TEegMIDDoc (TDocument *parent)
      : TTracksDoc ( parent )
{
InputStream         = 0;
BuffTFSize          = 0;

NumTimeFrames       = 0;
NumElectrodes       = 0;
TotalElectrodes     = 0;
DataCalibrated      = false;
Polarity            = false;
BlockStructure      = false;
PackedDatas         = false;
Zero                = 0;
Unit                = 1;
DataOrg             = 0;
ExtHeaderOrg        = 0;

XHCalibration       = false;
XHCI.ncal           = 0;
XHCI.ampl           = 1;
forceCI             = 0;

SamplingFrequency   = 0;
Reference           = ReferenceAsInFile;
}


bool	TEegMIDDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegMIDDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegMIDDoc::ReadNativeMarkers ()
{
ifstream                ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // read extended header
TDFileExtHeaderRecord   xhrec;
TDFileExtHeaderTT       xhtt;
int                     numtags = 0;
bool                    breakloop   = false;


ifs.seekg ( ExtHeaderOrg, ios::beg );

do {

    ifs.read ( (char *) &xhrec,     sizeof (xhrec) );

    switch ( xhrec.mnemo ) {

        case    'TT' :                  // Tag table
            ifs.read ( (char *) &xhtt,  sizeof (xhtt) );
            numtags     = xhtt.list_len / 4;
            breakloop   = true;
            break;

        default:                        // skip this record
            ifs.seekg ( xhrec.size, ios::cur );
            break;
        }

    } while ( xhrec.mnemo != 0 && !breakloop );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read the markers, if any
if ( numtags != 0 ) {

                                        // First, we have to build a table of the trigger names
    int                 numtd           = xhtt.def_len / sizeof ( TDFileTagDef );
    TDFileTagDef*       TagDefList      = new TDFileTagDef [ numtd ];


    ifs.seekg ( xhtt.def_off, ios::beg );

    for ( int i = 0; i < numtd; i++ )

        ifs.read ( (char *) ( TagDefList + i ), sizeof ( TDFileTagDef ) );

    TagDefList[numtd - 1].txtlen    &= 0x7FFF;  // clear msb


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Then, we can generate the actual triggers
    TMarker             marker;
    uchar               pos[ 3 ];
    uchar               cl;

    ifs.seekg ( xhtt.list_off, ios::beg );

    for ( int i = 0; i < numtags; i++ ) {

        ifs.read ( (char *) pos,    3 * sizeof ( char ) );
        cl  = (uchar) ifs.get ();
                                        // clearing up first bit
        cl     &= 0x7f;

                                        // 3 bytes unsigned integer
        marker.From        = pos[ 0 ] + ( pos[ 1 ] << 8 ) + ( pos[ 2 ] << 16 );
        marker.To          = marker.From;

        marker.Code        = (MarkerCode) cl;
        marker.Type        = MarkerTypeTrigger;
                                        // 2 characters-long trigger name
        marker.Name[ 0 ]   = TagDefList[ cl ].abrv[ 0 ];
        marker.Name[ 1 ]   = TagDefList[ cl ].abrv[ 1 ];
        marker.Name[ 2 ]   = EOS;


        Clipped ( marker.From, marker.To, (long) 0, (long) NumTimeFrames - 1 );

        AppendMarker ( marker );
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    delete []   TagDefList;
    }
}


//----------------------------------------------------------------------------
bool	TEegMIDDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


TDFileHeader            header;
TDFileExtHeaderRecord   xhrec;

ifs.read ( (char *) &header, sizeof (header) );

switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)   = header.nchan;
        return   true ;

    case ReadNumAuxElectrodes :
        *((int *) answer)   = header.naux;
        return   true ;

    case ReadSamplingFrequency :
        *((double *) answer)= header.fsamp;
        return  true;

    case ReadElectrodesNames :
        bool    found   = false;
        ifs.seekg ( header.xhdr_org * 16, ios::beg );
        do {
            ifs.read ( (char *) &xhrec,     sizeof (xhrec) );

            char        cnbuff[5];
            char      **en = (char **) answer;
            int         e;

            switch ( xhrec.mnemo ) {
                case    'CN' :          // channel names
                    cnbuff[4]   = 0;
                    for ( e=0; e < header.nchan; e++ ) {
                        ifs.read ( (char *) cnbuff,  4 );
                        StringCopy ( en[e], cnbuff );
                        }
                    found   = true;
                    break;

                default:                // skip this record
                    ifs.seekg ( xhrec.size, ios::cur );
                    break;
                }
            } while ( xhrec.mnemo != 0 );

        return   found ;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegMIDDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );

if ( GetDocPath () ) {

    InputStream     = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;


    TDFileHeader    header;
    InputStream->read ( (char *) &header,    sizeof (header) );

                                        // fill product info
    StringCopy ( CompanyName, "M&I" /*"Neuroscience Technology Research"*/ );
    StringCopy ( ProductName, "EASYS2" );
    VersionName[0]      = header.ftype;
    VersionName[1]      = 0;
    Subversion          = header.nchan;

                                        // decipher the header + type conversion
    NumElectrodes       = header.nchan;
    int NumAux          = header.naux;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = header.fsamp;
    NumTimeFrames       = header.nsamp;
    Zero                = header.zero;
    Unit                = header.unit;
    DataOrg             = header.data_org * 16;
    ExtHeaderOrg        = header.xhdr_org * 16;
    DataCalibrated      = header.d_val & 0x08;
    Polarity            = header.d_val & 0x10;
    BlockStructure      = header.d_val & 0x20;
    PackedDatas         = header.d_val & 0x40;
    BuffTFSize          = sizeof ( short ) * NumElectrodes;

                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read extended header
    TDFileExtHeaderRecord   xhrec;

    InputStream->seekg ( ExtHeaderOrg, ios::beg );

    do {
        InputStream->read ( (char *) &xhrec,     sizeof (xhrec) );

        switch ( xhrec.mnemo ) {

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case    'CI' :              // calibration info
                XHCalibration   = true;
                InputStream->read ( (char *) &XHCI,  2 * sizeof (short) );

                XHCI.zero .Resize ( NumElectrodes );
                XHCI.range.Resize ( NumElectrodes );

                int         i;
                for ( i=0; i < XHCI.ncal; i++ ) {
                    InputStream->read ( (char *) &( XHCI.zero [ i ] ),  sizeof (float) );
                    InputStream->read ( (char *) &( XHCI.range[ i ] ),  sizeof (float) );
                    }
                                        // in case, fill the remaining part of non-calibrated electrodes
                for ( ; i < NumElectrodes; i++ ) {
                    XHCI.zero [ i ]     = XHCI.zero [ 0 ];
                    XHCI.range[ i ]     = XHCI.range[ 0 ];
                    }
                break;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case    'FS' :              // real sampling frequency
                short       fs[2];

                InputStream->read ( (char *) fs,  2 * sizeof (short) );
                SamplingFrequency   = (double) fs[0] / (double) fs[1];
                break;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case    'CN' :              // channel names
                char        cnbuff[5];

                cnbuff[4]   = EOS;
                int         e;
                for ( e=0; e < NumElectrodes; e++ ) {
                    InputStream->read ( (char *) cnbuff,  4 );
                    StringCopy ( ElectrodesNames[e], cnbuff );
                    }
                break;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case    'DI' :              // data info
                DataInfo.Resize ( xhrec.size );
                InputStream->read ( DataInfo, xhrec.size );
                break;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case    'TI' :              // time info
                ulong       tmt32;      // !old 32 bits time stamp!
                time_t      tmt;        // !now 64 bits!
                struct  tm *tmb;

                InputStream->read ( (char *) &tmt32,  sizeof ( tmt32 ) );
                                        // convert to 64 bits
                tmt     = (time_t) tmt32;
                                        // should be good now
                tmb     = localtime ( &tmt );

                DateTime    = TDateTime ( tmb->tm_year + 1900, tmb->tm_mon, tmb->tm_mday,
                                          tmb->tm_hour, tmb->tm_min, tmb->tm_sec, 0, 0 );
                break;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            default:                    // skip this record
                InputStream->seekg ( xhrec.size, ios::cur );
                break;
            }
        } while ( xhrec.mnemo != 0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not found any calibration
    if ( ! DataCalibrated && ! XHCalibration ) {
        ShowMessage ( "No calibration found!", "Opening error", ShowMessageWarning );
        NumTimeFrames   = 0;    // to kill the view
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for auxilliary
    InitAuxiliaries ();

                                        // if auto-detect failed, force auxiliary channels at the end
    if ( AuxTracks.NumSet () != NumAux ) {
        
        if ( NumAux )   AuxTracks.Set ( NumElectrodes - NumAux, NumElectrodes - 1 );
        else            AuxTracks.Reset ();
        }

    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegMIDDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks  .Resize ( TotalElectrodes );
FileBuff.Resize ( NumElectrodes   );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
// Converts data from file to the real values in micro-volts
// according to the current calibration:
//      DataCalibrated  set if all data calibrated the same
//      XHCalibration   set if channels calibrated one by one
// one of the 2 should be set, otherwise error in opening the file.

void    TEegMIDDoc::FileDataToMicroVolts ( const TDFileExtHeaderCI* ci )
{
int                 t;
short*              toFB;
float*              toT;
const float*        toZ;
const float*        toR;
double              ciampl;
const float*        tociZ;
const float*        tociR;


if ( DataCalibrated ) {
                                        // calibration exists?
    if ( ci && XHCalibration ) {

        for ( t = 0, toT = Tracks, toFB = FileBuff,
              tociZ = ci->zero, toZ = XHCI.zero, tociR = ci->range, toR = XHCI.range; t < NumElectrodes; t++, toT++, toFB++, toZ++, toR++, tociZ++, tociR++ )

            *toT    = ( ( (double) *toFB - Zero ) / (double) ( Unit * XHCI.ampl ) * *toR + *toZ - *tociZ )
                      / (double) *tociR * ci->ampl ;
        }
    else               // can not calibrate
        for ( t = 0, toT = Tracks, toFB = FileBuff; t < NumElectrodes; t++, toT++, toFB++ )

            *toT    = ( (double) *toFB - Zero ) / Unit;
    }
else                    // calibrate using own or provided CI
    for ( t = 0, toT = Tracks, toFB = FileBuff,
          toZ = ci ? ci->zero : XHCI.zero, toR = ci ? ci->range : XHCI.range, ciampl = ci ? ci->ampl : XHCI.ampl; t < NumElectrodes; t++, toT++, toFB++, toZ++, toR++ )

        *toT    = ( (double) *toFB - *toZ ) / (double) *toR * ciampl;
}


//----------------------------------------------------------------------------
void    TEegMIDDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 el;
int                 tf;
long                tfi;
float*              toT;

                                        // set file to first TF
InputStream->seekg ( DataOrg + BuffTFSize * tf1, ios::beg );


for ( tfi = tf1, tf = 0; tfi <= tf2; tfi++, tf++ ) {
                                        // read 1 TF, all electrodes
    InputStream->read ( (char *) FileBuff.GetArray (),  BuffTFSize );

                                        // convert to real values
    FileDataToMicroVolts ();

                                        // then transfer
    for ( el = 0, toT = Tracks; el < NumElectrodes; el++, toT++ )
        buff (el,tfoffset + tf)  = *toT;
    }
}


void    TEegMIDDoc::UseCalibration ( TDFileExtHeaderCI* ci )
{
forceCI     = ci;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
