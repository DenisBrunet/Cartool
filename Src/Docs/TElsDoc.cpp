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

#include    "TElsDoc.h"

#include    "Files.Stream.h"
#include    "Math.Utils.h"
#include    "Dialogs.Input.h"

#include    "TTracksDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TElsDoc::TElsDoc ( TDocument *parent )
      : TElectrodesDoc ( parent )
{
}


bool    TElsDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


                                        // downgrading the els format?
if ( ! IsExtension ( GetDefaultExtension () ) ) {

    if ( ! GetAnswerFromUser ( "Do you really want to save this document to another format?\n(some informations might be either lost or missing in the new format)\n\nIf you proceed with this operation, it is advised to close and reopen the new file.", "Save As" ) )
        return  true;

                                        // we nicely follow the input names
    GetPoints ( DisplaySpace3D ).WriteFile ( (char *) GetDocPath (), &ElectrodesNames, 0, NumElectrodes );


    SetDirty ( false );
    return true;
    }


                                        // could be using WritePoints function instead?
ofstream            os ( TFileName ( GetDocPath (), TFilenameExtendedPath ) );
TPoints&            sop             = GetPoints ( DisplaySpace3D );
int                 el              = 0;


os << StreamFormatGeneral;
os << StreamFormatText    << ELSTXT_MAGICNUMBER1 << NewLine;
os << StreamFormatInt32   << NumElectrodes       << NewLine;
os << StreamFormatInt32   << NumClusters         << NewLine;


for ( int c = 0; c < NumClusters; c++ ) {

    os << StreamFormatGeneral;
    os << StreamFormatText    << Clusters[c].Name          << NewLine;
    os << StreamFormatInt32   << Clusters[c].NumElectrodes << NewLine;
    os << StreamFormatInt32   << Clusters[c].Type          << NewLine;


    os << StreamFormatFixed;

    for ( int e = 0; e < Clusters[ c ].NumElectrodes; e++, el++ ) {

        os << StreamFormatFloat32 << sop[ el ].X << Tab;
        os << StreamFormatFloat32 << sop[ el ].Y << Tab;
        os << StreamFormatFloat32 << sop[ el ].Z << Tab;
        os << StreamFormatText    << ElectrodesNames[ el ];
        if ( BadElectrodes[ el ] )
            os << Tab << StreamFormatText << "BAD";
        os << NewLine;
        }
    }


SetDirty ( false );

return true;
}


//----------------------------------------------------------------------------
void    TElsDoc::ExtractToFile ( const char* xyzfile, TSelection selection, bool removeselection )  const
{
if ( ! removeselection ) {
                                        // code has been written for removal...
    selection.Invert ();
                                        // make sure to not set pseudo-electrodes
    if ( selection.Size () > NumElectrodes )
        selection.Reset ( NumElectrodes, selection.Size () - 1 );
    }

                                        // set # electrodes / cluster, then # of remaining clusters
TArray1<int>        clu ( NumClusters );
int                 numclufinal     = 0;


for ( int e = 0; e < NumElectrodes; e++ )
    if ( ! selection[ e ] )
        clu[ ClusterIndexes[ e ] ]++;

for ( int c = 0; c < NumClusters; c++ )
    if ( clu[ c ] )
        numclufinal++;
                                        // try to merge any single electrode
                                        // remaining after a 3D set, as it is very likely to be the ref
bool                mergeref        = false;
int                 c1              = -1;
int                 c2              = -1;

// a better way could be to merge consecutive clusters of same names/same dimensionality

if ( numclufinal == 2 ) {
                                        // get the 2 clusters
    for ( int c = 0; c < NumClusters; c++ )
        if ( clu[ c ] ) {
            c1  = c;
            break;
            }

    for ( int c = c1 + 1; c < NumClusters; c++ )
        if ( clu[ c ] ) {
            c2  = c;
            break;
            }

    if (   Clusters[ c1 ].Type == Cluster3D
//    && ( Clusters[ c2 ].Type == ClusterPoint || Clusters[ c2 ].Type == Cluster3D )
      && clu[ c2 ] == 1 ) {
        mergeref    = true;

        clu[ c1 ]++;                    // trick these numbers
//      clu[ c2 ] = 0;
        numclufinal--;
        }
    }


ofstream            os ( TFileName ( xyzfile, TFilenameExtendedPath ) );
const TPoints&      sop             = GetPoints ( DisplaySpace3D );
int                 el              = 0;


os << StreamFormatGeneral;
os << StreamFormatText    << ELSTXT_MAGICNUMBER1                     << NewLine;
os << StreamFormatInt32   << ( NumElectrodes - selection.NumSet () ) << NewLine;
os << StreamFormatInt32   << numclufinal                             << NewLine;


for ( int c = 0; c < NumClusters; c++ ) {
                                        // now an empty cluster?
    if ( ! clu[ c ] ) {
        el     += Clusters[ c ].NumElectrodes;
        continue;                       // skip it
        }


    if ( ! mergeref || c == c1 ) {
        os << StreamFormatGeneral;
        os << StreamFormatText    << Clusters[c].Name << NewLine;
        os << StreamFormatInt32   << clu[ c ]         << NewLine;
        os << StreamFormatInt32   << Clusters[c].Type << NewLine;  // there should be a check for the type consistency?
        }


    os << StreamFormatFixed;

    for ( int e = 0; e < Clusters[ c ].NumElectrodes; e++, el++ ) {

        if ( selection[ el ] )
            continue;

        os << StreamFormatFloat32 << sop[ el ].X << Tab;
        os << StreamFormatFloat32 << sop[ el ].Y << Tab;
        os << StreamFormatFloat32 << sop[ el ].Z << Tab;
        os << StreamFormatText    << ElectrodesNames[ el ];
        if ( BadElectrodes[ el ] )
            os << Tab << StreamFormatText << "BAD";
        os << NewLine;
        }
    }
}


