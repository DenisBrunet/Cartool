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

//#define     CHECKASSERT
#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Geometry.TTriangleSurface.h"

#include    "MemUtil.h"

#include    "CartoolTypes.h"
#include    "Math.Stats.h"
#include    "Math.Histo.h"
//#include  "TCacheVolumes.h"

#include    "Geometry.TBoundingBox.h"
#include    "TVolume.h"

#include    "GlobalOptimize.Tracks.h"   // FunctionCenter

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void    TTriangleSurface::IsosurfaceFromVolume ( Volume& dataorig, const TTriangleSurfaceIsoParam& isoparam )
{
                                        // delete all existing content
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Persistent caches, so successive calls for the same volume
                                        // will use this cache, rather than reloading/recomputing
                                        // 9: 2 or 3 MRIs per LM, and 2 to 3 LM opened at once
//static TCacheVolumes<MriType> Caches ( 9 );

                                        // Allocated on the fly
Volume              data;
Volume*             todata          = &data;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

IsosurfaceMethod    how             = isoparam.How;
int                 downsampling    = isoparam.Downsampling < 1 ? 1 : isoparam.Downsampling;
double              isovalue        = isoparam.IsoValue; // + SingleFloatEpsilon;
TPointDouble        scale           = isoparam.Rescale;
TPointDouble        origin          = isoparam.Origin;
int                 margin          = isoparam.UseOriginalVolume ? 0 : TesselationVolumeMargin;


if ( how == UnknownIsosurface || dataorig.IsNotAllocated () )
    return;



if ( isoparam.UseOriginalVolume ) {     // the buffer volume is provided, use it but don't touch it

    todata  = &dataorig;

                                        // !caller granted us to use the original volume for filtering - hope this is not a mistake, because here it comes!
    if ( isoparam.SmoothData != FilterTypeNone ) {

        FctParams           p;
        p ( FilterParamDiameter )   = 4;
        todata->Filter ( isoparam.SmoothData, p );
        }
    }

else {                                  // use or create a cache

                                        // if not exact multiple, add 2 points for the spread of errors when downsampling
    int                 dim1            = dataorig.GetDim1() / downsampling + ( dataorig.GetDim1() % downsampling ? 2 : 0 ) + 2 * margin;
    int                 dim2            = dataorig.GetDim2() / downsampling + ( dataorig.GetDim2() % downsampling ? 2 : 0 ) + 2 * margin;
    int                 dim3            = dataorig.GetDim3() / downsampling + ( dataorig.GetDim3() % downsampling ? 2 : 0 ) + 2 * margin;
    int                 insertorigin[ 3 ];
    insertorigin[ 0 ] = insertorigin[ 1 ] = insertorigin[ 2 ] = margin;


    data.Resize ( dim1, dim2, dim3 );

    data.Insert ( dataorig, insertorigin, 1, downsampling );
                                        // optionally filtering (filtering can greatly reduce the number of triangles, up to -30%)
    if ( isoparam.SmoothData != FilterTypeNone ) {

        FctParams           p;
        p ( FilterParamDiameter )   = 4;
        data.Filter ( isoparam.SmoothData, p );
        }

/*
                                        // getting the next available cache slot
    bool                reload;

    todata    = Caches.GetCache ( &dataorig, dim1, dim2, dim3, reload );

                                        // should reload the data?
    if ( reload ) {
                                        // reset volume
        todata->ResetMemory ();
                                        // insert the original data
        todata->Insert ( dataorig, insertorigin, 1, downsampling );
                                        // optionally filtering (filtering can greatly reduce the number of triangles, up to -30%)
        if ( isoparam.SmoothData ) {
            FctParams           p;
            p ( FilterParamDiameter )     = 4;
            todata->Filter ( FilterTypeFastGaussian, p );
            }
        }
*/
    } // local copy


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocating space to store the produced triangles

int                 VerticePerBlock     = TruncateTo ( MaxVerticePerBlock, VerticeGranularity );
TVertex*            listvert[ MaxBlocksOfVertice ];
int                 currlistblock       = 0;


if      ( how == IsosurfaceMarchingCube )

    ComputeIsoSurfaceMarchingCube   (   todata, 
                                        isovalue,   isoparam.SmoothGradient, 
                                        listvert,   VerticePerBlock, currlistblock 
                                    );

else if ( how == IsosurfaceMinecraft )
                                        // set iso cut to 1 more, as to be intuitive (cutting above the threshold)
    ComputeIsoSurfaceMinecraft      (   todata, 
                                        isovalue,                          
                                        listvert,   VerticePerBlock, currlistblock 
                                    );

else if ( how == IsosurfaceBox )
                                        // isosurface 0 bypasses all cases, generating a big cube around data
    ComputeIsoSurfaceBox            (   todata,
                                        listvert,   36,              currlistblock 
                                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy to the inner data structure
                                        // copy the working list to the final list of triangles
ListTriangles    .Resize ( NumPoints );
ListPointsNormals.Resize ( NumPoints );

                                        // fast means: no rescalings, just shifts
bool                fastcopy        = downsampling == 1 && scale == 1;
int                 sizeofnormals   = sizeof ( listvert[ 0 ][ 0 ].Normal );


if ( fastcopy ) {

    float               xmax            = dataorig.GetDim1 () - 1;
    float               ymax            = dataorig.GetDim2 () - 1;
    float               zmax            = dataorig.GetDim3 () - 1;


    for ( int i = 0, j = 0, jb, ji; i < NumPoints; i++, j++ ) {

        jb  = j / VerticePerBlock;
        ji  = j % VerticePerBlock;

                                        // clip to original limits (to avoid texture leakages), then shift
        ListTriangles[ i ].X    = Clip ( listvert[ jb ][ ji ].Vertex.X - margin, (float) 0, xmax ) - origin.X;
        ListTriangles[ i ].Y    = Clip ( listvert[ jb ][ ji ].Vertex.Y - margin, (float) 0, ymax ) - origin.Y;
        ListTriangles[ i ].Z    = Clip ( listvert[ jb ][ ji ].Vertex.Z - margin, (float) 0, zmax ) - origin.Z;

    //  ListPointsNormals[ i ].X= listvert[ j ].Normal.X;
    //  ListPointsNormals[ i ].Y= listvert[ j ].Normal.Y;
    //  ListPointsNormals[ i ].Z= listvert[ j ].Normal.Z;
        CopyVirtualMemory ( &ListPointsNormals[ i ], &listvert[ jb ][ ji ].Normal, sizeofnormals );
        }
    }
else {                                  // slower method

    int                 firstinx;
    int                 firstiny;
    int                 firstinz;

    dataorig.DownsamplingOffset ( downsampling, firstinx, firstiny, firstinz );
                                        // compensate the integer shift introduced while downsampling
                                        // and the round-off correction done
    double              downshiftx      = (double) ( downsampling - 1 ) / 2 + firstinx;
    double              downshifty      = (double) ( downsampling - 1 ) / 2 + firstiny;
    double              downshiftz      = (double) ( downsampling - 1 ) / 2 + firstinz;


    for ( int i = 0, j = 0, jb, ji; i < NumPoints; i++, j++ ) {

        jb  = j / VerticePerBlock;
        ji  = j % VerticePerBlock;
                                        // transfer, rescale and shift - currently NOT clipping against limits
        ListTriangles[ i ].X    = ( ( listvert[ jb ][ ji ].Vertex.X - margin ) * downsampling + downshiftx ) * scale.X - origin.X;
        ListTriangles[ i ].Y    = ( ( listvert[ jb ][ ji ].Vertex.Y - margin ) * downsampling + downshifty ) * scale.Y - origin.Y;
        ListTriangles[ i ].Z    = ( ( listvert[ jb ][ ji ].Vertex.Z - margin ) * downsampling + downshiftz ) * scale.Z - origin.Z;

                                        // 1 OK on MRI, not OK on inverse
//      ListTriangles[ i ].X    = Clip ( (float) ( ( ( listvert[ jb ][ ji ].Vertex.X - margin ) * downsampling + downshiftx ) * scale.X ), (float) 0, xmax ) - origin.X;
//      ListTriangles[ i ].Y    = Clip ( (float) ( ( ( listvert[ jb ][ ji ].Vertex.Y - margin ) * downsampling + downshifty ) * scale.Y ), (float) 0, ymax ) - origin.Y;
//      ListTriangles[ i ].Z    = Clip ( (float) ( ( ( listvert[ jb ][ ji ].Vertex.Z - margin ) * downsampling + downshiftz ) * scale.Z ), (float) 0, zmax ) - origin.Z;
                                        // 2 OK on inverse, not OK on MRI
//      ListTriangles[ i ].X    = ( Clip ( listvert[ jb ][ ji ].Vertex.X - margin, (float) 0, xmax ) * downsampling + downshiftx ) * scale.X - origin.X;
//      ListTriangles[ i ].Y    = ( Clip ( listvert[ jb ][ ji ].Vertex.Y - margin, (float) 0, ymax ) * downsampling + downshifty ) * scale.Y - origin.Y;
//      ListTriangles[ i ].Z    = ( Clip ( listvert[ jb ][ ji ].Vertex.Z - margin, (float) 0, zmax ) * downsampling + downshiftz ) * scale.Z - origin.Z;

                                        // should be recalculated?
                                        // !!note: if scale is anisotropic, the normals are wrong
        CopyVirtualMemory ( &ListPointsNormals[ i ], &listvert[ jb ][ ji ].Normal, sizeofnormals );
        }

    } // ! fastcopy


NumTriangles = NumPoints / 3;


for ( ; currlistblock >= 0; currlistblock-- )
    delete[]    listvert[currlistblock];
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
