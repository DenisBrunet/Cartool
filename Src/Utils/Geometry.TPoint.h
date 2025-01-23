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

#include    "MemUtil.h"
#include    "CartoolTypes.h"            // MriType

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // For classes that needs an index to their objects
#define             TIndexNull          -1


class   TIndex
{
public:
                    TIndex ()                   { Reset (); }
                    TIndex ( int i )            { Index = i; }

    int             Index;

    void            Reset ()                    { Index = TIndexNull; }

    bool            IsNullIndex     ()  const   { return  Index == TIndexNull; }
    bool            IsNotNullIndex  ()  const   { return  Index != TIndexNull; }

                    TIndex          ( const TIndex& op  )   { Index = op .Index;}
    TIndex&         operator    =   ( const TIndex& op2 )   { Index = op2.Index;    return *this;   }
};


//----------------------------------------------------------------------------
                                        // Forward declarations
using       GLfloat             = float;
template <class GLfloat>    class   TGLCoordinates;
template <class MriType>    class   TVolume;


                                        // Either 3D point or 3D vector
template <class TypeD>
class   TPointT
{
public:
                    TPointT<TypeD> ()                                   { Reset (); }
                    TPointT<TypeD> ( TypeD v           )                { Set ( v, v, v ); }
                    TPointT<TypeD> ( TypeD x, TypeD y, TypeD z )        { Set ( x, y, z ); }
                    TPointT<TypeD> ( const TPointT<double>&p )          { Set ( p ); }
                    TPointT<TypeD> ( const TPointT<float> &p )          { Set ( p ); }
                    TPointT<TypeD> ( const TPointT<int>   &p )          { Set ( p ); }
                    TPointT<TypeD> ( const TPointT<float> *p )          { Set ( p ); }
                    TPointT<TypeD> ( const TPointT<double>*p )          { Set ( p ); }
                    TPointT<TypeD> ( const TGLCoordinates<GLfloat>& p ) { Set ( p ); }
//                  TPointT<TypeD> ( const float          *p )          { Set ( p ); }  // obsolete, use another constructor to avoid hidden cast to float*
//                  TPointT<TypeD> ( const double         *p )          { Set ( p ); }  // obsolete, use another constructor to avoid hidden cast to double*

                                        // Totally public, help yourself
    TypeD           X;
    TypeD           Y;
    TypeD           Z;


    size_t          AtomSize   ()   const                           { return sizeof ( TypeD ); }
    size_t          MemorySize ()   const                           { return 3 * AtomSize (); }

    void            Reset ()                                        { X = Y = Z = 0; }

    void            Set   ( TypeD x, TypeD y, TypeD z )             { X = x;            Y = y;          Z = z; }
//  void            Set   ( double x, double y, double z )          { X = (TypeD) x;    Y = (TypeD) y;	Z = (TypeD) z; }
//  void            Set   ( float  x, float  y, float  z )          { X = (TypeD) x;    Y = (TypeD) y;	Z = (TypeD) z; }
//  void            Set   ( int    x, int    y, int    z )          { X = (TypeD) x;    Y = (TypeD) y;	Z = (TypeD) z; }
    void            Set   ( const TPointT<double>& p )              { X = p.X;          Y = p.Y;        Z = p.Z; }
    void            Set   ( const TPointT<float>&  p )              { X = p.X;          Y = p.Y;        Z = p.Z; }
    void            Set   ( const TPointT<int>&    p )              { X = p.X;          Y = p.Y;        Z = p.Z; }
    void            Set   ( const TPointT<float>*  p )              { X = p->X;         Y = p->Y;       Z = p->Z; }
    void            Set   ( const TPointT<double>* p )              { X = p->X;         Y = p->Y;       Z = p->Z; }
    void            Set   ( const TGLCoordinates<GLfloat>& p )      { X = p.X / NonNull ( p.W ); Y = p.Y / NonNull ( p.W ); Z = p.Z / NonNull ( p.W ); }
//  void            Set   ( const float*  values )                  { X = values[ 0 ]; Y = values[ 1 ]; Z = values[ 2 ]; }
//  void            Set   ( const double* values )                  { X = values[ 0 ]; Y = values[ 1 ]; Z = values[ 2 ]; }


