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
                                        // Parallel optimization is working fine - Problem is triangles will appear all mixed up in transparency, so until output triangles are properly sorted, it is turned off
//OmpParallelFor

for ( int xi = -1; xi < data->GetDim1(); xi++ ) {

    double              x               = xi - 0.5;

    for ( int yi = -1; yi < data->GetDim2(); yi++ ) {

        double              y               = yi - 0.5;

        for ( int zi = -1; zi < data->GetDim3(); zi++ ) {

            double              z               = zi - 0.5;

                                        // Testing the central voxel + 3 neighbors' values
            bool                isvoxel         = data->GetValueChecked ( xi,     yi,     zi     ) >= isovalue;
            bool                isnextx         = data->GetValueChecked ( xi + 1, yi,     zi     ) >= isovalue;
            bool                isnexty         = data->GetValueChecked ( xi,     yi + 1, zi     ) >= isovalue;
            bool                isnextz         = data->GetValueChecked ( xi,     yi,     zi + 1 ) >= isovalue;

            TVector3Double      normal;


            if ( isvoxel ^ isnextx ) {

                normal.X    = isvoxel ? 1 : -1;
                normal.Y    =
                normal.Z    = 0;
                normal.Normalize ();
                                        // influence the shading through the norm of the normal, to give some depth cue
                                        // weight by # of neighbors
                #define     shadecoeff      ( 0.333 / 26.0 )

                auto    AdjustNormalX = [ data, isovalue, isvoxel, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.X   *= 1 - data->GetNumNeighbors ( xi + ( isvoxel ? 2 : -1 ), yi, zi, Neighbors26 ) * shadecoeff;
                };

                AdjustNormalX ( normal );
                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isvoxel ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isvoxel ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isvoxel ) tovn += 2; else tovn++;



                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y;           tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isvoxel ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isvoxel ) tovn--; else tovn++;


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

            if ( isvoxel ^ isnexty ) {

                normal.Y    = isvoxel ? 1 : -1;
                normal.X    =
                normal.Z    = 0;
                normal.Normalize ();

                auto    AdjustNormalY = [ data, isovalue, isvoxel, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.Y   *= 1 - data->GetNumNeighbors ( xi, yi + ( isvoxel ? 2 : -1 ), zi, Neighbors26 ) * shadecoeff;
                };

                AdjustNormalY ( normal );

                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnexty ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnexty ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnexty ) tovn += 2; else tovn++;



                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnexty ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnexty ) tovn--; else tovn++;


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

            if ( isvoxel ^ isnextz ) {

                normal.Z    = isvoxel ? 1 : -1;
                normal.X    =
                normal.Y    = 0;
                normal.Normalize ();

                auto    AdjustNormalZ = [ data, isovalue, isvoxel, xi, yi, zi ] ( TVector3Double& normal ) {
                normal.Z   *= 1 - data->GetNumNeighbors ( xi, yi, zi + ( isvoxel ? 2 : -1 ), Neighbors26 ) * shadecoeff;
                };

                AdjustNormalZ ( normal );

                                        // Insertion is critical, as we update all the pointers from the same structure
//                OmpCriticalBegin (TTriangleSurface)

                TVertex*            tovn            = &listvert[ currlistblock ][ currlistindex ];


                tovn->Vertex.X = x;           tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnextz ) tovn += 2; else tovn++;


                tovn->Vertex.X = x;           tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnextz ) tovn--; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnextz ) tovn += 2; else tovn++;



                tovn->Vertex.X = x;           tovn->Vertex.Y = y;           tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnextz ) tovn += 2; else tovn++;


                tovn->Vertex.X = x + 1;       tovn->Vertex.Y = y + 1;       tovn->Vertex.Z = z + 1;
                tovn->Normal.X = normal.X;    tovn->Normal.Y = normal.Y;    tovn->Normal.Z = normal.Z;
                currlistindex++; NumPoints++;
                if ( isnextz ) tovn--; else tovn++;


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
