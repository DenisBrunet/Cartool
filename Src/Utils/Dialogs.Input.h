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

#include    <windows.h>                 // before <commdlg.h>
#include    <commdlg.h>                 // OFN_OVERWRITEPROMPT

#include    <owl/inputdia.h>            // TInputDialog

#include    "Files.TGoF.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrapping MessageBox
enum                {
                    ShowMessageNormal           = 0x00,
                    ShowMessageWarning          = 0x01,

                    ShowMessageDefault          = ShowMessageNormal,
                    };

void                ShowMessage             ( const char*        text, const char*        title = 0,  int flags = ShowMessageDefault, owl::TWindow *that = 0 );
void                ShowMessage             ( const std::string& text, const std::string& title = "", int flags = ShowMessageDefault, owl::TWindow *that = 0 );
void                ShowValues              ( const char *title, const char *format, ... ); // format is i:integer f:double float c:char s:string


//----------------------------------------------------------------------------
                                        // Quick and Dirty debugging messages - for more complex output, just use the full ShowValues function

                                        // Text only
#define             DBGM(S1,B)                      { ShowValues ( B, "s",          (S1) );    }
#define             DBGM2(S1,S2,B)                  { ShowValues ( B, "ss",         (S1), (S2) ); }
#define             DBGM3(S1,S2,S3,B)               { ShowValues ( B, "sss",        (S1), (S2), (S3) ); }
#define             DBGM4(S1,S2,S3,S4,B)            { ShowValues ( B, "ssss",       (S1), (S2), (S3), (S4) ); }
#define             DBGM5(S1,S2,S3,S4,S5,B)         { ShowValues ( B, "sssss",      (S1), (S2), (S3), (S4), (S5) ); }
#define             DBGM6(S1,S2,S3,S4,S5,S6,B)      { ShowValues ( B, "ssssss",     (S1), (S2), (S3), (S4), (S5), (S6) ); }

                                        // Double values only (integer will be converted and displayed correctly anyway)
#define             DBGV(V1,B)                      { ShowValues ( B, "f",          (double) (V1) );    }
#define             DBGV2(V1,V2,B)                  { ShowValues ( B, "ff",         (double) (V1),(double) (V2) );  }
#define             DBGV3(V1,V2,V3,B)               { ShowValues ( B, "fff",        (double) (V1),(double) (V2),(double) (V3) );    }
#define             DBGV4(V1,V2,V3,V4,B)            { ShowValues ( B, "ffff",       (double) (V1),(double) (V2),(double) (V3),(double) (V4) );  }
#define             DBGV5(V1,V2,V3,V4,V5,B)         { ShowValues ( B, "fffff",      (double) (V1),(double) (V2),(double) (V3),(double) (V4),(double) (V5) );    }
#define             DBGV6(V1,V2,V3,V4,V5,V6,B)      { ShowValues ( B, "ffffff",     (double) (V1),(double) (V2),(double) (V3),(double) (V4),(double) (V5),(double) (V6) );  }
#define             DBGV7(V1,V2,V3,V4,V5,V6,V7,B)   { ShowValues ( B, "fffffff",    (double) (V1),(double) (V2),(double) (V3),(double) (V4),(double) (V5),(double) (V6),(double) (V7) );    }
#define             DBGV8(V1,V2,V3,V4,V5,V6,V7,V8,B){ ShowValues ( B, "ffffffff",   (double) (V1),(double) (V2),(double) (V3),(double) (V4),(double) (V5),(double) (V6),(double) (V7),(double) (V8) );  }


//----------------------------------------------------------------------------
                                        // Wrapping TCTInputDialog / TInputDialog
enum                SkullStrippingType;
template <class>    class TArray1;


bool                GetAnswerFromUser       ( const char *text, const char *title, owl::TWindow *that = 0 );

bool                GetInputFromUser        ( const char *text, const char *title, char   *answer, const char *defaultanswer, owl::TWindow *that = 0 ); // answer will be modified, whatever if successful or not

bool                GetValueFromUser        ( const char *text, const char *title, double &answer, const char *defaultanswer, owl::TWindow *that = 0 ); // answer will be modified, whatever if successful or not
bool                GetValueFromUser        ( const char *text, const char *title, float  &answer, const char *defaultanswer, owl::TWindow *that = 0 ); // answer will be modified, whatever if successful or not
bool                GetValueFromUser        ( const char *text, const char *title, long   &answer, const char *defaultanswer, owl::TWindow *that = 0 ); // answer will be modified, whatever if successful or not
bool                GetValueFromUser        ( const char *text, const char *title, int    &answer, const char *defaultanswer, owl::TWindow *that = 0 ); // answer will be modified, whatever if successful or not
bool                GetValuesFromUser       ( const char *text, const char *title, double* answers, int numanswers, const char *defaultanswers, owl::TWindow *that = 0 );
int                 GetValuesFromUser       ( const char *text, const char *title, TArray1<int>&    answers, const char *defaultanswers, owl::TWindow *that = 0 );
int                 GetValuesFromUser       ( const char *text, const char *title, TArray1<double>& answers, const char *defaultanswers, owl::TWindow *that = 0 );

