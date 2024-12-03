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

#include    "TEegCartoolSefDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegCartoolSefDoc::TEegCartoolSefDoc (TDocument *parent)
      : TTracksDoc (parent)
{
BuffSize            = 0;
}


bool    TEegCartoolSefDoc::Close ()
{
FileStream.Close ();

return  TFileDocument::Close ();
}


bool    TEegCartoolSefDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
bool    TEegCartoolSefDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


TSefHeader      sefheader;

ifs.read ( (char *) &sefheader,  sizeof (sefheader) );

switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)   = sefheader.NumElectrodes;
        return  true ;

    case ReadNumAuxElectrodes :
        *((int *) answer)   = sefheader.NumAuxElectrodes;
        return  true ;

    case ReadNumTimeFrames :
        *((int *) answer)   = sefheader.NumTimeFrames;
        return  true ;

    case ReadSamplingFrequency :
        *((double *) answer)= sefheader.SamplingFrequency;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------

bool	TEegCartoolSefDoc::Open	(int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    FileStream.Open ( GetDocPath(), FileStreamRead );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    TSefHeader      sefheader;

    FileStream.Read ( &sefheader, sizeof ( sefheader ) );

    if ( ! IsMagicNumber ( sefheader.Version, SEFBIN_MAGICNUMBER1 ) ) {

        ShowMessage ("Can not recognize this file (unknown magic number)!", "Open file", ShowMessageWarning );

        FileStream.Close ();

        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, CartoolRegistryCompany );
    StringCopy ( ProductName, FILEEXT_EEGSEF );
    Version             = sefheader.Version;
    Subversion          = sefheader.NumElectrodes;


    NumElectrodes       = sefheader.NumElectrodes;
    int NumAux          = sefheader.NumAuxElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = sefheader.SamplingFrequency;
    NumTimeFrames       = sefheader.NumTimeFrames;


    BuffSize            = sizeof ( float ) * NumElectrodes;

    DateTime            = TDateTime ( sefheader.Year, sefheader.Month,  sefheader.Day,
                                      sefheader.Hour, sefheader.Minute, sefheader.Second, sefheader.Millisecond, 0 );

    DataOrg             = sizeof ( sefheader ) + NumElectrodes * sizeof ( TSefChannelName );

                                        // space allocation + default names
    if ( ! SetArrays () ) {

        FileStream.Close ();

        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read electrode names
    char            buff[ KiloByte ];

    for ( int el = 0; el < NumElectrodes; el++ ) {

        FileStream.Read ( buff, sizeof ( TSefChannelName ) );

        buff[ sizeof ( TSefChannelName ) ] = EOS;   // force End Of String, i.e. 0

        StringCopy ( ElectrodesNames[ el ], buff );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for actual auxiliary channels, which could be anywhere
    int             oldnumaux       = NumAux;
                                        // smart scan
    InitAuxiliaries ();

                                        // is it different from expected?
    if ( (int) AuxTracks == 0 && oldnumaux != 0 ) {
                                        // blindly set the last electrodes
        AuxTracks.Set ( NumElectrodes - oldnumaux, NumElectrodes - 1 );
                                        // re-check & re-count
        InitAuxiliaries ();
                                        // set again more smartly from last known auxiliary
        if ( (int) AuxTracks == 0 && NumAux != oldnumaux ) {
            int i = AuxTracks.LastSelected ();
            AuxTracks.Set ( i - oldnumaux + 1, i );
            }
        }

    if ( NumAux != oldnumaux )
        SetDirty ( true );
    }

else { // creating file
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegCartoolSefDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.Resize ( TotalElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


BadTracks           = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks           = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();


return true;
}


//----------------------------------------------------------------------------
void    TEegCartoolSefDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 el;
float*              toT;

                                        // set file to first TF
FileStream.Seek ( DataOrg + BuffSize * tf1 );


for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

    FileStream.Read ( (float *) Tracks, BuffSize );

    for ( el = 0, toT = (float *) Tracks; el < NumElectrodes; el++, toT++ )

        buff[ el ][ tf ] = *toT;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
