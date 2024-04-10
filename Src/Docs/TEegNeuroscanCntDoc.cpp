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

#include    "TEegNeuroscanCntDoc.h"

#include    "Dialogs.Input.h"

#include    "TArray1.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegNeuroscanCntDoc::TEegNeuroscanCntDoc (TDocument *parent)
      : TTracksDoc (parent)
{
InputStream         = 0;
BuffSize            = 0;

NumElectrodesInFile = 0;
NumTimeFrames       = 0;
NumElectrodes       = 0;
NumMinElectrodes    = 0;
TotalElectrodes     = 0;
DataOrg             = 0;
NumEvents           = 0;

SamplingFrequency   = 0;
Reference           = ReferenceAsInFile;
}


bool	TEegNeuroscanCntDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegNeuroscanCntDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegNeuroscanCntDoc::ReadNativeMarkers ()
{
TNsSetup            setup;

ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

ifs.read ( (char *)(&setup), sizeof ( setup ) );

                                        // 2 differents way of storing events
if ( setup.ContinousType == ContinousTypeEventTable1
  || setup.ContinousType == ContinousTypeEventTable2 ) {
                                        // read events
    TNsTEeg             teeg;

    ifs.seekg ( setup.EventTablePos, ios::beg );
    ifs.read ( (char *)(&teeg), sizeof ( teeg ) );

    bool                event1          = teeg.Teeg == 1;
    NumEvents           = teeg.Size / ( event1 ? sizeof ( TNsEvent1 ) : sizeof ( TNsEvent2 ) );

    TNsEvent1           ev1;
    TNsEvent2           ev2;
    long                tf;
    MarkerCode          code;
    int                 kp;
    char                name[ MarkerNameMaxLength ];

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if ( event1 ) {

        for ( int e=0; e < NumEvents; e++ ) {

            ifs.read ( (char *)(&ev1), sizeof ( ev1 ) );

            tf  = ( ev1.Offset - ( sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc ) ) ) / sizeof ( short ) / NumElectrodes;

            if ( ev1.StimType ) {
                code    = (MarkerCode) ev1.StimType;
                sprintf ( name, "%0d", ev1.StimType );
                }
            else if ( ev1.KeyBoard ) {
                code    = (MarkerCode) ev1.KeyBoard;
                sprintf ( name, "F%0d", ev1.KeyBoard + 1 );
                }
            else if ( ev1.KeyPad_Accept ) {
                code    = (MarkerCode) ( ev1.KeyPad_Accept & 0xF );
                kp      = ( ev1.KeyPad_Accept & 0xF0 ) >> 4;

                if      ( kp == 0xD )   sprintf ( name, "KPA%0d", code );
                else if ( kp == 0xC )   sprintf ( name, "KPR%0d", code );
                else if ( kp == 0xE )   sprintf ( name, "KP%0d",  code );
                else                    sprintf ( name, "KP%0d",  code );
                }
            else {
                code    = (MarkerCode) 0;
                StringCopy ( name, "0" );
                }

            AppendMarker ( TMarker ( tf, tf, code, name, MarkerTypeTrigger ) );
            }
        } // event1

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else { // ! event1

        for ( int e=0; e < NumEvents; e++ ) {

            ifs.read ( (char *)(&ev2), sizeof ( ev2 ) );

            tf  = ( ev2.Offset - ( sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc ) ) ) / sizeof ( short ) / NumElectrodes;

            if ( ev2.StimType ) {
                code    = (MarkerCode) ev2.StimType;
                sprintf ( name, "%0d", ev2.StimType );
                }
            else if ( ev2.KeyBoard ) {
                code    = (MarkerCode) ev2.KeyBoard;
                sprintf ( name, "F%0d", ev2.KeyBoard + 1 );
                }
            else if ( ev2.KeyPad_Accept ) {
                code    = (MarkerCode) ( ev2.KeyPad_Accept & 0xF );
                kp      = ( ev2.KeyPad_Accept & 0xF0 ) >> 4;

                if      ( kp == 0xD )   sprintf ( name, "KPA%0d", code );
                else if ( kp == 0xC )   sprintf ( name, "KPR%0d", code );
                else if ( kp == 0xE )   sprintf ( name, "KP%0d",  code );
                else                    sprintf ( name, "KP%0d",  code );
                }
            else {
                code    = (MarkerCode) 0;
                StringCopy ( name, "0" );
                }

            AppendMarker ( TMarker ( tf, tf, code, name, MarkerTypeTrigger ) );
            }
        } // ! event1
    } // event table

else if (  setup.ContinousType == ContinousType2ExtraChannels ) {
                                        // TODO, when the case actually happens
    };
}


//----------------------------------------------------------------------------
bool	TEegNeuroscanCntDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

if ( ifs.fail() ) return false;


TNsSetup            setup;
TNsElectLoc         eloc;
int                 NumElectrodesInFile;
long                NumTimeFrames;

ifs.read ( (char *)(&setup), sizeof ( setup ) );

