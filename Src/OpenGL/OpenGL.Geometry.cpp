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

#include    "OpenGL.Geometry.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // specific bodies according to type
void    TGLCoordinates<GLint>::GLize ( int /*param*/ )
{
glVertex4iv ( (const GLint *) &X );
}


void    TGLCoordinates<GLfloat>::GLize ( int /*param*/ )
{
glVertex4fv ( (const GLfloat *) &X );
}


void    TGLCoordinates<GLdouble>::GLize ( int /*param*/ )
{
glVertex4dv ( (const GLdouble *) &X );
}


template <class TypeD>
void    TGLCoordinates<TypeD>::Show ( char *title )
{
ShowValues ( B, "ffff", (double) X, (double) Y, (double) Z, (double) W, StringIsEmpty ( title ) ? "Point" : title );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TGLMatrix::GLize ( int param )
{
                                        // we can skip that one...
if ( IsIdentity () )
    return;


switch ( param ) {
                                        // apply to current transformation
    case ApplyToCurrent :   
        
        glMultMatrixd ( Matrix );
        break;

    case ReplaceProjection:
                                        // overwrite matrix
        glMatrixMode  ( GL_PROJECTION );
        glLoadMatrixd ( Matrix );
        glMatrixMode  ( GL_MODELVIEW );
        break;

    case ReplaceTexture:

        glMatrixMode  ( GL_TEXTURE );
        glLoadMatrixd ( Matrix );
        glMatrixMode  ( GL_MODELVIEW );
        break;

    case ReplaceModelView:

        glMatrixMode ( GL_MODELVIEW );
        glLoadMatrixd ( Matrix );
        break;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLDepthRange::TGLDepthRange ()
{
UserShift   = 0;
NearPlane   = 0;
FarPlane    = 1;
}


void    TGLDepthRange::GLize ( int /*param*/ )
{                                       // apply current depth
glDepthRange ( NearPlane, FarPlane );
}


void    TGLDepthRange::unGLize ()
{                                       // reset OpenGL, and plane variables
NearPlane   = 0;
FarPlane    = 1;

glDepthRange ( 0, 1 );
}


void    TGLDepthRange::ShiftCloser ( double shift )
{                                       // bring close to eye plane (0)
NearPlane   -= shift;
FarPlane    -= shift;
}


void    TGLDepthRange::ShiftFurther ( double shift )
{
NearPlane   += shift;
FarPlane    += shift;
}


void    TGLDepthRange::DepthFromMultipass ( int pass, int maxpass )
{
if ( maxpass <= 1 ) {
    NearPlane   = 0;
    FarPlane    = 1;
    return;
    }

double              length          = 1 - (double) ( maxpass - 1 ) * MultipasDepthShift;

NearPlane   = (double) ( maxpass - pass - 1 )  * MultipasDepthShift;
FarPlane    = NearPlane + length;

//DBGV2 ( NearPlane, FarPlane, "near far" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
    TGLClipPlane::TGLClipPlane ()
{
Plane       = GL_CLIP_PLANE0;
Mode        = ClipPlaneNone;
ClearVirtualMemory ( Equation, 4 * sizeof ( *Equation ) );
Equation[0] = -1;
LimitInf    = LimitSup  = 0;
}


    TGLClipPlane::TGLClipPlane ( GLenum plane, GLdouble *equ, GLdouble linf, GLdouble lsup )
{
Plane       = plane;
Mode        = ClipPlaneNone;
memcpy ( Equation, equ, 4 * sizeof ( *Equation ) );
LimitInf    = linf;
LimitSup    = lsup;
}


    TGLClipPlane::TGLClipPlane ( GLenum plane, GLdouble equ0, GLdouble equ1, GLdouble equ2, GLdouble equ3, GLdouble linf, GLdouble lsup )
{
Plane       = plane;
Mode        = ClipPlaneNone;
Equation[0] = equ0; Equation[1] = equ1; Equation[2] = equ2; Equation[3] = equ3;
LimitInf    = linf;
LimitSup    = lsup;
}


void        TGLClipPlane::GLize ( int param )
{
if ( ! Mode )
    return;

if ( param & InvertedClipPlane )
    Invert ();

if ( param & StepClipPlane ) {
    Equation[4] = Equation[3];
    Equation[3] = GetPosition ( 1.0 );
    }


glEnable  ( Plane );

glClipPlane ( Plane, Equation );


if ( param & StepClipPlane )
    Equation[3] = Equation[4];

if ( param & InvertedClipPlane )
    Invert ();
}


void        TGLClipPlane::unGLize ()
{
glDisable ( Plane );
}

                                        // can return a stepped value
GLdouble    TGLClipPlane::GetPosition ( double step )
{
if ( step == 0 )
    return Equation[3];

                                        // step & clip the value
return  Clip ( RoundTo ( Equation[3], step ), LimitInf, LimitSup );
}

                                        // return an always correctly directed value
GLdouble    TGLClipPlane::GetAbsPosition ( double step )
{
return  Mode == ClipPlaneBackward ? - GetPosition ( step ) : GetPosition ( step );
}


void        TGLClipPlane::Set ( int mode )
{
if      ( mode == ClipPlaneNone     )   SetNone     ();
else if ( mode == ClipPlaneForward  )   SetForward  ();
else if ( mode == ClipPlaneBackward )   SetBackward ();
}


void        TGLClipPlane::SetNone ()
{
if ( Mode == ClipPlaneNone )
    return;

if ( Mode == ClipPlaneBackward )
    Invert();                           // leave the equations as in forward mode

Mode    = ClipPlaneNone;
}


void        TGLClipPlane::SetForward ()
{
if ( Mode == ClipPlaneBackward )
    Invert();

Mode    = ClipPlaneForward;
}


void        TGLClipPlane::SetBackward ()
{
if ( Mode == ClipPlaneForward || Mode == ClipPlaneNone ) {
    Mode    = ClipPlaneForward;
    Invert();
    }

Mode    = ClipPlaneBackward;
}


void        TGLClipPlane::Invert ()
{
if ( Mode == ClipPlaneBackward )    Mode = ClipPlaneForward;
else                                Mode = ClipPlaneBackward;

                                        // revert equation
Equation[0] = -Equation[0];
Equation[1] = -Equation[1];
Equation[2] = -Equation[2];
Equation[3] = -Equation[3];

                                        // invert also the limits
GLdouble    linf    = LimitInf;
LimitInf    = -LimitSup;
LimitSup    = -linf;
}


void        TGLClipPlane::Shift ( GLdouble value )
{
Equation[3] = Clip ( Equation[3] + value, LimitInf, LimitSup );
}


void        TGLClipPlane::SetPosition ( GLdouble pos )
{
Equation[3] = Clip ( pos, LimitInf, LimitSup );
}


void        TGLClipPlane::SetAbsPosition ( GLdouble pos )
{
SetPosition ( Mode == ClipPlaneBackward ? - pos : pos );
}


void        TGLClipPlane::SetEquation ( GLdouble equ0, GLdouble equ1, GLdouble equ2, GLdouble equ3 )
{
Equation[0] = equ0; 
Equation[1] = equ1; 
Equation[2] = equ2; 
Equation[3] = Clip ( equ3, LimitInf, LimitSup );
}


GLdouble    TGLClipPlane::DirectionRatio ( GLdouble x, GLdouble y, GLdouble *model, bool normalize )
{
TGLCoordinates<GLfloat> p;
                                        // copy current direction vector
p.X = Equation[0];
p.Y = Equation[1];
p.Z = Equation[2];

                                        // transform to world coordinates
GLApplyModelMatrix ( p, model );

                                        // normalizing the projected direction
                                        // makes the move more intuitive (f.ex. if vector points toward screen)
if ( normalize ) {
    double          n       = NonNull ( sqrt ( p.X * p.X + p.Y * p.Y ) );

    p.X    /= n;
    p.Y    /= n;
    }
                                        // scalar product
return  p.X * x + p.Y * y;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
