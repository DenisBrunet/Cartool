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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
class   TGLLight :  public TGLObject
{
public:
                    TGLLight ()         { LightNumber = GL_LIGHT0; }
                    TGLLight ( GLenum ln,
                               GLdouble ar, GLdouble ag, GLdouble ab, GLdouble aa,
                               GLdouble dr, GLdouble dg, GLdouble db, GLdouble da,
                               GLdouble sr, GLdouble sg, GLdouble sb, GLdouble sa,
                               GLdouble px, GLdouble py, GLdouble pz, GLdouble pw )
                                        { LightNumber   = ln;
                                          SetAmbient    ( ar, ag, ab, aa );
                                          SetDiffuse    ( dr, dg, db, da );
                                          SetSpecular   ( sr, sg, sb, sa );
                                          SetPosition   ( px, py, pz, pw ); }


    void            GLize   ( int param = 0 );
    void            unGLize ()                  { glDisable ( LightNumber ); }


    TGLColor<TypeD>        &GetAmbient()        { return Ambient; }
    TGLColor<TypeD>        &GetDiffuse()        { return Diffuse; }
    TGLColor<TypeD>        &GetSpecular()       { return Specular; }
    TGLCoordinates<TypeD>  &GetPosition()       { return Position; }

    void            SetAmbient   ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Ambient .Set ( r, g, b, a ); }
    void            SetDiffuse   ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Diffuse .Set ( r, g, b, a ); }
    void            SetSpecular  ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Specular.Set ( r, g, b, a ); }
    void            SetAlpha     ( GLdouble a )                                     { Ambient.SetAlpha ( a ); Diffuse.SetAlpha ( a ); Specular.SetAlpha ( a ); Emission.SetAlpha ( a ); }
    void            SetPosition  ( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { Position.Set ( x, y, z, w ); }


protected:
    GLenum          LightNumber;

    TGLColor<TypeD> Ambient;
    TGLColor<TypeD> Diffuse;
    TGLColor<TypeD> Specular;

    TGLCoordinates<TypeD>   Position;
};


//----------------------------------------------------------------------------

template <class TypeD>
class   TGLMaterial :  public TGLObject
{
public:
                    TGLMaterial ()          { Side = GL_FRONT_AND_BACK; Shininess = 0; }
                    TGLMaterial ( GLenum sd,
                                  GLdouble ar, GLdouble ag, GLdouble ab, GLdouble aa,
                                  GLdouble dr, GLdouble dg, GLdouble db, GLdouble da,
                                  GLdouble sr, GLdouble sg, GLdouble sb, GLdouble sa,
                                  GLdouble er, GLdouble eg, GLdouble eb, GLdouble ea,
                                  GLdouble sh )
                                            { Side          = sd;
                                              SetAmbient    ( ar, ag, ab, aa );
                                              SetDiffuse    ( dr, dg, db, da );
                                              SetSpecular   ( sr, sg, sb, sa );
                                              SetEmission   ( er, eg, eb, ea );
                                              Shininess     = sh; }


    void            GLize   ( int param = 0 );


    TGLColor<TypeD>        &GetAmbient()        { return Ambient; }
    TGLColor<TypeD>        &GetDiffuse()        { return Diffuse; }
    TGLColor<TypeD>        &GetSpecular()       { return Specular; }
    TGLColor<TypeD>        &GetEmission()       { return Emission; }
    TypeD                   GetShininess()      { return Shininess; }

    void            SetSide      ( GLenum sd )                                      { Side = sd; }
    void            SetAmbient   ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Ambient .Set ( r, g, b, a ); }
    void            SetDiffuse   ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Diffuse .Set ( r, g, b, a ); }
    void            SetSpecular  ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Specular.Set ( r, g, b, a ); }
    void            SetEmission  ( GLdouble r, GLdouble g, GLdouble b, GLdouble a ) { Emission.Set ( r, g, b, a ); }
    void            SetShininess ( GLdouble sh )                                    { Shininess = sh; }
    void            SetAlpha     ( GLdouble a )                                     { Ambient.SetAlpha ( a ); Diffuse.SetAlpha ( a ); Specular.SetAlpha ( a ); Emission.SetAlpha ( a ); }


protected:
    GLenum          Side;

    TGLColor<TypeD> Ambient;
    TGLColor<TypeD> Diffuse;
    TGLColor<TypeD> Specular;
    TGLColor<TypeD> Emission;

    TypeD           Shininess;
};


//----------------------------------------------------------------------------

template <class TypeD>
class   TGLFog :  public TGLObject
{
public:
                    TGLFog ()
                        { FogMode   = GL_LINEAR;
                          FogStart  =  1000;
                          FogEnd    = -1000;
                          FogDensity = 1; }

                                        // this constructor assumes a linear mode
                    TGLFog ( TypeD fs, TypeD fe,
                             TypeD fr, TypeD fg, TypeD fb, TypeD fa )
                        { FogMode   = GL_LINEAR;
                          FogColor.Set ( fr, fg, fb, fa );
                          FogStart  = fs;
                          FogEnd    = fe;
                          FogDensity = 1; }

                                        // this constructor assumes an exp2 mode
                    TGLFog ( TypeD fd,
                             TypeD fr, TypeD fg, TypeD fb, TypeD fa )
                        { FogMode   = GL_EXP2;
                          FogColor.Set ( fr, fg, fb, fa );
                          FogStart  =  1000;
                          FogEnd    = -1000;
                          FogDensity = fd; }


    void            GLize   ( int param = 0 );
    void            unGLize ()                  { glDisable ( GL_FOG ); }


    TypeD           GetStart()                  { return FogStart; }
    TypeD           GetEnd()                    { return FogEnd; }
    TypeD           GetDensity()                { return FogDensity; }

    void            SetStart ( TypeD fs )       { FogStart = fs; }
    void            SetEnd ( TypeD fe )         { FogEnd = fe; }
    void            SetDensity ( TypeD fd )     { FogDensity = fd; }
    void            SetColor ( TypeD r, TypeD g, TypeD b, TypeD a )     { FogColor.Set ( r, g, b, a ); }
    void            SetColor ( TGLColor<TypeD> &col )                   { FogColor = col; }


protected:
    int             FogMode;
    TGLColor<TypeD> FogColor;
    TypeD           FogStart;
    TypeD           FogEnd;
    TypeD           FogDensity;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
