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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "Files.Stream.h"
#include    "Dialogs.Input.h"
#include    "Geometry.TPoint.h"
#include    "Geometry.TPoints.h"
#include    "Math.TMatrix44.h"

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace crtl {

//----------------------------------------------------------------------------
// Matrix order in memory is Column-major, FORTRAN / Matlab style:
//      a[  0 ]  a[  4 ]  a[  8 ]  a[ 12 ]
//      a[  1 ]  a[  5 ]  a[  9 ]  a[ 13 ]
//      a[  2 ]  a[  6 ]  a[ 10 ]  a[ 14 ]
//      a[  3 ]  a[  7 ]  a[ 11 ]  a[ 15 ]
//
// With conventional meaning:
//      X1       Y1       Z1       TrX
//      X2       Y2       Z2       TrY
//      X3       Y3       Z3       TrZ
//      0        0        0        W
// 
// X? Y? Z? embeds rotations, symetries, shearings & scalings
// Tr? is the translation
// W should be 1, if not, it is a dividing factor that should be applied to results
//----------------------------------------------------------------------------


        TMatrix44::TMatrix44 ()
{
SetIdentity ();
}


        TMatrix44::TMatrix44    (   double  m11,    double  m12,    double  m13,    double  m14,
                                    double  m21,    double  m22,    double  m23,    double  m24,
                                    double  m31,    double  m32,    double  m33,    double  m34,
                                    double  m41,    double  m42,    double  m43,    double  m44
                                )
{
Set (   m11,    m12,    m13,    m14,
        m21,    m22,    m23,    m24,
        m31,    m32,    m33,    m34,
        m41,    m42,    m43,    m44
    );
}


void    TMatrix44::Reset ()
{
ClearVirtualMemory ( Matrix, HomogeneousMatrixSize * sizeof ( *Matrix ) );
                                        // force this - !matrix is therefor not full of 0's!
Matrix[ 15 ] = 1;
}


void    TMatrix44::Set    (  double v )
{
for ( int i = 0; i < HomogeneousMatrixSize; i++ )
    Matrix[ i ] = v;
}


void    TMatrix44::Set  (   double  m11,    double  m12,    double  m13,    double  m14,
                            double  m21,    double  m22,    double  m23,    double  m24,
                            double  m31,    double  m32,    double  m33,    double  m34,
                            double  m41,    double  m42,    double  m43,    double  m44
                        )
{
Matrix[  0 ] = m11;     Matrix[  4 ] = m12;     Matrix[  8 ] = m13;     Matrix[ 12 ] = m14;
Matrix[  1 ] = m21;     Matrix[  5 ] = m22;     Matrix[  9 ] = m23;     Matrix[ 13 ] = m24;
Matrix[  2 ] = m31;     Matrix[  6 ] = m32;     Matrix[ 10 ] = m33;     Matrix[ 14 ] = m34;
Matrix[  3 ] = m41;     Matrix[  7 ] = m42;     Matrix[ 11 ] = m43;     Matrix[ 15 ] = m44;
}


        TMatrix44::TMatrix44 ( const TMatrix44& op )
{
CopyVirtualMemory ( Matrix, (void *) op.Matrix, HomogeneousMatrixSize * sizeof ( *Matrix ) );
}


TMatrix44&  TMatrix44::operator= ( const TMatrix44& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


CopyVirtualMemory ( Matrix, (void *) op2.Matrix, HomogeneousMatrixSize * sizeof ( *Matrix ) );

return  *this;
}


double* TMatrix44::CopyTo   ( double* m )
{
return  (double *) CopyVirtualMemory ( m, Matrix, HomogeneousMatrixSize * sizeof ( *Matrix ) );
}


double* TMatrix44::CopyFrom ( const double* m )
{
return  (double *) CopyVirtualMemory ( Matrix, m, HomogeneousMatrixSize * sizeof ( *Matrix ) );
}


bool    TMatrix44::operator== ( const TMatrix44& op2 )  const
{
for ( int i = 0; i < HomogeneousMatrixSize; i++ )
    if ( Matrix[ i ] != op2[ i ] )
        return  false;

return  true;
}


bool    TMatrix44::operator!= ( const TMatrix44& op2 )  const
{
for ( int i = 0; i < HomogeneousMatrixSize; i++ )
    if ( Matrix[ i ] != op2[ i ] )
        return  true;

return  false;
}


//----------------------------------------------------------------------------
void    TMatrix44::SetIdentity ()
{
Reset ();

Matrix[  0 ] = 
                Matrix[  5 ] = 
                                Matrix[ 10 ] = 
                                                Matrix[ 15 ] = 1;
}

//----------------------------------------------------------------------------
//      Order and meaning of these angles:
// 1) precession / azimut            around Z axis, in the xy plane
// 2) nutation / elevation           from the Z axis, around new Y axis
// 3) spin / tilt / rotation propre  around new Z

void    TMatrix44::SetEulerAngles ( double p/*recession*/, double n/*utation*/, double s/*pin*/ )
{
p       = DegreesToRadians ( p );
n       = DegreesToRadians ( n );
s       = DegreesToRadians ( s );

double              cp              = cos ( p );
double              sp              = sin ( p );
double              cn              = cos ( n );
double              sn              = sin ( n );
double              cs              = cos ( s );
double              ss              = sin ( s );


Matrix[ 0]  =   cs * cn * cp - ss * sp;
Matrix[ 1]  = - ss * cn * cp - cs * sp;
Matrix[ 2]  =        sn * cp;
Matrix[ 3]  =   0;
        
Matrix[ 4]  =   cs * cn * sp + ss * cp;
Matrix[ 5]  = - ss * cn * sp + cs * cp;
Matrix[ 6]  =        sn * sp;
Matrix[ 7]  =   0;
        
Matrix[ 8]  = - cs * sn;
Matrix[ 9]  =   ss * sn;
Matrix[10]  =        cn;
Matrix[11]  =   0;

Matrix[12]  =   0;
Matrix[13]  =   0;
Matrix[14]  =   0;
Matrix[15]  =   1;
}


void    TMatrix44::SetColumn ( int c, const TPointFloat& p )
{
if ( ! IsInsideLimits ( c, 0, 3 ) )
    return;

Matrix[ 4 * c + 0 ]     = p.X;
Matrix[ 4 * c + 1 ]     = p.Y;
Matrix[ 4 * c + 2 ]     = p.Z;
}


//----------------------------------------------------------------------------
                                        // Full identity matrix
bool    TMatrix44::IsIdentity ( double epsilon )  const
{
TMatrix44           m ( *this );

m.Rounding ( epsilon );


return  m[  0 ] == 1 && m[  4 ] == 0 && m[  8 ] == 0 && m[ 12 ] == 0
     && m[  1 ] == 0 && m[  5 ] == 1 && m[  9 ] == 0 && m[ 13 ] == 0
     && m[  2 ] == 0 && m[  6 ] == 0 && m[ 10 ] == 1 && m[ 14 ] == 0
     && m[  3 ] == 0 && m[  7 ] == 0 && m[ 11 ] == 0 && m[ 15 ] == 1;
}

                                        // Identity rotation, scaling, and shearing, and non-null translation
bool    TMatrix44::IsTranslation ( double epsilon )  const
{
TMatrix44           m ( *this );

m.Rounding ( epsilon );


return  m[  0 ] == 1 && m[  4 ] == 0 && m[  8 ] == 0
     && m[  1 ] == 0 && m[  5 ] == 1 && m[  9 ] == 0
     && m[  2 ] == 0 && m[  6 ] == 0 && m[ 10 ] == 1
     && m[  3 ] == 0 && m[  7 ] == 0 && m[ 11 ] == 0 /*&& m[ 15 ] == 1*/

                                                     && ! (   m[ 12 ] == 0
                                                           && m[ 13 ] == 0
                                                           && m[ 14 ] == 0 );
}

                                        // testing for exact non-null values - skip W, though
bool    TMatrix44::IsNotNull () const
{
return  Matrix[  0 ] || Matrix[  4 ] || Matrix[  8 ] || Matrix[ 12 ]
     || Matrix[  1 ] || Matrix[  5 ] || Matrix[  9 ] || Matrix[ 13 ]
     || Matrix[  2 ] || Matrix[  6 ] || Matrix[ 10 ] || Matrix[ 14 ]
     || Matrix[  3 ] || Matrix[  7 ] || Matrix[ 11 ] /*|| Matrix[ 15 ]*/;
}


//----------------------------------------------------------------------------
void    ShowMatrixAnalysis  ( MatrixAnalysis ma, const char* title )
{
char            text[ 256 ];

if ( IsMatrixIdentity ( ma ) )

    StringCopy      ( text, "Identity" );

else {

    ClearString     ( text );

    if ( ma & HasRotation   )   StringAppend    ( text, ( ma & HasOrtho             )   ? "Rotation Ortho"      : "Rotation",       "\n" );
    if ( ma & HasSymmetry   )   StringAppend    ( text, ( ma & HasOrtho             )   ? "Symmetry Ortho"      : "Symmetry",       "\n" );
    if ( ma & HasScaling    )   StringAppend    ( text, ( ma & HasIntegerScaling    )   ? "Integer Scaling"     : "Scaling",        "\n" );
    if ( ma & HasTranslation)   StringAppend    ( text, ( ma & HasIntegerTranslation)   ? "Integer Translation" : "Translation",    "\n" );
    if ( ma & HasShearing   )   StringAppend    ( text,                                                           "Shearing",       "\n" );
    }

ShowMessage ( text, StringIsEmpty ( title ) ? "Matrix Analysis" : title );
}


//----------------------------------------------------------------------------
                                        // Analyze the type of transform
                                        // We could also return a bunch of TVector3Double with the translations, scaling etc... extracted
MatrixAnalysis  TMatrix44::Analyze ( double epsilon )  const
{
TMatrix44           m ( *this );

m.Rounding ( epsilon );

                                        // Basically 0, coding only the existing sub-transforms, not the non-existing ones
int                 ma              = HasNoTransform;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Translation is easy to spot
if (    m ( 0, 3 ) != 0
     || m ( 1, 3 ) != 0
     || m ( 2, 3 ) != 0 ) {

    ma |= HasTranslation;

    if (    IsInteger ( m ( 0, 3 ) )
         && IsInteger ( m ( 1, 3 ) )
         && IsInteger ( m ( 2, 3 ) ) )
                                        // complementing HasTranslation
        ma |= HasIntegerTranslation;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Shearing is also easy to spot
if (    m ( 3, 0 ) != 0
     || m ( 3, 1 ) != 0
     || m ( 3, 2 ) != 0 )

    ma |= HasShearing;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Isolating the transform of vectors (1,0,0), (0,1,0) and (0,0,1), ignoring any translation and shearing
                                        // It contains rotation / symmetry / scaling merged altogether, so there is some processing to be done here
TVector3Double      mx ( m ( 0, 0 ), m ( 1, 0 ), m ( 2, 0 ) );
TVector3Double      my ( m ( 0, 1 ), m ( 1, 1 ), m ( 2, 1 ) );
TVector3Double      mz ( m ( 0, 2 ), m ( 1, 2 ), m ( 2, 2 ) );

                                        // ?do we need this, rounding was already done component-wise?
//mx.RoundTo ( epsilon );
//my.RoundTo ( epsilon );
//mz.RoundTo ( epsilon );

                                        // this shouldn't happen, but better be safe
if ( mx.IsNull () || my.IsNull () || mz.IsNull () )
                                        // setting an error code, maybe?
    return  (MatrixAnalysis) ma;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Working on the scaling first
double              normx           = mx.Norm ();
double              normy           = my.Norm ();
double              normz           = mz.Norm ();

if (   normx != 1 
    || normy != 1 
    || normz != 1 ) {

    ma |= HasScaling;

    if (    IsInteger ( normx )
         && IsInteger ( normy )
         && IsInteger ( normz ) )
                                        // complementing HasScaling
        ma |= HasIntegerScaling;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Getting rid of scaling, so we can work on rotation / symmetry alone
mx /= normx;
my /= normy;
mz /= normz;

                                        // If all axis end up well aligned, then we have an orthogonal transform
bool                isortho         =    mx.IsAxisAligned ( epsilon )
                                      && my.IsAxisAligned ( epsilon )
                                      && mz.IsAxisAligned ( epsilon );

                                        // Do we have a symmetry somewhere, leaving the transform left-handed?
if      ( mx.VectorialProduct ( my ).IsOppositeDirection ( mz ) ) {
                                 // complementing HasSymmetry
    ma |= HasSymmetry | ( isortho ? HasOrtho : 0 );
    }

                                        // No symmetry, any rotation?
else if (    mx != TVector3Double ( 1, 0, 0 )
          || my != TVector3Double ( 0, 1, 0 )
          || mz != TVector3Double ( 0, 0, 1 ) ) {
                                 // complementing HasRotation
    ma |= HasRotation | ( isortho ? HasOrtho : 0 );
    }    

                                        // No rotation, no symmetry
//else nothing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  (MatrixAnalysis) ma;
}


//----------------------------------------------------------------------------
                                        // Transform 1 voxel and test results
                                        // Returns the voxel size if sufficientyl isotropic, 0 otherwise
TPointDouble    TMatrix44::IsIsotropic ( const TPointDouble& inputvoxel, double epsilon ) const
{
TPointDouble        outputvoxel[ 3 ];
TPointDouble        returnvoxel;


if ( ! ApplyVoxel ( inputvoxel, outputvoxel ) )
                                        // (0,0,0)
    return  returnvoxel;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

returnvoxel.Set ( 1 / outputvoxel[ 0 ].Norm (), 
                  1 / outputvoxel[ 1 ].Norm (), 
                  1 / outputvoxel[ 2 ].Norm () );

                                        // is transformed voxel nearly isotropic?
if ( returnvoxel.IsIsotropic ( epsilon ) )
                                        // take the min of all 3 axis
    returnvoxel     = returnvoxel.Mean ();
else
                                        // NOT considered isotropic, reset results
    returnvoxel.Reset ();


return  returnvoxel;
}


//----------------------------------------------------------------------------
                                        // Transform 1 voxel and test results
                                        // Returns the voxel size if sufficientyl orthogonal, (0,0,0) otherwise
TPointDouble    TMatrix44::IsOrthogonal ( const TPointDouble& inputvoxel, double epsilon )    const
{
TPointDouble        outputvoxel[ 3 ];
TPointDouble        returnvoxel;


if ( ! ApplyVoxel ( inputvoxel, outputvoxel ) )
                                        // (0,0,0)
    return  returnvoxel;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // are each transformed axis close to another axis?
if ( outputvoxel[ 0 ].IsAxisAligned ( epsilon )
  && outputvoxel[ 1 ].IsAxisAligned ( epsilon )
  && outputvoxel[ 2 ].IsAxisAligned ( epsilon ) ) {
                                        // compute output voxel size
    returnvoxel.X   = 1 / max ( outputvoxel[ 0 ].X, outputvoxel[ 1 ].X, outputvoxel[ 2 ].X );
    returnvoxel.Y   = 1 / max ( outputvoxel[ 0 ].Y, outputvoxel[ 1 ].Y, outputvoxel[ 2 ].Y );
    returnvoxel.Z   = 1 / max ( outputvoxel[ 0 ].Z, outputvoxel[ 1 ].Z, outputvoxel[ 2 ].Z );
    }


return  returnvoxel;
}


bool    TMatrix44::IsOrthogonal ( double epsilon )    const
{
MatrixAnalysis      ma          = Analyze ( epsilon );

return  crtl::IsOrthogonal ( ma );
}


//----------------------------------------------------------------------------
                                        // Transform 1 voxel and test results
                                        // Returns the voxel size if sufficientyl orthogonal, (0,0,0) otherwise
TPointDouble    TMatrix44::GetMeanVoxel ( const TPointDouble& inputvoxel )    const
{
TPointDouble        outputvoxel[ 3 ];
TPointDouble        returnvoxel;


if ( ! ApplyVoxel ( inputvoxel, outputvoxel ) )
                                        // (0,0,0)
    return  returnvoxel;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute output mean voxel size
returnvoxel.X   = 3 / ( outputvoxel[ 0 ].X + outputvoxel[ 1 ].X + outputvoxel[ 2 ].X );
returnvoxel.Y   = 3 / ( outputvoxel[ 0 ].Y + outputvoxel[ 1 ].Y + outputvoxel[ 2 ].Y );
returnvoxel.Z   = 3 / ( outputvoxel[ 0 ].Z + outputvoxel[ 1 ].Z + outputvoxel[ 2 ].Z );


return  returnvoxel;
}


//----------------------------------------------------------------------------
                                        // Transform a voxel into 3 vectors, one for each dimension
bool    TMatrix44::ApplyVoxel ( const TPointDouble& inputvoxel, TPointDouble (&outputvoxel)[ 3 ] )    const
{
outputvoxel[ 0 ].Reset ();
outputvoxel[ 1 ].Reset ();
outputvoxel[ 2 ].Reset ();

                                        // any non-sensical voxel size?
if ( inputvoxel.X <= 0 
  || inputvoxel.Y <= 0 
  || inputvoxel.Z <= 0 )
    
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transform 3 voxel vectors + getting rid of any translations
TPointDouble        p0 ( 0,                 0,                  0                );
TPointDouble        p1 ( 1 / inputvoxel.X,  0,                  0                );
TPointDouble        p2 ( 0,                 1 / inputvoxel.Y,   0                );
TPointDouble        p3 ( 0,                 0,                  1 / inputvoxel.Z );

                                        // transform the 4 points
Apply ( p0 );
Apply ( p1 );
Apply ( p2 );
Apply ( p3 );

                                        // getting rid of origin shift
p1     -= p0;
p2     -= p0;
p3     -= p0;

                                        // getting rid of signed values / directions
p1.Absolute ();
p2.Absolute ();
p3.Absolute ();

                                        // test if any dimension collapsed - it shouldn't, still better be safe
//if ( p1.Norm () < 1e-6
  //|| p2.Norm () < 1e-6
//  || p3.Norm () < 1e-6 )
if ( p1.Norm () == 0
  || p2.Norm () == 0
  || p3.Norm () == 0 )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transferring results
outputvoxel[ 0 ]    = p1;
outputvoxel[ 1 ]    = p2;
outputvoxel[ 2 ]    = p3;

return  true;
}


//----------------------------------------------------------------------------
                                        // clip all values to a given precision
void    TMatrix44::Rounding ( double precision )
{
Normalize ();

if ( precision == 0 )
    return;

for ( int i = 0; i < HomogeneousMatrixSize; i++ )

    Matrix[ i ] = RoundTo ( Matrix[ i ], precision );
}


//----------------------------------------------------------------------------
void    TMatrix44::ReadFile ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return;


ifstream            is ( TFileName ( file, TFilenameExtendedPath ) );
char                buff[ 1024 ];

if ( is.fail () )
    return;


Matrix[  0 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  4 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  8 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 12 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  1 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  5 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  9 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 13 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  2 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  6 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 10 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 14 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  3 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[  7 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 11 ] = StringToDouble ( GetToken ( &is, buff ) );
Matrix[ 15 ] = StringToDouble ( GetToken ( &is, buff ) );


if ( Matrix[ 15 ] == 0 )
    Matrix[ 15 ] = 1;
}


//----------------------------------------------------------------------------
void    TMatrix44::WriteFile ( const char* file )   const   
{
if ( StringIsEmpty ( file ) )
    return;


ofstream            os ( TFileName ( file, TFilenameExtendedPath ) );

if ( os.fail () )
    return;


os  << StreamFormatFixed;

os  << StreamFormatFloat32 << Matrix[  0 ] << Tab   << StreamFormatFloat32 << Matrix[  4 ] << Tab   << StreamFormatFloat32 << Matrix[  8 ] << Tab   << StreamFormatFloat32 << Matrix[ 12 ] << NewLine
    << StreamFormatFloat32 << Matrix[  1 ] << Tab   << StreamFormatFloat32 << Matrix[  5 ] << Tab   << StreamFormatFloat32 << Matrix[  9 ] << Tab   << StreamFormatFloat32 << Matrix[ 13 ] << NewLine
    << StreamFormatFloat32 << Matrix[  2 ] << Tab   << StreamFormatFloat32 << Matrix[  6 ] << Tab   << StreamFormatFloat32 << Matrix[ 10 ] << Tab   << StreamFormatFloat32 << Matrix[ 14 ] << NewLine
    << StreamFormatFloat32 << Matrix[  3 ] << Tab   << StreamFormatFloat32 << Matrix[  7 ] << Tab   << StreamFormatFloat32 << Matrix[ 11 ] << Tab   << StreamFormatFloat32 << Matrix[ 15 ] << NewLine;
}


//----------------------------------------------------------------------------
void    TMatrix44::Show ( const char *title ) const
{
char                buff[ 1024 ];

sprintf ( buff, "%10.7f \t%10.7f \t%10.7f \t%10.7f\n%10.7f \t%10.7f \t%10.7f \t%10.7f\n%10.7f \t%10.7f \t%10.7f \t%10.7f\n%10.7f \t%10.7f \t%10.7f \t%10.7f",
          Matrix[  0 ], Matrix[  4 ], Matrix[  8 ], Matrix[ 12 ],
          Matrix[  1 ], Matrix[  5 ], Matrix[  9 ], Matrix[ 13 ],
          Matrix[  2 ], Matrix[  6 ], Matrix[ 10 ], Matrix[ 14 ],
          Matrix[  3 ], Matrix[  7 ], Matrix[ 11 ], Matrix[ 15 ] );

ShowMessage ( buff, StringIsEmpty ( title ) ? "Matrix" : title );
}


//----------------------------------------------------------------------------
                                        // the hard way!
TMatrix44&  TMatrix44::Invert ()
{
double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


Matrix[ 0] = Mat[ 9]*Mat[14]*Mat[ 7] - Mat[13]*Mat[10]*Mat[ 7] + Mat[13]*Mat[ 6]*Mat[11] - Mat[ 5]*Mat[14]*Mat[11] - Mat[ 9]*Mat[ 6]*Mat[15] + Mat[ 5]*Mat[10]*Mat[15];
Matrix[ 1] = Mat[13]*Mat[10]*Mat[ 3] - Mat[ 9]*Mat[14]*Mat[ 3] - Mat[13]*Mat[ 2]*Mat[11] + Mat[ 1]*Mat[14]*Mat[11] + Mat[ 9]*Mat[ 2]*Mat[15] - Mat[ 1]*Mat[10]*Mat[15];
Matrix[ 2] = Mat[ 5]*Mat[14]*Mat[ 3] - Mat[13]*Mat[ 6]*Mat[ 3] + Mat[13]*Mat[ 2]*Mat[ 7] - Mat[ 1]*Mat[14]*Mat[ 7] - Mat[ 5]*Mat[ 2]*Mat[15] + Mat[ 1]*Mat[ 6]*Mat[15];
Matrix[ 3] = Mat[ 9]*Mat[ 6]*Mat[ 3] - Mat[ 5]*Mat[10]*Mat[ 3] - Mat[ 9]*Mat[ 2]*Mat[ 7] + Mat[ 1]*Mat[10]*Mat[ 7] + Mat[ 5]*Mat[ 2]*Mat[11] - Mat[ 1]*Mat[ 6]*Mat[11];
Matrix[ 4] = Mat[12]*Mat[10]*Mat[ 7] - Mat[ 8]*Mat[14]*Mat[ 7] - Mat[12]*Mat[ 6]*Mat[11] + Mat[ 4]*Mat[14]*Mat[11] + Mat[ 8]*Mat[ 6]*Mat[15] - Mat[ 4]*Mat[10]*Mat[15];
Matrix[ 5] = Mat[ 8]*Mat[14]*Mat[ 3] - Mat[12]*Mat[10]*Mat[ 3] + Mat[12]*Mat[ 2]*Mat[11] - Mat[ 0]*Mat[14]*Mat[11] - Mat[ 8]*Mat[ 2]*Mat[15] + Mat[ 0]*Mat[10]*Mat[15];
Matrix[ 6] = Mat[12]*Mat[ 6]*Mat[ 3] - Mat[ 4]*Mat[14]*Mat[ 3] - Mat[12]*Mat[ 2]*Mat[ 7] + Mat[ 0]*Mat[14]*Mat[ 7] + Mat[ 4]*Mat[ 2]*Mat[15] - Mat[ 0]*Mat[ 6]*Mat[15];
Matrix[ 7] = Mat[ 4]*Mat[10]*Mat[ 3] - Mat[ 8]*Mat[ 6]*Mat[ 3] + Mat[ 8]*Mat[ 2]*Mat[ 7] - Mat[ 0]*Mat[10]*Mat[ 7] - Mat[ 4]*Mat[ 2]*Mat[11] + Mat[ 0]*Mat[ 6]*Mat[11];
Matrix[ 8] = Mat[ 8]*Mat[13]*Mat[ 7] - Mat[12]*Mat[ 9]*Mat[ 7] + Mat[12]*Mat[ 5]*Mat[11] - Mat[ 4]*Mat[13]*Mat[11] - Mat[ 8]*Mat[ 5]*Mat[15] + Mat[ 4]*Mat[ 9]*Mat[15];
Matrix[ 9] = Mat[12]*Mat[ 9]*Mat[ 3] - Mat[ 8]*Mat[13]*Mat[ 3] - Mat[12]*Mat[ 1]*Mat[11] + Mat[ 0]*Mat[13]*Mat[11] + Mat[ 8]*Mat[ 1]*Mat[15] - Mat[ 0]*Mat[ 9]*Mat[15];
Matrix[10] = Mat[ 4]*Mat[13]*Mat[ 3] - Mat[12]*Mat[ 5]*Mat[ 3] + Mat[12]*Mat[ 1]*Mat[ 7] - Mat[ 0]*Mat[13]*Mat[ 7] - Mat[ 4]*Mat[ 1]*Mat[15] + Mat[ 0]*Mat[ 5]*Mat[15];
Matrix[11] = Mat[ 8]*Mat[ 5]*Mat[ 3] - Mat[ 4]*Mat[ 9]*Mat[ 3] - Mat[ 8]*Mat[ 1]*Mat[ 7] + Mat[ 0]*Mat[ 9]*Mat[ 7] + Mat[ 4]*Mat[ 1]*Mat[11] - Mat[ 0]*Mat[ 5]*Mat[11];
Matrix[12] = Mat[12]*Mat[ 9]*Mat[ 6] - Mat[ 8]*Mat[13]*Mat[ 6] - Mat[12]*Mat[ 5]*Mat[10] + Mat[ 4]*Mat[13]*Mat[10] + Mat[ 8]*Mat[ 5]*Mat[14] - Mat[ 4]*Mat[ 9]*Mat[14];
Matrix[13] = Mat[ 8]*Mat[13]*Mat[ 2] - Mat[12]*Mat[ 9]*Mat[ 2] + Mat[12]*Mat[ 1]*Mat[10] - Mat[ 0]*Mat[13]*Mat[10] - Mat[ 8]*Mat[ 1]*Mat[14] + Mat[ 0]*Mat[ 9]*Mat[14];
Matrix[14] = Mat[12]*Mat[ 5]*Mat[ 2] - Mat[ 4]*Mat[13]*Mat[ 2] - Mat[12]*Mat[ 1]*Mat[ 6] + Mat[ 0]*Mat[13]*Mat[ 6] + Mat[ 4]*Mat[ 1]*Mat[14] - Mat[ 0]*Mat[ 5]*Mat[14];
Matrix[15] = Mat[ 4]*Mat[ 9]*Mat[ 2] - Mat[ 8]*Mat[ 5]*Mat[ 2] + Mat[ 8]*Mat[ 1]*Mat[ 6] - Mat[ 0]*Mat[ 9]*Mat[ 6] - Mat[ 4]*Mat[ 1]*Mat[10] + Mat[ 0]*Mat[ 5]*Mat[10];


double              scale           = Determinant ();

if ( scale )    scale   = 1 / scale;
else            scale   = 1;

for ( int i = 0; i < HomogeneousMatrixSize; i++ )
    Matrix[ i ]    *= scale;


Normalize ();


return *this;
}


//----------------------------------------------------------------------------
void    TMatrix44::Normalize ()
{
if ( Matrix[ 15 ] != 0 )

    for ( int i = 0; i < HomogeneousMatrixSize - 1; i++ )

        Matrix[ i ]    /= Matrix[ 15 ];

                                        // Finally, force set this
Matrix[ 15 ]    = 1;
}


//----------------------------------------------------------------------------
double  TMatrix44::Determinant () const
{
return
Matrix[12] * Matrix[ 9] * Matrix[ 6] * Matrix[ 3] - Matrix[ 8] * Matrix[13] * Matrix[ 6] * Matrix[ 3] - Matrix[12] * Matrix[ 5] * Matrix[10] * Matrix[ 3] + Matrix[ 4] * Matrix[13] * Matrix[10] * Matrix[ 3] +
Matrix[ 8] * Matrix[ 5] * Matrix[14] * Matrix[ 3] - Matrix[ 4] * Matrix[ 9] * Matrix[14] * Matrix[ 3] - Matrix[12] * Matrix[ 9] * Matrix[ 2] * Matrix[ 7] + Matrix[ 8] * Matrix[13] * Matrix[ 2] * Matrix[ 7] +
Matrix[12] * Matrix[ 1] * Matrix[10] * Matrix[ 7] - Matrix[ 0] * Matrix[13] * Matrix[10] * Matrix[ 7] - Matrix[ 8] * Matrix[ 1] * Matrix[14] * Matrix[ 7] + Matrix[ 0] * Matrix[ 9] * Matrix[14] * Matrix[ 7] +
Matrix[12] * Matrix[ 5] * Matrix[ 2] * Matrix[11] - Matrix[ 4] * Matrix[13] * Matrix[ 2] * Matrix[11] - Matrix[12] * Matrix[ 1] * Matrix[ 6] * Matrix[11] + Matrix[ 0] * Matrix[13] * Matrix[ 6] * Matrix[11] +
Matrix[ 4] * Matrix[ 1] * Matrix[14] * Matrix[11] - Matrix[ 0] * Matrix[ 5] * Matrix[14] * Matrix[11] - Matrix[ 8] * Matrix[ 5] * Matrix[ 2] * Matrix[15] + Matrix[ 4] * Matrix[ 9] * Matrix[ 2] * Matrix[15] +
Matrix[ 8] * Matrix[ 1] * Matrix[ 6] * Matrix[15] - Matrix[ 0] * Matrix[ 9] * Matrix[ 6] * Matrix[15] - Matrix[ 4] * Matrix[ 1] * Matrix[10] * Matrix[15] + Matrix[ 0] * Matrix[ 5] * Matrix[10] * Matrix[15];
}


//----------------------------------------------------------------------------
void    TMatrix44::SetOrientation ( int orient )
{
Reset ();

switch ( orient ) {

    case    OrientTransverseTop:
        Matrix[0]   =  0;   Matrix[4]   =  0;   Matrix[8]   =  1;
        Matrix[1]   = -1;   Matrix[5]   =  0;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   = -1;   Matrix[10]  =  0;
        break;

    case    OrientTransverseBottom:
        Matrix[0]   =  0;   Matrix[4]   =  0;   Matrix[8]   = -1;
        Matrix[1]   = -1;   Matrix[5]   =  0;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   =  1;   Matrix[10]  =  0;
        break;

    case    OrientSagittalLeft:
        Matrix[0]   =  1;   Matrix[4]   =  0;   Matrix[8]   =  0;
        Matrix[1]   =  0;   Matrix[5]   = -1;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   =  0;   Matrix[10]  = -1;
        break;

    case    OrientSagittalRight:
        Matrix[0]   = -1;   Matrix[4]   =  0;   Matrix[8]   =  0;
        Matrix[1]   =  0;   Matrix[5]   = -1;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   =  0;   Matrix[10]  =  1;
        break;

    case    OrientCoronalFront:
        Matrix[0]   =  0;   Matrix[4]   =  0;   Matrix[8]   = -1;
        Matrix[1]   =  0;   Matrix[5]   = -1;   Matrix[9]   =  0;
        Matrix[2]   = -1;   Matrix[6]   =  0;   Matrix[10]  =  0;
        break;

    case    OrientCoronalBack:
        Matrix[0]   =  0;   Matrix[4]   =  0;   Matrix[8]   =  1;
        Matrix[1]   =  0;   Matrix[5]   = -1;   Matrix[9]   =  0;
        Matrix[2]   =  1;   Matrix[6]   =  0;   Matrix[10]  =  0;
        break;

    case    OrientMinusXY:
        Matrix[0]   =  1;   Matrix[4]   =  0;   Matrix[8]   =  0;
        Matrix[1]   =  0;   Matrix[5]   = -1;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   =  0;   Matrix[10]  = -1;
        break;

    case    OrientXZ:
        Matrix[0]   = -1;   Matrix[4]   =  0;   Matrix[8]   =  0;
        Matrix[1]   =  0;   Matrix[5]   =  0;   Matrix[9]   = -1;
        Matrix[2]   =  0;   Matrix[6]   = -1;   Matrix[10]  =  0;
        break;

    case    OrientMinusYX:
    default:
        Matrix[0]   =  0;   Matrix[4]   = -1;   Matrix[8]   =  0;
        Matrix[1]   =  1;   Matrix[5]   =  0;   Matrix[9]   =  0;
        Matrix[2]   =  0;   Matrix[6]   =  0;   Matrix[10]  =  1;
    }
}


//----------------------------------------------------------------------------
void    TMatrix44::RotateX ( double angle, MatrixMultiplicationSide side )
{
if ( ! angle )  return;

double              c               = cos ( DegreesToRadians ( angle ) );
double              s               = sin ( DegreesToRadians ( angle ) );
double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[1]  = Mat[1]  * c - Mat[2]  * s;
    Matrix[2]  = Mat[1]  * s + Mat[2]  * c;
    Matrix[5]  = Mat[5]  * c - Mat[6]  * s;
    Matrix[6]  = Mat[5]  * s + Mat[6]  * c;
    Matrix[9]  = Mat[9]  * c - Mat[10] * s;
    Matrix[10] = Mat[9]  * s + Mat[10] * c;
    Matrix[13] = Mat[13] * c - Mat[14] * s;
    Matrix[14] = Mat[13] * s + Mat[14] * c;
    }
else { // on right
    Matrix[4]  =  Mat[4]  * c + Mat[8 ] * s;
    Matrix[5]  =  Mat[5]  * c + Mat[9 ] * s;
    Matrix[6]  =  Mat[6]  * c + Mat[10] * s;
    Matrix[7]  =  Mat[7]  * c + Mat[11] * s;

    Matrix[8]  = -Mat[4]  * s + Mat[8]  * c;
    Matrix[9]  = -Mat[5]  * s + Mat[9]  * c;
    Matrix[10] = -Mat[6]  * s + Mat[10] * c;
    Matrix[11] = -Mat[7]  * s + Mat[11] * c;
    }
}


void    TMatrix44::RotateY ( double angle, MatrixMultiplicationSide side )
{
if ( ! angle )  return;

double              c               = cos ( DegreesToRadians ( angle ) );
double              s               = sin ( DegreesToRadians ( angle ) );
double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[0]  =  Mat[0]  * c + Mat[2]  * s;
    Matrix[2]  = -Mat[0]  * s + Mat[2]  * c;
    Matrix[4]  =  Mat[4]  * c + Mat[6]  * s;
    Matrix[6]  = -Mat[4]  * s + Mat[6]  * c;
    Matrix[8]  =  Mat[8]  * c + Mat[10] * s;
    Matrix[10] = -Mat[8]  * s + Mat[10] * c;
    Matrix[12] =  Mat[12] * c + Mat[14] * s;
    Matrix[14] = -Mat[12] * s + Mat[14] * c;
    }
else { // on right
    Matrix[0]  =  Mat[0]  * c - Mat[8]  * s;
    Matrix[1]  =  Mat[1]  * c - Mat[9]  * s;
    Matrix[2]  =  Mat[2]  * c - Mat[10] * s;
    Matrix[3]  =  Mat[3]  * c - Mat[11] * s;

    Matrix[8]  =  Mat[0]  * s + Mat[8]  * c;
    Matrix[9]  =  Mat[1]  * s + Mat[9]  * c;
    Matrix[10] =  Mat[2]  * s + Mat[10] * c;
    Matrix[11] =  Mat[3]  * s + Mat[11] * c;
    }
}


void    TMatrix44::RotateZ ( double angle, MatrixMultiplicationSide side )
{
if ( ! angle )  return;

double              c               = cos ( DegreesToRadians ( angle ) );
double              s               = sin ( DegreesToRadians ( angle ) );
double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[0]  = Mat[0]  * c - Mat[1]  * s;
    Matrix[1]  = Mat[0]  * s + Mat[1]  * c;
    Matrix[4]  = Mat[4]  * c - Mat[5]  * s;
    Matrix[5]  = Mat[4]  * s + Mat[5]  * c;
    Matrix[8]  = Mat[8]  * c - Mat[9]  * s;
    Matrix[9]  = Mat[8]  * s + Mat[9]  * c;
    Matrix[12] = Mat[12] * c - Mat[13] * s;
    Matrix[13] = Mat[12] * s + Mat[13] * c;
    }
else { // on right
    Matrix[0]  =  Mat[0]  * c + Mat[4]  * s;
    Matrix[1]  =  Mat[1]  * c + Mat[5]  * s;
    Matrix[2]  =  Mat[2]  * c + Mat[6]  * s;
    Matrix[3]  =  Mat[3]  * c + Mat[7]  * s;

    Matrix[4]  = -Mat[0]  * s + Mat[4]  * c;
    Matrix[5]  = -Mat[1]  * s + Mat[5]  * c;
    Matrix[6]  = -Mat[2]  * s + Mat[6]  * c;
    Matrix[7]  = -Mat[3]  * s + Mat[7]  * c;
    }
}


