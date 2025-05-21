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

#include    <owl/pch.h>

#include    "CoregistrationMrisUI.h"

#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Strings.Grep.h"
#include    "Files.TVerboseFile.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"
#include    "GlobalOptimize.Volumes.h"

#include    "TVolumeDoc.h"
#include    "Volumes.Coregistration.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const CoregistrationSpecsType   CoregistrationSpecs[ NumCoregistrationTypes ] =
        {
        {   CoregistrationTrans,                        "(O)rigin shift:"                    Tab Tab "3 translations",                                               "O",    true,   3,  0,  0,  0 },
        {   CoregistrationRotTrans,                     "(R)igid body:"                      Tab Tab "3 translations + 3 rotations",                                 "R",    true,   3,  3,  0,  0 },
        {   CoregistrationRotTransScale1,               "(G)lobal transform:"                    Tab "3 translations + 3 rotations + 1 global scaling",              "G",    true,   3,  3,  1,  0 },
        {   CoregistrationRotTransScale3,               "(E)lastic transform:"                   Tab "3 translations + 3 rotations + 3 scalings",                    "E",    true,   3,  3,  3,  0 },
        {   CoregistrationRotTransScale3Shear2,         "Aff(I)ne transform:"                    Tab "3 translations + 3 rotations + 3 scalings + 2 shearings",      "I",    true,   3,  3,  3,  2 },
        {   CoregistrationRotTransScale3Shear3,         "A(F)fine transform:"                    Tab "3 translations + 3 rotations + 3 scalings + 3 shearings",      "F",    true,   3,  3,  3,  3 },
        {   CoregistrationRotTransScale3Shear6,         "(A)ffine transform:"                    Tab "3 translations + 3 rotations + 3 scalings + 6 shearings",      "A",    true,   3,  3,  3,  6 },

        {   CoregistrationBrainTrans,                   "Brain to Brain, Origin (S)hift:"   Tab Tab "3 translations",                                               "S",    true,   3,  0,  0,  0 },
        {   CoregistrationBrainRotTrans,                "Brain to Brain, Rigid (B)ody:"     Tab Tab "3 translations + 3 rotations",                                 "B",    true,   3,  3,  0,  0 },
        {   CoregistrationBrainRotTransScale1,          "Brain to Brain, Global (T)ransform:"   Tab "3 translations + 3 rotations + 1 global scaling",              "T",    true,   3,  3,  1,  0 },
        {   CoregistrationBrainRotTransScale3,          "Brain to Brain, E(L)astic transform:"  Tab "3 translations + 3 rotations + 3 scalings",                    "L",    true,   3,  3,  3,  0 },
        {   CoregistrationBrainRotTransScale3Shear2,    "Brain to Brain, Affine transform:"     Tab "3 translations + 3 rotations + 3 scalings + (2) shearings",    "2",    true,   3,  3,  3,  2 },
        {   CoregistrationBrainRotTransScale3Shear3,    "Brain to Brain, Affine transform:"     Tab "3 translations + 3 rotations + 3 scalings + (3) shearings",    "3",    true,   3,  3,  3,  3 },
        {   CoregistrationBrainRotTransScale3Shear6,    "Brain to Brain, Affine transform:"     Tab "3 translations + 3 rotations + 3 scalings + (6) shearings",    "6",    true,   3,  3,  3,  6 },
        };


