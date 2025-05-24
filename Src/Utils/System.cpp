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
result.Clear ();

if ( StringIsEmpty ( variable ) )
    return  false;

return  GetEnvironmentVariable ( variable, result, result.Size () ) != 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    ConcatKeys      (   const char*             keynamestart,   const char*     keynameend,
                            char*                   keyname
                        ) 
{
StringCopy  ( keyname, keynamestart );
                                        // don't concat anything if second part is empty...
if ( StringIsNotEmpty ( keynameend ) )
                                        // add a proper separator
    StringAppend    ( keyname, *keynameend != '\\' ? "\\" : "", keynameend );
}


//----------------------------------------------------------------------------
                                        // Creating registry keys with correct editable attributes (using defines, as lambda do not seem to work properly here)
#define TRegKeyRead(HIVE,KEYNAME)       owl::TRegKey ( HIVE, KEYNAME, KEY_READ,       owl::TRegKey::NoCreate )
#define TRegKeyReadWrite(HIVE,KEYNAME)  owl::TRegKey ( HIVE, KEYNAME, KEY_ALL_ACCESS, owl::TRegKey::CreateOK )



//----------------------------------------------------------------------------
bool    NukeKey         (   owl::TRegKey&           hive,
                            const char*             keyname
                        ) 
{
if ( hive == 0 || StringIsEmpty ( keyname ) )
    return  false;

//DBGM2 ( hive.GetName (), keyname, "NukeKey: hive, key" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

try
{
return  hive.NukeKey ( keyname ) == ERROR_SUCCESS;
}

catch ( const std::exception&     ex )  { /*DBGM ( ex.what(), "NukeKey exception error"   );*/  return false;  }
catch (...)                             { /*DBGM ( "Error",   "NukeKey error"             );*/  return false;  }
}


//----------------------------------------------------------------------------
                                        // Re-create the old QueryDefValue function
