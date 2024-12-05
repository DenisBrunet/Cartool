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

#include    "Geometry.TPoint.h"
#include    "TArray.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD>
class   TArray3 :   public  TArray<TypeD>
{
public:
                    TArray3 () : TArray (), Dim1 ( 0 ), Dim2 ( 0 ), Dim3 ( 0 ) {}
                    TArray3 ( int dim1, int dim2, int dim3 );


    void            DeallocateMemory ()                                     { TArray<TypeD>::DeallocateMemory (); Dim1 = 0; Dim2 = 0; Dim3 = 0; }


    int             GetDim1 ()              const           { return Dim1; }
    int             GetDim2 ()              const           { return Dim2; }
    int             GetDim3 ()              const           { return Dim3; }
    int             GetDim  ( int axis )    const           { return axis == 0 ? Dim1 : axis == 1 ? Dim2 : Dim3; }
    void            GetDims ( int *dims )   const           { dims[ 0 ] = Dim1; dims[ 1 ] = Dim2; dims[ 2 ] = Dim3;  }
                  
    int             MinSize ()              const           { return min ( Dim1, Dim2 , Dim3 ); }
    int             MaxSize ()              const           { return max ( Dim1, Dim2 , Dim3 ); }
    double          MeanSize()              const           { return ( Dim1 + Dim2 + Dim3 ) / 3.0; }

    void            Resize  ( int newdim1, int newdim2, int newdim3 );
    void            Resize  ( const TPointInt& size )       { Resize ( size.X, size.Y, size.Z ); }

    bool            WithinBoundary ( int    x, int    y, int    z ) const   { return   x >= 0 &&   y >= 0 &&   z >= 0 &&   x < Dim1 &&   y < Dim2 &&   z < Dim3; }
    bool            WithinBoundary ( double x, double y, double z ) const   { return   x >= 0 &&   y >= 0 &&   z >= 0 &&   x < Dim1 &&   y < Dim2 &&   z < Dim3; }
    bool            WithinBoundary ( const TPointInt&   p )         const   { return p.X >= 0 && p.Y >= 0 && p.Z >= 0 && p.X < Dim1 && p.Y < Dim2 && p.Z < Dim3; }
    bool            WithinBoundary ( const TPointFloat& p )         const   { return p.X >= 0 && p.Y >= 0 && p.Z >= 0 && p.X < Dim1 && p.Y < Dim2 && p.Z < Dim3; }
    bool            WithinBoundary ( const TPointDouble&p )         const   { return p.X >= 0 && p.Y >= 0 && p.Z >= 0 && p.X < Dim1 && p.Y < Dim2 && p.Z < Dim3; }

    int             XStep ( int numsteps )                          const   { return  numsteps >= Dim1 ? 1 : Round ( (double) Dim1 / numsteps ); }
    int             YStep ( int numsteps )                          const   { return  numsteps >= Dim2 ? 1 : Round ( (double) Dim2 / numsteps ); }
    int             ZStep ( int numsteps )                          const   { return  numsteps >= Dim3 ? 1 : Round ( (double) Dim3 / numsteps ); }
    int             Step  ( int numsteps, bool minimum )            const   { return  minimum ? min ( XStep ( numsteps ), YStep ( numsteps ), ZStep ( numsteps ) ) : max ( XStep ( numsteps ), YStep ( numsteps ), ZStep ( numsteps ) ); }
    TPointInt       Steps ( int numsteps )                          const   { return TPointInt ( XStep ( numsteps ), YStep ( numsteps ), ZStep ( numsteps ) ); }


    using   TArray::GetValue;
    TypeD&          GetValue ( int i1, int i2, int i3 )             const   { return Array[ IndexesToLinearIndex ( i1, i2, i3 ) ]; }
    TypeD           GetValue ( const TPointInt&    p )              const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD           GetValue ( const TPointFloat&  p )              const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD           GetValue ( const TPointDouble& p )              const   { return Array[ IndexesToLinearIndex ( p ) ]; }