void    CoregistrationSpecsType::ToVerbose ( TVerboseFile& verbose )    const
{
verbose.Put ( "Transformation preset:",     Text              );
verbose.Put ( "Fully linear transform:",    IsLinearTransform );
                                        // detail transform
if ( NumTranslations > 0 )  verbose.Put ( "Number of Translations:",    NumTranslations );
if ( NumRotations    > 0 )  verbose.Put ( "Number of Rotations:",       NumRotations    );
if ( NumScalings     > 0 )  verbose.Put ( "Number of Scaling:",         NumScalings     );
if ( NumShearings    > 0 )  verbose.Put ( "Number of Shearing:",        NumShearings    );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::CoregistrationMrisUI  ()
{
crtl::CoregistrationMrisUI ( 0, 0 );    // default arguments
}


//----------------------------------------------------------------------------

void    CoregistrationMrisUI(   const TVolumeDoc*   SourceMri,
                                const TVolumeDoc*   TargetMri
                            )
{

TCartoolDocManager* DocManager      = CartoolObjects.CartoolDocManager;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( TargetMri == 0 ) {

    GetFileFromUser     filemri ( "MRI file to coregister to (target):", AllMriFilesFilter, 1, GetFileRead );

    if ( ! filemri.Execute () )
        return;

    TargetMri   = dynamic_cast< TVolumeDoc* > ( DocManager->OpenDoc ( filemri[ 0 ], dtOpenOptions ) );
    }


if ( TargetMri == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( SourceMri == 0 ) {

    GetFileFromUser     filemri ( "MRI file to be coregistered (source):", AllMriFilesFilter, 1, GetFileRead );

    if ( ! filemri.Execute () )
        return;

    SourceMri   = dynamic_cast< TVolumeDoc* > ( DocManager->OpenDoc ( filemri[ 0 ], dtOpenOptions ) );
    }


if ( SourceMri == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional MRI files to transform
static GetFileFromUser  getoptionalfiles ( "Optional files to transform, too:", AllMriPointsFilesFilter /*AllFilesFilter*/ /*AllMriFilesFilter "|" AllPointsFilesFilter*/, 1, GetFileMulti );

getoptionalfiles.Execute ();


TGoF&           gofoptionalfiles    = (TGoF&) getoptionalfiles;
TGoF            filestransfmris;
TGoF            filestransfspis;


if ( gofoptionalfiles.IsNotEmpty () ) {
                                        // check original source file is not this list again
    if ( gofoptionalfiles.Contains ( SourceMri->GetDocPath () ) )

        gofoptionalfiles.Remove ( SourceMri->GetDocPath () );

                                        // dispatch by types of files
    filestransfmris = gofoptionalfiles;
    filestransfspis = gofoptionalfiles;

    filestransfmris.GrepKeep ( AllMriFilesGrep,    GrepOptionDefaultFiles );
    filestransfspis.GrepKeep ( AllPointsFilesGrep, GrepOptionDefaultFiles );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // User input parameters
//char                    postfix[ 256 ];
//
//StringCopy ( postfix, "Coregistered" );
//
//if ( ! GetInputFromUser ( "Postfix transformed file names with:", MriCoregistrationTitle, postfix, postfix, this ) )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Matches the 2 objects exactly - other scenarios don't work well all the time
FitVolumeType       inclusionflags  = FitVolumeEqualSizes;

/*
                                        // Ask if either volume includes the other one, or if we should match them equally
                                        //  Head to MNI Head: Inclusive
                                        //  Brain to Brain  : Exact (or Inclusive + Fat Target)

                                        // Compare size in [mm], nbot voxels
double              boundratio      = (           TargetMri->GetBounding()->BoxDiameter () * TargetMri->GetVoxelSize ().Average () )
                                    / ( NonNull ( SourceMri->GetBounding()->BoxDiameter () * SourceMri->GetVoxelSize ().Average () ) );
char                inclusionhint[ 16 ];
char                buff[ 256 ];

StringCopy ( inclusionhint, boundratio < 0.80 ? "S" : boundratio > 1.25 ? "T" : "M" );
//DBGV ( boundratio, "boundratio" );


char                answer          = GetOptionFromUser ( "(S)ource volume includes the Target volume, or\n(T)arget volume includes the Source volume, or\nSource and Target volumes equally (M)atch?" , 
                                                          MriCoregistrationTitle, "S T M", inclusionhint );

if ( answer == EOS )   return;


inclusionflags  = answer == 'S' ? FitVolumeSourceBigger
                : answer == 'T' ? FitVolumeTargetBigger
                                : FitVolumeEqualSizes;
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Also forces same types
bool                sametypes       = true;
/*
char                sametypeshint[ 16 ];

if (   TargetMri->IsFullHead () &&   SourceMri->IsFullHead ()
  || ! TargetMri->IsFullHead () && ! SourceMri->IsFullHead () )
    StringCopy ( sametypeshint, "Yes" );
else
    StringCopy ( sametypeshint, "No" );

if ( ! GetInputFromUser ( "Are Source and Target volumes of the same types,\n like Brain / Brain or Head / Head?" , MriCoregistrationTitle, buff, sametypeshint ) )
    return;

sametypes   = StringToBool ( buff );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pick which transform to suggest to user
//bool                anyfullhead     = SourceMri->IsFullHead () || TargetMri->IsFullHead ();
//int                 transformmin    = anyfullhead ? CoregistrationRegularMin : CoregistrationRegularMin /*CoregistrationBrainMin*/;
//int                 transformmax    = anyfullhead ? CoregistrationRegularMax : CoregistrationBrainMax;

                                        // Not all coregistration are being offered to user's view
TSelection          coregforuser ( NumCoregistrationTypes, OrderSorted );

coregforuser.Set ( CoregistrationTrans                  );
coregforuser.Set ( CoregistrationRotTrans               );
coregforuser.Set ( CoregistrationRotTransScale1         );
coregforuser.Set ( CoregistrationRotTransScale3         );
coregforuser.Set ( CoregistrationRotTransScale3Shear6   );


char                pickcoregtypequestion[ NumCoregistrationTypes * 128 ];
char                pickcoregtype        [ NumCoregistrationTypes *   2 ];

ClearString     ( pickcoregtypequestion );
ClearString     ( pickcoregtype         );

for ( TIteratorSelectedForward ci ( coregforuser ); (bool) ci; ++ci ) {

    AppendSeparator ( pickcoregtypequestion, NewLine );
    StringAppend    ( pickcoregtypequestion, CoregistrationSpecs[ ci() ].Text   );

    AppendSeparator ( pickcoregtype, " " );
    StringAppend    ( pickcoregtype, CoregistrationSpecs[ ci() ].Choice );
    }


int                 corregspeci     = coregforuser.LastSelected ();
char                answer          = GetOptionFromUser ( pickcoregtypequestion, "Type of Coregistration", pickcoregtype, CoregistrationSpecs[ corregspeci ].Choice );

if ( answer == EOS )   return;


for ( int ci = 0; ci < NumCoregistrationTypes; ci++ )

    if ( CoregistrationSpecs[ ci ].Choice[ 0 ] == answer ) {

        corregspeci = ci;
        break;
        }

const CoregistrationSpecsType&  coregtype   = CoregistrationSpecs[ corregspeci ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              precision;

if ( ! GetValueFromUser ( "Precision of coregistration, from high (1e-6) to low (1e-2):" , MriCoregistrationTitle, precision, "1e-6" ) )
    return;

precision       = NoMore ( fabs ( precision ), 1e-2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                fileprefix[ 256 ];

if ( ! GetInputFromUser ( "Optional file name prefix:" , MriCoregistrationTitle, fileprefix, "" ) )
    return;


if ( StringIsNotEmpty ( fileprefix ) )
    StringAppend ( fileprefix, "." );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose the appropriate intensity remapping
RemapIntensityType  fromremap;
RemapIntensityType  toremap;


//if ( SourceMri->IsMask () )     fromremap       = RemapIntensityBinarize;
//else                            fromremap       = RemapIntensityEqualize;
//if ( TargetMri->IsMask () )     toremap         = RemapIntensityBinarize;
//else                            toremap         = RemapIntensityEqualize;

                                        // For Grey masks:
                                        // Exact > Inclusive
                                        // Mask > Binarize, with and without filters
                                        // Filter > No Filter for Mask
                                        // -> use Exact, Mask, Filters

                                        // if either one is a mask, then the other one should also be converted
                                        // also, both masks should be normalized
if ( SourceMri->IsMask () || TargetMri->IsMask () ) {
//  fromremap       = RemapIntensityBinarize;   // will binarize a grey and mask ,leaving holes in them
//  toremap         = RemapIntensityBinarize;
    fromremap       = RemapIntensityMask;       // will binarize a grey, will fill a mask
    toremap         = RemapIntensityMask;
    }
else {
    fromremap       = sametypes ? RemapIntensityRank : RemapIntensityNone;
    toremap         = sametypes ? RemapIntensityRank : RemapIntensityNone;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDirSource;
//TFileName           BaseDirTarget;
TFileName           FileNameSource;
TFileName           FileNameTarget;
TFileName           sourcecoregfile;
TFileName           targetcoregfile;
TFileName           verbosefile;

                                        // only directory
//StringCopy      ( BaseDirTarget,                TargetMri->GetDocPath () );
//RemoveFilename  ( BaseDirTarget );
                                        // only file name
StringCopy      ( FileNameTarget,               TargetMri->GetDocPath () );
GetFilename     ( FileNameTarget );

StringCopy      ( BaseDirSource,                SourceMri->GetDocPath () );
RemoveFilename  ( BaseDirSource );

StringCopy      ( FileNameSource,               SourceMri->GetDocPath () );
GetFilename     ( FileNameSource );


StringCopy      ( verbosefile,                  BaseDirSource,  "\\",   fileprefix );
StringAppend    ( verbosefile,                  FileNameSource,     InfixToCoregistered,    FileNameTarget );
AddExtension    ( verbosefile, FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some verbose could be helpful..

TVerboseFile        Verbose ( verbosefile, VerboseFileDefaultWidth );

Verbose.PutTitle ( "MRI Coregistration" );



Verbose.NextTopic ( "Input Files:" );
{

Verbose.Put ( "Source MRI:", (char *) SourceMri->GetDocPath () );

for ( int i = 0; i < (int) filestransfmris; i++ )
    Verbose.Put ( i == 0 ? "Additional MRI files:" : "", filestransfmris[ i ] );

for ( int i = 0; i < (int) filestransfspis; i++ )
    Verbose.Put ( i == 0 ? "Additional Points files:" : "", filestransfspis[ i ] );

Verbose.NextLine ();
Verbose.Put ( "Target MRI:", (char *) TargetMri->GetDocPath () );
}


Verbose.NextTopic ( "Parameters:" );
{
if      ( IsEqualSizes   ( inclusionflags ) )   Verbose.Put ( "Source and Target geometrical overlap:", "Exact match" );
else if ( IsTargetBigger ( inclusionflags ) )   Verbose.Put ( "Source and Target geometrical overlap:", "Target including Source" );
else if ( IsSourceBigger ( inclusionflags ) )   Verbose.Put ( "Source and Target geometrical overlap:", "Source including Target" );

//Verbose.Put ( "Source and Target content types:", sametypes ? "Equivalent" : "Different" );


Verbose.NextLine ();
Verbose.Put ( "Source intensity levels adjustment:", fromremap == RemapIntensityNone && ! sametypes ? "Adaptive Linear Scaling"
                                                                                                    : RemapIntensityNames[ fromremap ] );

Verbose.Put ( "Target intensity levels adjustment:", toremap   == RemapIntensityNone && ! sametypes ? "Adaptive Linear Scaling"
                                                                                                    : RemapIntensityNames[ toremap ] );

Verbose.NextLine ();
coregtype.ToVerbose ( Verbose );


Verbose.NextLine ();
Verbose.Put ( "Convergence precision:",     FloatToString ( precision ) );
}


Verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                outputmats;
TGoF                outputmris;
TGoF                outputpoints;
double              coregquality;
char                coregqualityopinion[ 256 ];


if ( IsInsideLimits ( coregtype.Code, CoregistrationBrainMin, CoregistrationBrainMax ) )

    CoregisterBrains(   SourceMri,          fromremap,
                        TargetMri,          toremap,
                        inclusionflags,
                        coregtype,
                        precision,
                        filestransfmris,    filestransfspis,
                        fileprefix,
                        outputmats,         outputmris,         outputpoints,
                        coregquality,       coregqualityopinion,
                        Interactive
                    );
else

    CoregisterMris  (   SourceMri,          fromremap,
                        TargetMri,          toremap,
                        inclusionflags,
                        coregtype,
                        GlobalNelderMead,
                        precision,
                        filestransfmris,    filestransfspis,
                        fileprefix,
                        outputmats,         outputmris,         outputpoints,
                        coregquality,       coregqualityopinion,
                        Interactive
                    );



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Verbose.NextTopic ( "Results:" );
{
Verbose.Put ( "Coregistration quality [%]:", coregquality, 1 );
Verbose.Put ( "Coregistration diagnostic :", coregqualityopinion );

//if ( ! sametypes ) {
//    Verbose.Put ( "Target / Source levels ratio:", exp ( govtov[ 0 ][ 0 ].Value ) );
//    Verbose.NextLine ( 2 );
//    }
}


Verbose.NextTopic ( "Output Files:" );
{
Verbose.Put ( "Verbose File (this):", verbosefile );

if ( (bool) outputmris ) {
    Verbose.NextLine ();
    for ( int i = 0; i < (int) outputmris; i++ )
        Verbose.Put ( i == 0 ? "Transformed MRI files:" : "", outputmris[ i ] );
    }

if ( (bool) outputpoints ) {
    Verbose.NextLine ();
    for ( int i = 0; i < (int) outputpoints; i++ )
        Verbose.Put ( i == 0 ? "Transformed Points files:" : "", outputpoints[ i ] );
    }

if ( (bool) outputmats ) {
    Verbose.NextLine ();
    for ( int i = 0; i < (int) outputmats; i++ )
        Verbose.Put ( i == 0 ? "Transformation matrix files:" : "", outputmats[ i ] );
//      Verbose.Put ( "Matrix of absolute transform:", outputmats[ 0 ] );
//      Verbose.Put ( "Matrix of relative transform:", outputmats[ 1 ] );
//      Verbose.Put ( "Matrix of absolute transform:", outputmats[ 2 ] );
//      Verbose.Put ( "Matrix of relative transform:", outputmats[ 3 ] );
    }
}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
