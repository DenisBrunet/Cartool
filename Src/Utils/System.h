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

#include    <omp.h>
#include    <iostream>

#include    <owl/registry.h>
#include    <owl/clipboar.h>

#include    "System.OpenMP.h"
#include    "Time.TAcceleration.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Physical, real-time, direct access to keyboard:
                                        // It totally BYPASSES THE MESSAGING SYSTEM, and comes in handy to check windows consistency when switching between them
                                        // It is however time-consuming (the real-tim thingy) and breaks the application messaging system - so use it with care
inline bool VkKey ( UCHAR K )       { return GetAsyncKeyState ( K ) & 0x8000; }

                                        // A few useful predefined keyboard keys
inline bool VkShift     ()          { return VkKey ( VK_SHIFT   ); }
inline bool VkControl   ()          { return VkKey ( VK_CONTROL ); }
inline bool VkMenu      ()          { return VkKey ( VK_MENU    ); }
inline bool VkNoAlts    ()          { return ! ( VkShift () || VkControl () || VkMenu () ); }
inline bool VkEscape    ()          { return VkKey ( VK_ESCAPE  ); }
inline bool VkQuery     ()          { return VkKey ('Q');          }

                                        // A few useful predefined mouse buttons
inline bool VkLButton   ()          { return VkKey ( VK_LBUTTON ); }
inline bool VkMButton   ()          { return VkKey ( VK_MBUTTON ); }
inline bool VkRButton   ()          { return VkKey ( VK_RBUTTON ); }
inline bool VkLMRButton ()          { return VkLButton () || VkMButton () || VkRButton (); }


//----------------------------------------------------------------------------
                                        // Class to directly peek into the WHOLE keyboard at once, and without any window context at all

                                        // defined by Windows
constexpr int       KeyboardSize    = 256;

                                        // embeds an acceleration class, so the caller can get a sense of time between key pressed
class  TKeyboard    :   public TAcceleration
{
public:
                    TKeyboard ();

                                        // current keyboard state
    UCHAR           KeyboardState[ KeyboardSize ];


    void            Reset ();


    void            GetState        ();             // call this once, then can test state multiple times

    bool            IsPressed       ( int vki )     const;
    bool            IsReleased      ( int vki )     const;
    bool            IsToggled       ( int vki )     const;
    bool            IsAnyKeyChange  ()              const;  // across the whole keyboard


    void            Show ( const char *title = 0 )  const;


                    operator    int     ()          const   { return    KeyboardSize; }
    bool            operator    []      ( int vki ) const   { return    IsPressed ( vki ); }


protected:
                                        // previous keyboard state
    UCHAR           PreviousKeyboardState[ KeyboardSize ];

    bool            IsPressed       ( const UCHAR* keyboardstate, int vki ) const;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // wrapper to show the last Windows error
void    ShowLastError ();


//----------------------------------------------------------------------------

enum    ThreadSafety
        {
        ThreadSafetyIgnore,             // ignore / bypass multi-threads checks
        ThreadSafetyCare,               // do care for multi-threads checks
        };


enum    ProcessPriorityFlags
        {
        ProcessPriority0,               // lower to higher
        ProcessPriority1,
        ProcessPriority2,
        ProcessPriority3,
        ProcessPriority4,
        ProcessPriority5,

        NumProcessPriorities,

        LowPriority             = ProcessPriority0,
        BelowNormalPriority     = ProcessPriority1,
        NormalPriority          = ProcessPriority2,
        AboveNormalPriority     = ProcessPriority3,
        HighPriority            = ProcessPriority4,
        RealTimePriority        = ProcessPriority5,     // not recommended

        DefaultPriority         = NormalPriority,       // Default Windows
        BatchProcessingPriority = BelowNormalPriority,  // We can lower priority so computer can remain responsive, or other Cartool can work in parallel
        };


void    SetProcessPriority  ( ProcessPriorityFlags how = DefaultPriority );


//----------------------------------------------------------------------------

bool    CreateConsole       ();
bool    DeleteConsole       ( bool showpresskey = false );
bool    HasConsole          ();
void    PrintConsole        ( const string& message );
void    ConsoleErrorMessage ( const char* option, const char* m1, const char* m2 = 0, const char* m3 = 0, const char* m4 = 0, const char* m5 = 0 );


//----------------------------------------------------------------------------
                                        // long long conversion, the correct way
inline  LARGE_INTEGER       To_LARGE_INTEGER ( LONGLONG pos )
{
LARGE_INTEGER       li;

li.QuadPart         = pos;

return  li;
}

inline  LARGE_INTEGER       To_LARGE_INTEGER ( long pos )
{
LARGE_INTEGER       li;

li.HighPart         = 0;
li.LowPart          = pos;

return  li;
}
                                        // and the dirty way, directly casting the long long to the matching union
inline const LARGE_INTEGER& LONGLONG_to_LARGE_INTEGER   ( const LONGLONG&      ll )     { return  *( (LARGE_INTEGER*) (&ll) ); }

inline const LONGLONG&      LARGE_INTEGER_to_LONGLONG   ( const LARGE_INTEGER& li )     { return  li.QuadPart;                 }

                                        // Extracting the low and high ulong's from a 64 bits pointer - Inspired from LOWORD / HIWORD macros
inline ULONG            LOULONG             ( const void* ll )      { return (ULONG) ( (   (ULONG_PTR)(ll) )         & 0xffffffff ); }
inline ULONG            HIULONG             ( const void* ll )      { return (ULONG) ( ( ( (ULONG_PTR)(ll) ) >> 32 ) & 0xffffffff ); }

