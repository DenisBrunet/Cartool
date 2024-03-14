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

#include    <stdio.h>
#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "TArray3.h"
#include    "Dialogs.Input.h"
#include    "TSelection.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

                                        // loop through existing values, respecting the order given by from and to
#define ForAllValues(VAR,FROMVALUE,TOVALUE)     for ( int VAR = FROMVALUE, step = FROMVALUE <= TOVALUE ? 1 : -1; VAR != TOVALUE + step; VAR += step )

                                        // loop through all possible values of current sorting type
#define ForAllPossibleSelection                 for ( int index = 0,             value = IsTemplate () ? IndexToValue[ index ] : index, maxindex = MaxSet (); \
                                                      index < maxindex; \
                                                      index++, value = IsTemplate () && index < maxindex ? IndexToValue[ index ] : index )

#define ForAllPossibleSelectionBackward         for ( int index = MaxSet () - 1, i = IsTemplate () ? IndexToValue[ index ] : index; \
                                                      index >= 0; \
                                                      index--, i = IsTemplate () && index >= 0       ? IndexToValue[ index ] : index )


//----------------------------------------------------------------------------

        TSelection::TSelection ()
{
Initialize ();
}

                                        // template can be whatever type
        TSelection::TSelection ( int size, TSelectionOrder ordertype, const TSelection* ordertemplate )
{
Initialize ();


Maxed ( size, 0 );

if ( ordertemplate )
    Maxed ( size, ordertemplate->Size () );

ListSet .Resize ( size );
ArraySet.Resize ( size );
NumSelected         = 0;

OrderType           = ordertype;

NumTemplate         = 0;

                                        // check consistency
if ( IsTemplate () ) {

    if ( ordertemplate == 0 || ordertemplate->NumSet () == 0 )

        OrderType       = DefaultOrder;

    else {
        ValueToIndex.Resize ( size );
        IndexToValue.Resize ( ordertemplate->NumSet() /*size*/ ); // can spare a bit of memory

                                        // set all indexes to SelectionInvalid
        for ( int i = 0; i < size; i++ )
            ValueToIndex[ i ]   = SelectionInvalid;

                                        // construct the correspondance: Value <-> Index
        for ( TIteratorSelectedForward seli ( *ordertemplate ); (bool) seli; ++seli ) {

            ValueToIndex[ seli.GetValue () ] = seli.GetIndex ();
            IndexToValue[ seli.GetIndex () ] = seli.GetValue ();
            }

                                        // also remember this
        NumTemplate         = ordertemplate->NumSet ();
        }
    }


Reset ();
}


void    TSelection::Initialize ()
{
ListSet     .DeallocateMemory ();
ArraySet    .DeallocateMemory ();
NumSelected         = 0;

OrderType           = OrderSorted;

ValueToIndex.DeallocateMemory ();
IndexToValue.DeallocateMemory ();
NumTemplate         = 0;

SentFrom            = 0;
SentTo              = 0;
}


        TSelection::TSelection ( const TSelection &op )
{
ListSet             = op.ListSet;
ArraySet            = op.ArraySet;
NumSelected         = op.NumSelected;

OrderType           = op.OrderType;

ValueToIndex        = op.ValueToIndex;
IndexToValue        = op.IndexToValue;
NumTemplate         = op.NumTemplate;
}


TSelection& TSelection::operator= ( const TSelection &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

                                        // not created, or not the same size? do a blind copy
if ( IsNotAllocated () || Size () != op2.Size () ) {

    ListSet             = op2.ListSet;
    ArraySet            = op2.ArraySet;
    NumSelected         = op2.NumSelected;

    OrderType           = op2.OrderType;

    ValueToIndex        = op2.ValueToIndex;
    IndexToValue        = op2.IndexToValue;
    NumTemplate         = op2.NumTemplate;

    return  *this;
    }

                                        // object already exist, just transfer the values
                                        // check for all cases of receiving and orginating types
if      ( IsSorted () ) {

    if ( op2.IsSorted () ) {            // sorted -> sorted (easy)
        ListSet         = op2.ListSet;
        ArraySet        = op2.ArraySet;
        NumSelected     = op2.NumSelected;
        }
    else {                              // template / arbitrary -> sorted

        Reset ();

        const int*          op2values       = op2.GetValues ();

        for ( int i = 0; i < (int) op2; i++ )
            Set ( op2values[ i ] );
        }
    }

else if ( IsTemplate () ) {             // filter out what is not in receiving template, and sort according to receiving template

    Reset ();

    const int*          op2values       = op2.GetValues ();

    for ( int i = 0; i < (int) op2; i++ )
        Set ( op2values[ i ] );
    }

else { // if ( IsArbitrary () ) {       // arbitrary case is not so picky!
    ListSet             = op2.ListSet;
    ArraySet            = op2.ArraySet;
    NumSelected         = op2.NumSelected;
    }


return  *this;
}


//----------------------------------------------------------------------------
void    TSelection::Show ( const char *title )    const
{
if ( IsNoneSet () ) {
    ShowMessage ( "- Empty -", StringIsEmpty ( title ) ? ( IsSorted () ? "Sorted Selection" : IsTemplate () ? "Templated Selection" : "Arbitrary Selection" ) : title );
    return;
    }


char                buff[ 8 * KiloByte ];
ClearString ( buff );

for ( int i = 0; i < NumSelected; i++ )

    sprintf ( StringEnd ( buff ), "%s%0d", i ? ", " : "", ListSet[ i ] );

ShowMessage ( buff, StringIsEmpty ( title ) ? ( IsSorted () ? "Sorted Selection" : IsTemplate () ? "Templated Selection" : "Arbitrary Selection" ) : title );
}


//----------------------------------------------------------------------------
bool    TSelection::operator!= ( const TSelection &op2 )    const
{
                                        // loose !=
                                        // just a selection not found in the other, whatever the order
int                 mini            = max ( MinValue (), op2.MinValue () );
int                 maxi            = min ( MaxValue (), op2.MaxValue () );


for ( int i = mini; i <= maxi; i++ )
    if ( ArraySet ( i ) != op2.ArraySet ( i ) )
        return  true;

/*                                        // very strict sense of !=
                                        // should be identical and same position (be it ordered or not)
int     maxi    = min ( NumSelected, op2.NumSelected );

for ( int i = mini; i <= maxi; i++ )
    if ( ListSet[ i ] != op2.ListSet[ i ] )
        return  true;
*/
return  false;
}


//----------------------------------------------------------------------------
                                        // add a selection to this one
