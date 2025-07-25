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

#include    "OpenGL.h"
#include    "OpenGL.Colors.h"
#include    "OpenGL.Geometry.h"
#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum    {
        BillboardSingleSide,
        BillboardDoubleSide
        };


class   TGLBillboard : public TGLObject
{
public:
                    TGLBillboard ();


    TGLCoordinates<GLfloat> Right;
    TGLCoordinates<GLfloat> Up;
    TGLCoordinates<GLfloat> Front;


    void            GLize       ( int param = BillboardSingleSide ) override;
    void            unGLize     ()                                  final;


protected:
    TGLMatrix       Matrix;
    bool            DoubleSide;
};

                                        // missing: copy constructor & assignation (we don't need it yet, we use static objects)
class   TGLBillboardSphere : public TGLBillboard
{
public:
                    TGLBillboardSphere  ()                                          { NumRounds = NumSlices = NumPoints = 0; Shape = 0; };
                    TGLBillboardSphere  ( int numrounds, int numslices );
                   ~TGLBillboardSphere  ();


    void            GLize               ( int param = BillboardSingleSide ) final   { TGLBillboard::GLize ( param ); }
    void            GLize               ( GLfloat x, GLfloat y, GLfloat z, GLfloat r );


protected:
    int             NumRounds;
    int             NumSlices;
    int             NumPoints;

    TGLCoordinates<GLfloat> *Shape;
};


//----------------------------------------------------------------------------

template <class TypeD>
class   TGLArrow :  public TGLObject
{
public:
                            TGLArrow    ()                 {}
                            TGLArrow    (  GLdouble    fromx,  GLdouble    fromy,  GLdouble    fromz, 
                                           GLdouble    tox,    GLdouble    toy,    GLdouble    toz, 
                                           double      headl,  double      headh, 
                                           GLdouble    r,      GLdouble    g,      GLdouble    b,      GLdouble    a );


    TGLCoordinates<TypeD>   From;
    TGLCoordinates<TypeD>   To;

    void                    GLize   ( int param = 0 )   final;


protected:
    TGLCoordinates<TypeD>   Head0;
    TGLCoordinates<TypeD>   Head1;
    TGLCoordinates<TypeD>   Head2;
    TGLCoordinates<TypeD>   Head3;
    TGLCoordinates<TypeD>   Head4;
    TGLColor<TypeD>         Color;
};


//----------------------------------------------------------------------------
/*
enum    {
        ApplyToCurrent,
        ReplaceProjection,
        ReplaceModelView
        };
*/

class   TGLPrimitives : public TGLObject
{
public:

    void            DrawTriangle    ( double ox, double oy, double oz, double angle );
    void            DrawRectangle   ( double ox, double oy, double oz, double l, double h, double angle );
    void            DrawCircle      ( double ox, double oy, double oz, double rmin, double rmax, double anglefrom, double angleto );
    void            DrawDisk        ( double ox, double oy, double oz, double r, double anglefrom, double angleto );
    void            DrawArrow       ( double ox, double oy, double oz, double rectl, double recth, double headl, double headh, double angle );

    void            DrawBrightness  ( double ox, double oy, double oz, double r );
    void            DrawContrast    ( double ox, double oy, double oz, double r );
    void            DrawPlus        ( double ox, double oy, double oz, double l );

    void            Draw3DCross     ( const TPointFloat& p, GLfloat size, GLfloat thick, bool depthoff = false );
    void            Draw3DWireFrame ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat thick );
    void            Draw3DFrame     ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat width );
    void            Draw3DBox       ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat thick );
    void            Draw3DCylinder  ( const TPointFloat& p1, const TPointFloat& p2, GLfloat radius );
    void            Draw3DPlane     ( const TPointFloat& p, const TPointFloat& normal, GLfloat radius );
    void            Draw3DArrow     ( const TPointFloat& p1, const TPointFloat& p2, double shaftradius = 0, double headlength = 0 );
};


//----------------------------------------------------------------------------
                                        // Fill the triangle colormap-correct
void    GLColorTrianglefv ( GLfloat                 v1[ 3 ],    GLfloat         v2[ 3 ],    GLfloat         v3[ 3 ],
                            float                   val1,       float           val2,       float           val3,
                            const TGLColorTable&    colormap,   double          sizelimit,  int             level   );

                                        // Subdivide a triangle (in material mode) to render it a la Phong
void    GLSmoothTrianglefv ( GLfloat        v1[ 3 ],    GLfloat         v2[ 3 ],    GLfloat         v3[ 3 ], 
                             GLfloat        n1[ 3 ],    GLfloat         n2[ 3 ],    GLfloat         n3[ 3 ],
                            /*double sizelimit,*/       int             level                               );


//----------------------------------------------------------------------------
                                        // Fill quad-strips with colormap-correct
enum    QuadQuality
        {
        QuadBestQuality         = 1,
        QuadRegularQuality,
        QuadFastestQuality
        };


