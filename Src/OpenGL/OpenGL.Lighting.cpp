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

#include    <windows.h>

#include    "OpenGL.Lighting.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // specific bodies according to type
void    TGLLight<GLint>::GLize ( int /*param*/ )
{
glLightiv ( LightNumber, GL_AMBIENT,  (const GLint *) Ambient  );
glLightiv ( LightNumber, GL_DIFFUSE,  (const GLint *) Diffuse  );
glLightiv ( LightNumber, GL_SPECULAR, (const GLint *) Specular );
glLightiv ( LightNumber, GL_POSITION, (const GLint *) Position );

glEnable ( LightNumber );
}


void    TGLLight<GLfloat>::GLize ( int /*param*/ )
{
glLightfv ( LightNumber, GL_AMBIENT,  (const GLfloat *) Ambient  );
glLightfv ( LightNumber, GL_DIFFUSE,  (const GLfloat *) Diffuse  );
glLightfv ( LightNumber, GL_SPECULAR, (const GLfloat *) Specular );
glLightfv ( LightNumber, GL_POSITION, (const GLfloat *) Position );

glEnable ( LightNumber );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // specific bodies according to type
void    TGLMaterial<GLint>::GLize ( int /*param*/ )
{
glMaterialiv ( Side, GL_AMBIENT,   (const GLint *) Ambient  );
glMaterialiv ( Side, GL_DIFFUSE,   (const GLint *) Diffuse  );
glMaterialiv ( Side, GL_SPECULAR,  (const GLint *) Specular );
glMaterialiv ( Side, GL_EMISSION,  (const GLint *) Emission );
glMaterialiv ( Side, GL_SHININESS, &Shininess );
}


void    TGLMaterial<GLfloat>::GLize ( int /*param*/ )
{
glMaterialfv ( Side, GL_AMBIENT,   (const GLfloat *) Ambient  );
glMaterialfv ( Side, GL_DIFFUSE,   (const GLfloat *) Diffuse  );
glMaterialfv ( Side, GL_SPECULAR,  (const GLfloat *) Specular );
glMaterialfv ( Side, GL_EMISSION,  (const GLfloat *) Emission );
glMaterialfv ( Side, GL_SHININESS, &Shininess );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // specific bodies according to type
void    TGLFog<GLint>::GLize ( int /*param*/ )
{
glFogi  ( GL_FOG_MODE, FogMode );
glFogiv ( GL_FOG_COLOR, (const GLint *) FogColor );
glFogi  ( GL_FOG_DENSITY, FogDensity );

glFogi  ( GL_FOG_START, FogStart );
glFogi  ( GL_FOG_END,   FogEnd );

//glHint  ( GL_FOG_HINT, GL_DONT_CARE );

GLFogOn         ();
}


void    TGLFog<GLfloat>::GLize ( int /*param*/ )
{
glFogi  ( GL_FOG_MODE, FogMode );
glFogfv ( GL_FOG_COLOR, (const GLfloat *) FogColor );
glFogf  ( GL_FOG_DENSITY, FogDensity );

glFogf  ( GL_FOG_START, FogStart );
glFogf  ( GL_FOG_END,   FogEnd );

//glHint  ( GL_FOG_HINT, GL_DONT_CARE );

GLFogOn         ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
