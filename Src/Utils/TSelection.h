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

#include    "TArray1.h"
#include    "TArray2.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class           TStrings;
class           TSelectionIterator;
class           TIteratorSelectedForward;
class           TIteratorSelectedBackward;
class           TIeratorNotSelectedForward;
class           TIeratorNotSelectedBackward;

                                        // used for both values and indexes
constexpr int   SelectionInvalid            = -1;


enum            TSelectionOrder
                {
                OrderSorted,
                OrderTemplate,
                OrderArbitrary,

                NumOrderType,
                DefaultOrder                = OrderSorted,
                };


enum            TSelectionScanFlags
                {
                ScanTextActionSilent        = 0x01,
                ScanTextActionStartFrom1    = 0x02,
                ScanTextActionSet           = 0x10,
                ScanTextActionClear         = 0x20,
                };


enum            TSelectionShiftFlags
                {
                TSelectionShiftIncr         = 0x0001,
                TSelectionShiftDecr         = 0x0002,

                TSelectionShiftReportReset  = 0x0010,
                TSelectionShiftReportSet    = 0x0020,
                TSelectionShiftReportCarry  = 0x0040,   // circular report of the carry
                TSelectionShiftReportStop   = 0x0100,   // stop when one value sets to From or To (butée)


                TSelectionShiftUp           = TSelectionShiftDecr,  // meaning <-> math
                TSelectionShiftDown         = TSelectionShiftIncr,
                TSelectionShiftLeft         = TSelectionShiftDecr,
                TSelectionShiftRight        = TSelectionShiftIncr,
                };


                                        // TSelection looks like a std::set or std::unordered_set, but it can be:
                                        //  - sorted - order of insertion / deletion is not relevant
                                        //  - arbitrary ordered - order of insertion / deletion DOES matter
                                        //  - template ordered - the allowed tokens and their actual ordering are specified by a template (which is actually another TSelection!)
                                        //  - setting & resetting is thread-safe
                                        //  - direct access/test like sel[ index ] is thread-safe
                                        //  - sequential access with appropriate iterators is naturally thread-safe
class   TSelection
{
public:
                    TSelection ();
                    TSelection ( int size, TSelectionOrder ordertype, const TSelection* ordertemplate = 0 );


    friend          TIteratorSelectedForward;
    friend          TIteratorSelectedBackward;
    friend          TIeratorNotSelectedForward;
    friend          TIeratorNotSelectedBackward;


    UINT            SentTo;             // used for message transmission
    UINT            SentFrom;


    int             Size        ()      const                                       { return    ListSet.GetDim1 ();                    }
    int             MaxSet      ()      const                                       { return    IsTemplate () ? NumTemplate : Size (); }
    int             MinValue    ()      const                                       { return    0;                                     }
    int             MaxValue    ()      const                                       { return    Size () - 1;                           }


    bool            IsAllocated    ()   const                                       { return    ListSet.IsAllocated    ();   }
    bool            IsNotAllocated ()   const                                       { return    ListSet.IsNotAllocated ();   }

    bool            IsSorted    ()      const                                       { return    OrderType == OrderSorted;    }
    bool            IsArbitrary ()      const                                       { return    OrderType == OrderArbitrary; }
    bool            IsTemplate  ()      const                                       { return    OrderType == OrderTemplate;  }

    bool            IsAllSet    ()      const                                       { return    NumSet () == MaxSet ();      }
    bool            IsNoneSet   ()      const                                       { return    NumSet () == 0;              }


    void            SetOrderType ( TSelectionOrder ordertype );     // changing the type of selection on the fly
    void            Sort         ();
//  void            Sort         ( TSelection &ordertemplate );
//  void            SetOrderTemplate ( TSelection *ordertemplate );
//  void            ReorderWithTemplate ();


    void            Set     ();
    void            Set     ( int value );
    void            Set     ( int value, bool set );
    void            Set     ( int fromvalue, int tovalue );
    void            Set     ( int fromvalue, int tovalue, bool set );
//  void            Set     ( const TSelection &s );
    void            Set     ( const char* list, bool                    startfrom1, bool verbose = true );
    void            Set     ( const char *list, const TStrings*         dictionary, bool verbose = true );
    void            SetOnly ( int value );


    void            Reset   ();             // resetting current selection, NOT the object itself (see Initialize)
    void            Reset   ( int value );
    void            Reset   ( int fromvalue, int tovalue );
//  void            Reset   ( const TSelection &s );
    void            Reset   ( const char* list, bool                    startfrom1, bool verbose = true );
    void            Reset   ( const char* list, const TStrings*         dictionary, bool verbose = true );

    void            Initialize ();          // resetting object


    void            Invert  ();
    void            Invert  ( int value );
    void            Invert  ( int fromvalue, int tovalue );