void    TMatrix44::Rotate ( double angle, int axis, MatrixMultiplicationSide side )
{
if      ( axis == 0 )   RotateX ( angle, side );
else if ( axis == 1 )   RotateY ( angle, side );
else if ( axis == 2 )   RotateZ ( angle, side );
}


void    TMatrix44::RotateXYZ ( double anglex, double angley, double anglez, MatrixMultiplicationSide side )
{
RotateZ ( anglez, side );
RotateY ( angley, side );
RotateX ( anglex, side );
}


//----------------------------------------------------------------------------
void    TMatrix44::TranslateX ( double x, MatrixMultiplicationSide side )
{
if ( ! x )    return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 0] += Matrix[ 3] * x;
    Matrix[ 4] += Matrix[ 7] * x;
    Matrix[ 8] += Matrix[11] * x;
    Matrix[12] += Matrix[15] * x;
    }
else {
    Matrix[12] += Matrix[ 0] * x;
    Matrix[13] += Matrix[ 1] * x;
    Matrix[14] += Matrix[ 2] * x;
    }
}


void    TMatrix44::TranslateY ( double y, MatrixMultiplicationSide side )
{
if ( ! y )    return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 1] += Matrix[ 3] * y;
    Matrix[ 5] += Matrix[ 7] * y;
    Matrix[ 9] += Matrix[11] * y;
    Matrix[13] += Matrix[15] * y;
    }