bool    QueryDefValue   (   const owl::TRegKey&     hive,
                            const char*             keyname, 
                            char*                   data 
                        ) 
{
ClearString ( data );

if ( hive == 0 || StringIsEmpty ( keyname ) )
    return  false;

//DBGM2 ( hive.GetName (), keyname, "QueryDefValue: hive, key" );

if ( ! hive.HasSubkey ( keyname ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff  [ RegistryMaxDataLength ];
long                size            = RegistryMaxDataLength;

                                                   // subkey
if ( ::RegQueryValue ( TRegKeyRead ( hive, keyname ), 0, buff, &size ) == ERROR_SUCCESS ) {

    StringCopy  ( data, buff );
//  DBGM3 ( hive.GetName (), keyname, data, "QueryDefValue: hive, key -> data" );
    return  true;
    }
else {
//  DBGV ( error, "QueryValue error" );
    return  false;
    }
}


//----------------------------------------------------------------------------
bool    SetDefValue     (   const owl::TRegKey&     hive,
                            const char*             keyname, 
                            const char*             data 
                        ) 
{
if ( hive == 0 || StringIsEmpty ( keyname ) )
    return  false;


try
{                                                        // subkey
return  ::RegSetValue ( TRegKeyReadWrite ( hive, keyname ), 0, REG_SZ, data, StringLength ( data ) ) == ERROR_SUCCESS;
}

catch ( const std::exception&     ex )  { /*DBGM ( ex.what(), "SetDefValue exception error"   );*/  return false;  }
catch (...)                             { /*DBGM ( "Error",   "SetDefValue error"             );*/  return false;  }
}


//----------------------------------------------------------------------------
                                        // Getting string
bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            char*                   data 
                        ) 
{
ClearString ( data );

if ( hive == 0 || StringIsEmpty ( keynamestart ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                keyname  [ RegistryMaxKeyLength ];

ConcatKeys  ( keynamestart, keynameend, keyname );

//DBGM3 ( hive.GetName (), keyname, varname, "QueryValue: hive, key, var" );

if ( ! hive.HasSubkey ( keyname ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff  [ RegistryMaxDataLength ];
owl::uint32         size            = RegistryMaxDataLength;
owl::uint32         type            = REG_SZ;               // UNICODE string, null terminated - but here we are currently in ASCII


if ( TRegKeyRead ( hive, keyname ).QueryValue ( varname, &type, (owl::uint8*) buff, &size ) == ERROR_SUCCESS ) {
                                        // no checks if receiving string is big enough...
    StringCopy  ( data, (char*) buff );
//  DBGM4 ( hive.GetName (), keyname, varname, data, "QueryValue: hive, key, var -> data" );
    return  true;
    }
else {
//  DBGV ( error, "QueryValue error" );
    return  false;
    }
}


//----------------------------------------------------------------------------
                                        // Getting DWORD
bool    QueryValue      (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            DWORD&                  data 
                        ) 
{
data    = 0;

if ( hive == 0 || StringIsEmpty ( keynamestart ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                keyname  [ RegistryMaxKeyLength ];

ConcatKeys  ( keynamestart, keynameend, keyname );

//DBGM3 ( hive.GetName (), keyname, varname, "QueryValue: hive, key, var" );

if ( ! hive.HasSubkey ( keyname ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

owl::uint32         size            = sizeof ( DWORD ); // this function wants uint32
owl::uint32         type            = REG_DWORD;        // DWORD


return  TRegKeyRead ( hive, keyname ).QueryValue ( varname, &type, (owl::uint8*) &data, &size ) == ERROR_SUCCESS;
}


//----------------------------------------------------------------------------
                                        // Setting string
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            const char*             data
                        )
{
if ( hive == 0 || StringIsEmpty ( keynamestart ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                keyname  [ RegistryMaxKeyLength ];

ConcatKeys  ( keynamestart, keynameend, keyname );

//DBGM4 ( hive.GetName (), keyname, varname, data, "SetValue: hive, key, var, data" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

try
{
return  TRegKeyReadWrite ( hive, keyname ).SetValue ( varname, REG_SZ, (const owl::uint8*) data, StringLength ( data ) ) == ERROR_SUCCESS;
}

catch ( const std::exception&     ex )  { /*DBGM ( ex.what(), "SetValue exception error"   );*/  return false;  }
catch (...)                             { /*DBGM ( "Error",   "SetValue error"             );*/  return false;  }
}


//----------------------------------------------------------------------------
                                        // Setting DWORD
bool    SetValue        (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname, 
                            DWORD                   data
                        )
{
if ( hive == 0 || StringIsEmpty ( keynamestart ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                keyname  [ RegistryMaxKeyLength ];

ConcatKeys  ( keynamestart, keynameend, keyname );

//DBGM4 ( hive.GetName (), keyname, varname, (char*) IntegerToString ( data ), "SetValue: hive, key, var, data" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

try
{
return  TRegKeyReadWrite ( hive, keyname ).SetValue ( varname, data ) == ERROR_SUCCESS;
}

catch ( const std::exception&     ex )  { /*DBGM ( ex.what(), "SetValue exception error"   );*/  return false;  }
catch (...)                             { /*DBGM ( "Error",   "SetValue error"             );*/  return false;  }
}


//----------------------------------------------------------------------------
                                        // Deleting variable of any type
bool    DeleteValue     (   const owl::TRegKey&     hive,
                            const char*             keynamestart,   const char*     keynameend,
                            const char*             varname
                        )
{
if ( hive == 0 || StringIsEmpty ( keynamestart ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate the key
char                keyname  [ RegistryMaxKeyLength ];

ConcatKeys  ( keynamestart, keynameend, keyname );

if ( ! hive.HasSubkey ( keyname ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

try
{
return  TRegKeyReadWrite ( hive, keyname ).DeleteValue ( varname ) == ERROR_SUCCESS;
}

catch ( const std::exception&     ex )  { /*DBGM ( ex.what(), "DeleteValue exception error"   );*/  return false;  }
catch (...)                             { /*DBGM ( "Error",   "DeleteValue error"             );*/  return false;  }
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


        TPreferences::TPreferences  (   const owl::TRegKey&     hive,   const char*     keynamestart    )
{
Reset ();

if ( StringIsEmpty ( keynamestart ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Hive    = &hive;
                                        // keynamestart contains all the string to the mother key - could be technically empty, but this is not allowed here
StringCopy ( KeyStart, keynamestart );
}


//----------------------------------------------------------------------------
char*   TPreferences::GetPreference ( const char *keynameend, const char *varname, char *data )
{
ClearString ( data );

if ( IsNotOpen () )
    return  data;

QueryValue  ( *Hive, KeyStart, keynameend, varname, data );

//DBGM4 ( KeyStart, keynameend, varname, StringIsEmpty ( data ) ? "<empty>" : data, "GetPreference: KeyStart keynameend varname data");

return  data;
}


void    TPreferences::SetPreference ( const char *keynameend, const char *varname, char *data )
{
if ( IsNotOpen () )
    return;

SetValue    ( *Hive, KeyStart, keynameend, varname, data );
}


//----------------------------------------------------------------------------
DWORD   TPreferences::GetPreference ( const char *keynameend, const char *varname, DWORD &data )
{
data    = 0;

if ( IsNotOpen () )
    return  0;

QueryValue  ( *Hive, KeyStart, keynameend, varname, data );

//DBGM4 ( KeyStart, keynameend, varname, (char*) IntegerToString ( data ), "GetPreference: KeyStart keynameend varname data" );

return  data;
}


void    TPreferences::SetPreference ( const char *keynameend, const char *varname, UINT32 &data )
{
if ( IsNotOpen () )
    return;

SetValue    ( *Hive, KeyStart, keynameend, varname, data );
}


//----------------------------------------------------------------------------
void    TPreferences::DeletePreference ( const char *keynameend, const char *varname )
{
if ( IsNotOpen () )
    return;

DeleteValue     ( *Hive, KeyStart, keynameend, varname );
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
