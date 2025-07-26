/************************************************************************\
© 2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "CartoolTypes.h"            // TFilenameFlags
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.Utils.h"
#include    "Dialogs.Input.h"
#include    "TCartoolDocManager.h"

#pragma     hdrstop

#include    <Shlwapi.h>                 // PathIsRelative

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Encapsulating a long enough (for a path) string
                                        // It adds path and file specific methods, like Extended path for Windows
                                        // Overriding TFixedString to be case insensitive for file name comparisons

constexpr long      TFilenameSize           = CartoolMaxExtendedPath;


class   TFileName   : public    TFixedString<TFilenameSize>
{
public:
    using           TFixedString::TFixedString;
    using           TFixedString::operator=;
                    TFileName               ( const char* filename, TFilenameFlags flags = TFilenameNoPreprocessing )       { Set ( filename, flags ); }


    void            Set                     ( const char* filename, TFilenameFlags flags = TFilenameNoPreprocessing )       { Copy ( filename ); CheckFileName ( flags ); }
    void            SetTempFileName         ( const char* ext )                 { GetTempFilePath ( String ); if ( StringIsNotEmpty ( ext ) ) ReplaceExtension ( ext ); }


    bool            IsRelativePath          ()      const                       { return  crtl::IsRelativePath  ( String );     }
    bool            IsAbsolutePath          ()      const                       { return  crtl::IsAbsolutePath  ( String );     }
    bool            IsExtendedPath          ()                                  { return  crtl::IsExtendedPath  ( String );     }
    bool            IsExtension             ( const char* ext )     const       { return  crtl::IsExtension     ( String, ext );}
    void            ResetExtendedPath       ()                                  { if ( IsExtendedPath () ) StringClip ( String, StringLength ( ExtendedPathPrefix ), StringLength ( String ) - 1 ); } // remove extended special chars


    bool            CanOpenFile             ()      const                       { return  crtl::CanOpenFile     ( String, CanOpenFileRead ); }
    inline void     CheckFileName           ( TFilenameFlags flags );
    void            SetExtendedPath         ()                                  { crtl::SetExtendedPath         ( String ); }
    void            SetCorrectPathCase      ()                                  { crtl::SetCorrectPathCase      ( String ); }
    void            SetAbsolutePath         ()                                  { crtl::SetAbsolutePath         ( String, Size () ); }
    void            SetToSiblingFile        ()                                  { crtl::SetToSiblingFile        ( String ); }
    void            CheckNoOverwrite        ()                                  { crtl::CheckNoOverwrite        ( String ); }
    void            GetCurrentDir           ()                                  { ::GetCurrentDirectory ( Size (), String );}


    void            ClipFileName            ( int from, int to )                {               StringClip      ( ToFileName ( String ), from, to ); }  // only the (last) file name part
    char*           AddExtension            ( const char* ext    )              { return  crtl::AddExtension    ( String, ext    ); }
    char*           ReplaceExtension        ( const char* newext )              { return  crtl::ReplaceExtension( String, newext ); }
    char*           RemoveExtension         ()                                  { return  crtl::RemoveExtension ( String );         }
    char*           ReplaceDir              ( const char* newdir )              { return  crtl::ReplaceDir      ( String, newdir ); }
    char*           GetFilename             ()                                  { return  crtl::GetFilename     ( String );         }


    void            Show                    ( const char* title = 0 )   const   { ShowMessage ( String, TFixedString<256> ( StringIsEmpty ( title ) ? "File Name" : title ) ); }
    void            Open                    ()                                  { if ( IsNotEmpty () && CanOpenFile () ) CartoolObjects.CartoolDocManager->OpenDoc ( String, dtOpenOptions ); }


    TFileName&      operator    =           ( const TFileName& op2 )            { if ( &op2 != this )   Copy ( op2 );                   return *this;   }

                                                    // !ALWAYS override to case INSENSITIVE, as Windows still assumes FILE.txt is equivalent to file.txt!
    int             Compare                 ( const TFixedString&  op, StringFlags flags = CaseInsensitive )   const final { return StringCompare ( String, (const char*) op, CaseInsensitive /*flags*/ );  }
    int             Compare                 ( const char*          op, StringFlags flags = CaseInsensitive )   const final { return       Compare ( TFixedString ( op ),      CaseInsensitive /*flags*/ );  }

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

void   TFileName::CheckFileName ( TFilenameFlags flags )
{
                                        // Check path with this preferred sequence:
if ( IsFlag ( flags, TFilenameCorrectCase       ) )     SetCorrectPathCase  ();                 // convert path with actual lower and upper cases (used to convert old MS-DOS short names)
if ( IsFlag ( flags, TFilenameAbsolutePath      ) )     SetAbsolutePath     ();                 // convert a relative path to an absolute one
if ( IsFlag ( flags, TFilenameExtendedPath      ) )     SetExtendedPath     ();                 // add special prefix for path longer than 256
if ( IsFlag ( flags, TFilenameToSibling         ) )     SetToSiblingFile    ();                 // convert to another extension / sibling file for Cartool purpose (.img to .hdr f.ex.)
if ( IsFlag ( flags, TFilenameToDirectory       ) )     RemoveFilename      ( String, false );  // keeping only the directory part - trailing \ is removed
if ( IsFlag ( flags, TFilenameNoOverwrite       ) )     CheckNoOverwrite    ();                 // avoiding overwrite by generating a variation of given file name
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
