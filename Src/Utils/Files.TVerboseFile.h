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

#include    <iostream>
#include    <iomanip>
#include    <io.h>

#include    "Strings.Utils.h"
#include    "Strings.TStrings.h"

#include    "Files.Utils.h"
#include    "Files.Stream.h"

#include    "TCartoolApp.h"             // TCartoolObjects

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       VerboseFileDefaultWidth     = 40;


class   TVerboseFile
{
public:
    inline          TVerboseFile ();
    inline          TVerboseFile ( const char* file, int desclength );
    inline         ~TVerboseFile ();

                                        // For outputting a table
    TStrings        TableColNames;      // holding: row title (could be empty) then n columns -> n + 1 strings
    TStrings        TableRowNames;


    inline void    Open    ( const char* file, int desclength );
    inline bool    IsOpen  ()      const           { return ofv != 0; }
    inline void    Close   ();
    inline void    Flush   ();


    inline void    PutTitle        ( const char* title )   const;
    inline void    NextLine        ( int numlines = 1 )    const;
    inline void    NextTopic       ( const char* topic )   const;
    inline void    NextBlock       ()                      const;

    inline void    Put             ( const char* desc );
    inline void    Put             ( const char* desc, const char   char1 );
    inline void    Put             ( const char* desc, const char*  text1, const char* text2 = 0, const char* text3 = 0 );
    inline void    Put             ( const char* desc, const char*  text1, int value, const char* text2 = 0 );
    inline void    Put             ( const char* desc, int          value, int width = 0,      const char* text1 = 0, const char* text2 = 0, const char* text3 = 0 );
    inline void    Put             ( const char* desc, double       value, int precision = -1, const char* text = 0 );
    inline void    Put             ( const char* desc, bool         b );
    inline void    Put             ( const char* desc, UINT32       value );
    inline void    Put             ( const char* desc, TPointFloat  p );
    inline void    Put             ( const char* desc, TPointDouble p );

    inline void    ResetTable      ();
    inline void    BeginTable      ( int colsize );
    inline void    EndTable        ();
    inline void    PutTable        ( const char* text                 );
    inline void    PutTable        ( int         value                );
    inline void    PutTable        ( double      value, int precision );


                    operator std::ofstream& ()      { return  *ofv; }
                    operator std::ofstream* ()      { return   ofv; }
    std::ofstream*  GetStream               ()      { return   ofv; }

                                        // Assignation & Copy not defined, so don't use verbose = TVerboseFile ( file, len );
                                        // use Open instead

private:
    std::ofstream*  ofv;
    int             DescriptionLength;

    char            buff[ 1024 ];

                                        // Reserved for table output
    int             TableColSize0;      // special width for first column, based of the length of rows' names
    int             TableColSize;
    int             TableCurrCol;
    int             TableCurrRow;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TVerboseFile::TVerboseFile ()
{
ofv                 = 0;
DescriptionLength   = 40;
}


        TVerboseFile::TVerboseFile ( const char* file, int desclength )
{
ofv                 = 0;

Open ( file, desclength );
}


