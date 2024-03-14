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

#include    "TEegEgiRawDoc.h"

#include    "Dialogs.Input.h"
#include    "Strings.Utils.h"
#include    "TArray1.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegEgiRawDoc::TEegEgiRawDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
InputStream         = 0;
BuffSize            = 0;

NumTimeFrames       = 0;
NumElectrodes       = 0;
NumAuxElectrodes    = 0;
NumMinElectrodes    = 0;
TotalElectrodes     = 0;
DataOrg             = 0;
NumEvents           = 0;
ScaleData           = 1;
ScaleDataCalibrated = 1;

NumElectrodesInFile = 0;
SamplingFrequency   = 0;
Reference           = ReferenceAsInFile;
}


bool	TEegEgiRawDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegEgiRawDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegEgiRawDoc::ReadNativeMarkers ()
{
//return;
                                        // scan the whole file for the triggers
long                tfi;
//char                tc;
TArray1<bool>       on          ( NumEvents );
TArray1<bool>       oldon       ( NumEvents );
TStrings            eventnames  ( NumEvents, 5 );
int                 e;
ushort              dincode;
char                dinname[ 32 ];
short              *toFBS;
float              *toFBF;
float               f;


toFBS = Version == 4 ? 0 : (short *) FileBuff.data ();
toFBF = Version != 4 ? 0 : (float *) FileBuff.data ();


ifstream    ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // read event names
ifs.seekg ( sizeof ( TEegEgiRawHeader ), ios::beg );
for ( e = 0; e < NumEvents; e++ ) {

    ClearString ( eventnames[ e ], 5 );
    ifs.read ( eventnames[ e ], 4 );
    }

ifs.seekg ( DataOrg, ios::beg );
ifs.read ( FileBuff.data (),  BuffSize );

for ( e=0; e < NumEvents; e++ ) {

//  SwapShort ( FileBuff + NumElectrodes + e );
    oldon[e]    = toFBS ? toFBS [NumElectrodesInFile + e] : toFBF [NumElectrodesInFile + e];
    }


for ( tfi=1; tfi < NumTimeFrames; tfi++ ) {

    ifs.read ( FileBuff.data (), BuffSize );

    dincode = 0;

    for ( e=0; e < NumEvents; e++ ) {

//      SwapShort ( FileBuff + NumElectrodes + e );

        on[e]   = toFBS ? toFBS [NumElectrodesInFile + e] : toFBF [NumElectrodesInFile + e];
                                        // trigger now?
        if ( on[e] && !oldon[e] ) {
                                        // cumulate din? values
            if      ( StringIs ( eventnames[ e ], "din1" ) )    dincode |= 0x1;
            else if ( StringIs ( eventnames[ e ], "din2" ) )    dincode |= 0x2;
            else if ( StringIs ( eventnames[ e ], "din3" ) )    dincode |= 0x4;
            else if ( StringIs ( eventnames[ e ], "din4" ) )    dincode |= 0x8;

            else {                      // not known code, put it straight away

//              tc  = (char) (e + 1);
//              AppendMarker ( TMarker ( tfi, tfi, tc, eventnames[e] ) );

                if ( toFBS ) {
                    dincode = toFBS [ NumElectrodesInFile + e ];
                    SwappedBytes ( dincode );
                    }
                else {
                    f       = SwapBytes ( toFBF [ NumElectrodesInFile + e ], true );
                    dincode = (ushort) f;
                    }
                }
            } // if on && !oldon

        oldon[e]    = on[e];
        }
                                        // should add a deciphered code?
    if ( dincode ) {
        sprintf ( dinname, "%0hu", dincode );
        AppendMarker ( TMarker ( tfi, tfi, dincode, dinname, MarkerTypeTrigger ) );
        }
    }
}


//----------------------------------------------------------------------------
bool	TEegEgiRawDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


TEegEgiRawHeader   rawheader;

ifs.read ( (char *) &rawheader,  sizeof (rawheader) );

SwappedBytes ( rawheader.Version );
SwappedBytes ( rawheader.SamplingRate );
SwappedBytes ( rawheader.Channels );
SwappedBytes ( rawheader.Samples );