else {
    Matrix[12] +=               Matrix[ 4] * y;
    Matrix[13] +=               Matrix[ 5] * y;
    Matrix[14] +=               Matrix[ 6] * y;
    }
}


void    TMatrix44::TranslateZ ( double z, MatrixMultiplicationSide side )
{
if ( ! z )    return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 2] += Matrix[ 3] * z;
    Matrix[ 6] += Matrix[ 7] * z;
    Matrix[10] += Matrix[11] * z;
    Matrix[14] += Matrix[15] * z;
    }
else {
    Matrix[12] +=                             Matrix[ 8] * z;
    Matrix[13] +=                             Matrix[ 9] * z;
    Matrix[14] +=                             Matrix[10] * z;
    }
}


void    TMatrix44::Translate ( double x, double y, double z, MatrixMultiplicationSide side )
{
if ( ! ( x || y || z ) )    return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 0] += Matrix[ 3] * x;
    Matrix[ 1] += Matrix[ 3] * y;
    Matrix[ 2] += Matrix[ 3] * z;

    Matrix[ 4] += Matrix[ 7] * x;
    Matrix[ 5] += Matrix[ 7] * y;
    Matrix[ 6] += Matrix[ 7] * z;

    Matrix[ 8] += Matrix[11] * x;
    Matrix[ 9] += Matrix[11] * y;
    Matrix[10] += Matrix[11] * z;

    Matrix[12] += Matrix[15] * x;
    Matrix[13] += Matrix[15] * y;
    Matrix[14] += Matrix[15] * z;
    }
