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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   owl::TModule;

                                        // Reading the VERSIONINFO resource.
class   TCartoolVersionInfo
{
public:
                    TCartoolVersionInfo ( owl::TModule* module );
                   ~TCartoolVersionInfo ();


    bool            GetProductName      ( LPSTR &s );
    bool            GetBranchName       ( LPSTR &s );
    bool            GetProductVersion   ( LPSTR &s );
    bool            GetProductRevision  ( LPSTR &s );
    bool            GetProductDate      ( LPSTR &s );
    bool            GetCopyright        ( LPSTR &s );
    bool            GetBuild            ( LPSTR &s );
    bool            GetArchitecture     ( LPSTR &s );
    bool            GetOpenMP           ( LPSTR &s );
    bool            GetRunTimeLibrary   ( LPSTR &s );
    bool            GetInstructionSet   ( LPSTR &s );
    bool            GetMKL              ( LPSTR &s );


protected:

    UCHAR*          TransBlock;
    void*           FVData;
    char            Revision[ 8 ];


private:
                                        // Don't allow this object to be copied.
                            TCartoolVersionInfo         ( const TCartoolVersionInfo& )  {}
    TCartoolVersionInfo&    operator                =   ( const TCartoolVersionInfo& )  {}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