    TypeD           GetValueChecked ( int    x, int    y, int    z                                  )   const;
    TypeD           GetValueChecked ( const TPointInt&     p                                        )   const;
    TypeD           GetValueChecked ( const TPointFloat&   p                                        )   const;
    TypeD           GetValueChecked ( const TPointDouble&  p                                        )   const;

                                        // last dimension indexes are consecutive
    int             IndexesToLinearIndex ( int i1, int i2, int i3 )         const   { return (          i1   * Dim2 +        i2 ) * Dim3 +        i3; }
    int             IndexesToLinearIndex ( const TPointInt&    p )          const   { return (         p.X   * Dim2 +       p.Y ) * Dim3 +       p.Z; }
    int             IndexesToLinearIndex ( const TPointFloat&  p )          const   { return ( ( (int) p.X ) * Dim2 + (int) p.Y ) * Dim3 + (int) p.Z; }
    int             IndexesToLinearIndex ( const TPointDouble& p )          const   { return ( ( (int) p.X ) * Dim2 + (int) p.Y ) * Dim3 + (int) p.Z; }
                  
    void            LinearIndexToXYZ     ( int i, int &x, int &y, int &z )  const   { x   = LinearIndexToX ( i ); y   = LinearIndexToY ( i ); z   = LinearIndexToZ ( i );  }
    void            LinearIndexToXYZ     ( int i, TPointFloat&  p        )  const   { p.X = LinearIndexToX ( i ); p.Y = LinearIndexToY ( i ); p.Z = LinearIndexToZ ( i );  }
    void            LinearIndexToXYZ     ( int i, TPointDouble& p        )  const   { p.X = LinearIndexToX ( i ); p.Y = LinearIndexToY ( i ); p.Z = LinearIndexToZ ( i );  }
    int             LinearIndexToX       ( int i )                          const   { return ( i / Dim3 ) / Dim2; }
    int             LinearIndexToY       ( int i )                          const   { return ( i / Dim3 ) % Dim2; }
    int             LinearIndexToZ       ( int i )                          const   { return   i % Dim3; }


    TypeD           GetMaxValue         ()                                                                  const;
    TypeD           GetMinValue         ()                                                                  const;
    TypeD           GetAbsMaxValue      ()                                                                  const;
    void            GetMinMaxValues     ( double& minvalue, double& maxvalue )                              const;
    void            GetMinMaxValues     ( double& minvalue, double& maxvalue, TPointFloat& minpos, TPointFloat& maxpos )  const;
    void            GetMaxPosition      ( float *pos )                                                      const;
    void            GetMinPosition      ( float *pos )                                                      const;


    void            DownsamplingOffset  ( int downsampling, int &shiftx, int &shifty, int &shiftz );

                                        // insert from another volume, usually smaller or equal size, given a downsampling and new origin
    void            Insert              ( TArray3<TypeD> &fromarray, int *origin = 0, double intensityrescale = 1, int downsampling = 1 );
    void            InsertToX           ( int tox,   TArray2<TypeD> &fromarray );
    void            InsertToZ           ( int toz,   TArray2<TypeD> &fromarray );
    void            ExtractFromX        ( int fromx, TArray2<TypeD> &toarray   )        const;
    void            ExtractFromZ        ( int fromz, TArray2<TypeD> &toarray   )        const;
                                        // crop from another bigger volume
    void            Crop                ( const TArray3<TypeD>& fromarray, const int* origin );


                    TArray3             ( const TArray3&        op  );
    TArray3<TypeD>& operator    =       ( const TArray3<TypeD>& op2 );


    using   TArray::operator    =;