TSelection& TSelection::operator+= ( const TSelection &op2 )
{
                                        // this is losing the order of op2
//int                 mini            = max ( MinValue (), op2.MinValue () );
//int                 maxi            = min ( MaxValue (), op2.MaxValue () );
//for ( int i = mini; i <= maxi; i++ )
//    if ( op2.ArraySet ( i ) )
//        Set ( i );


const int*          op2values       = op2.GetValues ();

for ( int i = 0; i < (int) op2; i++ )
    Set ( op2values[ i ] );

return  *this;
}

                                        // remove a selection to this one
TSelection& TSelection::operator-= ( const TSelection &op2 )
{
//int                 mini            = max ( MinValue (), op2.MinValue () );
//int                 maxi            = min ( MaxValue (), op2.MaxValue () );
//for ( int i = mini; i <= maxi; i++ )
//    if ( op2.ArraySet ( i ) )
//        Reset ( i );


const int*          op2values       = op2.GetValues ();

for ( int i = 0; i < (int) op2; i++ )
    Reset ( op2values[ i ] );


return  *this;
}

                                        // invert a selection with this one
TSelection& TSelection::operator^= ( const TSelection &op2 )
{
int                 mini            = max ( MinValue (), op2.MinValue () );
int                 maxi            = min ( MaxValue (), op2.MaxValue () );
uchar               seti;


for ( int i = mini; i <= maxi; i++ ) {

    seti    = ArraySet ( i );

    if ( seti ^ op2.ArraySet ( i ) )    {   if ( ! seti )   Set   ( i );    }
    else                                {   if (   seti )   Reset ( i );    }
    }

return  *this;
}

                                        // "and" a selection with this one
TSelection& TSelection::operator&= ( const TSelection &op2 )
{
int                 mini            = max ( MinValue (), op2.MinValue () );
int                 maxi            = min ( MaxValue (), op2.MaxValue () );


for ( int i = mini; i <= maxi; i++ )
    if ( ! op2.ArraySet ( i ) )
        Reset ( i );

return  *this;
}

                                        // "and" a selection with this one
TSelection  TSelection::operator& ( const TSelection &op2 )     const
{
int                 mini            = max ( MinValue (), op2.MinValue () );
int                 maxi            = min ( MaxValue (), op2.MaxValue () );
TSelection          temp ( maxi, OrderSorted ); // IsSorted () && op2.IsSorted ()

temp.Reset ();


for ( int i = mini; i <= maxi; i++ )
    if ( ArraySet ( i ) && op2.ArraySet ( i ) )
        temp.Set ( i );

return  temp;
}

                                        // invert selection
TSelection  TSelection::operator~ ()    const
{
TSelection          temp ( *this );

temp.Invert ();

return  temp;
}


//----------------------------------------------------------------------------
                                        // modify on the fly the type of the selection
void        TSelection::SetOrderType ( TSelectionOrder ordertype )
{
if ( OrderType == ordertype )

    return;


if ( ordertype == OrderTemplate )           // would need some work to allocate, plus passing a template as parameter

    return;


if      ( ordertype == OrderArbitrary ) {   // this type is easy-going, no need to change the order

    OmpCritical ( OrderType   = OrderArbitrary )

    return;
    }

else if ( ordertype == OrderSorted ) {      // sort

    OmpCritical ( OrderType   = OrderSorted )

    Sort ();

    return;
    }
}

                                        // sort numerically
void        TSelection::Sort ()
{
                                        // this is useless, or call Sort with a template?
if ( IsTemplate () )
    return;

                                        // create a temp sorted selection
TSelection          temp ( Size (), OrderSorted );

temp.Reset ();

                                        // temp sorts automatically
for ( int i = 0; i < NumSelected; i++ )
    temp.Set ( ListSet[ i ] );

Reset ();

Copy ( MinValue (), MaxValue (), &temp );
}

/*                                      // note that ordertemplate can also be of the Sorted type
void        TSelection::Sort ( TSelection &ordertemplate )
{
                                        // create a temp, arbitrarily ordered selection
TSelection          temp ( *this );

OrderType   = OrderArbitrary;           // change current type

Reset ();

                                        // transfer values according to the template sequence
for ( TIteratorSelectedForward seli ( ordertemplate ); (bool) seli; ++seli )

    if ( temp.ArraySet ( seli() ) )

        Set ( seli() );

                                        // now complete the transfer with any value not specified in the template
if ( NumSelected < (int) temp )

    for ( TIteratorSelectedForward seli ( temp ); (bool) seli; ++seli )

        if ( ! ArraySet ( seli() ) )    // missed this one?

            Set ( seli() );             // then just add them as they come from original, appended at the end
}
*/

//----------------------------------------------------------------------------
/*
void    TSelection::SetOrderTemplate ( TSelection *ordertemplate )
{
if ( ! ordertemplate ) {                // quit ordering
    OrderType       = DefaultOrder;

    OrderTemplate   = 0;

    return;
    }

                                        // switch to template order mode
OrderType       = OrderTemplate;
                                        // store a pointer to the TSelection, which can also be modified by the caller
OrderTemplate   = ordertemplate;
                                        // reorder
ReorderWithTemplate ();
}


void    TSelection::ReorderWithTemplate ()
{
if ( ! IsTemplate () || ! OrderTemplate )
    return;


TSelection  temp ( *this );             // save my data

temp.SetOrderTemplate ( 0 );            // clear off the template of the copy!

Reset ();                               // erase my data


                                        // transfer values according to the template sequence
for ( TIteratorSelectedForward seli ( *OrderTemplate ); (bool) seli; ++seli )

    if ( temp.ArraySet ( seli() ) )

        Set ( seli() );                 // !!!!!!! CHECK with SET !!!!!!!!

                                        // now complete the transfer with any value not specified in the template
if ( NumSelected < (int) temp )

    for ( TIteratorSelectedForward seli ( temp ); (bool) seli; ++seli )

        if ( ! ArraySet ( seli() ) )    // missed this one?

            Set ( seli() );             // then just add them as they come from original, appended at the end
}
*/

//----------------------------------------------------------------------------
int     TSelection::NumSet ( int fromvalue, int tovalue )   const
{
/*                                        // loop into ListSet
Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );

int                 numset          = 0;


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];
    int                 valuei;

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );


    for ( int i = 0; i < NumSelected; i++ ) {
        valuei      = ValueToIndex[ ListValue[ i ] ];
        if      ( valuei >= fromvaluei        && valuei <= tovaluei )           numset++;
        }
    }
else
    for ( int i = 0; i < NumSelected; i++ )
        if      ( ListValue[ i ] >= fromvalue && ListValue[ i ] <= tovalue )    numset++;
        else if ( IsSorted ()                 && ListValue[ i ] >  tovalue )    break;


return  numset;
*/
                                        // loop into range
Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );


