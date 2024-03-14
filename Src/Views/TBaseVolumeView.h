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

#include    "TSelection.h"
#include    "TInterval.h"
#include    "OpenGL.Geometry.h"

#include    "TVolumeDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handling slices for volume display
enum    {
        OffsetMinSlice = 0,
        OffsetMaxSlice,
        OffsetSummarySlice,
        NumSlicesAdded
        };


class   TSliceSelection
{
public:
    inline                  TSliceSelection ();
    inline                  TSliceSelection ( long limi, long lima );


    inline int              GetNumSlices   ()       const   { return    Select.NumSet ( LimitMin, LimitMax ); }
    inline int              GetTotalSlices ()       const   { return    Select.NumSet (); }
    inline int              GetLimitMin    ()       const   { return    LimitMin; }
    inline int              GetLimitMax    ()       const   { return    LimitMax; }
    inline int              GetRange       ()       const   { return    Interval.GetLength (); }


    inline TSelection*      GetSelection   ()               { return   &Select; }


    inline void             ShiftFirstSlice ( int ds );
    inline void             ShiftLastSlice  ( int ds );
    inline void             SetNumSlices    ( int n  );
    inline void             AddNumSlices    ( int dn );     // add or remove (if negative) a number of slices

    inline void             ShowMinSlice    ( bool b )      { Select.Set ( MinSlice,     b ); }
    inline void             ShowMaxSlice    ( bool b )      { Select.Set ( MaxSlice,     b ); }
    inline void             ShowSummarySlice( bool b )      { Select.Set ( SummarySlice, b ); }


    inline bool             IsSlice        ( int s )        const   { return    IsInsideLimits ( s, LimitMin, LimitMax ) && Select[ s ]; }
                                        // return the flag, or test the flag and the right index
    inline bool             IsMinSlice     ( int i = -1 )   const   { return    Select[ MinSlice     ] && ( i == -1 || i == MinSlice     ) ; }
    inline bool             IsMaxSlice     ( int i = -1 )   const   { return    Select[ MaxSlice     ] && ( i == -1 || i == MaxSlice     ) ; }
    inline bool             IsSummarySlice ( int i = -1 )   const   { return    Select[ SummarySlice ] && ( i == -1 || i == SummarySlice ) ; }


    inline                  TSliceSelection     ( const TSliceSelection &op  );
    inline TSliceSelection& operator    =       ( const TSliceSelection &op2 );


protected:

    TSelection              Select;
    TInterval               Interval;
    int                     NumSlices;

    int                     LimitMin;   // range of indexes
    int                     LimitMax;
    int                     MinSlice;   // indexes for Select
    int                     MaxSlice;
    int                     SummarySlice;


    inline void             IntervalToSelect    ();
    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Class that gathers common data & funtions for volume views, currently MRIs and ESI / Inverse


enum    SliceModeType
        {                               // slices order: Corronal -> Transverse -> Sagittal (our historical default for X, Y, Z cutting planes)
        SliceModeNone,
        SliceModeCor,
        SliceModeTra,
        SliceModeSag,
        NumSliceMode
        };


class   TBaseVolumeView
{
public:
    inline                  TBaseVolumeView ();


    SliceModeType           SliceMode;
    TSliceSelection         Slices              [ 3 ];
    int                     SlicesPerX;
    int                     SlicesPerY;
    int                     SeqPerX;
    int                     SeqPerY;

    int                     SliceModeToXYZ      [ NumSliceMode ];

    TGLMatrix               CTSMatrix           [ 3 ];  // transforms to Coronal, Transverse and Sagittal

    TGLMatrix               SlicesRotMatrix     [ 3 ];  // transforms for the slices
    TGLMatrix               SlicesSummaryMatrix [ 3 ];  // transforms for the summary in slice mode

//  TStateVariables         SavedState;                 // if used here, then remove from TVolumeView and TInverseView


    inline void             InitClipPlanes   ( TGLClipPlane ClipPlane[ 3 ] );

    inline void             SetSliceMatrices ( const TVolumeDoc* mridoc, const TPointFloat& modelcenter, float modelradius );

