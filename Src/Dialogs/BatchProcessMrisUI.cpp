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

#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TVolumeDoc.h"

#include    "GlobalOptimize.Tracks.h"   // FunctionCenter

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define             PreProcessMrisTitle     "MRIs to process"


void    TCartoolMdiClient::BatchProcessMrisUI ( owlwparam w )
{
static GetFileFromUser  getfiles ( PreProcessMrisTitle ":", AllMriFilesFilter, 1, GetFileMulti );

if ( ! getfiles.Execute () )
    return;


TFileName               newfile;
TFileName               base;

if ( ! GetInputFromUser ( "Specify an optional Base File Name suffix, or empty for none:", PreProcessMrisTitle, base, "", this ) )
    return;

if ( StringIsNotEmpty ( base ) )
    StringPrepend   ( base, "." );


TOpenDoc<TVolumeDoc>    mridoc;
char                orientation[ 4 ];
FctParams           p;
bool                batchmode       = (int) getfiles > 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Enter any needed parameters
                                        // !This is a copy from TVolumeDoc::Filter, so at one point, it should be done in only one place!

if      ( w == CM_SKULLSTRIPPING ) {

    p ( FilterParamSkullStrippingMethod )     = GetSkullStrippingTypeFromUser ( FilterPresets[ FilterTypeSkullStripping ].Text, SkullStrippingDialogDefault, this );
        
    if ( p ( FilterParamSkullStrippingMethod ) == SkullStrippingNone )
        return;
    }

else if ( w == CM_CUTBRAINSTEM   ) {

//  p ( FilterParamBrainstemMethod )    = -1;                   // not used
    }

else if ( w == CM_FILTERBIASFIELD ) {

    p ( FilterParamBiasFieldRepeat )     = 1;
    }

else if ( w == CM_FILTERSEGMENTCSF ) {

    }

else if ( w == CM_FILTERSEGMENTGREY ) {

    int                 g;

    if ( ! GetValueFromUser ( "Thickness type  1:Thin / 2:Regular / 3:Fat / 4:Whole brain :", FilterPresets[ FilterTypeSegmentGrey ].Text, g, "2", this ) )
        return;

    if ( g < 0 || g > 4 )   return;

    p ( FilterParamGreyType )     = ( g == 1 ? GreyMatterSlim
                                    : g == 2 ? GreyMatterRegular
                                    : g == 3 ? GreyMatterFat
                                    : g == 4 ? GreyMatterWhole
                                    :          GreyMatterRegular );

    bool                bf  = g != 4 ? GetAnswerFromUser ( "Correct for Bias Field beforehand?", FilterPresets[ FilterTypeSegmentGrey ].Text, this ) : false;

    p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) | ( bf ? GreyMatterBiasField : 0 );
    p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterAsymmetric;
    p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterPostprocessing;

//    p ( FilterParamGreyAxis )     = GetAxisIndex ( LeftRight );                   // for symmetric case
//    p ( FilterParamGreyOrigin )   = GetOrigin ()[ GetAxisIndex ( LeftRight ) ] ? GetOrigin ()[ GetAxisIndex ( LeftRight ) ]
//                                                                               : Bounding->GetCenter ()[ GetAxisIndex ( LeftRight ) ];    // risky if center is not the symmetric pivot!
    }

else if ( w == CM_FILTERSEGMENTWHITE ) {

    if ( ! GetValueFromUser ( "Details level 0..100:", FilterPresets[ FilterTypeSegmentWhite ].Text, p ( FilterParamWhiteDetails ), "", this ) )
        return;

    p ( FilterParamWhiteDetails )    /= 100;
    if ( p ( FilterParamWhiteDetails ) < 0 )  return;
    }

else if ( w == CM_FILTERSEGMENTTISSUES ) {

    bool                bf  = GetAnswerFromUser ( "Correct for Bias Field beforehand?", FilterPresets[ FilterTypeSegmentTissues ].Text, this );

    p ( FilterParamTissuesParams )     = bf ? GreyMatterBiasField : 0;
    }

else if ( w == CM_DOWNSAMPLEMRI ) {

    if ( ! GetValueFromUser ( "Give the downsampling ratio (integer value and min of 1):", "Downsampling MRIs", p ( 0 ), "2", this ) || p ( 0 ) <= 1 )
        return;

    p ( 0 )     = Round ( p ( 0 ) );
    }

else if ( w == CM_HEADCLEANUP ) {

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "MRI File", (int) getfiles, SuperGaugeLevelInter /*SuperGaugeLevelBatch*/ );


