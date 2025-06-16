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

#include    <GL\gl.h>
#include    <GL\glu.h>
#include    "khrplatform.h"             // Copyright 2008-2018 The Khronos Group Inc.
#include    "glext.h"                   // Copyright 2013-2020 The Khronos Group Inc. - See  registry.khronos.org/OpenGL/index_gl.php

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Parent class for all OpenGL objects
class   TGLObject
{
public:

    virtual        ~TGLObject()                 {}

                                        // A miniminalistic interface for all objects
    virtual void    GLize   ( int param = 0 )   {};     // Apply  / load   current OpenGL object
    virtual void    GLize   ( double value  )   {};     // Apply  / load   current OpenGL object
    virtual void    unGLize ()                  {};     // Revert / unload current OpenGL object
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Choosing the Pixel Format
                                                                                                            // while swapping Front/back, keep a copy in Back
constexpr DWORD GLPixelFormatDefaultFlags       = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
                                        // Parameters that seems to go through all drivers
constexpr BYTE  GLPixelFormatDefaultColorBits   = 32;
constexpr BYTE  GLPixelFormatDefaultDepthBits   = 24;
constexpr BYTE  GLPixelFormatDefaultStencilBits = 32;
constexpr BYTE  GLPixelFormatDefaultAccumBits   = 32;


enum    FormatWish
        {
                                        // symbolic
        FormatUseBest       = -1,
        FormatDontUse       = 0,
        FormatUse           = 1,
                                        // values
        Format1             = 1,
        Format2             = 2,
        Format4             = 4,
        Format8             = 8,
        Format16            = 16,
        Format24            = 24,
        Format32            = 32,
        Format48            = 48,
        Format64            = 64,
        Format128           = 128,
        Format192           = 192,
        Format256           = 256,
        };

FormatWish  ValueToFormatWish ( int v );


                                // Wrapping this OpenGL / Windows struct into a class
class   TGLPixelFormat  :   public PIXELFORMATDESCRIPTOR
{
public:
                    TGLPixelFormat ();


    void            Reset ();
    void            Set   ( DWORD   dwflags     = GLPixelFormatDefaultFlags, 
                            BYTE    colorbits   = GLPixelFormatDefaultColorBits, 
                            BYTE    depthbits   = GLPixelFormatDefaultDepthBits, 
                            BYTE    stencilbits = GLPixelFormatDefaultStencilBits, 
                            BYTE    accumbits   = GLPixelFormatDefaultAccumBits     );

    WORD            GetPfdSize ()                       { return    sizeof ( PIXELFORMATDESCRIPTOR ); }     // get the pixel format descriptor size only


    int             GetPixelFormat      ()      const   { return    PixelFormat; }
    int             GetColorBits        ()      const   { return    cColorBits; }
    int             GetDepthBits        ()      const   { return    cDepthBits; }
    int             GetStencilBits      ()      const   { return    cStencilBits; }
    int             GetAccumBits        ( bool computed = true )    const;

                                        // force no double buffer
    void            ClearDoubleBuffer   ()      { dwFlags  &= ~PFD_DOUBLEBUFFER; }


    bool            IsDetected          ()      const   { return    PixelFormat > 0; }
    bool            IsOpenGL            ()      const   { return    dwFlags & PFD_SUPPORT_OPENGL; }
    bool            IsWindow            ()      const   { return    dwFlags & PFD_DRAW_TO_WINDOW; }
    bool            IsBitmap            ()      const   { return    dwFlags & PFD_DRAW_TO_BITMAP; }
    bool            IsDoubleBuffer      ()      const   { return    dwFlags & PFD_DOUBLEBUFFER; }
    bool            IsPalette           ()      const   { return    iPixelType == PFD_TYPE_COLORINDEX; }
    bool            IsAcceleratedICD    ()      const   { return  ! ( dwFlags & PFD_GENERIC_FORMAT ) && ! ( dwFlags & PFD_GENERIC_ACCELERATED ); }  // Installable Client Driver: Hardware Accelerated
    bool            IsAcceleratedMCD    ()      const   { return    ( dwFlags & PFD_GENERIC_FORMAT ) &&   ( dwFlags & PFD_GENERIC_ACCELERATED ); }  // Microsoft Client Device or Direct Device Interface:  Microsoft Accelerated
    bool            IsSoftware          ()      const   { return    ( dwFlags & PFD_GENERIC_FORMAT ) && ! ( dwFlags & PFD_GENERIC_ACCELERATED ); }  // Microsoft Software, not accelerated
    bool            IsAccelerated       ()      const   { return    IsAcceleratedICD () || IsAcceleratedMCD (); }                                   // Either native (ICD) or Microsoft (MCD)


    int             GetNumPixelFormats      ( const HDC& hdc );
    bool            RetrievePixelFormat     ( const HDC& hdc, int i );
    int             ChooseBestPixelFormat   ( const HDC&      hdc, 
                                              FormatWish&     ask_colorbits,  FormatWish&     ask_depthbits,      FormatWish&     ask_stencilbits,
                                              FormatWish&     ask_accumbits,  FormatWish&     ask_doublebuffer,   FormatWish&     ask_accel       );

