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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   owl::TModule;

                                        // Reading the VERSIONINFO resource.
class   TCartoolVersionInfo
{
public:
                    TCartoolVersionInfo ( owl::TModule* module = 0 );
                   ~TCartoolVersionInfo ();


    bool            GetProductName      ( LPCTSTR& s )  const;
    bool            GetBranchName       ( LPCTSTR& s )  const;
    bool            GetProductVersion   ( LPCTSTR& s )  const;
    bool            GetProductRevision  ( LPCTSTR& s )  const;
    bool            GetProductDate      ( LPCTSTR& s )  const;
    bool            GetCopyright        ( LPCTSTR& s )  const;
    bool            GetBuild            ( LPCTSTR& s )  const;
    bool            GetArchitecture     ( LPCTSTR& s )  const;
    bool            GetOpenMP           ( LPCTSTR& s )  const;
    bool            GetRunTimeLibrary   ( LPCTSTR& s )  const;
    bool            GetInstructionSet   ( LPCTSTR& s )  const;
    bool            GetMKL              ( LPCTSTR& s )  const;
    bool            GetDPIAwareness     ( LPCTSTR& s )  const;


protected:

    UCHAR*          TransBlock;
    void*           FVData;
    char            Revision[ 8 ];

    bool            HasFileVersionInfo  ()              const   { return FVData != 0; }


private:
                                        // Prevent this object to be copied
                            TCartoolVersionInfo         ( const TCartoolVersionInfo& )  {}
    TCartoolVersionInfo&    operator                =   ( const TCartoolVersionInfo& )  {}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
