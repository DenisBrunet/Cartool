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

#include    "TEegMicromedTrcDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegMicromedTrcDoc::TEegMicromedTrcDoc (TDocument *parent)
      : TTracksDoc ( parent )
{
InputStream         = 0;

BuffSize            = 0;
DataOrg             = 0;
DataType            = 0;
}


bool	TEegMicromedTrcDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegMicromedTrcDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegMicromedTrcDoc::ReadNativeMarkers ()
{

Micromed_Header_Type_4      header;

InputStream->seekg ( 0, ios::beg );

InputStream->read ( (char *) &header, sizeof ( Micromed_Header_Type_4 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
long                fromtf;
long                totf;
char                buff[ 256 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan relevant blocks of informations
int                 NumItems;

                                        // Annotations (comments)
NumItems    = header.Note_Area.Length / sizeof ( Micromed_New_Annotation );
TArray1<Micromed_New_Annotation>    Notes ( NumItems );

InputStream->seekg ( header.Note_Area.Start_Offset, ios::beg );
InputStream->read  ( (char *) Notes.GetArray (), header.Note_Area.Length );

                                        // make sure we don't overflow
int                 maxnotesize     = NoMore ( ElectrodeNameSize, (int) sizeof ( Notes[ 0 ].Comment ) );

                                        // in case of sequence, we need to offset each TF
long                offsettf        = NumSequences > 0 ? Sequences[ CurrSequence ].OffsetTimeFrames : 0;


for ( int i = 0; i < NumItems; i++ ) {

    if ( Notes[ i ].Sample == 0 )       // end of annotations
        break;

    fromtf  =
    totf    = Notes[ i ].Sample - offsettf;

    if ( ! IsInsideLimits ( fromtf, totf, (long) 0, (long) NumTimeFrames - 1 ) )
        continue;


    StringCopy ( buff, StringIsNotEmpty ( Notes[ i ].Comment ) ? Notes[ i ].Comment : "Note", maxnotesize - 1 );

    StringCleanup ( buff );             // string can be filled with space (?)

    InsertMarker ( TMarker ( fromtf, totf, 1, buff, MarkerTypeTrigger ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Flags (intervals)
NumItems    = header.Flag_Area.Length / sizeof ( Micromed_New_Marker_Pair );
TArray1<Micromed_New_Marker_Pair>   Flags ( NumItems );

InputStream->seekg ( header.Flag_Area.Start_Offset, ios::beg );
InputStream->read  ( (char *) Flags.GetArray (), header.Flag_Area.Length );


for ( int i = 0; i < NumItems; i++ ) {

    if ( Flags[ i ].Begin == 0 )        // end of flags
        break;

    fromtf  =   Flags[ i ].Begin - offsettf;
    totf    = ( Flags[ i ].End ? Flags[ i ].End : Flags[ i ].Begin ) - offsettf;    // missing ends mean single TF

    if ( ! IsInsideLimits ( fromtf, totf, (long) 0, (long) NumTimeFrames - 1 ) )
        continue;


    StringCopy ( buff, "Flag" );

    InsertMarker ( TMarker ( fromtf, totf, 2, buff, MarkerTypeTrigger ) );

//    DBGV2 ( fromtf, totf, buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // EventA (user markers)
NumItems    = header.EventA_Area.Length / sizeof ( Micromed_New_Event );
TArray1<Micromed_New_Event>     Events ( NumItems );

InputStream->seekg ( header.EventA_Area.Start_Offset, ios::beg );
InputStream->read  ( (char *) Events.GetArray (), header.EventA_Area.Length );

                                        // make sure we don't overflow
int                 maxeventsize    = NoMore ( ElectrodeNameSize, MAX_EVENT );
Micromed_New_Marker_Pair   *topair;


for ( int i = 0; i < NumItems; i++ ) {
                                        // no more event in list
    if ( Events[ i ].Selection[ 0 ].Begin == 0 )
        break;


    StringCopy ( buff, StringIsNotEmpty ( Events[ i ].Description ) ? Events[ i ].Description : "EventA", maxeventsize - 1 );

//  if ( StringIsEmpty ( Events[ i ].Description ) )
//      IntegerToString ( StringEnd ( buff ), i + 1 );

                                        // there is a list of intervals for the current event
    for ( int j = 0; j < MAX_EVENT; j++ ) {

        topair  = &Events[ i ].Selection[ j ];
                                        // no more events from current one
        if ( topair->Begin == 0 )
            break;

        fromtf  = topair->Begin - offsettf;
        totf    = ( topair->End ? topair->End : topair->Begin ) - offsettf; // missing ends mean single TF

        if ( ! IsInsideLimits ( fromtf, totf, (long) 0, (long) NumTimeFrames - 1 ) )
            continue;


        InsertMarker ( TMarker ( fromtf, totf, 3, buff, MarkerTypeTrigger ) );

//        DBGV2 ( fromtf, totf, buff );
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // EventB (user markers)
NumItems    = header.EventB_Area.Length / sizeof ( Micromed_New_Event );
Events.Resize ( NumItems );

InputStream->seekg ( header.EventB_Area.Start_Offset, ios::beg );
InputStream->read  ( (char *) Events.GetArray (), header.EventB_Area.Length );


for ( int i = 0; i < NumItems; i++ ) {
                                        // no more event in list
    if ( Events[ i ].Selection[ 0 ].Begin == 0 )
        break;


    StringCopy ( buff, StringIsNotEmpty ( Events[ i ].Description ) ? Events[ i ].Description : "EventB", maxeventsize - 1 );

//  if ( StringIsEmpty ( Events[ i ].Description ) )
//      IntegerToString ( StringEnd ( buff ), i + 1 );

                                        // there is a list of intervals for the current event
    for ( int j = 0; j < MAX_EVENT; j++ ) {

        topair  = &Events[ i ].Selection[ j ];
                                        // no more events from current one
        if ( topair->Begin == 0 )
            break;

        fromtf  = topair->Begin - offsettf;
        totf    = ( topair->End ? topair->End : topair->Begin ) - offsettf; // missing ends mean single TF

        if ( ! IsInsideLimits ( fromtf, totf, (long) 0, (long) NumTimeFrames - 1 ) )
            continue;


        InsertMarker ( TMarker ( fromtf, totf, 4, buff, MarkerTypeTrigger ) );

//        DBGV2 ( fromtf, totf, buff );
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Triggers (RS232)
NumItems    = header.Trigger_Area.Length / sizeof ( Micromed_New_Trigger );
TArray1<Micromed_New_Trigger>   Triggers ( NumItems );

InputStream->seekg ( header.Trigger_Area.Start_Offset, ios::beg );
InputStream->read  ( (char *) Triggers.GetArray (), header.Trigger_Area.Length );


for ( int i = 0; i < NumItems; i++ ) {

    if ( Triggers[ i ].Sample < 0 )     // not sure if < 0 or <= 0, nothing is said about that, as it is a signed long
        break;

    fromtf  =
    totf    = Triggers[ i ].Sample - offsettf;

    if ( ! IsInsideLimits ( fromtf, totf, (long) 0, (long) NumTimeFrames - 1 ) )
        continue;


    IntegerToString ( buff, Triggers[ i ].Type );

    InsertMarker ( TMarker ( fromtf, totf, Triggers[ i ].Type, buff, MarkerTypeTrigger ) );

//    DBGV2 ( fromtf, totf, buff );
    }

}


//----------------------------------------------------------------------------
bool	TEegMicromedTrcDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
TFileName       fullfile ( file, TFilenameExtendedPath );
ifstream        ifs ( fullfile, ios::binary );

if ( ifs.fail () )  return  false;


Micromed_Header_Type_4          header;
TArray1<Micromed_New_Code>      Code;
TArray1<Micromed_New_Electrode> Elec;

ifs.read ( (char *) &header, sizeof ( Micromed_Header_Type_4 ) );


if ( ! ( header.Header_Type == 4
      && header.Compression == 0 ) )
    return false;


switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)       = header.Num_Chan;
        return  true;

    case ReadNumAuxElectrodes :
        int         NumItems;

                                        // Order of electrodes, picked from the next (big) block
        NumItems    = header.Code_Area.Length / sizeof ( Micromed_New_Code );
        Code.Resize ( NumItems );

        ifs.seekg ( header.Code_Area.Start_Offset, ios::beg );
        ifs.read  ( (char *) Code.GetArray (), header.Code_Area.Length );


                                        // Get array of electrodes info
        NumItems    = header.Electrode_Area.Length / sizeof ( Micromed_New_Electrode );
        Elec.Resize ( NumItems );

        ifs.seekg ( header.Electrode_Area.Start_Offset, ios::beg );
        ifs.read  ( (char *) Elec.GetArray (), header.Electrode_Area.Length );


        Micromed_New_Electrode*     toel;
        int             NumElectrodes;
                        NumElectrodes       = header.Num_Chan;
        int             NumAuxElectrodes;
                        NumAuxElectrodes    = 0;
        int             el;


        for ( el = 0; el < NumElectrodes; el++ ) {

            toel        = &Elec[ Code[ el ] ];      // go to indexed electrode

            if ( ( toel->Type & 0x1E )
                || IsElectrodeNameAux ( toel->Positive_Input_Label ) )

                NumAuxElectrodes++;
            }

        *((int *) answer)       = NumAuxElectrodes;
        return  true;

    case ReadNumTimeFrames :
                                        // this is the total # of TFs, all blocks/sessions
        *((int *) answer)       = ( GetFileSize ( fullfile ) - header.Data_Start_Offset ) / header.Multiplexer;
        return  true;

    case ReadSamplingFrequency :
                                        // returns min sampling frequency - there might be some tracks with multiples of it!
        *((double *) answer)    = header.Rate_Min;
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------
bool    TEegMicromedTrcDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {


    InputStream     = InStream ( ofRead | ofBinary );

    if ( ! InputStream )
        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Micromed_Header_Type_4      header;

    InputStream->read ( (char *) &header, sizeof ( Micromed_Header_Type_4 ) );


    if ( ! ( header.Header_Type == 4
          && header.Compression == 0 ) ) {
        ShowMessage ( "File is not of type 4, and can not be open for the moment.", GetDocPath (), ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodes       = header.Num_Chan;
    NumAuxElectrodes    = 0;                    // checked later
    NumMinElectrodes    = NumElectrodes - NumAuxElectrodes;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = header.Rate_Min;      // this can be multiplied by 2/4/8 factors for each track - not supported for the moment

    DateTime            = TDateTime ( header.Date.Year, header.Date.Month,  header.Date.Day,
                                      header.Time.Hour, header.Time.Min,    header.Time.Sec, 0, 0 );
                                        // size of a whole sample of all channels, all inclusive
    BuffSize            = header.Multiplexer;

    DataOrg             = header.Data_Start_Offset;
                                        // # of unsigned bytes per sample, 1 or 2
    DataType            = header.Bytes;
                                        // yup, # of TFs is not in the header...
    NumTimeFrames       = ( GetFileSize ( GetDocPath () ) - DataOrg ) / BuffSize;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "micromed" );
    StringCopy ( ProductName, FILEEXT_EEGTRC );
                                        // extract version
    Version     = header.Header_Type;   // file format
//  Version     = header.Filetype;      // content type
    Subversion  = NumElectrodes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () )
        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan relevant blocks of informations
    int         NumItems;

                                        // Order of electrodes, picked from the next (big) block
    NumItems    = header.Code_Area.Length / sizeof ( Micromed_New_Code );
    TArray1<Micromed_New_Code>  Code ( NumItems );

    InputStream->seekg ( header.Code_Area.Start_Offset, ios::beg );
	InputStream->read  ( (char *) Code.GetArray (), header.Code_Area.Length );

//    for ( int i = 0; i < NumItems; i++ )
//        DBGV2 ( i + 1, Code[ i ], header.Code_Area.Name );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get array of electrodes info
    NumItems    = header.Electrode_Area.Length / sizeof ( Micromed_New_Electrode );
    TArray1<Micromed_New_Electrode> Elec ( NumItems );

    InputStream->seekg ( header.Electrode_Area.Start_Offset, ios::beg );
	InputStream->read  ( (char *) Elec.GetArray (), header.Electrode_Area.Length );

//    for ( int i = 0; i < NumItems; i++ )
//        DBGM2 ( Elec[ i ].Positive_Input_Label, Elec[ i ].Negative_Input_Label, "Pos - Neg" );

                                        // scan electrodes & extract a few informations (electrode names, type, sampling frequency...)
    char            buff[ 256 ];
    int             maxnamesize     = NoMore ( ElectrodeNameSize, (int) sizeof ( Elec[ 0 ].Positive_Input_Label ) );
    Micromed_New_Electrode      *toel;
    bool            bipolar;
    int             ratecoeff       = 0;
    bool            ratesdiffer     = false;


    for ( int el = 0; el < NumElectrodes; el++ ) {

        toel        = &Elec[ Code[ el ] ];      // go to indexed electrode
        bipolar     = toel->Type & 0x01;

                                        // set positive part name
        StringCopy ( ElectrodesNames[ el ], toel->Positive_Input_Label, maxnamesize - 1 );

                                        // complete with second negative part
        if ( bipolar ) {
            StringCopy    ( buff, toel->Negative_Input_Label, maxnamesize - 1 );

            StringPrepend ( buff, " - " );
            StringPrepend ( buff, ElectrodesNames[ el ] );
            StringCopy    ( ElectrodesNames[ el ], buff );
            }

                                        // save linear transform to micro-volts
        Offset[ el ]    = toel->Logic_Ground;
        Gain  [ el ]    = (double) ( toel->Physic_Maximum - toel->Physic_Minimum ) / ( toel->Logic_Maximum - toel->Logic_Minimum + 1 );

                                        // Status meaning:
                                        // bit      0               1
                                        // 0        Not recorded    Recorded
        if ( toel->Status == 0 )
            BadTracks.Set ( el );       // shouldn't happen!

                                        // Type meaning:
                                        // bit      0               1
                                        // 0        Ref is G2       Bipolar
                                        // 1                        Marker
                                        // 2                        Oxym.
                                        // 3                        16DC
                                        // 4                        bip2eeg
        if ( toel->Type & 0x1E ) {
            AuxTracks.Set ( el );
            NumAuxElectrodes++;
            NumMinElectrodes--;
            }

                                        // check multipliers of the base sampling frequency
        if ( ratecoeff == 0 )
            ratecoeff   = toel->Rate_Coefficient;
        ratesdiffer = ratesdiffer || ratecoeff != toel->Rate_Coefficient;

//        DBGV2 ( toel->Status, toel->Type, "Status Type" );
        }

                                        // smart scan
    InitAuxiliaries ();


    if ( ratesdiffer ) {
        ShowMessage ( "Sampling rates vary across electrodes, which is currently not supported...", GetDocPath (), ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get segments (blocks)
    NumItems    = header.Segment_Area.Length / sizeof ( Micromed_New_Segment ); // or MAX_SEGM
    TArray1<Micromed_New_Segment>   Segment ( NumItems );

    InputStream->seekg ( header.Segment_Area.Start_Offset, ios::beg );
	InputStream->read  ( (char *) Segment.GetArray (), header.Segment_Area.Length );


    NumSequences    = 0;

    for ( int i = 0; i < NumItems; i++ ) {

//      DBGV3 ( NumSequences, Segment[ i ].Sample, Segment[ i ].Time, "#Session: Current TF -> Original TF" );

        if ( Segment[ i ].Time == 0 )   // stopping condition
            break;

        NumSequences++;
        }

//  DBGV3 ( DataOrg, NumTimeFrames, NumSequences, "DataOrg, NumTimeFrames, NumSequences" );

                                        // allocate sequence info
    if ( NumSequences ) {

        Sequences.Resize ( NumSequences );


        for ( int i = 0; i < NumSequences; i++ ) {

                                        // Offset in TF to reach sequence
            Sequences[ i ].OffsetTimeFrames = Segment[ i ].Sample;

                                        // "Sample" is the origin time frame within this file
            Sequences[ i ].DataOrg          = DataOrg + Segment[ i ].Sample * BuffSize;

                                        // We can deduce the number of time frames
            if ( i == NumSequences - 1 )    Sequences[ i ].NumTimeFrames    = NumTimeFrames           - Segment[ i ].Sample;
            else                            Sequences[ i ].NumTimeFrames    = Segment[ i + 1 ].Sample - Segment[ i ].Sample;

                                        // original time frame position
            long        orgtf       = Segment[ i ].Time;
                                        // extract only the seconds part
            long        sec         = Truncate  ( TimeFrameToSeconds ( orgtf, SamplingFrequency ) );
                                        // then the microseconds
            long        microsec    = TimeFrameToMicroseconds ( orgtf - SecondsToTimeFrame ( sec, SamplingFrequency ), SamplingFrequency );
                                        // Offset the original date and time
            Sequences[ i ].DateTime = TDateTime ( DateTime.GetYear (), DateTime.GetMonth (), DateTime.GetDay (),
                                                  DateTime.GetHour        (),
                                                  DateTime.GetMinute      (),
                                                  DateTime.GetSecond      () + sec,
                                                  DateTime.GetMillisecond (),
                                                  DateTime.GetMicrosecond () + microsec );
            } // for NumSequences


                                        // !Weird / buggy / undocumented case!
                                        // It looks like a segment is missing, if the first segment does not begin at TF 0
                                        // It seems to assume imply a default segment starting from 0, which we manually add here
        if ( NumSequences < NumItems && Segment[ 0 ].Sample != 0 ) {
                                        // Adding 1 slot
            Sequences.ResizeDelta ( 1, ResizeKeepMemory );

                                        // Move everything up 1 slot
            MoveVirtualMemory ( Sequences.GetArray () + 1, Sequences.GetArray (), NumSequences * Sequences.AtomSize () );

                                        // adding the ghost segment at the beginning
            Sequences[ 0 ].OffsetTimeFrames     = 0;
            Sequences[ 0 ].DataOrg              = DataOrg;
            Sequences[ 0 ].NumTimeFrames        = Segment[ 0 ].Sample;
            Sequences[ 0 ].DateTime             = DateTime;     // assume time is the beginning of recording


            NumSequences++;
            }


                                        // using the first sequence at opening time, or the next one if it appears to be too short
        CurrSequence        = 0;

        if ( NumSequences > 1 
          && Sequences[ 0 ].NumTimeFrames < Sequences[ 1 ].NumTimeFrames / 2 )
            CurrSequence        = 1;


        DataOrg             = Sequences[ CurrSequence ].DataOrg;
        NumTimeFrames       = Sequences[ CurrSequence ].NumTimeFrames;
        DateTime            = Sequences[ CurrSequence ].DateTime;

        UpdateTitle ();
        } // for NumSequences

    }
else {
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegMicromedTrcDoc::SetArrays ()
{
OffGfp          = NumElectrodes + PseudoTrackOffsetGfp;
OffDis          = NumElectrodes + PseudoTrackOffsetDis;
OffAvg          = NumElectrodes + PseudoTrackOffsetAvg;

Tracks.Resize ( BuffSize );
Offset.Resize ( NumElectrodes );
Gain  .Resize ( NumElectrodes );
                                        // do all allocations stuff

ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );


BadTracks           = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks           = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();

return true;
}


//----------------------------------------------------------------------------
bool    TEegMicromedTrcDoc::UpdateSession ( int newsession )
{
DataOrg             = Sequences[ newsession ].DataOrg;
NumTimeFrames       = Sequences[ newsession ].NumTimeFrames;
DateTime            = Sequences[ newsession ].DateTime;

return  true;
}


//----------------------------------------------------------------------------
void    TEegMicromedTrcDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
                                        // set file to first TF
InputStream->seekg ( DataOrg + BuffSize * tf1, ios::beg );


if      ( DataType == 2 ) {
    ushort             *toTus;

    for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

        InputStream->read ( (char *) &Tracks[ 0 ], BuffSize );
        toTus   = (ushort *) &Tracks[ 0 ];

        for ( int el = 0; el < NumElectrodes; el++, toTus++ )
            buff ( el, tf ) = ( *toTus - Offset[ el ] ) * Gain[ el ];
        }
    }

else if ( DataType == 1 ) {
    uchar              *toTub;

    for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

        InputStream->read ( (char *) &Tracks[ 0 ], BuffSize );
        toTub   = (uchar *) &Tracks[ 0 ];

        for ( int el = 0; el < NumElectrodes; el++, toTub++ )
            buff ( el, tf ) = ( *toTub - Offset[ el ] ) * Gain[ el ];
        }
    }

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