    bool            IsNull    ()                                                        const   { return ! ( X || Y || Z ); }
    bool            IsNotNull ()                                                        const   { return     X || Y || Z; }
    bool            IsOppositeDirection ( const TPointT<TypeD> &op2 )                   const   { return    ScalarProduct ( op2 ) < 0; }    // !strict comparison - a null vector should not return true!
    bool            IsSameDirection     ( const TPointT<TypeD> &op2 )                   const   { return    ScalarProduct ( op2 ) > 0; }    //    "        "
    bool            IsAxisAligned       ( double epsilon )                              const;
    bool            IsAligned           ( const TPointT<TypeD> &op2, double epsilon )   const   { return  fabs ( Cosine ( op2 ) ) > 1 - epsilon; }
    bool            IsIsotropic         ( double epsilon )                              const;


    TypeD           Norm      ()    const                           { return    sqrt ( X * X + Y * Y + Z * Z ); }
    TypeD           Norm2     ()    const                           { return           X * X + Y * Y + Z * Z; }
    TypeD           Norm3     ()    const                           { return    pow  ( X * X + Y * Y + Z * Z, 1.5 ); }
    TypeD           Average   ()    const                           { return    (TypeD) ( ( X + Y + Z ) / 3.0 ); }
    void            Normalize ()                                    { double    n = Norm (); if ( n ) { X = (TypeD) ( X / n ); Y = (TypeD) ( Y / n ); Z = (TypeD) ( Z / n ); } }
    void            Invert    ()                                    { X = (TypeD) -X; Y = (TypeD) -Y; Z = (TypeD) -Z; }
    void            Absolute  ()                                    { X = std::abs ( X ); Y = std::abs ( Y ); Z = std::abs ( Z ); }
    void            Power     ( int p )                             { X = crtl::Power ( X, p ); Y = crtl::Power ( Y, p ); Z = crtl::Power ( Z, p ); }
    void            Round     ();
    void            RoundTo   ( double precision );
    void            RoundAbove();
    void            RoundBelow();
    void            Truncate  ();
    void            Clipped   ( TypeD limitmin, TypeD limitmax );
    TypeD           AbsMax    ()    const                               { return  max ( fabs ( X ), fabs ( Y ), fabs ( Z ) ); }
    TypeD           Max       ()    const                               { return  max ( X, Y, Z ); }
    void            Maxed     ( const TPointT<TypeD> &op )              { return  Maxed ( X, op.X ); Maxed ( Y, op.Y ); Maxed ( Z, op.Z ); }
    TypeD           Min       ()    const                               { return  min ( X, Y, Z ); }
    void            Mined     ( const TPointT<TypeD> &op )              { return  Mined ( X, op.X ); Mined ( Y, op.Y ); Mined ( Z, op.Z ); }
    TypeD           Mean      ()    const                               { return  (TypeD) ( ( X + Y + Z ) / 3 ); }
    void            Cumulate  ( const TPointT<TypeD> &p, bool invert )  { if ( invert ) (*this) -= p; else (*this) += p; }


    void            ToSpherical ( double& theta, double& phi, double& rho );// Theta is in XY plane (0..2Pi, Longitude), Phi is angle from Z (0..Pi, Latitude)     
    void            ToCartesian ( double  theta, double  phi, double  rho );   