class   TGLQuadStrip :  public TGLObject
{
public:
                    TGLQuadStrip    ();


    void            GLize           ( int param = 0 )   final;

    void            Begin           ( QuadQuality quality, TGLColorTable &colormap );
    void            NextVertices    ( GLfloat v1[ 3 ], GLfloat v2[ 3 ], float val1, float val2 );


protected:
    QuadQuality     Quality;
    TGLColorTable*  ColorTable;
    double          TriangleSizeLimit;
    int             TriangleLevel;


private:
    bool            NotFirstCoordinates;
    TPointFloat     Vertex3;            // previous left points
    TPointFloat     Vertex4;
    float           Value3;
    float           Value4;
                                        // intermediate variables
    GLfloat         vecbuff[ 5 ][ 3 ];
    float           valbuff[ 5 ];
};


//----------------------------------------------------------------------------

class   TGLQuadMesh :  public TGLObject
{
public:
                            TGLQuadMesh ();


    void                    Reset       ();


    void                    Begin       ( int rowsize, int quality, TGLColorTable& colormap );  // rowsize can vary for each call, though it should remain < maxrowsize
    void                    NextVertex  ( GLfloat v[ 3 ], float val );


protected:
    int                     RowSize;
    int                     Quality;
    TGLColorTable*          ColorTable;
    double                  TriangleSizeLimit;  // Quality -> GLColorTrianglefv
    int                     TriangleLevel;      // Quality -> GLColorTrianglefv


private:
    bool                    FirstRow;   // internal counters
    int                     VertexCounter;
                                        // store a previous row and previous point
    TArray1<TPointFloat>    VerticesRow;
    TArray1<float>          ValuesRow;
    TPointFloat             Vertex3;
    float                   Value3;
                                        // intermediate variables
    GLfloat                 vecbuff[ 5 ][ 3 ];
    float                   valbuff[ 5 ];

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TGLArrow<TypeD>::TGLArrow ( GLdouble    fromx,  GLdouble    fromy,  GLdouble    fromz, 
                                    GLdouble    tox,    GLdouble    toy,    GLdouble    toz, 
                                    double      headl,  double      headh, 
                                    GLdouble    r,      GLdouble    g,      GLdouble    b,      GLdouble    a )
{
From    .Set ( fromx, fromy, fromz );
To      .Set ( tox,   toy,   toz );
Color   .Set ( r, g, b, a );


double              ftx,    fty,    ftz;
double              v1x,    v1y,    v1z;
double              v2x,    v2y,    v2z;
double              hlx,    hly,    hlz;
double              n2;

                                        // from-to vector
ftx     = tox - fromx;  
fty     = toy - fromy;  
ftz     = toz - fromz;
n2      = NonNull ( sqrt ( ftx * ftx + fty * fty + ftz * ftz ) );
ftx    /= n2;      
fty    /= n2;      
ftz    /= n2;

                                        // an orthogonal vector to from-to
v1x = 0;        v1y = -ftz;     v1z = fty;
if ( v1x == 0 && v1y == 0 && v1z == 0 ) {
    v1x = -ftz;     v1y = 0;        v1z = ftx;
    }
n2      = NonNull ( sqrt ( v1x * v1x + v1y * v1y + v1z * v1z ) );
v1x    /= n2;      
v1y    /= n2;      
v1z    /= n2;

                                        // a third vector, orthogonal to the two
v2x     = fty * v1z - ftz * v1y;
v2y     = ftz * v1x - ftx * v1z;
v2z     = ftx * v1y - fty * v1x;

                                        // starting point of the arror head
hlx     = tox - headl * ftx;
hly     = toy - headl * fty;
hlz     = toz - headl * ftz;

                                        // set 4 arrow head points
Head0.Set ( hlx, hly, hlz );
Head1.Set ( hlx + headh * v1x, hly + headh * v1y, hlz + headh * v1z );
Head2.Set ( hlx - headh * v1x, hly - headh * v1y, hlz - headh * v1z );
Head3.Set ( hlx + headh * v2x, hly + headh * v2y, hlz + headh * v2z );
Head4.Set ( hlx - headh * v2x, hly - headh * v2y, hlz - headh * v2z );
}


template <class TypeD>
void    TGLArrow<TypeD>::GLize ( int /*param*/ )
{
                                        // don't write in Z buffer
glPushAttrib ( GL_DEPTH_BUFFER_BIT );
glDepthMask ( GL_FALSE );

Color.GLize ();

glBegin ( GL_LINES );
From.GLize();
To.GLize();
glEnd();

glBegin ( GL_TRIANGLE_FAN );
To.GLize();
Head1.GLize();
Head3.GLize();
Head2.GLize();
Head4.GLize();
Head1.GLize();
glEnd();

glBegin ( GL_TRIANGLE_FAN );
Head0.GLize();
Head1.GLize();
Head3.GLize();
Head2.GLize();
Head4.GLize();
Head1.GLize();
glEnd();

glPopAttrib();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
