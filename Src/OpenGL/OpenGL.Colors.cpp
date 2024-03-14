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
#include    "Files.Utils.h"
#include    "Dialogs.Input.h"
#include    "Math.Utils.h"
#include    "OpenGL.Font.h"
#include    "OpenGL.Colors.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // specific bodies according to type
void    TGLColor<GLubyte>::GLize ( int /*param*/ )
{
glColor4ubv ( (const GLubyte *) &Red );
}


void    TGLColor<GLint>::GLize ( int /*param*/ )
{
glColor4iv ( (const GLint *) &Red );
}

template <>		// VS2010 complaining
void    TGLColor<GLfloat>::GLize ( int /*param*/ )
{
glColor4fv ( (const GLfloat *) &Red );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLColoring::TGLColoring ( int ref, ColoringEnum how, double minv, double maxv, TGLColor<GLfloat> cmin, TGLColor<GLfloat> cmax )
{
Ref           = ref;
How           = how;
                                        // be sure of min and max
CheckOrder ( minv, maxv );

ValueMin      = minv;
ValueMax      = maxv;

if ( How == Analog ) {
    if ( ValueMin > 0 ) ValueMin = 0;
    if ( ValueMax < 0 ) ValueMax = 0;
    }

NumColors     = how == Discrete ? Round ( ValueMax - ValueMin + 1 ) : MaxColoring;

ColorMin      = cmin;

ColorDelta    = cmax - cmin;
}


bool    TGLColoring::GetColor ( double v, TGLColor<GLfloat> &col )
{
if ( How == Discrete ) {

    int                 vi              = Round ( v - ValueMin );
                                        // Less bright colors, tries to compensate by also playing with the saturatrion
    col.SetHLS ( ( ( ( NumColors + 1 ) * vi / 2 ) % NumColors ) * 360 / NumColors,
//               0.25 + vi / (double) NumColors * 0.20,
                 0.25 + sqrt ( vi / (double) NumColors ) * 0.30,
                 0.50 + ( 1  - vi / (double) NumColors ) * 0.50 
                 );

//  col.SetHLS ( 0, 0.25 + (double) vi / 2 / NumColors, 0 );    // only luminance variation

    col.Alpha = ColorMin.Alpha + (double) vi / NumColors * ColorDelta.Alpha;

    return  v >= ValueMin && v <= ValueMax;
    }
else {
    col     = ColorMin + ColorDelta * sqrt ( v / NonNull ( v >= 0 ? ValueMax : ValueMin ) );

    return  true;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGLColorTable::TGLColorTable ()
{
TableType       = AbsColorTable_BlackWhite;
ZeroIndex       = 0;
DeltaIndex      = 0;
MaxIndex        = 0;
PMax            = 1;
NMax            = -1;
Contrast        = 1.0;                  // linear
Alpha           = 0;
MaxValue        = 1;
                                        // enable all tables
SetTableAllowed ( AllTables );
                                        // do a default init
Set ( AbsColorTable_BlackWhite );
}


void    TGLColorTable::Show  ( char *title )
{
char                buff[ 256 ];

for ( int ci = 0; ci < GLColorTableSize; ci++ ) {

    StringCopy      ( buff, "Entry:",   IntegerToString ( ci,                            3 ), "\t" );
    StringAppend    ( buff, "Red:",     IntegerToString ( Table[ ci ].Red   * UCHAR_MAX, 3 ), "\t" );
    StringAppend    ( buff, "Green:",   IntegerToString ( Table[ ci ].Green * UCHAR_MAX, 3 ), "\t" );
    StringAppend    ( buff, "Blue:",    IntegerToString ( Table[ ci ].Blue  * UCHAR_MAX, 3 ), "\t" );
    StringAppend    ( buff, "Alpha:",   IntegerToString ( Table[ ci ].Alpha * UCHAR_MAX, 3 )       );

    ShowMessage ( buff, StringIsEmpty ( title ) ? "ColorTable" : title, ShowMessageNormal );
    }
}


//----------------------------------------------------------------------------
                                        // Draw the color table itself, with markers and text from min and max values
void    TGLColorTable::Draw     (   const tagRECT&      paintrect,
                                    TGLColor<GLfloat>&  linecolor,  TGLColor<GLfloat>&  textcolor,
                                    int                 width,      int                 height,
                                    double              minvalue,   double              maxvalue,
                                    bool                showdata,   bool                showscaling,    int         precision,      const char*         units,
                                    TGLBitmapFont*      font
                                )
{
                                        // setting position, with limits
int                 paintrectwidth  = abs ( paintrect.right  - paintrect.left );
int                 paintrectheight = abs ( paintrect.bottom - paintrect.top  );

                    width           = NoMore ( paintrectwidth,                      width  );
                    height          = NoMore ( Truncate ( paintrectheight / 2.0 ),  height );

GLfloat             originx         = Truncate (   paintrectwidth  - width          );
GLfloat             originy         = Truncate ( ( paintrectheight - height ) / 2.0 );
GLfloat             originz         =  -0.5;    // furthest to the back


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing the frame background
glBegin ( GL_QUADS );
                                        // frame itself
linecolor.GLize ();

glVertex3f ( originx,         originy,          originz );
glVertex3f ( originx + width, originy,          originz );
glVertex3f ( originx + width, originy + height, originz );
glVertex3f ( originx        , originy + height, originz );

constexpr int     ColorMapBorderThickness   = 2;

originx    +=     ColorMapBorderThickness;
originy    +=     ColorMapBorderThickness;
width      -= 2 * ColorMapBorderThickness;   
height     -= 2 * ColorMapBorderThickness;

                                        // filling with "zero" color
//GLize ( ZeroIndex );
//glVertex3f ( originx,         originy,          originz );
//glVertex3f ( originx + width, originy,          originz );
//glVertex3f ( originx + width, originy + height, originz );
//glVertex3f ( originx        , originy + height, originz );

glEnd ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // filling with the actual colors
double              ci0         = ZeroIndex * height / MaxIndex;


glBegin ( GL_QUAD_STRIP );

for ( int ci = 0; ci <= height; ci++ ) {

    if ( ci < ci0 ) GLize ( NMax * ( ci - ci0 ) /          - ci0   );
    else            GLize ( PMax * ( ci - ci0 ) / ( height - ci0 ) );

    glVertex3f ( originx,         originy + ci,    originz );
    glVertex3f ( originx + width, originy + ci,    originz );
    }

glEnd ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the min/max positions
int                 maxindex        = Clip ( Round ( ZeroIndex + maxvalue * ( MaxIndex - ZeroIndex ) / PMax ), 0, MaxIndex );
int                 minindex        = Clip ( Round ( ZeroIndex - minvalue * ( MaxIndex - ZeroIndex ) / NMax ), 0, MaxIndex );
                                        // ?not this formula?
//int               maxindex        = Clip ( GetIndex ( maxvalue ), 0, colormax );
//int               minindex        = Clip ( GetIndex ( minvalue ), 0, colormax );


glEnable    ( GL_COLOR_LOGIC_OP );
glLogicOp   ( GL_OR_REVERSE     );


double              ipmax           = maxindex * height / MaxIndex;

if ( maxvalue > 0 ) {

    glColor3f ( 0, 0, 0 );

    glBegin ( GL_QUADS );

    glVertex3f ( originx,         originy + ipmax - 2,originz );
    glVertex3f ( originx + width, originy + ipmax - 2,originz );
    glVertex3f ( originx + width, originy + ipmax,    originz );
    glVertex3f ( originx,         originy + ipmax,    originz );

    glEnd ();
    }


double              ipmin           = minindex * height / MaxIndex;

if ( minvalue < 0 ) {

    glColor3f ( 0, 0, 0 );

    glBegin ( GL_QUADS );

    glVertex3f ( originx,         originy + ipmin + 2,originz );
    glVertex3f ( originx + width, originy + ipmin + 2,originz );
    glVertex3f ( originx + width, originy + ipmin,    originz );
    glVertex3f ( originx,         originy + ipmin,    originz );

    glEnd ();
    }



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // print data values at floating positions
char                buff[ 64 ];


textcolor.GLize ();


if ( showdata && maxvalue > 0 ) {

    FloatToString ( buff, maxvalue, fabs ( maxvalue ) < Power10 ( -precision ) ? 0 : precision );

    font->Print ( originx + width, originy + 4 + ipmax, originz, buff, TA_RIGHT | TA_BOTTOM );
    }


if ( showdata && minvalue < 0 ) {

    FloatToString ( buff, minvalue, fabs ( minvalue ) < Power10 ( -precision ) ? 0 : precision );

    font->Print ( originx + width, originy - 4 + ipmin, originz, buff, TA_RIGHT | TA_TOP );
    }


glDisable   ( GL_COLOR_LOGIC_OP );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // print scaling values at fixed position
if ( showscaling && maxvalue > 0 ) {

    FloatToString ( buff, PMax, fabs ( PMax ) < Power10 ( -precision ) ? 0 : precision );

    font->Print ( originx + width, originy + 4 + height + font->GetHeight(), originz, buff, TA_RIGHT | TA_BOTTOM );
    }

if ( showscaling && minvalue < 0 ) {

    FloatToString ( buff, NMax, fabs ( NMax ) < Power10 ( -precision ) ? 0 : precision );

    font->Print ( originx + width, originy - 4          - font->GetHeight(), originz, buff, TA_RIGHT | TA_TOP );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // print scaling values at fixed position
if ( StringIsNotEmpty ( units ) )

    font->Print ( originx + width, originy - 4          - 2 * font->GetHeight(), originz, units, TA_RIGHT | TA_BOTTOM );

}


//----------------------------------------------------------------------------
                                        // from a given index
void    TGLColorTable::GLize ( int param )
{
if ( param < 0 || param >= GetTableSize () )
    Table[ ZeroIndex ].GLize();
else
    Table[ param     ].GLize();
}

                                        // from a value, through current transformation
void    TGLColorTable::GLize ( double value )
{
GetColorIndex ( value, Color );

Color.GLize ();
}


bool    TGLColorTable::IsLuminance ()   const
{
return  TableType == AbsColorTable_BlackWhite
     || TableType == AbsColorTable_WhiteBlack
     || TableType == SignedColorTable_BlackWhiteBlack
     || TableType == SignedColorTable_BlackGrayWhite;
}


void    TGLColorTable::SetParameters    (   double  pmax,   double  nmax,   double  contrast,   double  alpha,  double  maxvalue )
{
                                        // store parameters, to enable calls from unknown objects
PMax        = pmax ? pmax :  1;
NMax        = nmax ? nmax : -1;
Contrast    = contrast;
Alpha       = alpha;
MaxValue    = fabs ( maxvalue );

                                        // avoiding / 0 in this mode, otherwise we allow MaxValue of 0
if ( Alpha == AlphaValueLinear && MaxValue == 0 )   
    MaxValue = 1;
}


void    TGLColorTable::SetTableAllowed ( ColorTablesEnum t )
{
if      ( t >= 0 && t < NumColorTables )

    TableAllowed[ t ] = true;

else if ( t == AllTables )

    for ( int i = 0; i < NumColorTables; i++ )
        TableAllowed[ i ] = true;

else if ( t == NoTables )

    for ( int i = 0; i < NumColorTables; i++ )
        TableAllowed[ i ] = false;
}


void    TGLColorTable::NextColorTable ( bool backward )
{
                                        // choose scan direction
int                 step            = backward ? NumColorTables - 1 : 1;
                                        // scan for next allowed table
                                        // nothing changed if only 1 or unknown table type
for ( int ns = 0, t = ( TableType + step ) % NumColorTables; ns < NumColorTables - 1; ns++, t = ( t + step ) % NumColorTables )

    if ( TableAllowed[ t ] ) {
        TableType   = (ColorTablesEnum) t;
        break;
        }

                                        // compute table according to its type
Set ( TableType );
}


int     TGLColorTable::GetIndex ( double value )    const
{
if ( value >= 0 )
    if ( value <= PMax  )               return  ZeroIndex + powl ( value / PMax, Contrast ) * DeltaIndex;

    else                                return  MaxIndex;
else
    if ( value >= NMax && ZeroIndex )   return  ZeroIndex - powl ( value / NMax, Contrast ) * DeltaIndex;

    else                                return  0;
}

/*                                        // trial: using a clipped linear/gamma curve, so as to have all colors shown for high contrasts, not only the highest colors
                                        // results are quite good, though every contrast formula should be revisited
                                        // Off for the moment, as one problem is the contrast also varying the alpha cut in slices display
int     TGLColorTable::GetIndex ( double value )
{
//double      c       = NoMore ( 0.95, Contrast );
double      c       = Clip ( Contrast, 0.01, 0.95 );


if ( value >= 0 ) {

    value  /= PMax;

//    if ( value <= 1 )   return  ZeroIndex + ( value > c ? ( value - c ) / ( 1 - c ) * DeltaIndex : 0 );   // clipping
    if ( value <= 1 )   return  ZeroIndex + ( value > c ? pow ( ( value - c ) / ( 1 - c ), c /*Contrast* / ) * DeltaIndex : 0 );   // both clipping and gamma
    else                return  MaxIndex;
    }

else
    if ( value >= NMax && ZeroIndex )   return  ZeroIndex - pow ( value / NMax, Contrast ) * DeltaIndex;
    else                                return  0;

}
*/

int     TGLColorTable::GetColorIndex ( double value, TGLColor<GLfloat> &glcol )     const
{
int             palindex    = GetIndex ( value );

                                        // copy color & default alpha
glcol   = Table[ palindex ];


                                        // constant, caller specified alpha
if      ( Alpha >  AlphaTable           )   glcol.Alpha = Alpha;                                                        // slightly negative threshold allows full slices with black background
                                        // variable alphas
else if ( Alpha == AlphaTable           )   ;

else if ( Alpha == AlphaLinear          )   glcol.Alpha = palindex == ZeroIndex ? 0 : fabs ( (double) ( palindex - ZeroIndex ) ) / DeltaIndex;

else if ( Alpha == AlphaLinearSaturated )   glcol.Alpha = palindex == ZeroIndex ? 0 : NoMore ( 1.0, 2 * fabs ( (double) ( palindex - ZeroIndex ) ) / DeltaIndex );

else if ( Alpha == AlphaSquare          )   glcol.Alpha  = Square ( ( palindex - ZeroIndex ) / (double) DeltaIndex );   // the square will remove the sign

else if ( Alpha == AlphaValueLinear     )   glcol.Alpha = fabs ( value ) / MaxValue;

else if ( Alpha == AlphaBool            )   glcol.Alpha = (bool) value;

else if ( Alpha == AlphaGreater         )   glcol.Alpha = fabs ( value ) >= MaxValue ? 0.45 : 0;                        // alpha value could be given somehow...

else                                        glcol.Alpha = 1.0;

//if ( VkQuery () ) DBGV4 ( value, palindex, Alpha, glcol.Alpha, "value, palindex, Alpha, glcol.Alpha" );


return palindex;
}


//----------------------------------------------------------------------------
void    TGLColorTable::Set ( ColorTablesEnum tabletype )
{
                                        // legal table requested?
if ( tabletype < 0 || tabletype >= NumColorTables || ! TableAllowed[ tabletype ] )
    return;


TableType   = tabletype >= 0 && tabletype < NumColorTables ? tabletype : UnknownTable;

                                        // set 0, delta & max according to table type
if ( IsSignedTable () ) {
    ZeroIndex   = SignedColorTable_0;
    DeltaIndex  = SignedColorTable_Delta;
    MaxIndex    = SignedColorTable_Max;
    }
else {
    ZeroIndex   = AbsColorTable_0;
    DeltaIndex  = AbsColorTable_Delta;
    MaxIndex    = AbsColorTable_Max;
    }

                                        // reset last index / color to none
Index1      = -1;
Color1.Set ( 0, 0, 0 );


switch ( tabletype ) {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Absolute tables, starting from 0

  case AbsColorTable_BlackWhite:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00 ), InterpolateColorsLogBackward, 20 );

    break;


  case AbsColorTable_WhiteBlack:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( GLColorNearlyWhite, 0.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.00 ), InterpolateColorsLogBackward, 20 );

    break;


  case AbsColorTable_BlackYellowWhite:
    #define     WhiteEndRatio   0.84

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00          ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * WhiteEndRatio ),         TGLColor<GLfloat> ( 1.00, 1.00, 0.00, WhiteEndRatio ), InterpolateColorsLinear         );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00        ), InterpolateColorsLogForward, 20 );

    break;


  case AbsColorTable_GrayYellow:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 0.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ), InterpolateColorsLogBackward, 20 );

    break;


  case AbsColorTable_BlackRed:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0       ),             TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00          ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 2.0 - 1 ),         TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00          ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 2.0 ),             TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 1.00          ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 2.0 ),             TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 1.00          ), InterpolateColorsLinear );

    break;


  case AbsColorTable_GrayGreenYellowRed:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 / 3.0 ),               TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 0 / 3.0 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 3.0 ),               TGLColor<GLfloat> ( 0.00, 0.71, 0.00, 1 / 3.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 3.0 ),               TGLColor<GLfloat> ( 0.86, 0.86, 0.00, 2 / 3.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 3.0 ),               TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 3 / 3.0 ), InterpolateColorsLinear );

    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


  case AbsColorTable_DarkRedYellowGreenCyanBlueMagenta:

//  #define             RYGCBM1             1.00, 0.00, 0.00, 0.00
//  #define             RYGCBM2             1.00, 1.00, 0.00, 0.20
//  #define             RYGCBM3             0.00, 1.00, 0.00, 0.40
//  #define             RYGCBM4             0.00, 1.00, 1.00, 0.60
//  #define             RYGCBM5             0.00, 0.00, 1.00, 0.80
//  #define             RYGCBM6             1.00, 0.00, 1.00, 1.00

    #define             RYGCBM1             0.80, 0.15, 0.15, 0.00
    #define             RYGCBM2             0.40, 0.40, 0.20, 0.20
    #define             RYGCBM3             0.00, 0.45, 0.00, 0.40
    #define             RYGCBM4             0.00, 0.36, 0.36, 0.60
    #define             RYGCBM5             0.00, 0.00, 0.60, 0.80
    #define             RYGCBM6             0.55, 0.00, 0.55, 1.00


    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM1 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM2 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM3 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM4 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 4 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM5 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 5 / 5.0 ),               TGLColor<GLfloat> ( RYGCBM6 ), InterpolateColorsLinear );

//  NormalizeColors ( 0, GLColorTableSize - 1, 100 );   // -> brighter, but the table looks like a dark one

    break;


  case AbsColorTable_MagentaBlueCyanGrayGreenYellowRed:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 / 6.0 ),               TGLColor<GLfloat> ( 0.78, 0.00, 0.78, 0 / 6.0 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.00, 1.00, 1 / 6.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.78, 0.78, 2 / 6.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 6.0 ),               TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 3 / 6.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 4 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.71, 0.00, 4 / 6.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 5 / 6.0 ),               TGLColor<GLfloat> ( 0.86, 0.86, 0.00, 5 / 6.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 6 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 6 / 6.0 ), InterpolateColorsLinear );

    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


  case AbsColorTable_BlackYellowWhiteMRIcro:
                                        // another version with more contrasted borders, best with  AlphaTable
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.00 ),                  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00    ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00    ), InterpolateColorsLinear          );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.60 ),                  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.90    ), InterpolateColorsLogForward, 200 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.85 ),                  TGLColor<GLfloat> ( 0.90, 0.90, 0.00, 1.00    ), InterpolateColorsLogForward, 200 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1.00 ),                  TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00  ), InterpolateColorsLogForward,  20 );

    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


  case AbsColorTable_SingleBump:
                                        // trial: black -> white -> black, to show slices of intensities

                                        // background: cst dark grey
