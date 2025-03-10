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
                        TListAtom ( const TypeD* todata = 0 )
                      : ToData ( (TypeD*) todata ), Next ( 0 ), Previous ( 0 )      {}

    TypeD*              ToData;

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
                            TList           ();
    virtual                ~TList           (); // does NOT release actual content, but only the list structure itself - user has to call Reset ( Deallocate ) beforehand to actually release content


    int                     Num             ()  const           { return    NumInside; }
    bool                    IsEmpty         ()  const           { return	NumInside == 0; }
    bool                    IsNotEmpty      ()  const           { return    NumInside != 0; }

    bool                    IsInside        ( const TypeD* todata )     const;


    void                    Append          ( const TypeD* todata );
    void                    Append          ( const TList& list   );
    bool                    Insert          ( const TypeD* todata, const TypeD* beforedata );

    void                    RemoveAtom      ( const TListAtom<TypeD>* tolistatom, DeallocateType deallocate );  // from a list node - safer
    void                    Remove          ( const TypeD*            todata,     DeallocateType deallocate );  // from content - might have side effects, if multiple elements happen to be equal
    void                    RemoveFirst     ( DeallocateType deallocate, int num = 1 );
    void                    RemoveLast      ( DeallocateType deallocate, int num = 1 );
    void                    Reset           ( DeallocateType deallocate );

    void                    Permutate       ( const TypeD* todata1, const TypeD* todata2 );
    void                    RevertOrder     ();


          TypeD*            GetFirst        ()                  { return    First ? First->ToData : 0; }
    const TypeD*            GetFirst        ()  const           { return    First ? First->ToData : 0; }
          TypeD*            GetLast         ()                  { return    Last  ? Last ->ToData : 0; }
    const TypeD*            GetLast         ()  const           { return    Last  ? Last ->ToData : 0; }

          TListAtom<TypeD>* GetFirstAtom    ()                  { return    First; }
    const TListAtom<TypeD>* GetFirstAtom    ()  const           { return    First; }
          TListAtom<TypeD>* GetLastAtom     ()                  { return    Last;  }
    const TListAtom<TypeD>* GetLastAtom     ()  const           { return    Last;  }

          TListAtom<TypeD>* GetAtom         ( int index );
    const TListAtom<TypeD>* GetAtom         ( int index )           const;
          TListAtom<TypeD>* GetAtom         ( const TypeD* todata );
    const TListAtom<TypeD>* GetAtom         ( const TypeD* todata ) const;

                                        // gets an array of pointers to data, for fast direct access
    void                    GetIndexes      ( TArray1<TListAtom<TypeD> *>&  indexes )   const;
    void                    GetIndexes      ( TArray1<TypeD *>&             indexes )   const;
    void                    UpdateIndexes   ( bool force = false ); // !this part is currently not thread / parallel safe, and should be called only once before entering a parallel block!


    TList<TypeD>                            ( const TList<TypeD>& op  );
    TList<TypeD>&           operator    =   ( const TList<TypeD>& op2 );

                                            // array-style access
          TypeD&            operator    ()  ( int index );          // indexed access - returns a reference
    const TypeD&            operator    ()  ( int index )   const;  
          TypeD*            operator    []  ( int index );          // indexed access - returns a pointer
    const TypeD*            operator    []  ( int index )   const;

                            operator bool   ()  const           { return NumInside > 0; }
                            operator int    ()  const           { return NumInside;     }

    TList<TypeD>&           operator    =   ( double op2 )      { return *this; }   // null operator, to allow TArray3< TList > to run ReadFile correctly


private:

    TListAtom<TypeD>*       First;
    TListAtom<TypeD>*       Last;
    int                     NumInside;
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


          TypeD*    operator ()                 ()              { return P ? P->ToData : 0;	}
    const TypeD*    operator ()                 ()  const       { return P ? P->ToData : 0;	}

                    operator bool               ()              { return P != 0;		    }
                    operator bool               ()  const       { return P != 0;		    }
                    operator int                ()              { return Index;			    }
                    operator int                ()  const       { return Index;			    }
                    operator       TypeD*       ()              { return P ? P->ToData : 0;	}
                    operator const TypeD*       ()  const       { return P ? P->ToData : 0;	}
                    operator TListAtom<TypeD>*  ()	            { return P;                 }
                    operator TListAtom<TypeD>*  ()	const       { return P;                 }


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

        if ( p->ToData != 0 )

            delete  p->ToData;

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
void    TList<TypeD>::Append ( const TypeD* todata )
{
OmpCriticalBegin (TListAppend)

if ( IsEmpty () )

    Last    = First         = new TListAtom<TypeD> ( todata );

else {
    Last->Next              = new TListAtom<TypeD> ( todata );
    Last->Next->Previous    = Last;
    Last                    = Last->Next;
    }

NumInside++;
AtomIndexesIsDirty  = true;

OmpCriticalEnd
}