    bool            ChoosePixelFormat       ( const HDC& hdc );
    bool            SetPixelFormat          ( const HDC& hdc );


                    operator    bool    ()      const   { return    IsDetected (); }

protected:

    int             PixelFormat;
};


//----------------------------------------------------------------------------
                                        // Wrapping OpenGL Rendering Context operations
                                        // http://www.opengl.org/wiki/Platform_specifics:_Windows:
                                        // Don't forget to deallocate textures, display lists, VBOs, PBOs, FBOs, shaders, etc before destroying the GL context.

class   TGLRenderingContext :   public TGLObject
{
public:
                    TGLRenderingContext ();
                    TGLRenderingContext ( HDC hdc, TGLPixelFormat& GLpfd, bool forcedetectpfd = false );
                   ~TGLRenderingContext ();


    bool            IsOpen ()   const                   { return    hGLRC != 0; }


    bool            Reset   ();                     // unGLize + deleting context
    bool            Set     ( HDC hdc, TGLPixelFormat &GLpfd, bool forcedetectpfd = false );
    bool            Set     ( HDC hdc, TGLPixelFormat &GLpfd, DWORD dwFlags );

    void            GLize   ( HDC hdc, bool *success = 0 );
    void            unGLize ( bool *success = 0 );  // just unloading context without deleting it


                    operator    bool    ()  const       { return    IsOpen (); }


protected:
    HGLRC           hGLRC;              // handle to rendering context

    bool            InUsed;
};


//----------------------------------------------------------------------------

double  GLVersion           ();
void    GLDisplayVersion    ();
void    GLDisplayError      ();

                                        // returns the screen coordinates of the point through current transformation
void    GLGetScreenCoord ( GLfloat v[ 3 ] );
bool    GLGetScreenCoord ( GLfloat v[ 3 ], GLint* viewp, GLdouble* proj = 0, GLdouble* model = 0 );

                                        // returns the object coordinates at window coordinates through current transformation
bool    GLGetObjectCoord ( GLfloat v[ 3 ], GLint* viewp = 0, GLdouble* proj = 0, GLdouble* model = 0 );


//----------------------------------------------------------------------------

template <class TypeD> class    TGLCoordinates;

                                        // multiply coordinates with current model view matrix
void    GLApplyModelMatrix ( TGLCoordinates<GLfloat> &p,         GLdouble *model=0 );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Group commonly used function calls together by usage,
                                        // also renaming with more explicit names.
void    GL3DAutoTextureOn   ( const double* origin );   // set 3D implicit texture, generating the texture coordinates automatically
void    GL3DAutoTextureOff  ();
void    GL3DTextureOn       ();         // specifically enables 3D texture, in case of multiple textures are bound
void    GL3DTextureOff      ();
void    GLAlphaAboveOn      ( double alphathreshold );
void    GLAlphaAboveOff     ();
void    GLBlendOn           ();
void    GLBlendOff          ();
void    GLFogOn             ();
void    GLFogOff            ();
void    GLLinesModeOn       ( bool udpatefog = true );  // for drawing lines & boxes in 3D - default is to also turn off the fog
void    GLLinesModeOff      ( bool udpatefog = true );
void    GLLineSmoothOn      ();
void    GLLineSmoothOff     ();
void    GLLoad3DTexture     ( GLuint& textureid );  // load in memory
void    GLUnloadTexture     ( GLuint& textureid );  // remove from memory
void    GLNormalizeNormalOn ();
void    GLNormalizeNormalOff();
void    GLPolygonSmoothOn   ();
void    GLPolygonSmoothOff  ();
void    GLShadingOn         ();         // using material shading
void    GLShadingOff        ();         // using plain coloring (no shading)
void    GLSmoothEdgesOn     ();
void    GLSmoothEdgesOff    ();
void    GLSmoothPointsOn    ();
void    GLSmoothPointsOff   ();
void    GLStencilOn         ();
void    GLStencilOff        ();
void    GLStencilInit       ();         // used before each slice
void    GLStencilUse        ();         // called before making use of the stencil
void    GLTestDepthOn       ();         // will hide parts below the z-buffer
void    GLTestDepthOff      ();         // will draw on top of everything
void    GLTransparencyOn    ();
void    GLTransparencyOff   ();
void    GLWriteDepthOn      ();         // updates the z-buffer
void    GLWriteDepthOff     ();         // doesn't update the z-buffer
constexpr auto GLColoringOn = GLShadingOff; // aliases
constexpr auto GLColoringOff= GLShadingOn;
constexpr auto GLMaterialOn = GLShadingOn;
constexpr auto GLMaterialOff= GLShadingOff;

void    GLWindowCoordinatesOn   ( double right, double top, bool savestate );
void    GLWindowCoordinatesOff  ( bool restorestate );
GLuint  GLGetTextureId      ();


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