switch ( what ) {

    case ReadNumElectrodes :
    case ReadNumAuxElectrodes :

        if ( what == ReadNumElectrodes )
                                                      // heuristic to add any missing reference channel: if number of electrodes is even, it is missing
            *((int *) answer)   = rawheader.Channels + ( IsEven ( rawheader.Channels ) ? 1 : 0 );
        else if ( what == ReadNumAuxElectrodes )
            *((int *) answer)   = 0;

        return  true ;

    case ReadNumTimeFrames :
        *((int *) answer)       = rawheader.Samples;
        return  true;

    case ReadSamplingFrequency :
        *((double *) answer)    = rawheader.SamplingRate;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegEgiRawDoc::Open	( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    InputStream     = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;

    TEegEgiRawHeader   rawheader;

    InputStream->read ( (char *)(&rawheader), sizeof ( rawheader ) );
                                        // re-order bytes, from Mac uses Big Indian
    SwappedBytes ( rawheader.Version );
    SwappedBytes ( rawheader.Year );
    SwappedBytes ( rawheader.Month );
    SwappedBytes ( rawheader.Day );
    SwappedBytes ( rawheader.Hour );
    SwappedBytes ( rawheader.Minute );
    SwappedBytes ( rawheader.Second );
    SwappedBytes ( rawheader.Millisecond );
    SwappedBytes ( rawheader.SamplingRate );
    SwappedBytes ( rawheader.Channels );
    SwappedBytes ( rawheader.BoardGain );
    SwappedBytes ( rawheader.Bits );
    SwappedBytes ( rawheader.Range );
    SwappedBytes ( rawheader.Samples );
    SwappedBytes ( rawheader.Events );


    if ( rawheader.Version == 3 || rawheader.Version == 5 ) {
        ShowMessage ( "This Raw format is not supported, sorry...", "File Open", ShowMessageWarning );
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Electrical Geodesics" );
    StringCopy ( ProductName, FILEEXT_EEGNSRRAW );
    Version             = rawheader.Version;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodesInFile = rawheader.Channels;       // used to access data in file
//  NumElectrodes       = rawheader.Channels + ( Version == 4 ? 0 : 1 );   // one more set to 0 (the reference)
    NumElectrodes       = rawheader.Channels;
    if ( IsEven ( NumElectrodesInFile ) )
        NumElectrodes++;                            // heuristic to add any missing reference channel: if number of electrodes is even, it is missing
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;

    SamplingFrequency   = rawheader.SamplingRate;
    NumTimeFrames       = rawheader.Samples;
    NumEvents           = rawheader.Events;         // number of tracks used to store trigger events
    DataOrg             = sizeof ( rawheader ) + NumEvents * 4;

    ScaleData           = (double) rawheader.Range / ( 1 << rawheader.Bits );
    if ( !ScaleData )   ScaleData = 1;
//  ScaleDataCalibrated = ScaleData / 0.24;
//  ScaleDataCalibrated = ScaleData * 1639 / 400; // value from version 1 of NSR
    ScaleDataCalibrated = ScaleData * 1495 / 400; // value from version 3 of NSR

    BuffSize            = ( Version == 4 ? sizeof ( float ) : sizeof ( short ) ) * ( NumElectrodesInFile + NumEvents );
    FileBuff.resize ( BuffSize );

    DateTime            = TDateTime ( rawheader.Year, rawheader.Month,  rawheader.Day,
                                      rawheader.Hour, rawheader.Minute, rawheader.Second, rawheader.Millisecond, 0 );

    Subversion          = NumElectrodes;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InitAuxiliaries ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can read external calibration files?
    TFileName           bg              = GetDocPath();
    TFileName           bz              = GetDocPath();

                                        // account for session id added before extension
    RemoveExtension ( bg );

    const char *toc = ToExtension ( bg );

    if ( StringLength ( toc ) == 3 && StringToInteger ( toc ) > 0 )
        RemoveExtension ( bg );
    StringAppend ( bg, ".GAIN" );


    RemoveExtension ( bz );

    toc = ToExtension ( bz );

    if ( StringLength ( toc ) == 3 && StringToInteger ( toc ) > 0 )
        RemoveExtension ( bz );
    StringAppend ( bz, ".ZERO" );

                                        // Version 4 does not seem to need any calibration anymore (float)
    bool                gg              = Version != 4 && CanOpenFile ( bg );
    bool                gz              = Version != 4 && CanOpenFile ( bz );


    if ( gg && gz ) {
        int         i;
        int         g;
        int         z;
        ifstream    ofsg ( TFileName ( bg, TFilenameExtendedPath ), ios::binary );

        Gains.resize ( NumElectrodesInFile );
                                        // skip first 3 lines
        ofsg.getline ( bg, MaxPathShort, 0xd );
        ofsg.getline ( bg, MaxPathShort, 0xd );
        ofsg.getline ( bg, MaxPathShort, 0xd );

        for ( i=0; i < NumElectrodesInFile; i++ ) {
            ofsg.getline ( bg, MaxPathShort, 0xd );
            sscanf ( bg, "%*d : %d", &g );
            Gains[i] = (double) 400 / g;
//          Gains[i] = (double) 2500 / g;
            }


        ifstream    ofsz ( TFileName ( bz, TFilenameExtendedPath ), ios::binary );
        Zeros.resize ( NumElectrodesInFile );
                                        // skip first 3 lines
        ofsz.getline ( bg, MaxPathShort, 0x0d );
        ofsz.getline ( bg, MaxPathShort, 0x0d );
        ofsz.getline ( bg, MaxPathShort, 0x0d );

        for ( i=0; i < NumElectrodesInFile; i++ ) {
            ofsz.getline ( bg, MaxPathShort, 0xd );
            sscanf ( bg, "%*d : %d", &z );
            Zeros[i] = z;
            }
        }

    else if ( Version != 4 ) {
        char                buff[ 256 ];

        sprintf ( buff, "This file does not seem to be calibrated properly:" NewLine 
                        Tab "Zeroes: %s" NewLine 
                        Tab "Gains:  %s" NewLine 
                        "Do you wish to open it anyway?", 
                        gz ? "found" : "not found", gg ? "found" : "not found" );

        if ( ! GetAnswerFromUser ( buff, "File Open" ) ) {
            delete  InputStream; InputStream = 0;
            return false;
            }
        }
    }
else {                          // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegEgiRawDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.resize ( TotalElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    IntegerToString ( ElectrodesNames[ i - 1 ], i );

StringCopy ( ElectrodesNames[ NumElectrodes - 1 ], "VREF" );    // provides default names

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegEgiRawDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float>& buff, int tfoffset )
{
                                        // set file to first TF
InputStream->seekg ( DataOrg + BuffSize * tf1, ios::beg );


for ( int tfi = tf1, tf0 = 0; tfi <= tf2; tfi++, tf0++ ) {

    InputStream->read ( FileBuff.data (), BuffSize );
                                        // version 4 is float, 2 is short int
    short*      toFBS   = Version == 4 ? 0 : (short *) FileBuff.data ();
    float*      toFBF   = Version != 4 ? 0 : (float *) FileBuff.data ();


    if ( Gains.size () ) {

        float*      toT     = Tracks.data ();
        double*     toG     = Gains .data ();
        double*     toZ     = Zeros .data ();

        for ( int el = 0; el < NumElectrodesInFile; el++, toT++, toG++, toZ++ ) {

            if ( toFBS ) {
                *toT    = ( SwappedBytes ( *toFBS ) * ScaleDataCalibrated - *toZ ) * *toG;
                toFBS++;
                }
            else {
                *toT    = ( SwappedBytes ( *toFBF ) * ScaleDataCalibrated - *toZ ) * *toG;
                toFBF++;
                }
            }
        }
    else {

        float*      toT     = Tracks.data ();

        for ( int el = 0; el < NumElectrodesInFile; el++, toT++ ) {

            if ( toFBS ) {
                *toT    = SwappedBytes ( *toFBS ) * ScaleData;
                toFBS++;
                }
            else {
                *toT    = SwappedBytes ( *toFBF ) * ScaleData;
                toFBF++;
                }
            }
        }

                                            // force this one more track to 0
    Tracks[ NumElectrodes - 1 ] = 0;


    float*      toT = Tracks.data ();

    for ( int el = 0; el < NumElectrodes; el++, toT++ )
        buff ( el, tfoffset + tf0 ) = *toT;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
