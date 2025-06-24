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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD>
class   TArray2 :   public  TArray<TypeD>
{
public:
                    TArray2             () : TArray (), Dim1 ( 0 ), Dim2 ( 0 ) {}
                    TArray2             ( int dim1, int dim2 );


    void            DeallocateMemory    ()                          { TArray<TypeD>::DeallocateMemory (); Dim1 = 0; Dim2 = 0; }


    int             GetDim1             ()          const           { return Dim1; }
    int             GetDim2             ()          const           { return Dim2; }

    int             MaxSize             ()          const           { return max ( Dim1, Dim2 ); }
    int             MinSize             ()          const           { return min ( Dim1, Dim2 ); }

    virtual void    Resize              ( int newdim1, int newdim2 );


    using   TArray::GetValue;
          TypeD&    GetValue            ( int i1, int i2 )         { return Array[ IndexesToLinearIndex ( i1, i2 ) ]; }
    const TypeD&    GetValue            ( int i1, int i2 ) const   { return Array[ IndexesToLinearIndex ( i1, i2 ) ]; }

                                        // last dimension indexes are consecutive
    int             IndexesToLinearIndex ( int i1, int i2 )             const   { return i1 * Dim2 + i2; }
    void            LinearIndexToIndexes ( int li, int& i1, int& i2 )   const   { i1 = li / Dim2; i2 = li % Dim2; }


    TypeD           GetMinValue         ()          const;
    TypeD           GetMaxValue         ()          const;
    TypeD           GetAbsMaxValue      ()          const;
    void            GetMinMaxValues     ( TypeD& minvalue, TypeD& maxvalue )    const;


    void            SortRows            ( int column, SortDirection direction, int numitems = -1 );
    void            Transpose           ();

    void            Insert              ( const TArray2<TypeD>& fromarray, const int* origin = 0 );


                    TArray2<TypeD>      ( const TArray2<TypeD> &op );
    TArray2<TypeD>& operator    =       ( const TArray2<TypeD> &op2 );

    using   TArray::operator    =;
                                                            
    using   TArray::operator    [];                         // access O[ i ], linear space - !different from TypeD* operator below!
          TypeD*    operator    []      ( int i )                   { return Array + i * Dim2; }    // returns a pointer to row i, to get a full line with  array[ rowi ], or using the old syntax  array[ rowi ][ colj ]
    const TypeD*    operator    []      ( int i ) const             { return Array + i * Dim2; }

    using   TArray::operator    ();                         // access O( i ), linear space
          TypeD&    operator    ()      ( int i1, int i2 )          { return Array[ IndexesToLinearIndex ( i1, i2 ) ]; }    // access O(x, y)
    const TypeD&    operator    ()      ( int i1, int i2 )  const   { return Array[ IndexesToLinearIndex ( i1, i2 ) ]; }


protected:

    int             Dim1;
    int             Dim2;


    void            SortRows_         ( int column, SortDirection direction, int l, int r );
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TArray2<TypeD>::TArray2 ( const TArray2 &op )
{
Dim1            = op.Dim1;
Dim2            = op.Dim2;
LinearDim       = op.LinearDim;
Array           = (TypeD *) AllocateMemory ( MemorySize () );

CopyMemoryFrom ( op.Array );
}


template <class TypeD>
TArray2<TypeD>& TArray2<TypeD>::operator= ( const TArray2<TypeD> &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

                                      
Resize ( op2.Dim1, op2.Dim2 );

CopyMemoryFrom ( op2.Array );


return  *this;
}