    TypeD           ScalarProduct           ( const TPointT<TypeD> &op2 )               const;
    TypeD           Cosine                  ( const TPointT<TypeD> &op2 )               const;
    TypeD           Sine                    ( const TPointT<TypeD> &op2 )               const;
    TPointT         VectorialProduct        ( const TPointT<TypeD> &op2 )               const;
    void            GetOrthogonalVectors    ( TPointT<TypeD> &p1, TPointT<TypeD> &p2 )  const;
    TPointT         ProjectedOn             ( const TPointT<TypeD> &dir )               const;  // result is in dir axis
    TPointT         OrthogonalTo            ( const TPointT<TypeD> &dir )               const;
    void            OrthogonalizedTo        ( const TPointT<TypeD> &dir );
    bool            ResurfacePoint          ( const Volume& volume, const TPointT<TypeD>& center, const TPointT<TypeD>& dir, double threshold );
    bool            ResurfacePoint          ( const Volume& volume, const TPointT<TypeD>& center, double threshold );


    void            Show ( const char *title = 0 )  const;


//                  TPointT<TypeD>  ( const TPointT<TypeD> &op  )           { X = op .X;          Y = op .Y;          Z = op .Z;                          }	// done in specialized constructors above, which allows copying across TPoinT's of different types
//  TPointT<TypeD>& operator    =   ( const TPointT<TypeD> &op2 )           { X = op2.X;          Y = op2.Y;          Z = op2.Z;          return *this;   }
                                        // Explicit assignations, which can be done from a different type, so it also does the conversion
    TPointT<TypeD>& operator    =   ( const TPointT<int>&           op2 )   { X = op2.X;          Y = op2.Y;          Z = op2.Z;          return *this;   }
    TPointT<TypeD>& operator    =   ( const TPointT<float>&         op2 )   { X = op2.X;          Y = op2.Y;          Z = op2.Z;          return *this;   }
    TPointT<TypeD>& operator    =   ( const TPointT<double>&        op2 )   { X = op2.X;          Y = op2.Y;          Z = op2.Z;          return *this;   }
    TPointT<TypeD>& operator    =   ( const TPointT<long double>&   op2 )   { X = op2.X;          Y = op2.Y;          Z = op2.Z;          return *this;   }


    TypeD&          operator    []  ( int     index )               { return    index == 0 ? X : index == 1 ? Y : Z; }
    const TypeD&    operator    []  ( int     index ) const         { return    index == 0 ? X : index == 1 ? Y : Z; }


                    operator    TypeD*          ()                  { return    &X; } // cast
                    operator    const TypeD*    ()                  { return    &X; } // cast
                    operator    const TypeD*    ()  const           { return    &X; } // cast
//                  operator    bool            ()                  { return    X || Y || Z; }  // seems an ambiguous operator, use IsNotNull instead


    TPointT<TypeD>& operator    =   ( TypeD             op2 )       {                       X = Y = Z = op2;                                            return *this;  }
    TPointT<TypeD>& operator    =   ( const TypeD*      op2 )       {                       X = op2[ 0 ];       Y = op2[ 1 ];       Z = op2[ 2 ];       return *this;  }
    TPointT<TypeD>& operator    =   ( const TGLCoordinates<GLfloat>& op2 )    { Set ( op2 );    return *this;  }


