/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "TEegCartoolEpDoc.h"

#include    "MemUtil.h"
#include    "Strings.TFixedString.h"

#include    "TArray1.h"

#include    "TBaseView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegCartoolEpDoc::TEegCartoolEpDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
CreatingEP          = false;
}


//----------------------------------------------------------------------------
bool    TEegCartoolEpDoc::CanClose ()
{                                       // if creating an EP, no closing allowed
if ( CreatingEP )

    return  false;

else {
    SetDirty ( false );                 // data haven't really changed

    return  TTracksDoc::CanClose ();
    }
}


//----------------------------------------------------------------------------
bool	TEegCartoolEpDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );
if ( ifs.fail() ) return false;

char            buff[ KiloByte ];


if ( crtl::IsExtension  ( file, FILEEXT_EEGEP ) ) {

    switch ( what ) {

        case ReadNumElectrodes :
            *((int *)answer) = CountPerLine ( file );
            return  true;

        case ReadNumAuxElectrodes :
            *((int *)answer) = 0;
            return  true;

        case ReadNumTimeFrames :
            *((int *)answer) = CountLines ( file );
            return  true;

        case ReadSamplingFrequency :
            *((double *)answer) = 0.0;
            return  true;
        }
    }
else {
    ifs.getline ( buff, KiloByte );

    switch ( what ) {

        case ReadNumElectrodes :
            sscanf ( buff, "%d", (int *) answer );
            return  true;

        case ReadNumAuxElectrodes :
            *((int *)answer) = 0;
            return  true;

        case ReadNumTimeFrames :
            sscanf ( buff, "%*d %d", (int *) answer );
            return  true;

        case ReadSamplingFrequency :
            sscanf ( buff, "%*d %*d %lf", (double *) answer );
            return  true;
        }
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegCartoolEpDoc::Open ( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    TInStream*          is              = InStream ( ofRead );

    if ( ! is )
        return  false;


    char                buff[ 256 ];

                                        // any header?
    if ( IsExtensionAmong ( FILEEXT_EEGEPH " " FILEEXT_EEGEPSD " " FILEEXT_EEGEPSE ) ) {
        is->getline ( buff, 255 );
        buff[255]   = 0;
        sscanf ( buff, "%d %ld %lf", &NumElectrodes, &NumTimeFrames, &SamplingFrequency );
        }
    else {                              // get sizes
        if ( ! ReadFromHeader ( (char *) GetDocPath(), ReadNumElectrodes, &NumElectrodes ) )
            { delete is; return false; }

        if ( ! ReadFromHeader ( (char *) GetDocPath(), ReadNumTimeFrames, &NumTimeFrames ) )
            { delete is; return false; }

        SamplingFrequency   = 0;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete is;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read all data at once
    for ( int tf = 0; tf < NumTimeFrames; tf++ )
    for ( int el = 0; el < NumElectrodes; el++ )

        Tracks ( el, tf )  = StringToDouble ( GetToken ( is, buff ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, CartoolRegistryCompany );

    if      ( IsExtension ( FILEEXT_EEGEP   ) ) StringCopy ( ProductName, FILEEXT_EEGEP   );
    else if ( IsExtension ( FILEEXT_EEGEPH  ) ) StringCopy ( ProductName, FILEEXT_EEGEPH  );
    else if ( IsExtension ( FILEEXT_EEGEPSD ) ) StringCopy ( ProductName, FILEEXT_EEGEPSD );
    else if ( IsExtension ( FILEEXT_EEGEPSE ) ) StringCopy ( ProductName, FILEEXT_EEGEPSE );
    Version             = 1;
    Subversion          = NumElectrodes;

                                        // check for auxiliaries
    InitAuxiliaries ();

    delete      is;
    }

else {                                  // new file -> choose how to create it
                                        // stand. dev. not created here!
    if ( IsExtension ( FILEEXT_EEGEPSD )
      || IsExtension ( FILEEXT_EEGEPSE ) ) {
        return false;
        }

    ShowMessage ( "Use the menu:  Tools | Averaging files", "New File" );
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return true;
}


//----------------------------------------------------------------------------
bool    TEegCartoolEpDoc::SetArrays ()
{
                                        // add the computed tracks to list of tracks
TotalElectrodes     = NumElectrodes + NumPseudoTracks;
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;


Tracks.Resize ( TotalElectrodes, NumTimeFrames );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );
                                        // regular electrodes
for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void        TEegCartoolEpDoc::SetReferenceType ( ReferenceType ref, const char* tracks, const TStrings* elnames, bool verbose )
{
if ( IsExtension ( FILEEXT_EEGEPSD )
  || IsExtension ( FILEEXT_EEGEPSE ) )  // stand. err.: no average reference

    TTracksDoc::SetReferenceType ( ReferenceAsInFile );
else
    TTracksDoc::SetReferenceType ( ref, tracks, elnames, verbose );
}


//----------------------------------------------------------------------------
void    TEegCartoolEpDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float>& buff, int tfoffset )
{
long                numtf           = tf2 - tf1 + 1;

for ( int el = 0; el < NumElectrodes; el++ )

    CopyVirtualMemory ( buff[ el ] + tfoffset, Tracks[ el ] + tf1, numtf * Tracks.AtomSize () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
