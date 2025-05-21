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

#include    "Volumes.SagittalTransversePlanes.h"

#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Math.TMatrix44.h"

#include    "GlobalOptimize.h"
#include    "GlobalOptimize.Points.h"
#include    "GlobalOptimize.Volumes.h"

#include    "TVolumeDoc.h"
#include    "TVolumeView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Saving a fake xyz file to show the sagittal plane
void    SaveSagittalPlane       (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    normtomriabs,
                                    const char*         fileoriginal,
                                    const char*         filetransformed
                                )
{
TPoints             pointstransf;
TPoints             pointsorigin;

double              ext             = mridoc->GetBounding ()->Radius ();
        
pointstransf.Add ( 0, -ext, -ext );
pointstransf.Add ( 0, -ext,  ext );
pointstransf.Add ( 0,  ext, -ext );
pointstransf.Add ( 0,  ext,  ext );

if ( StringIsNotEmpty ( filetransformed ) )
    pointstransf.WriteFile ( filetransformed, 0, 0, -1, ClusterGrid );  // force grid type for proper triangles orientation
        

pointsorigin    = pointstransf;

normtomriabs.Apply ( pointsorigin );

if ( StringIsNotEmpty ( fileoriginal ) )
    pointsorigin.WriteFile ( fileoriginal, 0, 0, -1, ClusterGrid );     // force grid type for proper triangles orientation
}


void    SaveTransversePlane     (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    normtomriabs,
                                    const char*         fileoriginal,
                                    const char*         filetransformed
                                )
{
TPoints             pointstransf;
TPoints             pointsorigin;

double              ext             = mridoc->GetBounding ()->Radius ();
        
pointstransf.Add ( -ext, -ext, 0 );
pointstransf.Add ( -ext,  ext, 0 );
pointstransf.Add (  ext, -ext, 0 );
pointstransf.Add (  ext,  ext, 0 );

if ( StringIsNotEmpty ( filetransformed ) )
    pointstransf.WriteFile ( filetransformed, 0, 0, -1, ClusterGrid );  // force grid type for proper triangles orientation
        

pointsorigin    = pointstransf;

normtomriabs.Apply ( pointsorigin );

if ( StringIsNotEmpty ( fileoriginal ) )
    pointsorigin.WriteFile ( fileoriginal, 0, 0, -1, ClusterGrid );     // force grid type for proper triangles orientation
}


//----------------------------------------------------------------------------
                                        // Input MRIDoc & reorientation matrix (Local -> RAS)
                                        // Output a new normalization matrix & center point
                                        // Returns a quality measure
