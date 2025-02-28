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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    <windows.h>
#include    <corecrt_io.h>
#include    <fstream>

#include    "Dialogs.Input.h"
#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "Files.TFindFile.h"
#include    "Files.TGoF.h"

#include    "TVolumeDoc.h"              // MriContentType

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Given a path "a la DOS", convert it to real Windows path (ie long file name + uppercase)
                                        // !path can actually grow, make sure the variable has enough allocated space!
void    MsDosPathToWindowsPath ( char* path )
{
if ( StringIsEmpty ( path ) )
    return;


TFileName           savedpath;
TFileName           incrpath;
TFindFile           findfile;
TSplitStrings       splitpath ( path, NonUniqueStrings, "\\" );
int                 spi             = 0;


//DBGM ( path, "MsDosPathToWindowsPath: old path" );


StringCopy  ( savedpath, path );
ClearString ( path     );
ClearString ( incrpath );

                                        // !we could also test for the optional extended string that stands before the drive letter!

                                        // copy first drive letter
if ( StringGrep ( splitpath[ spi ], "^[A-Za-z]:$", GrepOptionDefault ) ) {

    StringCopy ( path,     splitpath[ spi ] );
    StringCopy ( incrpath, splitpath[ spi ] );
                                        // skip drive letter
    ++spi;
    }


//CheckExtendedPath ( incrpath, true );

                                        // loop through each piece of path
for ( ; spi < (int) splitpath; ++spi ) {

                                        // next partial directory path
    StringAppend ( incrpath, StringIsNotEmpty ( incrpath ) ? "\\" : "", splitpath[ spi ] );

                                        // try to open current sub-path
    if ( findfile.SearchFirstFile ( incrpath ) )
                                        // next input piece OK, with actual Windows name
        StringAppend ( path, StringIsNotEmpty ( path ) ? "\\" : "", findfile.GetPathChunk () );

    else
                                        // next piece not OK, use the input one
        StringAppend ( path, StringIsNotEmpty ( path ) ? "\\" : "", splitpath[ spi ] );

//  DBGM ( path, "incrpath -> path" );
    }


//DBGM ( path, "MsDosPathToWindowsPath: processed path" );


                                        // do this here?
//CheckExtendedPath ( path /*, true*/ );

                                        // Old Borland does not like more than 256 long file names for the MRU
if ( StringLength ( path ) > WindowsMaxPath )
    StringCopy ( path, savedpath );


//DBGM ( path, "MsDosPathToWindowsPath: final path" );
}


//----------------------------------------------------------------------------
char*   AppendFilenameAsSubdirectory ( char* path )
{
if ( StringIsEmpty ( path ) )
    return  0;


TFileName           buff;


StringCopy   ( buff, ToFileName ( path ) );

StringAppend ( path, "\\", buff );

return  path;
}


bool    CreateSubdirectoryFromFilename ( char* path )
{
if ( StringIsEmpty ( path ) )
    return  false;

                                        // create sub-directory
bool                cdok            = CreatePath ( path, false ); // CreateDirectoryExtended ( path );

AppendFilenameAsSubdirectory ( path );

return  cdok;
}


