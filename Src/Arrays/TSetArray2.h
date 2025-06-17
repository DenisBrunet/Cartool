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

#include    "TArray2.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // derive to have 2.5D object, a set of planes
                                        // but it should behave nearly as a 2D object most of the time
template <class TypeD>
class   TSetArray2      : public TArray2<TypeD>
{
public:
                    TSetArray2 ();
                    TSetArray2 ( int numplanes, int dim1, int dim2 );


    size_t          TotalMemorySize ()      const           { return MaxPlanes * MemorySize (); }

    void            DeallocateMemory ();


    int             GetTotalLinearDim ()    const           { return MaxPlanes * LinearDim; }   // LinearDim is only a single plane long
    int             GetMaxPlanes ()         const           { return MaxPlanes; }
    int             GetCurrentPlane ()      const           { return CurrentPlane; }
    void            SetCurrentPlane ( int p );      // !NOT thread safe!

    void            Resize  ( int newnumplanes, int newdim1, int newdim2 );


                    TSetArray2              ( const TSetArray2&         op  );
    TSetArray2<TypeD>&  operator    =       ( const TSetArray2<TypeD>&  op2 );
    TSetArray2<TypeD>&  operator    =       ( const TArray2<TypeD>&     op2 );

    using      TArray2::operator    [];

    using      TArray2::operator    ();
    TypeD&              operator    ()      ( int i1, int i2, int i3 )          { return ArrayOrigin[ ( i1 * Dim1 + i2 ) * Dim2 + i3 ]; }
    TypeD&              operator    ()      ( int i1, int i2, int i3 )  const   { return ArrayOrigin[ ( i1 * Dim1 + i2 ) * Dim2 + i3 ]; }


    TSetArray2<TypeD>&  operator    +=      ( TSetArray2<TypeD> &op2 );
    TSetArray2<TypeD>&  operator    *=      ( TSetArray2<TypeD> &op2 );
    TSetArray2<TypeD>&  operator    /=      ( double op2 );
    TSetArray2<TypeD>   operator    -       ( TSetArray2<TypeD> &op2 );
    TSetArray2<TypeD>   operator    *       ( TSetArray2<TypeD> &op2 );
    TSetArray2<TypeD>   operator    /       ( double op2 );


protected:
    int             MaxPlanes;          // "third" dimension
    int             CurrentPlane;
    TypeD*          ArrayOrigin;        // original origin of the series of slices

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
        TSetArray2<TypeD>::TSetArray2 ()
{
CurrentPlane    = 0;
MaxPlanes       = 0;
ArrayOrigin     = 0;
}


template <class TypeD>
        TSetArray2<TypeD>::TSetArray2 ( int numplanes, int dim1, int dim2 )
{
Dim1            = dim1;
Dim2            = dim2;
LinearDim       = Dim1 * Dim2;
CurrentPlane    = 0;
MaxPlanes       = numplanes ? numplanes : 1;

ArrayOrigin     = (TypeD *) AllocateMemory ( TotalMemorySize () );
Array           = ArrayOrigin;
}


template <class TypeD>
void    TSetArray2<TypeD>::DeallocateMemory ()
{
                                       // re-position to first byte
SetCurrentPlane ( 0 );

TArray2<TypeD>::DeallocateMemory ();

MaxPlanes       = 0;
ArrayOrigin     = 0;
}


template <class TypeD>
        TSetArray2<TypeD>::TSetArray2 ( const TSetArray2& op )
{
Dim1            = op.Dim1;
Dim2            = op.Dim2;
LinearDim       = op.LinearDim;
CurrentPlane    = op.CurrentPlane;
MaxPlanes       = op.MaxPlanes;

ArrayOrigin     = (TypeD *) AllocateMemory ( TotalMemorySize () );
Array           = ArrayOrigin;

CopyMemoryFrom ( op.ArrayOrigin );
}


template <class TypeD>
TSetArray2<TypeD>& TSetArray2<TypeD>::operator= ( const TSetArray2<TypeD>& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Resize ( op2.MaxPlanes, op2.Dim1, op2.Dim2 );

CopyMemoryFrom ( op2.ArrayOrigin );
                                        // this was not done in Resize, so do it here
CurrentPlane    = op2.CurrentPlane;


return  *this;
}

                                        // copying a TArray2 into current plane
template <class TypeD>
TSetArray2<TypeD>& TSetArray2<TypeD>::operator= ( const TArray2<TypeD>& op2 )
{

if ( Dim1 != op2.GetDim1 () || Dim2 != op2.GetDim2 () )
    return  *this;


Insert ( op2 );
//CopyMemoryFrom ( op2.Array );

return  *this;
}

