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

#pragma once

#include    <iostream>
#include    <iomanip>
#include    <io.h>
#include    <fstream>

#include    "System.h"                  // LONGLONGTOLARGEINT

#include    "Strings.Utils.h"
#include    "Files.Utils.h"                   // TFilenameFlags

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Stream formatting
                                        // Fields should be accessed as  std::left  f.ex. but it seems std is already namespace'd somewhere(?)

constexpr auto      StreamFormatLeft        = left;
constexpr auto      StreamFormatRight       = right;

                                        // always show the digits afer the point (even if number is big)
inline  ostream&    StreamFormatFixed       ( ostream& os )     { os << fixed << showpoint << right; return os; }

                                        // always the same size, whatever the value
inline  ostream&    StreamFormatScientific  ( ostream& os )     { os << scientific         << right; return os; }

                                        // reset to default: fixed precision with variable point if enough room, scientific otherwise
inline  ostream&    StreamFormatGeneral     ( ostream& os )     { os << defaultfloat << noshowpoint; return os; }

                                        // define the total width for each type of variable, including 1 leading space and 1 sign
constexpr int       WidthFloat32            = 16;
constexpr int       WidthFloat64            = 26;
constexpr int       WidthFloat80            = 31;
constexpr int       WidthFloat128           = 46;
constexpr int       WidthInt8               =  6;
constexpr int       WidthInt16              =  8;
constexpr int       WidthInt32              = 16;
constexpr int       WidthInt64              = 26;
constexpr int       LeadingWidth            =  2;

                                                                     // force a space separator       remaining width (without space+sign)     repeat StreamFormatRight as left/right might change in a loop due to text f.ex.
