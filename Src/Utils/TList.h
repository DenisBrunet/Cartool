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

#include    "System.OpenMP.h"

#include    "TArray1.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // A doubly linked, indexed & thread-safe list class
//----------------------------------------------------------------------------
                                        // List atom
template <class TypeD>
class   TListAtom
{
public:
                        TListAtom ( const TypeD* toatom = 0 ) : To ( (TypeD*) toatom ), Next ( 0 ), Previous ( 0 )     {}

    TypeD*              To;

    TListAtom<TypeD>*   Next;
    TListAtom<TypeD>*   Previous;
};


//----------------------------------------------------------------------------
                                        // Used to control if actual data has to be deleted:
enum        DeallocateType
            {
            DontDeallocate,
            Deallocate,
            };

                                        // List itself
                                        // It will actually store TypeD*, allocation is done on the caller side
template <class TypeD>
class   TList
{
public:
                        TList ();
    virtual            ~TList ();       // does NOT release actual content, but only the list structure - user has to call Reset ( true ) to actually release content


    int                 Num             ()  const           { return    NumInside; }
    bool                IsEmpty         ()  const           { return	NumInside == 0; }
    bool                IsNotEmpty      ()  const           { return    NumInside != 0; }

    TListAtom<TypeD>*   IsInside        ( const TypeD* toatom )     const;


    void                Append          ( const TypeD*  toatom );
    void                Append          ( const TList&  list   );
    bool                Insert          ( const TypeD*  toatom, const TypeD* beforeatom );

    void                Remove          ( const TListAtom<TypeD>* tolistatom, DeallocateType deallocate );  // from a list node - safer
    void                Remove          ( const TypeD* toatom,                DeallocateType deallocate );  // from content - might have side effects, if multiple elements happen to be equal
    void                RemoveFirst     ( DeallocateType deallocate, int num = 1 );
    void                RemoveLast      ( DeallocateType deallocate, int num = 1 );
    void                Reset           ( DeallocateType deallocate );

    void                Permutate       ( const TypeD* toatom1, const TypeD* toatom2 );
    void                RevertOrder     ();


          TypeD*                GetFirst        ()          { return    First ? First->To : 0; }
    const TypeD*                GetFirst        ()  const   { return    First ? First->To : 0; }
          TypeD*                GetLast         ()          { return    Last  ? Last ->To : 0; }
    const TypeD*                GetLast         ()  const   { return    Last  ? Last ->To : 0; }

          TListAtom<TypeD>*     GetFirstAtom    ()          { return    First; }
    const TListAtom<TypeD>*     GetFirstAtom    ()  const   { return    First; }
          TListAtom<TypeD>*     GetLastAtom     ()          { return    Last;  }
    const TListAtom<TypeD>*     GetLastAtom     ()  const   { return    Last;  }

          TListAtom<TypeD>*     GetAtom         ( int index );
    const TListAtom<TypeD>*     GetAtom         ( int index )   const;

                                        // gets an array of pointers to data, for fast direct access
    void                GetIndexes      ( TArray1<TListAtom<TypeD> *>&  indexes )   const;
    void                GetIndexes      ( TArray1<TypeD *>&             indexes )   const;
    void                UpdateIndexes   ( bool force = false ); // !this part is currently not thread / parallel safe, and should be called only once before entering a parallel block!


    TList<TypeD>                        ( const TList<TypeD>& op  );
    TList<TypeD>&       operator    =   ( const TList<TypeD>& op2 );

                                        // array-style access
          TypeD&        operator    ()  ( int index );          // indexed access - returns a reference
    const TypeD&        operator    ()  ( int index )   const;  
          TypeD*        operator    []  ( int index );          // indexed access - returns a pointer
    const TypeD*        operator    []  ( int index )   const;

                        operator bool   ()  const           { return NumInside > 0; }
                        operator int    ()  const           { return NumInside;     }

    TList<TypeD>&       operator    =   ( double op2 )      { return *this; }   // null operator, to allow TArray3< TList > to run ReadFile correctly


private:

    TListAtom<TypeD>*   First;
    TListAtom<TypeD>*   Last;
    int                 NumInside;
                                        // provide a direct access mechanism, with an array of pointers to Atoms
                                        // !check any derived classes or objects that mess up directly with the atoms if it needs to call  UpdateIndexes  when done!
    TArray1< TListAtom<TypeD>* >    AtomIndexes;
    bool                            AtomIndexesIsDirty;

};


//----------------------------------------------------------------------------
                                        // iterator for TList, safe for multithreading
template <class TypeD>
class   TListIterator
{
public:
                    TListIterator ()                            { Reset (); }


    void            Reset      ()                               { P = 0; Index = -1; }

    void            SetForward (       TList<TypeD>&    List )  { if ( (bool) List )  { P =                     List.GetFirstAtom (); Index = 0;              } else Reset (); }
    void            SetForward ( const TList<TypeD>&    List )  { if ( (bool) List )  { P = (TListAtom<TypeD>*) List.GetFirstAtom (); Index = 0;              } else Reset (); }
    void            SetBackward(       TList<TypeD>&    List )  { if ( (bool) List )  { P =                     List.GetLastAtom  (); Index = (int) List - 1; } else Reset (); }
    void            SetBackward( const TList<TypeD>&    List )  { if ( (bool) List )  { P = (TListAtom<TypeD>*) List.GetLastAtom  (); Index = (int) List - 1; } else Reset (); }

    TListAtom<TypeD>*   Current  ()                             { return P; }
    void			Next     ()                                 { if ( P )  { P = P->Next;     Index++; } }
    void			Previous ()                                 { if ( P )  { P = P->Previous; Index--; } }


          TypeD*    operator ()                 ()              { return P ? P->To : 0;	}
    const TypeD*    operator ()                 ()  const       { return P ? P->To : 0;	}

                    operator bool               ()              { return P != 0;		}
                    operator bool               ()  const       { return P != 0;		}
                    operator int                ()              { return Index;			}
                    operator int                ()  const       { return Index;			}
                    operator       TypeD*       ()              { return P ? P->To : 0;	}
                    operator const TypeD*       ()  const       { return P ? P->To : 0;	}
                    operator TListAtom<TypeD>*  ()	            { return P;             }
                    operator TListAtom<TypeD>*  ()	const       { return P;             }


protected:
    TListAtom<TypeD>*   P;              // pointer to current atom
    int                 Index;          // current index
};


//----------------------------------------------------------------------------
                                        // a few handy macros
#define foreachin(LIST,ITERATOR)            for ( ITERATOR.SetForward  ( LIST ); (bool) ITERATOR; ITERATOR.Next     () )

#define foreachbackwardin(LIST,ITERATOR)    for ( ITERATOR.SetBackward ( LIST ); (bool) ITERATOR; ITERATOR.Previous () )
                                        // a break occurred, so we need to step once before continuing
#define continueforeachin(LIST,ITERATOR)    for ( ITERATOR.Next ()             ; (bool) ITERATOR; ITERATOR.Next     () )


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TList<TypeD>::TList ()
{
First               = 0;
Last                = 0;
NumInside           = 0;
AtomIndexesIsDirty  = false;
}


template <class TypeD>
        TList<TypeD>::~TList ()
{
                                        // default is to NOT destroy content, as we deal with a list of pointers - caller should take care of the actual release beforehand
Reset ( DontDeallocate );
}


template <class TypeD>
        TList<TypeD>::TList ( const TList<TypeD> &op )
{
First               = 0;
Last                = 0;
NumInside           = 0;
AtomIndexesIsDirty  = false;

Append ( op );
}


template <class TypeD>
TList<TypeD>& TList<TypeD>::operator= ( const TList<TypeD> &op2 )
{
if ( &op2 == this )
    return  *this;


Reset ( DontDeallocate );

Append ( op2 );

return  *this;
}