int                 numset          = 0;


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    ForAllValues ( valuei, fromvaluei, tovaluei )
        if ( ArraySet[ IndexToValue[ valuei ] ] )   numset++;
    }
else
    ForAllValues ( value, fromvalue, tovalue )
        if ( ArraySet[               value  ]   )   numset++;


return  numset;
}

                                        // Works with TSelection of different sizes
int     TSelection::NumSetAnd ( const TSelection &sel2 )    const
{
int                 mini            = max ( MinValue (), sel2.MinValue () );
int                 maxi            = min ( MaxValue (), sel2.MaxValue () );
int                 numset          = 0;


for ( TIteratorSelectedForward seli ( *this, mini, maxi ); (bool) seli; ++seli )

    if ( sel2.ArraySet ( seli() ) )

        numset++;

return  numset;
}


int     TSelection::NumNotSet ( int fromvalue, int tovalue )    const
{
Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );


if ( IsTemplate () ) {

    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    int                 support         = tovaluei - fromvaluei + 1;    // support: range within the template

    return  support - NumSet ( fromvalue, tovalue );
    }
else
    return  tovalue - fromvalue + 1 - NumSet ( fromvalue, tovalue );
}

                                        // Works with TSelection of different sizes
int     TSelection::NumSetOr ( const TSelection &sel2 )   const
{
                                        // Working with selection of any sizes
int                 n1              =      NumSet ();
int                 n2              = sel2.NumSet ();
int                 n               = n1 + n2;

                                        // rule out any null intersection
if ( n1 == 0 || n2 == 0 )
    return  n;

                                        // removing only any potential intersection
for ( int i = 0; i < NumSelected; i++ )     // is value from this..
    if ( sel2.IsSelected ( ListSet[ i ] ) ) // ..also found in sel2?
        --n;                                // ..then remove from the double count


return n;
}


//----------------------------------------------------------------------------
void    TSelection::Reset ()
{
OmpCriticalBegin (TSelectionReset)

for ( int i = 0; i < ListSet.GetDim (); i++ )
    ListSet[ i ]    = SelectionInvalid;

ArraySet.ResetMemory ();

NumSelected     = 0;

OmpCriticalEnd
}


void    TSelection::Reset ( int value )
{
                                        // already cleared?
if ( value < MinValue () || value > MaxValue ()
  || ! ArraySet[ value ] )
    return;


int                 i;
int                *tos;

for ( i = 0, tos = &ListSet[ 0 ]; i < NumSelected; i++, tos++ )
    if      (                  *tos                == value )   break;
    else if ( IsSorted ()   && *tos                 > value
           || IsTemplate () && ValueToIndex[ *tos ] > ValueToIndex[ value ] )
        return;


if ( i == NumSelected )                 // shouldn't happen here
    return;


OmpCriticalBegin (TSelectionReset)

if ( NumSelected && i < NumSelected - 1 )
    MoveVirtualMemory ( &ListSet[ 0 ] + i, &ListSet[ 0 ] + i + 1, ( NumSelected - i - 1 ) * ListSet.AtomSize () );

ArraySet[ value ]   = false;            // does not account for any order, just set or reset value in direct access
NumSelected--;

OmpCriticalEnd
}


void    TSelection::Reset ( int fromvalue, int tovalue )
{
                                        // out of range?
if ( fromvalue > MaxValue () || tovalue < MinValue () )
    return;


Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    ForAllValues ( valuei, fromvaluei, tovaluei )
        Reset ( IndexToValue[ valuei ] );
    }
else
    ForAllValues ( value, fromvalue, tovalue )
        Reset ( value );
}


//----------------------------------------------------------------------------
void    TSelection::Set ()
{
ForAllPossibleSelection
    Set ( value );                      // using Set works with all cases
}


void    TSelection::Set ( int value )
{
                                        // already set?
if ( value < MinValue () || value > MaxValue ()
  || ArraySet[ value ] )
    return;


if ( IsSorted () ) {                    // Sorted case will also test for existence
    int                 i;
    int                *tos;

    for ( i = 0, tos = &ListSet[ 0 ]; i < NumSelected; i++, tos++ )
        if      ( *tos == value )     return;   // shouldn't happen here
        else if ( *tos >  value )     break;    // insert here


    OmpCriticalBegin (TSelectionSet)

    if ( NumSelected && i < NumSelected )
        MoveVirtualMemory ( &ListSet[ 0 ] + i + 1, &ListSet[ 0 ] + i, ( NumSelected - i ) * ListSet.AtomSize () );

    ListSet [ i     ]   = value;
    ArraySet[ value ]   = true;
    NumSelected++;

    OmpCriticalEnd
    }

else if ( IsTemplate () ) {             // Sorted & restricted by template

    int                 indexvalue      = ValueToIndex[ value ];

    if ( indexvalue == SelectionInvalid )   // not in the template?
        return;                             // don't insert


    int                 i;
    int                *tos;

    for ( i = 0, tos = &ListSet[ 0 ]; i < NumSelected; i++, tos++ )
        if      (               *tos   ==      value )  return; // shouldn't happen here
        else if ( ValueToIndex[ *tos ] >  indexvalue )  break;  // insert here


    OmpCriticalBegin (TSelectionSet)

    if ( NumSelected && i < NumSelected )
        MoveVirtualMemory ( &ListSet[ 0 ] + i + 1, &ListSet[ 0 ] + i, ( NumSelected - i ) * ListSet.AtomSize () );

    ListSet [ i     ]   = value;        // insert value, at templated place
    ArraySet[ value ]   = true;         // does not account for any order, just set or reset value in direct access
    NumSelected++;

    OmpCriticalEnd
    }

else { // if ( IsArbitrary () ) {
    OmpCriticalBegin (TSelectionSet)

    ListSet [ NumSelected ] = value;    // within boundary, there is always enough room at the end
    ArraySet[ value       ] = true;
    NumSelected++;

    OmpCriticalEnd
    }
}


void    TSelection::Set ( int fromvalue, int tovalue )
{
                                        // out of range?
if ( fromvalue > MaxValue () || tovalue < MinValue () )
    return;


Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    ForAllValues ( valuei, fromvaluei, tovaluei )
        Set ( IndexToValue[ valuei ] );
    }
else
    ForAllValues ( value, fromvalue, tovalue )
        Set ( value );
}


void    TSelection::Set ( int value, bool set )
{
if ( set )  Set   ( value );
else        Reset ( value );
}


void    TSelection::Set ( int fromvalue, int tovalue, bool set )
{
if ( set )  Set   ( fromvalue, tovalue );
else        Reset ( fromvalue, tovalue );
}


