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

#include    "Dialogs.Input.h"
#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "Math.Utils.h"
#include    "CartoolTypes.h"            // EpochsType
#include    "TList.h"
#include    "TArray1.h"

#include    "TTFCursor.h"
#include    "TTracksDoc.h"

#include    "TMarkers.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMarker::TMarker ()
{
From        = 0;
To          = 0;
Code        = 0;
Type        = MarkerTypeUnknown;
ClearString ( Name );
}


        TMarker::TMarker ( long pos, const char* name, MarkerType type )
{
From        =
To          = pos;

Code        = MarkerDefaultCode;
Type        = type;

ClearVirtualMemory  ( Name, MarkerNameMaxLength );
StringCopy          ( Name, name );
}


        TMarker::TMarker ( long   from, long  to, MarkerCode code, const char* name, MarkerType type )
{
From        = from;
To          = to;

Code        = code;
Type        = type;

ClearVirtualMemory  ( Name, MarkerNameMaxLength );
StringCopy          ( Name, name );

CheckOrder  ( From, To );
}


        TMarker::TMarker ( const TMarker& op )
{
From        = op.From;
To          = op.To;
Code        = op.Code;
Type        = op.Type;

StringCopy ( Name, op.Name );
}


TMarker&    TMarker::operator= ( const TMarker& op2 )
{
From        = op2.From;
To          = op2.To;
Code        = op2.Code;
Type        = op2.Type;

StringCopy ( Name, op2.Name );

return  *this;
}


//----------------------------------------------------------------------------
bool     operator==  ( const TMarker& op1, const TMarker& op2 )
{
return  op1.From == op2.From 
     && op1.To   == op2.To
     && op1.Type == op2.Type
     && _stricmp ( op1.Name, op2.Name ) == 0;
}


bool     operator!=  ( const TMarker& op1, const TMarker& op2 )
{
return  ! ( op1 == op2 );
}


bool     operator>  ( const TMarker& op1, const TMarker& op2 )
{
return  op1.From >  op2.From
     || op1.From == op2.From  && op1.To >  op2.To
     || op1.From == op2.From  && op1.To == op2.To && _stricmp ( op1.Name, op2.Name ) > 0;
}


bool     operator<  ( const TMarker& op1, const TMarker& op2 )
{
return  op1.From <  op2.From
     || op1.From == op2.From  && op1.To <  op2.To
     || op1.From == op2.From  && op1.To == op2.To && _stricmp ( op1.Name, op2.Name ) < 0;
}


//----------------------------------------------------------------------------
double  TMarker::Overlap   (   const TMarker& marker,   RelativeIntervalOverlapEnum     how )   const
{
return  RelativeIntervalOverlap (   From,           To,
                                    marker.From,    marker.To,
                                    how 
                                );
}


//----------------------------------------------------------------------------
void    TMarker::Show ( const char* title )    const
{
char                buff [ 256 ];
char                buff2[  32 ];


StringCopy   ( buff, "Name" Tab Tab,    Name );

StringAppend ( buff, NewLine );
StringAppend ( buff, "From" Tab Tab,    IntegerToString  ( buff2, From      ) );

StringAppend ( buff, NewLine );
StringAppend ( buff, "To" Tab Tab,      IntegerToString  ( buff2, To        ) );

StringAppend ( buff, NewLine );
StringAppend ( buff, "Duration" Tab Tab,IntegerToString  ( buff2, Length () ) );

StringAppend ( buff, NewLine );
//StringAppend ( buff, "Type" Tab Tab,    IntegerToString  ( buff2, Type      ) );
StringAppend ( buff, "Type" Tab Tab,    IsFlag ( Type, MarkerTypeTrigger  ) ? MarkerNameTrigger
                                      : IsFlag ( Type, MarkerTypeMarker   ) ? MarkerNameMarker
                                      : IsFlag ( Type, MarkerTypeEvent    ) ? MarkerNameEvent
                                      : IsFlag ( Type, MarkerTypeTemp     ) ? MarkerNameTemporary
                                      :                                      "Unknown",
                                        IsFlag ( Type, MarkerTypeToRemove ) ? MarkerNameToRemove : "" );


ShowMessage ( buff, StringIsEmpty ( title ) ? "Marker" : title );
}


//----------------------------------------------------------------------------
                                        // All functions to output markers to (text) file
inline  ostream&    operator<<  (   ostream&    os,     const TMarker&      marker     )
{
os  << StreamFormatFixed;
os  << StreamFormatInt32 << setw ( 11 ) << marker.From << Tab;
os  << StreamFormatInt32 << setw ( 11 ) << marker.To   << Tab;
os  << StreamFormatText  << DoubleQuote << marker.Name << DoubleQuote;
os  << fastendl;

//os.flush (); DBGV2 ( marker.From, marker.To, marker.Name );

return os;
}


inline  ostream&    operator<<  (   ostream&    os,     const TMarker*      marker     )
{
os  << StreamFormatFixed;
os  << StreamFormatInt32 << setw ( 11 ) << marker->From << Tab;
os  << StreamFormatInt32 << setw ( 11 ) << marker->To   << Tab;
os  << StreamFormatText  << DoubleQuote << marker->Name << DoubleQuote;
os  << fastendl;

//os.flush (); DBGV2 ( marker->From, marker->To, marker->Name );

return os;
}


void    WriteMarkerHeader   (   ofstream&   os  )
{
os  << StreamFormatText << TLTXT_MAGICNUMBER2 << fastendl;
os  << StreamFormatFixed;
}


void    WriteMarker (   ofstream&   os,
                        long        From,   long        To,     const char*     Name
                    )
{
os  << StreamFormatFixed;
os  << StreamFormatInt32 << setw ( 11 ) << From << Tab;
os  << StreamFormatInt32 << setw ( 11 ) << To   << Tab;
os  << StreamFormatText  << DoubleQuote << Name << DoubleQuote;
os  << fastendl;

//os.flush (); DBGV2 ( From, To, Name );
}


void    WriteMarker (   ofstream&   os,
                        long        Pos,    const char*     Name
                    )
{
os  << StreamFormatFixed;
os  << StreamFormatInt32 << setw ( 11 ) << Pos  << Tab;
os  << StreamFormatInt32 << setw ( 11 ) << Pos  << Tab;
os  << StreamFormatText  << DoubleQuote << Name << DoubleQuote;
os  << fastendl;

//os.flush (); DBGV ( Pos, Name );
}


//----------------------------------------------------------------------------

TGLColoring     TMarker::TriggerColors      = TGLColoring ( 0, Discrete, 0, MarkersMaxColors - 1,
                                                            TGLColor<GLfloat> ( 0.25, 0.25, 0.25, 0.10 ),
                                                            TGLColor<GLfloat> ( 0.50, 0.50, 0.50, 0.10 ) );


