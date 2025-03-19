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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Dialogs.Input.h"           // first include

#include    <shlobj.h>                  // SHBrowseForFolder
#include    <stdarg.h>                  // variable number of arguments

#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "TVolume.h"

#include    "resource.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Adaptative height
        TCTInputDialog::TCTInputDialog( TWindow*        parent,
                                        const char*     title,
                                        const char*     prompt,
                                        char*           buffer,
                                        int             bufferSize,
//                                      TResId          resId,
//                                      int             numlines,
                                        TModule*        module,
                                        TValidator*     validator   )

      : TInputDialog ( parent, title, prompt, buffer, bufferSize, module, validator )
{
DialogAttr.Param    = 0;
//DialogAttr.Name     = resId;


int                 numlines        = StringNumLines ( prompt );

                                        // picking resources with appropriate number of lines
DialogAttr.Name     = (LPTSTR) UIntToPtr ( numlines <= 3  ?  IDD_INPUTDIALOG3
                                         : numlines <= 6  ?  IDD_INPUTDIALOG6
                                         :                   IDD_INPUTDIALOG9 );
}


        TCTInputDialog::~TCTInputDialog()
{
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void        ShowMessage ( const char* text, const char* title, int flags, TWindow *that )
{
MessageBox  (   that ? that->GetHandle () : 0,
                text,
                title ? title : "",
                MB_SETFOREGROUND | MB_OK | MB_TOPMOST | ( flags & ShowMessageWarning ? MB_ICONEXCLAMATION : 0 ) 
            );
}


void        ShowMessage ( const std::string& text, const std::string& title, int flags, TWindow *that )
{
MessageBox  (   that ? that->GetHandle () : 0,
                text .c_str (),
                title.c_str (),
                MB_SETFOREGROUND | MB_OK | MB_TOPMOST | ( flags & ShowMessageWarning ? MB_ICONEXCLAMATION : 0 ) 
            );
}


//----------------------------------------------------------------------------
void        ShowValues  ( const char *title, const char* format, ... )
{
va_list             vl;                 // actually a char*
char                buff [ 1024 ];


ClearString ( buff );


va_start ( vl, format );
                                        // stepping through the format list
for ( int fi = 0; format[ fi ] != EOS; fi++ ) {

    AppendSeparator ( buff, ", " );

    switch( format[ fi ] ) {            // type format

        case 'i':
            IntegerToString ( StringEnd ( buff ),   va_arg ( vl, int ) );
            break;

        case 'f':
            FloatToString   ( StringEnd ( buff ),   va_arg ( vl, double ) );
            break;

        case 'c':
            strncat         ( buff,                &va_arg ( vl, char ), 1 );
            break;

        case 's':
            StringAppend    ( buff,                 va_arg ( vl, char* ) );
            break;

        default: // skipping unknown formats for the moment
            break;
        }
    }

va_end ( vl );


ShowMessage ( buff, title );
}


//----------------------------------------------------------------------------
bool        GetAnswerFromUser ( const char *text, const char *title, TWindow *that )
{
return  MessageBox  (   that ? that->GetHandle () : 0,
                        text,
                        title,
                        MB_SETFOREGROUND | MB_YESNO 
                    ) == IDYES;
}


//----------------------------------------------------------------------------
bool        GetInputFromUser ( const char *text, const char *title, char *answer, const char *defaultanswer, TWindow *that )
{
                                        // use local buffer of known size
char                buff   [ 4 * KiloByte ];

if ( StringIsEmpty ( defaultanswer ) )  ClearString ( buff );
else                                    StringCopy  ( buff, defaultanswer );

                                        // This dialog will adapt to the amount of lines to be shown
                                        // needless to say that the answer parameter should be big enough!
                                        // OWlNext does not appreciate a null module pointer when running unit tests - let's be explicit
bool                successful      = TCTInputDialog ( that, title, text, buff, sizeof ( buff ), &TModule ( "nullmodule", GetModuleHandle (NULL) ) ).Execute () == IDOK;

StringCopy ( answer, buff );

return  successful;
}


//----------------------------------------------------------------------------
bool        GetValueFromUser ( const char *text, const char *title, double &answer, const char *defaultanswer, TWindow *that )
{
answer      = 0;

char                buff [ 256 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswer, that ) )
    return  false;

answer      = StringToDouble ( buff );

return  true;
}


