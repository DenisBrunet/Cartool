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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD>
class   TGLColor :  public TGLObject
{
public:
                    TGLColor ()                                                     { Red = Green = Blue = Alpha = 0; }
//                  TGLColor ( TypeD r, TypeD g, TypeD b, TypeD a )                 { Red = r; Green = g; Blue = b; Alpha = a; }
                    TGLColor ( GLfloat  r, GLfloat  g, GLfloat  b, GLfloat  a )     { Red = (TypeD) r; Green = (TypeD) g; Blue = (TypeD) b; Alpha = (TypeD) a; }
                    TGLColor ( GLdouble r, GLdouble g, GLdouble b, GLdouble a )     { Red = (TypeD) r; Green = (TypeD) g; Blue = (TypeD) b; Alpha = (TypeD) a; }


    void            Set         ( GLdouble r, GLdouble g, GLdouble b )              { Red = r; Green = g; Blue = b; }
    void            Set         ( GLdouble r, GLdouble g, GLdouble b, GLdouble a )  { Red = r; Green = g; Blue = b; Alpha = a; }
    void            SetAlpha    ( GLdouble a )                                      { Alpha = a; }
    void            SetHLS      ( double H, double L, double S );

    void            GLize   ( int param = 0 );


                    operator TypeD* ()                                              { return (TypeD *) &Red; }  // cast

    TGLColor        operator    -   ( const TGLColor& op2 );
    TGLColor        operator    +   ( const TGLColor& op2 );
    TGLColor        operator    *   ( double op2 );


    TypeD           Red;
    TypeD           Green;
    TypeD           Blue;
    TypeD           Alpha;
};


//----------------------------------------------------------------------------
                                        // A multicolor object embeds up to 4 colors, which will be context-sensitively used
constexpr int   MultiColorMax           = 4;


template <class TypeD>
class   TGLMultiColor :  public TGLObject
{
public:
                    TGLMultiColor ()        { NumColors = 0; }


    void            Set ( int ci, GLfloat r, GLfloat g, GLfloat b, GLfloat a );
    void            Set ( int ci, GLdouble r, GLdouble g, GLdouble b, GLdouble a );
    TypeD*          Get ( int ci );

    void            GLize   ( int param = 0 );


                    operator TypeD* ()      { return (TypeD *) Color[0]; }  // cast

    TGLColor<TypeD> &operator[] ( int i )   { return Color[ i ]; }


protected:
    int             NumColors;
    TGLColor<TypeD> Color[ MultiColorMax ];
};


//----------------------------------------------------------------------------
                                        // Historically, we could use some texture indexed colors, and needed a size of 256 - Now not anymore, so this size could be anything for the moment
constexpr int   GLColorTableSize        = 256;

constexpr int   AbsColorTable_0         = 0;
constexpr int   AbsColorTable_Delta     = 255;
constexpr int   AbsColorTable_Max       = 255;
                                        // signed table: one 0 in middle -> range is odd -> ignore table last entry
constexpr int   SignedColorTable_0      = 127;
constexpr int   SignedColorTable_Delta  = 127;
constexpr int   SignedColorTable_Max    = 254;

                                        // NOT mathematically white, as screenshots are done on a white background
                                        // and with some high saturation, this might brings a lot of this nearly-white color
                                        // Alpha blending is not specified here, this has to be done on a case by case basis
#define GLColorNearlyWhite              0.98, 0.98, 0.98


enum    ColorTablesEnum 
        {
                                        // Special values, not real tables
        UnknownTable                        = -1,
        AllTables                           = -2,
        NoTables                            = -3,

                                        // Absolute data tables
        AbsColorTableMin                    = 0,
        AbsColorTable_BlackWhite            = AbsColorTableMin,     // 2 colors
        AbsColorTable_WhiteBlack,
        AbsColorTable_BlackYellowWhite,
        AbsColorTable_GrayYellow,
        AbsColorTable_BlackYellowWhiteMRIcro,                       // contrasty brightness
        AbsColorTable_BlackRed,                                     // contrasty black then full red
        AbsColorTable_GrayGreenYellowRed,                           // 4 colors
        AbsColorTable_DarkRedYellowGreenCyanBlueMagenta,            // 6 colors
        AbsColorTable_MagentaBlueCyanGrayGreenYellowRed,
        AbsColorTable_SingleBump,                                   // a single bump in the middle, to show a narrow range of values

                                        // Cyclic/angular data tables
        CyclicColorTableMin,
        CyclicColorTable_WhiteBlackWhite    = CyclicColorTableMin,
        CyclicColorTable_WhiteBlackWhiteBlackWhite,
        CyclicColorTable_GreenBlueMagentaRedGreen,
        CyclicColorTable_YellowCyanBlueMagentaYellow,

                                        // Signed data tables
        SignedColorTableMin,
        SignedColorTable_WhiteBlackWhite    = SignedColorTableMin,  // 2 colors
        SignedColorTable_BlackWhiteBlack,
        SignedColorTable_WhiteYellowBlackYellowWhite,
        SignedColorTable_WhiteYellowBlackYellowWhiteMRIcro,
        SignedColorTable_RedBlackRed,
        SignedColorTable_BlackGrayWhite,
        SignedColorTable_RedYellowGreenGrayGreenYellowRed,          // 4 colors
        SignedColorTable_HalfGrayGreenYellowRed,                        
        SignedColorTable_MagentaBlueCyanGrayGreenYellowRed,         // 7 colors
        SignedColorTable_NeuroscanColors,                           // 13 colors
        SignedColorTable_BlueWhiteRed,                              // 3 colors
        SignedColorTable_WhiteBlueBlackRedWhite,
        SignedColorTable_CyanBlackYellow,

        NumColorTables,
        };