    TPointT<TypeD>  operator    +   ( int                   op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X += op2;                  temp.Y += op2;                  temp.Z += op2;      return  temp;  }
    TPointT<TypeD>  operator    +   ( double                op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X += op2;                  temp.Y += op2;                  temp.Z += op2;      return  temp;  }
    TPointT<TypeD>  operator    +   ( long double           op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X += op2;                  temp.Y += op2;                  temp.Z += op2;      return  temp;  }
    TPointT<TypeD>  operator    +   ( const TPointT<TypeD> &op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X += op2.X;                temp.Y += op2.Y;                temp.Z += op2.Z;    return  temp;  }
    TPointT<TypeD>  operator    -   ()                              const   { TPointT<TypeD> temp;                          temp.X = -X;                    temp.Y = -Y;                    temp.Z = -Z;        return  temp;  }
    TPointT<TypeD>  operator    -   ( int                   op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X -= op2;                  temp.Y -= op2;                  temp.Z -= op2;      return  temp;  }
    TPointT<TypeD>  operator    -   ( double                op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X -= op2;                  temp.Y -= op2;                  temp.Z -= op2;      return  temp;  }
    TPointT<TypeD>  operator    -   ( long double           op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X -= op2;                  temp.Y -= op2;                  temp.Z -= op2;      return  temp;  }
    TPointT<TypeD>  operator    -   ( const TPointT<TypeD> &op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X -= op2.X;                temp.Y -= op2.Y;                temp.Z -= op2.Z;    return  temp;  }
    TPointT<TypeD>  operator    *   ( int                   op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X *= op2;                  temp.Y *= op2;                  temp.Z *= op2;      return  temp;  }
    TPointT<TypeD>  operator    *   ( double                op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X *= op2;                  temp.Y *= op2;                  temp.Z *= op2;      return  temp;  }
    TPointT<TypeD>  operator    *   ( long double           op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X *= op2;                  temp.Y *= op2;                  temp.Z *= op2;      return  temp;  }
    TPointT<TypeD>  operator    *   ( const TPointT<TypeD> &op2 )   const   { TPointT<TypeD> temp ( *this );                temp.X *= op2.X;                temp.Y *= op2.Y;                temp.Z *= op2.Z;    return  temp;  }    // !component-wise operation!
    TPointT<TypeD>  operator    /   ( int                   op2 )   const   { TPointT<TypeD> temp ( *this ); if ( op2 ) {   temp.X /= op2;                  temp.Y /= op2;                  temp.Z /= op2; }    return  temp;  }
    TPointT<TypeD>  operator    /   ( double                op2 )   const   { TPointT<TypeD> temp ( *this ); if ( op2 ) {   temp.X /= op2;                  temp.Y /= op2;                  temp.Z /= op2; }    return  temp;  }
    TPointT<TypeD>  operator    /   ( long double           op2 )   const   { TPointT<TypeD> temp ( *this ); if ( op2 ) {   temp.X /= op2;                  temp.Y /= op2;                  temp.Z /= op2; }    return  temp;  }
    TPointT<TypeD>  operator    /   ( const TPointT<TypeD> &op2 )   const   { TPointT<TypeD> temp ( *this ); if ( op2.X )   temp.X /= op2.X; if ( op2.Y )   temp.Y /= op2.Y; if ( op2.Z )   temp.Z /= op2.Z;    return  temp;  }    // !component-wise operation!

    TPointT<TypeD>& operator    +=  ( double                op2 )           {                                               X     += op2;                   Y     += op2;                   Z     += op2;       return  *this; }
    TPointT<TypeD>& operator    +=  ( const TPointT<TypeD> &op2 )           {                                               X     += op2.X;                 Y     += op2.Y;                 Z     += op2.Z;     return  *this; }
    TPointT<TypeD>& operator    +=  ( const double         *op2 )           {                                               X     += op2[ 0 ];              Y     += op2[ 1 ];              Z     += op2[ 2 ];  return  *this; }
    TPointT<TypeD>& operator    -=  ( double                op2 )           {                                               X     -= op2;                   Y     -= op2;                   Z     -= op2;       return  *this; }
    TPointT<TypeD>& operator    -=  ( const TPointT<TypeD> &op2 )           {                                               X     -= op2.X;                 Y     -= op2.Y;                 Z     -= op2.Z;     return  *this; }
    TPointT<TypeD>& operator    -=  ( const double         *op2 )           {                                               X     -= op2[ 0 ];              Y     -= op2[ 1 ];              Z     -= op2[ 2 ];  return  *this; }
//  TPointT<TypeD>& operator    *=  ( double                op2 )           {                                               X     *= op2;                   Y     *= op2;                   Z     *= op2;       return  *this; }
    TPointT<TypeD>& operator    *=  ( double                op2 )           {                                               X      = (TypeD) ( X * op2 );   Y      = (TypeD) ( Y * op2 );   Z      = (TypeD) ( Z * op2 );	return  *this; }
    TPointT<TypeD>& operator    *=  ( const TPointT<TypeD> &op2 )           {                                               X     *= op2.X;                 Y     *= op2.Y;                 Z     *= op2.Z;     return  *this; }
    TPointT<TypeD>& operator    *=  ( double               *op2 )           {                                               X     *= op2[ 0 ];              Y     *= op2[ 1 ];              Z     *= op2[ 2 ];  return  *this; }
    TPointT<TypeD>& operator    /=  ( double                op2 )           {                                if ( op2 ) {   X     /= op2;                   Y     /= op2;                   Z     /= op2; }     return  *this; }
    TPointT<TypeD>& operator    /=  ( const TPointT<TypeD> &op2 )           {                                if ( op2.X )   X     /= op2.X; if ( op2.Y )    Y     /= op2.Y; if ( op2.Z )    Z     /= op2.Z;     return  *this; }


