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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Stats.h"
#include    "Math.Random.h"
#include    "TVector.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "TList.h"
#include    "Strings.Utils.h"

#include    "TTracksDoc.h"
#include    "TElectrodesDoc.h"

#include    "TCartoolMdiClient.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char          ElelectrodesContentTypeNames[ NumElContentType ][ ContentTypeMaxChars ] = 
                    {
                    "",
                    "Surface",
                    "Grid",
                    "Compound",
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TElectrodesDoc::TElectrodesDoc (TDocument *parent)
      : TBaseDoc (parent)
{
SetAtomType ( AtomTypeCoordinates );

ContentType             = ContentTypeElectrodes;
ExtraContentType        = UnknownElContentType;
CopyVirtualMemory ( ExtraContentTypeNames, ElelectrodesContentTypeNames, NumElContentType * ContentTypeMaxChars );

NumElectrodes           = 0;
ProjEquator             = 0;

NumClusters             = 0;
Clusters                = 0;
ClusterIndexes          = 0;

DisplaySpaces.Set ( ElectrodesNumSpaces );
}


const char* TElectrodesDoc::GetElectrodeName ( const char* name, int* index )
{
for ( int e = 0; e < NumElectrodes; e++ ) {

    if ( StringIs ( name, ElectrodesNames[ e ] ) ) {

        if ( index )    *index  = e;

        return  ElectrodesNames[ e ];
        }
    }

if ( index )    *index  = -1;

return  0;
}


bool    TElectrodesDoc::InitDoc ()
{
if ( ! IsOpen () )
    return  Open ( ofRead, 0 );

return  true;
}


//----------------------------------------------------------------------------
                                        // check for spaces in names
void    TElectrodesDoc::ElectrodesNamesCleanUp ()
{
for ( int e = 0; e < NumElectrodes; e++ ) {
                                        // first, remove trailing spaces
    StringCleanup ( ElectrodesNames[ e ] );

                                        // second, replace inner spaces with _
//  ReplaceChars ( ElectrodesNames[ e ], " ", "_" );
    }


for ( int c = 0; c < NumClusters; c++ )

    StringCleanup ( Clusters[ c ].Name );
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::Set ()
{
                                        // here, 3D data have been allocated and initialized

                                        // check names
ElectrodesNamesCleanUp ();

                                        // 3D setup
                                        // box the data
SetBounding       ( DisplaySpace3D );
                                        // useful measure, get an estimate of the spacing between electrodes
SetMedianDistance ( DisplaySpace3D );
                                        // guess if it is a grid (not so useful anymore)
InitContentType   ();
                                        // guess orientation
FindOrientation   ();

                                        // 2D setup
TPoints&            projpoints      = GetPoints ( DisplaySpaceProjected );
                                        // copy 3D points
projpoints.Resize ( NumElectrodes + NumPseudoTracks );
                                        // project them
ProjectElectrodes ();

SetBounding       ( DisplaySpaceProjected );

SetMedianDistance ( DisplaySpaceProjected );


                                        // copy DisplaySpaceProjected to DisplaySpaceHeight
DisplaySpaces[ DisplaySpaceHeight ]    = DisplaySpaces[ DisplaySpaceProjected ];

                                        // for both 3D and 2D spaces
SetTesselation ();
}


//----------------------------------------------------------------------------
                                        // !Used only for 3D interactive updates!
void    TElectrodesDoc::SetPoints ( int space, const TPoints& points )
{
                                        // Updating coordinates
TPoints&            docpoints       = GetPoints ( space );  // !non-const reference, as we are going to overwrite it!

                                        // there could be different numbers of points between the 2 variables (f.ex. if pseudo-electrodes are accounted for or not)
int                 numpoints       = min ( points.GetNumPoints (), docpoints.GetNumPoints () );

for ( int i = 0; i < numpoints; i++ )

    docpoints[ i ]  = points[ i ];

                                        // old way - does a lot of work for nothing, and is not working correctly anyway
//Flip ( 0 );
                                        // not happy if these are not recomputed...
SetBounding       ( DisplaySpace3D );

SetMedianDistance ( DisplaySpace3D );

//FindOrientation   ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Updating the tesselation

                                        // force updating the mesh - should be working, but does not(?)
//SetTesselation ();

                                        // OK, we can do it ourselves - for the current mesh display, we just need the 3D triangles positions updated after all
TTriangleSurface*   elrend          = GetSurfaceRendering ( space );
int                 numrendpoints   = elrend->GetNumPoints ();
TPointFloat*        lp              = elrend->GetListTriangles ();
const int*          li              = elrend->GetListTrianglesIndexes ();

for ( int i = 0; i < numrendpoints; i++ )

    lp[ i ]     = docpoints[ li[ i ] ];
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::Reset ( bool close )
{

SurfaceRendering.    Reset ();
SurfaceRenderingProj.Reset ();

                                        // total cleanup?
if ( close ) {
    if ( Clusters ) {
        delete[] Clusters;
        Clusters = 0;
        }

    if ( ClusterIndexes ) {
        delete[] ClusterIndexes;
        ClusterIndexes = 0;
        }
    }
}


//----------------------------------------------------------------------------
                                        // find dimensions of grid
                                        // by testing successive vectors
void    TElectrodesDoc::GetDimOfGrid ( int c, double* dims )    const
{
if ( Clusters[ c ].Type != ClusterGrid )
    return;


int                 numel           = Clusters[ c ].NumElectrodes;
int                 offsetindex     = 0;

for ( c--; c >= 0; offsetindex += Clusters[ c ].NumElectrodes, c-- );


TSelection          sel ( NumElectrodes, OrderSorted );
sel.Reset ();
sel.Set   ( offsetindex, offsetindex + numel - 1 );

TIteratorSelectedForward    seli ( sel );


TVector3Double      v;
TVector3Double      v1;
TVector3Double      v2;

GetCoordinates ( seli(), v,  DisplaySpace3D );

GetCoordinates ( ++seli, v1, DisplaySpace3D );

v1 -= v;

int                 Dim1            = 2;

for ( ; (bool) seli; ++seli, Dim1++ ) {

    GetCoordinates ( seli() + 1, v2, DisplaySpace3D );
    GetCoordinates ( seli(),     v , DisplaySpace3D );

    v2 -= v;
                                        // a reversed vector means going back close to 1st point
    if ( v1.IsOppositeDirection ( v2 ) )
        break;

    v1 = v2;
    }


int                 Dim2;
Dim2    = numel / ( Dim1 ? Dim1 : 1 );
Dim1    = numel / ( Dim2 ? Dim2 : 1 );
                                        // something weird here?
if ( Dim1 * Dim2 != numel ) {
    Dim1 = numel;
    Dim2 = 1;
    }

                                        // et voila
dims[ 0 ] = Dim1;
dims[ 1 ] = Dim2;

/*
if ( Dim1 * Dim2 != numel ) {
    char    buff[128];
    sprintf ( buff, "Found grid dimensions %0d x %0d,\nwhich does not match the %0d number of points!", Dim1, Dim2, nump );
    ShowMessage ( buff, "Surface Triangulation", ShowMessageWarning );
    }

if ( Dim1 < 2 || Dim2 < 2 ) {
    char    buff[128];
    sprintf ( buff, "Found grid dimensions %0d x %0d,\nerrors may occur during triangulation!", Dim1, Dim2 );
    ShowMessage ( buff, "Surface Triangulation", ShowMessageWarning );
    }
*/
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::ProjectElectrodes ()
{
TPoints&            sop             = GetPoints ( DisplaySpaceProjected );

TSelection          sel ( NumElectrodes, OrderSorted );
TVector3Double      org;
TVector3Double      step;


//step.Set  ( (double) Bounding.MeanSize () / sqrt ( NumElectrodes ) ); XYZDoc->GetMedianDistance() * 0.25
//step.Set  ( (double) Bounding.MaxSize () / 50 );
step        = GetMedianDistance ( DisplaySpace3D );

                                        // dummy equator
ProjEquator = 0;


sel.Reset ();
                                        // first, gather all points from all 3D sets
for ( int c = 0, offsetindex = 0, numel; c < NumClusters; offsetindex += numel, c++ ) {

    numel = Clusters[c].NumElectrodes;

    if ( Clusters[c].Type == Cluster3D )
        sel.Set ( offsetindex, offsetindex + numel - 1 );
    }


if ( (bool) sel ) {

    TPointFloat         pos;
    double              rxy;
    double              alpha;
    double              maxalpha        = -DBL_MAX;

/*
                                        // compute a matrix to put Top of head on Z axis
    double      center[ 3 ];
    double      mean[ 3 ];
    mean[ 0 ] = mean[ 1 ] = mean[ 2 ] = 0;

    for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

        GetCoordinates ( seli(), pos, DisplaySpace3D );

        mean[ 0 ] += pos[ 0 ];
        mean[ 1 ] += pos[ 1 ];
        mean[ 2 ] += pos[ 2 ];
        }

    mean[ 0 ] /= (int) sel;
    mean[ 1 ] /= (int) sel;
    mean[ 2 ] /= (int) sel;

    GetBounding ( DisplaySpace3D )->GetMiddle ( center );
                                        // relative difference from barycenter to middle
    mean[ 0 ] = ( mean[ 0 ] - center[ 0 ] ) / GetBounding ( DisplaySpace3D )->GetXExtent ();
    mean[ 1 ] = ( mean[ 1 ] - center[ 1 ] ) / GetBounding ( DisplaySpace3D )->GetYExtent ();
    mean[ 2 ] = ( mean[ 2 ] - center[ 2 ] ) / GetBounding ( DisplaySpace3D )->GetZExtent ();

//    DBGV3 ( mean[ 0 ], mean[ 1 ], mean[ 2 ], "ratio X Y Z" );
    TMatrix44           M;
/ *
                                        // the highest value gives the Top of head
    if      ( fabs ( mean[ 0 ] ) > fabs ( mean[ 1 ] ) && fabs ( mean[ 0 ] ) > fabs ( mean[ 2 ] ) )
        M.RotateY ( mean[ 0 ] > 0 ? 90 : -90, MultiplyLeft );
    else if ( fabs ( mean[ 1 ] ) > fabs ( mean[ 0 ] ) && fabs ( mean[ 1 ] ) > fabs ( mean[ 2 ] ) )
        M.RotateX ( mean[ 1 ] > 0 ? -90 : 90, MultiplyLeft );
    else {
        M.RotateY ( mean[ 2 ] < 0 ? 180 : 0, MultiplyLeft );
        M.RotateZ ( mean[ 2 ] < 0 ? 180 : 0, MultiplyLeft );
        }
*/
                                        // transform into space AL(S)
    TMatrix44           M ( GetStandardOrientation ( LocalToPIR ) );

    M.RotateZ ( 180, MultiplyLeft );
    M.RotateX (  90, MultiplyLeft );

                                        // transform non-null points
    TPoints             trpoints;

    for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

        GetCoordinates ( seli(), pos, DisplaySpace3D );

        if ( pos.IsNull () )            // not a good point
            continue;
                                        // re-orient
        M.Apply ( pos );

        trpoints.Add ( pos );
        }

                                        // get supposed best center
    TBoundingBox<double>    boxpoints ( trpoints );
    TPointFloat             center;
    bool                    iscentered;

                                        // guess the center is correct by checking the bounding box
    iscentered  = boxpoints.Min ( 0 ) * boxpoints.Max ( 0 ) < 0
               && boxpoints.Min ( 1 ) * boxpoints.Max ( 1 ) < 0
               && boxpoints.Min ( 2 ) * boxpoints.Max ( 2 ) < 0;

    if ( ! iscentered )                 // use an approximate center
        center.Set ( (float) boxpoints.XMean (), (float) boxpoints.YMean (), (float) boxpoints.ZMean () );
    else
        center.Reset ();

                                        // get top of projection
    double              zmax            = boxpoints.ZMax () - center.Z;
                                        // include translation
    M.Translate ( -center.X, -center.Y, -center.Z, MultiplyLeft );

                                        // project
    for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

        GetCoordinates ( seli(), pos, DisplaySpace3D );
                                        // re-orient & center
        M.Apply ( pos );

        rxy         = sqrt ( pos.X * pos.X + pos.Y * pos.Y );

        alpha       = ArcTangent ( rxy, pos.Z );
        maxalpha    = max ( alpha, maxalpha );

        pos        *= alpha / ( rxy ? rxy : 1 );
                                        // not absolutely exact, we are not always on a sphere
        sop[ seli() ].Set ( (float) ( pos.X * zmax ), (float) ( pos.Y * zmax ), 0 );
        }

                                        // equator position
    ProjEquator = iscentered ? HalfPi * zmax : 0;


    org.X       = - maxalpha * zmax - 2 * step.X;
    org.Y       = 0;
    org.Z       = 0;
    }

                                        // then project all the others: grid, strip, single electrode
for ( int c = 0, offsetindex = 0, numel; c < NumClusters; offsetindex += numel, c++ ) {

    numel = Clusters[c].NumElectrodes;

    if ( Clusters[c].Type == Cluster3D )
        continue;

    sel.Reset ();
    sel.Set   ( offsetindex, offsetindex + numel - 1 );


    if ( Clusters[c].Type == ClusterPoint ) {

        for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli )

            sop[ seli() ] = TPointFloat ( (float) org.X, (float) ( org.Y - seli.GetIndex () * 2 * step.Y ), (float) org.Z );


        org.X   -= 2 * step.X;
        org.Y    = 0;
        org.Z    = 0;
        }

    else if ( Clusters[c].Type == ClusterLine ) {
/*                                      // to reorder strips according with spatial consistency
        TVector3Double      dirstrip;
        double              pos1[ 3 ];
        double              pos2[ 3 ];
        double              center[ 3 ];
        Bounding.GetCenter ( center );

                                        // direction not yet computed?
        if ( !dir[ 0 ] && !dir[ 1 ] && !dir[ 2 ] ) {

            GetCoordinates ( sel.FirstSelected (), pos1, DisplaySpace3D );
            GetCoordinates ( sel.LastSelected  (), pos2, DisplaySpace3D );

            dir[ 0 ] = pos1[ 0 ] + pos2[ 0 ] - 2 * center[ 0 ];
            dir[ 1 ] = pos1[ 1 ] + pos2[ 1 ] - 2 * center[ 1 ];
            dir[ 2 ] = pos1[ 2 ] + pos2[ 2 ] - 2 * center[ 2 ];
            dir.Normalize ();
            }

                                        // estimate current direction
        GetCoordinates ( sel.FirstSelected (), pos1, DisplaySpace3D );
        GetCoordinates ( sel.LastSelected  (), pos2, DisplaySpace3D );

        dirstrip[ 0 ] = pos1[ 0 ] + pos2[ 0 ] - 2 * center[ 0 ];
        dirstrip[ 1 ] = pos1[ 1 ] + pos2[ 1 ] - 2 * center[ 1 ];
        dirstrip[ 2 ] = pos1[ 2 ] + pos2[ 2 ] - 2 * center[ 2 ];

        bool    invert  = dirstrip * dir < 0;
*/

        for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli )

            sop[ seli() ] = TPointFloat ( (float) org.X, (float) ( org.Y - seli.GetIndex () * step.Y ), (float) org.Z );
//          sop[ seli() ] = TPointFloat ( org.X, org.Y + ( invert ? -numel + 1 + seli.GetIndex () : - seli.GetIndex () ) * step.Y, org.Z );


        org.X   -= 2 * step.X;
        org.Y    = 0;
        org.Z    = 0;
        }

    else if ( Clusters[c].Type == ClusterGrid ) {
                                        // get dimensions of grid
        double              Dims[ 2 ];
        GetDimOfGrid ( c, Dims );

        int                 Dim1 = (int) Dims[ 0 ];
        int                 Dim2 = (int) Dims[ 1 ];


        for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli )

            sop[ seli() ] = TPointFloat ( (float) ( org.X - ( seli.GetIndex () / Dim1 ) * step.Y ), (float) ( org.Y - ( seli.GetIndex () % Dim1 ) * step.Y ), (float) org.Z );


        org.X   -= ( Dim2 + 1 ) * step.X;
        org.Y    = 0;
        org.Z    = 0;
        }
    }
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::SetBounding ( int space )
{
DisplaySpaces[ space ].Bounding.Set ( NumElectrodes, GetPoints ( space ) );

if ( space == DisplaySpaceProjected ) {
                                        // middle is better in 2D, as to give a full view of the data
    double              mid[ 3 ];

    DisplaySpaces[ space ].Bounding.SetCenter ( DisplaySpaces[ space ].Bounding.GetMiddle ( mid ) );
    }
}


