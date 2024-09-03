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
TransBlock  = 0;
FVData      = 0;

StringCopy          ( Revision, GitRevision7, 7 );
StringToUppercase   ( Revision );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           appfilename;
                                        // call the one function that does not crash
if ( module && module->GetHandle () != 0 )  module->GetModuleFileName (    appfilename, appfilename.Size () );
else                                              ::GetModuleFileName ( 0, appfilename, appfilename.Size () );

OemToCharBuffA      ( appfilename, appfilename, appfilename.Size () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DWORD               fvh;
uint32              fvsize          = ::GetFileVersionInfoSize ( appfilename, &fvh );


if ( fvsize ) {

    FVData  = (void *) new char [ (uint) fvsize ];

    if ( ::GetFileVersionInfo ( appfilename, 0, fvsize, FVData ) ) {

        // Copy string to buffer so if the -dc compiler switch(Put constant strings in code segments)
        // is on VerQueryValue will work under Win16.  This works around a problem in Microsoft's ver.dll
        // which writes to the string pointed to by subblockname.

        TFileName       subblockname    = "\\VarFileInfo\\Translation";
        uint            blocksize;

        if ( ! ::VerQueryValue  ( FVData, subblockname, (void**) &TransBlock, &blocksize ) ) {

            delete[] FVData;
            FVData  = 0;
            }
        else {
            // Swap the words so sprintf will print the lang-charset in the correct format.
            *(uint32 *)TransBlock = MAKELONG(HIWORD(*(uint32 *)TransBlock), LOWORD(*(uint32 *)TransBlock));
            }
        } // if GetFileVersionInfo
    } // if GetFileVersionInfoSize
}


//----------------------------------------------------------------------------
        TCartoolVersionInfo::~TCartoolVersionInfo ()
{
if ( FVData ) {

    delete[] FVData;
    FVData  = 0;
    }
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductName ( LPCTSTR& s )   const
{
if ( ! HasFileVersionInfo () )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "ProductName" );

return ::VerQueryValue ( FVData, subBlockName, (void**) &s, &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductVersion ( LPCTSTR& s )   const
{
if ( ! HasFileVersionInfo () )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "ProductVersion" );

return ::VerQueryValue ( FVData, subBlockName, (void**) &s, &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetBranchName ( LPCTSTR& s )   const
{
s   = GitBranchName;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductRevision ( LPCTSTR& s )   const
{
s   = Revision;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetProductDate ( LPCTSTR& s )   const
{
s   = GitDateNow;
return true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetCopyright ( LPCTSTR& copyright )   const
{
if ( ! HasFileVersionInfo () )
    return  false;

uint                vSize;
char				subBlockName[ KiloByte ];

sprintf ( subBlockName, "\\StringFileInfo\\%08lx\\%s", *(uint32 *)TransBlock, "LegalCopyright" );

return ::VerQueryValue ( FVData, subBlockName, (void**) &copyright, &vSize );
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetBuild ( LPCTSTR& s )   const
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
bool    TCartoolVersionInfo::GetArchitecture ( LPCTSTR& s )   const
{
#if defined (_WIN64)
s   = "64-bit";
#else
s   = "32-bit";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetOpenMP ( LPCTSTR& s )   const
{
#if defined (_OPENMP)
s   = "OpenMP";
return  true;
#else
return  false;
#endif
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetRunTimeLibrary ( LPCTSTR& s )   const
{
#if defined (_MT)
s   = "Multi-thread";
#else
s   = "Single-thread";
#endif

return  true;
}


//----------------------------------------------------------------------------
bool    TCartoolVersionInfo::GetInstructionSet ( LPCTSTR& s )   const
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
bool    TCartoolVersionInfo::GetMKL ( LPCTSTR& s )   const
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
bool    TCartoolVersionInfo::GetDPIAwareness ( LPCTSTR& s )   const
{
DPI_AWARENESS       dpiawareness    = TCartoolApp::GetDPIAwareness ();

switch ( dpiawareness ) {

    case    DPI_AWARENESS_SYSTEM_AWARE      :   s = "DPI System Aware";     return true;
    case    DPI_AWARENESS_PER_MONITOR_AWARE :   s = "DPI Monitor Aware";    return true;
    default                                 :   s = "Not DPI Aware";        return false;
    };
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