template <class TypeD>
void    TList<TypeD>::Reset ( DeallocateType deallocate )
{
if ( IsEmpty () )
    return;


OmpCriticalBegin (TListReset)

TListAtom<TypeD>*   p;
TListAtom<TypeD>*   p2;

                                        // first, optionally destroy actual content
if ( deallocate == Deallocate )

    for ( p = First; p != 0; p = p->Next )

        if ( p->To != 0 )

            delete  p->To;

                                        // then only destroy list structure itself
for ( p = First, p2 = p ? p->Next : 0; p != 0; p = p2, p2 = p ? p->Next : 0 )

    delete  p;


NumInside           = 0;
First               = 0;
Last                = 0;
AtomIndexesIsDirty  = false;

AtomIndexes.DeallocateMemory ();

OmpCriticalEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Append ( const TypeD* toatom )
{
OmpCriticalBegin (TListAppend)

if ( IsEmpty () )

    Last    = First         = new TListAtom<TypeD> ( toatom );

else {
    Last->Next              = new TListAtom<TypeD> ( toatom );
    Last->Next->Previous    = Last;
    Last                    = Last->Next;
    }

NumInside++;
AtomIndexesIsDirty  = true;

OmpCriticalEnd
}


template <class TypeD>
void    TList<TypeD>::Append ( const TList &list )
{
TListAtom<TypeD>*   p;

for ( p = list.First; p != 0; p = p->Next )
    Append ( p->To );

UpdateIndexes ( true );
}


template <class TypeD>
bool    TList<TypeD>::Insert ( const TypeD* toatom, const TypeD* beforeatom )
{
if ( IsEmpty () || beforeatom == 0 ) {
    Append ( toatom );
    return  true;
    }


TListAtom<TypeD>*   postatom    = IsInside ( beforeatom );

if ( ! postatom )                       // not found the before atom?
    return  false;


OmpCriticalBegin (TListInsert)

TListAtom<TypeD>*   newatom     = new TListAtom<TypeD> ( toatom );
TListAtom<TypeD>*   preatom;
                                        // before first?
if ( postatom == First ) {
    First                   = newatom;
    newatom ->Next          = postatom;
    postatom->Previous      = newatom;
    }
else {                                  // somewhere in the middle
    preatom                 = postatom->Previous;

    postatom->Previous      = newatom;
    newatom ->Previous      = preatom;
    newatom ->Next          = postatom;
    preatom ->Next          = postatom->Previous;
    }

NumInside++;
AtomIndexesIsDirty  = true;

OmpCriticalEnd

return  true;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::GetIndexes ( TArray1<TListAtom<TypeD>*>& indexes )  const
{
indexes.Resize ( Num () );

int                 i;
TListAtom<TypeD>*   p;

for ( i = 0, p = First ; p != 0; p = p->Next, i++ )
    indexes[ i ]    = p;
}


template <class TypeD>
void    TList<TypeD>::GetIndexes ( TArray1<TypeD*>& indexes )  const
{
indexes.Resize ( Num () );

int                 i;
TListAtom<TypeD>*   p;

for ( i = 0, p = First ; p != 0; p = p->Next, i++ )
    indexes[ i ]    = p->To;
}

                                        // refresh our private fast access index array - thread safe(?)
template <class TypeD>
void    TList<TypeD>::UpdateIndexes ( bool force )
{
if ( ! ( AtomIndexesIsDirty || force ) )
    return;


OmpCriticalBegin (TListUpdateIndexes)

GetIndexes ( AtomIndexes );

AtomIndexesIsDirty  = false;

OmpCriticalEnd
}


template <class TypeD>
TypeD&  TList<TypeD>::operator() ( int index )
{
UpdateIndexes ();

return  *AtomIndexes[ index ]->To;
}


template <class TypeD>
const TypeD&    TList<TypeD>::operator() ( int index ) const
{
                                        // hack until all methods are correctly const'ed
const_cast<TList*>( this )->UpdateIndexes ();

return  *AtomIndexes[ index ]->To;
}


template <class TypeD>
TypeD*  TList<TypeD>::operator[] ( int index )
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ]->To : 0;
}


template <class TypeD>
const TypeD*    TList<TypeD>::operator[] ( int index ) const
{
                                        // hack until all methods are correctly const'ed
const_cast<TList*>( this )->UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ]->To : 0;
}


template <class TypeD>
TListAtom<TypeD>*   TList<TypeD>::GetAtom ( int index )
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ] : 0;


