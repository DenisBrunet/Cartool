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
#include    "TArray.h"
#include    "TArray1.h"
#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Multi-dimensional array
template <class TypeD>
class   TMultiArray :   public  TArray<TypeD>
{
public:
                    TMultiArray () : TArray (), NumDims ( 0 ) {}
                    TMultiArray ( const TArray1<int>& dims )                                    { Resize ( dims ); }
                    TMultiArray ( int dim1 )                                                    { Resize ( dim1 ); }
                    TMultiArray ( int dim1, int dim2 )                                          { Resize ( dim1, dim2 ); }
                    TMultiArray ( int dim1, int dim2, int dim3 )                                { Resize ( dim1, dim2, dim3 ); }
                    TMultiArray ( int dim1, int dim2, int dim3, int dim4 )                      { Resize ( dim1, dim2, dim3, dim4 ); }
                    TMultiArray ( int dim1, int dim2, int dim3, int dim4, int dim5 )            { Resize ( dim1, dim2, dim3, dim4, dim5 ); }
                    TMultiArray ( int dim1, int dim2, int dim3, int dim4, int dim5, int dim6 )  { Resize ( dim1, dim2, dim3, dim4, dim5, dim6 ); }


    void            DeallocateMemory ()                                     { TArray<TypeD>::DeallocateMemory (); NumDims = 0; Dim.DeallocateMemory (); }


    int             GetNumDims      ()  const               { return    NumDims; }
//  const TArray1<int>&   GetDims   ()  const               { return    Dim; }
    int             GetDim1         ()  const               { return    NumDims >= 1 ? Dim[ 0 ] : 0; }
    int             GetDim2         ()  const               { return    NumDims >= 2 ? Dim[ 1 ] : 0; }
    int             GetDim3         ()  const               { return    NumDims >= 3 ? Dim[ 2 ] : 0; }
    int             GetDim4         ()  const               { return    NumDims >= 4 ? Dim[ 3 ] : 0; }
    int             GetDim5         ()  const               { return    NumDims >= 5 ? Dim[ 4 ] : 0; }
    int             GetDim6         ()  const               { return    NumDims >= 6 ? Dim[ 5 ] : 0; }


    void            LinearToIndexes ( int linindex, TArray1<int>& indexes )     const;
    int             IndexesToLinear ( const TArray1<int>& indexes )             const;


    void            ResetMemory     ()                      { TMemory::ResetMemory (); }    // NOT deallocating, just zero-ing containers
    void            Resize  ( const TArray1<int> &dims );   // general purpose - any number of dimensions
    void            Resize  ( int dim1 );                   // series of specialized Set for lowest dimensions
    void            Resize  ( int dim1, int dim2 );
    void            Resize  ( int dim1, int dim2, int dim3 );
    void            Resize  ( int dim1, int dim2, int dim3, int dim4 );
    void            Resize  ( int dim1, int dim2, int dim3, int dim4, int dim5 );
    void            Resize  ( int dim1, int dim2, int dim3, int dim4, int dim5, int dim6 );


    void            Filter  ( int dim, FilterTypes filtertype, int num = 1 );


                    TMultiArray         ( const TMultiArray&        op  );
    TMultiArray<TypeD>& operator    =   ( const TMultiArray<TypeD>& op2 );

    using   TArray::operator    =;

    using   TArray::operator    [];                         // access O[ i ], linear space

    using   TArray::operator    ();                         // access O( i ), linear space
    TypeD&          operator    ()      ( const TArray1<int>& indexes );                // access by array of indexes
    TypeD&          operator    ()      ( int i0, int i1, int i2, int i3 );             // separate indexes when # of dimensions is known
    TypeD&          operator    ()      ( int i0, int i1, int i2, int i3, int i4 );
    TypeD&          operator    ()      ( int i0, int i1, int i2, int i3, int i4, int i5 );


protected:

    int             NumDims;
    TArray1<int>    Dim;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TMultiArray<TypeD>::TMultiArray ( const TMultiArray& op )
{
NumDims         = op.NumDims;
Dim             = op.Dim;
LinearDim       = op.LinearDim;

Array           = (TypeD *) AllocateMemory ( MemorySize () );

CopyMemoryFrom ( op.Array );
}


template <class TypeD>
TMultiArray<TypeD>& TMultiArray<TypeD>::operator= ( const TMultiArray<TypeD>& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Resize ( op2.Dim );

CopyMemoryFrom ( op2.Array );

return  *this;

}

                                        // The real one
template <class TypeD>
void    TMultiArray<TypeD>::Resize ( const TArray1<int>& dims )
{
DeallocateMemory ();

NumDims         = dims.GetDim ();

Dim.Resize ( NumDims );
LinearDim       = 1;

for ( int i = 0; i < NumDims; i++ ) {
                                        // if dimension was 0, set it to 1 to allow non-null size and proper access
    Dim[ i ]    = AtLeast ( 1, dims[ i ] );

    LinearDim  *= Dim[ i ];
    }

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1 )
{
TArray1<int>    dim ( 1 );
dim[ 0 ]        = dim1;

Set ( dim );
}


template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1, int dim2 )
{
TArray1<int>    dim ( 2 );
dim[ 0 ]        = dim1;
dim[ 1 ]        = dim2;

Set ( dim );
}


template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1, int dim2, int dim3 )
{
TArray1<int>    dim ( 3 );
dim[ 0 ]        = dim1;
dim[ 1 ]        = dim2;
dim[ 2 ]        = dim3;

Set ( dim );
}


template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1, int dim2, int dim3, int dim4 )
{
TArray1<int>    dim ( 4 );
dim[ 0 ]        = dim1;
dim[ 1 ]        = dim2;
dim[ 2 ]        = dim3;
dim[ 3 ]        = dim4;

Set ( dim );
}


template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1, int dim2, int dim3, int dim4, int dim5 )
{
TArray1<int>    dim ( 5 );
dim[ 0 ]        = dim1;
dim[ 1 ]        = dim2;
dim[ 2 ]        = dim3;
dim[ 3 ]        = dim4;
dim[ 4 ]        = dim5;

Set ( dim );
}


template <class TypeD>
void    TMultiArray<TypeD>::Resize ( int dim1, int dim2, int dim3, int dim4, int dim5, int dim6 )
{
TArray1<int>    dim ( 6 );
dim[ 0 ]        = dim1;
dim[ 1 ]        = dim2;
dim[ 2 ]        = dim3;
dim[ 3 ]        = dim4;
dim[ 4 ]        = dim5;
dim[ 5 ]        = dim6;

Set ( dim );
}


//----------------------------------------------------------------------------
                                        // does not check dimensions, as this is to be fast
template <class TypeD>
void    TMultiArray<TypeD>::LinearToIndexes ( int linindex, TArray1<int>& indexes )   const
{
for ( int i = NumDims - 1; i >= 0; i-- ) {
    indexes[ i ]    = linindex % Dim[ i ];
    linindex       /= Dim[ i ];
    }
}


