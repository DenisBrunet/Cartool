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
//      TClipboardEx
//
//----------------------------------------------------------------------------


#include    <stdio.h>

#include    "Strings.Utils.h"
#include    "System.h"
#include    "Dialogs.Input.h"
#include    "Math.Utils.h"

#include    "TCartoolApp.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    ShowLastError ()
{
LPVOID              lpMsgBuf;

FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError (),
                MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL
                );

ShowMessage ( (char *) lpMsgBuf, "GetLastError", ShowMessageWarning );

LocalFree ( lpMsgBuf );
}


//----------------------------------------------------------------------------
                                        // Changing current process (Cartool for the moment) priority - including all future threads
                                        // !PROCESS_MODE_BACKGROUND_BEGIN is not an option, we don't want to reduce the heap & # of handles!
void    SetProcessPriority  ( ProcessPriorityFlags how )
{
switch ( how ) {
    case    ProcessPriority0    :   SetPriorityClass ( GetCurrentProcess (),    IDLE_PRIORITY_CLASS         );  break;
    case    ProcessPriority1    :   SetPriorityClass ( GetCurrentProcess (),    BELOW_NORMAL_PRIORITY_CLASS );  break;
    case    ProcessPriority2    :   SetPriorityClass ( GetCurrentProcess (),    NORMAL_PRIORITY_CLASS       );  break;
    case    ProcessPriority3    :   SetPriorityClass ( GetCurrentProcess (),    ABOVE_NORMAL_PRIORITY_CLASS );  break;
    case    ProcessPriority4    :   SetPriorityClass ( GetCurrentProcess (),    HIGH_PRIORITY_CLASS         );  break;
    case    ProcessPriority5    :   SetPriorityClass ( GetCurrentProcess (),    REALTIME_PRIORITY_CLASS     );  break;
    }
}


//----------------------------------------------------------------------------