else {
    Matrix[12] += Matrix[ 0] * x + Matrix[ 4] * y + Matrix[ 8] * z;
    Matrix[13] += Matrix[ 1] * x + Matrix[ 5] * y + Matrix[ 9] * z;
    Matrix[14] += Matrix[ 2] * x + Matrix[ 6] * y + Matrix[10] * z;
    }
}


void    TMatrix44::SetTranslation ( double x, double y, double z )
{
Matrix[12]  =  x;
Matrix[13]  =  y;
Matrix[14]  =  z;
}


//----------------------------------------------------------------------------
void    TMatrix44::ScaleX ( double x, MatrixMultiplicationSide side )
{
if ( x == 1.0 )
    return;

if ( side == MultiplyLeft ) {
    Matrix[ 0] *= x;
    Matrix[ 4] *= x;
    Matrix[ 8] *= x;
    Matrix[12] *= x;
    }
else {
    Matrix[ 0] *= x;
    Matrix[ 1] *= x;
    Matrix[ 2] *= x;
    Matrix[ 3] *= x;
    }
}


void    TMatrix44::ScaleY ( double y, MatrixMultiplicationSide side )
{
if ( y == 1.0 )
    return;

if ( side == MultiplyLeft ) {
                        Matrix[ 1] *= y;
                        Matrix[ 5] *= y;
                        Matrix[ 9] *= y;
                        Matrix[13] *= y;
    }
else {
                        Matrix[ 4] *= y;
                        Matrix[ 5] *= y;
                        Matrix[ 6] *= y;
                        Matrix[ 7] *= y;
    }
}