                                        // resize the array, previous data is lost - used mainly to avoid creation + assignment
template <class TypeD>
void            TSetArray2<TypeD>::Resize ( int newnumplanes, int newdim1, int newdim2 )
{
                                        // no changes?
if ( newnumplanes == MaxPlanes && newdim1 == Dim1 && newdim2 == Dim2 ) {
//  ResetMemory ();
    return;
    }

                                        // clean up existing memory
DeallocateMemory ();


Dim1            = newdim1;
Dim2            = newdim2;
LinearDim       = Dim1 * Dim2;
CurrentPlane    = 0;
MaxPlanes       = AtLeast ( 1, newnumplanes );

ArrayOrigin     = (TypeD *) AllocateMemory ( TotalMemorySize () );
Array           = ArrayOrigin;
}

/*
template <class TypeD>
void            TSetArray2<TypeD>::Resize ( int newdim1, int newdim2 )
{
                                        // no changes (ignoring the number of planes)?
if ( newdim1 == Dim1 && newdim2 == Dim2 ) {
//  ResetMemory ();
    return;
    }

                                        // clean up existing memory
DeallocateMemory ();

Dim1            = newdim1;
Dim2            = newdim2;
LinearDim       = Dim1 * Dim2;
//CurrentPlane    = 0;
MaxPlanes       = AtLeast ( 1, MaxPlanes );

Array           = (TypeD *) AllocateMemory ( TotalMemorySize () );
}
*/

//----------------------------------------------------------------------------
template <class TypeD>
TSetArray2<TypeD>& TSetArray2<TypeD>::operator+= ( TSetArray2<TypeD> &op2 )
{
if ( GetTotalLinearDim () == op2.GetTotalLinearDim () )       // same size -> simplified loop

    OmpParallelFor

    for ( int i = 0; i < GetTotalLinearDim (); i++ )
        Array[ i ] += op2.Array[ i ];
else {
    int     mindimp = min ( MaxPlanes, op2.MaxPlanes );
    int     mindim1 = min ( Dim1,      op2.Dim1      );
    int     mindim2 = min ( Dim2,      op2.Dim2      );

    OmpParallelFor

    for ( int dp = 0; dp < mindimp; dp++ )
    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = ( dp * Dim1 + d1 ) * Dim2 + d2; d2 < mindim2; d2++, i++ )
        Array[ i ] += op2 ( d1, d2 );
    }

return  *this;
}


template <class TypeD>
TSetArray2<TypeD>& TSetArray2<TypeD>::operator*= ( TSetArray2<TypeD> &op2 )
{
if ( GetTotalLinearDim () == op2.GetTotalLinearDim () )       // same size -> simplified loop

    OmpParallelFor

    for ( int i = 0; i < GetTotalLinearDim (); i++ )
        Array[ i ] *= op2.Array[ i ];
else {
    int     mindimp = min ( MaxPlanes, op2.MaxPlanes );
    int     mindim1 = min ( Dim1,      op2.Dim1      );
    int     mindim2 = min ( Dim2,      op2.Dim2      );

    OmpParallelFor

    for ( int dp = 0; dp < mindimp; dp++ )
    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0, i = ( dp * Dim1 + d1 ) * Dim2 + d2; d2 < mindim2; d2++, i++ )
        Array[ i ] *= op2 ( d1, d2 );
    }

return  *this;
}


template <class TypeD>
TSetArray2<TypeD>& TSetArray2<TypeD>::operator/= ( double op2 )
{
if ( op2 == 0 )
    return *this;

OmpParallelFor

for ( int i = 0; i < GetTotalLinearDim (); i++ )
    Array[ i ] /= op2;

return  *this;
}


template <class TypeD>
TSetArray2<TypeD>  TSetArray2<TypeD>::operator- ( TSetArray2<TypeD> &op2 )
{
TSetArray2<TypeD>   temp ( MaxPlanes, Dim1, Dim2 );

OmpParallelFor

for ( int i=0; i < GetTotalLinearDim (); i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] - op2.Array[ i ] );

return  temp;
}


template <class TypeD>
TSetArray2<TypeD>  TSetArray2<TypeD>::operator* ( TSetArray2<TypeD> &op2 )
{
TSetArray2<TypeD>   temp ( MaxPlanes, Dim1, Dim2 );

OmpParallelFor

for ( int i=0; i < GetTotalLinearDim (); i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] * op2.Array[ i ] );

return  temp;
}


template <class TypeD>
TSetArray2<TypeD>  TSetArray2<TypeD>::operator/ ( double op2 )
{
TSetArray2<TypeD>   temp ( MaxPlanes, Dim1, Dim2 );

OmpParallelFor

for ( int i=0; i < GetTotalLinearDim (); i++ )
    temp.Array[ i ] = (TypeD) ( Array[ i ] / op2 );

return  temp;
}


//----------------------------------------------------------------------------
template <class TypeD>
void        TSetArray2<TypeD>::SetCurrentPlane ( int p )
{                                       // check and leave if incorrect
if ( ! ( IsAllocated () && IsInsideLimits ( p, 0, MaxPlanes - 1 ) && p != CurrentPlane ) )
    return;
                                        // set new current plane
CurrentPlane = p;
                                        // shift the array pointer
Array        = ArrayOrigin + CurrentPlane * LinearDim;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