bool    GetEnvironmentVariableEx ( const char* variable, TFileName& result )
{
result.Reset ();

if ( StringIsEmpty ( variable ) )
    return  false;

return  GetEnvironmentVariable ( variable, result, result.Size () ) != 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    ConcatKeys  (   const char*             keystart,   const char*         keyend,
                        char*                   key
                    ) 
{
StringCopy  ( key, keystart );
                                        // don't concat anything if second part is empty...
if ( StringIsNotEmpty ( keyend ) )
                                        // add a proper separator
    StringAppend    ( key, *keyend != '\\' ? "\\" : "", keyend );
}


//----------------------------------------------------------------------------
                                        // Re-create the old QueryDefValue function
bool    QueryDefValue   (   const owl::TRegKey&     hive,
                            const char*             key, 
                            char*                   data 
                        ) 
{
ClearString ( data );

if ( hive == 0 || StringIsEmpty ( key ) )
    return  false;

//DBGM ( key, "QueryDefValue: key" );

if ( ! hive.HasSubkey ( key ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff  [ RegistryMaxDataLength ];
long                size            = RegistryMaxDataLength;


long                error           =

::RegQueryValue (   owl::TRegKey ( hive, key, KEY_READ, owl::TRegKey::NoCreate ),
                    0,                          // subkey
                    buff,   &size
                );


if ( error == ERROR_SUCCESS )
    StringCopy  ( data, buff );
//else
//    DBGV ( error, "QueryValue error" );
//DBGM2 ( key, data, "QueryDefValue: key -> data" );

return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
bool    SetDefValue     (   const owl::TRegKey&     hive,
                            const char*             key, 
                            const char*             data 
                        ) 
{
if ( hive == 0 || StringIsEmpty ( key ) )
    return  false;


long                error           =

::RegSetValue   (   owl::TRegKey ( hive, key, KEY_ALL_ACCESS, owl::TRegKey::CreateOK ),
                    0,                          // subkey
                    REG_SZ, 
                    data,   StringLength ( data )
                );

return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Getting string
bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keystart,   const char*         keyend,
                            const char*             varname, 
                            char*                   data 
                        ) 
{
ClearString ( data );

if ( hive == 0 || StringIsEmpty ( keystart ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                key  [ RegistryMaxKeyLength ];

ConcatKeys  ( keystart, keyend, key );

//DBGM2 ( key, varname, "QueryValue: key, var" );

if ( ! hive.HasSubkey ( key ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff  [ RegistryMaxDataLength ];
owl::uint32         size            = RegistryMaxDataLength;
owl::uint32         type            = REG_SZ;               // UNICODE string, null terminated - but here we are currently in ASCII


long                error           =

owl::TRegKey ( hive, (LPCSTR) key, KEY_READ, owl::TRegKey::NoCreate ).QueryValue    (   varname, 
                                                                                        &type, 
                                                                                        (owl::uint8 *) buff,     &size   // not sure about that conversion - function wants uchar
                                                                                    );

                                        // no checks if receiving string is big enough...
if ( error == ERROR_SUCCESS )
    StringCopy  ( data, (char*) buff );
//else
//    DBGV ( error, "QueryValue error" );
//DBGM3 ( key, varname, data, "QueryValue: key, var -> data" );

return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Getting DWORD
bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keystart,   const char*         keyend,
                            const char*             varname, 
                            DWORD&                  data 
                        ) 
{
data    = 0;

if ( hive == 0 || StringIsEmpty ( keystart ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                key  [ RegistryMaxKeyLength ];

ConcatKeys  ( keystart, keyend, key );

//DBGM2 ( key, varname, "QueryValue: key, var" );

if ( ! hive.HasSubkey ( key ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

owl::uint32         size            = sizeof ( DWORD ); // this function wants uint32
owl::uint32         type            = REG_DWORD;        // DWORD


long                error           =

owl::TRegKey ( hive, (LPCSTR) key, KEY_READ, owl::TRegKey::NoCreate ).QueryValue    (   varname, 
                                                                                        &type, 
                                                                                        (owl::uint8 *) &data,       &size
                                                                                    );
//if ( error == ERROR_SUCCESS )
//    DBGV ( data, "QueryValue" )
//else
//    DBGV ( error, "QueryValue error" );

return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Setting string
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keystart,   const char*         keyend,
                            const char*             varname, 
                            const char*             data
                        )
{
if ( hive == 0 || StringIsEmpty ( keystart ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                key  [ RegistryMaxKeyLength ];

ConcatKeys  ( keystart, keyend, key );

//DBGM3 ( key, varname, data, "SetValue: key, var, data" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                error           =

owl::TRegKey ( hive, (LPCSTR) key, KEY_ALL_ACCESS, owl::TRegKey::CreateOK ).SetValue    (   varname,
                                                                                            REG_SZ, 
                                                                                            (const UINT8 *) data,   StringLength ( data ) 
                                                                                        );
return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Setting DWORD
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keystart,   const char*         keyend,
                            const char*             varname, 
                            DWORD                   data
                        )
{
if ( hive == 0 || StringIsEmpty ( keystart ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                key  [ RegistryMaxKeyLength ];

ConcatKeys  ( keystart, keyend, key );

//DBGM3 ( key, varname, (char*) IntegerToString ( data ), "SetValue: key, var, data" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                error           =

owl::TRegKey ( hive, (LPCSTR) key, KEY_ALL_ACCESS, owl::TRegKey::CreateOK ).SetValue    (   varname,
                                                                                            data
                                                                                        );
return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Deleting variable of any type
bool    DeleteValue     (   const owl::TRegKey&     hive,
                            const char*             keystart,   const char*         keyend,
                            const char*             varname
                        )
{
if ( hive == 0 || StringIsEmpty ( keystart ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                key  [ RegistryMaxKeyLength ];

ConcatKeys  ( keystart, keyend, key );

if ( ! hive.HasSubkey ( key ) )
    return  false;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                error           =

owl::TRegKey ( hive, (LPCSTR) key, KEY_ALL_ACCESS, owl::TRegKey::CreateOK ).DeleteValue ( varname );

return  error == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TPreferences::TPreferences ()
{
Reset ();
}


void    TPreferences::Reset ()
{
Hive    = 0;
ClearString ( KeyStart );
}


        TPreferences::TPreferences ( const owl::TRegKey& hive, const char *keystart )
{
Reset ();

if ( StringIsEmpty ( keystart ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Hive    = &hive;
                                        // keystart contains all the string to the mother key - could be technically empty, but this is not allowed here
StringCopy ( KeyStart, keystart );
}


//----------------------------------------------------------------------------
char*   TPreferences::GetPreference ( const char *keyend, const char *varname, char *data )
{
ClearString ( data );

if ( IsNotOpen () )
    return  data;

QueryValue  ( *Hive, KeyStart, keyend, varname, data );

//DBGM4 ( KeyStart, keyend, varname, StringIsEmpty ( data ) ? "<empty>" : data, "GetPreference: KeyStart keyend varname data");

return  data;
}


void    TPreferences::SetPreference ( const char *keyend, const char *varname, char *data )
{
if ( IsNotOpen () )
    return;

SetValue    ( *Hive, KeyStart, keyend, varname, data );
}


//----------------------------------------------------------------------------
DWORD   TPreferences::GetPreference ( const char *keyend, const char *varname, DWORD &data )
{
data    = 0;

if ( IsNotOpen () )
    return  0;

QueryValue  ( *Hive, KeyStart, keyend, varname, data );

//DBGM4 ( KeyStart, keyend, varname, (char*) IntegerToString ( data ), "GetPreference: KeyStart keyend varname data" );

return  data;
}


void    TPreferences::SetPreference ( const char *keyend, const char *varname, UINT32 &data )
{
if ( IsNotOpen () )
    return;

SetValue    ( *Hive, KeyStart, keyend, varname, data );
}


//----------------------------------------------------------------------------
void    TPreferences::DeletePreference ( const char *keyend, const char *varname )
{
if ( IsNotOpen () )
    return;

DeleteValue     ( *Hive, KeyStart, keyend, varname );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TProductInfo::TProductInfo ()
{
ClearString ( CompanyName );
ClearString ( ProductName );
Version             = 0;
ClearString ( VersionName );
Subversion          = 0;
ClearString ( SubversionName );
}
                                        // returns a bool indicating that the level requested is attained
                                        // however, the end result is in name
bool    TProductInfo::GetRegistryName   (   RegistryDepthEnum   upto,       char*   name    )
{
if ( upto < FirstRegistryDepth || upto > LastRegistryDepth ) {
    ClearString ( name );
    return false;
    }

else if ( upto == UptoCompany ) {

    if ( *CompanyName ) {
        sprintf ( name, "\\%s", CompanyName );
        return true;
        }
    else {
        ClearString ( name );
        return false;
        }
    }

else if ( upto == UptoProduct ) {

    if ( GetRegistryName ( (RegistryDepthEnum) ( upto - 1 ), name ) )
        if ( *ProductName ) {
            StringAppend ( name, "\\", ProductName );
            return true;
            }
    return false;
    }

else if ( upto == UptoVersion ) {

    if ( GetRegistryName ( (RegistryDepthEnum) ( upto - 1 ), name ) )
        if ( *VersionName ) {
            StringAppend ( name, "\\", VersionName );
            return true;
            }
        else if ( Version != 0 ) {
            sprintf ( VersionName, "\\%0d", Version );
            StringAppend ( name, VersionName );
            ClearString ( VersionName );
            return true;
            }
    return false;
    }

else if ( upto == UptoSubversion ) {

    if ( GetRegistryName ( (RegistryDepthEnum) ( upto - 1 ), name ) )
        if ( *SubversionName ) {
            StringAppend ( name, "\\", SubversionName );
            return true;
            }
        else if ( Subversion != 0 ) {
            sprintf ( SubversionName, "\\%0d", Subversion );
            StringAppend ( name, SubversionName );
            ClearString ( SubversionName );
            return true;
            }
    return false;
    }


return false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TClipboardEx::TClipboardEx ()
{
MemoryHdl       = 0;
Buff            = 0;
}


        TClipboardEx::TClipboardEx ( HWND hw, int chunksize )
      : TClipboard ( hw )
{
MemoryHdl       = 0;
Buff            = 0;

Set ( hw, chunksize );
}


        TClipboardEx::~TClipboardEx ()
{                                       // when closing, DO NOT release memory
Reset ( false );
}


void    TClipboardEx::Reset ( bool releasememory )
{
if ( IsNotAllocated () )
    return;


::GlobalUnlock ( MemoryHdl );

if ( releasememory )
    ::GlobalFree ( MemoryHdl );


MemoryHdl       = 0;
Buff            = 0;
}


void    TClipboardEx::Set ( HWND hw, int chunksize )
{
Reset ();

if ( chunksize == 0 )
    return;


if ( ! OpenClipboard ( hw ) ) 
    return; 

                                        // flush current content
EmptyClipboard ();

                                        // put in the Heap - !this is supposed to be a costly operation (read: can take time)!
MemoryHdl       = ::GlobalAlloc ( GHND, chunksize );

                                        // !alloc could fail!
if ( MemoryHdl == NULL ) {

    CloseClipboard ();

    Reset ();
    }
else {
    Buff        = (char *) ::GlobalLock ( MemoryHdl );

    ClearString ( Buff );
    }
}


//----------------------------------------------------------------------------

void    TClipboardEx::Send ( UINT32 format )
{
if ( IsNotAllocated () )
    return;


::GlobalUnlock ( MemoryHdl );

TClipboard::SetClipboardData ( format, MemoryHdl );
}


void    TClipboardEx::AddNewLine ()
{
if ( Buff )     sprintf ( StringEnd ( Buff ), "%c%c", 0x0d, 0x0a );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TKeyboard::TKeyboard ()
{
Reset ();

DelayForPolling             =  250;
DelayBeforeAcceleration     = 1250;
DelayAfterNoAcceleration    = 2500;
MinSpeed                    =    1;
MaxSpeed                    =   10;
}


void    TKeyboard::Reset ()
{
ClearVirtualMemory ( KeyboardState,         KeyboardSize * sizeof ( *KeyboardState ) );
ClearVirtualMemory ( PreviousKeyboardState, KeyboardSize * sizeof ( *KeyboardState ) );

UpdateApplication;

StopAccelerating ();
}


void    TKeyboard::GetState ()
{
                                        // purge the queue first
UpdateApplication;
                                        // save current state
CopyVirtualMemory ( PreviousKeyboardState, KeyboardState, KeyboardSize * sizeof ( *KeyboardState ) );
                                        // load whole keyboard state at once
GetKeyboardState ( KeyboardState );
}


bool    TKeyboard::IsPressed ( const UCHAR* keyboardstate, int vki )    const
{
                                                      // high order bit tells if pressed
return  IsInsideLimits ( vki, 0, KeyboardSize - 1 ) && ( keyboardstate[ vki ] & 0x80 );
}


bool    TKeyboard::IsPressed ( int vki )    const
{
return  IsPressed ( KeyboardState, vki );
}


bool    TKeyboard::IsReleased ( int vki )   const
{

return  ! IsPressed ( KeyboardState, vki ) && IsPressed ( PreviousKeyboardState, vki );
}


bool    TKeyboard::IsToggled ( int vki )    const
{
                                                      // low order bit tells if toggled (supposedly non-relevant if key is non-toggle-able)
return  IsInsideLimits ( vki, 0, KeyboardSize - 1 ) && ( KeyboardState[ vki ] & 0x01 );
}


bool    TKeyboard::IsAnyKeyChange ()    const
{
for ( int i = 0; i < KeyboardSize; i++ )

//  if ( KeyboardState [ i ] != PreviousKeyboardState[ i ] )
    if ( ( KeyboardState [ i ] & 0x80 ) != ( PreviousKeyboardState[ i ] & 0x80 ) )

        return  true;

return  false;
}


void    TKeyboard::Show ( const char *title )     const
{
char                state[ 1024 ];

ClearString ( state );

for ( int i = 0; i < KeyboardSize; i++ )
    StringAppend ( state, IsPressed ( i ) ? "X" : "_",
//                        IsPressed ( i ) && IsToggled ( i ) ? "]"
//                      : IsPressed ( i )                    ? "P"
//                      :                    IsToggled ( i ) ? "T"
//                      :                                      "_",
                          ( ( i + 1 ) % 32 ) == 0 ? "\n" : "" );

ShowMessage ( state, StringIsEmpty ( title ) ? "Keyboard State" : title );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