void    TMatrix44::ScaleZ ( double z, MatrixMultiplicationSide side )
{
if ( z == 1.0 )
    return;

if ( side == MultiplyLeft ) {
                                            Matrix[ 2] *= z;
                                            Matrix[ 6] *= z;
                                            Matrix[10] *= z;
                                            Matrix[14] *= z;
    }
else {
                                            Matrix[ 8] *= z;
                                            Matrix[ 9] *= z;
                                            Matrix[10] *= z;
                                            Matrix[11] *= z;
    }
}


void    TMatrix44::Scale ( double x, double y, double z, MatrixMultiplicationSide side )
{
if ( x == 1.0 && y == 1.0 && z == 1.0 )
    return;

if ( side == MultiplyLeft ) {
    Matrix[ 0] *= x;    Matrix[ 1] *= y;    Matrix[ 2] *= z;
    Matrix[ 4] *= x;    Matrix[ 5] *= y;    Matrix[ 6] *= z;
    Matrix[ 8] *= x;    Matrix[ 9] *= y;    Matrix[10] *= z;
    Matrix[12] *= x;    Matrix[13] *= y;    Matrix[14] *= z;
    }
else {
    Matrix[ 0] *= x;    Matrix[ 4] *= y;    Matrix[ 8] *= z;
    Matrix[ 1] *= x;    Matrix[ 5] *= y;    Matrix[ 9] *= z;
    Matrix[ 2] *= x;    Matrix[ 6] *= y;    Matrix[10] *= z;
    Matrix[ 3] *= x;    Matrix[ 7] *= y;    Matrix[11] *= z;
    }
}


