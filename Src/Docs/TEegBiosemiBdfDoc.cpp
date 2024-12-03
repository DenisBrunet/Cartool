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

#include    <owl/file.h>

#include    "MemUtil.h"

#include    "TArray1.h"

#include    "TEegBiosemiBdfDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegBiosemiBdfDoc::TEegBiosemiBdfDoc (TDocument *parent)
      : TTracksDoc (parent)
{
InputStream         = 0;

FileType            = UnknownBdfEdf;
MaxSamplesPerBlock  = 0;
BlockSize           = 0;

NumElectrodesInFile = 0;
}


bool	TEegBiosemiBdfDoc::Close()
{
if ( InputStream != 0 ) {
    delete      InputStream;
    InputStream = 0;
    }

return  TTracksDoc::Close ();
}


bool	TEegBiosemiBdfDoc::CanClose ()
{
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegBiosemiBdfDoc::ReadNativeMarkers ()
{
                                        // no Status channel?
if ( NumElectrodes == NumElectrodesInFile )
    return;

                                        // scan the whole file for the triggers
int                 BuffSize        = ChannelsSampling[ NumElectrodes ].ChannelSize;
int                 cellsize        = CellSize ( FileType );


ifstream            ifs ( TFileName ( GetDocPath(), TFilenameExtendedPath ), ios::binary );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through all blocks

                                        // goto first block, and skip regular electrodes
ifs.seekg ( DataOrg + cellsize * NumElectrodes * MaxSamplesPerBlock );
ifs.read  ( (char*) Tracks.GetArray (), BuffSize );

                                        // scan first block, and guess if we have 16 or 8 bits per trigger code
bool                triggers8       = true;
                                        //  skip 1st TF, which is often irrelevent / noise
UCHAR*              toT             = &Tracks[ 1 * cellsize ];

for ( int tfi = 1, tf0 = 1; tf0 < MaxSamplesPerBlock - 1 && tfi < NumTimeFrames; tfi++, tf0++ ) {

    toT++;

//  if ( *toT != 0xFF ) {
    if ( *toT != *( toT + cellsize ) ) {    // if not constant in time, then should contain trigger information

        triggers8   = false;
        break;
        }

    toT    += cellsize - 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 blockmax        = ( NumTimeFrames - 1 ) / MaxSamplesPerBlock;
char                triggername[ 8 ];
long                starttf         = 0;    // global, to allow final unfinished trigger test
long                l               = 0;
long                oldl            = 0;

                                        // for relative access
ifs.seekg ( DataOrg + cellsize * NumElectrodes * MaxSamplesPerBlock, ios::beg );



for ( int   block     = 0,
            firsttf   = 0; 
                block <= blockmax;
                    block++,
                    firsttf = block * MaxSamplesPerBlock ) {

                                        // goto block, and skip regular electrodes
//  ifs.seekg ( DataOrg + block * BlockSize + cellsize * NumElectrodes * MaxSamplesPerBlock, ios::beg );
                                        // move relatively
    if ( block > 0 )
        ifs.seekg ( BlockSize - BuffSize, ios::cur );

                                        // get the complete line for this electrode
                                        // assuming Status channel has the same sampling as the max channel
    ifs.read ( (char*) Tracks.GetArray (), BuffSize );


    UCHAR*              toT             = Tracks.GetArray ();

    for ( int tfi = firsttf, tf0 = 0; tf0 < MaxSamplesPerBlock && tfi < NumTimeFrames; tfi++, tf0++ ) {

        l       = *( (long *) toT );
        toT    += cellsize;

        *( ( (ushort *)&l ) + 1 )   = 0;// only bits 0 to 15 holds "trigger input"

        if ( triggers8 )                // but second byte can also be unused
            *( ( (uchar *)&l ) + 1 )    = 0;

        if ( tfi == 0 )
            oldl    = l;

//        if ( *toT & 0x1 )
//            AppendMarker ( TMarker ( tfi, tfi, (MarkerCode) 0, MarkerNameEpoch ) );
//        int speed = ( (*toT & 0x0e) >> 1 ) | ( (*toT & 0x20) >> 2 );


        if ( l != oldl )                // transition ?

            if ( l != 0 ) {             // to a non-null state?
//          if ( l && ! oldl ) {        // to a non-null state?

                        starttf = tfi;
                long    endtf   = tfi;

                IntegerToString ( triggername, l ); // old way - storing only trigger's rising edge

                AppendMarker ( TMarker ( starttf, endtf, (MarkerCode) l, triggername, MarkerTypeTrigger ) );
                }
//          else {                      // to a null state - storing the full trigger's duration
//              long    endtf   = tfi - 1;
//
//              IntegerToString ( triggername, oldl );
// 
//              AppendMarker ( TMarker ( starttf, endtf, (MarkerCode) oldl, triggername ) );
//              }


        oldl    = l;                    // store previous level
        } // for tf

    } // for block


if ( l != 0 ) {                         // reach the EOF with an unfinished trigger?
                                        // close it ourselves
    IntegerToString ( triggername, l );

    AppendMarker ( TMarker ( starttf, NumTimeFrames - 1, (MarkerCode) l, triggername, MarkerTypeTrigger ) );
    }
}


//----------------------------------------------------------------------------
bool	TEegBiosemiBdfDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ifs.fail() ) return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char            buff[ 256 ];
EdfType         FileType;
int             NumElectrodesInFile;
int             NumAuxElectrodes;
int             NumElectrodes;   
int             TotalElectrodes; 
int             MaxSamplesPerBlock;
int             BlockSize;
TArray1<TEegBdfChannel> ChannelsSampling;
double          SamplingFrequency;
long            NumTimeFrames;

                                        // file header is in fixed-length formatted text

ifs.read ( buff, 8 );                   // read identification
buff[8] = 0;

if ( buff[0] == '0' ) {                 // char '0' for edf
                                        // check for next chars...
    if ( StringStartsWith ( buff + 1, "BIOSEMI", 7 ) )

        FileType    = BiosemiEdf;
    else
        FileType    = RegularEdf;
    }
else if ( (uchar) buff[0] == 255 )      // 255 for bdf

    FileType    = BiosemiBdf;

else {

    FileType    = UnknownBdfEdf;

    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip subject, recording infos, date time, data org, data version, # of blocks
ifs.seekg ( 80 + 80 + 3 * 8 + 44, ios::cur );


ifs.read ( buff, 8 );                   // read number of blocks (of fixed size)
buff[8] = 0;
int                 numblocks       = StringToInteger ( buff ); // !can be -1 if unkown -> need to recompute it later


ifs.read ( buff, 8 );                   // block duration in seconds - could be 0 in sampling frequency is unknown
buff[8] = 0;
int                 blockduration   = StringToInteger ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ifs.read ( buff, 4 );                   // number of channels
buff[4] = 0;
NumElectrodesInFile     = StringToInteger ( buff );

                                        // prepare channel sampling array
ChannelsSampling.Resize ( NumElectrodesInFile );

                                        // see if last channel is "Status" (& triggers)
ifs.seekg ( ( NumElectrodesInFile - 1 ) * 16, ios::cur );
ifs.read ( buff, 16 );

buff[16] = 0;
StringCleanup ( buff );
//if ( JumpToChars ( buff, " " ) )
//    *JumpToChars ( buff, " " )  = 0;


if ( StringIs ( buff, "Status" ) )      // then officially ignore it
    NumElectrodes       = NumElectrodesInFile - 1;
else
    NumElectrodes       = NumElectrodesInFile;

                                        // we can then set all the others
NumAuxElectrodes    = 0;
TotalElectrodes     = NumElectrodes + NumPseudoTracks;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip variable part
ifs.seekg ( NumElectrodesInFile * 200, ios::cur );


MaxSamplesPerBlock  = 0;
                                        // number of samples per channels
for ( int el=0; el < NumElectrodesInFile; el++ ) {

    ifs.read ( buff, 8 );
    buff[8] = 0;
                                        // get # of samples
    ChannelsSampling[ el ].SamplesPerBlock  = StringToInteger ( buff );

                                        // channel size in bytes per block
    ChannelsSampling[ el ].ChannelSize = CellSize ( FileType ) * ChannelsSampling[ el ].SamplesPerBlock;

                                        // keep max # of samples
    Maxed ( MaxSamplesPerBlock, ChannelsSampling[ el ].SamplesPerBlock );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the total block size
BlockSize   = 0;

for ( int el = 0; el < NumElectrodesInFile; el++ )
    BlockSize               += ChannelsSampling[ el ].ChannelSize;

                                        // here we have the (max) sampling frequency
SamplingFrequency   = blockduration <= 0 ? 0 : MaxSamplesPerBlock / (double) blockduration;

                                        // if numblocks is unknown, compute it from known values
if ( numblocks < 0 )
    numblocks       = ( GetFileSize ( file ) - ( NumElectrodesInFile + 1 ) * 256 ) / BlockSize;

                                        // finally, we have the number of time frames
NumTimeFrames       = MaxSamplesPerBlock * numblocks;

                                        // truncate end of file if null data
NumTimeFrames      -= GetTrailingSize ( file, BlockSize, MaxSamplesPerBlock, FileType, ChannelsSampling );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)   = NumElectrodes;
        return true;

    case ReadNumAuxElectrodes :
        *((int *) answer)   = NumAuxElectrodes;
        return true;

    case ReadNumTimeFrames :
        *((int *) answer)   = NumTimeFrames;
        return true;

    case ReadSamplingFrequency :
        *((double *) answer)= SamplingFrequency;
        return true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegBiosemiBdfDoc::Open	( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath() ) {

    InputStream     = InStream ( ofRead );

    if ( ! InputStream ) 
        return  false;

    char        buff[ 256 ];
                                        // file header is in fixed-length formatted text

    InputStream->read ( buff, 8 );      // read identification
    buff[8] = 0;

    if ( buff[0] == '0' ) {             // char '0' for edf
                                        // check for next chars...
        if ( StringStartsWith ( buff + 1, "BIOSEMI", 7 ) )

            FileType    = BiosemiEdf;
        else
            FileType    = RegularEdf;
        }

    else if ( (uchar) buff[0] == 255 )  // 255 for bdf

        FileType    = BiosemiBdf;

    else {

        FileType    = UnknownBdfEdf;

        ShowMessage ( "Unrecognized file format (unknown magic number)!", "Open file", ShowMessageWarning );

        delete  InputStream;
        InputStream = 0;

        return  false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip subject and recording infos
    InputStream->seekg ( 80 + 80, ios::cur );


    InputStream->read ( buff, 8 );      // read date
    buff[8] = 0;
    int                 dd;
    int                 MM;
    int                 yy;
    sscanf ( buff, "%2d.%2d.%2d", &dd, &MM, &yy );
    if ( yy < 91 )  yy = 2000 + yy;     // !format exist since 1991!
    else            yy = 1900 + yy;


    InputStream->read ( buff, 8 );      // read time
    buff[8] = 0;
    int                 hh;
    int                 mm;
    int                 ss;
    sscanf ( buff, "%2d.%2d.%2d", &hh, &mm, &ss );
                                        // !there are NO milliseconds, and much less microseconds in this format!
    DateTime            = TDateTime ( yy, MM, dd, hh, mm, ss, 0, 0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InputStream->read ( buff, 8 );      // read header size
    buff[8] = 0;
    DataOrg     = StringToInteger ( buff );


    InputStream->read ( buff, 44 );     // read data version
    buff[44] = 0;
//  DBGM ( buff, "44 bytes ID" );
                                        // can be more precise on file type?
    if      ( StringStartsWith ( buff, "EDF+C", 5 ) )

        FileType = EdfPlusCont;

    else if ( StringStartsWith ( buff, "EDF+D", 5 ) )

        FileType = EdfPlusDisc;

                                        // fill product info
    if      ( FileType == BiosemiBdf ) {

        StringCopy ( CompanyName, "BioSemi" );
        StringCopy ( ProductName, FILEEXT_EEGBDF );
        sscanf ( buff, "%s", VersionName );
        }
    else if ( FileType == BiosemiEdf ) {

        StringCopy ( CompanyName, "BioSemi" );
        StringCopy ( ProductName, FILEEXT_EEGEDF );
        sscanf ( buff, "%s", VersionName );
        }
    else if ( FileType == RegularEdf ) {

        StringCopy ( CompanyName, CartoolRegistryCompany );
        StringCopy ( ProductName, FILEEXT_EEGEDF );
        Version             = 1;
        }
    else if ( FileType == EdfPlusCont ) {

        StringCopy ( CompanyName, CartoolRegistryCompany );
        StringCopy ( ProductName, FILEEXT_EEGEDF );
        Version             = 1;
        }
    else if ( FileType == EdfPlusDisc ) {

        StringCopy ( CompanyName, CartoolRegistryCompany );
        StringCopy ( ProductName, FILEEXT_EEGEDF );
        Version             = 1;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InputStream->read ( buff, 8 );      // read number of blocks (of fixed size)
    buff[8] = 0;
    int                 numblocks       = StringToInteger ( buff ); // !can be -1 if unkown -> need to recompute it later


    InputStream->read ( buff, 8 );      // block duration in seconds - could be 0 in sampling frequency is unknown
    buff[8] = 0;
    int                 blockduration   = StringToInteger ( buff );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InputStream->read ( buff, 4 );      // number of channels
    buff[4] = 0;
    NumElectrodesInFile     = StringToInteger ( buff );

                                        // prepare channel sampling array
    ChannelsSampling.Resize ( NumElectrodesInFile );

                                        // beginning of variable part
    long    variableheader = InputStream->tellg ();
//    DBGV ( InputStream->tellg (), "Var Part begin" );

                                        // see if last channel is "Status" (& triggers)
    InputStream->seekg ( ( NumElectrodesInFile - 1 ) * 16, ios::cur );
    InputStream->read ( buff, 16 );
//    DBGV ( InputStream->tellg (), "Read Status name" );

    buff[16] = 0;
    StringCleanup ( buff );
//    if ( JumpToChars ( buff, " " ) )
//        *JumpToChars ( buff, " " )  = 0;

    if ( StringIs ( buff, "Status" ) )  // then officially ignore it
        NumElectrodes       = NumElectrodesInFile - 1;
    else
        NumElectrodes       = NumElectrodesInFile;

//  NumElectrodes       = NumElectrodesInFile; // uncomment to actually see the Status

                                        // we can then set all the others
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;

                                        // fill product info
    Subversion          = NumElectrodesInFile;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip variable part
//  InputStream->seekg ( NumElectrodesInFile * 216, ios::cur );
//  InputStream->seekg ( NumElectrodesInFile * 200, ios::cur );

                                        // previous formula screw up for 1 + 1 electrodes??
    InputStream->seekg ( variableheader + NumElectrodesInFile * 16 + NumElectrodesInFile * 200 );
//    DBGV ( InputStream->tellg (), "Channel Samples" );


    MaxSamplesPerBlock  = 0;
                                        // number of samples per channels
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 8 );
        buff[8] = 0;
                                        // get # of samples
        ChannelsSampling[ el ].SamplesPerBlock  = StringToInteger ( buff );

                                        // channel size in bytes per block
        ChannelsSampling[ el ].ChannelSize = CellSize ( FileType ) * ChannelsSampling[ el ].SamplesPerBlock;

                                        // keep max # of samples
        Maxed ( MaxSamplesPerBlock, ChannelsSampling[ el ].SamplesPerBlock );
        }

                                        // compute the total block size
    BlockSize               = 0;
    for ( int el = 0; el < NumElectrodesInFile; el++ ) {
        BlockSize               += ChannelsSampling[ el ].ChannelSize;
        }

                                        // here we have the max sampling frequency
    SamplingFrequency   = blockduration <= 0 ? 0 : MaxSamplesPerBlock / (double) blockduration;

                                        // if numblocks is unknown, compute it from known values
    if ( numblocks < 0 )
        numblocks       = ( GetFileSize ( GetDocPath () ) - ( NumElectrodesInFile + 1 ) * 256 ) / BlockSize;

                                        // finally, we have the number of time frames
    NumTimeFrames       = MaxSamplesPerBlock * numblocks;

                                        // truncate end of file if null data
    NumTimeFrames  -= GetTrailingSize ( GetDocPath (), BlockSize, MaxSamplesPerBlock, FileType, ChannelsSampling );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {

        delete  InputStream;
        InputStream = 0;

        return  false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // go back to beginning of variable part
    InputStream->seekg ( variableheader, ios::beg );

                                        // read electrode names
    for ( int el = 0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 16 );

        buff[ 16 ] = 0;
        buff[ ElectrodeNameSize - 1 ] = 0;

        if ( el >= NumElectrodes )  continue;

        sscanf ( buff, "%s", ElectrodesNames[ el ] );
        }

                                        // EDF files might have some very badly defined tracks names
    CheckElectrodesNamesDuplicates ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip transducer type
    InputStream->seekg ( NumElectrodesInFile * 80, ios::cur );

                                        // skip units
    InputStream->seekg ( NumElectrodesInFile * 8, ios::cur );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TArray1<double>     physmin     ( NumElectrodesInFile );
    TArray1<double>     physmax     ( NumElectrodesInFile );
    TArray1<double>     digitalmin  ( NumElectrodesInFile );
    TArray1<double>     digitalmax  ( NumElectrodesInFile );

                                        // read physical min
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 8 );
        buff[8] = 0;

        physmin[ el ]       = StringToDouble ( buff );
        }
                                        // read physical max
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 8 );
        buff[8] = 0;

        physmax[ el ]       = StringToDouble ( buff );
        }
                                        // read recorded min
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 8 );
        buff[8] = 0;

        digitalmin[ el ]    = StringToDouble ( buff );
        }
                                        // read recorded max
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 8 );
        buff[8] = 0;

        digitalmax[ el ]    = StringToDouble ( buff );
        }

                                        // compute gains and offsets
    for ( int el=0; el < NumElectrodes; el++ ) {

        double          ratio       =         ( physmax   [ el ] - physmin   [ el ] ) 
                                    / NonNull ( digitalmax[ el ] - digitalmin[ el ] );

        Gains  [ el ]   = ratio;
        Offsets[ el ]   = physmin[ el ] - digitalmin[ el ] * ratio;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan for channel type
    InputStream->seekg ( variableheader + NumElectrodesInFile * 16 );
                                        //
    for ( int el=0; el < NumElectrodesInFile; el++ ) {

        InputStream->read ( buff, 80 );
        buff[ 80 ] = 0;

        if      ( StringStartsWith ( buff, "Auxiliar" ) )    AuxTracks.Set   ( el );
        else if ( StringStartsWith ( buff, "Computed" ) )    AuxTracks.Set   ( el );
        else if ( StringStartsWith ( buff, "Status"   ) )    AuxTracks.Set   ( el );
        else if ( StringStartsWith ( buff, "Regular"  ) )    AuxTracks.Reset ( el );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InitAuxiliaries ();

                                        // re-open in binary mode
    delete  InputStream;

    InputStream     = InStream ( ofRead | ofBinary );

    if ( !InputStream ) return false;

    }
else {
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
                                        // EDF number of timae frames is always multiples of MaxSamplesPerBlock
                                        // however, actual duration might have been smaller and was then padded with 0's
                                        // This method will scan the last block of data and detect the size of padded physical (near) 0's
int     TEegBiosemiBdfDoc::GetTrailingSize ()
{
                                        // !does not expect nor search for clipped end of file for BDF files!
if ( IsBdf ( FileType ) )
    return  0;


TTracks<float>      eegbuff ( NumElectrodes, MaxSamplesPerBlock );

                                        // NumTimeFrames might already have been truncated, so find it back if needed
int                 lastfiletf      = RoundToAbove ( NumTimeFrames, MaxSamplesPerBlock ); // NumTimeFrames

                                        // no need to read more than a single block
ReadRawTracks ( lastfiletf - MaxSamplesPerBlock, lastfiletf - 1, eegbuff );

                                        // there should be at least 1 non-null TF(?)
for ( int tf0 = MaxSamplesPerBlock - 1; tf0 > 0; tf0-- )
for ( int e = 0; e  < NumElectrodes; e++ )
                                        // testing exact 0 does not work due to the EDF conversion method
    if ( abs ( eegbuff ( e, tf0 ) ) > SingleFloatEpsilon )

        return  MaxSamplesPerBlock - 1 - tf0;

                                        // error, just ignore
return  0;
}

                                        // This method will scan the last block of data and detect the size of constant digital values
int     TEegBiosemiBdfDoc::GetTrailingSize ( const char* file, int BlockSize, int MaxSamplesPerBlock, EdfType FileType, const TArray1<TEegBdfChannel>& ChannelsSampling )
{
                                        // !does not expect nor search for clipped end of file for BDF files!
if ( IsBdf ( FileType ) )
    return  0;


int                 NumElectrodes   = ChannelsSampling.GetDim (); // number of electrodes in file
int                 cellsize        = CellSize ( FileType );
TArray1<UCHAR>      Tracks ( MaxSamplesPerBlock * cellsize );
short*              todata          = (short*) Tracks.GetArray ();

                                        // this should be a proble, even if file is already open somewhere else
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

if ( ifs.fail () )
    return  0;
                                        // this is the trick: we know data is written as entire blocks, so we can simply read backward the last block
ifs.seekg ( -BlockSize , ios::end );


for ( int e = 0; e < NumElectrodes; e++ ) {
                                        // read current channel data
    ifs.read ( (char*) Tracks.GetArray (), ChannelsSampling[ e ].ChannelSize );

    int         samplesperblock     = ChannelsSampling[ e ].SamplesPerBlock;

                                        // early test
    if ( todata[ samplesperblock - 2 ] != todata[ samplesperblock - 1 ] )
        return  0;


    short       lastdata            = todata[ samplesperblock - 1 ];

    for ( int tf0 = samplesperblock - 2; tf0 > 0; tf0-- ) {

        //double  v       = *toS * Gains[ e ] + Offsets[ e ];
                                        // this is another trick: we simply look for EQUAL DIGITAL VALUES, NOT any physical close-to-0 ones
        if ( todata[ tf0 ] != lastdata )
                                        // !formula does not work if tf0 == samplesperblock - 2, hence the early test!
            return  Round ( ( ( samplesperblock - tf0 - 1 ) / (double) ( MaxSamplesPerBlock - 1 ) ) * ( samplesperblock - 1 ) );
        }
    }

                                        // error, just ignore
return  0;
}


//----------------------------------------------------------------------------
bool    TEegBiosemiBdfDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;


Tracks .Resize ( CellSize ( FileType ) * MaxSamplesPerBlock + sizeof ( long ) );    // little extra for safety, when reading the last long
Gains  .Resize ( NumElectrodes );
Offsets.Resize ( NumElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


BadTracks   = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();


return true;
}


//----------------------------------------------------------------------------
void    TEegBiosemiBdfDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 blockmax        = tf2 / MaxSamplesPerBlock;
double              mspb1           = MaxSamplesPerBlock - 1;   // !converted to double!


                                        // loop through all blocks
for ( int   block           = tf1 / MaxSamplesPerBlock,
            firsttfinblock  = tf1 % MaxSamplesPerBlock,
            firsttf         = tf1;   
                block <= blockmax;
                    block++,
                    tfoffset       += MaxSamplesPerBlock - firsttfinblock,
                    firsttfinblock  = 0,
                    firsttf         = block * MaxSamplesPerBlock ) {

                                        // max number of tf to be read in this block
    int     numtfinblock    = MaxSamplesPerBlock - firsttfinblock;
                                        // origin of next full track to be read
//  long    nextelpos       = DataOrg + block * BlockSize;
//  int     tfremainingread = tf2 - firsttf + 1;

                                        // read a whole block at once
    InputStream->seekg ( DataOrg + block * BlockSize );

                                        // within a single block, values for a given track are consecutives
    TEegBdfChannel*     toch    = ChannelsSampling;

    for ( int el = 0; el < NumElectrodes; el++, toch++ ) {
                                        // get that channel length
        int     spb1        = toch->SamplesPerBlock - 1;
                                        // get the complete line for this electrode
        InputStream->read ( (char*) Tracks.GetArray (), toch->ChannelSize );

                                        // taken from EGI MFF, but there seems to be a problem for the first TFs to be read...
/*                                        // optimal jump to where data are
        InputStream->seekg ( nextelpos + firsttfinblock * CellSize ( FileType ), ios::beg );
                                        // so that we can read the min # of data: min of remaining block or remaining data
        uint    tfminread   = min ( toch->SamplesPerBlock - firsttfinblock, tfremainingread );
                                        // still put the data at offset position
        InputStream->read ( &Tracks[ firsttfinblock * CellSize ( FileType ), tfminread * CellSize ( FileType ) ] );
                                        // next channel beginning
        nextelpos          += toch->ChannelSize;
*/


        if ( IsEdf ( FileType ) ) {

            for ( int tfi = firsttf, tf0 = 0; tfi <= tf2 && tf0 < numtfinblock; tfi++, tf0++ ) {

                short*  toS     = (short*) Tracks.GetArray () + Round ( ( ( firsttfinblock + tf0 ) / mspb1 ) * spb1 );

                buff ( el, tfoffset + tf0 )  = *toS * Gains[ el ] + Offsets[ el ];

//              if ( buff ( el, tfoffset + tf0 )    == -MAXSHORT-1 )    buff ( el, tfoffset + tf0 ) = 0;
                } // for tfi
            } // if Edf

        else { // Bdf

            for ( int tfi = firsttf, tf0 = 0; tfi <= tf2 && tf0 < numtfinblock; tfi++, tf0++ ) {

                UCHAR*  toT     = &Tracks[ 3 * Round ( ( ( firsttfinblock + tf0 ) / mspb1 ) * spb1 ) ];

                long    l       = *((long *) toT);

                toT    += 2;

                *( ((uchar *) &l) + 3 ) = (uchar) ( ( *toT & 0x80 ) ? 0xFF : 0x00 );
//              toT++;

                buff ( el, tfoffset + tf0 )  = l * Gains[ el ] + Offsets[ el ];
                } // for tfi
            } // if Bdf
        } // for el
    } // for block
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
