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

#include    "TVolume.h"
#include    "TCacheVolumes.h"
#include    "TCartoolApp.h"

#include    "OpenGL.Texture3D.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // one important thing:
                                        // texture possibilities depend on the CURRENT context
                                        // so you can not assume the results of these functions to be always the same
                                        // consequently, as little as possible is put in variables - use function calls
        TGLTexture3D::TGLTexture3D ()
{
Name                    = 0;

MinFilter               = GL_LINEAR;
MaxFilter               = GL_LINEAR;
Wrap                    = GL_CLAMP;
Compress                = false;

ColorTable              = 0;
ColorTableHash          = 0;
ZeroMemory ( ColorTablePos,  GLTextureColorTableSize * GLTextureNumRGBA * sizeof ( GLubyte ) );
ZeroMemory ( ColorTableNeg,  GLTextureColorTableSize * GLTextureNumRGBA * sizeof ( GLubyte ) );

Texture3DEnable         = -1;
PalettedTextureEnable   = -1;

Busy                    = false;
}


//----------------------------------------------------------------------------
                                        // not complete yet: missing texture loading glTexImage3D
                                        // right now, only the overloaded version in  TArray3Ext  is complete
void    TGLTexture3D::GLize ( int /*param*/ )
{
GenerateName ();                        // if not set, get and ID for the texture


if ( ! IsTextureLoaded () ) {
                                        // create empty texture - it will be deleted when context is deleted
    GLLoad3DTexture     ( Name );

    SetParameters ();
    }
else
    GLLoad3DTexture     ( Name );


// glPrioritizeTextures // to sort by importance
}


//----------------------------------------------------------------------------
void    TGLTexture3D::SetParameters ()
{
                                        // set parameters for texture
glTexParameteri ( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, MinFilter );
glTexParameteri ( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, MaxFilter );
glTexParameteri ( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     Wrap      );
glTexParameteri ( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     Wrap      );
glTexParameteri ( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     Wrap      );
}


//----------------------------------------------------------------------------
void    TGLTexture3D::unGLize ()
{
if ( IsTextureLoaded () )
    GLUnloadTexture ( Name );           // free the texture from memory

Name                    = 0;

ColorTable              = 0;
ColorTableHash          = 0;
                                        // subtle point: allow re-testing in case we changed context
Texture3DEnable         = -1;
PalettedTextureEnable   = -1;
}


//----------------------------------------------------------------------------
void    TGLTexture3D::GenerateName ()
{
if ( Name == 0 )
    Name    = GLGetTextureId ();    // if not set, get and ID for the texture
}


//----------------------------------------------------------------------------
bool    TGLTexture3D::IsTextureLoaded ()
{
return  Name && glIsTexture ( Name );
}


//----------------------------------------------------------------------------
bool    TGLTexture3D::IsTexture3DEnable ()
{
if ( Texture3DEnable != - 1 )           // the test has already been run?
    return  Texture3DEnable;


if ( CartoolObjects.CartoolApplication->PrefGraphic3DTextures == FormatDontUse )
    Texture3DEnable         = false;
else // FormatUse || FormatUseBest
    Texture3DEnable         = (bool) glTexImage3D ();

//Texture3DEnable  = false;


return  Texture3DEnable;
}


//----------------------------------------------------------------------------
                                        // !Obsolete - forced to off!
bool    TGLTexture3D::IsPalettedTextureEnable ()
{
if ( PalettedTextureEnable != - 1 )     // the test has already been run?
    return  PalettedTextureEnable;


//if ( CartoolObjects.CartoolApplication->PrefGraphic3DTextures == FormatDontUse )
//    PalettedTextureEnable   = false;
//else // FormatUse || FormatUseBest
//    PalettedTextureEnable   = (bool) glColorTableEXT ();


PalettedTextureEnable  = false;


return  PalettedTextureEnable;
}


//----------------------------------------------------------------------------
                                        // since OpenGL 1.2, this doesn't seem to be a problem
PFNGLTEXIMAGE3DPROC     TGLTexture3D::glTexImage3D ()
{
return (PFNGLTEXIMAGE3DPROC) wglGetProcAddress ( "glTexImage3D" );
}

/*                                      // since OpenGL 1.3
PFNGLCOMPRESSEDTEXIMAGE3DPROC   TGLTexture3D::glCompressedTexImage3D ()
{
return (PFNGLCOMPRESSEDTEXIMAGE3DPROC) wglGetProcAddress ( "glCompressedTexImage3D" );
}
*/

//----------------------------------------------------------------------------
                                        // since about 1995, but tends to not be supported anymore in new hardwares
PFNGLCOLORTABLEEXTPROC  TGLTexture3D::glColorTableEXT ()
{
return (PFNGLCOLORTABLEEXTPROC) wglGetProcAddress ( "glColorTableEXT" );

//glGetString(GL_EXTENSIONS) -> should contain "GL_EXT_paletted_texture"
}

