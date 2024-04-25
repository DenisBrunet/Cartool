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

#include    "CartoolTypes.h"

#include    "TVolume.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void    TTriangleSurface::ComputeIsoSurfaceMinecraft    (   const Volume*   data, 
                                                            double          isovalue, 
                                                            TVertex**       listvert,   int         pointsperblock,     int&        currlistblock 
                                                        )
{

                    currlistblock   = 0;
int                 currlistindex   = 0;

listvert[ currlistblock ] = new TVertex [ pointsperblock ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optimizing access from 1 voxel to a few of its 7 meighbors by computing their linear memory distances
int                 to8neigh[ 5 /*8*/ ];

to8neigh[ 0 ]   = 0;
to8neigh[ 1 ]   = data->IndexesToLinearIndex ( 1, 0, 0 ) - data->IndexesToLinearIndex ( 0, 0, 0 );
to8neigh[ 3 ]   = data->IndexesToLinearIndex ( 0, 1, 0 ) - data->IndexesToLinearIndex ( 0, 0, 0 );
to8neigh[ 4 ]   = data->IndexesToLinearIndex ( 0, 0, 1 ) - data->IndexesToLinearIndex ( 0, 0, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parallel optimization is working fine - Problem is triangles will appear all mixed up in transparency, so until output triangles are properly sorted, it is turned off
//OmpParallelFor

for ( int xi = 0; xi < data->GetDim1() - 1; xi++ ) {

    double              x               = xi - 0.5;

    for ( int yi = 0; yi < data->GetDim2() - 1; yi++ ) {

        double              y               = yi - 0.5;

        for ( int zi = 0; zi < data->GetDim3() - 1; zi++ ) {

            double              z               = zi - 0.5;
                                        // Getting the 4 voxels' values
            const MriType*      tovoxel         = & (*data) ( xi, yi, zi );

            bool                v0              = *  tovoxel                   >= isovalue;
            bool                v1              = *( tovoxel + to8neigh[ 1 ] ) >= isovalue;
            bool                v3              = *( tovoxel + to8neigh[ 3 ] ) >= isovalue;
            bool                v4              = *( tovoxel + to8neigh[ 4 ] ) >= isovalue;

            TVector3Double      normal;


            if ( v0 ^ v1 ) {

                normal.X    = v0 ? 1 : -1;
                normal.Y    =
                normal.Z    = 0;
                normal.Normalize ();
                                        // influence the shading through the norm of the normal, to give some depth cue
                                        // weight by # of neighbors
                #define     shadecoeff      ( 0.333 / 26.0 )

                auto    AdjustNormalX = [ data, isovalue, v0, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.X   *= 1 - data->GetNumNeighbors ( xi + ( v0 ? 2 : -1 ), yi, zi, Neighbors26 ) * shadecoeff;
                };

                AdjustNormalX ( normal );
                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v0 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v0 ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v0 ) tovn += 2; else tovn++;



                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v0 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v0 ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;


                                        // end of current block?
                if ( currlistindex >= pointsperblock ) {

                    currlistindex = 0;  // create new block

                    currlistblock++;
                                        // no more!
                    if ( currlistblock >= MaxBlocksOfVertice ) {
                        ShowMessage ( "You reached the maximum number of triangles!", "Surface Triangulation", ShowMessageWarning );
        //              return;     // not allowed anymore with OpenMP
                        exit ( 1 );
                        }

                    listvert[ currlistblock ] = new TVertex [ pointsperblock ];
                    }

//                OmpCriticalEnd

                } // case x


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if ( v0 ^ v3 ) {

                normal.Y    = v0 ? 1 : -1;
                normal.X    =
                normal.Z    = 0;
                normal.Normalize ();

                auto    AdjustNormalY = [ data, isovalue, v0, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.Y   *= 1 - data->GetNumNeighbors ( xi, yi + ( v0 ? 2 : -1 ), zi, Neighbors26 ) * shadecoeff;
                };

                AdjustNormalY ( normal );

                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v3 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v3 ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v3 ) tovn += 2; else tovn++;



                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v3 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v3 ) tovn--; else tovn++;


                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;


                                        // end of current block?
                if ( currlistindex >= pointsperblock ) {

                    currlistindex = 0;  // create new block

                    currlistblock++;
                                        // no more!
                    if ( currlistblock >= MaxBlocksOfVertice ) {
                        ShowMessage ( "You reached the maximum number of triangles!", "Surface Triangulation", ShowMessageWarning );
        //              return;     // not allowed anymore with OpenMP
                        exit ( 1 );
                        }

                    listvert[ currlistblock ] = new TVertex [ pointsperblock ];
                    }

//                OmpCriticalEnd

                } // case y


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if ( v0 ^ v4 ) {

                normal.Z    = v0 ? 1 : -1;
                normal.X    =
                normal.Y    = 0;
                normal.Normalize ();

                auto    AdjustNormalZ = [ data, isovalue, v0, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.Z   *= 1 - data->GetNumNeighbors ( xi, yi, zi + ( v0 ? 2 : -1 ), Neighbors26 ) * shadecoeff;
                };

                AdjustNormalZ ( normal );

                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x;           tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v4 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v4 ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v4 ) tovn += 2; else tovn++;



                tovn->Vertex.X = x;           tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v4 ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( v4 ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;


                                        // end of current block?
                if ( currlistindex >= pointsperblock ) {

                    currlistindex = 0;  // create new block

                    currlistblock++;
                                        // no more!
                    if ( currlistblock >= MaxBlocksOfVertice ) {
                        ShowMessage ( "You reached the maximum number of triangles!", "Surface Triangulation", ShowMessageWarning );
        //              return;     // not allowed anymore with OpenMP
                        exit ( 1 );
                        }

                    listvert[ currlistblock ] = new TVertex [ pointsperblock ];
                    }

//                OmpCriticalEnd

                } // case z

            } // for z
        } // for y
    } // for x

} // ComputeIsoSurfaceMinecraft


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
