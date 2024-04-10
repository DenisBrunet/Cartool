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

#include    "TEegNeuroscanAvgDoc.h"
#include    "TEegNeuroscanCntDoc.h"                  // Neuroscan headers

#include    "Dialogs.Input.h"

#include    "TArray1.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegNeuroscanAvgDoc::TEegNeuroscanAvgDoc ( TDocument *parent )
      : TEegCartoolEpDoc ( parent )
{
DataOrg             = 0;
}


bool	TEegNeuroscanAvgDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
bool	TEegNeuroscanAvgDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
return  TEegNeuroscanCntDoc::ReadFromHeader ( file, what, answer ); // yep, ReadFromHeader is a static method
}


//----------------------------------------------------------------------------
bool	TEegNeuroscanAvgDoc::Open ( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    TInStream*          InputStream     = InStream ( ofRead | ofBinary );

    if ( ! InputStream )
        return  false;

    TNsSetup        setup;

    InputStream->read ( (char *)(&setup), sizeof ( setup ) );

                                        // setup.type is not reliable
    if ( StringIsNot ( setup.rev, EEGNS_REV ) /*|| setup.type != EEGNS_EEG*/ ) {

        if ( ! GetAnswerFromUser ( "File version seems incorrect, proceeding anyway?", ToFileName ( GetDocPath () ) ) ) {
            delete  InputStream;
            return false;
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Neuroscan" );
    StringCopy ( ProductName, FILEEXT_EEGNSAVG );
    StringCopy ( VersionName, setup.rev );
    Subversion          = setup.nchannels;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodes       = setup.nchannels;
    NumMinElectrodes    = NumElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = setup.rate;
    DataOrg             = sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc );

//  InputStream->seekg ( 0, ios::end );
//  NumTimeFrames       = ( ( InputStream->tellg() - ( sizeof ( setup ) + NumElectrodes * sizeof ( TNsElectLoc ) ) ) / NumElectrodes - 5 )  / sizeof ( float );
    NumTimeFrames       = setup.pnts;


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
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read electrodes -> names, calibration
    TNsElectLoc         eloc;

    InputStream->seekg ( sizeof ( setup ), ios::beg );

    for ( int el = 0; el < NumElectrodes; el++ ) {

        InputStream->read ( (char *)(&eloc), sizeof ( eloc ) );

        StringCopy ( ElectrodesNames[ el ], eloc.lab );

//      Zeros[ el ] = eloc.baseline;
//      Gains[ el ] = (double) eloc.sensitivity * eloc.calib / 204.8;
        Zeros[ el ] = 0;
        Gains[ el ] = eloc.n && ! eloc.bad ? eloc.calib / (double) eloc.n : 0;
        }

    InitAuxiliaries ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read all data at once and for all
    InputStream->seekg ( DataOrg, ios::beg );

    for ( int el = 0; el < NumElectrodes; el++ ) {

        InputStream->seekg ( 5, ios::cur );     // skip 5 bytes

        InputStream->read ( (char *) Tracks[ el ], NumTimeFrames * sizeof ( float ) );
        } // for el

                                        // apply calibration
    for ( int el=0; el < NumElectrodes; el++ )
        for ( int tf=0; tf < NumTimeFrames; tf++ )
            Tracks ( el, tf )   = ( Tracks ( el, tf ) - Zeros[ el ] ) * Gains[ el ];


    delete  InputStream;
    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegNeuroscanAvgDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;


Tracks.Resize ( TotalElectrodes, NumTimeFrames );
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
//----------------------------------------------------------------------------

}