void    TMatrix44::Scale ( double s, MatrixMultiplicationSide side )
{
Scale ( s, s, s, side );
}


//----------------------------------------------------------------------------
void    TMatrix44::ShearX ( double sy, double sz, MatrixMultiplicationSide side )
{
if ( ! ( sy || sz ) )   return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 1] += Matrix[ 0] * sy;
    Matrix[ 2] += Matrix[ 0] * sz;

    Matrix[ 5] += Matrix[ 4] * sy;
    Matrix[ 6] += Matrix[ 4] * sz;

    Matrix[ 9] += Matrix[ 8] * sy;
    Matrix[10] += Matrix[ 8] * sz;

    Matrix[13] += Matrix[12] * sy;
    Matrix[14] += Matrix[12] * sz;
    }
else { // on right
    Matrix[ 0] += Matrix[ 4] * sy + Matrix[ 8] * sz;
    Matrix[ 1] += Matrix[ 5] * sy + Matrix[ 9] * sz;
    Matrix[ 2] += Matrix[ 6] * sy + Matrix[10] * sz;
    Matrix[ 3] += Matrix[ 7] * sy + Matrix[11] * sz;
    }
}


void    TMatrix44::ShearY ( double sx, double sz, MatrixMultiplicationSide side )
{
if ( ! ( sx || sz ) )   return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 0] += Matrix[ 1] * sx;
    Matrix[ 2] += Matrix[ 1] * sz;

    Matrix[ 4] += Matrix[ 5] * sx;
    Matrix[ 6] += Matrix[ 5] * sz;

    Matrix[ 8] += Matrix[ 9] * sx;
    Matrix[10] += Matrix[ 9] * sz;

    Matrix[12] += Matrix[13] * sx;
    Matrix[14] += Matrix[13] * sz;
    }
else { // on right
    Matrix[ 4] += Matrix[ 0] * sx + Matrix[ 8] * sz;
    Matrix[ 5] += Matrix[ 1] * sx + Matrix[ 9] * sz;
    Matrix[ 6] += Matrix[ 2] * sx + Matrix[10] * sz;
    Matrix[ 7] += Matrix[ 3] * sx + Matrix[11] * sz;
    }
}


void    TMatrix44::ShearZ ( double sx, double sy, MatrixMultiplicationSide side )
{
if ( ! ( sx || sy ) )   return;

                                        // no needs for temp copy
if ( side == MultiplyLeft ) {
    Matrix[ 0] += Matrix[ 2] * sx;
    Matrix[ 1] += Matrix[ 2] * sy;

    Matrix[ 4] += Matrix[ 6] * sx;
    Matrix[ 5] += Matrix[ 6] * sy;

    Matrix[ 8] += Matrix[10] * sx;
    Matrix[ 9] += Matrix[10] * sy;

    Matrix[12] += Matrix[14] * sx;
    Matrix[13] += Matrix[14] * sy;
    }
else { // on right
    Matrix[ 8] += Matrix[ 0] * sx + Matrix[ 4] * sy;
    Matrix[ 9] += Matrix[ 1] * sx + Matrix[ 5] * sy;
    Matrix[10] += Matrix[ 2] * sx + Matrix[ 6] * sy;
    Matrix[11] += Matrix[ 3] * sx + Matrix[ 7] * sy;
    }
}


//----------------------------------------------------------------------------
                                        // in the negative Z space, for symmetric case
void    TMatrix44::PerspectiveZtoXY ( double n, double f, MatrixMultiplicationSide side )
{
if ( n == f )   return;

                                        // some parameters checking?
//n       = - fabs ( n );
//f       = - fabs ( f );
//CheckOrder ( f, n );


double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[ 0] *= n;
    Matrix[ 1] *= n;
    Matrix[ 2]  = Mat[ 2] * ( n + f ) - Mat[ 3] * n * f;
    Matrix[ 3]  = Mat[ 2];

    Matrix[ 4] *= n;
    Matrix[ 5] *= n;
    Matrix[ 6]  = Mat[ 6] * ( n + f ) - Mat[ 7] * n * f;
    Matrix[ 7]  = Mat[ 6];

    Matrix[ 8] *= n;
    Matrix[ 9] *= n;
    Matrix[10]  = Mat[10] * ( n + f ) - Mat[11] * n * f;
    Matrix[11]  = Mat[10];

    Matrix[12] *= n;
    Matrix[13] *= n;
    Matrix[14]  = Mat[14] * ( n + f ) - Mat[15] * n * f;
    Matrix[15]  = Mat[14];
    }
else { // on right
    Matrix[ 0] *= n;
    Matrix[ 1] *= n;
    Matrix[ 2] *= n;
    Matrix[ 3] *= n;

    Matrix[ 4] *= n;
    Matrix[ 5] *= n;
    Matrix[ 6] *= n;
    Matrix[ 7] *= n;

    Matrix[ 8]  = Mat[ 8] * ( n + f ) + Mat[12];
    Matrix[ 9]  = Mat[ 9] * ( n + f ) + Mat[13];
    Matrix[10]  = Mat[10] * ( n + f ) + Mat[14];
    Matrix[11]  = Mat[11] * ( n + f ) + Mat[15];

    Matrix[12]  = - Mat[ 8] * n * f;
    Matrix[13]  = - Mat[ 9] * n * f;
    Matrix[14]  = - Mat[10] * n * f;
    Matrix[15]  = - Mat[11] * n * f;
    }

                                        // compensating perspective shift
//TranslateZ ( - Square ( n - f ) / 2 / ( n + f ), side );
}

                                        // !not sure if correct!
                                        // should be ZtoX, but the results seem look more like ZtoY...
void    TMatrix44::PerspectiveZtoY ( double n, double f, MatrixMultiplicationSide side )
{
if ( n == f )   return;

                                        // some parameters checking?
//n       = - fabs ( n );
//f       = - fabs ( f );
//CheckOrder ( f, n );


double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[ 0] *= n;
//  Matrix[ 1] *= n;
    Matrix[ 2]  = Mat[ 2] * ( n + f ) - Mat[ 3] * n * f;
    Matrix[ 3]  = Mat[ 2];

    Matrix[ 4] *= n;
//  Matrix[ 5] *= n;
    Matrix[ 6]  = Mat[ 6] * ( n + f ) - Mat[ 7] * n * f;
    Matrix[ 7]  = Mat[ 6];

    Matrix[ 8] *= n;
//  Matrix[ 9] *= n;
    Matrix[10]  = Mat[10] * ( n + f ) - Mat[11] * n * f;
    Matrix[11]  = Mat[10];

    Matrix[12] *= n;
//  Matrix[13] *= n;
    Matrix[14]  = Mat[14] * ( n + f ) - Mat[15] * n * f;
    Matrix[15]  = Mat[14];

                                        // revert in advance normalization
    Matrix[ 1] *= Matrix[15];
    Matrix[ 5] *= Matrix[15];
    Matrix[ 9] *= Matrix[15];
    Matrix[13] *= Matrix[15];
        }
