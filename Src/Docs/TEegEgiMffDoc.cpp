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
void    TEegEgiMffDoc::DateTimeStringToFields ( const char* datetime, int &year, int &month, int &day, int &hour, int &minute, double &second ) const
{
                                        // format is well known, just the seconds could be micro or nano seconds, but we don't care as it remains in the fraction!
sscanf ( datetime, "%4d-%2d-%2dT%2d:%2d:%lf", &year, &month, &day, &hour, &minute, &second );
}

                                        // converts a duration string to micro-seconds, with potentially a fractional part
double  TEegEgiMffDoc::DurationStringToMicroseconds ( const char* duration )    const
{
return  StringToDouble ( duration ) / ( IsNanoSecondsPrecision () ? 1000 : 1 );
}

                                        // unfold a duration string up to seconds fields (which should be enough precision wise)
void    TEegEgiMffDoc::DurationStringToFields ( const char* duration, int& sec, int& millisec, int& microsec, int& nanosec )  const
{
sec         = 0;
millisec    = 0;
microsec    = 0;
nanosec     = 0;


char                buff        [ 256 ];
StringCopy  ( buff, duration );

int                l                = StringLength ( buff );

auto                convertnext3digits  = [ &l, &buff ] ( int& v ) 
{
if ( l == 0 )    
    return;

l           = AtLeast ( 0, l - 3 );
v           = StringToInteger ( buff + l );
buff[ l ]   = EOS;  // clip the used end of string
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
Version             = 0;

NumBlocks           = 0;
MaxSamplesPerBlock  = 0;
EqualTracks         = true;

Reference           = ReferenceAsInFile;
}


bool	TEegEgiMffDoc::Close ()
{
FileStream.Close ();

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
                AppendMarker ( TMarker ( tf1, tf2, markercode, code, /*markertype*/ MarkerTypeTemp ) );
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
//              AppendMarker ( TMarker ( tf1, tf2, markercode, markername, markertype ) );


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
//                  AppendMarker ( TMarker ( tf1, tf2, markercode, keycode, markertype ) );

                                        // append to a single marker, but length is not enough
                                        // !don't put ',' in the name, BrainVision doesn't like that!
                    StringAppend ( markername, *LastChar ( markername ) == ':' ? " " : " ", keycode );

                    } while ( fiv.good () ); // read event


                                        // append to a big, single marker (note that length can be a problem)
                StringClip ( markername, MarkerNameMaxLength - 1 );

                AppendMarker ( TMarker ( tf1, tf2, markercode, markername, markertype ) );

                } // if Multi-Port ECI

            } // if tf1 tf2 OK

        } while ( fiv.good () ); // read event


    SortAndCleanMarkers ();

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
    RemoveMarkers ( MarkerTypeTemp );
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
        ShowMessage (   "MFF file 'info.xml' is missing," NewLine 
                        "will do without it...", 
                        "File Open", ShowMessageWarning );