//----------------------------------------------------------------------------
void    TSelection::Invert ()
{
OmpCriticalBegin (TSelectionInvert)
                                        // a bit tricky: use the part after the selected section to store future selection
int                 j               = NumSelected;


ForAllPossibleSelection                 // respect the sequence & extend in case of template
    if ( ! ArraySet ( value ) )
        ListSet[ j++ ]  = value;


if ( NumSelected && Size () - NumSelected > 0 )
    MoveVirtualMemory ( &ListSet[ 0 ], &ListSet[ 0 ] + NumSelected, ( Size () - NumSelected ) * ListSet.AtomSize () );


NumSelected     = MaxSet () - NumSelected;

                                        // invert array
for ( int i = 0; i < (int) ArraySet; i++ )
    ArraySet[ i ]   = ! ArraySet[ i ];

OmpCriticalEnd
}


void    TSelection::Invert ( int value )
{
if ( value < MinValue () || value > MaxValue () )
    return;                             // don't do anything if out of range!


Set ( value, ! ArraySet[ value ] );     // invert current value
}


void    TSelection::Invert ( int fromvalue, int tovalue )
{
Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    ForAllValues ( valuei, fromvaluei, tovaluei )
        Invert ( IndexToValue[ valuei ] );
    }
else
    ForAllValues ( value, fromvalue, tovalue )
        Invert ( value );
}


//----------------------------------------------------------------------------
void    TSelection::Set ( const char* list, bool           startfrom1, bool verbose )        
{
ScanTextAction (    list,  0,            
                    TSelectionScanFlags (     ScanTextActionSet 
                                          | ( startfrom1  ? ScanTextActionStartFrom1 : 0                    ) 
                                          | ( verbose     ? 0                        : ScanTextActionSilent ) ) ); 
}


void    TSelection::Set ( const char* list, const TStrings*    dictionary, bool verbose )        
{
ScanTextAction ( list,  dictionary,   TSelectionScanFlags (     ScanTextActionSet                                                 
                                                            | ( verbose ? 0 : ScanTextActionSilent ) ) ); 
}


//----------------------------------------------------------------------------
void    TSelection::Reset ( const char* list, bool           startfrom1, bool verbose )        
{
ScanTextAction (    list,  0,           
                    TSelectionScanFlags (   ScanTextActionClear 
                                          | ( startfrom1 ? ScanTextActionStartFrom1 : 0                    ) 
                                          | ( verbose    ? 0                        : ScanTextActionSilent ) ) );
}


void    TSelection::Reset ( const char* list, const TStrings*    dictionary, bool verbose )        
{
ScanTextAction (    list,  dictionary,  
                    TSelectionScanFlags (    ScanTextActionClear                                                 
                                          | ( verbose ? 0 : ScanTextActionSilent ) ) );
}


//----------------------------------------------------------------------------
void    TSelection::SetOnly ( int value )
{
Reset   ();

Set     ( value ); 
}


//----------------------------------------------------------------------------
                                        // Used by both Set and Reset
