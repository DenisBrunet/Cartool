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

#pragma once

#include    <queue>

#include    "Geometry.TPoint.h"
#include    "TSelection.h"
#include    "TArray1.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Delaunay 3D triangulation: making use of some extra precision locally
using                   D3Dfloat            = double;   // was  long double, but it does not seem necessary anymore
using                   D3DPoint            = TPointT <D3Dfloat>;
using                   D3DPointI           = TPointTI<D3Dfloat>;
using                   D3DVector           = TPointT <D3Dfloat>;

class                   TTriangle;


class   TEdge
{
public:
                        TEdge ()                                                { Vertex1 = 0; Vertex2 = 0; Triangle1 = 0; Triangle2 = 0; }

    const D3DPointI*    Vertex1;    // pointers to vertices of the edge
    const D3DPointI*    Vertex2;

    TTriangle*          Triangle1;  // pointers to triangles to which this edge might belong
    TTriangle*          Triangle2;

    bool                HasVertex               ( const D3DPointI* v )  const   { return  v && ( Vertex1 == v || Vertex2 == v );    }   // testing pointers
    bool                Has0Triangles           ()                      const   { return  (bool) Triangle1 + (bool) Triangle2 == 0; }
    bool                Has1Triangle            ()                      const   { return  (bool) Triangle1 + (bool) Triangle2 == 1; }
    bool                Has2Triangles           ()                      const   { return  (bool) Triangle1 + (bool) Triangle2 == 2; }
    const D3DPointI*    GetVertexNotBelonging   ( const D3DPointI* v1, 
                                                  const D3DPointI* v2, 
                                                  const D3DPointI* v3 ) const   { return  ! HasVertex ( v1 ) ? v1 : ! HasVertex ( v2 ) ? v2 : ! HasVertex ( v3 ) ? v3 : 0; }
};


//----------------------------------------------------------------------------
class   TTriangle
{
public:
                        TTriangle ()                                            { Edge1 = 0; Edge2 = 0; Edge3 = 0; }

    TEdge*              Edge1;      // pointers to edges which define the triangle
    TEdge*              Edge2;
    TEdge*              Edge3;
    
    bool                HasVertex           ( const D3DPointI* v )      const   { return  v && ( Edge1->HasVertex ( v ) || Edge2->HasVertex ( v ) ); }  // no need to test Edge3, which shares the same vertices as in the other two
};


//----------------------------------------------------------------------------
                                        // The complete structure for triangle tesselation
class   TTriangleNetwork
{
public:
                        TTriangleNetwork    ( const TArray1<D3DPointI>& points );
                       ~TTriangleNetwork    ();


    int                 GetMaxVertices      ()  const       { return MaxVertices;           }
    int                 GetMaxEdges         ()  const       { return MaxEdges;              }
    int                 GetMaxTriangles     ()  const       { return MaxTriangles;          }
    int                 GetNumVertices      ()  const       { return VerticesIn.NumSet ();  }
    int                 GetNumEdges         ()  const       { return NumEdges;              }
    int                 GetNumTriangles     ()  const       { return NumTriangles;          }

    bool                HasVertex           ( const D3DPointI& a )      const   { return VerticesIn.IsSelected ( a.Index ); }
    bool                CanConnect          ( const TEdge* e, const D3DPointI* v )   const;

    TEdge*              GetEdge             ( const D3DPointI* v1, const D3DPointI* v2 );
    TTriangle*          GetTriangle         ( const TEdge* e, const D3DPointI& v );
    void                GetTriangleVertices ( int ti, const D3DPointI* v[ 3 ] ) const;
    void                AddBorder           ( TEdge* e );
    TEdge*              GetNextBorder       ();

    void                InitEdge            ( const D3DPointI& a, const D3DPointI& b );
    void                AddTriangle         ( const D3DPointI& a, const D3DPointI& b, const D3DPointI& c );
    void                FillHoles           ();


private:

    int                 MaxVertices;
    int                 MaxEdges;
    int                 MaxTriangles;
    int                 NumEdges;
    int                 NumTriangles;

    const TArray1<D3DPointI>&   Vertices;   // const reference to some original list of points - we don't need to actually copy these
    TSelection                  VerticesIn; // vertices are "inserted" one at a time during tesselation
    TArray1<TEdge>              Edges;      // !should not be relocated during processing, due to the use of pointers!
    TArray1<TTriangle>          Triangles;  // same
    std::queue<TEdge*>          Borders;    // current list of edges that are parts of the border, and from which the tesselation grows


    const D3DPointI*    InsertVertex        ( const D3DPointI& a );
    TEdge*              InsertEdge          ( const D3DPointI* v1, const D3DPointI* v2 );
    TTriangle*          InsertTriangle      ( TEdge* e1, TEdge* e2, TEdge* e3 );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
