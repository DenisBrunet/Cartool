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
                                        // Storage only array
template <class TypeD>
class   TArray1 :   public  TArray<TypeD>
{
public:
                    TArray1 () : TArray (), Dim1 ( 0 )                      {}
                    TArray1 ( int dim1 );


    void            DeallocateMemory ()                                     { TArray<TypeD>::DeallocateMemory (); Dim1 = 0; }

    int             GetDim  ()                  const       { return Dim1; }
    int             GetDim1 ()                  const       { return Dim1; }
    int             MaxSize ()                  const       { return Dim1; }
    bool            WithinBoundary ( int x )    const       { return x >= 0 && x < Dim1; }

    void            Resize              ( int newdim1, MemoryAllocationType how = MemoryDefault );
    void            ResizeDelta         ( int delta,   MemoryAllocationType how = ResizeKeepMemory );


    void            Insert              ( TArray1<TypeD>& fromarray, int* origin = 0 /*, double intensityrescale = 1*/ );
    void            Insert              ( TArray1<TypeD>& fromarray, int offset );


                    TArray1<TypeD>      ( const TArray1<TypeD>& op  );
    TArray1<TypeD>& operator    =       ( const TArray1<TypeD>& op2 );

    using   TArray::operator    =;
    
    using   TArray::operator    [];
    TypeD&          operator    []      ( double v )        { return GetValue ( v ); }  //!converts value to index!

    using   TArray::operator    ();
    TypeD&          operator    ()      ( double v )        { return GetValue ( v ); }  //!converts value to index!


protected:
    int             Dim1;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TArray1<TypeD>::TArray1 ( const TArray1<TypeD>& op )
{
LinearDim       = op.LinearDim;
Dim1            = op.Dim1;
Array           = (TypeD *) AllocateMemory ( MemorySize () );

CopyMemoryFrom ( op.Array );
}


template <class TypeD>
TArray1<TypeD>& TArray1<TypeD>::operator= ( const TArray1<TypeD>& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

                                        // to avoid calling Resize for TMaps[ .. ] = TMap, as TMaps has done its own allocation and MemoryBlock / MemorySize are 0
if ( Dim1 != op2.Dim1 )
                                        // no need to reset memory, as we will copy from op2 right after this
    Resize ( op2.Dim1, (MemoryAllocationType) ( MemoryAuto | ResizeNoReset ) );


//CopyMemoryFrom ( op2.Array );
                                        // same as above, without going through TMemory which is not used for TMaps
CopyVirtualMemory ( Array, op2.Array, Dim1 * sizeof ( TypeD ) );

return  *this;
}


//----------------------------------------------------------------------------
                                        // resize the array, can optionally keep the old data, or if same size and requested so, reset the array
                                        // !Always call ResizeMemory which will handle the existing memory size, and clear / copy the memory appropriately!
template <class TypeD>
void            TArray1<TypeD>::Resize ( int newdim1, MemoryAllocationType how )
{
Dim1            = newdim1;
LinearDim       = Dim1;

                                        // force auto memory, only resize type left for caller
Array           = (TypeD *) ResizeMemory ( MemorySize (), SetMemoryType ( how, MemoryAuto ) );
}

                                        // Well, delta could be negative for shrinking...
template <class TypeD>
void            TArray1<TypeD>::ResizeDelta ( int delta, MemoryAllocationType how )
{
Resize ( AtLeast ( 0, Dim1 + delta ), how );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TArray1<TypeD>::TArray1 ( int dim1 )
{
Dim1            = dim1;
LinearDim       = Dim1;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TArray1<TypeD>::Insert ( TArray1<TypeD>& fromarray, int* origin /*, double intensityrescale*/ )
{
if ( ! origin ) {                       // this simply plugs in another array, clipping it if necessary
    int                 indim1          = min ( Dim1, fromarray.GetDim1 () );
    size_t              size            = indim1 * AtomSize ();

                                        // copy the minimum size
    CopyVirtualMemory ( Array, fromarray.GetArray (), min ( size, MemorySize () ) );
    }

else {
    int                 indim1          = min ( AtLeast ( 0, Dim1 - origin[ 0 ] ), fromarray.GetDim1 () );
    size_t              size            = indim1 * AtomSize ();

    CopyVirtualMemory ( &Array[ origin[ 0 ] ], fromarray.GetArray (), size );

                                        // scan within safe limits
//    for ( int x = 0; x < indim1; x++ )
//        (*this) ( x + origin[ 0 ] ) = fromarray ( x ) /** intensityrescale*/;
    }
}


//----------------------------------------------------------------------------
                                        // !arrays must be different, and of the same length!
template <class TypeD>
void    TArray1<TypeD>::Insert ( TArray1<TypeD>& fromarray, int offset )
{
                                        // slow and safe copy(?)
//for ( int i = 0, j = offset; i < Dim1; i++, j = ++j % Dim1 )
//    
//    Array[ i ]  = fromarray[ j ];


                                        // faster copy(?)
CopyVirtualMemory     (  Array                 , &fromarray[ offset ],   ( Dim1 - offset ) * AtomSize () );

if ( offset > 0 )                       // if real offset, copy remaining part

    CopyVirtualMemory ( &Array[ Dim1 - offset ],  fromarray.GetArray (),          offset   * AtomSize () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