for ( int i = 0; i < (int) getfiles; i++ ) {

    Gauge.Next ();


    if ( ! mridoc.Open ( getfiles[ i ], OpenDocHidden ) )
        continue;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    StringCopy          ( newfile, getfiles[ i ] );


    if      ( w == CM_SKULLSTRIPPING    ) {
                                        // remove text that explicitly describes a full head
        if      ( StringContains ( (const char*) newfile, ".Head."        ) )   StringReplace ( newfile, ".Head.",      "." );
        else if ( StringContains ( (const char*) newfile, ".Full Head."   ) )   StringReplace ( newfile, ".Full Head.", "." );
        else if ( StringContains ( (const char*) newfile, ".FullHead."    ) )   StringReplace ( newfile, ".FullHead.",  "." );
        else if ( StringContains ( (const char*) newfile, " Head "        ) )   StringReplace ( newfile, " Head ",      ""  );
        else if ( StringContains ( (const char*) newfile, "_Head"         ) )   StringReplace ( newfile, "_Head",       ""  );
        else if ( StringContains ( (const char*) newfile, "_pro"          ) )   StringReplace ( newfile, "_pro",        ""  );
        }


    RemoveExtension     ( newfile );

    StringAppend        ( newfile, base );

    if      ( w == CM_SKULLSTRIPPING        )   StringAppend    ( newfile, "." InfixBrain  );
    else if ( w == CM_CUTBRAINSTEM          )   StringAppend    ( newfile, "." "NoBrainstem"  );
    else if ( w == CM_FILTERBIASFIELD       )   StringAppend    ( newfile, "." InfixBiasFieldCorrection );
    else if ( w == CM_FILTERSEGMENTCSF      )   StringAppend    ( newfile, "." InfixCsf   );
    else if ( w == CM_FILTERSEGMENTGREY     )   StringAppend    ( newfile, "." InfixGrey   );
    else if ( w == CM_FILTERSEGMENTWHITE    )   StringAppend    ( newfile, "." InfixWhite  );
    else if ( w == CM_FILTERSEGMENTTISSUES  )   StringAppend    ( newfile, "." InfixTissues );
    else if ( w == CM_DOWNSAMPLEMRI         )   StringAppend    ( newfile, "." "Down", IntegerToString ( p ( 0 ) ) );
    else if ( w == CM_HEADCLEANUP           )   StringAppend    ( newfile, ".", FilterPresets[ FilterTypeHeadCleanup ].Ext );

    AddExtension        ( newfile, DefaultMriExt );

    CheckNoOverwrite    ( newfile );


    mridoc->OrientationToString ( orientation );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // have a local copy of volume
    TPointDouble        voxelsize ( mridoc->GetVoxelSize () );


    if      ( w == CM_SKULLSTRIPPING ) {
        Volume          volumecopy ( *mridoc->GetData () );

        p ( FilterParamSkullStrippingVoxelSize  ) = voxelsize.Average ();
        p ( FilterParamSkullStrippingIsTemplate ) = IsTemplateFileName ( getfiles[ i ] ); // this works if new name has not altered the very beginning of the file name

        volumecopy.Filter ( FilterTypeSkullStripping, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), mridoc->GetNiftiIntentCode (), NiftiIntentNameBrain
                                );
        }

    else if ( w == CM_CUTBRAINSTEM   ) {
        Volume          volumecopy ( *mridoc->GetData () );

//      p ( FilterParamBrainstemMethod    ) = -1;
        p ( FilterParamBrainstemVoxelSize ) = voxelsize.Average ();

        volumecopy.Filter ( FilterTypeBrainstemRemoval, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), mridoc->GetNiftiIntentCode (), mridoc->GetNiftiIntentName ()
                                );
        }

    else if ( w == CM_FILTERBIASFIELD ) {
        Volume          volumecopy ( *mridoc->GetData () );

        volumecopy.Filter ( FilterTypeBiasField, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), mridoc->GetNiftiIntentCode (), mridoc->GetNiftiIntentName ()
                                );
        }

    else if ( w == CM_FILTERSEGMENTCSF ) {
        Volume          volumecopy ( *mridoc->GetData () );

        volumecopy.Filter ( FilterTypeSegmentCSF, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), NiftiIntentCodeGreyMask, NiftiIntentNameGreyMask
                                );
        }

    else if ( w == CM_FILTERSEGMENTGREY ) {
        Volume          volumecopy ( *mridoc->GetData () );

        volumecopy.Filter ( FilterTypeSegmentGrey, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), NiftiIntentCodeGreyMask, NiftiIntentNameGreyMask
                                );
        }

    else if ( w == CM_FILTERSEGMENTWHITE ) {
        Volume          volumecopy ( *mridoc->GetData () );

        volumecopy.Filter ( FilterTypeSegmentWhite, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), NiftiIntentCodeGreyMask, NiftiIntentNameGreyMask
                                );
        }

    else if ( w == CM_FILTERSEGMENTTISSUES ) {
        Volume          volumecopy ( *mridoc->GetData () );
                                        // also forcing some smoothing?
//      p ( FilterParamTissuesParams )     = ( (GreyMatterFlags) p ( FilterParamTissuesParams ) ) | GreyMatterGaussian;

        volumecopy.Filter ( FilterTypeSegmentTissues, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), NiftiIntentCodeLabels, NiftiIntentNameLabels
                                );
        }

    else if ( w == CM_DOWNSAMPLEMRI ) {
                                        // use the ExportDownsampled from BaseMri
        mridoc->ExportDownsampled   ( newfile, (int) p ( 0 ) );
        }

    else if ( w == CM_HEADCLEANUP ) {
        Volume          volumecopy ( *mridoc->GetData () );

        p ( FilterParamHeadCleanupVoxelSize  )  = voxelsize.Average ();

        volumecopy.Filter ( FilterTypeHeadCleanup, p, true /*! batchmode*/ );

        volumecopy.WriteFile    ( newfile, &mridoc->GetOrigin (), &voxelsize, &mridoc->GetRealSize (), orientation,
                                  AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ), mridoc->GetNiftiIntentCode (), mridoc->GetNiftiIntentName ()
                                );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary opening file
    if ( ! batchmode )
        newfile.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    mridoc.Close ();

    UpdateApplication;
    }


//Gauge.Finished ();
Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