//----------------------------------------------------------------------------
bool    TElsDoc::Revert (bool clear)
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;


if ( ! clear ) {
                                        // a call to the destructor
    Close ();

    Open ( ofRead );
    }

                                        // restore original
NotifyDocViews ( vnNewBadSelection, (TParam2) &BadElectrodes );

return true;
}


bool    TElsDoc::Close ()
{
if ( IsOpen () )
    Reset ( true );

NumElectrodes = 0;

return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
bool	TElsDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail() ) return false;

char        buff[ KiloByte ];

ifs.getline ( buff, KiloByte );
if ( ! StringStartsWith ( buff, ELSTXT_MAGICNUMBER1, 4 ) )
    return  false;


switch ( what ) {
    case ReadNumElectrodes :
        ifs.getline ( buff, KiloByte );
        sscanf ( buff, "%d", (int *) answer );
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool    TElsDoc::Open (int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    TInStream*          is              = InStream ( ofRead );

    if ( !is || is->fail() )    return false;

    char    buff[ 1024 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read magic number
    is->getline ( buff, 128 );
    if ( ! StringStartsWith ( buff, ELSTXT_MAGICNUMBER1 ) ) {
        delete is;
        return false;
        }

    is->getline ( buff, 128 );
    sscanf ( buff, "%d", &NumElectrodes );

    if ( NumElectrodes <= 0 ) {
        delete is;
        return false;
        }

    is->getline ( buff, 128 );
    sscanf ( buff, "%d", &NumClusters );

    if ( NumClusters <= 0 ) {
        delete is;
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // bad tracks
    BadElectrodes   = TSelection ( NumElectrodes, OrderSorted );
    BadElectrodes.Reset();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cluster setup
    Clusters        = new TElectrodesCluster [ NumClusters ];

    ClusterIndexes  = new int [ NumElectrodes ];

                                        // electrodes positions
    TPoints&        sop = GetPoints ( DisplaySpace3D );
    sop.Resize ( NumElectrodes + NumPseudoTracks );


    ElectrodesNames.Set ( NumElectrodes + NumPseudoTracks, ElectrodeNameSize );

    StringCopy ( ElectrodesNames[ NumElectrodes     ], TrackNameGFP );
    StringCopy ( ElectrodesNames[ NumElectrodes + 1 ], TrackNameDIS );
    StringCopy ( ElectrodesNames[ NumElectrodes + 2 ], TrackNameAVG );


    int                 numel;
    int                 insert          = 0;
    char                flag  [  32 ];
    int                 n;
                                        // loop through the clusters
    for ( int c = 0; c < NumClusters; c++ ) {
                                        // cluster name
        is->getline ( Clusters[ c ].Name, ElectrodesClusterMaxName );
                                        // # of electrodes
        is->getline ( buff, 128 );
        sscanf ( buff, "%d", &numel );
        Clusters[ c ].NumElectrodes = numel;
                                        // cluster type
        is->getline ( buff, 128 );
        sscanf ( buff, "%d", &Clusters[ c ].Type );

                                        // read them
        for ( int i = 0; i < numel; i++, insert++ ) {
                                        // set cluster
            ClusterIndexes[ insert ] = c;

            is->getline ( buff, 128 );
                                        // extract data
            n = sscanf ( buff, "%f %f %f %s %s", &sop[ insert ].X, &sop[ insert ].Y, &sop[ insert ].Z,
                                                 ElectrodesNames[ insert ], flag );
                                                 
//            DBGV3 ( sop[ insert ].X, sop[ insert ].Y, sop[ insert ].Z, ElectrodesNames[ insert ] );

                                        // a flag after data?
            if ( n > 4 )
                if ( StringIs ( flag, "BAD" ) )
                    BadElectrodes.Set ( insert );
            }
        }

    delete is;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no names? -> provide default numbering
    if ( StringIs ( ElectrodesNames[ 0 ], ElectrodeDefaultName ) )

        for ( int i = 0; i < NumElectrodes; i++ )

            IntegerToString ( ElectrodesNames[ i ], i + 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for electrodes with same position
    int                 numremoved      = 0;
    double              d;
    double              radius          = NonNull ( sop.GetRadius () );
    char                elpairs[ 1024 ];

    ClearString ( elpairs );

    for ( int i = 0; i < NumElectrodes; i++ ) {
                                        // remove it if same position!
        for ( int j = i + 1; j < NumElectrodes; j++ ) {

            d   = ( sop[ j ] - sop[ i ] ).Norm () / radius;

            if ( d  < 1e-6 ) {
//              insert--;               // in fact don't remove
                numremoved++;
                StringAppend ( elpairs, StringIsEmpty ( elpairs ) ? "" : "  ", ElectrodesNames[ i ], "/", ElectrodesNames[ j ] );
                break;
                }
            }
        }


//  NumElectrodes   -= numremoved;

    if ( numremoved ) {
        if ( numremoved <= 32 )
            sprintf ( buff, "%0d electrode(s) have a position which overlap another electrode:\n\n\t%s\n\nTrying to proceed anyway...", numremoved, elpairs );
        else
            sprintf ( buff, "%0d electrode(s) have a position which overlap another electrode!\n\nTrying to proceed anyway...", numremoved );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        }

    }
else {
    return false;
    }

                                        // compute everything (projection, triangulation...)
Set ();


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