    void            Shift               ( int fromvalue, int tovalue, int numshift, TSelectionShiftFlags flags );
    void            ShiftDown           ( int fromvalue, int tovalue, int numshift = 1 )  { Shift ( fromvalue, tovalue, numshift, (TSelectionShiftFlags) ( TSelectionShiftDown | TSelectionShiftReportStop ) ); }
    void            ShiftUp             ( int fromvalue, int tovalue, int numshift = 1 )  { Shift ( fromvalue, tovalue, numshift, (TSelectionShiftFlags) ( TSelectionShiftUp   | TSelectionShiftReportStop ) ); }

    void            Copy                ( int fromvalue, int tovalue, const TSelection* sel2 ); // !sel2 must be of the same size as this!


    int             NumSet              ()                              const       { return    NumSelected; }
    int             NumSet              ( int fromvalue, int tovalue )  const;
    int             NumSetAnd           ( const TSelection &sel2 )      const;      // num sets with the intersection of sel2 - equivalent to a '&'
    int             NumSetOr            ( const TSelection &sel2 )      const;      // num sets with another TSelection       - equivalent to a '|'
    int             NumNotSet           ()                              const       { return    MaxSet () - NumSet (); }
    int             NumNotSet           ( int fromvalue, int tovalue )  const;

                                                                    // optimized fast access
    bool            IsSelected          ( int value )                   const       { return    value < MinValue () || value > MaxValue () ? false : ArraySet[ value ] != 0; }
    bool            IsNotSelected       ( int value )                   const       { return    value < MinValue () || value > MaxValue () ? false : ArraySet[ value ] == 0; }

                                                                    // These methods are now safe to use in multi-threads
    int             FirstSelected       ()                                  const;
    int             FirstSelected       ( int fromvalue, int tovalue = -1 ) const;
    int             LastSelected        ()                                  const;
    int             LastSelected        ( int fromvalue, int tovalue = -1 ) const;
    int             FirstNotSelected    ()                                  const;
    int             FirstNotSelected    ( int fromvalue, int tovalue = -1 ) const;
    int             LastNotSelected     ()                                  const;
    int             LastNotSelected     ( int fromvalue, int tovalue = -1 ) const;

    int             PreviousValue       ( int value )   const;          // handles all cases
    int             NextValue           ( int value )   const;

                                                                                    // index -> value, SelectionInvalid if not set
    int             GetValue            ( int index )                   const       { return    index >= 0 && index < NumSet () ? ListSet[ index ] : SelectionInvalid; }
    int             GetIndex            ( int value )                   const;      // value -> index, SelectionInvalid if not set


    char*           ToText              ( char* formatted, const TStrings*    dictionary, const char* alwaysenumerate = 0 ) const;
    void            Show                ( const char *title = 0 )                                                           const;



                    TSelection          ( const TSelection &op  );
    TSelection&     operator    =       ( const TSelection &op2 );


    bool            operator    []      ( int value )   const                       { return    ArraySet[ value ] != 0; /*IsSelected ( value );*/ }  // optimized fast access
                    operator    bool    ()              const                       { return    NumSet () != 0;     } // cast: return true if at least 1 set
                    operator    int     ()              const                       { return    NumSet ();          } // cast: return the # sets
                    operator    double  ()              const                       { return    NumSet ();          } // cast: return the # sets

    bool            operator    !=      ( const TSelection &op2 )   const;

    TSelection&     operator    +=      ( const TSelection &op2 );          // add a selection
    TSelection&     operator    -=      ( const TSelection &op2 );          // remove a selection
    TSelection&     operator    ^=      ( const TSelection &op2 );          // invert a selection
    TSelection&     operator    &=      ( const TSelection &op2 );          // and a selection
    TSelection      operator    &       ( const TSelection &op2 )   const;  // and a selection
    TSelection      operator    ~       ()                          const;  // invert a selection


private:

    TArray1<int>    ListSet;            // contains the sequential list of all set elements -> used in FirstSelected and the like
    TArray1<UCHAR>  ArraySet;           // an array of bool for direct test/access          -> used in []
    int             NumSelected;

    int             OrderType;

    TArray1<int>    ValueToIndex;       // for template case
    TArray1<int>    IndexToValue;
    int             NumTemplate;

                                        // Centralizing the string analysis for Set or Reset - Also gives more options
    void            ScanTextAction  ( const char* list, const TStrings*    dictionary, TSelectionScanFlags action );
    const int*      GetValues       ()                              const       { return    ListSet.GetArray (); }  // the whole array of values at once, faster as it skips boundary tests

};


//----------------------------------------------------------------------------
                                        // iterator TSelection, safe for multithreading
class   TSelectionIterator
{
public:
                    TSelectionIterator ()                           { Reset (); }
                    TSelectionIterator ( const TSelection& sel )    { Reset (); tosel = &sel; }


    void			Reset      ();

    bool            IsOk                ()	const   { return tosel != 0 && LastSearchIndex != SelectionInvalid; }   // must have be initialized AND current index is valid
    bool            IsNotOk             ()	const   { return tosel == 0 || LastSearchIndex == SelectionInvalid; }

