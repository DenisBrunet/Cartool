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

#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"

#include    "OpenGL.h"
#include    "OpenGL.Geometry.h"

#include    "TCartoolApp.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Commonly used function calls, grouped together and renamed

                                                                                   // usual blending function
void    GLBlendOn           ()          { glEnable    ( GL_BLEND );     glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); }
void    GLBlendOff          ()          { glDisable   ( GL_BLEND );         }


void    GLShadingOn         ()          { glEnable    ( GL_LIGHTING );      }   // use material equations
void    GLShadingOff        ()          { glDisable   ( GL_LIGHTING );      }   // plain colors through glColor
                                        // other way to think about it

                                        // will _update_ or not the z-buffer
void    GLWriteDepthOn      ()          { glDepthMask ( GL_TRUE  );         }   // updates the z-buffer
void    GLWriteDepthOff     ()          { glDepthMask ( GL_FALSE );         }   // doesn't update the z-buffer

                                        // will _use_ or not the _content_ of the z-buffer
void    GLTestDepthOn       ()          { glEnable    ( GL_DEPTH_TEST );    }   // will hide parts below the z-buffer
void    GLTestDepthOff      ()          { glDisable   ( GL_DEPTH_TEST );    }   // will draw on top of everything


void    GLFogOn             ()          { glEnable    ( GL_FOG );           }
void    GLFogOff            ()          { glDisable   ( GL_FOG );           }


void    GLLineSmoothOn      ()          { glEnable    ( GL_LINE_SMOOTH );   }
void    GLLineSmoothOff     ()          { glDisable   ( GL_LINE_SMOOTH );   }

                                        // Avoid this call, it can cause artifacts on triangle edges
void    GLPolygonSmoothOn   ()          { glEnable    ( GL_POLYGON_SMOOTH );}
void    GLPolygonSmoothOff  ()          { glDisable   ( GL_POLYGON_SMOOTH );}


void    GLNormalizeNormalOn ()          { glEnable    ( GL_NORMALIZE );     }
void    GLNormalizeNormalOff()          { glDisable   ( GL_NORMALIZE );     }


void    GL3DTextureOn       ()          { glEnable    ( GL_TEXTURE_3D );    }
void    GL3DTextureOff      ()          { glDisable   ( GL_TEXTURE_3D );    }


//----------------------------------------------------------------------------
void    GLTransparencyOn ()
{
GLBlendOn       ();                     // blend -> transparency & anit-aliasing
GLWriteDepthOff ();                     // transparent surfaces are thought of non-solid, therefore non-altering the z-buffer
}

void    GLTransparencyOff ()
{
GLBlendOff      ();                     // no more blending, i.e. transparency
GLWriteDepthOn  ();                     // will update the z-buffer from now
}


//----------------------------------------------------------------------------
void    GLSmoothEdgesOn ()
{
GLBlendOn           ();                 // blend -> transparency & anit-aliasing
GLLineSmoothOn      ();
//GLPolygonSmoothOn ();                 // causing more troubles than helping (triangles artifacts)
//glHint            ( GL_LINE_SMOOTH_HINT, GL_NICEST /*GL_NICEST GL_FASTEST GL_DONT_CARE*/ ); // not sure if that really helps
}

void    GLSmoothEdgesOff ()
{
GLBlendOff          ();
GLLineSmoothOff     ();
//GLPolygonSmoothOff  ();
}


//----------------------------------------------------------------------------
void    GLSmoothPointsOn ()
{
glEnable        ( GL_POINT_SMOOTH );
GLBlendOn       ();
}

void    GLSmoothPointsOff ()
{
glDisable       ( GL_POINT_SMOOTH );
GLBlendOff      ();
}


//----------------------------------------------------------------------------
void    GLLinesModeOn ( bool udpatefog )
{
GLSmoothEdgesOn     ();
GLColoringOn        ();
GLWriteDepthOff     ();

if ( udpatefog )
    GLFogOff        ();
}

void    GLLinesModeOff ( bool udpatefog )
{
GLSmoothEdgesOff    ();
GLColoringOff       ();
GLWriteDepthOn      ();

if ( udpatefog )
    GLFogOn         ();
}