//  InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ) );
//  InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ), InterpolateColorsLogBackward, 20 );

                                        // background: dark levels
//  InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ) );
//  InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ), InterpolateColorsLogBackward, 20 );

/*
    #define         bbwdeltai       0.05

                                        // background: dark wide bump from afar
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ) );
                        Round ( ZeroIndex + DeltaIndex * ( 0.50 - bbwdeltai ) ),  TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ), InterpolateColorsLogForward, 1 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * ( 0.50 + bbwdeltai ) ),  TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ),
                        Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ), InterpolateColorsLogBackward, 1 );
                                        // bright narrow bump
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * ( 0.50 - bbwdeltai ) ),  TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ),
                        Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00 ), InterpolateColorsLogForward, 1000 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00 ),
                        Round ( ZeroIndex + DeltaIndex * ( 0.50 + bbwdeltai ) ),  TGLColor<GLfloat> ( 0.30, 0.30, 0.30, 0.00 ), InterpolateColorsLogBackward, 1000 );
*/

/*
    #define         bbwdeltai       0.125

                                        // black background
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ),
                        Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ), InterpolateColorsLinear );

                                        // one bump in the middle
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * ( 0.50 - bbwdeltai ) ),  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ),
                        Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ), InterpolateColorsLogForward,  10 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ),
                        Round ( ZeroIndex + DeltaIndex * ( 0.50 + bbwdeltai ) ),  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ), InterpolateColorsLogBackward, 10 );

    Table[ Truncate ( ZeroIndex + DeltaIndex * 0.50 )     ]    = TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 );
    Table[ Truncate ( ZeroIndex + DeltaIndex * 0.50 ) - 1 ]    =
    Table[ Truncate ( ZeroIndex + DeltaIndex * 0.50 ) + 1 ]    = TGLColor<GLfloat> ( 1.00, 1.00, 0.50, 1.00 );
*/

    double              b;
    int                 i;

    for ( i = ZeroIndex; i <= MaxIndex; i++ ) {

        b       = Power ( ( 1 - cos ( (double) i / MaxIndex * TwoPi ) ) / 2, 10 );
//      b       = Square ( ( 1 - cos ( (double) i / MaxIndex * TwoPi * 3 ) ) / 2 );
//      b       = Square ( ( 1 - cos ( Power ( (double) i / MaxIndex, 0.25 ) * TwoPi * 2 ) ) / 2 );

                                       // yellow going to white in the center only         show more of the lower values
        Table[ i ]  = TGLColor<GLfloat> ( b, b, Clip ( Power ( b, 3 ) * 3 - 2, 0.0, 1.0 ), sqrt ( b ) );
        }