    bool            operator    ==  ( TypeD             op2 )       const   { return        X == op2    && Y == op2     && Z == op2;   }
    bool            operator    ==  ( const TPointT    &op2 )       const   { return        X == op2.X  && Y == op2.Y   && Z == op2.Z; }
    bool            operator    !=  ( const TPointT    &op2 )       const   { return        X != op2.X  || Y != op2.Y   || Z != op2.Z; }
    bool            operator    !=  ( TypeD             op2 )       const   { return ! (    X == op2    && Y == op2     && Z == op2 ); }
    bool            operator    >   ( double            op2 )       const   { return        X > op2     && Y > op2      && Z > op2; }
    bool            operator    >   ( const TPointT    &op2 )       const   { return        X > op2.X   || X == op2.X && ( Y > op2.Y || Y == op2.Y && Z > op2.Z ); }
    bool            operator    <   ( const TPointT    &op2 )       const   { return        X < op2.X   || X == op2.X && ( Y < op2.Y || Y == op2.Y && Z < op2.Z ); }
};


//----------------------------------------------------------------------------
                                        // Point with an index
template <class TypeD>
class   TPointTI :  public TPointT<TypeD>,
                    public TIndex
{
public:
                    TPointTI<TypeD> ()                              { Reset (); }

                                        // constructors that dispatch to each parents' classes
                    TPointTI<TypeD> ( TypeD x, TypeD y, TypeD z, int i = -1 )       : TPointT<TypeD> ( x, y, z ), TIndex ( i )        {}
                    TPointTI<TypeD> ( TPointT<TypeD>&               p, int i = -1 ) : TPointT<TypeD> ( p ), TIndex ( i )              {}
                    TPointTI<TypeD> ( const TPointTI<int>&          p )             : TPointT<TypeD> ( p ), TIndex ( p.Index )        {}
                    TPointTI<TypeD> ( const TPointTI<float>&        p )             : TPointT<TypeD> ( p ), TIndex ( p.Index )        {}
                    TPointTI<TypeD> ( const TPointTI<double>&       p )             : TPointT<TypeD> ( p ), TIndex ( p.Index )        {}
                    TPointTI<TypeD> ( const TPointTI<long double>&  p )             : TPointT<TypeD> ( p ), TIndex ( p.Index )        {}


    void            Reset ()                                        { TPointT<TypeD>::Reset (); TIndex::Reset (); }


//                  TPointTI<TypeD> ( const TPointTI<TypeD>& op  )  : TPointT<TypeD> ( op ), TIndex ( op.Index )    {}  // done explcitly above
    TPointTI<TypeD>& operator   =   ( const TPointTI<TypeD>& op2 )  { X = op2.X; Y = op2.Y; Z = op2.Z; Index = op2.Index; /*TPointT<TypeD>::operator= ( op2 ); TIndex::operator= ( op2 );*/  return *this;   }
};


