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

#include    "Geometry.TTriangleNetwork.h"

#include    "Math.Utils.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
        TTriangleNetwork::TTriangleNetwork ( const TArray1<D3DPointI>& points )
        : Vertices ( points )           // !a const reference to list of points!
{
MaxVertices         = AtLeast ( 0, (int) points );
                                        // simpler formulas
MaxEdges            = 3 * MaxVertices;
MaxTriangles        = 2 * MaxVertices;
                                        // exact formulas if the network makes a convex hull around all points
//MaxEdges            = MaxVertices <  2 ? 0
//                    : MaxVertices == 2 ? 1
//                    : MaxVertices == 3 ? 3
//                    :                    3 * ( MaxVertices - 2 );
//MaxTriangles        = MaxVertices <  3 ? 0
//                    : MaxVertices == 3 ? 1
//                    :                    2 * ( MaxVertices - 2 );

                                        // !indexes refer to some original points, NOT the one in Vertices, and range can therefor be different!
int                 maxindexes      = TIndexNull;
for ( int i = 0; i < (int) Vertices; i++ )
    Maxed ( maxindexes, Vertices[ i ].Index );
                                        // allocate on original indexes
VerticesIn          = TSelection ( maxindexes + 1, OrderSorted );


NumEdges            = 0;
NumTriangles        = 0;
                                        // we can allocate in advance with these limits
                                        // !we use pointers all along, so objects should not be moved around like std::vector could do!
Edges    .Resize ( MaxEdges );
Triangles.Resize ( MaxTriangles );
}


        TTriangleNetwork::~TTriangleNetwork ()
{
#if defined(CHECKASSERT)
//DBGV2 ( GetNumVertices (),  MaxVertices, "NumVertices, MaxVertices" );
//DBGV2 ( NumEdges,     MaxEdges, "NumEdges, MaxEdges" );
//DBGV2 ( NumTriangles, MaxTriangles, "NumTriangles, MaxTriangles" );
#endif
}


//------------------------------------------------------------------------------
                                        // 2 vertices -> any existing existing edge
TEdge*      TTriangleNetwork::GetEdge ( const D3DPointI* v1, const D3DPointI* v2 )
{
for ( int i = 0; i < NumEdges; i++ )

    if ( Edges[ i ].Vertex1 == v1 && Edges[ i ].Vertex2 == v2
      || Edges[ i ].Vertex1 == v2 && Edges[ i ].Vertex2 == v1 )

        return &Edges[ i ];

return  0;
}

                                        // edge + vertex -> any existing existing triangle
TTriangle*  TTriangleNetwork::GetTriangle ( const TEdge* e, const D3DPointI& v )
{
                                        // vertex was not inserted -> no triangles can contain it either
if ( ! HasVertex ( v ) )    
    return  0;

                                        // check the 2 connected triangles
if      ( e->Triangle1 != 0 && e->Triangle1->HasVertex ( &v ) )     return e->Triangle1;
else if ( e->Triangle2 != 0 && e->Triangle2->HasVertex ( &v ) )     return e->Triangle2;
else                                                                return 0;
}

                                        // triangle index -> 3 vertices
void        TTriangleNetwork::GetTriangleVertices ( int ti, const D3DPointI* v[ 3 ] )   const
{
const TTriangle&    t               = Triangles[ ti ];

v[ 0 ]  = t.Edge1->Vertex1;
v[ 1 ]  = t.Edge1->Vertex2;
                                        // copy the vertex which is NOT already referenced
v[ 2 ]  = v[ 0 ] != t.Edge2->Vertex1 
       && v[ 1 ] != t.Edge2->Vertex1 ? t.Edge2->Vertex1 
                                     : t.Edge2->Vertex2;
}


void        TTriangleNetwork::AddBorder ( TEdge* e )
{
if ( ! e->Has2Triangles () )            // don't even bother inserting edges that can not grow anymore
    Borders.push ( e );
}

                                        // used when building the tesselation
TEdge*      TTriangleNetwork::GetNextBorder ()
{
if ( Borders.empty () )     return 0;
                                        // getting top edge
TEdge*              nextedge        = Borders.front ();
                                        // then remove it from the list
Borders.pop ();

return  nextedge;
}