    inline bool             IsSliceModeX    ()              const   { return SliceModeToXYZ[ SliceMode ] == 0; }
    inline bool             IsSliceModeY    ()              const   { return SliceModeToXYZ[ SliceMode ] == 1; }
    inline bool             IsSliceModeZ    ()              const   { return SliceModeToXYZ[ SliceMode ] == 2; }

    inline int              CorronalIndex   ()              const   { return SliceModeToXYZ[ 1 ]; }
    inline int              TransverseIndex ()              const   { return SliceModeToXYZ[ 2 ]; }
    inline int              SagittalIndex   ()              const   { return SliceModeToXYZ[ 3 ]; }

    inline int              CtsToXyz        ( int cts )     const   { return SliceModeToXYZ[ cts + 1 ]; }

    inline void             GetSliceSize    ( TVolumeDoc* MRIDoc, double &x, double &y )   const;
    inline bool             SetNumSlices    ( owlwparam w, bool ShiftKey );
    inline void             MoveSlice       ( TBaseDoc *BaseDoc, owlwparam w, bool ShiftKey );
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TSliceSelection::TSliceSelection ()
{
NumSlices   = LimitMin  = LimitMax  = MinSlice  = MaxSlice  = SummarySlice  = 0;
}


        TSliceSelection::TSliceSelection ( long limi, long lima )
{
LimitMin        = limi;                 // between the 2 values, boundaries included
LimitMax        = lima;
                                        // store these indexes
MinSlice        = LimitMax + 1 + OffsetMinSlice;
MaxSlice        = LimitMax + 1 + OffsetMaxSlice;
SummarySlice    = LimitMax + 1 + OffsetSummarySlice;

                                        // the interval in which navigating
Interval        = TInterval ( LimitMin, LimitMax );

                                        // extreme slices are not really interesting, usually
Interval.SetMinMax ( LimitMin + 2, LimitMax - 2 );
                                        // set constant # of slices, as the same MRI should look the same in different resolutions
NumSlices       = min ( (int) Interval.GetLength (), 18 - 1 ); // 18 slices when including the Summary one


//DBGV2 ( Interval.GetLimitLength(), NumSlices, "Int  #slices" );

                                        // the actual selection
                                        // ignore limit min so no shift in indexes
Select          = TSelection ( LimitMax + 1 + NumSlicesAdded, OrderSorted );
Select.Reset();

IntervalToSelect();

ShowSummarySlice ( true );
}