//----------------------------------------------------------------------------
                                        // Typedefs for common types of points
using       TPointFloat         = TPointT <float>;
using       TPointFloatI        = TPointTI<float>;
using       TPointDouble        = TPointT <double>;
using       TPointDoubleI       = TPointTI<double>;
using       TPointInt           = TPointT <int>;
using       TPointIntI          = TPointTI<int>;

                                        // 3D vector is litterally the same beast as a 3D point
using       TVector3Float       = TPointFloat;
using       TVector3FloatI      = TPointFloatI;
using       TVector3Double      = TPointDouble;
using       TVector3DoubleI     = TPointDoubleI;
using       TVector3Int         = TPointInt;


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // test if equal or very close to an axis, like (1,0,0) or (0,-.9999,0.0001)
template <class TypeD>
bool        TPointT<TypeD>::IsAxisAligned ( double epsilon )  const
{
                          // !ignoring sign!
return  RelativeDifference ( AbsMax (), Norm () ) <= epsilon;
}

                                        // test if the 3 axis are pretty close
template <class TypeD>
bool        TPointT<TypeD>::IsIsotropic ( double epsilon )  const
{
return  RelativeDifference ( fabs ( X ), fabs ( Y ), fabs ( Z ) ) <= epsilon;
}


//----------------------------------------------------------------------------
template <class TypeD>
void        TPointT<TypeD>::Round ()
{
X       = (TypeD) crtl::Round ( X );
Y       = (TypeD) crtl::Round ( Y );
Z       = (TypeD) crtl::Round ( Z );
}


template <class TypeD>
void        TPointT<TypeD>::RoundTo ( double precision )
{
X       = (TypeD) crtl::RoundTo ( X, precision );
Y       = (TypeD) crtl::RoundTo ( Y, precision );
Z       = (TypeD) crtl::RoundTo ( Z, precision );
}


template <class TypeD>
void        TPointT<TypeD>::RoundAbove ()
{
X       = (TypeD) crtl::RoundAbove ( X );
Y       = (TypeD) crtl::RoundAbove ( Y );
Z       = (TypeD) crtl::RoundAbove ( Z );
}


template <class TypeD>
void        TPointT<TypeD>::RoundBelow ()
{
X       = (TypeD) crtl::RoundBelow ( X );
Y       = (TypeD) crtl::RoundBelow ( Y );
Z       = (TypeD) crtl::RoundBelow ( Z );
}


template <class TypeD>
void        TPointT<TypeD>::Truncate ()
{
X       = (TypeD) crtl::Truncate ( X );
Y       = (TypeD) crtl::Truncate ( Y );
Z       = (TypeD) crtl::Truncate ( Z );
}


template <class TypeD>
void        TPointT<TypeD>::Clipped ( TypeD limitmin, TypeD limitmax )
{
::Clipped ( X, limitmin, limitmax );
::Clipped ( Y, limitmin, limitmax );
::Clipped ( Z, limitmin, limitmax );
}


template <class TypeD>
TypeD       TPointT<TypeD>::ScalarProduct ( const TPointT<TypeD> &op2 ) const
{
return  X * op2.X + Y * op2.Y + Z * op2.Z;
}


template <class TypeD>
TypeD       TPointT<TypeD>::Cosine ( const TPointT<TypeD> &op2 )    const
{
return  ScalarProduct ( op2 ) / NonNull ( sqrt ( Norm2 () * op2.Norm2 () ) );
}


template <class TypeD>
TypeD       TPointT<TypeD>::Sine ( const TPointT<TypeD> &op2 )    const
{
return  VectorialProduct ( op2 ).Norm () / NonNull ( sqrt ( Norm2 () * op2.Norm2 () ) );
}