//    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cyclic tables
  case CyclicColorTable_WhiteBlackWhite:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 0.11, 0.11, 0.11, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 ), InterpolateColorsCosine );

    break;


  case CyclicColorTable_WhiteBlackWhiteBlackWhite:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.25 ),                  TGLColor<GLfloat> ( 0.11, 0.11, 0.11, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.75 ),                  TGLColor<GLfloat> ( 0.11, 0.11, 0.11, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00 ), InterpolateColorsCosine );

    break;


  case CyclicColorTable_GreenBlueMagentaRedGreen:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.00 ),                  TGLColor<GLfloat> ( 0.00, 1.00, 0.00, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.25 ),                  TGLColor<GLfloat> ( 0.00, 0.00, 0.70, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 0.30, 0.00, 0.30, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.75 ),                  TGLColor<GLfloat> ( 0.80, 0.10, 0.10, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1.00 ),                  TGLColor<GLfloat> ( 0.00, 1.00, 0.00, 1.00 ), InterpolateColorsCosine );

    break;


  case CyclicColorTable_YellowCyanBlueMagentaYellow:

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.25 ),                  TGLColor<GLfloat> ( 0.00, 0.31, 0.31, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.50 ),                  TGLColor<GLfloat> ( 0.50, 0.50, 1.00, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 0.75 ),                  TGLColor<GLfloat> ( 0.50, 0.10, 0.50, 1.00 ), InterpolateColorsCosine );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1.00 ),                  TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ), InterpolateColorsCosine );

    break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Signed tables

  case SignedColorTable_BlackWhiteBlack:

    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.00   ) );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( GLColorNearlyWhite, 0.00 ), InterpolateColorsLogForward,  20 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.00   ), InterpolateColorsLogBackward, 20 );

    break;


  case SignedColorTable_BlackGrayWhite:

    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.00   ) );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.65, 0.65, 0.65, 0.00   ), InterpolateColorsLogForward, 20 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00 ), InterpolateColorsLogForward, 20 );

    break;


  case SignedColorTable_BlueWhiteRed:

    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 0.00, 1.00, 1.00   ) );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( GLColorNearlyWhite, 0.00 ), InterpolateColorsLogForward,  20 );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 1.00   ), InterpolateColorsLogBackward, 20 );

    NormalizeColors ( 0, GLColorTableSize - 1, 95 );

    break;


  case SignedColorTable_CyanBlackYellow:
    #define     WhiteEndRatioS  0.94

