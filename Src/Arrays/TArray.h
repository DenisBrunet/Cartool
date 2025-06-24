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

namespace crtl {

//----------------------------------------------------------------------------
// Parent class to all array classes (TArray1, TArray2, TArray3 etc...)
// These classes focus only on the storage part.
//----------------------------------------------------------------------------
                                        // Sorting parameters
enum            SortDirection
                {
                Ascending,
                Descending,
                };

enum            SortToArrayEnum
                {
                SortDataIndex   = 0,
                SortDataValue,
                SortDataNum
                };


//----------------------------------------------------------------------------
                                        // Constraining array sizes
enum            ArraySizeType {
                AnySize                     = 0,
                OddSize,
                EvenSize,
                };


inline  int         DiameterToKernelSize ( double diameter, ArraySizeType sizeconstraint );


//----------------------------------------------------------------------------
                                        // Functions to be used with ComputeHistogram results
inline  double      RealToIndex             ( double data,     double &curvemin, double &curveratio )      { return    ( data   - curvemin   ) * curveratio;    }
inline  double      IndexToReal             ( double kernel,   double &curvemin, double &curveratio )      { return    ( kernel / curveratio ) + curvemin;      }
inline  double      RealWidthToIndexWidth   ( double kernel,                     double &curveratio )      { return      kernel * curveratio;                   }
inline  double      IndexWidthToRealWidth   ( double kernel,                     double &curveratio )      { return      kernel / curveratio;                   }

                                        // Convert a real value into an index
                                        // Very handy to index an array with floating point values
class   TRealIndex
{
public:
                    TRealIndex ()                                       { Reset (); }

                                        // Offset and ratio are totally public
    double          IndexMin;
    double          IndexRatio;


    void            Reset ()                                            { IndexMin = 0; IndexRatio = 1; }


    bool            IsNull          ()                          const   { return    IndexMin == 0 && IndexRatio == 1; }
    bool            IsNotNull       ()                          const   { return ! (IndexMin == 0 && IndexRatio == 1 ); }

    double          ToIndex         ( double realposition  )    const   { return  ( realposition  - IndexMin   ) * IndexRatio; }    // NOT clipped & result in double, so we can convert back and forth
    double          ToReal          ( double indexposition )    const   { return  ( indexposition / IndexRatio ) + IndexMin;   }
    double          ToIndexWidth    ( double realwidth     )    const   { return    realwidth  * IndexRatio;                   }
    double          ToRealWidth     ( double indexwidth    )    const   { return    indexwidth / IndexRatio;                   }
};


//----------------------------------------------------------------------------
                                        // Common fields & methods to all derived TArrayX classes
                                        // This class does not perform any memory allocation by itself, which is the duty of the derived classes
                                        // For the same reason, it does not have any constructor nor assignation operators either
                                        // Finally, it has not been designed to work on complex TypeD which require a non-null initialization,
                                        // as derived classes currently just wipe out newly allocated memory!
                                        // So current implementation is more like a  template <typename T> class TArray
template <class TypeD>
class   TArray  :   public  TMemory
{
public:
                    TArray              () : LinearDim ( 0 ), Array ( 0 )   {}


    size_t          AtomSize            ()          const   { return sizeof ( TypeD ); }
    size_t          MemorySize          ()          const   { return LinearDim * AtomSize (); }     // minimum consecutive memory needed to store this amount of data

    bool            IsAllocated         ()          const   { return LinearDim != 0; }
    bool            IsNotAllocated      ()          const   { return LinearDim == 0; }
    virtual void    DeallocateMemory    ();            // this is the only thing we can do about memory here


    size_t          GetLinearDim        ()          const   { return LinearDim; }


          TypeD*    GetArray            ()                  { return Array; }
    const TypeD*    GetArray            ()          const   { return Array; }

                                                            // access in linear space
          TypeD&    GetValue            ( int i )           { return Array[ i /*Clip ( i, 0, Dim1 - 1 )*/ ]; }
    const TypeD&    GetValue            ( int i )   const   { return Array[ i /*Clip ( i, 0, Dim1 - 1 )*/ ]; }


    TArray<TypeD>&  operator    =       ( TypeD op2 );

                                                            // access in linear space
          TypeD&    operator    []      ( int i )           { return Array[ i ]; }
    const TypeD&    operator    []      ( int i )   const   { return Array[ i ]; }
          TypeD&    operator    ()      ( int i )           { return Array[ i ]; }
    const TypeD&    operator    ()      ( int i )   const   { return Array[ i ]; }

                                                            // cast
    virtual explicit operator   int             ()  const   { return (int) LinearDim; } // used for loops
    virtual explicit operator   bool            ()  const   { return LinearDim != 0; }
    virtual         operator          TypeD*    ()          { return Array; }
    virtual         operator    const TypeD*    ()  const   { return Array; }


protected:

    size_t          LinearDim;          // linear dimension of the contiguous storage
    TypeD*          Array;              // contiguous storage for all derived arrays - where to allocate is up to derived class, though

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // Convert a requested Kernel diameter into its best discrete size
                                        // caller can force the final size to be either odd or even
int         DiameterToKernelSize ( double diameter, ArraySizeType sizeconstraint )
{
if ( diameter < 1 )
    diameter    = 1;

int                 size;

//#include    "Math.Utils.h" creates some circular reference issues
auto    Truncate    = []            ( double v )    { return  (int) v; };
auto    Fraction    = [ Truncate ]  ( double v )    { return  v - Truncate ( v ); };
auto    Round       = []            ( double v )    { return  v < 0 ? (int) ( v - 0.5 ) : (int) ( v + 0.5 ); };


if ( sizeconstraint == OddSize ) {
                                        // set to be big enough to contain the requested sphere
                                        // formula is based on considering the center of the voxels

                                        // the part above 1 voxel
    int                 radius          = Round ( diameter / 2 );
                                        // 1 -> 0; ]1..3] -> 1; ]3..5] -> 2 etc...
    if ( Fraction ( diameter / 2 + 0.5 ) == 0 )
        radius--;

    size        = 1 + 2 * radius;
    }

else if ( sizeconstraint == EvenSize ) {
                                        // [1..3[ -> 2; [3..5[ ->4 etc...
    int                 radius          = Round ( diameter / 2 );

    size        = 2 * radius;
    }

else {                                  // closest size, no hassles from centering anything
    size        = Round ( diameter );
    }


return  size;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TypeD>
void    TArray<TypeD>::DeallocateMemory ()
{
TMemory::DeallocateMemory ();

LinearDim       = 0;
Array           = 0;
}


template <class TypeD>
TArray<TypeD>&  TArray<TypeD>::operator= ( TypeD op2 )
{
if ( op2 == (TypeD) 0 )
                                        // save a loop
    TMemory::ResetMemory ();

else
    for ( int i = 0; i < LinearDim; i++ )
        Array[ i ]  = op2;

return  *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