switch ( what ) {

    case ReadNumElectrodes :
    case ReadNumAuxElectrodes :

        int         NumElectrodes;
        int         NumAuxElectrodes;
//      char       *toc;
//      char        auxnames[512];

        NumElectrodes       = setup.nchannels;
        NumAuxElectrodes    = 0;

        int         el;
        for ( el = 0; el < NumElectrodes; el++ ) {

            ifs.read ( (char *)(&eloc), sizeof ( eloc ) );

            if ( IsElectrodeNameAux ( eloc.lab ) )
                NumAuxElectrodes++;
            }

        if ( what == ReadNumElectrodes )
            *((int *) answer)   = NumElectrodes;
        else if ( what == ReadNumAuxElectrodes )
            *((int *) answer)   = NumAuxElectrodes;

        return  true ;


    case ReadNumTimeFrames :

        NumElectrodesInFile     = setup.nchannels + ( setup.ContinousType == ContinousType2ExtraChannels ? 2 : 0 );
        NumElectrodes           = setup.nchannels;

        if ( IsExtension ( file, FILEEXT_EEGNSCNT ) )
            NumTimeFrames           = ( setup.EventTablePos - ( sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc ) ) ) / ( sizeof ( short ) * NumElectrodesInFile );
        else
            NumTimeFrames           = setup.pnts;

        *((int *) answer)       = NumTimeFrames;
        return  true;


    case ReadSamplingFrequency :
        *((double *) answer)    = setup.rate;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegNeuroscanCntDoc::Open	(int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    InputStream     = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;


    TNsSetup        setup;

    InputStream->read ( (char *) ( &setup ), sizeof ( setup ) );

    setup.rev[ 19 ] = 0;
                                        // setup.type is not reliable
    if ( StringIsNot ( setup.rev, EEGNS_REV ) /*|| setup.type != EEGNS_EEG*/ ) {

        if ( ! GetAnswerFromUser ( "File version seems incorrect, proceeding anyway?", ToFileName ( GetDocPath () ) ) ) {
            delete  InputStream; InputStream = 0;
            return false;
            }
        }

    if ( setup.nchannels == 0 ) {

        ShowMessage ( "File header seems too much corrupted to proceed...", ToFileName ( GetDocPath () ), ShowMessageWarning );
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Neuroscan" );
    StringCopy ( ProductName, FILEEXT_EEGNSCNT );
    StringCopy ( VersionName, setup.rev );
    Subversion          = setup.nchannels;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodesInFile = setup.nchannels + ( setup.ContinousType == ContinousType2ExtraChannels ? 2 : 0 );
    NumElectrodes       = setup.nchannels;
    NumMinElectrodes    = NumElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = setup.rate;
    NumTimeFrames       = ( setup.EventTablePos - ( sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc ) ) ) / ( sizeof ( short ) * NumElectrodesInFile );
//  NumTimeFrames       = setup.pnts; // don't trust this!
    DataOrg             = sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc );

    BuffSize            = sizeof ( short ) * ( NumElectrodesInFile );
    FileBuff.Resize ( BuffSize );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract date and time
    if ( *setup.time && *setup.date ) {

        int     h,  m,  s;
        int     y,  mo, d;
        char    date[ 11 ];

        StringCopy  ( date, setup.date, 10 );
        sscanf      ( setup.time, "%d:%d:%d", &h, &m, &s );
        sscanf      ( date, "%d/%d/%d", &mo, &d, &y );

        if      ( y <=  50 )    y  += 2000;
        else if ( y <=  99 )    y  += 1900;
        else if ( y < 1000 )    y  *= 10;

        DateTime    = TDateTime ( y, mo, d, h, m, s, 0, 0 );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation & default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read electrodes -> names, calibration
    TNsElectLoc     eloc;

    for ( int el = 0; el < NumElectrodes; el++ ) {

        InputStream->read ( (char *)(&eloc), sizeof ( eloc ) );

        StringCopy ( ElectrodesNames[ el ], eloc.lab, ElectrodeNameSize - 1 );

        Zeros[ el ] = eloc.baseline;
        Gains[ el ] = ((double) eloc.sensitivity * eloc.calib ) / 204.8;
        }


    InitAuxiliaries ();
    }
else {                          // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegNeuroscanCntDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;


Tracks.Resize ( TotalElectrodes );
Gains .Resize (  NumElectrodes  );
Zeros .Resize (  NumElectrodes  );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegNeuroscanCntDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 el;
int                 tf;
long                tfi;
float              *toT;
short              *toFB;
double             *toG;
double             *toZ;

                                            // set file to first TF
InputStream->seekg ( DataOrg + BuffSize * tf1, ios::beg );


for ( tfi = tf1, tf = 0; tfi <= tf2; tfi++, tf++ ) {

    InputStream->read ( (char *) FileBuff.GetArray (),  BuffSize );


    for ( el=0, toT=Tracks, toFB=FileBuff, toZ=Zeros, toG=Gains; el < NumElectrodes; el++, toT++, toFB++, toZ++, toG++ )
        *toT    = ( *toFB - *toZ ) * *toG;


    for ( el=0, toT=Tracks; el < NumElectrodes; el++, toT++ )
        buff ( el, tfoffset + tf )  = *toT;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
