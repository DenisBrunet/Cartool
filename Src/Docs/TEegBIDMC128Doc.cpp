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

#include    "TEegBIDMC128Doc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TEegBIDMC128Doc::TfToBlockOffset ( const int& tf, int& block, int& offset )  const
{
block   = DataOrg + ( tf / EEG128DOC_TFPERRECORD ) * sizeof ( T128FileRec );
offset  =             tf % EEG128DOC_NUMCHANNELS;
}


//----------------------------------------------------------------------------
        TEegBIDMC128Doc::TEegBIDMC128Doc (TDocument *parent)
      : TTracksDoc ( parent )
{
NumTimeFrames       = 0;
NumElectrodes       = 0;
NumAuxElectrodes    = 0;
NumMinElectrodes    = 0;
TotalElectrodes     = 0;
SamplingFrequency   = 0;
Reference           = ReferenceAsInFile;

InputStream         = 0;

DataOrg             = 0;

ClearVirtualMemory ( &FileBuff, sizeof ( T128FileRec ) );
Block               = 0;
Offset              = 0;
MarkerIndex         = 0;
}


bool	TEegBIDMC128Doc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegBIDMC128Doc::CanClose ()
{
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegBIDMC128Doc::ReadNativeMarkers ()
{
ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );
                                                // scan the whole file for the triggers
char                buff[ 256 ];
bool                on;
bool                oldon;
int                 oldBlock;
long                tfi;
char                tc;
uchar               bp;
uchar               oldbp;
bool                bpon            = false;
long                lastbp          = -1000;
uchar               notrig;


TfToBlockOffset ( 0, Block, Offset );

ifs.seekg ( Block, ios::beg );
ifs.read ( (char *) &FileBuff, sizeof ( T128FileRec ) );

oldBlock    = Block;
oldon       = FileBuff.data[ Offset ][ MarkerIndex ] > 128;
oldbp       = FileBuff.data[ Offset ][ EEG128DOC_BPCHANNEL ];
notrig      = FileBuff.data[ Offset ][ MarkerIndex ];

                                        // opportunity to get the starting date
DateTime    = TDateTime ( FileBuff.date.da_year, FileBuff.date.da_mon,  FileBuff.date.da_day,
                          FileBuff.time.hours,   FileBuff.time.minutes, FileBuff.time.seconds, FileBuff.time.hundredths * 10, 0 );
InitDateTime ();


for ( tfi=1; tfi < NumTimeFrames; tfi++ ) {

    TfToBlockOffset ( tfi, Block, Offset );

    if ( Block != oldBlock ) {          // read 1 new block
        ifs.seekg ( Block, ios::beg );
        ifs.read ( (char *) &FileBuff, sizeof ( T128FileRec ) );
        oldBlock    = Block;
        }

    on  = FileBuff.data[ Offset ][ MarkerIndex ] != notrig;

    if ( on && on != oldon ) {
                                        // this empiric formula comes from the hardware cables...
//      tc  = (char) ( ( ( ( FileBuff.data[ Offset ][ MarkerIndex ] - 106 ) & 0x3c ) << 1 ) - FileBuff.data[ Offset ][ MarkerIndex ] + 109 );
        tc  = (char) ( ( ( ( FileBuff.data[ Offset ][ MarkerIndex ] - ( 106 + notrig - 109 ) ) & 0x3c ) << 1 ) - FileBuff.data[ Offset ][ MarkerIndex ] + notrig );
        sprintf ( buff, "%0d", (int) tc );
        AppendMarker ( TMarker ( tfi, tfi, tc, buff, MarkerTypeTrigger ) );
        }

    oldon   = on;
                                        // find a button pressed ?
    bp = FileBuff.data[ Offset ][ EEG128DOC_BPCHANNEL ];

    if ( bp != oldbp ) {
        if ( !bpon ) {
            if ( tfi - lastbp > 10 ) {
                bpon    = true;
                tc  = 0;
                sprintf ( buff, "bp" );
                AppendMarker ( TMarker ( tfi, tfi, tc, buff, MarkerTypeTrigger ) );
                }
            lastbp  = tfi;
            }
        }
    else
        bpon    = false;

    oldbp   = bp;
    }
}


//----------------------------------------------------------------------------
bool	TEegBIDMC128Doc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );
if ( ifs.fail() ) return false;

char            buff[ 256 ];

