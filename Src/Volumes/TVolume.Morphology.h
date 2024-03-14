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
// Implementation
//----------------------------------------------------------------------------
                                        // Dilate & Erode are very similar
template <class TypeD>
void    TVolume<TypeD>::FilterDilateErode   ( FctParams& params, bool dilate, bool showprogress )
{
double              diameter        = params ( FilterParamDiameter );
bool                erode           = ! dilate;

if ( diameter < 1 )                     // needs at least 1 voxel of element size
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, any size will do, though it is slightly better to be an odd number
TVolume<uchar>      K      ( diameter, AnySize );
TPointDouble        Ko     ( K.GetDim1 () / 2.0 - 0.5, K.GetDim2 () / 2.0 - 0.5, K.GetDim3 () / 2.0 - 0.5 );
                                                               // if even, add the distance from center to corner (1,1,1)
double              r2              = Square ( diameter / 2 ) + ( IsEven ( K.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointDouble        kp;


                                        // compute the Kernel
for ( int xk = 0; xk < K.GetDim1 (); xk++ )
for ( int yk = 0; yk < K.GetDim2 (); yk++ )
for ( int zk = 0; zk < K.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );
                                        // clip Kernel outside Euclidian norm!
    K ( xk, yk, zk )    = kp.Norm2 () <= r2;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create the border of the Kernel, to directly test neighbors
                                        // this is done by computing the real radius -> better detection
TVolume<uchar>      Kborder  ( K.GetDim1 () + 2, K.GetDim2 () + 2, K.GetDim3 () + 2 );
TPointDouble        Kbordero ( Ko.X + 1, Ko.Y + 1, Ko.Z + 1 );

double              rborder2        = Square ( ( diameter + 1 ) / 2 ) + ( IsEven ( K.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;

                                        // compute the Kernel border
for ( int xk = 0; xk < Kborder.GetDim1 (); xk++ )
for ( int yk = 0; yk < Kborder.GetDim2 (); yk++ )
for ( int zk = 0; zk < Kborder.GetDim3 (); zk++ ) {

    kp.Set ( xk - Kbordero.X, yk - Kbordero.Y, zk - Kbordero.Z );

    Kborder ( xk, yk, zk )  = fabs ( kp.Norm2 () - rborder2 ) <= 1.01
                          && ! K.GetValueChecked ( xk - 1, yk - 1, zk - 1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // give more room: Kernel size + 1 neighbor test
TPointInt           Kshift ( K.GetDim1 () + 1, K.GetDim2 () + 1, K.GetDim3 () + 1 );
TPointDouble        Koi    ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );


                                        // we need a cloned temp array, which includes a safety border,
                                        // as we're going to punch a whole Kernel into the data, so it is modified "in advance"
TVolume<TypeD>      orig ( Dim1 + Kshift.X * 2, Dim2 + Kshift.Y * 2, Dim3 + Kshift.Z * 2 );

orig.Insert ( *this, Kshift );


TypeD               newvalue        = dilate ? GetMaxValue () : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ dilate ? FilterTypeDilate : FilterTypeErode ].Text, showprogress ? orig.GetDim1 () - 2 * Kshift.X : 0 );

                                        // Progress bar goes to the roof for erode, as the first thread gets the first slice of MRI, which is often empty, so its processing ends quite quickly
OmpParallelFor

for ( int x = Kshift.X; x < orig.GetDim1 () - Kshift.X; x++ ) {

    Gauge.Next ();


    for ( int y = Kshift.Y; y < orig.GetDim2 () - Kshift.Y; y++ )
    for ( int z = Kshift.Z; z < orig.GetDim3 () - Kshift.Z; z++ ) {

                                        // Center - dilate: look for empty voxel - erode: look for non-empty voxel
        if ( dilate &&   orig ( x, y, z )
          || erode  && ! orig ( x, y, z ) )
            continue;

                                        // check Kernel border
        for ( int xki = 0, xk = x - (int) Koi.X - 1; xki < Kborder.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y - (int) Koi.Y - 1; yki < Kborder.GetDim2 (); yki++, yk++ )
        for ( int zki = 0, zk = z - (int) Koi.Z - 1; zki < Kborder.GetDim3 (); zki++, zk++ )

            if ( Kborder ( xki, yki, zki ) && ( dilate &&   orig ( xk, yk, zk )
                                             || erode  && ! orig ( xk, yk, zk ) ) )
                goto breakloops;
                                        // reaching that point means Kernel does not touch anything
        continue;


        breakloops:
                                        // punch in the FULL mask
        for ( int xki = 0, xk = x - (int) Koi.X - Kshift.X; xki < K.GetDim1 () && xk >= 0 && xk < Dim1; xki++, xk++ )
        for ( int yki = 0, yk = y - (int) Koi.Y - Kshift.Y; yki < K.GetDim2 () && yk >= 0 && yk < Dim2; yki++, yk++ )
        for ( int zki = 0, zk = z - (int) Koi.Z - Kshift.Z; zki < K.GetDim3 () && zk >= 0 && zk < Dim3; zki++, zk++ )

            if ( K ( xki, yki, zki ) )

                GetValue ( xk, yk, zk ) = newvalue;
        } // for y, z
    } // for x
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterDilate ( FctParams& params, bool showprogress )
{
FilterDilateErode ( params, true, showprogress );
}


template <class TypeD>
void    TVolume<TypeD>::FilterErode ( FctParams& params, bool showprogress )
{
FilterDilateErode ( params, false, showprogress );
}


template <class TypeD>
void    TVolume<TypeD>::FilterClose ( FctParams& params, bool showprogress )
{
FilterDilate ( params, showprogress );
FilterErode  ( params, showprogress );
}


template <class TypeD>
void    TVolume<TypeD>::FilterOpen ( FctParams& params, bool showprogress )
{
FilterErode  ( params, showprogress );
FilterDilate ( params, showprogress );
}


template <class TypeD>
void    TVolume<TypeD>::FilterMorphGradient ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
if ( params ( FilterParamDiameter ) <= 0 )
    return;


TVolume<TypeD>      eroded ( *this );

                                        // need to Erode?
if ( filtertype == FilterTypeMorphGradientInt
  || filtertype == FilterTypeMorphGradient    )

    eroded.FilterErode  ( params, showprogress );


                                        // need to Dilate?
if ( filtertype == FilterTypeMorphGradientExt
  || filtertype == FilterTypeMorphGradient    )

    FilterDilate ( params, showprogress );

                                        // dilate - erode
ClearMaskToData ( eroded );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
