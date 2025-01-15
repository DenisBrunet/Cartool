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

#include    "TXyzDoc.h"

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
        TXyzDoc::TXyzDoc ( TDocument *parent )
      : TElectrodesDoc ( parent )
{
}


bool    TXyzDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


if ( ! IsExtension ( GetDefaultExtension () ) ) {

    if ( ! GetAnswerFromUser ( "Do you really want to save this document to another format?\n(some informations might be either lost or missing in the new format)\n\nIf you proceed with this operation, it is advised to close and reopen the new file.", "Save As" ) )
        return  true;
    }

                                        // we nicely follow the input names
GetPoints ( DisplaySpace3D ).WriteFile ( (char *) GetDocPath (), &ElectrodesNames, 0, NumElectrodes );


SetDirty ( false );
return true;
}


//----------------------------------------------------------------------------
void    TXyzDoc::ExtractToFile ( const char* xyzfile, TSelection selection, bool removeselection )  const
{
if ( ! removeselection ) {
                                        // code has been written for removal...
    selection.Invert ();
                                        // make sure to not set pseudo-electrodes
    if ( selection.Size () > NumElectrodes )
        selection.Reset ( NumElectrodes, selection.Size () - 1 );
    }


ofstream            os ( TFileName ( xyzfile, TFilenameExtendedPath ) );
const TPoints&      sop             = GetPoints ( DisplaySpace3D );


os << StreamFormatGeneral;
os << StreamFormatInt32   << ( NumElectrodes - selection.NumSet () ) << Tab;
os << StreamFormatFloat32 << GetBounding ( 0 )->Radius ();
os << NewLine;


os << StreamFormatFixed;

for ( int e = 0; e < NumElectrodes; e++ ) {

    if ( selection[ e ] )
        continue;

    os << StreamFormatFloat32 << sop[ e ].X << Tab;
    os << StreamFormatFloat32 << sop[ e ].Y << Tab;
    os << StreamFormatFloat32 << sop[ e ].Z << Tab;
    os << StreamFormatText    << ElectrodesNames[ e ];
    os << NewLine;
    }
}


//----------------------------------------------------------------------------
bool    TXyzDoc::Revert (bool clear)
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


bool    TXyzDoc::Close ()
{
if ( IsOpen () )
    Reset ( true );

NumElectrodes = 0;

return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
bool	TXyzDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail() ) return false;

char                buff[ 256 ];
ifs.getline (buff, 256);

switch ( what ) {
    case ReadNumElectrodes :
        sscanf ( buff, "%d", (int *) answer );
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool    TXyzDoc::Open ( int mode, const char* path )
{
if ( path )
    SetDocPath ( path );


SetDirty ( false );


if ( GetDocPath () ) {

    ifstream            is ( GetDocPath () );

    if ( is.fail () )
        return false;


    char                buff[ 1024 ];

    is.getline ( buff, 256 );

    NumElectrodes   = StringToInteger ( buff );

    if ( NumElectrodes <= 0 )
        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Variables allocation
    SetArrays ( 1, NumElectrodes );

                                        // electrodes positions
    TPoints&            sop         = GetPoints ( DisplaySpace3D );
    char                elname[ 256 ];

                                        // read file
    for ( int i = 0; i < NumElectrodes; i++ ) {

    	is.getline      ( buff, 256 );

        StringAppend    ( buff, " ", ElectrodeDefaultName );
                                                // extract data
        sscanf          ( buff, "%f %f %f %s", &sop[ i ].X, &sop[ i ].Y, &sop[ i ].Z, elname );

        ElectrodesNames.Add ( elname );
        }


    is.close ();


    ElectrodesNames.Add ( TrackNameGFP );
    ElectrodesNames.Add ( TrackNameDIS );
    ElectrodesNames.Add ( TrackNameAVG );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no names? -> provide default numbering
    if ( StringIs ( ElectrodesNames[ 0 ], ElectrodeDefaultName ) )

        for ( int i = 0; i < NumElectrodes; i++ )
                                        // just numbers - simpler but inconsistent with other parts in Cartool
            IntegerToString ( ElectrodesNames[ i ], i + 1 );
                                        // more consistent but ugly
//          StringCopy  ( ElectrodesNames[ i ], "e",  IntegerToString ( buff, i + 1, 0 ) );


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
            StringCopy  ( buff, IntegerToString ( numremoved ), " electrode", numremoved == 1 ? " has" : "s have", " a position which overlap another electrode:" NewLine NewLine Tab, elpairs, NewLine NewLine "Trying to proceed anyway..." );
        else
            StringCopy  ( buff, IntegerToString ( numremoved ), " electrode", numremoved == 1 ? " has" : "s have", " a position which overlap another electrode!" NewLine NewLine "Trying to proceed anyway..." );

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

void    TXyzDoc::SetArrays ( int numclusters, int numelectrodes )
{
NumClusters     = numclusters;
NumElectrodes   = numelectrodes;


Clusters                    = new TElectrodesCluster [ NumClusters ];

Clusters[ 0 ].Type          = Cluster3D;
Clusters[ 0 ].NumElectrodes = NumElectrodes;
ClearString ( Clusters[ 0 ].Name, ElectrodesClusterMaxName );


ClusterIndexes              = new int [ NumElectrodes ];

for ( int i = 0; i < NumElectrodes; i++ )

    ClusterIndexes[ i ] = 0;

                                    // default bad tracks=none
BadElectrodes   = TSelection ( NumElectrodes, OrderSorted );
BadElectrodes.Reset ();

                                    // electrodes positions
GetPoints ( DisplaySpace3D ).Resize ( NumElectrodes + NumPseudoTracks );


ElectrodesNames.Reset ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
