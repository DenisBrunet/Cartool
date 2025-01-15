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

#include    <owl/pch.h>

#include    "TSolutionPointsDoc.h"

#include    "MemUtil.h"
#include    "Math.Stats.h"
#include    "Math.Random.h"
#include    "Math.Resampling.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TFilters.h"
#include    "TArray3.h"
#include    "Files.Stream.h"

#include    "TVolumeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  SPInterpolationTypeNames[ NumSPInterpolationTypes ][ 64 ] =
            {
            "No Solution Points Interpolation",
            "1 Nearest Neighbor",
            "4 Nearest Neighbors",
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TSolutionPointsDoc::TSolutionPointsDoc ( TDocument *parent ) :
        TBaseDoc ( parent )
{
SetAtomType ( AtomTypeCoordinates );

ContentType         = ContentTypeSp;
ExtraContentType    = PointsGeometryUnknown;
//CopyVirtualMemory ( ExtraContentTypeNames, PointsGeometryNames, NumPointsGeometry * ContentTypeMaxChars );
for ( int i = 0; i < NumPointsGeometry; i++ )
    StringCopy ( ExtraContentTypeNames[ i ], PointsGeometryNames[ i ], ContentTypeMaxChars - 1 );

HasNames            = false;

VoxelSize           = 1.0;
//RealSize            = 0.0;
OriginShift         = 0.0;

DisplaySpaces.Set ( SolutionPointsNumSpaces );
}


bool	TSolutionPointsDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return  true;


if ( IsExtension ( FILEEXT_LOC ) ) {

    if ( ! GetAnswerFromUser ( "Can not save this type of file!\nShould I rather use file type  '" FILEEXT_SPIRR "'  instead?", GetTitle () ) )
        return  false;

    TFileName           filename;

    StringCopy       ( filename, GetDocPath () );
    ReplaceExtension ( filename, FILEEXT_SPIRR );
    SetDocPath       ( filename );
    }
else if ( ! IsExtension ( GetDefaultExtension () ) ) {

    if ( ! GetAnswerFromUser ( "Do you really want to save this document to another format?\n(some informations might be either lost or missing in the new format)\n\nIf you proceed with this operation, it is advised to close and reopen the new file.", "Save As" ) )
        return  true;
    }

                                        // we nicely follow the input naming
GetPoints ( 0 ).WriteFile ( (char *) GetDocPath (), HasNames ? &SPNames : 0 );


SetDirty ( false );
return true;
}


bool	TSolutionPointsDoc::Revert ( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;

if ( ! clear )
    Open ( 0, 0 );

return true;
}


//----------------------------------------------------------------------------
bool	TSolutionPointsDoc::InitDoc ()
{

if ( ! IsOpen () ) {

    if ( ! Open ( ofRead, 0 ) )
        return  true; // false;         // should be false, but here to avoid the annoying message


    SetBounding         ();

    SetMedianDistance   ();

    SetOriginShift      ();

    FindOrientation     ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting Geometry transform

                                        // is it a known file?
    TFileName           filename;
    StringCopy    ( filename,  GetTitle () );
    GetFilename   ( filename );

//  TFileName           extension;
//  StringCopy    ( extension, GetTitle () );
//  KeepExtension ( extension );


//  static TStringGrep      greptalvmr      ( "(?i)(_TAL|_TAL_MASKED)(\\..+|)$" );

                                        // we know how to "hard-wire" these files to Talairach
//  if      ( StringIsNot ( extension, FILEEXT_LOC ) ) {

                                        // Many of these spaces are simply obsolete - might be a good time to retire them
        if      ( StringIs       ( (const char*) filename, "avg152t1v2.segrho"            ) )   GeometryTransform = new TGeometryTransform ( TalairachAvg152T1Fld   );
        else if ( StringIs       ( (const char*) filename, "avg152T1_smac_pro_grid.3005p" ) )   GeometryTransform = new TGeometryTransform ( TalairachAvg152T1_smac );
        else if ( StringIs       ( (const char*) filename, "ave152_sym_pro_grid.2738p"    ) )   GeometryTransform = new TGeometryTransform ( TalairachAve152_sym    );
        else if ( StringContains ( (const char*) filename, "MNI-6239-voxels"              ) )   GeometryTransform = new TGeometryTransform ( TalairachMNI152_2001   );

        else if ( (    StringStartsWith ( (const char*) filename, "MNI152.Nlin"           ) 
                    || StringStartsWith ( (const char*) filename, "NIHPD324.NlinAsym"     ) 
                    || StringStartsWith ( (const char*) filename, "mni_icbm152_"          ) 
                    || StringStartsWith ( (const char*) filename, "MNI152_"               ) 
                  )
               && ! StringContains ( (const char*) filename, "ToTalairach"                ) )   GeometryTransform = new TGeometryTransform ( TalairachMNI152_2009   );
                                            // wide guess, but prevents what seems to be converted files
        else if ( StringStartsWith ( (const char*) filename, "MNI152"                     ) 
               && ! StringContains ( (const char*) filename, "ToTalairach"                ) )   GeometryTransform = new TGeometryTransform ( TalairachMNI152_2001   );

        else if ( StringStartsWith ( (const char*) filename, "NIHPD.Asym.Infant"          ) 
               && ! StringContains ( (const char*) filename, "ToTalairach"                ) )   GeometryTransform = new TGeometryTransform ( TalairachNIHPD_Infant  );

//      } // non .loc
//
//    else if ( StringIs ( extension, FILEEXT_LOC ) ) {
//
//      if ( greptalvmr.Matched ( filename ) )                                                  GeometryTransform = new TGeometryTransform ( Talairach              );
//       } // .loc

    } // ! IsOpen


return  true;
}


//----------------------------------------------------------------------------
void	TSolutionPointsDoc::Sort ( TPointFloatI* ListSPI, int l, int r )
{
int                 i               = l;
int                 j               = r;
double              x,  y,  z;
TPointFloatI        p;

x   = ListSPI[( l + r ) / 2].X;
y   = ListSPI[( l + r ) / 2].Y;
z   = ListSPI[( l + r ) / 2].Z;

do {
    while (  ListSPI[i].Z <  z ||
           ( ListSPI[i].Z == z && ListSPI[i].X <  x ) ||
           ( ListSPI[i].Z == z && ListSPI[i].X == x && ListSPI[i].Y < y ) ) i++;

    while (  z <  ListSPI[j].Z ||
           ( z == ListSPI[j].Z && x <  ListSPI[j].X ) ||
           ( z == ListSPI[j].Z && x == ListSPI[j].X && y < ListSPI[j].Y ) ) j--;

    if ( i <= j ) {
        p             = ListSPI[i];
        ListSPI[i++]  = ListSPI[j];
        ListSPI[j--]  = p;
        }
    } while ( i <= j );


if ( l < j )
    Sort ( ListSPI, l, j );

if ( i < r )
    Sort ( ListSPI, i, r );
}


