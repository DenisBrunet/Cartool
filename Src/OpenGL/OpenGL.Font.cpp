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

#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"
#include    "Math.Utils.h"

#include    "OpenGL.Geometry.h"
#include    "OpenGL.Font.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Retrieve Windows font and store each and every 256 chars specs
bool    TGLBitmapFont::Set (    LPCTSTR face,   int     height, int     width,
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
                                DWORD   pitchandfamily   )
{
                                        // create a DC
HDC                 hdc         = CreateCompatibleDC ( wglGetCurrentDC() /*TScreenDC()*/ );

if ( ! hdc )
    return false;


HFONT               hfont           = CreateFont    (   height,     width,          escapement,     orientation,
                                                        weight,     italic,         underline,      strikeout,
                                                        charset,    outputprecision,clipprecision,
                                                        quality,    pitchandfamily,
                                                        face 
                                                    );
if ( ! hfont )
    return false;


HFONT               hfontOld        = (HFONT) SelectObject ( hdc, (HGDIOBJ) hfont );

if ( hfontOld == HGDI_ERROR )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve text metrics
TEXTMETRIC          tm;

GetTextMetrics ( hdc, &tm );

GLYPHMETRICS        gm;
                                        // OpenGL expect bitmaps starting from bottom
const MAT2          mat22id         = { { 0, 1 }, { 0, 0 },     // each matrix element is a pair (frac,value)
                                        { 0, 0 }, { 0,-1 } };

                                        // scan all glyphs to get the real sizes
uint                boxmaxw         = 0;
uint                boxmaxh         = 0;
double              avgw            = 0;
uint                numc            = 0;

for ( int chari = 0; chari <= tm.tmLastChar; chari++ )

    if ( GetGlyphOutline ( hdc, chari, GGO_METRICS, &gm, 0, NULL, &mat22id ) != GDI_ERROR ) {

        Maxed ( boxmaxw, gm.gmBlackBoxX );
        Maxed ( boxmaxh, gm.gmBlackBoxY );

        avgw   += gm.gmBlackBoxX;

        numc++;
        }

                                        // Use the actual SCANNED values..
FontHeight          = boxmaxh;                      // instead of tm.tmHeight
FontMaxWidth        = boxmaxw;                      // instead of tm.tmMaxCharWidth
FontAveWidth        = RoundAbove ( avgw / numc );   // instead of tm.tmAveCharWidth

                                        // do a proportional shrink as the best estimate to reality
BaseLineHeight      = Round ( ( tm.tmDescent * boxmaxh ) / (double) tm.tmHeight );  // instead of tm.tmDescent

int                 bytesperline        = RoundAbove ( FontMaxWidth / (double) 8 );
                                        // should be 1/2/4/8 for OpenGL
BytesPerCharLine    = bytesperline <= 2 ? bytesperline 
                    : bytesperline <= 4 ? 4 
                    : bytesperline <= 8 ? 8 
                    :                     8;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

BYTE                glyphbuff[ 4 * KiloByte ];
DWORD               dwBuffSize;
int                 glyphsize;
int                 glyphdatasize;

//for ( int chari = tm.tmFirstChar; chari <= tm.tmLastChar; chari++ ) {
for ( int chari = 0; chari <= 255; chari++ ) {

    TOneChar&       c       = CharSet[ chari ];

                                        // get size
    dwBuffSize  = GetGlyphOutline ( hdc, chari, GGO_BITMAP, &gm, 0, NULL, &mat22id );

    if ( dwBuffSize == GDI_ERROR )
        continue;


    if ( GetGlyphOutline ( hdc, chari, GGO_METRICS, &gm, 0, NULL, &mat22id ) == GDI_ERROR )
        continue;


    SetVirtualMemory ( glyphbuff, dwBuffSize, 0xff );

    if ( GetGlyphOutline ( hdc, chari, GGO_BITMAP, &gm, dwBuffSize, glyphbuff, &mat22id ) == GDI_ERROR )
        continue;

                                        // always set
    c.IncrementX    = gm.gmCellIncX;

                                        // is it empty?
    glyphsize       = dwBuffSize * bytesperline     / (double) 4;
    glyphdatasize   = dwBuffSize * BytesPerCharLine / (double) 4;

    if ( glyphsize == 0 )
        continue;

                                        // finish filling char
    c.Width         = gm.gmBlackBoxX;
    c.Height        = gm.gmBlackBoxY;
    c.OriginX       = gm.gmptGlyphOrigin.x;
    c.OriginY       = gm.gmptGlyphOrigin.y;
    c.GlyphSize     = glyphdatasize;
    c.Glyph         = new uchar [ glyphdatasize ];

                                        // finally copying the char bitmap
    ClearVirtualMemory ( c.Glyph, glyphdatasize );

    int         gi      = 0;

    for ( int fi = 0; fi < dwBuffSize; fi += 4 ) {

        for ( int b = 0; b < bytesperline; b++ )

            c.Glyph[ gi++ ] = glyphbuff[ fi + b ];

        gi += BytesPerCharLine - bytesperline;
        }

    } // for char


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Done, release OpenGL objects
SelectObject    ( hdc, hfontOld );
DeleteObject    ( hfont );
DeleteDC        ( hdc );

return true;
}