                                        // resize the array, previous data is lost - used mainly to avoid creation + assignment
template <class TypeD>
void            TArray2<TypeD>::Resize ( int newdim1, int newdim2 )
{
                                        // no changes?
if ( newdim1 == Dim1 && newdim2 == Dim2 ) {
//  ResetMemory ();
    return;
    }

                                        // clean up existing memory
DeallocateMemory ();

Dim1            = newdim1;
Dim2            = newdim2;
LinearDim       = Dim1 * Dim2;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TArray2<TypeD>::TArray2 ( int dim1, int dim2 )
{
Dim1            = dim1;
Dim2            = dim2;
LinearDim       = Dim1 * Dim2;

Array           = (TypeD *) AllocateMemory ( MemorySize () );
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TArray2<TypeD>::GetMaxValue ()  const
{
TypeD               maxvalue        = Lowest  ( maxvalue );

for ( int i = 0; i < LinearDim; i++ )
    Maxed ( maxvalue, Array[ i ] );

return  maxvalue;
}


template <class TypeD>
TypeD   TArray2<TypeD>::GetMinValue ()  const
{
TypeD               minvalue        = Highest ( minvalue );

for ( int i = 0; i < LinearDim; i++ )
    Mined ( minvalue, Array[ i ] );

return  minvalue;
}


template <class TypeD>
TypeD   TArray2<TypeD>::GetAbsMaxValue ()  const
{
TypeD               minvalue;
TypeD               maxvalue;

GetMinMaxValues ( minvalue, maxvalue );

return  max ( abs ( minvalue ), abs ( maxvalue ) ); 
}


template <class TypeD>
void    TArray2<TypeD>::GetMinMaxValues     (   TypeD&      minvalue,   TypeD&      maxvalue    )   const
{
minvalue    = Highest ( minvalue );
maxvalue    = Lowest  ( maxvalue );

for ( int i = 0; i < LinearDim; i++ ) {

    Mined ( minvalue, Array[ i ] );
    Maxed ( maxvalue, Array[ i ] );
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TArray2<TypeD>::Transpose ()
{
                                        // Do an in-place memory swapping
                                        // Method is not straightforward for non-squared arrays, and browse through sub-cycles in the succession of indexes
int                 lindim          = GetLinearDim ();
TArray1<uchar>      overwritten ( lindim );

overwritten     = false;


do {
    int         cycleentry;
                                        // get next cycle entry point, in linear space
    for ( cycleentry = 0; cycleentry < lindim; cycleentry++ )
        if ( ! overwritten[ cycleentry ] )
            break;

                                        // no next unswapped slot?
    if ( cycleentry == lindim )
        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int         currenti        = cycleentry;
    TypeD       currentvalue    = Array[ currenti ];

    do {
                                        // getting indexes from current linear index
        int         fromi1,     fromi2;

        LinearIndexToIndexes ( currenti, fromi1, fromi2 );
                                        // new linear address in swapped dimensions
        int         targeti     = crtl::IndexesToLinearIndex ( fromi2, fromi1, Dim1 );

                                        // 1 cycle: no need to swap anything
        if ( currenti == cycleentry && targeti == cycleentry ) {

            overwritten[ currenti ] = true;
            break;
            }

                                        // proceed with next cycle position
        Permutate ( currentvalue, Array[ targeti ] );
                                        // remember overwritten position
        overwritten[ targeti ]  = true;
                                        // go to next position in cycle
        currenti    = targeti;
                                        // have we completed the current cycle?
        } while ( currenti != cycleentry );

    } while ( true );

                                        // Finally, we can swap the dimensions
                                        // All other things remain constant: linear dimension and pointer to array
Permutate ( Dim1, Dim2 );
}


//----------------------------------------------------------------------------

template <class TypeD>
void    TArray2<TypeD>::Insert ( const TArray2<TypeD>& fromarray, const int* origin )
{
if ( ! origin ) {                       // this simply plugs in another array, clipping it if necessary
    int                 x;
    int                 indim1          = min ( Dim1, fromarray.GetDim1 () );
    int                 indim2          = min ( Dim2, fromarray.GetDim2 () );
    int                 size            = indim2 * AtomSize ();
    int                 inindex;
    int                 outindex;

                                        // actually the same array
    if ( indim1 == Dim1 && indim2 == Dim2 && AtomSize () == fromarray.AtomSize () )

        CopyVirtualMemory ( Array, fromarray.GetArray (), MemorySize () );
//      CopyMemoryFrom ( data );

    else                                // really need to insert
                                        // scan within safe limits
        for ( x = 0; x < indim1; x++ ) {
                                        // get index of first Y
            inindex  = fromarray.IndexesToLinearIndex ( x, 0 );
//          outindex =           IndexesToLinearIndex ( x + origin[0], y + origin[1] );
            outindex =           IndexesToLinearIndex ( x, 0 );

                                        // directly copy a line of Z
            CopyVirtualMemory ( &Array[ outindex ], fromarray[ inindex ], size );

            }
    }

else {
    int                 x,  y;
    int                 indim1          = min ( max ( 0, Dim1 - origin[ 0 ] ), fromarray.GetDim1 () );
    int                 indim2          = min ( max ( 0, Dim2 - origin[ 1 ] ), fromarray.GetDim2 () );

                                        // scan within safe limits
    for ( x = 0; x < indim1; x++ )
    for ( y = 0; y < indim2; y++ )
        (*this) ( x + origin[ 0 ], y + origin[ 1 ] )    = fromarray ( x, y ); //  * intensityrescale;
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TArray2<TypeD>::SortRows ( int column, SortDirection direction, int numitems )
{
if ( ! IsInsideLimits ( column, 0, Dim2 - 1 ) )     column  = 0;

if ( numitems == 0 )
    return;

SortRows_ ( column, direction, 0, numitems > 0 ? numitems - 1 : Dim1 - 1 );
}


template <class TypeD>
void    TArray2<TypeD>::SortRows_ ( int column, SortDirection direction, int l, int r )
{
if ( r <= l )   return;


int                 i               = l;
int                 j               = r;
TypeD               v               = (TypeD) GetValue ( ( l + r ) / 2, column );


do {
    while ( direction == Ascending  && GetValue ( i, column ) < v
         || direction == Descending && GetValue ( i, column ) > v                      )    i++;

    while ( direction == Ascending  && v                      < GetValue ( j, column )
         || direction == Descending && v                      > GetValue ( j, column ) )    j--;

    if ( i <= j ) {
                                        // permutate rows i and j
        for ( int coli = 0; coli < Dim2; coli++ )
            Permutate ( GetValue ( i, coli ), GetValue ( j, coli ) );

        i++;    j--;
        }

    } while ( i <= j );


if ( l < j )    SortRows_ ( column, direction, l, j );
if ( i < r )    SortRows_ ( column, direction, i, r );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
