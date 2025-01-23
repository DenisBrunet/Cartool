/*
==========================================================================
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
==========================================================================
*/

#pragma once

#include    "Files.Extensions.h"
#include    "Files.TGoF.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy utility class to read a Link-Many file and dispatch its content into appropriate lists
class   TSplitLinkManyFile
{
public:
    inline          TSplitLinkManyFile ( const char* file = 0 );


    TGoF            leeg;
    TGoF            lris;
    TGoF            lxyz;
    TGoF            lsp;
    TGoF            lis;
    TGoF            lmri;
    TGoF            lrois;


    inline void     Reset   ();
    inline void     Open    ( const char* file );
    inline void     Sort    ();

    inline          operator bool ()    const   { return (bool) leeg || (bool) lris || (bool) lxyz || (bool) lsp || (bool) lis || (bool) lmri || (bool) lrois; }
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

        TSplitLinkManyFile::TSplitLinkManyFile ( const char* file )
{
if ( StringIsNotEmpty ( file ) )
    Open ( file );
}


void    TSplitLinkManyFile::Reset ()
{
leeg    .Reset ();
lris    .Reset ();
lxyz    .Reset ();
lsp     .Reset ();
lis     .Reset ();
lmri    .Reset ();
lrois   .Reset ();
}


void    TSplitLinkManyFile::Sort ()
{
leeg    .Sort ();
lris    .Sort ();
lxyz    .Sort ();
lsp     .Sort ();
lis     .Sort ();
lmri    .Sort ();
lrois   .Sort ();
}


void    TSplitLinkManyFile::Open ( const char* file )
{
Reset ();

if ( StringIsEmpty ( file ) )
    return;

TFileName           lmfile; 

/*                                      // old behavior does not seem to be relevant anymore - asking for file name should rather be done by caller
if ( StringIsEmpty ( file ) ) {

    GetFileFromUser     getfile ( "Link Many File", AllLmFilesFilter, 1, GetFileRead );

    if ( ! getfile.Execute () )
        return;

    lmfile.Set ( (const char *) getfile, TFilenameExtendedPath );

    if ( file )                         // complimentary returning the filename
        StringCopy ( file, lmfile );
    }
else
*/
    lmfile.Set ( file, TFilenameExtendedPath );

                                        // is it a legal extension?
if ( ! IsExtensionAmong ( lmfile, AllLmFilesExt ) )
    return;

                                        // we can still have a non-existing lm file in case of  AllOpenedLmFilename
if ( ! CanOpenFile ( lmfile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // File is all good here, proceed to read it
ifstream            is ( lmfile );
TFileName           buff;

while ( ! is.fail () ) {

    is.getline ( buff, MaxPathShort );

    if ( StringIsSpace ( buff ) )
        continue;


    if      ( IsExtensionAmong ( buff, AllEegFreqFilesExt     ) )       leeg    .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllCoordinatesFilesExt ) )       lxyz    .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllSolPointsFilesExt   ) )       lsp     .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllInverseFilesExt     ) )       lis     .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllRisFilesExt         ) )       lris    .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllMriFilesExt         ) )       lmri    .Add ( buff );
    else if ( IsExtensionAmong ( buff, AllRoisFilesExt        ) )       lrois   .Add ( buff );

    } // while !eof
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