//  InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.00, 1.00, 1.00, 1.00 ) );
//  InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00 ), InterpolateColorsLogForward, 100 );
//  InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00 ), InterpolateColorsLogBackward, 100 );


    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 1.00, 1.00, 1.00, 1.00   ) );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * WhiteEndRatioS ),        TGLColor<GLfloat> ( 0.00, 0.80, 1.00, 1.00   ), InterpolateColorsLogForward, 20 );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 0.00   ), InterpolateColorsLinear         );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * WhiteEndRatioS ),        TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.00   ), InterpolateColorsLinear         );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 ),                     TGLColor<GLfloat> ( GLColorNearlyWhite, 1.00 ), InterpolateColorsLogForward, 20 );

    break;


  case SignedColorTable_GrayGreenYellowRed:
                                        // don't show negative part, used for templates which have been average referenced and we don't want to show negative data
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 ),                     TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 0.00    ) );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 0.00    ), InterpolateColorsLinear );

    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 3.0 ),               TGLColor<GLfloat> ( 0.00, 0.71, 0.00, 1 / 3.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 3.0 ),               TGLColor<GLfloat> ( 0.86, 0.86, 0.00, 2 / 3.0 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 3.0 ),               TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 3 / 3.0 ), InterpolateColorsLinear );

    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


  case SignedColorTable_MagentaBlueCyanGrayGreenYellowRed:

    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 3 / 3.0 ),               TGLColor<GLfloat> ( 0.78, 0.00, 0.78, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 2 / 3.0 ),               TGLColor<GLfloat> ( 0.00, 0.00, 1.00, 0.66 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 / 3.0 ),               TGLColor<GLfloat> ( 0.00, 0.78, 0.78, 0.33 ), InterpolateColorsLinear );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.51, 0.51, 0.51, 0.00 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 3.0 ),               TGLColor<GLfloat> ( 0.00, 0.71, 0.00, 0.33 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 3.0 ),               TGLColor<GLfloat> ( 0.86, 0.86, 0.00, 0.66 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 3.0 ),               TGLColor<GLfloat> ( 1.00, 0.00, 0.00, 1.00 ), InterpolateColorsLinear );

    NormalizeColors ( 0, GLColorTableSize - 1, 100 );

    break;


  case SignedColorTable_NeuroscanColors:
/*                                      // discrete version
InterpolateColors ( LutSpNsColor,  TColor(   0,   0, 128), TColor(   0,   0, 128), 0,                          LUT_SP_SIZE     / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor(   0,   0, 255), TColor(   0,   0, 255), LUT_SP_SIZE     / 13,       LUT_SP_SIZE * 2 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor(   0,  64, 255), TColor(   0,  64, 255), LUT_SP_SIZE * 2 / 13,       LUT_SP_SIZE * 3 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor(   0, 128, 255), TColor(   0, 128, 255), LUT_SP_SIZE * 3 / 13,       LUT_SP_SIZE * 4 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor(   0, 255, 255), TColor(   0, 255, 255), LUT_SP_SIZE * 4 / 13,       LUT_SP_SIZE * 5 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor(   0, 255, 128), TColor(   0, 255, 128), LUT_SP_SIZE * 5 / 13,       LUT_SP_SIZE * 6 / 13,       InterpolateColorsLinear );

InterpolateColors ( LutSpNsColor,  TColor(   0, 255,   0), TColor(   0, 255,   0), LUT_SP_SIZE * 6 / 13,       LUT_SP_SIZE * 7 / 13,       InterpolateColorsLinear );

InterpolateColors ( LutSpNsColor,  TColor( 128, 255,   0), TColor( 128, 255,   0), LUT_SP_SIZE * 7 / 13,       LUT_SP_SIZE * 8 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor( 255, 255,   0), TColor( 255, 255,   0), LUT_SP_SIZE * 8 / 13,       LUT_SP_SIZE * 9 / 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor( 255, 192,   0), TColor( 255, 192,   0), LUT_SP_SIZE * 9 / 13,       LUT_SP_SIZE * 10/ 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor( 255, 128,   0), TColor( 255, 128,   0), LUT_SP_SIZE * 10/ 13,       LUT_SP_SIZE * 11/ 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor( 255,  64,   0), TColor( 255,  64,   0), LUT_SP_SIZE * 11/ 13,       LUT_SP_SIZE * 12/ 13,       InterpolateColorsLinear );
InterpolateColors ( LutSpNsColor,  TColor( 255,   0, 128), TColor( 255,   0, 128), LUT_SP_SIZE * 12/ 13,       LUT_SP_SIZE-1,              InterpolateColorsLinear );
*/

    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 6 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.00, 0.50, 1.00 ) );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 5 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.00, 1.00, 0.83 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 4 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.25, 1.00, 0.66 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 3 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 0.50, 1.00, 0.50 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 2 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 1.00, 1.00, 0.33 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex - DeltaIndex * 1 / 6.0 ),               TGLColor<GLfloat> ( 0.00, 1.00, 0.50, 0.17 ), InterpolateColorsLinear );
    InterpolateColors ( ZeroIndex,                                                TGLColor<GLfloat> ( 0.00, 1.00, 0.00, 0.00 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 1 / 6.0 ),               TGLColor<GLfloat> ( 0.50, 1.00, 0.00, 0.17 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 2 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 0.33 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 3 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 0.75, 0.00, 0.50 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 4 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 0.50, 0.00, 0.66 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 5 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 0.25, 0.00, 0.83 ), InterpolateColorsLinear );
    InterpolateColors ( Round ( ZeroIndex + DeltaIndex * 6 / 6.0 ),               TGLColor<GLfloat> ( 1.00, 0.00, 0.50, 1.00 ), InterpolateColorsLinear );

    break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // switch tabletype

                                        // duplicate last useful entry, just to have a complete initialization of the table
