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

#include    <owl/pch.h>

#include    "TGlobalOpenGL.h"
#include    "TBaseView.h"

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Objects that need only to be allocated once, for all windows and for all time
TGLPixelFormat      TGlobalOpenGL::GLpfd;
//TGLRenderingContext TGlobalOpenGL::GLrc;  // static

TGLBitmapFont*      TGlobalOpenGL::SFont                = 0;
TGLBitmapFont*      TGlobalOpenGL::BFont                = 0;

TGLBillboardSphere  TGlobalOpenGL::BbLowSphere  ( BbSphereLowNumRounds,  BbSphereLowNumSlices );
TGLBillboardSphere  TGlobalOpenGL::BbHighSphere ( BbSphereHighNumRounds, BbSphereHighNumSlices );

GLfloat             TGlobalOpenGL::GLLineWidthMin       = 0;
GLfloat             TGlobalOpenGL::GLLineWidthMax       = 0;
GLfloat             TGlobalOpenGL::GLLineWidthStep      = 0;

GLint               TGlobalOpenGL::ViewportMaxSize[ 2 ] = { 0, 0 };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Application windows should have been created and alive at that point
    TGlobalOpenGL::TGlobalOpenGL ()
{
                                        // Although this class can be inherited from multiple windows, it needs this initialization only once
if ( IsOpen () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SFont   = new TGLBitmapFont ( SmallFontParameters );
BFont   = new TGLBitmapFont ( BigFontParameters   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // using MDI Client window, which should exist when a child window is created
UseThisDC           pdc  ( CartoolObjects.CartoolMdiClient->GetHandle () ); 


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rendering context might have already set this...
//if ( ! GLpfd.IsDetected () ) {
                                        // handle to dc
    HDC             hdc ( pdc );


    FormatWish      colorbits       = ValueToFormatWish ( 32 );     // at least
    FormatWish      depthbits       = ValueToFormatWish ( 24 );     // at least
    FormatWish      stencilbits     = ValueToFormatWish (  1 );     // at least
    FormatWish      accumbits       = ValueToFormatWish ( 32 );     // at least
    FormatWish      doublebuff      = FormatUse;                    // request double-buffering
//  FormatWish      accel           = FormatUseBest;                // will go first for hardware if available, software otherwise, without failing
    FormatWish      accel           = CartoolObjects.CartoolApplication->PrefGraphicAccel;
//  FormatWish      accel           = GetValue ( "Request Acceleration (-1:don't care, 0:off, 1:on):", "OpenGL" );


    GLpfd.ChooseBestPixelFormat ( hdc, colorbits, depthbits, stencilbits, accumbits, doublebuff, accel );
//  }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Let's initialize all these guys
GLrc.Set ( pdc, GLpfd );                // non-static


GLfloat             linewidthminmax[ 2 ];

glGetFloatv ( GL_LINE_WIDTH_RANGE,       linewidthminmax  );

GLLineWidthMin  = linewidthminmax[ 0 ] > 0 ? linewidthminmax[ 0 ] : DefaultLineWidthMin;
GLLineWidthMax  = linewidthminmax[ 1 ] > 0 ? linewidthminmax[ 1 ] : DefaultLineWidthMax;

Clipped ( GLLineWidthMin, GLLineWidthMax, DefaultLineWidthMin, DefaultLineWidthMax );


glGetFloatv ( GL_LINE_WIDTH_GRANULARITY, &GLLineWidthStep );

if      ( GLLineWidthStep <= 0   )  GLLineWidthStep = 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Viewport max size
glGetIntegerv ( GL_MAX_VIEWPORT_DIMS, ViewportMaxSize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLrc.Reset ();                          // non-static
}

                                        // Used to clean-up things upon Cartool exit
    TGlobalOpenGL::~TGlobalOpenGL ()
{
GLrc.Reset ();                          // non-static

if ( ! CartoolObjects.CartoolApplication->Closing )
    return;


//GLrc.Reset ();                        // static

                                        // delete only when leaving the app
if ( SFont ) {
    delete SFont;
    SFont = 0;
    }

if ( BFont ) {
    delete BFont;
    BFont = 0;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
