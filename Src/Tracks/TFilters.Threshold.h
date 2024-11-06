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

#include    "TMaps.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // Non-temporal filter
//----------------------------------------------------------------------------
                                        // With an enum, it allows for more complex combinations of thresholds...
enum            FilterThresholdEnum
                {
                FilterClipAbove,
                FilterClipBelow,
                FilterClipAboveOrBelow,
                FilterClipAboveAndBelow,
                };


template <class TypeD>
class   TFilterThreshold    : public TFilter<TypeD>
{
public:
                    TFilterThreshold        ();
                    TFilterThreshold        ( FilterThresholdEnum how, double threshold );


    void            Reset                   ();
    void            Set                     ( FilterThresholdEnum how, double threshold1, double threshold2 = DBL_MAX );

    inline void     Apply                   ( TypeD* data, int dim );
    inline void     Apply                   ( TVector<TypeD>& map );
    inline void     Apply                   ( TMaps& maps, int nummaps = -1 );


                        TFilterThreshold    ( const TFilterThreshold& op  );
    TFilterThreshold&   operator    =       ( const TFilterThreshold& op2 );


protected:

    FilterThresholdEnum     How;
    double                  Threshold1;
    double                  Threshold2;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
        TFilterThreshold<TypeD>::TFilterThreshold ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterThreshold<TypeD>::TFilterThreshold ( FilterThresholdEnum how, double threshold )
{
Set ( how, threshold );
}


template <class TypeD>
void    TFilterThreshold<TypeD>::Reset ()
{
How                 = FilterClipAbove;
Threshold1          = 0;
Threshold2          = 0;
}


template <class TypeD>
            TFilterThreshold<TypeD>::TFilterThreshold ( const TFilterThreshold& op )
{
How                 = op.How;
Threshold1          = op.Threshold1;
Threshold2          = op.Threshold2;
}


template <class TypeD>
TFilterThreshold<TypeD>& TFilterThreshold<TypeD>::operator= ( const TFilterThreshold& op2 )
{
if ( &op2 == this )
    return  *this;


How                 = op2.How;
Threshold1          = op2.Threshold1;
Threshold2          = op2.Threshold2;


return  *this;
}


template <class TypeD>
void    TFilterThreshold<TypeD>::Set ( FilterThresholdEnum how, double threshold1, double threshold2 )
{
Reset ();

How                 = how;
Threshold1          = threshold1;
Threshold2          = threshold2;   // even if DBL_MAX, we don't care

CheckOrder  ( Threshold1, Threshold2 );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TFilterThreshold<TypeD>::Apply ( TypeD* data, int dim )
{
if ( data == 0 || dim == 0 )

    return;


switch ( How ) {

    case    FilterClipAbove:
        for ( int i = 0; i < dim; i++ )
            if ( data[ i ] > Threshold1 )
                data[ i ]  = 0;
        break;


    case    FilterClipBelow:
        for ( int i = 0; i < dim; i++ )
            if ( data[ i ] < Threshold1 )
                data[ i ]  = 0;
        break;


    case    FilterClipAboveOrBelow:
        for ( int i = 0; i < dim; i++ )
            if ( data[ i ] < Threshold1 || data[ i ] > Threshold2 )
                data[ i ]  = 0;
        break;


    case    FilterClipAboveAndBelow:
        for ( int i = 0; i < dim; i++ )
            if ( data[ i ] > Threshold1 && data[ i ] < Threshold2 )
                data[ i ]  = 0;
        break;
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TFilterThreshold<TypeD>::Apply ( TVector<TypeD>& v )
{
Apply ( (TypeD*) v, v.GetDim () );
}


//----------------------------------------------------------------------------
                                        // Wrapping calls for a TMaps variable
template <>
void    TFilterThreshold<TMapAtomType>::Apply ( TMaps& maps, int nummaps )
{
if ( maps.IsNotAllocated () )
    return;

if ( maps.CheckNumMaps ( nummaps ) <= 0 )
    return;


OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )

    Apply ( maps[ mi ], maps.GetDimension () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