else { // on right
    Matrix[ 0] *= n;
    Matrix[ 1] *= n;
    Matrix[ 2] *= n;
    Matrix[ 3] *= n;

//  Matrix[ 4] *= n;
//  Matrix[ 5] *= n;
//  Matrix[ 6] *= n;
//  Matrix[ 7] *= n;

    Matrix[ 8]  = Mat[ 8] * ( n + f ) + Mat[12];
    Matrix[ 9]  = Mat[ 9] * ( n + f ) + Mat[13];
    Matrix[10]  = Mat[10] * ( n + f ) + Mat[14];
    Matrix[11]  = Mat[11] * ( n + f ) + Mat[15];

    Matrix[12]  = - Mat[ 8] * n * f;
    Matrix[13]  = - Mat[ 9] * n * f;
    Matrix[14]  = - Mat[10] * n * f;
    Matrix[15]  = - Mat[11] * n * f;

                                        // revert in advance normalization
    Matrix[ 4] *= Matrix[15];
    Matrix[ 5] *= Matrix[15];
    Matrix[ 6] *= Matrix[15];
    Matrix[ 7] *= Matrix[15];
    }
}

                                        // !not sure if correct!
void    TMatrix44::PerspectiveZtoX ( double n, double f, MatrixMultiplicationSide side )
{
if ( n == f )   return;

                                        // some parameters checking?
//n       = - fabs ( n );
//f       = - fabs ( f );
//CheckOrder ( f, n );


double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
//  Matrix[ 0] *= n;
    Matrix[ 1] *= n;
    Matrix[ 2]  = Mat[ 2] * ( n + f ) - Mat[ 3] * n * f;
    Matrix[ 3]  = Mat[ 2];

//  Matrix[ 4] *= n;
    Matrix[ 5] *= n;
    Matrix[ 6]  = Mat[ 6] * ( n + f ) - Mat[ 7] * n * f;
    Matrix[ 7]  = Mat[ 6];

//  Matrix[ 8] *= n;
    Matrix[ 9] *= n;
    Matrix[10]  = Mat[10] * ( n + f ) - Mat[11] * n * f;
    Matrix[11]  = Mat[10];

//  Matrix[12] *= n;
    Matrix[13] *= n;
    Matrix[14]  = Mat[14] * ( n + f ) - Mat[15] * n * f;
    Matrix[15]  = Mat[14];

                                        // revert in advance normalization
    Matrix[ 0] *= Matrix[15];
    Matrix[ 4] *= Matrix[15];
    Matrix[ 8] *= Matrix[15];
    Matrix[12] *= Matrix[15];
    }
else { // on right
//  Matrix[ 0] *= n;
//  Matrix[ 1] *= n;
//  Matrix[ 2] *= n;
//  Matrix[ 3] *= n;

    Matrix[ 4] *= n;
    Matrix[ 5] *= n;
    Matrix[ 6] *= n;
    Matrix[ 7] *= n;

    Matrix[ 8]  = Mat[ 8] * ( n + f ) + Mat[12];
    Matrix[ 9]  = Mat[ 9] * ( n + f ) + Mat[13];
    Matrix[10]  = Mat[10] * ( n + f ) + Mat[14];
    Matrix[11]  = Mat[11] * ( n + f ) + Mat[15];

    Matrix[12]  = - Mat[ 8] * n * f;
    Matrix[13]  = - Mat[ 9] * n * f;
    Matrix[14]  = - Mat[10] * n * f;
    Matrix[15]  = - Mat[11] * n * f;

                                        // revert in advance normalization
    Matrix[ 0] *= Matrix[15];
    Matrix[ 1] *= Matrix[15];
    Matrix[ 2] *= Matrix[15];
    Matrix[ 3] *= Matrix[15];
    }
}


//----------------------------------------------------------------------------
void    TMatrix44::OrientationRasToPir ( MatrixMultiplicationSide side )
{
TMatrix44           m;

if ( side == MultiplyRight ) {
    m.RotateY ( -90, MultiplyRight );   // Tested OK
    m.RotateX (  90, MultiplyRight );
    }
else {
    m.RotateX ( -90, MultiplyRight );   // NOT tested
    m.RotateY (  90, MultiplyRight );
    }
                                        // we know matrix is just swapping axis
m.Rounding ( 1e-6 );

Multiply ( m, MultiplyRight );
}


void    TMatrix44::OrientationPirToRas ( MatrixMultiplicationSide side )
{
TMatrix44           m;

if ( side == MultiplyRight ) {
    m.RotateX (  90, MultiplyLeft );    // NOT tested
    m.RotateY ( -90, MultiplyLeft );
    }
else {
    m.RotateY (  90, MultiplyLeft );    // Tested OK
    m.RotateX ( -90, MultiplyLeft );
    }
                                        // we know matrix is just swapping axis
m.Rounding ( 1e-6 );

Multiply ( m, MultiplyLeft );
}


//----------------------------------------------------------------------------
                                        // multiply on the right
TMatrix44   TMatrix44::operator* ( const TMatrix44& op2 )   const
{
TMatrix44           m ( *this );

m.Multiply ( op2, MultiplyRight );

return  m;
}

                                        // multiply on the right
TMatrix44&  TMatrix44::operator*= ( const TMatrix44& op )
{
Multiply ( op, MultiplyRight );

return  *this;
}

                                        // can conveniently multiply on either side
void        TMatrix44::Multiply ( const TMatrix44& m, MatrixMultiplicationSide side )
{
if ( m.IsIdentity () )
    return;


double              Mat[ HomogeneousMatrixSize ];  // temp copy

CopyTo ( Mat );


if ( side == MultiplyLeft ) {
    Matrix[0]   = Mat[0]  * m.Matrix[0]  + Mat[1]  * m.Matrix[4]  + Mat[2]  * m.Matrix[8]  + Mat[3]  * m.Matrix[12];
    Matrix[1]   = Mat[0]  * m.Matrix[1]  + Mat[1]  * m.Matrix[5]  + Mat[2]  * m.Matrix[9]  + Mat[3]  * m.Matrix[13];
    Matrix[2]   = Mat[0]  * m.Matrix[2]  + Mat[1]  * m.Matrix[6]  + Mat[2]  * m.Matrix[10] + Mat[3]  * m.Matrix[14];
    Matrix[3]   = Mat[0]  * m.Matrix[3]  + Mat[1]  * m.Matrix[7]  + Mat[2]  * m.Matrix[11] + Mat[3]  * m.Matrix[15];

    Matrix[4]   = Mat[4]  * m.Matrix[0]  + Mat[5]  * m.Matrix[4]  + Mat[6]  * m.Matrix[8]  + Mat[7]  * m.Matrix[12];
    Matrix[5]   = Mat[4]  * m.Matrix[1]  + Mat[5]  * m.Matrix[5]  + Mat[6]  * m.Matrix[9]  + Mat[7]  * m.Matrix[13];
    Matrix[6]   = Mat[4]  * m.Matrix[2]  + Mat[5]  * m.Matrix[6]  + Mat[6]  * m.Matrix[10] + Mat[7]  * m.Matrix[14];
    Matrix[7]   = Mat[4]  * m.Matrix[3]  + Mat[5]  * m.Matrix[7]  + Mat[6]  * m.Matrix[11] + Mat[7]  * m.Matrix[15];

    Matrix[8]   = Mat[8]  * m.Matrix[0]  + Mat[9]  * m.Matrix[4]  + Mat[10] * m.Matrix[8]  + Mat[11] * m.Matrix[12];
    Matrix[9]   = Mat[8]  * m.Matrix[1]  + Mat[9]  * m.Matrix[5]  + Mat[10] * m.Matrix[9]  + Mat[11] * m.Matrix[13];
    Matrix[10]  = Mat[8]  * m.Matrix[2]  + Mat[9]  * m.Matrix[6]  + Mat[10] * m.Matrix[10] + Mat[11] * m.Matrix[14];
    Matrix[11]  = Mat[8]  * m.Matrix[3]  + Mat[9]  * m.Matrix[7]  + Mat[10] * m.Matrix[11] + Mat[11] * m.Matrix[15];

    Matrix[12]  = Mat[12] * m.Matrix[0]  + Mat[13] * m.Matrix[4]  + Mat[14] * m.Matrix[8]  + Mat[15] * m.Matrix[12];
    Matrix[13]  = Mat[12] * m.Matrix[1]  + Mat[13] * m.Matrix[5]  + Mat[14] * m.Matrix[9]  + Mat[15] * m.Matrix[13];
    Matrix[14]  = Mat[12] * m.Matrix[2]  + Mat[13] * m.Matrix[6]  + Mat[14] * m.Matrix[10] + Mat[15] * m.Matrix[14];
    Matrix[15]  = Mat[12] * m.Matrix[3]  + Mat[13] * m.Matrix[7]  + Mat[14] * m.Matrix[11] + Mat[15] * m.Matrix[15];
    }

else { // MultiplyRight
    Matrix[0]   = Mat[0]  * m.Matrix[0]  + Mat[4]  * m.Matrix[1]  + Mat[8]  * m.Matrix[2]  + Mat[12] * m.Matrix[3];
    Matrix[1]   = Mat[1]  * m.Matrix[0]  + Mat[5]  * m.Matrix[1]  + Mat[9]  * m.Matrix[2]  + Mat[13] * m.Matrix[3];
    Matrix[2]   = Mat[2]  * m.Matrix[0]  + Mat[6]  * m.Matrix[1]  + Mat[10] * m.Matrix[2]  + Mat[14] * m.Matrix[3];
    Matrix[3]   = Mat[3]  * m.Matrix[0]  + Mat[7]  * m.Matrix[1]  + Mat[11] * m.Matrix[2]  + Mat[15] * m.Matrix[3];

    Matrix[4]   = Mat[0]  * m.Matrix[4]  + Mat[4]  * m.Matrix[5]  + Mat[8]  * m.Matrix[6]  + Mat[12] * m.Matrix[7];
    Matrix[5]   = Mat[1]  * m.Matrix[4]  + Mat[5]  * m.Matrix[5]  + Mat[9]  * m.Matrix[6]  + Mat[13] * m.Matrix[7];
    Matrix[6]   = Mat[2]  * m.Matrix[4]  + Mat[6]  * m.Matrix[5]  + Mat[10] * m.Matrix[6]  + Mat[14] * m.Matrix[7];
    Matrix[7]   = Mat[3]  * m.Matrix[4]  + Mat[7]  * m.Matrix[5]  + Mat[11] * m.Matrix[6]  + Mat[15] * m.Matrix[7];

    Matrix[8]   = Mat[0]  * m.Matrix[8]  + Mat[4]  * m.Matrix[9]  + Mat[8]  * m.Matrix[10] + Mat[12] * m.Matrix[11];
    Matrix[9]   = Mat[1]  * m.Matrix[8]  + Mat[5]  * m.Matrix[9]  + Mat[9]  * m.Matrix[10] + Mat[13] * m.Matrix[11];
    Matrix[10]  = Mat[2]  * m.Matrix[8]  + Mat[6]  * m.Matrix[9]  + Mat[10] * m.Matrix[10] + Mat[14] * m.Matrix[11];
    Matrix[11]  = Mat[3]  * m.Matrix[8]  + Mat[7]  * m.Matrix[9]  + Mat[11] * m.Matrix[10] + Mat[15] * m.Matrix[11];

    Matrix[12]  = Mat[0]  * m.Matrix[12] + Mat[4]  * m.Matrix[13] + Mat[8]  * m.Matrix[14] + Mat[12] * m.Matrix[15];
    Matrix[13]  = Mat[1]  * m.Matrix[12] + Mat[5]  * m.Matrix[13] + Mat[9]  * m.Matrix[14] + Mat[13] * m.Matrix[15];
    Matrix[14]  = Mat[2]  * m.Matrix[12] + Mat[6]  * m.Matrix[13] + Mat[10] * m.Matrix[14] + Mat[14] * m.Matrix[15];
    Matrix[15]  = Mat[3]  * m.Matrix[12] + Mat[7]  * m.Matrix[13] + Mat[11] * m.Matrix[14] + Mat[15] * m.Matrix[15];
    }

}


