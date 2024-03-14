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
#include    "Dialogs.Input.h"

#include    "TArray1.h"

#include    "TTracksView.h"

#include    "TEegBioLogicDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
istream& operator >> ( istream &is, TEBEegData &eegd )
{
is.read ( (char *) &eegd.point_num, eegd.sizeofpn );
is.read ( (char *)  eegd.value,     eegd.sizeofv  );

return is;
}


//----------------------------------------------------------------------------
        TEegBioLogicDoc::TEegBioLogicDoc (TDocument *parent)
      : TTracksDoc ( parent )
{
InputStream         = 0;

DataOrg             = 0;
StartingPacketNumber= 0;
FileBuff            = 0;
BuffSize            = 0;
CompressionMode     = 0;

Reference           = ReferenceAsInFile;
}


bool	TEegBioLogicDoc::Close()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

if ( FileBuff ) {
    delete      FileBuff;
    FileBuff = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegBioLogicDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegBioLogicDoc::ReadNativeMarkers ()
{
                                        // open marker file
TFileName           fm;

StringCopy          ( fm, GetDocPath () );
ReplaceExtension    ( fm, FILEEXT_BIOMRK );

ifstream            ifsmrk ( TFileName ( fm, TFilenameExtendedPath ), ios::binary );

                                        // if can not read the marker file, no problem, skip
if ( ifsmrk.good () ) {
    TEBMarker       mrk;
    bool            breakloop   = false;
                                        // substract the origin of this sequence
    long            frompn      = Sequences[ CurrSequence ].StartingPacketNumber;
    long            tf;

                                        // if the file is incorrect, do a silent break
    do {
                                        // read a standard marker
        ifsmrk.read ( (char *) &mrk, sizeof ( mrk ) );

                                        // does it concern this sequence?
        if ( mrk.marker_deleted
          || mrk.marker_point_num < Sequences[ CurrSequence ].StartingPacketNumber
          || mrk.marker_point_num > Sequences[ CurrSequence ].EndingPacketNumber   ) {
                                        // position to the next marker
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );
            continue;
            }

        switch ( mrk.marker_data_type ) {
            case    MARKER_FILE_ID :

                TEBFileID   datafid;
                ifsmrk.read ( (char *) &datafid, sizeof ( datafid ) );

                if ( StringIsNot ( datafid.mrk_id_ascii, "BLSC" ) || datafid.mrk_id_file_type != EEGBIODOC_FILETYPEMRK ) {
                    breakloop   = true;
                    break;
                    }
                break;

            case    MARKER_HEADER :

                TEBHeader   datah;
                ifsmrk.read ( (char *) &datah, sizeof ( datah ) );

                if ( datah.mrk_header_application != 0 ) {
                    breakloop   = true;
                    break;
                    }

                if ( datah.mrk_header_version < EEGBIODOC_VERSIONOK ) {
                    breakloop   = true;
                    break;
                    }
                break;

            case    MARKER_TAG :
                tf = mrk.marker_point_num - frompn;
                InsertMarker ( TMarker ( tf, tf, EEGBIODOC_MARKERCODE_TAG, EEGBIODOC_MARKERTEXT_TAG, MarkerTypeTrigger ), false );

                break;

            case    MARKER_PUSH_BUTTON :
                tf = mrk.marker_point_num - frompn;
                InsertMarker ( TMarker ( tf, tf, EEGBIODOC_MARKERCODE_PUSHB, EEGBIODOC_MARKERTEXT_PUSHB, MarkerTypeTrigger ), false );

                break;

            case    MARKER_COMMENT :
                TEBComment  datac;
                ifsmrk.read ( (char *) &datac, sizeof ( datac ) );

                tf = mrk.marker_point_num - frompn;
                InsertMarker ( TMarker ( tf, tf, datac.marker_comment_type, datac.marker_comment_text, MarkerTypeTrigger ), false );

                break;

            }
                                        // position to the next marker
        if ( (long) mrk.marker_next_offset != MARKER_TERMINATOR )
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );

        } while ( (long) mrk.marker_next_offset != MARKER_TERMINATOR && ifsmrk.good() && !breakloop );


    ifsmrk.close ();
    }
}


//----------------------------------------------------------------------------
bool	TEegBioLogicDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );

if ( ifs.fail () )  return false;


TEBMarker       mrk;

                                    // and get infos as a startup
