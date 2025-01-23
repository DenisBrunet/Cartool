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

#include    "ComputingTemplateMri.h"

#include    "Math.TMatrix44.h"
#include    "Math.Stats.h"
#include    "Math.Resampling.h"
#include    "Files.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Files.TGoF.h"
#include    "TArray1.h"
#include    "TVector.h"
#include    "TVolume.h"                 // NeighborhoodType
#include    "Dialogs.TSuperGauge.h"
#include    "GlobalOptimize.Points.h"
#include    "GlobalOptimize.Volumes.h"
#include    "GlobalOptimize.Tracks.h"

#include    "ESI.SolutionPoints.h"
#include    "ESI.InverseModels.h"       // InverseNeighborhood

#include    "TSolutionPointsDoc.h"
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

const CoregistrationSpecsType   CoregTemplate[ NumBuildTemplateType ][ NumCoregistrationTemplateTypes ] =

    {                                                                                                   // Tra Rot Scl Shr
      {                                 // BuildTemplateSelfRef
//      {   (CoregistrationTypes)   CoregistrationBoot,         "Initial rigid template",   "",     true,   2,  1,  1,  0 },    // boot: rigid transform, only allowing minimal adjustments for center, hard-preserving the sagittal plane orientation
        {   (CoregistrationTypes)   CoregistrationBoot,         "Initial rigid template",   "",     true,   3,  1,  1,  0 },    // boot: rigid transform, allowing full center adjustment, hard-preserving the sagittal plane orientation
//      {   (CoregistrationTypes)   CoregistrationBoot,         "Initial rigid template",   "",     true,   3,  3,  1,  0 },    // boot: full rigid transform, allowing to change the sagittal plane orientation
        {   (CoregistrationTypes)   CoregistrationLoop,         "Affine coregistration",    "",     true,   3,  3,  3,  6 },    // loop: full affine
      },

      {                                 // BuildTemplateMNI
        {   (CoregistrationTypes)   CoregistrationBoot,         "Affine coregistration",    "",     true,   3,  3,  3,  6 },    // full affine
        {   (CoregistrationTypes)   CoregistrationLoop,         "Affine coregistration",    "",     true,   3,  3,  3,  6 },    // - not actually used -
      }
    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Open and reorient all MRIs by themselves
void    NormalizeBrains         (   const TGoF&             mrifiles,
                                    NormalizeBrainResults*& allnorms,
                                    TVector3Int&            mrimaxdim,
                                    TSuperGauge&            Gauge
                                )
{
if ( (int) mrifiles == 0 )
    return;

int                 nummrifiles     = (int) mrifiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate array of results
allnorms    = new NormalizeBrainResults[ nummrifiles ];


mrimaxdim   = 0;


for ( int mi = 0; mi < nummrifiles; mi++ ) {

    Gauge.Next ( -1, SuperGaugeUpdateTitle );


    TOpenDoc<TVolumeDoc>    mridoc ( mrifiles[ mi ], /*OpenDocVisible*/ OpenDocHidden );

                                        // taking the opportunity to get the max size for template
//  Maxed ( mrimaxdim[ 0 ], mridoc->GetData()->GetDim ( mridoc->GetAxisIndex ( LeftRight ) ) );
//  Maxed ( mrimaxdim[ 1 ], mridoc->GetData()->GetDim ( mridoc->GetAxisIndex ( FrontBack ) ) );
//  Maxed ( mrimaxdim[ 2 ], mridoc->GetData()->GetDim ( mridoc->GetAxisIndex ( UpDown    ) ) );
                                        // hum, maybe we can settle down to just the bounding boxes, which are much smaller
    Maxed ( mrimaxdim[ 0 ], Round ( mridoc->GetBounding ()->GetExtent ( mridoc->GetAxisIndex ( LeftRight ) ) ) );
    Maxed ( mrimaxdim[ 1 ], Round ( mridoc->GetBounding ()->GetExtent ( mridoc->GetAxisIndex ( FrontBack ) ) ) );
    Maxed ( mrimaxdim[ 2 ], Round ( mridoc->GetBounding ()->GetExtent ( mridoc->GetAxisIndex ( UpDown    ) ) ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // MRI to RAS + Sagittal + Transverse MNI
                                        // We could save us this step if user can guarantee files are already preprocessed...
    NormalizeBrain  (   mridoc,  
                        allnorms[ mi ]
                    );

    } // for NormalizeBrain

}


//----------------------------------------------------------------------------

void    MergeMris               (   const TGoF&         mrifiles,
                                    BuildTemplateType   howtemplate,
                                    const NormalizeBrainResults*  allnorms,
                                    const char*         mnifile,
                                    const TVector3Int&  mrimaxdim,
                                    double              precision,              int             numiterations,
                                    bool                symmetrical,
                                    const char*         basefilename,   
                                    char*               templatefile,           TGoF&           gofmatrices,
                                    bool                savingcoregmris,
                                    TEasyStats&         quality,
                                    TSuperGauge&        Gauge
                                )
{
ClearString ( templatefile );
gofmatrices.Reset ();

if ( (int) mrifiles < 1 
  || ( howtemplate == BuildTemplateMNI && StringIsEmpty ( mnifile ) )
  || allnorms == 0 )

    return;


int                 nummrifiles     = (int) mrifiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // File names
#define             InfixGeneration         "Gen"

TFileName           matfile;
TFileName           mrinormfile;
TFileName           avgfile;
//TGoF                alignedmris;
char                buff[ 256 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocating volume for template

int                 avgdim1;
int                 avgdim2;
int                 avgdim3;
TPointDouble        avgorigin;


if ( howtemplate == BuildTemplateMNI ) {

    TOpenDoc< TVolumeDoc >      mnidoc ( mnifile, OpenDocHidden );

    TVector3Int                 mnidim;

    mnidoc->GetSize ( mnidim );

    avgorigin   = mnidoc->GetOrigin ();

    mnidoc->Close ();

    avgdim1     = mnidim.X;             // use exact target size
    avgdim2     = mnidim.Y;
    avgdim3     = mnidim.Z;
    }
else {
                                        // adding some margin the whole embedding box
    avgdim1     = RoundToOdd ( mrimaxdim.X * 1.20 );
    avgdim2     = RoundToOdd ( mrimaxdim.Y * 1.20 );
    avgdim3     = RoundToOdd ( mrimaxdim.Z * 1.20 );

                                        // mimicking the origin placement from the MNI template
    avgorigin   = TPointDouble ( avgdim1, avgdim2, avgdim3 ) 
                * TPointDouble ( (double) 96 / 192, (double) 132 / 228, (double) 78 / 192 );
                                        // and make it even, so it is well centered on the volume
    avgorigin.RoundTo ( 2 );
                                        // make the origin voxel-aligned
    avgorigin.Truncate ();
    }


Volume              avgvol ( avgdim1, avgdim2, avgdim3 );
FctParams           p;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need some list of all docs & matrices
TVector< TOpenDoc< TVolumeDoc > >   mridoc ( nummrifiles );
TArray1< TMatrix44 >                MriRel_to_CoregAbs ( nummrifiles );
TArray1< TMatrix44 >                CoregAbs_to_MriRel ( nummrifiles );

                                        // contol if all MRIs can be let open simultaneously or not
bool                openall         = true;
TOpenDocType        howopen         = OpenDocHidden;


if ( openall )

    for ( int mi = 0; mi < nummrifiles; mi++ )

        mridoc[ mi ].Open ( mrifiles[ mi ], howopen );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reference MRI
TOpenDoc< TVolumeDoc >  refdoc;
int                 initref         = 0;// in case of self-alignment, the index of the MRI used for booting
TMatrix44           refMriRel_to_TraAbs;
const TBoundingBox<double>* refbound;
TVector3Double      refvoxelsize;
TPointDouble        reforigin;
TVector3Int         refdim;

TMatrix44           MriRel_to_TraAbs;

                                        // variables for transformation and saving
FilterTypes         filtertype;
int                 numsubsampling;
InterpolationType   interpolate;
char                orientationstring[ 4 ]  = "RAS";
BoundingSizeType    targetsize              = BoundingSizeOptimal;

                                        // Allocate a slice of stats
TGoEasyStats        stat ( avgdim2 * avgdim3, ( symmetrical ? 2 : 1 ) * nummrifiles );
                                        // if symmetrical, we can cut the computation by a factor 2
int                 maxslicex       = symmetrical ? Round ( avgdim1 / 2.0 ) : avgdim1;

                                        // handy variables to know where we are in the sequence of templates
bool                isbooting;
bool                islooping;
bool                islast;
bool                deletetempgen   = true; // we don't want to see the intermediate templates, do we?


                                        // only now can we set the real range of progress bar
Gauge.SetRange ( -1, ( numiterations + 1 ) * ( 4 + nummrifiles * 3 + maxslicex )
                     + nummrifiles                                               );

quality.Resize ( nummrifiles );


for ( int li = 0; li <= numiterations; li++ ) {

    isbooting   = li == 0;
    islooping   = li != 0;
    islast      = li == numiterations;

    avgvol.ResetMemory ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Opening current reference / template
    Gauge.Next ( -1, SuperGaugeUpdateTitle );


    if ( isbooting ) {
                                        // first run -> picking 1 MRI as the reference reference (for global scaling only!)
        if      ( howtemplate == BuildTemplateSelfRef ) {

            refdoc.Open ( mrifiles[ initref ], howopen );

            refMriRel_to_TraAbs = allnorms[ initref ].Rel_to_Abs;
            }

        else if ( howtemplate == BuildTemplateMNI ) {

            refdoc.Open ( const_cast< char* > ( mnifile ), howopen );

                                        // we need the reorientation + center shift for current average
            GetNormalizationTransform ( refdoc,         0, 
                                        false,          TPointDouble ( 0.0 ),     
                                        0,             &refMriRel_to_TraAbs
                                        );
            }
        }

    else { // islooping
                                        // all other runs: opening last saved template
        refdoc.Open ( avgfile, howopen );

                                        // we need the reorientation + center shift for current average
        GetNormalizationTransform ( refdoc,         0, 
                                    false,          TPointDouble ( 0.0 ),     
                                    0,             &refMriRel_to_TraAbs
                                    );
        }


    refbound            = refdoc->GetBounding  ();
    refvoxelsize        = refdoc->GetVoxelSize ();
    reforigin           = refdoc->GetOrigin    ();
    refdoc->GetSize ( refdim );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Looping through all input MRIs
    quality.Reset ();


    for ( int mi = 0; mi < nummrifiles; mi++ ) {

        Gauge.Next ( -1, SuperGaugeUpdateTitle );

        CartoolObjects.CartoolApplication->SetMainTitle ( TemplateMriTitle, mrifiles[ mi ], Gauge );

                                        // opening or just accessing
        mridoc[ mi ].Open ( mrifiles[ mi ], howopen );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // recovering to RAS + Sagittal + Transverse MNI
        Gauge.Next ( -1, SuperGaugeUpdateTitle );

        MriRel_to_TraAbs    = allnorms[ mi ].Rel_to_Abs;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Fitting MRIs to reference
        Gauge.Next ( -1, SuperGaugeUpdateTitle );

        if ( howtemplate == BuildTemplateSelfRef && isbooting && mi == initref )
                                      // no need to coregister onto itself!
            MriRel_to_CoregAbs[ mi ]    = MriRel_to_TraAbs;

        else {
                                        // coregister current MRI to current reference (either another MRI or a temp template)
            RemapIntensityType  remapping   = RemapIntensityRank;


            TFitVolumeOnVolume  govtov (    mridoc[ mi ],       remapping,     &MriRel_to_TraAbs,
                                            refdoc,             remapping,     &refMriRel_to_TraAbs,
                                            FitVolumeEqualSizes 
                                        );

            TEasyStats          govtovq;

                                        // getting parameters for the type of coregistration and current state
            const CoregistrationSpecsType&  coregtype   = CoregTemplate[ howtemplate ][ islooping ];


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scaling

            const TBoundingBox<double>* boundfrom       = mridoc[ mi ]->GetBounding ();
            double                      extentratio     = boundfrom->Radius () / NonNull ( refbound->Radius () );

            if      ( coregtype.NumScalings == 1 ) {

                govtov.AddGroup ();
                                                    // global scale
                govtov.AddDim   ( Scale,    extentratio * 0.75, extentratio * 1.25 );
                }

            else if ( coregtype.NumScalings == 3 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( ScaleX,   extentratio * 0.75, extentratio * 1.25 );
                govtov.AddDim   ( ScaleY,   extentratio * 0.75, extentratio * 1.25 );
                govtov.AddDim   ( ScaleZ,   extentratio * 0.75, extentratio * 1.25 );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Rotations
            if      ( coregtype.NumRotations == 1 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( RotationX, -15,  15 );
                }

            else if ( coregtype.NumRotations == 2 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( RotationY, -15,  15 );
                govtov.AddDim   ( RotationZ, -15,  15 );
                }

            else if ( coregtype.NumRotations == 3 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( RotationX, -15,  15 );
                govtov.AddDim   ( RotationY, -15,  15 );
                govtov.AddDim   ( RotationZ, -15,  15 );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Translations
            if      ( coregtype.NumTranslations == 1 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( TranslationX,-refbound->GetRadius ( 0 ) * 0.5, refbound->GetRadius ( 0 ) * 0.5 );
                }

            else if ( coregtype.NumTranslations == 2 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( TranslationY,-refbound->GetRadius ( 1 ) * 0.5, refbound->GetRadius ( 1 ) * 0.5 );
                govtov.AddDim   ( TranslationZ,-refbound->GetRadius ( 2 ) * 0.5, refbound->GetRadius ( 2 ) * 0.5 );
                }

            else if ( coregtype.NumTranslations == 3 ) {

                govtov.AddGroup ();

                govtov.AddDim   ( TranslationX,-refbound->GetRadius ( 0 ) * 0.5, refbound->GetRadius ( 0 ) * 0.5 );
                govtov.AddDim   ( TranslationY,-refbound->GetRadius ( 1 ) * 0.5, refbound->GetRadius ( 1 ) * 0.5 );
                govtov.AddDim   ( TranslationZ,-refbound->GetRadius ( 2 ) * 0.5, refbound->GetRadius ( 2 ) * 0.5 );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Shearing
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
/*                                      // pinch along the X axis on the Y and Z axis
            govtov.AddGroup ();
            govtov.AddDim   ( PinchYtoX, 0.00, -0.25 );
            govtov.AddDim   ( PinchYtoZ, 0.00, -0.25 );

                                        // flattening front, back, left-right, up-down
            govtov.AddGroup ();
            govtov.AddDim   ( FlattenYPos, 0.00, 0.50 );
            govtov.AddDim   ( FlattenYNeg, 0.00, 0.50 );

            govtov.AddGroup ();
            govtov.AddDim   ( FlattenZPos, 0.00, 0.50 );
            govtov.AddDim   ( FlattenX,    0.00, 0.50 );
*/

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            govtov.GetSolution  (   GlobalNelderMead,       0,      // fast & good
                                    precision,              0, 
                                    "Coregistering Brain", 
                                    &govtovq 
                                );

            quality.Add ( govtov.GetFinalQuality ( govtovq ) );


            MriRel_to_CoregAbs[ mi ]        = govtov.ToRel_ToAbs * govtov.FromRel_ToRel;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally saving transformed MRI
            if ( savingcoregmris && islast ) {
                                        // extract some measures from convergence
                TEasyStats          scalestat;

                                        // put all possible scales in the stats (note that Scale should be exclusive with the other ScaleXYZ)
                if ( govtov.HasValue ( Scale  ) )   scalestat.Add ( govtov.GetValue ( Scale  ) );
                if ( govtov.HasValue ( ScaleX ) )   scalestat.Add ( govtov.GetValue ( ScaleX ) );
                if ( govtov.HasValue ( ScaleY ) )   scalestat.Add ( govtov.GetValue ( ScaleY ) );
                if ( govtov.HasValue ( ScaleZ ) )   scalestat.Add ( govtov.GetValue ( ScaleZ ) );

                double              targetresamp        = scalestat.IsNotEmpty () ? scalestat.Mean () : 1;


                filtertype          = mridoc[ mi ]->IsMask ()   ?   FilterTypeMedian           
                                    :                               FilterTypeMean;

                numsubsampling      = mridoc[ mi ]->IsMask ()   ?   3                          
                                    :                               AtLeast ( 1, Round ( targetresamp ) );

                interpolate         = mridoc[ mi ]->IsMask ()   ?   InterpolateNearestNeighbor      // !no interpolation for mask!
                                    : targetresamp > 1.5        ?   InterpolateCubicHermiteSpline   // downsampling -> make it faster & less artifacty     (InterpolateUniformCubicBSpline smoother)
                                    : targetresamp < 0.75       ?   InterpolateCubicHermiteSpline   // upsampling   -> avoiding Lanczos grid-like artifacts
                                    :                               InterpolateLanczos3;            // keeping same scale, can use Lanczos

                                                    // source to target MRI file
                StringCopy          ( mrinormfile,  basefilename );
                StringAppend        ( mrinormfile,  "Coreg", "." );
                StringAppend        ( mrinormfile,  ToFileName ( mrifiles[ mi ] ) );
                ReplaceExtension    ( mrinormfile,  DefaultMriExt );


                govtov.TransformToTarget    (   *mridoc[ mi ]->GetData (), 
                                                filtertype, 
                                                interpolate, 
                                                numsubsampling,
                                                refdoc->GetNiftiTransform  (),          // !target!
                                                mridoc[ mi ]->GetNiftiIntentCode (),    // !source!
                                                mridoc[ mi ]->GetNiftiIntentName (),    // !source!
                                                mrinormfile,        "Saving Coregistered Brain"
                                            );

                } // savingcoregmris

            } // else actual coregistration

                                        // conveniently invert matrix
        CoregAbs_to_MriRel[ mi ]    = TMatrix44 ( MriRel_to_CoregAbs[ mi ] ).Invert ();


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choosing weither closing or letting MRI open
        if ( ! ( openall 
              || howtemplate == BuildTemplateSelfRef && isbooting && mi == initref ) )  // let the first MRI open in case for the booting part

            mridoc[ mi ].Close ();

        } // for mrifiles


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally closing the reference MRI
    if ( howtemplate == BuildTemplateMNI || ! openall )

        refdoc.Close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing the template from currently coregistered MRIs
    TArray1<InterpolationType>      interpolate ( nummrifiles );

    interpolate = InterpolateUnknown;

                                        // looping per slice of x is a good trade-off
    for ( int x = 0; x < maxslicex; x++ ) {

        Gauge.Next ( -1, SuperGaugeUpdateTitle );

        stat.Reset ();


        for ( int mi = 0; mi < nummrifiles; mi++ ) {

                                        // opening or just accessing
            mridoc[ mi ].Open ( mrifiles[ mi ], howopen );

                                        // !using a reference, which means filters will persist while the doc is open!
                                        // otherwise we would need to duplicate ALL arrays...
            Volume&         mridata         = *mridoc[ mi ]->GetData ();


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Preprocessing MRI before summation - only when needed

            if ( ! openall || x == 0 ) {
//                                      // Smoothing volume
//              p ( FilterParamDiameter )     = 3.47;
//              mridata.Filter ( FilterTypeMedian, p, false );

//                                      // Histogram Equalization, "à la" Brain style:
//              mridata.Filter ( FilterTypeHistoEqualBrain, p, false );

                                        // Ranking, brain tissues looking-style
//              p ( FilterParamThreshold )     = 0; // GetBackgroundValue ();
//              mridata.Filter ( FilterTypeRankRamp, p, false );

                                        // Linear rescaling to the robust max
                mridata        *= 100 / TEasyStats ( mridata, &mridata, true, true ).Quantile ( 0.999 );

                                                    // pre-processed volume before actual sum - for checking only
//              StringCopy          ( mrinormfile,  basefilename );
//              StringAppend        ( mrinormfile,  "BeforeSum", "." );
//              StringAppend        ( mrinormfile,  ToFileName ( mrifiles[ mi ] ) );
//              ReplaceExtension    ( mrinormfile,  DefaultMriExt );
//              mridata.WriteFile ( mrinormfile, &mridoc[ mi ]->GetOrigin (), &mridoc[ mi ]->GetVoxelSize (), &mridoc[ mi ]->GetRealSize () );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Computing new template from coregistered MRIs
                                        // Target (template) to Source (MRI) direct scanning

                                        // Setting the interpolation type only once
            if ( interpolate[ mi ] == InterpolateUnknown ) {

                interpolate[ mi ]       = mridoc[ mi ]->IsMask ()  ?   InterpolateNearestNeighbor  :   InterpolateLanczos3;
                                        // do we really need this?
                SetOvershootingOption    ( interpolate[ mi ], mridata.GetArray (), mridata.GetLinearDim (), true );
                }


            OmpParallelFor

            for ( int stati = 0; stati < avgdim2 * avgdim3; stati++ ) {
                                        // convert to linear index
                int         y       = stati / avgdim3;
                int         z       = stati % avgdim3;

                                            // relative template
                TPointDouble        pabs ( x, y, z );
                                            // to absolute
                pabs       -= avgorigin;


                TPointDouble        psource ( pabs );
                                            // to relative MRI
                CoregAbs_to_MriRel[ mi ].Apply ( psource );

//              psource    -= 0.5;          // commented to be consistent with TFitVolumeOnVolume::TransformToTarget / TFitVolumeOnVolume::TransformToSource

                stat ( stati ).Add ( mridata.GetValueChecked ( psource.X, psource.Y, psource.Z, interpolate[ mi ] ), ThreadSafetyIgnore );


                if ( symmetrical ) {

                    psource     = pabs;
                                            // flip left-right
                    psource.X   = - psource.X;
                                            // to relative MRI
                    CoregAbs_to_MriRel[ mi ].Apply ( psource );

//                  psource    -= 0.5;      // commented to be consistent with TFitVolumeOnVolume::TransformToTarget / TFitVolumeOnVolume::TransformToSource

                    stat ( stati ).Add ( mridata.GetValueChecked ( psource.X, psource.Y, psource.Z, interpolate[ mi ] ), ThreadSafetyIgnore );
                    } // symmetrical

                } // for stati / y, z


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we might be closing the MRI at each time, if memory appears to be short
            if ( ! openall )

                mridoc[ mi ].Close ();

            } // for mrifiles


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time to save our slice of medians
        OmpParallelFor

        for ( int stati = 0; stati < avgdim2 * avgdim3; stati++ ) {
                                    // convert to linear index
            int         y       = stati / avgdim3;
            int         z       = stati % avgdim3;

                                        // at that point of the game, we could use any stat that we could fancy...
            avgvol ( x, y, z )  = stat ( stati ).Median ( false );

            if ( symmetrical )

                avgvol ( avgdim1 - 1 - x, y, z )  = avgvol ( x, y, z );
            }

        } // for slice x


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !We modified each doc, revert them before the next iteration - otherwise it does not matter!
    if ( openall && numiterations > 0 )
    
        for ( int mi = 0; mi < nummrifiles; mi++ )
  
            mridoc[ mi ]->Revert ( false );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // template post-filtering
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Volume              bigmask ( avgvol );


    Gauge.Next ( -1, SuperGaugeUpdateTitle );

    p ( FilterParamToMaskThreshold )     = AtLeast ( 0.05, avgvol.GetBackgroundValue () * 0.5 ); // clip deeper, the border is a bit fuzzy
    p ( FilterParamToMaskNewValue  )     = 1;
    p ( FilterParamToMaskCarveBack )     = true;
    bigmask.Filter ( FilterTypeToMask, p );


    Gauge.Next ( -1, SuperGaugeUpdateTitle );
                                        // fast binary median
    p ( FilterParamDiameter )     = 3.47;
    p ( FilterParamNumRelax )     = 1;
    bigmask.Filter ( FilterTypeRelax, p, false );

                                        // apply mask to have nicer boundaries
    avgvol.ApplyMaskToData ( bigmask );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for smoother look
//  p ( FilterParamDiameter )     = 2.0;
//  avgvol.Filter ( FilterTypeMedian, p, false );

                                        // coregistration being not equally reliable across the volume, it creates a sort of Bias Field - better correct it for any forthcoming process
    p ( FilterParamBiasFieldRepeat )     = 1;
    avgvol.Filter ( FilterTypeBiasField, p, true );

                                        // Linear rescaling to the robust max
    avgvol         *= 100 / TEasyStats ( avgvol, &bigmask, true, true ).Quantile ( 0.999 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Writing new template to file
    Gauge.Next ( -1, SuperGaugeUpdateTitle );

                                        // deleting previous template
    if ( islooping && deletetempgen )
        DeleteFileExtended ( avgfile, DefaultMriExt );


    StringCopy          ( avgfile,  basefilename );
    StringAppend        ( avgfile,  InfixTemplate );
    if ( ! islast )
        StringAppend    ( avgfile, ".",    InfixGeneration,    IntegerToString ( buff, li, NumIntegerDigits ( numiterations ) ) );
    AddExtension        ( avgfile,  DefaultMriExt );

                                        
    avgvol.WriteFile    ( avgfile, 
                          &avgorigin,   &refvoxelsize, 
                          0, 
                          orientationstring,
                          NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameBrain,
                          AtomFormatFloat /*AtomFormatByte*/
                        );

    } // for template iteration


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Final saves

for ( int mi = 0; mi < nummrifiles; mi++ ) {

    Gauge.Next ( -1, SuperGaugeUpdateTitle );

                                        // opening or just accessing
    mridoc[ mi ].Open ( mrifiles[ mi ], howopen );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Saving the absolute transform matrix

    TMatrix44           CoregAbs_to_MriAbs  = CoregAbs_to_MriRel[ mi ];

    TPointDouble        mriorigin ( mridoc[ mi ]->GetOrigin () );

                                        // adding "to relative brain", so final transform is AbsBrain to AbsTemplate
    CoregAbs_to_MriAbs.Translate   ( -mriorigin.X, -mriorigin.Y, -mriorigin.Z, MultiplyLeft );


    StringCopy          ( matfile,  basefilename );
    StringAppend        ( matfile,  InfixTemplate );
    StringAppend        ( matfile,  InfixToCoregistered );
    StringAppend        ( matfile,  ToFileName ( mrifiles[ mi ] ) );
    ReplaceExtension    ( matfile,  FILEEXT_TXT );


    CoregAbs_to_MriAbs.WriteFile    ( matfile );


    gofmatrices.Add     ( matfile );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( ! openall )

        mridoc[ mi ].Close ();

    } // for matrix


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int mi = 0; mi < nummrifiles; mi++ )

    mridoc[ mi ].Close ();


StringCopy      ( templatefile, avgfile );


Gauge.FinishPart ( -1 );
}


//----------------------------------------------------------------------------
                                        // The whole utility: template, grey, solution points
void    ComputingTemplateMri    (   const TGoF&         mrifiles,
                                    BuildTemplateType   howtemplate,
                                    TemplateSPType      howsp,
                                    const char*         mnifile,
                                    const char*         mnispfile,
                                    bool                templatesymmetrical,
                                    GreyMatterFlags     greyflags,
                                    int                 numsolpoints,           double          ressolpoints,
                                    GreyMatterFlags     spflags,
                                    double              precision,              int             numiterations,
                                    const char*         fileprefix,
                                    bool                savingcoregmris
                                )
{
if ( (int) mrifiles < 1 
  || ( howtemplate  == BuildTemplateMNI                 && StringIsEmpty ( mnifile   ) )
  || ( howsp        == TemplateLoadSPFromFile           && StringIsEmpty ( mnispfile ) )
  || ( howsp        == TemplateExtractSPFromTemplate    && numsolpoints <= 0 && ressolpoints <= 0 ) )

    return;

int                 nummrifiles     = (int) mrifiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseFileName;
TFileName           VerboseFile;
TFileName           templatefile;
TFileName           greyfile;
TFileName           spfile;
TFileName           sptomrifile;
TGoF                gofmatrices;
TGoF                gofsolpoints;
char                buff[ 256 ];

                                        // Output will start below the first given file
StringCopy          ( BaseFileName,             mrifiles[ 0 ] );
RemoveFilename      ( BaseFileName, true );
StringAppend        ( BaseFileName,             StringIsEmpty ( fileprefix ) ? ( howtemplate  == BuildTemplateMNI ? "MNI" InfixTemplate : "Self" InfixTemplate ) : fileprefix );  // sub-directory
                                        // File name prefix
AppendFilenameAsSubdirectory ( BaseFileName );
StringAppend        ( BaseFileName,             "." );

CreatePath          ( BaseFileName, true );


StringCopy          ( VerboseFile,              BaseFileName );
StringAppend        ( VerboseFile,              InfixTemplate );
AddExtension        ( VerboseFile,              FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Computing Template MRI" );


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Number of input files:", nummrifiles );

for ( int fi = 0; fi < nummrifiles; fi++ )
    verbose.Put ( fi ? "" : "Input file:", mrifiles[ fi ] );

if ( howtemplate == BuildTemplateMNI ) {
    verbose.NextLine ();
    verbose.Put ( "MNI brain file:", mnifile );
    }

if ( howsp  == TemplateLoadSPFromFile ) {
    verbose.NextLine ();
    verbose.Put ( "MNI Solution Points file:", mnispfile );
    }
}


verbose.NextTopic ( "Parameters:" );
{
verbose.Put ( "Template geometrical reference:", howtemplate == BuildTemplateMNI ? "MNI brain template" : "Self-referenced template" );
verbose.Put ( "Template is:", templatesymmetrical ? "Symmetrical" : "Asymmetrical" );


verbose.NextLine ();
verbose.Put ( "Source intensity levels adjustment:", RemapIntensityNames[ RemapIntensityRank ] );
verbose.Put ( "Target intensity levels adjustment:", RemapIntensityNames[ RemapIntensityRank ] );


if ( howtemplate == BuildTemplateSelfRef ) {
                                        // there are 2 different stages, each with their own parameters
    verbose.NextLine ();
    CoregTemplate[ howtemplate ][ CoregistrationBoot ].ToVerbose ( verbose );

    verbose.NextLine ();
    CoregTemplate[ howtemplate ][ CoregistrationLoop ].ToVerbose ( verbose );
    }

else if ( howtemplate == BuildTemplateMNI ) {

    verbose.NextLine ();
    CoregTemplate[ howtemplate ][ CoregistrationBoot ].ToVerbose ( verbose );
    }


verbose.NextLine ();
verbose.Put ( "Convergence precision:", FloatToString ( precision ) );
//verbose.Put ( "Number of iterations:", numiterations );


if ( howsp  == TemplateExtractSPFromTemplate ) {
    verbose.NextLine ();
    verbose.Put ( "Grey Matter Mask is:",           "Extracted from template" );
    verbose.Put ( "Grey Matter Mask thickness:",    GreyMatterProcessingToString ( greyflags ) );
    verbose.Put ( "Grey Matter Mask shape:",        GreyMatterSymmetryToString ( greyflags ) );
    }
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                GaugeTemplBrainEnum
                    {
                    gaugetemplmriglobal,
                    gaugetemplmrinormbrain,
                    gaugetemplmriloop,
                    };

TSuperGauge         Gauge;

Gauge.Set           ( TemplateMriTitle, SuperGaugeLevelInter );

Gauge.AddPart       ( gaugetemplmriglobal,      5 + 1,           6 );
Gauge.AddPart       ( gaugetemplmrinormbrain,   nummrifiles,    20 );
Gauge.AddPart       ( gaugetemplmriloop,        123,            74 );


//if ( IsBatchFirstCall () && (int) BatchFileNames > 5 )
if ( nummrifiles > 5 )
                                        // batch can be long, hide Cartool until we are done
    WindowMinimize ( CartoolObjects.CartoolMainWindow );


CartoolObjects.CartoolApplication->SetMainTitle ( TemplateMriTitle, BaseFileName, Gauge );

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Normalize all brains by themselves
Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


NormalizeBrainResults*  allnorms        = 0;
TVector3Int             mrimaxdim;


Gauge.CurrentPart   = gaugetemplmrinormbrain;


NormalizeBrains (   mrifiles,
                    allnorms,
                    mrimaxdim,
                    Gauge
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Compute the actual template
Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


Gauge.CurrentPart   = gaugetemplmriloop;

TEasyStats          quality;


MergeMris       (   mrifiles,
                    howtemplate,
                    allnorms,
                    mnifile,
                    mrimaxdim,
                    precision,      numiterations,
                    templatesymmetrical,
                    BaseFileName,
                    templatefile,   gofmatrices,
                    savingcoregmris,
                    quality,
                    Gauge
                );

                                        // opening template
TOpenDoc< TVolumeDoc >  templatedoc ( templatefile, OpenDocVisible /*OpenDocHidden*/ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Extracting grey matter from template
Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );

TPoints             solpoints;
TStrings            spnames;


if      ( howsp  == TemplateExtractSPFromTemplate ) {

    StringCopy          ( greyfile, templatefile );
    PostfixFilename     ( greyfile, "." InfixGrey );

    CopyFileExtended    ( templatefile, greyfile, FILEEXT_MRIAVW_IMG );


    TOpenDoc< TVolumeDoc >  greydoc ( greyfile, /*OpenDocVisible*/ OpenDocHidden );


    FctParams           p;

    p ( FilterParamGreyType   )     = greyflags;
    p ( FilterParamGreyAxis   )     = 0;
    p ( FilterParamGreyOrigin )     = greydoc->GetOrigin ()[ greydoc->GetAxisIndex ( LeftRight ) ];

    greydoc->GetData ()->Filter ( FilterTypeSegmentGrey, p, true );


    StringCopy ( greydoc->GetNiftiIntentName (), NiftiIntentNameGreyMask );

    greydoc->Commit  ( true );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) extracting solution points
    Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


    NeighborhoodType    lolauneighborhood   = InverseNeighborhood;


    StringCopy          ( spfile, templatefile );
    ReplaceExtension    ( spfile, FILEEXT_SPIRR );


    ComputeSolutionPoints (   
                        templatedoc,        greydoc,
                        numsolpoints,       ressolpoints,
                        spflags,    
                        lolauneighborhood,  lolauneighborhood,
                        solpoints,          spnames,
                        spfile
                        );

                                        // opening to update most recent file list
    TOpenDoc< TSolutionPointsDoc >  spidoc ( spfile, /*OpenDocVisible*/ OpenDocHidden );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( howsp  == TemplateLoadSPFromFile ) {

    Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


    StringCopy          ( spfile, templatefile );
    ReplaceExtension    ( spfile, FILEEXT_SPIRR );

    CopyFileExtended    ( mnispfile, spfile );


    TOpenDoc< TSolutionPointsDoc >  spidoc ( spfile, /*OpenDocVisible*/ OpenDocHidden );

    solpoints   = spidoc->GetPoints ( DisplaySpace3D );
    spnames     = *spidoc->GetSPNames ();

                                        // !if not in RAS / MNI space, apply some transform from MNI doc!
    }

else
    Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


if ( howsp  != TemplateNoSP ) {

    verbose.NextLine ();
    //verbose.Put ( "Number of solution points:", numsolpoints );   // requested number

    if ( ressolpoints > 0 )
        verbose.Put ( "Solution Points resolution:", ressolpoints, 2, " [mm]" );

    verbose.Put ( "Number of solution points:", (int) solpoints );  // actual number

    if ( howsp  == TemplateExtractSPFromTemplate )
        verbose.Put ( "Solution points shape:",         GreyMatterSymmetryToString ( spflags ) );

    verbose.Flush ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Converting solution points back to each individual's MRI (absolute) space
Gauge.Next ( gaugetemplmriglobal, SuperGaugeUpdateTitle );


if ( howsp  != TemplateNoSP ) {

    TMatrix44           templabstomribas;
    TPoints             solpointstomri;


    for ( int mi = 0; mi < nummrifiles; mi++ ) {

                                            // going from template to MRI
        templabstomribas    = TMatrix44 ( gofmatrices[ mi ] );

                                            // applying common solution points back to particular subject's MRI
        solpointstomri      = solpoints;

        templabstomribas.Apply ( solpointstomri );

                                            // save it!
        StringCopy          ( sptomrifile,  BaseFileName );
        StringAppend        ( sptomrifile,  ToFileName ( mrifiles[ mi ] ) );
        RemoveExtension     ( sptomrifile );
        AddExtension        ( sptomrifile,  FILEEXT_SPIRR );

        gofsolpoints.Add    ( sptomrifile );


        solpointstomri.WriteFile ( sptomrifile, &spnames );

        } // for gofmatrices
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Verbose file (this):", VerboseFile );


verbose.NextLine ();
if ( templatefile.IsNotEmpty () )   verbose.Put ( "Template file:", templatefile );
if ( greyfile    .IsNotEmpty () )   verbose.Put ( "Template grey matter file:", greyfile );
if ( spfile      .IsNotEmpty () )   verbose.Put ( "Template solution points file:", spfile );


verbose.NextLine ();
verbose.Put ( "Number of matrix files:", gofmatrices.NumFiles () );
for ( int fi = 0; fi < gofmatrices.NumFiles (); fi++ )
    verbose.Put ( fi ? "" : "Matrix file:", gofmatrices[ fi ] );


if ( howsp  != TemplateNoSP ) {

    verbose.NextLine ();
    verbose.Put ( "Number of solution points files:", gofsolpoints.NumFiles () );
    for ( int fi = 0; fi < gofsolpoints.NumFiles (); fi++ )
        verbose.Put ( fi ? "" : "Solution points file:", gofsolpoints[ fi ] );
    }
}


verbose.NextTopic ( "Results:" );
{
verbose.Put ( "Mean coregistration quality [%]:", quality.Mean (), 1 );
verbose.Put ( "Mean coregistration diagnostic :", TFitVolumeOnVolume::GetQualityOpinion ( quality.Mean () ) );

verbose.NextLine ();
verbose.Put ( "Coregistration quality for each MRI:", "" );
for ( int i = 0; i < (int) quality; i++ )
    verbose.Put ( ToFileName ( mrifiles[ i ] ), quality[ i ], 1, StringCopy ( buff, " / ", TFitVolumeOnVolume::GetQualityOpinion ( quality[ i ] ) ) );
}


verbose.NextLine ();
verbose.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( allnorms )
    delete[]    allnorms;

templatedoc .Close  ( CloseDocLetOpen );
//greydoc   .Close  ( CloseDocLetOpen );
//spidoc    .Close  ( CloseDocLetOpen );


WindowMaximize ( CartoolObjects.CartoolMainWindow );

Gauge.FinishParts ();

CartoolObjects.CartoolApplication->SetMainTitle ( TemplateMriTitle, BaseFileName, Gauge );

Gauge.HappyEnd ();

SetProcessPriority ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
