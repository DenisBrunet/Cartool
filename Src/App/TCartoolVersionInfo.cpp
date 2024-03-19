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

#include    <stdio.h>

#include    <owl/pch.h>
#include    <owl/version.h>
#include    <owl/module.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolVersionInfo.h"
#include    "..\..\Setup\GitWCRev.h"

#include    "Strings.Utils.h"

#include    "TCartoolApp.h"
#include    "mkl_types.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TCartoolVersionInfo::TCartoolVersionInfo ( TModule* module )
{
DWORD               fvHandle;
uint                vSize;
char                appFName    [ CartoolMaxExtendedPath ];
char				subBlockName[ CartoolMaxExtendedPath ];


TransBlock  = 0;
FVData      = 0;


module->GetModuleFileName ( (LPTSTR) appFName, sizeof appFName );

OemToCharBuffA ( (LPCSTR) appFName, (LPSTR) appFName, sizeof appFName );


uint32              dwSize          = ::GetFileVersionInfoSize ( (LPTSTR) appFName, &fvHandle );

if ( dwSize ) {

    FVData  = (void *) new char [ (uint) dwSize ];

    if ( ::GetFileVersionInfo ( (LPTSTR) appFName, 0, dwSize, FVData ) ) {

        // Copy string to buffer so if the -dc compiler switch(Put constant strings in code segments)
        // is on VerQueryValue will work under Win16.  This works around a problem in Microsoft's ver.dll
        // which writes to the string pointed to by subBlockName.

        StringCopy ( subBlockName, "\\VarFileInfo\\Translation" );

        if ( ! ::VerQueryValue  (   (LPTSTR) FVData,    (LPTSTR) subBlockName,  (void**) &TransBlock,   &vSize ) ) {

            delete[] FVData;
            FVData  = 0;
            }
        else
            // Swap the words so sprintf will print the lang-charset in the correct format.
            *(uint32 *)TransBlock = MAKELONG(HIWORD(*(uint32 *)TransBlock), LOWORD(*(uint32 *)TransBlock));
        }
    }
}


        TCartoolVersionInfo::~TCartoolVersionInfo ()
{
if ( FVData ) {

    delete[] FVData;
    FVData  = 0;
    }
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductName ( LPSTR &s )
{
if ( FVData == 0 )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "ProductName" );

return ::VerQueryValue (    (LPTSTR) FVData,    (LPTSTR) subBlockName,  (void**) &s,     &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductVersion ( LPSTR &s )
{
if ( FVData == 0 )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "ProductVersion" );

return ::VerQueryValue (   (LPTSTR) FVData,    (LPTSTR) subBlockName,  (void**) &s,  &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetBranchName ( LPSTR &s )
{
s   = GitBranchName;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductRevision ( LPSTR &s )
{
s   = GitRevision7;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductDate ( LPSTR &s )
{
s   = GitDateNow;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetCopyright ( LPSTR &copyright )
{
if ( FVData == 0 )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "LegalCopyright" );

return ::VerQueryValue (   (LPTSTR) FVData,    (LPTSTR) subBlockName,  (void**) &copyright,    &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetBuild ( LPSTR &s )
{
#if defined (_CONSOLE)
s   = "Console";
#elif defined (_DEBUG)
s   = "Debug";
#else
s   = "Release";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetArchitecture ( LPSTR &s )
{
#if defined (_WIN64)
s   = "64-bit";
#else
s   = "32-bit";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetOpenMP ( LPSTR &s )
{
#if defined (_OPENMP)
s   = "OpenMP";
return  true;
#else
return  false;
#endif
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetRunTimeLibrary ( LPSTR &s )
{
#if defined (_MT)
s   = "Multi-thread";
#else
s   = "Single-thread";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetInstructionSet ( LPSTR &s )
{
#if defined (__AVX512BW__)
s   = "AVX512";
#elif defined (__AVX2__)
s   = "AVX2";
#elif defined (__AVX__)
s   = "AVX";
#elif _M_IX86_FP == 0
s   = "No Vectorization"; // "IA32";
return false;
#elif _M_IX86_FP == 1
s   = "SSE";
#elif _M_IX86_FP == 2
s   = "SSE2";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetMKL ( LPSTR &s )
{
                                        // or _MKL_SERVICE_H_
#if defined (_MKL_TYPES_H_)
s   = "Intel® MKL";
return  true;
#else
return  false;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
