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

#include    "Volumes.Coregistration.h"

#include    "Math.Histo.h"
#include    "Math.TMatrix44.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Dialogs.TSuperGauge.h"

#include    "GlobalOptimize.Points.h"       // GlobalOptimizeMaxPoints, geometrical transform enums
#include    "GlobalOptimize.Volumes.h"
#include    "Volumes.SagittalTransversePlanes.h"

#include    "TElectrodesDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "TVolumeDoc.h"

#include    "TVolumeView.h"
#include    "TSolutionPointsView.h"
#include    "TElectrodesView.h"

#include    "CoregistrationMrisUI.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    CoregisterMris      (   const TVolumeDoc*   SourceMri,  RemapIntensityType  fromremap,
                                const TVolumeDoc*   TargetMri,  RemapIntensityType  toremap,
                                FitVolumeType       inclusionflags,
                                const CoregistrationSpecsType&  coregtype,
                                GOMethod            method,
                                double              precision,
                                const TGoF&         buddymris,  const TGoF&         buddypoints,
                                const char*         fileprefix,
                                TGoF&               outputmats, TGoF&               outputmris, TGoF&               outputpoints,
                                double&             quality,    char*               qualityopinion,
                                VerboseType         verbose
                            )
{
                                        // Coregistration can take quite some time - forbid closing these docs
                                        // bypassing the const'ness as it doesn't really modify the doc, this just a flag
const_cast< TVolumeDoc* > ( SourceMri )->PreventClosing ();
const_cast< TVolumeDoc* > ( TargetMri )->PreventClosing ();


outputmats  .Reset ();
outputmris  .Reset ();
outputpoints.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Be nice: show the 2 windows side to side
TVolumeView        *sourceview      =  dynamic_cast< TVolumeView * > ( SourceMri->GetViewList () );
TVolumeView        *targetview      =  dynamic_cast< TVolumeView * > ( TargetMri->GetViewList () );
int                 targetwidth;
int                 targetheight;
int                 targetleft;
int                 targettop;


if ( targetview && sourceview ) {
                                        // modify their positions
    targetwidth     =
    targetheight    = min ( GetWindowWidth ( CartoolObjects.CartoolMainWindow->GetClientWindow () ), GetWindowHeight ( CartoolObjects.CartoolMainWindow->GetClientWindow () ) ) / 2;

    targetview->WindowRestore();
    targetview->WindowSetPosition ( 0, 0, targetwidth, targetheight );

    sourceview->WindowRestore();
    sourceview->WindowSetPosition ( targetview->GetWindowRight () + 1,  targetview->GetWindowTop (),
                                    targetwidth,                        targetheight );

    targetleft  = targetview->GetWindowLeft   ();
    targettop   = targetview->GetWindowBottom () + 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setup the search
TFitVolumeOnVolume  govtov (    SourceMri,      fromremap,  0,
                                TargetMri,      toremap,    0,
                                inclusionflags 
                            );
TEasyStats          govtovq;

govtov.Set ( GOStepsDefault );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scaling
const TBoundingBox<double>* boundfrom   = SourceMri->GetBounding ();
const TBoundingBox<double>* boundto     = TargetMri->GetBounding ();
double              extentratio         = boundfrom->Radius () / NonNull ( boundto->Radius () );


if      ( coregtype.NumScalings == 1 ) {

    govtov.AddGroup ();
                                        // global scale
    govtov.AddDim   ( Scale,  extentratio * 0.75, extentratio * 1.25 );
    }

else if ( coregtype.NumScalings == 3 ) {

    govtov.AddGroup ();

//  if ( sametypes ) {                      // start with a good estimate of the scaling in each directions
//      govtov.AddDim   ( ScaleX, boundfrom->GetRadius ( 0 ) / boundto->GetRadius ( 0 ) * 0.75, boundfrom->GetRadius ( 0 ) / boundto->GetRadius ( 0 ) * 1.25 );
//      govtov.AddDim   ( ScaleY, boundfrom->GetRadius ( 1 ) / boundto->GetRadius ( 1 ) * 0.75, boundfrom->GetRadius ( 1 ) / boundto->GetRadius ( 1 ) * 1.25 );
//      govtov.AddDim   ( ScaleZ, boundfrom->GetRadius ( 2 ) / boundto->GetRadius ( 2 ) * 0.75, boundfrom->GetRadius ( 2 ) / boundto->GetRadius ( 2 ) * 1.25 );
//      }
//  else {                                  // start with equal scaling in each directions
        govtov.AddDim   ( ScaleX, extentratio * 0.75, extentratio * 1.25 );
        govtov.AddDim   ( ScaleY, extentratio * 0.75, extentratio * 1.25 );
        govtov.AddDim   ( ScaleZ, extentratio * 0.75, extentratio * 1.25 );
//      }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rotations
if ( coregtype.NumRotations == 3 ) {

    govtov.AddGroup ();

    govtov.AddDim   ( RotationX, -15,  15 );
    govtov.AddDim   ( RotationY, -15,  15 );
    govtov.AddDim   ( RotationZ, -15,  15 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // translations - always
if ( coregtype.NumTranslations == 3 ) {

    govtov.AddGroup ();

    govtov.AddDim   ( TranslationX,-boundto->GetRadius ( 0 ) * 0.5, boundto->GetRadius ( 0 ) * 0.5 );
    govtov.AddDim   ( TranslationY,-boundto->GetRadius ( 1 ) * 0.5, boundto->GetRadius ( 1 ) * 0.5 );
    govtov.AddDim   ( TranslationZ,-boundto->GetRadius ( 2 ) * 0.5, boundto->GetRadius ( 2 ) * 0.5 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // non-rigid parts

                                        // Shearing - still affine transform
if ( coregtype.NumShearings != 0 ) {
                                        // shape: adjusting center
    govtov.AddGroup ();
    govtov.AddDim   ( FitVolumeShearShiftX,  - boundfrom->GetRadius ( 0 ) * 0.5,  boundfrom->GetRadius ( 0 ) * 0.5 );
    govtov.AddDim   ( FitVolumeShearShiftY,  - boundfrom->GetRadius ( 1 ) * 0.5,  boundfrom->GetRadius ( 1 ) * 0.5 );
    govtov.AddDim   ( FitVolumeShearShiftZ,  - boundfrom->GetRadius ( 2 ) * 0.5,  boundfrom->GetRadius ( 2 ) * 0.5 );


    //govtov.AddGroup ();
    //govtov.AddDim   ( FitVolumeNormCenterRotateX, -10, 10 );
    //govtov.AddDim   ( FitVolumeNormCenterRotateZ, -10, 10 );


    if      ( coregtype.NumShearings == 2 ) {
        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,    0.10 );
        } // 2

    else if ( coregtype.NumShearings == 3 ) {
        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearXtoZ,  -0.10,    0.10 );
        } // 3

    else if ( coregtype.NumShearings == 6 ) {

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearXtoY,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,   0.10 );

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearXtoZ,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearZtoX,  -0.10,   0.10 );

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearZtoY,  -0.10,   0.10 );
        } // 6
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                          // shape: perspective
    //govtov.AddGroup ();
    //govtov.AddDim   ( FitVolumePerspectiveZtoXYDelta,  - 0.10 * boundfrom->GetRadius ( 2 ),  0.10 * boundfrom->GetRadius ( 2 ) );
    //govtov.AddDim   ( FitVolumePerspectiveZtoYDelta,   - 0.10 * boundfrom->GetRadius ( 2 ),  0.10 * boundfrom->GetRadius ( 2 ) );
    //govtov.AddDim   ( FitVolumePerspectiveZtoXDelta,   - 0.10 * boundfrom->GetRadius ( 2 ),  0.10 * boundfrom->GetRadius ( 2 ) );

                                            // shape: pyramidal & flattening distortion, per axis
    //govtov.AddGroup ();
    //govtov.AddDim   ( FitVolumeNormCenterDeltaX,  - boundfrom->GetRadius ( 0 ) * 0.5,  boundfrom->GetRadius ( 0 ) * 0.5 );
    //govtov.AddDim   ( FitVolumeNormCenterDeltaY,  - boundfrom->GetRadius ( 1 ) * 0.5,  boundfrom->GetRadius ( 1 ) * 0.5 );
    //govtov.AddDim   ( FitVolumeNormCenterDeltaZ,  - boundfrom->GetRadius ( 2 ) * 0.5,  boundfrom->GetRadius ( 2 ) * 0.5 );
    //govtov.AddDim   ( FitVolumeNormCenterRotateX, -20, 20 );
    //govtov.AddDim   ( FitVolumeNormCenterRotateZ, -20, 20 );

                                            // Pinching - NOT affine
    //govtov.AddGroup ();
    //govtov.AddDim   ( PinchYtoX,  -0.05,    0.05 );
    //govtov.AddDim   ( PinchZtoX,  -0.05,    0.05 );
    //govtov.AddDim   ( FlattenX,    0.00,    0.50 );
    //
    //govtov.AddGroup ();
    //govtov.AddDim   ( PinchZtoY,  -0.05,    0.05 );
    //govtov.AddDim   ( FlattenYNeg, 0.00,    0.50 );
    //govtov.AddDim   ( FlattenYPos, 0.00,    0.50 );
    //
    //govtov.AddGroup ();
    //govtov.AddDim   ( PinchYtoZ,  -0.05,    0.05 );
    //govtov.AddDim   ( FlattenZPos, 0.00,    0.50 );
    //govtov.AddDim   ( FlattenZNeg, 0.00,    0.50 );
    }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Search!
govtov.GetSolution  (   method,                 0,
                        precision,              0, 
                        MriCoregistrationTitle, 
                        &govtovq 
                    );

                                        // return the quality of convergence
quality     =                govtov.GetFinalQuality   ( govtovq );
StringCopy ( qualityopinion, govtov.GetQualityOpinion ( quality ) );


//StringPrepend ( qualityopinion, " " );
//StringPrepend ( qualityopinion, FloatToString ( quality, 1 ) );
//govtovq.Show ( qualityopinion );
//DBGV ( govtov.GetValue ( FitVolumeShearXtoY ), "FitVolumeShearXtoY" );
//DBGV ( govtov.GetValue ( FitVolumeShearYtoX ), "FitVolumeShearYtoX" );
//DBGV ( govtov.GetValue ( FitVolumeShearXtoZ ), "FitVolumeShearXtoZ" );
//DBGV ( govtov.GetValue ( FitVolumeShearZtoX ), "FitVolumeShearZtoX" );
//DBGV ( govtov.GetValue ( FitVolumeShearYtoZ ), "FitVolumeShearYtoZ" );
//DBGV ( govtov.GetValue ( FitVolumeShearZtoY ), "FitVolumeShearZtoY" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract some measures from convergence
TEasyStats          scalestat;

                                        // put all possible scales in the stats (note that Scale should be exclusive with the other ScaleXYZ)
if ( govtov.HasValue ( Scale  ) )   scalestat.Add ( govtov.GetValue ( Scale  ) );
if ( govtov.HasValue ( ScaleX ) )   scalestat.Add ( govtov.GetValue ( ScaleX ) );
if ( govtov.HasValue ( ScaleY ) )   scalestat.Add ( govtov.GetValue ( ScaleY ) );
if ( govtov.HasValue ( ScaleZ ) )   scalestat.Add ( govtov.GetValue ( ScaleZ ) );

                                        // from that global scale, how much are we going to resample in each direction?
double              targetresamp        = scalestat.IsNotEmpty () ? scalestat.Mean () : 1;
//double            sourceresamp        = 1 / targetresamp;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDirSource;
//TFileName           BaseDirTarget;
TFileName           FileNameSource;
TFileName           FileNameTarget;
TFileName           sourcecoregfile;
TFileName           targetcoregfile;

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


//CreatePath ( BaseDir,     false );

                                        // delete any previous files, with my prefix only!
//sprintf ( buff,         "%s.*",                     BaseFileName );
//DeleteFiles ( buff );
//sprintf ( buff,         "%s.*",                     BaseFileNameMore );
//DeleteFiles ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // source to target matrix file
StringCopy      ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
StringAppend    ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );
StringAppend    ( sourcecoregfile,      ".Abs" );
AddExtension    ( sourcecoregfile,      FILEEXT_TXT );

govtov.FromAbs_ToAbs.WriteFile ( sourcecoregfile );

outputmats.Add  ( sourcecoregfile );



StringReplace   ( sourcecoregfile,      ".Abs.",    ".Vox." );

govtov.FromRel_ToRel.WriteFile ( sourcecoregfile );

outputmats.Add  ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transform Source + associated volumes to Target space

FilterTypes         filtertype;
InterpolationType   interpolate;
int                 numsubsampling;
const char*         tofile;

                                        // use the same loop to process both the source MRI + the optional MRIs
for ( int i = -1; i < (int) buddymris; i++ ) {

    bool            issource        = i == -1;
    bool            isbuddy         = i >=  0;

                                        // don't transform twice the source file
    if ( isbuddy && StringIs ( buddymris[ i ], SourceMri->GetDocPath () ) )
        continue;

                                        // special case for the source MRI
    tofile              = issource ? SourceMri->GetDocPath () : buddymris[ i ];

    TOpenDoc< TVolumeDoc >      TransfMri ( tofile, verbose == Silent ?  OpenDocHidden : OpenDocVisible );

    if ( ! TransfMri.IsOpen () )
        continue;

                                        // only file name
    StringCopy      ( FileNameSource,       tofile );
    GetFilename     ( FileNameSource );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    filtertype          = TransfMri->IsMask ()  ?   FilterTypeMedian           
                        :                           FilterTypeMean;

    numsubsampling      = TransfMri->IsMask ()  ?   3                          
                        :                           AtLeast ( 1, Round ( targetresamp ) );

    interpolate         = TransfMri->IsMask ()  ?   InterpolateNearestNeighbor      // !no interpolation for mask!
                        : targetresamp > 1.5    ?   InterpolateCubicHermiteSpline   // downsampling -> make it faster & less artifacty     (InterpolateUniformCubicBSpline smoother)
                        : targetresamp < 0.75   ?   InterpolateCubicHermiteSpline   // upsampling   -> avoiding Lanczos grid-like artifacts
                        :                           InterpolateLanczos3;            // keeping same scale, can use Lanczos

                                        // source to target MRI file
    StringCopy      ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
    StringAppend    ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );
    AddExtension    ( sourcecoregfile,      DefaultMriExt );


    govtov.TransformToTarget    (   *TransfMri->GetData (), 
                                    filtertype, 
                                    interpolate, 
                                    numsubsampling,
                                    TargetMri->GetNiftiTransform  (), // !target!
                                    TransfMri->GetNiftiIntentCode (), // !source!
                                    TransfMri->GetNiftiIntentName (), // !source!
                                    sourcecoregfile,        "Applying to Target"
                                );


    outputmris.Add ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( CartoolObjects.CartoolApplication->IsInteractive () && verbose == Interactive ) {

        TOpenDoc< TVolumeDoc >      SourceTransfMri ( sourcecoregfile, OpenDocVisible );
        TVolumeView*                sourcetransfview    =  dynamic_cast<TVolumeView*> ( SourceTransfMri->GetViewList () );


        if ( targetview && sourcetransfview ) {
                                            // modify position
            sourcetransfview->WindowRestore();

            sourcetransfview->WindowSetPosition ( targetleft, targettop, targetwidth, targetheight );
                                            // tiling all MRIs to the right
            targetleft     += targetwidth;
            }


        SourceTransfMri.Close ( CloseDocLetOpen );
        } // interactive

    } // for buddymris


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transform points (solution points or electrodes)
for ( int i = 0; i < (int) buddypoints; i++ ) {

    tofile              = buddypoints[ i ];

    bool        isspfile= IsExtensionAmong ( tofile, AllSolPointsFilesExt );

                                        // only file name
    StringCopy      ( FileNameSource,       tofile );
    GetFilename     ( FileNameSource );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TPoints             points ( tofile );

    govtov.FromAbs_ToAbs.Apply ( points );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // source to target matrix file
    StringCopy      ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
    StringAppend    ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );

    if ( isspfile )     AddExtension    ( sourcecoregfile,  FILEEXT_SPIRR );
    else                AddExtension    ( sourcecoregfile,  FILEEXT_XYZ   );


    points.WriteFile ( sourcecoregfile );


    outputpoints.Add ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( CartoolObjects.CartoolApplication->IsInteractive () && verbose == Interactive ) {

        if ( isspfile ) {

            TOpenDoc< TSolutionPointsDoc >  SpiDoc ( sourcecoregfile, OpenDocVisible );
            TSolutionPointsView*            sourcetransfview    =  dynamic_cast<TSolutionPointsView*> ( SpiDoc->GetViewList () );


            if ( targetview && sourcetransfview ) {
                                                // modify position
                sourcetransfview->WindowRestore();

                sourcetransfview->WindowSetPosition ( targetleft, targettop, targetwidth, targetheight );
                                                // tiling all MRIs to the right
                targetleft     += targetwidth;
                }


            SpiDoc.Close ( CloseDocLetOpen );
            }
        else {

            TOpenDoc< TElectrodesDoc >  XyzDoc ( sourcecoregfile, OpenDocVisible );
            TElectrodesView*            sourcetransfview    =  dynamic_cast<TElectrodesView*> ( XyzDoc->GetViewList () );


            if ( targetview && sourcetransfview ) {
                                                // modify position
                sourcetransfview->WindowRestore();

                sourcetransfview->WindowSetPosition ( targetleft, targettop, targetwidth, targetheight );
                                                // tiling all MRIs to the right
                targetleft     += targetwidth;
                }


            XyzDoc.Close ( CloseDocLetOpen );
            }
        } // interactive

    } // for buddypoints


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const_cast< TVolumeDoc* > ( SourceMri )->AllowClosing ();
const_cast< TVolumeDoc* > ( TargetMri )->AllowClosing ();