template <class TypeD>
TPointT<TypeD>  TPointT<TypeD>::VectorialProduct ( const TPointT<TypeD> &op2 )  const
{
TPointT<TypeD>      temp;

temp.X  = (TypeD) ( Y * op2.Z - Z * op2.Y );
temp.Y  = (TypeD) ( Z * op2.X - X * op2.Z );
temp.Z  = (TypeD) ( X * op2.Y - Y * op2.X );

return  temp;
}


template <class TypeD>
TPointT<TypeD>  TPointT<TypeD>::ProjectedOn ( const TPointT<TypeD>& dir )   const
{
//if ( dir.IsNull () )
//    return  TPointT ( 0, 0, 0 );
                                        // dividing twice by norm: one for the scalar product, one for the direction
return  dir * ScalarProduct ( dir ) / NonNull ( dir.Norm2 () );
}


template <class TypeD>
TPointT<TypeD>  TPointT<TypeD>::OrthogonalTo ( const TPointT<TypeD>& dir )   const
{
return  *this - ProjectedOn ( dir );
}


template <class TypeD>
void            TPointT<TypeD>::OrthogonalizedTo ( const TPointT<TypeD>& dir )
{
*this  -= ProjectedOn ( dir );
}


template <class TypeD>
void        TPointT<TypeD>::GetOrthogonalVectors ( TPointT<TypeD> &p1, TPointT<TypeD> &p2 )  const
{
                                        // no orthogonal basis for null vector - we assume caller knows about that...
//if ( IsNull () ) {
//    p1.Reset ();
//    p2.Reset ();
//    return;
//    }

                                        // using some arbitrary candidates
//TPointT<TypeD>    d1 ( 1, 0, 0 );
//TPointT<TypeD>    d2 ( 0, 1, 0 );
//TPointT<TypeD>    d3 ( 0, 0, 1 );
                                        // smarter: inferring 3 perpendicular candidates from input
TPointT<TypeD>      d1 ( -Y,  X,  0 );
TPointT<TypeD>      d2 (  0, -Z,  Y );
TPointT<TypeD>      d3 ( -Z,  0,  X );

                                        // get their norms
TypeD               d1n             = d1.Norm ();
TypeD               d2n             = d2.Norm ();
TypeD               d3n             = d3.Norm ();

                                        // normalize vectors
d1     /= d1n;
d2     /= d2n;
d3     /= d3n;

                                        // pick the 2 vectors with the highest norms
if      ( d2n > d1n && d3n > d1n )  {   p1  = d2;   p2  = d3;   }   // d1 is the worst -> keep the others
else if ( d1n > d2n && d3n > d2n )  {   p1  = d1;   p2  = d3;   }   // d2 is the worst
else                                {   p1  = d1;   p2  = d2;   }   // d3 is the worst

                                        // finish the job by making sure p1 and p2 are indeed orthogonals
p2     -= p1 * ( p1.ScalarProduct ( p2 ) );
p2.Normalize ();
}


template <class TypeD>
void        TPointT<TypeD>::ToSpherical ( double& theta, double& phi, double& rho )
{
double              S               = sqrt ( X * X + Y * Y );

rho     = Norm ();
phi     = rho ? acos ( Z / rho ) : 0;
theta   = S ? X >= 0 ? asin ( Y / S ) : M_PI - asin ( Y / S ) : 0;
}


template <class TypeD>
void        TPointT<TypeD>::ToCartesian ( double theta, double phi, double rho )
{
X   = rho * sin ( phi ) * cos ( theta );
Y   = rho * sin ( phi ) * sin ( theta );
Z   = rho * cos ( phi );
}


/*                                        // minus
template <class TypeD>
TPointT TPointT<TypeD>::operator- ( TPointT &op2 )
{
return TPointT<TypeD> ( (TypeD) ( x - op2.x), (TypeD) ( y - op2.y ), (TypeD) ( z - op2.z ) );
}


template <class TypeD>
TypeD   TPointT<TypeD>::operator* ( TPointT &op2 )
{
return  (TypeD) ( x * op2.x + y * op2.y + z * op2.z );
}
*/
/*
template <class TypeD>
bool    TPointT<TypeD>::operator== ( TPointT &op2 )
{
return  x == op2.x && y == op2.y && z == op2.z;
}
*/


