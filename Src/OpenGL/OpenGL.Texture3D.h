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

#include    "OpenGL.h"
#include    "OpenGL.Colors.h"
#include    "OpenGL.Geometry.h"
#include    "OpenGL.Drawing.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Stores the setup of a texture, not the texture itself
constexpr int   GLTextureColorTableSize         = 256;
constexpr int   GLTextureNumRGBA                = 4;
                                        // the absolute for OpenGL - well, that actually depends on the graphic card
constexpr int   GLTextureOpenGLMaxTextureSize   = 512;
                                        // the limit we set ourselves - safe one
//constexpr int   GLTextureMaxTextureSize         = 256;
                                        // the limit we set ourselves - give it a little more chance for 256 MRIs that get tilted
constexpr int   GLTextureMaxTextureSize         = 384;


class   TGLTexture3D :  public TGLObject
{
public:
                    TGLTexture3D ();


    GLuint          Name;               // Texture Id
    bool            Compress;           // ask for compression - useful for non-paletted textures
    bool            Busy;               // when rebuilding

    GLint           MinFilter;          // OpenGL parameters
    GLint           MaxFilter;
    GLint           Wrap;

    TGLColorTable*  ColorTable;         // color tables stuff
    UINT            ColorTableHash;
    GLubyte         ColorTablePos [ GLTextureColorTableSize ][ GLTextureNumRGBA ];  // caching RGBA color table values - positive data
    GLubyte         ColorTableNeg [ GLTextureColorTableSize ][ GLTextureNumRGBA ];  // caching RGBA color table values - negative data

    TGLMatrix       Matrix;             // for texture coordinates


    void            GLize   ( int param = 0 );
    void            unGLize ();

    void            GenerateName            ();
    void            SetParameters           ();

    bool            IsTextureLoaded         ();
    bool            IsTexture3DEnable       ();
    bool            IsPalettedTextureEnable ();

    UINT            HashColorTable ();

                                // dynamically returning pointers to function
    PFNGLTEXIMAGE3DPROC                     glTexImage3D ();
    PFNGLCOLORTABLEEXTPROC                  glColorTableEXT ();
//  PFNGLCOMPRESSEDTEXIMAGE3DPROC           glCompressedTexImage3D ();
//  PFNGLGETCOLORTABLEPARAMETERIVEXTPROC    glGetColorTableParameterivEXT () { return (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC) wglGetProcAddress ( "glGetColorTableParameterivEXT" ); }
//  void            LoadFunctions           ();


protected:
    int             Texture3DEnable;
    int             PalettedTextureEnable;

};


//----------------------------------------------------------------------------
                                        // To render a volume, at the moment to show slices of it
                                        // There can be 3 different renderings: texture, fast and slow quads
template <class TypeD> class        TVolume;

                                        // A single volume can be loaded in different context, so we need to keep track to a few "instances" of TGLTexture3D
constexpr int   GLTextureMaxNames       = 10;


class   TGLVolume :     public TGLObject
{
public:
                    TGLVolume ();


    TGLTexture3D    Texture[ GLTextureMaxNames ];


    void            GLize ( Volume* data, TGLColorTable* colortable, bool interpolate, const double* origin = 0 );
    void            unGLize ();


    int             GetLoadedTexture    ();
    int             GetTextureIndex     ();


    void            DrawPlaneX ( GLfloat x, int quality, bool moreborder = false );
    void            DrawPlaneY ( GLfloat y, int quality, bool moreborder = false );
    void            DrawPlaneZ ( GLfloat z, int quality, bool moreborder = false );


protected:
    Volume*         Data;
    int             Dim1;
    int             Dim2;
    int             Dim3;
    double          Origin    [ 3 ];    // voxel for ( 0, 0, 0 )


private:
    TGLPrimitives           Prim;
    TGLQuadMesh             QuadMesh;
    TGLCoordinates<GLfloat> pos;
    TGLCoordinates<GLfloat> val;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