void    TMarker::ColorGLize ()     const
{
int                 hn              = 0;

                                        // hash name
for ( uchar *toc = (uchar *) Name; *toc; toc++ )
    hn     += *toc;

hn     %= MarkersMaxColors;

                                        // send to OpenGL
TriggerColors.GLize ( hn );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Markers are written on disk on every addition or removal,
// to avoid loss during a crash, and to avoid asking the user when
// closing the main document.
// Routines therefore assume this fact, and have to specify
// if they want or not to effectively write on disk.

        TMarkers::TMarkers ()
{
TracksDoc           = 0;
MarkersFileName.Reset ();
MarkersDirty        = false;
}


        TMarkers::TMarkers ( const MarkersList& markerslist )
{
                                        // ignore MarkersDirty
SetMarkers  ( markerslist );

TracksDoc           = 0;
MarkersFileName.Reset ();
MarkersDirty        = false;
}


        TMarkers::TMarkers ( const TTracksDoc* tracksdoc ) :
        TracksDoc ( tracksdoc )
{
MarkersFileName.Reset ();
MarkersDirty        = false;
}


        TMarkers::TMarkers ( const char* file )
{
TracksDoc           = 0;
MarkersFileName.Reset ();
MarkersDirty        = false;
                                        // !forward any change from reading file!
StringCopy  ( MarkersFileName, file );
SetMarkers  ( MarkersFileName );
}


        TMarkers::~TMarkers ()
{
ResetMarkers ();
}


void    TMarkers::ResetMarkers ()
{
if ( ! IsEmpty () )
    MarkersDirty    = true;

                                        // delete content & structure
Markers.Reset ( true );
}


//----------------------------------------------------------------------------
        TMarkers::TMarkers ( const TMarkers& op )
{
                                        // ignore MarkersDirty
SetMarkers ( op.Markers );

TracksDoc           = op.TracksDoc;
MarkersFileName     = op.MarkersFileName;
MarkersDirty        = op.MarkersDirty;
}


TMarkers&   TMarkers::operator= ( const TMarkers& op2 )
{
if ( &op2 == this )
    return  *this;

                                        // ignore MarkersDirty
SetMarkers ( op2.Markers );

TracksDoc           = op2.TracksDoc;
MarkersFileName     = op2.MarkersFileName;
MarkersDirty        = op2.MarkersDirty;


return  *this;
}


//----------------------------------------------------------------------------
void    TMarkers::CommitMarkers ( bool force )
{
if ( IsInMemory () ) {
    MarkersDirty    = false;
    return;
    }

if ( ! ( MarkersDirty || force ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           file;

                                        // default name, taken at last time in case of "Save As"
if ( MarkersFileName.IsEmpty () ) {
                                        // check for multi-session -> multi mrk files
    StringCopy      ( file, TracksDoc->GetDocPath () );

    if ( TracksDoc->GetCurrentSession () >= 1 )
        StringAppend( file, ".", IntegerToString ( TracksDoc->GetCurrentSession () ) );

    AddExtension    ( file, FILEEXT_MRK );
    }
else
    StringCopy ( file, MarkersFileName );


if ( MarkersDirty && ! force )
    if ( ! GetAnswerFromUser ( "Markers have been modified, do you want to save them to file now?", ToFileName ( file ) ) )
        return;


file.CheckExtendedPath ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // special case: if there are no more markers
                                        // erase any existing mrk file as disk clean up
if ( GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {

    DeleteFileExtended ( file );
    }
else {                                  // write to file
    ofstream                os ( file );


    if ( ! os.good () ) {
        MarkersDirty    = false;        // we will lose the markers
        ShowMessage ( "Marker file can not be written to disk!" NewLine "Check if disk is full or if file is write-protected...", file, ShowMessageWarning );
        return;
        }

    WriteMarkerHeader ( os );

    for ( int i = 0; i < (int) Markers; i++ ) {

        if ( ! IsFlag ( Markers[ i ]->Type, MarkerTypeUserCoded ) )
            continue;

        os  << Markers[ i ];
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MarkersDirty    = false;
}


//----------------------------------------------------------------------------
void    TMarkers::WriteFile ( const char* file, MarkerType type, long mintf, long maxtf )   const
{
if ( StringIsEmpty ( file ) )
    return;


if ( GetNumMarkers ( type ) == 0 ) {
                                        // delete file if no markers of the given type
    DeleteFileExtended ( file );

    return;
    }

                                        // requiring both limits for the moment
bool                testtimelimits  = mintf >= 0 && maxtf >= 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write to file
ofstream            os ( (char*) TFileName ( file, TFilenameExtendedPath ) );

if ( os.fail () ) {
    ShowMessage ( "Marker file can not be written to disk!" NewLine "Check if disk is full or if file is write-protected...", file, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

WriteMarkerHeader ( os );


for ( int i = 0; i < (int) Markers; i++ ) {

    if ( ! IsFlag ( Markers[ i ]->Type, type ) )
        continue;


    if ( testtimelimits ) {
                                        // in case markers are sorted, we can get out here
        if ( Markers[ i ]->From > maxtf )
            break;

                                        // optionally keeping only those markers completely within time interval
        if ( ! IsInsideLimits ( Markers[ i ]->From, Markers[ i ]->To, mintf, maxtf ) )
            continue;
        }


    os  << Markers[ i ];
    } // for markers


os.close ();
}


//----------------------------------------------------------------------------
void    TMarkers::Show ( const char* title )    const
{
if ( IsEmpty () ) {
    ShowMessage ( "- Empty -", StringIsEmpty ( title ) ? "Triggers" : title );
    return;
    }

for ( int i = 0; i < (int) Markers; i++ )

    Markers[ i ]->Show ( StringIsEmpty ( title ) ? "Triggers" : title );
}


//----------------------------------------------------------------------------
bool    TMarkers::InitMarkers ( const char* file )
{
                                        // clear everything
ResetMarkers ();

                                        // read optional native markers first
ReadNativeMarkers ();

                                        // then additional mrk file
if ( StringIsEmpty ( file ) ) {

    TFileName           mrkfile;

    StringCopy ( mrkfile, TracksDoc->GetDocPath () );
                                        // check for multi-session -> multiple mrk files
    if ( TracksDoc->GetCurrentSession () >= 1 )
        StringAppend ( mrkfile, ".", IntegerToString ( TracksDoc->GetCurrentSession (), 0 ) );

    AddExtension    ( mrkfile, FILEEXT_MRK );
                                        // !forward any change from reading file!
    AppendMarkers   ( mrkfile );
    }
else {
    StringCopy      ( MarkersFileName, file );
                                        // !forward any change from reading file!
    AppendMarkers   ( MarkersFileName );
    }

                                        // check & sort the whole list
SortAndCleanMarkers ();

                                        // !forward any change from reading file!
//MarkersDirty    = false;


return  true;
}


//----------------------------------------------------------------------------
int     TMarkers::GetNumMarkers ( MarkerType type )    const
{
KeepFlags   ( type, AllMarkerTypes );

                                        // fastest case
if ( type == AllMarkerTypes )
    return  GetNumMarkers ();

                                        // longer, we have to count for specific markers
int                 n               = 0;

for ( int i = 0; i < (int) Markers; i++ )

    if ( IsFlag ( Markers[ i ]->Type, type ) )

        n++;


return  n;
}


void    TMarkers::GetMarkerNames ( TStrings&    markernames, MarkerType type, int maxnames )
{
markernames.Reset ();

int                 numtags         = GetNumMarkers ( type );
                                        // we can save some string search time if there are no markers of requested type
if ( numtags == 0 )
    return;


                                        // scan all known markers
for ( int i = 0; i < (int) Markers; i++ ) {

    if ( ! IsFlag ( Markers[ i ]->Type, type ) )
        continue;

                                        // ?skip dumb names - this seems quite dangerous, like, missing a lot of markers?
//  if ( StringStartsWith ( Markers[ i ]->Name, MarkerNameEpoch ) )
//      continue;


    markernames.AddNoDuplicates ( Markers[ i ]->Name );

                                        // insert up to the requested # of names
    if ( (int) markernames >= maxnames )
        break;
    }


markernames.Sort ();
}


bool    TMarkers::TimeRangeToIndexes ( MarkerType type, long timemin, long timemax, int &indexmin, int &indexmax )
{
indexmin = indexmax = -1;

if ( IsEmpty () )
    return  false;


                                        // find first trigger to consider
for ( int i = 0; i < (int) Markers; i++ )

    if ( IsFlag ( Markers[ i ]->Type, type ) )

        if ( ( Markers[ i ]->From >= timemin && Markers[ i ]->From <= timemax )     // trigger starts within range, don't care where it ends
          || ( Markers[ i ]->From <= timemax && Markers[ i ]->To   >= timemin ) ) { // trigger ends past beginning of range, but starts before the end of range
        indexmin = i;
        break;
        }
                                        // no low bound found, not useful to explore any further...
if ( indexmin == -1 )
    return false;

                                        // set indexmax at least as
indexmax = indexmin;
                                        // scan for upper bound
for ( int i = 0; i < (int) Markers; i++ )

    if ( IsFlag ( Markers[ i ]->Type, type ) )

        if ( Markers[ i ]->From <= timemax )
            indexmax = i;
        else
            break;                      // don't scan any further, starting position is beyond the end


return  true; // indexmin >= 0 && indexmax >= 0;
}


//----------------------------------------------------------------------------
                                        // skips same position markers
bool    TMarkers::GetNextMarker ( TMarker& marker, bool forward, MarkerType type )     const
{
if ( IsEmpty () )
    return false;


if ( forward ) {

    for ( int i = 0; i < (int) Markers; i++ )

        if ( IsFlag ( Markers[ i ]->Type, type ) )

            if ( Markers[ i ]->From >  marker.To
              || Markers[ i ]->From >= marker.From && Markers[ i ]->From <= marker.To && Markers[ i ]->To > marker.To ) {

                marker     = *Markers[ i ];
                return  true;
                }
    }
else { // backward

    for ( int i = (int) Markers - 1; i >= 0; i-- )

        if ( IsFlag ( Markers[ i ]->Type, type ) )

            if ( Markers[ i ]->To <  marker.From
              || Markers[ i ]->To >= marker.From && Markers[ i ]->To <= marker.To && Markers[ i ]->From < marker.From ) {

                marker     = *Markers[ i ];
                return  true;
                }
    }

return  false;
}


bool    TMarkers::GetNextMarker ( const TTFCursor* tfc, TMarker& marker, bool forward, MarkerType type )     const
{
if ( IsEmpty () )
    return false;


marker.Set ( tfc->GetPosMin (), tfc->GetPosMax () );


if ( forward ) {

    for ( int i = 0; i < (int) Markers; i++ )

        if ( IsFlag ( Markers[ i ]->Type, type ) )

            if ( Markers[ i ]->From >  marker.To
              || Markers[ i ]->From >= marker.From && Markers[ i ]->From <= marker.To && Markers[ i ]->To > marker.From
                 && ( Markers[ i ]->To   > marker.To   || ( tfc->IsExtending () && tfc->GetFixedPos () > tfc->GetExtendingPos () ) ) ) {

                marker     = *Markers[ i ];
                return  true;
                }
    }
else {

    for ( int i = (int) Markers - 1; i >= 0; i-- )

        if ( IsFlag ( Markers[ i ]->Type, type ) )

            if ( Markers[ i ]->To < marker. From
              || Markers[ i ]->To >= marker.From && Markers[ i ]->To <= marker.To && Markers[ i ]->From < marker.To
                 && ( Markers[ i ]->From < marker.From || ( tfc->IsExtending () && tfc->GetFixedPos () < tfc->GetExtendingPos () ) ) ) {

                marker     = *Markers[ i ];
                return  true;
                }
    }

return  false;
}


//----------------------------------------------------------------------------
                                        // Try to avoid calling this method, which can be quite slow for a big amount of markers
bool    TMarkers::HasMarker ( const TMarker& marker )
{
for ( int i = 0; i < (int) Markers; i++ )

    if ( *Markers[ i ] == marker )      // tests for content, NOT for pointers!

        return  true;

return  false;
}


//----------------------------------------------------------------------------
                                        // Force sort the content - Output will therefore also be sorted
void    TMarkers::SortAndCleanMarkers ()
{
                                        // save time, also avoiding changing the MarkersDirty flag uselessly
if ( GetNumMarkers () < 2 )
    return;

                                        // does not update MarkersDirty
SortMarkers ();

                                        // When playing directly within the list, DO NOT USE []
TListIterator<TMarker>  iterator;
bool                    listdirty       = false;

                                        // testing if NEXT marker exists
for ( iterator.SetForward ( Markers ); iterator.Current ()->Next != 0; ) {

    if ( *iterator.Current ()->To == *iterator.Current ()->Next->To ) {

        Markers.Remove ( iterator.Current ()->Next->To );
                                        // change occured
        listdirty   = true;
        }
    else 
        iterator.Next ();
    }


if ( listdirty ) {
                                        // rebuild indexes
    Markers.UpdateIndexes ();
                                        // set only if marker list was actually changed
    MarkersDirty    = true;
    }
}


//----------------------------------------------------------------------------
const TMarker*  TMarkers::GetMarker ( const char* markername )  const
{
for ( int i = 0; i < (int) Markers; i++ )

    if ( StringIs ( Markers[ i ]->Name, markername ) )

        return  Markers[ i ];

return  0;
}


//----------------------------------------------------------------------------
void    TMarkers::SetMarkers ( const TMarkers& markers )
{
ResetMarkers    ();

AppendMarkers   ( markers );

SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::SetMarkers ( const MarkersList& markerslist )
{
ResetMarkers    ();

AppendMarkers   ( markerslist );

SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // !returned MarkersDirty reflects any change from reading the file only!
void    TMarkers::SetMarkers ( const char* file )
{
ResetMarkers    ();
                                        // !forward any change from reading file!
AppendMarkers   ( file );

SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::AppendMarker ( const TMarker& marker )
{
if ( TracksDoc && marker.IsNotOverlappingInterval ( (long) 0, TracksDoc->GetNumTimeFrames () - 1 ) )
    return;

                                        // clone marker + clip its length
TMarker*            markerc         = new TMarker ( marker );

if ( TracksDoc )
    Clipped ( markerc->From, markerc->To, (long) 0, TracksDoc->GetNumTimeFrames () - 1 );

                                        // just put it at the end
Markers.Append ( markerc );

                                        // changing state only for markers, the one that can be aved to file by the user
if ( IsFlag ( marker.Type, MarkerTypeUserCoded ) )
    MarkersDirty    = true;
}


//----------------------------------------------------------------------------
void    TMarkers::AppendMarkers ( const TMarkers& markers )
{
for ( int i = 0; i < (int) markers; i++ )

    AppendMarker ( *markers[ i ] );
}


//----------------------------------------------------------------------------
void    TMarkers::AppendMarkers ( const MarkersList& markerslist )
{
for ( int i = 0; i < (int) markerslist; i++ )

    AppendMarker ( *markerslist[ i ] );
}


//----------------------------------------------------------------------------
                                        // !returned MarkersDirty reflects any change from reading the file only!
void    TMarkers::AppendMarkers ( const char* file )
{
TMarkers            markers;
                                        // !forward any change from reading file!
markers.ReadFile ( file );

                                        // save current dirty state
bool                oldmarkersdirty = MarkersDirty;

for ( int i = 0; i < (int) markers; i++ )

    AppendMarker ( *markers[ i ] );

                                        // !forward any change from reading file!
MarkersDirty    = oldmarkersdirty || markers.MarkersDirty;
}


//----------------------------------------------------------------------------
int     TMarkers::InsertMarker ( const TMarker& marker, int indexfrom )
{
if ( TracksDoc && marker.IsNotOverlappingInterval ( (long) 0, TracksDoc->GetNumTimeFrames () - 1 ) )
    return  0;


if ( HasMarker ( marker ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 i;

                                        // we have an optional index from where to start searching
for ( i = indexfrom; i < (int) Markers; i++ )
                                        // stop at first element in the list that is beyond marker
    if ( *Markers[ i ] > marker )

        break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // make a new marker, and clip its length
TMarker*            markerc            = new TMarker ( marker );

if ( TracksDoc )  Clipped ( markerc->From, markerc->To, (long) 0, TracksDoc->GetNumTimeFrames () - 1 );


if ( i == (int) Markers )   Markers.Append ( markerc );               // nothing past marker?
else                        Markers.Insert ( markerc, Markers[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsFlag ( marker.Type, MarkerTypeUserCoded ) )
    MarkersDirty    = true;

                                        // return index of inserted element
return  i;
}


//----------------------------------------------------------------------------
                                        // Read from file, then sort
void    TMarkers::ReadFile ( const char* file )
{
                                        // clear everything
ResetMarkers ();
                                        // ResetMarkers sets the Dirty flag, so reset it
MarkersDirty    = false;

if ( ! CanOpenFile ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ifstream            is ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
int                 magic;
TMarker             marker;
char                buff[ KiloByte ];


is.read ( (char *) &magic, sizeof ( magic ) );

                                        // doesn't look like a proper Cartool marker file!
if ( ! IsMagicNumber ( magic, TLBIN_MAGICNUMBER2 ) ) {

//  ShowMessage ( "Can not recognize this marker file (unknown magic number)!", "Open file", ShowMessageWarning );
    return;
    }
                                        // avoid reading & writing at the same time
                                        // in case we read the associated mrk file
if      ( IsMagicNumber ( magic, TLBIN_MAGICNUMBER2 ) ) {
                                        // reopen as text file
    is.close ();
    is.open  ( file );
                                        // skip header
    is.getline ( buff, KiloByte );


    while ( ! is.eof () ) {
                                        // get next line
        is.getline ( buff, KiloByte );

                                        // be nice when encountering an empty line
        if ( StringIsEmpty ( buff ) )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract TF limits
        sscanf ( buff, "%ld %ld", &marker.From, &marker.To );

        CheckOrder ( marker.From, marker.To );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract name - goes through a lot of hoops to try to recover non-conforming files
                                        // get everything past the 2 TFs
        SkipFirstWords ( buff, buff, 2 );

        StringCleanup ( buff );
                                        // correct syntax is between 2 quotes: "ABC"
                                        // otherwise take the string as is, but it will skip spaces at each sides
//      if ( *buff == '\"' && *LastChar ( buff, 1 ) == '\"' )
            ReplaceChars ( buff, DoubleQuote, "" );

        StringCopy ( marker.Name, StringIsEmpty ( buff ) ? MarkerNameDefault : buff, MarkerNameMaxLength - 1 );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // code and type
        marker.Code    = (MarkerCode) StringToInteger ( marker.Name );

        marker.Type    = MarkerTypeMarker;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // just append
        AppendMarker ( marker );
        }

    } // TLBIN_MAGICNUMBER2

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // AppendMarker sets the Dirty flag, so reset it
MarkersDirty    = false;

                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // Can read non-sorted file correctly
void    TMarkers::InsertMarkers ( const char* file )
{
TMarkers            markers;
                                        // list from file is sorted here
markers.ReadFile ( file );


int                 indexfrom       = 0;

for ( int i = 0; i < (int) markers; i++ )
                                        // insert by sorting, and don't write to file now!
                                        // provide / update last insertion index so we can insert faster an already sorted list
    indexfrom   = InsertMarker ( *markers[ i ], indexfrom );

                                        // check & sort the whole list
//SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::InsertMarkers ( const TMarkers& markers, const char *filteredwith )
{
TSplitStrings       filteredwithsplit ( filteredwith, UniqueStrings );

                                        // make sure the list is sorted, insertion will be faster
TMarkers            markerssorted ( markers );


int                 indexfrom       = 0;

for ( int i = 0; i < (int) markerssorted; i++ )

    if ( filteredwithsplit.Contains ( markerssorted[ i ]->Name ) )
                                        // insert by sorting, and don't write to file now!
                                        // provide / update last insertion index so we can insert faster an already sorted list
        indexfrom   = InsertMarker ( *markerssorted[ i ], indexfrom );

                                        // check & sort the whole list
//SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // sometime we have a TList object insted of a TMarkers
                                        // no sort here
void    TMarkers::InsertMarkers ( const MarkersList& markerslist )
{
for ( int i = 0; i < (int) markerslist; i++ )

    InsertMarker ( *markerslist[ i ] );

                                        // check & sort the whole list
//SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::SortMarkers ()
{
_Sort ( 0, GetNumMarkers () - 1 );

Markers.UpdateIndexes ( true );

//MarkersDirty    = true;               // content hasn't really changed, has it?
}

                                        // Works directly with atoms from the list
void    TMarkers::_Sort ( int l, int r )
{
if ( r <= l )   return;


int                 i               = l;
int                 j               = r;
const TMarker*      v               = Markers[ ( l + r ) / 2 ];


do {
    while ( *Markers[ i ] < *v            )     i++;
    while ( *v            < *Markers[ j ] )     j--;

    if ( i <= j )
        Permutate ( Markers.GetAtom ( i++ )->To, Markers.GetAtom ( j-- )->To ); // atoms actually remain in place, we just permutate the To pointers

    } while ( i <= j );


if ( l < j )    _Sort ( l, j );
if ( i < r )    _Sort ( i, r );
}


//----------------------------------------------------------------------------
                                        // Epochs should be at least intersecting the given time range, if totally out they are not taken
                                        // Epochs will be clipped to the limits
void    TMarkers::EpochsToMarkers   (   EpochsType          epochs,
                                        const TStrings*     epochfrom,      const TStrings*     epochto,    
                                        long                mintf,          long           maxtf,
                                        long                periodiclength
                                    )
{
ResetMarkers ();

                                        // in case some duration is null
if ( maxtf < mintf || maxtf < 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check parameters
                                        // asking for list of epochs, but none provided -> downgrade
if ( epochs == EpochsFromList
     && ! ( epochfrom && epochfrom->IsNotEmpty ()
         && epochto   && epochto  ->IsNotEmpty ()
         && epochfrom->NumStrings () == epochto->NumStrings () ) )

    epochs  = EpochWholeTime;
//  return;

                                        // periodic but bad length parameter -> downgrade
if ( epochs == EpochsPeriodic
  && periodiclength <= 0      )

    epochs  = EpochWholeTime;
//  return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init a default marker
TMarker             marker ( 0, 0, 0, MarkerNameEpoch, MarkerTypeTemp );

                                        // still a valid value
if      ( epochs == EpochNone )

    return;


else if ( epochs == EpochWholeTime ) {
                                        // this is usually the whole time range
    marker.From        = mintf;
    marker.To          = maxtf;

    AppendMarker ( marker );            // insert at the end
    } // EpochWholeTime


else if ( epochs == EpochsPeriodic ) {

    GeneratePeriodicEpochs  (   mintf,  maxtf, 
                                periodiclength,
                                true,   false,
                                /*MarkerNameEpoch*/ MarkerNameAutoEpochs 
                            );
    }


else if ( epochs == EpochsFromList ) {

                                        // loop backward, as first values entered by the user have been pushed down
    for ( int epoch = epochfrom->NumStrings () - 1; epoch >= 0; epoch-- ) {

        long            fromtf  = StringToInteger ( (*epochfrom)[ epoch ] );
        long            totf    = StringToInteger ( (*epochto  )[ epoch ] );

                                        // no intersection?
        if ( IsNotOverlappingInterval ( fromtf, totf, mintf, maxtf ) )
            continue;

                                        // clip intersecting epoch
        Clipped ( fromtf, totf, mintf, maxtf );


        marker.From        = fromtf;
        marker.To          = totf;


        AppendMarker ( marker );        // insert at the end
        } // for epoch

    } // EpochsFromList

                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::TrackToMarkers (  const TArray1<float>&   track, 
                                    const char*             markerprefix,
                                    bool                    postfixvalue
                                 )
{
if ( track.IsNotAllocated () )
    return;


//ResetMarkers ();                      // allow to append


char                markertext  [ 256 ];
char                buff        [ 256 ];

StringCopy ( markertext, StringIsEmpty ( markerprefix ) ? "MarkerFromTrack" : markerprefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                numtf           = track.GetDim ();
TMarker             marker;

marker.Code     = MarkerDefaultCode;
marker.Type     = MarkerTypeMarker;


for ( long tfi1 = 0; tfi1 < numtf; tfi1++ ) {
                                        // found onset?
    if ( ! ( track ( tfi1 ) != 0 && ( tfi1 == 0 || track ( tfi1 - 1 ) == 0 ) ) )

        continue;

                                        // search for offset
    for ( long tfi2 = tfi1; tfi2 < numtf; tfi2++ ) {
                                        // found offset?
        if ( track ( tfi2 ) != 0 && ( tfi2 == numtf - 1 || track ( tfi2 + 1 ) == 0 ) ) {

            marker.From     = tfi1;
            marker.To       = tfi2;

            StringCopy  ( marker.Name, markertext );

            if ( postfixvalue )
                StringAppend ( marker.Name, ":", FloatToString ( buff, track ( ( tfi1 + tfi2 ) / 2 ), 3 ) );    // !should scan for max value instead!

            AppendMarker ( marker );

                                        // jump past current end
            tfi1    = tfi2;

            break;
            }
        } // for tfi2
    } // for tfi1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // Generate successive epoch markers
void    TMarkers::GeneratePeriodicEpochs    (   long            mintf,              long            maxtf,  
                                                long            steptf,
                                                bool            firstincomplete,    bool            lastincomplete,
                                                const char*     name
                                            )
{
ResetMarkers ();


TMarker             marker;

marker.Code        = 0;
marker.Type        = MarkerTypeTemp;
StringCopy ( marker.Name, name, MarkerNameMaxLength - 1 );

                                        // add a default epoch? difficult if parameters are wrong...
if ( ! ( mintf <= maxtf && steptf > 0 ) ) {

//    marker.From        = mintf;
//    marker.To          = maxtf;
//
//    AppendMarker ( marker );          // insert at the end

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( long epochmin = mintf, epochmax = epochmin + steptf - 1; epochmin <= maxtf; epochmin += steptf, epochmax += steptf ) {

                                        // do we face an incomplete epoch?
    if ( epochmax > maxtf ) {
                                        
        if ( epochmin == mintf && ! firstincomplete     // is first epoch?
          || epochmin != mintf && ! lastincomplete  )   // is last  epoch?
                                        // if first epoch is already incomplete, there will be of course no others past this one...
            break;
        }

                                        // clip intersecting epoch
    marker.From        = Clip ( epochmin, mintf, maxtf );
    marker.To          = Clip ( epochmax, mintf, maxtf );


    AppendMarker ( marker );            // insert at the end
    } // for epoch


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::MaxTrackToMarkers (   TTracksDoc*     EEGDoc, 
                                        long            mintf,      long            maxtf,      int         tracki, 
                                        const char*     name 
                                    )
{
ResetMarkers ();


if ( EEGDoc == 0 /*|| *EEGDoc == 0*/
  || ! IsInsideLimits ( tracki, 0, EEGDoc->GetTotalElectrodes () - 1 ) )

    return;


long                NumTimeFrames   = EEGDoc->GetNumTimeFrames ();
                                        // check time range
Clipped ( mintf, maxtf, (long) 0, NumTimeFrames - 1 );

long                NumTime         = maxtf - mintf + 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // load & scan everything at once
TTracks<float>      EegBuff ( EEGDoc->GetTotalElectrodes (), NumTime );

                                        // get data - doing pseudos only if needed
EEGDoc->GetTracks   (   mintf,      maxtf, 
                        EegBuff,    0, 
                        AtomTypeUseCurrent, 
                        tracki >= EEGDoc->GetNumElectrodes () ? ComputePseudoTracks : NoPseudoTracks, 
                        ReferenceAsInFile 
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TMarker             marker;

marker.Code        = 0;
marker.Type        = MarkerTypeTemp;
StringCopy ( marker.Name, StringIsEmpty ( name ) ? MarkerNameMax : name, MarkerNameMaxLength - 1 );


for ( long tf0 = 1; tf0 < NumTime - 1; tf0++ ) {
                                        // borders are OK
    if ( EegBuff[ tracki ][ tf0 ] > EegBuff[ tracki ][ tf0 - 1 ]
      && EegBuff[ tracki ][ tf0 ] > EegBuff[ tracki ][ tf0 + 1 ] ) {
                                        // convert to original position
        marker.From        =
        marker.To          = mintf + tf0;

        AppendMarker ( marker );        // insert at the end
        }

    } // for tf0

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // Can detect either local max or local min
void    TMarkers::MaxTrackToMarkers (   const TArray1<double>&  track, 
                                        long                    mintf,      long                maxtf,
                                        bool                    peak,
                                        const char*             name 
                                    )
{
ResetMarkers ();


if ( track.IsNotAllocated () )
    return;


long                NumTimeFrames   = track.GetDim1 ();
                                        // check time range
Clipped ( mintf, maxtf, (long) 0, NumTimeFrames - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TMarker             marker;

marker.Code        = 0;
marker.Type        = MarkerTypeTemp;
StringCopy ( marker.Name, StringIsEmpty ( name ) ? MarkerNameMax : name, MarkerNameMaxLength - 1 );

                                        // We can try to scan from the included edges
for ( long tf = AtLeast ( (long) 1, mintf ); tf <= NoMore ( NumTimeFrames - 2, maxtf ); tf++ ) {
                                        // borders are OK
    if (   peak && (    track[ tf ] > track[ tf - 1 ]           // local max
                     && track[ tf ] > track[ tf + 1 ] )
      || ! peak && (    track[ tf ] < track[ tf - 1 ]           // local min
                     && track[ tf ] < track[ tf + 1 ] ) ) {

        marker.From        =
        marker.To          = tf;

        AppendMarker ( marker );        // insert at the end
        }

    } // for tf0

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // Either keep or exclude from the marker list
void    TMarkers::MarkersToTimeChunks   (   const TMarkers& inputmarkers,
                                            const char*     inputmarkernames,   MarkersChunksFlags   flag,   
                                            long            fromtf,             long                totf, 
                                            const char*     newmarkername
                                        )
{
ResetMarkers ();

TFileName           markername;

StringCopy ( markername, StringIsEmpty ( newmarkername ) ? MarkerNameBlock : newmarkername );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//long                lasttimeframes  = EEGDoc->GetNumTimeFrames () - 1;


if ( StringIsEmpty ( inputmarkernames ) ) {

    if ( flag == ExcludingMarkers )       // no excluded zones? return the whole time range
        AppendMarker ( TMarker ( fromtf, totf, /*0, lasttimeframes,*/ 0, markername, MarkerTypeTemp ) );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here markers exist

                                        // We need to build an enumeration of the TFs to be saved,
                                        // as we don't know if the input list has some overlapping markers or not
TArray1<int>        label ( totf + 1 /*lasttimeframes*/ );
TMarkers            markers;

                                        // get & select from the original triggers, by names
markers.InsertMarkers ( inputmarkers, inputmarkernames );

                                        // !Merge overlapping markers!
label     = 0;

for ( int i = 0; i < (int) markers; i++ )
for ( long tf = AtLeast ( (long) 0, markers[ i ]->From ); tf <= NoMore ( totf /*lasttimeframes*/, markers[ i ]->To ); tf++ )
                                        // OR'ed all markers, but keep track of which markers contributed
    label ( tf )  = i + 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convert to markers, by blocks following the original markers - except any overlaps will be solved
long                begintf;
long                endtf;
int                 segi;

                                        // !not necessarily beginning from 0!
begintf     = 0; // fromtf;


                                        // Transform the labeling into discrete states
for ( long tf = 0 /*fromtf*/; tf <= /*lasttimeframes*/ totf; tf++ ) {

    if ( ! ( tf == /*lasttimeframes*/ totf
          || label[ tf + 1 ] != label[ tf ] ) )

         continue;
                                        // here we have a new segment (could even be of UndefinedLabel)

    endtf       = tf;
//  duration    = endtf - begintf + 1;
    segi        = label[ begintf ];

                                        // use current time chunk?
    if ( flag == KeepingMarkers   && segi != 0
      || flag == ExcludingMarkers && segi == 0 )
                                        // we can append at the end
        AppendMarker ( TMarker ( begintf, endtf, 0, markername, MarkerTypeTemp ) );

                                        // next segment will begin after current TF
    begintf     = tf + 1;
    } // for tfi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove markers not completely within requested limits
KeepMarkers   ( fromtf, totf, AllMarkerTypes );

                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
bool    TMarkers::IsOverlapping (   long        mintf,  long        maxtf   )   const
{
if ( IsEmpty () )
    return  false;


for ( int i = 0; i < (int) Markers; i++ )

    if ( IsOverlappingInterval  (   mintf,              maxtf,
                                    Markers[ i ]->From,    Markers[ i ]->To ) )

        return  true;


return  false;
}


//----------------------------------------------------------------------------
void    TMarkers::CreateTimeChunks (    const TTracksDoc*   EEGDoc,
                                        double              blockdurationms,    long            fromtf,     long            totf )
{
ResetMarkers ();


if ( blockdurationms <= 0 )
    return;

                                        // Create blocks from duration given by caller
double              samplingfrequency   = EEGDoc->GetSamplingFrequency ();
long                chunksize           = MillisecondsToTimeFrame ( blockdurationms, samplingfrequency );
TMarker             marker;


for ( long tf = fromtf; tf + chunksize - 1 <= totf; tf += chunksize ) {

    marker.From    = tf;
    marker.To      = tf + chunksize - 1;
    marker.Code    = 0;
    marker.Type    = MarkerTypeTemp;
    StringCopy ( marker.Name, MarkerNameBlock, MarkerNameMaxLength - 1 );

    AppendMarker ( marker );
    }

                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
void    TMarkers::GranularizeMarkers ( int downsampling )
{
                                        // adjust each time chunk to fall on the downsampling "grid"
for ( int i = 0; i < (int) Markers; i++ ) {

    Markers[ i ]->From   = TruncateTo ( Markers[ i ]->From, downsampling );
    Markers[ i ]->To     = TruncateTo ( Markers[ i ]->To  , downsampling );
    }

//Show ( "Markers to grid" );
}


void    TMarkers::DownsampleMarkers ( int downsampling )
{
if ( downsampling <= 1 )
    return;


for ( int i = 0; i < (int) Markers; i++ ) {

    Markers[ i ]->From  /= downsampling;
    Markers[ i ]->To    /= downsampling;
    }

//Show ( "Markers downsampled" );
}


void    TMarkers::UpsampleMarkers ( int upsampling )
{
if ( upsampling <= 1 )
    return;


for ( int i = 0; i < (int) Markers; i++ ) {
                                        // broader formula:
                                        //   - beginning at first upsampled TF
    Markers[ i ]->From  *= upsampling;
                                        //   - ending to last upsampled TF
    Markers[ i ]->To     = ( Markers[ i ]->To + 1 ) * upsampling - 1;
    }

//Show ( "Markers upsampled" );
}

                                        // reslice all markers to sequences of 1 TF markers
                                        // !not tested!
void    TMarkers::ResliceMarkers ()
{
if ( GetLongestMarker () == 1 )
    return;


TMarkers            oldlist ( *this );

ResetMarkers ();


for ( int i = 0; i < (int) oldlist; i++ ) {

    if ( oldlist[ i ]->Length () == 1 ) {

        AppendMarker ( *oldlist[ i ] );

        continue;
        }

                                        // slice this marker
    for ( ULONG j = 0; j < oldlist[ i ]->Length (); j++ )

        AppendMarker ( TMarker ( oldlist[ i ]->From + j, oldlist[ i ]->From + j, oldlist[ i ]->Code, oldlist[ i ]->Name, oldlist[ i ]->Type ) );

    }

                                        // check & sort the whole list
SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
                                        // Concatenate all markers into a linear structure
                                        // !it allows for overlapping or repeating markers - these TFs will be repeated, too!
void    TMarkers::ConcatenateToVector ( TArray1<long>& concattf )    const
{
long                numtf           = GetMarkersTotalLength ();

concattf.Resize ( numtf );

if ( IsEmpty () )
    return;


for ( int i = 0, tf0 = 0; i < (int) Markers; i++ )

    for ( long tf = Markers[ i ]->From; tf <= Markers[ i ]->To; tf++ )

        concattf[ tf0++ ]   = tf;
}

                                        // Projects markers to a linear time line
void    TMarkers::ProjectToVector ( TArray1<bool>& projecttf, long numtf )   const
{
projecttf.Resize ( numtf );

if ( IsEmpty () )
    return;


for ( int i = 0; i < (int) Markers; i++ )

    for ( long tf = Markers[ i ]->From; tf <= Markers[ i ]->To; tf++ )
                                        // poke current TF's
        projecttf[ tf ]   = true;
}


//----------------------------------------------------------------------------
long   TMarkers::GetLongestMarker ( MarkerType type )   const
{
long                maxlength       = 0;


for ( int i = 0; i < (int) Markers; i++ )
    if ( IsFlag ( Markers[ i ]->Type, type ) )
        Maxed ( maxlength, Markers[ i ]->Length () );


return  maxlength;
}

                                        // !doesn't check for overlaps between markers!
long   TMarkers::GetMarkersTotalLength ( MarkerType type )   const
{
long                totallength     = 0;


for ( int i = 0; i < (int) Markers; i++ )
    if ( IsFlag ( Markers[ i ]->Type, type ) )
        totallength    += Markers[ i ]->Length ();


return  totallength;
}


long   TMarkers::GetMinPosition ( MarkerType type )   const
{
long                minpos          = Highest ( minpos );


for ( int i = 0; i < (int) Markers; i++ )
    if ( IsFlag ( Markers[ i ]->Type, type ) )
        Mined ( minpos, Markers[ i ]->From );


return  minpos;
}


long   TMarkers::GetMaxPosition ( MarkerType type )   const
{
long                maxpos          = Lowest ( maxpos );


for ( int i = 0; i < (int) Markers; i++ )
    if ( IsFlag ( Markers[ i ]->Type, type ) )
        Maxed ( maxpos, Markers[ i ]->To );


return  maxpos;
}


//----------------------------------------------------------------------------
                                        // compact joining markers, either to=from, to=from-1, from=to, from=to+1
                                        // does not test for Type, Name; does not concatenate Names either
bool    TMarkers::CompactConsecutiveMarkers ( bool identicalnames, long mergedmaxlength )
{
if ( IsEmpty () )
    return  false;


MarkersList             remaining ( Markers ); // do a local copy of pointers
TMarker*                toaggregate;        // current marker we try to aggregate
TMarker*                todelete;
bool                    mergedsome;
bool                    namesok;
bool                    markersdirty        = false;


                                        // clear current pointer list
Markers.Reset ( false );


for ( ; (bool) remaining; ) {
                                        // pick next marker, try to upgrade it
                                        // use the pointer, we will not destroy the actual object, just updating it
    toaggregate     =  remaining.GetFirst ();
    remaining.Remove ( remaining.GetFirst () );

//    toaggregate->Show ( "toaggregate - before" );

                                        // try to merge one of the remaining marker
    do {
        mergedsome      = false;

                                        // loop into remaining markers for a match to the current aggregate
        for ( int i = 0; i < (int) remaining; i++ ) {

            namesok     = identicalnames && StringIs ( toaggregate->Name, remaining[ i ]->Name ) || ! identicalnames;

                                        // merge on the right?
            if      ( ( toaggregate->To == remaining[ i ]->From || toaggregate->To == remaining[ i ]->From - 1 )
                   && namesok ) {

//                remaining[ i ]->Show ( "to merge" );

                toaggregate->To     = remaining[ i ]->To;

                todelete            = remaining[ i ];
                remaining.Remove ( remaining[ i ] );    // remove pointer from list
                delete ( todelete );             // we still have the pointer copy to object, so delete it

                mergedsome          = true;
                break;
                }
                                        // or merge on the left?
            else if ( ( toaggregate->From == remaining[ i ]->To || toaggregate->From == remaining[ i ]->To + 1 )
                   && namesok ) {
//                remaining[ i ]->Show ( "to merge" );

                toaggregate->From   = remaining[ i ]->From;

                todelete            = remaining[ i ];
                remaining.Remove ( remaining[ i ] );    // remove pointer from list
                delete ( todelete );             // we still have the pointer copy to object, so delete it

                mergedsome          = true;
                break;
                }

            } // foreach remaining


        markersdirty  |= mergedsome;
        MarkersDirty  |= mergedsome;

                             // length test will not work correctly, if markers are not sorted
        } while ( mergedsome && toaggregate->Length () <= mergedmaxlength );


//    toaggregate->Show ( "toaggregate - after" );

                                        // insertion assumes marker list was sorted upon call time
    Markers.Append ( toaggregate );
    }

                                        // tell if list was touched
return  markersdirty;
}


//----------------------------------------------------------------------------
                                        // Will remove markers within given boundary
void    TMarkers::RemoveMarkers ( long from, long to, MarkerType type )
{
KeepFlags   ( type, AllMarkerTypes );

if ( ! type )
    return;


MarkersList             markersok;

                                            // copy markers OK to a temp list
for ( int i = 0; i < (int) Markers; i++ ) {

    if (   IsFlag ( Markers[ i ]->Type, type )
        && IsInsideLimits ( Markers[ i ]->From, Markers[ i ]->To, from, to ) )

        delete ( Markers[ i ] );            // delete object, but not pointer (yet)
    else
        markersok.Append ( Markers[ i ] );  // save pointer
    }


if ( (int) markersok == (int) Markers )     // nothing to remove
    return;


if ( markersok.IsEmpty () )                 // all to be erased
    Markers.Reset ( false );                // objects are already deleted, only clear-up the pointers now

else {
    Markers.Reset ( false );

    for ( int i = 0; i < (int) markersok; i++ )
        Markers.Append ( markersok[ i ] );  // just copy the pointer
    }


MarkersDirty    = true;
}


//----------------------------------------------------------------------------
void    TMarkers::RemoveMarkers ( MarkerType type )
{
KeepFlags   ( type, AllMarkerTypes );

if ( ! type )
    return;


if ( type == AllMarkerTypes )
    ResetMarkers ();

else
    RemoveMarkers ( Lowest<long> (), Highest<long> (), type );
}


void    TMarkers::RemoveMarkers ( const char* greppedwith, MarkerType type )
{
KeepFlags   ( type, AllMarkerTypes );

if ( ! type || StringIsEmpty ( greppedwith ) )
    return;


MarkersList             markersok;
TStringGrep             greptags ( greppedwith, GrepOptionDefaultMarkers );

                                            // copy markers OK to a temp list
for ( int i = 0; i < (int) Markers; i++ ) {

    if ( IsFlag ( Markers[ i ]->Type, type )
//    && StringContains ( Markers[ i ]->Name, regexp ) )   // old & simplistic
      && greptags.Matched ( Markers[ i ]->Name ) )

        delete ( Markers[ i ] );            // delete object, but not pointer (yet)
    else
        markersok.Append ( Markers[ i ] );  // save pointer
    }


if ( (int) markersok == (int) Markers )     // nothing to remove
    return;


if ( markersok.IsEmpty () )                 // all to be erased
    Markers.Reset ( false );                // objects are already deleted, only clear-up the pointers now

else {
    Markers.Reset ( false );

    for ( int i = 0; i < (int) markersok; i++ )
        Markers.Append ( markersok[ i ] );  // just copy the pointer
    }


MarkersDirty    = true;
}


void    TMarkers::RemoveMarkers ( const TMarkers& removelist, MarkerType type )
{
for ( int ki = 0; ki < (int) removelist; ki++ )

    RemoveMarkers ( removelist[ ki ]->From, removelist[ ki ]->To, type );
}


//----------------------------------------------------------------------------
                                        // Will punch through existing markers
void    TMarkers::ClipMarkers ( long from, long to, MarkerType type )
{
KeepFlags   ( type, AllMarkerTypes );

if ( ! type )
    return;


MarkersList             clippedtags;

                                        // copy markers OK to a temp list
for ( int i = 0; i < (int) Markers; i++ ) {
                                        // not right type?
    if ( ! IsFlag ( Markers[ i ]->Type, type ) ) {
        clippedtags.Append ( Markers[ i ] );   // save pointer
        continue;
        }

                                        // 0 intersecting part
    if ( Markers[ i ]->IsNotOverlappingInterval ( from, to ) ) {
        clippedtags.Append ( Markers[ i ] );   // save pointer
        continue;
        }
                                    
                                        // 1 intersecting part
    if      ( IsInsideLimits ( Markers[ i ]->From, Markers[ i ]->To, from, to ) )
        delete ( Markers[ i ] );           // delete object, but not pointer (yet)

                                        // 1 intersecting part
    else if ( IsInsideLimits ( Markers[ i ]->From, from, to ) && Markers[ i ]->To > to ) {
        Markers[ i ]->From     = to + 1;       // clip the left edge
        clippedtags.Append ( Markers[ i ] );   // save pointer
        continue;
        }

                                        // 1 intersecting part
    else if ( IsInsideLimits ( Markers[ i ]->To, from, to ) && Markers[ i ]->From < from ) {
        Markers[ i ]->To       = from - 1;     // clip the right edge
        clippedtags.Append ( Markers[ i ] );   // save pointer
        continue;
        }

                                        // here the interval cuts current marker in 2
    else {
        clippedtags.Append ( new TMarker ( Markers[ i ]->From, from - 1,       Markers[ i ]->Code, Markers[ i ]->Name, Markers[ i ]->Type ) );
        clippedtags.Append ( new TMarker ( to + 1,          Markers[ i ]->To,  Markers[ i ]->Code, Markers[ i ]->Name, Markers[ i ]->Type ) );

        delete ( Markers[ i ] );           // delete current object, but not pointer (yet)
        }
    }


Markers.Reset ( false );                // some objects are already deleted, only clear-up the pointers now


InsertMarkers ( clippedtags );          // this will copy the data..

                                        // ..so delete the list now.
for ( int i = 0; i < (int) clippedtags; i++ )
    delete  clippedtags[ i ];


MarkersDirty    = true;
}


void    TMarkers::ClipMarkers ( const TMarkers& cliplist, MarkerType type )
{
for ( int ki = 0; ki < (int) cliplist; ki++ )

    ClipMarkers ( cliplist[ ki ]->From, cliplist[ ki ]->To, type );
}


//----------------------------------------------------------------------------
void    TMarkers::KeepMarkers ( long from, long to, MarkerType type )
{
RemoveMarkers ( Lowest<long> (), from - 1,         type );
RemoveMarkers ( to + 1,          Highest<long> (), type );
}


void    TMarkers::KeepMarkers ( const char* greppedwith )
{
//TSplitStrings     filteredwithsplit ( filteredwith, UniqueStrings );
TStringGrep         grepmarker ( greppedwith, GrepOptionDefaultMarkers );
TMarkers            markers;

                                        // Scan list and copy the markers to be kept in another list
for ( int i = 0; i < (int) Markers; i++ )

//  if ( filteredwithsplit.Contains ( Markers[ i ]->Name ) )
    if ( grepmarker.Matched ( Markers[ i ]->Name ) )

        markers.AppendMarker ( *Markers[ i ] );

                                        // finally replace with markers
SetMarkers ( markers );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