bool        GetValueFromUser ( const char *text, const char *title, float &answer, const char *defaultanswer, TWindow *that )
{
answer      = 0;

char                buff [ 256 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswer, that ) )
    return  false;

answer      = StringToFloat ( buff );

return  true;
}


bool        GetValueFromUser ( const char *text, const char *title, long &answer, const char *defaultanswer, TWindow *that )
{
answer      = 0;

char                buff [ 256 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswer, that ) )
    return  false;

answer      = StringToLong ( buff );

return  true;
}


bool        GetValueFromUser ( const char *text, const char *title, int &answer, const char *defaultanswer, TWindow *that )
{
answer      = 0;

char                buff [ 256 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswer, that ) )
    return  false;

answer      = StringToInteger ( buff );

return  true;
}


//----------------------------------------------------------------------------
                                        // Predetermined number of values
bool        GetValuesFromUser ( const char *text, const char *title, double* answers, int numanswers, const char *defaultanswers, TWindow *that )
{
if ( answers == 0 || numanswers <= 0 )  
    return  false;

                                        // reset answers
for ( int i = 0; i < numanswers; i++ )
    answers[ i ]    = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff [ 1024 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswers, that ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSplitStrings       split ( buff, NonUniqueStrings );

                                        // not enough answers -> fail - too much is OK-ish
if ( split.GetNumTokens () < numanswers )
    return  false;

                                        // just copy what we need, ignore remaining answers, if any
for ( int i = 0; i < numanswers; i++ )

    answers[ i ]    = StringToDouble ( split[ i ] );


return  true;
}


//----------------------------------------------------------------------------
                                        // Any number of values, but at least 1
int         GetValuesFromUser ( const char *text, const char *title, TArray1<int>& answers, const char *defaultanswers, TWindow *that )
{
                                        // reset answers
answers.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff [ 1024 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswers, that ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSplitStrings       split ( buff, NonUniqueStrings );
int                 numanswers      = split.GetNumTokens ();

                                        // no answers means failing
if ( numanswers == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

answers.Resize ( numanswers );

                                        // just copy what we need, ignore remaining answers, if any
for ( int i = 0; i < numanswers; i++ )

    answers[ i ]    = StringToInteger ( split[ i ] );


return  (int) answers;
}


int         GetValuesFromUser ( const char *text, const char *title, TArray1<double>& answers, const char *defaultanswers, TWindow *that )
{
                                        // reset answers
answers.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff [ 1024 ];

if ( ! GetInputFromUser ( text, title, buff, defaultanswers, that ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSplitStrings       split ( buff, NonUniqueStrings );
int                 numanswers      = split.GetNumTokens ();

                                        // no answers means failing
if ( numanswers == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

answers.Resize ( numanswers );

                                        // just copy what we need, ignore remaining answers, if any
for ( int i = 0; i < numanswers; i++ )

    answers[ i ]    = StringToDouble ( split[ i ] );


return  (int) answers;
}


//----------------------------------------------------------------------------
int         GetResultTypeFromUser ( const char *title, const char *defaultanswer, TWindow *that )
{
char                answer          = GetOptionFromUser ( "(+)(P)ositive, (-)(N)egative, (*)(S)igned or (0)(A)bsolute results:", 
                                                          title, "P + N - S * A 0", defaultanswer, that );

if ( answer == EOS )   return -1;


return  answer == 'P' || answer == '+' ? FilterResultPositive
      : answer == 'N' || answer == '-' ? FilterResultNegative
      : answer == 'S' || answer == '*' ? FilterResultSigned
      : answer == 'A' || answer == '0' ? FilterResultAbsolute
      :                                  FilterResultSigned;
}


//----------------------------------------------------------------------------
                                        // All actual options are: 0->1A; 1->1B; 2->2; 3->3
SkullStrippingType  GetSkullStrippingTypeFromUser ( const char *title, const char *defaultanswer, TWindow *that )
{
int                 ssi;

if ( ! GetValueFromUser ( "Skull-Stripping method:" NewLine
                          NewLine
                          Tab "1) Multiple Region Growing - Recommended"            NewLine
                          Tab "2) Single Region Growing   - If 1) failed"           NewLine
                          Tab "3) Multiple Masking           - If 1) and 2) failed", 
                          title, ssi, defaultanswer, that ) )

    return  SkullStrippingNone;


return  IsInsideLimits ( ssi, /*1*/ 0, 3 ) ? (SkullStrippingType) ( SkullStripping1B + ssi - 1 ) : SkullStrippingNone;
}


//----------------------------------------------------------------------------
                                        // returns a single char for answer
                                        // allowed chars should be separated by spaces, like "A B C", and in upper case
                                        // No answers allowed in the following chars:  ' '\t\n;,
char        GetOptionFromUser ( const char *text, const char *title, const char *answers, const char *defaultanswer, TWindow *that )
{
char                answer [ 256 ];

ClearString         ( answer );


if ( ! GetInputFromUser ( text, title, answer, defaultanswer, that ) )
    return  EOS;


StringCleanup       ( answer );

StringToUppercase   ( answer );

                                        // answering is mandatory
if ( ! IsStringAmong ( answer, answers ) )
    return  EOS;


return  answer[ 0 ];
}

                                        // returns a full string, to be copied & tested
                                        // allowed chars could be (not mandatory) separated by spaces, like "A B C", and in upper case
bool        GetOptionsFromUser ( const char *text, const char *title, const char *answers, const char *defaultanswer, char *answer, TWindow *that )
{
if ( answer == 0 )
    return  false;

ClearString         ( answer );


if ( ! GetInputFromUser ( text, title, answer, defaultanswer, that ) )
    return  EOS;


StringToUppercase   ( answer );

KeepChars           ( answer, answers );    // will replace illegal chars with space

StringNoSpace       ( answer );             // now compact spaces

                            
return  StringIsNotEmpty ( answer );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
            GetFileFromUser::GetFileFromUser ()
{
Flags                   = GetFileRead;

ClearString ( OFNDialogTitle  );
ClearString ( OFNDialogFilter );
OFNDialogFilterIndex    = 1;
OFNDialogFlags          = OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}


//----------------------------------------------------------------------------
            GetFileFromUser::GetFileFromUser ( const char *title, const char *filefilter, int filefilterindex, GetFileFlags flags )
{
                                        // save for later
Flags                   = flags;


if ( StringIsNotEmpty ( title ) )

    StringCopy ( OFNDialogTitle, title );

else
    if      ( Flags == GetFileRead      )   StringCopy  ( OFNDialogTitle, "Open File"               ); 
    else if ( Flags == GetFileWrite     )   StringCopy  ( OFNDialogTitle, "Save File"               );
    else if ( Flags == GetFilePath      )   StringCopy  ( OFNDialogTitle, "Get Path"                );
    else if ( Flags == GetFileMulti     )   StringCopy  ( OFNDialogTitle, "Open Multiple Files"     );
    else if ( Flags == GetFileDirectory )   StringCopy  ( OFNDialogTitle, "Open Directory"          );
    else                                    ClearString ( OFNDialogTitle );


SetFileFilter ( filefilter );

OFNDialogFilterIndex    = filefilterindex;

                                        // flags for all cases
OFNDialogFlags          = OFN_EXPLORER | OFN_LONGNAMES;

if      ( Flags == GetFileRead          )   OFNDialogFlags |= OFN_PATHMUSTEXIST      | OFN_FILEMUSTEXIST;
else if ( Flags == GetFileWrite         )   OFNDialogFlags |= OFN_EXTENSIONDIFFERENT | OFN_OVERWRITEPROMPT;
else if ( Flags == GetFilePath          )   OFNDialogFlags |= 0;
else if ( Flags == GetFileMulti         )   OFNDialogFlags |= OFN_PATHMUSTEXIST      | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
else if ( Flags == GetFileDirectory     )   OFNDialogFlags |= 0;
else                                        OFNDialogFlags |= OFN_PATHMUSTEXIST      | OFN_FILEMUSTEXIST;
}


//----------------------------------------------------------------------------
void    GetFileFromUser::SetTitle ( const char* title )
{
StringCopy ( OFNDialogTitle, StringIsEmpty ( title ) ? "" : title );
}


//----------------------------------------------------------------------------
void    GetFileFromUser::SetFileFilter ( const char *filefilter )
{
if ( StringIsEmpty ( filefilter ) )
                                        // provide default file filter
    StringCopy ( OFNDialogFilter, "All files\0*.*\0" );

else {
    StringCopy ( OFNDialogFilter, filefilter );
                                        // always complete with a general file filter
    if ( ! StringContains ( (const char*) OFNDialogFilter, FILEFILTER_ALL, CaseSensitive ) )
        StringAppend ( OFNDialogFilter, "|" AllFilesFilter );
    }

                                        // needs to convert from old style of file filter?
if ( ! StringContains ( (const char*) OFNDialogFilter, "|", CaseSensitive ) )
    return;
                                        // convert from old to new (>=XP) file filter style
                                        // now separated by nulls, & ending with one more null
int                 len             = StringLength ( OFNDialogFilter );

for ( int i = 0; i < len; i++ )
    if ( OFNDialogFilter[ i ] == '|' )
        OFNDialogFilter[ i ] = 0;

OFNDialogFilter[ len     ] = 0;
OFNDialogFilter[ len + 1 ] = 0;
}


//----------------------------------------------------------------------------
                                        // This callback allows us to initialize SHBrowseForFolder with an initial directory
static int CALLBACK     SHBrowseForFolderCallbackProc ( HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData )
{
switch ( uMsg ) {
                                        // If the BFFM_INITIALIZED message is received -> set the path to the start path.
	case    BFFM_INITIALIZED:
		if ( lpData != NULL )
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    }

                                        // The function should always return 0.
return  0; 
}


//----------------------------------------------------------------------------
bool    GetFileFromUser::Execute ( const char *fileinit )
{
GoF.Reset ();


int                 filesize        = 1 * MegaByte;             // old limit was 32K for multiple files, but using a bigger buffer seems OK to hold a lot of files
char*               files           = new char [ filesize ];

                                        // smartly pre-fill the dialog
if ( IsSingleFile () || Flags == GetFileDirectory ) {
    if   ( StringIsEmpty ( fileinit ) )     StringCopy ( files, GetFile () );   // recall stored string
    else                                    StringCopy ( files, fileinit   );   // use parameter string
    }
else                                        ClearString ( files );              // IsMultiFiles (), no pre-fill


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                  // to select a directory, we have to use  SHBrowseForFolder
if ( Flags == GetFileDirectory )  {

    static TFileName    lastdirpath;    // local storage of last selected directory
    TFileName           tempdir;
    TFileName           dirpath;
    TFileName           filetempl;
    char                allexts     [ 1024 ];
                                        // for SHBrowseForFolder
    BROWSEINFO          bi              = { 0 };
    LPITEMIDLIST        pidl;
    IMalloc*            imalloc         = 0;


    bi.hwndOwner    = 0;
    bi.ulFlags      = BIF_RETURNONLYFSDIRS      // hide control panel, recycle bin...
//                  | BIF_BROWSEINCLUDEFILES    // also showing the files
                    | BIF_USENEWUI;             // resizeable dialog
    bi.lpszTitle    = OFNDialogTitle;
    bi.lpfn         = SHBrowseForFolderCallbackProc;

                                        // set starting directory
    if ( lastdirpath.IsEmpty () )

        if ( IsDirectory ( files ) )
            bi.lParam       = (LPARAM) files;   // file given is already a directory
        else {
            StringCopy      ( tempdir, files );
            RemoveFilename  ( tempdir );
            bi.lParam       = (LPARAM) tempdir.FileName; // extract directory from file
            }
    else
        bi.lParam       = (LPARAM) lastdirpath.FileName; // favor the last directory

                                        // run the special dialog for directory (thanks Microsoft)
    if ( ( pidl = SHBrowseForFolder ( &bi ) ) == 0 )
        return  false;

                                        // extract the path from the answer (thanks again Microsoft)
    if ( ! SHGetPathFromIDList ( pidl, dirpath ) )
        dirpath.Reset ();

//    GoF.Add ( dirpath );

                                        // free memory (did I thanked you Microsoft?)
    if ( SUCCEEDED ( SHGetMalloc ( &imalloc ) ) ) {
        imalloc->Free    ( pidl );
        imalloc->Release ();
        }


    if ( dirpath.IsEmpty () )
        return  false;

                                        // locally remember where we are for next call
    lastdirpath     = dirpath;

    GoF.SetOnly ( dirpath );

                                        // ?What is this already?
//                                      // transform dialog filter to Grep filter
//  FileFiltersToGrep ( allexts );
//                                      // grep any extension at the end
//  StringCopy  ( filetempl, allexts, "$" );
//                                      // get only the type of files from selected directory
//  GoF.GrepFiles   ( dirpath, filetempl );
    } // open directory


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else { // open file(s)

    if ( ! OpenFileName ( OFNDialogTitle, files, filesize, OFNDialogFilter, OFNDialogFilterIndex, OFNDialogFlags, 0 ) )
        return  false;


    TFileName           buff;
    char               *in              = files;
    char               *todir           = files;

                                            // !All file strings will have at a minimal allocated memory, to allow expansions & concatenations!

    if ( IsDirectory ( in ) ) {
                                            // concat directory + each of the file names
        for ( char *toc = StringEnd ( in ) + 1; *toc != 0; toc += StringSize ( toc ) ) {

            if ( *LastChar ( todir ) == '\\' )  StringCopy ( buff, todir,       toc );
            else                                StringCopy ( buff, todir, "\\", toc );

            GoF.Add ( buff );
            } // for toc
        } // if isdir
    else {                                  // single file

        if ( IsWriting () )
            CheckMissingExtension ( files );

        GoF.SetOnly ( files );
        }

    } // open file(s)


delete[]  files;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Windows always mess-up the order!
GoF.Sort ();

//GoF.Show ( "GetFileFromUser" );

return  true;
}


//----------------------------------------------------------------------------
bool    GetFileFromUser::Execute ( char *fileinit )
{
bool                returnvalue     = Execute ( (const char *) fileinit );

                                        // complimentary copy a single answer, if not empty
if ( IsSingleFile () && fileinit != 0 && StringIsNotEmpty ( GetFile () ) )
    StringCopy ( fileinit, GetFile () );


return  returnvalue;
}


//----------------------------------------------------------------------------
/*                                        // transform dialog filter to Grep filter
void    GetFileFromUser::FileFiltersToGrep ( char *allexts )
{
TSplitStrings       splitsemicol;
char               *toc             = OFNDialogFilter + StringSize ( OFNDialogFilter ); // skip first filter name


ClearString ( allexts );

                                        // browse all groups of preset
while ( StringIsNotEmpty ( toc ) ) {

    splitsemicol.Reset  ();
    splitsemicol.Set    ( toc, NonUniqueStrings, ";" );

                                        // add each extension from current preset
    for ( int ssi = 0; ssi < (int) splitsemicol; ssi++ )

        if ( StringIsNot      ( splitsemicol[ ssi ], FILEFILTER_ALL )
          && StringStartsWith ( splitsemicol[ ssi ], "*." )           )

            StringAppend ( allexts, "|", splitsemicol[ ssi ] + 2 );


    toc += StringSize  ( toc );             // skip current group of wildchars
    if ( StringIsEmpty ( toc ) )    break;  // end of all groups?
    toc += StringSize  ( toc );             // skip next filter name
    }

                                        // final cooking, put everything in rounded brackets
allexts[ 0 ]    = '(';
StringAppend ( allexts, ")" );

//DBGM ( allexts, "allexts" );
}
*/

//----------------------------------------------------------------------------
// Bypasses Windows behavior, which does not officially allow for extensions longer than 3 chars (!)
// This also allows one "default extension" per file filter, the first specified
void    GetFileFromUser::CheckMissingExtension ( char *file )
{
                                        // ending with a "." means: don't interfere!
if ( *LastChar ( file ) == '.' ) {
    *LastChar ( file ) = 0;             // remove the "."
    return;
    }


char               *toc             = OFNDialogFilter;
char                firstext[ 256 ];
char                allexts [ 256 ];

ClearString ( firstext );

                                        // loop until we reach the correct file filter combo
for ( int ffi = OFNDialogFilterIndex - 1; ffi > 0 && *toc != 0; ffi-- ) {
    toc += StringSize ( toc );          // skip filter name
    toc += StringSize ( toc );          // skip filters
    }

if ( StringIsEmpty ( toc ) )
    return;


toc += StringSize ( toc );              // skip filter name

if ( StringIsEmpty ( toc ) )
    return;


                                        // convert to a string containing all the available extensions
StringCopy ( allexts, toc );

StringReplace ( allexts, "*.", " " );
StringReplace ( allexts, ";",  " " );

if ( StringIsSpace  ( allexts )
  || StringContains ( (const char*) allexts, "*", CaseSensitive ) )  // included an "*.*"
    return;

//DBGM ( allexts, "All Extensions" );

                                        // skip begining with "*." or "." or "*xxx"
if ( *toc == '*' )  toc++;
if ( *toc == '.' )  toc++;


                                        // process the first extension
StringCopy ( firstext, toc );

                                        // trim trailing chars after first ";..."
if ( ( toc = StringContains ( (char*) firstext, (char*) ";", CaseSensitive ) ) != 0 )
    ClearString ( toc );

                                        // don't allow for a remaining '*' or '.'
if ( StringContains ( (const char*) firstext, "*", CaseSensitive )
  || StringStartsWith ( firstext, "." ) )
    ClearString ( firstext );

//DBGM ( firstext, "First Ext" );

if ( StringIsSpace ( firstext ) )
    return;


                                        // finally, if the extension is not legal, add the first one
if ( ! IsExtensionAmong ( file, allexts ) ) {
    StringAppend ( file, "." );
    StringAppend ( file, firstext );
//    DBGM ( file, "Corrected file" );
    }
}


//----------------------------------------------------------------------------
                                        // Wrap that ugly call to GetOpenFileName
bool    GetFileFromUser::OpenFileName ( const char *title, char *file, int filesize, const char *filefilter, DWORD &filefilterindex, DWORD flags, HWND hwnd )
{
OPENFILENAME        ofn;                // common dialog box structure

ZeroMemory ( &ofn, sizeof ( ofn ) );
ofn.lStructSize     = sizeof ( ofn );   // OPENFILENAME_SIZE_VERSION_400 ?


ofn.lpstrTitle      = title;
ofn.lpstrFile       = file;             // !file is not const char* !
ofn.nMaxFile        = filesize;
ofn.lpstrFilter     = filefilter;
ofn.nFilterIndex    = filefilterindex;
ofn.Flags           = flags;
ofn.hwndOwner       = hwnd;             // owner window, or 0

                                        // run the Save or the Open dialog box according to desired operation
bool                result          = IsWriting () ? GetSaveFileName ( &ofn ) == TRUE
                                                   : GetOpenFileName ( &ofn ) == TRUE;


//DWORD               error           = CommDlgExtendedError (); // see  CDERR_DIALOGFAILURE
//if ( ! result )     DBGV ( error, "Error in GetOpenFileName" );

                                        // save actual index
filefilterindex     = ofn.nFilterIndex;

return  result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
