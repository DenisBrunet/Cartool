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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*                                      // from WINGDI.H
#define TA_NOUPDATECP                0
#define TA_UPDATECP                  1

#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24
#if (WINVER >= 0x0400)
#define TA_RTLREADING                256
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP+TA_RTLREADING)
#else
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP)
#endif
*/
                                        // Surreptitiously slipping in our own custom text alignment flags:
#define TA_CENTERY                   0x0020
#define TA_BOX                       0x0040
#define TA_TOFRONT                   0x0080
#define TA_TOBACK                    0x0100

                                        // 1 char glyph properties
class   TOneChar {
public:
                    TOneChar ()     { Width = Height = OriginX = OriginY = IncrementX = IncrementY = GlyphSize = 0; Glyph = 0; }
                   ~TOneChar ()     { if ( Glyph ) delete[] Glyph; }


    int             Width;              // of this char
    int             Height;
    int             OriginX;            // from left
    int             OriginY;            // from baseline
    int             IncrementX;         // to next char origin
    int             IncrementY;         // to next char origin
    int             GlyphSize;          // # bytes to be stored
    uchar*          Glyph;              // bytes with bitmap
};


class   TGLBitmapFont :  public TGLObject
{
public:
                    TGLBitmapFont   ( LPCTSTR face,   int     height, int     width,
                                      int     escapement          = 0,
                                      int     orientation         = 0,
                                      int     weight              = FW_MEDIUM,
                                      DWORD   italic              = false,
                                      DWORD   underline           = false,
                                      DWORD   strikeout           = false,
                                      DWORD   charset             = DEFAULT_CHARSET,
                                      DWORD   outputprecision     = OUT_TT_ONLY_PRECIS,
                                      DWORD   clipprecision       = CLIP_DEFAULT_PRECIS,
                                      DWORD   quality             = PROOF_QUALITY,
                                      DWORD   pitchandfamily      = DEFAULT_PITCH | FF_DONTCARE );


	GLfloat         GetHeight       ()                  const                       { return FontHeight;            }
	GLfloat         GetLineSpacing  ()                  const                       { return FontHeight * 0.25;     }
	GLfloat         GetMaxWidth     ()                  const                       { return FontMaxWidth;          }
	GLfloat         GetAvgWidth     ()                  const                       { return FontAveWidth;          }
	GLfloat         GetStringWidth  ( const char *s )   const;

    void            SetBoxColor     ( GLfloat r, GLfloat g, GLfloat b, GLfloat a )  { BoxColor.Set ( r, g, b, a );  }

	void            Print           ( float         x,      float           y,      float       z, 
                                      const char*   text,   UINT            textalign = TA_LEFT | TA_BASELINE, 
                                      GLdouble      screenoffsetx = 0,  GLdouble    screenoffsety = 0,  GLdouble    screenoffsetz = 0 );

protected:

	GLfloat             FontHeight;
	GLfloat             FontMaxWidth;
	GLfloat             FontAveWidth;
	GLfloat             BaseLineHeight;
    int                 BytesPerCharLine;

    TOneChar            CharSet [ 256 ];    // Fixed size for ASCII

    TGLColor<GLfloat>   BoxColor;

    bool            Set (   LPCTSTR face,   int     height, int     width,
                            int     escapement,
                            int     orientation,
                            int     weight,
                            DWORD   italic,
                            DWORD   underline,
                            DWORD   strikeout,
                            DWORD   charset,
                            DWORD   outputprecision,
                            DWORD   clipprecision,
                            DWORD   quality,
                            DWORD   pitchandfamily   );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