//----------------------------------------------------------------------------
void    GLAlphaAboveOn ( double alphathreshold )
{
glAlphaFunc         ( GL_GREATER, alphathreshold );
glEnable            ( GL_ALPHA_TEST );
}

void    GLAlphaAboveOff ()
{
glDisable           ( GL_ALPHA_TEST );
}


//----------------------------------------------------------------------------
void    GLStencilOn ()
{
glClearStencil  ( 0 );
glStencilMask   ( 1 );
glEnable        ( GL_STENCIL_TEST );
}

void    GLStencilOff ()
{
glDisable       ( GL_STENCIL_TEST );
}
                                        // Sets / Resets upon entering and exiting isourface
void    GLStencilInit ()
{
glClear         ( GL_STENCIL_BUFFER_BIT );
glStencilFunc   ( GL_ALWAYS, 0, 1 );
glStencilOp     ( GL_KEEP, GL_INVERT, GL_INVERT );
}

void    GLStencilUse ()
{
glStencilFunc   ( GL_NOTEQUAL, 0, 1 );
//glStencilOp   ( GL_KEEP, GL_KEEP, GL_KEEP );
}


//----------------------------------------------------------------------------
void    GLLoad3DTexture ( GLuint& textureid )
{
if ( textureid )
    glBindTexture       ( GL_TEXTURE_3D, textureid );
}

void    GLUnloadTexture ( GLuint& textureid )
{
glDeleteTextures ( 1, &textureid );     // should not complain if 0, or if texture does not exist
}


//----------------------------------------------------------------------------
                                        // set 3D implicit texture, generating the texture coordinates automatically