ULONG           DataOrg         = 0;
int             NumElectrodes;
int             NumMinElectrodes;
int             NumAuxElectrodes;
int             TotalElectrodes;
double          SamplingFrequency;


do {
                                    // read a standard marker
    ifs.read ( (char *) &mrk, sizeof ( mrk ) );

    if ( mrk.marker_deleted ) {
                                    // position to the next marker
        ifs.seekg ( mrk.marker_next_offset, ios::beg );
        continue;
        }

    switch ( mrk.marker_data_type ) {
        case    MARKER_FILE_ID :

            TEBFileID   datafid;
            ifs.read ( (char *) &datafid, sizeof ( datafid ) );

            if ( StringIsNot ( datafid.mrk_id_ascii, "BLSC" ) || datafid.mrk_id_file_type != EEGBIODOC_FILETYPEEEG ) {
                return  false;
                }
            break;

        case    MARKER_HEADER :

            TEBHeader   datah;
            ifs.read ( (char *) &datah, sizeof ( datah ) );

            if ( datah.mrk_header_application != 0 ) {
                return  false;
                }

/*            if ( datah.mrk_header_version < EEGBIODOC_VERSIONOK ) {
                sprintf ( buff, "EEG file version is %0.2f,\nonly versions beyond %0.2f are safely supported,\ncontinue anyway?", (double) datah.mrk_header_version / 100, (double) EEGBIODOC_VERSIONOK / 100 );
                if ( ! GetAnswerFromUser ( buff, "Opening error" ) )
                    return  false;
                }
*/
            break;

        case    MARKER_XL_COLLECT_MONTAGE :

            TEBCollectMontage   datacm;
            ifs.read ( (char *) &datacm, sizeof ( datacm ) );

            NumElectrodes       = datacm.mrk_cmont_num_chan;
            NumMinElectrodes    = NumElectrodes;
            NumAuxElectrodes    = 0;
            TotalElectrodes     = NumElectrodes + NumPseudoTracks;

            break;

        case    MARKER_AD_PARMS_NEW :

            TEBADParms   dataadp;
            ifs.read ( (char *) &dataadp, sizeof ( dataadp ) );
                                    // take the integer version of sampling frequency
            SamplingFrequency   = dataadp.mrk_ad_point_float_rate;
            break;
        }
                                    // position to the next marker
    if ( (long) mrk.marker_next_offset != MARKER_TERMINATOR )
        ifs.seekg ( mrk.marker_next_offset, ios::beg );

    } while ( (long) mrk.marker_next_offset != MARKER_TERMINATOR && ifs.good() );

