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

#pragma once

#include    "Strings.Utils.h"
#include    "Strings.TStrings.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // File path length (is a mess)

                                        // Windows backward limit (256+4)
constexpr int       WindowsMaxPath              = MAX_PATH;
                                        // Windows extended limit, BUT "\\?\" has to be added as prefix (see http://msdn.microsoft.com/en-us/library/aa365247%28v=vs.85%29.aspx#maxpath)
constexpr int       WindowsMaxExtendedPath      = 32767;
                                        // Cartool sets a smaller extended path:
constexpr int       CartoolMaxExtendedPath      = 2048;
                                        // Windows single directory maximum length (should be calling GetVolumeInformation)
constexpr int       WindowsMaxComponentLength   = 255;


                                        // Set to the restrictive side:
//constexpr int     MaxPathShort                = WindowsMaxPath;
                                        // Set to the extended side:
constexpr int       MaxPathShort                = CartoolMaxExtendedPath;

                                        // Prefix path with this magic string if path is greater than 260
constexpr char*     ExtendedPathPrefix          = "\\\\?\\";
                                        // Prefix for UNC convention, which therefore should be followed by "computer_name\shared_folder"
constexpr char*     UNCPathPrefix               = "\\\\?\\UNC\\";

                                        // for SetCaption
constexpr int       MaxAppTitleLength           = 1 * KiloByte;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class               TGoF;

                                        // Functions ACTUALLY operating on disk
void                SetCorrectPathCase              ( char* path );                         // updates path with actual cases
bool                CreateSubdirectoryFromFilename  ( char* path );                         // take the filename part, and insert a subdirectory with same name
char*               AppendFilenameAsSubdirectory    ( char* path );
bool                CreatePath                      ( const char* path, bool endswithfilename );  // make sure the whole chain of directories are created, up to the final file
int                 FilenameToSiblings              ( char* file, TGoF &gof );              // transforms/expands a filename into its legal sibling (.img -> .hdr, MFF -> signal?.bin, etc..)
void                SetToSiblingFile                ( char* path );
bool                IsRelativePath                  ( const char* path );
bool                IsAbsolutePath                  ( const char* path );
bool                IsExtendedPath                  ( const char* path );
void                SetAbsolutePath               ( char* path, int maxsize );            // converts to absolute
void                SetExtendedPath               ( char* path, bool force = false );     // checks if path needs the magic prefix "\\?\", then adds it
void                CheckNoOverwrite                ( char* path );                         // prevent overwriting by making the current file a variation

enum                FilesWithEegExt
                    {
                    Biologic,
                    BrainVision,
                    };

bool                DisambiguateExtEEG      ( const char* path, FilesWithEegExt filetype ); // we need to disambiguate some files with same extensions


void                NukeDirectory           ( const char* dir, bool confirm = false );                                  // !USE WITH CARE!
void                CopyFileExtended        ( const char* filefrom, const char* filedest, const char* buddyexts = 0 );  // Will use TFileName to safely and transparently manage long path in Windows
void                CopyFileToDirectory     ( const char* filetocopy, const char* todir, char* finalfile = 0 );
bool                CreateDirectoryExtended ( char* path );
void                MoveFileExtended        ( const char* filefrom,   const char* filedest );                           // Will use TFileName to safely and transparently manage long path in Windows
void                MoveFileToDirectory     ( const char* filetocopy, const char* todir, char* finalfile = 0 );
void                _CopyFileToDirectory    ( const char* filetocopy, const char* todir, bool deletesource, char* finalfile = 0 );
void                DeleteOneFile           ( const char* file );                                                       // !delete from disk! (can not use name 'DeleteFile' as it is a macro from Windows)
void                DeleteFiles             ( const char* filetemplate );                                               // !delete from disk!
void                DeleteFileExtended      ( const char* file, const char* buddyexts = 0 );                            // !delete from disk!
char*               GetFirstFile            ( char* templ );
//int               FindFiles               ( char* templ, TGoF& filenames, bool searchfiles = true );
//int               GrepFiles               ( char* path, char* regexp, TGoF& filenames, GrepOption option, bool searchfiles = true );


size_t              GetFileSize             ( char*        file, bool showerror = false );
size_t              GetFileSize             ( const char*  file, bool showerror = false );


//----------------------------------------------------------------------------
                                        // Specialized string utilities to deal with paths and file names

bool                IsAbsoluteFilename      ( const char* filename );
bool                IsFile                  ( const char* path );
bool                IsDirectory             ( const char* path );
bool                IsEmptyDirectory        ( const char* path );
bool                IsGeodesicsMFFPath      ( const char* filename );   // true for signal*.bin in mff directory - used so we can move up one directory


enum                MriContentType;
MriContentType      GuessTypeFromFilename   ( const char* filepath );
void                CheckMriExtension       ( char* filepath );
bool                IsTemplateFileName      ( const char* filepath );


const char*         ToLastDirectory         ( const char* filename );      // returning a pointer to the last directory part (backslash), without changing the content
char*               ToLastDirectory         ( char* filename );            // returning a pointer to the last directory part (backslash), without changing the content
char*               RemoveDir               ( char* filename );            // remove directory part from the string, not from the disk!
char*               RemoveLastDir           ( char* filename );
char*               GetLastDir              ( char* filename, char* dir );
char*               ReplaceDir              ( char* filename, const char* newdir );
char*               GetTempDir              ( char* tempdir );
char*               AppendDir               ( char* path, bool endswithfilename, const char* addeddir );

const char*         ToFileName              ( const char*  filepath );
char*               ToFileName              ( char*        filepath );
char*               GetFilename             ( char* filename );
char*               RemoveFilename          ( char* filename, bool keepbackslash = false );
char*               ReplaceFilename         ( char* filename, const char* newfilename );
char*               PrefixFilename          ( char* filename, const char* prefix, const char* tail = 0 );
char*               PostfixFilename         ( char* filename, const char* postfix );
char*               GetTempFileName         ( char* filename );            // only file name part
char*               GetTempFilePath         ( char* filename );            // full path
bool                TouchFile               ( const char* filename );

const char*         ToExtension             ( const char* filename );
void                GetExtension            ( char* ext, char*         filename );
void                GetExtension            ( char* ext, const char*   filename );
char*               RemoveExtension         ( char* filename, int num = 1 );
char*               ReplaceExtension        ( char* filename, const char* newext );
char*               AddExtension            ( char* filename, const char* ext );
char*               AddExtensionFromFile    ( char* filename, char* filewithext );
bool                IsExtension             ( const char* filename, const char* ext );
bool                IsExtensionAmong        ( const char* filename, const char* exts );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                CanOpenFlags
                    {
                    CanOpenFileRead             = 0x01,
                    CanOpenFileWrite            = 0x02,

                    CanOpenFileVerbose          = 0x10,
                    CanOpenFileAskUser          = 0x20,

                    CanOpenFileDefault          = CanOpenFileRead,
                    CanOpenFileReadAndAsk       = CanOpenFileRead  | CanOpenFileAskUser,
                    CanOpenFileWriteAndAsk      = CanOpenFileWrite | CanOpenFileAskUser,
                    };


bool                CanOpenFile      ( char*              file,   CanOpenFlags flags = CanOpenFileDefault );  // !can update path parameter!
bool                CanOpenFile      ( const char*        file,   CanOpenFlags flags = CanOpenFileDefault );
bool                CanOpenFile      ( const std::string& file,   CanOpenFlags flags = CanOpenFileDefault );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
