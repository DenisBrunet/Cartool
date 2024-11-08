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

#include    "TEegEgiMffDoc.h"
#include    "TEegEgiNsrDoc.h"           // EgiEventCodes

#include    "MemUtil.h"
#include    "Dialogs.Input.h"
#include    "Strings.Utils.h"
#include    "TArray1.h"
#include    "Files.XML.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // unfold a date & time string to fields - note the seconds are in double, with potentially a fractional part
void    TEegEgiMffDoc::DateTimeStringToFields ( char *datetime, int &year, int &month, int &day, int &hour, int &minute, double &second )
{
                                        // format is well known, just the seconds could be micro or nano seconds, but we don't care as it remains in the fraction!
sscanf ( datetime, "%4d-%2d-%2dT%2d:%2d:%lf", &year, &month, &day, &hour, &minute, &second );
}

                                        // converts a duration string to micro-seconds, with potentially a fractional part
double  TEegEgiMffDoc::DurationStringToMicroseconds ( char *duration )
{
return  StringToDouble ( duration ) / ( IsNanoSecondsPrecision () ? 1000 : 1 );
}

                                        // unfold a duration string up to seconds fields (which should be enough precision wise)
void    TEegEgiMffDoc::DurationStringToFields ( char* duration, int& sec, int& millisec, int& microsec, int& nanosec )
{
sec     = millisec  = microsec  = nanosec   = 0;


int                l                = StringLength ( duration );

auto                convertnext3digits  = [ &l, &duration ] ( int& v ) 
{
if ( l == 0 )    
    return;

l               = AtLeast ( 0, l - 3 );
v               = StringToInteger ( duration + l );
duration[ l ]   = EOS;      // clip the used end of string
};

                                        // conversion is from end of string back to beginning
if ( IsNanoSecondsPrecision () )
    convertnext3digits ( nanosec );

convertnext3digits ( microsec );
convertnext3digits ( millisec );
convertnext3digits ( sec      );
}


//----------------------------------------------------------------------------
        TEegEgiMffDoc::TEegEgiMffDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
InputStream         = 0;
Version             = 0;

NumBlocks           = 0;
MaxSamplesPerBlock  = 0;
EqualTracks         = true;

Reference           = ReferenceAsInFile;
}