//----------------------------------------------------------------------------

TGLBitmapFont::TGLBitmapFont ( LPCTSTR face,   int     height, int     width,
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
                               DWORD   pitchandfamily   )
{
FontHeight          = 0;
FontMaxWidth        = 0;
FontAveWidth        = 0;
BaseLineHeight      = 0;
BytesPerCharLine    = 0;

if ( ! Set (    face,   height,     width, 
                escapement, 
                orientation, 
                weight,
                italic, 
                underline, 
                strikeout,
                charset, 
                outputprecision, 
                clipprecision, 
                quality, 
                pitchandfamily ) )

    ShowMessage ( "Can not create this bitmapped font!", face, ShowMessageWarning );
}


//----------------------------------------------------------------------------
                                        // Print text beginning at the current position
                                        // Handles multiple lines
                                        // Note that it currently works only in windows coordinates, otherwise it has to handle some sort of rescaling
void    TGLBitmapFont::Print    (   float       x,              float       y,              float       z, 
                                    const char* text,           UINT        textalign, 
                                    GLdouble    screenoffsetx,  GLdouble    screenoffsety,  GLdouble    screenoffsetz 
                                )
{
if ( StringIsEmpty ( text ) )   return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glPushAttrib  ( GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT /*| GL_TEXTURE_BIT*/ );

glPixelStorei ( GL_UNPACK_ALIGNMENT, BytesPerCharLine );


GLint               Viewport   [  4 ];

glGetIntegerv ( GL_VIEWPORT, Viewport );
                                        // retrieve screen coordinates of origin
TGLCoordinates<GLfloat> p ( x, y, z, 1 );

GLGetScreenCoord ( p, Viewport );

                                        // round to pixel coordinates
p.X     = Truncate ( p.X + 0.25 );
p.Y     = Truncate ( p.Y + 0.25 );

                                        // set Z into -1..1 range
if      ( textalign & ( TA_TOFRONT ) )  p.Z =  1;
else if ( textalign & ( TA_TOBACK  ) )  p.Z = (GLfloat) ( -1 + SingleFloatEpsilon );    // should be -1, but some drivers don't get up to there...
else                                    p.Z =  1 - 2 * p.Z;

                                        // switch to screen coordinates
GLWindowCoordinatesOn   ( Viewport[ 2 ], Viewport[ 3 ], true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // artificially force the fog according to new depth
GLfloat             currentfog[ 2 ];

if ( glIsEnabled ( GL_FOG ) ) {

    glGetFloatv ( GL_FOG_START, currentfog     );
    glGetFloatv ( GL_FOG_END,   currentfog + 1 );

    if ( p.Z > 0 ) {
        glFogf  ( GL_FOG_START, 0.5 ); // kind of off, it seems to wrap around 0, and fog toward viewer!?
        glFogf  ( GL_FOG_END,   1.0 );
        }
    else {
        glFogf  ( GL_FOG_START, 0.0 );
        glFogf  ( GL_FOG_END,   0.5 );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional origin shift (in screen coordinates)
if ( screenoffsetx )    p.X += screenoffsetx;
if ( screenoffsety )    p.Y += screenoffsety;
if ( screenoffsetz )    p.Z += screenoffsetz;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allow empty lines?
TSplitStrings       tokens ( text, NonUniqueStrings, "\n" );


for ( int i = 0; i < (int) tokens; i++ ) {

    const char* toc         = tokens[ i ];

    GLfloat     xorig       = 0;
    GLfloat     yorig       = 0;
                                        // get the x size
    double      delta       = GetStringWidth ( toc );

                                        // change X starting position according to format
    switch ( textalign & ( TA_LEFT | TA_RIGHT | TA_CENTER ) ) {

        case TA_LEFT:                                                   break;
        case TA_RIGHT:      xorig += delta;                             break;
        case TA_CENTER:     xorig += delta / 2;                         break;
        }

                                        // change Y starting position according to format
    switch ( textalign & ( TA_BOTTOM | TA_TOP | TA_BASELINE | TA_CENTERY ) ) {

        case TA_BASELINE:                                               break;
        case TA_BOTTOM:     yorig -= BaseLineHeight;                    break;
        case TA_TOP:        yorig += FontHeight - BaseLineHeight;       break;
        case TA_CENTERY:    yorig += FontHeight / 2 - BaseLineHeight;   break;
        }

                                        // other flags
    switch ( textalign & ( TA_BOX ) ) {

        case TA_BOX:

            TGLColor<GLfloat>   currentcol;

            glGetFloatv ( GL_CURRENT_COLOR, currentcol );

            BoxColor.GLize ();

                                        // set position, in screen coordinates
            glTranslatef ( p.X - xorig - 1, p.Y - yorig - BaseLineHeight, p.Z );

                                        // draw the background
            glBegin ( GL_QUADS );
            glVertex3f   ( 0,         0,          0 );
            glVertex3f   ( delta + 2, 0,          0 );
            glVertex3f   ( delta + 2, FontHeight, 0 );
            glVertex3f   ( 0,         FontHeight, 0 );
            glEnd();

                                        // reset position
            glTranslatef ( - p.X + xorig + 1, - p.Y + yorig + BaseLineHeight, - p.Z );

                                        // reset color
            currentcol.GLize ();
            break;
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ready to send to screen
    glRasterPos3f ( p.X, p.Y, p.Z );


    for ( int i = 0; i < StringLength ( toc ); i++ ) {

        const TOneChar&         c       = CharSet[ (uchar) toc[ i ] ];

        glBitmap    (   c.Width,            c.Height,
                        xorig - c.OriginX,  yorig + c.OriginY,
                        c.IncrementX,       c.IncrementY,      
                        c.Glyph );
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // next line vertical shift
    if ( textalign & ( TA_BOTTOM | TA_BASELINE ) )  p.Y += FontHeight;
    else                                            p.Y -= FontHeight;

    } // for toc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore previous states
GLWindowCoordinatesOff ( true );

                                        // artificially force the fog according to new depth
if ( glIsEnabled ( GL_FOG ) ) {
    glFogfv ( GL_FOG_START, currentfog     );
    glFogfv ( GL_FOG_END,   currentfog + 1 );
    }

                                        // GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT
glPopAttrib ();
}


//----------------------------------------------------------------------------

GLfloat TGLBitmapFont::GetStringWidth ( const char* s )     const
{
double              pixelwidth      = 0;
                                        // get the x size
for ( int i = StringLength ( s ) - 1; i >= 0; i-- )

    pixelwidth     += CharSet[ (uchar) s[ i ] ].IncrementX;

return  pixelwidth;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
