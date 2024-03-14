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

namespace crtl {

//----------------------------------------------------------------------------
// Experience has shown that it seems a better idea to have separate classes to
// implement filters, instead of methods that are part of classes.
// With a filter class, it is more practical to store any local / temp variables
// needed for the filtering. It will also centralize calls from different types
// of data structures, usually through a bit of data shuffling, and all at the
// same place.
//----------------------------------------------------------------------------
                                        // Base class for a single filter
template <class TypeD>
class   TFilter
{
public:
    virtual        ~TFilter ()  {}

    virtual void    Reset   ()  {}

    virtual void    Apply                   ( TypeD* data, int numpts ) {} // = 0;

};


//----------------------------------------------------------------------------
                                        // Group of TFilter's
                                        // !Note that each filter has its own sampling frequency, so for the moment nothing prevents to have a bank of filters with different sf!
constexpr int   TGoFiltersMaxFilters        = 4096 / 8;


template <class TypeD>
class   TFilters
{
public:
                    TFilters ();
                   ~TFilters ();


//  int             GetNumFilters   ()  const           { return    (int) Filters; }
//  bool            IsEmpty         ()  const           { return    Filters.IsEmpty (); }
    int             GetNumFilters   ()  const           { return    NumFilters; }
    bool            IsEmpty         ()  const           { return    NumFilters == 0; }


    void            Reset           ();

    void            Add             ( TFilter<TypeD>* filter );

    void            Apply           ( TypeD* data, int numpts )         const;

//  void            Show            ( char *title = 0 );


                    TFilters        ( const TFilters& op  );
    TFilters&       operator    =   ( const TFilters& op2 );


    TFilter<TypeD>& operator    []                  ( int index )           { return *Filters[ index ]; }
    TFilter<TypeD>& operator    []                  ( int index )   const   { return *Filters[ index ]; }

//                  operator    int                 ()              const   { return (int)  Filters; }
//                  operator    bool                ()              const   { return (bool) Filters; }
                    operator    int                 ()              const   { return NumFilters; }
                    operator    bool                ()              const   { return NumFilters != 0; }

protected:
                                        // 2 different implementations:
//  TList< TFilter<TypeD> > Filters;                    // list of pointers - universal, but adds another layer of allocations - not thread-friendly (concurent access to GetIndexes)

    TFilter<TypeD>* Filters[ TGoFiltersMaxFilters ];    // fixed array of pointers - faster, and also thread-friendly
    int             NumFilters;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TFilters<TypeD>::TFilters ()
{
NumFilters  = 0;

Reset ();
}


template <class TypeD>
        TFilters<TypeD>::~TFilters ()
{
Reset ();
}


template <class TypeD>
void    TFilters<TypeD>::Reset ()
{
//Filters.Reset ( true );

                                        // actually deleting filters
for ( int fi = 0; fi < NumFilters; fi++ )
    delete  Filters[ fi ];

                                        // resetting array of pointers, for correctness
for ( int fi = 0; fi < TGoFiltersMaxFilters; fi++ )
    Filters[ fi ]   = 0;


NumFilters  = 0;
}


template <class TypeD>
            TFilters<TypeD>::TFilters ( const TFilters& op )
{
for ( int i = 0; i < op.GetNumFilters (); i++ )
    Add ( new TFilter<TypeD> ( *op.Filters[ i ] ) );
}


template <class TypeD>
TFilters<TypeD>& TFilters<TypeD>::operator= ( const TFilters& op2 )
{
if ( &op2 == this )
    return  *this;

Reset ();

for ( int i = 0; i < op2.GetNumFilters (); i++ )
    Add ( new TFilter<TypeD> ( *op2.Filters[ i ] ) );

return  *this;
}

//----------------------------------------------------------------------------
template <class TypeD>
void    TFilters<TypeD>::Add ( TFilter<TypeD>* filter )
{
//if ( filter )
//    Filters.Append ( filter );

if ( NumFilters == TGoFiltersMaxFilters )
    return;

Filters[ NumFilters++ ] = filter;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TFilters<TypeD>::Apply ( TypeD* data, int numpts )    const
{
for ( int i = 0; i < GetNumFilters (); i++ )
    Filters[ i ]->Apply ( data, numpts );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