bool	TEegEgiMffDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegEgiMffDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegEgiMffDoc::ReadNativeMarkers ()
{
                                        // Look for all files beginning with "events_", as we don't know in advance...
TFileName           filestemplate;
TGoF                eventsfiles;


StringCopy      ( filestemplate,  GetDocPath () );
ReplaceFilename ( filestemplate,  "events*.xml" );

                                        // get the files matching the template
if ( ! eventsfiles.FindFiles ( filestemplate ) )
    return;


char                element     [ 1024 ];
char                begintime   [  256 ];
char                duration    [  256 ];
char                code        [ 2 * MarkerNameMaxLength ];
char                label       [  256 ];
char                description [  256 ];
char                keycode     [  256 ];
char                keydata     [  256 ];
int                 yyyy,   MM,     dd;
int                 hh,     mm;
double              ss;
int                 deltaday;
double              deltatime;
long                tf1;
long                tf2;
MarkerCode          markercode;
char                markername  [ 2 * MarkerNameMaxLength ];
MarkerType          filemarkertype  = MarkerTypeUnknown;
MarkerType          markertype      = MarkerTypeUnknown;
bool                dinfile;
int                 numdinfile      = 0;
ifstream            fiv;



for ( int i = 0; i < (int) eventsfiles; i++ ) {

//    DBGM ( eventsfiles[ i ], "events file" );

    fiv.open ( eventsfiles[ i ], ios::binary );

    if ( fiv.fail () )
        continue;

                                        // goto root element
    if ( ! XMLGotoElement ( fiv, "eventTrack", 0, 0, true ) )
        continue;

                                        // "name" of trigger "track": "ECI TCP/IP 55513", "Markup Track", "Marks", "DIN 1"...
                                        // we rely on that for DIN triggers
    if ( XMLGetElement  ( fiv, "name", "eventTrack", element ) ) {
        dinfile     = StringStartsWith ( element, "DIN" );
        numdinfile++;
        }
    else
        dinfile     = false;

                                        // global type of triggers within this file: "STIM", "PAT ", "EVNT"
    if ( XMLGetElement  ( fiv, "trackType", "eventTrack", element ) ) {
//        DBGM ( element, "type of triggers" );

        filemarkertype  = StringIs ( element, "STIM" ) ? MarkerTypeTrigger
                        : StringIs ( element, "PAT " ) ? MarkerTypeEvent      // what is this?
                        : StringIs ( element, "EVNT" ) ? MarkerTypeEvent
                                                       : MarkerTypeTrigger;

        if ( dinfile )                  // just by security, alhtough it doesn't seem to happen with the sample files
            filemarkertype = MarkerTypeTrigger;
        }


//    TStrings      allcodes;
                                        // now loop through all events
    do {
                                        // no more event?
        if ( ! XMLGotoElement ( fiv, "event", "eventTrack" ) )
            break;

                                        // retrieve timing info only
        XMLGetElement  ( fiv, "beginTime",      "event", begintime   );
        XMLGetElement  ( fiv, "duration",       "event", duration    );

                                        // extract variables from date
        DateTimeStringToFields ( begintime, yyyy, MM, dd, hh, mm, ss );

                                        // we need any difference of days, in case of midnight crossing (include new year crossing and the like)
        deltaday    = GetDayOfTheYear ( yyyy, MM, dd ) - DateTime.GetDayOfTheYear ();

                                        // has crossed new year (hence negative difference)?
        if ( DateTime.GetYear () < yyyy )
            deltaday   += DateTime.GetNumberOfDays ();

                                        // finally, the time difference in micro-seconds (needs 37 bits, double is 52)
        deltatime   = deltaday * 24 * 60 * 60 * 1000 * 1000.0           // difference of days in micro-seconds
                    + ( ( ( ( ( hh - DateTime.GetHour   () ) * 60.0 )
                            + ( mm - DateTime.GetMinute () )          ) * 60.0
                            + ( ss - DateTime.GetSecond () )                   ) * 1000.0
                                   - DateTime.GetMillisecond ()                           ) * 1000.0
                                   - DateTime.GetMicrosecond ();


        tf1         = MicrosecondsToTimeFrame ( deltatime                                             + 0.5, SamplingFrequency );
        tf2         = MicrosecondsToTimeFrame ( deltatime + DurationStringToMicroseconds ( duration ) - 0.5, SamplingFrequency );
//      tf2         = tf1;              // do this if some rounding problems occur

//        if ( tf1 > 0 && tf1 < NumTimeFrames && tf1 != tf2 )
//            DBGV2 ( tf1, tf2, "tf1 tf2" );

                                        // within current sequence time range?
        if ( tf1 >= 0 && tf2 < NumTimeFrames ) {

                                        // proceed by retrieving the remaining info of event
            XMLGetElement  ( fiv, "code",           "event", code        );
            XMLGetElement  ( fiv, "label",          "event", label       );
            XMLGetElement  ( fiv, "description",    "event", description );
//          XMLGetElement  ( fiv, "sourceDevice",   "event", device      );

                                        // remove beginning/trailing spaces
            StringCleanup ( code );

//            if ( ! allcodes.Contains ( code ) )     // for debugging info
//                allcodes.Add ( code );

                                        // set to event if either the whole file is flagged as event (?)
                                        // or with current trigger is recognized as an event
            if ( ! dinfile
              && ( filemarkertype == MarkerTypeEvent || StringContains ( (const char*) EgiEventCodes, (const char*) code ) ) )
                markertype     = MarkerTypeEvent;
            else
                markertype     = MarkerTypeTrigger;


            if ( dinfile ) {

                KeepChars       ( code, "0123456789" );
                StringNoSpace   ( code );
                StringClip      ( code, MarkerNameMaxLength - 1 );

                markercode     = (MarkerCode) StringToInteger ( code );
                                        // ready, insert (ie by sorting) new marker
                                        // insert as temp, for further merging overlapping triggers
                InsertMarker ( TMarker ( tf1, tf2, markercode, code, /*markertype*/ MarkerTypeTemp ), false );
                } // if dinfile

            else { // Multi-Port ECI
                                        // build final marker name
                StringCopy   ( markername, code );
                StringAppend ( markername, ":" );

                if      ( StringIsNotEmpty ( description ) )    StringAppend ( markername, description );
                else if ( StringIsNotEmpty ( label       ) )    StringAppend ( markername, label       );
                                        // crop final marker!
                StringClip ( markername, MarkerNameMaxLength - 1 );


                markercode     = (MarkerCode) 100;

                                        // ready, insert (ie by sorting) new marker
//              InsertMarker ( TMarker ( tf1, tf2, markercode, markername, markertype ), false );


                                        // scan associated keys
                do {
                                        // no (more) keys?

                                        // should reach /keys but in case it doesn't exist, it will exhaust the file
                                        // so bind the search to /event instead - Best would be to search for either /keys or fallback to /event
                    if ( ! XMLGotoElement ( fiv, "key", /*"keys"*/ "event" /*"keys event"*/ ) ) {
                        break;
                        }

                                        // pairs of code/data
                    XMLGetElement  ( fiv, "keyCode", "key", keycode );
                    XMLGetElement  ( fiv, "data",    "key", keydata );

                    if ( StringIsNotEmpty ( keydata ) )         StringAppend ( keycode, "=", keydata );
                                        // crop final marker
                    StringClip ( keycode, MarkerNameMaxLength - 1 );

                                        // add "key" marker, associated to current marker
//                  InsertMarker ( TMarker ( tf1, tf2, markercode, keycode, markertype ), false );

                                        // append to a single marker, but length is not enough
                                        // !don't put ',' in the name, BrainVision doesn't like that!
                    StringAppend ( markername, *LastChar ( markername ) == ':' ? " " : " ", keycode );

                    } while ( fiv.good () ); // read event


                                        // append to a big, single marker (note that length can be a problem)
                StringClip ( markername, MarkerNameMaxLength - 1 );

                InsertMarker ( TMarker ( tf1, tf2, markercode, markername, markertype ), false );

                } // if Multi-Port ECI

            } // if tf1 tf2 OK

        } while ( fiv.good () ); // read event


//    allcodes.Show ( "all codes" );

    fiv.close ();

    } // for eventsfile


                                        // din triggers need some reworking: merging & converting to trigger
if ( numdinfile == 1 ) {
                                        // only one din file -> no need to merge, as there is no overlap whatsoever
    MarkersList&    markers        = GetMarkersList ();

                                        // simply convert any trigger of type temp into trigger
    for ( int i = 0; i < markers.Num (); i++ )
        if ( IsFlag ( markers[ i ]->Type, MarkerTypeTemp ) )
            markers[ i ]->Type    = MarkerTypeTrigger;
    }

else if ( numdinfile > 1 ) {
                                        // do some binary fusion on overlapping temp triggers
    MarkersList&        markers            = GetMarkersList ();
    TMarker*            currentmarker      = 0;


    for ( int i = 0; i < markers.Num (); i++ ) {

        if ( ! IsFlag ( markers[ i ]->Type, MarkerTypeTemp ) )
            continue;


        if ( currentmarker == 0 || markers[ i ]->From > currentmarker->From ) {
                                        // start new trigger, we will overwrite on this very guy
            currentmarker          = markers[ i ];
            currentmarker->Type    = MarkerTypeTrigger;  // convert new trigger type, Name has not changed (yet)
            continue;
            }

        else if ( markers[ i ]->SamePosition ( *currentmarker ) ) {
                                        // binary merge of codes
            currentmarker->Code   |= markers[ i ]->Code;
                                        // update Name at each Code change, so we don't have to do anything else when switching to another trigger
            IntegerToString ( currentmarker->Name, currentmarker->Code );

            continue;
            }

//      else // same From, different To, just ignore

        } // foreach marker

                                        // finally, purge out all useless remaining temp triggers
    RemoveMarkers ( MarkerTypeTemp, false );
    }

}