/*
static  PFNGLTEXIMAGE3DPROC             glTexImage3D            = 0;    // since OpenGL 1.2, this doesn't seem to be a problem
static  PFNGLCOLORTABLEEXTPROC          glColorTableEXT         = 0;    // since about 1995, but tends to not be supported anymore in new hardwares
//static  PFNGLCOMPRESSEDTEXIMAGE3DPROC   glCompressedTexImage3D  = 0;    // since OpenGL 1.3
//static  PFNGLGETCOLORTABLEPARAMETERIVEXTPROC    glGetColorTableParameterivEXT   = 0;


void    TGLTexture3D::LoadFunctions ()
{
if ( glTexImage3D )
    return;

glTexImage3D            = (PFNGLTEXIMAGE3DPROC)             wglGetProcAddress ( "glTexImage3D"              );
glColorTableEXT         = (PFNGLCOLORTABLEEXTPROC)          wglGetProcAddress ( "glColorTableEXT"           );
//glCompressedTexImage3D  = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)   wglGetProcAddress ( "glCompressedTexImage3D"    );
//glGetColorTableParameterivEXT   = (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC) wglGetProcAddress ( "glGetColorTableParameterivEXT" );
}
*/

//----------------------------------------------------------------------------
uint    TGLTexture3D::HashColorTable ()
{
if ( ! ColorTable )
    return  0;

uint                newhash         = 0;

                                        // use original colortable object
                                        // sum as int32
for ( int i = 0; i < ( sizeof ( *ColorTable ) >> 2 ); i++ )
    newhash    += *( ( (uint *) ColorTable ) + i );


/*                                        // use values of local colortable
TGLColor<GLfloat>   glcol;
                                        // apply current ColorTable settings, convert and store into byte array
for ( int i = 0, j = 0; i < GLTextureColorTableSize; i++, j += 4 ) {
                                        // summing floats
    ColorTable->GetColorIndex ( i, glcol );
    newhash    += *(uint*) &glcol.Red + *(uint*) &glcol.Green + *(uint*) &glcol.Blue + *(uint*) &glcol.Alpha;
                                        // summing bytes
//    newhash    += (uint) ( ColorTableByte[ j ] + ColorTableByte[ j + 1 ] + ColorTableByte[ j + 2 ] + ColorTableByte[ j + 3 ] );
    }
*/

return  newhash;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLVolume::TGLVolume ()
{
Data                = 0;
Dim1                = Dim2  = Dim3  = 0;
Origin   [ 0 ]      = Origin   [ 1 ]        = Origin   [ 2 ]        = 0;
}


//----------------------------------------------------------------------------
                                        // index of loaded, or to be loaded texture
int     TGLVolume::GetLoadedTexture ()
{
for ( int i = 0; i < GLTextureMaxNames; i++ )
    if ( Texture[ i ].IsTextureLoaded () )
        return  i;

return  -1;
}


//----------------------------------------------------------------------------
                                        // get an index of texture to work on: either current active or next available
int     TGLVolume::GetTextureIndex ()
{
                                        // find a loaded texture - volume can be only loaded once in a OpenGL context
int                 texi            = GetLoadedTexture ();

if ( texi >= 0 )
    return  texi;

                                        // not loaded, get the first available slot
for ( int i = 0; i < GLTextureMaxNames; i++ )
    if ( Texture[ i ].Name == 0 )
        return  i;


//#if defined(CHECKASSERT)
//assert ( false );
//#endif
                                        // oopsie!
return  0;
}


//----------------------------------------------------------------------------
void    TGLVolume::GLize ( Volume* data, TGLColorTable* colortable, bool interpolate, const double* origin )
{
                                        // never enter!
if ( data == 0 || data->IsNotAllocated () )
    return;


int                 texi            = GetTextureIndex ();
TGLTexture3D&       tex             = Texture[ texi ];

                                        // wait until better times
if ( tex.Busy )
    return;

tex.Busy    = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save parameters
Data            = data;
tex.ColorTable  = colortable;

if ( origin ) {
    Origin   [ 0 ]  = origin   [ 0 ];
    Origin   [ 1 ]  = origin   [ 1 ];
    Origin   [ 2 ]  = origin   [ 2 ];
    }
else
    Origin   [ 0 ]  = Origin   [ 1 ]    = Origin   [ 2 ]    = 0;


if ( Data ) {
    Dim1            = Data->GetDim1 ();
    Dim2            = Data->GetDim2 ();
    Dim3            = Data->GetDim3 ();
    }
else
    Dim1            = Dim2              = Dim3              = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need all these data to work with texture
if ( ! ( Data && tex.ColorTable && tex.IsTexture3DEnable () ) ) {
    tex.Busy    = false;
    return;
    }

                                        // test texture size, there is a limit to this!
                                        // get power of 2s dimensions
int                 xtex            = NoMore ( GLTextureMaxTextureSize, Power2Above ( Dim1 ) );
int                 ytex            = NoMore ( GLTextureMaxTextureSize, Power2Above ( Dim2 ) );
int                 ztex            = NoMore ( GLTextureMaxTextureSize, Power2Above ( Dim3 ) );

                                        // it seems textures should have the same dimensions in all axis
xtex    = ytex  = ztex  = max ( xtex, ytex, ztex );

                                        // do we need to downsample the texture?
                                        // downsampling is done isotropic here
int                 texdownsampling = max ( 1, 
                                            RoundAbove ( Dim1 / (double) xtex ), 
                                            RoundAbove ( Dim2 / (double) ytex ), 
                                            RoundAbove ( Dim3 / (double) ztex ) );

//if ( VkQuery () )  DBGV5 ( Dim1, Dim2, Dim3, texdownsampling, xtex, "Data: Dim1 Dim2 Dim3 -> texdownsampling, texsize" );

                                        // test for null dimensions, too big dimensions, or too much memory to reserve
if ( xtex == 0 || xtex > GLTextureOpenGLMaxTextureSize
  || ! tex.IsPalettedTextureEnable () && ( xtex * ytex * ztex * sizeof ( uint ) > 256 * MegaByte ) ) {
    tex.Busy    = false;
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // automatic name generation - only once
//tex.GenerateName ();

#if defined (_WIN64)

                                        // hash the 2 64 bits values into 32 bits
//LARGE_INTEGER       li1;
//li1.QuadPart    = (LONGLONG) data;
//
//LARGE_INTEGER       li2;
//li2.QuadPart    = (LONGLONG) wglGetCurrentContext ();
//
//tex.Name        = (GLuint) ( li1.HighPart + li1.LowPart + li2.HighPart + li2.LowPart );

                                        // same but with macros
tex.Name        = ( LOULONG ( data ) + HIULONG ( data ) + LOULONG ( wglGetCurrentContext () ) + HIULONG ( wglGetCurrentContext () ) );

#else
                                        // this unambiguously determines the volume to draw
//tex.Name        = (GLuint) ( (void *) data );
                                        // same *data can be used in different contexts, and each of them needs its own version
tex.Name        = (GLuint) ( (void *) data ) + (GLuint) wglGetCurrentContext ();

#endif


//if ( tex.Name == 0 )
//    tex.Name    = (GLuint) ( (void *) data ) + (GLuint) wglGetCurrentContext ();

//tex.Name = 0;               // or we can hash name
//for ( const char *toc = MRIDoc->GetTitle (); *toc; toc++ )
//    tex.Name += (GLuint) *toc;


tex.MinFilter   = interpolate ? GL_LINEAR : GL_NEAREST;
tex.MaxFilter   = interpolate ? GL_LINEAR : GL_NEAREST;
tex.Wrap        = GL_CLAMP;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init color table
TGLColor<GLfloat>   glcol;

                                        // apply current ColorTable settings, convert and store into byte array
                                        // Note that ColorTableByte is now just used internally, and not directly provided to OpenGL
for ( int i = 0; i < GLTextureColorTableSize; i++ ) {
                                        // Store and convert the actual table values for positive data
    colortable->GetColorIndex ( i / (double) ( GLTextureColorTableSize - 1 ) * colortable->GetPMax (), glcol );
                                        // convert to ubyte
    tex.ColorTablePos[ i ][ 0 ]     = UCHAR_MAX * glcol.Red;
    tex.ColorTablePos[ i ][ 1 ]     = UCHAR_MAX * glcol.Green;
    tex.ColorTablePos[ i ][ 2 ]     = UCHAR_MAX * glcol.Blue;
    tex.ColorTablePos[ i ][ 3 ]     = UCHAR_MAX * glcol.Alpha;  // ?we can have negative alpha, which will wrap to positive - do we need to investigate?
//  tex.ColorTablePos[ i ][ 3 ]     = UCHAR_MAX * NoMore ( (float) 1.0, glcol.Alpha );

//    tex.ColorTablePos[ i ][ 0 ]     = UCHAR_MAX * Clip ( glcol.Red  , (float) 0.0, (float) 1.0 );
//    tex.ColorTablePos[ i ][ 1 ]     = UCHAR_MAX * Clip ( glcol.Green, (float) 0.0, (float) 1.0 );
//    tex.ColorTablePos[ i ][ 2 ]     = UCHAR_MAX * Clip ( glcol.Blue , (float) 0.0, (float) 1.0 );
//    tex.ColorTablePos[ i ][ 3 ]     = UCHAR_MAX * Clip ( glcol.Alpha, (float) 0.0, (float) 1.0 );


    if ( colortable->IsSignedTable () ) {
                                        // Store and convert the actual table values for negative data
        colortable->GetColorIndex ( i / (double) ( GLTextureColorTableSize - 1 ) * colortable->GetNMax (), glcol );
                                            // convert to ubyte
        tex.ColorTableNeg[ i ][ 0 ]     = UCHAR_MAX * glcol.Red;
        tex.ColorTableNeg[ i ][ 1 ]     = UCHAR_MAX * glcol.Green;
        tex.ColorTableNeg[ i ][ 2 ]     = UCHAR_MAX * glcol.Blue;
        tex.ColorTableNeg[ i ][ 3 ]     = UCHAR_MAX * glcol.Alpha;
        }
    }

/*
TMaps   savedlut ( GLTextureColorTableSize, 4 );
for ( int i = 0; i < GLTextureColorTableSize; i++ ) {
                                        // Store and convert the actual table values for positive data
    colortable->GetColorIndex ( i / (double) ( GLTextureColorTableSize - 1 ) * colortable->GetPMax (), glcol );
                                        // convert to ubyte
    savedlut[ i ][ 0 ]  = tex.ColorTablePos[ i ][ 0 ];
    savedlut[ i ][ 1 ]  = tex.ColorTablePos[ i ][ 1 ];
    savedlut[ i ][ 2 ]  = tex.ColorTablePos[ i ][ 2 ];
    savedlut[ i ][ 3 ]  = tex.ColorTablePos[ i ][ 3 ];
    }
savedlut.WriteFile ( "E:\\Data\\ColorTable.LUT.RGBA.sef" );
*/
                                        // hash the colortable variables as a simple means to check for changes!
                                        // hash is far enough, as we only need to detect successive changes
                                        // this also handles all the hassles for the caller to know when to reload
uint                newhash         = tex.HashColorTable ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init & load texture

if ( ! tex.IsTextureLoaded ()
  || ( newhash != tex.ColorTableHash && ! tex.IsPalettedTextureEnable () ) ) {
                                        // create empty texture - it will be deleted when context is deleted
    GLLoad3DTexture     ( tex.Name );

    if ( glGetError () ) {
        tex.Busy    = false;
//      tex.Name    = 0;
        return;
        }

    tex.SetParameters ();

                                        // actually loading the texture

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // this is old code, drivers don't support this anymore - it should be replaced by some sort of shader instead
    if ( tex.IsPalettedTextureEnable () ) {
                                        // data match exactly the texture size?
        if ( xtex == Dim1 && ytex == Dim2 && ztex == Dim3 )

            tex.glTexImage3D () (   GL_TEXTURE_3D, 0, GL_COLOR_INDEX8_EXT,
                                    Dim1, Dim2, Dim3, 0,
                                    GL_COLOR_INDEX, GL_UNSIGNED_BYTE, Data->GetArray () );

        else {
                                        // we have to use a temp array that has texture compatibles dimensions
            static TCacheVolumes<uchar>     Caches ( 9 );

                                        // get a cache slot
            bool                    reload;
            TVolume<uchar>         *datacache       = Caches.GetCache ( xtex, ytex, ztex, reload );


            datacache->ResetMemory ();

            datacache->Insert ( *Data, 0, 1, texdownsampling );

            tex.glTexImage3D () (   GL_TEXTURE_3D, 0, GL_COLOR_INDEX8_EXT,
                                    datacache->GetDim1 (), datacache->GetDim2 (), datacache->GetDim3 (), 0,
                                    GL_COLOR_INDEX, GL_UNSIGNED_BYTE, datacache->GetArray () );

            } // dimensions not OK

        } // paletted texture

    else { // no paletted texture
*/
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // the long way: use a temp volume and fill it with RGBA values, then load it
                                        // if you don't have paletted texture, then you need to have some memory AND cpu!
                                        // !! Could crash everything with 512x512x512 !!
                                        // Note: uint type is used for storing the 4 bytes RGBA
        static TCacheVolumes<uint>  Caches ( 9 );

                                        // get a cache slot
        bool                reload;
        TVolume<uint>*      datacache       = Caches.GetCache ( xtex, ytex, ztex, reload );

        datacache->ResetMemory ();

//      uint*               texposuint      = (uint *) tex.ColorTablePos;
//      uint*               texneguint      = (uint *) tex.ColorTableNeg;
        double              rescalevaluepos = ( GLTextureColorTableSize - 1 ) / colortable->GetPMax ();
        double              rescalevalueneg = ( GLTextureColorTableSize - 1 ) / colortable->GetNMax ();
                                        // use some integer arithmetic for speed - precision is OK as we land into GLubyte anyway
//      constexpr   UINT    ColorInterpolPrecBits   = 16;   // this can produce overflows (also with extreme brightness), while 13 seems fine
        constexpr   UINT    ColorInterpolPrecBits   = 13;
        constexpr   UINT    ColorInterpolPrec       = 1 << ColorInterpolPrecBits;
                                        // this is another way to counter the overflow - shown here only for positive data
//      double              maxrescalepos   = INT_MAX / (double) ( ColorInterpolPrec * NonNull ( colortable->MaxValue ) * 10 );
//      Mined ( rescalevaluepos, maxrescalepos );

                                        // Computing the rescaled truncated value, then the rescaled fraction
                                        // Using the integer weights on the lowest and highest colormap slots
//      auto InterpolateRGBA    = [ tex, rescalevaluepos, rescalevalueneg ] ( const MriType& v ) {
//
//          int             vi          = Clip ( v * ( v >= 0 ? rescalevaluepos : rescalevalueneg ), 0.0, (double) GLTextureColorTableSize - 1 ) * ColorInterpolPrec; // avoiding overflow, maybe?
//
//          const auto&     totex       = v >= 0 ? tex.ColorTablePos : tex.ColorTableNeg;
//
//          return  (UINT) (   ( ( totex[ vi ][ 0 ] )       ) 
//                           | ( ( totex[ vi ][ 1 ] ) <<  8 ) 
//                           | ( ( totex[ vi ][ 2 ] ) << 16 ) 
//                           | ( ( totex[ vi ][ 3 ] ) << 24 ) );
//          };



//      auto InterpolateRGBA    = [ tex, rescalevaluepos, rescalevalueneg ] ( const MriType& v ) {
//
//          int             vi          = Clip ( Round ( v * ( v >= 0 ? rescalevaluepos : rescalevalueneg ) ), 0, GLTextureColorTableSize - 1 ); 
//
//          const auto&     totex       = v >= 0 ? tex.ColorTablePos : tex.ColorTableNeg;
//
//          return  (UINT) (   ( ( totex[ vi ][ 0 ] )       ) 
//                           | ( ( totex[ vi ][ 1 ] ) <<  8 ) 
//                           | ( ( totex[ vi ][ 2 ] ) << 16 ) 
//                           | ( ( totex[ vi ][ 3 ] ) << 24 ) );
//          };

                                        // ?what if ci1 == last bin?
        auto InterpolateRGBA    = [ tex, rescalevaluepos, rescalevalueneg, ColorInterpolPrecBits, ColorInterpolPrec ] ( const MriType& v ) {

            int             vi          = v * ( v >= 0 ? rescalevaluepos : rescalevalueneg ) * ColorInterpolPrec;
            int             w2          = vi % ColorInterpolPrec;
            int             w1          = ColorInterpolPrec - w2;
            int             ci1         = NoMore ( GLTextureColorTableSize - 1, (int) ( vi / ColorInterpolPrec ) );
            int             ci2         = NoMore ( GLTextureColorTableSize - 1, ci1 + 1 );

            const auto&     totex       = v >= 0 ? tex.ColorTablePos : tex.ColorTableNeg;

            return  (UINT) (   ( ( ( totex[ ci1 ][ 0 ] * w1 + totex[ ci2 ][ 0 ] * w2 ) >> ColorInterpolPrecBits )       )
                             | ( ( ( totex[ ci1 ][ 1 ] * w1 + totex[ ci2 ][ 1 ] * w2 ) >> ColorInterpolPrecBits ) <<  8 )
                             | ( ( ( totex[ ci1 ][ 2 ] * w1 + totex[ ci2 ][ 2 ] * w2 ) >> ColorInterpolPrecBits ) << 16 )
                             | ( ( ( totex[ ci1 ][ 3 ] * w1 + totex[ ci2 ][ 3 ] * w2 ) >> ColorInterpolPrecBits ) << 24 ) );
            };
                                        // expand index values to RGBA
                                        // same dimensions -> fastest way
        if ( datacache->GetLinearDim () == Data->GetLinearDim () 
          && texdownsampling == 1 ) {

            uint*               totexout        = datacache->GetArray ();

            OmpParallelFor

            for ( int i = datacache->GetLinearDim () - 1; i >= 0; i-- ) {
                                        // write RGBA bytes directly
//              *totexout = *( (uint *) tex.ColorTableByte + (*Data)[ i ] );

                                        // with truncation
//              if ( v >= 0 )   *totexout   = texposuint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevaluepos ) ) ];
//              else            *totexout   = texneguint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevalueneg ) ) ];

                                        // with linear interpolation
                totexout[ i ]   = InterpolateRGBA ( (*Data)[ i ] );
                }
            } // fast copy

        else {                          // different dimensions -> slower way

            if ( texdownsampling == 1 ) {
                                        // not averaging data - faster, also working on masks
                OmpParallelFor

                for ( int xi = 0; xi < Dim1; xi++ ) {

                    int     xo      = xi;

                    for ( int yi = 0, yo = 0; yi < Dim2; yi++, yo++ )
                    for ( int zi = 0, zo = 0; zi < Dim3; zi++, zo++ ) {
                                            // write RGBA bytes directly, when Data was uchar
//                      (*datacache) ( xo, yo, zo ) = *( (uint *) tex.ColorTableByte + (*Data) ( xi, yi, zi ) );

                                            // with truncation
//                      if ( v >= 0 )   (*datacache) ( xo, yo, zo ) = texposuint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevaluepos ) ) ];
//                      else            (*datacache) ( xo, yo, zo ) = texneguint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevalueneg ) ) ];

                                            // with linear interpolation
                        (*datacache) ( xo, yo, zo ) = InterpolateRGBA ( (*Data) ( xi, yi, zi ) );
                        }
                    }
                } // no downsampling

            else { // texdownsampling > 1
                                        // Note that this is less likely since we reslice input MRIs to 1[mm]
                                        // variable should be here, but compiler sorts of optimize this shit out
//              TEasyStats          stats ( 8 );

                OmpParallelFor

                for ( int xi = 0; xi <= Dim1 - texdownsampling; xi += texdownsampling ) {
                    
                    int     xo      = xi / texdownsampling;

                    for ( int yi = 0, yo = 0; yi <= Dim2 - texdownsampling; yi += texdownsampling, yo++ )
                    for ( int zi = 0, zo = 0; zi <= Dim3 - texdownsampling; zi += texdownsampling, zo++ ) {

                                            // lazy and fast downsampling
//                      (*datacache) ( xo, yo, zo ) = *( (uint *) tex.ColorTableByte + (*Data) ( xi, yi, zi ) );

                                            // with truncation
//                      if ( v >= 0 )   (*datacache) ( xo, yo, zo ) = texposuint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevaluepos ) ) ];
//                      else            (*datacache) ( xo, yo, zo ) = texneguint[ NoMore ( GLTextureColorTableSize - 1, (int) ( v * rescalevalueneg ) ) ];

                                            // with linear interpolation
                        (*datacache) ( xo, yo, zo ) = InterpolateRGBA ( (*Data) ( xi, yi, zi ) );

                                            // we restrict ourselves to downsampling = 2, as to avoid more loops (higher than 2 is very unlikely)
                                            // !it is important to do the average of the data, NOT the average of the colors!
                                            // off, because it takes way too much time for interactive adjustments
//                      stats.Reset ();
//
//                      stats.Add ( (*Data) ( xi    , yi    , zi     ), true );
//                      stats.Add ( (*Data) ( xi    , yi    , zi + 1 ), true );
//                      stats.Add ( (*Data) ( xi    , yi + 1, zi     ), true );
//                      stats.Add ( (*Data) ( xi    , yi + 1, zi + 1 ), true );
//                      stats.Add ( (*Data) ( xi + 1, yi    , zi     ), true );
//                      stats.Add ( (*Data) ( xi + 1, yi    , zi + 1 ), true );
//                      stats.Add ( (*Data) ( xi + 1, yi + 1, zi     ), true );
//                      stats.Add ( (*Data) ( xi + 1, yi + 1, zi + 1 ), true );
//                                          // !median is important here, so it can work with ROIs / masks!
//                      (*datacache) ( xo, yo, zo ) = *( (uint *) tex.ColorTableByte
//                                                     + (uchar) stats.Median ( true ) );
                        } // for xi, yi, zi
                    }
                } // texdownsampling > 1
            } // different dimensions

                                        // to update an already loaded texture, use glTexSubImage3D
        if ( tex.Compress )

            tex.glTexImage3D () (   GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA_ARB, // or use DXT5
                                    datacache->GetDim1 (), datacache->GetDim2 (), datacache->GetDim3 (), 0,
                                    GL_RGBA, GL_UNSIGNED_BYTE, datacache->GetArray () );
        else
                                                     // optimize internal format according to current colors
            tex.glTexImage3D () (   GL_TEXTURE_3D, 0, colortable->IsLuminance () ? GL_LUMINANCE8_ALPHA8 : GL_RGBA8 /*GL_RGBA4*/,
                                    datacache->GetDim1 (), datacache->GetDim2 (), datacache->GetDim3 (), 0,
                                    GL_RGBA, GL_UNSIGNED_BYTE, datacache->GetArray () );

//      } // no paletted texture
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last OpenGL call is glTexImage3D
    if ( glGetError () ) {
        tex.Busy    = false;
//      tex.Name    = 0;
        return;
        }

                                        // create texture matrix
    tex.Matrix.SetIdentity ();

    tex.Matrix.Scale       ( 1.0 / ( texdownsampling * xtex ), 
                             1.0 / ( texdownsampling * ytex ), 
                             1.0 / ( texdownsampling * ztex ),  MultiplyRight );

    tex.Matrix.Translate   ( 0.5,        0.5,        0.5,       MultiplyRight );

                                        // permutate X <-> Z (storage of TVolume)
    TMatrix44       swap;
    swap.Reset ();
    swap[2] = swap[5] = swap[8] = swap[15] = 1;

    tex.Matrix *= swap;
    } // if ! tex.IsTextureLoaded ()

else                                    // texture is already loaded - activate this texture, though, as it could compete with other 3D textures in the same object

    GLLoad3DTexture     ( tex.Name );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( tex.IsPalettedTextureEnable () )
//
//    tex.glColorTableEXT () ( GL_TEXTURE_3D, GL_RGBA8, GLTextureColorTableSize, GL_RGBA, GL_UNSIGNED_BYTE, tex.ColorTableByte );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remember ColorTable "settings"
tex.ColorTableHash  = newhash;

                                        // init coordinates transform for texture
tex.Matrix.GLize ( ReplaceTexture );


tex.Busy    = false;
}


//----------------------------------------------------------------------------
void    TGLVolume::unGLize ()
{
for ( int i = 0; i < GLTextureMaxNames; i++ )
    Texture[ i ].unGLize ();

Data            = 0;
Dim1    = Dim2  = Dim3  = 0;
}


//----------------------------------------------------------------------------
                                        // extra border width - could be put as parameter
constexpr int   PM          = 10;

void    TGLVolume::DrawPlaneX ( GLfloat x, int quality, bool moreborder )
{
x      += Origin[ 0 ];

if ( x < 0 || x >= Dim1 )
    return;


int                 texi            = GetLoadedTexture ();

                                        // use texture?
if ( texi >= 0 && Texture[ texi ].IsTexture3DEnable () && quality == QuadFastestQuality ) {

    GLfloat     mb  = moreborder ? PM : 0;


    GLLoad3DTexture     ( Texture[ texi ].Name );
    GL3DTextureOn       ();

    Texture[ texi ].Matrix.GLize ( ReplaceTexture );

//    if ( Texture[ texi ].IsPalettedTextureEnable () ) // not compatible with more than 1 clipping plane
//        Texture[ texi ].glColorTableEXT () ( GL_TEXTURE_3D, GL_RGBA8, GLTextureColorTableSize, GL_RGBA, GL_UNSIGNED_BYTE, Texture[ texi ].ColorTableByte );
                                        // global color of the texture
    glColor4f ( 1.00, 1.00, 1.00, 1.00 );


    glBegin ( GL_QUADS );

    glTexCoord3f ( x, -mb, -mb );
    glVertex3f   ( x - Origin[ 0 ], -mb - Origin[ 1 ], -mb - Origin[ 2 ] );

    glTexCoord3f ( x, Dim2 - 1 + mb, -mb );
    glVertex3f   ( x - Origin[ 0 ],  Dim2 - 1 + mb - Origin[ 1 ], -mb - Origin[ 2 ] );

    glTexCoord3f ( x, Dim2 - 1 + mb, Dim3 - 1 + mb );
    glVertex3f   ( x - Origin[ 0 ],  Dim2 - 1 + mb - Origin[ 1 ], Dim3 - 1 + mb - Origin[ 2 ] );

    glTexCoord3f ( x, -mb, Dim3 - 1 + mb );
    glVertex3f   ( x - Origin[ 0 ], -mb - Origin[ 1 ], Dim3 - 1 + mb - Origin[ 2 ] );

    glEnd();


    GL3DTextureOff      ();
    }
else {                                  // use Quads

    QuadMesh.Begin ( Dim3, quality, *Texture[ texi ].ColorTable );

    pos.X   = x - Origin[ 0 ];
    val.X   = x;

    if ( Texture[ texi ].MaxFilter == GL_LINEAR ) {     // linear interpolation
        int     i1  = x;
        int     i2  = i1 + 1;

        if ( i2 >= Dim1 )   i2 = Dim1 - 1;

        double  w1      = fabs ( i2 - x );
        double  w2      = 1 - w1;

        for ( val.Y = 0, pos.Y = - Origin[ 1 ]; val.Y < Dim2; val.Y++, pos.Y++ )
        for ( val.Z = 0, pos.Z = - Origin[ 2 ]; val.Z < Dim3; val.Z++, pos.Z++ )
            QuadMesh.NextVertex ( pos, w1 * Data->GetValue ( i1, val.Y, val.Z ) + w2 * Data->GetValue ( i2, val.Y, val.Z ) );
        }
    else {                              // non-interpolated
        x   += 0.5;

        for ( val.Y = 0, pos.Y = - Origin[ 1 ]; val.Y < Dim2; val.Y++, pos.Y++ )
        for ( val.Z = 0, pos.Z = - Origin[ 2 ]; val.Z < Dim3; val.Z++, pos.Z++ )
            QuadMesh.NextVertex ( pos, Data->GetValue ( x, val.Y, val.Z ) );

        x   -= 0.5;
        }

    QuadMesh.GLize ();


    if ( moreborder ) {
                                        // add a frame around our slice, in case isosurface is a little bigger
        Texture[ texi ].ColorTable->GLize ( -1 );       // set to color of 0
        Prim.Draw3DFrame ( x - Origin[ 0 ], x - Origin[ 0 ], - Origin[ 1 ], Dim2 - 1 - Origin[ 1 ], - Origin[ 2 ], Dim3 - 1 - Origin[ 2 ], PM );
        }
    }
}


//----------------------------------------------------------------------------
void    TGLVolume::DrawPlaneY ( GLfloat y, int quality, bool moreborder )
{
y       += Origin[ 1 ];

if ( y < 0 || y >= Dim2 )
    return;


int                 texi            = GetLoadedTexture ();

                                        // use texture?
if ( texi >= 0 && Texture[ texi ].IsTexture3DEnable () && quality == QuadFastestQuality ) {

    GLfloat     mb  = moreborder ? PM : 0;


    GLLoad3DTexture     ( Texture[ texi ].Name );
    GL3DTextureOn       ();

    Texture[ texi ].Matrix.GLize ( ReplaceTexture );

//    if ( Texture[ texi ].IsPalettedTextureEnable () )
//        Texture[ texi ].glColorTableEXT () ( GL_TEXTURE_3D, GL_RGBA8, GLTextureColorTableSize, GL_RGBA, GL_UNSIGNED_BYTE, Texture[ texi ].ColorTableByte );
                                        // global color of the texture
    glColor4f ( 1.00, 1.00, 1.00, 1.00 );


    glBegin ( GL_QUADS );

    glTexCoord3f ( -mb, y, -mb );
    glVertex3f   ( -mb - Origin[ 0 ], y - Origin[ 1 ], -mb - Origin[ 2 ] );

    glTexCoord3f ( Dim1 - 1 + mb, y, -mb );
    glVertex3f   ( Dim1 - 1 + mb - Origin[ 0 ], y - Origin[ 1 ], -mb - Origin[ 2 ] );

    glTexCoord3f ( Dim1 - 1 + mb, y, Dim3 - 1 + mb );
    glVertex3f   ( Dim1 - 1 + mb - Origin[ 0 ], y - Origin[ 1 ], Dim3 - 1 + mb - Origin[ 2 ] );

    glTexCoord3f ( -mb, y, Dim3 - 1 + mb );
    glVertex3f   ( -mb - Origin[ 0 ], y - Origin[ 1 ], Dim3 - 1 + mb - Origin[ 2 ] );

    glEnd();

    GL3DTextureOff      ();
    }
else {                                  // use Quads

    QuadMesh.Begin ( Dim3, quality, *Texture[ texi ].ColorTable );

    pos.Y   = y - Origin[ 1 ];
    val.Y   = y;

    if ( Texture[ texi ].MaxFilter == GL_LINEAR ) {     // linear interpolation
        int     i1  = y;
        int     i2  = i1 + 1;

        if ( i2 >= Dim2 )   i2 = Dim2 - 1;

        double  w1      = fabs ( i2 - y );
        double  w2      = 1 - w1;

        for ( val.X = 0, pos.X = - Origin[ 0 ]; val.X < Dim1; val.X++, pos.X++ )
        for ( val.Z = 0, pos.Z = - Origin[ 2 ]; val.Z < Dim3; val.Z++, pos.Z++ )
            QuadMesh.NextVertex ( pos, w1 * Data->GetValue ( val.X, i1, val.Z ) + w2 * Data->GetValue ( val.X, i2, val.Z ) );
        }
    else {                              // non-interpolated
        y   += 0.5;

        for ( val.X = 0, pos.X = - Origin[ 0 ]; val.X < Dim1; val.X++, pos.X++ )
        for ( val.Z = 0, pos.Z = - Origin[ 2 ]; val.Z < Dim3; val.Z++, pos.Z++ )
            QuadMesh.NextVertex ( pos, Data->GetValue ( val.X, y, val.Z ) );

        y   -= 0.5;
        }

    QuadMesh.GLize ();


    if ( moreborder ) {
                                        // add a frame around our slice, in case isosurface is a little bigger
        Texture[ texi ].ColorTable->GLize ( -1 );       // set to color of 0
        Prim.Draw3DFrame ( - Origin[ 0 ], Dim1 - 1 - Origin[ 0 ], y - Origin[ 1 ], y - Origin[ 1 ], - Origin[ 2 ], Dim3 - 1 - Origin[ 2 ], PM );
        }
    }
}


//----------------------------------------------------------------------------
void    TGLVolume::DrawPlaneZ ( GLfloat z, int quality, bool moreborder )
{
z       += Origin[ 2 ];

if ( z < 0 || z >= Dim3 )
    return;

int                 texi            = GetLoadedTexture ();

                                        // use texture?
if ( texi >= 0 && Texture[ texi ].IsTexture3DEnable () && quality == QuadFastestQuality ) {

    GLfloat     mb  = moreborder ? PM : 0;


    GLLoad3DTexture     ( Texture[ texi ].Name );
    GL3DTextureOn       ();

    Texture[ texi ].Matrix.GLize ( ReplaceTexture );

//    if ( Texture[ texi ].IsPalettedTextureEnable () )
//        Texture[ texi ].glColorTableEXT () ( GL_TEXTURE_3D, GL_RGBA8, GLTextureColorTableSize, GL_RGBA, GL_UNSIGNED_BYTE, Texture[ texi ].ColorTableByte );
                                        // global color of the texture
    glColor4f ( 1.00, 1.00, 1.00, 1.00 );


    glBegin ( GL_QUADS );

    glTexCoord3f ( -mb, -mb, z );
    glVertex3f   ( -mb - Origin[ 0 ], -mb - Origin[ 1 ], z - Origin[ 2 ] );

    glTexCoord3f ( Dim1 - 1 + mb, -mb, z );
    glVertex3f   ( Dim1 - 1 + mb - Origin[ 0 ], -mb - Origin[ 1 ], z - Origin[ 2 ] );

    glTexCoord3f ( Dim1 - 1 + mb, Dim2 - 1 + mb, z );
    glVertex3f   ( Dim1 - 1 + mb - Origin[ 0 ], Dim2 - 1 + mb - Origin[ 1 ], z - Origin[ 2 ] );

    glTexCoord3f ( -mb, Dim2 - 1 + mb, z );
    glVertex3f   ( -mb - Origin[ 0 ], Dim2 - 1 + mb - Origin[ 1 ], z - Origin[ 2 ] );

    glEnd();

    GL3DTextureOff      ();
    }
else {                                  // use Quads

    QuadMesh.Begin ( Dim2, quality, *Texture[ texi ].ColorTable );

    pos.Z   = z - Origin[ 2 ];
    val.Z   = z;

    if ( Texture[ texi ].MaxFilter == GL_LINEAR ) {     // linear interpolation
        int     i1  = z;
        int     i2  = i1 + 1;

        if ( i2 >= Dim3 )   i2 = Dim3 - 1;

        double  w1      = fabs ( i2 - z );
        double  w2      = 1 - w1;

        for ( val.X = 0, pos.X = - Origin[ 0 ]; val.X < Dim1; val.X++, pos.X++ )
        for ( val.Y = 0, pos.Y = - Origin[ 1 ]; val.Y < Dim2; val.Y++, pos.Y++ )
            QuadMesh.NextVertex ( pos, w1 * Data->GetValue ( val.X, val.Y, i1 ) + w2 * Data->GetValue ( val.X, val.Y, i2 ) );
        }
    else {                              // non-interpolated
        z   += 0.5;

        for ( val.X = 0, pos.X = - Origin[ 0 ]; val.X < Dim1; val.X++, pos.X++ )
        for ( val.Y = 0, pos.Y = - Origin[ 1 ]; val.Y < Dim2; val.Y++, pos.Y++ )
            QuadMesh.NextVertex ( pos, Data->GetValue ( val.X, val.Y, z ) );

        z   -= 0.5;
        }

    QuadMesh.GLize ();


    if ( moreborder ) {
                                        // add a frame around our slice, in case isosurface is a little bigger
        Texture[ texi ].ColorTable->GLize ( -1 );       // set to color of 0
        Prim.Draw3DFrame ( - Origin[ 0 ], Dim1 - 1 - Origin[ 0 ], - Origin[ 1 ], Dim2 - 1 + - Origin[ 1 ], z - Origin[ 2 ], z - Origin[ 2 ], PM );
        }
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
