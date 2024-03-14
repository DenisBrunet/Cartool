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

#include    "Math.TMatrix44.h"
#include    "Geometry.TPoint.h"
#include    "OpenGL.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD>
class   TGLCoordinates :  public TGLObject
{
public:
                    TGLCoordinates ()                                           { Reset (); }
                    TGLCoordinates ( TypeD x, TypeD y, TypeD z )                { X = x; Y = y; Z = z; W = 1; }
                    TGLCoordinates ( TypeD x, TypeD y, TypeD z, TypeD w )       { X = x; Y = y; Z = z; W = w; }

                                        // public, we actually want to change them!
    TypeD           X;
    TypeD           Y;
    TypeD           Z;
    TypeD           W;


    void            Set ( TypeD x, TypeD y, TypeD z )           { X = x;    Y = y;    Z = z;    W = 1; }
    void            Set ( TypeD x, TypeD y, TypeD z, TypeD w )  { X = x;    Y = y;    Z = z;    W = w; }
    void            Set ( TPointDouble &p )                     { X = p.X;  Y = p.Y;  Z = p.Z;  W = 1; }
    void            Set ( TPointFloat  &p )                     { X = p.X;  Y = p.Y;  Z = p.Z;  W = 1; }

    void            Reset ()                                    { X = Y = Z = 0; W = 1; }
    void            Show  ( char *title = 0 );

    double          Norm ()                                     { return sqrt ( X * X + Y * Y + Z * Z ); }
    void            Normalize ()                                { double n = Norm (); if ( n != 0 ) { X /= n; Y /= n; Z /= n; } }

    void            GLize   ( int param = 0 );


                    operator TypeD* ()      { return (TypeD *) &X; }    // cast
                    operator bool   ()      { return X || Y || Z; }


    TGLCoordinates<TypeD>   operator    *=  ( const double          op2[] )     { X *= op2[0]; Y *= op2[1]; Z *= op2[2]; W *= op2[3];   return *this; }
    TGLCoordinates<TypeD>   operator    /=  ( const double          op2[] )     { X /= op2[0]; Y /= op2[1]; Z /= op2[2]; W /= op2[3];   return *this; }
    TGLCoordinates<TypeD>   operator    *=  ( double                op2   )     { X *= op2;    Y *= op2;    Z *= op2;                   return *this; }
    TGLCoordinates<TypeD>   operator    /=  ( double                op2   )     { X /= op2;    Y /= op2;    Z /= op2;                   return *this; }
    TGLCoordinates<TypeD>   operator    +=  ( const TPointDouble&   op2   )     { X += op2.X;  Y += op2.Y;  Z += op2.Z;                 return *this; }
    TGLCoordinates<TypeD>   operator    +=  ( const TPointFloat&    op2   )     { X += op2.X;  Y += op2.Y;  Z += op2.Z;                 return *this; }

};


//----------------------------------------------------------------------------
                                        // TGLMatrix GLize params
enum    {
        ApplyToCurrent,
        ReplaceProjection,
        ReplaceModelView,
        ReplaceTexture
        };


class   TGLMatrix : public TGLObject,
                    public TMatrix44    // the math stuff is actually there
{
public:
                                        // copying current OpenGL matrices
    void            CopyProjection ()           { glGetDoublev ( GL_PROJECTION_MATRIX, Matrix ); }
    void            CopyModelView  ()           { glGetDoublev ( GL_MODELVIEW_MATRIX,  Matrix ); }
    void            CopyTexture    ()           { glGetDoublev ( GL_TEXTURE_MATRIX,    Matrix ); }


     void           GLize   ( int param = ApplyToCurrent );

};


//----------------------------------------------------------------------------

                                        // these are very arbitrary values, and may depend on OpenGL implementation
constexpr double    SmallDepthShift     = 0.005;
constexpr double    MultipasDepthShift  = 0.0005;


class   TGLDepthRange:  public TGLObject
{
public:
                    TGLDepthRange ();


    GLclampd        UserShift;          // for user's need, not directly used

    void            ShiftCloser         ( double shift = SmallDepthShift );
    void            ShiftFurther        ( double shift = SmallDepthShift );
    void            DepthFromMultipass  ( int pass, int maxpass );
    bool            IsShifted           ()  const                   { return NearPlane != 0 || FarPlane != 1; }

    void            GLize   ( int param = 0 );
    void            unGLize ();


protected:
    GLclampd        NearPlane;
    GLclampd        FarPlane;
};


//----------------------------------------------------------------------------
enum    ClipPlaneDirection
        {
        ClipPlaneNone,
        ClipPlaneBackward,
        ClipPlaneForward,
        NumClipPlane,

        ClipPlaneOn         = ClipPlaneBackward,
        NumClipPlaneSym     = ClipPlaneBackward + 1,
        };

enum    {
        CurrentClipPlane    = 0x0,
        InvertedClipPlane   = 0x1,
        StepClipPlane       = 0x2
        };


class   TGLClipPlane :  public TGLObject
{
public:
                    TGLClipPlane ();
                    TGLClipPlane ( GLenum plane, GLdouble *equ, GLdouble linf, GLdouble lsup );
                    TGLClipPlane ( GLenum plane, GLdouble equ0, GLdouble equ1, GLdouble equ2, GLdouble equ3, GLdouble linf, GLdouble lsup );


    void            GLize   ( int param = CurrentClipPlane );
    void            unGLize ();


    ClipPlaneDirection  GetMode ()      { return Mode; }
    GLdouble            GetLimitInf ()  { return LimitInf; }
    GLdouble            GetLimitSup ()  { return LimitSup; }
    GLdouble            GetPosition    ( double step = 0 );
    GLdouble            GetAbsPosition ( double step = 0 );
    GLdouble*           GetVector   ()  { return Equation; }

    bool            IsNone      ()  { return Mode == ClipPlaneNone;     }
    bool            IsForward   ()  { return Mode == ClipPlaneForward;  }
    bool            IsBackward  ()  { return Mode == ClipPlaneBackward; }


    void            Set         ( int mode );
    void            SetNone     ();
    void            SetForward  ();
    void            SetBackward ();
    void            Invert      ();

    void            Shift           ( GLdouble value );
    void            SetPosition     ( GLdouble pos );
    void            SetAbsPosition  ( GLdouble pos );
    void            SetEquation     ( GLdouble equ0, GLdouble equ1, GLdouble equ2, GLdouble equ3 );

    GLdouble        DirectionRatio ( GLdouble x, GLdouble y, GLdouble *model, bool normalize );


    operator        int     ()      { return Mode; }
    operator        bool    ()      { return Mode; }


protected:
    GLenum          Plane;
    ClipPlaneDirection  Mode;           // On forward, On backward
    GLdouble        Equation[ 5 ];      // plane equation + 1 as buffer
    GLdouble        LimitInf;
    GLdouble        LimitSup;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
