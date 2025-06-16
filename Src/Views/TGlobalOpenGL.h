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
#include    "OpenGL.Drawing.h"
#include    "OpenGL.Font.h"
#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Global initialization for OpenGL
class   TGlobalOpenGL
{
public:
                                TGlobalOpenGL   ();
                               ~TGlobalOpenGL   ();


    static bool                 IsOpen          ()          { return  GLLineWidthStep != 0; }   // Using one of the variable as test
    static void                 CreateFonts     ();

                                        // static objects will be unique for all inherited classes / objects
    static TGLPixelFormat       GLpfd;                  // Declare and define a single pixel format variable (bits per pixel, per depth, per stencil, per accumulation buffer etc..)
//  static TGLRenderingContext  GLrc;                   // Static, 1 for all windows. Technically speaking, we can have a single rendering context - current state is not working, so this is off for the moment
    TGLRenderingContext         GLrc;                   // Non-static, 1 per window

    static TGLBitmapFont*       SFont;
    static TGLBitmapFont*       BFont;

    static TGLBillboardSphere   BbLowSphere;            // Billboards allow for fast drawing of predefined glyphs
    static TGLBillboardSphere   BbHighSphere;

    static GLfloat              GLLineWidthMin;
    static GLfloat              GLLineWidthMax;
    static GLfloat              GLLineWidthStep;

    static GLint                ViewportMaxSize[ 2 ];

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