void    TSelection::ScanTextAction ( const char* list, const TStrings*    dictionary, TSelectionScanFlags action )
{
if ( StringIsEmpty ( list ) )
    return;


char*               toc;
char*               toc2;
int                 i;
int                 j;
int                 k;
int                 n;
//char*             pi;
int                 maxnames        = dictionary ? dictionary->NumStrings () : 0;
bool                startfrom1      =   IsFlag ( action, ScanTextActionStartFrom1 );
bool                set             =   IsFlag ( action, ScanTextActionSet        );
bool                verbose         = ! IsFlag ( action, ScanTextActionSilent     );


                                        // break string into tokens
TSplitStrings       tokens ( list, UniqueStrings );


for ( int toki = 0; toki < (int) tokens; toki++ ) {

    toc     = tokens[ toki ];

//  ShowMessage ( toc, "Token" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Looking for regex-style strings

                                        // meeting the big star?
    if      ( StringIs ( toc, "*" ) ) {

        if ( set )  Set   ();
        else        Reset ();
        break;                          // there is no more tracks to add or remove
        }

    else if ( StringContains ( (const char*) toc, "*" ) && dictionary ) {
                                        // use the string as a simple regex
       TStringGrep         greppy ( toc, GrepOptionSyntaxShell );

        for ( k = 0; k < maxnames; k++ ) {

            if ( greppy.Matched ( (*dictionary)[ k ] ) )

                Set ( k, set );
            }

        continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have only index values, not names of values
    if ( dictionary == 0 ) {
                                        // directly scan the token
        n   = sscanf ( toc, "%d%*[-]%d", &i, &j );

                                        // adjust to starting index
        if ( startfrom1 )
            i--, j--;

                                        // set according to number of integer retrieved
        if      ( n == 2 )  Set   ( i, j, set );
        else if ( n == 1 )  Set   ( i,    set );
        } // dictionary == 0


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else {                              // dictionary provided

                                        // We have to be smart: some names may include a '-'
                                        // so that ranges like  'a-b'-'c-d'  are legal

        i   = -1;                       // index for first name
        j   = -1;                       // index for second name

                                        // try to find the first electrode
                                        // if strstr matches, keep the longest match
                                        // as "e1" will strstr with "e10" and "e100" f.ex.
        for ( k = 0; k < maxnames; k++ ) {

            if ( StringStartsWith ( toc, (*dictionary)[ k ] )
              && ( i == -1 || StringLength ( (*dictionary)[ k ] ) > StringLength ( (*dictionary)[ i ] ) ) )

                i   = k;
            }

                                            // first electrode not found, abort current token
        if ( i == -1 ) {
            if ( verbose ) {
                char    buff[ 256 ];
                StringCopy  ( buff, "Skipping unknown part:" NewLine Tab, toc );
                ShowMessage ( buff, list, ShowMessageWarning );
                }
            continue;
            }

                                            // now, try to find a second electrode
                                            // the whole token is not the first name found
        if ( StringIsNot ( toc, (*dictionary)[ i ] ) ) {

                                            // start after first name
            toc2    = toc + StringLength ( (*dictionary)[ i ] );

                                            // we now strongly expect an interval separator
            if ( *toc2 != '-' ) {
                if ( verbose ) {
                    char    buff[ 256 ];
                    StringCopy  ( buff, "Skipping unknown part:" NewLine Tab, toc );
                    ShowMessage ( buff, list, ShowMessageWarning );
                    }
                continue;
                }

                                            // skip this "-"
            toc2++;

            for ( k = 0; k < maxnames; k++ ) {
                                            // second part now has to match exactly another name
                if ( StringIs ( toc2, (*dictionary)[ k ] ) ) {
                    j   = k;
                    break;
                    }
                }

                                            // the second part of interval was not found...
            if ( j == -1 ) {
                if ( verbose ) {
                    char    buff[ 256 ];
                    StringCopy  ( buff, "Skipping unknown part:" NewLine Tab, toc2 );
                    ShowMessage ( buff, list, ShowMessageWarning );
                    }
                continue;
                }
            }

//      DBGV2 ( i, j, "Scan: Index Range" );

                                        // set according to number of names retrieved
        if ( j >=  0 )      Set   ( i, j, set );
        else                Set   ( i,    set );
        } // dictionary != 0

    } // for token

}


//----------------------------------------------------------------------------
char*   TSelection::ToText ( char* formatted, const TStrings*    dictionary, const char* alwaysenumerate )  const
{
ClearString ( formatted );

if ( ! NumSelected )
    return  formatted;


int                 maxnames        = dictionary ? dictionary->NumStrings () : 0;
bool                first           = true;
int                 direction       = 0;
int                 from;
int                 to;
char                buff[ 256 ];

                                        // scan until the given limit
for ( int i = 0; i < NumSelected; i++ ) {

    if ( first ) {                      // first of a series
        from    = to    = ListSet[ i ];
        first   = false;
        }
/*
    if ( i < NumSelected - 1 && ListSet[ i + 1 ] == to + 1 ) {
        to++;
        continue;
        }
*/

    if ( i < NumSelected - 1 ) {
                                        // set initial direction - can be ascending or descending
        if ( direction == 0 && ! IsSorted () && ListSet[ i + 1 ] == to - 1 )    direction = -1;
        if ( direction == 0 &&                  ListSet[ i + 1 ] == to + 1 )    direction =  1;

                                        // we still go in the right direction?
                                        // check for forced enumeration
        if ( ListSet[ i + 1 ] == to + direction
          && (    StringIsEmpty ( alwaysenumerate )
               || StringIsNot   ( alwaysenumerate, "*" ) && dictionary && ! IsStringAmong ( (*dictionary)[ to + ( direction < 0 ? 0 : 1 ) ], alwaysenumerate )
              ) ) {

            to += direction;
            continue;
            }
        }

                                        // add a space in between
    if ( StringIsNotEmpty ( formatted ) )
        StringAppend ( formatted, " " );

                                        // write first limit
    if ( dictionary && from < maxnames )    StringAppend    ( formatted, (*dictionary)[ from ] );
    else                                    StringAppend    ( formatted, IntegerToString ( buff, from + 1 ) );

                                        // write second limit, if any
    if ( from != to ) {
                                        // case of a step of one, don't generate an underscore
        StringAppend ( formatted, to == from + direction ? " " : "-" );

        if ( dictionary && to < maxnames )  StringAppend    ( formatted, (*dictionary)[ to ] );
        else                                StringAppend    ( formatted, IntegerToString ( buff, to + 1 ) );
        }

                                        // ready for next part?
    first       = true;                 // go!
    direction   = 0;
    }


//ShowMessage ( formatted, "formatted" );
return  formatted;
}


//----------------------------------------------------------------------------
                                        // sel2 must be of the same size
void    TSelection::Copy ( int fromvalue, int tovalue, const TSelection* sel2 )
{
if ( ! sel2 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check global limits
int                 minvalue        = max ( MinValue (), sel2->MinValue () );
int                 maxvalue        = min ( MaxValue (), sel2->MaxValue () );

Clipped ( fromvalue, tovalue, minvalue, maxvalue );


if ( IsTemplate () ) {
                                        // do the counting in the index space
    int                 fromvaluei      = ValueToIndex[ fromvalue ];
    int                 tovaluei        = ValueToIndex[ tovalue   ];
    int                 value;

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );


    ForAllValues ( valuei, fromvaluei, tovaluei ) {

        value   = IndexToValue[ valuei ];

        if      (   sel2->ArraySet ( value ) && ! ArraySet ( value ) )  Set   ( value );
        else if ( ! sel2->ArraySet ( value ) &&   ArraySet ( value ) )  Reset ( value );
        }
    }
else
    ForAllValues ( value, fromvalue, tovalue )

        if      (   sel2->ArraySet ( value ) && ! ArraySet ( value ) )  Set   ( value );
        else if ( ! sel2->ArraySet ( value ) &&   ArraySet ( value ) )  Reset ( value );
}


//----------------------------------------------------------------------------
void    TSelection::Shift ( int fromvalue, int tovalue, int numshift, TSelectionShiftFlags flags )
{
Clipped ( fromvalue, tovalue, MinValue (), MaxValue () );

if ( fromvalue == tovalue
  || numshift < 1         )

    return;


int                 fromvaluei;
int                 tovaluei;
int                 valuei;

if ( IsTemplate () ) {                  // we're going to work directly on the indexes

    fromvaluei  = ValueToIndex[ fromvalue ];
    tovaluei    = ValueToIndex[ tovalue   ];

    Clipped ( fromvaluei, tovaluei, 0, NumTemplate - 1 );

    if ( fromvaluei == tovaluei )
        return;

                                        // convert back to values, by safety
    fromvalue   = IndexToValue[ fromvaluei ];
    tovalue     = IndexToValue[ tovaluei   ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                saved;
bool                reportstop      = IsFlag ( flags, TSelectionShiftReportStop  );
bool                reportcarry     = IsFlag ( flags, TSelectionShiftReportCarry );
bool                reportreset     = IsFlag ( flags, TSelectionShiftReportReset );
bool                reportset       = IsFlag ( flags, TSelectionShiftReportSet   );
int                 n;
//int                 value;
int                 previousvalue;
int                 nextvalue;

                                        // have to deal in 2 times with ArraySet:
                                        // resetting old values, setting new ones
                                        // otherwise, a wrong set/reset can happen due to ordering
if ( IsFlag ( flags, TSelectionShiftDecr ) ) {

    for ( n = 0; n < numshift; n++ ) {

        saved   = ArraySet ( fromvalue );

        if ( saved )
            if ( reportstop )   break;
            else                Reset ( fromvalue );


        OmpCriticalBegin (TSelectionShift)

        if ( IsTemplate () )

            for ( int i = 0; i < NumSelected; i++ ) {

                valuei      = ValueToIndex[ ListSet[ i ] ];

                if ( valuei >  fromvaluei && valuei <= tovaluei ) {
                    previousvalue               = PreviousValue ( ListSet[ i ] );
                    ArraySet[ ListSet[ i ]  ]  &= 0xFE;                 // clear old value
                    ArraySet[ previousvalue ]  |= 0x02;                 // set new value
                    ListSet[ i ]                = previousvalue;        // update list
                    }
                }
        else

            for ( int i = 0; i < NumSelected; i++ )

                if ( ListSet[ i ] >  fromvalue && ListSet[ i ] <= tovalue ) {
                    previousvalue               = PreviousValue ( ListSet[ i ] );
                    ArraySet[ ListSet[ i ]  ]  &= 0xFE;                 // clear old value
                    ArraySet[ previousvalue ]  |= 0x02;                 // set new value
                    ListSet[ i ]                = previousvalue;        // update list
                    }

                                        // transform new values into regular values
        ForAllValues ( value, fromvalue, tovalue )
            ArraySet[ value ]   = (bool) ArraySet[ value ];

        OmpCriticalEnd

                                        // then see what to set or reset
        if      ( reportcarry )     Set   ( tovalue, saved );
        else if ( reportreset )     Reset ( tovalue );
        else if ( reportset   )     Set   ( tovalue );
        }
    }

else { // TSelectionShiftIncr

    for ( n = 0; n < numshift; n++ ) {

        saved   = ArraySet ( tovalue );

        if ( saved )
            if ( reportstop )   break;
            else                Reset ( tovalue );


        OmpCriticalBegin (TSelectionShift)

        if ( IsTemplate () )

            for ( int i = 0; i < NumSelected; i++ ) {

                valuei      = ValueToIndex[ ListSet[ i ] ];

                if ( valuei >= fromvaluei && valuei <  tovaluei ) {
                    nextvalue                   = NextValue ( ListSet[ i ] );
                    ArraySet[ ListSet[ i ] ]   &= 0xFE;                 // clear old value
                    ArraySet[ nextvalue ]      |= 0x02;                 // set new value
                    ListSet[ i ]                = nextvalue;            // update list
                    }
                }
        else

            for ( int i = 0; i < NumSelected; i++ )

                if ( ListSet[ i ] >= fromvalue && ListSet[ i ] <  tovalue ) {
                    nextvalue                   = NextValue ( ListSet[ i ] );
                    ArraySet[ ListSet[ i ] ]   &= 0xFE;                 // clear old value
                    ArraySet[ nextvalue ]      |= 0x02;                 // set new value
                    ListSet[ i ]                = nextvalue;            // update list
                    }

                                        // transform new values into regular values
        ForAllValues ( value, fromvalue, tovalue )
            ArraySet[ value ]   = (bool) ArraySet[ value ];

        OmpCriticalEnd

                                        // then see what to set or reset
        if      ( reportcarry )     Set   ( fromvalue, saved );
        else if ( reportreset )     Reset ( fromvalue );
        else if ( reportset   )     Set   ( fromvalue );
        }
    }
}

                                        // these 2 functions handle all cases
int     TSelection::PreviousValue ( int value )     const
{
if ( IsTemplate () ) {

    int                 indexvalue      = ValueToIndex[ value ];

    if ( indexvalue == SelectionInvalid || indexvalue <= 0 )            // not in the template, or first value
        return  SelectionInvalid;

    return  IndexToValue[ indexvalue - 1 ];
    }
else
    return  value > MinValue () ? --value : SelectionInvalid;
}


int     TSelection::NextValue ( int value )         const
{
if ( IsTemplate () ) {

    int                 indexvalue      = ValueToIndex[ value ];

    if ( indexvalue == SelectionInvalid || indexvalue >= MaxSet () - 1 )// not in the template, or last value
        return  SelectionInvalid;

    return  IndexToValue[ indexvalue + 1 ];
    }
else
    return  value < MaxValue () ? ++value : SelectionInvalid;
}


//----------------------------------------------------------------------------
                                        // Multi-thread safe wrapper functions
int     TSelection::FirstSelected ()    const
{
TIteratorSelectedForward    firstseli ( *this );

return  firstseli();
}


int     TSelection::FirstSelected ( int fromvalue, int tovalue )    const
{
TIteratorSelectedForward    firstseli ( *this, fromvalue, tovalue );

return  firstseli();
}


int     TSelection::LastSelected () const
{
TIteratorSelectedBackward   lastseli  ( *this );

return  lastseli();
}


int     TSelection::LastSelected ( int fromvalue, int tovalue )    const
{
TIteratorSelectedBackward   lastseli  ( *this, fromvalue, tovalue );

return  lastseli();
}


int     TSelection::FirstNotSelected ()    const
{
TIeratorNotSelectedForward  firstnotseli ( *this );

return  firstnotseli();
}


int     TSelection::FirstNotSelected ( int fromvalue, int tovalue )    const
{
TIeratorNotSelectedForward  firstnotseli ( *this, fromvalue, tovalue );

return  firstnotseli();
}


int     TSelection::LastNotSelected () const
{
TIeratorNotSelectedBackward lastnotseli  ( *this );

return  lastnotseli();
}


int     TSelection::LastNotSelected ( int fromvalue, int tovalue )    const
{
TIeratorNotSelectedBackward lastnotseli  ( *this, fromvalue, tovalue );

return  lastnotseli();
}


//----------------------------------------------------------------------------
int     TSelection::GetIndex ( int value )  const
{
if ( IsNoneSet () )
    return  SelectionInvalid;


for ( int i = 0; i < NumSelected; i++ )
    if ( ListSet[ i ] == value )
        return  i;


return  SelectionInvalid;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void	TSelectionIterator::Reset ()
{
tosel               = 0;

LastSearchFromValue = SelectionInvalid;
LastSearchToValue   = SelectionInvalid;
LastSearchValue     = SelectionInvalid;
LastSearchFromIndex = SelectionInvalid;
LastSearchToIndex   = SelectionInvalid;
LastSearchIndex     = SelectionInvalid;
}


//----------------------------------------------------------------------------
int     TIteratorSelectedForward::FirstSelected ( const TSelection& sel )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;
                                        // set full range of values/indexes
LastSearchFromValue = tosel->MinValue ();
LastSearchToValue   = tosel->MaxValue ();


if ( tosel->IsNoneSet () ) {

    Reset ();

    return  SelectionInvalid;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // list is correctly ordered, so take the first element
LastSearchFromIndex = 0;
LastSearchToIndex   = tosel->NumSelected - 1;
LastSearchIndex     = LastSearchFromIndex;
LastSearchValue     = tosel->ListSet[ LastSearchIndex ];

return  LastSearchValue;
}


int     TIteratorSelectedForward::FirstSelected ( const TSelection& sel, int fromvalue, int tovalue )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;


if ( tovalue == -1 )                    // setting a default value?
    tovalue = tosel->IsTemplate () ? tosel->IndexToValue[ tosel->MaxSet () - 1 ] : tosel->MaxValue ();


CheckOrder ( fromvalue, tovalue );

                                        // out of bounds?
if ( fromvalue > tosel->MaxValue () || tovalue < tosel->MinValue () )
    return  SelectionInvalid;


LastSearchFromValue = fromvalue;
LastSearchToValue   = tovalue;

Clipped ( LastSearchFromValue, LastSearchToValue, tosel->MinValue (), tosel->MaxValue () );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Find range of indexes in the ListSet list, that satisfy the range condition

if ( tosel->IsTemplate () ) {
                                        // do the range test in the index space
    int                 fromvaluei      = tosel->ValueToIndex[ LastSearchFromValue ];
    int                 tovaluei        = tosel->ValueToIndex[ LastSearchToValue   ];
    int                 valuei;

    Clipped ( fromvaluei, tovaluei, 0, tosel->NumTemplate - 1 );


    for ( int i = 0; i < tosel->NumSelected; i++ ) {

        valuei      = tosel->ValueToIndex[ tosel->ListSet[ i ] ];

        if ( valuei >= fromvaluei && valuei <= tovaluei ) {     // if within range, store range of indexes

            if ( LastSearchFromIndex == SelectionInvalid )  LastSearchFromIndex = i;

            LastSearchToIndex   = i;
            }
        else if ( LastSearchFromIndex >= 0                      // range exists? exit, else continue the search
                || valuei > tovaluei        )                   // sure to fail?
            break;
        }
    }
else

    for ( int i = 0; i < tosel->NumSelected; i++ ) {

        if ( IsInsideLimits ( tosel->ListSet[ i ], LastSearchFromValue, LastSearchToValue ) ) {   // if within range, store range of indexes

            if ( LastSearchFromIndex == SelectionInvalid )  LastSearchFromIndex = i;

            LastSearchToIndex   = i;
            }
        else if ( LastSearchFromIndex >= 0                          // range exists? exit, else continue the search
                || tosel->IsSorted () && tosel->ListSet[ i ] > LastSearchToValue ) // sure to fail?
            break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Return first element within range, if any

                                        // no elements within requested range?
if ( LastSearchFromIndex == SelectionInvalid ) {

    Reset ();

    return  SelectionInvalid;
    }

                                        // first element within range
LastSearchIndex     = LastSearchFromIndex;
LastSearchValue     = tosel->ListSet[ LastSearchIndex ];

return  LastSearchValue;
}


int     TIteratorSelectedForward::NextSelected ( UINT step )
{
                                        // initialization failed or loop ended?
if ( IsNotOk () ) {

    Reset ();

    return  SelectionInvalid;
    }

                                        // update index with proper step
LastSearchIndex    += step;

                                        // check range of indexes (of ListSet)
if ( IsInsideLimits ( LastSearchIndex, LastSearchFromIndex, LastSearchToIndex ) ) {

    LastSearchValue     = tosel->ListSet[ LastSearchIndex ];
    return  LastSearchValue;
    }

                                        // end it all
Reset ();

return  SelectionInvalid;
}


//----------------------------------------------------------------------------
int     TIteratorSelectedBackward::LastSelected ( const TSelection& sel )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;
                                        // set full range of values/indexes
LastSearchFromValue = tosel->MinValue ();
LastSearchToValue   = tosel->MaxValue ();


if ( tosel->IsNoneSet () ) {

    Reset ();

    return  SelectionInvalid;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // list is correctly ordered, so take the first element
LastSearchFromIndex = 0;
LastSearchToIndex   = tosel->NumSelected - 1;
LastSearchIndex     = LastSearchToIndex;
LastSearchValue     = tosel->ListSet[ LastSearchIndex ];

return  LastSearchValue;
}


int     TIteratorSelectedBackward::LastSelected ( const TSelection& sel, int fromvalue, int tovalue )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;

                                        // initial starting point is not correct
if ( tovalue == -1 && fromvalue < tosel->MinValue () )
    return  SelectionInvalid;


if ( tovalue == -1 )                    // setting a default value?
    tovalue = tosel->IsTemplate () ? tosel->IndexToValue[ 0 ] : tosel->MinValue ();


CheckOrder ( fromvalue, tovalue );

                                        // out of bounds?
if ( fromvalue > tosel->MaxValue () || tovalue < tosel->MinValue () )
    return  SelectionInvalid;


LastSearchFromValue = fromvalue;
LastSearchToValue   = tovalue;

Clipped ( LastSearchFromValue, LastSearchToValue, tosel->MinValue (), tosel->MaxValue () );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Find range of indexes in the ListSet list, that satisfy the range condition

if ( tosel->IsTemplate () ) {
                                        // do the range test in the index space
    int                 fromvaluei      = tosel->ValueToIndex[ LastSearchFromValue ];
    int                 tovaluei        = tosel->ValueToIndex[ LastSearchToValue   ];
    int                 valuei;

    Clipped ( fromvaluei, tovaluei, 0, tosel->NumTemplate - 1 );


    for ( int i = tosel->NumSelected - 1; i >= 0; i-- ) {

        valuei      = tosel->ValueToIndex[ tosel->ListSet[ i ] ];

        if ( valuei >= fromvaluei && valuei <= tovaluei ) {     // if within range, store range of indexes

            if ( LastSearchToIndex == SelectionInvalid )    LastSearchToIndex = i;

            LastSearchFromIndex = i;
            }
        else if ( LastSearchToIndex >= 0                        // range exists? exit, else continue the search
                || valuei < fromvaluei      )                   // sure to fail?
            break;
        }
    }
else

    for ( int i = tosel->NumSelected - 1; i >= 0; i-- ) {

        if ( IsInsideLimits ( tosel->ListSet[ i ], LastSearchFromValue, LastSearchToValue ) ) {   // if within range, store range of indexes

            if ( LastSearchToIndex == SelectionInvalid )    LastSearchToIndex = i;

            LastSearchFromIndex = i;
            }
        else if ( LastSearchToIndex >= 0                        // range exists? exit, else continue the search
                || tosel->IsSorted () && tosel->ListSet[ i ] < LastSearchFromValue )  // sure to fail?
            break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Return first element within range, if any

                                        // no elements within requested range?
if ( LastSearchToIndex == SelectionInvalid ) {

    Reset ();

    return  SelectionInvalid;
    }

                                        // first element within range
LastSearchIndex     = LastSearchToIndex;
LastSearchValue     = tosel->ListSet[ LastSearchIndex ];

return  LastSearchValue;
}


int     TIteratorSelectedBackward::PreviousSelected ( UINT step )
{
if ( IsNotOk () ) {

    Reset ();

    return  SelectionInvalid;
    }

                                        // update index with proper step
LastSearchIndex    -= step;

                                        // check range of indexes (of ListSet)
if ( IsInsideLimits ( LastSearchIndex, LastSearchFromIndex, LastSearchToIndex ) ) {

    LastSearchValue     = tosel->ListSet[ LastSearchIndex ];
    return  LastSearchValue;
    }

                                        // end it all
Reset ();

return  SelectionInvalid;
}


//----------------------------------------------------------------------------
int     TIeratorNotSelectedForward::FirstNotSelected ( const TSelection& sel )
{
return  FirstNotSelected ( sel, sel.IsTemplate () ? sel.IndexToValue[ 0 ] : sel.MinValue () );
}

                                        // !LastSearchIndex will contain either the index of template, or the value!
int     TIeratorNotSelectedForward::FirstNotSelected ( const TSelection& sel, int fromvalue, int tovalue )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;


if ( tovalue == -1 )                    // setting a default value?
    tovalue = tosel->IsTemplate () ? tosel->IndexToValue[ tosel->MaxSet () - 1 ] : tosel->MaxValue ();


CheckOrder ( fromvalue, tovalue );

                                        // out of bounds?
if ( fromvalue > tosel->MaxValue () || tovalue < tosel->MinValue () )
    return  SelectionInvalid;


LastSearchFromValue = fromvalue;
LastSearchToValue   = tovalue;

Clipped ( LastSearchFromValue, LastSearchToValue, tosel->MinValue (), tosel->MaxValue () );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( tosel->IsAllSet () ) {

    Reset ();

    return  SelectionInvalid;
    }


if ( tosel->IsTemplate () ) {
                                        // use ordering from the index space
    int                 fromvaluei      = tosel->ValueToIndex[ LastSearchFromValue ];
    int                 tovaluei        = tosel->ValueToIndex[ LastSearchToValue   ];

    Clipped ( fromvaluei, tovaluei, 0, tosel->NumTemplate - 1 );

    ForAllValues ( valuei, fromvaluei, tovaluei )

        if ( ! tosel->ArraySet[ tosel->IndexToValue[ valuei ] ] ) {

            LastSearchIndex     = valuei;   // store index of template
            LastSearchValue     = tosel->IndexToValue[ valuei ];
            return  LastSearchValue;
            }
    }
else
    ForAllValues ( value, LastSearchFromValue, LastSearchToValue )

        if ( ! tosel->ArraySet[                      value  ]   ) {

            LastSearchIndex     = 0;        // !not a valid index, as it does not exist, but set as to be different from  SelectionInvalid  for IsOk()!
            LastSearchValue     = value;
            return  LastSearchValue;
            }


Reset ();

return  SelectionInvalid;
}

/*
                                        // !NOT up-to-date!
int     TIeratorNotSelectedForward::NextNotSelected ( int step )
{
                                        // code is identical weither sorted or not
                                        // because the complement of Selected is implicitely of a sorted kind
//for ( int i = LastSearchIndex + 1; i < Size (); i++ )

for ( int index = ( IsTemplate () ? ValueToIndex[ LastSearchIndex ] : LastSearchIndex ) + 1, i = IsTemplate () ? IndexToValue[ index ] : index, maxindex = MaxSet (); \
    index < maxindex; \
    index++, i = IsTemplate () && index < maxindex ? IndexToValue[ index ] : index )

    if ( i >= LastSearchFromValue && i <= LastSearchToValue && ! ArraySet ( i ) )
        if ( ! --step ) {
            LastSearchIndex = i;
            return  i;
            }


return  SelectionInvalid;
}
*/

//----------------------------------------------------------------------------
int     TIeratorNotSelectedBackward::LastNotSelected ( const TSelection& sel )
{
return  LastNotSelected ( sel, sel.IsTemplate () ? sel.IndexToValue[ sel.NumTemplate - 1 ] : sel.MaxValue () );
}

                                        // !LastSearchIndex will contain either the index of template, or the value!
int     TIeratorNotSelectedBackward::LastNotSelected ( const TSelection& sel, int fromvalue, int tovalue )
{
Reset ();

if ( sel.IsNotAllocated () )
    return  SelectionInvalid;

tosel               = &sel;

                                        // initial starting point is not correct
if ( tovalue == -1 && fromvalue < tosel->MinValue () )
    return  SelectionInvalid;


if ( tovalue == -1 )                    // setting a default value?
    tovalue = tosel->IsTemplate () ? tosel->IndexToValue[ 0 ] : tosel->MinValue ();


CheckOrder ( fromvalue, tovalue );

                                        // out of bounds?
if ( fromvalue > tosel->MaxValue () || tovalue < tosel->MinValue () )
    return  SelectionInvalid;


LastSearchFromValue = fromvalue;
LastSearchToValue   = tovalue;

Clipped ( LastSearchFromValue, LastSearchToValue, tosel->MinValue (), tosel->MaxValue () );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( tosel->IsAllSet () ) {

    Reset ();

    return  SelectionInvalid;
    }


if ( tosel->IsTemplate () ) {
                                        // use ordering from the index space
    int                 fromvaluei      = tosel->ValueToIndex[ LastSearchFromValue ];
    int                 tovaluei        = tosel->ValueToIndex[ LastSearchToValue   ];

    Clipped ( fromvaluei, tovaluei, 0, tosel->NumTemplate - 1 );

    ForAllValues ( valuei, tovaluei, fromvaluei )   // !backward!

        if ( ! tosel->ArraySet[ tosel->IndexToValue[ valuei ] ] ) {

            LastSearchIndex     = valuei;   // store index of template
            LastSearchValue     = tosel->IndexToValue[ valuei ];
            return  LastSearchValue;
            }
    }
else
    ForAllValues ( value, LastSearchToValue, LastSearchFromValue )  // !backward!

        if ( ! tosel->ArraySet[                      value  ]   ) {

            LastSearchIndex     = 0;        // !not a valid index, as it does not exist, but set as to be different from  SelectionInvalid  for IsOk()!
            LastSearchValue     = value;
            return  LastSearchValue;
            }


Reset ();

return  SelectionInvalid;
}

/*
                                        // !NOT up-to-date!
int     TIeratorNotSelectedBackward::PreviousNotSelected ( int step )
{
                                        // code is identical weither sorted or not
                                        // because the complement of Selected is implicitely of a sorted kind
//for ( int i = LastSearchIndex - 1; i >= 0; i-- )

for ( int index = ( IsTemplate () ? ValueToIndex[ LastSearchIndex ] : LastSearchIndex ) - 1, i = IsTemplate () ? IndexToValue[ index ] : index; \
    index >= 0; \
    index--, i = IsTemplate () && index >= 0 ? IndexToValue[ index ] : index )

    if ( i >= LastSearchFromValue && i <= LastSearchToValue && ! ArraySet ( i ) )
        if ( ! --step ) {
            LastSearchIndex = i;
            return  i;
            }


return  SelectionInvalid;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