//----------------------------------------------------------------------------
bool	TEegEgiMffDoc::Open	(int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {
                                        // Look for some related files
    TFileName           fileinfo            = GetDocPath ();
    TFileName           fileinfox           = GetDocPath ();
    TFileName           fileepochs          = GetDocPath ();
    TFileName           fileindex           = GetDocPath ();
    TFileName           filepns             = GetDocPath ();


    GetFilename     ( fileindex );
    KeepChars       ( fileindex, "0123456789" );
    StringCleanup   ( fileindex );

    ReplaceFilename ( fileinfo ,    "info.xml" );
    ReplaceFilename ( fileepochs ,  "epochs.xml" );
    ReplaceFilename ( filepns ,     "pnsSet.xml" );

    RemoveFilename  ( fileinfox );
    StringAppend    ( fileinfox, "\\", "info", fileindex, ".xml" );


    if ( ! CanOpenFile ( fileinfo,  CanOpenFileRead ) ) {
        ShowMessage ( "MFF file 'info.xml' is missing,\nwill do without it...", "File Open", ShowMessageWarning );
//      return  false;
        }

    if ( ! CanOpenFile ( fileinfox,  CanOpenFileRead ) ) {
        ShowMessage ( "MFF file 'infoX.xml' is missing,\ncalibration might be wrong...", "File Open", ShowMessageWarning );
//      return  false;
        }

    if ( ! CanOpenFile ( fileepochs,  CanOpenFileRead ) ) {
        ShowMessage ( "MFF file 'epochs.xml' is missing,\nCartool can not proceed any further, sorry...", "File Open", ShowMessageWarning );
        return  false;  // or not?
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InputStream     = InStream ( ofRead | ofBinary );

    if ( ! InputStream ) return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fixed header
    TEegEgi_Mff_Bin_HeaderFixed   mffbinheader;

    InputStream->read ( (char *)(&mffbinheader), sizeof ( mffbinheader ) );

                                        // there sould be a full header, with Version != 0, at the beginning
                                        // though later repetitions of this header, for other blocks, might have a Version == 0
    if ( mffbinheader.Version != 1 ) {
                                        // does not seem to really matter..
        ShowMessage ( "MFF .bin file version not recognized,\nreading file might give some unexpected results...", "File Open", ShowMessageWarning );
//      delete  InputStream; InputStream = 0;
//      return false;
        }

                                        // first block gives number of tracks
    NumElectrodes       = mffbinheader.NumTracks;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    size_t          filesize    = GetFileSize ( GetDocPath () );

                                        // optional header (should be there for the first block!)
                                        // has important infos: (total # of TF) # of blocks
    int             optheaderlength;
                                        // jump to optional header
    InputStream->seekg ( 2 * sizeof ( TEegEgi_Mff_Bin_HeaderVariable1 ) * NumElectrodes, ios::cur );

    InputStream->read  ( (char *)(&optheaderlength), sizeof ( optheaderlength ) );

                                        // get number of blocks
    if ( optheaderlength ) {

        TEegEgi_Mff_Bin_HeaderOptional    mffbinheaderopt;

        InputStream->read ( (char *)(&mffbinheaderopt), sizeof ( mffbinheaderopt ) );

//      if ( mffbinheaderopt.Type == 1 ) { // assume this is always the case, no doc on other options for now
        NumBlocks       = mffbinheaderopt.NumberOfBlocks;
//      NumTimeFrames   = mffbinheaderopt.TotalNumberOfSamples;    // this is the sum of all blocks (ie all sequences)
        }
    else {                              // oops, this is not good news...

                                        // we have to search for the # of blocks ourselves, with an iterative search through all file
        InputStream->seekg ( 0, ios::beg );
        NumBlocks   = 0;

        do {
                                        // get next header
                                        // is it possible that mffbinheader.Version could be 0 here? it doesn't really make sense, and it would complicate some more the scan..
            InputStream->read ( (char *)(&mffbinheader), sizeof ( mffbinheader ) );

//            DBGV5 ( NumBlocks, mffbinheader.Version, mffbinheader.HeaderSize, mffbinheader.DataSize, mffbinheader.NumTracks, "NumBlocks, mffbinheader.Version, mffbinheader.HeaderSize, mffbinheader.DataSize, mffbinheader.NumTracks" );

                                        // eof?
            if ( ! InputStream->good () )
                break;

                                        // no, we are good
            NumBlocks++;

                                        // skip block of data, and repeat
            InputStream->seekg ( mffbinheader.DataSize + mffbinheader.HeaderSize - sizeof ( mffbinheader ), ios::cur );

            } while ( true );

                                        // we basically crashed the stream, so re-open it
        delete  InputStream;

        InputStream     = InStream ( ofRead | ofBinary );

//      ShowMessage ( "MFF file is missing the number of time blocks,\nreading file might give some unexpected results...", "File Open", ShowMessageWarning );
//      NumBlocks       = 1;            // fallback to a single block
//      NumTimeFrames   = MaxSamplesPerBlock;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate block structure
    Blocks.resize ( NumBlocks );


    TArray1<uint>       TrackOffset ( NumElectrodes + 1 );
    TEegEgi_Mff_Bin_HeaderVariable2   mffbinheaderv2;
    TArray1<uchar>      bits      ( NumElectrodes );
    TArray1<uint>       frequency ( NumElectrodes );


    InputStream->seekg ( 0, ios::beg );

                                        // fill the block structure
    for ( int b = 0; b < NumBlocks; b++ ) {

//        DBGV2 ( b + 1, (ulong) InputStream->tellg (), "Block#  ->  Input Stream position" );

                                        // allocate channel info
        Blocks[ b ].ChannelsSpec.resize ( NumElectrodes );


                                        // read only the Version part, as the header could be missing
        InputStream->read ( (char *)(&mffbinheader.Version), sizeof ( mffbinheader.Version ) );


        if ( mffbinheader.Version == 0 ) {
                                        // header is missing, so we rely on the previous one
//            DBGV ( b + 1, "Block#, missing header" );
                                        // this is where data begin
            Blocks[ b ].FileOrigin          = (ulong) InputStream->tellg ();
                                        // copy from previous block
            Blocks[ b ].NumTimeFrames       = Blocks[ b - 1 ].NumTimeFrames;
            Blocks[ b ].SamplingFrequency   = Blocks[ b - 1 ].SamplingFrequency;
                                        // also copy channel info from previous block
            for ( int el = 0; el < NumElectrodes; el++ )
                Blocks[ b ].ChannelsSpec[ el ]  = Blocks[ b - 1 ].ChannelsSpec[ el ];
                                        // skip block data
            InputStream->seekg ( mffbinheader.DataSize, ios::cur );

            continue;
            }


                                        // not null -> read the remaining part of the block header
        InputStream->read  ( (char *)(&mffbinheader) + sizeof ( mffbinheader.Version ), sizeof ( mffbinheader ) - sizeof ( mffbinheader.Version ) );


                                        // header, variable part 1
        for ( int el = 0; el < NumElectrodes; el++ )
            InputStream->read ( (char *)(&TrackOffset[ el ]), TrackOffset.AtomSize () );
                                        // ends with one more = block size
        TrackOffset[ NumElectrodes ]    = mffbinheader.DataSize;


                                        // header, variable part 2
        Blocks[ b ].SamplingFrequency   = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            InputStream->read ( (char *)(&mffbinheaderv2), sizeof ( mffbinheaderv2 ) );

            bits     [ el ] = mffbinheaderv2.BitsPerTracks;
            frequency[ el ] = mffbinheaderv2.GetTrackFrequency ();

//            DBGV3 ( b, el, mffbinheaderv2.GetTrackFrequency (), "block | electrode -> mffbinheaderv2.GetTrackFrequency ()" );
                                        // store max sampling frequency of block
            Blocks[ b ].SamplingFrequency   = max ( frequency[ el ], Blocks[ b ].SamplingFrequency );
            }


        Blocks[ b ].NumTimeFrames   = 0;
                                        // fill channel info with gathered evidences
        for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // size of sample
            Blocks[ b ].ChannelsSpec[ el ].BytesPerSample   = bits[ el ] / 8;
                                        // space between next signal and current gives the size, in bytes
            Blocks[ b ].ChannelsSpec[ el ].ChannelSize      = TrackOffset[ el + 1 ] - TrackOffset[ el ];
                                        // -> # of samples
            Blocks[ b ].ChannelsSpec[ el ].SamplesPerBlock  = Blocks[ b ].ChannelsSpec[ el ].ChannelSize / Blocks[ b ].ChannelsSpec[ el ].BytesPerSample;

                                        // channel size in bytes per block
                                        // keep max # of samples
            Blocks[ b ].NumTimeFrames   = max ( Blocks[ b ].NumTimeFrames, Blocks[ b ].ChannelsSpec[ el ].SamplesPerBlock );
            }

//        DBGV3 ( b, Blocks[ b ].SamplingFrequency, Blocks[ b ].NumTimeFrames, "block -> SF #TF" );

                                        // skip optional header, already read
        InputStream->read  ( (char *)(&optheaderlength), sizeof ( optheaderlength ) );
        if ( optheaderlength )
            InputStream->seekg ( sizeof ( TEegEgi_Mff_Bin_HeaderOptional ), ios::cur );

                                        // this is where data begin
        Blocks[ b ].FileOrigin      = (ulong) InputStream->tellg ();

                                        // skip block data
        InputStream->seekg ( mffbinheader.DataSize, ios::cur );
        } // for block


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // browse the blocks, doing some computations & checking
    MaxSamplesPerBlock  = 0;
    EqualTracks         = true;

    for ( int b = 0; b < NumBlocks; b++ ) {

//      DBGV3 ( b + 1, Blocks[ b ].NumTimeFrames, Blocks[ b ].TimeFrameOrigin, "Block#  ->  #TF  TF origin" );

                                        // max block length
        MaxSamplesPerBlock  = max ( MaxSamplesPerBlock, Blocks[ b ].NumTimeFrames );


        for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // all channels must be 4 bytes floating points
            if ( Blocks[ b ].ChannelsSpec[ el ].BytesPerSample != 4 ) {

                ShowMessage ( "Only single float (4 bytes) data type is suppported for now...", "File Open", ShowMessageWarning );
                delete  InputStream; InputStream = 0;
                return false;
                }

                                        // check for equal tracks, which simplifies the reading
            if ( EqualTracks
              && ( el && Blocks[ b ].ChannelsSpec[ el ].ChannelSize != Blocks[ b     ].ChannelsSpec[ el - 1 ].ChannelSize
                || b  && Blocks[ b ].ChannelsSpec[ el ].ChannelSize != Blocks[ b - 1 ].ChannelsSpec[ el     ].ChannelSize ) )

                EqualTracks = false;
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Electrical Geodesics" );
    StringCopy ( ProductName, FILEEXT_EEGMFF );
    Subversion          = NumElectrodes;

                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    InitAuxiliaries ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reading info file
    char                element [ 65 * KiloByte ];
    ifstream            fi  ( TFileName ( fileinfo, TFilenameExtendedPath ),  ios::binary );

                                        // Note: Version field is from TProductInfo
    if ( XMLGotoElement ( fi, "fileInfo",   0,          0,  true   )
      && XMLGetElement  ( fi, "mffVersion", "fileInfo", element ) ) {

//        DBGM ( element, "info.xml / mffVersion" );

        Version     = StringToInteger ( element );
        }
    else                                // if marker does not exist, then this is version 0
        Version     = 0;

//    DBGV ( Version, "fileInfo::Version" );

    if ( Version > 3 ) {
        ShowMessage ( "MFF info.xml file version not recognized,\nreading file might give some unexpected results...", "File Open", ShowMessageWarning );
//      delete  InputStream; InputStream = 0;
//      return false;
        }


                                        // get date & time?
    if ( XMLGotoElement ( fi, "fileInfo",   0,          0,  true   )
      && XMLGetElement  ( fi, "recordTime", "fileInfo", element ) ) {
                                        // f.ex.: "2009-08-10T15:29:32.368000000-08:00"
                                        // last -08:00 refers to UTC time
//        DBGM ( element, "info.xml / recordTime" );

        int                 yyyy,   MM,     dd;
        int                 hh,     mm;
        double              ss;

        DateTimeStringToFields ( element, yyyy, MM, dd, hh, mm, ss );
                                                                   // seconds include fractions, get it up to the micro-second
                                                                   // nanoseconds, if any (Version 0), will simply be ignored by Cartool after the rounding...
        DateTime            = TDateTime ( yyyy, MM, dd, hh, mm, 0, 0, Round ( ss * 1000000 ) );

                                        // some microseconds included in starting date?
        if ( Fraction ( ss * 1000 ) > 0 )

            DateTime.MicrosecondPrecision   = true;
        }


    fi.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reading infoX file
    ifstream            fix ( TFileName ( fileinfox, TFilenameExtendedPath ), ios::binary );
//  bool                needshighpass;
//    TArray1<double>     Impedances;

                                        // get calibrations?
    if ( XMLGotoElement ( fix, "dataInfo",      0,          0, true )
      && XMLGotoElement ( fix, "calibrations",  "dataInfo", 0 ) ) {

        TStrings        attr;
        double          value;
        int             index;

                                        // read all calibration, actually storing the last of each type
        do {
            if ( ! XMLGotoElement ( fix, "calibration", "calibrations", 0 ) )
                break;                  // no more calibration


            if ( ! XMLGetElement ( fix, "type", "calibration", element )
              || ! IsStringAmong ( element, "GCAL ZCAL ICAL" ) )
                continue;               // not the right type of calibration? skip it


            if ( ! XMLGotoElement ( fix, "channels", "calibration", 0 ) )
                continue;               // no channels info?

//          DBGV ( XMLCountElement ( fix, "ch", "channels" ), "counting channels" );


            if ( StringIs ( element, "GCAL" ) ) {

                Gains.Resize ( NumElectrodes );

                for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // retrieve element, including attributes
                    value   = XMLGetElement ( fix, "ch", "channels", element, &attr ) ? StringToDouble ( element ) : 1;
                                        // attributes should contain the index of channel, starting from 1
                    index   = (int) attr > 2 && StringIs ( attr[ 0 ], "n" ) ? StringToInteger ( attr[ 1 ] ) - 1 : el;

                    Gains[ index ]  = value;

                    //DBGV2 ( index + 1, Gains[ index ], attr[ 1 ] );
                    }
                } // Gains

            else if ( StringIs ( element, "ZCAL" ) ) {

                Zeros.Resize ( NumElectrodes );

                for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // retrieve element, including attributes
                    value   = XMLGetElement ( fix, "ch", "channels", element, &attr ) ? StringToDouble ( element ) : 1;
                                        // attributes should contain the index of channel, starting from 1
                    index   = (int) attr > 2 && StringIs ( attr[ 0 ], "n" ) ? StringToInteger ( attr[ 1 ] ) - 1 : el;

                    Zeros[ index ]  = value;

                    //DBGV2 ( index + 1, Zeros[ index ], attr[ 1 ] );
                    }
                } // Zeros
/*
            else if ( StringIs ( element, "ICAL" ) ) {

                Impedances.Resize ( NumElectrodes );

                for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // retrieve element, including attributes
                    value   = XMLGetElement ( fix, "ch", "channels", element, &attr ) ? StringToDouble ( element ) : 1;
                                        // attributes should contain the index of channel, starting from 1
                    index   = (int) attr > 2 && StringIs ( attr[ 0 ], "n" ) ? StringToInteger ( attr[ 1 ] ) - 1 : el;

                    Impedances[ index ] = value;

//                    DBGV2 ( index + 1, Impedances[ index ], attr[ 1 ] );
                    }
                } // Impedances
*/
            } while ( fix.good () ); // reading calibration
        }

                                        // Give some feedbacks, but maybe only for Gains, as Zeros seem to be always missing(?)
  //if ( ! (bool) Gains &&   (bool) Zeros )     ShowMessage ( "MFF xml file is missing Gains,\nreading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );
  //if (   (bool) Gains && ! (bool) Zeros )     ShowMessage ( "MFF xml file is missing Zeros,\nreading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );
  //if ( ! (bool) Gains && ! (bool) Zeros )     ShowMessage ( "MFF xml file is missing both Zeros and Gains,\nreading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // signal has been filtered?
//  if ( XMLGotoElement ( fix, "dataInfo",      0,          0, true )
//    && XMLGetElement  ( fix, "filters",       "dataInfo", element ) ) {
//
//      needshighpass   = ! StringContains ( element, "highpass" );
//      }
//  else
//      needshighpass   = true;

//    DBGM ( BoolToString ( needshighpass ), "needshighpass" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // type of signal?
//  bool                iseeg           = false;
    bool                ispns           = false;
//  bool                issource        = false;
//  bool                isspectral      = false;
//  bool                isjtf           = false;
//  bool                istvalues       = false;

    if ( XMLGotoElement ( fix, "dataInfo",              0,          0, true )
      && XMLGotoElement ( fix, "generalInformation",    "dataInfo", 0 )
      && XMLGetElement  ( fix, "fileDataType",          "generalInformation",   element ) ) {

//      iseeg       = StringContains ( (const char*) element, (const char*) "EEG" );
        ispns       = StringContains ( (const char*) element, (const char*) "PNSData" );    // Peripheral Nervous System
//      issource    = StringContains ( (const char*) element, (const char*) "EEG" );
//      isspectral  = StringContains ( (const char*) element, (const char*) "spectral" );
//      isjtf       = StringContains ( (const char*) element, (const char*) "JTF" );
//      istvalues   = StringContains ( (const char*) element, (const char*) "tValues" );
        }

//    DBGM2 ( BoolToString ( iseeg ), BoolToString ( ispns ), "EEG? PNS?" );

    fix.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // there seems to be additional info in "pnsSet.xml"
                                        // we are going to retrieve the names
    if ( ispns && CanOpenFile ( filepns, CanOpenFileRead ) ) {

        ifstream            fipns ( TFileName ( filepns, TFilenameExtendedPath ), ios::binary );

        if ( XMLGotoElement ( fipns, "PNSSet",      0,          0, true )
          && XMLGotoElement ( fipns, "sensors",     "PNSSet",   0 ) ) {

            int             el          = -1;

            do {
                el++;

                if ( ! XMLGotoElement ( fipns, "sensor", "sensors", 0 )
                  || el >= NumElectrodes )
                    break;              // no more sensors


                if ( ! XMLGetElement ( fipns, "name", "sensor", element ) )
                    continue;
                                        // yep, these names are quite long
//              StringShrink     ( element, element, ElectrodeNameSize - 1 );
                                        // overwrite our default names
                StringCopy ( ElectrodesNames[ el ], element );

                } while ( fipns.good () );// reading calibration

            } // if sensors

        } // if ispns


    if ( ispns ) {
                                        // set all electrodes to auxs
        AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );
        AuxTracks.Set ( 0, NumElectrodes - 1 );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // check impedances
    if ( (bool) Impedances ) {
        TEasyStats          stats ( NumElectrodes );


        for ( int el = 0; el < NumElectrodes; el++ )
            stats.Add ( Impedances[ el ] );

        stats.Show ( "Impedances" );
        DBGV2 ( stats.Median (), stats.MAD (), "Median MAD" );

        double              badimpedance    = ( stats.Median () + stats.Max () ) / 2;

        for ( int el = 0; el < NumElectrodes; el++ )
            if ( Impedances[ el ] > badimpedance )
                DBGV2 ( el + 1, Impedances[ el ], "el#  bad impedance" );
        }
*/

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ifstream            fie ( TFileName ( fileepochs, TFilenameExtendedPath ), ios::binary );


    if ( XMLGotoElement ( fie, "epochs", 0, 0, true )
      && ( NumSequences = XMLCountElement ( fie, "epoch", "epochs" ) ) != 0 ) {

                                        // allocate sequence info
        Sequences.resize ( NumSequences );

        char                begintime   [ 32 ];
        char                endtime     [ 32 ];
        char                firstblock  [ 32 ];
        char                lastblock   [ 32 ];
        int                 sec;
        int                 millisec;
        int                 microsec;
        int                 nanosec;


        for ( int i = 0; i < NumSequences; i++ ) {
                                        // next epoch
            XMLGotoElement ( fie, "epoch", "epochs", 0 );

                                        // get infos
            XMLGetElement ( fie, "beginTime",  "epoch", begintime  );
            XMLGetElement ( fie, "endTime",    "epoch", endtime    );
            XMLGetElement ( fie, "firstBlock", "epoch", firstblock );
            XMLGetElement ( fie, "lastBlock",  "epoch", lastblock  );
//            DBGM4 ( begintime, endtime, firstblock, lastblock, "begintime, endtime, firstblock, lastblock" );

                                        // process info into useful sequence structure

                                        // expand duration string into sub-fields
            DurationStringToFields ( begintime, sec, millisec, microsec, nanosec );


            Sequences[ i ].FirstBlock           = Clip ( (ULONG) StringToInteger ( firstblock ) - 1, (ULONG) 0, NumBlocks - 1 );    // clip by security, in case NumBlocks might not have been set correctly
            Sequences[ i ].LastBlock            = Clip ( (ULONG) StringToInteger ( lastblock  ) - 1, (ULONG) 0, NumBlocks - 1 );
            Sequences[ i ].SamplingFrequency    = Blocks[ (int) Sequences[ i ].FirstBlock ].SamplingFrequency;            // pick first sampling frequency, a full check is done below
            Sequences[ i ].DateTime             = TDateTime ( DateTime.GetYear (), DateTime.GetMonth (), DateTime.GetDay (),
                                                              DateTime.GetHour        (),
                                                              DateTime.GetMinute      (),
                                                              DateTime.GetSecond      () + sec,
                                                              DateTime.GetMillisecond () + millisec,
                                                              DateTime.GetMicrosecond () + microsec
                                                                                         + Round ( nanosec / 1000.0 ) );

                                        // some microseconds included in starting date or high sampling frequency?
            Sequences[ i ].DateTime.MicrosecondPrecision    = DateTime.MicrosecondPrecision || Sequences[ i ].SamplingFrequency > 1000.0;

            Sequences[ i ].NumTimeFrames        = 0;

                                        // look for all blocks that belong to this sequence
            for ( int b = Sequences[ i ].FirstBlock; b <= Sequences[ i ].LastBlock; b++ ) {
                                        // sum up all TFs
                Sequences[ i ].NumTimeFrames   += Blocks[ b ].NumTimeFrames;
                                        // compute relative TF origin for each block
                Blocks[ b ].TimeFrameOrigin     = b == Sequences[ i ].FirstBlock ? 0 : Blocks[ b - 1 ].TimeFrameOrigin + Blocks[ b - 1 ].NumTimeFrames;
                } // for block

//          DBGV5 ( i, NumBlocks, Sequences[ i ].FirstBlock, Sequences[ i ].LastBlock, Sequences[ i ].NumTimeFrames, "Sequence# NumBlocks-> firstblock, lastblock, #TF" );
            } // for epoch
        }
    else {
        ShowMessage ( "MFF file 'epochs.xml' seems incomplete," NewLine "Cartool can not proceed any further, sorry...", "File Open", ShowMessageWarning );
        delete  InputStream; InputStream = 0;
        return  false;
        }

//    DBGV ( NumSequences, "NumSequences" );

    fie.close ();

                                        // check all sequences have a consistent sampling frequency (though it can vary from sequence to sequence)
    for ( int i = 0; i < NumSequences; i++ )
    for ( int b = Sequences[ i ].FirstBlock + 1; b <= Sequences[ i ].LastBlock; b++ )

        if ( Blocks[ b - 1 ].SamplingFrequency != Blocks[ b ].SamplingFrequency ) {

            ShowMessage ( "MFF file seems to have different sampling frequencies within the same sequence," NewLine "Cartool can not proceed any further, sorry...", "File Open", ShowMessageWarning );
            delete  InputStream; InputStream = 0;
            return  false;
            }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // using the first sequence at opening time, or the next one if it appears to be too short
    CurrSequence        = 0;

    if ( NumSequences > 1 
      && Sequences[ 0 ].NumTimeFrames < Sequences[ 1 ].NumTimeFrames / 2 )
        CurrSequence        = 1;


    NumTimeFrames       = Sequences[ CurrSequence ].NumTimeFrames;
    SamplingFrequency   = Sequences[ CurrSequence ].SamplingFrequency;
    DateTime            = Sequences[ CurrSequence ].DateTime;

                                        // update in case of sequences
    UpdateTitle ();

    }