//----------------------------------------------------------------------------
                                        // min distance between electrodes
                                        // (actually, a kind of weighted mean)
void    TElectrodesDoc::SetMedianDistance ( int space )
{
TEasyStats          statd ( NumElectrodes );
double              norm2;
double              minnorm2;
TPointFloat         p1;
TPointFloat         p2;

double              MedDistance     = 0;
double              ElRadius        = 0;
int                 numc            = 0;


for ( int c = 0, firstel = 0, lastel = Clusters[ c ].NumElectrodes - 1; c < NumClusters; c++, firstel = lastel + 1, lastel += c < NumClusters ? Clusters[ c ].NumElectrodes : 0 ) {

    statd.Reset ();

    for ( int e = firstel; e < lastel; e++ ) {

        GetCoordinates ( e, p1, space );

        if ( p1.IsNull () )
            continue;


        minnorm2    = DBL_MAX;

        for ( int f = e + 1; f <= lastel; f++ ) {

            GetCoordinates ( f, p2, space );

            if ( p2.IsNull () )
                continue;

            norm2       = ( p2 - p1 ).Norm2 ();

            minnorm2    = min ( norm2, minnorm2 );
            } // for f

        statd.Add ( sqrt ( minnorm2 ) );
        } // for e

                                        // add only non-null distance
    if ( (bool) statd ) {
                                        // unit interval
        MedDistance    += statd.Median ( false );
                                                                               // weight the radius by the number of electrodes
        ElRadius       += statd.Median ( false ) * ( Clusters[ c ].Type != ClusterLine ? log ( 1 + Clusters[ c ].NumElectrodes / 4.0 ) / 6.0 : 1 );

        numc++;
        }

    } // for cluster

                                        // add some safety checks
MedDistance     = NumElectrodes <= 1 || MedDistance <= 0 || numc == 0 ? 1.0 : MedDistance / numc;
ElRadius        = NumElectrodes <= 1 || ElRadius    <= 0 || numc == 0 ? 0.1 : ElRadius    / ( 2 * numc );


DisplaySpaces[ space ].MedianDistance   = MedDistance;
DisplaySpaces[ space ].PointRadius      = ElRadius;
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::SetTesselation ()
{
const TPoints&      points          = GetPoints ( DisplaySpace3D );

                                        // stores the points to be tesselated
TSelection          sel     ( NumElectrodes, OrderSorted );

                                        // remember which 3D clusters have been done
TSelection          cludone ( NumClusters,   OrderSorted );
cludone.Reset ();


double              params[ 3 ];
params[ 0 ] = GetMedianDistance ( DisplaySpace3D );


SurfaceRendering.    Reset ();
SurfaceRenderingProj.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first, do all 3D sets
                                        // allow blending different clusters with same name together
                                        // therefore, there is no mandatory 1 cluster <-> 1 tesselation
for ( int c = 0, offsetindex = 0; c < NumClusters; offsetindex += Clusters[ c ].NumElectrodes, c++ ) {

    sel.Reset ();

    if ( Clusters[ c ].Type != Cluster3D )  continue;
    if ( cludone[ c ] )                     continue;


    for ( int c2 = c, offsetindex2 = offsetindex; c2 < NumClusters; offsetindex2 += Clusters[ c2 ].NumElectrodes, c2++ ) {

        if ( Clusters[ c2 ].Type != Cluster3D ) continue;
        if ( cludone[ c2 ] )                    continue;
                                        // not same name?
        if ( StringIsNot ( Clusters[ c2 ].Name, Clusters[ c ].Name ) )
            continue;

                                        // mark this cluster
        cludone.Set ( c2 );
                                        // add to current selection
        sel.Set ( offsetindex2, offsetindex2 + Clusters[ c2 ].NumElectrodes - 1 );
                                        // set local bounding
        Clusters[ c2 ].Bounding.Set ( Clusters[ c2 ].NumElectrodes, &points[ offsetindex2 ] );
        }

    if ( (bool) sel ) {
                                        // tesselate this cluster
        TTriangleSurface    tess ( GetPoints ( DisplaySpace3D ), sel, Cluster3D, params );

        SurfaceRendering       += tess;
                                        // !if 3D, use the same triangulation!
        SurfaceRenderingProj   += TTriangleSurface ( tess, GetPoints ( DisplaySpaceProjected ) );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // then do all the other, non-3D sets
for ( int c = 0, offsetindex = 0; c < NumClusters; offsetindex += Clusters[ c ].NumElectrodes, c++ ) {

    if ( Clusters[ c ].Type == Cluster3D )  continue;


    sel.Reset ();
    sel.Set ( offsetindex, offsetindex + Clusters[ c ].NumElectrodes - 1 );

                                        // set local bounding
    Clusters[ c ].Bounding.Set ( Clusters[ c ].NumElectrodes, &points[ offsetindex ] );

                                        // get dimensions if grid
    if ( Clusters[ c ].Type == ClusterGrid )
        GetDimOfGrid ( c, params + 1 );

                                        // add tesselation to global rendering
    SurfaceRendering       += TTriangleSurface ( GetPoints ( DisplaySpace3D        ), sel, Clusters[ c ].Type, params );

                                        // do a new tesselation with the projected coordinates
    SurfaceRenderingProj   += TTriangleSurface ( GetPoints ( DisplaySpaceProjected ), sel, Clusters[ c ].Type, params );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can also do a full TOrientation object, if needed real orientation
bool                has3D           = false;

for ( int c = 0; c < NumClusters && ! has3D ; c++ )
    has3D |= Clusters[ c ].Type == Cluster3D;

if ( has3D ) {
    ProjBoxSides[ XMinSide ]    = ToBack;
    ProjBoxSides[ XMaxSide ]    = ToFront;
    ProjBoxSides[ YMinSide ]    = ToRight;
    ProjBoxSides[ YMaxSide ]    = ToLeft;
    ProjBoxSides[ ZMinSide ]    = ToDown;
    ProjBoxSides[ ZMaxSide ]    = ToUp;
    ProjBoxSides[ XAxis    ]    = FrontBack;
    ProjBoxSides[ YAxis    ]    = LeftRight;
    ProjBoxSides[ ZAxis    ]    = UpDown;
    }
else {
    ProjBoxSides[ XMinSide ]    = ToXMin;
    ProjBoxSides[ XMaxSide ]    = ToXMax;
    ProjBoxSides[ YMinSide ]    = ToYMin;
    ProjBoxSides[ YMaxSide ]    = ToYMax;
    ProjBoxSides[ ZMinSide ]    = ToZMin;
    ProjBoxSides[ ZMaxSide ]    = ToZMax;
    ProjBoxSides[ XAxis    ]    = ToXAxis;
    ProjBoxSides[ YAxis    ]    = ToYAxis;
    ProjBoxSides[ ZAxis    ]    = ToZAxis;
    }
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::InitContentType ()
{
                                        // could be smarter than that...
if ( NumClusters == 1 )
    if ( Clusters[ 0 ].Type == ClusterGrid )
        ExtraContentType    = ElContentTypeGrid;
    else
        ExtraContentType    = ElContentTypeSurface;
else
    ExtraContentType    = ElContentTypeComplex;
}


void    TElectrodesDoc::SetBadElectrodes ( const TSelection& sel, bool notify )
{
if ( sel != BadElectrodes ) {
                                        // copy new bad electrodes
    BadElectrodes   = sel;
                                        // document has changed
    SetDirty ( true );
                                        // tell the good news to others
    if ( notify )
        NotifyDocViews ( vnNewBadSelection, (TParam2) &BadElectrodes );
    }
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::Flip ( owlwparam wparam )
{
                                        // not allowed to permutate if it is already in use
//if ( (bool) UsedBy )
//    return;

TPoints             NewPoints ( GetPoints ( DisplaySpace3D ).GetNumPoints () );
TPointFloat         p;

                                        // do the permutation
for ( int e = 0; e < NumElectrodes; e++ ) {

    GetCoordinates ( e, p, DisplaySpace3D );


    if      ( wparam == IDB_FLIPX )  p.X = -p.X;
    else if ( wparam == IDB_FLIPY )  p.Y = -p.Y;
    else if ( wparam == IDB_FLIPZ )  p.Z = -p.Z;
    else if ( wparam == IDB_FLIPXY ) Permutate ( p.X, p.Y );
    else if ( wparam == IDB_FLIPYZ ) Permutate ( p.Y, p.Z );
    else if ( wparam == IDB_FLIPXZ ) Permutate ( p.X, p.Z );

                                        // put it in a new list
    NewPoints[ e ]  = p;
    }

                                        // a call to the destructor
Reset ();
                                        // copy new points
GetPoints ( DisplaySpace3D ) = NewPoints;
                                        // compute all sorts of things
Set ();

CartoolMdiClient->RefreshWindows ();

SetDirty ( true );
}


//----------------------------------------------------------------------------
                                        // scan all electrodes for the closest match
int     TElectrodesDoc::NearestElement ( TPointFloat& from, int space, double dmax )   const
{
const TPoints&      points          = GetPoints ( space );
int                 index           = -1;
double              dmin            = DBL_MAX;


for ( int e = 0; e < NumElectrodes; e++ ) {

    double      d       = ( points[ e ] - from ).Norm2 ();

    if ( d < dmin ) {
        dmin    = d;
        index   = e;
        }
    }


return dmax > 0 ? ( dmin <= Square ( dmax ) ? index : -1 )
                : index;
}


//----------------------------------------------------------------------------
                                        // Wrapper to convert the selection string to a TSelection
void    TElectrodesDoc::ExtractToFile ( const char* xyzfile, const char* selectionstring, bool removeselection )    const
{
TSelection      selection ( NumElectrodes, OrderArbitrary );

selection.Set ( selectionstring, &ElectrodesNames );

ExtractToFile ( xyzfile, selection, removeselection );
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::GetNeighborhoodIndexes ( TArray2<int>& neighborhood )   const
{
neighborhood.DeallocateMemory ();

                                        // Getting the Delaunay triangulation
const TTriangleSurface*     elrend          = GetSurfaceRendering ( DisplaySpace3D );

if ( elrend == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtri          = elrend->GetNumTriangles ();
const int*          liidx           = elrend->GetListTrianglesIndexes ();
const int*          toi1;
const int*          toi2;
const int*          toi3;
int                 t;


TArray2<uchar>      allneighbors ( NumElectrodes, NumElectrodes + 1 );

                                        // scan triangulation
for ( t = 0, toi1 = liidx, toi2 = liidx + 1, toi3 = liidx + 2 ; t < numtri; t++, toi1 += 3, toi2 += 3, toi3 += 3 ) {
                                        // set big neighborhood matrix
                                        // one advantage here is that even if a triangle is missing, its neighbors are enough to set all the neighborhood correctly
    allneighbors ( *toi1, *toi2 )   = true;
    allneighbors ( *toi1, *toi3 )   = true;
    allneighbors ( *toi2, *toi1 )   = true;
    allneighbors ( *toi2, *toi3 )   = true;
    allneighbors ( *toi3, *toi1 )   = true;
    allneighbors ( *toi3, *toi2 )   = true;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Count each point number of neighbors + max of all of them
int                 maxneighbors    = 0;


for ( int e1 = 0; e1 < NumElectrodes; e1++ ) {

    for ( int e2 = 0; e2 < NumElectrodes; e2++ )

        if ( e1 != e2 && allneighbors ( e1, e2 ) )  allneighbors ( e1, NumElectrodes )++;

    Maxed ( maxneighbors, (int) allneighbors ( e1, NumElectrodes ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocate neighborhood array - one more for the actual count
neighborhood.Resize ( NumElectrodes, maxneighbors + 1 );

                                        // then transfer to smaller array
for ( int e1 = 0;      e1 < NumElectrodes; e1++ )
for ( int e2 = e1 + 1; e2 < NumElectrodes; e2++ )   // we're supposed to be symmetrical...

    if ( allneighbors ( e1, e2 ) ) {

        neighborhood ( e1, ++neighborhood ( e1, 0 ) ) = e2;
        neighborhood ( e2, ++neighborhood ( e2, 0 ) ) = e1;
        }
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::GetNeighborhoodDistances ( TArray2<double>& neighborhood )  const
{
neighborhood.Resize ( NumElectrodes, NumElectrodes );

neighborhood.ResetMemory ();


double              step            = GetMedianDistance ( DisplaySpace3D );

if ( step <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TPoints&      points          = GetPoints ( DisplaySpace3D );


for ( int e1 = 0;      e1 < NumElectrodes; e1++ )
for ( int e2 = e1 + 1; e2 < NumElectrodes; e2++ )
                                        // normalized by the median distance
    neighborhood ( e1, e2 ) = neighborhood ( e2, e1 )   = ( points[ e1 ] - points[ e2 ] ).Norm () / step;
}


//----------------------------------------------------------------------------
void    TElectrodesDoc::GetNeighborhoodDistances ( TArray3<double>& neighborhood, int maxnumneigh )     const
{
                                  // self + neighbors count
neighborhood.Resize ( NumElectrodes, maxnumneigh + 1, NumNeighborhoodInfo );

neighborhood.ResetMemory ();


double              step            = GetMedianDistance ( DisplaySpace3D );

if ( step <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TPoints&      points          = GetPoints ( DisplaySpace3D );
                                        // neighborhood info for a single electrode
TArray2<double>     neigh ( NumElectrodes, NumNeighborhoodInfo );


for ( int e1 = 0; e1 < NumElectrodes; e1++ ) {

    for ( int e2 = 0; e2 < NumElectrodes; e2++ ) {
                                        // index
        neigh ( e2, NeighborhoodIndex    )  = e2;
                                        // Norm2 is enough for sorting
        neigh ( e2, NeighborhoodDistance )  = e1 == e2 ? 0 : ( points[ e1 ] - points[ e2 ] ).Norm2 ();
        }

                                        // sort by distance2
    neigh.SortRows ( NeighborhoodDistance, Ascending );

                                        // transfer all distances and their respective index
    for ( int e2 = 0; e2 <= maxnumneigh; e2++ ) {

        neighborhood ( e1, e2, NeighborhoodIndex    )   = neigh ( e2, NeighborhoodIndex     );
        neighborhood ( e1, e2, NeighborhoodDistance )   = sqrt ( neigh ( e2, NeighborhoodDistance ) ) / step;   // normalized by the median distance
        }
    }

}


//----------------------------------------------------------------------------
void    TElectrodesDoc::FindOrientation ()
{
TOrientationType    XAxis           = OrientationUnknown;
TOrientationType    YAxis           = OrientationUnknown;
TOrientationType    ZAxis           = OrientationUnknown;

int                 numdirnames     = 0;

TVector3Float       pf;
TVector3Double      p1;
TVector3Double      p2;
TVector3Double      p3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case for Head Models, which are NOT electrodes coordinates. Plus we know we are in RAS orientation
if ( StringContains ( GetTitle (), "." InfixHeadModel "." ) ) {

    SetOrientation ( ToRight, ToFront, ToUp );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Old detection was limited to a very little set of knowned paired electrodes, which could fail if the net has weird holes in it
/*
                                        // 2 arrays with pairs of electrodes names giving a meaningful direction
char                PairsFront[][ 2 ][ ElectrodeNameSize ] = { { "Iz", "Nz" }, { "Oz", "Fpz" }, { "Pz", "Fz" }, { "CPz", "FCz" } };
char                PairsRight[][ 2 ][ ElectrodeNameSize ] = { { "C1", "C2" }, { "C3", "C4" }, { "C5", "C6" }, { "T3", "T4" }, { "T9", "T10" }, { "T7", "T8" }, { "P7", "P8" }, { "A1", "A2" } };

int                 NumPairsFront   = sizeof ( PairsFront ) / 2 / ElectrodeNameSize;
int                 NumPairsRight   = sizeof ( PairsRight ) / 2 / ElectrodeNameSize;

int                 e1, e2;


for ( int p = 0; p < NumPairsFront; p++ ) {

    if ( ! ( GetElectrodeName ( PairsFront[ p ][ 0 ], &e1 ) && GetElectrodeName ( PairsFront[ p ][ 1 ], &e2 ) ) )
        continue;
                                        // found a pair, get coordinates
    GetCoordinates ( e1, p1, DisplaySpace3D );
    GetCoordinates ( e2, p2, DisplaySpace3D );
                                        // compute vector
    p2     -= p1;
    p3      = p2;
    p2.Absolute ();
                                        // check for greatest axis component
    if      ( p2[ 0 ] > p2[ 1 ] && p2[ 0 ] > p2[ 2 ] )  XAxis = p3[ 0 ] > 0 ? ToFront : ToBack;
    else if ( p2[ 1 ] > p2[ 0 ] && p2[ 1 ] > p2[ 2 ] )  YAxis = p3[ 1 ] > 0 ? ToFront : ToBack;
    else                                                ZAxis = p3[ 2 ] > 0 ? ToFront : ToBack;
                                        // add one direction found
    numdirnames++;
    break;
    }

                                        // scan for electrodes pointing to Right (and only if Front has been found!)
for ( int p = 0; p < NumPairsRight && numdirnames == 1; p++ ) {

    if ( ! ( GetElectrodeName ( PairsRight[ p ][ 0 ], &e1 ) && GetElectrodeName ( PairsRight[ p ][ 1 ], &e2 ) ) )
        continue;
                                        // found a pair, get coordinates
    GetCoordinates ( e1, p1, DisplaySpace3D );
    GetCoordinates ( e2, p2, DisplaySpace3D );
                                        // compute vector
    p2     -= p1;
    p3      = p2;
    p2.Absolute ();
                                        // check for greatest axis component
    if      ( p2[ 0 ] > p2[ 1 ] && p2[ 0 ] > p2[ 2 ] )  XAxis = p3[ 0 ] < 0 ? ToLeft : ToRight;
    else if ( p2[ 1 ] > p2[ 0 ] && p2[ 1 ] > p2[ 2 ] )  YAxis = p3[ 1 ] < 0 ? ToLeft : ToRight;
    else                                                ZAxis = p3[ 2 ] < 0 ? ToLeft : ToRight;
                                        // add one direction found
    numdirnames++;
    break;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Look for 10-10 system-like names: FPz AF1 F2 Oz O3 etc...
                                        // Cumulate all front, back, left and right electrodes independently (using weights for the best ones)
                                        // then compute a global direction from the paires front/back and left/right
                                        // This method is stronger then the old one, as we don't look for specific names, but all of them.
                                        // Note: worse case of false positive detection would be names starting with 'F' and 'O' or 'P' in electrodes names, very unlikely
char                ename[ 256 ];
int                 evalue;
TVector3Double      front;
TVector3Double      back;
TVector3Double      left;
TVector3Double      right;


for ( int e = 0; e < NumElectrodes; e++ ) {

    StringCopy      ( ename, GetElectrodeName ( e ) );
    GetCoordinates  ( e, p1, DisplaySpace3D );

                                        // look at strings that points toward front or back
    if      ( StringStartsWith ( ename, "FP" ) )    front  += p1 * 4;
    else if ( StringStartsWith ( ename, "NZ" ) )    front  += p1 * 4;
    else if ( StringStartsWith ( ename, "AF" ) )    front  += p1 * 3;
    else if ( StringStartsWith ( ename, "F"  ) )    front  += p1 * 2;
    else if ( StringStartsWith ( ename, "FC" ) )    front  += p1 * 1;

    else if ( StringStartsWith ( ename, "O"  ) )    back   += p1 * 4;
    else if ( StringStartsWith ( ename, "IZ" ) )    back   += p1 * 4;
    else if ( StringStartsWith ( ename, "PO" ) )    back   += p1 * 3;
    else if ( StringStartsWith ( ename, "P"  ) )    back   += p1 * 2;
    else if ( StringStartsWith ( ename, "CP" ) )    back   += p1 * 1;

                                        // look at names like 'ABC123', where the number points toward left or right
    KeepChars ( ename, "0123456789" );
    evalue      = StringToInteger ( ename );
//    DBGV ( evalue, ename );

       // first char is space: was a letter, and value is not null
    if ( ename[ 0 ] == ' ' && evalue )
        if ( IsOdd ( evalue ) )                     left   += p1;
        else                                        right  += p1;

                                        // analysis of up/down is a little more tricky - also, if any of the above fail, chances are high the up/down would fail too
    }

                                        // both front & back should have been detected
if ( front.IsNotNull () && back.IsNotNull () ) {
                                        // not normalizing gives more weight to the consistent electrodes, like "FP" or "O"
//  front.Normalize ();
//  back .Normalize ();
    front  -= back;
    front.Normalize ();
//  front.Show ( "front" );

    p1      = front;
    p1.Absolute ();
                                        // check for greatest axis component
    if      ( p1[ 0 ] > p1[ 1 ] && p1[ 0 ] > p1[ 2 ] )  XAxis = front[ 0 ] > 0 ? ToFront : ToBack;
    else if ( p1[ 1 ] > p1[ 0 ] && p1[ 1 ] > p1[ 2 ] )  YAxis = front[ 1 ] > 0 ? ToFront : ToBack;
    else                                                ZAxis = front[ 2 ] > 0 ? ToFront : ToBack;

    numdirnames++;
    }

                                        // both left & right should have been detected, and front/back succeeded, too!
if ( numdirnames && right.IsNotNull () && left.IsNotNull () ) {

//  left .Normalize ();
//  right.Normalize ();
    right  -= left;
    right.Normalize ();

//  right.Show ( "right" );

    p1      = right;
    p1.Absolute ();
                                        // check for greatest axis component
    if      ( p1[ 0 ] > p1[ 1 ] && p1[ 0 ] > p1[ 2 ] )  XAxis = right[ 0 ] > 0 ? ToRight : ToLeft;
    else if ( p1[ 1 ] > p1[ 0 ] && p1[ 1 ] > p1[ 2 ] )  YAxis = right[ 1 ] > 0 ? ToRight : ToLeft;
    else                                                ZAxis = right[ 2 ] > 0 ? ToRight : ToLeft;

    numdirnames++;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // found the 2 directions through names scanning?
if ( numdirnames == 2 ) {
                                        // finish the job with a vectorial product
    CompleteOrientation ( XAxis, YAxis, ZAxis );

//    DBGM3 ( OrientationLabels[XAxis], OrientationLabels[YAxis], OrientationLabels[ZAxis], "Found by name  X Y Z" );

    SetOrientation ( XAxis, YAxis, ZAxis );
    return;
    }
else {
    XAxis   = OrientationUnknown;       // no, reset direction, and rely on geometric scanning
    YAxis   = OrientationUnknown;
    ZAxis   = OrientationUnknown;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we will analyse the geometry of electrodes to guess the orientation

                                        // put in the selection ALL electrodes of type Cluster3D
TSelection          sel ( NumElectrodes, OrderSorted );
sel.Reset ();

for ( int c = 0, offsetindex = 0, numel; c < NumClusters; offsetindex += numel, c++ ) {

    numel = Clusters[ c ].NumElectrodes;

    if ( Clusters[ c ].Type == Cluster3D ) {
        sel.Set ( offsetindex, offsetindex + numel - 1 );
//      break; // remove this to take all 3D
        }
    }

                                        // plus, check out null ones
for ( int e = 0; e < NumElectrodes; e++ ) {

    GetCoordinates ( e, p1, DisplaySpace3D );

    if ( p1.IsNull () )
        sel.Reset ( e );
    }

                                        // not enough points? (arbitrary)
if ( (int) sel <= 10 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TBoundingBox<double>* bounding        = GetBounding ( DisplaySpace3D );
TVector3Double              center;
TVector3Double              middle;

//bounding->GetCenter ( center );
bounding->GetMiddle ( middle );


                                        // compute the real center of our points
center      = (double) 0;

for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), p1, DisplaySpace3D );

    center     += p1;
    }

center     /= (int) sel;

//center.Show ( "new center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Find Left-Right direction
                                        // This is the strongest measure, so we begin with it!

                                        // compute how symetric are the electrodes according to 3 planes passing trough center
                                        // works pretty well even with slants, though there is a limit to the angles vs the symetry of electrodes
TEasyStats          stat1;
TEasyStats          stat2;
TEasyStats          stat3;

stat1.Reset ();     stat2.Reset ();     stat3.Reset ();


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), p1, DisplaySpace3D );

    p2      = p1 - center;

    pf[ 0 ] = (float) ( 2 * center[ 0 ] - p1[ 0 ] );
    pf[ 1 ] = (float)                     p1[ 1 ];
    pf[ 2 ] = (float)                     p1[ 2 ];

    GetCoordinates ( NearestElement ( pf, DisplaySpace3D ), p2, DisplaySpace3D );

//    stat1.Add ( p2[ 0 ] - pf[ 0 ] );
//    stat1.Add ( fabs ( p2[ 0 ] - pf[ 0 ] ) );
    stat1.Add ( ( p2 - TPointDouble ( &pf ) ).Norm2 () );


    pf[ 0 ] = (float)                     p1[ 0 ];
    pf[ 1 ] = (float) ( 2 * center[ 1 ] - p1[ 1 ] );
    pf[ 2 ] = (float)                     p1[ 2 ];

    GetCoordinates ( NearestElement ( pf, DisplaySpace3D ), p2, DisplaySpace3D );

//    stat2.Add ( p2[ 1 ] - pf[ 1 ] );
//    stat2.Add ( fabs ( p2[ 1 ] - pf[ 1 ] ) );
    stat2.Add ( ( p2 - TPointDouble ( &pf ) ).Norm2 () );


    pf[ 0 ] = (float)                     p1[ 0 ];
    pf[ 1 ] = (float)                     p1[ 1 ];
    pf[ 2 ] = (float) ( 2 * center[ 2 ] - p1[ 2 ] );

    GetCoordinates ( NearestElement ( pf, DisplaySpace3D ), p2, DisplaySpace3D );

//    stat3.Add ( p2[ 2 ] - pf[ 2 ] );
//    stat3.Add ( fabs ( p2[ 2 ] - pf[ 2 ] ) );
    stat3.Add ( ( p2 - TPointDouble ( &pf ) ).Norm2 () );
    }

//DBGV3 ( stat1.Sum2 (), stat2.Sum2 (), stat3.Sum2 (), "electrodes side symetry (sum)" );
//DBGV3 ( stat1.SD (), stat2.SD (), stat3.SD (), "electrodes side symetry (SD)" );


                                    // choose the min (highest symetry) as LeftRight
if ( stat1.Sum2 () < stat2.Sum2 () && stat1.Sum2 () < stat3.Sum2 () )   XAxis   = LeftRight;
if ( stat2.Sum2 () < stat1.Sum2 () && stat2.Sum2 () < stat3.Sum2 () )   YAxis   = LeftRight;
if ( stat3.Sum2 () < stat2.Sum2 () && stat3.Sum2 () < stat1.Sum2 () )   ZAxis   = LeftRight;

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 1)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // Need to know if we have spherical coordinates
stat1.Reset ();

for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), p1, DisplaySpace3D );
                                        // don't subtract center: spherical electrodes are centered
    stat1.Add ( p1.Norm () );
    }

//DBGV ( stat1.CoefficientOfVariation (), "Spherical Coeff" );

                                        // max spherical    : 0.002
                                        // min non-shperical: 0.058
bool                isspherical     = stat1.CoefficientOfVariation () < 0.03;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Find Front-Back axis
                                        // Compute a kind of numerical approximation of the highest eigenvector,
                                        // except we use vectors between random set of points (intrinsic relations between electrodes),
                                        // giving a global direction, which we average the ratio of the 2 unknown dimensions.
                                        // The final average ratio is very stable, even in very ambiguous coordinates.
TVector3Double      globaldir;

                                        // very stable at 100, less can be a problem
#define             MaxRandomRepeat     100
#define             MaxRandomPairs      1000
TRandUniform        randunif;
double              avgratio        = 0;

                                        // repeat a random process
for ( int randi = 0; randi < MaxRandomRepeat; randi++ ) {

    globaldir.Reset ();

                                        // sum vectors taken from random pairs
    for ( int i = 0; i < MaxRandomPairs; i++ ) {

        int         e       = sel.GetValue ( randunif ( (uint) sel.NumSet () ) );
        int         e2;

        do  e2 = sel.GetValue ( randunif ( (uint) sel.NumSet () ) );    while ( e2 == e );


        GetCoordinates ( e,  p1, DisplaySpace3D );
        GetCoordinates ( e2, p2, DisplaySpace3D );

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
                                        // 3) Find Up-Down Direction and Axis
                                        // Compute the sum of vectors from center to all points, which will point to Up
TVector3Double      updown;


                                        // scan all points
for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), p1, DisplaySpace3D );

    p1     -= center;

    p1.Normalize ();
                                        // cumulate sums
    updown += p1;
    } // for e


updown.Normalize ();

//updown.Show ( "UpDown" );

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


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), p1, DisplaySpace3D );

    if ( p1.IsNull () )     continue;

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


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Finish the job with a vectorial product
CompleteOrientation ( XAxis, YAxis, ZAxis );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 5)" );


SetOrientation ( XAxis, YAxis, ZAxis );
}


/*
                                        // 1) Find Up-Down axis
                                        // compute how asymetric are the electrodes according to 3 planes passing through center
TEasyStats          stat1;
TEasyStats          stat2;
TEasyStats          stat3;

stat1.Reset ();     stat2.Reset ();     stat3.Reset ();


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), pos, DisplaySpace3D );

    if ( pos.IsNull () )    continue;       // null point means it's void

    pos3        = pos - center;

    pos2[ 0 ]   = 2 * center[ 0 ] - pos[ 0 ];
    pos2[ 1 ]   =                   pos[ 1 ];
    pos2[ 2 ]   =                   pos[ 2 ];

    GetCoordinates ( NearestElement ( pos2, DisplaySpace3D ), pos3, DisplaySpace3D );

    if ( pos3.IsNull () )   continue;
                                        // use a vectorial difference
    stat1.Add ( ( pos3 - TVector3Double ( pos2 ) ).Norm2 () );


    pos2[ 0 ]   =                   pos[ 0 ];
    pos2[ 1 ]   = 2 * center[ 1 ] - pos[ 1 ];
    pos2[ 2 ]   =                   pos[ 2 ];

    GetCoordinates ( NearestElement ( pos2, DisplaySpace3D ), pos3, DisplaySpace3D );

    if ( pos3.IsNull () )   continue;

    stat2.Add ( ( pos3 - TVector3Double ( pos2 ) ).Norm2 () );


    pos2[ 0 ]   =                   pos[ 0 ];
    pos2[ 1 ]   =                   pos[ 1 ];
    pos2[ 2 ]   = 2 * center[ 2 ] - pos[ 2 ];

    GetCoordinates ( NearestElement ( pos2, DisplaySpace3D ), pos3, DisplaySpace3D );

    if ( pos3.IsNull () )   continue;

    stat3.Add ( ( pos3 - TVector3Double ( pos2 ) ).Norm2 () );
    }

//DBGV3 ( stat1.Sum2 (), stat2.Sum2 (), stat3.Sum2 (), "electrodes vertical symetry" );


                                    // choose the max (lowest vectorial symetry) as UpDown
if ( stat1.Sum2 () > stat2.Sum2 () && stat1.Sum2 () > stat3.Sum2 () )   XAxis   = UpDown;
if ( stat2.Sum2 () > stat1.Sum2 () && stat2.Sum2 () > stat3.Sum2 () )   YAxis   = UpDown;
if ( stat3.Sum2 () > stat2.Sum2 () && stat3.Sum2 () > stat1.Sum2 () )   ZAxis   = UpDown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1') Select between Up and Down
                                        // Count electrodes within a centered cylinder, either above or below middle

int                 countx1         = 0;
int                 countx2         = 0;
int                 county1         = 0;
int                 county2         = 0;
int                 countz1         = 0;
int                 countz2         = 0;

double              xlimit          = bounding->GetXExtent () ? bounding->GetXExtent () / 4 : 1;
double              ylimit          = bounding->GetYExtent () ? bounding->GetYExtent () / 4 : 1;
double              zlimit          = bounding->GetZExtent () ? bounding->GetZExtent () / 4 : 1;


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), pos, DisplaySpace3D );

    if ( pos.IsNull () )    continue;

    pos    -= middle;

    if ( fabs ( pos.Y ) < ylimit && fabs ( pos.Z ) < zlimit )
        if ( pos.X < 0 )    countx1++;  else    countx2++;

    if ( fabs ( pos.X ) < xlimit && fabs ( pos.Z ) < zlimit )
        if ( pos.Y < 0 )    county1++;  else    county2++;

    if ( fabs ( pos.X ) < xlimit && fabs ( pos.Y ) < ylimit )
        if ( pos.Z < 0 )    countz1++;  else    countz2++;

    }

//DBGV6 ( countx1, countx2, county1, county2, countz1, countz2, "counts x, y, z" );

                                        // more points on a side gives the top
if ( XAxis == UpDown )  XAxis   = countx1 > countx2 ? ToDown : ToUp;
if ( YAxis == UpDown )  YAxis   = county1 > county2 ? ToDown : ToUp;
if ( ZAxis == UpDown )  ZAxis   = countz1 > countz2 ? ToDown : ToUp;
*/

/*
                                        // Find Up Down direction

// Build a 2D binary histogram, in theta-phi space, used to pave
// the solid sphere with all the directions pointing to electrodes.
// The histogram can then be read for the biggest empty area within it,
// which points to where no electrodes are, the neck, the opposite of the vertex.

#define             thetaphisize    12
TArray3< int >      thetaphi ( 3, thetaphisize, thetaphisize );
int                 theta, phi;

thetaphi.Reset ();


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), pos, DisplaySpace3D );

    if ( pos.IsNull () )    continue;

    pos    -= center;
    pos.Normalize ();

    phi     = atan2 ( pos.Y, pos.X ) / M_PI * thetaphisize / 2 + thetaphisize / 2;
    theta   = acos ( pos.Z / pos.Norm () ) / M_PI * thetaphisize;
    thetaphi ( 0, theta, phi ) = 1;

    phi     = atan2 ( pos.X, pos.Z ) / M_PI * thetaphisize / 2 + thetaphisize / 2;
    theta   = acos ( pos.Y / pos.Norm () ) / M_PI * thetaphisize;
    thetaphi ( 1, theta, phi ) = 1;

    phi     = atan2 ( pos.Z, pos.Y ) / M_PI * thetaphisize / 2 + thetaphisize / 2;
    theta   = acos ( pos.X / pos.Norm () ) / M_PI * thetaphisize;
    thetaphi ( 2, theta, phi ) = 1;
    }
*/

/*
// Build 3 binary histograms of theta values, one for each axis
// The one with the biggest empty space gives the direction of
// the neck, where no electrodes point.

#define             thetaphisize    36
TArray2< int >      thetaphi ( 3, thetaphisize );

thetaphi.Reset ();


for ( TIteratorSelectedForward seli ( sel ); (bool) seli; ++seli ) {

    GetCoordinates ( seli(), pos, DisplaySpace3D );

    if ( pos.IsNull () )    continue;

    pos    -= center;
    pos.Normalize ();

    thetaphi ( 0, acos ( pos.X ) / M_PI * thetaphisize ) = 1;
    thetaphi ( 1, acos ( pos.Y ) / M_PI * thetaphisize ) = 1;
    thetaphi ( 2, acos ( pos.Z ) / M_PI * thetaphisize ) = 1;
    }

                                        // invert: true if no theta was present
for ( int axis = 0; axis < 3; axis++ )
    for ( int t = 0; t < thetaphisize; t++ )
        thetaphi ( axis, t )   = ! thetaphi ( axis, t );

/*
ofstream    ooo ( "E:\\Data\\thetaphi.txt" );
for ( int axis = 0; axis < 3; axis++ ) {
    if ( axis )     ooo << "\n";
    for ( int t = 0; t < thetaphisize; t++ )
        ooo << ( thetaphi ( axis, t ) ? "X" : "." );
    }
* /

int     firstt;
int     numt;
int     scanaxis[ 3 ][ 3 ];

for ( int axis = 0; axis < 3; axis++ ) {

    firstt  = -1;
    scanaxis[ axis ][ 0 ]   = scanaxis[ axis ][ 1 ] = scanaxis[ axis ][ 2 ] = 0;

    for ( int t = 0; t < thetaphisize; t++ ) {

        if ( thetaphi ( axis, t ) ) {

            if ( firstt == -1 )
                firstt  = t;

            if ( firstt != -1 ) {
                numt    = t - firstt + 1;

                if ( numt > scanaxis[ axis ][ 0 ] ) {
                    scanaxis[ axis ][ 0 ]   = numt;
                    scanaxis[ axis ][ 1 ]   = firstt;
                    scanaxis[ axis ][ 2 ]   = t;
                    }
                }
            }
        else
            firstt  = -1;
        }
    }

//DBGV3 ( scanaxis[ 0 ][ 0 ], scanaxis[ 1 ][ 0 ], scanaxis[ 2 ][ 0 ], "X Y Z Thetas" );
//DBGV3 ( scanaxis[ 0 ][ 0 ], scanaxis[ 0 ][ 1 ], scanaxis[ 0 ][ 2 ], "X data" );
//DBGV3 ( scanaxis[ 1 ][ 0 ], scanaxis[ 1 ][ 1 ], scanaxis[ 1 ][ 2 ], "Y data" );
//DBGV3 ( scanaxis[ 2 ][ 0 ], scanaxis[ 2 ][ 1 ], scanaxis[ 2 ][ 2 ], "Z data" );

                                        // The position on the histogram also gives us the correct direction
if ( fabs ( scanaxis[ 0 ][ 0 ] ) > fabs ( scanaxis[ 1 ][ 0 ] )
  && fabs ( scanaxis[ 0 ][ 0 ] ) > fabs ( scanaxis[ 2 ][ 0 ] ) )
    XAxis   = ( scanaxis[ 0 ][ 1 ] + scanaxis[ 0 ][ 2 ] ) / 2.0 < thetaphisize / 2.0 ? ToDown : ToUp;

if ( fabs ( scanaxis[ 1 ][ 0 ] ) > fabs ( scanaxis[ 0 ][ 0 ] )
  && fabs ( scanaxis[ 1 ][ 0 ] ) > fabs ( scanaxis[ 2 ][ 0 ] ) )
    YAxis   = ( scanaxis[ 1 ][ 1 ] + scanaxis[ 1 ][ 2 ] ) / 2.0 < thetaphisize / 2.0 ? ToDown : ToUp;

if ( fabs ( scanaxis[ 2 ][ 0 ] ) > fabs ( scanaxis[ 0 ][ 0 ] )
  && fabs ( scanaxis[ 2 ][ 0 ] ) > fabs ( scanaxis[ 1 ][ 0 ] ) )
    ZAxis   = ( scanaxis[ 2 ][ 1 ] + scanaxis[ 2 ][ 2 ] ) / 2.0 < thetaphisize / 2.0 ? ToDown : ToUp;
*/


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