void    GL3DAutoTextureOn ( const double* origin )
{
GL3DTextureOn       ();

glTexGeni           ( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
glTexGeni           ( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
glTexGeni           ( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );

glEnable            ( GL_TEXTURE_GEN_S );
glEnable            ( GL_TEXTURE_GEN_T );
glEnable            ( GL_TEXTURE_GEN_R );

                                        // set the spatial directions for each texture coordinates
GLfloat             xplane[ 4 ]     = { 1, 0, 0, (GLfloat) origin[ 0 ] };
GLfloat             yplane[ 4 ]     = { 0, 1, 0, (GLfloat) origin[ 1 ] };
GLfloat             zplane[ 4 ]     = { 0, 0, 1, (GLfloat) origin[ 2 ] };

glTexGenfv          ( GL_S, GL_OBJECT_PLANE, xplane );
glTexGenfv          ( GL_T, GL_OBJECT_PLANE, yplane );
glTexGenfv          ( GL_R, GL_OBJECT_PLANE, zplane );
}

void    GL3DAutoTextureOff ()
{
GL3DTextureOff      ();

glDisable           ( GL_TEXTURE_GEN_S );
glDisable           ( GL_TEXTURE_GEN_T );
glDisable           ( GL_TEXTURE_GEN_R );
}


GLuint  GLGetTextureId ()
{
GLuint              textureid;

glGenTextures ( 1, &textureid );

return  textureid;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // switch to window coordinates
void    GLWindowCoordinatesOn ( double right, double top, bool savestate )
{
GLColoringOn        ();
GLWriteDepthOff     ();
//GLTestDepthOff      ();
//GLFogOff            ();


glMatrixMode ( GL_PROJECTION );

if ( savestate )        glPushMatrix ();

glLoadIdentity ();


glMatrixMode ( GL_MODELVIEW );

if ( savestate )        glPushMatrix ();

glLoadIdentity ();


gluOrtho2D ( 0, right, 0, top );
}

                                        // restore the previous state before entering window coordinates mode
void    GLWindowCoordinatesOff ( bool restorestate )
{
GLColoringOff       ();
GLWriteDepthOn      ();
//GLTestDepthOn       ();
//GLFogOn             ();


glMatrixMode ( GL_PROJECTION );

if ( restorestate )     glPopMatrix ();


glMatrixMode ( GL_MODELVIEW );

if ( restorestate )     glPopMatrix();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLPixelFormat::TGLPixelFormat ()
      : PixelFormat ( 0 )
{
Reset ();
}


void    TGLPixelFormat::Reset ()
{
ClearVirtualMemory ( this, sizeof ( TGLPixelFormat /*PIXELFORMATDESCRIPTOR*/ ) );

nSize           = GetPfdSize ();
nVersion        = 1;
}


void    TGLPixelFormat::Set ( DWORD dwflags, BYTE colorbits, BYTE depthbits, BYTE stencilbits, BYTE accumbits )
{
Reset ();
                                        // set by user
dwFlags         = dwflags;
cColorBits      = colorbits;
cDepthBits      = depthbits;
cStencilBits    = stencilbits;
cAccumBits      = accumbits;
                                        // always set
iPixelType      = PFD_TYPE_RGBA;
iLayerType      = PFD_MAIN_PLANE;
}


int     TGLPixelFormat::GetAccumBits ( bool computed )  const
{
if ( ! computed )
    return  cAccumBits;                 // does not always work, but does not need  wglMakeCurrent (or TGLRenderingContext::GLize)

                                        // !must call  wglMakeCurrent  before!
GLint               redaccum;
GLint               greenaccum;
GLint               blueaccum;
GLint               alphaaccum;

glGetIntegerv ( GL_ACCUM_RED_BITS,   &redaccum   );
glGetIntegerv ( GL_ACCUM_GREEN_BITS, &greenaccum );
glGetIntegerv ( GL_ACCUM_BLUE_BITS,  &blueaccum  );
glGetIntegerv ( GL_ACCUM_ALPHA_BITS, &alphaaccum );

return  redaccum + greenaccum + blueaccum + alphaaccum;
}


//----------------------------------------------------------------------------
int     TGLPixelFormat::GetNumPixelFormats ( const HDC& hdc )
{
Reset ();
                                        // retrieve total number of formats
return  DescribePixelFormat ( hdc, 1, GetPfdSize (), this );
}


bool    TGLPixelFormat::RetrievePixelFormat ( const HDC& hdc, int i )
{
Reset ();

PixelFormat     = i;                    // store the pixel descriptor number
                                        // retrieve total number of formats
return  (bool) DescribePixelFormat ( hdc, i, GetPfdSize (), this );
}

                                        // regular call, pixel format should have been set beforehand!
bool    TGLPixelFormat::ChoosePixelFormat ( const HDC& hdc )
{
int                 pf              =  ::ChoosePixelFormat ( hdc, this );

PixelFormat         = pf;               // and store the pixel descriptor number

return  IsDetected ();  // PixelFormat > 0
}


bool    TGLPixelFormat::SetPixelFormat ( const HDC& hdc )
{
return  ::SetPixelFormat ( hdc, PixelFormat, this ) == TRUE;
}


//----------------------------------------------------------------------------

FormatWish  ValueToFormatWish ( int v )
{
switch ( v ) {
    case  -1    :   return  FormatUseBest;
    case   0    :   return  FormatDontUse;
    case   1    :   return  FormatUse;

//  case   1    :   return  Format1;
    case   2    :   return  Format2;
    case   4    :   return  Format4;
    case   8    :   return  Format8;
    case  16    :   return  Format16;
    case  24    :   return  Format24;
    case  32    :   return  Format32;
    case  48    :   return  Format48;
    case  64    :   return  Format64;
    case 128    :   return  Format128;
    case 192    :   return  Format192;
    case 256    :   return  Format256;

    default     :   return  FormatUseBest;
    }
}

                                        // Guessing which pixel format to select according to caller's preferences
int     TGLPixelFormat::ChooseBestPixelFormat ( const HDC&      hdc, 
                                                FormatWish&     ask_colorbits,  FormatWish&     ask_depthbits,      FormatWish&     ask_stencilbits,
                                                FormatWish&     ask_accumbits,  FormatWish&     ask_doublebuffer,   FormatWish&     ask_accel       )
{
                                        // retrieve total number of formats
int                 numpf           = GetNumPixelFormats ( hdc );

if ( numpf == 0 ) {
    Reset ();
    return  0;
    }

Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGLPixelFormat      pfd;

unsigned long       best_qual           = 0;
FormatWish          best_colorbits      = FormatUseBest;
FormatWish          best_depthbits      = FormatUseBest;
FormatWish          best_stencilbits    = FormatUseBest;
FormatWish          best_accumbits      = FormatUseBest;
FormatWish          best_doublebuffer   = FormatUseBest;
FormatWish          best_accel          = FormatUseBest;
int                 forceformat         = 0;
//int               forceformat         = GetValue ( "Index of OpenGL format:", "OpenGL Init" );

                                        // scan each format
for ( int i = 1; i <= numpf; i++ ) {

                                        // get format
    if ( ! pfd.RetrievePixelFormat ( hdc, i ) )
        continue;

                                        // decipher data
    FormatWish      colorbits       = ValueToFormatWish ( pfd.GetColorBits      () );
    FormatWish      depthbits       = ValueToFormatWish ( pfd.GetDepthBits      () );
    FormatWish      stencilbits     = ValueToFormatWish ( pfd.GetStencilBits    () );
    FormatWish      accumbits       = ValueToFormatWish ( pfd.GetAccumBits      ( false ) );
    FormatWish      doublebuff      = ValueToFormatWish ( pfd.IsDoubleBuffer    () );
    bool            ispalette       = pfd.IsPalette         ();
    bool            accelicd        = pfd.IsAcceleratedICD  ();
    bool            accelmcd        = pfd.IsAcceleratedMCD  ();
    bool            accel           = accelicd || accelmcd;
    bool            software        = ! pfd.IsAccelerated   (); // pfd.IsSoftware ();
    bool            isopengl        = pfd.IsOpenGL          ();
    bool            iswindow        = pfd.IsWindow          ();
    bool            isbitmap        = pfd.IsBitmap          ();

                                        // build quality factor according to some priorities
    unsigned long   q               = 0;

                                        // cumulate these various flags, by decreasing order of importance
    if ( isopengl && iswindow )                                                 q |= 0x80000000;    // without these, that would be a bad starting point...

    if (   ask_accel == FormatDontUse && software )                             q |= 0x40000000;
                                                                                  
    if ( ( ask_accel == FormatUseBest
        || ask_accel >= FormatUse     ) && accel )                              q |= accelicd ? 0x40000000  // more weight to real (ICD) acceleration
                                                                                              : 0x20000000;

    if ( ( ask_colorbits == FormatDontUse && colorbits == FormatDontUse )
        || ask_colorbits == FormatUseBest
        || ask_colorbits >= colorbits                                     )     q |= 0x00080000;


    if ( ( ask_depthbits == FormatDontUse && depthbits == FormatDontUse )
        || ask_depthbits == FormatUseBest
        || ask_depthbits >= depthbits                                     )     q |= 0x00040000;


    if ( ( ask_accumbits == FormatDontUse && accumbits == FormatDontUse )
        || ask_accumbits == FormatUseBest
        || ask_accumbits >= accumbits                                     )     q |= 0x00020000;     // more priority to accumulation buffer than stencil


    if ( ( ask_stencilbits == FormatDontUse && stencilbits == FormatDontUse )
        || ask_stencilbits == FormatUseBest
        || ask_stencilbits >= stencilbits                                 )     q |= 0x00010000;


    if ( ( ask_doublebuffer == FormatDontUse && doublebuff == FormatDontUse )
      || ( (   ask_doublebuffer == FormatUseBest
            || ask_doublebuffer >= FormatUse ) && doublebuff == FormatUse ) )   q |= 0x00004000;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cumulate these less critical features (independently of the request) - the higher still the better
    if ( ! ispalette     )                                                      q |= 0x00000080;
    if ( accelicd        )                                                      q |= 0x00000040;    // hardware acceleration
    if ( accelmcd        )                                                      q |= 0x00000020;    // some acceleration
    if ( colorbits >= 16 )                                                      q |= 0x00000010;
    if ( depthbits >= 16 )                                                      q |= 0x00000008;
    if ( colorbits == 16 )                                                      q |= 0x00000004;
    if ( depthbits == 16 )                                                      q |= 0x00000002;
    if ( isbitmap        )                                                      q |= 0x00000001;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset if some requirements are not fulfilled
    if ( ask_stencilbits  >= FormatUse && stencilbits == FormatDontUse )        q = 0;
    if ( ask_accel        >= FormatUse && ! accel                      )        q = 0;
    if ( ask_doublebuffer >= FormatUse && doublebuff == FormatDontUse  )        q = 0;
    if ( ask_accumbits    >= FormatUse && accumbits == FormatDontUse   )        q = 0;

    //if ( VkQuery () )  DBGV6 ( i, q, colorbits, depthbits, stencilbits, accumbits, "pfd#:  Quality, colorbits, depthbits, stencilbits, accumbits" );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retain the one descriptor with the "highest requirements"
    if ( forceformat >  0
      && i == forceformat

      || forceformat <= 0
      && q
      && q           >= best_qual
      && colorbits   >= ask_colorbits       // q does not include the # of bits within it (not enough bits!)
      && depthbits   >= ask_depthbits       // so we test here the actual various resolutions
      && stencilbits >= ask_stencilbits
      && accumbits   >= ask_accumbits   ) {

        best_qual           = q;
        best_colorbits      = colorbits;
        best_depthbits      = depthbits;
        best_stencilbits    = stencilbits;
        best_accumbits      = accumbits;
        best_doublebuffer   = doublebuff;
        best_accel          = ValueToFormatWish ( accel );

        *this               = pfd;          // this will also copy PixelFormat
        }
    } // for pixelformat


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no pixel format found?
if ( ! IsDetected () ) {
                                        // then force a reasonable format
    pfd.Set (   GLPixelFormatDefaultFlags,
                GLPixelFormatDefaultColorBits,
                GLPixelFormatDefaultDepthBits,
                GLPixelFormatDefaultStencilBits,
                GLPixelFormatDefaultAccumBits   );

    pfd.ChoosePixelFormat ( hdc );

    best_colorbits      = ValueToFormatWish ( pfd.GetColorBits      () );
    best_depthbits      = ValueToFormatWish ( pfd.GetDepthBits      () );
    best_stencilbits    = ValueToFormatWish ( pfd.GetStencilBits    () );
    best_accumbits      = ValueToFormatWish ( pfd.GetAccumBits      ( false ) );
    best_doublebuffer   = ValueToFormatWish ( pfd.IsDoubleBuffer    () );
    best_accel          = ValueToFormatWish ( pfd.IsAccelerated     () );

    *this               = pfd;
    }

//DBGV6 ( PixelFormat, best_qual, best_colorbits, best_depthbits, best_stencilbits, best_accumbits, "BEST pfd#:  Quality, colorbits, depthbits, stencilbits, accumbits" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update the returned values
ask_colorbits       = best_colorbits;
ask_depthbits       = best_depthbits;
ask_stencilbits     = best_stencilbits;
ask_accumbits       = best_accumbits;
ask_doublebuffer    = best_doublebuffer;
ask_accel           = best_accel;

return  PixelFormat;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLRenderingContext::TGLRenderingContext ()
      : hGLRC ( 0 ), InUsed ( false )
{
}


        TGLRenderingContext::TGLRenderingContext ( HDC hdc, TGLPixelFormat &GLpfd, bool forcedetectpfd )
      : hGLRC ( 0 ), InUsed ( false )
{
Set ( hdc, GLpfd, forcedetectpfd );
}


        TGLRenderingContext::~TGLRenderingContext ()
{
Reset ();
}


//----------------------------------------------------------------------------
void    TGLRenderingContext::GLize ( HDC hdc, bool* success )
{
//if ( InUsed ) // && not the same hdc
//    DBGV ( (long) hdc, "TGLRenderingContext::GLize already in use!" );

bool                ok              = wglMakeCurrent ( hdc, hGLRC ) == TRUE;

if ( success )
    *success    = ok;

if ( ok )
    InUsed      = true;
}


void    TGLRenderingContext::unGLize ( bool* success )
{
bool                ok              = wglMakeCurrent ( NULL, NULL ) == TRUE;

if ( success )
    *success    = ok;

if ( ok )
    InUsed      = false;
}


//----------------------------------------------------------------------------
bool    TGLRenderingContext::Set ( HDC hdc, TGLPixelFormat& GLpfd, bool forcedetectpfd )
{
                                        // cancel current context
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run smart detection (a lengthy operation)?
if ( ! GLpfd.IsDetected () || forcedetectpfd ) {

    FormatWish      colorbits       = ValueToFormatWish ( 32 );     // at least
    FormatWish      depthbits       = ValueToFormatWish ( 24 );     // at least
    FormatWish      stencilbits     = ValueToFormatWish (  1 );     // at least
    FormatWish      accumbits       = ValueToFormatWish ( 32 );     // at least
    FormatWish      doublebuff      = FormatUse;                    // request double-buffering
//  FormatWish      accel           = FormatUseBest;                // will go first for hardware if available, software otherwise, without failing
    FormatWish      accel           = CartoolObjects.CartoolApplication->PrefGraphicAccel;
//  FormatWish      accel           = GetValue ( "Request Acceleration (-1:don't care, 0:off, 1:on):", "OpenGL" );


    GLpfd.ChooseBestPixelFormat ( hdc, colorbits, depthbits, stencilbits, accumbits, doublebuff, accel );
    } // detect pfd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set pfd
bool                pfdok           = GLpfd.SetPixelFormat ( hdc );

                                        // create the context
                    hGLRC           = wglCreateContext ( hdc );

                                        // then make it current -> ready to use OpenGL
bool                useok;
GLize ( hdc, &useok );

                                        // all OK?
return  pfdok && hGLRC && useok;
}


//----------------------------------------------------------------------------
                                        // Regular way, with explicit flags
bool    TGLRenderingContext::Set ( HDC hdc, TGLPixelFormat& GLpfd, DWORD dwFlags )
{
                                        // cancel current context
Reset ();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use specified flags
GLpfd.Set ( dwFlags,
            GLPixelFormatDefaultColorBits,
            GLPixelFormatDefaultDepthBits,
            GLPixelFormatDefaultStencilBits,
            GLPixelFormatDefaultAccumBits   );

GLpfd.ChoosePixelFormat ( hdc );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set pfd
bool                pfdok           = GLpfd.SetPixelFormat ( hdc );

                                        // create the context
                    hGLRC           = wglCreateContext ( hdc );

                                        // then make it current -> ready to use OpenGL
bool                useok;
GLize ( hdc, &useok );

                                        // all OK?
return  pfdok && hGLRC && useok;
}


//----------------------------------------------------------------------------
bool    TGLRenderingContext::Reset ()
{
                                        // always reset current context
bool                releaseok;
unGLize ( &releaseok );


bool                dcok            = true;


if ( IsOpen () ) {                      // delete context & reset handle
    dcok        = wglDeleteContext ( hGLRC ) == TRUE;
    hGLRC       = 0;
    }


return  releaseok && dcok;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

double  GLVersion ()
{
return  StringToDouble ( (const char *) glGetString ( GL_VERSION ) );
}


void    GLDisplayVersion ()
{
char                buff[ 32 * KiloByte ];

sprintf ( buff, "Version:"      Tab Tab "%s" NewLine
                "Vendor:"       Tab Tab "%s" NewLine
                "Renderer:"     Tab     "%s" NewLine
                "Extensions:"   Tab     "%s",
          glGetString ( GL_VERSION ),
          glGetString ( GL_VENDOR ),
          glGetString ( GL_RENDERER ),
          glGetString ( GL_EXTENSIONS ) );

ShowMessage ( buff, "OpenGL Version" );
}


void    GLDisplayError ()
{
GLenum              error           = glGetError ();

if ( error ) {
    char            buff[1024];
    sprintf ( buff, "%s", gluErrorString ( error ) );

    ShowMessage ( buff, "OpenGL Error", ShowMessageWarning );
	}
}


//----------------------------------------------------------------------------
                                        // First version using the feedback mechanism - Slow but runs with any context
void    GLGetScreenCoord ( GLfloat v[ 3 ] )
{
                                        // set position
                                        // send 3D point, retrieve its window-coordinates position
GLfloat             buffer[ 3 ];

glFeedbackBuffer( 3, GL_2D, buffer );
glRenderMode    ( GL_FEEDBACK );

glBegin         ( GL_POINTS );
glVertex3fv     ( v );
glEnd();

if ( glRenderMode ( GL_RENDER ) == 3 && buffer[ 0 ] == GL_POINT_TOKEN ) {

    v[ 0 ]  = buffer[ 1 ];
    v[ 1 ]  = buffer[ 2 ];
    v[ 2 ]  = 0;
    }
else {                                  // out of bounds
    v[ 0 ]  =
    v[ 1 ]  =
    v[ 2 ]  = 0;
    }
}

                                        // Second version using the much faster gluProject function - User should provide as much context as possible
bool    GLGetScreenCoord ( GLfloat v[ 3 ], GLint* viewp, GLdouble* proj, GLdouble* model )
{
GLint               Viewport   [  4 ];
GLdouble            ProjMatrix [ 16 ];
GLdouble            ModelMatrix[ 16 ];
GLdouble            screen     [  3 ];

                                        // smartly retrieve all matrices
if ( viewp )    CopyVirtualMemory ( Viewport,    viewp,  4 * sizeof ( *viewp ) );
else            glGetIntegerv ( GL_VIEWPORT, Viewport );

if ( proj )     CopyVirtualMemory ( ProjMatrix,  proj,  16 * sizeof ( *proj ) );
else            glGetDoublev ( GL_PROJECTION_MATRIX, ProjMatrix );

if ( model )    CopyVirtualMemory ( ModelMatrix, model, 16 * sizeof ( *model ) );
else            glGetDoublev ( GL_MODELVIEW_MATRIX, ModelMatrix );


if ( gluProject ( v[ 0 ], v[ 1 ], v[ 2 ], ModelMatrix, ProjMatrix, Viewport, screen, screen + 1, screen + 2 ) == GL_TRUE ) {

    v[ 0 ]  = screen[ 0 ];
    v[ 1 ]  = screen[ 1 ];
    v[ 2 ]  = screen[ 2 ];

    return true;
    }
else {                                  // out of bounds
    v[ 0 ]  =
    v[ 1 ]  =
    v[ 2 ]  = 0;

    return false;
    }
}


//----------------------------------------------------------------------------
bool    GLGetObjectCoord ( GLfloat v[ 3 ], GLint* viewp, GLdouble* proj, GLdouble* model )
{
                                        // retrieving only the non-provided info
GLint               Viewport   [  4 ];

if ( viewp )    CopyVirtualMemory ( Viewport,    viewp,  4 * sizeof ( *viewp ) );
else            glGetIntegerv ( GL_VIEWPORT, Viewport );

                                        // Windows coordinates to OpenGL Windows coordinates
v[ 1 ]  = Viewport[ 3 ] - 1 - v[ 1 ];

                                        // read missing Z position
glReadPixels ( v[ 0 ], v[ 1 ], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &v[ 2 ] );

                                        // no objects encountered?
if ( v[ 2 ] == 1.0 ) {

    v[ 0 ]  =
    v[ 1 ]  =
    v[ 2 ]  = 0;

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLdouble            ProjMatrix [ 16 ];
GLdouble            ModelMatrix[ 16 ];
GLdouble            obj        [  3 ];

if ( proj )     CopyVirtualMemory ( ProjMatrix,  proj,  16 * sizeof ( *proj ) );
else            glGetDoublev ( GL_PROJECTION_MATRIX, ProjMatrix );

if ( model )    CopyVirtualMemory ( ModelMatrix, model, 16 * sizeof ( *model ) );
else            glGetDoublev ( GL_MODELVIEW_MATRIX, ModelMatrix );

                                          // push an epsilon further in Z to be "inside" any object
if ( gluUnProject ( v[ 0 ], v[ 1 ], v[ 2 ] + 1e-4, ModelMatrix, ProjMatrix, Viewport, obj, obj + 1, obj + 2 ) != GL_TRUE ) {

    v[ 0 ]  =
    v[ 1 ]  =
    v[ 2 ]  = 0;

    return  false;
    }

v[ 0 ]  = obj[ 0 ];
v[ 1 ]  = obj[ 1 ];
v[ 2 ]  = obj[ 2 ];

return  true;
}


//----------------------------------------------------------------------------
void    GLApplyModelMatrix ( TGLCoordinates<GLfloat> &p, GLdouble *model )
{
GLdouble            ModelMatrix[ 16 ];
GLdouble            xm;
GLdouble            ym;
GLdouble            zm;

if ( ! model ) {
    glGetDoublev ( GL_MODELVIEW_MATRIX, ModelMatrix );
    model   = ModelMatrix;
    }

                                        // apply modelview
xm  = ( model[0] * p.X + model[4] * p.Y + model[8]  * p.Z + model[12] ) / model[15];
ym  = ( model[1] * p.X + model[5] * p.Y + model[9]  * p.Z + model[13] ) / model[15];
zm  = ( model[2] * p.X + model[6] * p.Y + model[10] * p.Z + model[14] ) / model[15];

p.X = xm;
p.Y = ym;
p.Z = zm;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
