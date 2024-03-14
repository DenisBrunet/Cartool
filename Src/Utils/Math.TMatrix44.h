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

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy utility used to analyse and deduce what the matrix is actually made of
enum            MatrixAnalysis
                {
                HasNoTransform          = 0x0000,

                HasTranslation          = 0x0002,
                HasIntegerTranslation   = 0x0001,
                HasTranslationMask      = HasTranslation | HasIntegerTranslation,

                HasShearing             = 0x0010,
                HasShearingMask         = HasShearing,

                HasScaling              = 0x0200,
                HasIntegerScaling       = 0x0100,
                HasScalingMask          = HasScaling | HasIntegerScaling,

                HasRotation             = 0x4000,
                HasSymmetry             = 0x2000,
                HasOrtho                = 0x1000,   // !only complementing any of the 2 above!
                HasRotationMask         = HasRotation | HasSymmetry | HasOrtho,
                };


inline  bool    IsMatrixIdentity    ( MatrixAnalysis ma )   { return  ma == HasNoTransform; }

inline  bool    IsOrthogonal        ( MatrixAnalysis ma )   { return       ( ma &   HasOrtho )                          // obvious case
                                                                      || ! ( ma & ( HasRotation | HasSymmetry ) ); }    // no rotation and no symmetry (including Identity) means transform is also orthogonal
void            ShowMatrixAnalysis  ( MatrixAnalysis ma, const char* title = 0 );


//----------------------------------------------------------------------------

constexpr int   HomogeneousMatrixSize   = 16;   // 4x4 matrix


enum            OrientEnum
                {
                OrientCoronalBack,
                OrientCoronalFront,
                OrientSagittalLeft,
                OrientSagittalRight,
                OrientTransverseTop,
                OrientTransverseBottom,

                OrientMinusYX,                  // AB: X axis/Y axis
                OrientMinusXY,
                OrientXZ,

                NumOrient       = OrientTransverseBottom + 1    // to toggle between 6 predefined views
                };


enum            MatrixMultiplicationSide
                {
                MultiplyLeft,
                MultiplyRight,
                };


//----------------------------------------------------------------------------
                                        // 4x4 homogeneous matrix
                                        // Implements all operations and does not make use of any other matrix library
class           TPoints;

class   TMatrix44
{
public:
                    TMatrix44   ();

                    TMatrix44   ( double v )                                            { Set ( v ) ; }

                    TMatrix44   ( double  m11,    double  m12,    double  m13,    double  m14,
                                  double  m21,    double  m22,    double  m23,    double  m24,
                                  double  m31,    double  m32,    double  m33,    double  m34,
                                  double  m41,    double  m42,    double  m43,    double  m44 );

                    TMatrix44   ( double precession, double nutation, double spin )     { SetEulerAngles ( precession, nutation, spin ); }

                    TMatrix44   ( const TPointDouble& p )                               { SetIdentity (); SetTranslation ( p ); }

                    TMatrix44   ( const char* file )                                    { ReadFile ( file ); }


    void            Reset           ();

    void            SetIdentity     ();
    void            SetOrientation  ( int orient );
    void            SetEulerAngles  ( double precession, double nutation, double spin );
    void            Set             ( double v );
    void            Set             ( double  m11,    double  m12,    double  m13,    double  m14,
                                      double  m21,    double  m22,    double  m23,    double  m24,
                                      double  m31,    double  m32,    double  m33,    double  m34,
                                      double  m41,    double  m42,    double  m43,    double  m44 );
    void            SetColumn       ( int c, const TPointFloat& p );


    MatrixAnalysis  Analyze         ( double epsilon )                                  const;
    bool            IsNull          ()                                                  const   { return  ! IsNotNull (); }
    bool            IsNotNull       ()                                                  const;
    bool            IsIdentity      ( double epsilon = SingleFloatEpsilon )             const;
    bool            IsNotIdentity   ( double epsilon = SingleFloatEpsilon )             const   { return  ! IsIdentity ( epsilon ); }
    bool            IsTranslation   ( double epsilon = SingleFloatEpsilon )             const;
    TPointDouble    IsIsotropic     ( const TPointDouble& inputvoxel, double epsilon )  const;
    bool            IsOrthogonal    ( double epsilon )                                  const;
    TPointDouble    IsOrthogonal    ( const TPointDouble& inputvoxel, double epsilon )  const;
    TPointDouble    GetMeanVoxel    ( const TPointDouble& inputvoxel )                  const;
    bool            ApplyVoxel      ( const TPointDouble& inputvoxel, TPointDouble (&outputvoxel)[ 3 ] )    const;

                                        // Cumulating transforms:
    void            RotateX         ( double angle, MatrixMultiplicationSide side );
    void            RotateY         ( double angle, MatrixMultiplicationSide side );
    void            RotateZ         ( double angle, MatrixMultiplicationSide side );
    void            Rotate          ( double angle, int axis, MatrixMultiplicationSide side );
    void            RotateXYZ       ( double anglex, double angley, double anglez, MatrixMultiplicationSide side );  // X, then Y, then Z