switch ( what ) {

    case ReadNumElectrodes :
    case ReadNumAuxElectrodes :

        char        ver         [  50 ];
        char        channelline [ 100 ];
        char        elname      [  50 ];
        TSelection* ValidElectrodes;
        int         NumElectrodes;
        int         NumAuxElectrodes;

        ifs.getline ( buff, 256 );
        sscanf ( buff, "%*s %*s %s", ver );

        if ( StringIsNot ( ver, EEG128DOC_FILEVERSION ) ) {
            return false;
            }

        ifs.getline ( buff, 256 );
        ifs.getline ( buff, 256 );
        ifs.getline ( buff, 256 );
        sscanf ( buff, "%*s %*s %*[A-Za-z0-9].%d", &NumElectrodes );

        ValidElectrodes = new TSelection ( NumElectrodes, OrderSorted );
        ValidElectrodes->Set ();

                                            // skip lines until the electrodes description
        ifs.getline ( buff, 256 );
        do {
            ifs.getline ( buff, 256 );
            sscanf ( buff, "%s %*s %s", channelline, elname );
            }
        while ( StringIsNot ( channelline, "Channel" ) );

                                            // read el names, test validity
        int         e;
        e                   = 0;
        NumAuxElectrodes    = 0;

        while ( StringIs ( channelline, "Channel" ) ) {

            if ( StringIs ( elname, "/off/" ) ||
                 StringIs ( elname, "ecg" ) )
                ValidElectrodes->Reset ( e );

            if ( StringIs ( elname, EEG128DOC_TAGNAME ) ) {
                ValidElectrodes->Reset ( e );
                }

            if ( StringIs ( elname, "eog1" ) ||  // should be at the end
                 StringIs ( elname, "eog2" ) )
                NumAuxElectrodes++;

            ifs.getline ( buff, 256 );
            sscanf ( buff, "%s %*s %s", channelline, elname );
            e++;
            }

        NumElectrodes       = ValidElectrodes->NumSet();

        if ( what == ReadNumElectrodes )
            *((int *) answer)   = NumElectrodes;
        else if ( what == ReadNumAuxElectrodes )
            *((int *) answer)   = NumAuxElectrodes;

        delete  ValidElectrodes;
        return  true ;

    case ReadSamplingFrequency :
        *((double *) answer)= EEG128DOC_SAMPLFREQUENCY;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegBIDMC128Doc::Open	(int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );

if ( GetDocPath () ) {

    InputStream     = InStream ( ofRead );
    if ( !InputStream ) return false;

    char        buff       [ 256 ];
    char        ver        [  16 ];
    char        channelline[  33 ];
    char        elname     [  16 ];

    SamplingFrequency   = EEG128DOC_SAMPLFREQUENCY;

    InputStream->getline ( buff, 256 );
    sscanf ( buff, "%*s %*s %s %ld", ver, &DataOrg );

    if ( StringIsNot ( ver, EEG128DOC_FILEVERSION ) ) {
        ShowMessage ("Unrecognized file format!", "Open File", ShowMessageWarning );
        delete  InputStream; InputStream = 0;
        return false;
        }

    InputStream->getline ( buff, 256 );
    InputStream->getline ( buff, 256 );
    InputStream->getline ( buff, 256 );
    sscanf ( buff, "%*s %*s %*[A-Za-z0-9].%d", &NumElectrodes );

    ValidElectrodes = TSelection ( NumElectrodes, OrderSorted );
    ValidElectrodes.Set ();

                                        // skip lines until the electrodes description
    InputStream->getline ( buff, 256 );
    do {
        InputStream->getline ( buff, 256 );
        sscanf ( buff, "%s %*s %s", channelline, elname );
        }
    while ( StringIsNot ( channelline, "Channel" ) );

                                        // read el names, test validity
    int                 e       = 0;
    int                 ev;

    TStrings            allnames ( NumElectrodes, ElectrodeNameSize );

    NumAuxElectrodes    = 0;

    while ( StringIs ( channelline, "Channel" ) ) {

        StringCopy ( allnames[ e ], elname );     // save the name

        if ( StringIs ( elname, "/off/" ) ||
             StringIs ( elname, "ecg" ) )
            ValidElectrodes.Reset ( e );

        if ( StringIs ( elname, EEG128DOC_TAGNAME ) ) {
            ValidElectrodes.Reset ( e );
//            NumAuxElectrodes++; // remove this
            MarkerIndex     = e;
            }

        if ( StringIs ( elname, "eog1" ) ||  // should be at the end
             StringIs ( elname, "eog2" ) )
            NumAuxElectrodes++;

        InputStream->getline ( buff, 256 );
        sscanf ( buff, "%s %*s %s", channelline, elname );
        e++;
        }

    NumElectrodes       = ValidElectrodes.NumSet();
    NumMinElectrodes    = NumElectrodes - NumAuxElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;

                                        // fill product info
    StringCopy ( CompanyName, "Beth Israel Deaconess Medical Center" );
    StringCopy ( ProductName, FILEEXT_EEG128 );
    StringCopy ( VersionName, ver );
    Subversion          = NumElectrodes;

                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }
                                        // copy only the names to keep
    for ( TIteratorSelectedForward seli ( ValidElectrodes ); (bool) seli && seli.GetIndex () < NumElectrodes; ++seli )
        StringCopy ( ElectrodesNames[ seli.GetIndex () ], allnames[ seli() ] );
                                        // check for auxilliary
    InitAuxiliaries ();

                                        // compute the # of TF
    InputStream->seekg ( 0, ios::end );
    NumTimeFrames       = ((int) ( (ulong) InputStream->tellg() - DataOrg ) / sizeof ( T128FileRec ) ) * EEG128DOC_TFPERRECORD ;

    delete  InputStream;
    InputStream     = InStream ( ofRead | ofBinary );
    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegBIDMC128Doc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegBIDMC128Doc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
TIteratorSelectedForward seli;
int                 oldBlock        = -1;


for ( int tfi = tf1, tf0 = 0; tfi <= tf2; tfi++, tf0++ ) {

    TfToBlockOffset ( tfi, Block, Offset );

    if ( Block != oldBlock ) {              // read 1 new block
        InputStream->seekg ( Block, ios::beg );
        InputStream->read ( (char *) &FileBuff, sizeof ( T128FileRec ) );
        oldBlock    = Block;
        }


    seli.FirstSelected ( ValidElectrodes );
    uchar              *toFB        = FileBuff.data[ Offset ];

    for ( int el = 0; el < NumElectrodes /*&& (bool) seli*/; el++, ++seli )

        buff ( el, tfoffset + tf0 )    = ( toFB[ seli() ] - EEG128DOC_ZERO ) * EEG128DOC_SCALE;
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