//------------------------------------------------------------------------------
                                        // Test wether a vertex can be connected to an edge
bool    TTriangleNetwork::CanConnect ( const TEdge* e, const D3DPointI* v )    const
{
                                        // testing non existent things...
if ( e == 0 || v == 0 )         return  true;
                                        // no triangles connected, no problems
if ( e->Has0Triangles () )      return  true;
                                        // triangles already all connected, reject
if ( e->Has2Triangles () )      return  false;

                                        // here, only Triangle1 exists
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the 3 vertices from Triangle1
const D3DPointI*    vertices[ 3 ];
                   // !retrieving triangle index!
GetTriangleVertices ( e->Triangle1 - &Triangles[ 0 ], vertices );

                                        // should not happen, but will mess the procedure, so...
if ( vertices[ 0 ] == v || vertices[ 1 ] == v || vertices[ 2 ] == v )
    return  true;                       // triangle is "ok", as it already exists


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test if candidate vertex is on the other side of existing triangle
                                        // !pay attention to the ordering of the vertices!
D3DPoint            v0              = *e->Vertex1;
D3DVector           v1              = *e->Vertex2;
D3DVector           v2              = *e->GetVertexNotBelonging ( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ] );
D3DVector           vc              = *v;

v1 -= v0;                               // vector 0-1
v2 -= v0;                               // vector 0-2
vc -= v0;                               // vector 0-candidate

v1.Normalize ();
                                        // equivalent and simpler: true if on the other side of the plane orthogonal to triangle
return  vc.IsOppositeDirection ( v2 - v1 * v1.ScalarProduct ( v2 ) );


//v2.Normalize ();
//vc.Normalize ();
//D3DVector           n12             = v1.VectorialProduct ( v2 );   // normal v1-v2
//D3DVector           n1c             = v1.VectorialProduct ( vc );   // normal v1-vc
//                                        // opposite normals means vertex is on the other side of Triangle1 - what we desire
//return  n12.IsOppositeDirection ( n1c );
}


//------------------------------------------------------------------------------
const D3DPointI*    TTriangleNetwork::InsertVertex ( const D3DPointI& a )
{
VerticesIn.Set ( a.Index );

return  &a;
}


//------------------------------------------------------------------------------
TEdge*      TTriangleNetwork::InsertEdge ( const D3DPointI* v1, const D3DPointI* v2 )
{
#if defined(CHECKASSERT)
assert ( NumEdges < MaxEdges );
#endif

                                        // getting next available slot
TEdge&              e               = Edges[ NumEdges ];

                    e.Vertex1       = v1;
                    e.Vertex2       = v2;
                    e.Triangle1     = 0;
                    e.Triangle2     = 0;
                                        // each new inserted edge becomes part of the border for further search
AddBorder ( &e );
                                        // bumping current count
return  &Edges[ NumEdges++ ];
}


//------------------------------------------------------------------------------
TTriangle*  TTriangleNetwork::InsertTriangle ( TEdge* e1, TEdge* e2, TEdge* e3 )
{
#if defined(CHECKASSERT)
assert ( NumTriangles < MaxTriangles );
#endif

                                        // getting next available slot
TTriangle&          t               = Triangles[ NumTriangles ];

                                        // edges pointing to next triangle
if      ( ! e1->Triangle1 )     e1->Triangle1   = &t;   // !cautious which triangle to update!
else if ( ! e1->Triangle2 )     e1->Triangle2   = &t;

if      ( ! e2->Triangle1 )     e2->Triangle1   = &t;
else if ( ! e2->Triangle2 )     e2->Triangle2   = &t;

if      ( ! e3->Triangle1 )     e3->Triangle1   = &t;
else if ( ! e3->Triangle2 )     e3->Triangle2   = &t;

                                        // next triangle pointing to edges
                                t.Edge1         = e1;
                                t.Edge2         = e2;
                                t.Edge3         = e3;

                                        // bumping current count
return  &Triangles[ NumTriangles++ ];
}


//------------------------------------------------------------------------------
                                        // Triangulation should be empty here