if ( IsSignedTable () )
    Table[ MaxIndex + 1 ]   = Table[ MaxIndex ];

}


//----------------------------------------------------------------------------
                                        // Interpolate between 2 colors: stored Color1 and provided color2, to be distributed from indexes Index1 to index2
                                        // It needs only the second color/index, as first one is stored from a previous call
                                        // Interpolation method can vary to achieve different visual effects
void    TGLColorTable::InterpolateColors ( int                      index2, TGLColor<GLfloat>   color2,
                                           InterpolateColorsEnum    how,    double              logstrength )
{
                                        // first call?
if ( Index1 < 0 ) {
                                        // just store and returns
    Index1      = index2;
    Color1      = color2;
    return;
    }

                                        // safety checks
Clipped ( Index1, index2, 0, GLColorTableSize - 1 );


int                 i;
double              deltai          = index2 - Index1;
double              w;
TGLColor<GLfloat>  *toc;


for ( i = Index1, toc = Table + i; i <= index2 ; i++, toc++ ) {

    if      ( how == InterpolateColorsLogForward ) {

        w           =   ( log ( i - Index1 + logstrength ) - log ( logstrength ) )
                      / ( log ( deltai     + logstrength ) - log ( logstrength ) );

                                        // log interpolation for color components
        toc->Red    = Color1.Red   + w * ( color2.Red   - Color1.Red   );
        toc->Green  = Color1.Green + w * ( color2.Green - Color1.Green );
        toc->Blue   = Color1.Blue  + w * ( color2.Blue  - Color1.Blue  );
                                        // linear interpolation for the alpha
        toc->Alpha  = Color1.Alpha + (double) ( i - Index1 ) / deltai * ( color2.Alpha - Color1.Alpha );
        }

    else if ( how == InterpolateColorsLogBackward ) {

        w           =   ( log ( index2 - i + logstrength ) - log ( logstrength ) )
                      / ( log ( deltai     + logstrength ) - log ( logstrength ) );

                                        // log interpolation for color components
        toc->Red    = color2.Red   + w * ( Color1.Red   - color2.Red   );
        toc->Green  = color2.Green + w * ( Color1.Green - color2.Green );
        toc->Blue   = color2.Blue  + w * ( Color1.Blue  - color2.Blue  );
                                        // linear interpolation for the alpha
        toc->Alpha  = color2.Alpha + (double) ( index2 - i ) / deltai * ( Color1.Alpha - color2.Alpha );
        }
                                        // cosine, as linear, is symmetrical and can be used for increasing as well as for decreasing intensities
    else if ( how == InterpolateColorsCosine ) {
                                                          // half Hanning, just the part 0->1
        w           = Hanning ( (double) ( i - Index1 ) / deltai / 2 );

                                        // cosine interpolation for color components
        toc->Red    = Color1.Red   + w * ( color2.Red   - Color1.Red   );
        toc->Green  = Color1.Green + w * ( color2.Green - Color1.Green );
        toc->Blue   = Color1.Blue  + w * ( color2.Blue  - Color1.Blue  );
                                        // linear interpolation for the alpha
        toc->Alpha  = Color1.Alpha + (double) ( i - Index1 ) / deltai * ( color2.Alpha - Color1.Alpha );
        }

    else { // if ( how == InterpolateColorsLinear ) {

        w           = (double) ( i - Index1 ) / deltai;

                                        // all linear interpolation
        toc->Red    = Color1.Red   + w * ( color2.Red   - Color1.Red   );
        toc->Green  = Color1.Green + w * ( color2.Green - Color1.Green );
        toc->Blue   = Color1.Blue  + w * ( color2.Blue  - Color1.Blue  );
        toc->Alpha  = Color1.Alpha + w * ( color2.Alpha - Color1.Alpha );
        }
    }

                                        // this is what we wanted: storing the last updated index / color, so that next call will start from it
Index1      = index2;
Color1      = color2;
}