//----------------------------------------------------------------------------
void    TSolutionPointsDoc::SetBounding ()
{

float               xmin            = Highest ( xmin );
float               ymin            = Highest ( ymin );
float               zmin            = Highest ( zmin );
float               xmax            = Lowest  ( xmax );
float               ymax            = Lowest  ( ymax );
float               zmax            = Lowest  ( zmax );
const TPoints&      Points          = GetPoints ( DisplaySpace3D );


for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {

    Mined ( xmin,   Points[ sp ].X );
    Maxed ( xmax,   Points[ sp ].X );
    Mined ( ymin,   Points[ sp ].Y );
    Maxed ( ymax,   Points[ sp ].Y );
    Mined ( zmin,   Points[ sp ].Z );
    Maxed ( zmax,   Points[ sp ].Z );
    }


if ( GetNumSolPoints () <= 1 )

    GetBounding ( DisplaySpace3D )->Set ( xmin - 10, xmax + 10, ymin - 10, ymax + 10, zmin - 10, zmax + 10 );

else {
    GetBounding ( DisplaySpace3D )->Set ( xmin, xmax, ymin, ymax, zmin, zmax );

                                        // widen the bounding, by security - some set of SPs can be far from the MRI mask borders
    int     mx  = GetBounding ( DisplaySpace3D )->GetXExtent () * 0.1;
    int     my  = GetBounding ( DisplaySpace3D )->GetYExtent () * 0.1;
    int     mz  = GetBounding ( DisplaySpace3D )->GetZExtent () * 0.1;

    GetBounding ( DisplaySpace3D )->Set ( xmin - mx, xmax + mx, ymin - my, ymax + my, zmin - mz, zmax + mz );
    }


//GetBounding ( DisplaySpace3D )->Set ( GetNumSolPoints (), Points );
}


//----------------------------------------------------------------------------
                                        // First part is equivalent to TArrayPoint::GetMedianDistance
                                        // but we could use more info than in this function, hence the duplicate here
