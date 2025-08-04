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

#include    <typeinfo>

#include    <owl/pch.h>

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "BrainToSolutionPointsUI.h"

#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"

#include    "TVolumeDoc.h"
#include    "ESI.InverseModels.h"
#include    "ESI.SolutionPoints.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::BrainToSolutionPointsUI  ()
{
crtl::BrainToSolutionPointsUI ( 0, 0 ); // default arguments
}


//----------------------------------------------------------------------------
                                        // UI with optional volumes parameters
bool    BrainToSolutionPointsUI     ( 
                                    TVolumeDoc*     mribraindoc,        
                                    TVolumeDoc*     mrigreydoc
                                    )
{
                                        // Caller might have provided us with 0 / 1 / 2 MRIs already
                                        // therefore, we might need to complete the required documents to have exactly 2

                                        // Let's start by looking at all opened docs:
TGoF                mrifiles;

Cartool.CartoolDocManager->GetDocs ( mrifiles, AllMriFilesExt );

                                    // browse through all opened docs and try to retrieve the proper MRIs
for ( int i = 0; i < (int) mrifiles; i++ ) {

    TVolumeDoc*     mridoc         = static_cast<TVolumeDoc*> ( Cartool.CartoolDocManager->IsOpen ( mrifiles[ i ] ) );

    if ( mridoc == 0 )  continue;   // just for safety, it should be OK

    if ( mribraindoc == 0 && mridoc->IsBrain () )   mribraindoc = mridoc;
    if ( mrigreydoc  == 0 && mridoc->IsMask  () )   mrigreydoc  = mridoc;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // If opened doc failed, ask user
if ( mribraindoc == 0 ) {

    GetFileFromUser     filemri ( "Provide the Brain MRI file:", AllMriFilesFilter, 1, GetFileRead );

    if ( ! filemri.Execute () )
        return  false;

    mribraindoc = dynamic_cast<TVolumeDoc*> ( Cartool.CartoolDocManager->OpenDoc ( filemri[ 0 ], dtOpenOptions ) );

    if ( mribraindoc && ! mribraindoc->IsBrain () ) {

        ShowMessage (   "This MRI does not seem to be a proper brain," NewLine 
                        "please, try again with another file...", 
                        "Computing Solution Points", ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // If opened doc failed, ask user (again)
if ( mrigreydoc == 0 ) {

    GetFileFromUser     filemri ( "Provide the Grey Mask MRI file:", AllMriFilesFilter, 1, GetFileRead );

    if ( ! filemri.Execute () )
        return  false;

    mrigreydoc  = dynamic_cast< TVolumeDoc* > ( Cartool.CartoolDocManager->OpenDoc ( filemri[ 0 ], dtOpenOptions ) );

    if ( mrigreydoc && ! mrigreydoc->IsMask () ) {

        ShowMessage (   "This MRI does not seem to be a proper grey mask," NewLine 
                        "please, try again with another file...", 
                        "Computing Solution Points", ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // code prototype to generate the grey mask
TFileName               filemrigrey;
TOpenDoc<TVolumeDoc>    greydoc;


if ( mrigreydoc == 0 ) {

    char                buff[ 256 ];

    StringCopy      ( filemrigrey, mribraindoc->GetDocPath () );
    RemoveExtension ( filemrigrey );
    StringAppend    ( filemrigrey, ".Tmp", StringRandom ( buff, 6 ) );
    AddExtension    ( filemrigrey, DefaultMriExt );
    CheckNoOverwrite( filemrigrey );


    GreyMatterFlags     greyflags           = (GreyMatterFlags) ( GreyMatterAsymmetric | /*GreyMatterFat* / GreyMatterRegular | GreyMatterPostprocessing );


    bool                greymaskok          = SegmentGreyMatter ( mribraindoc, greyflags, filemrigrey );

    if ( greymaskok )
        greydoc.Open ( filemrigrey, OpenDocHidden );
    else
        return  false;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this shouldn't happen
if ( StringIs ( mribraindoc->GetDocPath (), mrigreydoc->GetDocPath () ) ) {

    ShowMessage (   "You can not use the same MRIs as brain AND grey mask," NewLine 
                    "please, try again with other files...", 
                    "Computing Solution Points", ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( mribraindoc == 0 || mrigreydoc == 0 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Asking some Solution Points options
int                 numsolpoints    = 0;
double              ressolpoints    = 0;
double              v;

if ( ! GetValueFromUser ( "Number of solution points, or resolution in [mm]:", "Computing Solution Points", v, "" ) )
    return  false;

if      ( v <= 0 )  return  false;
else if ( v > 50 )  numsolpoints    = Round ( v );
else                ressolpoints    = v;

                                        // standard parameters
NeighborhoodType    lolauneighborhood   = InverseNeighborhood;
GreyMatterFlags     spflags             = (GreyMatterFlags) ( GreyMatterAsymmetric | GreyMatterLauraCheck | GreyMatterSinglePointCheck );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           filesp;
TPoints             solpoints;      // results here
TStrings            spnames;


StringCopy          ( filesp, mrigreydoc->GetDocPath () );
RemoveExtension     ( filesp );
StringReplace       ( filesp, ".Grey", "" );    // use some grep at some point..
StringReplace       ( filesp, ".Gray", "" );
StringReplace       ( filesp, "_Grey", "" );
StringReplace       ( filesp, "_Gray", "" );
AddExtension        ( filesp, FILEEXT_SPIRR );
CheckNoOverwrite    ( filesp );


bool                solpointsok     = ComputeSolutionPoints (
                                                            mribraindoc,        mrigreydoc, 
                                                            numsolpoints,       ressolpoints,
                                                            spflags, 
                                                            lolauneighborhood,  lolauneighborhood,
                                                            solpoints,          spnames,
                                                            filesp
                                                            );
if ( solpointsok )
    filesp.Open ();


return  solpointsok;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