//----------------------------------------------------------------------------
                                        // Normalize to give equal perceived intensity in the L*ab color space
void    TGLColorTable::NormalizeColors ( int i1, int i2, double norm )
{
int                 i;
double              r;
double              g;
double              b;
//double            x;
double              y;
//double            z;
double              lsuv;
double              lsab;
double              ratio;
double              m;
double              yn              = 1.0;  // max value for white
TGLColor<GLfloat>  *toc;


for ( i = i1, toc = Table + i1; i <= i2 ; i++, toc++ ) {

    r       = toc->Red;
    g       = toc->Green;
    b       = toc->Blue;

//  x       = 0.412453 * r + 0.357580 * g + 0.180423 * b;
    y       = 0.212671 * r + 0.715160 * g + 0.072169 * b;
//  z       = 0.019334 * r + 0.119193 * g + 0.950227 * b;

    lsuv    = 116 * ( y > 0 ? Power ( ( y / yn ), 1.0 / 3 ) : 0 ) - 16;
    lsab    = ( y > 0.008856 ) ? lsuv : 903.3 * y / yn;

//  ratio = 0.3 * (double) lut[i].Red() + 0.6 * (double) lut[i].Green() + 0.1 * (double) lut[i].Blue();

    if ( lsab > 0 )
        ratio = norm / lsab;
    else
        ratio = lsab;

    r       = toc->Red   * ratio;
    g       = toc->Green * ratio;
    b       = toc->Blue  * ratio;

    m       = max ( r, g, b );

    if ( m > 1.0 ) {
        r  /= m;
        g  /= m;
        b  /= m;
        }

    toc->Red    = r;
    toc->Green  = g;
    toc->Blue   = b;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