        TSliceSelection::TSliceSelection ( const TSliceSelection &op )
{
Select              = (TSelection) op.Select;
Interval            = op.Interval;
NumSlices           = op.NumSlices;

LimitMin            = op.LimitMin;
LimitMax            = op.LimitMax;
MinSlice            = op.MinSlice;
MaxSlice            = op.MaxSlice;
SummarySlice        = op.SummarySlice;
}


TSliceSelection&  TSliceSelection::operator= ( const TSliceSelection &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;


Select              = op2.Select;
Interval            = op2.Interval;
NumSlices           = op2.NumSlices;

LimitMin            = op2.LimitMin;
LimitMax            = op2.LimitMax;
MinSlice            = op2.MinSlice;
MaxSlice            = op2.MaxSlice;
SummarySlice        = op2.SummarySlice;

return  *this;
}


//----------------------------------------------------------------------------
                                        // update the selection
void    TSliceSelection::IntervalToSelect ()
{
                                        // clear the right part
Select.Reset ( LimitMin, LimitMax );

//if ( NumSlices == 0 || Interval.GetLength () == 0 )
if      ( NumSlices == 0 )
    ;                                   // no slices
else if ( NumSlices == 1 )
    Select.Set ( Interval.GetMin () );   // 1 slice: choose the min
else                                    // spread n slices between min and max, included
    for ( int i=0; i < NumSlices; i++ )
//      Select.Set ( Round ( Interval.GetMin () + (double) i / ( NumSlices - 1 ) * ( Interval.GetTotalLength() - 1 ) ) );
        Select.Set ( Round ( Interval.GetMin () + (double) i / ( NumSlices - 1 ) * ( Interval.GetLength () - 1 ) ) );
}


//----------------------------------------------------------------------------
void    TSliceSelection::ShiftFirstSlice ( int ds )
{
                                        // shift min
if ( Interval.GetLength () == 1 )
//    Interval.SetMinMax ( Interval.GetMin () + ds, Interval.GetMax () + ds );
    Interval.AddMin ( ds, true );
else
    Interval.AddMin ( ds );
                                        // readjust length if too small
if ( Interval.GetLength () < (ULONG) NumSlices )
                                        // push toward max if possible
//  Interval.SetLength ( NumSlices, true, true );
    SetNumSlices ( Interval.GetLength () );

IntervalToSelect ();
}


void    TSliceSelection::ShiftLastSlice ( int ds )
{
                                        // shift max
if ( Interval.GetLength () == 1 )
//    Interval.SetMinMax ( Interval.GetMin () + ds, Interval.GetMax () + ds );
    Interval.AddMax ( ds, true );
else
    Interval.AddMax ( ds );
                                        // readjust length if too small
if ( Interval.GetLength () < (ULONG) NumSlices )
                                        // push toward min if possible
//  Interval.SetLength ( NumSlices, false, true );
    SetNumSlices ( Interval.GetLength () );

IntervalToSelect ();
}


//----------------------------------------------------------------------------
void    TSliceSelection::AddNumSlices ( int dn )
{
SetNumSlices ( NumSlices + dn );
}


//----------------------------------------------------------------------------
void    TSliceSelection::SetNumSlices ( int n )
{
NumSlices   = n;

if ( NumSlices < 0 )

    NumSlices = 0;

else if ( (ULONG) NumSlices > Interval.GetLength () ) {
    Interval.SetLength ( NumSlices, (bool) ( Interval.GetLength () & 1 ), true );
    NumSlices = Interval.GetLength ();
    }

IntervalToSelect ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TBaseVolumeView::TBaseVolumeView ()
{
SliceMode           = SliceModeNone;

SlicesPerX          = 20;
SlicesPerY          = 20;
SeqPerX             = 1;
SeqPerY             = 1;

for ( int i = 0; i < NumSliceMode; i++ )
    SliceModeToXYZ[ i ] = -1;
}


void    TBaseVolumeView::SetSliceMatrices ( const TVolumeDoc* mridoc, const TPointFloat& modelcenter, float modelradius )
{
                                        // transform Coronal / Transverse / Sagittal -> index of planes X, Y or Z
SliceModeToXYZ[ SliceModeNone ]     = -1; // SliceMode start from 1 to 3, so 0 is "off mode"
SliceModeToXYZ[ SliceModeCor  ]     = mridoc->GetAxisIndex ( FrontBack );
SliceModeToXYZ[ SliceModeTra  ]     = mridoc->GetAxisIndex ( UpDown    );
SliceModeToXYZ[ SliceModeSag  ]     = mridoc->GetAxisIndex ( LeftRight );


                                        // set matrices Coronal / Transverse / Sagittal
CTSMatrix[ 0 ].SetOrientation ( OrientCoronalBack   );
CTSMatrix[ 1 ].SetOrientation ( OrientTransverseTop );
CTSMatrix[ 2 ].SetOrientation ( OrientSagittalLeft  );

CTSMatrix[ 0 ]  *= mridoc->GetStandardOrientation ( LocalToPIR );
CTSMatrix[ 1 ]  *= mridoc->GetStandardOrientation ( LocalToPIR );
CTSMatrix[ 2 ]  *= mridoc->GetStandardOrientation ( LocalToPIR );


                                        // set matrices for slices (kept as copies, the user can modify them!)
SlicesRotMatrix[ 0 ]    = CTSMatrix[ 0 ];
SlicesRotMatrix[ 1 ]    = CTSMatrix[ 1 ];
SlicesRotMatrix[ 2 ]    = CTSMatrix[ 2 ];

                                        // set matrices for summary of slices
                                        // get the ratio of sizes, so that the summary slices don't overflow into the regular slices
const TBoundingBox<double>* b           = mridoc->GetBounding ();
double                      shrinksum   = b->MaxSize () ? (double) b->MinSize () / b->MaxSize () : 1;


SlicesSummaryMatrix[ 0 ].SetIdentity ();


SlicesSummaryMatrix[ 0 ].Translate ( 0, 0, - modelradius, MultiplyRight );
SlicesSummaryMatrix[ 2 ] = SlicesSummaryMatrix[ 1 ] = SlicesSummaryMatrix[ 0 ];


SlicesSummaryMatrix[ 0 ].Scale ( shrinksum, shrinksum, shrinksum, MultiplyRight );
SlicesSummaryMatrix[ 1 ].Scale ( shrinksum, shrinksum, shrinksum, MultiplyRight );
SlicesSummaryMatrix[ 2 ].Scale ( shrinksum, shrinksum, shrinksum, MultiplyRight );


SlicesSummaryMatrix[ 0 ] *= CTSMatrix[ 2 ]; // Sagittal
SlicesSummaryMatrix[ 1 ] *= CTSMatrix[ 2 ]; // Sagittal
SlicesSummaryMatrix[ 2 ] *= CTSMatrix[ 1 ]; // Transverse


SlicesSummaryMatrix[ 0 ].Translate ( - modelcenter.X, - modelcenter.Y, - modelcenter.Z, MultiplyRight );
SlicesSummaryMatrix[ 1 ].Translate ( - modelcenter.X, - modelcenter.Y, - modelcenter.Z, MultiplyRight );
SlicesSummaryMatrix[ 2 ].Translate ( - modelcenter.X, - modelcenter.Y, - modelcenter.Z, MultiplyRight );
}


void    TBaseVolumeView::GetSliceSize ( TVolumeDoc* MRIDoc, double &x, double &y ) const
{
/*
TOrientationType*       boxsides    = MRIDoc->GetBoxSides ();
TBoundingBox<double>*   b           = MRIDoc->GetBounding ();

                                        // scan each axis to recover the dimensions of Coronal / Transverse / Sagittal
                                        // also account for missing orientation, providing default sizes
if      ( SliceMode == SliceModeCor ) {
    if      ( boxsides[ XAxis ] == LeftRight )  x = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == LeftRight )  x = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == LeftRight )  x = b->GetZExtent ();
    else                                        x = b->GetZExtent ();

    if      ( boxsides[ XAxis ] == UpDown    )  y = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == UpDown    )  y = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == UpDown    )  y = b->GetZExtent ();
    else                                        y = b->GetYExtent ();
    }

else if ( SliceMode == SliceModeTra ) {
    if      ( boxsides[ XAxis ] == LeftRight )  x = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == LeftRight )  x = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == LeftRight )  x = b->GetZExtent ();
    else                                        x = b->GetZExtent ();

    if      ( boxsides[ XAxis ] == FrontBack )  y = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == FrontBack )  y = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == FrontBack )  y = b->GetZExtent ();
    else                                        y = b->GetXExtent ();
    }

else if ( SliceMode == SliceModeSag ) {
    if      ( boxsides[ XAxis ] == FrontBack )  x = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == FrontBack )  x = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == FrontBack )  x = b->GetZExtent ();
    else                                        x = b->GetXExtent ();

    if      ( boxsides[ XAxis ] == UpDown    )  y = b->GetXExtent ();
    else if ( boxsides[ YAxis ] == UpDown    )  y = b->GetYExtent ();
    else if ( boxsides[ ZAxis ] == UpDown    )  y = b->GetZExtent ();
    else                                        y = b->GetYExtent ();
    }

else // if ( SliceMode == SliceModeNone )  // if not in SliceMode, give a generalistic answer

    x   = y = b->MeanSize ();
*/

                                        // Give up being smart, use the same size for types of slices
                                        // The aim is to tile n slices at the same positions, either for transverse, coronal or sagittal
                                        // so that when switching between these types of cuts, the slices remain at the same position
                                        // which is more predictible visually, hence more comfortable

x   = y     = MRIDoc->GetBounding ()->MaxSize ();
}


bool    TBaseVolumeView::SetNumSlices ( owlwparam w, bool ShiftKey )
{
if ( ! SliceMode )
    return  false;


TSliceSelection*    slch    = &Slices[ SliceMode - 1 ];
int                 ons     = slch->GetNumSlices();


if      ( w == IDB_LESSSLICES ) {
    if ( ShiftKey )         slch->SetNumSlices ( 0 );
    else if ( ons <= 14 )   slch->AddNumSlices ( -1 );
    else                    slch->AddNumSlices ( -5 );
    }
else {
//  if ( ShiftKey )         slch->SetNumSlices ( slch->GetRange() );
                                        // set the max number of slices to the min of the 3 types of slices, so that we have at most the same number of slice for the three cuts, which is more visually pleasing
    if ( ShiftKey )         slch->SetNumSlices ( min ( Slices[ 0 ].GetRange(), Slices[ 1 ].GetRange(), Slices[ 2 ].GetRange() ) );
    else if ( ons <= 14 )   slch->AddNumSlices ( 1 );
    else                    slch->AddNumSlices ( 5 );
    }

                                        // returns true if any change has been made
return  ons != slch->GetNumSlices ();
}


void    TBaseVolumeView::MoveSlice ( TBaseDoc *BaseDoc, owlwparam w, bool ShiftKey )
{
if ( ! SliceMode )
    return;


TSliceSelection  *slch    = &Slices[ SliceMode - 1 ];

                                        // This is going to be a little tough:
                                        // According to orientation, we will move either the first or the last slice
                                        // to give the user the feeling to move only what he sees as "first" (left-most on the screen)
                                        // Directions can also reverse.
const TOrientationType* boxsides    = BaseDoc->GetBoxSides();

                                        // get the min side of current mode
int                 whichside   = IsSliceModeX () ? XMinSide : IsSliceModeY () ? YMinSide : ZMinSide;

                                        // then test if it's something "left" "top" or "fron" -> the left side
bool                movefirst   = boxsides[ whichside ] == ToFront || boxsides[ whichside ] == ToUp   || boxsides[ whichside ] == ToLeft
                               || boxsides[ whichside ] == ToXMin  || boxsides[ whichside ] == ToYMin || boxsides[ whichside ] == ToZMin;

                                        // parameter actually ask for the last slice, so reverse if needed
movefirst   = movefirst ^ ( w == IDB_LASTSLICEBWD || w == IDB_LASTSLICEFWD );


if ( movefirst ) {
                                        // also test for direction
    if      ( w == IDB_FIRSTSLICEBWD || w == IDB_LASTSLICEFWD )
        slch->ShiftFirstSlice ( ShiftKey ? -5 : -1 );
    else
        slch->ShiftFirstSlice ( ShiftKey ?  5 :  1 );
    }
else {
    if      ( w == IDB_LASTSLICEBWD || w == IDB_FIRSTSLICEFWD )
        slch->ShiftLastSlice ( ShiftKey ? -5 : -1 );
    else
        slch->ShiftLastSlice ( ShiftKey ?  5 :  1 );
    }
}


//----------------------------------------------------------------------------
void    TBaseVolumeView::InitClipPlanes ( TGLClipPlane ClipPlane[ 3 ] )
{
if ( ! SliceMode )
    return;


ClipPlane[ 0 ].SetNone ();
ClipPlane[ 1 ].SetNone ();
ClipPlane[ 2 ].SetNone ();


if      ( SliceMode == SliceModeCor 
       || SliceMode == SliceModeTra ) {

    ClipPlane[ SagittalIndex ()   ].SetBackward ();
    ClipPlane[ SagittalIndex ()   ].SetPosition ( ClipPlane[ SagittalIndex () ].GetLimitInf () + 0.45 * ( ClipPlane[ SagittalIndex () ].GetLimitSup () - ClipPlane[ SagittalIndex () ].GetLimitInf () ) );
    }
else if ( SliceMode == SliceModeSag ) {

    ClipPlane[ TransverseIndex () ].SetBackward ();
    ClipPlane[ TransverseIndex () ].SetPosition ( ( ClipPlane[ TransverseIndex () ].GetLimitInf () + ClipPlane[ TransverseIndex () ].GetLimitSup () ) / 2 );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
