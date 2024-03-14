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

#include    <wtypes.h>

#include    "System.h"

#include    "Strings.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Encapsulate FindFirstFile and FindNextFile
class   TFindFile
{
public:
    inline              TFindFile ();
    inline              TFindFile ( const char* path );
    inline             ~TFindFile ();


    inline void         Reset               ();


    inline bool         SearchFirstFile     ( const char* path );
    inline bool         SearchNextFile      ();
    inline bool         SearchFile          ( const char* path );


    inline bool         IsOpen              ()  const   { return  hff != INVALID_HANDLE_VALUE; }
                                                // due to IsOpen, call the one function you need, like  IsDirectory  and not  !IsFile
    inline bool         IsDirectory         ()  const   { return  IsOpen () && ( finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0; }
    inline bool         IsFile              ()  const   { return  IsOpen () && ( finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0; }

    inline bool         IsOwnDirectory      ()  const   { return  StringIs ( GetPathChunk (), "."  ); }
    inline bool         IsAboveDirectory    ()  const   { return  StringIs ( GetPathChunk (), ".." ); }

    inline size_t       GetFileSize         ()  const;
    inline bool         GetFileTime         ( FILETIME &CreationTime, FILETIME &LastAccessTime, FILETIME &LastWriteTime )   const;
    inline FILETIME     GetCreationTime     ()  const;
    inline FILETIME     GetLastAccessTime   ()  const;
    inline FILETIME     GetLastWriteTime    ()  const;

                                           // returns only the last part of the path
    inline const char*  GetPathChunk        ()  const   { return  IsOpen () ? finddata.cFileName : ""; }


    inline TFindFile                            ( const TFindFile &op  );
    inline TFindFile&   operator    =           ( const TFindFile &op2 );

    inline              operator    bool        ()      const       { return  IsOpen ();  }
    inline              operator    const char* ()      const       { return  GetPathChunk (); }

private:
                                        // struct and handle used by the Win32 APIs
    WIN32_FIND_DATA finddata;
    HANDLE          hff;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TFindFile::TFindFile ()
      : hff ( INVALID_HANDLE_VALUE )
{
Reset ();
}


        TFindFile::TFindFile ( const char* path )
      : hff ( INVALID_HANDLE_VALUE )
{
SearchFirstFile ( path );
}


        TFindFile::~TFindFile ()
{
Reset ();
}


        TFindFile::TFindFile ( const TFindFile &op )
{
                                        // not sure if we can really duplicate these struct, or launch a new search instead?
finddata        = op.finddata;
hff             = op.hff;
}


TFindFile& TFindFile::operator= ( const TFindFile &op2 )
{
                                        // not sure if we can really duplicate these struct, or launch a new search instead?
finddata        = op2.finddata;
hff             = op2.hff;

return  *this;
}


//----------------------------------------------------------------------------
                                        // close any open search, reset variables
void   TFindFile::Reset ()
{
if ( IsOpen () )
    FindClose ( hff );


ClearVirtualMemory ( &finddata, sizeof ( finddata ) );
hff         = INVALID_HANDLE_VALUE;
}

                                        // force close anything open, then tries to retrieve first chunk
bool   TFindFile::SearchFirstFile ( const char* path )
{
Reset ();

if ( StringIsEmpty ( path ) )
    return  false;

                                        // also check for extended path
hff     = ::FindFirstFile ( TFileName ( path, TFilenameExtendedPath ), &finddata );


if ( IsOpen () )                        // first access is OK
    return  true;
else {                                  // nope, something is not right in the search
    Reset ();
    return  false;
    }
}


bool   TFindFile::SearchNextFile ()
{
if ( ! IsOpen () )
    return  false;


if ( ::FindNextFile ( hff, &finddata ) )// next is OK
    return  true;
else {                                  // no more
    Reset ();
    return  false;
    }
}

                                        // group both functions in a single one
                                        // Note that until SearchNextFile is done, there is no way to search for another string. In that case force call SearchFirstFile instead
bool   TFindFile::SearchFile ( const char* path )
{
if ( ! IsOpen () )  return  SearchFirstFile ( path );
else                return  SearchNextFile  ();
}


//----------------------------------------------------------------------------
bool        TFindFile::GetFileTime ( FILETIME &CreationTime, FILETIME &LastAccessTime, FILETIME &LastWriteTime )    const
{
                                        // struct is properly reset in case of closing or error
CreationTime    = finddata.ftCreationTime;
LastAccessTime  = finddata.ftLastAccessTime;
LastWriteTime   = finddata.ftLastWriteTime;

return  IsOpen ();
}


FILETIME    TFindFile::GetCreationTime ()   const
{
return  finddata.ftCreationTime;
}


FILETIME    TFindFile::GetLastAccessTime ()   const
{
return  finddata.ftLastAccessTime;
}


FILETIME    TFindFile::GetLastWriteTime ()   const
{
return  finddata.ftLastWriteTime;
}


size_t      TFindFile::GetFileSize  ()  const
{
return  IsFile () ? DWORDs_to_size_t ( finddata.nFileSizeHigh, finddata.nFileSizeLow ) 
                  : 0; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