        TVerboseFile::~TVerboseFile ()
{
Close ();
}


void    TVerboseFile::Open ( const char* file, int desclength )
{
Close ();


DescriptionLength   = desclength;


if ( StringIsEmpty ( file ) )
    return;


ofv                 = new std::ofstream ( TFileName ( file, TFilenameExtendedPath ) );
//(*ofv).open ( file );

*ofv << StreamFormatFixed;
*ofv << StreamFormatLeft;
}


void    TVerboseFile::Close ()
{
if ( ! IsOpen () )
    return;


ofv->close ();
delete  ofv;
ofv                 = 0;
}


void    TVerboseFile::Flush ()
{
if ( IsOpen () )
    ofv->flush ();
}


//----------------------------------------------------------------------------
void    TVerboseFile::PutTitle ( const char* title )    const
{
                                        // text title
(*ofv) << "\t\t\t" << title << fastendl;
(*ofv) << "\t\t\t";

for ( int i = 0; i < StringLength ( title ); i++ )
    (*ofv) << "=";

(*ofv) << fastendl << fastendl;

                                        // current program info
//(*ofv) << "\t\t\t" <<  CartoolObjects.CartoolApplication->ApplicationFileName /*ProdName* / << " " << CartoolObjects.CartoolApplication->ProdVersion << " (" << CartoolObjects.CartoolApplication->ProdRevision << ")" << fastendl;
(*ofv) << "\t\t\t" <<  CartoolObjects.CartoolApplication->DefaultTitle << fastendl;

                                        // date & time
char            timestamp[ 64 ];

GetTimeStamp ( true, true, timestamp );

(*ofv) << "\t\t\t" << timestamp << fastendl;
(*ofv) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::NextLine ( int numlines )     const
{
do (*ofv) << fastendl; while ( --numlines > 0 );

//ofv->flush ();
}


void    TVerboseFile::NextTopic ( const char* topic )   const
{
(*ofv) << fastendl << fastendl;
(*ofv) << Tab << topic << fastendl;
(*ofv) << Tab;

for ( int i = 0; i < StringLength ( topic ); i++ )
    (*ofv) << "-";

(*ofv) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::NextBlock ()  const
{
(*ofv) << fastendl << fastendl;
(*ofv) << "-----------------------------------------------------------------------------" << fastendl;
(*ofv) << "-----------------------------------------------------------------------------" << fastendl;
(*ofv) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc )
{
if ( StringIsNotEmpty ( desc ) )    StringCopy  ( buff, desc, DescriptionLength );
else                                ClearString ( buff );

(*ofv) << setw ( DescriptionLength ) << buff;

//ofv->flush ();
}

                                        // putting a single char
void    TVerboseFile::Put ( const char* desc, const char char1 )
{
Put ( desc );

(*ofv) << char1
       << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, const char* text1, const char* text2, const char* text3 )
{
Put ( desc );

(*ofv) << ( text1 ? text1 : "" )
       << ( text2 ? text2 : "" )
       << ( text3 ? text3 : "" )
       << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, const char*  text1, int value, const char* text2 )
{
Put ( desc );

(*ofv) << text1
       << setw ( 0 ) << value
       << ( text2 ? text2 : "" )
       << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, int value, int width, const char* text1, const char* text2, const char* text3 )
{
Put ( desc );

(*ofv) << setw ( width ) << StreamFormatRight << value << StreamFormatLeft;

if ( StringIsNotEmpty ( text1 ) )   (*ofv) << text1;
if ( StringIsNotEmpty ( text2 ) )   (*ofv) << text2;
if ( StringIsNotEmpty ( text3 ) )   (*ofv) << text3;

(*ofv) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, double value, int precision, const char* text )
{
Put ( desc );

if ( precision >= 0 )
    (*ofv) << setprecision ( precision ) << value;
else
    (*ofv)                               << value;

if ( StringIsNotEmpty ( text ) )
    (*ofv) << text;

(*ofv) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, bool b )
{
Put ( desc );

(*ofv) << BoolToString ( b ) << fastendl;

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, UINT32 value )
{
Put ( desc, CheckToBool ( value ) );

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, TPointFloat p )
{
char                buff[ 256 ];

sprintf ( buff, "%.2f, %.2f, %.2f", p.X, p.Y, p.Z );

Put ( desc, buff );

//ofv->flush ();
}


void    TVerboseFile::Put ( const char* desc, TPointDouble p )
{
Put ( desc, TPointFloat ( p.X, p.Y, p.Z ) );
}


//----------------------------------------------------------------------------
void    TVerboseFile::ResetTable ()
{
TableColNames.Reset ();
TableRowNames.Reset ();
TableColSize0       = 10;
TableColSize        = 10;
TableCurrCol        = 0;
TableCurrRow        = 0;
}

                                        // additional space in the row columns
static constexpr int    ColSize0Margin      = 4;


void    TVerboseFile::BeginTable ( int colsize )
{
                                        // row title should be big enough for the title or the content, or it could be left to 0 to be ignored
TableColSize0       = max ( StringLength ( TableColNames[ 0 ] ),    TableRowNames.GetMaxStringLength () );       
                                        // columns width should be big enough for all columns
TableColSize        = max ( (long) colsize,                         TableColNames.GetMaxStringLength () + 1 );   
                                        // indexes of current row and columns
TableCurrCol        = 0;
TableCurrRow        = 0;


                                        // Plot all column names, including row title
for ( int coli = 0; coli < (int) TableColNames; coli++ ) {

                                        // skip row title if all strings are empty
    if ( coli == 0 && TableColSize0 == 0 )
        continue;

                                        // only the row title is aligned left
    (*ofv) <<      ( coli == 0 ? StreamFormatLeft               : StreamFormatRight )
           << setw ( coli == 0 ? TableColSize0 + ColSize0Margin : TableColSize      ) 
           << TableColNames[ coli ];
    }

(*ofv) << fastendl;


                                        // draw a horizontal line below the title
if ( TableColSize0 != 0 ) {

    for ( int i = 0; i < TableColSize0 + ColSize0Margin - 1; i++ )
        (*ofv) << "_";

    (*ofv) << "|";
    }

for ( int i = 0; i < ( (int) TableColNames - 1 ) * TableColSize; i++ )
    (*ofv) << "_";

(*ofv) << fastendl;
}

                                        // Back to general purpose formatting
void    TVerboseFile::EndTable ()
{
(*ofv) << StreamFormatGeneral << StreamFormatLeft;
}


//----------------------------------------------------------------------------

void    TVerboseFile::PutTable ( const char* text )
{
                                        // don't output anything beyong the last row
//if ( TableCurrRow >= (int) TableRowNames )
//    return;

                                        // output header of row?
                                        // if row names are unallocated, or we passed the last string, then just put a blank
if ( TableCurrCol == 0 && TableColSize0 != 0 ) {
    (*ofv) << StreamFormatLeft
           << setw ( TableColSize0 + ColSize0Margin ) 
           << ( TableCurrRow < (int) TableRowNames ? TableRowNames[ TableCurrRow ] : "" );
    }


(*ofv) << StreamFormatFixed
       << StreamFormatRight 
       << setw ( TableColSize );


(*ofv) << text;

                                        // next column
TableCurrCol    = ++TableCurrCol % ( (int) TableColNames - 1 );

                                        // next row?
if ( TableCurrCol == 0 ) {
    TableCurrRow++;
    (*ofv) << fastendl;
    }
}



void    TVerboseFile::PutTable ( int value )
{
char                text[ 32 ];

IntegerToString ( text, value );

PutTable ( text );
}


void    TVerboseFile::PutTable ( double value, int precision )
{
char                text[ 64 ];
                                        // force converting it ourselves with 0 precision  123.4 -> 124 and 234.5 -> 235
if ( Log10 ( abs ( value ) ) < TableColSize - ( precision + 2 ) )

    sprintf ( text, "%.*lf", precision, value );

else
                                        // general purpose formatting
    sprintf ( text, "%.lg", value );

PutTable ( text );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