template <class TypeD>
void        TPointT<TypeD>::Show ( const char *title )    const
{
char                buff[ 256 ];

StringCopy  ( buff, FloatToString ( X ), ", ", FloatToString ( Y ), ", ", FloatToString ( Z ) );
ShowMessage ( buff, StringIsEmpty ( title ) ? "Point" : title );
}


//------------------------------------------------------------------------------
                                        // Works on both real volumes and masks
                                        // It could be interesting to provide optional inner and outer distance limits, to specify when to give up projecting.
                                        // It could also be used f.ex. to skip projection for any inner points, or outer points, separately.
                                        // Direction could be anything; vector could be non-normalized
template <class TypeD>
bool        TPointT<TypeD>::ResurfacePoint ( const Volume& volume, const TPointT<TypeD>& center, const TPointT<TypeD>& dir, double threshold )
{
if ( dir.IsNull () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPointT             origp ( *this );    // saving original point
TPointT             d     ( dir );
int                 maxiter;
int                 iter;
int                 subvoxel        = 2;
InterpolationType   interpolate     = InterpolateLinear;    // should be enough to detect above or below threshold

//SetOvershootingOption    ( interpolate, volume.GetArray (), volume.GetLinearDim (), true );

                                        // max # of steps - radius of bounding box would be better
maxiter     = subvoxel * volume.MeanSize () / 2;
                                        // rescale direction to sub-voxel precision
d          /= subvoxel * d.Norm ();
                                        // convert point to relative position
*this      += center;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smart way: pick the one, and only one, direction to scan: inward or outward

if ( volume.GetValueChecked ( X, Y, Z, interpolate ) <  threshold ) {
                                        // point is outside: going inward toward center - testing with strict inequality, as to avoid continuing digging below the surface
    for ( iter = 0; volume.GetValueChecked ( X, Y, Z, interpolate ) <  threshold && iter < maxiter; iter++ )

        *this  -= d;
                                        // make point just outside of threshold
    *this  += d;
    }

else {
                                        // point is inside: going outward to the surface
    for ( iter = 0; volume.GetValueChecked ( X, Y, Z, interpolate ) >= threshold && iter < maxiter; iter++ )

        *this  += d;
                                        // point is already outside of threshold
    }

                                        // loops reached the maximum iterations -> problem -> reset point to the original one to be the least inconsistent
if ( iter == maxiter ) {

    *this   = origp;

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Safe: scan both inward AND outward
                                        // going inward - testing with strict inequality, as to avoid continuing digging below the surface
for ( iter = 0; volume.GetValueChecked ( X, Y, Z, interpolate ) <  threshold && iter < maxiter; iter++ )

    *this  -= d;

                                        // loops reached the maximum iterations -> problem -> reset point to the original one to be the least inconsistent
if ( iter == maxiter ) {

    *this   = origp;

    return  false;
    }

                                        // going outward to the surface
for ( iter = 0; volume.GetValueChecked ( X, Y, Z, interpolate ) >= threshold && iter < maxiter; iter++ )

    *this  += d;
                                        // point is already outside of threshold

                                        // loops reached the maximum iterations -> problem -> reset point to the original one to be the least inconsistent
if ( iter == maxiter ) {

    *this   = origp;

    return  false;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally revert back to absolute position
*this  -= center;

return  true;
}

                                        // Simplified version, using current point as the direction
template <class TypeD>
bool        TPointT<TypeD>::ResurfacePoint ( const Volume& volume, const TPointT<TypeD>& center, double threshold )
{
return  ResurfacePoint ( volume, center, *this, threshold );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
