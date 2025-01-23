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

#include    "PreprocessMris.h"
#include    "TPreprocessMrisDialog.h"

#include    "System.h"
#include    "CartoolTypes.h"
#include    "Math.Resampling.h"
#include    "Strings.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TVolume.ReadWrite.h"
#include    "TExportVolume.h"
#include    "Geometry.TOrientation.h"

#include    "TVolumeDoc.h"

#include    "GlobalOptimize.Tracks.h"               // FunctionCenter
#include    "Volumes.SagittalTransversePlanes.h"    // SetSagittalPlaneMri, SetTransversePlaneMri

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // We are going to assume all volumes from gof are related to each others,
                                        // like head, brain, grey matter etc...
                                        // gauge shows a global step in the whole current file sequence
                                        // showsubprocess allows each sub-processing to also show up

                                        // Resizing value meaning:
                                        //   - ResizingDimensions:  value = target dimension
                                        //   - ResizingVoxels:      value = target voxel size
                                        //   - ResizingRatios:      value = downsampling ratio
void        PreprocessMris  (   const TGoF&         gofin,
                                MRISequenceType     mrisequence,
                                bool                isotropic,
                                ResizingType        resizing,           double              resizingvalue,
                                bool                anyresizing,        BoundingSizeType    targetsize,         const TPointInt&    sizeuser,
                                ReorientingType     reorienting,        const char*         reorientingstring,
                                bool                sagittalplane,      bool                transverseplane,
                                OriginFlags         origin,             const TPointDouble& arbitraryorigin,
                                SkullStrippingType  skullstripping,     bool                bfc,
                                const char*         infixfilename,
                                TGoF&               gofout,
                                TSuperGauge*        gauge,              bool                showsubprocess
                            )
{
gofout.Reset ();

if ( gofin.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           verbosefile;
TFileName           preprocfile;
TFileName           infixfile;
TFileName           coregfile;
TFileName           targetstring;
bool                userinfix       = StringIsNotEmpty ( infixfilename );
char                buff[ KiloByte ];

TOpenDoc<TVolumeDoc>    mridoc;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPointDouble        sourcevoxelsize;
TPointInt           sourcemrisize;
TPointInt           targetmrisize;
TPointDouble        mriorigin;
TPointDouble        sourceorigin;
TPointDouble        targetorigin;
char                orientationstring[ 4 ];
FilterTypes         filtertype;
InterpolationType   interpolate;
int                 numsubsampling;
FctParams           p;
//bool                orthotransform      = ! ( sagittalplane || transverseplane );   // transform is orthogonal if none of these planes get adjusted

TMatrix44           transform;


for ( int i = 0; i < (int) gofin; i++ ) {

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

    transform.SetIdentity ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    StringCopy      ( verbosefile,              gofin[ i ] );
    RemoveExtension ( verbosefile );
    StringAppend    ( verbosefile,              "." );
    if ( userinfix )
        StringAppend    ( verbosefile,          infixfilename,  " " );
    StringAppend    ( verbosefile,              "Preprocessing" );
    AddExtension    ( verbosefile,              FILEEXT_VRB );


    TVerboseFile    verbose ( verbosefile, VerboseFileDefaultWidth );

    verbose.PutTitle ( "MRI Preprocessing" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) For now, non-byte data types (short, float...) will be forcibly converted to unsigned byte at opening time
    if ( ! mridoc.Open ( gofin[ i ], OpenDocHidden ) )
        continue;


    ClearString     ( infixfile );


    verbose.NextTopic ( "Input File:" );
    {
    verbose.Put ( "Input file:", gofin[ i ] );
    verbose.Put ( "MRI Sequence Type:", MRISequenceNames[ mrisequence ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retrieve source MRI informations
    mridoc->GetSize     ( sourcemrisize );
    mriorigin           =  mridoc->GetOrigin     ();
    sourcevoxelsize     =  mridoc->GetVoxelSize  ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Correct any anisotropy?
    if ( isotropic ) {

        TPointDouble        sourcevoxelratio ( 1 );

                                        // is there any numerical difference between any directions?
        if ( ! sourcevoxelsize.IsIsotropic ( 0.01 ) ) {

                                        // don't allow less than ~ 1% difference between voxel sizes
            sourcevoxelratio.X  = RoundTo ( sourcevoxelsize.X / sourcevoxelsize.Min (), 0.01 );
            sourcevoxelratio.Y  = RoundTo ( sourcevoxelsize.Y / sourcevoxelsize.Min (), 0.01 );
            sourcevoxelratio.Z  = RoundTo ( sourcevoxelsize.Z / sourcevoxelsize.Min (), 0.01 );

                                        // finally an actual operation?
            if ( sourcevoxelratio != 1 ) {

                transform.Scale ( sourcevoxelratio.X, sourcevoxelratio.Y, sourcevoxelratio.Z, MultiplyLeft );
                                        // new voxel size is isotropic, and equal to the min voxel size
                sourcevoxelsize     = sourcevoxelsize.Min ();

                StringAppend ( infixfile, ".Iso" );
                }

            } // if ! IsIsotropic

        } // isotropic


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Any global size rescaling?

    double              scalingfactor   = 1;

                                        // Here we assume that voxels are isotropic
    if ( resizing != ResizingNone ) {

        if      ( resizing == ResizingDimensions ) {
                                        // getting the current dimension reference
            int                 refsize;
                                        // take the real max dimension
            refsize         = sourcemrisize.Max ();

    /*                                      // look at relative difference between all dimensions
            double              reldiffxy       = RelativeDifference ( sourcemrisize.X, sourcemrisize.Y );
            double              reldiffxz       = RelativeDifference ( sourcemrisize.X, sourcemrisize.Z );
            double              reldiffyz       = RelativeDifference ( sourcemrisize.Y, sourcemrisize.Z );

                                            // pick the 2 dims that are the closest, which is usually the main scan, which gives the reference dimension
            if      ( reldiffxy < reldiffxz && reldiffxy < reldiffyz )  refsize = max ( sourcemrisize.X, sourcemrisize.Y );
            else if ( reldiffxz < reldiffxy && reldiffxz < reldiffyz )  refsize = max ( sourcemrisize.X, sourcemrisize.Z );
            else                                                        refsize = max ( sourcemrisize.Y, sourcemrisize.Z );
    */
                                        // value = target dimension
            scalingfactor   = resizingvalue / refsize;
            }

        else if ( resizing == ResizingVoxels ) {
                                        // value = target voxel size
            scalingfactor   = sourcevoxelsize.Min () / resizingvalue;
            }

        else if ( resizing == ResizingRatios ) {
                                        // value = downsampling ratio
            scalingfactor   = 1 / NonNull ( resizingvalue );
            }

                                        // depending on option, prevent from upsizing...
        if ( ! anyresizing && scalingfactor > 1.0 )

            scalingfactor   = 1;

                                        // test for a meaningful resizing
        if (   anyresizing && RelativeDifference ( scalingfactor, 1 ) > 1e-3
          || ! anyresizing &&                      scalingfactor      < 1.0 /*&& refsize > resizingdim*/ ) {

            transform.Scale   ( scalingfactor, scalingfactor, scalingfactor, MultiplyLeft );

            if      ( resizing == ResizingDimensions )  StringAppend ( infixfile, ".Dim",  IntegerToString ( buff, resizingvalue ) );
            else if ( resizing == ResizingVoxels     )  StringAppend ( infixfile, ".Vox",  FloatToString   ( buff, resizingvalue, IsInteger ( resizingvalue     ) ? 0 : 3 ) );
            else if ( resizing == ResizingRatios     )  
                if ( resizingvalue >= 1 )               StringAppend ( infixfile, ".Down", FloatToString   ( buff, resizingvalue, IsInteger ( resizingvalue     ) ? 0 : 3 ) );
                else                                    StringAppend ( infixfile, ".Up",   FloatToString   ( buff, scalingfactor, IsInteger ( 1 / resizingvalue ) ? 0 : 3 ) );
            }

        } // resizing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    verbose.NextTopic ( "Resolution:" );
    {
    verbose.Put ( "Isotropic voxels:", isotropic );

    verbose.NextLine ();
    verbose.Put ( "Resampling:", resizing != ResizingNone );

    if ( resizing != ResizingNone ) {

        verbose.Put ( "Resampling method:", resizing == ResizingDimensions  ? "By dimension" 
                                          : resizing == ResizingVoxels      ? "By voxel size" 
                                          : resizing == ResizingRatios      ? "By ratio factor" 
                                          :                                   "Unknown resampling" );

        if      ( resizing == ResizingDimensions ) {

            verbose.Put ( "Resampling to dimension:", resizingvalue, 3, " [voxel]" );
            verbose.Put ( "Resulting voxel size:", sourcevoxelsize.Min () / scalingfactor, 3, " [mm]" );
            }

        else if ( resizing == ResizingVoxels )

            verbose.Put ( "Resampling to voxel size:", resizingvalue, 3, " [mm]" );

        else if ( resizing == ResizingRatios ) {

            if ( resizingvalue >= 1 )
                verbose.Put ( "Downsampling by factor:", resizingvalue, 3 );
            else
                verbose.Put ( "Upsampling by factor:", 1 / resizingvalue, 3 );

            verbose.Put ( "Resulting voxel size:", sourcevoxelsize.Min () / scalingfactor, 3, " [mm]" );
            }

//      verbose.Put ( "Resampling factor:", scalingfactor );
        } // resizing

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Setting origin, still in source space

                                        // check validity of MRI origin (not null, not vertex, not outside size, close to center)
    bool            mrioriginok         = mridoc->IsValidOrigin ( mriorigin );

                                        // no options set? use current MRI origin
    if      ( ! IsOrigin ( origin ) ) 
                                                        sourceorigin    = mriorigin;

                                        // setting origin?
    else if ( IsOriginSetAlways    ( origin ) 
           || IsOriginSetNoDefault ( origin ) && ! mrioriginok ) {

        if        ( IsOriginArbitrary ( origin ) )      sourceorigin    = arbitraryorigin;
        else /*if ( IsOriginCenter    ( origin ) )*/    sourceorigin    = mridoc->GetDefaultCenter ();

        }
                                        // there is an existing origin, and in that case, user instructed to keep it
    else if ( IsOriginSetNoDefault ( origin ) && mrioriginok )

                                                        sourceorigin    = mriorigin;

                                        // force resetting the target origin - option is not part of GUI for the moment
    else // if ( IsOriginReset ( origin ) )
                                                        sourceorigin.Reset ();


                                        // temp force behavior: if null source origin, then force null target origin
    if ( sourceorigin.IsNull () )

        origin  = OriginReset;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5.A) Reorientation?
                                        // We split here the transform into 2 parts:
                                        // - Local to RAS - applied now, so that Sagittal search can operate
                                        // - RAS to Any   - applied AFTER the Sagittal search, if needed only

                                        // First part: always going to Local -> RAS, so any intermediate processing will be OK (Transverse, Geometrical center, etc...)
    transform.Multiply ( mridoc->GetStandardOrientation ( LocalToRAS ), MultiplyLeft );

                                        // Second part
    TMatrix44           reorientaftersag;


    if      ( reorienting == ReorientingRAS ) {
                                        // No transform applied after Sagittal search
        reorientaftersag.SetIdentity ();


        StringCopy      ( orientationstring, "RAS" );
        StringAppend    ( infixfile,    ".", orientationstring );
        }


    else if ( reorienting == ReorientingArbitrary ) {
                                        // here, orientation string is supposed to be legal
        TOrientation        orientobject ( reorientingstring );
                                        // RAS -> Any
        reorientaftersag      = orientobject.GetStandardOrientation ( RASToLocal );


        StringCopy      ( orientationstring, reorientingstring );
        StringAppend    ( infixfile,    ".", orientationstring );
        }

                                        // No forced reorientation
    else if ( reorienting == ReorientingNone ) {

                                        // Transform back to original: RAS -> Local
        reorientaftersag      = mridoc->GetStandardOrientation ( RASToLocal );

                                        
        mridoc->OrientationToString ( orientationstring );          // if no rotations, we can simply clone the input orientation
//      StringAppend    ( infixfile,    ".", orientationstring );   // not appending, input file is unchanged
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    verbose.NextTopic ( "Orientation:" );
    {
    verbose.Put ( "Axis reorientation:", reorienting != ReorientingNone );

    if      ( reorienting == ReorientingRAS ) {

        verbose.Put ( "Reorienting to:", "MNI orientation" );
        verbose.Put ( "X axis pointing toward:", "R" );
        verbose.Put ( "Y axis pointing toward:", "A" );
        verbose.Put ( "Z axis pointing toward:", "S" );
        }

    else if ( reorienting == ReorientingArbitrary ) {

        verbose.Put ( "Reorienting to:", "Arbitrary orientation" );
        verbose.Put ( "X axis pointing toward:", reorientingstring[ 0 ] );
        verbose.Put ( "Y axis pointing toward:", reorientingstring[ 1 ] );
        verbose.Put ( "Z axis pointing toward:", reorientingstring[ 2 ] );
        }


    if ( reorienting != ReorientingNone ) {
        verbose.NextLine ();
        verbose.Put ( "Mid-Sagittal plane adjustment:", sagittalplane );
        verbose.Put ( "MNI Transverse Plane adjustment:", transverseplane );
        }
    }


    verbose.NextTopic ( "Origin:" );
    {
    verbose.Put ( "Setting new origin:",    IsOriginSetAlways ( origin )    ?   "Always" 
                                          : IsOriginSetNoDefault ( origin ) ?   "Only by default" 
                                          : IsOriginReset ( origin )        ?   "Resetting" 
                                          :                                     "No" );

    if ( IsOrigin ( origin ) )
        verbose.Put ( "New origin position:",   IsOriginMni ( origin )      ?   "MNI origin" 
                                              : IsOriginCenter ( origin )   ?   "Brain geometrical center" 
                                              : IsOriginArbitrary ( origin )?   "Specified position" 
                                              : IsOriginReset ( origin )    ?   "(0,0,0) [voxel]" 
                                              :                                 "Somewhere" );

    if ( IsOriginArbitrary ( origin ) ) {
        sprintf ( buff, "(%g,%g,%g) [voxel]", arbitraryorigin.X, arbitraryorigin.Y, arbitraryorigin.Z );
        verbose.Put ( "Specified origin, from input space:", buff );
        }
    }


    verbose.NextTopic ( "Options:" );
    {
    verbose.Put ( "Skull-Stripping full head:", skullstripping != SkullStrippingNone );

    if ( skullstripping != SkullStrippingNone ) {
        verbose.Put ( "Skull-Stripping method:", SkullStrippingNames[ skullstripping ] );
        verbose.Put ( "Brain Bias Field Correction (BFC):",   bfc );
        }
    }


    verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Searching for sagittal plane:
                                        // - needs a center
                                        // - needs some reorientation transform to RAS space
                                        // Search for Sagittal plane will work
                                        // - even without reorientation
                                        // - even if final space is not RAS

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( sagittalplane ) {

                                        // we REALLY need a center - if no parameters have set it already, we do it right now
        if ( sourceorigin.IsNull () /*! mridoc->IsValidOrigin ( sourceorigin )*/ )
            if ( mrioriginok )  sourceorigin    = mriorigin;
            else                sourceorigin    = mridoc->GetDefaultCenter ();  // in last resort!


        SetSagittalPlaneMri (   mridoc, 
                                transform,          // input:  Local -> Rescaling -> RAS
                                transform,          // input & output: RAS -> Local -> Translate
                                sourceorigin,       // input & output: LocalCenter
                                0,
                                showsubprocess ? "Sagittal Plane Search" : 0
                            );

                                        // we want the other way round...
                                        // Translate(-LocalCenter) -> RASOrient (centered) -> Rotates(RAS)
        transform.Invert ();

        StringAppend    ( infixfile, ".", InfixSagittal );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 7) Needing a temp brain (Transverse transform, geometrical center...)?
                                        // - save to temp file
                                        // - extract brain - even if no user Skull-Stripping flag
                                        // - retrieve info, compose to global transform
                                        // - delete temp files
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    TOpenDoc<TVolumeDoc>    temprasdoc;
    TFileName               temprasfile;
    TPointDouble            temprasdocorigin;

                                        // we need a temp copy to later extract the brain in these cases:
    if ( transverseplane || IsOriginCenter ( origin ) ) {

        StringCopy      ( temprasfile, gofin[ i ] );
        RemoveExtension ( temprasfile );
        StringAppend    ( temprasfile, infixfile, ".Tmp" );
        StringRandom    ( StringEnd ( temprasfile ), 5 );
        AddExtension    ( temprasfile, DefaultMriExt );


                                        // We set the parameters to be as fast as possible - actually not used if numsubsampling == 1
        filtertype      = mridoc->IsMask () ? FilterTypeMedian           : FilterTypeNone;

        interpolate     = mridoc->IsMask () ? InterpolateNearestNeighbor 
                        : sagittalplane     ? InterpolateLanczos3           // better for non 90 degrees rotations(?)
                        :                     InterpolateLinear;

        numsubsampling  = 1;
                                        // transform origin into the new space
        targetorigin    = sourceorigin;

        transform.Apply ( targetorigin );


                                        // apply transform and save to file
        mridoc->GetData ()->TransformAndSave (  transform,
                                                TransformAndSaveFlags ( TransformToTarget | TransformSourceRelative | TransformTargetRelative ),
                                                filtertype,      interpolate,    numsubsampling,
                                                &sourceorigin,   &targetorigin, 
                                                mridoc->GetVoxelSize (),    0,
                                                BoundingSizeOptimal,        0,
                                                "RAS",
                                                NiftiTransformDefault,      NiftiIntentCodeDefault,     NiftiIntentNameDefault,
                                                temprasfile,                0,  0,
                                                showsubprocess ? "Transforming MRI" : 0 
                                            );


        temprasdoc.Open     ( temprasfile, OpenDocHidden );


        if ( temprasdoc.IsOpen () )
            temprasdocorigin    = temprasdoc->GetOrigin ();

        } // if istempras

    else
        temprasfile.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 8) Temp file to Temp Brain?
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // guess from content and file name
    bool                isfullhead      = true; // mridoc->IsFullHead ();   // !detection could fail for some weirdly distributed MRIs!


    if ( temprasdoc.IsOpen () && isfullhead ) {
                                        // we can use the tempdoc data
        Volume*         tempbrain       = temprasdoc->GetData ();


//      p ( FilterParamSkullStrippingMethod )       = SkullStripping2;                                          // force method in all cases
        p ( FilterParamSkullStrippingMethod )       = skullstripping != SkullStrippingNone ? skullstripping : SkullStripping2;  // use user's selected method, it might be relevant to the current MRI
                                        // !NOT up-to-date, to be all re-tested!
//      p ( FilterParamSkullStrippingMethod )       = mrisequence == MRISequenceUnknown    ? SkullStripping1B    // SkullStripping1 is faster
//                                                  : mrisequence == MRISequenceT1         ? SkullStripping2     // Usually best results for T1, but it seems SkullStripping1A is also doing fine
//                                                  : mrisequence == MRISequenceT1Gad      ? SkullStripping1B    // Gadolinium case
//                                                  :                                        SkullStripping1B;

        p ( FilterParamSkullStrippingVoxelSize  )   = temprasdoc->GetVoxelSize ().Average ();
        p ( FilterParamSkullStrippingIsTemplate )   = IsTemplateFileName ( gofin[ i ] );        // for method 1 - template file has some specific tuning

        tempbrain->Filter ( FilterTypeSkullStripping, p, showsubprocess );

                                        // we changed the content type
        StringCopy  ( temprasdoc->GetNiftiIntentName (), NiftiIntentNameBrain );

                                        // Save brain in RAS file - not really needed, but useful for intermediate checks
        temprasdoc->Commit ( true );

                                        // Recomputing background, limits and stuff...
        temprasdoc->ResetThenSetMri ();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 10.A) Transverse Plane search, in RAS local brain space
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    TMatrix44           RASTransverse;
    TPointDouble        deltaorigin;

                                        // there needs to include the translation - the sagittal processing one is lost at that point
    if ( RASTransverse.HasNoTranslation ()  )
        RASTransverse.SetTranslation ( temprasdocorigin );

                                        // are we good for Transverse search?
    if ( temprasdoc.IsOpen () && transverseplane ) {

        TPointDouble        mnineworigin    = temprasdocorigin;

                                        // Note that it doesn't rescale, although some rescaling is done during the coregistration
        SetTransversePlaneMri   (   temprasdoc, 
                                    temprasdoc->GetStandardOrientation ( LocalToRAS ),  // input:  Local -> RAS (actually Identity here)
                                    RASTransverse,                                      // input & output: Rotate -> RAS -> Local -> Translate
                                    mnineworigin,                                       // input & output: LocalCenter
                                    0,
                                    showsubprocess ? "Transverse Plane" : 0
                                );

                                        // Translate(-LocalCenter) -> Local -> RAS -> -Rotate
        RASTransverse.Invert ();

                                        // we need this to account from the shift in origin between the before and after search
        if      ( IsOriginMni ( origin ) )

            deltaorigin     = mnineworigin - temprasdocorigin;
        else
            deltaorigin.Reset ();


        StringAppend    ( infixfile, ! sagittalplane ? "." : "", InfixTransverse );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 10.B) Geometrical Center?
                                        // Geometrical Center is now independent of Transverse Plane search
                                        // Also note that Geometrical Center has already been set from the whole original file, this will simply update it
    if ( IsOriginCenter ( origin ) ) {
                                        // Evaluate new barycenter, either after brain extraction or from original segmented MRI
        TPointDouble        geomcenter  = ( temprasdoc.IsOpen () ? temprasdoc : mridoc )->GetBarycenter ();
                                        // that will be the new shift
        if ( temprasdoc.IsOpen () )
            deltaorigin     = geomcenter - temprasdocorigin;
        }

                                        // in all cases, make sure we don't change the sagittal plane
    deltaorigin.X   = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we're done with the temp file
    if ( temprasdoc.IsOpen () ) {
                                        // Transverse plane is done, get rid of temp file
        temprasdoc.Close ();
                                        // delete temp files
        DeleteFileExtended  ( temprasfile );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 11) Setting origin, in target space

                                        // resetting is really setting null origin, after transform - NOT used for the moment
                                        // !except it doesn't work, as TransformAndSave will shift the origin anyway!
                                        // Also note that, for the moment, the Reset option is NOT available from the UI...
    if ( IsOriginReset ( origin )
      || sourceorigin.IsNull () )       // assume a null source origin, at that point, means it is useless -> reset the target origin, too, as to avoid a weird corner origin

        targetorigin.Reset ();

    else {
                                        // transform origin into the new space
        if ( ! IsOrigin ( origin ) )
            targetorigin    = mriorigin;    // weird case: no origin setting, keep input one
        else
            targetorigin    = sourceorigin;

                                        // decompose the final transform..
        transform       .Apply ( targetorigin );
                                        // ..so we can introduce some shift in-between (the one occuring from the Transverse Plane search)
        targetorigin   += deltaorigin;

        RASTransverse   .Apply ( targetorigin );

        reorientaftersag.Apply ( targetorigin );

        targetorigin.RoundTo ( 0.1 );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 12) Compose full transform from original MRI down to fully reoriented stuff

    transform   = reorientaftersag * RASTransverse * transform;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cropping is done at the end of the transform
//  if ( targetsize == BoundingSizeOptimal )
//
//      StringAppend    ( infixfile, ".Crop" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Can compute target size now according to options

    if      ( targetsize == BoundingSizeOptimal )
        ;                               // nothing to be done, function will compute the optimal size by itself

    else if ( targetsize == BoundingSizeGiven ) {

        if ( sizeuser.IsNull() ) {      // used to specify Transformed Size
                                        
            targetmrisize   = sourcemrisize;
                                        // output size is proportional to input size
            transform.ApplyRange ( targetmrisize );
            }
        else 
                                        // OK, use this specified size as is - function will crop / pad while centering the data
            targetmrisize   = sizeuser;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 13) Final processing and save to file
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // Set filename, as result will be written to a file
    StringCopy          ( preprocfile, gofin[ i ] );
    RemoveExtension     ( preprocfile );
                                        // cooking the output file name according to options and default values
    if      ( userinfix )                       StringAppend    ( preprocfile, ".", infixfilename   );
    else if ( StringIsNotEmpty ( infixfile ) )  StringAppend    ( preprocfile, infixfile            );
    else                                        StringAppend    ( preprocfile, ".Preproc"           );
                                        // we know for sure it is a full head if we run some skull-stripping
    if ( skullstripping != SkullStrippingNone && isfullhead && ! StringContains ( (const char*) preprocfile, "." InfixHead ) )
        StringAppend    ( preprocfile, "." InfixHead );

    AddExtension        ( preprocfile, DefaultMriExt );

    CheckNoOverwrite    ( preprocfile );


                                        // Tuning the filtering and subsampling
    filtertype      = mridoc->IsMask () ? FilterTypeMedian           : FilterTypeMean;      // FilterTypeAntialiasing?

    interpolate     = mridoc->IsMask () ? InterpolateNearestNeighbor 
                    :                     InterpolateLanczos3;          // slower, but looks better at the end, even for upscaling
//                  : orthotransform    ? InterpolateLinear
//                  :                     InterpolateLanczos3;          // better for non 90 degrees rotations(?)

//  numsubsampling  = AtLeast ( 1, RoundAbove ( 1 / scalingfactor ) );  // scaling factor gives us the amount of subsampling needed, mask or not
    numsubsampling  = AtLeast ( 1, Round      ( 1 / scalingfactor ) );  // if too close from the original resolution, forcing subsampling makes it a low-pass filtering, which might not be visually pleasant

                                        // apply transform and save to file
    mridoc->GetData ()->TransformAndSave (  transform,       
                                            TransformAndSaveFlags (                               TransformToTarget 
                                                                   |                              TransformSourceRelative 
                                                                   | ( IsOriginReset ( origin ) ? TransformTargetReset 
                                                                                                : TransformTargetRelative ) ),
                                            filtertype,      interpolate,    numsubsampling,
                                            &sourceorigin,   &targetorigin, 
                                            mridoc->GetVoxelSize (),    0,
                                            targetsize,      &targetmrisize,
                                            orientationstring,
                                            AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ),    mridoc->GetNiftiIntentCode (),  mridoc->GetNiftiIntentName (),
                                            preprocfile,    0,          0,
                                            showsubprocess ? "Transforming MRI" : 0 
                                        );

    gofout.Add ( preprocfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 14) Extracting brain for real, from final orientation and resampling
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    TOpenDoc<TVolumeDoc>    postprocdoc;
    TFileName               brainfile;
    Volume                  brain;

                        // skip step if the MRI does not appear to be a full head
    if ( skullstripping != SkullStrippingNone && isfullhead ) {


        postprocdoc.Open ( preprocfile, OpenDocHidden );

                                        // copy full head into future brain volume    
        brain           = *postprocdoc->GetData ();

                                        // get current voxelsize
        sourcevoxelsize = postprocdoc->GetVoxelSize ();

        p ( FilterParamSkullStrippingMethod     ) = skullstripping;                       // method choice
        p ( FilterParamSkullStrippingVoxelSize  ) = sourcevoxelsize.Average ();
        p ( FilterParamSkullStrippingIsTemplate ) = IsTemplateFileName ( gofin[ i ] );    // this works if new name has not altered the very beginning of the file name

        brain.Filter    ( FilterTypeSkullStripping, p, showsubprocess );


        StringCopy      ( brainfile, preprocfile );

                                        // try to be smart, and avoid having something like .Head.Brain
        if ( StringContains ( (const char*) brainfile, "." InfixHead ) )    StringReplace   ( brainfile, "." InfixHead, "." InfixBrain );
        else                                                                PostfixFilename ( brainfile, "." InfixBrain );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 15) BFC on brain?
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // applies BFC only if brain has been extracted (possibly loaded?)
    if ( skullstripping != SkullStrippingNone && brain.IsAllocated () && bfc ) {

        p ( FilterParamBiasFieldRepeat )     = 1;
                                        // if we really needed, we could save / retrieve the correction volume and apply it to the whole head MRI(?)
        brain.Filter ( FilterTypeBiasField, p, showsubprocess );


/*                                      // direct call, so to retrieve the correction factors
        TVolume<float>      bfcfactor;
        brain.FilterBiasField ( p, &bfcfactor, showsubprocess );

                                        // apply to head - code is working but problem is:
                                        // correction is computed around the brain, so it does not exist beyond the brain's bounding box
        Volume*             head        = postprocdoc->GetData ();

        for ( int i = 0; i < head->GetLinearDim (); i++ )

//            (*head)[ i ]    = bfcfactor[ i ] ? (*head)[ i ] / bfcfactor[ i ] : 0;
                                        // !function returned the inverse of correction!
            (*head)[ i ]    = bfcfactor[ i ] ? (*head)[ i ] / bfcfactor[ i ] : (*head)[ i ];


        postprocdoc->Commit ( true );
*/
                                        // avoid spoiling brain file name
        if ( ! userinfix )      PostfixFilename ( brainfile, "." InfixBiasFieldCorrection );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 16) Time to save da brain in RAS
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( brain.IsAllocated () ) {
                                        // !in case if loading a brain, set sourcevoxelsize & brainfile!
                                        // Exact same space as temp Sagittal RAS head
        brain.WriteFile ( brainfile, &postprocdoc->GetOrigin (), &sourcevoxelsize, &postprocdoc->GetRealSize (), orientationstring,
                          AtLeast ( NiftiTransformDefault, mridoc->GetNiftiTransform () ),    mridoc->GetNiftiIntentCode (),  NiftiIntentNameBrain
                        );

        gofout.Add ( brainfile );

//      PostfixFilename ( brainfile, ".BFCFactor" );
//      bfcfactor.WriteFile ( brainfile, &postprocdoc->GetOrigin (), &sourcevoxelsize, &postprocdoc->GetRealSize (), orientationstring );
        }



    verbose.NextTopic ( "Output Files:" );
    {
    verbose.Put ( "Verbose file (this):", verbosefile );

    verbose.NextLine ();
    verbose.Put ( "Number of output files:", gofout.NumFiles () );

    for ( int fi = 0; fi < gofout.NumFiles (); fi++ )
        verbose.Put ( fi ? "" : "Output file:", gofout[ fi ] );

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