ifs.close();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)   = NumElectrodes;
        return true;

    case ReadNumAuxElectrodes :
        *((int *) answer)   = NumAuxElectrodes;
        return true;

    case ReadSamplingFrequency :
        *((double *) answer)= SamplingFrequency;
        return true;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TEegBioLogicDoc::Open	(int /*mode*/, const char* path)
{
char        buff[ 256 ];
char        fm  [ 256 ];

if ( path )
    SetDocPath ( path );

SetDirty ( false );

if ( GetDocPath () ) {
    TEBMarker       mrk;

    ifstream    ifseeg ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // and get infos as a startup
    DataOrg = 0;

    do {
                                        // read a standard marker
        ifseeg.read ( (char *) &mrk, sizeof ( mrk ) );

        if ( mrk.marker_deleted ) {
                                        // position to the next marker
            ifseeg.seekg ( mrk.marker_next_offset, ios::beg );
            continue;
            }

        switch ( mrk.marker_data_type ) {
            case    MARKER_FILE_ID :

                TEBFileID   datafid;
                ifseeg.read ( (char *) &datafid, sizeof ( datafid ) );

                if ( StringIsNot ( datafid.mrk_id_ascii, "BLSC" ) || datafid.mrk_id_file_type != EEGBIODOC_FILETYPEEEG ) {
                    ShowMessage ( "Not a Biologic EEG file.", "Opening error", ShowMessageWarning );
                    return  false;
                    }

                StringCopy ( ProductName, FILEEXT_EEGBIO );
                break;

            case    MARKER_HEADER :

                TEBHeader   datah;
                ifseeg.read ( (char *) &datah, sizeof ( datah ) );

                if ( datah.mrk_header_application != 0 ) {
                    ShowMessage ( "EEG file is not a CeeGraphIV one.", "Opening error", ShowMessageWarning );
                    return  false;
                    }

                if ( datah.mrk_header_version < EEGBIODOC_VERSIONOK ) {
                    sprintf ( buff, "EEG file version is %0.2f,\nonly versions beyond %0.2f are safely supported,\ncontinue anyway?", (double) datah.mrk_header_version / 100, (double) EEGBIODOC_VERSIONOK / 100 );
                    if ( ! GetAnswerFromUser ( buff, "Opening error" ) )
                        return  false;
                    }

                Version     = datah.mrk_header_version;
                break;

            case    MARKER_XL_COLLECT_MONTAGE :

                TEBCMontMap         cmm;
                TEBCollectMontage   datacm;
                int                 e;
                ifseeg.read ( (char *) &datacm, sizeof ( datacm ) );

                NumElectrodes       = datacm.mrk_cmont_num_chan;
                NumMinElectrodes    = NumElectrodes;
                NumAuxElectrodes    = 0;
                TotalElectrodes     = NumElectrodes + NumPseudoTracks;

                if ( NumElectrodes == 0 ) {
                    ShowMessage ( "EEG file has 0 electrodes recorded.", "Opening error", ShowMessageWarning );
                    return  false;
                    }
                                        // space allocation + default names
                if ( ! SetArrays () ) {
                    return false;
                    }
                                        // define a 1 time point buffer
                FileBuff    = new TEBEegData ( NumElectrodes );
                BuffSize    = FileBuff->SizeOf ();

                for ( e=0; e < datacm.mrk_cmont_num_chan; e++ ) {
                    cmm = datacm.mrk_cmont[ e ];
//                  el  = cmm.cmont_out - 1;  // dont use the internal numbering of Biologic
                                        // copy the user label, or the default label
                    if ( cmm.cmont_user_label )
                        StringCopy ( ElectrodesNames[ e ], cmm.cmont_user_label );
                    else if ( cmm.cmont_label )
                        StringCopy ( ElectrodesNames[ e ], cmm.cmont_label );

                    // should do something with cmm.cmont_ref or ref_label
                    }

                break;

            case    MARKER_AD_PARMS_NEW :

                TEBADParms   dataadp;
                ifseeg.read ( (char *) &dataadp, sizeof ( dataadp ) );
                                        // take the integer version of sampling frequency
                SamplingFrequency   = dataadp.mrk_ad_point_float_rate;
                break;

            case    MARKER_XL_SW_ADRES :
                                        // Signal Window Resolution
                TEBSWADRes  dataswa;
                ifseeg.read ( (char *) &dataswa, sizeof ( dataswa ) );

                Gains.Resize ( NumElectrodes );
                for ( e = 0; e < NumElectrodes; e++ ) {
                    Gains[ e ]  = (double) dataswa.adres_num[ e ] / dataswa.adres_den[ e ];
                    }
                break;

            case    MARKER_XL_DISPLAY_MONTAGE :

//                TEBDispMont datadm;
//                ifseeg.read ( (char *) &datadm, sizeof ( datadm ) );
                break;

            case    MARKER_FILE_SEQ :

//                TEBFileSeq  datafs;
//                ifseeg.read ( (char *) &datafs, sizeof ( datafs ) );
//                datfs.seq_number
                break;

            case    MARKER_DATA_START :

                TEBDataStart    datads;
                ifseeg.read ( (char *) &datads, sizeof ( datads ) );
                                        // 1 in EEG file
                StartingPacketNumber    = mrk.marker_point_num;
                                        // data immediately follow this marker
                DataOrg = ifseeg.tellg();

                ifseeg.seekg ( 0, ios::end );
                NumTimeFrames   = ( (ulong) ifseeg.tellg() - DataOrg + 1 ) / BuffSize;

                                        // time in seconds since 1-1-1970
                TDate   pcstart ( 1, 1970 );

                long s         = mrk.marker_time_of_day - 18000;

                                        // strange shift of 18000 s (5 hours)
                DateTime = TDateTime ( pcstart.Year(), pcstart.Month(), pcstart.DayOfMonth(),
                                       0, s / 60, s % 60, 0, 0 );
//                                     0, 0, mrk.marker_time_of_day - 18000.0, 0, 0 );

                CompressionMode = datads.mrk_compression_mode;
                break;
            }
                                        // position to the next marker
        if ( (long) mrk.marker_next_offset != MARKER_TERMINATOR )
            ifseeg.seekg ( mrk.marker_next_offset, ios::beg );

        } while ( (long) mrk.marker_next_offset != MARKER_TERMINATOR && ifseeg.good() );

    ifseeg.close();

                                        // in case we attempt to open a DeltaMed file
    if ( DataOrg == 0 || NumElectrodes == 0 )
        return false;

                                        // fill product info
    StringCopy ( CompanyName, "Bio-logic" );
    Subversion          = NumElectrodes;


                                        // open EEG file
    InputStream     = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;

                                        // check for auxilliary
    InitAuxiliaries ();

    UpdateTitle ();

                                        // open marker file
    StringCopy ( fm, GetDocPath() );
    ReplaceExtension ( fm, FILEEXT_BIOMRK );
    ifstream    ifsmrk ( TFileName ( fm, TFilenameExtendedPath ), ios::binary );
                                        // if can not read the marker file, no problem, get out
    if ( ifsmrk.fail () )
        return true;


                                        // count the number of appended sequences in file
                                        // and check the file is OK
                                        // if it is not, just skip it
    NumSequences    = 1;
    CurrSequence    = 0;
    do {
                                        // read a standard marker
        ifsmrk.read ( (char *) &mrk, sizeof ( mrk ) );

        if ( mrk.marker_deleted ) {
                                        // position to the next marker
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );
            continue;
            }

        switch ( mrk.marker_data_type ) {
            case    MARKER_FILE_ID :

                TEBFileID   datafid;
                ifsmrk.read ( (char *) &datafid, sizeof ( datafid ) );

                if ( StringIsNot ( datafid.mrk_id_ascii, "BLSC" ) || datafid.mrk_id_file_type != EEGBIODOC_FILETYPEMRK ) {
                    ShowMessage ( "Not a Biologic MRK file.", "Opening error", ShowMessageWarning );
                    return  true;
                    }
                break;

            case    MARKER_HEADER :

                TEBHeader   datah;
                ifsmrk.read ( (char *) &datah, sizeof ( datah ) );

                if ( datah.mrk_header_application != 0 ) {
                    ShowMessage ( "EEG file is not a CeeGraphIV one.", "Opening error", ShowMessageWarning );
                    return  true;
                    }
/*
                if ( datah.mrk_header_version < EEGBIODOC_VERSIONOK ) {
                    sprintf ( buff, "EEG file version %0.2f is not supported,\nonly versions beyond %0.2f can  be read.", (double) datah.mrk_header_version / 100, (double) EEGBIODOC_VERSIONOK / 100 );
                    ShowMessage ( buff, "Opening error", ShowMessageWarning );
                    return  true;
                    }
*/
                break;

            case    MARKER_DATA_START :
                                        // add sequences only if not the first data start
                                        // (data start 1 may appear 2 times!!)
                if ( mrk.marker_point_num != 1 )
                    NumSequences++;
                break;
            }
                                        // position to the next marker
        if ( (long) mrk.marker_next_offset != MARKER_TERMINATOR )
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );

        } while ( (long) mrk.marker_next_offset != MARKER_TERMINATOR && ifsmrk.good() );

    ifsmrk.close();
    ifsmrk.open ( fm, ios::binary );


    Sequences.Resize ( NumSequences );
    Sequences[ 0 ].DataOrg  = DataOrg;

    TDate   pcstart;
                                        // now scan through again
                                        // at this point NumTimeFrames is the total number of TF in the file
    do {
                                        // read a standard marker
        ifsmrk.read ( (char *) &mrk, sizeof ( mrk ) );

        if ( mrk.marker_deleted ) {
                                        // position to the next marker
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );
            continue;
            }

        switch ( mrk.marker_data_type ) {
            case    MARKER_DATA_START :

                TEBDataStart    datads;
                ifsmrk.read ( (char *) &datads, sizeof ( datads ) );

                Sequences[ CurrSequence ].DataOrg               = DataOrg + ( mrk.marker_point_num - 1 ) * BuffSize;

                Sequences[ CurrSequence ].StartingPacketNumber  = mrk.marker_point_num;
                                        // set as default, go up to the physical end of the file
                Sequences[ CurrSequence ].EndingPacketNumber    = NumTimeFrames;
                Sequences[ CurrSequence ].NumTimeFrames         = Sequences[ CurrSequence ].EndingPacketNumber
                                                                - Sequences[ CurrSequence ].StartingPacketNumber
                                                                + 1;
                                        // time in seconds since 1-1-1970
                pcstart = TDate ( 1, 1970 );

                long s;
                s   = mrk.marker_time_of_day - 18000;
//                DBGV ( s, "bio start" );

                                        // strange shift of 18000 s (5 hours)
                Sequences[ CurrSequence ].DateTime = TDateTime ( pcstart.Year(), pcstart.Month(), pcstart.DayOfMonth(),
//                                                                 0, 0, mrk.marker_time_of_day - 18000.0, 0, 0 );
                                                                 0, s / 60, s % 60, 0, 0 );

                Sequences[ CurrSequence ].CompressionMode       = datads.mrk_compression_mode;
                break;

            case    MARKER_DATA_END :
                                        // real end is now known
                                        // but test its consistency anyway!
                if ( mrk.marker_point_num <= (ulong) NumTimeFrames )
                    Sequences[ CurrSequence ].EndingPacketNumber= mrk.marker_point_num;

                Sequences[ CurrSequence ].NumTimeFrames         = Sequences[ CurrSequence ].EndingPacketNumber
                                                                - Sequences[ CurrSequence ].StartingPacketNumber
                                                                + 1;
                CurrSequence++;
                break;
            }
                                        // position to the next marker
        if ( (long) mrk.marker_next_offset != MARKER_TERMINATOR )
            ifsmrk.seekg ( mrk.marker_next_offset, ios::beg );

        } while ( (long) mrk.marker_next_offset != MARKER_TERMINATOR && ifsmrk.good() && CurrSequence < NumSequences );

    ifsmrk.close();


                                        // using the first sequence at opening time, or the next one if it appears to be too short
    CurrSequence        = 0;

    if ( NumSequences > 1 
      && Sequences[ 0 ].NumTimeFrames < Sequences[ 1 ].NumTimeFrames / 2 )
        CurrSequence        = 1;

    DataOrg             = Sequences[ CurrSequence ].DataOrg;
    StartingPacketNumber= Sequences[ CurrSequence ].StartingPacketNumber;
    NumTimeFrames       = Sequences[ CurrSequence ].NumTimeFrames;
    DateTime            = Sequences[ CurrSequence ].DateTime;
    CompressionMode     = Sequences[ CurrSequence ].CompressionMode;

                                        // right now, can not decode
    if ( CompressionMode != 0 ) {
        sprintf ( buff, "Compression mode %0d is not recognized,\ndata will therefore appear cryptic.", CompressionMode );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        }

                                        // again update, in case of some sequences were read
    UpdateTitle ();

    if ( NumSequences > 1 && VkQuery () ) {
        sprintf ( buff, "There are %0d sessions in this file,\nonly one can be used at a time.", NumSequences );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        }

    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegBioLogicDoc::SetArrays ()
{
OffGfp          = NumElectrodes + PseudoTrackOffsetGfp;
OffDis          = NumElectrodes + PseudoTrackOffsetDis;
OffAvg          = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.Resize ( TotalElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegBioLogicDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 el;
int                 tf;
long                tfi;
float              *toT;
short              *toFB;
double             *toG;

                                        // set file to first TF
InputStream->seekg ( DataOrg + BuffSize * tf1, ios::beg );


for ( tfi=tf1, tf=0; tfi <= tf2; tfi++, tf++ ) {

    *InputStream >> *FileBuff;


    for ( el=0, toT=Tracks, toFB=FileBuff->value, toG=Gains; el < NumElectrodes; el++, toT++, toFB++, toG++ )
        *toT    = *toFB * *toG;


    for ( el=0, toT=Tracks; el < NumElectrodes; el++, toT++ )
        buff (el,tfoffset + tf)  = *toT;
    }
}


bool    TEegBioLogicDoc::UpdateSession ( int newsession )
{
DataOrg             = Sequences[ newsession ].DataOrg;
StartingPacketNumber= Sequences[ newsession ].StartingPacketNumber;
NumTimeFrames       = Sequences[ newsession ].NumTimeFrames;
DateTime            = Sequences[ newsession ].DateTime;
CompressionMode     = Sequences[ newsession ].CompressionMode;

return  true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