//      return  false;
        }

    if ( ! CanOpenFile ( fileinfox,  CanOpenFileRead ) ) {
        ShowMessage (   "MFF file 'infoX.xml' is missing," NewLine 
                        "calibration might be wrong...",
                        "File Open", ShowMessageWarning );
//      return  false;
        }

    if ( ! CanOpenFile ( fileepochs,  CanOpenFileRead ) ) {
        ShowMessage (   "MFF file 'epochs.xml' is missing," NewLine 
                        "Cartool can not proceed any further, sorry...", 
                        "File Open", ShowMessageWarning );
        return  false;  // or not?
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( ! FileStream.Open ( GetDocPath(), FileStreamRead ) )
        return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fixed header
    TEegEgi_Mff_Bin_HeaderFixed   mffbinheader;

    FileStream.Read ( &mffbinheader, sizeof ( mffbinheader ) );

                                        // there sould be a full header, with Version != 0, at the beginning
                                        // though later repetitions of this header, for other blocks, might have a Version == 0
    if ( mffbinheader.Version != 1 ) {
                                        // does not seem to really matter..
        ShowMessage (   "MFF .bin file version not recognized," NewLine 
                        "reading file might give some unexpected results...", 
                        "File Open", ShowMessageWarning );

//      FileStream.Close ();
// 
//      return false;
        }

                                        // first block gives number of tracks
    NumElectrodes       = mffbinheader.NumberSignals;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // jump to optional header
    FileStream.SeekCurrent  ( NumElectrodes * sizeof ( TEegEgi_Mff_Bin_HeaderVariable1 )
                            + NumElectrodes * sizeof ( TEegEgi_Mff_Bin_HeaderVariable2 ) );

                                        // optional header (should be there for the first block!) has important infos: (total # of TF) # of blocks
    TEegEgi_Mff_Bin_HeaderOptionalLength    optheaderlength;

    FileStream.Read ( &optheaderlength, sizeof ( optheaderlength ) );


    INT32       headertotalduration     = 0;

                                        // get number of blocks
    if ( optheaderlength != 0 ) {

        TEegEgi_Mff_Bin_HeaderOptional    mffbinheaderopt;

        if ( optheaderlength != sizeof ( TEegEgi_Mff_Bin_HeaderOptional ) )
            ShowMessage ( "Error in Optional Header, file might be corrupted...", "File Open", ShowMessageWarning );

        FileStream.Read ( &mffbinheaderopt, sizeof ( mffbinheaderopt ) );

//      if ( mffbinheaderopt.EGIType == 1 ) { // assume this is always the case, no doc on other options for now

            NumBlocks           = mffbinheaderopt.NumberOfBlocks;
            headertotalduration = mffbinheaderopt.NumberOfSamples;  // this is the sum of all blocks (ie all sequences)
//          }
        }
    else {                              // hu-ho, bad news...

                                        // we have to search for the # of blocks ourselves, with an iterative search through all file
        FileStream.SeekBegin ();
        NumBlocks   = 0;

        do {
                                        // get next header
                                        // is it possible that mffbinheader.Version could be 0 here? it doesn't really make sense, and it would complicate some more the scan..
            FileStream.Read ( &mffbinheader, sizeof ( mffbinheader ) );

            if ( FileStream.IsEndOfFile () )
                break;

                                        // no, we are good
            NumBlocks++;

                                        // skip block of data, and repeat
            FileStream.SeekCurrent ( mffbinheader.DataBlockSize + mffbinheader.HeaderSize - sizeof ( mffbinheader ) );

            } while ( true );

                                        // we basically crashed the stream, so re-open it
        FileStream.Close ();
        FileStream.Open  ( GetDocPath(), FileStreamRead );


//      ShowMessage ( "MFF file is missing the number of time blocks," NewLine "reading file might give some unexpected results...", "File Open", ShowMessageWarning );
//      NumBlocks       = 1;            // fallback to a single block
//      NumTimeFrames   = MaxSamplesPerBlock;
        } // optheaderlength == 0


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate block structure
    Blocks.resize ( NumBlocks );


    TArray1<TEegEgi_Mff_Bin_HeaderVariable1>    TrackOffset ( NumElectrodes + 1 );
    TEegEgi_Mff_Bin_HeaderVariable2             mffbinheaderv2;
    TArray1<UINT8>                              bits        ( NumElectrodes );
    TArray1<INT32>                              frequency   ( NumElectrodes );


    FileStream.SeekBegin ();

                                        // fill the block structure
    for ( int b = 0; b < NumBlocks; b++ ) {
                                        // allocate channel info
        TEegEgi_Mff_Bin_Block&      block       = Blocks[ b ];

        block.ChannelsSpec.Resize ( NumElectrodes );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read only the Version part, as the header could be missing
        FileStream.Read ( &mffbinheader.Version, sizeof ( mffbinheader.Version ) );

                                        // To save space, a missing header means repeating the last one
        if ( mffbinheader.Version == 0 ) {
                                        // this is where data begin
            block.FileOrigin          = FileStream.Tell ();
                                        // copy from previous block
            block.BlockDuration       = Blocks[ b - 1 ].BlockDuration;

            block.SamplingFrequency   = Blocks[ b - 1 ].SamplingFrequency;
                                        // also copy channel info from previous block
            for ( int el = 0; el < NumElectrodes; el++ )
                block.ChannelsSpec[ el ]  = Blocks[ b - 1 ].ChannelsSpec[ el ];

                                        // skip block data
            FileStream.SeekCurrent ( mffbinheader.DataBlockSize );

            continue;
            }

//      else if ( mffbinheader.Version != 1 ) 
//          problem;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not null -> read the remaining part of the block header
        FileStream.Read  ( (char *)(&mffbinheader) + sizeof ( mffbinheader.Version ), sizeof ( mffbinheader ) - sizeof ( mffbinheader.Version ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // header, variable part 1
        for ( int el = 0; el < NumElectrodes; el++ )
            FileStream.Read ( &TrackOffset[ el ], sizeof ( TEegEgi_Mff_Bin_HeaderVariable1 ) );
                                        // ends with one more = block size
        TrackOffset[ NumElectrodes ]    = mffbinheader.DataBlockSize;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // header, variable part 2
        block.SamplingFrequency   = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            FileStream.Read ( &mffbinheaderv2, sizeof ( mffbinheaderv2 ) );

            bits     [ el ] = mffbinheaderv2.BitsPerTracks;
            frequency[ el ] = mffbinheaderv2.GetTrackFrequency ();
                                        // store max sampling frequency of block
            Maxed ( block.SamplingFrequency, frequency[ el ] );
            }


        block.BlockDuration   = 0;
                                        // fill channel info with gathered evidences
        for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // size of sample
            block.ChannelsSpec[ el ].BytesPerSample   = bits[ el ] / 8;
                                        // space between next signal and current gives the size, in bytes
            block.ChannelsSpec[ el ].ChannelSize      = TrackOffset[ el + 1 ] - TrackOffset[ el ];
                                        // -> # of samples
            block.ChannelsSpec[ el ].SamplesPerBlock  = block.ChannelsSpec[ el ].ChannelSize / block.ChannelsSpec[ el ].BytesPerSample;

                                        // channel size in bytes per block
                                        // keep max # of samples
            Maxed ( block.BlockDuration, block.ChannelsSpec[ el ].SamplesPerBlock );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip optional header, already read
        FileStream.Read  ( &optheaderlength, sizeof ( optheaderlength ) );


        if ( optheaderlength != 0 ) {

            //if ( optheaderlength != sizeof ( TEegEgi_Mff_Bin_HeaderOptional ) )
            //    ShowMessage ( "Error in Optional Header, file might be corrupted...", "File Open", ShowMessageWarning );

            FileStream.SeekCurrent ( optheaderlength );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, this is where data begin
        block.FileOrigin      = FileStream.Tell ();

                                        // skip block data
        FileStream.SeekCurrent ( mffbinheader.DataBlockSize );
        } // for block


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // browse the blocks, doing some computations & checking
    MaxSamplesPerBlock  = 0;
    EqualTracks         = true;

    for ( int b = 0; b < NumBlocks; b++ ) {
                                        // max block length
        Maxed ( MaxSamplesPerBlock, Blocks[ b ].BlockDuration );


        for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // all channels must be 4 bytes floating points
            if ( Blocks[ b ].ChannelsSpec[ el ].BytesPerSample != 4 ) {

                ShowMessage ( "Only single float (4 bytes) data type is suppported for now...", "File Open", ShowMessageWarning );

                FileStream.Close ();

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

        FileStream.Close ();

        return false;
        }


    InitAuxiliaries ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reading info file
    char                element [ 64 * KiloByte ];
    ifstream            fi  ( TFileName ( fileinfo, TFilenameExtendedPath ),  ios::binary );

                                        // Note: Version field is from TProductInfo
    if ( XMLGotoElement ( fi, "fileInfo",   0,          0,  true   )
      && XMLGetElement  ( fi, "mffVersion", "fileInfo", element ) ) {

        Version     = StringToInteger ( element );
        }
    else                                // if marker does not exist, then this is version 0
        Version     = 0;


    if ( Version > 3 ) {

        ShowMessage (   "MFF info.xml file version not recognized," NewLine 
                        "reading file might give some unexpected results...", 
                        "File Open", ShowMessageWarning );

//      FileStream.Close ();
// 
//      return false;
        }


                                        // get date & time?
    if ( XMLGotoElement ( fi, "fileInfo",   0,          0,  true   )
      && XMLGetElement  ( fi, "recordTime", "fileInfo", element ) ) {
                                        // f.ex.: "2009-08-10T15:29:32.368000000-08:00"
                                        // last -08:00 refers to UTC time
        int                 yyyy,   MM,     dd;
        int                 hh,     mm;
        double              ss;

        DateTimeStringToFields ( element, yyyy, MM, dd, hh, mm, ss );
                                                                   // seconds include fractions, get it up to the micro-second
                                                                   // nanoseconds, if any (Version 0), will simply be ignored by Cartool after the rounding...
        DateTime            = TDateTime ( yyyy, MM, dd, hh, mm, 0, 0, Truncate ( ss * 1000000 ) );

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


            if ( StringIs ( element, "GCAL" ) ) {

                Gains.Resize ( NumElectrodes );

                for ( int el = 0; el < NumElectrodes; el++ ) {
                                        // retrieve element, including attributes
                    value   = XMLGetElement ( fix, "ch", "channels", element, &attr ) ? StringToDouble ( element ) : 1;
                                        // attributes should contain the index of channel, starting from 1
                    index   = (int) attr > 2 && StringIs ( attr[ 0 ], "n" ) ? StringToInteger ( attr[ 1 ] ) - 1 : el;

                    Gains[ index ]  = value;
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
                    }
                } // Impedances
*/
            } while ( fix.good () ); // reading calibration
        }

                                        // Give some feedbacks, but maybe only for Gains, as Zeros seem to be always missing(?)
  //if ( ! (bool) Gains &&   (bool) Zeros )     ShowMessage ( "MFF xml file is missing Gains," NewLine "reading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );
  //if (   (bool) Gains && ! (bool) Zeros )     ShowMessage ( "MFF xml file is missing Zeros," NewLine "reading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );
  //if ( ! (bool) Gains && ! (bool) Zeros )     ShowMessage ( "MFF xml file is missing both Zeros and Gains," NewLine "reading file might give some unexpected results...", ToFileName ( fileinfox ), ShowMessageWarning );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // signal has been filtered?
//  if ( XMLGotoElement ( fix, "dataInfo",      0,          0, true )
//    && XMLGetElement  ( fix, "filters",       "dataInfo", element ) ) {
//
//      needshighpass   = ! StringContains ( element, "highpass" );
//      }
//  else
//      needshighpass   = true;


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

                                        // Rough estimate of full file duration
        INT64       filesize                = GetFileSize ( GetDocPath () );
        INT32       filesizeduration        = ( filesize - Blocks[ 0 ].FileOrigin ) / ( NumElectrodes * sizeof ( float ) );
                    filesizeduration        = TruncateTo ( filesizeduration, Blocks[ 0 ].BlockDuration );   // last valid block size is always equal or smaller than all previous block sizes

                                        // we can compare the total duration found in main header, but only if there is only 1 sequence
        bool        headertotaldurationOK   = headertotalduration != 0 
                                           && NumSequences == 1;

                                        // we can compare the file size estimated duration only for 1 sequence
        bool        filesizedurationOK      = headertotaldurationOK
                                           && ( headertotalduration - filesizeduration ) > Blocks[ 0 ].BlockDuration    // difference between header duration and file size duration should be significant
                                           && NumSequences == 1;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

                                        // process info into useful sequence structure

                                        // expand duration string into sub-fields
            DurationStringToFields ( begintime, sec, millisec, microsec, nanosec );


            Sequences[ i ].FirstBlock           = Clip ( StringToInteger ( firstblock ) - 1, 0, NumBlocks - 1 );    // clipping by safety
            Sequences[ i ].LastBlock            = Clip ( StringToInteger ( lastblock  ) - 1, 0, NumBlocks - 1 );
            Sequences[ i ].SamplingFrequency    = Blocks[ Sequences[ i ].FirstBlock ].SamplingFrequency;            // pick first sampling frequency, a full check is done below
            Sequences[ i ].DateTime             = TDateTime ( DateTime.GetYear (), DateTime.GetMonth (), DateTime.GetDay (),
                                                              DateTime.GetHour        (),
                                                              DateTime.GetMinute      (),
                                                              DateTime.GetSecond      () + sec,
                                                              DateTime.GetMillisecond () + millisec,
                                                              DateTime.GetMicrosecond () + microsec
                                                                                         + Truncate ( nanosec / 1000.0 ) );

                                        // some microseconds included in starting date or high sampling frequency?
            Sequences[ i ].DateTime.MicrosecondPrecision    = DateTime.MicrosecondPrecision || Sequences[ i ].SamplingFrequency > 1000.0;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // conmputing duration from all blocks within this sequence
            INT32       sumblocksduration   = 0;

            for ( int b = Sequences[ i ].FirstBlock; b <= Sequences[ i ].LastBlock; b++ ) 
                sumblocksduration          += Blocks[ b ].BlockDuration;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // conmputing duration from all blocks within this sequence
                                        // computing duration from tags
            INT32       epochsduration      = MicrosecondsToTimeFrame (   DurationStringToMicroseconds ( endtime   )
                                                                        - DurationStringToMicroseconds ( begintime ), 
                                                                        Sequences[ i ].SamplingFrequency );

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can have between 2 to 4 different durations(!), pick the smallest value from the valid ones
                                        // Note: it seems good enough to just clip NumTimeFrames, which will prevent access to the suspcious blocks,
                                        // without the need to actually remove them from the Blocks structure

                                        // First, get the min duration from the current sequence
            Sequences[ i ].NumTimeFrames        = min ( sumblocksduration, epochsduration );

                                        // Then the min with any of the global durations, if valids
            if ( headertotaldurationOK )
                Mined ( Sequences[ i ].NumTimeFrames, headertotalduration );


            if ( filesizedurationOK )
                Mined ( Sequences[ i ].NumTimeFrames, filesizeduration );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // any discrepancy between the 4 numbers?
            if ( headertotalduration   != sumblocksduration && headertotaldurationOK
              || headertotalduration   != epochsduration    && headertotaldurationOK
              || headertotalduration   != filesizeduration  && headertotaldurationOK && filesizedurationOK
              || filesizeduration      != sumblocksduration                          && filesizedurationOK
              || filesizeduration      != epochsduration                             && filesizedurationOK
              || sumblocksduration     != epochsduration                                                           
               ) {

                char        buff[ KiloByte ];

                StringCopy      ( buff, "Found some MFF file duration inconsistencies:" NewLine );
                StringAppend    ( buff, NewLine " - " "Sequence #", IntegerToString ( i + 1 ), " sum of blocks     : ", TimeFrameToString ( sumblocksduration ),            " [TF]" );
                StringAppend    ( buff, NewLine " - " "Sequence #", IntegerToString ( i + 1 ), " epochs duration : ",   TimeFrameToString ( epochsduration ),               " [TF]" );

                if ( headertotaldurationOK )
                    StringAppend    ( buff, NewLine " - " "Total header duration              : ",                      TimeFrameToString ( headertotalduration ),          " [TF]" );

                if ( filesizedurationOK )
                    StringAppend    ( buff, NewLine " - " "File size duration                     : ",                  TimeFrameToString ( filesizeduration ),             " [TF]" );

                StringAppend    ( buff, NewLine NewLine "Opting for duration                    : ",                    TimeFrameToString ( Sequences[ i ].NumTimeFrames ), " [TF]" );

                ShowMessage     ( buff, "File Open", ShowMessageWarning );
                }

            } // for epoch

        } // epochs

    else { // no epochs
        ShowMessage (   "MFF file 'epochs.xml' seems incomplete," NewLine 
                        "Cartool can not proceed any further, sorry...", 
                        "File Open", ShowMessageWarning );

        FileStream.Close ();

        return  false;
        }

    fie.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check all sequences have a consistent sampling frequency (though it can vary from sequence to sequence)
    for ( int i = 0; i < NumSequences; i++ )
    for ( int b = Sequences[ i ].FirstBlock + 1; b <= Sequences[ i ].LastBlock; b++ )

        if ( Blocks[ b - 1 ].SamplingFrequency != Blocks[ b ].SamplingFrequency ) {

            ShowMessage (   "MFF file seems to have different sampling frequencies within the same sequence," NewLine 
                            "Cartool can not proceed any further, sorry...", 
                            "File Open", ShowMessageWarning );

            FileStream.Close ();

            return  false;
            }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // using the first sequence at opening time, or the next one if it appears to be too short
    CurrSequence        = 0;

    if ( HasMultipleSessions ()
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
INT32               firstblock          = Sequences[ CurrSequence ].FirstBlock;
INT32               firstblockduration  = Blocks[ firstblock ].BlockDuration;       // all blocks, except last block, have the same duration

INT32               blockmin            = firstblock + tf1 / firstblockduration;    // get blocks indexes - OK for last block
INT32               blockmax            = firstblock + tf2 / firstblockduration;
                                        
INT32               blocktforigin       = TruncateTo ( tf1, firstblockduration );   // first time frame of the first block - OK for last block


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

INT32               firsttfinblock  = tf1 - blocktforigin;                      // where tf1 lies within first block
INT32               numtfinblock;

                                        // loop through all blocks
for ( INT32 block = blockmin; block <= blockmax; block++ ) {

    INT32       blockduration   = Blocks[ block ].BlockDuration;                // current block duration - some file can have a whole duration which make the last block "shorter"

    INT32       firsttf         = blocktforigin + firsttfinblock;               // first tf within current block

                numtfinblock    = blockduration - firsttfinblock;               // max number of tf to be read in this block

    INT32       tfremainingread = tf2 - firsttf + 1;                            // total, global remaining number of tf to read

    INT64       nextelfilepos   = Blocks[ block ].FileOrigin;                   // origin of next full track to be read

  //FileStream.SeekBegin ( nextelfilepos );                                     // used for complete line of electrode

                                        // in the block, values for one electrode are consecutives
    const TEegEgi_Mff_Bin_Channel*      toch        = Blocks[ block ].ChannelsSpec.GetArray ();

    for ( int el = 0; el < NumElectrodes; el++ ) {

      //FileStream.Read ( Tracks.GetArray (), toch[ el ].ChannelSize );         // get the complete line for this electrode - very inefficient

                                        
        FileStream.SeekBegin ( nextelfilepos + firsttfinblock * sizeof ( float ) );                         // optimal jump to where the data actually is..

        INT32       tfminread       = min ( toch[ el ].SamplesPerBlock - firsttfinblock, tfremainingread ); // ..so that we can read the minimum amount of data: min of remaining block OR of remaining data
                                        
        FileStream.Read ( Tracks.GetArray () + firsttfinblock, tfminread * sizeof ( float ) );              // put the data at the correct offset position
                                        
                    nextelfilepos  += toch[ el ].ChannelSize;                                               // next channel beginning


        for ( int tfi = firsttf, tf = 0; tfi <= tf2 && tf < numtfinblock; tfi++, tf++ ) {

            float*  toF = Tracks.GetArray () + ( EqualTracks ?             firsttfinblock + tf
                                                             : Round ( ( ( firsttfinblock + tf ) / (double) ( blockduration - 1 ) ) * ( toch[ el ].SamplesPerBlock - 1 ) ) );

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


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // next block
    tfoffset       += numtfinblock;     // next tf insertion
    blocktforigin  += blockduration;    // next block's origin
    firsttfinblock = 0;                 // for all subsequent blocks, first tf to consider is always the first one
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

if ( HasMultipleSessions () )
    StringAppend ( buff, ":", IntegerToString ( GetCurrentSession () ) );

SetTitle ( buff );

return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