void    TSolutionPointsDoc::SetMedianDistance ()
{
const TPoints&      Points          = GetPoints ( DisplaySpace3D );
TPointFloat         mind;
TDownsampling       downsp ( GetNumSolPoints () - 1, 250 );
int                 step2           = GetNumSolPoints () < 250 ? 1 : 3;
double              norm2;
double              minnorm2;
TEasyStats          statd ( 3 * downsp.NumDownData );


                                        // first loop is a subsample of all sp's
for ( int sp1 = downsp.From; sp1 <= downsp.To; sp1 += downsp.Step ) {

    minnorm2    = Highest ( minnorm2 );

                                        // second loop does not need to see all points either, but going until the end is the point
    for ( int sp2 = sp1 + 1; sp2 < GetNumSolPoints (); sp2 += step2 ) {

        norm2       = ( Points[ sp1 ] - Points[ sp2 ] ).Norm2 ();

        Mined ( minnorm2 , norm2 );
        }

    statd.Add ( sqrt ( minnorm2 ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute MedDistance constant, used as a scale for all sorts of display things
                                        // median looks very robust
double              MedDistance     = statd.Median ( false );

                                        // add some safety checks
if ( GetNumSolPoints () <= 1 || MedDistance <= 0 )
    MedDistance     = 1;


DisplaySpaces[ DisplaySpace3D ].MedianDistance  = MedDistance;
DisplaySpaces[ DisplaySpace3D ].PointRadius     = MedDistance / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Guess content type - regular grid, aligned or tilted

                                        // also: MCoV is 0 for regular & aligned, < 0.028 for regular & tilted, > 0.07 for irregular
ExtraContentType    = statd.RobustCoV () < 0.05 ? PointsGeometryGridAligned : PointsGeometryIrregular;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Guess content type - regular grid, aligned only
if ( IsGeometryGrid ( ExtraContentType ) ) {
                                        // we restrict ourselves to strictly aligned grids for the moment
    statd.Reset  ();

                                        // first loop is a subsample of all sp's
    for ( int sp1 = downsp.From; sp1 <= downsp.To; sp1 += downsp.Step ) {

        if ( sp1 + 1 > GetNumSolPoints () - 1 )
            break;

                                        // compute vector between current point and the next one
        mind        = Points[ sp1 ] - Points[ sp1 + 1 ];
                                        // snap it to median distance
        mind       /= NonNull ( MedDistance );
                                        // then study how it fractions: regular should give a lot of 0's, irregular few
        statd.Add ( fabs ( Fraction ( mind.X ) ) );
        statd.Add ( fabs ( Fraction ( mind.Y ) ) );
        statd.Add ( fabs ( Fraction ( mind.Z ) ) );
        }

                                        // also: Median, MCoV
    ExtraContentType    = statd.InterQuartileRange () < 1e-4 ? PointsGeometryGridAligned : PointsGeometryGridNotAligned;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute Voxel size, to poke the SPs into an optimal volume

mind.Set ( MedDistance, MedDistance, MedDistance );

                                        // increase the resolution
mind       /= SolutionPointsSubSampling;

//if ( ! IsGridAligned () )
//    mind   /= SPIrregularVolumeSubsampling;

                                        // one more test: don't allow less than 1 (would oversample the MRI!)
if ( mind.X < 1 )           mind.X = 1;
if ( mind.Y < 1 )           mind.Y = 1;
if ( mind.Z < 1 )           mind.Z = 1;

                                        // if irregular, set all directions equal
if ( ! IsGridAligned () )
    mind.X      = mind.Y    = mind.Z    = min ( mind.X, mind.Y, mind.Z );

                                        // get the subsampling in all axis
if ( GetNumSolPoints () <= 1 )
    VoxelSize.Set ( 1, 1, 1 );
else
    VoxelSize.Set ( mind );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get real size extent
/*
RealSize.X      = GetBounding ( DisplaySpace3D )->GetXExtent ();
RealSize.Y      = GetBounding ( DisplaySpace3D )->GetYExtent ();
RealSize.Z      = GetBounding ( DisplaySpace3D )->GetZExtent ();
*/
}


//----------------------------------------------------------------------------
void    TSolutionPointsDoc::SetOriginShift ()
{
OriginShift     = 0.0;

                                        // if points are not regular, there seems to be no gain to search for a sub-voxel shift
//if ( ! IsGridAligned () )
if ( ! IsGrid () )
    return;


const TPoints&      Points          = GetPoints ( DisplaySpace3D );
float               minx            = Highest ( minx );
float               miny            = Highest ( miny );
float               minz            = Highest ( minz );

for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {
    Mined ( minx,   Points[ sp ].X );
    Mined ( miny,   Points[ sp ].Y );
    Mined ( minz,   Points[ sp ].Z );
    }

OriginShift.X   = minx < 0 ? 1 - Fraction ( fabs ( minx ) ) : Fraction ( minx );
OriginShift.Y   = miny < 0 ? 1 - Fraction ( fabs ( miny ) ) : Fraction ( miny );
OriginShift.Z   = minz < 0 ? 1 - Fraction ( fabs ( minz ) ) : Fraction ( minz );

//OriginShift.Show ( "OriginShift" );
}


//----------------------------------------------------------------------------
                                        // VoxelSize is fixed here, and will constrain the rendering volume - it could be a parameter, though
TPointInt   TSolutionPointsDoc::GetVolumeSize ()
{
TPointInt           size;

size.X  = Round ( GetBounding ( DisplaySpace3D )->GetXExtent () / VoxelSize.X ) + 2 * TesselationVolumeMargin + 1;
size.Y  = Round ( GetBounding ( DisplaySpace3D )->GetYExtent () / VoxelSize.Y ) + 2 * TesselationVolumeMargin + 1;
size.Z  = Round ( GetBounding ( DisplaySpace3D )->GetZExtent () / VoxelSize.Z ) + 2 * TesselationVolumeMargin + 1;

return  size;
}


//----------------------------------------------------------------------------
void    TSolutionPointsDoc::FindOrientation ()
{
const TPoints&      Points          = GetPoints ( DisplaySpace3D );
TOrientationType    XAxis           = OrientationUnknown;
TOrientationType    YAxis           = OrientationUnknown;
TOrientationType    ZAxis           = OrientationUnknown;
int                 step            = max ( 1, Round ( GetNumSolPoints () / 500 ) );


if ( GetNumSolPoints () <= 5 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Try with SP names
char                buff[ 256 ];
TPointFloat         left;
TPointFloat         right;
TPointFloat         post;
TPointFloat         ant;
TPointFloat         inf;
TPointFloat         sup;


for ( int sp = 0; sp < GetNumSolPoints (); sp += step ) {

    StringCopy      ( buff, SPNames[ sp ] );
    KeepChars       ( buff, "LRPAIS" );
    StringNoSpace   ( buff );

    if ( StringLength ( buff ) != 3 )
        continue;

    if      ( StringContains ( buff, 'L' ) )    left   += Points[ sp ];
    else if ( StringContains ( buff, 'R' ) )    right  += Points[ sp ];

    if      ( StringContains ( buff, 'P' ) )    post   += Points[ sp ];
    else if ( StringContains ( buff, 'A' ) )    ant    += Points[ sp ];

    if      ( StringContains ( buff, 'I' ) )    inf    += Points[ sp ];
    else if ( StringContains ( buff, 'S' ) )    sup    += Points[ sp ];
    }


if ( left != 0 && right != 0 ) {
    right  -= left;
    ant    -= post;
    sup    -= inf;

    right.Normalize ();
    right.Round     ();
    ant  .Normalize ();
    ant  .Round     ();
    sup  .Normalize ();
    sup  .Round     ();

    if      ( right.X > 0 )  XAxis   = ToRight;
    else if ( right.X < 0 )  XAxis   = ToLeft;
    else if ( right.Y > 0 )  YAxis   = ToRight;
    else if ( right.Y < 0 )  YAxis   = ToLeft;
    else if ( right.Z > 0 )  ZAxis   = ToRight;
    else if ( right.Z < 0 )  ZAxis   = ToLeft;

    if      ( ant  .X > 0 )  XAxis   = ToFront;
    else if ( ant  .X < 0 )  XAxis   = ToBack;
    else if ( ant  .Y > 0 )  YAxis   = ToFront;
    else if ( ant  .Y < 0 )  YAxis   = ToBack;
    else if ( ant  .Z > 0 )  ZAxis   = ToFront;
    else if ( ant  .Z < 0 )  ZAxis   = ToBack;

    if      ( sup  .X > 0 )  XAxis   = ToUp;
    else if ( sup  .X < 0 )  XAxis   = ToDown;
    else if ( sup  .Y > 0 )  YAxis   = ToUp;
    else if ( sup  .Y < 0 )  YAxis   = ToDown;
    else if ( sup  .Z > 0 )  ZAxis   = ToUp;
    else if ( sup  .Z < 0 )  ZAxis   = ToDown;


//    DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry Names" );

                                        // we made it?
    if ( XAxis != OrientationUnknown && YAxis != OrientationUnknown && ZAxis != OrientationUnknown ) {
        SetOrientation ( XAxis, YAxis, ZAxis );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we will analyse the geometry of electrodes to guess the orientation

TBoundingBox<double>*   bounding        = GetBounding ();
TPointFloat             center;
TPointFloat             middle;

//bounding->GetCenter ( center );
bounding->GetMiddle ( middle );


                                        // compute the real center of our points
center      = (double) 0;

for ( int sp = 0; sp < GetNumSolPoints (); sp++ )
    center     += Points[ sp ];

center     /= NonNull ( GetNumSolPoints () );

//center.Show ( "new center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Find Left-Right direction
                                        // This is the strongest measure, so we begin with it!

                                        // compute how symetric are the electrodes according to 3 planes passing trough center
                                        // works pretty well even with slants, though there is a limit to the angles vs the symetry of electrodes
TPointFloat         pf;
TPointFloat         p1;
TPointFloat         p2;
TPointFloat         p3;
TEasyStats          stat1;
TEasyStats          stat2;
TEasyStats          stat3;

stat1.Reset ();     stat2.Reset ();     stat3.Reset ();


for ( int sp = 0; sp < GetNumSolPoints (); sp += step ) {

    p1      = Points[ sp ];

    p2      = p1 - center;

    pf[ 0 ] = 2 * center[ 0 ] - p1[ 0 ];
    pf[ 1 ] =                   p1[ 1 ];
    pf[ 2 ] =                   p1[ 2 ];

    p2      = Points[ GetNearestElementIndex ( pf ) ];

//    stat1.Add ( p2[ 0 ] - pf[ 0 ] );
    stat1.Add ( fabs ( p2[ 0 ] - pf[ 0 ] ) );
//    stat1.Add ( ( p2 - pf ).Norm2 () );


    pf[ 0 ] =                   p1[ 0 ];
    pf[ 1 ] = 2 * center[ 1 ] - p1[ 1 ];
    pf[ 2 ] =                   p1[ 2 ];

    p2      = Points[ GetNearestElementIndex ( pf ) ];

//    stat2.Add ( p2[ 0 ] - pf[ 0 ] );
    stat2.Add ( fabs ( p2[ 0 ] - pf[ 0 ] ) );
//    stat2.Add ( ( p2 - pf ).Norm2 () );


    pf[ 0 ] =                   p1[ 0 ];
    pf[ 1 ] =                   p1[ 1 ];
    pf[ 2 ] = 2 * center[ 2 ] - p1[ 2 ];

    p2      = Points[ GetNearestElementIndex ( pf ) ];

//    stat3.Add ( p2[ 0 ] - pf[ 0 ] );
    stat3.Add ( fabs ( p2[ 0 ] - pf[ 0 ] ) );
//    stat3.Add ( ( p2 - pf ).Norm2 () );
    }

//DBGV3 ( stat1.Sum2 (), stat2.Sum2 (), stat3.Sum2 (), "SPs side symetry (sum)" );
//DBGV3 ( stat1.SD (), stat2.SD (), stat3.SD (), "SPs side symetry (SD)" );


                                    // choose the min (highest symetry) as LeftRight
if ( stat1.Sum2 () < stat2.Sum2 () && stat1.Sum2 () < stat3.Sum2 () )   XAxis   = LeftRight;
if ( stat2.Sum2 () < stat1.Sum2 () && stat2.Sum2 () < stat3.Sum2 () )   YAxis   = LeftRight;
if ( stat3.Sum2 () < stat2.Sum2 () && stat3.Sum2 () < stat1.Sum2 () )   ZAxis   = LeftRight;

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 1)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Find Front-Back axis
                                        // Compute a kind of numerical approximation of the highest eigenvector,
                                        // except we use vectors between random set of points (intrinsic relations between electrodes),
                                        // giving a global direction, which we average the ratio of the 2 unknown dimensions.
                                        // The final average ratio is very stable, even in very ambiguous coordinates.
TPointFloat         globaldir;

                                        // very stable at 100, less can be a problem
//#define             MaxRandomRepeat     100
#define             MaxRandomRepeat     50
#define             MaxRandomPairs      1000
TRandUniform        randunif;
double              avgratio        = 0;

                                        // repeat a random process
for ( int randi = 0; randi < MaxRandomRepeat; randi++ ) {

    globaldir.Reset ();

                                        // sum vectors taken from random pairs
    for ( int i = 0; i < MaxRandomPairs; i++ ) {

        int         e       = randunif ( (uint) GetNumSolPoints () );
        int         e2;

        do  e2 = randunif ( (uint) GetNumSolPoints () );    while ( e2 == e );


        p1      = Points [ e  ];
        p2      = Points [ e2 ];

                                        // kind of simple spherization (for non-spherical cases)
        p1     -= middle;
        p1.Normalize ();
        p2     -= middle;
        p2.Normalize ();
                                        // get vector between the 2 points
        p2     -= p1;

        p2.Normalize ();                // helps a bit badly aligned real coordinates

                                        // cumulate vectors, in the compatible direction
        if ( p2.ScalarProduct ( globaldir ) < 0 )
            p2.Invert ();

        globaldir   += p2;
        }

                                        // this is an approximate global direction
    globaldir.Normalize ();

                                        // average the ratios of axis
    if      ( XAxis )   avgratio   += fabs ( globaldir.Y ) / ( globaldir.Z ? fabs ( globaldir.Z ) : 1e-6 );
    else if ( YAxis )   avgratio   += fabs ( globaldir.X ) / ( globaldir.Z ? fabs ( globaldir.Z ) : 1e-6 );
    else if ( ZAxis )   avgratio   += fabs ( globaldir.X ) / ( globaldir.Y ? fabs ( globaldir.Y ) : 1e-6 );

//    DBGV2 ( randi + 1, avgratio / ( randi + 1 ), "AvgRatio" );
    } // for randi


avgratio   /= MaxRandomRepeat;

//DBGV ( avgratio, "Axis ratio" );

                                        // choose the one direction according to averaged ratio
if      ( XAxis )
    if ( avgratio > 1 )     YAxis   = FrontBack;
    else                    ZAxis   = FrontBack;
else if ( YAxis )
    if ( avgratio > 1 )     XAxis   = FrontBack;
    else                    ZAxis   = FrontBack;
else if ( ZAxis )
    if ( avgratio > 1 )     XAxis   = FrontBack;
    else                    YAxis   = FrontBack;


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 2)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // Try to cut short, by assuming the most common orientations
if ( XAxis == LeftRight && YAxis == FrontBack ) {
    SetOrientation ( "RAS" );
//    DBGM ( "RAS", "Geometry Guess" );
    return;
    }

else if ( ZAxis == LeftRight && XAxis == FrontBack ) {
    SetOrientation ( "PIR" );
//    DBGM ( "PIR", "Geometry Guess" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here: to be completed...
/*
                                        // 3) Find Up-Down Direction and Axis
                                        // Compute the sum of vectors from center to all points, which will point to Up
TPointFloat         updown;


                                        // scan all points
for ( int sp = 0; sp < GetNumSolPoints (); sp += step ) {

    p1      = Points [ sp ] - center;

    p1.Normalize ();
                                        // cumulate sums
    updown += p1;
    } // for e


updown.Normalize ();

updown.Show ( "UpDown" );

                                        // we already have two axis set!
                                        // we are also lucky to have the absolute direction
if      ( ! XAxis )     XAxis   = updown.X > 0 ? ToUp : ToDown;
else if ( ! YAxis )     YAxis   = updown.Y > 0 ? ToUp : ToDown;
else if ( ! ZAxis )     ZAxis   = updown.Z > 0 ? ToUp : ToDown;

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 3)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Find Front versus Back
                                        // count # of electrodes on each side, usually they are more on the back side
#define             fbdepth         0.30
#define             udheight        0.20
double              balance         = 0;


for ( int sp = 0; sp < GetNumSolPoints (); sp += step ) {

    p1      = Points [ sp ] - center;

    if ( p1 == 0 )     continue;

    p1     -= middle;
    p1     /= bounding->Radius ();

                                        // count lower, forward / backward electrodes
    if      ( XAxis == FrontBack && p1[ 0 ] < -fbdepth ) {
        if ( YAxis == ToUp && p1[ 1 ] < -udheight || YAxis == ToDown && p1[ 1 ] > udheight
          || ZAxis == ToUp && p1[ 2 ] < -udheight || ZAxis == ToDown && p1[ 2 ] > udheight )
            balance++;
        }
    else if ( YAxis == FrontBack && p1[ 1 ] < -fbdepth ) {
        if ( ZAxis == ToUp && p1[ 2 ] < -udheight || ZAxis == ToDown && p1[ 2 ] > udheight
          || XAxis == ToUp && p1[ 0 ] < -udheight || XAxis == ToDown && p1[ 0 ] > udheight )
            balance++;
        }
    else if ( ZAxis == FrontBack && p1[ 2 ] < -fbdepth ) {
        if ( YAxis == ToUp && p1[ 1 ] < -udheight || YAxis == ToDown && p1[ 1 ] > udheight
          || XAxis == ToUp && p1[ 0 ] < -udheight || XAxis == ToDown && p1[ 0 ] > udheight )
            balance++;
        }

    if      ( XAxis == FrontBack && p1[ 0 ] >  fbdepth ) {
        if ( YAxis == ToUp && p1[ 1 ] < -udheight || YAxis == ToDown && p1[ 1 ] > udheight
          || ZAxis == ToUp && p1[ 2 ] < -udheight || ZAxis == ToDown && p1[ 2 ] > udheight )
            balance--;
        }
    else if ( YAxis == FrontBack && p1[ 1 ] >  fbdepth ) {
        if ( ZAxis == ToUp && p1[ 2 ] < -udheight || ZAxis == ToDown && p1[ 2 ] > udheight
          || XAxis == ToUp && p1[ 0 ] < -udheight || XAxis == ToDown && p1[ 0 ] > udheight )
            balance--;
        }
    else if ( ZAxis == FrontBack && p1[ 2 ] >  fbdepth ) {
        if ( YAxis == ToUp && p1[ 1 ] < -udheight || YAxis == ToDown && p1[ 1 ] > udheight
          || XAxis == ToUp && p1[ 0 ] < -udheight || XAxis == ToDown && p1[ 0 ] > udheight )
            balance--;
        }

    }

//DBGV ( balance, "below-above balance" );

if ( XAxis == FrontBack )  XAxis = balance < 0 ? ToBack : ToFront;
if ( YAxis == FrontBack )  YAxis = balance < 0 ? ToBack : ToFront;
if ( ZAxis == FrontBack )  ZAxis = balance < 0 ? ToBack : ToFront;


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 2)" );

*/
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Finish the job with a vectorial product
CompleteOrientation ( XAxis, YAxis, ZAxis );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 5)" );

SetOrientation ( XAxis, YAxis, ZAxis );
}


//----------------------------------------------------------------------------
                                        // prints the sp positions for a human reader
void	TSolutionPointsDoc::ExportText ()  const
{
const TPoints&      Points          = GetPoints ( DisplaySpace3D );
ofstream            ooo;
TFileName           filename;
bool                found;
const TBoundingBox<double>* Bounding= GetBounding ( DisplaySpace3D );
double              step            = DisplaySpaces[ DisplaySpace3D ].MedianDistance;
int                 sepw            = 5 * ( Bounding->GetZExtent () / step + 2 );
char                sepline [ 1024 ];
double              mind            = step * SolutionPointsMinDistanceRatio;
TSelection          spdone ( GetNumSolPoints (), OrderSorted );
GetFileFromUser     getfile ( "Text File", AllTextFilesFilter, 1, GetFileWrite );


spdone.Reset ();


SetString   ( sepline, '-', sepw );


StringCopy ( filename, GetTitle () );
ReplaceExtension ( filename, "txt" );

if ( ! getfile.Execute ( filename ) )
    return;


TSuperGauge         Gauge ( "SPs to Text", Bounding->GetYExtent () / /*step*/ mind );


ooo.open ( getfile );

ooo << StreamFormatFixed;
                                        // 4 loops nested: it's a bit inefficient (but who cares here?)
                                        // BUT it works whatever the points'ordering is
for ( int y = Bounding->YMin (); y < Bounding->YMax (); y += /*step*/ mind ) {

    Gauge.Next ();


    ooo << "Y=   " << y << "\n";

    ooo << "Z=   ";

    for ( int z = Bounding->ZMin (); z < Bounding->ZMax (); z += /*step*/ mind ) {
        ooo << setw ( 5 ) << z;
        }
    ooo << "\n";


    ooo << "X" << "\n";
    for ( int x = Bounding->XMin (); x < Bounding->XMax (); x += /*step*/ mind ) {

        ooo << setw ( 5 ) << x;

        for ( int z = Bounding->ZMin (); z < Bounding->ZMax (); z += /*step*/ mind ) {

            found = false;

            for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {
                                        // this test is faster...
                if ( fabs ( Points[ sp ].Y - y ) > mind )
                    continue;
                                        // than this one
                if ( spdone[ sp ] )
                    continue;

                if ( fabs ( Points[ sp ].X - x ) <= mind
                  && fabs ( Points[ sp ].Z - z ) <= mind ) {

                    ooo << setw ( 5 ) << ( sp + 1 );
                    found   = true;
                    spdone.Set ( sp );
                    break;
                    }

                } // for sp

            if ( ! found )
                ooo << setw ( 5 ) << "";

            } // for z

        ooo << "\n" << "\n";
        } // for x

    ooo << sepline << "\n";
    } // for y


Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // scan all points for the closest match
int     TSolutionPointsDoc::GetNearestElementIndex ( const TPointFloat& p, double dmax )     const
{
const TPoints&      Points          = GetPoints ( DisplaySpace3D );
TPointFloat         delta;
int                 index           = TIndexNull;
double              d;
double              dmin            = Highest ( dmin );


for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {

    delta   = Points[ sp ] - p;

    d       = delta.Norm2 ();

    if ( d < dmin ) {
        dmin    = d;
        index   = sp;
        }
    }


return dmax > 0 ? dmin < Square ( dmax ) ? index : TIndexNull
                : index;
}


//----------------------------------------------------------------------------
                                        // scan all points for the closest match
int     TSolutionPointsDoc::GetIndex ( char* spname )  const
{
for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {

    if ( StringIs ( SPNames[ sp ], spname ) )
        return  sp;
    }

return  TIndexNull;
}


//----------------------------------------------------------------------------
                                        // Returns an array containing the indexes of all neighbors
                                        // Caller has to specify the type of neighborhood
void    TSolutionPointsDoc::GetNeighborsIndexes    (   TArray2<int>&   neighbi,    NeighborhoodType    neightype   )   const
{
const TPoints&      Points          = GetPoints ( DisplaySpace3D );

Points.GetNeighborsIndexes ( neighbi, neightype, GetMedianDistance ( DisplaySpace3D ) );
}


//----------------------------------------------------------------------------
void    TSolutionPointsDoc::GetNeighborhoodDistances ( TArray3<double>& neighborhood, int maxnumneigh )    const
{
int                 numsolp         = GetNumSolPoints ();
                                  // self + neighbors count
neighborhood.Resize ( numsolp, maxnumneigh + 1, NumNeighborhoodInfo );

neighborhood.ResetMemory ();


double              step            = GetMedianDistance ( DisplaySpace3D );

if ( step <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TPoints&      points          = GetPoints ( DisplaySpace3D );
                                        // neighborhood info for a single electrode
TArray2<double>     neigh ( numsolp, NumNeighborhoodInfo );


for ( int spi1 = 0; spi1 < numsolp; spi1++ ) {

    for ( int spi2 = 0; spi2 < numsolp; spi2++ ) {
                                        // index
        neigh ( spi2, NeighborhoodIndex    )    = spi2;
                                        // Norm2 is enough for sorting
        neigh ( spi2, NeighborhoodDistance )    = spi1 == spi2 ? 0 : ( points[ spi1 ] - points[ spi2 ] ).Norm2 ();
        }

                                        // sort by distance2
    neigh.SortRows ( NeighborhoodDistance, Ascending );

                                        // transfer all distances and their respective index
    for ( int spi2 = 0; spi2 <= maxnumneigh; spi2++ ) {

        neighborhood ( spi1, spi2, NeighborhoodIndex    )   = neigh ( spi2, NeighborhoodIndex     );
        neighborhood ( spi1, spi2, NeighborhoodDistance )   = sqrt ( neigh ( spi2, NeighborhoodDistance ) ) / step; // normalized by the median distance
        }
    }

}


//----------------------------------------------------------------------------
void     TSolutionPointsDoc::VolumeToAbsolute ( TPointFloat &p )   const
{
p.X     = GetBounding ( DisplaySpace3D )->XMin () + ( p.X - TesselationVolumeMargin ) * VoxelSize.X + OriginShift.X;
p.Y     = GetBounding ( DisplaySpace3D )->YMin () + ( p.Y - TesselationVolumeMargin ) * VoxelSize.Y + OriginShift.Y;
p.Z     = GetBounding ( DisplaySpace3D )->ZMin () + ( p.Z - TesselationVolumeMargin ) * VoxelSize.Z + OriginShift.Z;
}


void     TSolutionPointsDoc::AbsoluteToVolume ( TPointFloat &p )   const

{
p.X     = ( p.X - GetBounding ( DisplaySpace3D )->XMin () - OriginShift.X ) / VoxelSize.X + TesselationVolumeMargin;
p.Y     = ( p.Y - GetBounding ( DisplaySpace3D )->YMin () - OriginShift.Y ) / VoxelSize.Y + TesselationVolumeMargin;
p.Z     = ( p.Z - GetBounding ( DisplaySpace3D )->ZMin () - OriginShift.Z ) / VoxelSize.Z + TesselationVolumeMargin;
}


double  TSolutionPointsDoc::AbsoluteToVolumeX ( double xa )    const
{
return  ( xa - GetBounding ( DisplaySpace3D )->XMin () - OriginShift.X ) / VoxelSize.X + TesselationVolumeMargin;
}


double  TSolutionPointsDoc::AbsoluteToVolumeY ( double ya )    const
{
return  ( ya - GetBounding ( DisplaySpace3D )->YMin () - OriginShift.Y ) / VoxelSize.Y + TesselationVolumeMargin;
}


double  TSolutionPointsDoc::AbsoluteToVolumeZ ( double za )    const
{
return  ( za - GetBounding ( DisplaySpace3D )->ZMin () - OriginShift.Z ) / VoxelSize.Z + TesselationVolumeMargin;
}


//----------------------------------------------------------------------------
bool    TSolutionPointsDoc::BuildInterpolation ( SPInterpolationType interpol, const TVolumeDoc* MRIGrey )
{
if ( ! (    interpol == SPInterpolationNone
         || interpol == SPInterpolation1NN
         || interpol == SPInterpolation4NN ) )
    return  false;
                                        // already been computed?
                                        // we could redo the interpolation, if the previous call was not a grey mask and the current one is...
if ( interpol == SPInterpolationNone
  || interpol == SPInterpolation1NN && HasInterpol1NN ()
  || interpol == SPInterpolation4NN && HasInterpol4NN () )
    return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Fill the grey mask a little bit

const Volume&       greyvol         = *MRIGrey->GetData ();
Volume              greyvolfilled ( greyvol );
int                 mrithreshold    = MRIGrey->GetCsfCut ();
                                        // slightly inflate the mask, so it doesn't freak out for border SPs
                                        // also accounts for the downsampling, and defragment the visual aspect, too
//FctParams           params;
//params ( FilterParamDiameter   )  = 2.83;                // enough to have all SPs shown (aka filled), but not too much to avoid NN leaking too far
//params ( FilterParamResultType )  = FilterResultSigned;
//greyvolfilled.Filter ( FilterTypeMax, params );

                                        // Dilate - maybe a bit fat?
//FctParams           params;
//params ( FilterParamDiameter )     = 1;
//greyvolfilled.Filter ( FilterTypeDilate, params );

                                        // Faster & simpler Max, but good enough here (only 6 or 18 neighbors, and not a real max as it just sets the middle voxel even if it is already set)
OmpParallelFor

for ( int x = 1; x < greyvolfilled.GetDim1 () - 1; x++ )
for ( int y = 1; y < greyvolfilled.GetDim2 () - 1; y++ )
for ( int z = 1; z < greyvolfilled.GetDim3 () - 1; z++ )

    if ( greyvol ( x - 1, y,     z     ) || greyvol ( x,     y - 1, z     ) || greyvol ( x,     y,     z - 1 )
      || greyvol ( x + 1, y,     z     ) || greyvol ( x,     y + 1, z     ) || greyvol ( x,     y,     z + 1 )
//    || greyvol ( x + 1, y + 1, z     ) || greyvol ( x + 1, y - 1, z     ) || greyvol ( x - 1, y + 1, z     ) || greyvol ( x - 1, y - 1, z     )
//    || greyvol ( x + 1, y,     z + 1 ) || greyvol ( x + 1, y,     z - 1 ) || greyvol ( x - 1, y,     z + 1 ) || greyvol ( x - 1, y,     z - 1 )
//    || greyvol ( x,     y + 1, z + 1 ) || greyvol ( x,     y + 1, z - 1 ) || greyvol ( x,     y - 1, z + 1 ) || greyvol ( x,     y - 1, z - 1 )
         )
                                        // just above current background threshold
        greyvolfilled ( x, y, z )   = (MriType) ( mrithreshold + 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test SPs outliers

const TPoints&      Points          = GetPoints ( DisplaySpace3D );
                    MRIBounding     = *MRIGrey->GetBounding ();
TPointFloat         MRIOrigin       =  MRIGrey->GetOrigin ();

                                        // the test can sometime be fooled, as the bounding box of the MRI is computed
                                        // after thresholding the background, which can also remove some points on
                                        // the border of the brain, thus leading to an abnormal error message.

                                        // look more precisely if sol points are not in the grey matter
int                 outmri          = 0;

OmpParallelForSum ( outmri )

for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {

//    if ( MRIBounding.Contains ( Points[ sp ].x + MRIOrigin.X, Points[ sp ].y + MRIOrigin.Y, Points[ sp ].z + MRIOrigin.Z ) ) {
//        if ( greyvolfilled ( Points[ sp ].x + MRIOrigin.X, Points[ sp ].y + MRIOrigin.Y, Points[ sp ].z + MRIOrigin.Z ) <= mrithreshold )
                                        // de-shift solution points
    int     x   = Points[ sp ].X + MRIOrigin.X;
    int     y   = Points[ sp ].Y + MRIOrigin.Y;
    int     z   = Points[ sp ].Z + MRIOrigin.Z;

    if ( MRIBounding.Contains ( x, y, z ) ) {
        if ( greyvolfilled ( x, y, z ) <= mrithreshold )
            outmri++;
        }
    else
        outmri++;
    }

                                           // boundings don't match or more than 50 points are out (5% of points are out)
if ( /*! MRIBounding.Contains ( *Bounding ) ||*/ outmri > 50 && (double) outmri / GetNumSolPoints () > 0.25 ) {

    TBoundingBox<double>*   Bounding        = GetBounding ( DisplaySpace3D );
    char                    buff[ 1024 ];

    sprintf ( buff, "The whole set of solution points does not fit inside MRI data:\n\nBounding Boxes:\nSP    from ( %d, %d, %d )  to  ( %d, %d, %d )\nMRI  from ( %d, %d, %d )  to  ( %d, %d, %d )\n\nSolutions points out:\n%0d (%0d%%) points are not inside MRI data\n\nInterpolation might be hazardous.\nContinue anyway?",
//  sprintf ( buff, "The whole set of solution points does not fit inside MRI data:\n\nBounding Boxes:\nSP    from ( %d, %d, %d )  to  ( %d, %d, %d )\nMRI  from ( %d, %d, %d )  to  ( %d, %d, %d )\n\nSolutions points out:\n%0d (%0d%%) points are not inside MRI data\n\nInterpolation can not be computed.",
    (int) Bounding->XMin(), (int) Bounding->YMin(), (int) Bounding->ZMin(), (int) Bounding->XMax(), (int) Bounding->YMax(), (int) Bounding->ZMax(),
    (int) ( MRIBounding.XMin() - MRIOrigin.X ), (int) ( MRIBounding.YMin() - MRIOrigin.Y ), (int) ( MRIBounding.ZMin() - MRIOrigin.Z ), (int) MRIBounding.XMax(), (int) MRIBounding.YMax(), (int) MRIBounding.ZMax(),
    outmri, ( outmri * 100 ) / GetNumSolPoints () );

//  ShowMessage ( buff, "Interpolation", ShowMessageWarning );
    if ( ! GetAnswerFromUser ( buff, "Interpolation" ) )
        return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create a temp SP list, with indexes

TPointFloatI*       ListSPI         = new TPointFloatI [ GetNumSolPoints () ];

for ( int sp = 0; sp < GetNumSolPoints (); sp++ ) {
    ListSPI[ sp ].X     = Points[ sp ].X + MRIOrigin.X;
    ListSPI[ sp ].Y     = Points[ sp ].Y + MRIOrigin.Y;
    ListSPI[ sp ].Z     = Points[ sp ].Z + MRIOrigin.Z;
    ListSPI[ sp ].Index = sp;
    }
                                        // sort in Z/X/Y increasing order
Sort ( ListSPI, 0, GetNumSolPoints () - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create the right interpolation (4NN or 1NN)
double              step            = DisplaySpaces[ DisplaySpace3D ].MedianDistance;


if      ( interpol == SPInterpolation1NN  )   ComputeInterpol1NN ( ListSPI, step, MRIGrey, greyvolfilled );
else if ( interpol == SPInterpolation4NN  )   ComputeInterpol4NN ( ListSPI, step, MRIGrey, greyvolfilled );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

delete[]    ListSPI;


return  interpol == SPInterpolation1NN && HasInterpol1NN ()
     || interpol == SPInterpolation4NN && HasInterpol4NN ();
}


//----------------------------------------------------------------------------
void	TSolutionPointsDoc::ComputeInterpol4NN ( const TPointFloatI* ListSPI, double step, const TVolumeDoc* MRIGrey, const Volume& greyvol )
{
                                        // We are going to build an index in the Z axis, for faster access to points

int                 mridim3         = MRIGrey->GetSize ()->GetZExtent (); // scan MRI fully, not only the clipping, in case SPs overflowing...
TArray2<int>        zindex ( 2, mridim3 );
//double            mindz           = 4 * step;
double              mindz           = 4 / sqrt ( 3.0 ) * step;
int                 zminmri         = MRIBounding.ZMin () - 1;  // round the double bounding
int                 zmaxmri         = MRIBounding.ZMax () + 1;

zindex.ResetMemory ();

                                        // scan Z
OmpParallelFor

for ( int iz = 0; iz < mridim3; iz++ ) {

                                        // set default meaningful limits
    zindex ( 0 , iz ) = 0;
    zindex ( 1 , iz ) = GetNumSolPoints () - 1;

    int                 spdl;
    const TPointFloatI* tosp;

    for ( spdl = 0, tosp = &ListSPI[ spdl ]; spdl < GetNumSolPoints (); spdl++, tosp++ )
           // at least 4 "slices" from top     or at least 4 "slices" below current z
        if ( ( zmaxmri - tosp->Z ) <= mindz || tosp->Z >= iz - mindz ) {
            zindex ( 0 , iz ) = spdl;
            break;
            }

    for ( ; spdl < GetNumSolPoints (); spdl++, tosp++ )
           // at least 4 "slices" from bottom  and at least 4 "slices" above current z
        if ( ( tosp->Z - zminmri ) >= mindz && tosp->Z > iz + mindz ) {
            zindex ( 1 , iz ) = spdl;
            break;
            }
    }


                                        // to scan all points, without the indexing scheme (slow but safe)
//for ( int iz = 0; iz < mridim3; iz++ ) {
//    zindex ( 0 , iz ) = 0;
//    zindex ( 1 , iz ) = GetNumSolPoints () - 1;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TSuperGauge         Gauge ( "SP 4NN Interpolation", voldim3 );


TPointInt           volsize         = GetVolumeSize ();

                                        // create array of interpolation weights, same size and resolution as SPVolume
Interpol4NN.Resize ( volsize );

                                        // clear array
Interpol4NN.ResetMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                DistListEnum
                    {
                    Distance,
                    Index,
                    NumDistList
                    };

//TPoints&          Points          = GetPoints ( DisplaySpace3D );
int                 mrithreshold    = MRIGrey->GetCsfCut ();

OmpParallelBegin

TArray2<float>      distlist ( 4, NumDistList );// list of the distances of the current closest 4 points
//TArray2<float>    distlist ( GetNumSolPoints (), NumDistList );// list of the distances to all points
TPointFloat         pvol;
TPointFloat         pmri;

OmpFor
                                        // scan interpolation box
for ( int pvolZ = 0; pvolZ < volsize.Z; pvolZ++ ) {

    pvol.Z  = pvolZ;    // OpenMP whining

//  Gauge.Next ();

    for ( pvol.Y = 0; pvol.Y < volsize.Y; pvol.Y++ )
    for ( pvol.X = 0; pvol.X < volsize.X; pvol.X++ ) {

                                        // convert to absolute MRI coordinates
        pmri    = pvol;

        VolumeToAbsolute ( pmri );
        MRIGrey->ToRel   ( pmri );

        pmri   += 0.5;                  // !not Round, we shift but keep the point in float!

                                        // check coordinates fit into MRI clipping & that MRI mask not null
        if ( greyvol.GetValueChecked ( pmri.X, pmri.Y, pmri.Z ) <= mrithreshold )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set current list of distances
        distlist ( 0 , Distance ) = distlist ( 1 , Distance ) = distlist ( 2 , Distance ) = distlist ( 3 , Distance ) = Highest<float> ();
        distlist ( 0 , Index    ) = distlist ( 1 , Index    ) = distlist ( 2 , Index    ) = distlist ( 3 , Index    ) = 0;

                                        // find 4 nearest neighbors, "by hand"
        int                 lastzimax       = zindex ( 1, (int) pmri.Z );
        int                 spdl;
        const TPointFloatI* tosp;

        for ( spdl = zindex ( 0, (int) pmri.Z ), tosp = ListSPI + spdl; spdl <= lastzimax; spdl++, tosp++ ) {

                                        // save us some sqrt
            float       d       = ( *tosp - pmri ).Norm2 ();

                                        // insertion sort
            if ( d >= distlist ( 3 , Distance ) )  continue;
            if ( d >= distlist ( 2 , Distance ) ) {
                distlist ( 3 , Distance ) = d; distlist ( 3 , Index ) = tosp->Index;
                continue;
                }
            if ( d >= distlist ( 1 , Distance ) ) {
                MoveVirtualMemory ( distlist.GetArray () + 6, distlist.GetArray () + 4, 2 * distlist.AtomSize () );
                distlist ( 2 , Distance ) = d; distlist ( 2 , Index ) = tosp->Index;
                continue;
                }
            if ( d >= distlist ( 0 , Distance ) ) {
                MoveVirtualMemory ( distlist.GetArray () + 4, distlist.GetArray () + 2, 4 * distlist.AtomSize () );
                distlist ( 1 , Distance ) = d; distlist ( 1 , Index ) = tosp->Index;
                continue;
                }
                MoveVirtualMemory ( distlist.GetArray () + 2, distlist.GetArray (),     6 * distlist.AtomSize () );
                distlist ( 0 , Distance ) = d; distlist ( 0 , Index ) = tosp->Index;
            } // for sp


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        TWeightedPoints4*   toi4        = & Interpol4NN ( pvol );

                                        // special case: exactly on top of a SP
        if ( distlist ( 0 , Distance ) == 0 ) {
                                        // winner takes all
            toi4->i1 = (ushort) distlist ( 0 , Index );
                                        // assign max weight
            toi4->w1 = TWeightedPoints4SumWeights;
                                        // no other points used here
            toi4->i2 = toi4->i3 = toi4->i4 = 0;
            toi4->w2 = toi4->w3 = toi4->w4 = 0;
            }
        else {                          // do a real interpolation

                                        // normalize distances
            distlist ( 0 , Distance )   = sqrt ( distlist ( 0 , Distance ) ) / step;
            distlist ( 1 , Distance )   = sqrt ( distlist ( 1 , Distance ) ) / step;
            distlist ( 2 , Distance )   = sqrt ( distlist ( 2 , Distance ) ) / step;
            distlist ( 3 , Distance )   = sqrt ( distlist ( 3 , Distance ) ) / step;

            double  sumwi   = 1.0
                              / ( 1.0 / distlist ( 0 , Distance )
                                + 1.0 / distlist ( 1 , Distance )
                                + 1.0 / distlist ( 2 , Distance )
                                + 1.0 / distlist ( 3 , Distance ) );
                
                                        // classical, simple way
            toi4->i1 = (ushort) distlist ( 0 , Index    );
            toi4->i2 = (ushort) distlist ( 1 , Index    );
            toi4->i3 = (ushort) distlist ( 2 , Index    );
            toi4->i4 = (ushort) distlist ( 3 , Index    );

                                        // floating point case
//          toi4->w1 =  sumwi / distlist ( 0 , Distance );
//          toi4->w2 =  sumwi / distlist ( 1 , Distance );
//          toi4->w3 =  sumwi / distlist ( 2 , Distance );
//          toi4->w4 =  sumwi / distlist ( 3 , Distance );
                                        // unsigned byte case
            toi4->w1 = (uchar) Round ( sumwi / distlist ( 0 , Distance ) * TWeightedPoints4SumWeights );
            toi4->w2 = (uchar) Round ( sumwi / distlist ( 1 , Distance ) * TWeightedPoints4SumWeights );
            toi4->w3 = (uchar) Round ( sumwi / distlist ( 2 , Distance ) * TWeightedPoints4SumWeights );
//          toi4->w4 = (uchar) Round ( sumwi / distlist ( 3 , Distance ) * TWeightedPoints4SumWeights );
            toi4->w4 = (uchar) ( TWeightedPoints4SumWeights - ( toi4->w1 + toi4->w2 + toi4->w3 ) );       // make really sure the sum of weight is indeed 255

            } // else interpolation


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
        static int      fi      = 0;
        fi++;

        if ( ! ( fi % 1000 ) ) {

        TPoints         neighpoints;

        pmri    = pvol;
        VolumeToAbsolute ( pmri );
        neighpoints.Add ( pmri );

        if ( toi4->w1 == 1.0 ) 
            neighpoints.Add ( Points[ toi4->i1 ] );
        else {
            neighpoints.Add ( Points[ toi4->i1 ] );
            neighpoints.Add ( Points[ toi4->i2 ] );
            neighpoints.Add ( Points[ toi4->i3 ] );
            neighpoints.Add ( Points[ toi4->i4 ] );
            }
        TFileName       _file;
        StringCopy      ( _file, MRIGrey->GetDocPath () );
        RemoveFilename  ( _file );
        StringAppend    ( _file, "\\LocalNeighbors.4NN." );
        IntegerToString ( StringEnd ( _file ), fi, 4 );
        AddExtension    ( _file, FILEEXT_SPIRR );
        neighpoints.WriteFile ( _file );
        }*/


        } // for y, x

    } // for z

OmpParallelEnd

}

/*                                        // force overwriting all solution points
for ( spdl = 0, tosp = &ListSPI[ spdl ]; spdl < GetNumSolPoints (); spdl++, tosp++ ) {

    x   = AbsoluteToVolumeX ( tosp->x + 0.5 );
    y   = AbsoluteToVolumeY ( tosp->y + 0.5 );
    z   = AbsoluteToVolumeZ ( tosp->z + 0.5 );

    if ( x < 0 || x >= voldim1
      || y < 0 || y >= voldim2
      || z < 0 || z >= voldim3 )
        continue;

    toi4     = & Interpol4NN ( x, y, z );

    toi4->i1 = (ushort) tosp->Index;
    toi4->w1 = TWeightedPoints4SumWeights;
    toi4->i2 = toi4->i3 = toi4->i4 = 0;
    toi4->w2 = toi4->w3 = toi4->w4 = 0;
    }
*/


//----------------------------------------------------------------------------
void	TSolutionPointsDoc::ComputeInterpol1NN ( const TPointFloatI* ListSPI, double step, const TVolumeDoc* MRIGrey, const Volume& greyvol )
{
                                        // We are going to build an index in the Z axis, for faster access to points
int                 mridim3         = MRIGrey->GetSize ()->GetZExtent (); // scan MRI fully, not only the clipping, in case SPs overflowing...
TArray2<int>        zindex ( 2, mridim3 );
double              mintoeuclidean  = sqrt ( 3.0 );   // min distance can be on 1 axis, search has to be in all 3 dimensions so we need a conversion factor
double              mindz           = 2 * mintoeuclidean * step;
int                 zminmri         = MRIBounding.ZMin () - 1;  // round the double bounding
int                 zmaxmri         = MRIBounding.ZMax () + 1;

zindex.ResetMemory ();

                                        // scan Z
OmpParallelFor

for ( int iz = 0; iz < mridim3; iz++ ) {

                                        // set default meaningful limits
    zindex ( 0 , iz ) = 0;
    zindex ( 1 , iz ) = GetNumSolPoints () - 1;

    int                 spdl;
    const TPointFloatI* tosp;

    for ( spdl = 0, tosp = &ListSPI[ spdl ]; spdl < GetNumSolPoints (); spdl++, tosp++ )
           // at least 2 "slices" from top     or at least 2 "slices" below current z
        if ( ( zmaxmri - tosp->Z ) <= mindz || tosp->Z >= iz - mindz ) {
            zindex ( 0 , iz ) = spdl;
            break;
            }

    for ( ; spdl < GetNumSolPoints (); spdl++, tosp++ )
           // at least 2 "slices" from bottom  and at least 2 "slices" above current z
        if ( ( tosp->Z - zminmri ) >= mindz && tosp->Z > iz + mindz ) {
            zindex ( 1 , iz ) = spdl;
            break;
            }
    }

                                        // to scan all points, without the indexing scheme (slow but safe)
//for ( int iz = 0; iz < mridim3; iz++ ) {
//    zindex ( 0 , iz ) = 0;
//    zindex ( 1 , iz ) = GetNumSolPoints () - 1;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TSuperGauge         Gauge ( "SP 1NN Interpolation", voldim3 );


TPointInt           volsize         = GetVolumeSize ();

                                        // create array of interpolation weights, same size and resolution as SPVolume
Interpol1NN.Resize ( volsize );

                                        // max -> no interpolation
Interpol1NN     = UndefinedInterpolation1NN;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//double            mindinterpol    = Square ( step / 2 );    // Euclidian distance
//double            mindinterpol    = step * 0.50 /** ( IsGridAligned () ? 0.66 : 1.33 )*/;  // Max distance
double              mindinterpol    = step * 0.50 * mintoeuclidean;  // Max distance
int                 mrithreshold    = MRIGrey->GetCsfCut ();

OmpParallelBegin

TPointFloat         pvol;
TPointFloat         pmri;

OmpFor
                                        // scan interpolation box
for ( int pvolZ = 0; pvolZ < volsize.Z; pvolZ++ ) {

    pvol.Z  = pvolZ;    // OpenMP whining

//  Gauge.Next ();

    for ( pvol.Y = 0; pvol.Y < volsize.Y; pvol.Y++ )
    for ( pvol.X = 0; pvol.X < volsize.X; pvol.X++ ) {

                                        // convert to absolute MRI coordinates
        pmri    = pvol;

        VolumeToAbsolute ( pmri );
        MRIGrey->ToRel   ( pmri );

        pmri   += 0.5;                  // !not Round, we shift but keep the point in float!

                                        // check coordinates fit into MRI clipping & that MRI mask not null
        if ( greyvol.GetValueChecked ( pmri.X, pmri.Y, pmri.Z ) <= mrithreshold )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set distance
        double              mind    = Highest ( mind );
        int                 mini    = UndefinedInterpolation1NN;

                                        // find 1 nearest neighbors
        int                 lastzimax   = zindex ( 1, (int) pmri.Z );
        int                 spdl;
        const TPointFloatI* tosp;

        for ( spdl = zindex ( 0, (int) pmri.Z ), tosp = ListSPI + spdl; spdl <= lastzimax; spdl++, tosp++ ) {


            TPointFloat     delta   = *tosp - pmri;
                                        // use Max distance -> cubic shape
            delta.Absolute ();

            double          d       = delta.Max ();

            if ( d < mind ) {
                mind    = d;
                mini    = tosp->Index;
                }
            } // for sp


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // store only index - test not too far from SP (grey mask can be really fat sometimes)
        if ( mind <= mindinterpol )

            Interpol1NN ( pvol )    = (ushort) mini;

        } // for y, x

    } // for z

OmpParallelEnd
}

/*                                        // force SP to voxels
for ( spdl = 0, tosp = &ListSPI[ spdl ]; spdl < GetNumSolPoints (); spdl++, tosp++ ) {
                                        // formula tested in full resolution + downsampled
    pvol.Set         ( tosp );

    MRIGrey->ToAbs   ( pvol );
    AbsoluteToVolume ( pvol );

    pvol.Round ();
//  pvol   += 0.5;                  // !not Round, we shift but keep the point in float!

    if ( ! Interpol1NN.WithinBoundary ( pvol ) )
        continue;

    Interpol1NN ( pvol ) = (ushort) tosp->Index;
    }
return;
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
