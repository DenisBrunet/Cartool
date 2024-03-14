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

#pragma once

#include    "Strings.Utils.h"
#include    "TCartoolApp.h"

#include    "OpenGL.Drawing.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLBillboard::TGLBillboard ()
{
DoubleSide          = false;
}


void    TGLBillboard::GLize ( int param )
{
DoubleSide          = param == BillboardDoubleSide;


glMatrixMode ( GL_MODELVIEW );

glPushMatrix();
                                        // retrieve the current transformation
Matrix.CopyModelView ();
                                        // the matrix is (supposedly) orthogonal, transpose == inverse
Right.X     = Matrix[  0 ];
Right.Y     = Matrix[  4 ];
Right.Z     = Matrix[  8 ];

Up   .X     = Matrix[  1 ];
Up   .Y     = Matrix[  5 ];
Up   .Z     = Matrix[  9 ];

Front.X     = Matrix[  2 ];
Front.Y     = Matrix[  6 ];
Front.Z     = Matrix[ 10 ];

                                        // we lazy, let's use the hardware from time to time..
GLNormalizeNormalOn ();
glFrontFace         ( GL_CW );          // specifies the right orientation
}


void    TGLBillboard::unGLize ()
{
GLNormalizeNormalOff();

glPopMatrix();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TGLBillboardSphere::~TGLBillboardSphere ()
{
if ( Shape ) {
    delete[] Shape;
    Shape = 0;
    }
}

        TGLBillboardSphere::TGLBillboardSphere ( int numrounds, int numslices )
{
/*                                        // prepare the cone that simulates a sphere
for ( int i = 0; i < NumPoints; i++ ) {
    Shape[ i ].X = cos ( i * TwoPi / ( NumPoints - 1 ) );
    Shape[ i ].Y = sin ( i * TwoPi / ( NumPoints - 1 ) );
    Shape[ i ].Z = 0;
    }

                                        // cone middle
Shape[ NumPoints ].X = -0.30;
Shape[ NumPoints ].Y = -0.30;
Shape[ NumPoints ].Z = -0.30;

Shape[ NumPoints ].X = 0;
Shape[ NumPoints ].Y = 0;
Shape[ NumPoints ].Z = 0;
*/

if ( Shape ) {
    delete[] Shape;
    Shape = 0;
    }

NumRounds           = numrounds;
NumSlices           = numslices;
NumPoints           = ( NumSlices + 1 ) * NumRounds;

                                        // 1 more for the top-middle, 1 more for temp variable
Shape               = new TGLCoordinates<GLfloat> [ NumPoints + 2 ];

                                        // this is not really billboarding anymore, or a kind of 3D billboarding
                                        // still, one advantage is that the rendering appears constant (ie no jaggles) through rotations
double      lr, hr;


for ( int i = 0; i < NumRounds; i++ ) {
                                        // length & height of nth radius
    lr = cos ( i * 0.5 * Pi / NumRounds );
    hr = sin ( i * 0.5 * Pi / NumRounds );

    for ( int j = 0, j1 = i * ( NumSlices + 1 ); j <= NumSlices; j++, j1++ ) {

        Shape[ j1 ].X = cos ( j * TwoPi / NumSlices ) * lr;
        Shape[ j1 ].Y = sin ( j * TwoPi / NumSlices ) * lr;
        Shape[ j1 ].Z = hr;
        }
    }

Shape[ NumPoints ].X = 0;
Shape[ NumPoints ].Y = 0;
Shape[ NumPoints ].Z = 1;
}


void    TGLBillboardSphere::GLize ( GLfloat x, GLfloat y, GLfloat z, GLfloat r )
{
/*
GLfloat    *pos = ( GLfloat *) Shape[ NumPoints + 1 ];

glBegin ( GL_TRIANGLE_FAN );            // yes, I'm fan of fans (it's fun)

                                        // set center
//glNormal3f ( TempMatrix[ 2 ], TempMatrix[ 6 ], TempMatrix[ 10 ] );
//glVertex3f ( 0, 0, 0 );

                                        // shifting a bit the center makes a better rendering
glNormal3f ( Shape[ NumPoints ].X * Right.X,
             Shape[ NumPoints ].Y * Right.Y,
             Shape[ NumPoints ].Z * Right.Z );

                                        // set to correct depth -> the Fan is indeed a cone, + slight shift
glVertex3f ( Front.X + Shape[ NumPoints ].X * Right.X,
             Front.Y + Shape[ NumPoints ].Y * Right.Y,
             Front.Z + Shape[ NumPoints ].Z * Right.Z );

                                        // draw the shape: cone -> looks like a sphere
for ( int i = 0; i < NumPoints; i++ ) {

    pos[ 0 ] = Right.X * Shape[ i ].X + Up.X * Shape[ i ].Y;
    pos[ 1 ] = Right.Y * Shape[ i ].X + Up.Y * Shape[ i ].Y;
    pos[ 2 ] = Right.Z * Shape[ i ].X + Up.Z * Shape[ i ].Y;

    glNormal3fv ( pos );
    glVertex3fv ( pos );
    }

glEnd();
*/

//glFrontFace ( GL_CW );                  // specifies the right orientation
glTranslatef ( x, y, z );
glScaled     ( r, r, r );

                                        // get a temp variable
GLfloat    *pos = ( GLfloat *) Shape[ NumPoints + 1 ];

                                        // if needed, draw first the transparent, opposite side
for ( int side = DoubleSide ? 0 : 1; side < 2; side++ ) {

    if ( side == 0 )
        Front  *= -1;

                                        // draw a series of rings
    for ( int i = 0; i < NumRounds - 1; i++ ) {

        glBegin ( GL_TRIANGLE_STRIP );

        for ( int j = 0, j1 = i * ( NumSlices + 1 ), j2 = j1 + NumSlices + 1 ; j <= NumSlices; j++, j1++, j2++ ) {

            pos[ 0 ] = Right.X * Shape[ j1 ].X + Up.X * Shape[ j1 ].Y + Front.X * Shape[ j1 ].Z;
            pos[ 1 ] = Right.Y * Shape[ j1 ].X + Up.Y * Shape[ j1 ].Y + Front.Y * Shape[ j1 ].Z;
            pos[ 2 ] = Right.Z * Shape[ j1 ].X + Up.Z * Shape[ j1 ].Y + Front.Z * Shape[ j1 ].Z;

            glNormal3fv ( pos );
            glVertex3fv ( pos );

            pos[ 0 ] = Right.X * Shape[ j2 ].X + Up.X * Shape[ j2 ].Y + Front.X * Shape[ j2 ].Z;
            pos[ 1 ] = Right.Y * Shape[ j2 ].X + Up.Y * Shape[ j2 ].Y + Front.Y * Shape[ j2 ].Z;
            pos[ 2 ] = Right.Z * Shape[ j2 ].X + Up.Z * Shape[ j2 ].Y + Front.Z * Shape[ j2 ].Z;

            glNormal3fv ( pos );
            glVertex3fv ( pos );

                                        // for the normal geometrical transformation
//          glNormal3fv ( Shape[ j1 ] );
//          glVertex3fv ( Shape[ j1 ] );
//          glNormal3fv ( Shape[ j2 ] );
//          glVertex3fv ( Shape[ j2 ] );
            } // slices

        glEnd();
        } // rounds


                                        // draw the top
    glBegin ( GL_TRIANGLE_FAN );        // yes, I'm fan of fans (it's fun)
                                        // set center
    pos[ 0 ] = Right.X * Shape[ NumPoints ].X + Up.X * Shape[ NumPoints ].Y + Front.X * Shape[ NumPoints ].Z;
    pos[ 1 ] = Right.Y * Shape[ NumPoints ].X + Up.Y * Shape[ NumPoints ].Y + Front.Y * Shape[ NumPoints ].Z;
    pos[ 2 ] = Right.Z * Shape[ NumPoints ].X + Up.Z * Shape[ NumPoints ].Y + Front.Z * Shape[ NumPoints ].Z;

    glNormal3fv ( pos );
    glVertex3fv ( pos );

//  glNormal3fv ( Shape[ NumPoints ] );
//  glVertex3fv ( Shape[ NumPoints ] );


    for ( int j = 0, j1 = NumRounds * ( NumSlices + 1 ) - 1; j <= NumSlices; j++, j1-- ) {

        pos[ 0 ] = Right.X * Shape[ j1 ].X + Up.X * Shape[ j1 ].Y + Front.X * Shape[ j1 ].Z;
        pos[ 1 ] = Right.Y * Shape[ j1 ].X + Up.Y * Shape[ j1 ].Y + Front.Y * Shape[ j1 ].Z;
        pos[ 2 ] = Right.Z * Shape[ j1 ].X + Up.Z * Shape[ j1 ].Y + Front.Z * Shape[ j1 ].Z;

        glNormal3fv ( pos );
        glVertex3fv ( pos );

//      glNormal3fv ( Shape[ j1 ] );
//      glVertex3fv ( Shape[ j1 ] );
        }

    glEnd();


    if ( side == 0 )
        Front  *= -1;
    } // for side



glScaled ( 1 / r, 1 / r, 1 / r );
glTranslatef ( -x, -y, -z );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // 2 handy lambdas
auto    MiddleVertex        = [] ( GLfloat* M, const GLfloat* A, const GLfloat* B ) {
M[ 0 ]  = ( A[ 0 ] + B[ 0 ] ) / 2;
M[ 1 ]  = ( A[ 1 ] + B[ 1 ] ) / 2; 
M[ 2 ]  = ( A[ 2 ] + B[ 2 ] ) / 2;
};


auto    NormalizeNormal     = [] ( GLfloat* N ) {

double              norm            = sqrt ( N[ 0 ] * N[ 0 ] + N[ 1 ] * N[ 1 ] + N[ 2 ] * N[ 2 ] );

if ( norm ) {
    N[ 0 ]  /= norm;
    N[ 1 ]  /= norm;
    N[ 2 ]  /= norm;
    }
};


//----------------------------------------------------------------------------
                                        // Do a colormap-correct triangle, by a recursive 4 sub-triangles decomposition
void    GLColorTrianglefv ( GLfloat         v1[ 3 ],    GLfloat         v2[ 3 ],    GLfloat         v3[ 3 ],
                            float           val1,       float           val2,       float           val3,
                            TGLColorTable&  colormap,   double          sizelimit,  int             level    )
{
                                        // 0) value -> color
TGLColor<GLfloat>   glcol1;
TGLColor<GLfloat>   glcol2;
TGLColor<GLfloat>   glcol3;

colormap.GetColorIndex ( val1, glcol1 );
colormap.GetColorIndex ( val2, glcol2 );
colormap.GetColorIndex ( val3, glcol3 );


                                        // 1) reached maximum recursive depth?
if ( level <= 0 ) {
    glcol1.GLize ();    glVertex3fv ( v1 );
    glcol2.GLize ();    glVertex3fv ( v2 );
    glcol3.GLize ();    glVertex3fv ( v3 );
    return;
    }


                                        // 2) geometrical criterion: vertices proximity?
bool                verticesveryclose   = fabs ( v1[ 0 ] - v2[ 0 ] ) + fabs ( v1[ 1 ] - v2[ 1 ] ) + fabs ( v1[ 2 ] - v2[ 2 ] )
                                        + fabs ( v2[ 0 ] - v3[ 0 ] ) + fabs ( v2[ 1 ] - v3[ 1 ] ) + fabs ( v2[ 2 ] - v3[ 2 ] )
                                        + fabs ( v1[ 0 ] - v3[ 0 ] ) + fabs ( v1[ 1 ] - v3[ 1 ] ) + fabs ( v1[ 2 ] - v3[ 2 ] ) < sizelimit;

if ( verticesveryclose ) {
    glcol1.GLize ();    glVertex3fv ( v1 );
    glcol2.GLize ();    glVertex3fv ( v2 );
    glcol3.GLize ();    glVertex3fv ( v3 );
    return;
    }

                                        // 3) color criterion: color proximity?
                                        // threshold is looser with depth
double              colorveryclose      =  0.45 - (double) level * 0.07;

                                        // expensive to compute, but can cut down a lot of triangles
bool                b12                 = fabs ( glcol1[ 0 ] - glcol2[ 0 ] ) + fabs ( glcol1[ 1 ] - glcol2[ 1 ] ) + fabs ( glcol1[ 2 ] - glcol2[ 2 ] ) < colorveryclose;
bool                b23                 = fabs ( glcol3[ 0 ] - glcol2[ 0 ] ) + fabs ( glcol3[ 1 ] - glcol2[ 1 ] ) + fabs ( glcol3[ 2 ] - glcol2[ 2 ] ) < colorveryclose;
bool                b13                 = fabs ( glcol1[ 0 ] - glcol3[ 0 ] ) + fabs ( glcol1[ 1 ] - glcol3[ 1 ] ) + fabs ( glcol1[ 2 ] - glcol3[ 2 ] ) < colorveryclose;


if ( b12 && b23 && b13 ) {
    glcol1.GLize ();    glVertex3fv ( v1 );
    glcol2.GLize ();    glVertex3fv ( v2 );
    glcol3.GLize ();    glVertex3fv ( v3 );
    return;
    }


level--;

                                        // select the right subdivision according to normals similarity
if      ( b12 && b13 ) {
    float               val             = ( val2 + val3 ) / 2;
    GLfloat             v[ 3 ];
    MiddleVertex ( v, v2, v3 );

    GLColorTrianglefv ( v1,     v2,     v,      val1,     val2,     val, colormap, sizelimit, level );
    GLColorTrianglefv ( v3,     v1,     v,      val3,     val1,     val, colormap, sizelimit, level );
    }
else if ( b12 && b23 ) {
    float               val             = ( val1 + val3 ) / 2;
    GLfloat             v[ 3 ];
    MiddleVertex ( v, v1, v3 );

    GLColorTrianglefv ( v1,     v2,     v,      val1,     val2,     val, colormap, sizelimit, level );
    GLColorTrianglefv ( v2,     v3,     v,      val2,     val3,     val, colormap, sizelimit, level );
    }
else if ( b23 && b13 ) {
    float               val             = ( val1 + val2 ) / 2;
    GLfloat             v[ 3 ];
    MiddleVertex ( v, v1, v2 );

    GLColorTrianglefv ( v3,     v1,     v,      val3,     val1,     val, colormap, sizelimit, level );
    GLColorTrianglefv ( v2,     v3,     v,      val2,     val3,     val, colormap, sizelimit, level );
    }
else {
                                        // compute intermediate positions & normals
    constexpr uint      i12         = 0;
    constexpr uint      i23         = 1;
    constexpr uint      i13         = 2;
    GLfloat             v  [ 3 ][ 3 ];
    float               val[ 3 ];
                                        // vertices
    MiddleVertex ( v[ i12 ], v1, v2 );
    MiddleVertex ( v[ i23 ], v2, v3 );
    MiddleVertex ( v[ i13 ], v1, v3 );
                                        // values
    val[ i12 ]  = ( val1 + val2 ) / 2;
    val[ i23 ]  = ( val2 + val3 ) / 2;
    val[ i13 ]  = ( val1 + val3 ) / 2;

    GLColorTrianglefv ( v1,     v[i12], v[i13], val1,     val[i12], val[i13], colormap, sizelimit, level );
    GLColorTrianglefv ( v2,     v[i23], v[i12], val2,     val[i23], val[i12], colormap, sizelimit, level );
    GLColorTrianglefv ( v3,     v[i13], v[i23], val3,     val[i13], val[i23], colormap, sizelimit, level );
    GLColorTrianglefv ( v[i12], v[i23], v[i13], val[i12], val[i23], val[i13], colormap, sizelimit, level );
    }

                                        // complete the real border with the intermediate nodes
                                        // to avoid uncovered areas, due to roundings
/*
if ( fabs ( v1[0] + v2[0] - v[i12][0] + v1[1] + v2[1] - v[i12][1] + v1[2] + v2[2] - v[i12][2] ) > 1e-6 )
    GLColorTrianglefv ( v1,     v2,     v[i12], val1,     val2,     val[i12], colormap, sizelimit, 0 );
if ( fabs ( v1[0] + v3[0] - v[i13][0] + v1[1] + v3[1] - v[i13][1] + v1[2] + v3[2] - v[i13][2] ) > 1e-6 )
    GLColorTrianglefv ( v3,     v1,     v[i13], val3,     val1,     val[i13], colormap, sizelimit, 0 );
  if ( fabs ( v3[0] + v2[0] - v[i23][0] + v3[1] + v2[1] - v[i23][1] + v3[2] + v2[2] - v[i23][2] ) > 1e-6 )
    GLColorTrianglefv ( v2,     v3,     v[i23], val2,     val3,     val[i23], colormap, sizelimit, 0 );
*/
/*
if ( level <= 0 ) {
    GLfloat glcol12[4];
    GLfloat glcol23[4];
    GLfloat glcol13[4];
    colormap.GetColorIndex ( val[i12], glcol12 );
    colormap.GetColorIndex ( val[i23], glcol23 );
    colormap.GetColorIndex ( val[i13], glcol13 );

    glColor4fv ( glcol1 );      glVertex3fv ( v1 );
    glColor4fv ( glcol2 );      glVertex3fv ( v2 );
    glColor4fv ( glcol12 );     glVertex3fv ( v[i12] );

    glColor4fv ( glcol2 );      glVertex3fv ( v2 );
    glColor4fv ( glcol3 );      glVertex3fv ( v3 );
    glColor4fv ( glcol23 );     glVertex3fv ( v[i23] );

    glColor4fv ( glcol3 );      glVertex3fv ( v3 );
    glColor4fv ( glcol1 );      glVertex3fv ( v1 );
    glColor4fv ( glcol13 );     glVertex3fv ( v[i13] );
    }
*/
}


//----------------------------------------------------------------------------
void    GLSmoothTrianglefv (    GLfloat        v1[ 3 ],    GLfloat         v2[ 3 ],    GLfloat         v3[ 3 ], 
                                GLfloat        n1[ 3 ],    GLfloat         n2[ 3 ],    GLfloat         n3[ 3 ],
                              /*double sizelimit,*/        int             level                                )
{
                                        // 1) Reached maximum recursive level
if ( level <= 0 ) {
    glNormal3fv ( n1 ); glVertex3fv ( v1 );
    glNormal3fv ( n2 ); glVertex3fv ( v2 );
    glNormal3fv ( n3 ); glVertex3fv ( v3 );
    return;
    }


/*                                        // 2) geometrical criterion: vertices proximity
bool                verticesveryclose   = fabs ( v1[0] - v2[0] ) + fabs ( v1[1] - v2[1] ) + fabs ( v1[2] - v2[2] )
                                        + fabs ( v2[0] - v3[0] ) + fabs ( v2[1] - v3[1] ) + fabs ( v2[2] - v3[2] )
                                        + fabs ( v1[0] - v3[0] ) + fabs ( v1[1] - v3[1] ) + fabs ( v1[2] - v3[2] ) < sizelimit;
*/

                                        // 2) geometrical criterion: normals similarity
                                        // threshold is looser with depth
double              normalsveryclose    = 0.30 - (double) level * 0.1;

bool                b12                 = fabs ( n1[0] - n2[0] ) + fabs ( n1[1] - n2[1] ) + fabs ( n1[2] - n2[2] ) < normalsveryclose;
bool                b23                 = fabs ( n3[0] - n2[0] ) + fabs ( n3[1] - n2[1] ) + fabs ( n3[2] - n2[2] ) < normalsveryclose;
bool                b13                 = fabs ( n1[0] - n3[0] ) + fabs ( n1[1] - n3[1] ) + fabs ( n1[2] - n3[2] ) < normalsveryclose;


if ( b12 && b23 && b13 ) {
    glNormal3fv ( n1 ); glVertex3fv ( v1 );
    glNormal3fv ( n2 ); glVertex3fv ( v2 );
    glNormal3fv ( n3 ); glVertex3fv ( v3 );
    return;
    }

                                        // compute intermediate positions & normals
constexpr uint      i12         = 0;
constexpr uint      i23         = 1;
constexpr uint      i13         = 2;
GLfloat             v[ 3 ][ 3 ];
GLfloat             n[ 3 ][ 3 ];

                                        // vertices
MiddleVertex    ( v[i12], v1, v2 );
MiddleVertex    ( v[i23], v2, v3 );
MiddleVertex    ( v[i13], v1, v3 );

                                        // normals
MiddleVertex    ( n[i12], n1, n2 );
MiddleVertex    ( n[i23], n2, n3 );
MiddleVertex    ( n[i13], n1, n3 );
NormalizeNormal ( n[i12] );
NormalizeNormal ( n[i23] );
NormalizeNormal ( n[i13] );


level--;

                                        // select the right subdivision according to normals similarity
if      ( b12 && b13 ) {
    GLSmoothTrianglefv ( v1,     v2,     v[i23], n1,     n2,     n[i23], level );
    GLSmoothTrianglefv ( v3,     v1,     v[i23], n3,     n1,     n[i23], level );
    }
else if ( b12 && b23 ) {
    GLSmoothTrianglefv ( v1,     v2,     v[i13], n1,     n2,     n[i13], level );
    GLSmoothTrianglefv ( v2,     v3,     v[i13], n2,     n3,     n[i13], level );
    }
else if ( b23 && b13 ) {
    GLSmoothTrianglefv ( v3,     v1,     v[i12], n3,     n1,     n[i12], level );
    GLSmoothTrianglefv ( v2,     v3,     v[i12], n2,     n3,     n[i12], level );
    }
else {
    GLSmoothTrianglefv ( v1,     v[i12], v[i13], n1,     n[i12], n[i13], level );
    GLSmoothTrianglefv ( v2,     v[i23], v[i12], n2,     n[i23], n[i12], level );
    GLSmoothTrianglefv ( v3,     v[i13], v[i23], n3,     n[i13], n[i23], level );
    GLSmoothTrianglefv ( v[i12], v[i23], v[i13], n[i12], n[i23], n[i13], level );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLQuadStrip::TGLQuadStrip ()
{
Quality             = QuadRegularQuality;
ColorTable          = 0;
TriangleSizeLimit   = 1;
TriangleLevel       = 2;

NotFirstCoordinates = false;
Vertex3             = 0.0;
Vertex4             = 0.0;
Value3 = Value4     = 0;
}

                                        // start process, assume quality and colortable don't change
void    TGLQuadStrip::Begin ( QuadQuality quality, TGLColorTable &colormap )
{
Quality             = quality;
ColorTable          = &colormap;

NotFirstCoordinates = false;
Vertex3             = 0.0;
Vertex4             = 0.0;
Value3  = Value4    = 0;

                                        // set parameters according to quality
if      ( Quality == QuadBestQuality ) {
    TriangleSizeLimit   = 0;            // override with my parameters
    TriangleLevel       = 3;
    }
//else if ( Quality == QuadAdaptativeQuality ) {
//    TriangleSizeLimit = sizelimit;    // trust user's parameters
//    TriangleLevel     = 2;
//    }
else {
    TriangleSizeLimit   = 1;            // not used, but set anyway
    TriangleLevel       = 0;
    }

                                        // to avoid line artifacts between triangles
//GLPolygonSmoothOff ();
GLSmoothEdgesOff ();

                                        // choose correct OpenGL begin
if      ( Quality == QuadFastestQuality )
    glBegin ( GL_QUAD_STRIP );
else if ( Quality == QuadBestQuality )
    glBegin ( GL_TRIANGLES );
}

                                        // finishing all
void    TGLQuadStrip::GLize ( int /*param*/ )
{
if ( Quality != QuadRegularQuality )
    glEnd ();
}


// orders V4  V2
//        V3  V1
// and
void    TGLQuadStrip::NextVertices ( GLfloat v1[ 3 ], GLfloat v2[ 3 ], float val1, float val2 )
{
                                        // simplified plotting, using usual quadstrip - good for low resolution
if      ( Quality == QuadFastestQuality ) {

    ColorTable->GLize ( val1 );
    glVertex3fv ( v1 );

    ColorTable->GLize ( val2 );
    glVertex3fv ( v2 );
    }
                                        // highest quality, by using our smart GLColorTrianglefv function
else if ( Quality == QuadBestQuality ) {

    if ( NotFirstCoordinates ) {
//        GLColorTrianglefv ( v1, v2,      Vertex3, val1, val2,   Value3, *ColorTable, TriangleSizeLimit, TriangleLevel );
//        GLColorTrianglefv ( v2, Vertex4, Vertex3, val2, Value4, Value3, *ColorTable, TriangleSizeLimit, TriangleLevel );

                                        // smartly choose how to split the quad in 2 triangles
        if ( fabs ( val1 - Value4 ) > fabs ( val2 - Value3 ) ) {
            GLColorTrianglefv ( v1, v2,      Vertex3, val1, val2,   Value3, *ColorTable, TriangleSizeLimit, TriangleLevel );
            GLColorTrianglefv ( v2, Vertex4, Vertex3, val2, Value4, Value3, *ColorTable, TriangleSizeLimit, TriangleLevel );
            }
        else {
            GLColorTrianglefv ( v1, v2,      Vertex4, val1, val2,   Value4, *ColorTable, TriangleSizeLimit, TriangleLevel );
            GLColorTrianglefv ( v1, Vertex4, Vertex3, val1, Value4, Value3, *ColorTable, TriangleSizeLimit, TriangleLevel );
            }
        }
    }

else if ( Quality == QuadRegularQuality ) {

    if ( NotFirstCoordinates ) {
                                        // in-between quality
        constexpr uint  MidLeft             = 0;
        constexpr uint  MidLeftAboveLeft    = 1;
        constexpr uint  MidAbove            = 2;
        constexpr uint  MidAboveAboveLeft   = 3;
        constexpr uint  MidAll              = 4;
                                        // interpolate values
        valbuff[MidLeft]            = ( Value3 + val1                 ) / 2;
        valbuff[MidLeftAboveLeft]   = ( Value3 + Value4               ) / 2;
        valbuff[MidAbove]           = ( val2   + val1                 ) / 2;
        valbuff[MidAboveAboveLeft]  = ( val2   + Value4               ) / 2;
        valbuff[MidAll]             = ( Value3 + val1 + Value4 + val2 ) / 4;

                                        // interpolate positions
        MiddleVertex ( vecbuff[MidLeft],              Vertex3,                      v1 );
        MiddleVertex ( vecbuff[MidLeftAboveLeft],     Vertex3,                      Vertex4 );
        MiddleVertex ( vecbuff[MidAbove],             v1,                           v2 );
        MiddleVertex ( vecbuff[MidAboveAboveLeft],    Vertex4,                      v2 );
        MiddleVertex ( vecbuff[MidAll],               vecbuff[MidLeftAboveLeft],    vecbuff[MidAbove] );

                                        // use a fan to lessen the triangle-strip slant artifacts usually encountered
        glBegin ( GL_TRIANGLE_FAN );
        ColorTable->GLize ( valbuff[MidAll] );              glVertex3fv ( vecbuff[MidAll] );
        ColorTable->GLize ( Value3 );                       glVertex3fv ( Vertex3 );
        ColorTable->GLize ( valbuff[MidLeft] );             glVertex3fv ( vecbuff[MidLeft] );
        ColorTable->GLize ( val1 );                         glVertex3fv ( v1 );
        ColorTable->GLize ( valbuff[MidAbove] );            glVertex3fv ( vecbuff[MidAbove] );
        ColorTable->GLize ( val2 );                         glVertex3fv ( v2 );
        ColorTable->GLize ( valbuff[MidAboveAboveLeft] );   glVertex3fv ( vecbuff[MidAboveAboveLeft] );
        ColorTable->GLize ( Value4 );                       glVertex3fv ( Vertex4 );
        ColorTable->GLize ( valbuff[MidLeftAboveLeft] );    glVertex3fv ( vecbuff[MidLeftAboveLeft] );
        ColorTable->GLize ( Value3 );                       glVertex3fv ( Vertex3 );
        glEnd();
        }
    }

                                        // update all these variables properly
NotFirstCoordinates   = true;

Vertex3         = v1;
Vertex4         = v2;
Value3          = val1;
Value4          = val2;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLQuadMesh::TGLQuadMesh ()
{
Reset ();
}

                                       // only the mesh parameters, all working variables are reset through Begin call
void    TGLQuadMesh::Reset ()
{
RowSize             = 0;
Quality             = QuadRegularQuality;
ColorTable          = 0;
TriangleSizeLimit   = 1;
TriangleLevel       = 2;

FirstRow            = true;
VertexCounter       = 0;
}

                                        // begin drawing with specified quality and colortable at call time
void    TGLQuadMesh::Begin ( int rowsize, int quality, TGLColorTable &colormap )
{
Reset ();

RowSize             = rowsize;
Quality             = quality;
ColorTable          = &colormap;

                                        // resize / allocate arrays, only if new size is bigger than current
if ( (int) ValuesRow < RowSize ) {
    VerticesRow.Resize ( RowSize );
    ValuesRow  .Resize ( RowSize );
    }

Vertex3             = 0.0;
Value3              = 0;

                                        // set parameters according to quality
if      ( Quality == QuadBestQuality ) {
    TriangleSizeLimit   = 0;            // override with my parameters
    TriangleLevel       = 3;
    }
//else if ( Quality == QuadAdaptativeQuality ) {
//    TriangleSizeLimit = sizelimit;    // trust user's parameters
//    TriangleLevel     = 2;
//    }
else {
    TriangleSizeLimit   = 1;            // not used, but set anyway
    TriangleLevel       = 0;
    }

                                        // to avoid line artifacts between triangles
//GLPolygonSmoothOff ();
GLSmoothEdgesOff ();
}

// geometrical order is:
//        VAboveLeft  VAbove
//        V3          V1

#define     vabove          VerticesRow[ VertexCounter ]
#define     vaboveleft      VerticesRow[ VertexCounter - 1 ]
#define     vleft           Vertex3

#define     valueabove      ValuesRow[ VertexCounter     ]
#define     valueaboveleft  ValuesRow[ VertexCounter - 1 ]
#define     valueleft       Value3

void    TGLQuadMesh::NextVertex ( GLfloat v[ 3 ], float val )
{

if ( FirstRow ) {                       // process the first row, just by storing in a buffer

    vabove          = v;
    valueabove      = val;

    VertexCounter = ++VertexCounter % RowSize;

    if ( ! VertexCounter )
        FirstRow = false;               // ok, next time we will start drawing something

    return;
    }

                                        // simplified plotting, using usual quadstrip - good for low resolution
if      ( Quality == QuadFastestQuality ) {

    if ( ! VertexCounter )
        glBegin ( GL_QUAD_STRIP );


    ColorTable->GLize ( valueabove );
    glVertex3fv       ( vabove );

    ColorTable->GLize ( val );
    glVertex3fv       ( v );

    vabove          = v;
    valueabove      = val;
    }
                                        // highest quality, by using our smart GLColorTrianglefv function
else if ( Quality == QuadBestQuality ) {

    if ( VertexCounter ) {
//      GLColorTrianglefv ( v,      vabove,     vleft, val,        valueabove,     valueleft, *ColorTable, TriangleSizeLimit, TriangleLevel );
//      GLColorTrianglefv ( vabove, vaboveleft, vleft, valueabove, valueaboveleft, valueleft, *ColorTable, TriangleSizeLimit, TriangleLevel );

                                        // smartly choose how to split the quad in 2 triangles
        if ( fabs ( val - valueaboveleft ) > fabs ( valueleft - valueabove ) ) {
            GLColorTrianglefv ( v,      vabove,     vleft,      val,        valueabove,     valueleft,      *ColorTable, TriangleSizeLimit, TriangleLevel );
            GLColorTrianglefv ( vabove, vaboveleft, vleft,      valueabove, valueaboveleft, valueleft,      *ColorTable, TriangleSizeLimit, TriangleLevel );
            }
        else {
            GLColorTrianglefv ( v,      vabove,     vaboveleft, val,        valueabove,     valueaboveleft, *ColorTable, TriangleSizeLimit, TriangleLevel );
            GLColorTrianglefv ( v,      vaboveleft, vleft,      val,        valueaboveleft, valueleft,      *ColorTable, TriangleSizeLimit, TriangleLevel );
            }
        }
    else
        glBegin ( GL_TRIANGLES );
    }

else if ( Quality == QuadRegularQuality ) {

    if ( VertexCounter ) {
/*                                      // even faster, just turn the quad to make it divide differently
                                        // not as nice as the fan, and could be hardware dependent (how the quad is split)
        glBegin ( GL_QUADS );

        if ( fabs ( val - valueaboveleft ) > fabs ( valueleft - valueabove ) ) {
            ColorTable->GLize ( valueabove );
            glVertex3fv       ( vabove );

            ColorTable->GLize ( valueaboveleft );
            glVertex3fv       ( vaboveleft );

            ColorTable->GLize ( valueleft );
            glVertex3fv       ( vleft );

            ColorTable->GLize ( val );
            glVertex3fv       ( v );
            }
        else {
            ColorTable->GLize ( val );
            glVertex3fv       ( v );

            ColorTable->GLize ( valueabove );
            glVertex3fv       ( vabove );

            ColorTable->GLize ( valueaboveleft );
            glVertex3fv       ( vaboveleft );

            ColorTable->GLize ( valueleft );
            glVertex3fv       ( vleft );
            }
        glEnd();
*/
                                        // in-between quality
        constexpr uint  MidLeft             = 0;
        constexpr uint  MidLeftAboveLeft    = 1;
        constexpr uint  MidAbove            = 2;
        constexpr uint  MidAboveAboveLeft   = 3;
        constexpr uint  MidAll              = 4;
                                        // interpolate values
        valbuff[MidLeft]            = ( valueleft  + val                               ) / 2;
        valbuff[MidLeftAboveLeft]   = ( valueleft  + valueaboveleft                    ) / 2;
        valbuff[MidAbove]           = ( valueabove + val                               ) / 2;
        valbuff[MidAboveAboveLeft]  = ( valueabove + valueaboveleft                    ) / 2;
        valbuff[MidAll]             = ( valueleft  + val + valueaboveleft + valueabove ) / 4;

                                        // interpolate positions
        MiddleVertex ( vecbuff[MidLeft],            vleft,                      v );
        MiddleVertex ( vecbuff[MidLeftAboveLeft],   vleft,                      vaboveleft );
        MiddleVertex ( vecbuff[MidAbove],           v,                          vabove );
        MiddleVertex ( vecbuff[MidAboveAboveLeft],  vaboveleft,                 vabove );
        MiddleVertex ( vecbuff[MidAll],             vecbuff[MidLeftAboveLeft],  vecbuff[MidAbove] );

                                        // use a fan to lessen the triangle-strip slant artifacts usually encountered
        glBegin ( GL_TRIANGLE_FAN );
        ColorTable->GLize ( valbuff[MidAll] );              glVertex3fv ( vecbuff[MidAll] );
        ColorTable->GLize ( valueleft );                    glVertex3fv ( vleft );
        ColorTable->GLize ( valbuff[MidLeft] );             glVertex3fv ( vecbuff[MidLeft] );
        ColorTable->GLize ( val );                          glVertex3fv ( v );
        ColorTable->GLize ( valbuff[MidAbove] );            glVertex3fv ( vecbuff[MidAbove] );
        ColorTable->GLize ( valueabove );                   glVertex3fv ( vabove );
        ColorTable->GLize ( valbuff[MidAboveAboveLeft] );   glVertex3fv ( vecbuff[MidAboveAboveLeft] );
        ColorTable->GLize ( valueaboveleft );               glVertex3fv ( vaboveleft );
        ColorTable->GLize ( valbuff[MidLeftAboveLeft] );    glVertex3fv ( vecbuff[MidLeftAboveLeft] );
        ColorTable->GLize ( valueleft );                    glVertex3fv ( vleft );
        glEnd ();
        }
    }

                                        // push lower left to upper left
if ( VertexCounter ) {
    vaboveleft      = vleft;
    valueaboveleft  = valueleft;
    }
                                        // push to left
vleft           = v;
valueleft       = val;

                                        // store last element
if ( VertexCounter == RowSize - 1 ) {
    vabove          = v;
    valueabove      = val;
    }

                                        // next vertex
VertexCounter       = ++VertexCounter % RowSize;

                                        // time to end a row?
if ( ! VertexCounter && Quality != QuadRegularQuality )
    glEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TGLPrimitives::DrawTriangle ( double ox, double oy, double oz, double angle )
{
//GLPolygonSmoothOn   ();

glTranslated ( ox, oy, oz );
glRotated ( angle, 0, 0, 1 );

glBegin ( GL_TRIANGLES );
glVertex3d (  0.00, -0.50,  1.00 );
glVertex3d (  0.00,  0.50,  1.00 );
glVertex3d (  1.00,  0.00,  1.00 );
glEnd ();

glRotated ( -angle, 0, 0, 1 );
glTranslated ( -ox, -oy, -oz );

//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawRectangle ( double ox, double oy, double oz, double l, double h, double angle )
{
//GLPolygonSmoothOn   ();

glTranslated ( ox, oy, oz );
glRotated ( angle, 0, 0, 1 );

glBegin ( GL_QUADS );
glVertex3d ( -l, -h,  1.00 );
glVertex3d ( -l,  h,  1.00 );
glVertex3d (  l,  h,  1.00 );
glVertex3d (  l, -h,  1.00 );
glEnd();

glRotated ( -angle, 0, 0, 1 );
glTranslated ( -ox, -oy, -oz );

//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawCircle ( double ox, double oy, double oz, double rmin, double rmax, double anglefrom, double angleto )
{
glTranslated ( ox, oy, oz );

glBegin ( GL_QUAD_STRIP );

for ( double angle = DegreesToRadians ( anglefrom ); angle < DegreesToRadians ( angleto ); angle += 0.1 ) {
    glVertex3f ( cos ( angle ) * rmin, sin ( angle ) * rmin,  1.00 );
    glVertex3f ( cos ( angle ) * rmax, sin ( angle ) * rmax,  1.00 );
    }
                                        // clean finish
glVertex3f ( cos ( DegreesToRadians ( angleto ) ) * rmin, sin ( DegreesToRadians ( angleto ) ) * rmin,  1.00 );
glVertex3f ( cos ( DegreesToRadians ( angleto ) ) * rmax, sin ( DegreesToRadians ( angleto ) ) * rmax,  1.00 );

glEnd();

glTranslated ( -ox, -oy, -oz );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawDisk ( double ox, double oy, double oz, double r, double anglefrom, double angleto )
{
glTranslated ( ox, oy, oz );

glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  1.00 );

for ( double angle = DegreesToRadians ( anglefrom ); angle < DegreesToRadians ( angleto ); angle += 0.1 )
    glVertex3f ( cos ( angle ) * r, sin ( angle ) * r,  1.00 );
                                       // clean finish
glVertex3f ( cos ( DegreesToRadians ( angleto ) ) * r, sin ( DegreesToRadians ( angleto ) ) * r,  1.00 );

glEnd();

glTranslated ( -ox, -oy, -oz );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawBrightness ( double ox, double oy, double oz, double r )
{
//GLPolygonSmoothOn   ();

glTranslated ( ox, oy, oz );

glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  1.00 );

for ( double angle = 0; angle < TwoPi; angle += 0.1 )
    glVertex3f ( cos ( angle ) * r, sin ( angle ) * r,  1.00 );
                                       // clean finish
glVertex3f ( cos ( (double) 0 ) * r, sin ( (double) 0 ) * r,  1.00 );

glEnd();


for ( int a = 0; a < 360; a += 45 ) {
    glRotated ( 45, 0, 0, 1 );
    DrawRectangle ( 1.28 * r, 0, 0, 0.22 * r, 0.12 * r, 0 );
    }

//glRotated ( 45, 0, 0, 1 );

glTranslated ( -ox, -oy, -oz );

//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawContrast ( double ox, double oy, double oz, double r )
{
//GLPolygonSmoothOn   ();

glTranslated ( ox, oy, oz );

glBegin ( GL_QUAD_STRIP );

for ( double angle = DegreesToRadians ( -90 ); angle < DegreesToRadians ( 90 ); angle += 0.1 ) {
    glVertex3f ( cos ( angle ) * r * 0.75, sin ( angle ) * r * 0.75,  1.00 );
    glVertex3f ( cos ( angle ) * r, sin ( angle ) * r,  1.00 );
    }
                                        // clean finish
glVertex3f ( cos ( DegreesToRadians ( 90 ) ) * r * 0.75, sin ( DegreesToRadians ( 90 ) ) * r * 0.75,  1.00 );
glVertex3f ( cos ( DegreesToRadians ( 90 ) ) * r, sin ( DegreesToRadians ( 90 ) ) * r,  1.00 );

glEnd();


glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  1.00 );

for ( double angle = DegreesToRadians ( 90 ); angle < DegreesToRadians ( 270 ); angle += 0.1 )
    glVertex3f ( cos ( angle ) * r, sin ( angle ) * r,  1.00 );
                                       // clean finish
glVertex3f ( cos ( DegreesToRadians ( 270 ) ) * r, sin ( DegreesToRadians ( 270 ) ) * r,  1.00 );

glEnd();


glTranslated ( -ox, -oy, -oz );

//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawPlus ( double ox, double oy, double oz, double l )
{
//GLPolygonSmoothOn   ();

glTranslated ( ox, oy, oz );

glBegin ( GL_QUADS );

glVertex3d ( -l, -l * 0.25,  1.00 );
glVertex3d ( -l,  l * 0.25,  1.00 );
glVertex3d (  l,  l * 0.25,  1.00 );
glVertex3d (  l, -l * 0.25,  1.00 );

glVertex3d ( -l * 0.25, -l * 0.375 + 0.3125,  1.00 );
glVertex3d ( -l * 0.25,  l * 0.375 + 0.3125,  1.00 );
glVertex3d (  l * 0.25,  l * 0.375 + 0.3125,  1.00 );
glVertex3d (  l * 0.25, -l * 0.375 + 0.3125,  1.00 );

glVertex3d ( -l * 0.25, -l * 0.375 - 0.3125,  1.00 );
glVertex3d ( -l * 0.25,  l * 0.375 - 0.3125,  1.00 );
glVertex3d (  l * 0.25,  l * 0.375 - 0.3125,  1.00 );
glVertex3d (  l * 0.25, -l * 0.375 - 0.3125,  1.00 );

glEnd();

//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::DrawArrow ( double ox, double oy, double oz, double rectl, double recth, double headl, double headh, double angle )
{
//GLSmoothEdgesOn ();

glTranslated ( ox, oy, oz );
glRotated    ( angle, 0, 0, 1 );


double              l1              = - ( rectl + headl ) / 2;
double              l2              = l1 + rectl;

glBegin ( GL_QUADS );
glVertex3d ( l1, -recth,  1.00 );
glVertex3d ( l1,  recth,  1.00 );
glVertex3d ( l2,  recth,  1.00 );
glVertex3d ( l2, -recth,  1.00 );
glEnd();


glBegin ( GL_TRIANGLES );
glVertex3d ( l2,         -headh,  1.00 );
glVertex3d ( l2,          headh,  1.00 );
glVertex3d ( l2 + headl,  0.00,   1.00 );
glEnd();


glRotated    ( -angle, 0, 0, 1 );
glTranslated ( -ox, -oy, -oz );

//GLSmoothEdgesOff ();
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DCross ( const TPointFloat& p, GLfloat size, GLfloat thick, bool depthoff )
{
GLSmoothEdgesOn ();
GLColoringOn    ();

if ( depthoff )                         // sometimes useful to not affect the depth buffer
    GLWriteDepthOff ();

glLineWidth ( thick );


glTranslatef ( p.X, p.Y, p.Z );

//glColor4f ( 0, 0, 0, 1 );


glBegin ( GL_LINES );
glVertex3f ( -size, 0, 0 );
glVertex3f (  size, 0, 0 );
glVertex3f ( 0, -size, 0 );
glVertex3f ( 0,  size, 0 );
glVertex3f ( 0, 0, -size );
glVertex3f ( 0, 0,  size );
glEnd ();


glTranslatef ( -p.X, -p.Y, -p.Z );


if ( depthoff )
    GLWriteDepthOn ();

GLSmoothEdgesOff ();
GLColoringOff    ();

glLineWidth ( 1 );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DWireFrame ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat thick )
{
//GLLinesModeOn       ( false );

glLineWidth ( thick );


if      ( xmin == xmax ) {
    glBegin ( GL_LINE_LOOP );
    glVertex3f ( xmin, ymin, zmin ); glVertex3f ( xmin, ymin, zmax );
    glVertex3f ( xmin, ymax, zmax ); glVertex3f ( xmin, ymax, zmin );
    glEnd();
    }
else if ( ymin == ymax ) {
    glBegin ( GL_LINE_LOOP );
    glVertex3f ( xmin, ymin, zmin ); glVertex3f ( xmin, ymin, zmax );
    glVertex3f ( xmax, ymin, zmax ); glVertex3f ( xmax, ymin, zmin );
    glEnd();
    }
else {
    glBegin ( GL_LINE_LOOP );
    glVertex3f ( xmin, ymin, zmin ); glVertex3f ( xmin, ymax, zmin );
    glVertex3f ( xmax, ymax, zmin ); glVertex3f ( xmax, ymin, zmin );
    glEnd();
    }


//GLLinesModeOff      ( false );

glLineWidth ( 1 );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DBox ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat thick )
{
glLineWidth ( thick );


glBegin ( GL_LINE_LOOP );
glVertex3f ( xmin, ymin, zmin ); glVertex3f ( xmin, ymin, zmax );
glVertex3f ( xmin, ymax, zmax ); glVertex3f ( xmin, ymax, zmin );
glEnd();

glBegin ( GL_LINE_LOOP );
glVertex3f ( xmax, ymin, zmin ); glVertex3f ( xmax, ymin, zmax );
glVertex3f ( xmax, ymax, zmax ); glVertex3f ( xmax, ymax, zmin );
glEnd();

glBegin ( GL_LINES );
glVertex3f ( xmin, ymin, zmin ); glVertex3f ( xmax, ymin, zmin );
glVertex3f ( xmin, ymin, zmax ); glVertex3f ( xmax, ymin, zmax );
glVertex3f ( xmin, ymax, zmax ); glVertex3f ( xmax, ymax, zmax );
glVertex3f ( xmin, ymax, zmin ); glVertex3f ( xmax, ymax, zmin );
glEnd();


glLineWidth ( 1 );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DFrame ( GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax, GLfloat width )
{
if      ( xmin == xmax ) {
    glBegin ( GL_QUADS );
    glVertex3f ( xmin,          ymin - width,   zmin - width );
    glVertex3f ( xmin,          ymin - width,   zmax + width );
    glVertex3f ( xmin,          ymin,           zmax + width );
    glVertex3f ( xmin,          ymin,           zmin - width );

    glVertex3f ( xmin,          ymax,           zmin - width );
    glVertex3f ( xmin,          ymax,           zmax + width );
    glVertex3f ( xmin,          ymax + width,   zmax + width );
    glVertex3f ( xmin,          ymax + width,   zmin - width );

    glVertex3f ( xmin,          ymin,           zmin - width );
    glVertex3f ( xmin,          ymax,           zmin - width );
    glVertex3f ( xmin,          ymax,           zmin         );
    glVertex3f ( xmin,          ymin,           zmin         );

    glVertex3f ( xmin,          ymin,           zmax + width );
    glVertex3f ( xmin,          ymax,           zmax + width );
    glVertex3f ( xmin,          ymax,           zmax         );
    glVertex3f ( xmin,          ymin,           zmax         );
    glEnd();
    }
else if ( ymin == ymax ) {
    glBegin ( GL_QUADS );
    glVertex3f ( xmin - width,  ymin,           zmin - width );
    glVertex3f ( xmin - width,  ymin,           zmax + width );
    glVertex3f ( xmin,          ymin,           zmax + width );
    glVertex3f ( xmin,          ymin,           zmin - width );

    glVertex3f ( xmax,          ymin,           zmin - width );
    glVertex3f ( xmax,          ymin,           zmax + width );
    glVertex3f ( xmax + width,  ymin,           zmax + width );
    glVertex3f ( xmax + width,  ymin,           zmin - width );

    glVertex3f ( xmin,          ymin,           zmin - width );
    glVertex3f ( xmax,          ymin,           zmin - width );
    glVertex3f ( xmax,          ymin,           zmin         );
    glVertex3f ( xmin,          ymin,           zmin         );

    glVertex3f ( xmin,          ymin,           zmax + width );
    glVertex3f ( xmax,          ymin,           zmax + width );
    glVertex3f ( xmax,          ymin,           zmax         );
    glVertex3f ( xmin,          ymin,           zmax         );
    glEnd();
    }
else {
    glBegin ( GL_QUADS );
    glVertex3f ( xmin - width,  ymin - width,   zmin         );
    glVertex3f ( xmin - width,  ymax + width,   zmin         );
    glVertex3f ( xmin,          ymax + width,   zmin         );
    glVertex3f ( xmin,          ymin - width,   zmin         );

    glVertex3f ( xmax,          ymin - width,   zmin         );
    glVertex3f ( xmax,          ymax + width,   zmin         );
    glVertex3f ( xmax + width,  ymax + width,   zmin         );
    glVertex3f ( xmax + width,  ymin - width,   zmin         );

    glVertex3f ( xmin,          ymin - width,   zmin         );
    glVertex3f ( xmax,          ymin - width,   zmin         );
    glVertex3f ( xmax,          ymin,           zmin         );
    glVertex3f ( xmin,          ymin,           zmin         );

    glVertex3f ( xmin,          ymax + width,   zmin         );
    glVertex3f ( xmax,          ymax + width,   zmin         );
    glVertex3f ( xmax,          ymax,           zmin         );
    glVertex3f ( xmin,          ymax,           zmin         );
    glEnd();
    }
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DCylinder  ( const TPointFloat& p1, const TPointFloat& p2, GLfloat radius )
{
TVector3Float       p1p2 ( p2 - p1 );
TVector3Float       ortho1;
TVector3Float       ortho2;
TPointFloat         c;


p1p2.GetOrthogonalVectors ( ortho1, ortho2 );


glTranslated ( p1.X, p1.Y, p1.Z );


                                        // body
glBegin ( GL_QUAD_STRIP );

for ( int anglei = 0; anglei <= 64; anglei++ ) {

    c   = ( ortho1 * cos ( anglei * TwoPi / 64 ) + ortho2 * sin ( anglei * TwoPi / 64 ) ) * radius;
    glVertex3fv ( c );

    c  += p1p2;
    glVertex3fv ( c );
    }

glEnd();


                                        // one cap - we are already centered
glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  0 );

for ( int anglei = 0; anglei <= 64; anglei++ ) {

    c   = ( ortho1 * cos ( anglei * TwoPi / 64 ) + ortho2 * sin ( anglei * TwoPi / 64 ) ) * radius;
    glVertex3fv ( c );
    }

glEnd();


                                        // the other cap - do a relative shift
glTranslated ( p1p2.X, p1p2.Y, p1p2.Z );


glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  0 );

for ( int anglei = 0; anglei <= 64; anglei++ ) {

    c   = ( ortho1 * cos ( anglei * TwoPi / 64 ) + ortho2 * sin ( anglei * TwoPi / 64 ) ) * radius;
    glVertex3fv ( c );
    }

glEnd();


                                        // going back to original position
glTranslated ( -p2.X, -p2.Y, -p2.Z );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DPlane ( const TPointFloat& p, const TPointFloat& normal, GLfloat radius )
{
TVector3Float       ortho1;
TVector3Float       ortho2;
TPointFloat         c;


normal.GetOrthogonalVectors ( ortho1, ortho2 );


glTranslated ( p.X, p.Y, p.Z );

/*                                      // squared version
glBegin ( GL_QUAD_STRIP );

c   = ( - ortho1 - ortho2 ) * radius;
glVertex3fv ( c );

c   = ( - ortho1 + ortho2 ) * radius;
glVertex3fv ( c );

c   = (   ortho1 - ortho2 ) * radius;
glVertex3fv ( c );

c   = (   ortho1 + ortho2 ) * radius;
glVertex3fv ( c );

glEnd();
*/

                                        // circular version
glBegin ( GL_TRIANGLE_FAN );

glVertex3d (  0,  0,  0 );

for ( int anglei = 0; anglei <= 64; anglei++ ) {

    c   = ( ortho1 * cos ( anglei * TwoPi / 64 ) + ortho2 * sin ( anglei * TwoPi / 64 ) ) * radius;
    glVertex3fv ( c );
    }

glEnd();


                                        // going back to original position
glTranslated ( -p.X, -p.Y, -p.Z );
}


//----------------------------------------------------------------------------
void    TGLPrimitives::Draw3DArrow  (   const TPointFloat&  p,              const TVector3Float&    d, 
                                        double              shaftradius,    double                  headlength
                                    )
{
TVector3Float       ortho1;
TVector3Float       ortho2;
TPointFloat         c;


d.GetOrthogonalVectors ( ortho1, ortho2 );


glTranslated ( p.X, p.Y, p.Z );


double              length          = d.Norm ();

if ( shaftradius <= 0 )
    shaftradius     = length * 0.020;   // estimate shaft radius

if ( headlength <= 0 )
    headlength      = length * 0.20;    // estimate head length

                                        // shaft length is vector - head
TVector3Float       shaftd          = d * AtLeast ( 0.0, ( length - headlength ) / NonNull ( length ) );

                                        // estimate head radius from head length
double              headradius      = headlength * 0.20;


                                        // shaft
glBegin ( GL_QUAD_STRIP );

for ( int anglei = 0; anglei <= 16; anglei++ ) {

    c   = ( ortho1 * cos ( anglei * TwoPi / 16 ) + ortho2 * sin ( anglei * TwoPi / 16 ) ) * shaftradius;
    glVertex3fv ( c );

    c  += shaftd;
    glVertex3fv ( c );
    }

glEnd();


                                        // head: conic part
glBegin ( GL_TRIANGLE_FAN );

glVertex3d ( d.X, d.Y, d.Z );

for ( int anglei = 0; anglei <= 16; anglei++ ) {

    c   = shaftd + ( ortho1 * cos ( anglei * TwoPi / 16 ) + ortho2 * sin ( anglei * TwoPi / 16 ) ) * headradius;
    glVertex3fv ( c );
    }

glEnd();


                                        // head: joint between shaft and conic part
glBegin ( GL_QUAD_STRIP );

for ( int anglei = 0; anglei <= 16; anglei++ ) {

    c   = shaftd + ( ortho1 * cos ( anglei * TwoPi / 16 ) + ortho2 * sin ( anglei * TwoPi / 16 ) ) * shaftradius;
    glVertex3fv ( c );

    c   = shaftd + ( ortho1 * cos ( anglei * TwoPi / 16 ) + ortho2 * sin ( anglei * TwoPi / 16 ) ) * headradius;
    glVertex3fv ( c );
    }

glEnd();


                                        // going back to original position
glTranslated ( -p.X, -p.Y, -p.Z );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