template <class TypeD>
void    TList<TypeD>::Append ( const TList& list )
{
                                        // does not work with itself
if ( &list == this || list.IsEmpty () )
    return;

for ( TListAtom<TypeD>* p = list.First; p != 0; p = p->Next )
    Append ( p->ToData );

UpdateIndexes ( true );
}


template <class TypeD>
bool    TList<TypeD>::Insert ( const TypeD* todata, const TypeD* beforedata )
{
if ( IsEmpty () || beforedata == 0 ) {
    Append ( todata );
    return  true;
    }


TListAtom<TypeD>*   postatom    = GetAtom ( beforedata );

if ( ! postatom )                       // not found the before atom?
    return  false;


OmpCriticalBegin (TListInsert)

TListAtom<TypeD>*   newatom     = new TListAtom<TypeD> ( todata );
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
    indexes[ i ]    = p->ToData;
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

return  *AtomIndexes[ index ]->ToData;
}


template <class TypeD>
const TypeD&    TList<TypeD>::operator() ( int index ) const
{
                                        // hack until all methods are correctly const'ed
const_cast<TList*>( this )->UpdateIndexes ();

return  *AtomIndexes[ index ]->ToData;
}


template <class TypeD>
TypeD*  TList<TypeD>::operator[] ( int index )
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ]->ToData : 0;
}


template <class TypeD>
const TypeD*    TList<TypeD>::operator[] ( int index ) const
{
                                        // hack until all methods are correctly const'ed
const_cast<TList*>( this )->UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ]->ToData : 0;
}


template <class TypeD>
TListAtom<TypeD>*   TList<TypeD>::GetAtom ( int index )
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ] : 0;
}


template <class TypeD>
const TListAtom<TypeD>*   TList<TypeD>::GetAtom ( int index ) const
{
UpdateIndexes ();

return  IsInsideLimits ( index, 0, NumInside - 1 ) ? AtomIndexes[ index ] : 0;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Permutate ( const TypeD *todata1, const TypeD *todata2 )
{
if ( IsEmpty () )
    return;

TListAtom<TypeD>*   p1              = GetAtom ( todata1 );

if ( p1 == 0 )
    return;

TListAtom<TypeD>*   p2              = GetAtom ( todata2 );

if ( p2 == 0 )
    return;


OmpCriticalBegin (TListPermutate)
                                        // just permutating the POINTERS to content!
Permutate ( p1->ToData, p2->ToData );

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
    revlist.Append ( p->ToData );


Reset ( DontDeallocate );

                                        // copy back the reverted temp list
for ( p = revlist.First; p != 0; p = p->Next )
    Append ( p->ToData );


AtomIndexesIsDirty  = true;

UpdateIndexes ( true );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::RemoveAtom ( const TListAtom<TypeD>* tolistatom, DeallocateType deallocate )
{
if ( IsEmpty () || tolistatom == 0 )
    return;

/*                                        // should we take some resources to test if the atom really belongs?
bool                atombelongs = false;

for ( TListAtom<TypeD>* p = First; p != 0; p = p->Next )
    if ( p == tolistatom ) {
        atombelongs = true;
        break;
        }

if ( ! atombelongs )
    return;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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
    delete  tolistatom->ToData;         // first, content if requested by caller

delete  tolistatom;                     // then the list atom itself

                                        // decrement count
NumInside--;
AtomIndexesIsDirty  = true;

OmpCriticalEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TList<TypeD>::Remove ( const TypeD* todata, DeallocateType deallocate )
{
RemoveAtom ( GetAtom ( todata ), deallocate );
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
    RemoveAtom ( First, deallocate );
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
    RemoveAtom ( Last, deallocate );
}


//----------------------------------------------------------------------------
template <class TypeD>
const TListAtom<TypeD>*     TList<TypeD>::GetAtom ( const TypeD* todata )   const
{
if ( IsEmpty () || todata == 0 )
    return  0;


const TListAtom<TypeD>* p       = 0;

OmpCriticalBegin (TListGetAtom)

for ( p = First; p != 0; p = p->Next )
                                        // returns first element with pointer to content equality
    if ( p->ToData == todata )

        break;

OmpCriticalEnd

return  p;
}


template <class TypeD>
TListAtom<TypeD>*  TList<TypeD>::GetAtom ( const TypeD* todata )
{
if ( IsEmpty () || todata == 0 )
    return  0;


TListAtom<TypeD>*   p           = 0;

OmpCriticalBegin (TListGetAtom)

for ( p = First; p != 0; p = p->Next )
                                        // returns first element with pointer to content equality
    if ( p->ToData == todata )

        break;

OmpCriticalEnd

return  p;
}


//----------------------------------------------------------------------------
template <class TypeD>
bool    TList<TypeD>::IsInside ( const TypeD* todata ) const
{
if ( IsEmpty () || todata == 0 )
    return  0;


bool                isinside        = false;

OmpCriticalBegin (TListIsInside)

for ( TListAtom<TypeD>* p = First; p != 0; p = p->Next )
                                        // returns first element with pointer to content equality
    if ( isinside = ( p->ToData == todata ) )

        break;

OmpCriticalEnd

return  isinside;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
