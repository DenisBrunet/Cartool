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

#include    "Math.Stats.h"
#include    "Dialogs.Input.h"
#include    "Geometry.TBoundingBox.h"
#include    "TVolumeRegions.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeRegion::TVolumeRegion ()
      : Translation ( 0 ), TIndex ( 1 )
{
Reset ();
}

                                        // Shrinking size allocation & resetting memory
        TVolumeRegion::TVolumeRegion ( int dim1, int dim2, int dim3, int shift1, int shift2, int shift3, int index )
      : TVolume<RegionType> ( dim1, dim2, dim3 ), Translation ( shift1, shift2, shift3 ), TIndex ( index )
{
Reset ();
}

                                        // Full size allocation & resetting memory
        TVolumeRegion::TVolumeRegion ( const Volume* volume, int index )
      : TVolume<RegionType> ( volume->GetDim1 (), volume->GetDim2 (), volume->GetDim3 () ), Translation ( 0 ), TIndex ( index )
{
Reset ();
}

                                        // Does NOT change: Translation, Index
void    TVolumeRegion::Reset ()
{
TVolume<RegionType>::ResetMemory ();         // clear content, don't deallocate array
                                        
NumPoints           = 0;

UpdateStats ();
}

                                        // Sets: limits, NumPoints, Center, Radius, Inertia, Compactness, (compact volume) Translation, Array3 size
void    TVolumeRegion::Set ( bool compact )
{
if ( compact )
    Compact ();

NumPoints           = GetNumSet ();

UpdateStats ();
}


        TVolumeRegion::TVolumeRegion ( const TVolumeRegion& op )
      : TVolume<RegionType> ( op ), TIndex ( op )
{
NumPoints       = op.NumPoints;
Translation     = op.Translation;

Center          = op.Center;
Radius          = op.Radius;
Inertia         = op.Inertia;
Compactness     = op.Compactness;
}


TVolumeRegion&  TVolumeRegion::operator= ( const TVolumeRegion &op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

                                        // calling parents' assignment
TVolume<RegionType>::operator= ( *( dynamic_cast<const TVolume<RegionType>*> ( &op2 ) ) );  // force to TVolume and NOT TVolumeRegion
TIndex             ::operator= ( op2 );


NumPoints       = op2.NumPoints;
Translation     = op2.Translation;

Center          = op2.Center;
Radius          = op2.Radius;
Inertia         = op2.Inertia;
Compactness     = op2.Compactness;

return  *this;
}


//----------------------------------------------------------------------------
void    TVolumeRegion::Compact ()
{
                                        // update bounding, integer version
TBoundingBox<int>   newbounding     = TBoundingBox<int> ( *this, false, DefaultRegionBackground /*BackgroundValue*/ );

                                        // same size?
if ( newbounding.GetXExtent () == Dim1 
  && newbounding.GetYExtent () == Dim2 
  && newbounding.GetZExtent () == Dim3 )
                                        // already compacted!
    return;
                                        // need to test for null size too?

                                        // create a smaller TVolumeRegion
TVolumeRegion       newvolume ( newbounding.GetXExtent (),  newbounding.GetYExtent (),  newbounding.GetZExtent (),
                                newbounding.XMin (),        newbounding.YMin (),        newbounding.ZMin (),
                                Index                                                                               );
                                        // plug the bigger one into the smaller one
newvolume.Crop ( *this, newvolume.Translation );
                                        // smaller volume replaces this
*this   = newvolume;
                                        // update variables
Set ( false );
}


//----------------------------------------------------------------------------
/*                                        // Converting from a volume
TVolumeRegion&  TVolumeRegion::operator= ( const Volume& op2 )
{
Reset ();

if ( ! (bool) op2 )
    return  *this;

                                        // manually copying and converting
Resize ( op2.GetDim1 (), op2.GetDim2 (), op2.GetDim3 () );

for ( int i = 0; i < LinearDim; i++ )
    Array[ i ]  = op2[ i ] > DefaultRegionBackground;

                                        // no translation
Translation.Reset ();
                                        // guessing index
Index           = AtLeast ( 1, Round ( op2.GetMaxValue () ) );
                                        // update variables
Set ( true );

return  *this;
}


TVolumeRegion&  TVolumeRegion::operator+= ( TVolumeRegion &op2 )
{
Add ();

UpdateStats ();

// Index remains the same

return  *this;
}
*/

//----------------------------------------------------------------------------
void        TVolumeRegion::CopyTo ( Volume& array, bool clear )     const
{
if ( clear )
    array.ResetMemory ();

if ( array.IsNotAllocated () )
    return;


int                 x,  y,  z;

for ( int i = 0; i < LinearDim; i++ ) {

    if ( ! GetValue ( i ) )
        continue;

    LinearIndexToXYZ ( i, x, y, z );
    CoordinatesToAbs    ( x, y, z );

    if ( array.WithinBoundary ( x, y, z ) )
        array ( x, y, z )   = (MriType) Index;
    }
}


//----------------------------------------------------------------------------
double      TVolumeRegion::ComparedTo ( const TVolumeRegion &region )
{
double              rn              = 1 - 0.5 * RelativeDifference ( NumPoints,   region.GetNumPoints () );
double              rd              = 1 - 0.5 * RelativeDifference ( Radius,      region.Radius          );
double              ri              = 1 - 0.5 * RelativeDifference ( Inertia,     region.Inertia         );
double              rc              = 1 - 0.5 * RelativeDifference ( Compactness, region.Compactness     );

                                        // !give more weight to # of points!
double              r               = ( 3 * rn + rd + ri + rc ) / 6;


return  r;
}