//if ( SourceMri != 0 && SourceMri->CanClose ( true ) )
//    DocManager->CloseDoc ( SourceMri );

                                        // until a proper local TSuperGauge
TSuperGauge         Gauge ( MriCoregistrationTitle );
Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // Brain -> ( RAS + Sagittal + Transverse ) transform matrix
bool    NormalizeBrain      (   const TVolumeDoc*       MriDoc,
                                NormalizeBrainResults&  norm
                            )
{
norm.Reset ();

if ( MriDoc == 0 )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Setting origins
if ( MriDoc->IsValidOrigin () )

    norm.Origin     = MriDoc->GetOrigin ();

else
                                        // provide a fall-back origin
    norm.Origin     = MriDoc->GetDefaultCenter ();


norm.OriginToTarget     = norm.Origin;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Early exit?
bool                sourceistemplate    = MriDoc->IsTemplate ();
bool                sourcesagtra        = StringGrep ( MriDoc->GetDocPath (), "\\." InfixSagittal "(" InfixEquatorial "|" InfixTransverse ")\\.", GrepOptionDefaultFiles );

                                        // either a MNI template, or we trust the file name to contain ".SagTra." or ".SagEqu." as the output of MRI pre-processing
if ( sourceistemplate || sourcesagtra ) {
                                        // compose the matrix, which is reorientation
    norm.Rel_to_Abs     = MriDoc->GetStandardOrientation ( LocalToRAS );

                                        // + translation
    norm.Rel_to_Abs.Translate ( -norm.Origin.X, -norm.Origin.Y, -norm.Origin.Z, MultiplyRight );


    // norm.OriginToTarget needs some transform? not for RAS orientation at least...

    return  true;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Align to Sagittal Plane + RAS
bool                sagittalok;
bool                transverseok;
TMatrix44           MriRel_to_MriAbs    = MriDoc->GetStandardOrientation ( LocalToRAS );
TMatrix44           SagAbs_to_MriRel;
TMatrix44           MriRel_to_SagAbs;


sagittalok          =   SetSagittalPlaneMri (   MriDoc, 
                                                MriRel_to_MriAbs,   // input:  Local -> Rescaling -> RAS
                                                SagAbs_to_MriRel,   // input & output: RAS -> Local -> Translate
                                                norm.OriginToTarget,// input & output
                                                0,
                                                "Sagittal Plane"
                                            );

                                        // Translate(-LocalCenter) -> RASOrient (centered) -> Rotates(RAS)
MriRel_to_SagAbs    = TMatrix44 ( SagAbs_to_MriRel ).Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Adjust Transverse Plane to MNI

                                        // used to initialize RAS to MRI
TMatrix44           TraAbs_to_MriRel    = SagAbs_to_MriRel;
TMatrix44           MriRel_to_TraAbs;

                                        // !at that point brain should be Bias Field Corrected!
                                        // Also, function could be modified as to return the full matrix to MNI slice, i.e. to 1[mm] voxel
transverseok    =   SetTransversePlaneMri   (   MriDoc, 
                                                MriRel_to_SagAbs,       // not used
                                                TraAbs_to_MriRel,       // input & output: Rotate -> RAS -> Local -> Translate
                                                norm.OriginToTarget,    // input & output: LocalCenter
                                                0,
                                                "Transverse Plane"
                                            );

                                        // Translate(-LocalCenter) -> Local -> RAS -> -Rotate
MriRel_to_TraAbs    = TMatrix44 ( TraAbs_to_MriRel ).Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final transform
norm.Rel_to_Abs     = MriRel_to_TraAbs;


return  sagittalok && transverseok;
}


//----------------------------------------------------------------------------
                                        // Specialized coregistration, used only for brains

void    CoregisterBrains    (   const TVolumeDoc*   SourceMri,  RemapIntensityType  fromremap,
                                const TVolumeDoc*   TargetMri,  RemapIntensityType  toremap,
                                FitVolumeType       inclusionflags,
                                const CoregistrationSpecsType& coregtype,
                                double              precision,
                                const TGoF&         buddymris,  const TGoF&         buddypoints,
                                const char*         fileprefix,
                                TGoF&               outputmats, TGoF&               outputmris, TGoF&               outputpoints,
                                double&             quality,    char*               qualityopinion,
                                VerboseType         verbose
                            )
{
                                        // Coregistration can take quite some time - forbid closing these docs
                                        // bypassing the const'ness as it doesn't really modify the doc, this just a flag
const_cast< TVolumeDoc* > ( SourceMri )->PreventClosing ();
const_cast< TVolumeDoc* > ( TargetMri )->PreventClosing ();


outputmats  .Reset ();
outputmris  .Reset ();
outputpoints.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Be nice: show the 2 windows side to side
TVolumeView        *sourceview      =  dynamic_cast< TVolumeView * > ( SourceMri->GetViewList () );
TVolumeView        *targetview      =  dynamic_cast< TVolumeView * > ( TargetMri->GetViewList () );
int                 targetwidth;
int                 targetheight;
int                 targetleft;
int                 targettop;


if ( targetview && sourceview ) {
                                        // modify their positions
    targetwidth     =
    targetheight    = min ( GetWindowWidth ( CartoolObjects.CartoolMainWindow->GetClientWindow () ), GetWindowHeight ( CartoolObjects.CartoolMainWindow->GetClientWindow () ) ) / 2;

    targetview->WindowRestore();
    targetview->WindowSetPosition ( 0, 0, targetwidth, targetheight );

    sourceview->WindowRestore();
    sourceview->WindowSetPosition ( targetview->GetWindowRight () + 1,  targetview->GetWindowTop (),
                                    targetwidth,                        targetheight );

    targetleft  = targetview->GetWindowLeft   ();
    targettop   = targetview->GetWindowBottom () + 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Reorient SourceMri only
bool                sourceistemplate            = SourceMri->IsTemplate ();
NormalizeBrainResults   SourceNorm;


NormalizeBrain  (   SourceMri,  
                    SourceNorm
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Reorient TargetMri only
bool                targetistemplate            = TargetMri->IsTemplate ();
NormalizeBrainResults   TargetNorm;


NormalizeBrain  (   TargetMri, 
                    TargetNorm
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Adjust scaling & shearing, now source to target

                                        // providing the reorientation + Sagittal + Transverse transform matrix
TFitVolumeOnVolume  govtov (    SourceMri,      fromremap,  sourceistemplate ? 0 : &SourceNorm.Rel_to_Abs,
                                TargetMri,      toremap,    targetistemplate ? 0 : &TargetNorm.Rel_to_Abs,
                                inclusionflags 
                            );

TEasyStats          govtovq;

                                        // Set if not using Nelder-Mead
//govtov.Set ( GOStepsFaster );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scaling
const TBoundingBox<double>  *boundfrom  = SourceMri->GetBounding ();
const TBoundingBox<double>  *boundto    = TargetMri->GetBounding ();

                                        // dimensions won't match as there is some reorientation occuring in-between
double              fromextent      = boundfrom->Radius ();
double              toextent        = boundto  ->Radius ();
double              extentratio     = fromextent / toextent;


if      ( coregtype.NumScalings == 1 ) {

    govtov.AddGroup ();
                                        // global scale
    govtov.AddDim   ( Scale,  extentratio * 0.60, extentratio * 1.40 );

    }

else if ( coregtype.NumScalings == 3 ) {

    govtov.AddGroup ();
                                        // have quite some margin, Nelder-Mead always remain within the input range
    govtov.AddDim   ( ScaleX, extentratio * 0.60, extentratio * 1.40 );
    govtov.AddDim   ( ScaleY, extentratio * 0.60, extentratio * 1.40 );
    govtov.AddDim   ( ScaleZ, extentratio * 0.60, extentratio * 1.40 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // non-rigid parts

                                        // Shearing - still affine transform
if ( coregtype.NumShearings != 0 ) {
                                        // shape: adjusting center
    govtov.AddGroup ();
    govtov.AddDim   ( FitVolumeShearShiftX,  - fromextent * 0.25,  fromextent * 0.25 );
    govtov.AddDim   ( FitVolumeShearShiftY,  - fromextent * 0.25,  fromextent * 0.25 );
    govtov.AddDim   ( FitVolumeShearShiftZ,  - fromextent * 0.25,  fromextent * 0.25 );


    //govtov.AddGroup ();
    //govtov.AddDim   ( FitVolumeNormCenterRotateX, -10, 10 );
    //govtov.AddDim   ( FitVolumeNormCenterRotateZ, -10, 10 );


    if      ( coregtype.NumShearings == 2 ) {
        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,    0.10 );
        } // 2

    else if ( coregtype.NumShearings == 3 ) {
        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,    0.10 );
        govtov.AddDim   ( FitVolumeShearXtoZ,  -0.10,    0.10 );
        } // 3

    else if ( coregtype.NumShearings == 6 ) {

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearXtoY,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearYtoX,  -0.10,   0.10 );

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearXtoZ,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearZtoX,  -0.10,   0.10 );

        govtov.AddGroup ();
        govtov.AddDim   ( FitVolumeShearYtoZ,  -0.10,   0.10 );
        govtov.AddDim   ( FitVolumeShearZtoY,  -0.10,   0.10 );
        } // 6
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Search!
govtov.GetSolution  (   GlobalNelderMead,       0,  // fast & good
                        precision,              0, 
                        "Source to Target" /*MriCoregistrationTitle*/, 
                        &govtovq 
                    );    

                                        // return the quality of convergence
quality     =                govtov.GetFinalQuality   ( govtovq );
StringCopy ( qualityopinion, govtov.GetQualityOpinion ( quality ) );


//StringPrepend ( qualityopinion, " " );
//StringPrepend ( qualityopinion, FloatToString ( quality, 1 ) );
//govtovq.Show ( qualityopinion );
//DBGV ( govtov.GetValue ( FitVolumeShearXtoY ), "FitVolumeShearXtoY" );
//DBGV ( govtov.GetValue ( FitVolumeShearYtoX ), "FitVolumeShearYtoX" );
//DBGV ( govtov.GetValue ( FitVolumeShearXtoZ ), "FitVolumeShearXtoZ" );
//DBGV ( govtov.GetValue ( FitVolumeShearZtoX ), "FitVolumeShearZtoX" );
//DBGV ( govtov.GetValue ( FitVolumeShearYtoZ ), "FitVolumeShearYtoZ" );
//DBGV ( govtov.GetValue ( FitVolumeShearZtoY ), "FitVolumeShearZtoY" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract some measures from convergence
TEasyStats          scalestat;

                                        // put all possible scales in the stats (note that Scale should be exclusive with the other ScaleXYZ)
if ( govtov.HasValue ( Scale  ) )   scalestat.Add ( govtov.GetValue ( Scale  ) );
if ( govtov.HasValue ( ScaleX ) )   scalestat.Add ( govtov.GetValue ( ScaleX ) );
if ( govtov.HasValue ( ScaleY ) )   scalestat.Add ( govtov.GetValue ( ScaleY ) );
if ( govtov.HasValue ( ScaleZ ) )   scalestat.Add ( govtov.GetValue ( ScaleZ ) );

                                        // from that global scale, how much are we going to resample in each direction?
double              targetresamp        = scalestat.IsNotEmpty () ? scalestat.Mean () : 1;
//double            sourceresamp        = 1 / targetresamp;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieving final cumulated transforms
TMatrix44           transform       = govtov.FromRel_ToRel;

                                        // freeing some memory
govtov.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDirSource;
//TFileName           BaseDirTarget;
TFileName           FileNameSource;
TFileName           FileNameTarget;
TFileName           sourcecoregfile;
//TFileName           targetcoregfile;

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


//CreatePath ( BaseDir,     false );

                                        // delete any previous files, with my prefix only!
//sprintf ( buff,         "%s.*",                     BaseFileName );
//DeleteFiles ( buff );
//sprintf ( buff,         "%s.*",                     BaseFileNameMore );
//DeleteFiles ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // source to target matrix file
StringCopy          ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
StringAppend        ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );
StringAppend        ( sourcecoregfile,      ".Abs" );
AddExtension        ( sourcecoregfile,      FILEEXT_TXT );
CheckNoOverwrite    ( sourcecoregfile );

transform.WriteFile ( sourcecoregfile );

outputmats.Add      ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transform Source + associated volumes to Target space

char                orientationstring[ 4 ];
FilterTypes         filtertype;
InterpolationType   interpolate;
int                 numsubsampling;
BoundingSizeType    targetsize      = BoundingSizeOptimal; // BoundingSizeGiven
TVector3Double      nullvoxels ( 0.0 );
//TVector3Int         TargetMriSize;
const char*         tofile;


TargetMri->OrientationToString ( orientationstring );

//TargetMri->GetData ()->GetDims ( TargetMriSize );

transform.Apply ( SourceNorm.OriginToTarget );


                                        // use the same loop to process both the source MRI + the optional MRIs
for ( int i = -1; i < (int) buddymris; i++ ) {

                                        // don't transform twice the source file
    if ( i >= 0 && StringIs ( buddymris[ i ], SourceMri->GetDocPath () ) )
        continue;

                                        // special case for the source MRI
    tofile              = i == -1 ? SourceMri->GetDocPath () : buddymris[ i ];

    TOpenDoc< TVolumeDoc >      TransfMri ( tofile, verbose == Silent ?  OpenDocHidden : OpenDocVisible );

    if ( ! TransfMri.IsOpen () )
        continue;

                                        // only file name
    StringCopy      ( FileNameSource,       tofile );
    GetFilename     ( FileNameSource );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    filtertype          = TransfMri->IsMask ()  ?   FilterTypeMedian           
                        :                           FilterTypeMean;

    numsubsampling      = TransfMri->IsMask ()  ?   3                          
                        :                           AtLeast ( 1, Round ( targetresamp ) );

    interpolate         = TransfMri->IsMask ()  ?   InterpolateNearestNeighbor      // !no interpolation for mask!
                        : targetresamp > 1.5    ?   InterpolateCubicHermiteSpline   // downsampling -> make it faster & less artifacty     (InterpolateUniformCubicBSpline smoother)
                        : targetresamp < 0.75   ?   InterpolateCubicHermiteSpline   // upsampling   -> avoiding Lanczos grid-like artifacts
                        :                           InterpolateLanczos3;            // keeping same scale, can use Lanczos

                                        // source to target MRI file
    StringCopy          ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
    StringAppend        ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );
    AddExtension        ( sourcecoregfile,      DefaultMriExt );
    CheckNoOverwrite    ( sourcecoregfile );


                                        // apply transform and save to file
    TransfMri->GetData ()->TransformAndSave (
                                            transform,       
                                            TransformAndSaveFlags ( TransformToTarget | TransformSourceRelative ),
                                            filtertype,         interpolate,    numsubsampling,
                                            &SourceNorm.Origin, &SourceNorm.OriginToTarget, 
                                            nullvoxels,         &TargetMri->GetVoxelSize (),
                                            targetsize,         0 /*&TargetMriSize*/,
                                            orientationstring,
                                            TargetMri->GetNiftiTransform  (), // !target!
                                            TransfMri->GetNiftiIntentCode (), // !source!
                                            TransfMri->GetNiftiIntentName (), // !source!
                                            sourcecoregfile,    0,              0,
                                            "Applying Transform" 
                                            );


    outputmris.Add      ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( CartoolObjects.CartoolApplication->IsInteractive () && verbose == Interactive ) {

        TOpenDoc< TVolumeDoc >      SourceTransfMri ( sourcecoregfile, verbose == Silent ?  OpenDocHidden : OpenDocVisible );
        TVolumeView*                sourcetransfview    =  dynamic_cast< TVolumeView * > ( SourceTransfMri->GetViewList () );


        if ( targetview && sourcetransfview ) {
                                            // modify position
            sourcetransfview->WindowRestore();

            sourcetransfview->WindowSetPosition ( targetleft, targettop, targetwidth, targetheight );
                                            // tiling all MRIs to the right
            targetleft     += targetwidth;
            }


        SourceTransfMri.Close ( CloseDocLetOpen );
        }

    } // for buddymris


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transform points (solution points or electrodes)
for ( int i = 0; i < (int) buddypoints; i++ ) {

    tofile              = buddypoints[ i ];

    bool        isspfile= IsExtensionAmong ( tofile, AllSolPointsFilesExt );

                                        // only file name
    StringCopy      ( FileNameSource,       tofile );
    GetFilename     ( FileNameSource );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TPoints             points ( tofile );

    transform.Apply ( points );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // source to target matrix file
    StringCopy          ( sourcecoregfile,      BaseDirSource,  "\\",   fileprefix );
    StringAppend        ( sourcecoregfile,      FileNameSource,     InfixToCoregistered,    FileNameTarget );

    if ( isspfile )     AddExtension    ( sourcecoregfile,  FILEEXT_SPIRR );
    else                AddExtension    ( sourcecoregfile,  FILEEXT_XYZ   );

    CheckNoOverwrite    ( sourcecoregfile );

    points.WriteFile    ( sourcecoregfile );


    outputpoints.Add    ( sourcecoregfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( CartoolObjects.CartoolApplication->IsInteractive () && verbose == Interactive ) {

        if ( isspfile ) {
            TOpenDoc< TSolutionPointsDoc >  SpiDoc ( sourcecoregfile, OpenDocVisible );
            SpiDoc.Close ( CloseDocLetOpen );
            }
        else {
            TOpenDoc< TElectrodesDoc >      XyzDoc ( sourcecoregfile, OpenDocVisible );
            XyzDoc.Close ( CloseDocLetOpen );
            }
        }

    } // for buddypoints


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const_cast< TVolumeDoc* > ( SourceMri )->AllowClosing ();
const_cast< TVolumeDoc* > ( TargetMri )->AllowClosing ();


//if ( SourceMri != 0 && SourceMri->CanClose ( true ) )
//    DocManager->CloseDoc ( SourceMri );

                                        // until a proper local TSuperGauge
TSuperGauge         Gauge ( MriCoregistrationTitle );
Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