else {                                  // can not create
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegEgiMffDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.Resize ( MaxSamplesPerBlock );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    IntegerToString ( ElectrodesNames[ i - 1 ], i );

StringCopy ( ElectrodesNames[ NumElectrodes - 1 ], "VREF" );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegEgiMffDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
int                 blockmax        = -1;
int                 block;

                                        // get first & last block indexes
if ( Sequences[ CurrSequence ].FirstBlock == Sequences[ CurrSequence ].LastBlock )

    block   = blockmax  = Sequences[ CurrSequence ].FirstBlock;

else
    for ( block = Sequences[ CurrSequence ].LastBlock; block >= Sequences[ CurrSequence ].FirstBlock; block-- ) {

        if ( (ulong) tf2 >= Blocks[ block ].TimeFrameOrigin && blockmax == -1 )
            blockmax    = block;

        if ( (ulong) tf1 >= Blocks[ block ].TimeFrameOrigin )
            break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get first time frame within first block
int                 firsttfinblock  = tf1 - Blocks[ block ].TimeFrameOrigin;
int                 numtfinblock    = 0;

                                        // loop through all blocks
for ( ; block <= blockmax; tfoffset      += numtfinblock /*Blocks[ block ].NumTimeFrames - firsttfinblock*/,
                           block++,
                           firsttfinblock = 0 ) {

    int         firsttf         = Blocks[ block ].TimeFrameOrigin + firsttfinblock;
                                        // max number of tf to be read in this block
                numtfinblock    = Blocks[ block ].NumTimeFrames   - firsttfinblock;

    int         maxtfperblock   = Blocks[ block ].NumTimeFrames - 1;
                                        // origin of next full track to be read
    ULONG       nextelpos       = Blocks[ block ].FileOrigin;

    ULONG       tfremainingread = tf2 - firsttf + 1;

//  InputStream->seekg ( nextelpos, ios::beg );


                                        // in the block, values for one electrode are consecutives
    auto&       toch            = Blocks[ block ].ChannelsSpec;

    for ( int el = 0; el < NumElectrodes; el++ ) {

                                        // get that channel length
        int         tfpertrack      = toch[ el ].SamplesPerBlock - 1;
                                        // get the complete line for this electrode - very inefficient
//      InputStream->read ( (char*) Tracks.GetArray (), toch->ChannelSize );
                                        // optimal jump to where data are
        InputStream->seekg ( nextelpos + firsttfinblock * sizeof ( float ), ios::beg );
                                        // so that we can read the min # of data: min of remaining block or remaining data
        ULONG       tfminread       = min ( toch[ el ].SamplesPerBlock - firsttfinblock, tfremainingread );
                                        // still put the data at offset position
        InputStream->read ( (char*) ( Tracks.GetArray () + firsttfinblock ), tfminread * sizeof ( float ) );
                                        // next channel beginning
                    nextelpos      += toch[ el ].ChannelSize;


        for ( int tfi = firsttf, tf = 0; tfi <= tf2 && tf < numtfinblock; tfi++, tf++ ) {

            float*  toF = Tracks.GetArray () + ( EqualTracks ? firsttfinblock + tf 
                                                             : Round ( ( ( firsttfinblock + tf ) / (double) maxtfperblock ) * tfpertrack ) );

//          if ( IsNotAProperNumber ( *toF ) )
//              buff ( el, tfoffset + tf ) = 0;
//          else
                                        // don't know if all possibilities can occur, so put all formulas
                buff ( el, tfoffset + tf )  = (bool) Gains && (bool) Zeros  ? ( *toF - Zeros[ el ] ) * Gains[ el ]
                                            : (bool) Gains                  ?   *toF                 * Gains[ el ]
                                            :                 (bool) Zeros  ?   *toF - Zeros[ el ]
                                                                            :   *toF;
            } // for tfi
        } // for el

    } // for block
}


//----------------------------------------------------------------------------
bool    TEegEgiMffDoc::UpdateSession ( int newsession )
{
NumTimeFrames       = Sequences[ newsession ].NumTimeFrames;
SamplingFrequency   = Sequences[ newsession ].SamplingFrequency;
DateTime            = Sequences[ newsession ].DateTime;

return  true;
}


//----------------------------------------------------------------------------
void    TEegEgiMffDoc::UpdateTitle ()
{
TFileName           buff            = GetDocPath ();
char*               tolastbackslash;
char*               tonexttolastbackslash;

                                        // get last backslash
tolastbackslash         = ToLastDirectory ( buff );
                                        // temporarily replace it
*tolastbackslash        = '|';
                                        // now get the next to last backslash
tonexttolastbackslash   = ToLastDirectory ( buff );
                                        // restore last backslash
*tolastbackslash        = '\\';
                                        // finally copy, skipping the leading backslash
StringCopy ( buff, tonexttolastbackslash + 1 );

if ( NumSequences > 1 )
    StringAppend ( buff, ":", IntegerToString ( GetCurrentSession () ) );

SetTitle ( buff );

return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