//----------------------------------------------------------------------------
void    TVolumeRegion::ComputeDataStats ( const Volume& data, TEasyStats &stat )
{
stat.Reset ();

if ( data.IsNotAllocated () || IsNotAllocated () )
    return;


int                 x,  y,  z;

for ( int i = 0; i < LinearDim; i++ ) {

    if ( ! GetValue ( i ) )
        continue;

    LinearIndexToXYZ ( i, x, y, z );
    CoordinatesToAbs    ( x, y, z );

    if ( data.WithinBoundary ( x, y, z ) )
        stat.Add ( data ( x, y, z ), ThreadSafetyIgnore );
    }
}


//----------------------------------------------------------------------------
void    TVolumeRegion::UpdateStats ()
{
Center.Reset ();
Radius              = 0;
Inertia             = 0;
Compactness         = 0;

if ( IsEmpty () )
    return;


TEasyStats          stat;
TPointFloat         p;
int                 x,  y,  z;

for ( int i = 0; i < LinearDim; i++ ) {

    if ( ! GetValue ( i ) )
        continue;

    LinearIndexToXYZ ( i, x, y, z );
    CoordinatesToAbs    ( x, y, z );

    Center     += TPointFloat ( x, y, z );
    }

Center /= GetNumPoints ();


for ( int i = 0; i < LinearDim; i++ ) {

    if ( ! GetValue ( i ) )
        continue;

    LinearIndexToXYZ ( i, x, y, z );
    CoordinatesToAbs    ( x, y, z );

    p   = TPointFloat ( x, y, z ) - Center;
    stat.Add ( p.Norm (), ThreadSafetyIgnore );
    }


Radius      = stat.Average ();
Inertia     = stat.SD ();
Compactness = GetNumPoints () / ( Radius ? Power ( Radius, 3 ) : 1 );
}


//----------------------------------------------------------------------------
                                        // Call UpdateStats beforehand
void    TVolumeRegion::Show ( char *title ) const
{
char                buff[ 256 ];

sprintf ( buff, "Index: %0d\nNumber of Points: %0d\nCenter: %.1f, %.1f, %.1f\nRadius: %.2f\nInertia: %.2f\nCompactness: %.2f",
                 Index,
                 GetNumPoints (),
                 Center.X, Center.Y, Center.Z,
                 Radius, Inertia, Compactness );

ShowMessage ( buff, StringIsEmpty ( title ) ? "Region" : title );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeRegions::TVolumeRegions ()
{
Reset ();
}


        TVolumeRegions::~TVolumeRegions ()
{
Reset ();
}


void    TVolumeRegions::Reset ()
{
                                        // delete content & structure
Group.Reset ( true );
}


void    TVolumeRegions::Add ( TVolumeRegion* region /*, bool copy*/ )
{
/*
if ( copy ) {
                                        // allocate a new TVolumeRegion and copy content
    TVolumeRegion*      newregion  = new TVolumeRegion ();

    TListIterator<TPointFloat>  iterator;

    foreachin ( *region, iterator )
        newregion->Add ( iterator () );

    Group.Append ( newregion );
    }
else*/
                                        // compacting should have been done before(?)
    region->Set ( /*true*/ false );

    Group.Append ( region );
}


//----------------------------------------------------------------------------
void    TVolumeRegions::UpdateStats ()
{
for ( int i = 0; i < NumRegions (); i++ )

    Group[ i ]->UpdateStats ();
}


//----------------------------------------------------------------------------
                                        // Call UpdateStats beforehand
void    TVolumeRegions::Show ( char *title )   const
{
if ( IsEmpty () ) {
    ShowMessage ( "- Empty -", StringIsEmpty ( title ) ? "Region" : title );
    return;
    }


for ( int i = 0; i < NumRegions (); i++ )

    Group[ i ]->Show ( StringIsEmpty ( title ) ? "Region" : title );
}


//----------------------------------------------------------------------------
                                        // Comparison functions used for sorting in decreasing order (use some lambda later on?)
static bool     GreaterCount        ( const TVolumeRegion* va, const TVolumeRegion* vb )    { return va->GetNumPoints () > vb->GetNumPoints ();  }
static bool     GreaterCompact      ( const TVolumeRegion* va, const TVolumeRegion* vb )    { return va->Compactness     > vb->Compactness;      }
static bool     GreaterCompactCount ( const TVolumeRegion* va, const TVolumeRegion* vb )    { return RoundTo ( va->GetCompactCount (), 10-3 ) > RoundTo ( vb->GetCompactCount (), 10-3 );   }


void    TVolumeRegions::Sort ( VolumeRegionsSort how )
{
if      ( how == SortRegionsCount        )  Sort ( 0, NumRegions () - 1, GreaterCount        );
else if ( how == SortRegionsCompact      )  Sort ( 0, NumRegions () - 1, GreaterCompact      );
else if ( how == SortRegionsCompactCount )  Sort ( 0, NumRegions () - 1, GreaterCompactCount );

Group.UpdateIndexes ( true );
}


void    TVolumeRegions::Sort  ( int l, int r, bool (*f) ( const TVolumeRegion* va, const TVolumeRegion* vb ) )
{
if ( r <= l )   return;


int                     i           = l;
int                     j           = r;
const TVolumeRegion*    v           = Group[ ( l + r ) / 2 ];


do {
    while ( (*f) ( Group[ i ],  v          ) )      i++;
    while ( (*f) ( v,           Group[ j ] ) )      j--;

    if ( i <= j )
        Permutate ( Group.GetAtom ( i++ )->To, Group.GetAtom ( j-- )->To );   // content remain in place, only the pointers are permutated

    } while ( i <= j );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
