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

#include    "MemUtil.h"
#include    "TArray.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD>
class   TArray4 :   public  TArray<TypeD>
{
public:
                    TArray4 () : TArray (), Dim1 ( 0 ), Dim2 ( 0 ), Dim3 ( 0 ), Dim4 ( 0 ) {}
                    TArray4 ( int dim1, int dim2, int dim3, int dim4 );


    void            DeallocateMemory ()                                     { TArray<TypeD>::DeallocateMemory (); Dim1 = 0; Dim2 = 0; Dim3 = 0; Dim4 = 0; }


    int             GetDim1 ()                      const   { return Dim1; }
    int             GetDim2 ()                      const   { return Dim2; }
    int             GetDim3 ()                      const   { return Dim3; }
    int             GetDim4 ()                      const   { return Dim4; }
    int             GetDim  ( int axis )            const   { return axis == 0 ? Dim1 : axis == 1 ? Dim2 : axis == 2 ? Dim3 : Dim4; }
    void            GetDims ( int *dims )           const   { dims[ 0 ] = Dim1; dims[ 1 ] = Dim2; dims[ 2 ] = Dim3; dims[ 3 ] = Dim4;  }

    void            Resize  ( int newdim1, int newdim2, int newdim3, int newdim4 );


    using   TArray::GetValue;
    TypeD&          GetValue ( int i1, int i2, int i3, int i4 )                     { return Array[ IndexesToLinearIndex ( i1, i2, i3, i4 ) ]; }

                                 // row-major - last dimension indexes are consecutive
    int             IndexesToLinearIndex ( int i1, int i2, int i3, int i4 ) const   { return ( ( i1 * Dim2 + i2 ) * Dim3 + i3 ) * Dim4 + i4; }


                    TArray4             ( const TArray4&        op );
    TArray4<TypeD>& operator    =       ( const TArray4<TypeD>& op2 );

    using   TArray::operator    =;

    using   TArray::operator    [];                         // access O[ i ], linear space

    using   TArray::operator    ();                         // access O( i ), linear space
    TypeD&          operator    ()      ( int i1, int i2, int i3, int i4 )          { return Array[ IndexesToLinearIndex ( i1, i2, i3, i4 ) ]; }
    TypeD&          operator    ()      ( int i1, int i2, int i3, int i4 )  const   { return Array[ IndexesToLinearIndex ( i1, i2, i3, i4 ) ]; }


protected:
    int             Dim1;
    int             Dim2;
    int             Dim3;
    int             Dim4;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TArray4<TypeD>::TArray4 ( const TArray4& op )
{
Dim1            = op.Dim1;
Dim2            = op.Dim2;
Dim3            = op.Dim3;
Dim4            = op.Dim4;
LinearDim       = op.LinearDim;

Array           = (TypeD *) AllocateMemory ( MemorySize () );

CopyMemoryFrom ( op.Array );
}


template <class TypeD>
TArray4<TypeD>& TArray4<TypeD>::operator= ( const TArray4<TypeD>& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Resize ( op2.Dim1, op2.Dim2, op2.Dim3, op2.Dim4 );

CopyMemoryFrom ( op2.Array );

return  *this;
}

                                        // resize the array, previous data is lost - used mainly to avoid creation + assignment
template <class TypeD>
void            TArray4<TypeD>::Resize ( int newdim1, int newdim2, int newdim3, int newdim4 )
{
                                        // no changes?
if ( newdim1 == Dim1 && newdim2 == Dim2 && newdim3 == Dim3 && newdim4 == Dim4 ) {
//  ResetMemory ();
    return;
    }

                                        // clean up existing memory
DeallocateMemory ();

Dim1            = newdim1;
Dim2            = newdim2;
Dim3            = newdim3;
Dim4            = newdim4;
LinearDim       = Dim1 * Dim2 * Dim3 * Dim4;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TArray4<TypeD>::TArray4 ( int dim1, int dim2, int dim3, int dim4 )
{
Dim1            = dim1;
Dim2            = dim2;
Dim3            = dim3;
Dim4            = dim4;
LinearDim       = Dim1 * Dim2 * Dim3 * Dim4;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