/*                                      // old & slow code
if ( IsEmpty () || index < 0 || index >= Num () )
    return  0;


int                 i;
TListAtom<TypeD>*   p;

if ( index < Num () / 2 ) {                 // choose the closest end
    for ( i = 0, p = First ; p != 0; p = p->Next, i++ )
        if ( i == index )   return  p;
    }
else {
    for ( i = Num () - 1, p = Last; p != 0; p = p->Previous, i-- )
        if ( i == index )   return  p;
    }

return  0;
*/
}


template <class TypeD>
const TListAtom<TypeD>*   TList<TypeD>::GetAtom ( int index ) const
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ] : 0;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Permutate ( const TypeD *toatom1, const TypeD *toatom2 )
{
if ( IsEmpty () )
    return;

TListAtom<TypeD>*   p1              = IsInside ( toatom1 );

if ( p1 == 0 )
    return;

TListAtom<TypeD>*   p2              = IsInside ( toatom2 );

if ( p2 == 0 )
    return;


OmpCriticalBegin (TListPermutate)
                                        // just permutating the POINTERS to content!
Permutate ( p1->To, p2->To );

AtomIndexesIsDirty  = true;

OmpCriticalEnd
}


template <class TypeD>
void    TList<TypeD>::RevertOrder ()
{
if ( Num () <= 1 )
    return;


TList<TypeD>        revlist;
TListAtom<TypeD>*   p;
                                        // revert copy to a temp list
for ( p = Last; p != 0; p = p->Previous )
    revlist.Append ( p->To );


Reset ( DontDeallocate );

                                        // copy back the reverted temp list
for ( p = revlist.First; p != 0; p = p->Next )
    Append ( p->To );


AtomIndexesIsDirty  = true;

UpdateIndexes ( true );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Remove ( const TListAtom<TypeD>* tolistatom, DeallocateType deallocate )
{
if ( IsEmpty () || tolistatom == 0 )
    return;


OmpCriticalBegin (TListRemove)
                                        // store previous and next atoms pointers - could be 0
TListAtom<TypeD>*           previousatom            = tolistatom->Previous;
TListAtom<TypeD>*           nextatom                = tolistatom->Next;


if ( previousatom )         previousatom->Next      = nextatom;     // could be 0 when removing last element
if ( nextatom     )         nextatom    ->Previous  = previousatom; // could be 0 when removing first element


if ( First == tolistatom )  First                   = nextatom;     // could be 0 when removing last element
if ( Last  == tolistatom )  Last                    = previousatom; // same

                                        // finally, we can delete objects
if ( deallocate == Deallocate )
    delete  tolistatom->To;             // first, content if requested by caller

delete  tolistatom;                     // then the list atom itself

                                        // decrement count
NumInside--;
AtomIndexesIsDirty  = true;

OmpCriticalEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Remove ( const TypeD *toatom, DeallocateType deallocate )
{
Remove ( IsInside ( toatom ), deallocate );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::RemoveFirst ( DeallocateType deallocate, int num )
{
if ( IsEmpty () || num < 1 )
    return;

                                        // remove all?
if ( num >= NumInside ) {
    Reset ( deallocate );
    return;
    }


for ( ; num > 0; num-- )
    Remove ( First, deallocate );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::RemoveLast ( DeallocateType deallocate, int num )
{
if ( IsEmpty () || num < 1 )
    return;

                                        // remove all?
if ( num >= NumInside ) {
    Reset ( deallocate );
    return;
    }


for ( ; num > 0; num-- )
    Remove ( Last, deallocate );
}


//----------------------------------------------------------------------------
template <class TypeD>
TListAtom<TypeD>*  TList<TypeD>::IsInside ( const TypeD* toatom ) const
{
if ( IsEmpty () || toatom == 0 )
    return  0;


TListAtom<TypeD>*   p               = 0;

OmpCriticalBegin (TListIsInside)

for ( p = First; p != 0; p = p->Next )
                                        // returns first element with pointer to content equality
    if ( p->To == toatom )

        break;

OmpCriticalEnd

return  p;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