    using   TArray::operator    [];                         // access O[ i ], linear space
    TypeD&          operator    []      ( const TPointInt&    p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD&          operator    []      ( const TPointFloat&  p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD&          operator    []      ( const TPointDouble& p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }


    using   TArray::operator    ();                         // access O( i ), linear space
    TypeD&          operator    ()      ( int i1, int i2, int i3 )  const   { return Array[ IndexesToLinearIndex ( i1, i2, i3 ) ]; }
    TypeD&          operator    ()      ( const float*  p        )  const   { return Array[ IndexesToLinearIndex ( p[ 0 ], p[ 1 ], p[ 2 ] ) ]; }
    TypeD&          operator    ()      ( const double* p        )  const   { return Array[ IndexesToLinearIndex ( p[ 0 ], p[ 1 ], p[ 2 ] ) ]; }
    TypeD&          operator    ()      ( const TPointInt&    p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD&          operator    ()      ( const TPointFloat&  p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }
    TypeD&          operator    ()      ( const TPointDouble& p  )  const   { return Array[ IndexesToLinearIndex ( p ) ]; }


protected:
    int             Dim1;
    int             Dim2;
    int             Dim3;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // Redefinitions - until we sort out all includes:
#define         UpdateApplication       { if ( IsMainThread () ) CartoolObjects.CartoolApplication->PumpWaitingMessages (); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
        TArray3<TypeD>::TArray3 ( const TArray3 &op )
{
Dim1            = op.Dim1;
Dim2            = op.Dim2;
Dim3            = op.Dim3;
LinearDim       = op.LinearDim;

Array           = (TypeD *) AllocateMemory ( MemorySize () );

CopyMemoryFrom ( op.Array );
}


template <class TypeD>
TArray3<TypeD>& TArray3<TypeD>::operator= ( const TArray3<TypeD> &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Resize ( op2.Dim1, op2.Dim2, op2.Dim3 );

CopyMemoryFrom ( op2.Array );


return  *this;
}

                                        // resize the array, previous data is lost - used mainly to avoid creation + assignment
template <class TypeD>
void            TArray3<TypeD>::Resize ( int newdim1, int newdim2, int newdim3 )
{
                                        // no changes?
if ( newdim1 == Dim1 && newdim2 == Dim2 && newdim3 == Dim3 ) {
//  ResetMemory ();
    return;
    }

                                        // clean up existing memory
DeallocateMemory ();

Dim1            = newdim1;
Dim2            = newdim2;
Dim3            = newdim3;
LinearDim       = Dim1 * Dim2 * Dim3;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TArray3<TypeD>::TArray3 ( int dim1, int dim2, int dim3 )
{
Dim1            = dim1;
Dim2            = dim2;
Dim3            = dim3;
LinearDim       = Dim1 * Dim2 * Dim3;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
                                        // to have a correct non-integer downsampling insertion,
                                        // provide 2 MORE VOXELS on each required dimensions for the receiving array (this)
                                        // though the routine will not crash if it is not the case
template <class TypeD>
void    TArray3<TypeD>::Insert ( TArray3<TypeD> &fromarray, int *origin, double intensityrescale, int downsampling )
{
                                        // No optional parameters - just plug the data, clipping it if necessary
if ( ! origin ) {

    int                 x,  y;
    int                 indim1          = min ( Dim1, fromarray.GetDim1 () );
    int                 indim2          = min ( Dim2, fromarray.GetDim2 () );
    int                 indim3          = min ( Dim3, fromarray.GetDim3 () );
    int                 size            = indim3 * AtomSize ();
    int                 inindex;
    int                 outindex;

                                        // actually the same array
    if ( indim1 == Dim1 && indim2 == Dim2 && indim3 == Dim3 && AtomSize () == fromarray.AtomSize () )

        CopyVirtualMemory ( Array, fromarray.GetArray (), MemorySize () );

    else                                // really need to insert
                                        // scan within safe limits
        for ( x = 0; x < indim1; x++ )
        for ( y = 0; y < indim2; y++ ) {
                                            // get index of first Z
            inindex  = fromarray.IndexesToLinearIndex ( x, y, 0 );
//          outindex =           IndexesToLinearIndex ( x + origin[0], y + origin[1], origin[2] );
            outindex =           IndexesToLinearIndex ( x, y, 0 );

                                        // directly copy a line of Z
            CopyVirtualMemory ( &Array[ outindex ], &fromarray[ inindex ], size );

//          for ( z=0; z < indim3; z++ )
//              (*this) ( x, y, z )  = fromarray ( x, y, z ) * intensityrescale;
            }
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( downsampling <= 1 ) {         // Shifting, no downsampling, optional rescaling

    int                 x,  y, z;
    int                 indim1          = min ( max ( 0, Dim1 - origin[ 0 ] ), fromarray.GetDim1 () );
    int                 indim2          = min ( max ( 0, Dim2 - origin[ 1 ] ), fromarray.GetDim2 () );
    int                 indim3          = min ( max ( 0, Dim3 - origin[ 2 ] ), fromarray.GetDim3 () );

                                        // scan within safe limits
    if ( intensityrescale == 1 )
        
        for ( x = 0; x < indim1; x++ )
        for ( y = 0; y < indim2; y++ )
        for ( z = 0; z < indim3; z++ )

            (*this) ( x + origin[ 0 ], y + origin[ 1 ], z + origin[ 2 ] ) = fromarray ( x, y, z );
    else
        for ( x = 0; x < indim1; x++ )
        for ( y = 0; y < indim2; y++ )
        for ( z = 0; z < indim3; z++ )

            (*this) ( x + origin[ 0 ], y + origin[ 1 ], z + origin[ 2 ] ) = (TypeD) ( fromarray ( x, y, z ) * intensityrescale );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else {                                  // Shifting, downsampling & rescaling

    int                 x,  y,  z;
    int                 xr, yr, zr;
    int                 xx, yy, zz;
                                        // case where fromarray is bigger not handled
    int                 indim1          = fromarray.GetDim1 ();
    int                 indim2          = fromarray.GetDim2 ();
    int                 indim3          = fromarray.GetDim3 ();
    int                 d3              = Cube ( downsampling );
    double              sum;            // use a double - an int could be find up to downsampling 32 times, but no more

    int                 firstinx;
    int                 firstiny;
    int                 firstinz;
    fromarray.DownsamplingOffset ( downsampling, firstinx, firstiny, firstinz );

                                        // downsample and copy
    for ( x = origin[ 0 ], xr = firstinx; x < Dim1 && xr < indim1; x++, xr += downsampling )
    for ( y = origin[ 1 ], yr = firstiny; y < Dim2 && yr < indim2; y++, yr += downsampling )
    for ( z = origin[ 2 ], zr = firstinz; z < Dim3 && zr < indim3; z++, zr += downsampling ) {

        sum = 0;
                                // average of cube
        for ( xx = xr; xx < xr + downsampling && xx < indim1; xx++ )
        for ( yy = yr; yy < yr + downsampling && yy < indim2; yy++ )
        for ( zz = zr; zz < zr + downsampling && zz < indim3; zz++ )

            if ( ! ( xx < 0 || yy < 0 || zz < 0 ) )

                sum    += fromarray ( xx, yy, zz );

        (*this) ( x, y, z ) = (TypeD) ( sum * intensityrescale / d3 );
        }
    }
}


//----------------------------------------------------------------------------
                                        // Plug a TArray2 into a TArray3
template <class TypeD>
void    TArray3<TypeD>::InsertToX ( int tox, TArray2<TypeD> &fromarray )
{
int                 indim2          = min ( Dim2, fromarray.GetDim1 () );
int                 indim3          = min ( Dim3, fromarray.GetDim2 () );

                                    // compatible array?
if ( indim2 == Dim2 && indim3 == Dim3 && AtomSize () == fromarray.AtomSize () )

    CopyVirtualMemory ( Array + tox * ( Dim2 * Dim3 ), fromarray.GetArray (), fromarray.MemorySize () );

else                                // manual insertion

    for ( int y = 0; y < indim2; y++ )
    for ( int z = 0; z < indim3; z++ )

        GetValue ( tox, y, z )  = fromarray ( y, z );
}

                                        // Plug a TArray2 into a TArray3
template <class TypeD>
void    TArray3<TypeD>::InsertToZ ( int toz, TArray2<TypeD> &fromarray )
{
int                 indim1          = min ( Dim1, fromarray.GetDim1 () );
int                 indim2          = min ( Dim2, fromarray.GetDim2 () );

                                    // manual insertion
for ( int x = 0; x < indim1; x++ )
for ( int y = 0; y < indim2; y++ )

    GetValue ( x, y, toz )  = fromarray ( x, y );
}

                                        // Extract a slice of TArray3 to a TArray2
template <class TypeD>
void    TArray3<TypeD>::ExtractFromX ( int fromx, TArray2<TypeD> &toarray )     const
{
int                 indim2          = min ( Dim2, toarray.GetDim1 () );
int                 indim3          = min ( Dim3, toarray.GetDim2 () );

                                    // compatible array?
if ( indim2 == Dim2 && indim3 == Dim3 && AtomSize () == toarray.AtomSize () )

    CopyVirtualMemory ( toarray.GetArray (), Array + fromx * ( Dim2 * Dim3 ), toarray.MemorySize () );

else                                // manual insertion

    for ( int y = 0; y < indim2; y++ )
    for ( int z = 0; z < indim3; z++ )

        toarray ( y, z )    = GetValue ( fromx, y, z );
}


                                        // Extract a slice of TArray3 to a TArray2
template <class TypeD>
void    TArray3<TypeD>::ExtractFromZ ( int fromz, TArray2<TypeD> &toarray )     const
{
int                 indim1          = min ( Dim1, toarray.GetDim1 () );
int                 indim2          = min ( Dim2, toarray.GetDim2 () );

                                    // manual insertion
for ( int x = 0; x < indim1; x++ )
for ( int y = 0; y < indim2; y++ )

    toarray ( x, y )    = GetValue ( x, y, fromz );
}


//----------------------------------------------------------------------------
                                        // Crop a bigger one into (this) smaller one
template <class TypeD>
void    TArray3<TypeD>::Crop ( const TArray3<TypeD>& fromarray, const int* origin )
{
int                 indim1          = Clip ( fromarray.GetDim1 () - origin[ 0 ], 0, Dim1 );
int                 indim2          = Clip ( fromarray.GetDim2 () - origin[ 1 ], 0, Dim2 );
int                 indim3          = Clip ( fromarray.GetDim3 () - origin[ 2 ], 0, Dim3 );

                                    // scan within safe limits
for ( int x = 0, xo = x + origin[ 0 ]; x < indim1; x++, xo++ )
for ( int y = 0, yo = y + origin[ 1 ]; y < indim2; y++, yo++ )
for ( int z = 0, zo = z + origin[ 2 ]; z < indim3; z++, zo++ )

    GetValue ( x, y, z )    = fromarray ( xo, yo, zo );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TArray3<TypeD>::GetMaxPosition ( float *pos )   const
{
pos[ 0 ]    =
pos[ 1 ]    =
pos[ 2 ]    = 0;

TypeD               maxvalue        = Lowest ( maxvalue );

for ( int i = 0; i < LinearDim; i++ )

    if ( Array[ i ] >= maxvalue ) {

        maxvalue    = Array[ i ];

        pos[ 0 ]    = LinearIndexToX ( i );
        pos[ 1 ]    = LinearIndexToY ( i );
        pos[ 2 ]    = LinearIndexToZ ( i );
        }
}


template <class TypeD>
void    TArray3<TypeD>::GetMinPosition ( float *pos )   const
{
pos[ 0 ]    =
pos[ 1 ]    =
pos[ 2 ]    = 0;

TypeD               minvalue        = Highest ( minvalue );

for ( int i = 0; i < LinearDim; i++ )

    if ( Array[ i ] <= minvalue ) {

        minvalue    = Array[ i ];

        pos[ 0 ]    = LinearIndexToX ( i );
        pos[ 1 ]    = LinearIndexToY ( i );
        pos[ 2 ]    = LinearIndexToZ ( i );
        }
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TArray3<TypeD>::GetMaxValue ()  const
{
TypeD               maxvalue        = Lowest  ( maxvalue );

for ( int i = 0; i < LinearDim; i++ )
    Maxed ( maxvalue, Array[ i ] );

return  maxvalue;
}


template <class TypeD>
TypeD   TArray3<TypeD>::GetMinValue ()  const
{
TypeD               minvalue        = Highest ( minvalue );

for ( int i = 0; i < LinearDim; i++ )
    Mined ( minvalue, Array[ i ] );

return  minvalue;
}


template <class TypeD>
TypeD   TArray3<TypeD>::GetAbsMaxValue ()  const
{
double              minvalue;
double              maxvalue;

GetMinMaxValues ( minvalue, maxvalue );

return  (TypeD) max ( abs ( minvalue ), abs ( maxvalue ) ); 
}


template <class TypeD>
void    TArray3<TypeD>::GetMinMaxValues     (   double&     minvalue,   double&     maxvalue    )   const
{
minvalue    = Highest ( minvalue );
maxvalue    = Lowest  ( maxvalue );

for ( int i = 0; i < LinearDim; i++ ) {

    Mined ( minvalue, (double) Array[ i ] );
    Maxed ( maxvalue, (double) Array[ i ] );
    }
}


template <class TypeD>
void    TArray3<TypeD>::GetMinMaxValues     (   double&         minvalue,   double&         maxvalue, 
                                                TPointFloat&    minpos,     TPointFloat&    maxpos
                                            )   const
{
minvalue    = Highest ( minvalue );
maxvalue    = Lowest  ( maxvalue );

minpos.Reset ();
maxpos.Reset ();


for ( int i = 0; i < LinearDim; i++ ) {

    if ( Array[ i ] < minvalue ) {
        minvalue    = Array[ i ];
        LinearIndexToXYZ ( i, minpos );
        }

    if ( Array[ i ] > maxvalue ) {
        maxvalue    = Array[ i ];
        LinearIndexToXYZ ( i, maxpos );
        }
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TArray3<TypeD>::GetValueChecked ( int x, int y, int z )     const
{
return  WithinBoundary ( x, y, z ) ? GetValue ( x, y, z ) : (TypeD) 0;
}


template <class TypeD>
TypeD   TArray3<TypeD>::GetValueChecked ( const TPointInt& p )      const
{
return  WithinBoundary ( p ) ? GetValue ( p ) : (TypeD) 0;
}


template <class TypeD>
TypeD   TArray3<TypeD>::GetValueChecked ( const TPointFloat& p )    const
{
return  WithinBoundary ( p ) ? GetValue ( p ) : (TypeD) 0;
}


template <class TypeD>
TypeD   TArray3<TypeD>::GetValueChecked ( const TPointDouble& p )   const
{
return  WithinBoundary ( p ) ? GetValue ( p ) : (TypeD) 0;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TArray3<TypeD>::DownsamplingOffset ( int downsampling, int &shiftx, int &shifty, int &shiftz )
{
                                        // tricky offset: when downsampling with a non-integer ratio,
                                        // split in 2 the shift error that will normally appear when reaching the max borders
                                        // and spread this shift on both sides. There is still a round off of 1 original voxel error that can't be removed
                                        // Without this, the max borders sides will have lower values, therefore giving a strange, hollow shape,
                                        // plus a shift toward the 0 border.
shiftx = - ( ( downsampling - ( Dim1 / downsampling ) * downsampling ) % downsampling ) / 2;
shifty = - ( ( downsampling - ( Dim2 / downsampling ) * downsampling ) % downsampling ) / 2;
shiftz = - ( ( downsampling - ( Dim3 / downsampling ) * downsampling ) % downsampling ) / 2;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