inline  bool    IsSignedTable ( ColorTablesEnum c )     { return    c >= SignedColorTableMin; }


enum    InterpolateColorsEnum 
        {
        InterpolateColorsLinear,
        InterpolateColorsLogForward,
        InterpolateColorsLogBackward,
        InterpolateColorsCosine,

        NumInterpolateColors,
        };

                                        // Alpha behavior
enum {                                          // any >= 0 value -> take value as alpha
        AlphaTable              = -1,           // take the alpha from the color table
        AlphaLinear             = -2,           // alpha linear
        AlphaLinearSaturated    = -3,           // alpha linear * 2
        AlphaSquare             = -4,           // alpha square
        AlphaValueLinear        = -5,
        AlphaBool               = -6,
        AlphaGreater            = -7,
    };

class   TGLBitmapFont;


class   TGLColorTable : public TGLObject
{
public:
                    TGLColorTable ();


    double          Alpha;
    double          MaxValue;

    TGLColor<GLfloat>   Table[ GLColorTableSize ];


    void            GLize   ( int param = 0 );
    void            GLize   ( double value );


    bool            IsLuminance     ()  const;
    bool            IsSignedTable   ()  const   { return  crtl::IsSignedTable ( TableType ); }


    void            Set             ( ColorTablesEnum tabletype );
    void            SetTableAllowed ( ColorTablesEnum t );
    void            NextColorTable  ( bool backward );
    void            SetParameters   ( double pmax, double nmax, double contrast, double alpha, double maxvalue = 0 );
    void            SetAlpha        ( double alpha )   { Alpha = alpha; }


    int             GetZeroIndex    ()  const   { return ZeroIndex; }
    int             GetMaxIndex     ()  const   { return MaxIndex; }
    double          GetPMax         ()  const   { return PMax; }
    double          GetNMax         ()  const   { return NMax; }
    double          GetContrast     ()  const   { return Contrast; }
    int             GetDeltaIndex   ()  const   { return DeltaIndex; }
    int             GetIndex        ( double value )                            const;
    int             GetColorIndex   ( double value, TGLColor<GLfloat> &glcol )  const;
    int             GetTableSize    ()  const   { return GLColorTableSize; }
    ColorTablesEnum GetTableType    ()  const   { return TableType; }


    void            InterpolateColors ( int index2, TGLColor<GLfloat> color2, InterpolateColorsEnum how = InterpolateColorsLinear, double logstrength = 2 );
    void            NormalizeColors   ( int i1, int i2, double norm );

    void            Show  ( char *title = 0 );
    void            Draw            (   const tagRECT&      paintrect,
                                        TGLColor<GLfloat>&  linecolor,  TGLColor<GLfloat>&  textcolor,
                                        int                 width,      int                 height,
                                        double              minvalue,   double              maxvalue,
                                        bool                showdata,   bool                showscaling,    int         precision,      const char*         units,
                                        TGLBitmapFont*      font
                                    );

    TGLColor<GLfloat>  &operator[] ( int i )    { return Table[ i ]; }


protected:
    ColorTablesEnum TableType;          // current type
    bool            TableAllowed[ NumColorTables ]; // tables currently allowed

    int             ZeroIndex;         // index for value 0
    int             MaxIndex;          // last color index
    int             DeltaIndex;        // difference between index 0 to index max
    double          PMax;
    double          NMax;
    double          Contrast;


private:
    TGLColor<GLfloat>   Color;          // needed in GLize

    int                 Index1;         // used when setting a colortable
    TGLColor<GLfloat>   Color1;         // 
};


//----------------------------------------------------------------------------
enum    ColoringEnum
        {
        Discrete,
        Analog      
        };