double  SearchSagittalPlaneMri  (   const TVolumeDoc*   mridoc,
                                    const TMatrix44&    MriRel_to_MriAbs,
                                    TMatrix44&          SagAbs_to_MriRel,   // input & output
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    int                 how,
                                    double              precision,
                                    const char*         title,
                                    bool                verbose
                                )
{
TPointDouble        originalcenter    ( center );


TVolumeProperties   gomri   (   mridoc, 
                                MriRel_to_MriAbs,
                                -1,
                                &SagAbs_to_MriRel,  // input
                                &center             // input
                            );
TEasyStats          gomriq;

double              centerx         = gomri.Center          [ gomri.LeftRightIndex ];
double              deltax          = gomri.Bound.GetRadius ( gomri.LeftRightIndex ) * 0.20;
double              roty            = 7.5;
double              rotz            = 7.5;


gomri.Set ( GOStepsDefault );

gomri.AddGroup ();
gomri.AddDim   ( TranslationX, centerx - deltax, centerx + deltax );

gomri.AddGroup ();
gomri.AddDim   ( RotationY, -roty, roty );
gomri.AddDim   ( RotationZ, -rotz, rotz );


gomri.GetSolution   (   gomethod,       // Global Optimization technique
                        how,            // Which plane to search for
                        precision,  0, 
                        title, 
                        &gomriq
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // compare the results with the original ranges
double              suspicioustrx   = RoundTo ( fabs ( gomri.GetValue ( TranslationX ) - centerx ) / deltax, 0.01 );
double              suspiciousroty  = RoundTo ( fabs ( gomri.GetValue ( RotationY    )           ) / roty,   0.01 );
double              suspiciousrotz  = RoundTo ( fabs ( gomri.GetValue ( RotationZ    )           ) / rotz,   0.01 );

//bool                suspicious      = suspicioustrx  > 2.00
//                                   || suspiciousroty > 2.00
//                                   || suspiciousrotz > 2.00;

                                        // parameters energy: more transform -> more energy
double              mintransf   = sqrt ( Square ( suspicioustrx ) + Square ( suspiciousroty ) + Square ( suspiciousrotz ) );
                                        // difference energy: more difference -> more energy
double              mindiff     = gomriq.Average ();
                                        // merge both: we need to minimize both the amount of transformation and the final difference
double              Q           = sqrt ( mintransf * mindiff );


if ( verbose ) {

//    DBGV7 ( RoundTo ( gomri.GetValue ( TranslationX ), 0.1 ) /*- centerx*/, RoundTo ( gomri.GetValue ( RotationY ), 0.1 ), RoundTo ( gomri.GetValue ( RotationZ ), 0.1 ),
//            suspicioustrx * 100, suspiciousroty * 100, suspiciousrotz * 100,
//            Q,
//            GetGOMethodName ( gomethod ) );
//
//    gomriq.Show ( "Sagittal Plane Search Quality" );

    DBGV3 ( Round ( mintransf * 100 ), Round ( mindiff * 100 ), Round ( Q * 100 ), "mintransf, mindiff -> Q" );

                                            // not looking good?
//  if ( suspicious )
//      ShowMessage ( "Sagittal Plane search encountered some problems!", GetGOMethodName ( gomethod ), ShowMessageWarning );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save the transformation matrix, even if results are suspicious
SagAbs_to_MriRel    = gomri.NormToMRI;
                                        // save new center
center              = gomri.Center;

                                        // check for a null transform (less than 0.10 voxel shift & less than 0.10° rotations)
if ( fabs ( originalcenter[ gomri.LeftRightIndex ] - center[ gomri.LeftRightIndex ] ) < 0.10
  && fabs ( gomri.GetValue ( RotationY ) ) < 0.10 
  && fabs ( gomri.GetValue ( RotationZ ) ) < 0.10 ) {

    if ( verbose )
        ShowMessage ( "Sagittal Plane transform is close to identity." NewLine "Resetting it!", GetGOMethodName ( gomethod ), ShowMessageWarning );

    center[ gomri.LeftRightIndex ]  = originalcenter[ gomri.LeftRightIndex ];
    SagAbs_to_MriRel.SetIdentity ();
    SagAbs_to_MriRel.SetTranslation ( center[ 0 ], center[ 1 ], center[ 2 ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  Q;
}


//----------------------------------------------------------------------------
                                        // Runs different combinations of parameters, when we don't know which case is relevant (T1, T1Gad, head, brain, weird cases...)
bool    SetSagittalPlaneMri     (   const TVolumeDoc*   mridoc,     
                                    const TMatrix44&    MriRel_to_MriAbs,
                                    TMatrix44&          SagAbs_to_MriRel,   // input & output   
                                    TPointDouble&       center,             // input & output
                                    double*             Q,
                                    const char*         title
                                )
{
TMatrix44           originalSagAbs_to_MriRel ( SagAbs_to_MriRel );
TPointDouble        originalcenter           ( center    );
TMatrix44           bestSagAbs_to_MriRel;
TPointDouble        bestcenter;
double              bestQ               = DBL_MAX;
int                 bestcombo           = -1;
GOMethod            gomethod;
int                 how;
char                localtitle[ 256 ]   = "";


constexpr double    MinQ                = 0.10;     // Very little to be transformed? like 0 to 10?
constexpr double    MaxQ                = 0.70;     // Looking suspicious: Max OK:66, Min Not OK:71

                                            // Repeat search with different parameters
for ( int combo = 0; combo < 3; combo++ ) {
                                        // !set before each run!
    SagAbs_to_MriRel    = originalSagAbs_to_MriRel;

    center              = originalcenter;

                                        // After a lot of testing, there appear to be 3 main contenders:
    if      ( combo == 0 )  {   gomethod    = CyclicalCrossHairScan;    how     = SagittalPlaneSymmetric;       }   // T1 Head, T1 Brain, T1 Weird Head, T1Gad Weird Head, T2 Head, T2 Brain
    else if ( combo == 1 )  {   gomethod    = CyclicalCrossHairScan;    how     = SagittalPlaneSymmetricT1;     }   // T1 Head
    else                    {   gomethod    = GlobalCrossHairScan;      how     = SagittalPlaneSymmetricT1Gad;  }   // T1Gad Head

    if ( StringIsNotEmpty ( title ) )
        StringCopy      ( localtitle, title, " v", IntegerToString ( combo + 1 ) );

    double          Q           = SearchSagittalPlaneMri(   mridoc,
                                                            MriRel_to_MriAbs,
                                                            SagAbs_to_MriRel,   // input & output
                                                            center,             // input & output
                                                            gomethod,
                                                            how,
                                                            GODefaultConvergence,
                                                            localtitle
                                                        );

//    DBGV2 ( combo + 1, Round ( Q * 100 ), "SetSagittalPlaneMri: combo -> Q" );

                                        // is it the best estimate so far, null transform included?
    if ( Q < bestQ ) {
        bestcombo           = combo;
        bestQ               = Q;
        bestSagAbs_to_MriRel= SagAbs_to_MriRel;
        bestcenter          = center;
        }

    } // for combo

//DBGV2 ( bestcombo + 1, Round ( bestQ * 100 ), "SetSagittalPlaneMri: Best combo -> Q" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Q != 0 )
    *Q  = bestQ;

                                        // Check if transform is really to be applied:
if ( IsInsideLimits ( bestQ, MinQ, MaxQ ) ) {
                                        // copy the best transform results
    SagAbs_to_MriRel    = bestSagAbs_to_MriRel;

    center              = bestcenter;
                                        // we good
    return  true;
    }

else {                                  // either close to no transform at all, or way too big

    SagAbs_to_MriRel   = TMatrix44 ( originalSagAbs_to_MriRel ).Invert ();    // because sagittal search actually returns an "inverted" matrix

    center              = originalcenter;
                                        // it might miss the translation part?
    if ( SagAbs_to_MriRel.HasNoTranslation ()  )
        SagAbs_to_MriRel.SetTranslation ( center );
                                        // report error only if high Q, otherwise report as OK
    return  bestQ <= MinQ;
    }
}


//----------------------------------------------------------------------------

double  SearchTransversePlaneMri(   const TVolumeDoc*   mribrain,
                                    const TMatrix44&    MriRel_to_MriAbs,   // not actually used
                                    TMatrix44&          TraAbs_to_MriRel,   // input & output
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    int                 how,
                                    double              precision,
                                    const char*         title,
                                    bool                verbose
                                )
{
TPointDouble        originalcenter    ( center    );


TVolumeProperties   gomri   (   mribrain, 
                                MriRel_to_MriAbs,
                                -1,
                                &TraAbs_to_MriRel,  // input
                                &center             // input
                            );
TEasyStats          gomriq;


gomri.Set ( GOStepsDefault );


double              centerz         = gomri.Center[ gomri.UpDownIndex ];
double              deltaminz       = gomri.Bound.GetRadius ( gomri.UpDownIndex    ) * 0.60;
double              deltamaxz       = gomri.Bound.GetRadius ( gomri.UpDownIndex    ) * 0.60;

double              centery         = gomri.Center[ gomri.FrontBackIndex ];
double              deltaminy       = gomri.Bound.GetRadius ( gomri.FrontBackIndex ) * 0.60;
double              deltamaxy       = gomri.Bound.GetRadius ( gomri.FrontBackIndex ) * 0.60;

                                        // ?seems to work on all cases - updated for Brain Coregistration, after a Sagittal search?
//double            centerz         = gomri.Center[ 2 ];
//double            deltaminz       = gomri.Bound.GetRadius ( 2 ) * 0.40;
//double            deltamaxz       = gomri.Bound.GetRadius ( 2 ) * 0.80;
//double            centery         = gomri.Center[ 1 ];
//double            deltaminy       = gomri.Bound.GetRadius ( 1 ) * 0.40;
//double            deltamaxy       = gomri.Bound.GetRadius ( 1 ) * 0.80;

                                        // rotations are exclusively found to be > 0, heads are rather tilted backward than inward
                                        // also, some heads are tilted A LOT (>20°), so give it some room here
double              rotxmiddle      =  0;   // !could be already oriented to MNI, so start around 0 just in case!
double              rotxdelta       = 60;   // but then with even more room for non-aligned MRIs!

double              scalex          = 0;
double              scaley          = 0;
double              scalez          = 0;

                                        // using MNI brain size predefined values for the moment:
constexpr double    MniBrainSizeX   = 144;
constexpr double    MniBrainSizeY   = 181;
constexpr double    MniBrainSizeZ   = 154;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                istrmni         = IsTransversePlaneMNI ( how );

if ( istrmni ) {

                                        // doing a first fast transverse plane
    TVolumeProperties   gomrifast   (   mribrain,
                                        MriRel_to_MriAbs,
                                        -1
                                    );

    gomrifast.Set ( GOStepsDefault );


    gomrifast.AddGroup ();
    gomrifast.AddDim   ( TranslationZ, centerz - deltaminz, centerz + deltamaxz );
    gomrifast.AddDim   ( TranslationY, centery - deltaminy, centery + deltamaxy );


    gomrifast.AddGroup ();
    gomrifast.AddDim   ( RotationX, rotxmiddle - rotxdelta, rotxmiddle + rotxdelta );


    char                localtitle[ 256 ]   = "";

    if ( StringIsNotEmpty ( title ) )
        StringCopy      ( localtitle, title, "-Raw" );

                                        // this plane estimate is robust and fast
    gomrifast.GetSolution   (   GlobalNelderMead,               // bypassing gomethod
                                TransversePlaneBiggestSurface, 
                                precision,      0,
                                localtitle
                            );

    //DBGV3 ( gomrifast.GetValue ( TranslationZ ), gomrifast.GetValue ( TranslationY ), gomrifast.GetValue ( RotationX ), "Fast: TranslationZ, TranslationY, RotationX" );

                                        // we have now more precise estimates of initial values
    double      finecenterz         = gomrifast.GetValue ( TranslationZ );
    double      finecentery         = gomrifast.GetValue ( TranslationY ) + 0.10 * gomri.Bound.GetExtent ( gomri.FrontBackIndex );  // expected y center is known to be a bit forward
    double      finerotxmiddle      = gomrifast.GetValue ( RotationX );
    double      finerotxdelta       = 12;               // should be less, like 5, but give it some slack

                                        // Note that this also could be used as a fallback if the next search seems to fail...

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // and now we can run the full search, with all needed parameters

    gomri.AddGroup ();
    gomri.AddDim   ( TranslationZ, finecenterz - deltaminz / 2, finecenterz + deltamaxz / 2 );
    gomri.AddDim   ( TranslationY, finecentery - deltaminy / 2, finecentery + deltamaxy / 2 );


    gomri.AddGroup ();
    gomri.AddDim   ( RotationX, finerotxmiddle - finerotxdelta, finerotxmiddle + finerotxdelta );

                                        // For MNI-like slice convergence, it also needs some re-scaling
    const TBoundingBox<double>* boundfrom       = mribrain   ->GetBounding ();
  //TBoundingBox<double>*       boundto         = mnislicedoc->GetBounding ();
    TBoundingBox<double>        boundto ( 0, MniBrainSizeX, 0, MniBrainSizeY, 0, MniBrainSizeZ );   // using predefined size for the moment...

    scalex      = boundfrom->GetRadius ( gomri.LeftRightIndex ) / boundto.GetRadius ( 0 );
    scaley      = boundfrom->GetRadius ( gomri.FrontBackIndex ) / boundto.GetRadius ( 1 );
    scalez      = boundfrom->GetRadius ( gomri.UpDownIndex    ) / boundto.GetRadius ( 2 );

    gomri.AddGroup ();
    gomri.AddDim   ( ScaleX, scalex * 0.90, scalex * 1.10 );
    gomri.AddDim   ( ScaleY, scaley * 0.90, scaley * 1.10 );
    gomri.AddDim   ( ScaleZ, scalez * 0.90, scalez * 1.10 );


    if ( StringIsNotEmpty ( title ) )
        StringCopy      ( localtitle, title, "-Precise" );


    gomri.GetSolution   (   gomethod, 
                            how, 
                            precision,  0, 
                            localtitle,
                            &gomriq
                        );

    //DBGV3 ( gomri.GetValue ( TranslationZ ), gomri.GetValue ( TranslationY ), gomri.GetValue ( RotationX ), "Best: TranslationZ, TranslationY, RotationX" );
    } // if TransversePlaneMNI


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // else ! TransversePlaneMNI

    gomri.AddGroup ();
    gomri.AddDim   ( TranslationZ, centerz - deltaminz, centerz + deltamaxz );


    gomri.AddGroup ();
    gomri.AddDim   ( RotationX, rotxmiddle - rotxdelta, rotxmiddle + rotxdelta );


    gomri.GetSolution   (   gomethod, 
                            how, 
                            precision,  0, 
                            title,
                            &gomriq
                        );
    } // else ! TransversePlaneMNI


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compare the results with the original ranges
double              suspicioustrz       = RoundTo (             fabs ( gomri.GetValue ( TranslationZ ) - centerz    ) / max ( deltaminz, deltamaxz ),      0.01 );
double              suspicioustry       = RoundTo ( istrmni ?   fabs ( gomri.GetValue ( TranslationY ) - centery    ) / max ( deltaminy, deltamaxy )  : 0, 0.01 );
double              suspiciousrotx      = RoundTo (             fabs ( gomri.GetValue ( RotationX    ) - rotxmiddle ) / rotxdelta,                         0.01 );
double              suspiciousscaley    = RoundTo ( istrmni ?   fabs ( gomri.GetValue ( ScaleY       ) - scaley     ) / scaley                        : 0, 0.01 );
double              suspiciousscalez    = RoundTo ( istrmni ?   fabs ( gomri.GetValue ( ScaleZ       ) - scalez     ) / scalez                        : 0, 0.01 );

                                        // for non-Nelder-Mead, the convergence can go beyond the original limits
                                        // but if too far, that means the convergence is really suspicious
//bool                suspicious          = suspicioustrz     >= 1.0
//                                       || suspicioustry     >= 1.5      // give some extra room for Y translation, as the AC is quite forward actually
//                                       || suspiciousrotx    >= 1.0
//                                       || suspiciousscaley  >= 1.0
//                                       || suspiciousscalez  >= 1.0;

                                        // parameters energy: more transform -> more energy
double              mintransf   = sqrt (   Square ( suspicioustrz  ) + Square ( suspicioustry  )
                                         + Square ( suspiciousrotx )
                                      // + Square ( suspiciousscaley ) + Square ( suspiciousscalez )    // skipping parameters that will specifically penalize the MNI slices search
                                       );
                                        // statistics are different for each type of search
double              mindiff     = how == TransversePlaneLongest         ?   100.0 /        gomriq.Sum ()
                                : how == TransversePlaneBiggestBox      ?   100.0 / sqrt ( gomriq.Sum () * ( MniBrainSizeY / MniBrainSizeZ ) )              // rescale to corresponding length
                                : how == TransversePlaneBiggestSurface  ?   100.0 / sqrt ( gomriq.Sum () * ( MniBrainSizeY / MniBrainSizeZ ) * ( 4 / Pi ) ) // idem + rescale surface from circle to square
                                : how == TransversePlaneMNIT1           ?                  gomriq.Average ()
                                : how == TransversePlaneMNIT2           ?                  gomriq.Average ()
                                :                                           1;
                                        // merge both: we need to minimize both the amount of transformation and the final difference
double              Q           = sqrt ( mintransf * mindiff );


if ( verbose ) {

//  DBGV5 ( gomri.GetValue ( TranslationZ ), gomri.GetValue ( TranslationY ), gomri.GetValue ( RotationX ), gomri.GetValue ( ScaleY ), gomri.GetValue ( ScaleZ ), "TrZ, TrY, RotX, ScaleY, ScaleZ" );
//  DBGV6 ( suspicioustrz * 100, suspicioustry * 100, suspiciousrotx * 100, suspiciousscaley * 100, suspiciousscalez * 100,
//          Q,
//          GetGOMethodName ( gomethod ) );
    DBGV3 ( Round ( mintransf * 100 ), Round ( mindiff * 100 ), Round ( Q * 100 ), "mintransf, mindiff -> Q" );
                                            // not looking good?
//  if ( suspicious )
//      ShowMessage ( "Transverse Plane search encountered some problems!", GetGOMethodName ( gomethod ), ShowMessageWarning );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save the transformation matrix, even if results are suspicious
TraAbs_to_MriRel    = gomri.NormToMRI;
                                        // save new center
center              = gomri.Center;

                                        // Correcting from MNI to Talairach - It appears MNI is a bit higher than Talairach?
//#define     MniConvergenceCenterShiftY      +0.5
//#define     MniConvergenceCenterShiftZ      -3.0
//center     += TPointDouble ( 0, 
//                             MniConvergenceCenterShiftY / mribrain->GetVoxelSize ().Y, 
//                             MniConvergenceCenterShiftZ / mribrain->GetVoxelSize ().Z
//                           );

center.RoundTo ( 0.1 );
                                        // if SliceExtent is skipped, call this
//gomri.CenterSlice ();

                                        // check for a null transform (less than 0.10 voxel shift & less than 0.10° rotations)
if ( fabs ( originalcenter[ gomri.UpDownIndex    ] - center[ gomri.UpDownIndex    ] ) < 0.10
  && fabs ( originalcenter[ gomri.FrontBackIndex ] - center[ gomri.FrontBackIndex ] ) < 0.10
  && fabs ( gomri.GetValue ( RotationX ) ) < 0.10 
//  && fabs ( gomri.GetValue ( ScaleY    ) )    // ?
//  && fabs ( gomri.GetValue ( ScaleZ    ) ) 
    ) {

    if ( verbose )
        ShowMessage ( "Transverse Plane transform is close to identity." NewLine "Resetting it!", GetGOMethodName ( gomethod ), ShowMessageWarning );

    center[ gomri.UpDownIndex    ]  = originalcenter[ gomri.UpDownIndex    ];
    center[ gomri.FrontBackIndex ]  = originalcenter[ gomri.FrontBackIndex ];
    TraAbs_to_MriRel.SetIdentity ();
    TraAbs_to_MriRel.SetTranslation ( center[ 0 ], center[ 1 ], center[ 2 ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  Q;
}


//----------------------------------------------------------------------------
                                        // Try to find the most MNI-looking slice
bool    SetTransversePlaneMri   (   const TVolumeDoc*   mribrain, 
                                    const TMatrix44&    MriRel_to_MriAbs,   // not actually used
                                    TMatrix44&          TraAbs_to_MriRel,   // input & output
                                    TPointDouble&       center,             // input & output
                                    double*             Q,
                                    const char*         title
                                )
{
TMatrix44           originalTraAbs_to_MriRel ( TraAbs_to_MriRel );
TPointDouble        originalcenter           ( center    );
TMatrix44           bestTraAbs_to_MriRel;
TPointDouble        bestcenter;
double              bestQ               = DBL_MAX;
int                 bestcombo           = -1;
GOMethod            gomethod;
int                 how;
char                localtitle[ 256 ]   = "";


constexpr double    MinQ                = 0.08;     // less than 0.10
constexpr double    ReliableMniQ        = 0.35;     // below this, there is no need for backup searches, as MNI results are quite reliable
constexpr double    MaxQ                = 1.00;     // quite liberal, because backup searches will always succeed

                                        // Repeat search with different parameters
for ( int combo = 0; combo < 4; combo++ ) {

    bool    islastmnisearch         = combo == 1;
    bool    isfirstbackupsearch     = combo == 2;


    if ( isfirstbackupsearch )
        bestQ   = DBL_MAX;              // force reset bestQ before running the backup searches, as Q factors do not compare nicely

                                        // !set before each run!
    TraAbs_to_MriRel    = originalTraAbs_to_MriRel;

    center              = originalcenter;

                                        // Testing T1 vs T2 slices + 2 backup searches (might not be necessary according to tests from 2023-11)
    if      ( combo == 0 )  {   gomethod    = GlobalNelderMead;     how     = TransversePlaneMNIT1;         }   // T1 slice
    else if ( combo == 1 )  {   gomethod    = GlobalNelderMead;     how     = TransversePlaneMNIT2;         }   // T2 slice
    else if ( combo == 2 )  {   gomethod    = GlobalNelderMead;     how     = TransversePlaneBiggestBox;    }   // backup search 1 - Biggest Box works well on weirdly shaped brains
    else                    {   gomethod    = GlobalNelderMead;     how     = TransversePlaneBiggestSurface;}   // backup search 2 - Biggest Surface works results are very similar to the AC-PC transverse slice

    if ( StringIsNotEmpty ( title ) )
        StringCopy      ( localtitle, title, " v", IntegerToString ( combo + 1 ) );

    double          Q           = SearchTransversePlaneMri( mribrain,
                                                            MriRel_to_MriAbs,               // not actually used
                                                            TraAbs_to_MriRel,               // input & output
                                                            center,                         // input & output
                                                            gomethod,
                                                            how,
                                                            1e-3, // GODefaultConvergence,  // 1e-3 is good enough and will produce around 0.10 voxel difference, and at max 0.3 voxel, while saving 40% of time
                                                            localtitle
                                                        );

//  DBGV2 ( combo + 1, Round ( Q * 100 ), "SetTransversePlaneMri: combo -> Q" );

                                        // is it the best estimate so far, null transform included?
    if ( Q < bestQ ) {
        bestcombo           = combo;
        bestQ               = Q;
        bestTraAbs_to_MriRel= TraAbs_to_MriRel;
        bestcenter          = center;
        }

                                        // do we have an acceptable results without resorting to a backup?
    if ( islastmnisearch && bestQ < ReliableMniQ )
        break;                          // yes, we can stop here, otherwise we will resort to backup plans

    } // for combo

//DBGV2 ( bestcombo + 1, Round ( bestQ * 100 ), "SetTransversePlaneMri: Best combo -> Q  [0..8..31..100..]" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Q != 0 )
    *Q  = bestQ;

                                        // Check if transform is really to be applied:
if ( IsInsideLimits ( bestQ, MinQ, MaxQ ) ) {
                                        // copy the best transform results
    TraAbs_to_MriRel    = bestTraAbs_to_MriRel;

    center              = bestcenter;
                                        // we good
    return  true;
    }

else {                                  // either close to no transform at all, or way too big

    TraAbs_to_MriRel    = originalTraAbs_to_MriRel;

    center              = originalcenter;
                                        // it might miss the translation part?
    if ( TraAbs_to_MriRel.HasNoTranslation ()  )
        TraAbs_to_MriRel.SetTranslation ( center );
                                        // report error only for high Q, otherwise report as OK
    return  bestQ <= MinQ;
    }
}


//----------------------------------------------------------------------------
                                        // Search for the neck cutting plane
double  SearchGuillotinePlane   (   const TVolumeDoc*   mrihead,
                                    TMatrix44&          mriabstoguillotine, // input & output 
                                    TPointDouble&       center,             // input & output
                                    GOMethod            gomethod,
                                    double              precision,
                                    const char*         title,
                                    bool                verbose
                                )
{
//TPointDouble        originalcenter    ( center    );

                                        // Use a low cut for background, as neck area usually has lower values
double              mribackground   = mrihead->GetBackgroundValue () * 0.5;


TVolumeProperties   gomri   (   mrihead, 
                                mrihead->GetStandardOrientation ( LocalToRAS ),
                                mribackground,
                                &mriabstoguillotine,    // input
                                &center                 // input
                            );
//TEasyStats          gomriq;


gomri.Set ( GOStepsFaster );

                                        // we need to know on which side of  UpDownIndex  to position the search
bool                upispositive    = mrihead->GetAxis ( ToUp ) == ZMaxSide 
                                   || mrihead->GetAxis ( ToUp ) == YMaxSide 
                                   || mrihead->GetAxis ( ToUp ) == XMaxSide;
                                        // it should be very close to the appropriate border
double              borderz         = upispositive ? gomri.Bound.Min ( gomri.UpDownIndex ) : gomri.Bound.Max ( gomri.UpDownIndex );

double              deltaz          = gomri.Bound.GetRadius ( gomri.UpDownIndex ) * 0.20;

double              rotx            = 30;
double              roty            = 10;


gomri.AddGroup ();
gomri.AddDim   ( TranslationZ, borderz - deltaz,
                               borderz + deltaz );

gomri.AddGroup ();
gomri.AddDim   ( RotationX, -rotx, rotx );
gomri.AddDim   ( RotationY, -roty, roty );

                                        // with this simple error measure, it seems it needs a more thorough exploration
gomri.GetSolution   (   gomethod, 
                        TransversePlaneGuillotine, 
                        precision,       0, 
                        title
                        //&gomriq
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compare the results with the original ranges
double              suspicioustrz       = RoundTo ( fabs ( gomri.GetValue ( TranslationZ ) - borderz ) / deltaz, 0.01 );
double              suspiciousrotx      = RoundTo ( fabs ( gomri.GetValue ( RotationX    )           ) / rotx,   0.01 );
double              suspiciousroty      = RoundTo ( fabs ( gomri.GetValue ( RotationY    )           ) / roty,   0.01 );

//bool                suspicious          = suspicioustrz  >= 1.00 
//                                       || suspiciousrotx >= 1.00
//                                       || suspiciousroty >= 1.00

                                        // parameters energy: more transform -> more energy
double              mintransf   = sqrt (  Square ( suspicioustrz ) 
                                        + Square ( suspiciousrotx ) + Square ( suspiciousroty )
                                       );

double              Q           = mintransf;


if ( verbose ) {

//    DBGV7 ( RoundTo ( gomri.GetValue ( TranslationZ ), 0.1 ) /*- centerz*/, RoundTo ( gomri.GetValue ( RotationX ), 0.1 ), RoundTo ( gomri.GetValue ( RotationY ), 0.1 ),
//            suspicioustrz * 100, suspiciousrotx * 100, suspiciousroty * 100,
//            Q,
//            GetGOMethodName ( gomethod ) );

//    gomriq.Show ( "Guillotine Plane Search Quality" );

    DBGV ( Round ( Q * 100 ), "Q" );

                                            // not looking good?
//  if ( suspicious )
//      ShowMessage ( "Guillotine Plane search encountered some problems!", GetGOMethodName ( gomethod ), ShowMessageWarning );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cooking the results
TPointDouble        origin          = mrihead->GetOrigin ();

                                        // compute the transformation matrix
TMatrix44           guillotineabstomriabs ( gomri.NormToMRI );  // Guillotine Abs -> MRI Rel

                                        // !converting to absolute original MRI space!
guillotineabstomriabs.Translate      ( -origin, MultiplyLeft );

                                        // finally returning the inverse transform MRI Abs to Guillotine Abs
mriabstoguillotine  = TMatrix44 ( guillotineabstomriabs ).Invert ();

                                        // !not tested, not used!
center              = gomri.Center - origin;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  Q;
}


//----------------------------------------------------------------------------
bool    SetGuillotinePlane  (   const TVolumeDoc*   mrihead, 
                                TMatrix44&          mriabstoguillotine, // output
                                double*             Q,
                                const char*         title
                            )
{
mriabstoguillotine.SetIdentity ();

TPointDouble        center          = mrihead->GetOrigin ();
TMatrix44           originalmriabstoguillotine  ( mriabstoguillotine );
TPointDouble        originalcenter              ( center    );
TMatrix44           bestmriabstoguillotine;
TPointDouble        bestcenter;
double              bestQ               = DBL_MAX;
int                 bestcombo           = -1;
GOMethod            gomethod;
char                localtitle[ 256 ]   = "";


constexpr double    MinQ                = 0.00;     // no lower bound
constexpr double    ReliableQ           = 0.05;     // very good fit
constexpr double    MaxQ                = 1.50;     // quite liberal - highest good being about 0.75

                                        // Repeat search with different parameters
for ( int combo = 0; combo < 2; combo++ ) {
                                        // !set before each run!
    mriabstoguillotine  = originalmriabstoguillotine;

    center              = originalcenter;


    if      ( combo == 0 )  gomethod    = GlobalNelderMead;
    else                    gomethod    = WeakestDimensionCrossHairScan;

    if ( StringIsNotEmpty ( title ) )
        StringCopy      ( localtitle, title, " v", IntegerToString ( combo + 1 ) );


    double          Q           = SearchGuillotinePlane (   mrihead,
                                                            mriabstoguillotine,     // input & output
                                                            center,                 // input & output
                                                            gomethod,
                                                            GODefaultConvergence,
                                                            localtitle,
                                                            false
                                                        );

//  DBGV2 ( combo + 1, Round ( Q * 100 ), "SetGuillotinePlane: combo -> Q" );

                                        // is it the best estimate so far?
    if ( Q < bestQ ) {
        bestcombo               = combo;
        bestQ                   = Q;
        bestmriabstoguillotine  = mriabstoguillotine;
        bestcenter              = center;
        }

                                        // stop as soon as we have a very good enough fit
    if ( bestQ <= ReliableQ )
        break;

    } // for combo


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Q != 0 )
    *Q  = bestQ;


if ( IsInsideLimits ( bestQ, MinQ, MaxQ ) ) {
                                        // copy the best transform results
    mriabstoguillotine  = bestmriabstoguillotine;

    center              = bestcenter;
                                        // we good
    return  true;
    }

else {

    mriabstoguillotine  = TMatrix44 ( originalmriabstoguillotine ).Invert ();

    center              = originalcenter;
                                        // it might miss the translation part?
    if ( mriabstoguillotine.HasNoTranslation ()  )
        mriabstoguillotine.SetTranslation ( center );

    return  false;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