int                 GetResultTypeFromUser   ( const char *title, const char *defaultanswer, owl::TWindow *that = 0 );   // can be: Positive / Negative / Signed / Absolute
SkullStrippingType  GetSkullStrippingTypeFromUser   ( const char *title, const char *defaultanswer, owl::TWindow *that = 0 );

char                GetOptionFromUser       ( const char *text, const char *title, const char *answers, const char *defaultanswer,               owl::TWindow *that = 0 );
bool                GetOptionsFromUser      ( const char *text, const char *title, const char *answers, const char *defaultanswer, char *answer, owl::TWindow *that = 0 );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Deriving from owl::TInputDialog so as to be able to adjust its height according
                                        // to the length of text to be shown

class   TCTInputDialog  : public owl::TInputDialog 
{
public:
                                        // none seem to be virtual(?)
                    TCTInputDialog( owl::TWindow*   parent,
                                    const char*     title,
                                    const char*     prompt,
                                    char*           buffer,
                                    int             bufferSize,
//                                  owl::TResId     resId,
//                                  int             numlines,
                                    owl::TModule*   module = 0,
                                    owl::TValidator*valid = 0   );

                   ~TCTInputDialog ();
};


//----------------------------------------------------------------------------

enum                GetFileFlags
                    {
                    UnknownGetFile     = 0,
                    GetFileRead,
                    GetFileWrite,       // if no extension returned, will add the first extension of current file filter
                    GetFilePath,        // incomplete file name allowed
                    GetFileMulti,       // implies read only
                    GetFileDirectory,   // open 1 directory (uses another dialog), then expanded as ALL files + filter
                    };

                                        // Stores data needed to get a single file or a set of files
                                        // Encapsulates the calls to dialog
class   GetFileFromUser
{
public:
                    GetFileFromUser ();
                    GetFileFromUser ( const char *title, const char *filefilter, int filefilterindex, GetFileFlags flags );


    void            Reset ()                                    { GoF.Reset (); }

    bool            IsSingleFile ()                     const   { return ! IsMultiFiles ();                  }
    bool            IsMultiFiles ()                     const   { return Flags == GetFileMulti || Flags == GetFileDirectory; }
    bool            IsWriting    ()                     const   { return OFNDialogFlags & OFN_OVERWRITEPROMPT;  }


    int             GetNumFiles ()                      const   { return GoF.NumFiles (); }
    void            SetFileFilterIndex ( DWORD index )          { OFNDialogFilterIndex  = index; }  // no boundary check, which actually depends on the file filter string
    int             GetFileFilterIndex ()               const   { return OFNDialogFilterIndex; }


    const char*     GetFile ( int index = 0 )           const   { return (bool) GoF ? IsSingleFile () ? GoF[ 0 ]
                                                                                                      : GoF[ Clip ( index, 0, GetNumFiles () - 1 ) ]
                                                                                    :                   ""; }
    char*           GetFile ( int index = 0 )                   { return (bool) GoF ? IsSingleFile () ? GoF[ 0 ]
                                                                                                      : GoF[ Clip ( index, 0, GetNumFiles () - 1 ) ]
                                                                                    :                   ""; }

    void            SetOnly      ( const char* file )           { GoF.SetOnly ( file ); }
    void            SetTitle     ( const char* title );

    bool            Execute ( const char *fileinit );
    bool            Execute ( char       *fileinit = 0 );


    void            Show ( const char* title = 0 )      const   { GoF.Show ( title ); }


    const char*     operator    []              ( int index )   const   { return GetFile ( index ); }
    char*           operator    []              ( int index )           { return GetFile ( index ); }

                    operator    int             ()    /*const*/ { return (int)  GoF; }
                    operator    bool            ()      const   { return (bool) GoF; }
                    operator    const char*     ()      const   { return GetFile ( 0 ); }
                    operator    char*           ()              { return GetFile ( 0 ); }
                    operator    const TGoF*     ()      const   { return &GoF; }
                    operator    TGoF*           ()              { return &GoF; }
                    operator    const TGoF&     ()      const   { return GoF; }
                    operator    TGoF&           ()              { return GoF; }


protected:
    TGoF            GoF;
    GetFileFlags    Flags;

                                        // OpenFileName dialog parameters
    char            OFNDialogTitle [  256 ];
    char            OFNDialogFilter[ 1024 ];
    DWORD           OFNDialogFilterIndex;
    DWORD           OFNDialogFlags;


    void            SetFileFilter           ( const char *filefilter );
    void            CheckMissingExtension   ( char *file );
//  void            FileFiltersToGrep       ( char *allexts );
    bool            OpenFileName            ( const char *title, char *file, int filesize, const char *filefilter, DWORD &filefilterindex, DWORD flags, HWND hwnd );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