constexpr int   MaxColoring             = 256;
                                        // translate a value to a color

class   TGLColoring
{
public:
                    TGLColoring ()          { Ref           = 0;
                                              How           = Analog;
                                              ValueMin      = ValueMax      = 0;
                                              NumColors     = 0; }

                    TGLColoring ( int ref, ColoringEnum how, double minv, double maxv, TGLColor<GLfloat> cmin, TGLColor<GLfloat> cmax );


    int             GetRef ()               { return Ref; }
    ColoringEnum    GetHow ()               { return How; }
    double          GetValueMin ()          { return ValueMin; }
    double          GetValueMax ()          { return ValueMax; }

    bool            GetColor ( double v, TGLColor<GLfloat> &col );

    void            GLize ( int param )     { GetColor ( param, ColorReturned ); ColorReturned.GLize (); }


protected:
    int                 Ref;
    ColoringEnum        How;
    double              ValueMin;
    double              ValueMax;
    int                 NumColors;
    TGLColor<GLfloat>   ColorMin;
    TGLColor<GLfloat>   ColorDelta;
    TGLColor<GLfloat>   ColorReturned;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
void     TGLColor<TypeD>::SetHLS ( double H, double L, double S )
{
double  value = L <= 0.5 ? L * ( 1.0 + S ) : L + S * ( 1 - L );

if ( value <= 0 ) {
    Red = Green = Blue = 0;
    return;
    }


double  m;
double  sv;
int     sextant;
double  fract, vsf;

m       = L + L - value;
sv      = ( value - m ) / value;
H       /= 60;
sextant = ( (int) H ) % 6;
fract   = H - sextant;
vsf     = value * sv * fract;

switch ( sextant ) {
  case 0:
    Red     = value;        Green   = m + vsf;      Blue    = m;
    break;
  case 1:
    Red     = value - vsf;  Green   = value;        Blue    = m;
    break;
  case 2:
    Red     = m;            Green   = value;        Blue    = m + vsf;
    break;
  case 3:
    Red     = m;            Green   = value - vsf;  Blue    = value;
    break;
  case 4:
    Red     = m + vsf;      Green   = m;            Blue    = value;
    break;
  case 5:
    Red     = value;        Green   = m;            Blue    = value - vsf;
    break;
    }
}


template <class TypeD>
TGLColor<TypeD> TGLColor<TypeD>::operator- ( const TGLColor& op2 )
{
TGLColor<TypeD>  temp;

temp.Red    = (TypeD) ( Red   - op2.Red );
temp.Green  = (TypeD) ( Green - op2.Green );
temp.Blue   = (TypeD) ( Blue  - op2.Blue );
temp.Alpha  = (TypeD) ( Alpha - op2.Alpha );

return  temp;
}


template <class TypeD>
TGLColor<TypeD> TGLColor<TypeD>::operator+ ( const TGLColor& op2 )
{
TGLColor<TypeD>  temp;

temp.Red    = (TypeD) ( Red   + op2.Red );
temp.Green  = (TypeD) ( Green + op2.Green );
temp.Blue   = (TypeD) ( Blue  + op2.Blue );
temp.Alpha  = (TypeD) ( Alpha + op2.Alpha );

return  temp;
}


template <class TypeD>
TGLColor<TypeD> TGLColor<TypeD>::operator* ( double op2 )
{
TGLColor<TypeD>  temp;

temp.Red    = (TypeD) ( op2 * Red );
temp.Green  = (TypeD) ( op2 * Green );
temp.Blue   = (TypeD) ( op2 * Blue );
//temp.Alpha  = (TypeD) ( op2 * Alpha );
temp.Alpha  = Alpha;

return  temp;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
void    TGLMultiColor<TypeD>::Set ( int ci, GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
                                        // no more than 4 colors
if ( ci < 0 || ci > MultiColorMax - 1 )
    return;

NumColors   = max ( NumColors, ci + 1 );

Color[ci].Set ( r, g, b, a );
}


template <class TypeD>
void    TGLMultiColor<TypeD>::Set ( int ci, GLdouble r, GLdouble g, GLdouble b, GLdouble a )
{
                                        // no more than 4 colors
if ( ci < 0 || ci > MultiColorMax - 1 )
    return;

NumColors   = max ( NumColors, ci + 1 );

Color[ci].Set ( r, g, b, a );
}


template <class TypeD>
void    TGLMultiColor<TypeD>::GLize ( int param )
{
if ( param < 0 || param > NumColors - 1 )
    return;

Color[param].GLize();
}


template <class TypeD>
TypeD  *TGLMultiColor<TypeD>::Get ( int ci )
{
if ( ci < 0 || ci > NumColors - 1 )
    return 0;

return (TypeD *) Color[ci];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