    int             GetIndex            ()  const   { return IsOk () ?  LastSearchIndex : SelectionInvalid;     }
    int             GetValue            ()  const   { return IsOk () ?  LastSearchValue : SelectionInvalid;     }

                    operator bool       ()	const   { return IsOk ();		  }
                    operator int        ()	const   { return LastSearchValue; }             // by safety, to avoid the (bool) cast to ne used for int
                    operator UINT       ()	const   { return (UINT) LastSearchValue; }      // by safety, to avoid the (bool) cast to ne used for uint
                    operator float      ()	const   { return (float) LastSearchValue; }     // by safety, to avoid the (bool) cast to ne used for float
                    operator double     ()	const   { return (double) LastSearchValue; }    // by safety, to avoid the (bool) cast to ne used for double
    int             operator ()         ()	const   { return LastSearchValue; }             // will be  SelectionInvalid  if anything failed, or browsing reached the end


protected:

    const TSelection*   tosel;

    int             LastSearchFromValue;    // not really used anymore
    int             LastSearchToValue;
    int             LastSearchFromIndex;    // !index within ListSet! (nothing to do with ValueToIndex/IndexToValue in Template case)
    int             LastSearchToIndex;
    int             LastSearchValue;
    int             LastSearchIndex;        // points to the last valid search (in ListSet), SelectionInvalid otherwise (failed search or end of search)
};

                                        // Specialized iterators that initialize with the right search
class   TIteratorSelectedForward    :   public  TSelectionIterator
{
public:
                    TIteratorSelectedForward ()                                                           : TSelectionIterator ()         {}
                    TIteratorSelectedForward ( const TSelection& sel )                                    : TSelectionIterator ( sel )    { FirstSelected ( sel ); }
                    TIteratorSelectedForward ( const TSelection& sel, int fromvalue, int tovalue = -1 )   : TSelectionIterator ( sel )    { FirstSelected ( sel, fromvalue, tovalue ); }

    int             FirstSelected       ( const TSelection& sel );
    int             FirstSelected       ( const TSelection& sel, int fromvalue, int tovalue = -1 );
    int             NextSelected        ( UINT step = 1 );

    int             operator    ++      ()                  { return  NextSelected ();          }   // pre
    int             operator    ++      ( int )             { return  NextSelected ();          }   // post
    int             operator    +=      ( UINT step )       { return  NextSelected ( step );    }
};


class   TIteratorSelectedBackward   :   public  TSelectionIterator
{
public:
                    TIteratorSelectedBackward ()                                                          : TSelectionIterator ()         {}
                    TIteratorSelectedBackward ( const TSelection& sel )                                   : TSelectionIterator ( sel )    { LastSelected ( sel ); }
                    TIteratorSelectedBackward ( const TSelection& sel, int fromvalue, int tovalue = -1 )  : TSelectionIterator ( sel )    { LastSelected ( sel, fromvalue, tovalue ); }

    int             LastSelected        ( const TSelection& sel );
    int             LastSelected        ( const TSelection& sel, int fromvalue, int tovalue = -1 );   // if tovalue = -1, fromvalue should be the largest value
    int             PreviousSelected    ( UINT step = 1 );

    int             operator    --      ()                  { return  PreviousSelected ();      }   // pre
    int             operator    --      ( int )             { return  PreviousSelected ();      }   // post
    int             operator    -=      ( UINT step )       { return  PreviousSelected ( step );}
};

                                        // Currently used only for single call FirstNotSelected
class   TIeratorNotSelectedForward  :   public  TSelectionIterator
{
public:
                    TIeratorNotSelectedForward ()                                                         : TSelectionIterator ()         {}
                    TIeratorNotSelectedForward ( const TSelection& sel )                                  : TSelectionIterator ( sel )    { FirstNotSelected ( sel ); }
                    TIeratorNotSelectedForward ( const TSelection& sel, int fromvalue, int tovalue = -1 ) : TSelectionIterator ( sel )    { FirstNotSelected ( sel, fromvalue, tovalue ); }

    int             FirstNotSelected    ( const TSelection& sel );
    int             FirstNotSelected    ( const TSelection& sel, int fromvalue, int tovalue = -1 );
//  int             NextNotSelected     ( UINT step = 1 );
};

                                        // Currently used only for single call LastNotSelected
class   TIeratorNotSelectedBackward :   public  TSelectionIterator
{
public:
                    TIeratorNotSelectedBackward ()                                                        : TSelectionIterator ()         {}
                    TIeratorNotSelectedBackward ( const TSelection& sel )                                 : TSelectionIterator ( sel )    { LastNotSelected ( sel ); }
                    TIeratorNotSelectedBackward ( const TSelection& sel, int fromvalue, int tovalue = -1 ): TSelectionIterator ( sel )    { LastNotSelected ( sel, fromvalue, tovalue ); }

    int             LastNotSelected     ( const TSelection& sel );
    int             LastNotSelected     ( const TSelection& sel, int fromvalue, int tovalue = -1 );   // if tovalue = -1, fromvalue should be the largest value
//  int             PreviousNotSelected ( UINT step = 1 );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