    void            TranslateX      ( double x, MatrixMultiplicationSide side );
    void            TranslateY      ( double y, MatrixMultiplicationSide side );
    void            TranslateZ      ( double z, MatrixMultiplicationSide side );
    void            Translate       ( double x, double y, double z, MatrixMultiplicationSide side );
    void            Translate       ( const TPointDouble& p, MatrixMultiplicationSide side )        { Translate ( p.X, p.Y, p.Z, side ); }

    void            ScaleX          ( double x, MatrixMultiplicationSide side );
    void            ScaleY          ( double y, MatrixMultiplicationSide side );
    void            ScaleZ          ( double z, MatrixMultiplicationSide side );
    void            Scale           ( const TPointDouble& s, MatrixMultiplicationSide side )        { Scale ( s.X, s.Y, s.Z, side ); }
    void            Scale           ( double x, double y, double z, MatrixMultiplicationSide side );
    void            Scale           ( double s, MatrixMultiplicationSide side );

    void            ShearX          ( double sy, double sz, MatrixMultiplicationSide side );  // 2 shear factors: X->Y and X->Z
    void            ShearY          ( double sx, double sz, MatrixMultiplicationSide side );  // 2 shear factors: Y->X and Y->Z
    void            ShearZ          ( double sx, double sy, MatrixMultiplicationSide side );  // 2 shear factors: Z->X and Z->Y

    void            PerspectiveZtoXY( double n, double f, MatrixMultiplicationSide side );
    void            PerspectiveZtoX ( double n, double f, MatrixMultiplicationSide side );
    void            PerspectiveZtoY ( double n, double f, MatrixMultiplicationSide side );

    void            OrientationRasToPir ( MatrixMultiplicationSide side );
    void            OrientationPirToRas ( MatrixMultiplicationSide side );


    TMatrix44&      Invert      ();
    double          Determinant ()  const;
    void            Normalize   ();
    void            Rounding    ( double precision = 1e-6 );


    double*         CopyTo   ( double* m );
    double*         CopyFrom ( const double* m );


    TPointDouble    GetTranslation  ()                          const   { return TPointDouble ( GetTranslationX (), GetTranslationY (), GetTranslationZ () ); }
    double          GetTranslationX ()                          const   { return Matrix[ 12 ]; }
    double          GetTranslationY ()                          const   { return Matrix[ 13 ]; }
    double          GetTranslationZ ()                          const   { return Matrix[ 14 ]; }
    void            SetTranslation  ( double x, double y, double z );                   // !sets ONLY the translation slots, the remaining part is ignored!
    void            SetTranslation  ( const TPointDouble& p )   { SetTranslation ( p.X, p.Y, p.Z ); }
    void            ResetTranslation ()                         { SetTranslation ( 0, 0, 0 ); }
    bool            HasSomeTranslation  ()                      const   { return  GetTranslationX () != 0 || GetTranslationY () != 0 || GetTranslationZ () != 0; }
    bool            HasNoTranslation()                          const   { return  ! HasSomeTranslation (); }


    void            Multiply    ( const TMatrix44& m, MatrixMultiplicationSide side );  // apply another matrix to this
    void            Apply       ( float*  v )                   const;
    void            Apply       ( double* v )                   const;
    void            Apply       ( TVector3Float&  p )           const;
    void            Apply       ( TVector3Double& p )           const;
    void            Apply       ( TVector3Int&    p )           const;
    void            ApplyRange  ( TVector3Int&    r )           const;
    void            Apply       ( TPoints& points )             const;


    void            WriteFile   ( const char* file )            const;
    void            ReadFile    ( const char* file );
    void            Show        ( const char* title = 0 )       const;


                    TMatrix44           ( const TMatrix44& op  );
    TMatrix44&      operator    =       ( const TMatrix44& op2 );

                    operator    double* ()                              { return Matrix; }                      // cast
    const double&   operator    []      ( int index )           const   { return Matrix[ index ]; }             // linear array indexing, can be used in left & right parts of expressions
    double&         operator    []      ( int index )                   { return Matrix[ index ]; }             // linear array indexing, can be used in left & right parts of expressions
    double&         operator    ()      ( int iline, int icol )         { return Matrix[ 4 * icol + iline ]; }  // access to line/column, can be used in left & right parts of expressions
    double          operator    ()      ( int iline, int icol ) const   { return Matrix[ 4 * icol + iline ]; }  // access to line/column, can be used in left & right parts of expressions

    TMatrix44       operator    *       ( const TMatrix44& op2 )const ;                                         // multiply on the right
    TMatrix44&      operator    *=      ( const TMatrix44& op  );                                               // multiply on the right

    bool            operator    ==      ( const TMatrix44& op2 )const;              
    bool            operator    !=      ( const TMatrix44& op2 )const;              


protected:

    double          Matrix[ HomogeneousMatrixSize ];    // Stored internally as Column-major (FORTRAN / Matlab style)
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
