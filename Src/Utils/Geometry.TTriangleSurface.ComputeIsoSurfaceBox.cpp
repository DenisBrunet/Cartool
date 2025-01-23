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

//#define     CHECKASSERT
#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Geometry.TTriangleSurface.h"

#include    "TVolume.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//------------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Just a cube around the data
void    TTriangleSurface::ComputeIsoSurfaceBox          (   const Volume*   data, 
                                                            TVertex**       listvert,   int         pointsperblock,     int&        currlistblock 
                                                        )
{
                    currlistblock   = 0;

listvert[ currlistblock ] = new TVertex [ pointsperblock ];

TVertex*            tovn            = &listvert[ currlistblock ][ 0 ];

float               x1              = 0;
float               x2              = data->GetDim1 () - 1;
float               y1              = 0;
float               y2              = data->GetDim2 () - 1;
float               z1              = 0;
float               z2              = data->GetDim3 () - 1;

                                        // !Does not test for a block being smaller than 6 * 2 * 3 = 36 vertices!

                                        // X-Y planes
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  = -1;   tovn++; NumPoints++;

tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = 0;    tovn->Normal.Z  =  1;   tovn++; NumPoints++;

                                        // X-Z planes
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  =  1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;

tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = 0;    tovn->Normal.Y  = -1;   tovn->Normal.Z  = 0;    tovn++; NumPoints++;

                                        // Y-Z planes
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x1;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  = -1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;

tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z1;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z1;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y1;   tovn->Vertex.Z  = z2;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;    tovn++; NumPoints++;
tovn->Vertex.X  = x2;   tovn->Vertex.Y  = y2;   tovn->Vertex.Z  = z2;   tovn->Normal.X  =  1;   tovn->Normal.Y  = 0;    tovn->Normal.Z  = 0;            NumPoints++;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