void    TTriangleNetwork::InitEdge     ( const D3DPointI& a, const D3DPointI& b )
{
const D3DPointI*    v1              = InsertVertex ( a );
const D3DPointI*    v2              = InsertVertex ( b );

InsertEdge ( v1, v2 );
}


//------------------------------------------------------------------------------
                                        // There is a lot going on here, as vertices may or may not exist, and/or edges may or may not exist
void    TTriangleNetwork::AddTriangle ( const D3DPointI& a, const D3DPointI& b, const D3DPointI& c )
{
                                        // test for non-degenerated triangle
if ( a == b || a == c || b == c )
    return;

                                        // count number of existing vertices
int                 numexistingvertices = HasVertex ( a ) + HasVertex ( b ) + HasVertex ( c );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no vertices exist?
if ( numexistingvertices == 0 || numexistingvertices == 1 ) {
                                        // insert the 2 or 3 missing vertices
    const D3DPointI*    v1              = HasVertex ( a ) ? &a : InsertVertex ( a );
    const D3DPointI*    v2              = HasVertex ( b ) ? &b : InsertVertex ( b );
    const D3DPointI*    v3              = HasVertex ( c ) ? &c : InsertVertex ( c );

                                        // insert the 3 edges, e1 < e2 < e3
    TEdge*              e1              = InsertEdge ( v1, v2 );
    TEdge*              e2              = InsertEdge ( v1, v3 );
    TEdge*              e3              = InsertEdge ( v2, v3 );

                                        // insert new triangle
    InsertTriangle  ( e1, e2, e3 );

    return;
    } // 0 or 1 vertices


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // from here, 2 or 3 already existing vertices -> insert 0 or 1 vertex
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const D3DPointI*    v1              = HasVertex ( a ) ? &a : 0;
const D3DPointI*    v2              = HasVertex ( b ) ? &b : 0;
const D3DPointI*    v3              = HasVertex ( c ) ? &c : 0;

                                        // testing numexistingvertices == 3 is not correct, 3 points can exist, but may not be connected, or only partially, so we have to work with edges from now on

                                        // find any existing common edge(s) (1 to 3)
TEdge*              e1              = GetEdge ( v1, v2 );
TEdge*              e2              = GetEdge ( v1, v3 );
TEdge*              e3              = GetEdge ( v2, v3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // count number of existing edges
int                 numexistingedges    = (bool) e1 + (bool) e2 + (bool) e3;

                                        // here, we can insert any needed vertex
if ( ! v1 )         v1              = InsertVertex ( a );
if ( ! v2 )         v2              = InsertVertex ( b );
if ( ! v3 )         v3              = InsertVertex ( c );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test coplanar, or overlapping triangle
if ( e1 && ! CanConnect ( e1, e1->GetVertexNotBelonging ( v1, v2, v3 ) ) )  return;

if ( e2 && ! CanConnect ( e2, e2->GetVertexNotBelonging ( v1, v2, v3 ) ) )  return;

if ( e3 && ! CanConnect ( e3, e3->GetVertexNotBelonging ( v1, v2, v3 ) ) )  return;

                                        // these edges are not borders anymore
                                        // note that it might lead to some inconsistencies in some rare cases where the tesselation closes itself around a still empty triangle - fixed as post-processing in FillHoles
//if ( e1 )   RemoveBorders ( e1 );
//if ( e2 )   RemoveBorders ( e2 );
//if ( e3 )   RemoveBorders ( e3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // see which & how many edge(s) to insert
                                        // any inserted edge is part of the border
if      ( numexistingedges == 3 )
    ;                                   // no new edge to insert

else if ( numexistingedges == 2 ) {
                                        // references to which edge(s) eXist and which edge(s) to Insert (1:edge exists, 0:edge missing):
                                        //                          e1:  1 1 0  ->  x1 x1 e1
                                        //                          e2:  1 0 1  ->  x2 ei x1
                                        //                          e3:  0 1 1  ->  ei x2 x2
    TEdge*&             x1              = !e3 ? e1 : !e2 ? e1 : e2;
    TEdge*&             x2              = !e3 ? e2 : !e2 ? e3 : e3;
    TEdge*&             ei              = !e3 ? e3 : !e2 ? e2 : e1;

                                        // 1 edge to insert
    if      ( x1->Vertex1 == x2->Vertex1 )  ei  = InsertEdge ( x1->Vertex2, x2->Vertex2 );
    else if ( x1->Vertex1 == x2->Vertex2 )  ei  = InsertEdge ( x1->Vertex2, x2->Vertex1 );
    else if ( x1->Vertex2 == x2->Vertex1 )  ei  = InsertEdge ( x1->Vertex1, x2->Vertex2 );
    else if ( x1->Vertex2 == x2->Vertex2 )  ei  = InsertEdge ( x1->Vertex1, x2->Vertex1 );
    }

else if ( numexistingedges == 1 ) {
                                        // references to which edge(s) eXist and which edge(s) to Insert
                                        //                          e1:  1 0 0  ->  x    ei1  ei1
                                        //                          e2:  0 1 0  ->  ei1  x    ei2
                                        //                          e3:  0 0 1  ->  ei2  ei2  x 
    TEdge*&             x               = e1 ? e1 : e2 ? e2 : e3;
    TEdge*&             ei1             = e1 ? e2 : e2 ? e1 : e1;
    TEdge*&             ei2             = e1 ? e3 : e2 ? e3 : e2;

                                        // 2 edges to insert
    if      ( ! x->HasVertex ( v1 ) )   {   ei1 = InsertEdge ( v1, x->Vertex1 );    ei2 = InsertEdge ( v1, x->Vertex2 ); }
    else if ( ! x->HasVertex ( v2 ) )   {   ei1 = InsertEdge ( v2, x->Vertex1 );    ei2 = InsertEdge ( v2, x->Vertex2 ); }
    else                                {   ei1 = InsertEdge ( v3, x->Vertex1 );    ei2 = InsertEdge ( v3, x->Vertex2 ); }
    }

else if ( numexistingedges == 0 ) {
                                        // 3 edges to insert - straightforward
                                            e1 = InsertEdge ( v1, v3 );
                                            e2 = InsertEdge ( v1, v2 );
                                            e3 = InsertEdge ( v2, v3 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // insert new triangle
InsertTriangle  ( e1, e2, e3 );
}


//------------------------------------------------------------------------------
                                        // After the main scan is done, it might be necessary to check and fill some specific holes in the tesselation
                                        // Holes can be either missing single triangles, or missing pairs of triangles arranged as circumcircular quadrangle
                                        // It still goes fast because most of the edges should be properly connected with 2 triangles
void    TTriangleNetwork::FillHoles ()
{
                                        // does it really make sense with so few points?
if ( (int) Vertices <= 5 )
    return;


const D3DPointI*    v12;
const D3DPointI*    v13;
const D3DPointI*    v14;
const D3DPointI*    v23;
const D3DPointI*    v24;
const D3DPointI*    v34;
TEdge*              e1;
TEdge*              e2;
TEdge*              e3;
TEdge*              e4;
TEdge*              e5;
int                 i;
int                 j;
int                 k;
int                 l;


auto                GetCommonVertex = [] ( const TEdge* e1, const TEdge* e2 )
{
if      ( e1->Vertex1 == e2->Vertex1 || e1->Vertex1 == e2->Vertex2 )    return  e1->Vertex1;
else if ( e1->Vertex2 == e2->Vertex1 || e1->Vertex2 == e2->Vertex2 )    return  e1->Vertex2;
else                                                                    return  (const D3DPointI*) 0;
};


                                        // Search for edge 1 (e1)
for ( i = 0, e1 = &Edges[ i ]; i < NumEdges; i++, e1++ ) {
                                        // skip edges with 0 or 2 attached triangles
    if ( ! e1->Has1Triangle () )
        continue;

                                        // Search for edge 2 (e2)
    for ( j = i + 1, e2 = &Edges[ j ]; j < NumEdges; j++, e2++ ) {

        if ( ! e2->Has1Triangle () )
            continue;

        if ( ! ( v12  = GetCommonVertex ( e1, e2 ) ) )  // no common vertex e1-e2?
            continue;

                                        // Search for edge 3 (e3)
        for ( k = j + 1, e3 = &Edges[ k ]; k < NumEdges; k++, e3++ ) {

            if ( ! e3->Has1Triangle () )
                continue;

                                        // get the 2 possible connections: e1-e3 and e2-e3
            v13  = GetCommonVertex ( e1, e3 );
            v23  = GetCommonVertex ( e2, e3 );

            if (   v13 == 0             // no connections at all
                && v23 == 0 
                || v13 == v12 
                || v23 == v12  
                || v13 == v23 )
                continue;


            // looking for a single missing triangle:
                                        // any common vertex edge 2 & edge 3?
//          if ( ( v23  = GetCommonVertex ( e2, e3 ) )
//              && v12 != v23 
//              && v13 != v23 ) {
//                                      // insert the missing triangle
//              InsertTriangle  ( e1, e2, e3 );
//
//              break;
//              }

                                        // Quadrangular case: connection e1-e3 but not e2-e3
            if      ( v13 && ! v23 ) {
                                        // Search for intermediate edge 4 that could bridge e2 and e3
                for ( l = k + 1, e4 = &Edges[ l ]; l < NumEdges; l++, e4++ ) {

                    if ( ! e4->Has1Triangle () )
                        continue;

                    if ( ! ( v34  = GetCommonVertex ( e3, e4 ) )    // no e3-e4?
                        || v12 == v34
                        || v13 == v34 )
                        continue;

                    if ( ! ( v24  = GetCommonVertex ( e2, e4 ) )    // no e2-e4?
                        || v12 == v24
                        || v13 == v24
                        || v34 == v24 )
                        continue;
                                        // Here we have 4 edges connected through v12, v13, v24 and v34
                                        // picking the best edge to insert
                    if ( ( *v13 - *v24 ).Norm2 () < ( *v12 - *v34 ).Norm2 () ) {

                        e5      = InsertEdge ( v13, v24 );

                        InsertTriangle  ( e1, e2, e5 );
                        InsertTriangle  ( e5, e4, e3 );
                        }
                    else {

                        e5      = InsertEdge ( v12, v34 );

                        InsertTriangle  ( e3, e1, e5 );
                        InsertTriangle  ( e5, e2, e4 );
                        }

                    break;
                    } // for edge 4
                } // ! v23

                                        // Quadrangular case: connection e2-e3 but not e1-e3
            else if ( v23 && ! v13 ) {
                                        // Search for intermediate edge 4 that could bridge e1 and e3
                for ( l = k + 1, e4 = &Edges[ l ]; l < NumEdges; l++, e4++ ) {

                    if ( ! e4->Has1Triangle () )
                        continue;

                    if ( ! ( v34  = GetCommonVertex ( e3, e4 ) )    // no e3-e4?
                        || v12 == v34
                        || v23 == v34 )
                        continue;

                    if ( ! ( v14  = GetCommonVertex ( e1, e4 ) )    // no e1-e4?
                        || v12 == v14
                        || v23 == v14
                        || v34 == v14 )
                        continue;
                                        // Here we have 4 edges connected through v12, v13, v24 and v34
                                        // picking the best edge to insert
                    if ( ( *v14 - *v23 ).Norm2 () < ( *v12 - *v34 ).Norm2 () ) {

                        e5      = InsertEdge ( v14, v23 );

                        InsertTriangle  ( e1, e2, e5 );
                        InsertTriangle  ( e5, e3, e4 );
                        }
                    else {

                        e5      = InsertEdge ( v12, v34 );

                        InsertTriangle  ( e4, e1, e5 );
                        InsertTriangle  ( e5, e2, e3 );
                        }

                    break;
                    } // for edge 4
                } // ! v13

                                        // Triangular case: connection e1-e3 and e2-e3
            else if ( v13 && v23 ) {
                                        // Here we have a missing triangle / 3 edges connected through v12, v13 and v23
                InsertTriangle  ( e1, e2, e3 );

                break;
                } // v13 && v23

            } // for edge 3
        } // for edge 2
    } // for edge 1
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