                                        // Converting 2 ULONG / DWORD to ULONGLONG / size_t / unsigned int64
inline size_t           DWORDs_to_size_t    ( const DWORD& hdw, const DWORD& ldw )  { return ( ( (size_t) (hdw) ) << 32 ) | ( ( (size_t) (ldw) ) & 0xffffffff ); }

                                        // Handy expression
constexpr size_t        size_t_max          = (size_t) -1;


//----------------------------------------------------------------------------
                                        // Turns all optimizations (g, s/t, y) off
#define OptimizeOff         __pragma( optimize ( "", off ) )
                                        // Restores project's current optimization flags /O..
#define OptimizeOn          __pragma( optimize ( "", on  ) )

                                        // Surround structs / classes that need byte-granularity packing with these:
#define BeginBytePacking    __pragma( pack ( push, cartoolpack, 1 ) )

#define EndBytePacking      __pragma( pack ( pop,  cartoolpack    ) )


//----------------------------------------------------------------------------

inline  bool    IsMagicNumber   ( const char* totest, const char* magicnumber )
{
return  _strnicmp ( totest, magicnumber, 4 ) == 0;
}


inline  bool    IsMagicNumber   ( UINT32 totest, UINT32 magicnumber )
{
return  IsMagicNumber   ( (const char*) &totest, (const char*) &magicnumber );
}


inline  bool    IsMagicNumber   ( const char* totest, UINT32 magicnumber )
{
return  IsMagicNumber   ( totest, (const char*) &magicnumber );
}


//----------------------------------------------------------------------------
                                        // Bumping into the next state, forward or backward
                                        // As an extra check, the number of states should also be of same type as input state
template <typename TypeD>
inline  TypeD   NextState ( TypeD state, TypeD numstates, bool backward = false )
{
if ( backward )     return  (TypeD) ( ( state + numstates - 1 ) % numstates );
else                return  (TypeD) ( ( state +             1 ) % numstates );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TFileName;

bool    GetEnvironmentVariableEx ( const char* variable, TFileName& result );


//----------------------------------------------------------------------------
                                        // Registry & preferences stuff
constexpr int   RegistryMaxKeyLength    = 1 * KiloByte;
constexpr int   RegistryMaxVarLength    = 1 * KiloByte;
constexpr int   RegistryMaxDataLength   = 1 * KiloByte;

                                        // Wrapper functions
bool    NukeKey         (   owl::TRegKey&           hive,
                            const char*             keyname
                        );
                                        // Re-create the old QueryDefValue function
bool    QueryDefValue   (   const owl::TRegKey&     hive,
                            const char*             keyname, 
                            char*                   data 
                        );

bool    SetDefValue     (   const owl::TRegKey&     hive,
                            const char*             keyname, 
                            const char*             data 
                        );

bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            char*                   data 
                        );
bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            DWORD&                  data 
                        );
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            const char*             data
                        );
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            DWORD                   data
                        );
bool    DeleteValue     (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname
                        );

                                        // More wrappers for:
//TRegKey::GetSubkeyCount();
//TRegKey::EnumKey (..);


//----------------------------------------------------------------------------

class   TPreferences
{
public:
                    TPreferences        ();
                    TPreferences        ( const owl::TRegKey& hive, const char *keynamestart );

    bool            IsOpen              ()  const       { return  Hive != 0; }
    bool            IsNotOpen           ()  const       { return  Hive == 0; }

    void            Reset               ();

    char*           GetPreference       ( const char *keynameend, const char *varname, char   *data );
    void            SetPreference       ( const char *keynameend, const char *varname, char   *data );
    DWORD           GetPreference       ( const char *keynameend, const char *varname, DWORD  &data );
    void            SetPreference       ( const char *keynameend, const char *varname, UINT32 &data );

    void            DeletePreference    ( const char *keynameend, const char *value );

protected:

    const owl::TRegKey* Hive;                           // storing a pointer, as TRegKey as no default constructor
    char            KeyStart[ RegistryMaxKeyLength ];   // in the form: Software\Company\Product
};


//----------------------------------------------------------------------------

enum    RegistryDepthEnum
        {
        UptoCompany         = 1,
        UptoProduct,
        UptoVersion,
        UptoSubversion,

        FirstRegistryDepth  = UptoCompany,
        LastRegistryDepth   = UptoSubversion,
        };


class   TProductInfo
{
public:
                    TProductInfo ();

                                        // return true if level is reached, but name is filled as much as possible
    bool            GetRegistryName ( RegistryDepthEnum upto, char* name );

protected:
    char            CompanyName     [ 64 ];
    char            ProductName     [ 64 ];
    long            Version;
    char            VersionName     [ 32 ];
    long            Subversion;
    char            SubversionName  [ 32 ];
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrap the TClipboard class by including all the variables & tests needed for text handling
class   TClipboardEx :  public  owl::TClipboard
{
public:
                    TClipboardEx        ();
                    TClipboardEx        ( HWND hw, int chunksize );
                   ~TClipboardEx        ();


    bool            IsAllocated         ()          {   return  MemoryHdl != 0; }
    bool            IsNotAllocated      ()          {   return  MemoryHdl == 0; }

    void            Set                 ( HWND hw, int chunksize );
    void            Reset               ( bool releasememory = true );


    void            Send                ( UINT32 format );
    void            AddNewLine          ();         // adding an Excel-compatible newline

                                                                                                                                     // flush & test
                    operator    bool    ()          { return    IsAllocated () && (bool) *( dynamic_cast< owl::TClipboard *> (this)) && EmptyClipboard (); }
                    operator    char*   ()          { return    IsAllocated () ? Buff : 0; }


protected:

    HANDLE          MemoryHdl;
    char*           Buff;       // basically an alias to MemoryHdl
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
