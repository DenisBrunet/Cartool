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

#include    <owl/pch.h>

#include    "TSegDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
       TSegDoc::TSegDoc ( TDocument *parent )
     : TTracksDoc ( parent )
{
NumClusters         = 0;
NumFiles            = 0;
NumVars             = 0;
}


//----------------------------------------------------------------------------
bool	TSegDoc::Close ()
{
//if ( IsOpen() )

return  TFileDocument::Close ();
}


bool    TSegDoc::CanClose ()
{
SetDirty ( false );                     // data haven't really changed

return  TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
bool	TSegDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail() ) return false;


char            buff[ KiloByte ];
int             numfiles;
int             numvars;


ifs.getline ( buff, KiloByte );

switch ( what ) {

    case ReadNumElectrodes :

        if ( crtl::IsExtension ( file, FILEEXT_SEG ) )
            sscanf ( buff, "%*d %d", &numfiles );
        else
            sscanf ( buff, "%d", &numfiles );

        ifs.getline ( buff, KiloByte );
        sscanf ( buff, "%d", &numvars );

        *((int *)answer) = numvars * numfiles;
        return  true;

    case ReadNumAuxElectrodes :
        *((int *)answer) = 0;
        return  true;

    case ReadNumTimeFrames :

        if ( crtl::IsExtension ( file, FILEEXT_SEG ) )
            sscanf ( buff, "%*d %*d %d", (int *) answer );
        else
            sscanf ( buff, "%*d %d", (int *) answer );

        return  true;

    case ReadNumClusters :

        if ( crtl::IsExtension ( file, FILEEXT_SEG ) )
            sscanf ( buff, "%d", (int *) answer );
        else
            return  false;

        return  true;

    case ReadSamplingFrequency :
        *((double *)answer) = 0;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TSegDoc::Open ( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath() ) {

    ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ) );

    if ( ifs.fail() ) return false;


    char                buff[ KiloByte ];
    buff[255]   = 0;

    ifs.getline ( buff, KiloByte );

    if ( IsExtension ( FILEEXT_SEG ) )
        sscanf ( buff, "%d %d %d", &NumClusters, &NumFiles, &NumTimeFrames );
    else {
        NumClusters     = 1;
        sscanf ( buff, "%d %d", &NumFiles, &NumTimeFrames );
        }

    ifs >> NumVars;

    TStrings            names;

    for ( int i = 0; i < NumVars; i++ ) {

        names.Add ( "", StringFixedSize, 128 );

        ifs >> names[ i ];
        }

    ifs.getline ( buff, 128 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodes       = NumVars * NumFiles;
    SamplingFrequency   = 0;

                                        // space allocation + default names
    if ( ! SetArrays () )
        return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( NumFiles > 1 )

        for ( int i = 0; i < NumElectrodes; i++ )
            StringCopy  ( ElectrodesNames[ i ], names[ i % NumVars ], IntegerToString ( ( i / NumVars ) + 1 ) );
    else

        for ( int i = 0; i < NumElectrodes; i++ )
            StringCopy  ( ElectrodesNames[ i ], names[ i % NumVars ] );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read all data at once
    for ( int tf = 0; tf < NumTimeFrames; tf++ ) 
    for ( int el = 0; el < NumElectrodes; el++ )

        Tracks ( el, tf )   = StringToDouble ( GetToken ( &ifs, buff ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set first cluster
    if ( IsExtension ( FILEEXT_SEG ) )

        StartingTimeFrame   = 0;
    else {
        NumClusters         = 1;

        StartingTimeFrame   = 1;

        for ( int i = 0; i < NumVars; i++ )

            if ( StringIs ( names[ 0 ], "Clust" ) ) {

                StartingTimeFrame    = Tracks ( 0 , 0 );
                break;
                }
        }
    }
else {
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TSegDoc::SetArrays ()
{
                                        // no computed tracks, no aux tracks
TotalElectrodes     = NumElectrodes;

                                        // do all allocations stuff
Tracks.Resize ( TotalElectrodes, NumTimeFrames );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "s", IntegerToString ( i ) );


BadTracks   = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();


return true;
}


//----------------------------------------------------------------------------
void    TSegDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
long                tf;
float*              toT;
float*              b;

for ( int el = 0; el < TotalElectrodes; el++ )
for ( tf = tf1, b = buff[ el ] + tfoffset, toT = Tracks[ el ] + tf; tf <= tf2; tf++, toT++, b++ )

    *b  = *toT;
}


//----------------------------------------------------------------------------
void    TSegDoc::GetTracks  (   long                tf1,            long            tf2,
                                TArray2<float>&     buff,           int             tfoffset, 
                                AtomType            /*atomtype*/,
                                PseudoTracksType    /*pseudotracks*/,
                                ReferenceType       /*reference*/,  const TSelection*   /*referencetracks*/,
                                const TRois*        /*rois*/
                            )
{
TTracksDoc::GetTracks   (   tf1,                tf2,
                            buff,               tfoffset,
                            AtomTypeUseCurrent, 
                            NoPseudoTracks,                 // always overriding
                            ReferenceAsInFile,        0,    // always overriding
                            0
                        );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
 
}