//----------------------------------------------------------------------------
bool    CreatePath ( const char* path, bool endswithfilename )
{
if ( path == 0 )
    return  false;

if ( *path == 0 )
    return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName           pathok ( path, TFilenameExtendedPath );

                                        // remove any trailing backslash, to avoid an empty ending split string
if      ( StringEndsWith ( pathok, "\\" ) )

    StringClip ( pathok, StringLength ( pathok ) - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TSplitStrings       splitpath;
TFileName           incrpath;

                                        // test for extended path, which is a mess of backslashes
if ( pathok.IsExtendedPath () ) {
    StringCopy      ( incrpath,  ExtendedPathPrefix );  // is it too early to do that?
                                        // don't include the extended string
    splitpath.Set   ( (const char*) pathok + StringLength ( ExtendedPathPrefix ), NonUniqueStrings, "\\" );
    }
else
    splitpath.Set   ( pathok, NonUniqueStrings, "\\" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through all directories

                                        // if ending with a file name, don't use it as a directory
int                 maxnuggets      = (int) splitpath - ( endswithfilename ? 1 : 0 );
TFindFile           findfile;

                                        // loop through each piece of path
for ( int spi = 0; spi < maxnuggets; ++spi ) {

                                        // test for first nugget to be optionally a drive letter
    if ( spi == 0 && StringGrep ( splitpath[ spi ], "^[A-Za-z]:$", GrepOptionDefault ) ) {
        StringAppend    ( incrpath,     splitpath[ spi ] );
        continue;
        }

                                        // next partial directory path
    StringAppend    ( incrpath, "\\", splitpath[ spi ] );

                                        // test for existing path
    if ( findfile.SearchFirstFile ( incrpath ) ) {
                                        // a non-directory within our path? not so good
        if ( ! findfile.IsDirectory () )

            return  false;
                                        // a directory part is OK
//      else                            
//          ;                           // nothing to do, simply navigate to it
        }

    else                                // non-existing part 
                                        // create and test result
        if ( ! CreateDirectoryExtended ( incrpath ) )

            return  false;
    }


//DBGM3 ( path, pathok.FileName, incrpath.FileName, "CreatePath:OK" );

return  true;
}


//----------------------------------------------------------------------------
int     FilenameToSiblings ( char* file, TGoF &gof )
{
gof.Reset ();

if ( StringIsEmpty ( file ) )
    return  0;


if ( IsDirectory ( file ) ) {
                                        // try to convert a potential MFF directory into all signalX.bin
    TFileName               signaltemplate;
    TGoF                    signalsfiles;

    StringCopy   ( signaltemplate, file );
    StringAppend ( signaltemplate, "\\signal*.bin" );

                                        // get the files matching the template
    signalsfiles.FindFiles ( signaltemplate );

    for ( int i = 0; i < (int) signalsfiles; i++ ) {
//        DBGM2 ( file, signalsfiles[ i ], "replaced with signal file" );
        gof.Add ( signalsfiles[ i ] );
        }

    } // IsDirectory

else { // IsFile
                                        // be smart: replace .img files with .hdr
    if ( IsExtension ( file, FILEEXT_MRIAVW_IMG ) ) {

        ReplaceExtension ( file, FILEEXT_MRIAVW_HDR );
                                        // oops, can not find it? back-paddling to previous file...
        if ( ! CanOpenFile ( file, CanOpenFileRead ) )
            ReplaceExtension ( file, FILEEXT_MRIAVW_IMG );
        }

    else if ( IsExtension ( file, FILEEXT_BVHDR ) ) {
                                        // there could be 2 alternatives
        ReplaceExtension ( file, FILEEXT_BVEEG );

        if ( ! CanOpenFile ( file, CanOpenFileRead ) )
            ReplaceExtension ( file, FILEEXT_BVDAT );

        if ( ! CanOpenFile ( file, CanOpenFileRead ) )
            ReplaceExtension ( file, FILEEXT_BVHDR );
        }

    }


if ( ! (bool) gof )                     // no transformed files?
    gof.Add ( file );                   // put back original file

gof.Sort ();

return  (int) gof;
}


//----------------------------------------------------------------------------
                                        // !path will be overwritten, does it have enough space?!
void    CheckSiblingFile ( char* path )
{
//DBGM ( path, "CheckSiblingFile in" );


TGoF                siblingfiles;
                                        // MFF Directory to signal.bin / Analyze .img to .hdr / EEG header to data
FilenameToSiblings  ( path, siblingfiles );
                                        // copy only the first file, hopefully the good one?
StringCopy          ( path, siblingfiles[ 0 ] );


//DBGM ( path, "CheckSiblingFile out" );
}


//----------------------------------------------------------------------------
                                        // Disambiguate files with the same EEG extension
bool    DisambiguateExtEEG ( const char* path, FilesWithEegExt filetype )
{
if ( StringIsEmpty ( path ) )
    return  false;


TFileName           buff;
ifstream            ifs;


switch ( filetype ) {

    case    Biologic:

        ifs.open    ( path, ios::binary );

        ifs.seekg   ( 34, ios::beg );
        ifs.read    ( buff,  5 );

        buff[ 5 ]   = 0;

        return  StringIs ( buff, "BLSC" );


    case    BrainVision:

        StringCopy          ( buff, path );
        ReplaceExtension    ( buff, FILEEXT_BVHDR );

        return  CanOpenFile ( buff );
    }

return  false;
}


//----------------------------------------------------------------------------
void        CheckAbsolutePath ( char* path, int maxsize )
{
if ( StringIsEmpty ( path ) )
    return;


_fullpath ( path, path, maxsize );
}


//----------------------------------------------------------------------------
                                        // check for very long filenames, and prepend the magic string if needed
void        CheckExtendedPath ( char* path, bool force )
{
if ( StringIsEmpty ( path ) )
    return;

                                        // Windows default path length is still 256 + 4 = 260
if ( ( force || StringLength ( path ) >= WindowsMaxPath )
  && ! IsExtendedPath ( path ) )

    StringPrepend ( path, ExtendedPathPrefix );
}


bool        IsExtendedPath  ( char* path )
{
return  StringStartsWith ( path, ExtendedPathPrefix ); 
}


//----------------------------------------------------------------------------
                                        // check the given path does not exist, and if it does, add a counter to disambiguate it as to avoid overwriting files
void        CheckNoOverwrite ( char* path )
{
if ( StringIsEmpty ( path ) )
    return;

                                        // file doesn't seem to exist -> we good
if ( ! CanOpenFile ( path, CanOpenFileRead ) )
    return;

                                        // Make some variation and test for existence

TFileName           oldpath ( path, TFilenameExtendedPath );
char                ext [ 256 ];

                                        // special case for error.data files, treating it as a single file extension
if ( StringEndsWith ( oldpath, FILEEXT_ERRORDATA ) ) {

    StringCopy      ( ext, FILEEXT_ERRORDATA );

    RemoveExtension ( oldpath, 2 );
    }
else {
    GetExtension    ( ext, oldpath );

    RemoveExtension ( oldpath );
    }

                                        // starts variation from 2, as this is the second version of the file
for ( int vari = 2; vari < 1000; vari++ ) {

    StringCopy  ( path, oldpath, "_", IntegerToString ( vari ), ".", ext );

    if ( ! CanOpenFile ( path, CanOpenFileRead ) )
        return;
    }

                                        // we have too much variations - what to do now? crashing maybe?
ClearString ( path );
}


//----------------------------------------------------------------------------
                                        // total recursive clean-up
                                        // ! USE IT WITH CARE!
void    NukeDirectory ( const char* dir, bool confirm )
{
if ( StringIsEmpty ( dir ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName           realdir;
TFileName           file;

StringCopy ( realdir, dir );                // work with a local copy


bool                dirhaswildchar  = StringContains ( (const char*) realdir, "*", CaseSensitive );


if ( dirhaswildchar ) {                 // search will begin at directory level
    StringCopy      ( file, realdir );
    RemoveFilename  ( realdir );
    }
else {
                                        // in case the directory is not a dir, move up to parent directory
    if ( IsFile ( realdir ) )
        RemoveFilename ( realdir );

    StringCopy      ( file, realdir, "\\*" ); // search will begin under specified directory

//    DBGM ( file, "nuke directory" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OK, get a last chance...
if ( confirm )
    if ( ! GetAnswerFromUser ( "Deleting directory (NO undo)?", realdir ) )
        return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan for files & sub-directories
TFindFile           findfile;


while ( findfile.SearchFile ( file ) ) {
                                        // don't loop within same directory AND DON'T SWITCH TO DIR ABOVE (yes, I did it...)!
    if ( findfile.IsOwnDirectory   () 
      || findfile.IsAboveDirectory () )
        continue;


    StringCopy ( file, realdir, "\\", findfile.GetPathChunk () );


    if ( findfile.IsDirectory () )
                                        // recursively nuke sub-directories
        NukeDirectory       ( file );
    else {
                                        // actual delete here
        DeleteFileExtended  ( file );
//      DBGM ( file, "File - Would be deleted" );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if ( ! dirhaswildchar ) {
    WaitMilliseconds ( 100 );           // wait a bit?
    RemoveDirectory ( realdir );
//  DBGM ( dir, "Dir  - Would be removed" );
    }
}


//----------------------------------------------------------------------------
                                        // Extended version, i.e. checking for long paths and convert them to Windows special format on the fly
void    CopyFileExtended ( const char* filefrom, const char* filedest, const char* buddyexts )
{
if ( StringIsEmpty  ( filefrom ) 
  || StringIsEmpty  ( filedest ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert, check for extended path, and cast to resulting char*
TFileName           fnfrom ( filefrom, TFilenameExtendedPath );
TFileName           fndest ( filedest, TFilenameExtendedPath );

                                        // useless copy?
if ( fnfrom == fndest )
    return;


CopyFile    ( fnfrom, fndest, false );
                                        // copying is also copying the file attributes - update them all
TouchFile   ( fndest );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // given buddy extensions
TSplitStrings           exts ( buddyexts, UniqueStrings );

                                        // loop through exts + no ext change case
for ( int i = 0; i < (int) exts; i++ ) {

                                        // avoiding doing twice a copy
    if ( IsExtension ( filefrom, exts[ i ] ) )
         continue;

    ReplaceExtension    ( fnfrom, exts[ i ] );
    ReplaceExtension    ( fndest, exts[ i ] );

    if ( fnfrom == fndest )
        continue;


    CopyFile    ( fnfrom, fndest, false );
                                        // copying is also copying the file attribures - update them all
    TouchFile   ( fndest );
    } // for exts

}


//----------------------------------------------------------------------------
void    MoveFileExtended ( const char* filefrom, const char* filedest )
{
if ( StringIsEmpty  ( filefrom ) 
  || StringIsEmpty  ( filedest ) )
    return;

                                        // convert, check for extended path, and cast to resulting char*
TFileName           fnfrom ( filefrom, TFilenameExtendedPath );
TFileName           fndest ( filedest, TFilenameExtendedPath );

                                        // nothing to move?
if ( fnfrom == fndest )
    return;

                                        // convert, check for extended path, and cast to resulting char*
MoveFileEx  (   fnfrom, fndest, 
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED 
            );
}


//----------------------------------------------------------------------------
bool    CreateDirectoryExtended ( char* path )
{
if ( StringIsEmpty ( path ) )
    return  false;

                                                                   // no security attributes
return  CreateDirectory ( TFileName ( path, TFilenameExtendedPath ), 0 );
}


//----------------------------------------------------------------------------
void    CopyFileToDirectory ( const char* filetocopy, const char* todir, char* finalfile )
{
_CopyFileToDirectory ( filetocopy, todir, false, finalfile );
}


void    MoveFileToDirectory ( const char* filetocopy, const char* todir, char* finalfile )
{
_CopyFileToDirectory ( filetocopy, todir, true, finalfile );
}


void    _CopyFileToDirectory ( const char* filetocopy, const char* todir, bool deletesource, char* finalfile )
{
if ( StringIsEmpty ( filetocopy ) || StringIsEmpty ( todir ) )
    return;


TFileName           CopyFromFile ( filetocopy, TFilenameNoPreprocessing );
TFileName           CopyToFile   ( filetocopy, TFilenameNoPreprocessing );


ReplaceDir          ( CopyToFile, todir ); 


if ( deletesource )     MoveFileExtended    ( CopyFromFile, CopyToFile );
else                    CopyFileExtended    ( CopyFromFile, CopyToFile );

                                        // caller might want to get the resulting file name?
if ( finalfile )
    StringCopy ( finalfile, CopyToFile );
}


//----------------------------------------------------------------------------
                                        // Just wrapping DeleteFile / DeleteFileA
void    DeleteOneFile ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return;

//ShowValues ( "DeleteOneFile", "s", file );  // in case of doubt

DeleteFile ( TFileName ( file, TFilenameExtendedPath ) );
}


//----------------------------------------------------------------------------
                                        // Delte all files from search template, like "SomeFile*"
void    DeleteFiles ( const char* filetemplate )
{
if ( StringIsEmpty ( filetemplate ) )
    return;


TGoF                allfiles;
                                        // this is supposed to be a simple wildchar search - use GrepFiles otherwise
allfiles.FindFiles ( filetemplate );


for ( int i = 0; i < (int) allfiles; i++ ) {

//  ShowValues ( "DeleteFiles", "is", i + 1, allfiles[ i ] );   // in case of doubt

    DeleteOneFile ( allfiles[ i ] );
    }
}


//----------------------------------------------------------------------------
                                        // Delete given file + optional variations from a specified list of extensions
void    DeleteFileExtended ( const char* file, const char* buddyexts )
{
if ( StringIsEmpty ( file ) )
    return;


DeleteOneFile ( file );


if ( StringIsEmpty ( buddyexts ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // given buddy extensions
TSplitStrings           exts ( buddyexts, UniqueStrings );
TFileName               filename;

                                        // .mrk extensions need a bit of work-around
if ( exts.Contains ( FILEEXT_MRK ) ) {

    char            extmrk[ 256 ];
                                        // actually add the compound extension "<currentext>.mrk"
    StringCopy      ( extmrk, ToExtension ( file ), ".", FILEEXT_MRK );

    exts.AddToken   ( extmrk );
    }
    
                                        // .vrb extensions need a bit of work-around
if ( exts.Contains ( FILEEXT_VRB ) ) {

    char            extvrb[ 256 ];
                                        // actually add the compound extension "<currentext>.mrk"
    StringCopy      ( extvrb, ToExtension ( file ), ".", FILEEXT_VRB );

    exts.AddToken   ( extvrb );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through replacement extensions
for ( int i = 0; i < (int) exts; i++ ) {

    StringCopy          ( filename, file );

    ReplaceExtension    ( filename, exts[ i ] );

//  ShowValues ( "DeleteFileExtended", "iss", i + 1, ToFileName ( file ), ToFileName ( filename.FileName ) );   // in case of doubt

    DeleteOneFile       ( filename );
    } // for exts

}


//----------------------------------------------------------------------------
char*   GetFirstFile ( char* templ )
{
if ( StringIsEmpty ( templ ) )
    return  0;


TFindFile           findfile;


while ( findfile.SearchFile ( templ ) ) {

    if ( findfile.IsDirectory () )
        continue;
                                        // we have a real file here

    RemoveFilename  ( templ );
    StringAppend    ( templ, "\\", findfile.GetPathChunk () );


//    DBGM ( templ, "First File found" );

    return  templ;
    }

                                        // nothing found
ClearString ( templ );

return  0;
}


//----------------------------------------------------------------------------
                                        // old code, use preferably from TGoF now
//int     FindFiles ( char* templ, TGoF& filenames, bool searchfiles )
//{
//return  filenames.FindFiles ( templ, searchfiles );
//}


//----------------------------------------------------------------------------
                                        // old code, use preferably from TGoF now
//int     GrepFiles ( char* path, char* regexp, TGoF& filenames, GrepOption option, bool searchfiles )
//{
//return  filenames.GrepFiles ( path, regexp, option, searchfiles );
//}


//----------------------------------------------------------------------------
size_t      GetFileSize ( char* file, bool /*showerror*/ )
{
return  TFindFile ( file ).GetFileSize ();
}


size_t      GetFileSize ( const char*  file, bool showerror )
{
return  GetFileSize ( (char *) file, showerror ); 
}


//----------------------------------------------------------------------------
/*
bool    IsAbsolutePath ( const char* filename )
{
if ( filename == 0 || StringLength ( filename ) < 3 )
    return  false;

return  isalpha ( *filename ) && StringStartsWith ( filename + 1, ":\\" );
}


bool    IsAbsoluteFilename ( const char* filename )
{
if ( StringIsEmpty ( filename ) || StringLength ( filename ) < 3 )
    return  false;

return  isalpha ( *filename ) && StringStartsWith ( filename + 1, ":\\" ) && *LastChar ( filename ) != '\\';
}
*/

bool    IsAbsoluteFilename ( const char* filename )
{
//static TStringGrep    grepdos         ( "^[A-Za-z]:(\\\\[^\\\\]+)+$",                     GrepOptionDefault );    // DOS / Windows    C:\basefilename
//static TStringGrep    grepwinunc1     ( "^\\\\\\\\\\?\\\\[A-Za-z]:(\\\\[^\\\\]+)+$",      GrepOptionDefault );    // Windows UNC      \\?\C:\basefilename
static TStringGrep      grepdoswinunc1  ( "^(\\\\\\\\\\?\\\\|)[A-Za-z]:(\\\\[^\\\\]+)+$",   GrepOptionDefault );    // the previous 2

//static TStringGrep    grepwinunc2     ( "^\\\\(\\\\[^\\\\\\?]+){2,}$",                    GrepOptionDefault );    // Windows UNC      \\server\volume\basefilename
//static TStringGrep    grepwinunc3     ( "^\\\\\\\\\\?(\\\\[^\\\\\\?]+){2,}$",             GrepOptionDefault );    // Windows UNC      \\?\server\volume\basefilename
//static TStringGrep    grepwinunc4     ( "^\\\\\\\\\\?\\\\UNC(\\\\[^\\\\\\?]+){2,}$",      GrepOptionDefault );    // Windows UNC      \\?\UNC\server\volume\basefilename
static TStringGrep      grepwinunc234   ( "^\\\\(\\\\\\?(\\\\UNC|)|)(\\\\[^\\\\\\?]+){2,}$",GrepOptionDefault );    // all the previous 3

//static TStringGrep    grepunix        ( "^(/[^/]+)+$",                                    GrepOptionDefault );    // Unix             /basefilename

//static TStringGrep    grepmac         ( "^\\S.*(:[^:\\\\]+)+$",                           GrepOptionDefault );    // Mac OS           Hard drive:basefilename


return  grepdoswinunc1.Matched ( filename )
     || grepwinunc234 .Matched ( filename )
//   || grepunix      .Matched ( filename )
//   || grepmac       .Matched ( filename )     // it interferes a bit too much with DOS notation
        ;
}


//----------------------------------------------------------------------------
bool    IsFile ( const char* path )
{
//return  TFindFile ( path ).IsFile ();

DWORD               dwAttrib        = GetFileAttributes ( path );

return     dwAttrib != INVALID_FILE_ATTRIBUTES
    && ! ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY );
}


//----------------------------------------------------------------------------
bool    IsDirectory ( const char* path )
{
//return  TFindFile ( path ).IsDirectory ();

DWORD               dwAttrib        = GetFileAttributes ( path );

return     dwAttrib != INVALID_FILE_ATTRIBUTES
    &&   ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY );
}


//----------------------------------------------------------------------------
bool    IsEmptyDirectory ( const char* path )
{
if ( StringIsEmpty ( path ) )
    return  false;


TFileName           templ;

StringCopy   ( templ, path );

if ( ! StringEndsWith ( templ, "\\" ) )
    StringAppend ( templ, "\\" );

StringAppend ( templ, "*" );


return  IsDirectory ( path ) && ! GetFirstFile ( templ );
}


//----------------------------------------------------------------------------
bool        IsGeodesicsMFFPath ( const char* filename )
{
if ( StringIsEmpty ( filename ) )
    return  false;


static TStringGrep  grepeegmff ( "signal.+\\." FILEEXT_EEGMFF "$", GrepOptionDefaultFiles );


return  grepeegmff.Matched ( filename ) > 0;
}


//----------------------------------------------------------------------------
MriContentType  GuessTypeFromFilename   ( const char* filepath )
{
if ( StringIsEmpty ( filepath ) )
    return  UnknownMriContentType;


GrepOption          options         = GrepOptionDefaultFiles;
#define             nameseparator   "(\\.|_|-)"


if ( IsExtensionAmong ( filepath, AllMriFilesExt " " FILEEXT_MRIAVW_IMG ) ) {

                                        // !tests are kept SEQUENTIALS on purpose: start from the LATEST POSSIBLE PROCESSING, in case the name was inflated one step after the other!
    if ( StringGrep ( filepath, nameseparator "ROI"                         nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeRoiMask      );
    if ( StringGrep ( filepath, nameseparator "mask"                        nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeBinaryMask   );
    if ( StringGrep ( filepath, nameseparator "(gray|grey|class-GM)"        nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeGrey         );
    if ( StringGrep ( filepath, nameseparator "(white|class-WM)"            nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeWhite        );
    if ( StringGrep ( filepath, nameseparator "(brain|seg|full brain)"      nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeBrain        );
    if ( StringGrep ( filepath, nameseparator "(head|full|original)"        nameseparator,  options ) )     return                     MriContentTypeFullHead;
    if ( StringGrep ( filepath, nameseparator "tissues"                     nameseparator,  options ) )     return  (MriContentType) ( MriContentTypeSegmented | MriContentTypeRoiMask      );
    } // MRI type


return  UnknownMriContentType;
}


//----------------------------------------------------------------------------
void    CheckMriExtension ( char* filepath )
{
if ( StringIsEmpty ( filepath ) )
    return;

                                        // replace .img with .hdr
if (   IsExtension ( filepath, FILEEXT_MRIAVW_IMG ) )

    ReplaceExtension ( filepath, FILEEXT_MRIAVW_HDR );

                                        // check within the list of legal MRI extensions
if ( ! IsExtensionAmong ( filepath, AllMriFilesExt ) )
                                        // if not, add the default one
    AddExtension ( filepath, DefaultMriExt );
}


//----------------------------------------------------------------------------
bool    IsTemplateFileName ( const char* filepath )
{
if ( StringIsEmpty ( filepath ) )
    return  false;


TFileName           filename;

StringCopy  ( filename, filepath );
GetFilename ( filename );


bool                istemplate      = StringStartsWith ( filename, "MNI152" )   // T1/T2/PD
                                   || StringStartsWith ( filename, "MNI305" )
                                   || StringStartsWith ( filename, "avg152" )
                                   || StringStartsWith ( filename, "ave152" );

return  istemplate;
}


//----------------------------------------------------------------------------
char*   RemoveDir ( char* filename )
{
if ( StringIsEmpty ( filename ) )
    return 0;

const char*         tof             = ToFileName ( filename );

if ( StringIsNotEmpty ( tof ) )

    MoveVirtualMemory ( filename, (void *) tof, StringSize ( tof ) );


return  filename;
}


char*   RemoveLastDir ( char* filename )
{
if ( StringIsSpace ( filename ) )
    return 0;


TFileName           buff ( filename, TFilenameNoPreprocessing );

                                        // remove file name
RemoveFilename  ( filename );
                                        // remove directory
RemoveFilename  ( filename );
                                        // reconstruct by adding saved trail
StringAppend    ( filename, "\\", ToFileName ( buff ) );


return  filename;
}


char*   GetLastDir ( char* filename, char* dir )
{
if ( StringIsSpace ( filename ) || dir == 0 )
    return 0;


TFileName           buff ( filename, TFilenameNoPreprocessing );

                                        // remove file name
RemoveFilename  ( buff, false );

StringCopy      ( dir, ToFileName ( buff ) );


return  dir;
}


char*   ReplaceDir ( char* filename, const char* newdir )
{
if ( StringIsEmpty ( filename ) || StringIsEmpty ( newdir ) )
    return 0;


TFileName           buff ( filename, TFilenameNoPreprocessing );

StringCopy  ( filename, newdir, "\\", ToFileName ( buff ) );

return  filename;
}


char*   AppendDir        ( char* path, bool endswithfilename, const char* addeddir )
{
if ( StringIsEmpty ( path ) || StringIsEmpty ( addeddir ) )
    return  path;


//TFileName           buff ( path, TFilenameNoPreprocessing );
TFileName           filename;


if ( endswithfilename ) {               // save file name itself
    StringCopy      ( filename, path );
    RemoveDir       ( filename );
    RemoveFilename  ( path );
    }


if ( *LastChar  ( path ) != '\\' )
    StringAppend ( path, "\\" );

                                        // add new directory, without ending backslash
StringAppend ( path, addeddir );

                                        // put back file name if needed
if ( endswithfilename )
    StringAppend ( path, "\\", filename );


return  path;
}


//----------------------------------------------------------------------------
char*   GetTempDir ( char* tempdir )
{
if ( ! tempdir )
    return 0;

ClearString ( tempdir );


if ( GetTempPath ( MaxPathShort, tempdir ) == 0 )
    return  0;

return  tempdir;
}


char*   GetTempFileName  ( char* filename )
{
if ( ! filename )
    return 0;

ClearString ( filename );


StringRandom ( StringEnd ( filename ), 8 );
AddExtension ( filename, "tmp" );

return  filename;
}


char*   GetTempFilePath  ( char* filename )
{
if ( ! filename )
    return 0;

ClearString ( filename );


GetTempDir      ( filename );

if ( *LastChar  ( filename ) != '\\' )
    StringAppend ( filename, "\\" );

GetTempFileName ( StringEnd ( filename ) );

return  filename;
}


//----------------------------------------------------------------------------
bool    TouchFile ( const char* filename )
{
if ( ! filename )
    return  false;


SYSTEMTIME          systemtime;
FILETIME            ftsystemtime;


GetSystemTime           ( &systemtime );

                                        // Converts the current system time to file time format
SystemTimeToFileTime    ( &systemtime, &ftsystemtime);


                                        // Maybe put the CreateFile in wrapper function?
HANDLE              hFile;

hFile       = CreateFile    (   TFileName ( filename, TFilenameExtendedPath ), 
                                GENERIC_WRITE,  // !this one is important here!
                                0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 
                            );


if ( hFile == INVALID_HANDLE_VALUE ) {

//  if ( showerror )
//      ShowLastError ();

    return  false;
    }

                                        // use  (LPFILETIME) NULL  to skip some time stamps
SetFileTime (   hFile, 
                &ftsystemtime,          // Creation Time            
                &ftsystemtime,          // Last Access Time
                &ftsystemtime           // Last Write Time (update)
            );


CloseHandle ( hFile );


return  true;
}


//----------------------------------------------------------------------------
                                        // pointing to the last backslash
const char*     ToLastDirectory ( const char* filename )
{
if ( StringIsEmpty ( filename ) )
    return  0;

return  StringContains ( filename, '\\', StringContainsBackward );
}


char*           ToLastDirectory ( char* filename )
{
if ( StringIsEmpty ( filename ) )
    return  0;

return  StringContains ( filename, '\\', StringContainsBackward );
}


const char*     ToFileName ( const char* filepath )
{
const char*         tof             = ToLastDirectory ( filepath );
                                        // skip the backslash
return  tof ? tof + 1 : ""; // 0;
}


char*           ToFileName ( char* filepath )
{
char*               tof             = ToLastDirectory ( filepath );
                                        // skip the backslash
return  tof ? tof + 1 : ""; // 0;
}


const char*     ToExtension ( const char* filename )
{
if ( StringIsEmpty ( filename ) || *LastChar ( filename ) == '\\' )
    return  "";

const char*             toext           = StringContains ( filename, '.', StringContainsBackward );

return  toext ? toext + 1 : "";
}


//----------------------------------------------------------------------------
char*   GetFilename ( char* filename )
{
return  RemoveDir ( RemoveExtension ( filename ) );
}


char*   RemoveFilename ( char* filename, bool keepbackslash )
{
if ( filename == 0 )
    return 0;

char*               tof             = ToLastDirectory ( filename );

if ( tof == 0 )
    return  0;

                                        // keeping the backslash?
if ( keepbackslash )  
    tof++;

                                        // clearing within string filename
ClearString ( tof );


return  filename;
}


char*   ReplaceFilename ( char* filename, const char* newfilename )
{
                                        // remove file name
RemoveFilename  ( filename );
                                        // reconstruct by adding saved trail
StringAppend    ( filename, "\\", newfilename );

return  filename;
}


char*   PrefixFilename ( char* filename, const char* prefix, const char* tail )
{
if ( StringIsEmpty ( filename ) || StringIsEmpty ( prefix ) )
    return  0;


TFileName           buff ( filename, TFilenameNoPreprocessing );


RemoveFilename  ( filename );
                                       // !tail of the prefix!
StringAppend    ( filename, "\\", prefix, tail, ToFileName ( buff ) );

return  filename;
}


char*   PostfixFilename ( char* filename, const char* postfix )
{
if ( StringIsEmpty ( filename ) || StringIsEmpty ( postfix ) )
    return  0;


char                ext[ 256 ];


GetExtension    ( ext, filename );

RemoveExtension ( filename );

StringAppend    ( filename, postfix );

AddExtension    ( filename, ext );

return  filename;
}


//----------------------------------------------------------------------------
void   GetExtension ( char* ext, char* filename )
{
if ( ! ext || StringIsEmpty ( filename ) )
    return;

                                        // might not work when called with both arguments the same variable
//StringCopy      ( ext, ToExtension ( filename ) );

                                        // safe version
const char*             toext           = ToExtension ( filename );

MoveVirtualMemory   ( ext, const_cast<char*> ( toext ), StringSize ( toext ) );

                                        // safe version
//char                extcopy[ 256 ];
//
//StringCopy      ( extcopy, ToExtension ( filename ) );
//
//StringCopy      ( ext, extcopy );
}


void    GetExtension     ( char* ext, const char*   filename )
{
GetExtension ( ext, (char *) filename ); 
}


char*   RemoveExtension ( char* filename, int num )
{
if ( filename == 0 || num <= 0 )
    return 0;


char*               toext;

for ( ; num > 0; num-- ) {

    toext   = StringContains ( filename, '.', StringContainsBackward );

    if ( toext )    ClearString ( toext );
    else            break;
    }


return  filename;
}


char*   ReplaceExtension ( char* filename, const char* newext )
{
RemoveExtension ( filename );

AddExtension    ( filename, newext );

return  filename;
}


char*   AddExtension ( char* filename, const char* ext )
{
if ( StringIsEmpty ( filename ) || StringIsEmpty ( ext ) )
    return  filename;


StringAppend ( filename, ".", ext );

return  filename;
}


char*   AddExtensionFromFile ( char* filename, char* filewithext )
{
if ( StringIsEmpty ( filename ) || StringIsEmpty ( filewithext ) )
    return  filename;


char                ext[ 256 ];

GetExtension        ( ext, filewithext );
                                        // force extension to lowercase, as some files have it in uppercase
StringToLowercase   ( ext );


return  AddExtension ( filename, ext );
}


//----------------------------------------------------------------------------
bool    IsExtension ( const char* filename, const char* ext )
{
return  StringIs ( ToExtension ( filename ), ext );
}


bool    IsExtensionAmong ( const char* filename, const char* exts )
{
return  IsStringAmong ( ToExtension ( filename ), exts );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool    CanOpenFile ( const char *file, CanOpenFlags flags )
{
                           // shielding the file parameter from any modification
return  CanOpenFile ( (char*) TFileName ( file ), flags );
}


bool    CanOpenFile ( const string& file, CanOpenFlags flags )
{
return  CanOpenFile ( (char*) TFileName ( file.c_str () ), flags );
}


//----------------------------------------------------------------------------
bool    CanOpenFile ( char *file, CanOpenFlags flags )
{
if ( StringIsEmpty ( file ) )
    return  false;
                                        // not specified how to open?
if ( ! IsFlag ( flags, CanOpenFlags ( CanOpenFileRead | CanOpenFileWrite ) ) )
    return  false;

                                        // modify given file?
CheckExtendedPath ( file );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test for reading
if      ( IsFlag ( flags, CanOpenFileRead ) ) {

    if ( IsFile ( file ) )
        return  true;

                                        // there is a problem, should we be asking?
    if      ( IsFlag ( flags, CanOpenFileAskUser ) ) {

        static GetFileFromUser  getfile ( "File could not be read, please choose another file:", AllFilesFilter, 1, GetFileRead );

        if ( ! getfile.Execute ( file ) )

            return  false;
        else {
            StringCopy ( file, (char *) getfile );
            return  true;
            }
        }
                                        // or should we just be reporting?
    else if ( IsFlag ( flags, CanOpenFileVerbose ) )

        ShowMessage ( "Can not open file!", file, ShowMessageWarning );


    return  false;
    } // CanOpenFileRead


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test for writing
else if ( IsFlag ( flags, CanOpenFileWrite ) ) {

    static GetFileFromUser  getfile ( "", AllFilesFilter, 1, GetFileWrite );

                                        // does the file already exist?
//  bool                fileexist   = _access   ( file, 00 ) == 0;
    bool                fileexist   = _access_s ( file, 00 ) == 0;  // more secure version(?)

                                        // if file already exist, optionally ask for overwriting permission
    if ( fileexist ) {

        if      ( IsFlag ( flags, CanOpenFileAskUser ) ) {

            getfile.SetTitle ( "File already exists, select another one or overwrite it?" );

            if ( ! getfile.Execute ( file ) )

                return  false;
            else
                StringCopy ( file, (char *) getfile );
            }

        else if ( IsFlag ( flags, CanOpenFileVerbose ) )

            if ( ! GetAnswerFromUser ( "File already exists, overwite it?", file ) )

                return  false;

        //else: overwriting is the default
        }

                                        // real attempt to create the file, and see (without overwriting, we are just performing a test!)
    fstream             fs ( TFileName ( file, TFilenameExtendedPath ), ios::out | ios::binary | ios::app );
    bool                fileok          = fs.good ();
    fs.close ();


    if ( fileok ) {
//        DBGV2 ( fileexist, GetFileSize ( file ), file );

        if ( GetFileSize ( file ) == 0 )

            DeleteFileExtended ( file );    // to be clean, remove file with null size

        return  true;
        }

                                        // here, write failed
    if      ( IsFlag ( flags, CanOpenFileAskUser ) ) {

        getfile.SetTitle ( "File could not be written, please choose another file:" );

        if ( ! getfile.Execute ( file ) )

            return  false;
        else {
            StringCopy ( file, (char *) getfile );
            return  true;
            }
        }

    else if ( IsFlag ( flags, CanOpenFileVerbose ) )

        ShowMessage ( "Can not write to file!", file, ShowMessageWarning );


    return  false;
    } // CanOpenFileWrite


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // neither read or write - shouldn't happen
return  false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TFileName::TFileName ()
{
Reset ();
}


void   TFileName::Reset ()
{
ClearString ( FileName, TFilenameSize );
}


void   TFileName::Set ( const char* filename, TFilenameFlags flags )
{
Reset ();
                                        // make a safe copy, in a big enough fixed buffer
StringCopy ( FileName, filename, TFilenameSize - 1 );

CheckFileName ( flags );
}


void   TFileName::Set ( char* filename, TFilenameFlags flags )
{
Set ( (const char*) filename, flags );
}


void   TFileName::SetTempFileName ( const char* ext )
{
GetTempFilePath     ( FileName );

if ( StringIsNotEmpty ( ext ) )
    ReplaceExtension ( ext );
}


//----------------------------------------------------------------------------
void   TFileName::CheckFileName ( TFilenameFlags flags )
{
                                        // Check path with this preferred sequence:

                                        // can expand to a longer string - though is currently prevented to be longer than 256
if ( IsFlag ( flags, TFilenameMsDosToWindows ) )
    MsDosPathToWindowsPath ();

                                        // convert a relative path to an absolute one
if ( IsFlag ( flags, TFilenameAbsolutePath ) )
    CheckAbsolutePath ();

                                        // add special prefix for path longer than 256
if ( IsFlag ( flags, TFilenameExtendedPath ) )
    CheckExtendedPath ();

                                        // convert to another extension / sibling file for Cartool purpose (.img to .hdr f.ex.)
if ( IsFlag ( flags, TFilenameSibling ) )
    CheckSiblingFile ();

                                        // keeping only the directory part - trailing \ is removed
if ( IsFlag ( flags, TFilenameDirectory ) )
    RemoveFilename ( FileName, false );

                                        // avoiding overwrite by generating a variation of given file name
if ( IsFlag ( flags, TFilenameNoOverwrite ) )
    CheckNoOverwrite ();
}


//----------------------------------------------------------------------------
        TFileName::TFileName ( const TFileName &op )
{
StringCopy          ( FileName, op.FileName, TFilenameSize - 1 );
}


TFileName& TFileName::operator= ( const TFileName &op2 )
{
StringCopy          ( FileName, op2.FileName, TFilenameSize - 1 );

return  *this;
}


TFileName& TFileName::operator= ( const char* op2 )
{
StringCopy          ( FileName, op2, TFilenameSize - 1 );

return  *this;
}


//----------------------------------------------------------------------------
bool   TFileName::IsRelativePath  ()    const
{
return  PathIsRelative  ( FileName );   // !doc says up to MAX_PATH chars!
}


bool   TFileName::IsAbsolutePath  ()    const
{
return  ! IsRelativePath  (); 
}


bool   TFileName::IsExtendedPath  ()
{
return  crtl::IsExtendedPath ( FileName ); 
}


bool   TFileName::IsExtension ( const char* ext )   const
{
return  crtl::IsExtension ( FileName, ext ); 
}


void   TFileName::ResetExtendedPath ()
{
if ( IsExtendedPath () )
                                        // remove extended special chars
    StringClip ( FileName, StringLength ( ExtendedPathPrefix ), StringLength ( FileName ) - 1 );
}


void   TFileName::CheckExtendedPath ()
{
crtl::CheckExtendedPath ( FileName );
}


void   TFileName::MsDosPathToWindowsPath ()
{
crtl::MsDosPathToWindowsPath ( FileName );
}


void   TFileName::CheckAbsolutePath ()
{
crtl::CheckAbsolutePath ( FileName, TFilenameSize );
}


void   TFileName::CheckSiblingFile ()
{
crtl::CheckSiblingFile ( FileName );
}


void   TFileName::CheckNoOverwrite ()
{
crtl::CheckNoOverwrite ( FileName );
}


bool   TFileName::CanOpenFile ()    const
{
return  crtl::CanOpenFile ( FileName, CanOpenFileRead );
}


//----------------------------------------------------------------------------
void   TFileName::ClipFileName ( int from, int to )
{
StringClip ( (char *) ToFileName ( FileName ), from, to );
}


void   TFileName::Show ( const char* title )    const
{
char                localtitle[ 256 ];

StringCopy  ( localtitle, StringIsEmpty ( title ) ? "FileName" : title );

ShowMessage ( FileName, localtitle );
}


void   TFileName::Open ()
{
if ( IsEmpty () || ! CanOpenFile () )
    return;

CartoolObjects.CartoolDocManager->OpenDoc ( FileName, dtOpenOptions );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