inline  ostream&    StreamFormatFloat32     (ostream& os)       { os << " " << setprecision (  7 ) << setw ( WidthFloat32  - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatFloat64     (ostream& os)       { os << " " << setprecision ( 15 ) << setw ( WidthFloat64  - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatFloat80     (ostream& os)       { os << " " << setprecision ( 19 ) << setw ( WidthFloat80  - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatFloat128    (ostream& os)       { os << " " << setprecision ( 34 ) << setw ( WidthFloat128 - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatInt8        (ostream& os)       { os << " " << setprecision (  0 ) << setw ( WidthInt8     - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatInt16       (ostream& os)       { os << " " << setprecision (  0 ) << setw ( WidthInt16    - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatInt32       (ostream& os)       { os << " " << setprecision (  0 ) << setw ( WidthInt32    - LeadingWidth ) << StreamFormatRight; return os; }
inline  ostream&    StreamFormatInt64       (ostream& os)       { os << " " << setprecision (  0 ) << setw ( WidthInt64    - LeadingWidth ) << StreamFormatRight; return os; }


constexpr auto      StreamFormatText        = StreamFormatLeft;


//----------------------------------------------------------------------------
                                        // Stream utilities

//inline void       SkipCurrentLine     ( std::istream *is );
//inline void       SkipTrailingSpaces  ( std::istream *is );
inline char*        GetToken            ( std::istream* is, char* token );
inline int          CountLines          ( const char* file );
inline int          CountPerLine        ( const char* file );   // will count the # of space/tab/comma/semi-colon-separated tokens on the first line
inline int          CountTokens         ( const char* file );   // will count the # of tokens, irrespective of # per line & # of lines
inline void         AllocateFileSpace   ( std::ofstream* of, size_t sizetoallocate, DWORD fromwhere ); // pre-fill some empty space


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        TFileStreamEnums
            {
            FileStreamRead      = 1,
            FileStreamWrite,
            FileStreamUpdate,
            };

                                        // Wraps Windows system-level function CreateFile, which has more security options, and is supposed to be faster than fopen/streams
                                        // It could later use memory mapping, which is said to be the ultimate faster way to write files
                                        // Class also offers to update an existing file (without resetting it of course) to poke some data in
class   TFileStream
{
public:

    inline          TFileStream     ();
    inline          TFileStream     ( const char* file, TFileStreamEnums flags );
    inline         ~TFileStream     ();


    inline bool     IsOpen          ()  const           { return    hfile != 0; }

    inline bool     Open            ( const char* file, TFileStreamEnums flags );
    inline void     Close           ();

    inline void     Seek            ( const LARGE_INTEGER& pos );
    inline void     Seek            ( LONGLONG pos );

    inline  void    Read            (               LPVOID  data, DWORD sizeofdata );

    inline  void    Write           (               LPCVOID data, DWORD sizeofdata );
    inline  void    Write           ( LONGLONG pos, LPCVOID data, DWORD sizeofdata );
    inline  void    Write           ( LONGLONG pos, short   data );


    inline          operator bool   ()  const           { return    IsOpen (); }


protected:

    HANDLE          hfile;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
/*                                      // skip spaces until EOL
void    SkipCurrentLine     (   istream*    is  )
{
while ( (char) is->get() != '\n' );
}

                                        // skip spaces until EOL
void    SkipTrailingSpaces  (   istream*    is  )
{
char            buff;

                                        // if there is some non-space char remaining, don't skip it!
if ( ! IsSpaceNewline ( buff = (char) is->get() ) ) {
    is->putback ( buff );
    return;
    }
                                        // else eat white until EOL is found
SkipCurrentLine ( is );
}
*/

//----------------------------------------------------------------------------
                                        // Reading the next element not made of space-equivalent characters
                                        // Will skip first any space characters (including CR)
char*   GetToken    (   istream*    is, char*   token   )
{
char*               totoken         = token;

                                        // skip leading spacey things (space, tab, newline \n, carriage return \r ...)
while ( is->good () &&   IsSpaceNewline      ( *totoken   = (char) is->get () ) );

                                        // read non-spacey things, until a space, newline, a comma or semicolon
while ( is->good () && ! IsSpaceNewlineComma ( *++totoken = (char) is->get () ) );

                                        // setting EOS, overwriting the illegal character
ClearString ( totoken );


return  token;
}


//----------------------------------------------------------------------------
                                        // Counting all lines, including the empty ones
int     CountLines ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return  0;


ifstream       		ifs ( TFileName ( file, TFilenameExtendedPath ) );
                                        // in case of any opening error
if ( ifs.eof () || ifs.fail () )
    return  0;

return  count ( istreambuf_iterator<char> ( ifs ), istreambuf_iterator<char> (), '\n' );
}


//----------------------------------------------------------------------------
                                        // Counting the number of words, on the first line, separated by spaces or comma or semi-colon
int     CountPerLine ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return  0;


char                buff;
int                 numperline      = 0;
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ) );

                                        // in case of any opening error
if ( ifs.eof () || ifs.fail () )
    return  0;


do {
                                        // skip space, tabs, commas, semi-colon, but NOT \n
    while (   IsSpaceComma ( buff = (char) ifs.get () ) );

    if ( buff == '\n' )
        break;

                                        // read non-spacey things, until a space, newline, a comma or semicolon
    while ( ! IsSpaceNewlineComma ( buff = (char) ifs.get () ) );

                                        // that makes one more element
    numperline++;

    } while ( buff != '\n' );


return  numperline;
}


//----------------------------------------------------------------------------
int     CountTokens ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return  0;


char                buff[ 1024 ];
int                 numtokens       = 0;
ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ) );


while ( ! ifs.eof () && ifs.good () )

    if ( StringIsNotEmpty ( GetToken ( &ifs, buff ) ) )

        numtokens++;


return  numtokens;
}


//----------------------------------------------------------------------------
                                        // Fast allocation on disk
void    AllocateFileSpace   (   ofstream*   of,     size_t      sizetoallocate,     DWORD   fromwhere   )
{
if  ( ! of || sizetoallocate <= 0 )
    return;

/*
                                        // !There is no official way to get the file handle from an ofstream - code was maybe working before, but not in VS!

                                        // Fast Windows way (it seems to cast the stream* to a handle)
                                        // It is legal to access past the end of file
                                        // !Can use 64 bits adressing, with the optional high part!
DWORD               sfperr          = SetFilePointer  ( of , sizetoallocate, NULL, fromwhere );

if ( sfperr == ((DWORD)-1) /*INVALID_SET_FILE_POINTER* / ) {
    sfperr  = GetLastError ();
    if ( sfperr ) {}
    return;
    }

                                        // Actually extending file (could also be used to truncate)
DWORD               seoferr         = SetEndOfFile    ( of );

if ( seoferr == 0 ) {
    seoferr  = GetLastError ();
    if ( seoferr ) {}
    return;
    }
*/

                                        // Somehow optimized, and universal
//#define             RestBlockSize       KiloByte
#define             RestBlockSize       8192

char                buff[ RestBlockSize ];

ClearVirtualMemory  ( buff, RestBlockSize );
                                        // write by blocks first
for ( size_t i = 0; i < sizetoallocate / RestBlockSize; i++ )
    of->write ( buff, RestBlockSize );

                                        // then finish byte by byte
for ( size_t i = 0; i < sizetoallocate % RestBlockSize; i++ )
    of->put ( (char) 0 );


of->flush ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TFileStream::TFileStream ()
      : hfile ( 0 )
{
}


        TFileStream::TFileStream ( const char* file, TFileStreamEnums flags )
      : hfile ( 0 )
{
Open ( file, flags );
}


        TFileStream::~TFileStream ()
{
Close ();
}


//----------------------------------------------------------------------------
void    TFileStream::Close ()
{
if ( IsOpen ()  )
    CloseHandle ( hfile );
    
hfile       = 0;
}


//----------------------------------------------------------------------------
bool    TFileStream::Open ( const char* file, TFileStreamEnums flags )
{
Close ();


if ( StringIsEmpty ( file ) )
    return  false;

                                        // All files are open in random access mode, as this is our most common use
if      ( flags == FileStreamRead )

    hfile   = CreateFile    (   file,
                                GENERIC_READ,
                                FILE_SHARE_READ,                // yes, we can share while reading
                                NULL,                           // security
                                OPEN_EXISTING,                  // file must exist
                                FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/ | FILE_FLAG_RANDOM_ACCESS, 
                                NULL
                            );


else if ( flags == FileStreamWrite )

    hfile   = CreateFile    (   file,
                                GENERIC_WRITE,
                                0 /*FILE_SHARE_READ*/,          // forbid other processes to open this file while we write
                                NULL,                           // security
                                CREATE_ALWAYS,                  // will erase existing file
                                FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/ | FILE_FLAG_RANDOM_ACCESS,
                                NULL
                            );

                                        // Reopen for read-write
else if ( flags == FileStreamUpdate )

    hfile   = CreateFile    (   file,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,                // ?allowing other processes to read while we write?
                                NULL,                           // security
                                OPEN_EXISTING,                  // !updating only existing file!
                                FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/ | FILE_FLAG_RANDOM_ACCESS,
                                NULL
                            );


if ( hfile == INVALID_HANDLE_VALUE ) {
    hfile       = 0;
    return  false;
    }


return  true;
}


//----------------------------------------------------------------------------

void    TFileStream::Seek ( const LARGE_INTEGER& pos )
{
//if ( ! IsOpen () )
//    return;

SetFilePointerEx    (   hfile,
                        pos,
                        0,
                        FILE_BEGIN
                    );
}


void    TFileStream::Seek ( LONGLONG pos )
{
//if ( ! IsOpen () )
//    return;

SetFilePointerEx    (   hfile,
                        LONGLONGTOLARGEINT ( pos ), // ToLargeInt ( pos ),
                        0,
                        FILE_BEGIN
                    );
}


//----------------------------------------------------------------------------
                                        // Any write at current position
void    TFileStream::Write  ( LPCVOID data, DWORD sizeofdata )
{
WriteFile   (   hfile,
                data,
                sizeofdata,
                NULL,
                NULL
            );
}

                                        // Any write at given position
void    TFileStream::Write  ( LONGLONG pos, LPCVOID data, DWORD sizeofdata )
{
Seek    ( pos );

Write   ( data, sizeofdata );
}

                                        // Write short at given position
void    TFileStream::Write  ( LONGLONG pos, short data )
{
Seek    ( pos );

Write   ( &data, sizeof ( short ) );
}


//----------------------------------------------------------------------------
                                        // Any read at current position
void    TFileStream::Read   ( LPVOID data, DWORD sizeofdata )
{
ReadFile    (   hfile,
                data,
                sizeofdata,
                NULL,
                NULL
            );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