template <class TypeD>
int     TMultiArray<TypeD>::IndexesToLinear ( const TArray1<int>& indexes )     const
{
long                linindex        = indexes[ 0 ];

for ( int i = 1; i < NumDims; i++ )
    linindex    = linindex * Dim[ i ] + indexes[ i ];

return  linindex;
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD&  TMultiArray<TypeD>::operator()  ( const TArray1<int>& indexes )
{
return  Array[ IndexesToLinear ( indexes ) ]; 
}

                                        // !It is caller responsibility to ensure dimensions are OK!
template <class TypeD>
TypeD&  TMultiArray<TypeD>::operator()  ( int i0, int i1, int i2, int i3 )
{
long                linindex    =                       i0;
                    linindex    = linindex * Dim[ 1 ] + i1;
                    linindex    = linindex * Dim[ 2 ] + i2;
                    linindex    = linindex * Dim[ 3 ] + i3;

return  Array[ linindex ];
}


template <class TypeD>
TypeD&  TMultiArray<TypeD>::operator()  ( int i0, int i1, int i2, int i3, int i4 )
{
long                linindex    =                       i0;
                    linindex    = linindex * Dim[ 1 ] + i1;
                    linindex    = linindex * Dim[ 2 ] + i2;
                    linindex    = linindex * Dim[ 3 ] + i3;
                    linindex    = linindex * Dim[ 4 ] + i4;

return  Array[ linindex ];
}


template <class TypeD>
TypeD&  TMultiArray<TypeD>::operator()  ( int i0, int i1, int i2, int i3, int i4, int i5 )
{
long                linindex    =                       i0;
                    linindex    = linindex * Dim[ 1 ] + i1;
                    linindex    = linindex * Dim[ 2 ] + i2;
                    linindex    = linindex * Dim[ 3 ] + i3;
                    linindex    = linindex * Dim[ 4 ] + i4;
                    linindex    = linindex * Dim[ 5 ] + i5;

return  Array[ linindex ];
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TMultiArray<TypeD>::Filter ( int dim, FilterTypes filtertype, int num )
{
if ( dim < 0 || dim >= NumDims || num <= 0 )
    return;


constexpr int       Xm          = 0;
constexpr int       X0          = 1;
constexpr int       Xp          = 2;
double              w[ 3 ];
double              sumw;
//double            sumw2;
TypeD               vm;
TypeD               v0;
TArray1<int>        indexes ( GetNumDims () );
int                 li0;


if      ( filtertype == FilterTypeGaussian ) {
    w[ Xm ] = 1;
    w[ X0 ] = 2;
    w[ Xp ] = 1;
    sumw    = 4;
//  sumw2   = 2;
    }
else if ( filtertype == FilterTypeMean ) {
    w[ Xm ] = 1;
    w[ X0 ] = 1;
    w[ Xp ] = 1;
    sumw    = 3;
//  sumw2   = 1.5;
    }
else
    return;

                                        // compute the inner step to reach the next value
int                 dimstep         = 1;
for ( int i = NumDims - 2; i >= dim; i-- )
    dimstep    *= Dim[ i + 1 ];


for ( int fi = 0; fi < num; fi++ ) {
                                        // scan all points
    for ( int li = 0; li < GetLinearDim (); li++ ) {

        LinearToIndexes ( li, indexes );

        if ( indexes[ dim ] != 0 )      // ignore / don't repeat rows that do not start at the smoothing dimension
            continue;

        li0             = li;           // starting index

        vm              = (*this)[ li0 ];
                                        // compute starting edge, padding with 0
//      (*this)[ li0 ]  = (TypeD) ( ( w[ X0 ] * (*this)[ li0 ] + w[ Xp ] * (*this)[ li0 + dimstep ] ) / sumw );
                                        // compute starting edge, padding with copy
        (*this)[ li0 ]  = (TypeD) ( ( ( w[ Xm ] + w[ X0 ] ) * (*this)[ li0 ] + w[ Xp ] * (*this)[ li0 + dimstep ] ) / sumw );

                                        // loop within safe bounds
        for ( int i = 1; i < Dim[ dim ] - 1; i++ ) {
            li0    += dimstep;
            v0      = (*this)[ li0 ];   // save current value

            (*this)[ li0 ]  = (TypeD) ( ( w[ Xm ] * vm + w[ X0 ] * (*this)[ li0 ] + w[ Xp ] * (*this)[ li0 + dimstep ] ) / sumw );

            vm      = v0;
            }

        li0    += dimstep;
                                        // compute last edge, padding with 0
//      (*this)[ li0 ]  = (TypeD) ( ( w[ Xm ] * vm + w[ X0 ] * (*this)[ li0 ] ) / sumw );
                                        // compute last edge, padding with copy
        (*this)[ li0 ]  = (TypeD) ( ( w[ Xm ] * vm + ( w[ X0 ] + w[ Xp ] ) * (*this)[ li0 ] ) / sumw );
        }

    } // for fi
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