//----------------------------------------------------------------------------
void    TMatrix44::Apply ( float *v ) const
{
float               vo[ 4 ];
CopyVirtualMemory ( vo, v, 4 * sizeof ( *v ) );

v[ 0 ]          =  Matrix[0]  * vo[0]  + Matrix[4]  * vo[1]  + Matrix[8]  * vo[2]  + Matrix[12] * vo[3];
v[ 1 ]          =  Matrix[1]  * vo[0]  + Matrix[5]  * vo[1]  + Matrix[9]  * vo[2]  + Matrix[13] * vo[3];
v[ 2 ]          =  Matrix[2]  * vo[0]  + Matrix[6]  * vo[1]  + Matrix[10] * vo[2]  + Matrix[14] * vo[3];
v[ 3 ]          =  Matrix[3]  * vo[0]  + Matrix[7]  * vo[1]  + Matrix[11] * vo[2]  + Matrix[15] * vo[3];

if ( v[ 3 ] != 0 && v[ 3 ] != 1 ) {
    v[ 0 ] /= v[ 3 ];
    v[ 1 ] /= v[ 3 ];
    v[ 2 ] /= v[ 3 ];
    v[ 3 ]  = 1;
    }
}


void    TMatrix44::Apply ( double *v )    const
{
double              vo[ 4 ];
CopyVirtualMemory ( vo, v, 4 * sizeof ( *v ) );

v[ 0 ]          =  Matrix[0]  * vo[0]  + Matrix[4]  * vo[1]  + Matrix[8]  * vo[2]  + Matrix[12] * vo[3];
v[ 1 ]          =  Matrix[1]  * vo[0]  + Matrix[5]  * vo[1]  + Matrix[9]  * vo[2]  + Matrix[13] * vo[3];
v[ 2 ]          =  Matrix[2]  * vo[0]  + Matrix[6]  * vo[1]  + Matrix[10] * vo[2]  + Matrix[14] * vo[3];
v[ 3 ]          =  Matrix[3]  * vo[0]  + Matrix[7]  * vo[1]  + Matrix[11] * vo[2]  + Matrix[15] * vo[3];

if ( v[ 3 ] != 0 && v[ 3 ] != 1 ) {
    v[ 0 ] /= v[ 3 ];
    v[ 1 ] /= v[ 3 ];
    v[ 2 ] /= v[ 3 ];
    v[ 3 ]  = 1;
    }
}


void    TMatrix44::Apply ( TVector3Float &p ) const
{
TVector3Float       vo ( p );

p.X             =  Matrix[0]  * vo.X  + Matrix[4]  * vo.Y  + Matrix[8]  * vo.Z  + Matrix[12];
p.Y             =  Matrix[1]  * vo.X  + Matrix[5]  * vo.Y  + Matrix[9]  * vo.Z  + Matrix[13];
p.Z             =  Matrix[2]  * vo.X  + Matrix[6]  * vo.Y  + Matrix[10] * vo.Z  + Matrix[14];
double pT       =  Matrix[3]  * vo.X  + Matrix[7]  * vo.Y  + Matrix[11] * vo.Z  + Matrix[15];

if ( pT != 0 && pT != 1 )
    p  /= pT;
}


void    TMatrix44::Apply ( TVector3Double &p )    const
{
TVector3Double      vo ( p );

p.X             =  Matrix[0]  * vo.X  + Matrix[4]  * vo.Y  + Matrix[8]  * vo.Z  + Matrix[12];
p.Y             =  Matrix[1]  * vo.X  + Matrix[5]  * vo.Y  + Matrix[9]  * vo.Z  + Matrix[13];
p.Z             =  Matrix[2]  * vo.X  + Matrix[6]  * vo.Y  + Matrix[10] * vo.Z  + Matrix[14];
double pT       =  Matrix[3]  * vo.X  + Matrix[7]  * vo.Y  + Matrix[11] * vo.Z  + Matrix[15];

if ( pT != 0 && pT != 1 )
    p  /= pT;
}

                                        // convert to double, transform, then round the results
void    TMatrix44::Apply ( TVector3Int &p )   const
{
TVector3Double      pd ( p.X, p.Y, p.Z );

Apply ( pd );

pd.Round ();

p.Set ( pd );
}

                                        // Input is a range (positive) -> transform all extrema to get new range
void    TMatrix44::ApplyRange ( TVector3Int &r )  const
{
TPoints             corners;

corners.SetCorners ( 0, 0, 0, r.X, r.Y, r.Z );

Apply ( corners );

                                        // now we can compute all diagonals
corners[ 4 ]   -= corners[ 3 ];
corners[ 5 ]   -= corners[ 2 ];
corners[ 6 ]   -= corners[ 1 ];
corners[ 7 ]   -= corners[ 0 ];
                                        // can be any direction
corners[ 4 ].Absolute ();
corners[ 5 ].Absolute ();
corners[ 6 ].Absolute ();
corners[ 7 ].Absolute ();

                                        // new range is the max of all diagonals
r.X             = Round ( max ( corners[ 4 ].X, corners[ 5 ].X, corners[ 6 ].X, corners[ 7 ].X ) );
r.Y             = Round ( max ( corners[ 4 ].Y, corners[ 5 ].Y, corners[ 6 ].Y, corners[ 7 ].Y ) );
r.Z             = Round ( max ( corners[ 4 ].Z, corners[ 5 ].Z, corners[ 6 ].Z, corners[ 7 ].Z ) );
}


void    TMatrix44::Apply ( TPoints& points )    const
{
for ( int i = 0; i < points.GetNumPoints (); i++ )

    Apply ( points[ i ] );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
